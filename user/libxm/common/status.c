/*
 * $FILE: status.c
 *
 * Status functionality
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
#include <status.h>
#include <xm_inc/objects/status.h>

xm_s32_t XM_get_system_status(xmSystemStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_HYPERVISOR_ID, 0), XM_GET_SYSTEM_STATUS, status);
}

xm_s32_t XM_get_partition_status(xmId_t id, xmPartitionStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, id, 0), XM_GET_SYSTEM_STATUS, status);
}

xm_s32_t XM_get_vcpu_status(xmId_t vCpu, xmVirtualCpuStatus_t *status) {
    if (vCpu>XM_get_number_vcpus())
       return XM_INVALID_PARAM;
    return XM_ctrl_object(OBJDESC_BUILD_VCPUID(OBJ_CLASS_STATUS, vCpu,XM_PARTITION_SELF, 0), XM_GET_VCPU_STATUS, status);
}

xm_s32_t XM_set_partition_opmode(xm_s32_t opMode) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_PARTITION_SELF, 0), XM_SET_PARTITION_OPMODE, &opMode);
}

/*only for testing*/
xm_s32_t XM_get_partition_vcpu_status(xmId_t id, xmId_t vCpu, xmVirtualCpuStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD_VCPUID(OBJ_CLASS_STATUS, vCpu, id, 0), XM_GET_VCPU_STATUS, status);
}

xm_s32_t XM_get_plan_status(xmPlanStatus_t *status) {
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_PARTITION_SELF, 0), XM_GET_SCHED_PLAN_STATUS, status);
}

xm_s32_t XM_get_physpage_status(xmAddress_t pAddr, xmPhysPageStatus_t *status) {
    status->pAddr=pAddr;
    return XM_ctrl_object(OBJDESC_BUILD(OBJ_CLASS_STATUS, XM_PARTITION_SELF, 0), XM_GET_PHYSPAGE_STATUS, status);
}
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_s32_t XM_get_system_node_status(xmId_t nodeId,xmSystemStatusRemote_t * status){
    if (nodeId>CONFIG_TTNOC_NODES)
       return XM_INVALID_PARAM;
    return XM_ctrl_object(OBJDESC_BUILD_NODEID(OBJ_CLASS_STATUS, nodeId,XM_PARTITION_SELF, 0), XM_GET_NODE_STATUS, status);
}

xm_s32_t XM_get_partition_node_status(xmId_t nodeId,xmId_t id,xmPartitionStatusRemote_t * status){
    if (nodeId>CONFIG_TTNOC_NODES)
       return XM_INVALID_PARAM;
    return XM_ctrl_object(OBJDESC_BUILD_NODEID(OBJ_CLASS_STATUS, nodeId,id, 0), XM_GET_NODE_PARTITION, status);
}
#endif

