/*
 * $FILE: mpspec.c
 *
 * Intel multiprocessor specification based table parsing
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
#include <arch/paging.h>
#include <assert.h>
#include <boot.h>
#include <sched.h>
#include <smp.h>

#ifdef CONFIG_SMP_INTERFACE_MPSPEC

#define EBDA_BIOS 0x40EUL

static inline xm_u32_t __VBOOT GetBiosEbda(void) {
    xm_u32_t address = *(xm_u16_t *) (EBDA_BIOS);
    address <<= 4;
    return address;
}

static xm_s32_t __VBOOT MpfChecksum(xm_u8_t *mp, xm_s32_t len) {
    xm_s32_t sum = 0;

    while (len--)
        sum += *mp++;

    return sum & 0xFF;
}

static struct mpFloatingPointer *__VBOOT MpScanConfigAt(xmAddress_t base, xmSize_t length) {
    xm_u32_t *bp = (xm_u32_t *) (base);
    struct mpFloatingPointer *mpf;

    while (length > 0) {
        mpf = (struct mpFloatingPointer *) bp;
        if ((*bp == SMP_MAGIC_IDENT) && (mpf->length == 1) && !MpfChecksum((xm_u8_t *) bp, 16)
                && ((mpf->specification == 1) || (mpf->specification == 4))) {
            eprintf(">> SMP MP-table found at 0x%x\n", (mpf));
            return mpf;
        }
        bp += 4;
        length -= 16;
    }
    return 0;
}

static struct mpFloatingPointer *__VBOOT MpScanConfig(void) {
    struct mpFloatingPointer *mpf;

    mpf = NULL;
    if ((mpf=MpScanConfigAt(0x0, 0x400)) || (mpf=MpScanConfigAt(639 * 0x400, 0x400)) || (mpf=MpScanConfigAt(0xf0000, 0x10000)));

    return mpf;
}

static inline xm_s32_t MpAddProc(void *entry) {
    struct mpcConfigProcessor *m = entry;

    if (m->cpuFlag & CPU_ENABLED) {
        if (x86MpConf.noCpu >= CONFIG_NO_CPUS) {
            x86SystemPanic("Only supported %d cpus\n", CONFIG_NO_CPUS);
        }
        x86MpConf.cpu[x86MpConf.noCpu].bsp = (m->cpuFlag & CPU_BOOTPROCESSOR)>>1;
        x86MpConf.cpu[x86MpConf.noCpu].enabled = m->cpuFlag & CPU_ENABLED;
        x86MpConf.cpu[x86MpConf.noCpu].id = m->apicId;  /* Map ApicID to noCpu? */
        x86MpConf.noCpu++;
    }

    return sizeof(struct mpcConfigProcessor);
}

static inline xm_s32_t MpAddBus(void *entry) {
    struct mpcConfigBus *m = entry;

    if (x86MpConf.noBus >= CONFIG_MAX_NO_BUSES) {
        x86SystemPanic("Only supported %d buses\n", CONFIG_MAX_NO_BUSES);
    }
    x86MpConf.bus[x86MpConf.noBus].id = m->busId;
    if (strncmp(m->busType, BUSTYPE_ISA, strlen(BUSTYPE_ISA)) == 0) {
        x86MpConf.bus[x86MpConf.noBus].type = MP_BUS_ISA;
        x86MpConf.bus[x86MpConf.noBus].polarity = BUS_HIGH_POLARITY;
        x86MpConf.bus[x86MpConf.noBus].triggerMode = BUS_EDGE_TRIGGER;
    } else if (strncmp(m->busType, BUSTYPE_PCI, strlen(BUSTYPE_PCI)) == 0) {
        x86MpConf.bus[x86MpConf.noBus].type = MP_BUS_PCI;
        x86MpConf.bus[x86MpConf.noBus].polarity = BUS_LOW_POLARITY;
        x86MpConf.bus[x86MpConf.noBus].triggerMode = BUS_LEVEL_TRIGGER;
    } else {
        x86SystemPanic("Found unknown bus type %s\n", m->busType);
    }
    x86MpConf.noBus++;

    return sizeof(struct mpcConfigBus);
}

static inline xm_s32_t MpAddIoApic(void *entry) {
    struct mpcConfigIoApic *m = entry;

    if (x86MpConf.noIoApic >= CONFIG_MAX_NO_IOAPICS) {
        x86SystemPanic("Only supported %d IO-Apic\n", CONFIG_MAX_NO_IOAPICS);
    }
    x86MpConf.ioApic[x86MpConf.noIoApic].id = m->apicId;
    x86MpConf.ioApic[x86MpConf.noIoApic].baseAddr = m->apicAddr;
    x86MpConf.noIoApic++;

    return sizeof(struct mpcConfigIoApic);
}

static inline xm_s32_t MpAddIntSrc(void *entry) {
    struct mpcConfigIntSrc *m = entry;

    if (x86MpConf.noIoInt >= CONFIG_MAX_NO_IOINT) {
        x86SystemPanic("Only supported %d IO-Irqs", CONFIG_MAX_NO_IOINT);
    }
    x86MpConf.ioInt[x86MpConf.noIoInt].irqType = m->irqType;
    x86MpConf.ioInt[x86MpConf.noIoInt].polarity = m->irqFlag & 0x3;
    x86MpConf.ioInt[x86MpConf.noIoInt].triggerMode = (m->irqFlag >> 2) & 0x3;
    x86MpConf.ioInt[x86MpConf.noIoInt].srcBusId = m->srcBus;
    x86MpConf.ioInt[x86MpConf.noIoInt].srcBusIrq = m->srcBusIrq;
    x86MpConf.ioInt[x86MpConf.noIoInt].dstIoApicId = m->dstApic;
    x86MpConf.ioInt[x86MpConf.noIoInt].dstIoApicIrq = m->dstIrq;

    x86MpConf.noIoInt++;

    return sizeof(struct mpcConfigIntSrc);
}

static inline xm_s32_t MpAddLint(void *entry) {
    x86MpConf.noLInt++;

    return sizeof(struct mpcConfigLIntSrc);
}

static xm_s32_t (*AddMpItem[])(void *entry) = {
        [MP_PROCESSOR] = MpAddProc,
        [MP_BUS] = MpAddBus,
        [MP_IOAPIC] = MpAddIoApic,
        [MP_INTSRC] = MpAddIntSrc,
        [MP_LINTSRC] = MpAddLint,
        [MP_MAX] = 0,
};

static inline void *MpTestConfigTab(struct mpFloatingPointer *mpf) {
    struct mpConfigTable *mpc;
    xm_s32_t stdConf;

    mpc = (void *)((xmAddress_t) mpf->physPtr);
    if ((stdConf = (mpf->feature1 & 0xFF))) {
        x86SystemPanic("Std. configuration %d found\n", stdConf);
    }
    if (!mpf->physPtr) {
        x86SystemPanic("Not std. conf. found and conf. tables missed\n");
    }
    if (mpc->signature != MPC_SIGNATURE) {
        x86SystemPanic("SMP mptable: bad signature [0x%x]\n", mpc->signature);
    }
    if (MpfChecksum((xm_u8_t *)mpc, mpc->length)) {
        x86SystemPanic("SMP mptable: checksum error\n");
    }
    if ((mpc->spec != 0x01) && (mpc->spec != 0x04)) {
        x86SystemPanic("SMP mptable: bad table version (%d)\n", mpc->spec);
    }
    if (!mpc->lApic) {
        x86SystemPanic("SMP mptable: null local APIC address\n");
    }
    x86MpConf.imcr = (mpf->feature2 & ICMR_FLAG) ? 1 : 0;

    return mpc;
}

static void __VBOOT ParseMp(struct mpFloatingPointer *mpf) {
    struct mpConfigTable *mpc;
    xm_s32_t ret, count;
    xm_u8_t *mpt;

    mpc = MpTestConfigTab(mpf);
    count = sizeof(struct mpConfigTable);
    mpt = (xm_u8_t *)mpc + count;
    while (count < mpc->length) {
        if (*mpt < MP_MAX) {
            if (AddMpItem[*mpt]) {
                ret = AddMpItem[*mpt](mpt);
                count += ret;
                mpt += ret;
            }
        } else {
            count = mpc->length;
        }
    }
}

void __VBOOT InitSmpMpspec(void) {
    struct mpFloatingPointer *mpf;

    //if (!(mpf=SmpScanConfig(0x0, 0x400)) && !(mpf=SmpScanConfig(639 * 0x400, 0x400)) && !(mpf=SmpScanConfig(0xf0000, 0x10000))) {
    mpf = MpScanConfig();
    if (!mpf) {
        x86SystemPanic("SMP mpspec: MP-table not found\n");
    }
    ParseMp(mpf);
}

#endif /*CONFIG_SMP_INTERFACE_MPSPEC*/

