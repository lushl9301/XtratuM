/*
 * $FILE: kthread.c
 *
 * Kernel, Guest or L0 context (ARCH dependent part)
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
#include <rsvmem.h>
#include <gaccess.h>
#include <kthread.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <vmmap.h>
#include <arch/segments.h>
#include <arch/xm_def.h>

void SwitchKThreadArchPre(kThread_t *new, kThread_t *current) {
    if (current->ctrl.g) {
        current->ctrl.g->kArch.ptdL1=SaveCr3();
        current->ctrl.g->kArch.cr0=SaveCr0();
        if (!(current->ctrl.g->kArch.cr0&_CR0_EM)) {
            if (SaveCr0()&_CR0_TS) {
                CLTS();
            }
            SaveFpuState(current->ctrl.g->kArch.fpCtxt);
        }
    }

    if (new->ctrl.g) {
#ifdef CONFIG_VCPU_MIGRATION
        new->ctrl.g->kArch.gdtTab[PERCPU_SEL>>3] = gdtTab[GDT_ENTRY(GET_CPU_ID(), PERCPU_SEL)];
#endif
        LoadGdt(new->ctrl.g->kArch.gdtr);
        LoadIdt(new->ctrl.g->kArch.idtr);
        if (new->ctrl.g->kArch.ptdL1) {
            LoadCr3(new->ctrl.g->kArch.ptdL1);
        }
        TssClearBusy(&new->ctrl.g->kArch.gdtr, TSS_SEL);
        LoadTr(TSS_SEL);
    } else {
        LoadXmPageTable();
    }
    LoadCr0(_CR0_PE|_CR0_PG);
}

void SwitchKThreadArchPost(kThread_t *current) {
    if (current->ctrl.g) {
        if (!(current->ctrl.g->kArch.cr0&_CR0_EM))
            RestoreFpuState(current->ctrl.g->kArch.fpCtxt);
        LoadCr0(current->ctrl.g->kArch.cr0);
    }
}

extern void SetupKStack(kThread_t *k, void *StartUp, xmAddress_t entryPoint) {
    k->ctrl.kStack=(xm_u32_t *)&k->kStack[CONFIG_KSTACK_SIZE];
    *--(k->ctrl.kStack)=entryPoint;
    *--(k->ctrl.kStack)=0;
    *--(k->ctrl.kStack)=(xmAddress_t)StartUp;
}

void KThreadArchInit(kThread_t *k) {
    if (AreKThreadFlagsSet(k, KTHREAD_FP_F)) {
        LoadCr0(SaveCr0()&(~(_CR0_EM|_CR0_TS)));
        FNINIT();
        SaveFpuState(k->ctrl.g->kArch.fpCtxt);
    }

    k->ctrl.g->kArch.tss.t.ss0=DS_SEL;
    k->ctrl.g->kArch.tss.t.sp0=(xmAddress_t)&k->kStack[CONFIG_KSTACK_SIZE];
    k->ctrl.g->kArch.gdtTab[PERCPU_SEL>>3] = gdtTab[GDT_ENTRY(GET_CPU_ID(), PERCPU_SEL)];
    SetWp();
}

void SetupKThreadArch(kThread_t *k) {
    partition_t *p = GetPartition(k);

    ASSERT(k->ctrl.g);
    ASSERT(p);
    memcpy(k->ctrl.g->kArch.gdtTab, gdtTab, sizeof(struct x86Desc)*(XM_GDT_ENTRIES+CONFIG_PARTITION_NO_GDT_ENTRIES));

    k->ctrl.g->kArch.gdtr.limit=(sizeof(struct x86Desc)*(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES))-1;
    k->ctrl.g->kArch.gdtr.linearBase=(xmAddress_t)k->ctrl.g->kArch.gdtTab;

    k->ctrl.g->kArch.cr0=_CR0_PE|_CR0_PG;
    if (!AreKThreadFlagsSet(k, KTHREAD_FP_F)) {
        k->ctrl.g->kArch.cr0|=_CR0_EM;
    }
    
    memcpy(k->ctrl.g->kArch.idtTab, idtTab, sizeof(struct x86Gate)*IDT_ENTRIES);
    
    k->ctrl.g->kArch.idtr.limit=(sizeof(struct x86Gate)*IDT_ENTRIES)-1;
    k->ctrl.g->kArch.idtr.linearBase=(xmAddress_t)k->ctrl.g->kArch.idtTab;
    
    if (p->cfg->noIoPorts>0) {
        memcpy(k->ctrl.g->kArch.tss.ioMap, xmcIoPortTab[p->cfg->ioPortsOffset].map, 2048*sizeof(xm_u32_t));
        EnableTssIoMap(&k->ctrl.g->kArch.tss);
    } else
        DisableTssIoMap(&k->ctrl.g->kArch.tss);
    LoadTssDesc(&k->ctrl.g->kArch.gdtTab[TSS_SEL>>3], &k->ctrl.g->kArch.tss);
}

void SetupPctArch(partitionControlTable_t *partCtrlTab, kThread_t *k) {
    xm_s32_t e;

    partCtrlTab->arch.cr3=k->ctrl.g->kArch.ptdL1;
    partCtrlTab->arch.cr0=_CR0_PE|_CR0_PG;
    if (!AreKThreadFlagsSet(k, KTHREAD_FP_F)) {
        partCtrlTab->arch.cr0|=_CR0_EM;
    }

    for (e=0; e<NO_TRAPS; e++)
	k->ctrl.g->partCtrlTab->trap2Vector[e]=e;

    for (e=0; e<CONFIG_NO_HWIRQS; e++)
	k->ctrl.g->partCtrlTab->hwIrq2Vector[e]=e+FIRST_EXTERNAL_VECTOR;
    
    for (e=0; e<XM_VT_EXT_MAX; e++)
	k->ctrl.g->partCtrlTab->extIrq2Vector[e]=0x90+e;
}

