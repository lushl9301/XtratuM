/*
 * $FILE: setup.c
 *
 * Setting up and starting up the kernel
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions. All rights reserved.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <comp.h>
#include <rsvmem.h>
#include <kdevice.h>
#include <ktimer.h>
#include <stdc.h>
#include <irqs.h>
#include <objdir.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <spinlock.h>
#include <smp.h>
#include <kthread.h>
#include <vmmap.h>
#include <virtmm.h>
#include <xmconf.h>

#include <objects/console.h>
#include <arch/paging.h>
#include <arch/xm_def.h>

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#include <drivers/ttnocports.h>
#endif

// CPU's frequency
xm_u32_t cpuKhz;
xm_u16_t __nrCpus=0;


struct xmcPartition *xmcPartitionTab;
struct xmcMemoryRegion *xmcMemRegTab;
struct xmcMemoryArea *xmcPhysMemAreaTab;
struct xmcCommChannel *xmcCommChannelTab;
struct xmcCommPort *xmcCommPorts;
struct xmcIoPort *xmcIoPortTab;
#ifdef CONFIG_CYCLIC_SCHED
struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
#endif
struct xmcRsvMem *xmcRsvMemTab;
struct xmcBootPart *xmcBootPartTab;
struct xmcRswInfo *xmcRswInfo;
xm_u8_t *xmcDstIpvi;
xm_s8_t *xmcStringTab;
#ifdef CONFIG_FP_SCHED
struct xmcFpSched *xmcFpSchedTab;
#endif
struct xmcVCpu *xmcVCpuTab;

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
struct xmcMemBlock *xmcMemBlockTab;
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
struct xmcTTnocSlot *xmcTTnocSlotTab;
#endif

// Local info
localCpu_t localCpuInfo[CONFIG_NO_CPUS];
localTime_t localTimeInfo[CONFIG_NO_CPUS];
localSched_t localSchedInfo[CONFIG_NO_CPUS];

extern xm_u32_t resetStatusInit[];

barrier_t smpStartBarrier=BARRIER_INIT;
barrier_t smpBootBarrier=BARRIER_INIT;
barrierMask_t smpBarrierMask=BARRIER_MASK_INIT;

static volatile xm_s32_t procWaiting __VBOOTDATA=0;

extern xm_u8_t _sxm[], _exm[], physXmcTab[];
extern void start(void);

struct xmHdr xmHdr __XMHDR={
    .sSignature=XMEF_XM_MAGIC,
    .compilationXmAbiVersion=XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION),
    .compilationXmApiVersion=XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION),
    .noCustomFiles=1,
    .customFileTab={
	[0]={.sAddr=(xmAddress_t)physXmcTab, .size=0,},
    },
    .eSignature=XMEF_XM_MAGIC,
};

extern __NOINLINE void FreeBootMem(void);

void IdleTask(void) {
    while(1) {
	DoPreemption();
    }
}

void HaltSystem(void) {
    extern void __HaltSystem(void);
#ifdef CONFIG_SMP
    if (GET_CPU_ID()==0)
#endif
#ifdef CONFIG_DEBUG
       kprintf("System halted.\n");
#endif
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_HYP_HALT, 0, 0);
#endif
#ifdef CONFIG_SMP
    SmpHaltAll();
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateStateHyp(XM_STATUS_HALTED);
    setManualStatePart(0xff);
    sendStateToAllNodes();
#endif
    __HaltSystem();
}

void ResetSystem(xm_u32_t resetMode) {
    extern xm_u32_t sysResetCounter[];
    extern void start(void);
    extern void _Reset(xmAddress_t);
    cpuCtxt_t ctxt;

    ASSERT(!HwIsSti());
    sysResetCounter[0]++;
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_HYP_RESET, 1, (xmWord_t *)&resetMode);
#endif
    if ((resetMode&XM_RESET_MODE)==XM_WARM_RESET) {
	_Reset((xmAddress_t)start);
    } else { // Cold Reset       
        sysResetCounter[0]=0;
	_Reset((xmAddress_t)start);
    }    
    GetCpuCtxt(&ctxt);
    SystemPanic(&ctxt, "Unreachable point\n");
}

static void __VBOOT CreateLocalInfo(void) {
    xm_s32_t e;
    if (!GET_NRCPUS()) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
        SystemPanic(&ctxt, "No cpu found in the system\n");    
    }
    memset(localCpuInfo, 0, sizeof(localCpu_t)*CONFIG_NO_CPUS);
    memset(localTimeInfo, 0, sizeof(localTime_t)*CONFIG_NO_CPUS);
    memset(localSchedInfo, 0, sizeof(localSched_t)*CONFIG_NO_CPUS);
    for (e=0; e<CONFIG_NO_CPUS; e++)
        localCpuInfo[e].globalIrqMask=~0;
}

static void __VBOOT LocalSetup(xm_s32_t cpuId, kThread_t *idle) {
    ASSERT(!HwIsSti());
    ASSERT(xmcTab.hpv.noCpus>cpuId);
    SetupCpu();
    SetupArchLocal(cpuId);
    SetupHwTimer();
    SetupKTimers();
    InitSchedLocal(idle);
#ifdef CONFIG_SMP
    BarrierWriteMask(&smpBarrierMask);
#endif

}

static void __VBOOT SetupPartitions(void) {
    xmAddress_t st, end, vSt, vEnd;
    partition_t *p;
    xm_s32_t e, a;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateInfoNode();
    updateStateHyp(XM_STATUS_READY);
//    sendStateToAllNodes();
#endif
    kprintf ("%d Partition(s) created\n", xmcTab.noPartitions);

    // Creating the partitions
    for (e=0; e<xmcTab.noPartitions; e++) {
        if ((p=CreatePartition(&xmcPartitionTab[e]))) {
            kprintf("P%d (\"%s\":%d:%d) flags: [", e, &xmcStringTab[xmcPartitionTab[e].nameOffset], xmcPartitionTab[e].id, xmcPartitionTab[e].noVCpus);
	    if (xmcPartitionTab[e].flags&XM_PART_SYSTEM)
		kprintf(" SYSTEM");
	    if (xmcPartitionTab[e].flags&XM_PART_FP)
		kprintf(" FP");
	    kprintf(" ]:\n");
	    for (a=0; a<xmcPartitionTab[e].noPhysicalMemoryAreas; a++) {
		st=xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].startAddr;
		end=st+xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].size-1;
                vSt=xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].mappedAt;
		vEnd=vSt+xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].size-1;
                
		kprintf("    [0x%lx:0x%lx - 0x%lx:0x%lx]", st, vSt, end, vEnd);
		kprintf(" flags: 0x%x", xmcPhysMemAreaTab[a+xmcPartitionTab[e].physicalMemoryAreasOffset].flags);
		kprintf("\n");
	    }

            if (xmcBootPartTab[e].flags&XM_PART_BOOT){
                if (ResetPartition(p, XM_COLD_RESET, resetStatusInit[0])<0)
                    kprintf("Unable to reset partition %d\n", p->cfg->id);
                p->opMode=XM_OPMODE_IDLE;
            }
                    
	} else {
            cpuCtxt_t ctxt;
            GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt, "[LoadGuests] Error creating partition");
        }
    }
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    updateStateAllPart();
//    sendStateToAllNodes();
    receiveInitStateFromAllNodes();
#endif
#ifdef CONFIG_FP_SCHED
    xm_s32_t cpuId;
    for (cpuId=0;cpuId<GET_NRCPUS();cpuId++){
        if (xmcTab.hpv.cpuTab[cpuId].schedPolicy==FP_SCHED) {
            localCpu_t *lCpu=&localCpuInfo[cpuId];
            xm_u32_t gIrqMask=~0;
            xm_s32_t e;
            for (e=0; e<xmcTab.hpv.cpuTab[cpuId].noFpEntries; e++)
                gIrqMask&=~(xmcPartitionTab[xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].partitionId].hwIrqs);
            
            lCpu->globalIrqMask&=gIrqMask;
        }
    }
#endif
}

static void __VBOOT LoadCfgTab(void) {
    // Check configuration file
    if (xmcTab.signature!=XMC_SIGNATURE)
	HaltSystem();
#define CALC_ABS_ADDR_XMC(_offset) (void *)(xmcTab._offset+(xmAddress_t)&xmcTab)
    
    xmcPartitionTab=CALC_ABS_ADDR_XMC(partitionTabOffset);
    xmcBootPartTab=CALC_ABS_ADDR_XMC(bootPartitionTabOffset);
    xmcRswInfo=CALC_ABS_ADDR_XMC(rswInfoOffset);
    xmcMemRegTab=CALC_ABS_ADDR_XMC(memoryRegionsOffset);
    xmcPhysMemAreaTab=CALC_ABS_ADDR_XMC(physicalMemoryAreasOffset);
    xmcCommChannelTab=CALC_ABS_ADDR_XMC(commChannelTabOffset);
    xmcCommPorts=CALC_ABS_ADDR_XMC(commPortsOffset);
    xmcIoPortTab=CALC_ABS_ADDR_XMC(ioPortsOffset);
#ifdef CONFIG_CYCLIC_SCHED
    xmcSchedCyclicSlotTab=CALC_ABS_ADDR_XMC(schedCyclicSlotsOffset);
    xmcSchedCyclicPlanTab=CALC_ABS_ADDR_XMC(schedCyclicPlansOffset);
#endif
    xmcStringTab=CALC_ABS_ADDR_XMC(stringsOffset);   
    xmcRsvMemTab=CALC_ABS_ADDR_XMC(rsvMemTabOffset);
    xmcDstIpvi=CALC_ABS_ADDR_XMC(ipviDstOffset);
#ifdef CONFIG_FP_SCHED
    xmcFpSchedTab=CALC_ABS_ADDR_XMC(fpSchedTabOffset);
#endif
    xmcVCpuTab=CALC_ABS_ADDR_XMC(vCpuTabOffset);


#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
    xmcMemBlockTab=CALC_ABS_ADDR_XMC(deviceTab.memBlocksOffset);
#endif

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    xmcTTnocSlotTab=CALC_ABS_ADDR_XMC(deviceTab.ttnocSlotOffset);
#endif
}

void __VBOOT Setup(xm_s32_t cpuId, kThread_t *idle) {
#ifdef CONFIG_EARLY_OUTPUT
    extern void SetupEarlyOutput(void);
#endif

    ASSERT(!HwIsSti());
    ASSERT(GET_CPU_ID()==0);
#ifdef CONFIG_EARLY_OUTPUT
    SetupEarlyOutput();
#endif
    LoadCfgTab();
    InitRsvMem();
    EarlySetupArchCommon();
    SetupVirtMM();
    SetupPhysMM();
    SetupArchCommon();
    CreateLocalInfo();
    SetupIrqs();

    SetupKDev();
    SetupObjDir();

    kprintf("XM Hypervisor (%x.%x r%x)\n", (XM_VERSION>>16)&0xFF, (XM_VERSION>>8)&0xFF, XM_VERSION&0xFF);
    kprintf("Detected %lu.%luMHz processor.\n", (xm_u32_t)(cpuKhz/1000), (xm_u32_t)(cpuKhz%1000));
    BarrierLock(&smpStartBarrier);
    InitSched();
    SetupSysClock();
    LocalSetup(cpuId, idle);

#ifdef CONFIG_SMP
    SetupSmp();
    BarrierWaitMask(&smpBarrierMask);
#endif
    SetupPartitions();
#ifdef CONFIG_DEBUG
    RsvMemDebug();
#endif
    FreeBootMem();
}

#ifdef CONFIG_SMP

void __VBOOT InitSecondaryCpu(xm_s32_t cpuId, kThread_t *idle) {
    ASSERT(GET_CPU_ID()!=0);

    LocalSetup(cpuId, idle);
    BarrierWait(&smpStartBarrier);
    GET_LOCAL_SCHED()->flags|=LOCAL_SCHED_ENABLED;
    Schedule();
    IdleTask();
}

#endif

