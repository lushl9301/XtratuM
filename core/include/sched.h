/*
 * $FILE: sched.h
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

#ifndef _XM_SCHED_H_
#define _XM_SCHED_H_

#ifdef CONFIG_FP_SCHED
#define MaxPriorityFP() 0
#define MinPriorityFP() 65536
#endif

#ifdef _XM_KERNEL_

#include <processor.h>
#include <kthread.h>
#include <smp.h>
#include <objects/status.h>
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#include <drivers/ttnocports.h>
#endif

struct schedData {
 #ifdef CONFIG_CYCLIC_SCHED
    struct cyclicData {
        kTimer_t kTimer;
        struct {
            const struct xmcSchedCyclicPlan *current;
            const struct xmcSchedCyclicPlan *new;
            const struct xmcSchedCyclicPlan *prev;
        } plan;
        xm_s32_t slot; // next slot to be processed
        xmTime_t mjf;
        xmTime_t sExec;
        xmTime_t planSwitchTime;
        xmTime_t nextAct;
        kThread_t *kThread;
        xm_u32_t flags;
#define RESCHED_ENABLED 0x1
    } cyclic;
#endif
#ifdef CONFIG_FP_SCHED
    struct fpData {
        struct xmcFpSched *fpTab;
        xm_s32_t noFpEntries;
    } fp;
#endif
};

typedef struct {
    kThread_t *idleKThread;
    kThread_t *cKThread;
    kThread_t *fpuOwner;
    struct schedData *data;
    xm_u32_t flags;
#define LOCAL_SCHED_ENABLED 0x1
    kThread_t *(*GetReadyKThread)(struct schedData *schedData);
} localSched_t;

extern localSched_t localSchedInfo[];

#ifdef CONFIG_SMP
#define GET_LOCAL_SCHED() \
    (&localSchedInfo[GET_CPU_ID()])
#else
#define GET_LOCAL_SCHED() localSchedInfo
#endif

extern void InitSched(void);
extern void InitSchedLocal(kThread_t *idle);
extern void Schedule(void);
extern void SetSchedPending(void);
extern xm_s32_t SwitchSchedPlan(xm_s32_t newPlanId, xm_s32_t *oldPlanId);

static inline void SchedYield(localSched_t *sched, kThread_t *k) {
#ifdef CONFIG_CYCLIC_SCHED
    sched->data->cyclic.kThread=k;
#endif
    Schedule();
}

static inline void DoPreemption(void) {
    localSched_t *sched=GET_LOCAL_SCHED();
    localCpu_t *cpu=GET_LOCAL_CPU();
    
    HwIrqSetMask(cpu->globalIrqMask);
    HwSti();

    DoNop();

    HwCli();
    HwIrqSetMask(sched->cKThread->ctrl.irqMask);
}

static inline void PreemptionOn(void) {
#ifdef CONFIG_VOLUNTARY_PREEMPTION
    localSched_t *sched=GET_LOCAL_SCHED();

    sched->cKThread->ctrl.irqMask=HwIrqGetMask();
    HwIrqSetMask(globalIrqMask);
    HwSti();
#endif
}

static inline void PreemptionOff(void) {
#ifdef CONFIG_VOLUNTARY_PREEMPTION
    localSched_t *sched=GET_LOCAL_SCHED();
    
    HwCli();
    HwIrqSetMask(sched->cKThread->ctrl.irqMask);
#endif
}

#include <audit.h>

static inline void SUSPEND_VCPU(xmId_t partId, xmId_t vCpuId) {
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t arg=PART_VCPU_ID2KID(partId, vCpuId);
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_VCPU_SUSPEND, 1, &arg);
#endif
    SetKThreadFlags(partitionTab[partId].kThread[vCpuId], KTHREAD_SUSPENDED_F);    
}

static inline void RESUME_VCPU(xmId_t partId, xmId_t vCpuId) {
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t arg=PART_VCPU_ID2KID(partId, vCpuId);
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_VCPU_RESUME, 1, &arg);
#endif
    ClearKThreadFlags(partitionTab[partId].kThread[vCpuId], KTHREAD_SUSPENDED_F);    
}

static inline void HALT_VCPU(xmId_t partId, xmId_t vCpuId) {
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t arg=PART_VCPU_ID2KID(partId, vCpuId);
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_VCPU_HALT, 1, &arg);
#endif
    SetKThreadFlags(partitionTab[partId].kThread[vCpuId], KTHREAD_HALTED_F); 
    partitionTab[partId].kThread[vCpuId]->ctrl.g->opMode=XM_OPMODE_IDLE;
#ifdef CONFIG_FP_SCHED
    xm_s32_t cpuId=xmcVCpuTab[(partitionTab[partId].cfg->id*xmcTab.hpv.noCpus)+vCpuId].cpu;
        if(xmcTab.hpv.cpuTab[cpuId].schedPolicy==FP_SCHED){
            DisarmKTimer(&partitionTab[partId].kThread[vCpuId]->ctrl.g->kTimer);
            DisarmKTimer(&partitionTab[partId].kThread[vCpuId]->ctrl.g->watchdogTimer);
        }
#endif
}

static inline void SUSPEND_PARTITION(xmId_t id) {
    xm_s32_t e;
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PART_SUSPEND, 1, (xmWord_t *)&id);
#endif
    for (e=0; e<partitionTab[id].cfg->noVCpus; e++)
        SetKThreadFlags(partitionTab[id].kThread[e], KTHREAD_SUSPENDED_F);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateStatePart(id);
#endif
}

static inline void RESUME_PARTITION(xmId_t id) {
    xm_s32_t e;
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PART_RESUME, 1, (xmWord_t *)&id);
#endif    
    for (e=0; e<partitionTab[id].cfg->noVCpus; e++)
        ClearKThreadFlags(partitionTab[id].kThread[e], KTHREAD_SUSPENDED_F);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateStatePart(id);
#endif
}

static inline void SHUTDOWN_PARTITION(xmId_t id) {
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PART_SHUTDOWN, 1, (xmWord_t *)&id);
#endif    
    SetPartitionExtIrqPending(&partitionTab[id], XM_VT_EXT_SHUTDOWN);
}
/*
static inline void IDLE_PARTITION(xmId_t id) {
    xm_s32_t e;
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PART_IDLE, 1, (xmWord_t *)&id);
#endif
    for (e=0; e<partitionTab[id].cfg->noVCpus; e++)
        ClearKThreadFlags(partitionTab[id].kThread[e], KTHREAD_READY_F);
}
*/
static inline void HALT_PARTITION(xmId_t id) { 
    xm_s32_t e;
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_PART_HALT, 1, (xmWord_t *)&id);
#endif
    for (e=0; e<partitionTab[id].cfg->noVCpus; e++){
        SetKThreadFlags(partitionTab[id].kThread[e], KTHREAD_HALTED_F);
        partitionTab[id].kThread[e]->ctrl.g->opMode=XM_OPMODE_IDLE;
#ifdef CONFIG_FP_SCHED
        xm_s32_t cpuId=xmcVCpuTab[(partitionTab[id].cfg->id*xmcTab.hpv.noCpus)+e].cpu;
        if(xmcTab.hpv.cpuTab[cpuId].schedPolicy==FP_SCHED){
            DisarmKTimer(&partitionTab[id].kThread[e]->ctrl.g->kTimer);
            DisarmKTimer(&partitionTab[id].kThread[e]->ctrl.g->watchdogTimer);
        }
#endif
    }
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateStatePart(id);
#endif
    partitionTab[id].opMode=XM_OPMODE_IDLE;
}

#ifdef CONFIG_CYCLIC_SCHED
extern inline void MakePlanSwitch(xmTime_t cTime, struct cyclicData *cyclic);
#endif

#endif
#endif
