/*
 * $FILE: acpi.h
 *
 * ACPI interface
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_ACPI_H_
#define _XM_ARCH_ACPI_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <linkage.h>

struct AcpiHeader {
    xm_s8_t signature[4];
    xm_u32_t length;
    xm_u8_t revision;
    xm_u8_t checksum;
    xm_s8_t oemId[6];
    xm_s8_t oemTableId[8];
    xm_u32_t oemRevision;
    xm_s8_t creatorId[4];
    xm_u32_t creatorRevision;
};

struct AcpiRsdp {
    xm_s8_t signature[8];
    xm_u8_t checksum;
    xm_s8_t oemId[6];
    xm_u8_t revision;
    xm_u32_t rsdtAddr;
    xm_u32_t length;
    xm_u64_t xsdtAddr;
    xm_u8_t extChecksum;
    xm_u8_t reserved[3];
};

struct AcpiRsdt {
    struct AcpiHeader header;
    xm_u32_t entry[0];
}__PACKED;

struct AcpiMadt {
    struct AcpiHeader header;
    xmAddress_t lApicAddr;
    xm_u32_t flags;
}__PACKED;

struct AcpiMadtLapic {
    xm_u8_t acpiProcId;
    xm_u8_t apicId;
    xm_u32_t flags;
}__PACKED;

struct AcpiMadtIoApic {
    xm_u8_t id;
    xm_u8_t reserved;
    xm_u32_t ioApicAddr;
    xm_u32_t gsIrqBase;
}__PACKED;

struct AcpiMadtIrqSrc {
#define ACPI_POL_MASK 0xc000
#define ACPI_POL_HIGH 0x4000
#define ACPI_POL_LOW 0xc000
    xm_u8_t bus;
    xm_u8_t source;
    xm_u32_t gsIrq;
    xm_u16_t flags;
}__PACKED;

struct AcpiMadtNmiSrc {
    xm_u16_t flags;
    xm_u32_t gsIrq;
}__PACKED;

struct AcpiMadtLapicNmiSrc {
    xm_u8_t acpiCpuId;
    xm_u16_t flags;
    xm_u8_t lApicLint;
}__PACKED;

struct AcpiMadtLapicAddrOver {
    xm_u16_t reserved;
    xm_u64_t lApicAddr;
}__PACKED;

struct AcpiMadtSapic {
    xm_u8_t ioApicId;
    xm_u8_t reserved;
    xm_u32_t gsIrqBase;
    xm_u64_t lSapicAddr;
}__PACKED;

struct AcpiMadtLsapic {
    xm_u8_t acpiCpuId;
    xm_u8_t lSapicId;
    xm_u8_t lSapicEid;
    xm_u8_t reserved[3];
    xm_u32_t fkags;
    xm_u32_t acpiCpuUid;
    char cpuUidStr[0];
}__PACKED;

struct AcpiMadtPirqSrc {
    xm_u16_t flags;
    xm_u8_t irqType;
    xm_u8_t cpuId;
    xm_u8_t cpuEid;
    xm_u8_t ioSapicVector;
    xm_u32_t gsIrq;
    xm_u32_t pirqSrcFlags;
}__PACKED;

struct AcpiMadtLx2apic {
    xm_u8_t reserver[2];
    xm_u32_t x2apicId;
    xm_u32_t flags;
    xm_u32_t acpiCpuId;
}__PACKED;

struct AcpiMadrLx2apicNmi {
    xm_u16_t flags;
    xm_u32_t acpiCpuUid;
    xm_u8_t lx2apicLint;
    xm_u8_t reserved[3];
}__PACKED;

#define ACPI_MADT_LAPIC 0
#define ACPI_MADT_IOAPIC 1
#define ACPI_MADT_IRQ_SRC 2
#define ACPI_MADT_NMI_SRC 3
#define ACPI_MADT_LAPIC_NMI 4
#define ACPI_MADT_LAPIC_ADDR_OVER 5
#define ACPI_MADT_IOSAPIC 6
#define ACPI_MADT_LSAPIC 7
#define ACPI_MADT_PIRQ_SRC 8
#define ACPI_MADT_LX2APIC 9
#define ACPI_MADT_LX2APIC_NMI 10
#define ACPI_MADT_MAX 11

struct AcpiMadtIcs {
    xm_u8_t type;
    xm_u8_t length;
    union {
        struct AcpiMadtLapic lapic;
        struct AcpiMadtIoApic ioApic;
        struct AcpiMadtIrqSrc irqSrc;
        struct AcpiMadtNmiSrc nmiSrc;
        struct AcpiMadtLapicNmiSrc lapicNmiSrc;
        struct AcpiMadtLapicAddrOver lapicAddrOver;
        struct AcpiMadtSapic sapic;
        struct AcpiMadtLsapic lsapic;
        struct AcpiMadtPirqSrc pIrqSrc;
        struct AcpiMadtLx2apic lx2Apic;
        struct AcpiMadrLx2apicNmi lx2ApicNmi;
    };
}__PACKED;

#endif /*_XM_ARCH_ACPI_H_*/
