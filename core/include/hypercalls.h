/*
 * $FILE: hypercalls.h
 *
 * Hypercalls definition
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_HYPERCALLS_H_
#define _XM_HYPERCALLS_H_

#include __XM_INCFLD(arch/hypercalls.h)

/* <track id="abi-api-versions"> */
#define XM_ABI_VERSION 3
#define XM_ABI_SUBVERSION 1
#define XM_ABI_REVISION 0

#define XM_API_VERSION 3
#define XM_API_SUBVERSION 1
#define XM_API_REVISION 2
/* </track id="abi-api-versions"> */
// Generic hypercalls 

#define HYPERCALL_NOT_IMPLEMENTED (~0)
#define multicall_nr __MULTICALL_NR
#define halt_partition_nr __HALT_PARTITION_NR
#define suspend_partition_nr __SUSPEND_PARTITION_NR
#define resume_partition_nr __RESUME_PARTITION_NR
#define reset_partition_nr __RESET_PARTITION_NR
    #define XM_RESET_MODE 0x1
/* <track id="reset-values"> */
    #define XM_COLD_RESET 0x0
    #define XM_WARM_RESET 0x1
/* </track id="reset-values"> */
#define shutdown_partition_nr __SHUTDOWN_PARTITION_NR
#define halt_system_nr __HALT_SYSTEM_NR
#define reset_system_nr __RESET_SYSTEM_NR

#define reset_vcpu_nr __RESET_VCPU_NR
#define halt_vcpu_nr __HALT_VCPU_NR
#define suspend_vcpu_nr __SUSPEND_VCPU_NR
#define resume_vcpu_nr __RESUME_VCPU_NR

#define get_vcpuid_nr __GET_VCPUID_NR

#define idle_self_nr __IDLE_SELF_NR
#define get_time_nr __GET_TIME_NR
//@ \void{<track id="DOC-CLOCKS-AVAILABLE">}
    #define XM_HW_CLOCK (0x0)
    #define XM_EXEC_CLOCK (0x1)
//@ \void{</track id="DOC-CLOCKS-AVAILABLE">}
    #define XM_WATCHDOG_TIMER (0x2)

#define set_timer_nr __SET_TIMER_NR
#define read_object_nr __READ_OBJECT_NR
#define write_object_nr __WRITE_OBJECT_NR
#define seek_object_nr __SEEK_OBJECT_NR
    #define XM_OBJ_SEEK_CUR 0x0
    #define XM_OBJ_SEEK_SET 0x1
    #define XM_OBJ_SEEK_END 0x2
#define ctrl_object_nr __CTRL_OBJECT_NR

#define clear_irqmask_nr __CLEAR_IRQ_MASK_NR
#define set_irqmask_nr __SET_IRQ_MASK_NR
#define set_irqpend_nr __FORCE_IRQS_NR
#define clear_irqpend_nr __CLEAR_IRQS_NR
#define route_irq_nr __ROUTE_IRQ_NR
    #define XM_TRAP_TYPE 0x0
    #define XM_HWIRQ_TYPE 0x1
    #define XM_EXTIRQ_TYPE 0x2

#define update_page32_nr __UPDATE_PAGE32_NR
#define set_page_type_nr __SET_PAGE_TYPE_NR
  #define PPAG_STD 0
  #define PPAG_PTDL1 1
  #define PPAG_PTDL2 2
  #define PPAG_PTDL3 3
// ...

#define invld_tlb_nr __INVLD_TLB_NR
#define override_trap_hndl_nr __OVERRIDE_TRAP_HNDL_NR
#define raise_ipvi_nr __RAISE_IPVI_NR
#define raise_partition_ipvi_nr __RAISE_PARTITION_IPVI_NR

#define flush_cache_nr __FLUSH_CACHE_NR
#define set_cache_state_nr __SET_CACHE_STATE_NR
  #define XM_DCACHE 0x1
  #define XM_ICACHE 0x2

#define switch_sched_plan_nr __SWITCH_SCHED_PLAN_NR
#define get_gid_by_name_nr __GET_GID_BY_NAME_NR
  #define XM_PARTITION_NAME 0x0
  #define XM_PLAN_NAME 0x1

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#define reset_partition_node_nr __RESET_PARTITION_NODE_NR
#define halt_partition_node_nr __HALT_PARTITION_NODE_NR
#define reset_system_node_nr __RESET_SYSTEM_NODE_NR
#define halt_system_node_nr __HALT_SYSTEM_NODE_NR
#define switch_sched_plan_node_nr __SWITCH_SCHED_PLAN_NODE_NR
#endif

// Returning values

/* <track id="error-codes-list"> */
#define XM_OK                (0)
#define XM_NO_ACTION         (-1)
#define XM_UNKNOWN_HYPERCALL (-2)
#define XM_INVALID_PARAM     (-3)
#define XM_PERM_ERROR        (-4)
#define XM_INVALID_CONFIG    (-5)
#define XM_INVALID_MODE      (-6)
#define XM_OP_NOT_ALLOWED    (-7)
#define XM_MULTICALL_ERROR   (-8)
/* </track id="error-codes-list"> */

#ifndef __ASSEMBLY__

#define HYPERCALLR_TAB(_hc, _args) \
    __asm__ (".section .hypercallstab, \"a\"\n\t" \
	     ".align 4\n\t" \
	     ".long "#_hc"\n\t" \
	     ".previous\n\t" \
             ".section .hypercallflagstab, \"a\"\n\t" \
	     ".long (0x80000000|"#_args")\n\t" \
	     ".previous\n\t")

#define HYPERCALL_TAB(_hc, _args) \
    __asm__ (".section .hypercallstab, \"a\"\n\t" \
	     ".align 4\n\t" \
	     ".long "#_hc"\n\t" \
	     ".previous\n\t" \
             ".section .hypercallflagstab, \"a\"\n\t" \
	     ".long ("#_args")\n\t" \
	     ".previous\n\t")

#endif

#endif
