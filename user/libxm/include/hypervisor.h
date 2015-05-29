/*
 * $FILE: hypervisor.h
 *
 * hypervisor management functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_HYPERVISOR_H_
#define _LIB_XM_HYPERVISOR_H_

#include <xm_inc/config.h>
#include <xm_inc/xmconf.h>
#include <arch/hypervisor.h>

#include <xm_inc/linkage.h>

#define XM_PARTITION_SELF (PCT_GET_PARTITION_ID(XM_get_PCT0()))
#define XM_VCPU_SELF (PCT_GET_VCPU_ID(XM_get_PCT()))

/* Partition management */
extern xm_s32_t XM_write_console(char *buffer, xm_s32_t length);
extern xm_s32_t XM_memory_copy(xmId_t destId, xm_u32_t destAddr, xmId_t srcId,  xm_u32_t srcAddr, xm_u32_t size);

/* Interrupt management */
//extern __stdcall xm_s32_t XM_are_irqs_enabled(void);
//extern __stdcall xm_s32_t XM_mask_irq(xm_u32_t noIrq);
//extern __stdcall xm_s32_t XM_unmask_irq(xm_u32_t noIrq);
//extern __stdcall xm_s32_t XM_clear_irq(xm_u32_t noIrq);
//extern __stdcall xm_s32_t XM_set_irqpend(xm_u32_t noIrq);

/* Deferred hypercalls */
extern void init_batch(void);
extern __stdcall xm_s32_t XM_flush_hyp_batch(void);
extern __stdcall void XM_lazy_hypercall(xm_u32_t noHyp, xm_s32_t noArgs, ...);

#define xm_lazy_hcall0(_hc) \
__stdcall void XM_lazy_##_hc(void) { \
    XM_lazy_hypercall(_hc##_nr, 0); \
}

#define xm_lazy_hcall1(_hc, _t0, _a0) \
__stdcall void XM_lazy_##_hc(_t0 _a0) { \
    XM_lazy_hypercall(_hc##_nr, 1, _a0); \
}

#define xm_lazy_hcall2(_hc, _t0, _a0, _t1, _a1) \
__stdcall void XM_lazy_##_hc(_t0 _a0, _t1 _a1) { \
    XM_lazy_hypercall(_hc##_nr, 2, _a0, _a1); \
}

#define xm_lazy_hcall3(_hc, _t0, _a0, _t1, _a1, _t2, _a2) \
__stdcall void XM_lazy_##_hc(_t0 _a0, _t1 _a1, _t2 _a2) { \
    XM_lazy_hypercall(_hc##_nr, 3, _a0, _a1, _a2); \
}

#define xm_lazy_hcall4(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3) \
__stdcall void XM_lazy_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3) { \
    XM_lazy_hypercall(_hc##_nr, 4, _a0, _a1, _a2, _a3); \
}

#define xm_lazy_hcall5(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3, _t4, _a4) \
__stdcall void XM_lazy_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3, _t4 _a4) { \
    XM_lazy_hypercall(_hc##_nr, 5, _a0, _a1, _a2, _a3, _a4); \
}

#endif
