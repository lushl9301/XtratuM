/*
 * $FILE: irqs.h
 *
 * IRQS
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_IRQS_H_
#define _XM_IRQS_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#ifndef __ASSEMBLY__

#include <linkage.h>
#include <arch/irqs.h>

#define SCHED_PENDING 0x80000000
#define IRQ_IN_PROGRESS (~SCHED_PENDING)

typedef void (*irqHandler_t)(cpuCtxt_t *, void *);

typedef xm_s32_t (*trapHandler_t)(cpuCtxt_t *, xm_u16_t *);

struct irqTabEntry {
    irqHandler_t handler;
    void *data;
};

#if (CONFIG_NO_HWIRQS)>32
#error CONFIG_NO_HWIRQS is greater than 32
#endif
#if (NO_TRAPS)>32
#error NO_TRAPS is greater than 32
#endif

extern struct irqTabEntry irqHandlerTab[CONFIG_NO_HWIRQS];
extern trapHandler_t trapHandlerTab[NO_TRAPS];
extern void SetupIrqs(void);
extern irqHandler_t SetIrqHandler(xm_s32_t, irqHandler_t, void *);
extern trapHandler_t SetTrapHandler(xm_s32_t, trapHandler_t);

extern void ArchSetupIrqs(void);

// Control over each interrupt
typedef struct {
    void (*Enable)(xm_u32_t irq);
    void (*Disable)(xm_u32_t irq);
    void (*Ack)(xm_u32_t irq);
    void (*End)(xm_u32_t irq);
    void (*Force)(xm_u32_t irq);
    void (*Clear)(xm_u32_t irq);
} hwIrqCtrl_t;

extern xm_u32_t HwIrqGetMask(void);
extern void HwIrqSetMask(xm_u32_t mask);

extern hwIrqCtrl_t hwIrqCtrl[CONFIG_NO_HWIRQS];

static inline void HwDisableIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].Disable)
	hwIrqCtrl[irq].Disable(irq);
}

static inline void HwEnableIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].Enable)
	hwIrqCtrl[irq].Enable(irq);
}

static inline void HwAckIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].Ack)
	hwIrqCtrl[irq].Ack(irq);
}

static inline void HwEndIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].End)
	hwIrqCtrl[irq].End(irq);
}

static inline void HwForceIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].Force)
	hwIrqCtrl[irq].Force(irq);
}

static inline void HwClearIrq(xm_s32_t irq) {
    if ((irq<CONFIG_NO_HWIRQS)&&hwIrqCtrl[irq].Clear)
	hwIrqCtrl[irq].Clear(irq);
}

extern xm_s32_t MaskHwIrq(xm_s32_t irq);
extern xm_s32_t UnmaskHwIrq(xm_s32_t irq);
extern void SetTrapPending(cpuCtxt_t *ctxt);
extern xm_s32_t ArchTrapIsSysCtxt(cpuCtxt_t *ctxt);
extern xmAddress_t IrqVector2Address(xm_s32_t vector);

#ifdef CONFIG_VERBOSE_TRAP
extern xm_s8_t *trap2Str[];
#endif

#endif

#endif
