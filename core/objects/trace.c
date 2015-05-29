/*
 * $FILE: trace.c
 *
 * Tracing mechanism
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
#include <checksum.h>
#include <hypercalls.h>
#include <kthread.h>
#include <kdevice.h>
#include <stdc.h>
#include <xmconf.h>
#include <sched.h>
#include <objects/trace.h>
#include <objects/hm.h>
#include <logstream.h>

static struct logStream *traceLogStream, xmTraceLogStream;
static xm_u32_t seq=0;

static xm_s32_t ReadTrace(xmObjDesc_t desc, xmTraceEvent_t *__gParam event, xm_u32_t size) {
    xm_s32_t e, noTraces=size/sizeof(xmTraceEvent_t);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct logStream *log;
    xmId_t partId;
    
    partId=OBJDESC_GET_PARTITIONID(desc);
    if (partId!=KID2PARTID(sched->cKThread->ctrl.g->id))
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;

    if (!noTraces)
	return XM_INVALID_PARAM;

    if (CheckGParam(event, size, 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;

    if (partId==XM_HYPERVISOR_ID)
	log=&xmTraceLogStream;
    else {
	if ((partId<0)||(partId>=xmcTab.noPartitions))
	    return XM_INVALID_PARAM;
	log=&traceLogStream[partId];
    }

    for (e=0; e<noTraces; e++)
	if (LogStreamGet(log, &event[e])<0)
	    return e*sizeof(xmTraceEvent_t);

    return noTraces*sizeof(xmTraceEvent_t);
}

static xm_s32_t WriteTrace(xmObjDesc_t desc, xmTraceEvent_t *__gParam event, xm_u32_t size, xm_u32_t *bitmap) {
    xm_s32_t e, noTraces=size/sizeof(xmTraceEvent_t), written;
    localSched_t *sched=GET_LOCAL_SCHED();
    struct logStream *log;
    xmId_t partId;
    partId=OBJDESC_GET_PARTITIONID(desc);
    if (partId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;
    
    if (!noTraces)
	return XM_INVALID_PARAM;

    if (CheckGParam(event, size, 4, PFLAG_NOT_NULL)<0) return XM_INVALID_PARAM;
    if (CheckGParam(bitmap, sizeof(xm_u32_t), 4, PFLAG_NOT_NULL)<0) return XM_INVALID_PARAM;
    log=&traceLogStream[partId];
    for (written=0, e=0; e<noTraces; e++) {
        if (bitmap&&(GetPartition(sched->cKThread)->cfg->trace.bitmap&*bitmap)) {
            xm_u32_t tmpSeq;
            event[e].signature=XMTRACE_SIGNATURE;
            event[e].opCodeL&=~TRACE_OPCODE_PARTID_MASK;
	    event[e].opCodeL|=(KID2PARTID(sched->cKThread->ctrl.g->id)<<TRACE_OPCODE_PARTID_BIT)&TRACE_OPCODE_PARTID_MASK;
            event[e].opCodeL&=~TRACE_OPCODE_VCPUID_MASK;
	    event[e].opCodeL|=(KID2VCPUID(sched->cKThread->ctrl.g->id)<<TRACE_OPCODE_VCPUID_BIT)&TRACE_OPCODE_VCPUID_MASK;                        
	    event[e].timestamp=GetSysClockUsec();
            tmpSeq=seq++;
            event[e].opCodeH&=~TRACE_OPCODE_SEQ_MASK;
            event[e].opCodeH|=tmpSeq<<TRACE_OPCODE_SEQ_BIT;
            event[e].checksum=0;
            event[e].checksum=CalcCheckSum((xm_u16_t *)&event[e], sizeof(struct xmTraceEvent));
            LogStreamInsert(log, &event[e]);
	    written++;
	}
        if (((event->opCodeH&TRACE_OPCODE_CRIT_MASK)>>TRACE_OPCODE_CRIT_BIT)==XM_TRACE_UNRECOVERABLE) {
	    xmHmLog_t log;
            log.opCodeL=
                (KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT)|(KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT)|(XM_HM_EV_PARTITION_ERROR<<HMLOG_OPCODE_EVENT_BIT);	    
            memcpy(log.payload, event->payload, sizeof(xmWord_t)*XM_HMLOG_PAYLOAD_LENGTH-1);

	    HmRaiseEvent(&log);
	}
    }

    return written*sizeof(xmTraceEvent_t);
}

static xm_s32_t SeekTrace(xmObjDesc_t desc, xm_u32_t offset, xm_u32_t whence) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct logStream *log;
    xmId_t partId;
    
    partId=OBJDESC_GET_PARTITIONID(desc);
    if (partId!=KID2PARTID(sched->cKThread->ctrl.g->id))
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;

    if (partId==XM_HYPERVISOR_ID)
	log=&xmTraceLogStream;
    else {
	if ((partId<0)||(partId>=xmcTab.noPartitions))
	    return XM_INVALID_PARAM;
	log=&traceLogStream[partId];
    }

    return LogStreamSeek(log, offset, whence);
}

static xm_s32_t CtrlTrace(xmObjDesc_t desc, xm_u32_t cmd, union traceCmd *__gParam args) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct logStream *log;
    xmId_t partId;
    
    partId=OBJDESC_GET_PARTITIONID(desc);
    if (partId!=KID2PARTID(sched->cKThread->ctrl.g->id))
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;
    if (CheckGParam(args, sizeof(union traceCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) 
	return XM_INVALID_PARAM;
    if (partId==XM_HYPERVISOR_ID)
	log=&xmTraceLogStream;
    else {
	if ((partId<0)||(partId>=xmcTab.noPartitions))
	    return XM_INVALID_PARAM;
	log=&traceLogStream[partId];
    }

    switch(cmd) {
    case XM_TRACE_GET_STATUS:
	args->status.noEvents=log->ctrl.elem;
	args->status.maxEvents=log->info.maxNoElem;
	args->status.currentEvent=log->ctrl.d;
	return XM_OK;
    case XM_TRACE_LOCK:
        LogStreamLock(log);
        return XM_OK;
    case XM_TRACE_UNLOCK:
        LogStreamUnlock(log);
        return XM_OK;
    case XM_TRACE_RESET:
        if (partId==XM_HYPERVISOR_ID)
            LogStreamInit(log, LookUpKDev(&xmcTab.hpv.trace.dev), sizeof(xmTraceEvent_t));
        else
            LogStreamInit(log, LookUpKDev(&xmcPartitionTab[partId].trace.dev), sizeof(xmTraceEvent_t));
        return XM_OK;
    }
    return XM_INVALID_PARAM;
}

static const struct object traceObj={
    .Read=(readObjOp_t)ReadTrace,
    .Write=(writeObjOp_t)WriteTrace,
    .Seek=(seekObjOp_t)SeekTrace,
    .Ctrl=(ctrlObjOp_t)CtrlTrace,
};

xm_s32_t __VBOOT SetupTrace(void) {
    xm_s32_t e;
    GET_MEMZ(traceLogStream, sizeof(struct logStream)*xmcTab.noPartitions);
    LogStreamInit(&xmTraceLogStream, LookUpKDev(&xmcTab.hpv.trace.dev), sizeof(xmTraceEvent_t));
    
    for (e=0; e<xmcTab.noPartitions; e++)
	LogStreamInit(&traceLogStream[e], LookUpKDev(&xmcPartitionTab[e].trace.dev), sizeof(xmTraceEvent_t));
    
    objectTab[OBJ_CLASS_TRACE]=&traceObj;
    
    //TraceHypEvent(TRACE_EV_HYP_AUDIT_INIT);
    
    return 0;
}

xm_s32_t TraceWriteSysEvent(xm_u32_t bitmap, xmTraceEvent_t *event) {
    ASSERT(event);
    
    if (xmcTab.hpv.trace.bitmap&bitmap) {
        xm_u32_t tmpSeq;
        event->signature=XMTRACE_SIGNATURE;
	event->timestamp=GetSysClockUsec();

        tmpSeq=seq++;
        event->opCodeH&=~TRACE_OPCODE_SEQ_MASK;
        event->opCodeH|=tmpSeq<<TRACE_OPCODE_SEQ_BIT;

        event->checksum=0;
        event->checksum=CalcCheckSum((xm_u16_t *)event, sizeof(struct xmTraceEvent)); 
        return LogStreamInsert(&xmTraceLogStream, event);
    }

    return -2;
}

REGISTER_OBJ(SetupTrace);

#ifdef CONFIG_AUDIT_EVENTS
xm_s32_t IsAuditEventMasked(xm_u32_t module) {
    if (xmcTab.hpv.trace.bitmap&(1<<module))
        return 1;
    return 0;
}

void RaiseAuditEvent(xm_u32_t module, xm_u32_t event, xm_s32_t payloadLen, xmWord_t *payload) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmTraceEvent_t trace;
    ASSERT((module>=0)&&(module<TRACE_NO_MODULES));
    ASSERT(payloadLen<=XMTRACE_PAYLOAD_LENGTH);
    if (xmcTab.hpv.trace.bitmap&(1<<module)) {
        xmId_t partId, vCpuId;
        if (sched->cKThread->ctrl.g) {
            partId=KID2PARTID(sched->cKThread->ctrl.g->id);
            vCpuId=KID2VCPUID(sched->cKThread->ctrl.g->id);
        } else {
            partId=-1;
            vCpuId=-1;
        }
        
        trace.opCodeL=((((module<<8)|event)<<TRACE_OPCODE_CODE_BIT)&TRACE_OPCODE_CODE_MASK)|((partId<<TRACE_OPCODE_PARTID_BIT)&TRACE_OPCODE_PARTID_MASK)|
            ((vCpuId<<TRACE_OPCODE_VCPUID_BIT)&TRACE_OPCODE_VCPUID_MASK);

        trace.opCodeH=XM_TRACE_NOTIFY<<TRACE_OPCODE_CRIT_BIT|
            TRACE_OPCODE_SYS_MASK;

        if (payloadLen)
            memcpy(trace.payload, payload, sizeof(xm_u32_t)*payloadLen);
        TraceWriteSysEvent((1<<module), &trace);
    }
}
#endif
