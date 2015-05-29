/*
 * $FILE: setup.c
 *
 * Setting up and starting up the kernel (arch dependent part)
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
#include <smp.h>
#include <stdc.h>
#include <physmm.h>
#include <arch/pic.h>

volatile xm_s8_t localInfoInit=0;

void __VBOOT SetupArchLocal(xm_s32_t cpuId) {
    extern void InitLApic(xm_s32_t cpuId);

    localInfoInit=1;
    SET_CPU_ID(cpuId);
#ifdef CONFIG_APIC
    SET_CPU_HWID(x86MpConf.cpu[cpuId].id);
    InitLApic(x86MpConf.cpu[cpuId].id);
#else
    SET_CPU_HWID(cpuId);
#endif
}

void __VBOOT EarlySetupArchCommon(void) {
    extern void SetupX86Idt(void);

    SET_NRCPUS(1);
    SetupX86Idt();
}

xm_u32_t __VBOOT GetCpuKhz(void) {
    extern xm_u32_t CalculateCpuFreq(void);
    xm_u32_t cpuKhz=xmcTab.hpv.cpuTab[GET_CPU_ID()].freq;
    if (cpuKhz==XM_CPUFREQ_AUTO)
    cpuKhz=CalculateCpuFreq()/1000;

    return cpuKhz;
}

void __VBOOT SetupArchCommon(void) {
#ifdef CONFIG_HPET
    extern void InitHpet(void);

    InitHpet();
#endif
    cpuKhz=GetCpuKhz();
    InitPic(0x20, 0x28);
#ifdef CONFIG_SMP
    SET_NRCPUS(InitSmp());
    SetupApicCommon();
#endif
}

void __VBOOT EarlyDelay(xm_u32_t cycles) {
    hwTime_t t, c;
    t=RdTscLL()+cycles;
    do {
        c=RdTscLL();
    } while(t>c);
}
