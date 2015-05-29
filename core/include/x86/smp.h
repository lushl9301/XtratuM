/*
 * $FILE: smp.h
 *
 * SMP related stuff
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_SMP_H_
#define _XM_ARCH_SMP_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/mpspec.h>
#include <arch/apic.h>
#include <linkage.h>

#ifdef CONFIG_APIC

#define BUS_HIGH_POLARITY 0
#define BUS_LOW_POLARITY 1
#define BUS_EDGE_TRIGGER 0
#define BUS_LEVEL_TRIGGER 1

struct x86MpConf {
    xm_s32_t imcr;
    xm_s32_t noCpu;
    struct cpuConf {
        xm_u32_t rsv:22,
            enabled:1,
            bsp:1,
            id:8;
    } cpu[CONFIG_NO_CPUS];
    xm_s32_t noBus;
    struct busConf {
        xm_u32_t id:8,
            type:8,
            polarity:3,
            triggerMode:3;
    } bus[CONFIG_MAX_NO_BUSES];
    xm_s32_t noIoApic;
    struct ioApicConf {
        xm_u32_t id;
        xmAddress_t baseAddr;
    } ioApic[CONFIG_MAX_NO_IOAPICS];
    xm_s32_t noIoInt;
    struct ioIntConf {
        xm_u64_t irqType:7,
            irqOver:1,
            polarity:3,
            triggerMode:3,
            srcBusId:8,
            srcBusIrq:8,
            dstIoApicId:8,
            dstIoApicIrq:8;              
    } ioInt[CONFIG_MAX_NO_IOINT];
    xm_s32_t noLInt;
    struct lIntConf {
    } lInt[CONFIG_MAX_NO_LINT];
};

extern struct x86MpConf x86MpConf;

#endif  /* CONFIG_APIC */

#endif
