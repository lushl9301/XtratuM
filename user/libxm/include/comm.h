/*
 * $FILE: comm.h
 *
 * Comm ports
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_COMM_H_
#define _LIB_XM_COMM_H_

#include <xm_inc/config.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/commports.h>

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
// TTNoC port related functions
extern xm_s32_t XM_create_ttnoc_port(char *portName,  xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod);
extern xm_s32_t XM_read_ttnoc_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize, xm_u32_t *flags);
extern xm_s32_t XM_write_ttnoc_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize);
extern xm_s32_t XM_get_ttnoc_port_status(xm_u32_t portId, xmTTnocPortStatus_t *status);
extern xm_s32_t XM_get_ttnoc_port_info(char *portName, xmTTnocPortInfo_t *info);
#endif

// Sampling port related functions
extern xm_s32_t XM_create_sampling_port(char *portName,  xm_u32_t maxMsgSize, xm_u32_t direction, xmTime_t validPeriod);
extern xm_s32_t XM_read_sampling_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize, xm_u32_t *flags);
extern xm_s32_t XM_write_sampling_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize);
extern xm_s32_t XM_get_sampling_port_status(xm_u32_t portId, xmSamplingPortStatus_t *status);
extern xm_s32_t XM_get_sampling_port_info(char *portName, xmSamplingPortInfo_t *info);

// Queuing port related functions
extern xm_s32_t XM_create_queuing_port(char *portName, xm_u32_t maxNoMsgs, xm_u32_t maxMsgSize,  xm_u32_t direction);
extern xm_s32_t XM_send_queuing_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize);
extern xm_s32_t XM_receive_queuing_message(xm_s32_t portId, void *msgPtr, xm_u32_t msgSize);
extern xm_s32_t XM_get_queuing_port_status(xm_u32_t portId, xmQueuingPortStatus_t *status);
extern xm_s32_t XM_get_queuing_port_info(char *portName, xmQueuingPortInfo_t *info);

#endif
