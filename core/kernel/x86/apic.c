/*
 * $FILE: apic.c
 *
 * Local & IO Advanced Programming Interrupts Controller (APIC)
 * IOAPIC, A.K.A. i82093AA
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <stdc.h>
#include <kdevice.h>
#include <ktimer.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/asm.h>
#include <arch/atomic.h>
#include <arch/apic.h>
#include <arch/irqs.h>
#include <arch/processor.h>
#include <arch/pic.h>
#include <arch/io.h>

RESERVE_PHYSPAGES(LAPIC_DEFAULT_BASE, 1);
RESERVE_PHYSPAGES(IOAPIC_DEFAULT_BASE, 1);
static xmAddress_t lApicAddr;
static xmAddress_t ioApicAddr[CONFIG_MAX_NO_IOAPICS];
static spinLock_t ioApicLock = SPINLOCK_INIT;
#if CONFIG_MAX_NO_IOAPICS>1
static xm_u8_t ioIrq2IoApic[CONFIG_MAX_NO_IOINT];
#endif

xm_u32_t LApicRead(xm_u32_t reg) {
    return *(volatile xm_u32_t *)(lApicAddr+reg);
}

void LApicWrite(xm_u32_t reg, xm_u32_t v) {
    *(volatile xm_u32_t *)(lApicAddr+reg)=v;
}

void LApicEoi(void) {
    LApicWrite(APIC_EOI, 0);
}

static inline xm_u32_t IoApicRead(xm_u32_t reg, xm_s32_t ioApic) {
    xm_u32_t val;

    *((volatile xm_u32_t *)(ioApicAddr[ioApic]+APIC_IOREGSEL))=reg;
    val = *((volatile xm_u32_t *)(ioApicAddr[ioApic]+APIC_IOWIN));

    return val;
}

static inline void IoApicWrite(xm_u32_t reg, xm_u32_t v, xm_s32_t ioApic) {
    SpinLock(&ioApicLock);
    *((volatile xm_u32_t *)(ioApicAddr[ioApic]+APIC_IOREGSEL))=reg;
    *((volatile xm_u32_t *)(ioApicAddr[ioApic]+APIC_IOWIN))=v;
    SpinUnlock(&ioApicLock);
}

void IoApicWriteEntry(xm_u8_t pin, struct ioApicRouteEntry *e, xm_s32_t ioApic) {
    IoApicWrite(0x11+(pin*2), *(((xm_s32_t *)e)+1), ioApic);
    IoApicWrite(0x10+(pin*2), *(((xm_s32_t *)e)+0), ioApic);
}

struct ioApicRouteEntry IoApicReadEntry(xm_u8_t pin, xm_s32_t ioApic) {
    struct ioApicRouteEntry e;

    SpinLock(&ioApicLock);
    *(((xm_s32_t *)&e)+1)=IoApicRead(0x11+(pin*2), ioApic);
    *(((xm_s32_t *)&e)+0)=IoApicRead(0x10+(pin*2), ioApic);
    SpinUnlock(&ioApicLock);

    return e;
}

static inline xm_u32_t ApicWaitIcrIdle(void) {
    xm_u32_t sendStatus;
    xm_s32_t timeOut;

    for (timeOut=0; timeOut<1000; ++timeOut) {
        sendStatus=LApicRead(APIC_ICR_LOW)&APIC_ICR_BUSY;
        if (!sendStatus)
            break;
        EarlyDelay(200);
    }
    
    return sendStatus;
}

static inline xm_u32_t GetLApicId(void) {
    xm_u32_t id=LApicRead(APIC_ID);
	return (((id)>>24)&0xFF);
}

static inline void IoApicMaskIrq(xm_s32_t irq, xm_s32_t ioApic) {
    struct ioApicRouteEntry entry;

    entry=IoApicReadEntry(irq, ioApic);
    entry.mask=1;
    IoApicWriteEntry(irq, &entry, ioApic);
}

static inline void IoApicUnmaskIrq(xm_s32_t irq, xm_s32_t ioApic) {
    struct ioApicRouteEntry entry;

    entry=IoApicReadEntry(irq, ioApic);
    entry.mask=0;
    IoApicWriteEntry(irq, &entry, ioApic);
}

static inline void IoApicLevelIrq(xm_s32_t irq, xm_s32_t ioApic) {
    struct ioApicRouteEntry entry;

    entry=IoApicReadEntry(irq, ioApic);
    entry.trigger=1;
    IoApicWriteEntry(irq, &entry, ioApic);
}

static inline void IoApicEdgeIrq(xm_s32_t irq, xm_s32_t ioApic) {
    struct ioApicRouteEntry entry;

    entry=IoApicReadEntry(irq, ioApic);
    entry.trigger=0;
    IoApicWriteEntry(irq, &entry, ioApic);
}

#if defined(CONFIG_CHIPSET_ICH)
static inline void IoApicEoi(xm_s32_t irq, xm_s32_t ioApic) {
    *((volatile xm_u32_t *)(ioApicAddr[ioApic] + IO_APIC_EOI)) = irq;
}
#else
static inline void IoApicEoi(xm_s32_t irq, xm_s32_t ioApic) {
}
#endif

static void IoApicEnableIrq(xm_u32_t irq) {
    IoApicUnmaskIrq(irq, 0);
    x86HwIrqsMask[GET_CPU_ID()]&=~(1<<irq);
}

static void IoApicDisableIrq(xm_u32_t irq) {
    x86HwIrqsMask[GET_CPU_ID()]|=1<<irq;
}

#ifdef CONFIG_APIC
void HwIrqSetMask(xm_u32_t mask) {
    xm_s32_t e;

    for (e=0; e<CONFIG_NO_HWIRQS; e++) {
        if (mask&(1<<e))
            HwDisableIrq(e);
        else
            HwEnableIrq(e);
    }
}
#endif

void IoApicAckEdgeIrq(xm_u32_t irq) {
    LApicEoi();
}

void IoApicAckLevelIrq(xm_u32_t irq) {
    xm_u32_t vector, mask;

    IoApicMaskIrq(irq, 0);

    vector = irq+FIRST_EXTERNAL_VECTOR;
    mask = LApicRead(APIC_TMR + ((vector & ~0x1f) >> 1));

    LApicEoi();

    if (!(mask & (1 << (mask & 0x1f)))) {
        IoApicEoi(irq, 0);
    }
}

static void LvttEnableIrq(xm_u32_t irq) {
    LApicWrite(APIC_LVTT, ~APIC_LVT_MASKED&LApicRead(APIC_LVTT));
    x86HwIrqsMask[GET_CPU_ID()]&=~(1<<LAPIC_TIMER_IRQ);
}

static void LvttDisableIrq(xm_u32_t irq) {
    LApicWrite(APIC_LVTT, APIC_LVT_MASKED|LApicRead(APIC_LVTT));
    x86HwIrqsMask[GET_CPU_ID()]|=1<<LAPIC_TIMER_IRQ;
}

static void LvttMaskAndAckIrq(xm_u32_t irq) {
    LApicWrite(APIC_LVTT, APIC_LVT_MASKED|LApicRead(APIC_LVTT));
    LApicWrite(APIC_EOI, 0);
    x86HwIrqsMask[GET_CPU_ID()]|=1<<LAPIC_TIMER_IRQ;
}

static inline xm_s32_t FindIrqVector(xm_s32_t noInt) {
    return FIRST_EXTERNAL_VECTOR+noInt;
}

static inline void __VBOOT PrintEntry(xm_s32_t noInt, struct ioApicRouteEntry *ioApicEntry) {
    eprintf("APIC entry %d, %d, %d, %d\n", noInt, ioApicEntry->vector, ioApicEntry->trigger, ioApicEntry->deliveryMode);
}

static inline void __VBOOT GetApicVer(void) {
    struct ioApicVersionEntry apicVer;

    apicVer.raw = IoApicRead(IO_APIC_VER, 0);
    eprintf("Version %d Entries %d\n", apicVer.version, apicVer.entries);
}

static inline void __VBOOT SetupIoApicEntry(xm_s32_t irq, xm_s32_t apic, xm_s32_t trigger, xm_s32_t polarity) {
    struct ioApicRouteEntry ioApicEntry;

    ioApicEntry = IoApicReadEntry(irq, apic);
    ioApicEntry.deliveryMode = destFixed;
    ioApicEntry.destMode = 1;
    ioApicEntry.dest.logical.logicalDest=0xf;
    ioApicEntry.mask|=1;
    ioApicEntry.vector=FindIrqVector(irq);
    ioApicEntry.trigger=trigger;
    ioApicEntry.polarity=polarity;
    IoApicWriteEntry(irq, &ioApicEntry, apic);

    hwIrqCtrl[irq].Enable=IoApicEnableIrq;
    hwIrqCtrl[irq].Disable=IoApicDisableIrq;
    hwIrqCtrl[irq].End=IoApicDisableIrq;
    if (ioApicEntry.trigger==IO_APIC_TRIG_LEVEL) {
        hwIrqCtrl[irq].Ack=IoApicAckLevelIrq;
    } else if (ioApicEntry.trigger==IO_APIC_TRIG_EDGE) {
        hwIrqCtrl[irq].Ack=IoApicAckEdgeIrq;
    }
}

static inline void __VBOOT InitIoApic(void) {
    xm_s32_t e, i;

    if (x86MpConf.noIoApic!=CONFIG_MAX_NO_IOAPICS) {
        x86SystemPanic("Only supported %d IO-APIC, found %d", CONFIG_MAX_NO_IOAPICS, x86MpConf.noIoApic);
    }

    for (e=0; e<x86MpConf.noIoApic; e++) {
        ioApicAddr[e]=VmmAlloc(1);
        VmMapPage(x86MpConf.ioApic[e].baseAddr, ioApicAddr[e], _PG_ARCH_PRESENT|_PG_ARCH_RW|_PG_ARCH_GLOBAL|_PG_ARCH_PCD);
    }
    ApicWaitIcrIdle();
    LApicWrite(APIC_ICR_LOW, APIC_DEST_ALLINC|APIC_INT_LEVELTRIG|APIC_DM_INIT);

    // Disabling all PIC interrupts
    for (e=0; e<PIC_IRQS; e++)
        HwDisableIrq(e);

#if defined(CONFIG_CHIPSET_ICH)
    IoApicWrite(IO_APIC_ID, (x86MpConf.ioApic[0].id&0xff)<<24, 0);
    for (i=0; i<PIC_IRQS; i++) {
        SetupIoApicEntry(i, 0, IO_APIC_TRIG_EDGE, IO_APIC_POL_HIGH);
        HwDisableIrq(i);
    }
    for (i=PIC_IRQS; i<23; ++i) {
        SetupIoApicEntry(i, 0, IO_APIC_TRIG_LEVEL, IO_APIC_POL_LOW);
        HwDisableIrq(i);
    }
#else
    for (e=0; e<x86MpConf.noIoApic; ++e) {  /* FIXME: To be removed... */
        IoApicWrite(IO_APIC_ID, (x86MpConf.ioApic[e].id&0xff)<<24, e);
        for (i=0; i<x86MpConf.noIoInt; i++) {
            if (x86MpConf.ioInt[i].dstIoApicId==x86MpConf.ioApic[e].id) {
                SetupIoApicEntry(i, e, IO_APIC_TRIG_EDGE, IO_APIC_POL_HIGH);
            }
        }
        for (i=0; i<x86MpConf.noIoInt; i++) {
            HwDisableIrq(i);
        }
    }
#endif
}

static inline void InitLApicMap(void) {
    xm_u64_t apicMsr;
    xmAddress_t lApicPhysAddr;

    apicMsr=ReadMsr(MSR_IA32_APIC_BASE);
    if (!(apicMsr&_MSR_APICBASE_ENABLE))
        x86SystemPanic("APIC not supported");

#if 0
    if (apicMsr&_MSR_APICBASE_EXTD)
        /* TODO: Use x2APIC */;
#endif

    lApicPhysAddr=apicMsr&APIC_BASE_MASK_MSR;
    if (lApicPhysAddr!=LAPIC_DEFAULT_BASE)
        x86SystemPanic("APIC base address (0x%x) not expected", lApicPhysAddr);

    lApicAddr=VmmAlloc(1);
    VmMapPage(lApicPhysAddr, lApicAddr, _PG_ARCH_PRESENT|_PG_ARCH_RW|_PG_ARCH_GLOBAL|_PG_ARCH_PCD);
}

void SendIpi(xm_u8_t dst, xm_u8_t dstShortHand, xm_u8_t vector) {
    xm_u32_t hIpi, lIpi;

    hIpi=((localIdTab[dst].hwId)<<24);
    lIpi=((dstShortHand&0x3)<<18)|vector;
    LApicWrite(APIC_ICR_HIGH, hIpi);
    LApicWrite(APIC_ICR_LOW, lIpi);
}

void __VBOOT InitLApic(xm_s32_t cpuId) {
    xm_u32_t v;

    LApicWrite(APIC_DFR, APIC_DFR_FLAT);
    v=(1<<cpuId)<<24;
    LApicWrite(APIC_LDR, v);
    v=LApicRead(APIC_TASKPRI);
    v&=~APIC_TPRI_MASK;
    LApicWrite(APIC_TASKPRI, v);
    
    v=LApicRead(APIC_SVR);
    v&=~APIC_VECTOR_MASK;
    v|=APIC_SVR_APIC_ENABLED;
    v|=APIC_SVR_FOCUS_DISABLED;
    LApicWrite(APIC_SVR, v);

#if 0
    if (cpuId==0) {
        LApicWrite(APIC_LVT0, APIC_LVT_LEVEL_TRIGGER|APIC_MODE_EXTINT);
        LApicWrite(APIC_LVT1, APIC_MODE_NMI);
    } else {
        LApicWrite(APIC_LVT0, APIC_LVT_MASKED);
        LApicWrite(APIC_LVT1, APIC_LVT_MASKED);
    }
#endif

    LApicWrite(APIC_ESR, 0);
    LApicWrite(APIC_ESR, 0);
    LApicWrite(APIC_ESR, 0);
    LApicWrite(APIC_ESR, 0);
    LApicWrite(APIC_LVTERR, APIC_LVT_MASKED);
    LApicWrite(APIC_LVTPC, APIC_LVT_MASKED);
    LApicWrite(APIC_LVTT, APIC_LVT_MASKED);
    LApicWrite(APIC_LVT0, APIC_LVT_MASKED);
    LApicWrite(APIC_LVT1, APIC_LVT_MASKED);
}

void __VBOOT SetupApicCommon(void) {
    xm_u64_t msr;

    msr = ReadMsr(MSR_IA32_APIC_BASE);
    if (!(msr & _MSR_APICBASE_ENABLE)) {
        kprintf("[FATAL] APIC not enabled\n", 0);
        return;
    }

    if (x86MpConf.imcr) {
        OutB(0x70, 0x22);
        OutB(0x00, 0x23);
    }

    InitIoApic();
    InitLApicMap();

    hwIrqCtrl[LAPIC_TIMER_IRQ].Enable=LvttEnableIrq;
    hwIrqCtrl[LAPIC_TIMER_IRQ].Disable=LvttDisableIrq;
    hwIrqCtrl[LAPIC_TIMER_IRQ].Ack=LvttMaskAndAckIrq;
    hwIrqCtrl[LAPIC_TIMER_IRQ].End=LvttEnableIrq;
    hwIrqCtrl[SCHED_PENDING_IPI_IRQ].Ack=IoApicAckEdgeIrq;
}

