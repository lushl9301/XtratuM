/*
 * $FILE: hm.c
 *
 * Health Monitor
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
#include <boot.h>
#include <hypercalls.h>
#include <kthread.h>
#include <physmm.h>
#include <stdc.h>
#include <sched.h>
#include <objects/status.h>
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#include <drivers/ttnocports.h>
#endif

xmSystemStatus_t systemStatus;
xmPartitionStatus_t *partitionStatus;
xmVirtualCpuStatus_t vCpuStatus[CONFIG_NO_CPUS];
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern struct messageTTNoC xmMessageTTNoCRemoteRx[CONFIG_TTNOC_NODES];
#endif

static xm_s32_t CtrlStatus(xmObjDesc_t desc, xm_u32_t cmd, union statusCmd *__gParam args) {
    localSched_t *sched=GET_LOCAL_SCHED();
    extern xm_u32_t sysResetCounter[];
    xmId_t partId;
    xmId_t vCpuId;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    xmId_t nodeId;
#endif
    partId=OBJDESC_GET_PARTITIONID(desc);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
     if ((cmd!=XM_GET_NODE_STATUS)||(cmd!=XM_GET_NODE_STATUS))
#endif
       if (partId!=GetPartition(sched->cKThread)->cfg->id){
          if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	     return XM_PERM_ERROR;
       }
       
    switch(cmd) {
    case XM_SET_PARTITION_OPMODE:
        if (partId==XM_HYPERVISOR_ID)
            return XM_INVALID_PARAM;
        if ((partId<0)||(partId>=xmcTab.noPartitions))
                return XM_INVALID_PARAM;
        if ((args->opMode<0)||(args->opMode>3))
                return XM_INVALID_PARAM;
        partitionTab[partId].opMode=args->opMode;
        return XM_OK;
    case XM_GET_SYSTEM_STATUS:
	if (partId==XM_HYPERVISOR_ID) {
	    systemStatus.resetCounter=sysResetCounter[0];
	    memcpy(&args->status.system, &systemStatus, sizeof(xmSystemStatus_t));
	} else {
            xm_s32_t noVCpus;
            xm_s32_t e;
            xm_s32_t halt=0,ready=0,suspend=0;
            if ((partId<0)||(partId>=xmcTab.noPartitions))
		return XM_INVALID_PARAM;
            noVCpus=partitionTab[partId].cfg->noVCpus;
            for (e=0; e<noVCpus; e++) {
                if (AreKThreadFlagsSet(partitionTab[partId].kThread[e], KTHREAD_HALTED_F))
                    halt++;
                else if (AreKThreadFlagsSet(partitionTab[partId].kThread[e], KTHREAD_SUSPENDED_F))
                    suspend++;
                else if (AreKThreadFlagsSet(partitionTab[partId].kThread[e], KTHREAD_READY_F))
                    ready++;
           }
            
            if (ready) {
                partitionStatus[partId].state=XM_STATUS_READY;
            } else if (suspend) {
                partitionStatus[partId].state=XM_STATUS_SUSPENDED;
            } else  if (halt) {
                partitionStatus[partId].state=XM_STATUS_HALTED;
            } else {
                partitionStatus[partId].state=XM_STATUS_IDLE;
            }        
            
	    partitionStatus[partId].resetCounter=partitionTab[partId].kThread[0]->ctrl.g->partCtrlTab->resetCounter;
	    partitionStatus[partId].resetStatus=partitionTab[partId].kThread[0]->ctrl.g->partCtrlTab->resetStatus;
            partitionStatus[partId].opMode=partitionTab[partId].opMode;
	    //partitionStatus[partId].execClock=GetTimeUsecVClock(&partitionTab[partId]->ctrl.g->vClock);
            
	    memcpy(&args->status.partition, &partitionStatus[partId].state, sizeof(xmPartitionStatus_t));
	}	
	return XM_OK;
    case XM_GET_VCPU_STATUS:
        if ((partId<0)||(partId>=xmcTab.noPartitions))
		return XM_INVALID_PARAM;
        vCpuId=OBJDESC_GET_VCPUID(desc);
        if (vCpuId>partitionTab[partId].cfg->noVCpus)
           return XM_INVALID_PARAM;
        if (AreKThreadFlagsSet(partitionTab[partId].kThread[vCpuId], KTHREAD_HALTED_F)) {
                vCpuStatus[vCpuId].state=XM_STATUS_HALTED;
        } else if (AreKThreadFlagsSet(partitionTab[partId].kThread[vCpuId], KTHREAD_SUSPENDED_F)) {
                vCpuStatus[vCpuId].state=XM_STATUS_SUSPENDED;
        } else  if (AreKThreadFlagsSet(partitionTab[partId].kThread[vCpuId], KTHREAD_READY_F)) {
                vCpuStatus[vCpuId].state=XM_STATUS_READY;
        } else {
                vCpuStatus[vCpuId].state=XM_STATUS_IDLE;
        }
        vCpuStatus[vCpuId].opMode=partitionTab[partId].kThread[vCpuId]->ctrl.g->opMode;
        memcpy(&args->status.vcpu,&vCpuStatus[vCpuId],sizeof(xmVirtualCpuStatus_t));
        return XM_OK;
    case XM_GET_SCHED_PLAN_STATUS:
    {
        if (xmcTab.hpv.cpuTab[GET_CPU_ID()].schedPolicy!=CYCLIC_SCHED)
           return XM_NO_ACTION;
#ifdef CONFIG_CYCLIC_SCHED
        args->status.plan.switchTime=sched->data->cyclic.planSwitchTime;
        args->status.plan.next=sched->data->cyclic.plan.new->id;
        args->status.plan.current=sched->data->cyclic.plan.current->id;
        if (sched->data->cyclic.plan.prev)
            args->status.plan.prev=sched->data->cyclic.plan.prev->id;
        else
            args->status.plan.prev=-1;        
        return XM_OK;
#endif
    }
    case XM_GET_PHYSPAGE_STATUS:
    {
        struct physPage *page;
        if ((partId<0)||(partId>=xmcTab.noPartitions))
            return XM_INVALID_PARAM;
        if (!(page=PmmFindPage(args->status.physPage.pAddr, &partitionTab[partId], 0)))
            return XM_INVALID_PARAM;
        args->status.physPage.counter=page->counter;
        args->status.physPage.type=page->type;
        return XM_OK;
    }
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    case XM_GET_NODE_STATUS:
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	     return XM_PERM_ERROR;
        nodeId=OBJDESC_GET_NODEID(desc);
        partId=OBJDESC_GET_PARTITIONID(desc);
        if ((ttnocNodes[nodeId].nodeId==-1)||(ttnocNodes[nodeId].rxSlot==NULL))
           return XM_INVALID_PARAM;
        if (KDevRead(ttnocNodes[nodeId].rxSlot, NULL,0)<0)
           return XM_OP_NOT_ALLOWED;
// 	xmSystemStatusRemote_t * nodeStatus;
// 	nodeStatus=args->status.nodeStatus;
	
	    args->status.nodeStatus.nodeId=xmMessageTTNoCRemoteRx[nodeId].infoNode.nodeId;
        args->status.nodeStatus.state=xmMessageTTNoCRemoteRx[nodeId].stateHyp.stateHyp;
        args->status.nodeStatus.noPartitions=xmMessageTTNoCRemoteRx[nodeId].infoNode.noParts;
        args->status.nodeStatus.noSchedPlans=xmMessageTTNoCRemoteRx[nodeId].infoNode.noSchedPlans;
        args->status.nodeStatus.currentPlan=xmMessageTTNoCRemoteRx[nodeId].stateHyp.currSchedPlan;
        return XM_OK;
    case XM_GET_NODE_PARTITION:
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	     return XM_PERM_ERROR;
        nodeId=OBJDESC_GET_NODEID(desc);
        if ((ttnocNodes[nodeId].nodeId==-1)||(ttnocNodes[nodeId].rxSlot==NULL))
           return XM_INVALID_PARAM;
        if (KDevRead(ttnocNodes[nodeId].rxSlot, NULL,0)<0)
           return XM_OP_NOT_ALLOWED;
        if (partId>xmMessageTTNoCRemoteRx[nodeId].infoNode.noParts)
           return XM_INVALID_PARAM;
// 	xmPartitionStatusRemote_t *nodePartition;
//         nodePartition=args->status.nodePartition;
	args->status.nodePartition.state=((xmMessageTTNoCRemoteRx[nodeId].statePart.partState)&(0x3<<partId))>>partId;
        return XM_OK;
#endif
    default:
      return XM_INVALID_PARAM;
    }
    return XM_INVALID_PARAM;
}

static const struct object statusObj={
    .Ctrl=(ctrlObjOp_t)CtrlStatus,
};

xm_s32_t __VBOOT SetupStatus(void) {
    GET_MEMZ(partitionStatus, sizeof(xmPartitionStatus_t)*xmcTab.noPartitions);
    objectTab[OBJ_CLASS_STATUS]=&statusObj;

    return 0;
}

REGISTER_OBJ(SetupStatus);


