/*
 * $FILE: mpspec.h
 *
 * Intel's MP specification
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_MPSPEC_H_
#define _XM_ARCH_MPSPEC_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#define SMP_MAGIC_IDENT (('_'<<24)|('P'<<16)|('M'<<8)|'_')
#define MAX_MPC_ENTRY 1024
#define MAX_APICS 256

struct mpFloatingPointer {
    xm_s8_t signature[4];
    xm_u32_t physPtr; 
    xm_u8_t length;
    xm_u8_t specification;
    xm_u8_t checksum;
    xm_u8_t feature1;
    xm_u8_t feature2;
#define ICMR_FLAG (1<<7)
    xm_u8_t feature3;
    xm_u8_t feature4;
    xm_u8_t feature5;
} __PACKED;

struct mpConfigTable {
    xm_u32_t signature;
#define MPC_SIGNATURE (('P'<<24)|('M'<<16)|('C'<<8)|'P')
    xm_u16_t length;
    xm_s8_t  spec;
    xm_s8_t  checksum;
    xm_s8_t  oem[8];
    xm_s8_t  productId[12];
    xm_u32_t oemPtr;
    xm_u16_t oemSize;
    xm_u16_t oemCount;
    xm_u32_t lApic;
    xm_u32_t reserved;
} __PACKED;

struct mpcConfigProcessor {
    xm_u8_t type;
    xm_u8_t apicId;
    xm_u8_t apicVer;
    xm_u8_t cpuFlag;
#define CPU_ENABLED 1
#define CPU_BOOTPROCESSOR 2
    xm_u32_t cpuFeature;
#define CPU_STEPPING_MASK 0x0F
#define CPU_MODEL_MASK  0xF0
#define CPU_FAMILY_MASK 0xF00
    xm_u32_t featureflag;
    xm_u32_t reserved[2];
} __PACKED;

struct mpcConfigBus {
    xm_u8_t type;
    xm_u8_t busId;
    xm_u8_t busType[6];
} __PACKED;

// List of Bus Type string values, Intel MP Spec.
#define BUSTYPE_EISA	"EISA"
#define BUSTYPE_ISA	"ISA"
#define BUSTYPE_INTERN	"INTERN"	/* Internal BUS */
#define BUSTYPE_MCA	"MCA"
#define BUSTYPE_VL	"VL"		/* Local bus */
#define BUSTYPE_PCI	"PCI"
#define BUSTYPE_PCMCIA	"PCMCIA"
#define BUSTYPE_CBUS	"CBUS"
#define BUSTYPE_CBUSII	"CBUSII"
#define BUSTYPE_FUTURE	"FUTURE"
#define BUSTYPE_MBI	"MBI"
#define BUSTYPE_MBII	"MBII"
#define BUSTYPE_MPI	"MPI"
#define BUSTYPE_MPSA	"MPSA"
#define BUSTYPE_NUBUS	"NUBUS"
#define BUSTYPE_TC	"TC"
#define BUSTYPE_VME	"VME"
#define BUSTYPE_XPRESS	"XPRESS"

struct mpcConfigIoApic {
    xm_u8_t type;
    xm_u8_t apicId;
    xm_u8_t apicVer;
    xm_u8_t flags;
#define MPC_APIC_USABLE 0x01
    xm_u32_t apicAddr;
} __PACKED;

struct mpcConfigIntSrc {
    xm_u8_t type;
    xm_u8_t irqType;
    xm_u16_t irqFlag;
    xm_u8_t srcBus;
    xm_u8_t srcBusIrq;
    xm_u8_t dstApic;
    xm_u8_t dstIrq;
} __PACKED;

#define MP_IRQDIR_DEFAULT 0
#define MP_IRQDIR_HIGH 1
#define MP_IRQDIR_LOW 3

struct mpcConfigLIntSrc {
    xm_u8_t type;
    xm_u8_t irqType;
    xm_u16_t irqFlag;
    xm_u8_t srcBusId;
    xm_u8_t srcBusIrq;
    xm_u8_t destApic;	
#define MP_APIC_ALL 0xFF
    xm_u8_t destApicLInt;
} __PACKED;

struct mpConfigOemTable {
    xm_s8_t oemSignature[4];
#define MPC_OEM_SIGNATURE "_OEM"
    xm_u16_t oemLength;
    xm_s8_t  oemRev;
    xm_s8_t  oemChecksum;
    xm_s8_t  mpcOem[8];
} __PACKED;

/*
 *	Default configurations
 *
 *	1	2 CPU ISA 82489DX
 *	2	2 CPU EISA 82489DX neither IRQ 0 timer nor IRQ 13 DMA chaining
 *	3	2 CPU EISA 82489DX
 *	4	2 CPU MCA 82489DX
 *	5	2 CPU ISA+PCI
 *	6	2 CPU EISA+PCI
 *	7	2 CPU MCA+PCI
 */

enum mpBusType {
    MP_BUS_ISA = 1,
    MP_BUS_EISA,
    MP_BUS_PCI,
    MP_BUS_MCA,
};

enum mpIrqSourceTypes {
    mpINT=0,
    mpNMI=1,
    mpSMI=2,
    mpExtINT=3,
};

enum mpConfigEntryTypes {
    MP_PROCESSOR=0,
    MP_BUS,
    MP_IOAPIC,
    MP_INTSRC,
    MP_LINTSRC,
    MP_MAX,
};

#endif
