/*
 * $FILE: trace.c
 *
 * Tracing functionality
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/trace.h>

xm_s32_t XM_trace_open(xmId_t id) {
    xm_s32_t noPartitions;
    if (id!=XM_PARTITION_SELF)
	if (!(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags&XM_PART_SYSTEM))
	    return XM_PERM_ERROR;

    noPartitions=(libXmParams.partCtrlTab[XM_get_vcpuid()]->flags>>16)&0xff;
    if ((id!=XM_HYPERVISOR_ID)&&((id<0)||(id>=noPartitions)))
        return XM_INVALID_PARAM;
    return OBJDESC_BUILD(OBJ_CLASS_TRACE, id, 0);
}

xm_s32_t XM_trace_event(xm_u32_t bitmask, xmTraceEvent_t *event) {
    if (!event)
	return XM_INVALID_PARAM;
    if (XM_write_object(OBJDESC_BUILD(OBJ_CLASS_TRACE, XM_PARTITION_SELF, 0), event, sizeof(xmTraceEvent_t), &bitmask)<sizeof(xmTraceEvent_t))
	return XM_INVALID_CONFIG;
    return XM_OK;
}

xm_s32_t XM_trace_read(xm_s32_t traceStream, xmTraceEvent_t *traceEventPtr) {
    xm_s32_t ret;
    if (OBJDESC_GET_CLASS(traceStream)!=OBJ_CLASS_TRACE)
        return XM_INVALID_PARAM;
    if (!traceEventPtr)
	return XM_INVALID_PARAM;
    
    ret=XM_read_object(traceStream, traceEventPtr, sizeof(xmTraceEvent_t), 0);
    return (ret>0)?(ret/sizeof(xmTraceEvent_t)):ret;
}

xm_s32_t XM_trace_seek(xm_s32_t traceStream, xm_s32_t offset, xm_u32_t whence) {
    if (OBJDESC_GET_CLASS(traceStream)!=OBJ_CLASS_TRACE)
        return XM_INVALID_PARAM;

    return XM_seek_object(traceStream, offset, whence);
}

xm_s32_t XM_trace_status(xm_s32_t traceStream, xmTraceStatus_t *traceStatusPtr) {
    if (OBJDESC_GET_CLASS(traceStream)!=OBJ_CLASS_TRACE)
        return XM_INVALID_PARAM;

    if (!traceStatusPtr)
	return XM_INVALID_PARAM;
    return XM_ctrl_object(traceStream, XM_TRACE_GET_STATUS, traceStatusPtr);
}
