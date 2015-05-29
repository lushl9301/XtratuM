/*
 * $FILE: acpi.c
 *
 * Advanced Configuration and Power Interface
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
#include <arch/acpi.h>
#include <arch/paging.h>
#include <assert.h>
#include <boot.h>
#include <sched.h>
#include <smp.h>

#ifdef CONFIG_SMP_INTERFACE_ACPI

static struct AcpiRsdp *acpiRsdp;

static inline xm_u32_t AcpiCreateTmpMapping(xmAddress_t addr) {
    xm_u32_t *pgTable;
    xm_u32_t page, entry;

    entry = 0;
    pgTable = (xm_u32_t *)_PHYS2VIRT(SaveCr3());
    page = addr & LPAGE_MASK;
    if (page < CONFIG_XM_OFFSET) {
        entry = pgTable[VA2PtdL1(page)];
        pgTable[VA2PtdL1(page)] = page|_PG_ARCH_PRESENT|_PG_ARCH_PSE|_PG_ARCH_RW|_PG_ARCH_GLOBAL;
    }

    return entry;
}

static inline void AcpiClearTmpMapping(xmAddress_t addr) {
    xm_u32_t *pgTable;
    xm_u32_t page, entry;

    entry = 0;
    pgTable = (xm_u32_t *)_PHYS2VIRT(SaveCr3());
    page = addr & LPAGE_MASK;
    if (page < CONFIG_XM_OFFSET) {
        pgTable[VA2PtdL1(page)] = 0;
    }
}

static inline xm_s32_t CheckSignature(const char *str, const char *sig, xm_s32_t len) {
    xm_s32_t e;

    for (e=0; e<len; ++e) {
        if (*str != *sig) {
            return 0;
        }
        ++str;
        ++sig;
    }

    return 1;
}

xmAddress_t __VBOOT AcpiScan(const char *signature, xmAddress_t ebdaStart, xmAddress_t ebdaEnd) {
    xmAddress_t addr;

    for (addr=ebdaStart; addr<ebdaEnd; addr+=16) {
        if (CheckSignature((char *)addr, signature, 8)) {
            return addr;
        }
    }

    return 0;
}

void AcpiMadtLapicHandle(struct AcpiMadtIcs *ics) {
    static xm_s32_t bsp = 1;

    if (ics->lapic.flags & CPU_ENABLED) {
        if (x86MpConf.noCpu >= CONFIG_NO_CPUS) {
            x86SystemPanic("Only supported %d cpus\n", CONFIG_NO_CPUS);
        }
        if (bsp) {
            x86MpConf.cpu[x86MpConf.noCpu].bsp = 1;
            bsp = 0;
        }
        x86MpConf.cpu[x86MpConf.noCpu].enabled = 1;
        x86MpConf.cpu[x86MpConf.noCpu].id = ics->lapic.apicId;
        x86MpConf.noCpu++;
    }
}

void AcpiMadtIoApicHandle(struct AcpiMadtIcs *ics) {
    xm_s32_t e;

    x86MpConf.ioApic[x86MpConf.noIoApic].id = ics->ioApic.id;
    x86MpConf.ioApic[x86MpConf.noIoApic].baseAddr = ics->ioApic.ioApicAddr;
    for (e=0; e<24; ++e) {
        x86MpConf.ioInt[e+x86MpConf.noIoInt].dstIoApicId = ics->ioApic.id;
        x86MpConf.ioInt[e+x86MpConf.noIoInt].dstIoApicIrq = e;
    }
    x86MpConf.noIoInt += 24;
    x86MpConf.noIoApic++;
}

void AcpiMadtIrqSrcHandle(struct AcpiMadtIcs *ics) {
    if (ics->irqSrc.flags) {
            x86MpConf.ioInt[ics->irqSrc.gsIrq].irqOver = 1;
        switch (ics->irqSrc.flags & 0x3) {
            case 1:
                x86MpConf.ioInt[ics->irqSrc.gsIrq].polarity = 0;
                break;
            case 3:
                x86MpConf.ioInt[ics->irqSrc.gsIrq].polarity = 1;
                break;
        }
        switch ((ics->irqSrc.flags >> 2) & 0x3) {
            case 1:
                x86MpConf.ioInt[ics->irqSrc.gsIrq].triggerMode = 0;
                break;
            case 3:
                x86MpConf.ioInt[ics->irqSrc.gsIrq].triggerMode = 1;
                break;
        }
    }
}

static void (*AcpiMadtHandle[])(struct AcpiMadtIcs *) = {
    [ACPI_MADT_LAPIC] = AcpiMadtLapicHandle,
    [ACPI_MADT_IOAPIC] = AcpiMadtIoApicHandle,
    [ACPI_MADT_IRQ_SRC] = AcpiMadtIrqSrcHandle,
    [ACPI_MADT_MAX] = 0,
};

static void __VBOOT AcpiParseMadtTable(struct AcpiMadt *madt) {
    xm_u32_t madtEnd, entry;
    struct AcpiMadtIcs *ics;

    madtEnd = (xm_u32_t)madt + madt->header.length;
    entry = (xm_u32_t)madt + sizeof(struct AcpiMadt);
    while (entry < madtEnd) {
        ics = (struct AcpiMadtIcs *)entry;
        if (ics->type < ACPI_MADT_MAX) {
            if (AcpiMadtHandle[ics->type]) {
                AcpiMadtHandle[ics->type](ics);
            }
        }
        entry += ics->length;
    }
}

static void __VBOOT AcpiParseMp(struct AcpiRsdp *rsdp) {
    struct AcpiRsdt *rsdt;
    struct AcpiHeader *header;
    xm_s32_t e, entries;

    AcpiCreateTmpMapping(rsdp->rsdtAddr);
    rsdt = (struct AcpiRsdt *)rsdp->rsdtAddr;
    header = &rsdt->header;
    entries = (header->length - sizeof(struct AcpiHeader))>>2;
    for (e=0; e<entries; ++e) {
        header = (struct AcpiHeader *)rsdt->entry[e];
        if (CheckSignature(header->signature, "APIC", 4)) {
            AcpiParseMadtTable((struct AcpiMadt *)header);
        }
    }
    AcpiClearTmpMapping(rsdp->rsdtAddr);
}

void __VBOOT InitSmpAcpi(void) {
    acpiRsdp = (struct AcpiRsdp *)AcpiScan("RSD PTR ", (0xe0000), (0xfffff));
    if (!acpiRsdp) {
        x86SystemPanic("ACPI: No RSDP found\n");
    }
    AcpiParseMp(acpiRsdp);
}
#endif /*CONFIG_SMP_INTERFACE_ACPI*/

