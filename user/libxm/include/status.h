/*
 * $FILE: trace.h
 *
 * tracing subsystem
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_STATUS_H_
#define _LIB_XM_STATUS_H_

#include <xm_inc/config.h>
#include <xm_inc/objdir.h>
#include <xm_inc/objects/status.h>

extern xm_s32_t XM_get_partition_status(xmId_t id, xmPartitionStatus_t *status);
extern xm_s32_t XM_get_system_status (xmSystemStatus_t  *status);
extern xm_s32_t XM_get_vcpu_status(xmId_t vCpu, xmVirtualCpuStatus_t *status);
extern xm_s32_t XM_set_partition_opmode(xm_s32_t opMode);
extern xm_s32_t XM_get_plan_status(xmPlanStatus_t *status);
extern xm_s32_t XM_get_physpage_status(xmAddress_t pAddr, xmPhysPageStatus_t *status);

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_s32_t XM_get_system_node_status(xmId_t nodeId,xmSystemStatusRemote_t * status);
xm_s32_t XM_get_partition_node_status(xmId_t nodeId,xmId_t id,xmPartitionStatusRemote_t * status);
#endif

/*only for testing*/
extern xm_s32_t XM_get_partition_vcpu_status(xmId_t id, xmId_t vCpu, xmVirtualCpuStatus_t *status);
#endif
