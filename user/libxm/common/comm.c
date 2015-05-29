/*
 * $FILE: comm.c
 *
 * Communication wrappers
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
#include <comm.h>
#include <hypervisor.h>
#include <xmhypercalls.h>

#include <xm_inc/hypercalls.h>

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_s32_t XM_create_ttnoc_port(char *portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_TTNOC_PORT, XM_PARTITION_SELF, 0);
    union ttnocPortCmd cmd;
    xm_s32_t id;

    cmd.create.portName=portName;
    cmd.create.maxMsgSize=maxMsgSize;
    cmd.create.direction=direction;
    cmd.create.validPeriod=validPeriod;

    id=XM_ctrl_object(desc, XM_COMM_CREATE_PORT, &cmd);
    return id;
}

xm_s32_t XM_read_ttnoc_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize, xm_u32_t *flags) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_TTNOC_PORT, XM_PARTITION_SELF, portId);
    return XM_read_object(desc, msgPtr, msgSize, flags);
}

xm_s32_t XM_write_ttnoc_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_TTNOC_PORT, XM_PARTITION_SELF, portId);
    return XM_write_object(desc, msgPtr, msgSize, 0);
}
#endif

xm_s32_t XM_create_sampling_port(char *portName, xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, 0);
    union samplingPortCmd cmd;
    xm_s32_t id;

    cmd.create.portName=portName;
    cmd.create.maxMsgSize=maxMsgSize;
    cmd.create.direction=direction;
    cmd.create.validPeriod=validPeriod;

    id=XM_ctrl_object(desc, XM_COMM_CREATE_PORT, &cmd);
    return id;
}

xm_s32_t XM_read_sampling_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize, xm_u32_t *flags) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, portId);
    return XM_read_object(desc, msgPtr, msgSize, flags);
}

xm_s32_t XM_write_sampling_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, portId);
    return XM_write_object(desc, msgPtr, msgSize, 0);
}

xm_s32_t XM_create_queuing_port(char *portName, xm_u32_t maxNoMsgs, xm_u32_t maxMsgSize, xm_u32_t direction) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, 0);
    union queuingPortCmd cmd;
    xm_s32_t id;

    cmd.create.portName=portName;
    cmd.create.maxNoMsgs=maxNoMsgs;
    cmd.create.maxMsgSize=maxMsgSize;
    cmd.create.direction=direction;

    id=XM_ctrl_object(desc, XM_COMM_CREATE_PORT, &cmd);
    return id;
}

xm_s32_t XM_send_queuing_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, portId);
    
    return XM_write_object(desc, msgPtr, msgSize, 0);
}

xm_s32_t XM_receive_queuing_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, portId);
    return XM_read_object(desc, msgPtr, msgSize, 0);
}

xm_s32_t XM_get_queuing_port_status(xm_u32_t portId, xmQueuingPortStatus_t *status) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, portId);
    if (!status)
	return XM_INVALID_PARAM;

    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_STATUS, status)!=XM_OK)
	return XM_INVALID_PARAM;

    return XM_OK;
}

xm_s32_t XM_get_queuing_port_info(char *portName, xmQueuingPortInfo_t *info) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, 0);
    if (!info)
	return XM_INVALID_PARAM;

    info->portName=portName;
    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_INFO, (union queuingPortCmd *)info)!=XM_OK)
	return XM_INVALID_PARAM;
    return XM_OK;
}


xm_s32_t XM_get_sampling_port_status(xm_u32_t portId, xmSamplingPortStatus_t *status) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_QUEUING_PORT, XM_PARTITION_SELF, 0);
    if (!status)
	return XM_INVALID_PARAM;

    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_STATUS, status)!=XM_OK)
	return XM_INVALID_PARAM;
    return XM_OK;
}

xm_s32_t XM_get_sampling_port_info(char *portName, xmSamplingPortInfo_t *info) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_SAMPLING_PORT, XM_PARTITION_SELF, 0);
    if (!info)
	return XM_INVALID_PARAM;

    info->portName=portName;
    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_INFO, (union samplingPortCmd *)info)!=XM_OK)
	return XM_INVALID_PARAM;
    return XM_OK;
}
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_s32_t XM_get_ttnoc_port_status(xm_u32_t portId, xmTTnocPortStatus_t *status) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_TTNOC_PORT, XM_PARTITION_SELF, 0);
    if (!status)
        return XM_INVALID_PARAM;

    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_STATUS, status)!=XM_OK)
        return XM_INVALID_PARAM;
    return XM_OK;
}

xm_s32_t XM_get_ttnoc_port_info(char *portName, xmTTnocPortInfo_t *info) {
    xmObjDesc_t desc=OBJDESC_BUILD(OBJ_CLASS_TTNOC_PORT, XM_PARTITION_SELF, 0);
    if (!info)
        return XM_INVALID_PARAM;

    info->portName=portName;
    if (XM_ctrl_object(desc, XM_COMM_GET_PORT_INFO, (union ttnocPortCmd *)info)!=XM_OK)
        return XM_INVALID_PARAM;
    return XM_OK;
}
#endif



