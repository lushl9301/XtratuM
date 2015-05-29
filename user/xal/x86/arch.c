/*
 * $FILE: arch.c
 *
 * Architecture initialization functions
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
#include <stdio.h>
#include <irqs.h>

typedef struct {
    xm_u32_t offsetLow:16,      /* offset 0..15 */
        selector:16,
        wordCount:8,
        access:8,
        offsetHigh:16;  /* offset 16..31 */
} gateDesc_t;

#define IDT_ENTRIES (256+32)
extern gateDesc_t idtTab[IDT_ENTRIES];
extern struct x86DescReg idtDesc;

static inline void HwSetIrqGate(xm_s32_t e, void *hndl, xm_u32_t dpl) {
    idtTab[e].selector=GUEST_CS_SEL;
    idtTab[e].offsetLow=(xmAddress_t)hndl&0xffff;
    idtTab[e].offsetHigh=((xmAddress_t)hndl>>16)&0xffff;
    idtTab[e].access=0x8e|(dpl&0x3)<<5;
}

static inline void HwSetTrapGate(xm_s32_t e, void *hndl, xm_u32_t dpl) {
    idtTab[e].selector=GUEST_CS_SEL;
    idtTab[e].offsetLow=(xmAddress_t)hndl&0xffff;
    idtTab[e].offsetHigh=((xmAddress_t)hndl>>16)&0xffff;
    idtTab[e].access=0x8f|(dpl&0x3)<<5;
}

void InitArch(void) {
    extern void (*trapTable[0])(void);
    long irqNr;

    HwSetTrapGate(0, trapTable[0], 1);
    HwSetIrqGate(1, trapTable[1], 1);
    HwSetIrqGate(2, trapTable[2], 1);
    HwSetTrapGate(3, trapTable[3], 1);
    HwSetTrapGate(4, trapTable[4], 1);
    HwSetTrapGate(5, trapTable[5], 1);
    HwSetTrapGate(6, trapTable[6], 1);
    HwSetTrapGate(7, trapTable[7], 1);
    HwSetTrapGate(8, trapTable[8], 1);
    HwSetTrapGate(9, trapTable[9], 1);
    HwSetTrapGate(10, trapTable[10], 1);
    HwSetTrapGate(11, trapTable[11], 1);
    HwSetTrapGate(12, trapTable[12], 1);
    HwSetTrapGate(13, trapTable[13], 1);
    HwSetIrqGate(13, trapTable[13], 1);
    HwSetIrqGate(14, trapTable[14], 1);
    HwSetTrapGate(15, trapTable[15], 1);
    HwSetTrapGate(16, trapTable[16], 1);
    HwSetTrapGate(17, trapTable[17], 1);
    HwSetTrapGate(18, trapTable[18], 1);
    HwSetTrapGate(19, trapTable[19], 1);

    /* Setting up the HW irqs */
    for (irqNr=0x20; irqNr<IDT_ENTRIES; irqNr++)
        HwSetIrqGate(irqNr, trapTable[irqNr], 1);

    XM_x86_load_idtr(&idtDesc);
}

void Halt(void) {
    XM_halt_partition(XM_PARTITION_SELF);
}
