/*
 * $FILE: sched.c
 *
 * Scheduling related stuffs
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
#include <boot.h>
#include <rsvmem.h>
#include <irqs.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <arch/asm.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#include <drivers/ttnocports.h>
#endif
partition_t *partitionTab;
static struct schedData *schedDataTab;

#ifdef CONFIG_CYCLIC_SCHED
static const struct xmcSchedCyclicPlan idleCyclicPlanTab= {
    .nameOffset = 0,
    .id = 0,
    .majorFrame = 0,
    .noSlots = 0,
    .slotsOffset = 0,
};


#ifdef CONFIG_PLAN_EXTSYNC
static volatile xm_s32_t extSync[CONFIG_NO_CPUS];
static volatile xm_s32_t actExtSync[CONFIG_NO_CPUS];

static void SchedSyncHandler(cpuCtxt_t *irqCtxt, void *extra) {
    extSync[GET_CPU_ID()]=1;
#ifdef CONFIG_MASKING_VT_HW_IRQS
    HwEndIrq(irqCtxt->irqNr);
#endif
    Schedule();
}
#endif

////////////////// CYCLIC SCHEDULER
xm_s32_t SwitchSchedPlan(xm_s32_t newPlanId, xm_s32_t *oldPlanId) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t e;

    *oldPlanId=-1;
    if (sched->data->cyclic.plan.current)
        *oldPlanId=sched->data->cyclic.plan.current->id;

    for (e=0; e<xmcTab.hpv.noCpus; e++) {
        if (newPlanId<xmcTab.hpv.cpuTab[e].noSchedCyclicPlans)
            localSchedInfo[e].data->cyclic.plan.new=&xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[e].schedCyclicPlansOffset+newPlanId];
        else
            localSchedInfo[e].data->cyclic.plan.new=&idleCyclicPlanTab;
    }

#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PLAN_SWITCH_REQ, 1, (xmWord_t *)&newPlanId);
#endif
    return 0;
}

inline void MakePlanSwitch(xmTime_t cTime, struct cyclicData *cyclic) {
    extern void IdleTask(void);    
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t planIds[2];
#endif
    if (cyclic->plan.current!=cyclic->plan.new) {
        cyclic->plan.prev=cyclic->plan.current;
        cyclic->plan.current=cyclic->plan.new;
        cyclic->planSwitchTime=cTime;
        cyclic->slot=-1;
        cyclic->mjf=0;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
        updateStateHyp(XM_STATUS_READY);
#endif
        if (!cyclic->plan.new->majorFrame)
            IdleTask();
#ifdef CONFIG_AUDIT_EVENTS
        planIds[0]=cyclic->plan.current->id;
        planIds[1]=cyclic->plan.new->id;
        RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PLAN_SWITCH_DONE, 2, planIds);
#endif
    }
}

static kThread_t *GetReadyKThreadCyclic(struct schedData *schedData) {
    const struct xmcSchedCyclicPlan *plan;
    xmTime_t cTime=GetSysClockUsec();
    struct cyclicData *cyclic=&schedData->cyclic;
    kThread_t *newK=0;
    xm_u32_t t, nextTime;
    xm_s32_t slotTabEntry;
#ifdef CONFIG_PLAN_EXTSYNC
    xm_s32_t nCpu=GET_CPU_ID();
#endif

    if (cyclic->nextAct>cTime && !(cyclic->flags&RESCHED_ENABLED))
            return (cyclic->kThread&&!AreKThreadFlagsSet(cyclic->kThread, KTHREAD_HALTED_F|KTHREAD_SUSPENDED_F)&&AreKThreadFlagsSet(cyclic->kThread, KTHREAD_READY_F))?cyclic->kThread:0;

    cyclic->flags&=~RESCHED_ENABLED;
    plan=cyclic->plan.current;
    if (cyclic->mjf<=cTime) {
#ifdef CONFIG_PLAN_EXTSYNC
        if (actExtSync[nCpu]&&!extSync[nCpu]) {
            cyclic->slot=-1;
            return 0; // Idle
        }
        //extSync=0;
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
        updateStateAllPart();
        sendStateToAllNodes();
        receiveStateFromAllNodes();
#endif
        MakePlanSwitch(cTime, cyclic);
        plan=cyclic->plan.current;
	if (cyclic->slot>=0) {
            while(cyclic->mjf<=cTime) {
                cyclic->sExec=cyclic->mjf;
                cyclic->mjf+=plan->majorFrame;
            }
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.currentMaf++;
#endif
        } else {
	    cyclic->sExec=cTime;
	    cyclic->mjf=plan->majorFrame+cyclic->sExec;
	}

	cyclic->slot=0;
    }
#ifdef CONFIG_PLAN_EXTSYNC
    extSync[nCpu]=0;
#endif
    t=cTime-cyclic->sExec;
    nextTime=plan->majorFrame;

    // Calculate our next slot
    if (cyclic->slot>=plan->noSlots)
	goto out; // getting idle

    while (t>=xmcSchedCyclicSlotTab[plan->slotsOffset+cyclic->slot].eExec) {
	cyclic->slot++;
	if (cyclic->slot>=plan->noSlots)
	    goto out; // getting idle
    }
    slotTabEntry=plan->slotsOffset+cyclic->slot;
    
    if (t>=xmcSchedCyclicSlotTab[slotTabEntry].sExec) {
        ASSERT((xmcSchedCyclicSlotTab[slotTabEntry].partitionId>=0)&&(xmcSchedCyclicSlotTab[slotTabEntry].partitionId<xmcTab.noPartitions));
        ASSERT(partitionTab[xmcSchedCyclicSlotTab[slotTabEntry].partitionId].kThread[xmcSchedCyclicSlotTab[slotTabEntry].vCpuId]);
        newK=partitionTab[xmcSchedCyclicSlotTab[slotTabEntry].partitionId].kThread[xmcSchedCyclicSlotTab[slotTabEntry].vCpuId];

        if (!AreKThreadFlagsSet(newK, KTHREAD_HALTED_F|KTHREAD_SUSPENDED_F)&&
            AreKThreadFlagsSet(newK, KTHREAD_READY_F)) {
            nextTime=xmcSchedCyclicSlotTab[slotTabEntry].eExec;
        } else {
            newK=0;
            if ((cyclic->slot+1)<plan->noSlots)
                nextTime=xmcSchedCyclicSlotTab[slotTabEntry+1].sExec;
        }        
    } else {
	nextTime=xmcSchedCyclicSlotTab[slotTabEntry].sExec;
    }

out:
//    ASSERT(cyclic->nextAct<(nextTime+cyclic->sExec));
    cyclic->nextAct=nextTime+cyclic->sExec;
    ArmKTimer(&cyclic->kTimer, cyclic->nextAct, 0);
    slotTabEntry=plan->slotsOffset+cyclic->slot;
#if 0
    if (newK) {
	kprintf("[%d:%d:%d] cTime %lld -> sExec %lld eExec %lld\n", 
                GET_CPU_ID(),
		cyclic->slot,
		xmcSchedCyclicSlotTab[slotTabEntry].partitionId, 
		cTime, xmcSchedCyclicSlotTab[slotTabEntry].sExec+cyclic->sExec, 
		xmcSchedCyclicSlotTab[slotTabEntry].eExec+cyclic->sExec);
    } else {
	kprintf("[%d] IDLE: %lld\n", GET_CPU_ID(), cTime);
    }
#endif
  
    if (newK&&newK->ctrl.g) {
        newK->ctrl.g->opMode=XM_OPMODE_NORMAL;
        newK->ctrl.g->partCtrlTab->schedInfo.noSlot=cyclic->slot;
        newK->ctrl.g->partCtrlTab->schedInfo.id=xmcSchedCyclicSlotTab[slotTabEntry].id;
        newK->ctrl.g->partCtrlTab->schedInfo.slotDuration=xmcSchedCyclicSlotTab[slotTabEntry].eExec-xmcSchedCyclicSlotTab[slotTabEntry].sExec;
        SetExtIrqPending(newK, XM_VT_EXT_CYCLIC_SLOT_START);
    }

    cyclic->kThread=newK;

    return newK;
} 


#endif

#ifdef CONFIG_FP_SCHED

static kThread_t *GetReadyKThreadFP(struct schedData *schedData) {
    struct fpData *fp=&schedData->fp;
    kThread_t *newK=0;
    xm_s32_t e;

    for (e=0; e<fp->noFpEntries; e++) {
        newK=partitionTab[fp->fpTab[e].partitionId].kThread[fp->fpTab[e].vCpuId];
        if (!AreKThreadFlagsSet(newK, KTHREAD_HALTED_F|KTHREAD_SUSPENDED_F)&&
            AreKThreadFlagsSet(newK, KTHREAD_READY_F)) {
            newK->ctrl.g->opMode=XM_OPMODE_NORMAL;
            return newK;
        }
    }

    return 0;
}

#endif

void __VBOOT InitSched(void) {
    extern void SetupKThreads(void);

    SetupKThreads();
    GET_MEMZ(partitionTab, sizeof(partition_t)*xmcTab.noPartitions);
    GET_MEMZ(schedDataTab, sizeof(struct schedData)*xmcTab.hpv.noCpus);
}

void __VBOOT InitSchedLocal(kThread_t *idle) {
    localSched_t *sched=GET_LOCAL_SCHED();
#ifdef CONFIG_CYCLIC_SCHED
#ifdef CONFIG_PLAN_EXTSYNC
    extSync[GET_CPU_ID()]=0;
    actExtSync[GET_CPU_ID()]=0;
    xm_u32_t irqNr = xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset].extSync;
    if (irqNr != -1) {
        localCpu_t *lCpu=GET_LOCAL_CPU();
        lCpu->globalIrqMask &=~(1<<irqNr); //Keep in mind the ext sync irq in the globalIrqMask
        actExtSync[GET_CPU_ID()]=1;
        SetIrqHandler(irqNr, SchedSyncHandler, 0);
        HwEnableIrq(irqNr);
    }
#endif
#endif

    InitIdle(idle, GET_CPU_ID());
    sched->cKThread=sched->idleKThread=idle;
    sched->data=&schedDataTab[GET_CPU_ID()];
    memset(sched->data, 0, sizeof(struct schedData));
#ifdef CONFIG_CYCLIC_SCHED
    InitKTimer(GET_CPU_ID(),&sched->data->cyclic.kTimer, (void (*)(struct kTimer *, void *))SetSchedPending, NULL, NULL);
    sched->data->cyclic.slot=-1;
    sched->data->cyclic.plan.new=&xmcSchedCyclicPlanTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedCyclicPlansOffset];
    sched->data->cyclic.plan.current=0;
    sched->data->cyclic.plan.prev=0;
    if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy==CYCLIC_SCHED)
        sched->GetReadyKThread=GetReadyKThreadCyclic;
#endif

#ifdef CONFIG_FP_SCHED
    sched->data->fp.fpTab=&xmcFpSchedTab[xmcTab.hpv.cpuTab[GET_CPU_ID()].schedFpTabOffset];
    sched->data->fp.noFpEntries=xmcTab.hpv.cpuTab[GET_CPU_ID()].noFpEntries;
    if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy==FP_SCHED)
        sched->GetReadyKThread=GetReadyKThreadFP;
#endif
}

void SetSchedPending(void) {
    localCpu_t *cpu=GET_LOCAL_CPU();
    cpu->irqNestingCounter|=SCHED_PENDING;
}

void Schedule(void) {
    localSched_t *sched=GET_LOCAL_SCHED();
    localCpu_t *cpu=GET_LOCAL_CPU();
    xmWord_t hwFlags;
    kThread_t *newK;

    CHECK_KTHR_SANITY(sched->cKThread);
    if (!(sched->flags&LOCAL_SCHED_ENABLED)) {
	cpu->irqNestingCounter&=~(SCHED_PENDING);
	return;
    }

    HwSaveFlagsCli(hwFlags);
    // When an interrupt is in-progress, the scheduler shouldn't be invoked
    if (cpu->irqNestingCounter&IRQ_IN_PROGRESS) {
	cpu->irqNestingCounter|=SCHED_PENDING;
	HwRestoreFlags(hwFlags);
	return;
    }
 
    cpu->irqNestingCounter&=(~SCHED_PENDING);
    if (!(newK=sched->GetReadyKThread(sched->data)))
	newK=sched->idleKThread;

    CHECK_KTHR_SANITY(newK);
    if (newK!=sched->cKThread) {
#ifdef CONFIG_AUDIT_EVENTS
        xmWord_t auditArgs;         

        auditArgs=(newK!=sched->idleKThread)?newK->ctrl.g->id:-1;
        RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_CONTEXT_SWITCH, 1, &auditArgs);
#endif
#if 0
	if (newK->ctrl.g)
	    kprintf("newK: [%d:%d] 0x%x ", KID2PARTID(newK->ctrl.g->id), KID2VCPUID(newK->ctrl.g->id), newK);
	else
	    kprintf("newK: idle ");

	if (sched->cKThread->ctrl.g)
	    kprintf("curK: [%d:%d] 0x%x\n", KID2PARTID(sched->cKThread->ctrl.g->id), KID2VCPUID(sched->cKThread->ctrl.g->id));
	else
	    kprintf("curK: idle\n");
#endif

	SwitchKThreadArchPre(newK, sched->cKThread);
/*#ifdef CONFIG_FP_SCHED
        if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy==CYCLIC_SCHED){
	   if (sched->cKThread->ctrl.g) // not idle kthread
	       StopVClock(&sched->cKThread->ctrl.g->vClock,  &sched->cKThread->ctrl.g->vTimer);
        }
#else*/
	if (sched->cKThread->ctrl.g) // not idle kthread
	    StopVClock(&sched->cKThread->ctrl.g->vClock,  &sched->cKThread->ctrl.g->vTimer);
//#endif

	if (newK->ctrl.g)
	    SetHwTimer(TraverseKTimerQueue(&newK->ctrl.localActiveKTimers, GetSysClockUsec()));
#ifdef CONFIG_FP_SCHED
        if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy==CYCLIC_SCHED){
	   sched->cKThread->ctrl.irqMask=HwIrqGetMask();
        }
	HwIrqSetMask(newK->ctrl.irqMask);
#else
	sched->cKThread->ctrl.irqMask=HwIrqGetMask();
	HwIrqSetMask(newK->ctrl.irqMask);
#endif

	CONTEXT_SWITCH(newK, &sched->cKThread);

/*#ifdef CONFIG_FP_SCHED
        if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy==CYCLIC_SCHED){
	   if (sched->cKThread->ctrl.g) {
	       ResumeVClock(&sched->cKThread->ctrl.g->vClock, &sched->cKThread->ctrl.g->vTimer);
           }
        }
#else*/
	if (sched->cKThread->ctrl.g) {
	    ResumeVClock(&sched->cKThread->ctrl.g->vClock, &sched->cKThread->ctrl.g->vTimer);
        }
//#endif
	SwitchKThreadArchPost(sched->cKThread);
    }
    HwRestoreFlags(hwFlags);
}
