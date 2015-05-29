/*
 * $FILE: xmhypercalls.h
 *
 * Generic hypercall definition
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_HYPERCALLS_H_
#define _LIB_XM_HYPERCALLS_H_

#ifdef _XM_KERNEL_
#error Guest file, do not include.
#endif

#include <xm_inc/config.h>
#include <xm_inc/linkage.h>
#include <xm_inc/hypercalls.h>
#include <xm_inc/arch/irqs.h>
#include <xm_inc/arch/linkage.h>
#include <arch/xmhypercalls.h>

#ifndef __ASSEMBLY__
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/objdir.h>

/* <track id="hypercall-list"> */
extern __stdcall xm_s32_t XM_get_gid_by_name(xm_u8_t *name, xm_u32_t entity);
extern __stdcall xmId_t XM_get_vcpuid(void);

// Time management hypercalls
extern __stdcall xm_s32_t XM_get_time(xm_u32_t clock_id, xmTime_t *time);
extern __stdcall xm_s32_t XM_set_timer(xm_u32_t clock_id, xmTime_t abstime, xmTime_t interval);

// Partition status hypercalls
extern __stdcall xm_s32_t XM_suspend_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_resume_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_shutdown_partition(xm_u32_t partition_id);
extern __stdcall xm_s32_t XM_reset_partition(xm_u32_t partition_id, xm_u32_t resetMode, xm_u32_t status);
extern __stdcall xm_s32_t XM_halt_partition(xm_u32_t partition_id);
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern __stdcall xm_s32_t XM_reset_partition_node(xm_u32_t nodeId, xm_u32_t partition_id, xm_u32_t resetMode);
extern __stdcall xm_s32_t XM_halt_partition_node(xm_u32_t nodeId, xm_u32_t partition_id);
#endif
extern __stdcall xm_s32_t XM_idle_self(void);
extern __stdcall xm_s32_t XM_suspend_vcpu(xm_u32_t vcpu_id);
extern __stdcall xm_s32_t XM_resume_vcpu(xm_u32_t vcpu_id);
extern __stdcall xm_s32_t XM_reset_vcpu(xm_u32_t vcpu_id, xmAddress_t ptdL1, xmAddress_t entry, xm_u32_t status);
extern __stdcall xm_s32_t XM_halt_vcpu(xm_u32_t vcpu_id);

// system status hypercalls
extern __stdcall xm_s32_t XM_halt_system(void);
extern __stdcall xm_s32_t XM_reset_system(xm_u32_t resetMode);

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern __stdcall xm_s32_t XM_halt_system_node(xm_u32_t nodeId);
extern __stdcall xm_s32_t XM_reset_system_node(xm_u32_t nodeId, xm_u32_t resetMode);
#endif

// Object related hypercalls
//@ \void{<track id="object-hc">}
extern __stdcall xm_s32_t XM_read_object(xmObjDesc_t objDesc, void *buffer, xm_u32_t size, xm_u32_t *flags);
extern __stdcall xm_s32_t XM_write_object(xmObjDesc_t objDesc, void *buffer, xm_u32_t size, xm_u32_t *flags);
extern __stdcall xm_s32_t XM_seek_object(xmObjDesc_t objDesc, xm_u32_t offset, xm_u32_t whence);
extern __stdcall xm_s32_t XM_ctrl_object(xmObjDesc_t objDesc, xm_u32_t cmd, void *arg);
//@ \void{</track id="object-hc">}

// Paging hypercalls
extern __stdcall xm_s32_t XM_set_page_type(xmAddress_t pAddr, xm_u32_t type);
extern __stdcall xm_s32_t XM_update_page32(xmAddress_t pAddr, xm_u32_t val);
extern __stdcall xm_s32_t XM_invld_tlb(xmAddress_t vAddr);

extern __stdcall void XM_lazy_set_page_type(xmAddress_t pAddr, xm_u32_t type);
extern __stdcall void XM_lazy_update_page32(xmAddress_t pAddr, xm_u32_t val);
extern __stdcall void XM_lazy_invld_tlb(xmAddress_t vAddr);

// Hw interrupt management
extern __stdcall xm_s32_t XM_override_trap_hndl(xm_s32_t entry, struct trapHandler *);
extern __stdcall xm_s32_t XM_clear_irqmask(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask);
extern __stdcall xm_s32_t XM_set_irqmask(xm_u32_t hwIrqsMask, xm_u32_t extIrqsMask);
extern __stdcall xm_s32_t XM_set_irqpend(xm_u32_t hwIrqPend, xm_u32_t extIrqPend);
extern __stdcall xm_s32_t XM_clear_irqpend(xm_u32_t hwIrqPend, xm_u32_t extIrqPend);
extern __stdcall xm_s32_t XM_route_irq(xm_u32_t type, xm_u32_t irq, xm_u16_t vector);

extern __stdcall xm_s32_t XM_raise_ipvi(xm_u8_t no_ipvi);
extern __stdcall xm_s32_t XM_raise_partition_ipvi(xm_u32_t partition_id, xm_u8_t no_ipvi);

extern __stdcall xm_s32_t XM_flush_cache(xm_u32_t cache);
extern __stdcall xm_s32_t XM_set_cache_state(xm_u32_t cache);

extern __stdcall xm_s32_t XM_switch_sched_plan(xm_u32_t newPlanId, xm_u32_t *currentPlanId);

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern __stdcall xm_s32_t XM_switch_sched_plan_node(xm_u32_t nodeId, xm_u32_t newPlanId, xm_u32_t *currentPlanId);
#endif
// Deferred hypercalls
extern __stdcall xm_s32_t XM_multicall(void *startAddr, void *endAddr);

/* </track id="hypercall-list"> */
#endif

#endif
