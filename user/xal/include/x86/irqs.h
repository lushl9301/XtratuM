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

#ifndef __ASSEMBLY__

#define HwCli() XM_x86_clear_if()
#define HwSti() XM_x86_set_if()
#define HwIsSti() ((XM_get_PCT()->iFlags)&_CPU_FLAG_IF)

typedef struct trapCtxt_t {
    xm_u32_t ebx;
    xm_u32_t ecx;
    xm_u32_t edx;
    xm_u32_t esi;
    xm_u32_t edi;
    xm_u32_t ebp;
    xm_u32_t eax;
    xm_u32_t ds;
    xm_u32_t es;
    xm_u32_t fs;
    xm_u32_t gs;

    xm_u32_t irqNr;
    xm_u32_t eCode;

    xm_u32_t ip;
    xm_u32_t cs;
    xm_u32_t flags;
    xm_u32_t sp;
    xm_u32_t ss;
} trapCtxt_t;

#endif /*__ASSEMBLY__*/
#endif /*_XAL_ARCH_IRQS_H_*/
