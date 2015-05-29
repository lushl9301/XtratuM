/*
 * $FILE: processor.c
 *
 * Processor
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
#include <processor.h>
#include <physmm.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <xmconf.h>
#include <arch/xm_def.h>
#include <arch/segments.h>
#include <arch/io.h>

#define MAX_CPU_ID      16    /* XXX: Should be customized for each processor */

struct localId localIdTab[CONFIG_NO_CPUS];
xm_u32_t cpuFeatures;
void (*Idle)(void);

struct x86Desc gdtTab[(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES)*CONFIG_NO_CPUS];
extern struct x86Desc earlyGdtTab[];

void _Reset(xmAddress_t addr) {
    xmAddress_t *ptdLx;
    xmAddress_t page;
    extern xmAddress_t xmPhys[];
    extern void _Reset2(xmAddress_t);

    LoadXmPageTable();
    page = ((xmAddress_t)_Reset2)&PAGE_MASK;
    ptdLx = (xm_u32_t *)_PHYS2VIRT(SaveCr3());
    ptdLx[VA2PtdL1(page)] = (_VIRT2PHYS((xmAddress_t)xmPhys)&PAGE_MASK)|_PG_ARCH_RW|_PG_ARCH_PRESENT;
    ptdLx=(xmAddress_t *)_PHYS2VIRT(ptdLx[VA2PtdL1(page)]&PAGE_MASK);
    ptdLx[VA2PtdL2(page)]=page|_PG_ARCH_RW|_PG_ARCH_PRESENT;
    FlushTlb();
    _Reset2(addr);
}

void __VBOOT SetupCpu(void) {
}

#ifdef CONFIG_SMP
void __VBOOT SetupCpuIdTab(xm_u32_t noCpus) {
    xm_u32_t e;

    for (e=0;e<noCpus;e++) {
        localIdTab[e].id=e;
    }
}
#endif

void __VBOOT SetupCr(void) {
    LoadCr0(_CR0_PE|_CR0_PG);
    LoadCr4(_CR4_PSE|_CR4_PGE);
}

void __VBOOT SetupGdt(xm_s32_t cpuId) {
    extern void AsmHypercallHndl(void);
    extern void AsmIRetHndl(void);
    struct x86DescReg gdtDesc;

    gdtTab[GDT_ENTRY(cpuId, CS_SEL)]=earlyGdtTab[EARLY_CS_SEL>>3];
    gdtTab[GDT_ENTRY(cpuId, DS_SEL)]=earlyGdtTab[EARLY_DS_SEL>>3];
    
    gdtTab[GDT_ENTRY(cpuId, GUEST_CS_SEL)].low=(((CONFIG_XM_OFFSET-1)>>12)&0xf0000)|0xc0bb00;
    gdtTab[GDT_ENTRY(cpuId, GUEST_CS_SEL)].high=((CONFIG_XM_OFFSET-1)>>12)&0xffff;
    gdtTab[GDT_ENTRY(cpuId, GUEST_DS_SEL)].low=(((CONFIG_XM_OFFSET-1)>>12)&0xf0000)|0xc0b300;
    gdtTab[GDT_ENTRY(cpuId, GUEST_DS_SEL)].high=((CONFIG_XM_OFFSET-1)>>12)&0xffff;

    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].base31_24=((xmAddress_t)&localIdTab[cpuId]>>24)&0xff;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].granularity=0xc;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].segLimit19_16=0xf;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].access=0x93;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].base23_16=((xmAddress_t)&localIdTab[cpuId]>>16)&0xff;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].base15_0=(xmAddress_t)&localIdTab[cpuId]&0xffff;
    gdtTab[GDT_ENTRY(cpuId, PERCPU_SEL)].segLimit15_0=0xffff;

    gdtTab[GDT_ENTRY(cpuId, XM_HYPERCALL_CALLGATE_SEL)].segSel=CS_SEL;
    gdtTab[GDT_ENTRY(cpuId, XM_HYPERCALL_CALLGATE_SEL)].offset15_0=(xmAddress_t)AsmHypercallHndl&0xffff;
    gdtTab[GDT_ENTRY(cpuId, XM_HYPERCALL_CALLGATE_SEL)].wordCount=1;
    gdtTab[GDT_ENTRY(cpuId, XM_HYPERCALL_CALLGATE_SEL)].access=0x8c|(2<<5);
    gdtTab[GDT_ENTRY(cpuId, XM_HYPERCALL_CALLGATE_SEL)].offset31_16=((xmAddress_t)AsmHypercallHndl&0xffff0000)>>16;

    gdtTab[GDT_ENTRY(cpuId, XM_IRET_CALLGATE_SEL)].segSel=CS_SEL;
    gdtTab[GDT_ENTRY(cpuId, XM_IRET_CALLGATE_SEL)].offset15_0=(xmAddress_t)AsmIRetHndl&0xffff;
    gdtTab[GDT_ENTRY(cpuId, XM_IRET_CALLGATE_SEL)].wordCount=1;
    gdtTab[GDT_ENTRY(cpuId, XM_IRET_CALLGATE_SEL)].access=0x8c|(2<<5);
    gdtTab[GDT_ENTRY(cpuId, XM_IRET_CALLGATE_SEL)].offset31_16=((xmAddress_t)AsmIRetHndl&0xffff0000)>>16;

    gdtDesc.limit=(sizeof(struct x86Desc)*(CONFIG_PARTITION_NO_GDT_ENTRIES+XM_GDT_ENTRIES)*CONFIG_NO_CPUS)-1;
    gdtDesc.linearBase=(xmAddress_t)&gdtTab[GDT_ENTRY(cpuId, 0)];
    
    LoadGdt(gdtDesc);
    LoadSegSel(CS_SEL, DS_SEL);
    LoadGs(PERCPU_SEL);
}

xm_u32_t __ArchGetLocalId(void) {
    xm_u32_t id;
    __asm__ __volatile__ ("mov %%gs:0, %0\n\t" :"=r" (id));
    return id;
}

xm_u32_t __ArchGetLocalHwId(void) {
    xm_u32_t hwId;
    __asm__ __volatile__ ("mov %%gs:4, %0\n\t" :"=r" (hwId));
    return hwId;
}

void __ArchSetLocalId(xm_u32_t id) {
    __asm__ __volatile__ ("movl %0, %%gs:0\n\t" ::"r" (id));
}

void __ArchSetLocalHwId(xm_u32_t hwId) {
    __asm__ __volatile__ ("movl %0, %%gs:4\n\t" ::"r" (hwId));
}

#define PIT_CH2             0x42
#define PIT_MODE            0x43
#define CALIBRATE_CYCLES    14551
#define CALIBRATE_MULT      82

__VBOOT xm_u32_t CalculateCpuFreq(void) {
    xm_u64_t cStart, cStop, delta;

    OutB((InB(0x61) & ~0x02) | 0x01, 0x61);
    OutB(0xb0, PIT_MODE);
    OutB(CALIBRATE_CYCLES & 0xff, PIT_CH2);
    OutB(CALIBRATE_CYCLES >> 8, PIT_CH2);
    cStart = RdTscLL();
    delta = RdTscLL();
    InB(0x61);
    delta = RdTscLL()-delta;
    while ((InB(0x61) & 0x20) == 0);
    cStop = RdTscLL();

    return (cStop-(cStart+delta))*CALIBRATE_MULT;
}
