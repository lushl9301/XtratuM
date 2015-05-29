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

#ifndef _XAL_ARCH_IRQS_H_
#define _XAL_ARCH_IRQS_H_

#define TRAPTAB_LENGTH 256

#define SPARC_HWIRQ_TRAP(_irqNr) (_irqNr+0x10)

#define HwCli() XM_sparc_set_pil()
#define HwSti() XM_sparc_clear_pil()

#define HwSaveFlags(hwFlags) do { \
    (hwFlags)=XM_sparcv8_get_psr(); \
} while(0)

#define HwSaveFlagsCli(hwFlags) do { \
    HwSaveFlags(hwFlags); \
    HwCli(); \
} while(0)

#define HwRestoreFlags(hwFlags) do { \
    XM_sparcv8_set_psr(hwFlags); \
} while(0)

#define HwIsSti() (((XM_params_get_PCT()->iFlags)&PSR_ET_BIT)&&!((XM_params_get_PCT()->iFlags)&PSR_PIL_MASK))

#ifndef __ASSEMBLY__

typedef struct trapCtxt {
    xm_u32_t y;
    xm_u32_t g1;
    xm_u32_t g2;
    xm_u32_t g3;
    xm_u32_t g4;
    xm_u32_t g5;
    xm_u32_t g6;
    xm_u32_t g7;
    xm_u32_t nPc;
    xm_u32_t irqNr;
    xm_u32_t flags;
    xm_u32_t pc;
} trapCtxt_t;

#endif
#endif
