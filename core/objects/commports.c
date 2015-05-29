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

static inline xm_s32_t CreateSamplingPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod) {
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

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
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
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;

        if (xmcCommChannelTab[xmcCommPorts[port].channelId].s.validPeriod!=validPeriod)
            return XM_INVALID_CONFIG;
        
        SpinLock(&channelTab[xmcCommPorts[port].channelId].s.lock);
        if (direction==XM_DESTINATION_PORT) {
            ASSERT_LOCK(channelTab[xmcCommPorts[port].channelId].s.noReceivers<xmcCommChannelTab[xmcCommPorts[port].channelId].s.noReceivers, &channelTab[xmcCommPorts[port].channelId].s.lock);
            channelTab[xmcCommPorts[port].channelId].s.receiverTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].s.receiverPortTab[channelTab[xmcCommPorts[port].channelId].s.noReceivers]=port-partition->commPortsOffset;
            channelTab[xmcCommPorts[port].channelId].s.noReceivers++;        
        } else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].s.sender=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].s.senderPort=port-partition->commPortsOffset;
        }
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].s.lock);
    }

    SpinLock(&portTab[port].lock);
    portTab[port].flags|=COMM_PORT_OPENED|COMM_PORT_EMPTY;
    portTab[port].partitionId=KID2PARTID(sched->cKThread->ctrl.g->id);
    SpinUnlock(&portTab[port].lock);
    return port;
}

static xm_s32_t ReadSamplingPort(xmObjDesc_t desc,  void *__gParam msgPtr, xmSize_t msgSize, xm_u32_t *__gParam flags) {
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

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].direction!=XM_DESTINATION_PORT)
	return XM_INVALID_PARAM;
    
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->s.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL|PFLAG_RW)<0)
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->s.lock);
	retSize=(msgSize<channel->s.length)?msgSize:channel->s.length;
	memcpy(msgPtr, channel->s.buffer, retSize);
        SpinLock(&portTab[port].lock);
        portTab[port].flags&=~COMM_PORT_MSG_MASK;
        portTab[port].flags|=COMM_PORT_CONSUMED_MSG;
        SpinUnlock(&portTab[port].lock);
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsRead++;
        if (sched->cKThread->ctrl.g)
            partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noSamplingPortMsgsRead++;
#endif

        g=sched->cKThread->ctrl.g;
        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
        ASSERT((port-GetPartition(sched->cKThread)->cfg->commPortsOffset)<g->partCtrlTab->noCommPorts);
        xmClearBit(commPortBitmap, (port-GetPartition(sched->cKThread)->cfg->commPortsOffset), g->partCtrlTab->noCommPorts);

        if (channel->s.sender) {
            xm_s32_t e;
            for (e=0; e<channel->s.sender->cfg->noVCpus; e++) {
                g=channel->s.sender->kThread[e]->ctrl.g;
                commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
                xmClearBit(commPortBitmap, channel->s.senderPort, g->partCtrlTab->noCommPorts);
            }
            SetPartitionExtIrqPending(channel->s.sender, XM_VT_EXT_SAMPLING_PORT);
        }

	if (flags) {
	    *flags=0;
            if (retSize&&(xmcChannel->s.validPeriod!=XM_INFINITE_TIME)&&(channel->s.timestamp+xmcChannel->s.validPeriod)>GetSysClockUsec())
		*flags=XM_COMM_MSG_VALID;
	}
        SpinUnlock(&channel->s.lock);
    }

    return retSize;
}

static xm_s32_t WriteSamplingPort(xmObjDesc_t desc, void *__gParam msgPtr, xmSize_t msgSize) {
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
	if (msgSize>xmcChannel->s.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL)<0) 
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->s.lock);
	memcpy(channel->s.buffer, msgPtr, msgSize);
#ifdef CONFIG_OBJ_STATUS_ACC
        systemStatus.noSamplingPortMsgsWritten++;
        if (sched->cKThread->ctrl.g)
            partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noSamplingPortMsgsWritten++;
#endif
        ASSERT_LOCK(channel->s.sender==GetPartition(sched->cKThread), &channel->s.lock);
        for (e=0; e<GetPartition(sched->cKThread)->cfg->noVCpus; e++) {
            g=GetPartition(sched->cKThread)->kThread[e]->ctrl.g;
            commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
            xmSetBit(commPortBitmap, port, g->partCtrlTab->noCommPorts);
        }

        for (e=0; e<channel->s.noReceivers; e++) {
            if (channel->s.receiverTab[e]) {
                for (i=0; i<channel->s.receiverTab[e]->cfg->noVCpus; i++) {
                    g=channel->s.receiverTab[e]->kThread[i]->ctrl.g;
                    ASSERT_LOCK(channel->s.receiverPortTab[e]<g->partCtrlTab->noCommPorts, &channel->s.lock);
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
                    xmSetBit(commPortBitmap, channel->s.receiverPortTab[e], g->partCtrlTab->noCommPorts);
                    portTab[channel->s.receiverPortTab[e]].flags&=~COMM_PORT_MSG_MASK;
                    portTab[channel->s.receiverPortTab[e]].flags|=COMM_PORT_NEW_MSG;
                }
                SetPartitionExtIrqPending(channel->s.receiverTab[e], XM_VT_EXT_SAMPLING_PORT);
            }
        }
	channel->s.length=msgSize;
        channel->s.timestamp=GetSysClockUsec();	
        SpinUnlock(&channel->s.lock);
    }

    return XM_OK;
}

static inline xm_s32_t GetSPortInfo(xmObjDesc_t desc, xmSamplingPortInfo_t *info) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartition(sched->cKThread)->cfg;
    
    if (OBJDESC_GET_PARTITIONID(desc)!=partition->id) 
	return XM_PERM_ERROR;

    if (CheckGParam(info->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(info->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
        info->validPeriod=xmcChannel->s.validPeriod;
	info->maxMsgSize=xmcChannel->s.maxLength;
	info->direction=xmcCommPorts[port].direction;
    } else {
	info->validPeriod=XM_INFINITE_TIME;
	info->maxMsgSize=0;
	info->direction=0;
    }

    return XM_OK;
}

static inline xm_s32_t GetSPortStatus(xmObjDesc_t desc, xmSamplingPortStatus_t *status) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xm_u32_t flags;
    xmId_t partitionId;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    SpinLock(&portTab[port].lock);
    partitionId=portTab[port].partitionId;
    flags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);

    // Reading a port which does not belong to this partition
    if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_SAMPLING_PORT) 
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	status->flags=0;
	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->s.lock);
        status->lastMsgSize=channel->s.length;
        status->timestamp=channel->s.timestamp;
        if (flags&COMM_PORT_NEW_MSG)
            status->flags|=XM_COMM_NEW_MSG;
        else if (flags&COMM_PORT_CONSUMED_MSG) {
            status->flags|=XM_COMM_CONSUMED_MSG;
        } else {
            status->flags|=XM_COMM_EMPTY_PORT;
        }

        if (channel->s.timestamp&&(xmcChannel->s.validPeriod!=XM_INFINITE_TIME)&&((channel->s.timestamp+xmcChannel->s.validPeriod)>GetSysClockUsec()))
	    status->flags|=XM_COMM_MSG_VALID;
        SpinUnlock(&channel->s.lock);
    } else
	memset(status, 0, sizeof(xmSamplingPortStatus_t));
    return XM_OK;
}

static xm_s32_t CtrlSamplingPort(xmObjDesc_t desc, xm_u32_t cmd, union samplingPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union samplingPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) 
        return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_COMM_CREATE_PORT:
	if (!args->create.portName||(CheckGParam(args->create.portName, CONFIG_ID_STRING_LENGTH,1, PFLAG_NOT_NULL)<0)) return XM_INVALID_PARAM;

        return CreateSamplingPort(desc, args->create.portName, args->create.maxMsgSize, args->create.direction, args->create.validPeriod);
    case XM_COMM_GET_PORT_STATUS:
	return GetSPortStatus(desc, &args->status);
    case XM_COMM_GET_PORT_INFO:
	return GetSPortInfo(desc, &args->info);
    }
    return XM_INVALID_PARAM;
}

static const struct object samplingPortObj={
    .Read=(readObjOp_t)ReadSamplingPort,
    .Write=(writeObjOp_t)WriteSamplingPort,
    .Ctrl=(ctrlObjOp_t)CtrlSamplingPort,
};

void ResetPartPorts(partition_t *p) {
    struct xmcPartition *partition;
    xm_s32_t port;

    partition=p->cfg;

    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++) {
        portTab[port].flags&=~COMM_PORT_OPENED;
    }
}



static inline xm_s32_t CreateQueuingPort(xmObjDesc_t desc, xm_s8_t *__gParam portName, xm_s32_t maxNoMsgs, xm_s32_t maxMsgSize, xm_u32_t direction) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct xmcPartition *partition;
    xm_s32_t port;
    xm_u32_t flags;

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
    
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
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
        SpinLock(&channelTab[xmcCommPorts[port].channelId].q.lock);
	if (direction==XM_DESTINATION_PORT) {
            channelTab[xmcCommPorts[port].channelId].q.receiver=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].q.receiverPort=port-partition->commPortsOffset;
	} else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].q.sender=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].q.senderPort=port-partition->commPortsOffset;
        }  
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].q.lock);
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxNoMsgs!=maxNoMsgs)
	    return XM_INVALID_CONFIG;
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].q.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;	
    }
    
    SpinLock(&portTab[port].lock);
    portTab[port].flags|=COMM_PORT_OPENED;
    portTab[port].partitionId=KID2PARTID(sched->cKThread->ctrl.g->id);
    SpinUnlock(&portTab[port].lock);
    return port;
}

static xm_s32_t SendQueuingPort(xmObjDesc_t desc, void *__gParam msgPtr, xm_u32_t msgSize) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc), e;
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    struct msg *msg;
    struct guest *g;
    xmWord_t *commPortBitmap;
    xm_u32_t flags;
    xmId_t partitionId;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;
    
    SpinLock(&portTab[port].lock);
    partitionId=portTab[port].partitionId;
    flags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);

    // Reading a port which does not belong to this partition
    if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    if (xmcCommPorts[port].direction!=XM_SOURCE_PORT)
	return XM_INVALID_PARAM;
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->q.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL)<0) 
            return XM_INVALID_PARAM;

	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->q.lock);
	if (channel->q.usedMsgs<xmcChannel->q.maxNoMsgs) {
    	    if (!(msg=(struct msg *)DynListRemoveTail(&channel->q.freeMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.lock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[SendQueuingPort] Queuing channels internal error");
            }
    	    memcpy(msg->buffer, msgPtr, msgSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsSent++;
            if (sched->cKThread->ctrl.g)
                partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noQueuingPortMsgsSent++;
#endif
	    msg->length=msgSize;
    	    DynListInsertHead(&channel->q.recvMsgs, &msg->listNode);
    	    channel->q.usedMsgs++;
	    
	    if (channel->q.receiver) {
                for (e=0; e<channel->q.receiver->cfg->noVCpus; e++) {
                    g=channel->q.receiver->kThread[e]->ctrl.g;
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);

                    ASSERT_LOCK(channel->q.receiverPort<g->partCtrlTab->noCommPorts, &channel->q.lock);
                    xmSetBit(commPortBitmap, channel->q.receiverPort, g->partCtrlTab->noCommPorts);
                }
                SetPartitionExtIrqPending(channel->q.receiver, XM_VT_EXT_QUEUING_PORT);
            }
            
            for (e=0; e<GetPartition(sched->cKThread)->cfg->noVCpus; e++) {
                g=GetPartition(sched->cKThread)->kThread[e]->ctrl.g;
                commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
                ASSERT_LOCK(channel->q.sender==GetPartition(sched->cKThread), &channel->q.lock);
                xmSetBit(commPortBitmap, channel->q.senderPort, g->partCtrlTab->noCommPorts);
            }
        } else {
            SpinUnlock(&channel->q.lock);
            return XM_OP_NOT_ALLOWED;
        }
        SpinUnlock(&channel->q.lock);
    }

    return XM_OK;
}

static xm_s32_t ReceiveQueuingPort(xmObjDesc_t desc, void *__gParam msgPtr, xm_u32_t msgSize) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    struct xmcCommChannel *xmcChannel;
    union channel *channel;
    xmSize_t retSize=0;
    struct msg *msg;
    xmId_t partitionId;
    xm_u32_t flags;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    SpinLock(&portTab[port].lock);
    partitionId=portTab[port].partitionId;
    flags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);

    // Reading a port which does not belong to this partition
    if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].direction!=XM_DESTINATION_PORT)
	return XM_INVALID_PARAM;
    
    if (!msgSize)
	return XM_INVALID_CONFIG;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];
	if (msgSize>xmcChannel->q.maxLength)
	    return XM_INVALID_CONFIG;

        if (CheckGParam(msgPtr, msgSize, 1, PFLAG_NOT_NULL|PFLAG_RW)<0)
            return XM_INVALID_PARAM;
        
	channel=&channelTab[xmcCommPorts[port].channelId];
        SpinLock(&channel->q.lock);
	if (channel->q.usedMsgs>0) {
	    if (!(msg=(struct msg *)DynListRemoveTail(&channel->q.recvMsgs))) {
                cpuCtxt_t ctxt;
                SpinUnlock(&channel->q.lock);
                GetCpuCtxt(&ctxt);
    		SystemPanic(&ctxt, "[ReceiveQueuingPort] Queuing channels internal error");
            }
	    retSize=(msgSize<msg->length)?msgSize:msg->length;
	    memcpy(msgPtr, msg->buffer, retSize);
#ifdef CONFIG_OBJ_STATUS_ACC
            systemStatus.noQueuingPortMsgsReceived++;
            if (sched->cKThread->ctrl.g)
                partitionStatus[KID2PARTID(sched->cKThread->ctrl.g->id)].noQueuingPortMsgsReceived++;
#endif
	    DynListInsertHead(&channel->q.freeMsgs, &msg->listNode);            
    	    channel->q.usedMsgs--;

            if (channel->q.sender)
                SetPartitionExtIrqPending(channel->q.sender, XM_VT_EXT_QUEUING_PORT);

            if(!channel->q.usedMsgs) {
                xm_s32_t e;
                struct guest *g;
                xmWord_t *commPortBitmap;
                for (e=0; e<GetPartition(sched->cKThread)->cfg->noVCpus; e++) {
                    g=GetPartition(sched->cKThread)->kThread[e]->ctrl.g;
                    commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);

                    ASSERT_LOCK((port-GetPartition(sched->cKThread)->cfg->commPortsOffset)<g->partCtrlTab->noCommPorts, &channel->q.lock);
                    xmClearBit(commPortBitmap, (GetPartition(sched->cKThread)->cfg->commPortsOffset), g->partCtrlTab->noCommPorts);
                }

                if (channel->q.sender) {
                    for (e=0; e<channel->q.sender->cfg->noVCpus; e++) {
                        g=channel->q.sender->kThread[e]->ctrl.g;
                        commPortBitmap=(xmWord_t *)((xmAddress_t)g->partCtrlTab+sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*GetPartition(sched->cKThread)->cfg->noPhysicalMemoryAreas);
                        
                        ASSERT_LOCK(channel->q.senderPort<g->partCtrlTab->noCommPorts, &channel->q.lock);
                        xmClearBit(commPortBitmap, channel->q.senderPort, g->partCtrlTab->noCommPorts);
                    }
                }
            }
        } else{
            SpinUnlock(&channel->q.lock);
            return XM_OP_NOT_ALLOWED;
        }
        SpinUnlock(&channel->q.lock);
    }
    return retSize;      
}

static inline xm_s32_t GetQPortStatus(xmObjDesc_t desc, xmQueuingPortStatus_t *status) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_s32_t port=OBJDESC_GET_ID(desc);
    xmId_t partitionId;
    xm_u32_t flags;

    if (OBJDESC_GET_PARTITIONID(desc)!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    SpinLock(&portTab[port].lock);
    partitionId=portTab[port].partitionId;
    flags=portTab[port].flags;
    SpinUnlock(&portTab[port].lock);

    // Reading a port which does not belong to this partition
    if (partitionId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_INVALID_PARAM;
    
    if (!(flags&COMM_PORT_OPENED))
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].type!=XM_QUEUING_PORT)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
        SpinLock(&channelTab[xmcCommPorts[port].channelId].q.lock);
	status->noMsgs=channelTab[xmcCommPorts[port].channelId].q.usedMsgs;
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].q.lock);
    } else
	memset(status, 0, sizeof(xmSamplingPortStatus_t));
    
    return XM_OK;
}

static inline xm_s32_t GetQPortInfo(xmObjDesc_t desc, xmQueuingPortInfo_t *info) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct xmcPartition *partition;
    struct xmcCommChannel *xmcChannel;
    xm_s32_t port;
    
    partition=GetPartition(sched->cKThread)->cfg;

    if (OBJDESC_GET_PARTITIONID(desc)!=partition->id)
	return XM_PERM_ERROR;
    
    if (CheckGParam(info->portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;
    
    // Look for the channel
    for (port=partition->commPortsOffset; port<(partition->noPorts+partition->commPortsOffset); port++)
	if (!strcmp(info->portName, &xmcStringTab[xmcCommPorts[port].nameOffset])) break;

    if (port>=xmcTab.noCommPorts)
	return XM_INVALID_PARAM;

    if (xmcCommPorts[port].type!=XM_QUEUING_PORT) 
	return XM_INVALID_PARAM;
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	xmcChannel=&xmcCommChannelTab[xmcCommPorts[port].channelId];        
	info->maxNoMsgs=xmcChannel->q.maxNoMsgs;
	info->maxMsgSize=xmcChannel->q.maxLength;
	info->direction=xmcCommPorts[port].direction;
    } else {
        info->maxNoMsgs=0;
	info->maxMsgSize=0;
	info->direction=0;
    }

    return XM_OK;
}

static xm_s32_t CtrlQueuingPort(xmObjDesc_t desc, xm_u32_t cmd, union queuingPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union queuingPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_COMM_CREATE_PORT:
	if (!args->create.portName||(CheckGParam(args->create.portName, CONFIG_ID_STRING_LENGTH, 1, PFLAG_NOT_NULL)<0)) return XM_INVALID_PARAM;
	return CreateQueuingPort(desc, args->create.portName, args->create.maxNoMsgs, args->create.maxMsgSize, args->create.direction);
    case XM_COMM_GET_PORT_STATUS:
	return GetQPortStatus(desc, &args->status);
    case XM_COMM_GET_PORT_INFO:
	return GetQPortInfo(desc, &args->info);
    }
    return XM_INVALID_PARAM;
}

static const struct object queuingPortObj={
    .Read=(readObjOp_t)ReceiveQueuingPort,
    .Write=(writeObjOp_t)SendQueuingPort,    
    .Ctrl=(ctrlObjOp_t)CtrlQueuingPort,
};

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)

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

//     kprintf("[CreaTTnocPort] port=%d - channelId=%d  noChannels=%d\n",port,xmcCommPorts[port].channelId,xmcTab.noCommChannels);
    
    if (xmcCommPorts[port].channelId!=XM_NULL_CHANNEL) {
	ASSERT((xmcCommPorts[port].channelId>=0)&&(xmcCommPorts[port].channelId<xmcTab.noCommChannels));
	if (xmcCommChannelTab[xmcCommPorts[port].channelId].t.maxLength!=maxMsgSize)
	    return XM_INVALID_CONFIG;

//         if (xmcCommChannelTab[xmcCommPorts[port].channelId].t.validPeriod!=validPeriod)
//             return XM_INVALID_CONFIG;
        
        SpinLock(&channelTab[xmcCommPorts[port].channelId].t.lock);
        if (direction==XM_DESTINATION_PORT) {
// 	    kprintf("[CreaTTnocPort]DESTINATION PORT\n");
            ASSERT_LOCK(channelTab[xmcCommPorts[port].channelId].t.noReceivers<xmcCommChannelTab[xmcCommPorts[port].channelId].t.noReceivers, &channelTab[xmcCommPorts[port].channelId].t.lock);
// 	    kprintf("[CreaTTnocPort]phase1\n");
// 	    kprintf("[CreaTTnocPort]%d-%d\n",xmcCommPorts[port].channelId,xmcCommChannelTab[xmcCommPorts[port].channelId].t.noReceivers);
// 	    kprintf("[CreaTTnocPort]phase1-1\n");
            channelTab[xmcCommPorts[port].channelId].t.receiverTab[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=GetPartition(sched->cKThread);
// 	    kprintf("[CreaTTnocPort]phase2\n");
            channelTab[xmcCommPorts[port].channelId].t.receiverPortTab[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=port-partition->commPortsOffset;
// 	    kprintf("[CreaTTnocPort]phase3\n");
//             channelTab[xmcCommPorts[port].channelId].t.nodeId[channelTab[xmcCommPorts[port].channelId].t.noReceivers]=xmcCommChannelTab[xmcCommPorts[port].channelId].t.nodeId;
// 	    kprintf("[CreaTTnocPort]phase4\n");
            channelTab[xmcCommPorts[port].channelId].t.noReceivers++;
// 	    kprintf("[CreaTTnocPort]phase5\n");
        } else { // XM_SOURCE_PORT
            channelTab[xmcCommPorts[port].channelId].t.sender=GetPartition(sched->cKThread);
            channelTab[xmcCommPorts[port].channelId].t.senderPort=port-partition->commPortsOffset;
        }
        SpinUnlock(&channelTab[xmcCommPorts[port].channelId].t.lock);
    }

    SpinLock(&portTab[port].lock);
    portTab[port].flags|=COMM_PORT_OPENED|COMM_PORT_EMPTY;
    portTab[port].partitionId=KID2PARTID(sched->cKThread->ctrl.g->id);
//     kprintf("[CreaTTnocPort] ttnocDev=%d\n",portTab[port].ttnocDev->subId);
    if (xmcCommPorts[port].devId.id==XM_DEV_TTNOC_ID){
      portTab[port].ttnocDev=(kDevice_t *)LookUpKDev(&xmcCommPorts[port].devId);
    }
    else{
       portTab[port].ttnocDev=NULL;
       PWARN("[CreateTTnocPort] Invalid assignation of DEV\n");
//        return XM_INVALID_CONFIG;
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

	if (KDevRead(portTab[port].ttnocDev,msgPtr,retSize)<0)
	   return XM_OP_NOT_ALLOWED;
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
// 	if (flags) {
// 	    *flags=0;
//             if (retSize&&(xmcChannel->t.validPeriod!=XM_INFINITE_TIME)&&(channel->t.timestamp+xmcChannel->t.validPeriod)>GetSysClockUsec())
// 		*flags=XM_COMM_MSG_VALID;
// 	}
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

    if (xmcCommPorts[port].type!=XM_TTNOC_PORT) 
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
        if (KDevWrite(portTab[port].ttnocDev,msgPtr,msgSize)<0)
	   return XM_OP_NOT_ALLOWED;
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

static xm_s32_t CtrlTTnocPort(xmObjDesc_t desc, xm_u32_t cmd, union ttnocPortCmd *__gParam args) {
    if (CheckGParam(args, sizeof(union ttnocPortCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0)
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

#endif

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
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
	case XM_TTNOC_CHANNEL:
// 	    GET_MEMZ(channelTab[e].t.buffer, xmcCommChannelTab[e].t.maxLength);
            GET_MEMZ(channelTab[e].t.receiverTab, xmcCommChannelTab[e].t.noReceivers*sizeof(partition_t *));
            GET_MEMZ(channelTab[e].t.receiverPortTab, xmcCommChannelTab[e].t.noReceivers*sizeof(xm_s32_t));
            channelTab[e].t.lock=SPINLOCK_INIT;
	    break;
#endif
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
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    objectTab[OBJ_CLASS_TTNOC_PORT]=&ttnocPortObj;
#endif
    return 0;
}

REGISTER_OBJ(SetupComm);
