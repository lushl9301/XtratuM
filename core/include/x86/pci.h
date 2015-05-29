/*
 * $FILE: pci.h
 *
 * PCI related
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_PCI_H_
#define _XM_ARCH_PCI_H_

#define PCI_CONFIG_DATA    0xcfc
#define PCI_CONFIG_ADDRESS 0xcf8

#define PCI_MAXBUSES       256
#define PCI_MAXDEVICES     32
#define PCI_MAXFUNCTIONS   8

#define MKPCIID(bus, device, function)	(((bus) << 8) | ((device) << 3) | (function))
#define PCI_BUS(pciid)			(((pciid) >> 8) & 0xff)
#define PCI_DEVICE(pciid)		(((pciid) >> 3) & 0x1f)
#define PCI_FUNC(pciid)			((pciid) & 0x7)

enum pci_magic_e {
    PCI_VENDOR_INVALID = 0xffff,
    PCI_IDNULL = -1,
    PCI_IDZERO = 0,
    PCI_BAR_IOMASK = 0xfffffffe,
};

enum pci_conf_space_reg_e {
    PCI_DEVICE_VENDOR = 0x00,
    PCI_CLASS_REVISION = 0x08,
    PCI_BAR0 = 0x10,
    PCI_BAR1 = 0x14,
    PCI_BAR2 = 0x18,
    PCI_BAR3 = 0x1c,
    PCI_BAR4 = 0x20,
    PCI_BAR5 = 0x24,
    PCI_INTERRUPT = 0x3c,
};

typedef int pciid_t;

typedef struct pcidev_s {
    pciid_t pci_id;
    xm_u16_t vendor;
    xm_u16_t deviceid;
    xm_u16_t revision;
    xm_u32_t class;
    xm_u32_t BAR[6];
    xm_s32_t irq;
} pcidev_t;

static inline xm_u8_t PciReadConfigByte (pciid_t id, xm_u8_t offset) {
    OutL ((1 << 31) | (id << 8) | offset, PCI_CONFIG_ADDRESS);
    return InB (PCI_CONFIG_DATA + (offset&3));
}

static inline xm_u32_t PciReadConfig (pciid_t id, xm_u8_t offset) {
    OutL ((1 << 31) | (id << 8) | offset, PCI_CONFIG_ADDRESS);
    return InL (PCI_CONFIG_DATA);
}

static inline pciid_t PciNextDev (pciid_t id) {
    xm_s32_t bus, dev, fn;

    bus = PCI_BUS(id);
    dev = PCI_DEVICE (id);
    fn = PCI_FUNC (id);
    fn++;
    if (fn >= PCI_MAXFUNCTIONS) {
        fn = 0;
        dev ++;
    }
    if (dev >= PCI_MAXDEVICES) {
        dev = 0;
        bus++;
    }
    if (bus >= PCI_MAXBUSES)
        return PCI_IDNULL;
    return MKPCIID (bus, dev, fn);
}

static inline xm_s32_t PciIdInfo (pciid_t id, pcidev_t *dev) {
    dev->pci_id = id;
    dev->vendor = PciReadConfig (id, PCI_DEVICE_VENDOR) & 0xffff;
    if (dev->vendor == PCI_VENDOR_INVALID)
        return PCI_VENDOR_INVALID;

    dev->revision = PciReadConfig (id, PCI_CLASS_REVISION) & 0xff;
    dev->deviceid = (PciReadConfig (id, PCI_DEVICE_VENDOR) >> 16) & 0xffff;
    dev->class = (PciReadConfig (id, PCI_CLASS_REVISION) >> 8) & 0xffffff;
    dev->BAR[0] = PciReadConfig (id, PCI_BAR0);
    dev->BAR[1] = PciReadConfig (id, PCI_BAR1);
    dev->BAR[2] = PciReadConfig (id, PCI_BAR2);
    dev->BAR[3] = PciReadConfig (id, PCI_BAR3);
    dev->BAR[4] = PciReadConfig (id, PCI_BAR4);
    dev->BAR[5] = PciReadConfig (id, PCI_BAR5);
    dev->irq = PciReadConfig (id, PCI_INTERRUPT) & 0xff;
    return 0;
}

#endif
