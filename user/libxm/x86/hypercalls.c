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

//#include <xmhypercalls.h>
#include <xm_inc/hypercalls.h>
#include <hypervisor.h>

xm_hcall0r(halt_system);
xm_hcall1r(reset_system, xm_u32_t, resetMode);
xm_hcall1r(halt_partition, xm_u32_t, partitionId);
xm_hcall1r(suspend_partition, xm_u32_t, partitionId);
xm_hcall1r(resume_partition, xm_u32_t, partitionId);
xm_hcall3r(reset_partition, xm_u32_t, partitionId, xm_u32_t, resetMode, xm_u32_t, status);
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

xm_hcall2r(switch_sched_plan, xm_u32_t, newPlanId, xm_u32_t *, currentPlanId);
xm_hcall2r(get_gid_by_name, xm_u8_t *, name, xm_u32_t, entity);

xm_hcall1r(x86_load_cr0, xmWord_t, val);
xm_hcall1r(x86_load_cr3, xmWord_t, val);
xm_hcall1r(x86_load_cr4, xmWord_t, val);
xm_hcall1r(x86_load_tss, struct x86Tss *, t);
xm_hcall1r(x86_load_gdt, struct x86DescReg *, desc);
xm_hcall1r(x86_load_idtr, struct x86DescReg *, desc);
xm_hcall3r(x86_update_ss_sp, xmWord_t, ss, xmWord_t, sp, xm_u32_t, level);
xm_hcall2r(x86_update_gdt, xm_s32_t, entry, struct x86Desc *, gdt);
xm_hcall2r(x86_update_idt, xm_s32_t, entry, struct x86Gate *, desc);

xm_hcall0(x86_set_if);
xm_hcall0(x86_clear_if);

xm_hcall2r(override_trap_hndl, xm_s32_t, entry, struct trapHandler *, handler);
xm_hcall1r(invld_tlb, xmAddress_t, vAddr);

xm_lazy_hcall1(x86_load_cr0, xmWord_t, val);
xm_lazy_hcall1(x86_load_cr3, xmWord_t, val);
xm_lazy_hcall1(x86_load_cr4, xmWord_t, val);
xm_lazy_hcall1(invld_tlb, xmAddress_t, vAddr);
xm_lazy_hcall1(x86_load_tss, struct x86Tss *, t);
xm_lazy_hcall1(x86_load_gdt, struct x86DescReg *, desc);
xm_lazy_hcall1(x86_load_idtr, struct x86DescReg *, desc);
xm_lazy_hcall3(x86_update_ss_sp, xmWord_t, ss, xmWord_t, sp, xm_u32_t, level);
xm_lazy_hcall2(x86_update_gdt, xm_s32_t, entry, struct x86Desc *, gdt);
xm_lazy_hcall2(x86_update_idt, xm_s32_t, entry, struct x86Gate *, desc);

__stdcall xm_s32_t XM_multicall(void *startAddr, void *endAddr) {
    xm_s32_t _r;
    _XM_HCALL2(startAddr, endAddr, multicall_nr, _r);
    return _r;
}

__stdcall xm_s32_t XM_set_timer(xm_u32_t clock_id, xmTime_t abstime, xmTime_t interval) {
    xm_s32_t _r;
    _XM_HCALL5(clock_id, abstime, abstime>>32, interval, interval>>32, set_timer_nr, _r);
    return _r;
}

void XM_x86_iret(void) {
    __asm__ __volatile__("lcall $("TO_STR(XM_IRET_CALLGATE_SEL)"), $0x0\n\t");
}
