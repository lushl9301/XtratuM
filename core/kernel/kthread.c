/*
 * $FILE: kthread.c
 *
 * Kernel and Guest context
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
#include <rsvmem.h>
#include <ktimer.h>
#include <kthread.h>
#include <physmm.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <xmef.h>

#include <objects/trace.h>
#include <arch/asm.h>
#include <arch/xm_def.h>

static xm_s32_t noVCpus;

static void KThrTimerHndl(kTimer_t *kTimer, void *args) {
    kThread_t *k=(kThread_t *)args;    
    CHECK_KTHR_SANITY(k);
    SetExtIrqPending(k, XM_VT_EXT_HW_TIMER);
}

static void KThrWatchdogTimerHndl(kTimer_t *kTimer, void *args) {
    kThread_t *k=(kThread_t *)args;

    CHECK_KTHR_SANITY(k);
    SetExtIrqPending(k, XM_VT_EXT_WATCHDOG_TIMER);
    RaiseHmPartEvent(XM_HM_EV_WATCHDOG_TIMER,  KID2PARTID(k->ctrl.g->id), KID2VCPUID(k->ctrl.g->id), 0);
}

void SetupKThreads(void) { 
    xm_s32_t e;

    ASSERT(GET_CPU_ID()==0);
    for (e=0, noVCpus=0; e<xmcTab.noPartitions; e++)
        noVCpus+=xmcPartitionTab[e].noVCpus;
}

void InitIdle(kThread_t *idle, xm_s32_t cpu) {
    localCpu_t *lCpu=GET_LOCAL_CPU();

    idle->ctrl.magic1=idle->ctrl.magic2=KTHREAD_MAGIC;
    DynListInit(&idle->ctrl.localActiveKTimers);
    idle->ctrl.lock=SPINLOCK_INIT;
    idle->ctrl.irqCpuCtxt=0;
    idle->ctrl.kStack=0;
    idle->ctrl.g=0;
    //idle->ctrl.localIrqMask=globalIrqMask;
    idle->ctrl.irqMask=HwIrqGetMask()&lCpu->globalIrqMask;
    SetKThreadFlags(idle, KTHREAD_DCACHE_ENABLED_F|KTHREAD_ICACHE_ENABLED_F);
    SetKThreadFlags(idle, KTHREAD_READY_F);
    ClearKThreadFlags(idle, KTHREAD_HALTED_F);
}

void StartUpGuest(xmAddress_t entry) {
    localSched_t *sched=GET_LOCAL_SCHED();   
    kThread_t *k=sched->cKThread;
    //partition_t *p=GetPartition(sched->cKThread);
    cpuCtxt_t ctxt;
    
    //entry=xmcBootPartTab[p->cfg->id].entryPoint;
    KThreadArchInit(k);
    SetKThreadFlags(sched->cKThread, KTHREAD_DCACHE_ENABLED_F|KTHREAD_ICACHE_ENABLED_F);
    SetCacheState(DCACHE|ICACHE);
    ResumeVClock(&k->ctrl.g->vClock, &k->ctrl.g->vTimer);
    SwitchKThreadArchPost(k);

    // JMP_PARTITION must enable interrupts
    JMP_PARTITION(entry, k);
    
    GetCpuCtxt(&ctxt);
    PartitionPanic(&ctxt, __XM_FILE__":%u:0x%x: executing unreachable code!", __LINE__, k);
}

static inline kThread_t *AllocKThread(xmId_t id) {
    kThread_t *k;

    GET_MEMAZ(k, sizeof(kThread_t), ALIGNMENT);
    GET_MEMAZ(k->ctrl.g, sizeof(struct guest), ALIGNMENT);

    k->ctrl.magic1=k->ctrl.magic2=KTHREAD_MAGIC;
    k->ctrl.g->id=id;
    DynListInit(&k->ctrl.localActiveKTimers);
    k->ctrl.lock=SPINLOCK_INIT;
    return k;
}

partition_t *CreatePartition(struct xmcPartition *cfg) {
    xm_u8_t *pct;
    xmSize_t pctSize;
    xm_u32_t localIrqMask;
    partition_t *p;
    xm_s32_t i;

    ASSERT((cfg->id>=0)&&(cfg->id<xmcTab.noPartitions));
    p=&partitionTab[cfg->id];
    GET_MEMAZ(p->kThread, cfg->noVCpus*sizeof(kThread_t *), ALIGNMENT);
    p->cfg=cfg;
    
    pctSize=sizeof(partitionControlTable_t)+sizeof(struct xmPhysicalMemMap)*cfg->noPhysicalMemoryAreas+(cfg->noPorts>>XM_LOG2_WORD_SZ);
    
    if (cfg->noPorts&((1<<XM_LOG2_WORD_SZ)-1))
        pctSize+=sizeof(xmWord_t);
    
    GET_MEMAZ(pct, pctSize*cfg->noVCpus, PAGE_SIZE);
    p->pctArraySize=pctSize*cfg->noVCpus;
    p->pctArray=(xmAddress_t)pct;
    for (i=0; i<cfg->noVCpus; i++) {
        kThread_t *k, *scK;
        xm_s32_t e;
        xm_u32_t cpuId;
        p->kThread[i]=k=AllocKThread(PART_VCPU_ID2KID(cfg->id, i));
        if (cfg->flags&XM_PART_SYSTEM) {
            ClearKThreadFlags(k, KTHREAD_NO_PARTITIONS_FIELD);
            SetKThreadFlags(k, (xmcTab.noPartitions<<16)&KTHREAD_NO_PARTITIONS_FIELD);
        }

        cpuId=xmcVCpuTab[(cfg->id*xmcTab.hpv.noCpus)+i].cpu;

        if (cfg->flags&XM_PART_FP)
            SetKThreadFlags(k, KTHREAD_FP_F);

        scK=(xmcTab.hpv.cpuTab[cpuId].schedPolicy==CYCLIC_SCHED)?k:0;
            
        InitKTimer(cpuId,&k->ctrl.g->kTimer, KThrTimerHndl, k, scK);
        InitKTimer(cpuId,&k->ctrl.g->watchdogTimer, KThrWatchdogTimerHndl, k, scK);
        InitVTimer(cpuId,&k->ctrl.g->vTimer, k);
        
        SetKThreadFlags(k, KTHREAD_HALTED_F);
        localIrqMask=localCpuInfo[cpuId].globalIrqMask;
        for (e=0; e<CONFIG_NO_HWIRQS; e++)
            if (xmcTab.hpv.hwIrqTab[e].owner==cfg->id)
               localIrqMask&=~(1<<e);
            
#ifdef CONFIG_FP_SCHED
        if (xmcTab.hpv.cpuTab[cpuId].schedPolicy==FP_SCHED) {
            xm_u32_t lPriority=~0;
            for (e=0; e<xmcTab.hpv.cpuTab[cpuId].noFpEntries; e++) {
                if ((xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].partitionId==cfg->id)&&(xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].vCpuId==i)){
                   lPriority=xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].priority;
                }
            }
            for (e=0; e<xmcTab.hpv.cpuTab[cpuId].noFpEntries; e++) {
                if (lPriority>=xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].priority){
                   localIrqMask&=~(xmcPartitionTab[xmcFpSchedTab[e+xmcTab.hpv.cpuTab[cpuId].schedFpTabOffset].partitionId].hwIrqs);
                }
            }
        }
#endif

        k->ctrl.irqCpuCtxt=0;
        k->ctrl.irqMask=localIrqMask;
        k->ctrl.g->partCtrlTab=(partitionControlTable_t *)(pct+pctSize*i);
        k->ctrl.g->partCtrlTab->partCtrlTabSize=pctSize;
        SetupKThreadArch(k);
    }
    
    return p;
}

static inline void SetupPct(partitionControlTable_t *partCtrlTab, kThread_t *k, struct xmcPartition *cfg) {
    struct xmPhysicalMemMap *memMap=(struct xmPhysicalMemMap *)((xmAddress_t)partCtrlTab+sizeof(partitionControlTable_t));
    struct xmcMemoryArea *xmcMemArea;
    xmWord_t *commPortBitmap;
    xm_s32_t e, resetCounter;
    partition_t *p = GetPartition(k);

    commPortBitmap=(xmWord_t *)((xmAddress_t)memMap+sizeof(struct xmPhysicalMemMap)*cfg->noPhysicalMemoryAreas);

    resetCounter=partCtrlTab->resetCounter;
    memset(partCtrlTab, 0, sizeof(partitionControlTable_t));
    partCtrlTab->resetCounter=resetCounter;
    partCtrlTab->magic=KTHREAD_MAGIC;
    partCtrlTab->xmVersion=XM_VERSION;
    partCtrlTab->xmAbiVersion=XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION);
    partCtrlTab->xmApiVersion=XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION);
    partCtrlTab->cpuKhz=cpuKhz;
    partCtrlTab->hwIrqs=cfg->hwIrqs;
    partCtrlTab->flags=k->ctrl.flags;
    partCtrlTab->id=k->ctrl.g->id;
    partCtrlTab->noVCpus=cfg->noVCpus;
    partCtrlTab->schedPolicy=xmcTab.hpv.cpuTab[xmcVCpuTab[(KID2PARTID(k->ctrl.g->id)*xmcTab.hpv.noCpus)+KID2VCPUID(k->ctrl.g->id)].cpu].schedPolicy;
    strncpy(partCtrlTab->name, &xmcStringTab[cfg->nameOffset], CONFIG_ID_STRING_LENGTH);
    partCtrlTab->hwIrqsMask|=~0;
    partCtrlTab->extIrqsMask|=~0;
    partCtrlTab->imgStart=p->imgStart;

    InitPCtrlTabIrqs(&partCtrlTab->iFlags);
    partCtrlTab->noPhysicalMemAreas=cfg->noPhysicalMemoryAreas;
    partCtrlTab->partCtrlTabSize=sizeof(partitionControlTable_t)+partCtrlTab->noPhysicalMemAreas*sizeof(struct xmPhysicalMemMap);
    for (e=0; e<cfg->noPhysicalMemoryAreas; e++) {
        xmcMemArea=&xmcPhysMemAreaTab[e+cfg->physicalMemoryAreasOffset];
        if (xmcMemArea->flags&XM_MEM_AREA_TAGGED)
            strncpy(memMap[e].name, &xmcStringTab[xmcMemArea->nameOffset], CONFIG_ID_STRING_LENGTH);
        memMap[e].startAddr=xmcMemArea->startAddr;
        memMap[e].mappedAt=xmcMemArea->mappedAt;
        memMap[e].size=xmcMemArea->size;
        memMap[e].flags=xmcMemArea->flags;
    }

    xmClearBitmap(commPortBitmap, cfg->noPorts);
    partCtrlTab->noCommPorts=cfg->noPorts;
    
    SetupPctMm(partCtrlTab, k);
    SetupPctArch(partCtrlTab, k);
}

void ResetKThread(kThread_t *k, xmAddress_t ptdL1, xmAddress_t entryPoint, xm_u32_t status) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page=NULL;
    xmAddress_t vPtd;
   
    vPtd=EnableByPassMmu(ptdL1,GetPartition(k),&page);
 
    SetupPtdL1((xmWord_t *)vPtd, k);

    DisableByPassMmu(page);

    SetupPct(k->ctrl.g->partCtrlTab, k, GetPartition(k)->cfg);
    k->ctrl.g->partCtrlTab->resetCounter++;
    k->ctrl.g->partCtrlTab->resetStatus=status;
    k->ctrl.g->kArch.ptdL1=ptdL1;
    k->ctrl.g->partCtrlTab->arch._ARCH_PTDL1_REG=ptdL1;

    SetKThreadFlags(k, KTHREAD_READY_F);
    ClearKThreadFlags(k, KTHREAD_HALTED_F);
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_SCHED_MODULE, AUDIT_SCHED_VCPU_RESET, 1, &k->ctrl.g->id);
#endif
    if (k!=sched->cKThread) {
        SetupKStack(k, StartUpGuest, entryPoint);
#ifdef CONFIG_SMP
    if (k->ctrl.g){
       xm_u8_t cpu=xmcVCpuTab[(KID2PARTID(k->ctrl.g->id)*xmcTab.hpv.noCpus)+KID2VCPUID(k->ctrl.g->id)].cpu;
       if (cpu!=GET_CPU_ID())
          SendIpi(cpu,NO_SHORTHAND_IPI,SCHED_PENDING_IPI_VECTOR);
       else
          Schedule();
    }
#else
    Schedule();
#endif
    } else {
        LoadPartitionPageTable(k);
        StartUpGuest(entryPoint);
    }
}

xm_s32_t ResetPartition(partition_t *p, xm_u32_t cold, xm_u32_t status) {
    extern void ResetPartPorts(partition_t *p);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct xmcBootPart *xmcBootPart;
    struct xmImageHdr *xmImageHdr;
    xmAddress_t ptdL1;

    // All the VCpus are halted
    HALT_PARTITION(p->cfg->id);
    xmcBootPart=&xmcBootPartTab[p->cfg->id];
    
    // Is partition image valid?
    if (!(xmcBootPart->flags&XM_PART_BOOT))
        return -1;

    if (!PmmFindArea(xmcBootPart->hdrPhysAddr, sizeof(struct xmImageHdr), p, 0))
        return -1;
 
    xmImageHdr=(struct xmImageHdr *)xmcBootPart->hdrPhysAddr;
    
    if ((ReadByPassMmuWord(&xmImageHdr->sSignature)!=XMEF_PARTITION_MAGIC)||
        (ReadByPassMmuWord(&xmImageHdr->eSignature)!=XMEF_PARTITION_MAGIC))
        return -1;
    
    if (ReadByPassMmuWord(&xmImageHdr->compilationXmAbiVersion)!=XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION)) return -1;
    
    if (ReadByPassMmuWord(&xmImageHdr->compilationXmApiVersion)!=XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION)) return -1;

    if (sched->cKThread==p->kThread[0])
        LoadXmPageTable();

    PmmResetPartition(p);
    ptdL1=SetupPageTable(p, ReadByPassMmuWord(&xmImageHdr->pageTable), ReadByPassMmuWord(&xmImageHdr->pageTableSize));
    if (ptdL1==~0)
        return -1;

    switch (cold){
    case XM_WARM_RESET: /*WARM RESET*/
        p->kThread[0]->ctrl.g->opMode=XM_OPMODE_WARM_RESET;
        p->opMode=XM_OPMODE_WARM_RESET;
        ResetPartPorts(p);
        ResetKThread(p->kThread[0], ptdL1,xmcBootPartTab[p->cfg->id].entryPoint, status);
        break;
    case XM_COLD_RESET: /*COLD RESET -> Partition Loader*/
        p->kThread[0]->ctrl.g->opMode=XM_OPMODE_COLD_RESET;
        p->opMode=XM_OPMODE_COLD_RESET;
        p->kThread[0]->ctrl.g->partCtrlTab->resetCounter=-1;
        ResetPartPorts(p);
        ResetKThread(p->kThread[0], ptdL1,XM_PCTRLTAB_ADDR-256*1024, status);
        break;
    case 2: /*COLD RESET -> used at boot time*/
        p->kThread[0]->ctrl.g->opMode=XM_OPMODE_COLD_RESET;
        p->opMode=XM_OPMODE_COLD_RESET;
        p->kThread[0]->ctrl.g->partCtrlTab->resetCounter=-1;
        ResetPartPorts(p);
        ResetKThread(p->kThread[0], ptdL1,xmcBootPartTab[p->cfg->id].entryPoint, status);
        break;
    }
/*
       ResetPartPorts(p);
       ResetKThread(p->kThread[0], ptdL1,xmcBootPartTab[p->cfg->id].entryPoint, status);
*/

    if (sched->cKThread==p->kThread[0])
        LoadPartitionPageTable(p->kThread[0]);
    
    return 0;
}

