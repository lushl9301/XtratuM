/*
 * $FILE: commports.c
 *
 * Inter-partition communication mechanisms
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
#include <list.h>
#include <gaccess.h>
#include <kthread.h>
#include <hypercalls.h>
#include <rsvmem.h>
#include <sched.h>
#include <spinlock.h>
#include <stdc.h>
#include <xmconf.h>
#include <objects/commports.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif

static union channel *channelTab;
static struct port *portTab;

static inline xm_s32_t CreateTTnocPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct xmcPartition *partition;
    xm_u32_t flags;
    xm_s32_t port;
        
    partition=GetPartition(sched->cKThread)->cfg;

    if (OBJDESC_GET_PARTITIONID(desc)!=partition->id) 
	return XM_PERM_ERROR;

    if (CheckGParam(portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    if ((direction!=XM_SOURCE_PORT)&&(direction!=XM_DESTINATION_PORT))
        return XM_INVALID_PARAM;

    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strncmp(portName, &xmcStringTab[xmcCommPorts[port].nameOffset], CONFIG_ID_STRING_LENGTH)) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_TTNOC_PORT) 
	return XM_INVALID_CONFIG;

    if (direction!=xmcCommPorts[port].direction)
	return XM_INVALID_CONFIG;

    SpinLock(&portTab[port].lock);
    flags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);

    if (flags&COMM_PORT_OPENED)
        return XM_NO_ACTION;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].t.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;

//         if (xmcCommChannelTab[xmcCommPorts[port].channelId].t.validPeriod!=validPeriod)
//             return XM_INVALID_CONFIG;
        
        SpinLock(&channelTab[xmcCommPorts[port].channelId].t.lock);
        if (direction==XM_DESTINATION_PORT) {
            ASSERT_LOCK(channelTab[xmcCommPorts[port].channelId].t.noReceivers<xmcCommChannelTab[xmcCommPorts[port].channelId].t.noReceivers, &channelTab[xmcCommPorts[port].channelId].t.lock);
            channelTab[xmcCommPorts[port].channelId].t.receiverTab[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].t.receiverPortTab[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=port-partition->commPortsOffset;
            channelTab[xmcCommPorts[port].channelId].t.nodeId[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=xmcCommChannelTab[xmcCommPorts[port].channelId].t.nodeId;
            channelTab[xmcCommPorts[port].channelId].t.noReceivers++;
        } else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].t.sender=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].t.senderPort=port-partition->commPortsOffset;
        }
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].t.lock);
    }

    SpinLock(&portTab[port].lock);
    portTab[port].flags|=COMM_PORT_OPENED|COMM_PORT_EMPTY;
    portTab[port].partitionId=KID2PARTID(sched->cKThread->ctrl.g->id);
    portTab[port].ttnocDev=LookUpKDev(xmcCommPorts[port].devId);
    if (portTab[port].ttnocDev==0){
       kprintf("Invalid assignation of DEV\n");
       return XM_INVALID_CONFIG;
    }
    SpinUnlock(&portTab[port].lock);
    return port;
}

static xm_s32_t ReadTTnocPort(xmObjDesc_t desc,  void *__gParam msgPtr, xmSize_t msgSize, xm_u32_t *__gParam flags) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xmSize_t retSize=0;
    struct guest *g;
    xmWord_t *commPortBitmap;
    xmId_t partitionId;
    xm_u32_t pTFlags;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    if (CheckGParam(flags, sizeof(xm_u32_t), 4, PFLAG_RW)<0)
        return XM_INVALID_PARAM;

    SpinLock(&portTab[port].lock);
    partitionId=portTab[port].partitionId;
    pTFlags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);
    // Reading a port which does not belong to this partition
    if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;

    if (!(pTFlags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_TTNOC_CHANNEL) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].direction!=XM_DESTINATION_PORT)
	return XM_INVALID_PARAM;
    
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->t.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL|PFLAG_RW)<0)
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->t.lock);
	retSize=(msgSize<channel->t.length)?msgSize:channel->t.length;

	KDevRead(portTab[port].ttnocDev,msgPtr,retSize);
	//	memcpy(msgPtr, channel->t.buffer, retSize);
        SpinLock(&portTab[port].lock);
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_CONSUMED_MSG;
        SpinUnlock(&portTab[port].lock);
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noTTnocPortMsgsRead++;
        if (sched->cKThread->ctrl.g)
            partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noTTnocPortMsgsRead++;
#endif

        g=sched->cKThread->ctrl.g;
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
        ASSERT((port-GetPartition(sched->cKThread)->cfg->commPortsOffset)<g->partCtrlTab->noCommPorts);
        xmClearBit(commPortBitmap, (port-GetPartition(sched->cKThread)->cfg->commPortsOffset), g->partCtrlTab->noCommPorts);

//         if (channel->s.sender) {
//             xm_s32_t e;
//             for (e=0; e<channel->s.sender->cfg->noVCpus; e++) {
//                 g=channel->s.sender->kThread[e]->ctrl.g;
//                 commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
//                 xmClearBit(commPortBitmap, channel->s.senderPort, g->partCtrlTab->noCommPorts);
//             }
//             SetPartitionExtIrqPending(channel->s.sender, XM_VT_EXT_SAMPLING_PORT);
//         }
// 
 	if (flags) {
 	    *flags=0;
//             if (retSize&&(xmcChannel->t.validPeriod!=XM_INFINITE_TIME)&&(channel->t.timestamp+xmcChannel->t.validPeriod)>GetSysClockUsec())
            *flags=XM_COMM_MSG_VALID;
 	}
        SpinUnlock(&channel->t.lock);
    }

    return retSize;
}

static xm_s32_t WriteTTnocPort(xmObjDesc_t desc, void *__gParam msgPtr, xmSize_t msgSize) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xm_s32_t e, i;
    struct guest *g;
    xmWord_t *commPortBitmap;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    
    // Reading a port which does not belong to this partition    
    if (portTab[port].partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;

    if (!(portTab[port].flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].direction!=XM_SOURCE_PORT)
	return XM_INVALID_PARAM;

    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->t.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL)<0) 
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->t.lock);
        KDevWrite(portTab[port].ttnocDev,msgPtr,msgSize);
// 	memcpy(channel->s.buffer, msgPtr, msgSize);
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noTTnocPortMsgsWritten++;
        if (sched->cKThread->ctrl.g)
            partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noTTnocPortMsgsWritten++;
#endif
        ASSERT_LOCK(channel->t.sender==GetPartition(sched->cKThread), &channel->t.lock);
        for (e=0; e<GetPartition(sched->cKThread)->cfg->noVCpus; e++) {
            g=GetPartition(sched->cKThread)->kThread[e]->ctrl.g;
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
            xmSetBit(commPortBitmap, port, g->partCtrlTab->noCommPorts);
        }

//         for (e=0; e<channel->t.noReceivers; e++) {
//             if (channel->t.receiverTab[e]) {
//                 for (i=0; i<channel->t.receiverTab[e]->cfg->noVCpus; i++) {
//                     g=channel->s.receiverTab[e]->kThread[i]->ctrl.g;
//                     ASSERT_LOCK(channel->s.receiverPortTab[e]<g->partCtrlTab->noCommPorts, &channel->s.lock);
//                     commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
//                     xmSetBit(commPortBitmap, channel->s.receiverPortTab[e], g->partCtrlTab->noCommPorts);
//                     portTab[channel->s.receiverPortTab[e]].flags&=~COMM_PORT_MSG_MASK;
//                     portTab[channel->s.receiverPortTab[e]].flags|=COMM_PORT_NEW_MSG;
//                 }
//                 SetPartitionExtIrqPending(channel->s.receiverTab[e], XM_VT_EXT_SAMPLING_PORT);
//             }
//         }
	channel->t.length=msgSize;
        channel->t.timestamp=GetSysClockUsec();	
        SpinUnlock(&channel->t.lock);
    }

    return XM_OK;
}

static inline xm_s32_t GetTTnocPortInfo(xmObjDesc_t desc, xmTTnocPortInfo_t *info) {
//     localSched_t *sched=GET_LOCAL_SCHED();
//     struct xmcPartition *partition;
//     struct xmcCommChannel *xmcChannel;
//     xm_s32_t port;
//     
//     partition=GetPartition(sched->cKThread)->cfg;
//     
//     if (OBJDESC_GET_PARTITIONID(desc)!=partition->id) 
// 	return XM_PERM_ERROR;
// 
//     if (CheckGParam(info->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
//         return XM_INVALID_PARAM;
// 
//     // Look for the channel
//     for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
// 	if (!strcmp(info->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;
// 
//     if (port>=xmcTab.noCommPorts)
// 	return XM_INVALID_PARAM;
// 
//     if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
// 	return XM_INVALID_PARAM;
//     
//     if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
// 	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
//         info->validPeriod=xmcChannel->s.validPeriod;
// 	info->maxMsgSize=xmcChannel->s.maxLength;
// 	info->direction=xmcCommPorts[port].direction;
//     } else {
// 	info->validPeriod=XM_INFINITE_TIME;
// 	info->maxMsgSize=0;
// 	info->direction=0;
//     }

    return XM_OK;
}

static inline xm_s32_t GetTTnocPortStatus(xmObjDesc_t desc, xmTTnocPortStatus_t *status) {
//     localSched_t *sched=GET_LOCAL_SCHED();
//     xm_s32_t port=OBJDESC_GET_ID(desc);
//     struct xmcCommChannel *xmcChannel;
//     union channel *channel;
//     xm_u32_t flags;
//     xmId_t partitionId;
// 
//     if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
// 	return XM_PERM_ERROR;
// 
//     SpinLock(&portTab[port].lock);
//     partitionId=portTab[port].partitionId;
//     flags=portTab[port].flags;
//     SpinUnlock(&portTab[port].lock);
// 
//     // Reading a port which does not belong to this partition
//     if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
// 	return XM_INVALID_PARAM;
//     
//     if (!(flags&COMM_PORT_OPENED))
// 	return XM_INVALID_PARAM;
//     
//     if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
// 	return XM_INVALID_PARAM;
// 
//     if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
// 	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
// 	status->flags=0;
// 	channel=&channelTab[xmcCommPorts[port].channelId];
//         SpinLock(&channel->s.lock);
//         status->lastMsgSize=channel->s.length;
//         status->timestamp=channel->s.timestamp;
//         if (flags&COMM_PORT_NEW_MSG)
//             status->flags|=XM_COMM_NEW_MSG;
//         else if (flags&COMM_PORT_CONSUMED_MSG) {
//             status->flags|=XM_COMM_CONSUMED_MSG;
//         } else {
//             status->flags|=XM_COMM_EMPTY_PORT;
//         }
// 
//         if (channel->s.timestamp&&(xmcChannel->s.validPeriod!=XM_INFINITE_TIME)&&((channel->s.timestamp+xmcChannel->s.validPeriod)>GetSysClockUsec()))
// 	    status->flags|=XM_COMM_MSG_VALID;
//         SpinUnlock(&channel->s.lock);
//     } else
// 	memset(status, 0, sizeof(xmSamplingPortStatus_t));
    return XM_OK;
}

static xm_s32_t CtrlTTnocPort(xmObjDesc_t desc, xm_u32_t cmd, union samplingPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union samplingPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0)
        return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_COMM_CREATE_PORT:
	if (!args->create.portName||(CheckGParam(args->create.portName, CONFIG_ID_STRING_LENGTH,1, PFLAG_NOT_NULL)<0)) return XM_INVALID_PARAM;

        return CreateTTnocPort(desc, args->create.portName, args->create.maxMsgSize, args->create.direction, args->create.validPeriod);
    case XM_COMM_GET_PORT_STATUS:
	return GetTTnocPortStatus(desc, &args->status);
    case XM_COMM_GET_PORT_INFO:
	return GetTTnocPortInfo(desc, &args->info);
    }
    return XM_INVALID_PARAM;
}

static const struct object ttnocPortObj={
    .Read=(readObjOp_t)ReadTTnocPort,
    .Write=(writeObjOp_t)WriteTTnocPort,
    .Ctrl=(ctrlObjOp_t)CtrlTTnocPort,
};



xm_s32_t __VBOOT SetupComm(void) {
    xm_s32_t e, i;

    ASSERT(GET_CPU_ID()==0);
    GET_MEMZ(channelTab, sizeof(union channel)*xmcTab.noCommChannels);
    GET_MEMZ(portTab, sizeof(struct port)*xmcTab.noCommPorts);

    for (e=0; e<xmcTab.noCommPorts; e++)
        portTab[e].lock=SPINLOCK_INIT;

    /* create the channels */
    for (e=0; e<xmcTab.noCommChannels; e++) {
	switch(xmcCommChannelTab[e].type) {
	case XM_SAMPLING_CHANNEL:
	    GET_MEMZ(channelTab[e].s.buffer, xmcCommChannelTab[e].s.maxLength);
            GET_MEMZ(channelTab[e].s.receiverTab, xmcCommChannelTab[e].s.noReceivers*sizeof(partition_t *));
            GET_MEMZ(channelTab[e].s.receiverPortTab, xmcCommChannelTab[e].s.noReceivers*sizeof(xm_s32_t));
            channelTab[e].s.lock=SPINLOCK_INIT;
	    break;
	case XM_TTNOC_CHANNEL:
// 	    GET_MEMZ(channelTab[e].t.buffer, xmcCommChannelTab[e].t.maxLength);
            GET_MEMZ(channelTab[e].t.receiverTab, xmcCommChannelTab[e].t.noReceivers*sizeof(partition_t *));
            GET_MEMZ(channelTab[e].t.receiverPortTab, xmcCommChannelTab[e].t.noReceivers*sizeof(xm_s32_t));
            channelTab[e].t.lock=SPINLOCK_INIT;
	    break;
	case XM_QUEUING_CHANNEL:
	    GET_MEMZ(channelTab[e].q.msgPool, sizeof(struct msg)*xmcCommChannelTab[e].q.maxNoMsgs);
	    DynListInit(&channelTab[e].q.freeMsgs);
	    DynListInit(&channelTab[e].q.recvMsgs);
	    for (i=0; i<xmcCommChannelTab[e].q.maxNoMsgs; i++) {
		GET_MEMZ(channelTab[e].q.msgPool[i].buffer, xmcCommChannelTab[e].q.maxLength);
		if(DynListInsertHead(&channelTab[e].q.freeMsgs, &channelTab[e].q.msgPool[i].listNode)) {
                    cpuCtxt_t ctxt;
                    GetCpuCtxt(&ctxt);
		    SystemPanic(&ctxt, "[SetupComm] Queuing channels initialisation error");
                }
	    }
            channelTab[e].q.lock=SPINLOCK_INIT;
	    break;
	}
    }

    objectTab[OBJ_CLASS_SAMPLING_PORT]=&samplingPortObj;
    objectTab[OBJ_CLASS_QUEUING_PORT]=&queuingPortObj;
    objectTab[OBJ_CLASS_TTNOC_PORT]=&ttnocPortObj;
    
    return 0;
}

REGISTER_OBJ(SetupComm);

