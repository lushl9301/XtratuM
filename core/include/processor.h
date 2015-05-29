/*
 * $FILE: processor.h
 *
 * Processor functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_PROCESSOR_H_
#define _XM_PROCESSOR_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <irqs.h>
#include <smp.h>
#include <arch/processor.h>

typedef struct {
    xm_u32_t flags;
#define CPU_SLOT_ENABLED (1<<0)
#define BSP_FLAG (1<<1)
    volatile xm_u32_t irqNestingCounter;
    xm_u32_t globalIrqMask;
} localCpu_t;

extern localCpu_t localCpuInfo[];
#ifdef CONFIG_SMP
#define GET_LOCAL_CPU()	\
    (&localCpuInfo[GET_CPU_ID()])
#else
#define GET_LOCAL_CPU() localCpuInfo
#endif

extern void HaltSystem(void);
extern void ResetSystem(xm_u32_t resetMode);

extern void DumpState(cpuCtxt_t *regs);
extern void PartitionPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...);
extern void SystemPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...);
//extern void StackBackTrace(xm_u32_t);

extern xm_u32_t cpuKhz;
extern void SetupCpu(void);
extern void SetupArchLocal(xm_s32_t cpuid);
extern void EarlySetupArchCommon(void);
extern void SetupArchCommon(void);

#endif
