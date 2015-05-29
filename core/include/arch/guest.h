/*
 * $FILE: guest.h
 *
 * Guest shared info
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_GUEST_H_
#define _XM_ARCH_GUEST_H_

#include __XM_INCFLD(arch/atomic.h)
#include __XM_INCFLD(arch/processor.h)

// XXX: this structure is visible from the guest
/*  <track id="doc-Partition-Control-Table-x86"> */
struct pctArch {
    struct x86DescReg gdtr;
    struct x86DescReg idtr;
    xm_u32_t maxIdtVec;
    volatile xm_u32_t tr;
    volatile xm_u32_t cr4;
    volatile xm_u32_t cr3;
#define _ARCH_PTDL1_REG cr3
    volatile xm_u32_t cr2;
    volatile xm_u32_t cr0;
};
/*  </track id="doc-Partition-Control-Table-x86"> */

#endif
