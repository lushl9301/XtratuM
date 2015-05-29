/*
 * $FILE: kthread.h
 *
 * Arch kernel thread
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
*/

#ifndef _XM_ARCH_KTHREAD_H_
#define _XM_ARCH_KTHREAD_H_

#include <irqs.h>
#include <arch/processor.h>
#include <arch/segments.h>
#include <arch/xm_def.h>
#include <arch/atomic.h>

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

struct kThreadArch {
    xmAddress_t ptdL1;
    xm_u32_t cr0;
    xm_u32_t cr4;
    xm_u32_t pCpuId;

    xm_u8_t fpCtxt[108];
    struct x86DescReg gdtr;
    struct x86Desc gdtTab[CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES];
    struct x86DescReg idtr;
    struct x86Gate idtTab[IDT_ENTRIES];
    struct ioTss tss;
};

#endif
