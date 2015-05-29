/*
 * $FILE: hwconf.h
 *
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
*/
#ifndef _XM_ARCH_HWCONF_H_
#define _XM_ARCH_HWCONF_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

struct x86LApic {
    xm_u32_t rsv:22, enabled:1, bsp:1, id:8;
};

struct x86IoApic {
    xm_u32_t rsv:23, enabled:1, id: 8; 
    xm_u32_t baseAddr;
};

struct x86IoApicIrq {
    xm_u32_t rsv0: 4,
	dstIoApic:8,
	srcIrq:8,
	dstIrq: 8, 
	polarity:2,
	triggerMode:2;
    xm_u32_t rsv1: 16,
	type: 8, // NMI, EXT, ...
	busId: 8;
};

struct x86LApicIrq {
    xm_u32_t rsv0: 4,
	dstLApic:8, // 0xFF: ALL
	srcIrq:8,
	dstLInt: 8,
	polarity:2,
	triggerMode:2;
    xm_u32_t rsv1: 16,
	type: 8, // NMI, EXT, ...
	busId: 8;
};

enum busTypes {
    BUS_ISA,
    BUS_PCI,
};

struct x86Bus {
    xm_u32_t rsv:6, polarity: 1, triggerMode: 1, type:16, id:8;
#define BUS_HIGH_POLARITY 0
#define BUS_LOW_POLARITY 1
#define BUS_EDGE_TRIGGER 0
#define BUS_LEVEL_TRIGGER 1
};

struct x86conf {
    xm_u32_t rsv:31, imcr:1;
    xm_u64_t lApicBaseAddr;
    struct x86LApic *lApic;
    struct x86LApicIrq *lApicIrq;
    struct x86IoApic *ioApic;
    struct x86IoApicIrq *ioApicIrq;
    struct x86Bus *bus;
    xm_u32_t lApicLen;
    xm_u32_t lApicIrqLen;
    xm_u32_t ioApicLen;
    xm_u32_t ioApicIrqLen;
    xm_u32_t busLen;
};

extern struct x86Conf x86ConfTab;

#endif
