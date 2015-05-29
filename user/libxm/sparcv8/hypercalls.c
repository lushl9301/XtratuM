/*
 * $FILE: hypercalls.c
 *
 * XM system calls definitions
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

xm_hcall0r(halt_system);
xm_hcall1r(reset_system, xm_u32_t, resetMode);
xm_hcall1r(halt_partition, xm_u32_t, partitionId);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_hcall1r(halt_system_node,xm_u32_t, nodeId);
xm_hcall2r(reset_system_node,xm_u32_t, nodeId, xm_u32_t, resetMode);
#endif
xm_hcall1r(suspend_partition, xm_u32_t, partitionId);
xm_hcall1r(resume_partition, xm_u32_t, partitionId);
xm_hcall3r(reset_partition, xm_u32_t, partitionId, xm_u32_t, resetMode, xm_u32_t, status);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_hcall3r(reset_partition_node, xm_u32_t, nodeId, xm_u32_t, partitionId, xm_u32_t, resetMode);
xm_hcall2r(halt_partition_node, xm_u32_t, nodeId, xm_u32_t, partition_id);
#endif


xm_hcall1r(halt_vcpu, xm_u32_t, vcpuId);
xm_hcall1r(suspend_vcpu, xm_u32_t, vcpuId);
xm_hcall1r(resume_vcpu, xm_u32_t, vcpuId);
xm_hcall4r(reset_vcpu, xm_u32_t, vcpuId, xmAddress_t, ptdL1, xmAddress_t, entry, xm_u32_t, status);

__stdcall xmId_t XM_get_vcpuid(void) {
    xm_s32_t _r;
    _XM_HCALL0(get_vcpuid_nr, _r);
    return _r;
}
xm_hcall1r(shutdown_partition, xm_u32_t, partitionId);
xm_hcall0r(idle_self);
xm_hcall4r(read_object, xmObjDesc_t, objDesc, void *, buffer, xm_u32_t, size, xm_u32_t *, flags);
xm_hcall4r(write_object, xmObjDesc_t, objDesc, void *, buffer, xm_u32_t, size, xm_u32_t *, flags);
xm_hcall3r(ctrl_object, xmObjDesc_t, objDesc, xm_u32_t, cmd, void *, arg);
xm_hcall3r(seek_object, xmObjDesc_t, traceStream, xm_u32_t, offset, xm_u32_t, whence);
xm_hcall2r(get_time, xm_u32_t, clock_id, xmTime_t *, time);

xm_hcall2r(clear_irqmask, xm_u32_t, hwIrqsMask, xm_u32_t, extIrqsPend);
xm_hcall2r(set_irqmask, xm_u32_t, hwIrqsMask, xm_u32_t, extIrqsPend);
xm_hcall2r(set_irqpend, xm_u32_t, hwIrqMask, xm_u32_t, extIrqMask);
xm_hcall2r(clear_irqpend, xm_u32_t, hwIrqMask, xm_u32_t, extIrqMask);
xm_hcall3r(route_irq, xm_u32_t, type, xm_u32_t, irq, xm_u16_t, vector);
xm_hcall1r(raise_ipvi, xm_u8_t, no_ipvi);
xm_hcall2r(raise_partition_ipvi, xm_u32_t, partitionId, xm_u8_t, no_ipvi);

xm_hcall2r(update_page32, xmAddress_t, pAddr, xm_u32_t, val);
xm_hcall2r(set_page_type, xmAddress_t, pAddr, xm_u32_t, type);

xm_hcall1r(flush_cache, xm_u32_t, cache);
xm_hcall1r(set_cache_state, xm_u32_t, cache);
xm_hcall2r(switch_sched_plan, xm_u32_t, newPlanId, xm_u32_t *, currentPlanId);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
xm_hcall3r(switch_sched_plan_node, xm_u32_t, nodeId, xm_u32_t, newPlanId, xm_u32_t *, currentPlanId);
#endif
xm_hcall2r(get_gid_by_name, xm_u8_t *, name, xm_u32_t, entity);
xm_hcall2r(sparc_atomic_and, xm_u32_t *, at, xm_u32_t, val);
xm_hcall2r(sparc_atomic_or, xm_u32_t *, at, xm_u32_t, val);
xm_hcall2r(sparc_atomic_add, xm_u32_t *, at, xm_u32_t, val);

__stdcall xm_s32_t XM_set_timer(xm_u32_t clock_id, xmTime_t abstime, xmTime_t interval) {
    xm_s32_t _r ;
    _XM_HCALL5(clock_id, abstime, abstime<<32, interval, interval<<32, set_timer_nr, _r);
    return _r;
}

xm_hcall2r(sparc_inport, xm_u32_t, port, xm_u32_t *, value);
xm_hcall2r(sparc_outport, xm_u32_t, port, xm_u32_t, value);
xm_hcall1(sparc_write_tbr, xmWord_t, val);
xm_hcall1(sparc_write_ptdl1, xmWord_t, val);

__stdcall xm_s32_t XM_multicall(void *startAddr, void *endAddr) {
    xm_s32_t _r;
    _XM_HCALL2(startAddr, endAddr, multicall_nr, _r);
    return _r;
}

