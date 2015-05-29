/*
 * $FILE: irqs.c
 *
 * Independent part of interrupt handling
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <audit.h>
#include <bitwise.h>
#include <boot.h>
#include <irqs.h>
#include <kthread.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <processor.h>
#include <objects/hm.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif

// Definitions
struct irqTabEntry irqHandlerTab[CONFIG_NO_HWIRQS];
trapHandler_t trapHandlerTab[NO_TRAPS];
hwIrqCtrl_t hwIrqCtrl[CONFIG_NO_HWIRQS];

void DoUnrecovExcp(cpuCtxt_t *ctxt);

void DefaultIrqHandler(cpuCtxt_t *ctxt, void *data) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmHmLog_t hmLog;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCodeL|=XM_HM_EV_UNEXPECTED_TRAP<<HMLOG_OPCODE_EVENT_BIT;

    hmLog.opCodeL|=(ctxt->irqNr&HMLOG_OPCODE_EVENT_MASK)<<(HMLOG_OPCODE_EVENT_BIT+8);
  
    if (sched->cKThread!=sched->idleKThread) {
        hmLog.opCodeL|=KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT;
    } else {
        hmLog.opCodeL|=(~0&HMLOG_OPCODE_PARTID_MASK)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=(~0&HMLOG_OPCODE_VCPUID_MASK)<<HMLOG_OPCODE_VCPUID_BIT;
    }

    hmLog.opCodeH|=HMLOG_OPCODE_SYS_MASK;
    
    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCodeH|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
    HmRaiseEvent(&hmLog);

    kprintf("Unexpected irq %d\n", ctxt->irqNr);
}

static void TriggerIrqHandler(cpuCtxt_t *ctxt, void *data) {
    xmId_t partId;
    partId=xmcTab.hpv.hwIrqTab[ctxt->irqNr].owner;

    SetPartitionHwIrqPending(&partitionTab[partId], ctxt->irqNr);
}

void SetTrapPending(cpuCtxt_t *ctxt) {
    localSched_t *sched=GET_LOCAL_SCHED();
    
    ASSERT(!AreKThreadFlagsSet(sched->cKThread, KTHREAD_TRAP_PENDING_F));
    SetKThreadFlags(sched->cKThread, KTHREAD_TRAP_PENDING_F);
}

static inline xmAddress_t IsInPartExTable(xmAddress_t addr) {
    extern struct exPTable {
        xmAddress_t a;
        xmAddress_t b;
    } exPTable[]; 
    struct exPTable *exPTablePtr;
    xm_s32_t e;

    for (exPTablePtr=exPTable; exPTablePtr; exPTablePtr=(struct exPTable *)exPTablePtr[e].b) {
        for (e=0; exPTablePtr[e].a; e++)
            if (addr==exPTablePtr[e].a) 
                return exPTablePtr[e].b;
    }
    return 0;
}

void DoTrap(cpuCtxt_t *ctxt) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmHmLog_t hmLog;
    xm_s32_t action;
    xm_u16_t hmEvent;
    xmWord_t pc;
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t auditArgs[2];
#endif

    ASSERT(ctxt->irqNr<NO_TRAPS);
    if ((pc=IsInPartExTable(GET_CTXT_PC(ctxt)))) {
        SET_CTXT_PC(ctxt, pc);
        return;
    }

#ifdef CONFIG_AUDIT_EVENTS
    if (IsAuditEventMasked(TRACE_IRQ_MODULE)) {
        auditArgs[0]=ctxt->irqNr;
        auditArgs[1]=GET_CTXT_PC(ctxt);
        RaiseAuditEvent(TRACE_IRQ_MODULE, AUDIT_TRAP_RAISED, 2, auditArgs);
    }
#endif

    hmEvent=ctxt->irqNr+XM_HM_MAX_GENERIC_EVENTS;

    if (trapHandlerTab[ctxt->irqNr])
        if (trapHandlerTab[ctxt->irqNr](ctxt, &hmEvent))
            return;

    if (sched->cKThread->ctrl.g){
       if (!ArePCtrlTabTrapsSet(sched->cKThread->ctrl.g->partCtrlTab->iFlags))
          DoUnrecovExcp(ctxt);
    }

    /*Fast propagation of partition events not logged.*/
    if (!IsSvIrqCtxt(ctxt)) {
       ASSERT(hmEvent<XM_HM_MAX_EVENTS);
       if ((GetPartition(sched->cKThread)->cfg->hmTab[hmEvent].action==XM_HM_AC_PROPAGATE)&&(!GetPartition(sched->cKThread)->cfg->hmTab[hmEvent].log)){
#ifdef CONFIG_VERBOSE_TRAP
          kprintf("[TRAP] %s(0x%x)\n", trap2Str[ctxt->irqNr], ctxt->irqNr);
          DumpState(ctxt);
#endif
          SetTrapPending(ctxt);
          return;
       }
    }
 
    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCodeL|=hmEvent<<HMLOG_OPCODE_EVENT_BIT;

    if (sched->cKThread!=sched->idleKThread) {
        hmLog.opCodeL|=KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT;
    }
    
    if (IsSvIrqCtxt(ctxt)) {
        if (ArchTrapIsSysCtxt(ctxt))
            hmLog.opCodeH|=HMLOG_OPCODE_SYS_MASK;
    }

    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCodeH|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
#ifdef CONFIG_VERBOSE_TRAP
    kprintf("[TRAP] %s(0x%x)\n", trap2Str[ctxt->irqNr], ctxt->irqNr);
    DumpState(ctxt);
#endif
    action=HmRaiseEvent(&hmLog);
    if (IsSvIrqCtxt(ctxt)&&((hmLog.opCodeH&HMLOG_OPCODE_SYS_MASK)!=HMLOG_OPCODE_SYS_MASK))
        PartitionPanic(ctxt, "Partition in unrecoverable state\n");
    if (!IsSvIrqCtxt(ctxt)) {
	if (action)
	    SetTrapPending(ctxt);
    } else
	SystemPanic(ctxt, "Unexpected/unhandled trap - TRAP: 0x%x ERROR CODE: 0x%x\n", sched->cKThread->ctrl.g->partCtrlTab->trap2Vector[ctxt->irqNr], GET_ECODE(ctxt));
}

void DoUnrecovExcp(cpuCtxt_t *ctxt) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmHmLog_t hmLog;

    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCodeL|=XM_HM_EV_SYSTEM_ERROR<<HMLOG_OPCODE_EVENT_BIT;

    if (sched->cKThread!=sched->idleKThread) {
        hmLog.opCodeL|=KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT;
    }

    if (IsSvIrqCtxt(ctxt)) {
         hmLog.opCodeH|=HMLOG_OPCODE_SYS_MASK;
    }

    CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);
    hmLog.opCodeH|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
    DumpState(ctxt);
    HmRaiseEvent(&hmLog);

    PartitionPanic(ctxt, "Partition unrecoverable error : 0x%x\n", ctxt->irqNr);
}

void DoIrq(cpuCtxt_t *ctxt) {
    localCpu_t *cpu=GET_LOCAL_CPU();
    ASSERT(!HwIsSti());
#ifdef CONFIG_AUDIT_EVENTS
    if (IsAuditEventMasked(TRACE_IRQ_MODULE))
        RaiseAuditEvent(TRACE_IRQ_MODULE, AUDIT_IRQ_RAISED, 1, &ctxt->irqNr);
#endif
#ifdef CONFIG_OBJ_STATUS_ACC
    systemStatus.noIrqs++;
#endif
    cpu->irqNestingCounter++;
    HwAckIrq(ctxt->irqNr);
    if (irqHandlerTab[ctxt->irqNr].handler)
	(*(irqHandlerTab[ctxt->irqNr].handler))(ctxt, irqHandlerTab[ctxt->irqNr].data);
    else
	DefaultIrqHandler(ctxt, 0);
#ifndef CONFIG_MASKING_VT_HW_IRQS
    HwEndIrq(ctxt->irqNr);
#endif
    cpu->irqNestingCounter--;
    do {
	Schedule();
    } while (cpu->irqNestingCounter==SCHED_PENDING);
    ASSERT(!HwIsSti());
    ASSERT(!(cpu->irqNestingCounter&SCHED_PENDING));
}

void __VBOOT SetupIrqs(void) {
    xm_s32_t irqNr;

    for (irqNr=0; irqNr<CONFIG_NO_HWIRQS; irqNr++) {
	if (xmcTab.hpv.hwIrqTab[irqNr].owner!=XM_IRQ_NO_OWNER) {
	    irqHandlerTab[irqNr]=(struct irqTabEntry){
		.handler=TriggerIrqHandler, 
		.data=0,
	    };
	} else  {
	    irqHandlerTab[irqNr]=(struct irqTabEntry){
		.handler=DefaultIrqHandler, 
		.data=0,
	    };
	}
    }

    for (irqNr=0; irqNr<NO_TRAPS; irqNr++)
	trapHandlerTab[irqNr]=0;

    ArchSetupIrqs();
}

irqHandler_t SetIrqHandler(xm_s32_t irq, irqHandler_t irqHandler, void *data) {
    irqHandler_t oldHandler=irqHandlerTab[irq].handler;

    if (irqHandler) {
	irqHandlerTab[irq]=(struct irqTabEntry){
	    .handler=irqHandler,
	    .data=data,
	};
    } else
	irqHandlerTab[irq]=(struct irqTabEntry){
	    .handler=DefaultIrqHandler,
	    .data=0,
	};
    return oldHandler;
}

trapHandler_t SetTrapHandler(xm_s32_t trap, trapHandler_t trapHandler) {
    trapHandler_t oldHandler=trapHandlerTab[trap];

    trapHandlerTab[trap]=trapHandler;
    return oldHandler;
}

static inline xm_s32_t AreHwIrqsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;

    eIrq=partCtrlTab->hwIrqsPend&~partCtrlTab->hwIrqsMask;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
        ASSERT(eIrq>=0&&eIrq<CONFIG_NO_HWIRQS);	    
        return eIrq;
    }
    
    return -1;
}

static inline xm_s32_t AreExtIrqsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;
    
    eIrq=partCtrlTab->extIrqsPend&~partCtrlTab->extIrqsMask;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
	return eIrq;
    }       
  
    return -1;
}

static inline xm_s32_t AreExtTrapsPending(partitionControlTable_t *partCtrlTab) {
    xm_s32_t eIrq;

    eIrq=partCtrlTab->extIrqsPend&XM_EXT_TRAPS;
    if (eIrq) {
#ifdef CONFIG_HWIRQ_PRIO_FBS
        eIrq=_Ffs(eIrq);
#else
        eIrq=_Fls(eIrq);
#endif
	return eIrq;
    }       
  
    return -1;
}

#ifdef CONFIG_AUDIT_EVENTS
#define RAISE_PENDIRQ_AUDIT_EVENT(pI) do { \
    if (IsAuditEventMasked(TRACE_IRQ_MODULE)) { \
        xmWord_t _tmp=(pI); \
        RaiseAuditEvent(TRACE_IRQ_MODULE, AUDIT_IRQ_EMUL, 1, &_tmp); \
    } \
} while(0)
#else
#define RAISE_PENDIRQ_AUDIT_EVENT(pI)
#endif

xm_s32_t RaisePendIrqs(cpuCtxt_t *ctxt) {
    localSched_t *sched=GET_LOCAL_SCHED();
    partitionControlTable_t *partCtrlTab;
    xm_s32_t eIrq, emul;

    if (!sched->cKThread->ctrl.g||IsSvIrqCtxt(ctxt))
	return ~0;

    // SwTrap
    if (sched->cKThread->ctrl.g->swTrap&0x1) {
        emul=sched->cKThread->ctrl.g->swTrap>>1;
        sched->cKThread->ctrl.g->swTrap=0;
        RAISE_PENDIRQ_AUDIT_EVENT(emul);
        return IrqVector2Address(emul);
    }

    partCtrlTab=sched->cKThread->ctrl.g->partCtrlTab;
    // 1) Check pending traps
    if (AreKThreadFlagsSet(sched->cKThread, KTHREAD_TRAP_PENDING_F)) {
        ClearKThreadFlags(sched->cKThread, KTHREAD_TRAP_PENDING_F);
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulTrapIrq(ctxt, partCtrlTab, ctxt->irqNr);
        RAISE_PENDIRQ_AUDIT_EVENT(emul);
        return IrqVector2Address(emul);
    }

    // 2) Check pending extended trap
    if ((eIrq=AreExtTrapsPending(partCtrlTab))>-1) {
        partCtrlTab->extIrqsPend&=~(1<<eIrq);
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulExtIrq(ctxt, partCtrlTab, eIrq);
        RAISE_PENDIRQ_AUDIT_EVENT(emul);
        return IrqVector2Address(emul);
    }

    // At this moment irq flags must be set
    if (!ArePCtrlTabIrqsSet(partCtrlTab->iFlags))
        return ~0;

    // 3) Check pending hwirqs
    if ((eIrq=AreHwIrqsPending(partCtrlTab))>-1) {
        partCtrlTab->hwIrqsPend&=~(1<<eIrq);
        MaskPCtrlTabIrq(&partCtrlTab->hwIrqsMask, (1<<eIrq));
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulHwIrq(ctxt, partCtrlTab, eIrq);
        RAISE_PENDIRQ_AUDIT_EVENT(emul);
        return IrqVector2Address(emul);
    }

     // 4) Check pending extirqs
    if ((eIrq=AreExtIrqsPending(partCtrlTab))>-1) {
        partCtrlTab->extIrqsPend&=~(1<<eIrq);
        MaskPCtrlTabIrq(&partCtrlTab->extIrqsMask, (1<<eIrq));
        DisablePCtrlTabIrqs(&partCtrlTab->iFlags);
	emul=ArchEmulExtIrq(ctxt, partCtrlTab, eIrq);
        RAISE_PENDIRQ_AUDIT_EVENT(emul);
        return IrqVector2Address(emul);
    }

    // No emulation required
    return ~0;
}
