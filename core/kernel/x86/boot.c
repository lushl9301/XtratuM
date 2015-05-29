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
#include <arch/asm.h>
#include <arch/processor.h>
#include <arch/physmm.h>

xm_u32_t __BOOT BootDetectCpuFeat(void) {
    xm_u32_t feat=0, flags0, flags1, eax, ebx, ecx, edx;
    
    HwSaveFlags(flags0);
    flags1=flags0|_CPU_FLAG_AC;
    HwRestoreFlags(flags1);
    HwSaveFlags(flags1);

    if (!(flags1&_CPU_FLAG_AC))
        return feat;

    flags1=flags0|_CPU_FLAG_ID;
    HwRestoreFlags(flags1);
    HwSaveFlags(flags1);
    if (!(flags1&_CPU_FLAG_ID))
        return feat;
    
    CpuId(0, &eax, &ebx, &ecx, &edx);   
    if (!eax)
        return feat;
    
    feat|=_DETECTED_I586;

    CpuId(1, &eax, &ebx, &ecx, &edx);

    if (edx&_CPUID_PAE)
        feat|=_PAE_SUPPORT;

    if (edx&_CPUID_PSE)
        feat|=_PSE_SUPPORT;

    if (edx&_CPUID_PGE)
        feat|=_PGE_SUPPORT;
        
    if (ecx&_CPUID_X2APIC)
        feat|=_X2APIC_SUPPORT;

//#define _CPUID_REQ_FLG1 (_CPUID_FPU|_CPUID_PSE|_CPUID_MSR|_CPUID_PAE|_CPUID_CX8|_CPUID_PGE|_CPUID_FXSR|_CPUID_CMOV|_CPUID_XMM|_CPUID_XMM2)

    return feat;
}

void __BOOT BootInitPgTab(void) {
    extern xmAddress_t _pgTables[];
    xm_u32_t *ptdL1, *ptdL2;
    xm_s32_t addr;

    ptdL1=(xm_u32_t *)_VIRT2PHYS(_pgTables);
    ptdL2=(xm_u32_t *)(_VIRT2PHYS(_pgTables)+PAGE_SIZE);

    ptdL1[VA2PtdL1(0)]=(xm_u32_t)ptdL2|_PG_ARCH_PRESENT|_PG_ARCH_RW;
    for (addr=LOW_MEMORY_START_ADDR; addr<LOW_MEMORY_END_ADDR; addr+=PAGE_SIZE) {
        ptdL2[VA2PtdL2(addr)]=addr|_PG_ARCH_PRESENT|_PG_ARCH_RW;
    }
    ptdL1[VA2PtdL1(CONFIG_XM_LOAD_ADDR)]=(CONFIG_XM_LOAD_ADDR&LPAGE_MASK)|_PG_ARCH_PRESENT|_PG_ARCH_PSE|_PG_ARCH_RW;
    ptdL1[VA2PtdL1(CONFIG_XM_OFFSET)]=(CONFIG_XM_LOAD_ADDR&LPAGE_MASK)|_PG_ARCH_PRESENT|_PG_ARCH_PSE|_PG_ARCH_RW;
}

