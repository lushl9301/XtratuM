/*
 * $FILE: smp.c
 *
 * Symmetric multi-processor
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

#define SMP_RM_START_ADDR 0x1000

#ifdef CONFIG_SMP

struct x86MpConf x86MpConf;
extern xm_s32_t UDelay(xm_u32_t usec);
extern void SetupCpuIdTab(xm_u32_t noCpus);
extern void InitSmpAcpi(void);
extern void InitSmpMpspec(void);

xm_s32_t __VBOOT InitSmp(void)
{
    memset(&x86MpConf, 0, sizeof(struct x86MpConf));
#ifdef CONFIG_SMP_INTERFACE_ACPI
    InitSmpAcpi();
#endif
#ifdef CONFIG_SMP_INTERFACE_MPSPEC
    InitSmpMpspec();
#endif
    return x86MpConf.noCpu;
}

static xm_u32_t ApicWaitForDelivery(void)
{
    xm_u32_t timeout, sendPending;

    sendPending = 1;
    for (timeout=0; timeout<1000 && (sendPending != 0); timeout++) {
        UDelay(100);
        sendPending = LApicRead(APIC_ICR_LOW) & APIC_ICR_BUSY;
    }

    return sendPending;
}

static void WakeupAp(xm_u32_t startEip, xm_u32_t cpuId)
{
    xm_u32_t i, sendStatus, acceptStatus, maxLvt;

    LApicWrite(APIC_ICR_HIGH, SET_APIC_DEST_FIELD(cpuId));
    LApicWrite(APIC_ICR_LOW, APIC_INT_LEVELTRIG | APIC_INT_ASSERT | APIC_DM_INIT);
    ApicWaitForDelivery();
    UDelay(10000);
    LApicWrite(APIC_ICR_HIGH, SET_APIC_DEST_FIELD(cpuId));
    LApicWrite(APIC_ICR_LOW, APIC_INT_LEVELTRIG | APIC_DM_INIT);
    ApicWaitForDelivery();

    maxLvt = LApicGetMaxLvt();

    for (i=0; i<2; i++) {
        LApicWrite(APIC_ESR, 0);
        LApicRead(APIC_ESR);
        LApicWrite(APIC_ICR_HIGH, SET_APIC_DEST_FIELD(cpuId));
        LApicWrite(APIC_ICR_LOW, APIC_DM_STARTUP | (startEip >> 12));
        UDelay(300);
        sendStatus = ApicWaitForDelivery();
        UDelay(200);

        if (maxLvt > 3) {
            LApicRead(APIC_SVR);
            LApicWrite(APIC_ESR, 0);
        }

        acceptStatus = LApicRead(APIC_ESR) & 0xEF;
        if (sendStatus || acceptStatus) {
            break;
        }
    }
}

static inline void __VBOOT SetupApStack(xm_u32_t ncpu)
{
    extern struct {
        volatile xm_u32_t *esp;
        volatile xm_u16_t ss;
    } _sstack;
    xm_u32_t *ptr;

    ptr = (xm_u32_t *) ((xm_u32_t) _sstack.esp + CONFIG_KSTACK_SIZE);
    *(--ptr) = (xm_u32_t) _sstack.esp;
    *(--ptr) = ncpu;
    _sstack.esp = ptr;
}

void __VBOOT SetupSmp(void)
{
    extern const xm_u8_t smpStart16[], smpStart16End[];
    extern volatile xm_u8_t aspReady[];
    xm_u32_t startEip, ncpu;

    startEip = SMP_RM_START_ADDR;

    FlushTlb();
    SET_NRCPUS((GET_NRCPUS()<xmcTab.hpv.noCpus)?GET_NRCPUS():xmcTab.hpv.noCpus);
    if (GET_NRCPUS() > 1) {
        SetupCpuIdTab(GET_NRCPUS());
        for (ncpu=0; ncpu < GET_NRCPUS(); ncpu++) {     /* XXX: This code assumes that the BSP will be found here */
            if (x86MpConf.cpu[ncpu].enabled && !x86MpConf.cpu[ncpu].bsp) {
                kprintf("Waking up (%d) AP CPU\n", ncpu);
                aspReady[0] &= ~0x80;
                SetupApStack(ncpu);
                memcpy((xm_u8_t *)startEip, (xm_u8_t *)_PHYS2VIRT(smpStart16), smpStart16End-smpStart16);
                WakeupAp(startEip, x86MpConf.cpu[ncpu].id);
                while ((aspReady[0] & 0x80) != 0x80);
            }
        }
    }
}

#else
xm_s32_t __VBOOT InitSmp(void)
{
    return 0;
}
#endif /* CONFIG_SMP */



