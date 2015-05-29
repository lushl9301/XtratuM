/*
 * $FILE: irqs.c
 *
 * IRQS' code
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
#include <bitwise.h>
#include <irqs.h>
#include <kdevice.h>
#include <kthread.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <stdc.h>
#include <arch/segments.h>

#ifdef CONFIG_SMI_DISABLE
#include <arch/io.h>
#include <arch/pci.h>
#include <arch/smi.h>
#endif
#ifdef CONFIG_APIC
#include <arch/apic.h>
#else
#include <arch/pic.h>
#endif

xm_u32_t x86HwIrqsMask[CONFIG_NO_CPUS]={[0 ... (CONFIG_NO_CPUS-1)]=0xffffffff};

#ifdef CONFIG_VERBOSE_TRAP
xm_s8_t *trap2Str[]={
    __STR(DIVIDE_ERROR), // 0
    __STR(RESERVED_TRAP_1), // 1
    __STR(NMI_INTERUPT), // 2
    __STR(BREAKPOINT), // 3
    __STR(OVERFLOW), // 4
    __STR(BOUND_RANGE_EXCEEDED), // 5
    __STR(UNDEFINED_OPCODE), // 6
    __STR(DEVICE_NOT_AVAILABLE), // 7
    __STR(DOUBLE_FAULT), // 8
    __STR(COPROCESSOR_SEGMENT_OVERRUN), // 9
    __STR(INVALID_TSS), // 10
    __STR(SEGMENT_NOT_PRESENT), // 11
    __STR(STACK_SEGMENT_FAULT), // 12
    __STR(GENERAL_PROTECTION), // 13
    __STR(PAGE_FAULT), // 14
    __STR(RESERVED_TRAP_15), // 15
    __STR(X87_FPU_ERROR),// 16
    __STR(ALIGNMENT_CHECK), // 17
    __STR(MACHINE_CHECK), // 18
    __STR(SIMD_EXCEPTION), // 19
};
#endif

#ifdef CONFIG_SMP
RESERVE_HWIRQ(HALT_ALL_IPI_IRQ);
RESERVE_HWIRQ(SCHED_PENDING_IPI_IRQ);

static void SmpHaltAllHndl(cpuCtxt_t *ctxt, void *data) {
    HaltSystem();
}

static void SmpSchedPendingIPIHndl(cpuCtxt_t *ctxt, void *data) {
    localSched_t *sched=GET_LOCAL_SCHED();
    SetSchedPending();
    sched->data->cyclic.flags|=RESCHED_ENABLED;
}
#endif

xm_s32_t ArchTrapIsSysCtxt(cpuCtxt_t *ctxt) {
    return 1;
}

static inline void _SetIrqGate(xm_s32_t e, void *hndl, xm_u32_t dpl) {
    idtTab[e].segSel=CS_SEL;
    idtTab[e].offset15_0=(xmAddress_t)hndl&0xffff;
    idtTab[e].offset31_16=((xmAddress_t)hndl>>16)&0xffff;
    idtTab[e].access=0x8e|(dpl&0x3)<<5;
}

static inline void _SetTrapGate(xm_s32_t e, void *hndl, xm_u32_t dpl) {
    idtTab[e].segSel=CS_SEL;
    idtTab[e].offset15_0=(xmAddress_t)hndl&0xffff;
    idtTab[e].offset31_16=((xmAddress_t)hndl>>16)&0xffff;
    idtTab[e].access=0x8f|(dpl&0x3)<<5;
}

void SetupX86Idt(void) {
    extern void (*hwIrqHndlTab[0])(void);
    extern void (*trapHndlTab[0])(void);
    extern void UnexpectedIrq(void);
    xm_s32_t irqNr;
    
    for (irqNr=0; irqNr<CONFIG_NO_HWIRQS; irqNr++) {
        ASSERT(hwIrqHndlTab[irqNr]);
        _SetIrqGate(irqNr+FIRST_EXTERNAL_VECTOR, hwIrqHndlTab[irqNr], 0);
    }

    _SetTrapGate(0, trapHndlTab[0], 0);
    _SetTrapGate(1, trapHndlTab[1], 0);
    _SetIrqGate(2, trapHndlTab[2], 0);

    _SetTrapGate(3, trapHndlTab[3], 3);
    _SetTrapGate(4, trapHndlTab[4], 3);
    _SetTrapGate(5, trapHndlTab[5], 3);

    _SetTrapGate(6, trapHndlTab[6], 0);
    _SetIrqGate(7, trapHndlTab[7], 0);
    _SetTrapGate(8, trapHndlTab[8], 0);
    _SetTrapGate(9, trapHndlTab[9], 0);
    _SetTrapGate(10, trapHndlTab[10], 0);
    _SetTrapGate(11, trapHndlTab[11], 0);
    _SetTrapGate(12, trapHndlTab[12], 0);
    _SetIrqGate(13, trapHndlTab[13], 0);
    _SetIrqGate(14, trapHndlTab[14], 0);
    _SetTrapGate(15, trapHndlTab[15], 0);
    _SetTrapGate(16, trapHndlTab[16], 0);
    _SetTrapGate(17, trapHndlTab[17], 0);
    _SetTrapGate(18, trapHndlTab[18], 0);
    _SetTrapGate(19, trapHndlTab[19], 0);
}

static xm_s32_t X86TrapDeviceNotAvailable(cpuCtxt_t *ctxt, xm_u16_t *hmEvent) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_u32_t cr0;

    if (sched&&sched->cKThread->ctrl.g) {
        cr0 = sched->cKThread->ctrl.g->partCtrlTab->arch.cr0;
        cr0 = (cr0&~_CR0_TS)|(SaveCr0()&_CR0_TS);
        sched->cKThread->ctrl.g->partCtrlTab->arch.cr0=cr0;
    }

    return 0;
}

static xm_s32_t X86TrapPageFault(cpuCtxt_t *ctxt, xm_u16_t *hmEvent) {
    xmAddress_t faultAddress=SaveCr2();
    localSched_t *sched=GET_LOCAL_SCHED();

    if (sched&&sched->cKThread->ctrl.g)
        sched->cKThread->ctrl.g->partCtrlTab->arch.cr2=faultAddress;
    if (faultAddress>=CONFIG_XM_OFFSET) {
        *hmEvent=XM_HM_EV_MEM_PROTECTION;
    }
    return 0;
}

void ArchSetupIrqs(void) {
    SetTrapHandler(PAGE_FAULT, X86TrapPageFault);
    SetTrapHandler(DEVICE_NOT_AVAILABLE, X86TrapDeviceNotAvailable);
#ifdef CONFIG_SMP
    SetIrqHandler(HALT_ALL_IPI_IRQ, SmpHaltAllHndl, 0);
    SetIrqHandler(SCHED_PENDING_IPI_IRQ, SmpSchedPendingIPIHndl, 0);
#endif
#ifdef CONFIG_SMI_DISABLE
	SmiDisable();
#endif
}

xmAddress_t IrqVector2Address(xm_s32_t vector) {
    return 0;
}

static inline xm_s32_t TestSp(xmAddress_t *sp, xm_u32_t size) {
    xm_s32_t ret;
    SetWp();
    ret=AsmRWCheck(((xmAddress_t)sp)-size, size, 1);
    ClearWp();
    return ret;
}

#define ERRCODE_TAB 0x27d00UL
void FixStack(cpuCtxt_t *ctxt, partitionControlTable_t *partCtrlTab, xm_s32_t irqNr, xm_s32_t vector, xm_s32_t trap) {
    xmWord_t ip, iFlags;
    localSched_t *sched=GET_LOCAL_SCHED();
    struct x86Gate *idtEntry;
    xmAddress_t *sp=0;
    xm_u16_t cs;

    if (vector>=partCtrlTab->arch.maxIdtVec) {
        PartitionPanic(ctxt, "error emulating IRQ (%d) bad vector (%d)", irqNr, vector);
    }
    idtEntry = (struct x86Gate *)partCtrlTab->arch.idtr.linearBase + vector;
    cs = idtEntry->segSel;
    ip = (idtEntry->offset31_16<<16)|idtEntry->offset15_0;
    if ((ip>=CONFIG_XM_OFFSET)||!(cs&0x3)) {
        PartitionPanic(ctxt, "error emulating IRQ (%d) CS:IP (0x%x:0x%x) invalid in IDT", irqNr, cs, ip);
    }
    iFlags=partCtrlTab->iFlags;
    if (!(idtEntry->access&0x1)) {
        partCtrlTab->iFlags &= ~_CPU_FLAG_IF;
    }

    if ((ctxt->cs&0x3)==(cs&0x3)) {
        sp=(xmAddress_t *)ctxt->sp;
        if (TestSp(sp, 16)<0)
            PartitionPanic(ctxt, "error emulating IRQ (%d) bad stack pointer 0x%x", irqNr, sp);
    } else {
        xm_u16_t ss=0;
        switch(cs&0x3) {
        case 0x1:
            ss=sched->cKThread->ctrl.g->kArch.tss.t.ss1;
            sp=(xmAddress_t *)sched->cKThread->ctrl.g->kArch.tss.t.sp1;
            break;
        case 0x2:
            ss=sched->cKThread->ctrl.g->kArch.tss.t.ss2;
            sp=(xmAddress_t *)sched->cKThread->ctrl.g->kArch.tss.t.sp2;
            break;
        default:
            PartitionPanic(ctxt, "error emulating IRQ (%d) CS (0x%x) corrupted", irqNr, cs);
            break;
        }
        if (!ss || !sp) {
            PartitionPanic(ctxt, "error emulating IRQ (%d) SS:ESP (0x%x:0x%x) invalid", irqNr, ss, sp);
        }
        if (TestSp(sp, 24)<0)
            PartitionPanic(ctxt, "error emulating IRQ (%d) bad stack pointer 0x%x", irqNr, sp);
        *(--sp)=ctxt->ss;
        *(--sp)=ctxt->sp;
        ctxt->ss=ss;
    }
    *(--sp)=(ctxt->flags&~_CPU_FLAG_IF)|iFlags;
    *(--sp)=ctxt->cs;
    *(--sp)=ctxt->ip;

    if (trap && ((1<<irqNr)&ERRCODE_TAB))
        *(--sp)=ctxt->eCode;

    ctxt->sp=(xmWord_t)sp;
    ctxt->cs=cs;
    ctxt->ip=ip;
}

xm_u32_t HwIrqGetMask(void) {
    return x86HwIrqsMask[GET_CPU_ID()];
}
