/*
 * $FILE: hypercalls.c
 *
 * XM's hypercalls
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <gaccess.h>
#include <guest.h>
#include <hypercalls.h>
#include <physmm.h>
#include <sched.h>
#include <stdc.h>

#define EMPTY_SEG 0
#define CODE_DATA_SEG 1
#define TSS_SEG 2
#define LDT_SEG 3

static inline xm_s32_t IsGdtDescValid(struct x86Desc *desc, xm_u32_t *type)
{
    xmAddress_t base, limit;

    if (!(desc->low & X86DESC_LOW_P)) {
        *type = CODE_DATA_SEG;
        return 1;
    }
    *type = EMPTY_SEG;
    limit = GetDescLimit(desc);
    base = (xm_u32_t) GetDescBase(desc);
    if ((limit + base) > CONFIG_XM_OFFSET) {
        PWARN("[%d] GDT desc (0x%x:0x%x) limit too large\n", KID2PARTID(GET_LOCAL_SCHED()->cKThread->ctrl.g->id), desc->high, desc->low);
        return 0;
    }
    if (desc->low & X86DESC_LOW_S) {    /* Code/Data segment */
        *type = CODE_DATA_SEG;
        if (!((desc->low >> X86DESC_LOW_DPL_POS) & 0x3)) {
            PWARN("DESCR (%x:%x) bad permissions\n", desc->high, desc->low);
            return 0;
        }
        desc->low |= (1 << X86DESC_LOW_TYPE_POS);
    } else {                            /* System segment */
        switch ((desc->low >> X86DESC_LOW_TYPE_POS) & 0xF) {
            case 0x9:
            case 0xb:
                *type = TSS_SEG;
                break;
            default:
                return 0;
        }

        return 1;
    }
    return 1;
}

__hypercall xm_s32_t X86LoadCr0Sys(xmWord_t val) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if ((val&(_CR0_PG|_CR0_PE))!=(_CR0_PG|_CR0_PE))
        return XM_OP_NOT_ALLOWED;
    if (!AreKThreadFlagsSet(sched->cKThread, KTHREAD_FP_F))
        val|=_CR0_EM;
    sched->cKThread->ctrl.g->partCtrlTab->arch.cr0=val|_CR0_ET;
    val&=~(_CR0_WP|_CR0_CD|_CR0_NW);
    sched->cKThread->ctrl.g->kArch.cr0=val;
    LoadCr0(val);

    return XM_OK;
}

__hypercall xm_s32_t X86LoadCr3Sys(xmWord_t val) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *newCr3Page, *oldCr3Page;
    xmWord_t oldCr3;

    oldCr3=SaveCr3();
    if (oldCr3==val) {
        FlushTlb();
    } else {
        if (!(oldCr3Page=PmmFindPage(oldCr3, GetPartition(sched->cKThread), 0)))
            return XM_INVALID_PARAM;
       
        if (!(newCr3Page=PmmFindPage(val, GetPartition(sched->cKThread), 0)))
            return XM_INVALID_PARAM;
        
        if (newCr3Page->type!=PPAG_PTDL1) {
            PWARN("Page %x is not PTDL1\n", val&PAGE_MASK);
            return XM_INVALID_PARAM;
        }
        PPagDecCounter(oldCr3Page);
        PPagIncCounter(newCr3Page);
        sched->cKThread->ctrl.g->kArch.ptdL1=val;
        sched->cKThread->ctrl.g->partCtrlTab->arch.cr3=val;
        LoadCr3(val);
    }

    return XM_OK;
}

__hypercall xm_s32_t X86LoadCr4Sys(xmWord_t val) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if (val&_CR4_PAE)
        return XM_OP_NOT_ALLOWED;
    sched->cKThread->ctrl.g->partCtrlTab->arch.cr4=val;
    val|=_CR4_PSE|_CR4_PGE;
    sched->cKThread->ctrl.g->kArch.cr4=val;
    LoadCr4(sched->cKThread->ctrl.g->kArch.cr4);

    return XM_OK;
}

__hypercall xm_s32_t X86LoadTss32Sys(struct x86Tss *__gParam t) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if (CheckGParam(t, sizeof(struct x86Tss), 4, PFLAG_NOT_NULL)<0)
	return XM_INVALID_PARAM;

    if (!(t->ss1&0x3)||((t->ss1>>3)>=(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES)))
        return XM_INVALID_PARAM;

    if (t->sp1>=CONFIG_XM_OFFSET)
        return XM_INVALID_PARAM;

    if (!(t->ss2&0x3)||((t->ss2>>3)>=(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES)))
        return XM_INVALID_PARAM;

    if (t->sp2>=CONFIG_XM_OFFSET)
        return XM_INVALID_PARAM;

    sched->cKThread->ctrl.g->kArch.tss.t.ss1=t->ss1;
    sched->cKThread->ctrl.g->kArch.tss.t.sp1=t->sp1;
    sched->cKThread->ctrl.g->kArch.tss.t.ss2=t->ss2;
    sched->cKThread->ctrl.g->kArch.tss.t.sp2=t->sp2;

    sched->cKThread->ctrl.g->partCtrlTab->arch.cr0|=_CR0_TS;
    sched->cKThread->ctrl.g->kArch.cr0|=_CR0_TS;
    LoadCr0(sched->cKThread->ctrl.g->kArch.cr0);

    return XM_OK;
}

__hypercall xm_s32_t X86LoadGdtSys(struct x86DescReg *__gParam desc) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t gdtNoEntries, e;
    struct x86Desc *gdt;
    xm_u32_t type;

    if (CheckGParam(desc, sizeof(struct x86DescReg), 4, PFLAG_NOT_NULL)<0)
	return XM_INVALID_PARAM;
    
    gdtNoEntries=(desc->limit+1)/sizeof(struct x86Desc);
    if (gdtNoEntries>CONFIG_PARTITION_NO_GDT_ENTRIES) 
	return XM_INVALID_PARAM;

    sched->cKThread->ctrl.g->partCtrlTab->arch.gdtr=*desc;    
    memset(sched->cKThread->ctrl.g->kArch.gdtTab, 0, CONFIG_PARTITION_NO_GDT_ENTRIES*sizeof(struct x86Desc));
    for (e=0, gdt=(struct x86Desc *)desc->linearBase; e<gdtNoEntries; e++)
	if (IsGdtDescValid(&gdt[e], &type)&&(type==CODE_DATA_SEG)) {
	    sched->cKThread->ctrl.g->kArch.gdtTab[e]=gdt[e];
	    gdtTab[e]=gdt[e];
	}

    return XM_OK;
}

__hypercall xm_s32_t X86LoadIdtrSys(struct x86DescReg *__gParam desc) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if (CheckGParam(desc, sizeof(struct x86DescReg), 4, PFLAG_NOT_NULL)<0)
	return XM_INVALID_PARAM;
    
    sched->cKThread->ctrl.g->partCtrlTab->arch.idtr=*desc;
    sched->cKThread->ctrl.g->partCtrlTab->arch.maxIdtVec = (desc->limit+1)/sizeof(struct x86Gate);

    return XM_OK;
}

__hypercall xm_s32_t X86UpdateSsSpSys(xmWord_t ss, xmWord_t sp, xm_u32_t level) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if (!(ss&0x3)||
        ((ss>>3)>=(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES))) 
        return XM_INVALID_PARAM;

    if (sp>=CONFIG_XM_OFFSET) return XM_INVALID_PARAM;

    switch(level) {
    case 1:
        sched->cKThread->ctrl.g->kArch.tss.t.ss1=ss;
        sched->cKThread->ctrl.g->kArch.tss.t.sp1=sp;
        break;
    case 2:
        sched->cKThread->ctrl.g->kArch.tss.t.ss2=ss;
        sched->cKThread->ctrl.g->kArch.tss.t.sp2=sp;
        break;
    default:
        return XM_INVALID_PARAM;
    }

    return XM_OK;
}

__hypercall xm_s32_t X86UpdateGdtSys(xm_s32_t entry, struct x86Desc *__gParam gdt) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_u32_t type;

    if (CheckGParam(gdt, sizeof(struct x86Desc), 4, PFLAG_NOT_NULL)<0)
	return XM_INVALID_PARAM;

    if (entry>=CONFIG_PARTITION_NO_GDT_ENTRIES) 
	return XM_INVALID_PARAM;

    if (IsGdtDescValid(gdt, &type)&&(type==CODE_DATA_SEG)) {
	sched->cKThread->ctrl.g->kArch.gdtTab[entry]=*gdt;
    } else
	return XM_INVALID_PARAM;

    return XM_OK;
}

static inline xm_s32_t IsGateDescValid(struct x86Gate *desc)
{
    xmWord_t base, seg;

    if ((desc->low0 & 0x1fe0) == 0xf00) {   /* Only trap gates supported */
        base = (desc->low0 & 0xFFFF0000) | (desc->high0 & 0xFFFF);
        if (base >= CONFIG_XM_OFFSET) {
            kprintf("[GateDesc] Base (0x%x) > XM_OFFSET\n", base);
            return 0;
        }
        seg = desc->high0 >> 16;
        if (!(seg & 0x3)) {
            kprintf("[GateDesc] ring (%d) can't be zero\n", seg & 0x3);
            return 0;
        }
        return 1;
    }

    return 0;
}

__hypercall xm_s32_t X86UpdateIdtSys(xm_s32_t entry, struct x86Gate *__gParam desc) {
    localSched_t *sched=GET_LOCAL_SCHED();

    if (CheckGParam(desc, sizeof(struct x86Gate), 4, PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;
    if (entry<FIRST_USER_IRQ_VECTOR||entry>=IDT_ENTRIES)
        return XM_INVALID_PARAM;

    if ((desc->low0&X86GATE_LOW0_P)&&!IsGateDescValid(desc))
        return XM_INVALID_PARAM;

    sched->cKThread->ctrl.g->kArch.idtTab[entry]=*desc;

    return XM_OK;
}

__hypercall void X86SetIFSys(void) {
    localSched_t *sched=GET_LOCAL_SCHED();
    sched->cKThread->ctrl.g->partCtrlTab->iFlags|=_CPU_FLAG_IF;
}

__hypercall void X86ClearIFSys(void) {
    localSched_t *sched=GET_LOCAL_SCHED();
    sched->cKThread->ctrl.g->partCtrlTab->iFlags&=~_CPU_FLAG_IF;
}

__hypercall void X86IRetSys(void) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct x86IrqStackFrame *stackFrame;

    cpuCtxt_t *ctxt=sched->cKThread->ctrl.irqCpuCtxt;
    stackFrame=(struct x86IrqStackFrame *)(ctxt->sp);

    if ((stackFrame->cs&0x3)!=(ctxt->cs&0x3)) {
        ctxt->sp=stackFrame->sp;
        ctxt->ss=stackFrame->ss;
    } else {
        ctxt->sp=((xmAddress_t)stackFrame)+sizeof(xmWord_t)*3;
    }
    ctxt->flags=stackFrame->flags;
    ctxt->ip=stackFrame->ip;
    ctxt->cs=stackFrame->cs;
    sched->cKThread->ctrl.g->partCtrlTab->iFlags=ctxt->flags&_CPU_FLAG_IF;
    ctxt->flags|=_CPU_FLAG_IF;
    ctxt->flags&=~_CPU_FLAG_IOPL;
}

__hypercall xm_s32_t OverrideTrapHndlSys(xm_s32_t entry, struct trapHandler *__gParam handler) {
    return XM_OK;
}

// Hypercall table
HYPERCALLR_TAB(MulticallSys, 0);  // 0
HYPERCALLR_TAB(HaltPartitionSys, 1); // 1
HYPERCALLR_TAB(SuspendPartitionSys, 1); // 2
HYPERCALLR_TAB(ResumePartitionSys, 1); // 3
HYPERCALLR_TAB(ResetPartitionSys, 1); // 4
HYPERCALLR_TAB(ShutdownPartitionSys, 1); // 5
HYPERCALLR_TAB(HaltSystemSys, 0); // 6
HYPERCALLR_TAB(ResetSystemSys, 1); // 7
HYPERCALLR_TAB(IdleSelfSys, 0); // 8

HYPERCALLR_TAB(GetTimeSys, 2); // 9
HYPERCALLR_TAB(SetTimerSys, 3); // 10
HYPERCALLR_TAB(ReadObjectSys, 4); // 11
HYPERCALLR_TAB(WriteObjectSys, 4); // 12
HYPERCALLR_TAB(SeekObjectSys, 3); // 13
HYPERCALLR_TAB(CtrlObjectSys, 3); // 14

HYPERCALLR_TAB(ClearIrqMaskSys, 2); // 15
HYPERCALLR_TAB(SetIrqMaskSys, 2); // 16
HYPERCALLR_TAB(ForceIrqsSys, 2); // 17
HYPERCALLR_TAB(ClearIrqsSys, 2); // 18
HYPERCALLR_TAB(RouteIrqSys, 3); // 19

HYPERCALLR_TAB(UpdatePage32Sys, 2); // 20
HYPERCALLR_TAB(SetPageTypeSys, 2); // 21
HYPERCALLR_TAB(InvldTlbSys, 1); // 22
HYPERCALLR_TAB(RaiseIpviSys, 1); // 23
HYPERCALLR_TAB(RaisePartitionIpviSys, 2); // 24

HYPERCALLR_TAB(OverrideTrapHndlSys, 2); // 25

HYPERCALLR_TAB(SwitchSchedPlanSys, 2); // 26
HYPERCALLR_TAB(GetGidByNameSys, 2); // 27
HYPERCALLR_TAB(ResetVCpuSys, 4); // 28
HYPERCALLR_TAB(HaltVCpuSys, 1); // 29
HYPERCALLR_TAB(SuspendVCpuSys, 1); // 30
HYPERCALLR_TAB(ResumeVCpuSys, 1); // 31
HYPERCALLR_TAB(GetVCpuIdSys, 0); // 32

HYPERCALLR_TAB(X86LoadCr0Sys, 1); // 33
HYPERCALLR_TAB(X86LoadCr3Sys, 1); // 34
HYPERCALLR_TAB(X86LoadCr4Sys, 1); // 35
HYPERCALLR_TAB(X86LoadTss32Sys, 1); // 36
HYPERCALLR_TAB(X86LoadGdtSys, 1); // 37
HYPERCALLR_TAB(X86LoadIdtrSys, 1); // 38
HYPERCALLR_TAB(X86UpdateSsSpSys, 3); // 39
HYPERCALLR_TAB(X86UpdateGdtSys, 2); // 40
HYPERCALLR_TAB(X86UpdateIdtSys, 2); // 41
HYPERCALL_TAB(X86SetIFSys, 0); // 42
HYPERCALL_TAB(X86ClearIFSys, 0); // 43

