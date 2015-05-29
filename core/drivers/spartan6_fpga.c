/*
 * $FILE: spartan6_fpga.c
 *
 * Add external syncronization signal to the FPGA SPARTAN-6 in the MultiPARTES project.
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <kdevice.h>
#if defined(CONFIG_LEON3)&&defined(CONFIG_EXT_SYNC_MPT_IO)&&defined(CONFIG_PLAN_EXTSYNC)
#include <boot.h>
#include <arch/leon.h>
#include <objdir.h>
#include <processor.h>

#define EXTERNAL_SYNC_MPT_SPARTAN_6_FPGA 5

#define LEON_GPIO_IO_IN              (LEON_GPIO_BASE+0x00)
#define LEON_GPIO_IO_OUTPUT          (LEON_GPIO_BASE+0x04)
#define LEON_GPIO_IO_DIR             (LEON_GPIO_BASE+0x08)
#define LEON_GPIO_IRQ_MASK           (LEON_GPIO_BASE+0x0C)
#define LEON_GPIO_IRQ_POLARITY       (LEON_GPIO_BASE+0x10)
#define LEON_GPIO_IRQ_EDGE           (LEON_GPIO_BASE+0x14)

RESERVE_HWIRQ(EXTERNAL_SYNC_MPT_SPARTAN_6_FPGA);

static const kDevice_t spartan6Dev={
    .subId=0,
    .Reset=0,
    .Write=0,
    .Read=0,
    .Seek=0,
};

static const kDevice_t *GetSpartan6ExtSync(xm_u32_t subId) {
    return &spartan6Dev;
}

xm_s32_t InitExtSync(void) {
    eprintf("Init MPT Ext Sync\n");
    StoreIoReg(LEON_GPIO_IO_DIR,0x0f);
    StoreIoReg(LEON_GPIO_IRQ_EDGE,0x10);
    StoreIoReg(LEON_GPIO_IRQ_MASK,0x10);
    GetKDevTab[XM_DEV_SPARTAN6_EXTSYNC_ID]=GetSpartan6ExtSync;
    return 0;
}

REGISTER_KDEV_SETUP(InitExtSync);

#endif

