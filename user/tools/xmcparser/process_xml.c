/*
 * $FILE: process_xml.c
 *
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <string.h>
#include "common.h"
#include "parser.h"
#include "conv.h"
#include "checks.h"
#include "xmc.h"

#include <xm_inc/sched.h>

static int cPartition=0;
#ifdef CONFIG_CYCLIC_SCHED
static int expectedSlotNr=0, cProcCyclicPlan=0;
#endif

static void NameMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].nameOffset=AddString((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].name=node->line;
    xmcMemAreaTab[C_PHYSMEMAREA].flags|=XM_MEM_AREA_TAGGED;
}

static struct attrXml nameMemArea_A={BAD_CAST"name", NameMemArea_AH};

static void StartMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].startAddr=ToU32((char *)val, 16);
    xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=ToU32((char *)val, 16);
    // if (cPartition==-2)
    //  xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=CONFIG_XM_OFFSET;

    xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;
    xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
}

static struct attrXml startMemArea_A={BAD_CAST"start", StartMemArea_AH};

static void VirtAddrMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=ToU32((char *)val, 16);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
}

static struct attrXml mappedAtMemArea_A={BAD_CAST"mappedAt", VirtAddrMemArea_AH};

static void SizeMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].size=ToSize((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].size=node->line;
}

static struct attrXml sizeMemArea_A={BAD_CAST"size", SizeMemArea_AH};

static void FlagsMemArea_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].flags|=ToPhysMemAreaFlags((char *)val, node->line);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].flags=node->line;
}

static struct attrXml flagsMemArea_A={BAD_CAST"flags", FlagsMemArea_AH};

static xm_s32_t *noMemAreaPtr=0;
static void Area_NH0(xmlNodePtr node) {
    if (noMemAreaPtr)
        (*noMemAreaPtr)++;
    xmc.noPhysicalMemoryAreas++;
    DO_REALLOC(xmcMemAreaTab, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryArea));
    DO_REALLOC(xmcMemAreaTabNoL, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryAreaNoL));
    memset(&xmcMemAreaTab[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryArea));
    memset(&xmcMemAreaTabNoL[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryAreaNoL));
    xmcMemAreaTabNoL[C_PHYSMEMAREA].line=node->line;
    xmcMemAreaTab[C_PHYSMEMAREA].nameOffset=0;
    if (cPartition==-2) {
        xmcMemAreaTab[C_PHYSMEMAREA].startAddr=CONFIG_XM_LOAD_ADDR;
        xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;
        xmcMemAreaTab[C_PHYSMEMAREA].mappedAt=CONFIG_XM_OFFSET;
        xmcMemAreaTabNoL[C_PHYSMEMAREA].mappedAt=node->line;
    }
}

static void Area_NH1(xmlNodePtr node) {
    xmcMemAreaTabNoL[C_PHYSMEMAREA].partitionId=cPartition;
    xmcMemAreaTab[C_PHYSMEMAREA].memoryRegionOffset=CheckPhysMemArea(C_PHYSMEMAREA);
}

static struct nodeXml area_N={BAD_CAST"Area", Area_NH0, Area_NH1, 0, (struct attrXml *[]){&nameMemArea_A, &startMemArea_A, &sizeMemArea_A, &flagsMemArea_A, &mappedAtMemArea_A, 0}, 0};

static struct nodeXml memArea_N={BAD_CAST"PhysicalMemoryAreas", 0, 0, 0, 0, (struct nodeXml *[]){&area_N, 0}};

static struct nodeXml hypMemArea_N={BAD_CAST"PhysicalMemoryArea", Area_NH0, Area_NH1, 0, (struct attrXml *[]){&sizeMemArea_A, &flagsMemArea_A, 0}, 0};

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)

static xm_s32_t nodeIdLinks[CONFIG_TTNOC_NODES-1];

static void NodeIdLink_AH(xmlNodePtr node, const xmlChar *val) {
    nodeIdLinks[C_TTNOCLINKS]=ToU32((char *)val, 10);
}

static struct attrXml nodeIdLink_A={BAD_CAST"nodeId", NodeIdLink_AH};

static void TxRxPortLink_AH(xmlNodePtr node, const xmlChar *val) {
   xmDev_t idTTnocSlot=LookUpDev((char *)val, node->line);
   xm_s32_t i=0;
   while ((i<xmc.deviceTab.noTTnocSlots)&&(idTTnocSlot.subId!=xmcTTnocSlotTab[i].devId.subId))
       i++;
   if (i!=xmc.deviceTab.noTTnocSlots)
      xmcTTnocSlotTab[i].nodeId=nodeIdLinks[C_TTNOCLINKS];
 
}

static struct attrXml txPortLink_A={BAD_CAST"txPort", TxRxPortLink_AH};
static struct attrXml rxPortLink_A={BAD_CAST"rxPort", TxRxPortLink_AH};

static void Link_NH0(xmlNodePtr node) {
   noTTnocLinks++;
}

static struct nodeXml link_N={BAD_CAST"Link", Link_NH0, 0, 0, (struct attrXml *[]){&nodeIdLink_A, &txPortLink_A, &rxPortLink_A, 0}, 0};

static struct nodeXml ttnocLink_N={BAD_CAST"TTnocLinks", 0, 0, 0, 0, (struct nodeXml *[]){&link_N, 0}};

#endif

static int hmSlotNo=0, hmSlotLine=0;
static struct xmcHmSlot *hmTabPtr=0;

static void NameHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmSlotNo=ToHmEvent((char *)val, node->line);
}

static struct attrXml nameHMEvent_A={BAD_CAST"name", NameHMEvent_AH};

static void ActionHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmTabPtr[hmSlotNo].action=ToHmAction((char *)val, node->line);
}

static struct attrXml actionHMEvent_A={BAD_CAST"action", ActionHMEvent_AH};

static void LogHMEvent_AH(xmlNodePtr node, const xmlChar *val) {
    hmTabPtr[hmSlotNo].log=ToYNTF((char *)val, node->line);
}

static struct attrXml logHMEvent_A={BAD_CAST"log", LogHMEvent_AH};

static void EventHealthMonitor_NH0(xmlNodePtr node) {
    hmSlotLine=node->line;
    if (cPartition<0) 
        HmHpvIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
    else
        HmPartIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
}

static void EventHealthMonitor_NH1(xmlNodePtr node) {
    if (cPartition<0) 
        HmHpvIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine); 
    else
        HmPartIsActionPermittedOnEvent(hmSlotNo, hmTabPtr[hmSlotNo].action, hmSlotLine);
}

static struct nodeXml eventHealthMonitor_N={BAD_CAST"Event",  EventHealthMonitor_NH0, EventHealthMonitor_NH1, 0, (struct attrXml *[]){&nameHMEvent_A, &actionHMEvent_A, &logHMEvent_A, 0}, 0};

static struct nodeXml healthMonitor_N={BAD_CAST"HealthMonitor",  0, 0, 0, 0, (struct nodeXml *[]){&eventHealthMonitor_N, 0}};

static struct xmcTrace *tracePtr=0;
static struct xmcTraceNoL *traceNoLPtr=0;

static void DeviceTrace_AH(xmlNodePtr node, const xmlChar *val) {
    tracePtr->dev.id=AddDevName((char *)val);
    traceNoLPtr->dev=node->line;
}

static struct attrXml deviceTrace_A={BAD_CAST"device", DeviceTrace_AH};

static void BitmaskTrace_AH(xmlNodePtr node, const xmlChar *val) {
    tracePtr->bitmap=ToU32((char *)val, 16);
}

static struct attrXml bitmaskTrace_A={BAD_CAST"bitmask", BitmaskTrace_AH};

static struct nodeXml trace_N={BAD_CAST"Trace",  0, 0, 0, (struct attrXml *[]){&deviceTrace_A, &bitmaskTrace_A, 0}, 0};

static void BitmaskTraceHyp_AH(xmlNodePtr node, const xmlChar *val) {
    tracePtr->bitmap=ToBitmaskTraceHyp((char *)val, node->line);
}

static struct attrXml bitmaskTraceHyp_A={BAD_CAST"bitmask", BitmaskTraceHyp_AH};

static struct nodeXml traceHyp_N={BAD_CAST"Trace",  0, 0, 0, (struct attrXml *[]){&deviceTrace_A, &bitmaskTraceHyp_A, 0}, 0};

#ifdef CONFIG_CYCLIC_SCHED
static void IdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    
    if (d!=expectedSlotNr)
        LineError(node->line, "slot id (%d) shall be consecutive starting at 0", d);
    expectedSlotNr++;
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].id=d;
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].id=node->line;
}

static struct attrXml idSlot_A={BAD_CAST"id", IdSlot_AH};

static void VCpuIdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].vCpuId=d;
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].vCpuId=node->line;
}

static struct attrXml vCpuIdSlot_A={BAD_CAST"vCpuId", VCpuIdSlot_AH};

static void StartSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].sExec=ToTime((char *)val);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].sExec=node->line;
}

static struct attrXml startSlot_A={BAD_CAST"start", StartSlot_AH};

static void DurationSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].eExec=ToTime((char *)val);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].eExec=node->line;
}

static struct attrXml durationSlot_A={BAD_CAST"duration", DurationSlot_AH};

static void PartitionIdSlot_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].partitionId=ToU32((char *)val, 10);
    xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT].partitionId=node->line;
}

static struct attrXml partitionIdSlot_A={BAD_CAST"partitionId", PartitionIdSlot_AH};

static void Slot_NH0(xmlNodePtr node) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].noSlots++;
    xmc.noSchedCyclicSlots++;
    DO_REALLOC(xmcSchedCyclicSlotTab, xmc.noSchedCyclicSlots*sizeof(struct xmcSchedCyclicSlot));
    DO_REALLOC(xmcSchedCyclicSlotTabNoL, xmc.noSchedCyclicSlots*sizeof(struct xmcSchedCyclicSlotNoL));
    memset(&xmcSchedCyclicSlotTab[C_CYCLICSLOT], 0, sizeof(struct xmcSchedCyclicSlot));
    memset(&xmcSchedCyclicSlotTabNoL[C_CYCLICSLOT], 0, sizeof(struct xmcSchedCyclicSlotNoL));
}

static void Slot_NH1(xmlNodePtr node) {
    xmcSchedCyclicSlotTab[C_CYCLICSLOT].eExec+=xmcSchedCyclicSlotTab[C_CYCLICSLOT].sExec;
}

static struct nodeXml slot_N={BAD_CAST"Slot", Slot_NH0, Slot_NH1, 0, (struct attrXml *[]){&idSlot_A, &vCpuIdSlot_A, &startSlot_A, &durationSlot_A, &partitionIdSlot_A, 0}, 0};

static void NameCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].nameOffset=AddString((char *)val);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].name=node->line;
}

static struct attrXml nameCyclicPlan_A={BAD_CAST"name", NameCyclicPlan_AH};

static void IdCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    if (d!=(cProcCyclicPlan-1))
	LineError(node->line, "Cyclic plan id (%d) shall be consecutive starting at 0", d);

    xmcSchedCyclicPlanTab[C_CYCLICPLAN].id=d;
}

static struct attrXml idCyclicPlan_A={BAD_CAST"id", IdCyclicPlan_AH};

static void MajorFrameCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].majorFrame=ToTime((char *)val);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].majorFrame=node->line;
}

static struct attrXml majorFrameCyclicPlan_A={BAD_CAST"majorFrame", MajorFrameCyclicPlan_AH};

#ifdef CONFIG_PLAN_EXTSYNC
static void ExtSyncCyclicPlan_AH(xmlNodePtr node, const xmlChar *val) {
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].extSync=ToU32((char *)val, 10);
    xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN].extSync=node->line;
}

static struct attrXml extSyncCyclicPlan_A={BAD_CAST"extSync", ExtSyncCyclicPlan_AH};
#endif

static void Plan_NH0(xmlNodePtr node) {
    cProcCyclicPlan++;
    xmc.noSchedCyclicPlans++;
    xmc.hpv.cpuTab[C_CPU].noSchedCyclicPlans++;
    xmcNoL.hpv.cpuTab[C_CPU].plan=node->line;
    DO_REALLOC(xmcSchedCyclicPlanTab, xmc.noSchedCyclicPlans*sizeof(struct xmcSchedCyclicPlan));
    DO_REALLOC(xmcSchedCyclicPlanTabNoL, xmc.noSchedCyclicPlans*sizeof(struct xmcSchedCyclicPlanNoL));
    memset(&xmcSchedCyclicPlanTab[C_CYCLICPLAN], 0, sizeof(struct xmcSchedCyclicPlan));
    memset(&xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN], 0, sizeof(struct xmcSchedCyclicPlanNoL));
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].slotsOffset=xmc.noSchedCyclicSlots;
#ifdef CONFIG_PLAN_EXTSYNC
    xmcSchedCyclicPlanTab[C_CYCLICPLAN].extSync=-1;
#endif
    expectedSlotNr=0;
}

static void Plan_NH2(xmlNodePtr node) {
    CheckSchedCyclicPlan(&xmcSchedCyclicPlanTab[C_CYCLICPLAN], &xmcSchedCyclicPlanTabNoL[C_CYCLICPLAN]);    
}

static struct nodeXml plan_N={BAD_CAST"Plan", Plan_NH0, 0, Plan_NH2, (struct attrXml *[]){&nameCyclicPlan_A, &idCyclicPlan_A, &majorFrameCyclicPlan_A,
#ifdef CONFIG_PLAN_EXTSYNC
&extSyncCyclicPlan_A,
#endif
0}, (struct nodeXml *[]){&slot_N, 0}};

static void CyclicPlan_NH0(xmlNodePtr node) {    
    xmc.hpv.cpuTab[C_CPU].schedCyclicPlansOffset=xmc.noSchedCyclicPlans;
    xmc.hpv.cpuTab[C_CPU].schedPolicy=CYCLIC_SCHED;
}

static struct nodeXml cyclicPlan_N={BAD_CAST"CyclicPlanTable", CyclicPlan_NH0, 0, 0, 0, (struct nodeXml *[]){&plan_N, 0}};

#endif

#ifdef CONFIG_FP_SCHED
static int partIdFP, vCpuIdFP, partIdFPNoL, vCpuIdFPNoL;
static void IdPartitionFixedPrio_AH(xmlNodePtr node, const xmlChar *val) {
    partIdFP=ToU32((char *)val, 10);
    partIdFPNoL=node->line;
}

static struct attrXml idPartitionFixedPrio_A={BAD_CAST"id", IdPartitionFixedPrio_AH};

static void VCpuIdFixedPrio_AH(xmlNodePtr node, const xmlChar *val) {
    vCpuIdFP=ToU32((char *)val, 10);
    vCpuIdFPNoL=node->line;
}

static struct attrXml vCpuIdFixedPrio_A={BAD_CAST"vCpuId", VCpuIdFixedPrio_AH};

static void PriorityFixedPrio_AH(xmlNodePtr node, const xmlChar *val) {
    int e, d=ToU32((char *)val, 10);

    if ((d<MaxPriorityFP())||(d>MinPriorityFP()))
        LineError(node->line, "Invalid priority value (%d)", d);

    for (e=0; e<xmc.noFpEntries; e++) {
        if ((fpSchedTab[e].partitionId==partIdFP)&&
            (fpSchedTab[e].vCpuId==vCpuIdFP))
            LineError(node->line, "Partition (%d:%d) is already allocated to processor %d", fpSchedTab[e].partitionId, fpSchedTab[e].vCpuId, fpSchedTab[e].cpuId);        
    }
    
    xmc.noFpEntries++;
    DO_REALLOC(fpSchedTab, xmc.noFpEntries*sizeof(struct fpSched));
    DO_REALLOC(fpSchedTabNoL, xmc.noFpEntries*sizeof(struct fpSchedNoL));

    fpSchedTab[C_FPSCHED].partitionId=partIdFP;
    fpSchedTab[C_FPSCHED].vCpuId=vCpuIdFP;
    fpSchedTab[C_FPSCHED].priority=d;
    fpSchedTab[C_FPSCHED].cpuId=C_CPU;
    fpSchedTabNoL[C_FPSCHED].priority=node->line;
    fpSchedTabNoL[C_FPSCHED].id=partIdFPNoL;
    fpSchedTabNoL[C_FPSCHED].vCpuId=vCpuIdFPNoL;
    xmc.hpv.cpuTab[C_CPU].noFpEntries++;
}

static struct attrXml priorityFixedPrio_A={BAD_CAST"priority", PriorityFixedPrio_AH};

static struct nodeXml partitionFixedPrio_N={BAD_CAST"Partition", 0, 0, 0, (struct attrXml *[]){&idPartitionFixedPrio_A, &vCpuIdFixedPrio_A, &priorityFixedPrio_A, 0}, 0};

static void FixedPrioSched_NH0(xmlNodePtr node) {    
    xmc.hpv.cpuTab[C_CPU].schedPolicy=FP_SCHED;
    xmc.hpv.cpuTab[C_CPU].schedFpTabOffset=xmc.noFpEntries;
}

static struct nodeXml fixedPrioSched_N={BAD_CAST"FixedPriority", FixedPrioSched_NH0, 0, 0, 0, (struct nodeXml *[]){&partitionFixedPrio_N, 0}};

#endif

static void IdProcessor_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    if (d!=C_CPU)
	LineError(node->line, "processor id (%d) shall be consecutive starting at 0", d);
    xmc.hpv.cpuTab[C_CPU].id=d;
    xmcNoL.hpv.cpuTab[C_CPU].id=node->line;
}

static struct attrXml idProcessor_A={BAD_CAST"id", IdProcessor_AH};

static void FrequencyProcessor_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.cpuTab[C_CPU].freq=ToFreq((char *)val);
    xmcNoL.hpv.cpuTab[C_CPU].freq=node->line;
}

static struct attrXml frequencyProcessor_A={BAD_CAST"frequency", FrequencyProcessor_AH};

static void FeaturesProcessor_AH(xmlNodePtr node, const xmlChar *val) {
}

static struct attrXml featuresProcessor_A={BAD_CAST"features",FeaturesProcessor_AH};

static void Processor_NH0(xmlNodePtr node) {
    xmc.hpv.noCpus++;
#ifdef CONFIG_CYCLIC_SCHED
    cProcCyclicPlan=0;
#endif
    if (xmc.hpv.noCpus>CONFIG_NO_CPUS)
        LineError(node->line, "no more than %d cpus are supported", CONFIG_NO_CPUS);
}

static struct nodeXml processor_N={BAD_CAST"Processor", Processor_NH0, 0, 0, (struct attrXml *[]){&idProcessor_A, &frequencyProcessor_A, &featuresProcessor_A, 0}, (struct nodeXml *[]){
#ifdef CONFIG_CYCLIC_SCHED
&cyclicPlan_N, 
#endif
#ifdef CONFIG_FP_SCHED
&fixedPrioSched_N,
#endif
0}};

static struct nodeXml processorTable_N={BAD_CAST"ProcessorTable", 0, 0, 0, 0, (struct nodeXml *[]){&processor_N, 0}};

static void TypeMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].flags=ToRegionFlags((char *)val);
    xmcMemRegTabNoL[C_REGION].flags=node->line;
}

static struct attrXml typeMemReg_A={BAD_CAST"type", TypeMemReg_AH};

static void StartMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].startAddr=ToU32((char *)val, 16);
    xmcMemRegTabNoL[C_REGION].startAddr=node->line;
}

static struct attrXml startMemReg_A={BAD_CAST"start", StartMemReg_AH};

static void SizeMemReg_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemRegTab[C_REGION].size=ToSize((char *)val);
    xmcMemRegTabNoL[C_REGION].size=node->line;
}

static struct attrXml sizeMemReg_A={BAD_CAST"size", SizeMemReg_AH};

static void Region_NH0(xmlNodePtr node) {
    xmc.noRegions++;
    DO_REALLOC(xmcMemRegTab, xmc.noRegions*sizeof(struct xmcMemoryRegion));
    DO_REALLOC(xmcMemRegTabNoL, xmc.noRegions*sizeof(struct xmcMemoryRegionNoL)); 
    xmcMemRegTabNoL[C_REGION].line=node->line;
}

static void Region_NH1(xmlNodePtr node) {
    CheckMemoryRegion(C_REGION);
}

static struct nodeXml region_N={BAD_CAST"Region", Region_NH0, Region_NH1, 0, (struct attrXml *[]){&typeMemReg_A, &startMemReg_A, &sizeMemReg_A, 0}, 0};

static struct nodeXml memoryLayout_N={BAD_CAST"MemoryLayout", 0, 0, 0, 0, (struct nodeXml *[]){&region_N, 0}};

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
static void nameMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_LOGSTORAGE_ID, xmc.deviceTab.noMemBlocks-1}, node->line);
}

static struct attrXml nameMemBlockDev_A={BAD_CAST"name", nameMemBlockDev_AH};

static void startMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].flags=XM_MEM_AREA_SHARED;
    xmcMemAreaTab[C_PHYSMEMAREA].startAddr=ToU32((char *)val, 16);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].startAddr=node->line;

//    xmcMemBlockTab[C_MEMORYBLOCK_DEV].startAddr=ToU32((char *)val, 16);
//    xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].startAddr=node->line;
}

static struct attrXml startMemBlockDev_A={BAD_CAST"start", startMemBlockDev_AH};

static void sizeMemBlockDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcMemAreaTab[C_PHYSMEMAREA].size=ToSize((char *)val);
    xmcMemAreaTabNoL[C_PHYSMEMAREA].size=node->line;

    //xmcMemBlockTab[C_MEMORYBLOCK_DEV].size=ToSize((char *)val);
    //xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].size=node->line;
}

static struct attrXml sizeMemBlockDev_A={BAD_CAST"size", sizeMemBlockDev_AH};

static void MemoryBlockD_NH0(xmlNodePtr node) {
    xmc.deviceTab.noMemBlocks++;
    DO_REALLOC(xmcMemBlockTab, xmc.deviceTab.noMemBlocks*sizeof(struct xmcMemBlock));
    DO_REALLOC(xmcMemBlockTabNoL, xmc.deviceTab.noMemBlocks*sizeof(struct xmcMemBlockNoL));
    memset(&xmcMemBlockTab[C_MEMORYBLOCK_DEV], 0, sizeof(struct xmcMemBlock));
    memset(&xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV], 0, sizeof(struct xmcMemBlockNoL));
    xmcMemBlockTabNoL[C_MEMORYBLOCK_DEV].line=node->line;
    xmcMemBlockTab[C_MEMORYBLOCK_DEV].physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    ////////////////
    xmc.noPhysicalMemoryAreas++;
    DO_REALLOC(xmcMemAreaTab, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryArea));
    DO_REALLOC(xmcMemAreaTabNoL, xmc.noPhysicalMemoryAreas*sizeof(struct xmcMemoryAreaNoL));
    memset(&xmcMemAreaTab[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryArea));
    memset(&xmcMemAreaTabNoL[C_PHYSMEMAREA], 0, sizeof(struct xmcMemoryAreaNoL));
    xmcMemAreaTabNoL[C_PHYSMEMAREA].line=node->line;

}

static void MemoryBlockD_NH1(xmlNodePtr node) {
    //CheckMemBlock(C_MEMORYBLOCK_DEV);
    
    /////////////
    xmcMemAreaTabNoL[C_PHYSMEMAREA].partitionId=cPartition;
    xmcMemAreaTab[C_PHYSMEMAREA].memoryRegionOffset=CheckPhysMemArea(C_PHYSMEMAREA);
}

static struct nodeXml memoryBlockD_N={BAD_CAST"MemoryBlock", MemoryBlockD_NH0, MemoryBlockD_NH1, 0, (struct attrXml *[]){&nameMemBlockDev_A, &startMemBlockDev_A, &sizeMemBlockDev_A, 0}, 0};
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
static int uartId;

static void IdUart_AH(xmlNodePtr node, const xmlChar *val) {
    uartId=ToU32((char *)val, 10);
    CheckUartId(uartId, node->line);
}

static struct attrXml idUart_A={BAD_CAST"id", IdUart_AH};

static void NameUart_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_UART_ID, uartId}, node->line);
}

static struct attrXml nameUart_A={BAD_CAST"name", NameUart_AH};

static void BaudRateUart_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.deviceTab.uart[uartId].baudRate=ToU32((char *)val, 10);
}

static struct attrXml baudRateUart_A={BAD_CAST"baudRate", BaudRateUart_AH};

static struct nodeXml uartD_N={BAD_CAST"Uart", 0, 0, 0, (struct attrXml *[]){&idUart_A, &nameUart_A, &baudRateUart_A, 0}, 0};
#endif

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)

static void nameTTnocPortDev_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_TTNOC_ID, xmc.deviceTab.noTTnocSlots-1}, node->line);
}

static struct attrXml nameTTnocPortDev_A={BAD_CAST"name", nameTTnocPortDev_AH};

static void ttsocIdTTnocPortDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].ttsocId=ToU32((char *)val, 10);
}

static struct attrXml baseAddTTnocPortDev_A={BAD_CAST"ttsocId", ttsocIdTTnocPortDev_AH};


static void sizeTTnocPortDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].size=ToSize((char *)val);
}

static struct attrXml sizeTTnocPortDev_A={BAD_CAST"size", sizeTTnocPortDev_AH};


static void typeTTnocPortDev_AH(xmlNodePtr node, const xmlChar *val) {
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].type=ToCommPortDirection((char *)val, node->line);
}

static struct attrXml typeTTnocPortDev_A={BAD_CAST"type", typeTTnocPortDev_AH};

static void ttnocPortD_NH0(xmlNodePtr node) {
    xmc.deviceTab.noTTnocSlots++;
    DO_REALLOC(xmcTTnocSlotTab, xmc.deviceTab.noTTnocSlots*sizeof(struct xmcTTnocSlot));
    memset(&xmcTTnocSlotTab[C_TTNOC_PORT_DEV], 0, sizeof(struct xmcTTnocSlot));
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].devId.id=XM_DEV_TTNOC_ID;
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].devId.subId=C_TTNOC_PORT_DEV;
    xmcTTnocSlotTab[C_TTNOC_PORT_DEV].nodeId=-1;
}

static void ttnocPortD_NH1(xmlNodePtr node) {
    int i;
    for (i=0;i<C_TTNOC_PORT_DEV;i++){
        if (xmcTTnocSlotTab[C_TTNOC_PORT_DEV].ttsocId==xmcTTnocSlotTab[i].ttsocId)
           LineError(node->line, "ttsocId (%d) shall be unique", xmcTTnocSlotTab[C_TTNOC_PORT_DEV].ttsocId); 
    }
}

static struct nodeXml ttnocPortD_N={BAD_CAST"TTnocPort", ttnocPortD_NH0, ttnocPortD_NH1, 0, (struct attrXml *[]){&nameTTnocPortDev_A, &baseAddTTnocPortDev_A, &sizeTTnocPortDev_A,&typeTTnocPortDev_A, 0}, 0};

#endif


#ifdef CONFIG_x86
static void NameVga_AH(xmlNodePtr node, const xmlChar *val) {
    RegisterDev((char *)val, (xmDev_t){XM_DEV_VGA_ID, 0}, node->line);
}

static struct attrXml nameVga_A={BAD_CAST"name", NameVga_AH};

static struct nodeXml vgaD_N={BAD_CAST"Vga", 0, 0, 0, (struct attrXml *[]){&nameVga_A, 0}, 0};
#endif

static void Devices_NH0(xmlNodePtr node) {
    RegisterDev((char *)"Null", (xmDev_t){XM_DEV_INVALID_ID, 0}, node->line);
}

static struct nodeXml devices_N={BAD_CAST"Devices", Devices_NH0, 0, 0, 0, (struct nodeXml *[]){
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
&memoryBlockD_N, 
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
&uartD_N, 
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
&ttnocPortD_N,
#endif
#ifdef CONFIG_x86
&vgaD_N,
#endif
0}};

static void PartitionIdIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].partitionId=ToU32((char *)val, 10);
    ipcPortTabNoL[C_IPCPORT].partitionId=node->line;
}

static struct attrXml partitionIdIpc_A={BAD_CAST"partitionId", PartitionIdIpc_AH};

static void PartitionNameIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].partitionName=strdup((char *)val);
    ipcPortTabNoL[C_IPCPORT].partitionName=node->line;
}

static struct attrXml partitionNameIpc_A={BAD_CAST"partitionName", PartitionNameIpc_AH};

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
static void DeviceIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].devId=LookUpDev((char *)val, node->line);
    ipcPortTabNoL[C_IPCPORT].devId=node->line;
}

static struct attrXml deviceIpc_A={BAD_CAST"device", DeviceIpc_AH};
#endif

static void PortNameIpc_AH(xmlNodePtr node, const xmlChar *val) {
    ipcPortTab[C_IPCPORT].portName=strdup((char *)val);
    ipcPortTabNoL[C_IPCPORT].portName=node->line;
}
static struct attrXml portNameIpc_A={BAD_CAST"portName", PortNameIpc_AH};

static void IpcPort_NH0(xmlNodePtr node) {
    noIpcPorts++;
    DO_REALLOC(ipcPortTab, noIpcPorts*sizeof(struct ipcPort));
    DO_REALLOC(ipcPortTabNoL, noIpcPorts*sizeof(struct ipcPortNoL));
    memset(&ipcPortTab[C_IPCPORT], 0, sizeof(struct ipcPort));
    memset(&ipcPortTabNoL[C_IPCPORT], 0, sizeof(struct ipcPortNoL));
    ipcPortTab[C_IPCPORT].channel=C_COMM_CHANNEL;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    ipcPortTab[C_IPCPORT].devId=(xmDev_t){-1,-1};
#endif
}

static void SourceIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_SOURCE_PORT;
}

static struct nodeXml sourceIpcPort_N={BAD_CAST"Source", IpcPort_NH0, SourceIpcPort_NH1, 0, (struct attrXml *[]){&partitionIdIpc_A, &partitionNameIpc_A, &portNameIpc_A, 
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
&deviceIpc_A,0}, 0};
#else
0}, 0};
#endif

static void DestinationIpcPort_NH1(xmlNodePtr node) {
    ipcPortTab[C_IPCPORT].direction=XM_DESTINATION_PORT;
}

static struct nodeXml destinationIpcPort_N={BAD_CAST"Destination", IpcPort_NH0, DestinationIpcPort_NH1, 0, (struct attrXml *[]){&partitionIdIpc_A, &partitionNameIpc_A, &portNameIpc_A,
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
&deviceIpc_A,0}, 0};
#else
 0}, 0};
#endif

static void MaxMessageLengthSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.maxLength=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].s.maxLength=node->line;
}

static struct attrXml maxMessageLengthSChannel_A={BAD_CAST"maxMessageLength", MaxMessageLengthSChannel_AH};

static void ValidPeriodSChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].s.validPeriod=ToTime((char *)val);
}

static struct attrXml validPeriodSChannel_A={BAD_CAST"validPeriod", ValidPeriodSChannel_AH};

static void SamplingChannel_NH0(xmlNodePtr node) {
    xmc.noCommChannels++;
    DO_REALLOC(xmcCommChannelTab, xmc.noCommChannels*sizeof(struct xmcCommChannel));
    DO_REALLOC(xmcCommChannelTabNoL, xmc.noCommChannels*sizeof(struct xmcCommChannelNoL));
    memset(&xmcCommChannelTab[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannel));
    memset(&xmcCommChannelTabNoL[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannelNoL));
    xmcCommChannelTab[C_COMM_CHANNEL].s.validPeriod=(xm_u32_t)-1;
}

static void QueuingChannel_NH0(xmlNodePtr node) {
    xmc.noCommChannels++;
    DO_REALLOC(xmcCommChannelTab, xmc.noCommChannels*sizeof(struct xmcCommChannel));
    DO_REALLOC(xmcCommChannelTabNoL, xmc.noCommChannels*sizeof(struct xmcCommChannelNoL));
    memset(&xmcCommChannelTab[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannel));
    memset(&xmcCommChannelTabNoL[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannelNoL));    
}

static void SamplingChannel_NH1(xmlNodePtr node) {
    xmcCommChannelTab[C_COMM_CHANNEL].type=XM_SAMPLING_CHANNEL;
    xmcCommChannelTabNoL[C_COMM_CHANNEL].type=node->line;
}

static struct nodeXml samplingChannel_N={BAD_CAST"SamplingChannel", SamplingChannel_NH0, SamplingChannel_NH1, 0, (struct attrXml *[]){&maxMessageLengthSChannel_A, &validPeriodSChannel_A, 0}, (struct nodeXml *[]){&sourceIpcPort_N, &destinationIpcPort_N, 0}};

static void MaxMessageLengthQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].q.maxLength=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].q.maxLength=node->line;
}

static void MaxNoMessagesQChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].q.maxNoMsgs=ToU32((char *)val, 10);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].q.maxNoMsgs=node->line;
}

static struct attrXml maxMessageLengthQChannel_A={BAD_CAST"maxMessageLength", MaxMessageLengthQChannel_AH};

static struct attrXml maxNoMessagesQChannel_A={BAD_CAST"maxNoMessages", MaxNoMessagesQChannel_AH};

static void QueuingChannel_NH1(xmlNodePtr node) {
    xmcCommChannelTab[C_COMM_CHANNEL].type=XM_QUEUING_CHANNEL;
    xmcCommChannelTabNoL[C_COMM_CHANNEL].type=node->line;
}

static struct nodeXml queuingChannel_N={BAD_CAST"QueuingChannel", QueuingChannel_NH0, QueuingChannel_NH1, 0, (struct attrXml *[]){&maxMessageLengthQChannel_A, &maxNoMessagesQChannel_A, 0}, (struct nodeXml *[]){&sourceIpcPort_N, &destinationIpcPort_N, 0}};


#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
static void nodeIdTChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].t.nodeId=ToU32((char *)val, 10);
    CheckNoNodeId(xmcCommChannelTab[C_COMM_CHANNEL].t.nodeId,node->line);
}

static struct attrXml nodeIdTChannel_A={BAD_CAST"nodeId", nodeIdTChannel_AH};

static void MaxMessageLengthTChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].t.maxLength=ToSize((char *)val);
    xmcCommChannelTabNoL[C_COMM_CHANNEL].t.maxLength=node->line;
}

static struct attrXml maxMessageLengthTChannel_A={BAD_CAST"maxMessageLength", MaxMessageLengthTChannel_AH};

static void ValidPeriodTChannel_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommChannelTab[C_COMM_CHANNEL].t.validPeriod=ToTime((char *)val);
}

static struct attrXml validPeriodTChannel_A={BAD_CAST"validPeriod", ValidPeriodTChannel_AH};

static void TTnocChannel_NH0(xmlNodePtr node) {
    xmc.noCommChannels++;
    DO_REALLOC(xmcCommChannelTab, xmc.noCommChannels*sizeof(struct xmcCommChannel));
    DO_REALLOC(xmcCommChannelTabNoL, xmc.noCommChannels*sizeof(struct xmcCommChannelNoL));
    memset(&xmcCommChannelTab[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannel));
    memset(&xmcCommChannelTabNoL[C_COMM_CHANNEL], 0, sizeof(struct xmcCommChannelNoL));
    xmcCommChannelTab[C_COMM_CHANNEL].t.nodeId=(xm_u32_t)-1;
}

static void TTnocChannel_NH1(xmlNodePtr node) {
    xmcCommChannelTab[C_COMM_CHANNEL].type=XM_TTNOC_CHANNEL;
    xmcCommChannelTabNoL[C_COMM_CHANNEL].type=node->line;
}

static struct nodeXml ttnocChannel_N={BAD_CAST"TTnocChannel", TTnocChannel_NH0, TTnocChannel_NH1, 0, (struct attrXml *[]){&maxMessageLengthTChannel_A, &validPeriodTChannel_A, &nodeIdTChannel_A, 0}, (struct nodeXml *[]){&sourceIpcPort_N, &destinationIpcPort_N, 0}};

#endif



static xm_s32_t ipviId;
struct srcIpvi *srcIpvi;
struct dstIpvi *dstIpvi;
struct srcIpviNoL *srcIpviNoL;
struct dstIpviNoL *dstIpviNoL;

static int ipviId;

static void SrcIdIpvi_AH(xmlNodePtr node, const xmlChar *val) {
    int id=ToU32((char *)val, 10), e;
    for (e=0; e<noSrcIpvi; e++) {
        srcIpvi=&srcIpviTab[e];
        srcIpviNoL=&srcIpviTabNoL[e];
//        if (srcIpvi->id==id)
//            break;
    }
    if (e>=noSrcIpvi) {
        noSrcIpvi++;
        DO_REALLOC(srcIpviTab, noSrcIpvi*sizeof(struct srcIpvi));
        DO_REALLOC(srcIpviTabNoL, noSrcIpvi*sizeof(struct srcIpviNoL));
        srcIpvi=&srcIpviTab[noSrcIpvi-1];
        srcIpviNoL=&srcIpviTabNoL[noSrcIpvi-1];
        memset(srcIpvi, 0, sizeof(struct srcIpvi));
        memset(srcIpviNoL, 0, sizeof(struct srcIpviNoL));
        srcIpvi->id=id;
        srcIpviNoL->id=node->line;
    }
    srcIpvi->ipviId=ipviId;
}

static struct attrXml srcIdIpvi_A={BAD_CAST"sourceId", SrcIdIpvi_AH};

static void DstIdIpvi_AH(xmlNodePtr node, const xmlChar *val) {
    void _CB(int line, char *val) {
        int id=ToU32((char *)val, 10), e;

        for (e=0; e<srcIpvi->noDsts; e++) {
            dstIpvi=&srcIpvi->dst[e];
            dstIpviNoL=&srcIpviNoL->dst[e];
            if (dstIpvi->id==id)
                break;
        }
        
        if (e>=srcIpvi->noDsts) {
            srcIpvi->noDsts++;
            DO_REALLOC(srcIpvi->dst, srcIpvi->noDsts*sizeof(struct dstIpvi));
            DO_REALLOC(srcIpviNoL->dst, srcIpvi->noDsts*sizeof(struct dstIpviNoL));
            dstIpvi=&srcIpvi->dst[srcIpvi->noDsts-1];
            dstIpviNoL=&srcIpviNoL->dst[srcIpvi->noDsts-1];
            memset(dstIpvi, 0, sizeof(struct dstIpvi));
            memset(dstIpviNoL, 0, sizeof(struct dstIpviNoL));
            dstIpvi->id=id;
            dstIpviNoL->id=node->line;
        } else {
            LineError(node->line, "Duplicated ipvi partition destination");
        }
    }
    ProcessIdList((char *)val, _CB, node->line);
}

static struct attrXml dstIdIpvi_A={BAD_CAST"destinationId", DstIdIpvi_AH};

static void IdIpvi_AH(xmlNodePtr node, const xmlChar *val) {    
    ipviId=ToU32((char *)val, 10);
    if ((ipviId<0)||(ipviId>CONFIG_XM_MAX_IPVI))
        LineError(node->line, "Invalid ipvi id %d", ipviId);
}

static struct attrXml idIpvi_A={BAD_CAST"id", IdIpvi_AH};

static struct nodeXml ipviChannel_N={BAD_CAST"Ipvi", 0, 0, 0, (struct attrXml *[]){&idIpvi_A, &srcIdIpvi_A, &dstIdIpvi_A, 0}, (struct nodeXml *[]){0}};

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
static struct nodeXml channels_N={BAD_CAST"Channels", 0, 0, 0, 0, (struct nodeXml *[]){&samplingChannel_N, &queuingChannel_N, &ttnocChannel_N, &ipviChannel_N, 0}};
#else
static struct nodeXml channels_N={BAD_CAST"Channels", 0, 0, 0, 0, (struct nodeXml *[]){&samplingChannel_N, &queuingChannel_N, &ipviChannel_N, 0}};
#endif

static void IntLines_AH(xmlNodePtr node, const xmlChar *val) {
    ToHwIrqLines((char *)val, node->line);
}

static struct attrXml intLines_A={BAD_CAST"lines", IntLines_AH};

static struct nodeXml hwInterrupts_N={BAD_CAST"Interrupts", 0, 0, 0, (struct attrXml *[]){&intLines_A, 0}, 0};

extern struct nodeXml ioPorts_N;

static struct nodeXml hwResourcesPartition_N={BAD_CAST"HwResources", 0, 0, 0, 0, (struct nodeXml *[]){
&ioPorts_N, 
&hwInterrupts_N, 0}};

static void NameCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].nameOffset=AddString((char *)val);
    xmcCommPortTabNoL[C_COMMPORT].name=node->line;
}

static struct attrXml nameCommPort_A={BAD_CAST"name", NameCommPort_AH};

static void DirectionCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].direction=ToCommPortDirection((char *)val, node->line);
    xmcCommPortTabNoL[C_COMMPORT].direction=node->line;
}

static struct attrXml directionCommPort_A={BAD_CAST"direction", DirectionCommPort_AH};

static void TypeCommPort_AH(xmlNodePtr node, const xmlChar *val) {
    xmcCommPortTab[C_COMMPORT].type=ToCommPortType((char *)val, node->line);
    xmcCommPortTabNoL[C_COMMPORT].type=node->line;
}

static struct attrXml typeCommPort_A={BAD_CAST"type", TypeCommPort_AH};

static void CommPort_NH0(xmlNodePtr node) {
    xmc.noCommPorts++;
    xmcPartitionTab[C_PARTITION].noPorts++;
    DO_REALLOC(xmcCommPortTab, xmc.noCommPorts*sizeof(struct xmcCommPort));
    DO_REALLOC(xmcCommPortTabNoL, xmc.noCommPorts*sizeof(struct xmcCommPortNoL));
    memset(&xmcCommPortTab[C_COMMPORT], 0, sizeof(struct xmcCommPort));
    memset(&xmcCommPortTabNoL[C_COMMPORT], 0, sizeof(struct xmcCommPortNoL));
    xmcCommPortTab[C_COMMPORT].channelId=-1;
}

static void CommPort_NH1(xmlNodePtr node) {
    CheckPortName(C_COMMPORT, C_PARTITION);
}

static struct nodeXml commPort_N={BAD_CAST"Port", CommPort_NH0, CommPort_NH1, 0, (struct attrXml *[]){&nameCommPort_A, &directionCommPort_A, &typeCommPort_A, 0}, 0};

static void PortTable_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].commPortsOffset=xmc.noCommPorts;
}

static struct nodeXml portTable_N={BAD_CAST"PortTable", PortTable_NH0, 0, 0, 0, (struct nodeXml *[]){&commPort_N, 0}};

static void IdPartition_AH(xmlNodePtr node, const xmlChar *val) {    
    int d=ToU32((char *)val, 10);
    
    if (d!=C_PARTITION)
	LineError(node->line, "partition id (%d) shall be consecutive starting at 0", d);

    xmcPartitionTab[C_PARTITION].id=d;
    xmcPartitionTabNoL[C_PARTITION].id=node->line;
}

static struct attrXml idPartition_A={BAD_CAST"id", IdPartition_AH};

static void NamePartition_AH(xmlNodePtr node, const xmlChar *val) {
    CheckPartitionName((char *)val, node->line);
    xmcPartitionTab[C_PARTITION].nameOffset=AddString((char *)val);
    xmcPartitionTabNoL[C_PARTITION].name=node->line;
}

static struct attrXml namePartition_A={BAD_CAST"name", NamePartition_AH};

static void ConsolePartition_AH(xmlNodePtr node, const xmlChar *val) {
    xmcPartitionTab[C_PARTITION].consoleDev.id=AddDevName((char *)val);
    xmcPartitionTabNoL[C_PARTITION].consoleDev=node->line;
}

static struct attrXml consolePartition_A={BAD_CAST"console", ConsolePartition_AH};

static void NoVCpusPartition_AH(xmlNodePtr node, const xmlChar *val) {
    int d=ToU32((char *)val, 10);
    xmcPartitionTab[C_PARTITION].noVCpus=d;
    xmcPartitionTabNoL[C_PARTITION].noVCpus=node->line;
    if (xmcPartitionTab[C_PARTITION].noVCpus>CONFIG_MAX_NO_VCPUS)
        LineError(node->line, "too many virtual Cpus (%d) allocated to the partition", d);
}

static struct attrXml noVCpusPartition_A={BAD_CAST"noVCpus", NoVCpusPartition_AH};

static void FlagsPartition_AH(xmlNodePtr node, const xmlChar *val) {
    xmcPartitionTab[C_PARTITION].flags=ToPartitionFlags((char *)val, node->line);
    xmcPartitionTabNoL[C_PARTITION].flags=node->line;
}

static struct attrXml flagsPartition_A={BAD_CAST"flags", FlagsPartition_AH};

static void Partition_NH0(xmlNodePtr node) {
    xmc.noPartitions++;
    cPartition=C_PARTITION;
    DO_REALLOC(xmcPartitionTab, xmc.noPartitions*sizeof(struct xmcPartition));
    DO_REALLOC(xmcPartitionTabNoL, xmc.noPartitions*sizeof(struct xmcPartitionNoL));
    memset(&xmcPartitionTab[C_PARTITION], 0, sizeof(struct xmcPartition));
    memset(&xmcPartitionTabNoL[C_PARTITION], 0, sizeof(struct xmcPartitionNoL));
    xmcPartitionTab[C_PARTITION].consoleDev.id=XM_DEV_INVALID_ID;
    xmcPartitionTab[C_PARTITION].physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas;
    hmTabPtr=xmcPartitionTab[C_PARTITION].hmTab;    
    tracePtr=&xmcPartitionTab[C_PARTITION].trace;
    traceNoLPtr=&xmcPartitionTabNoL[C_PARTITION].trace;
    tracePtr->dev.id=XM_DEV_INVALID_ID;
    SetupDefaultPartHmActions(xmcPartitionTab[C_PARTITION].hmTab);
}

static void Partition_NH2(xmlNodePtr node) {
    CheckMemAreaPerPart();
}

static struct nodeXml partition_N={BAD_CAST"Partition", Partition_NH0, 0, Partition_NH2, (struct attrXml *[]){&idPartition_A, &namePartition_A, &consolePartition_A, &flagsPartition_A, &noVCpusPartition_A , 0}, (struct nodeXml *[]){&memArea_N, &healthMonitor_N, &trace_N, &hwResourcesPartition_N, &portTable_N, 0}};

static struct nodeXml partitionTable_N={BAD_CAST"PartitionTable", 0, 0, 0, 0, (struct nodeXml *[]){&partition_N, 0}};

static void ResidentSw_NH0(xmlNodePtr node) {
    xmc.rsw.physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmc.rsw.noPhysicalMemoryAreas; 
    cPartition=-1;
}

static struct nodeXml residentSw_N={BAD_CAST"ResidentSw", ResidentSw_NH0, 0, 0, 0, (struct nodeXml *[]){&memArea_N, 0}};

static void ConsoleH_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.consoleDev.id=AddDevName((char *)val);
    xmcNoL.hpv.consoleDev=node->line;
}

static struct attrXml consoleH_A={BAD_CAST"console", ConsoleH_AH};

static void HealthMonitorDevH_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.hmDev.id=AddDevName((char *)val);
    xmcNoL.hpv.hmDev=node->line;
}

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
static void nodeIdHyp_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.hpv.nodeId=ToU32((char *)val, 10);
    xmcNoL.hpv.nodeId=node->line;
}

static struct attrXml nodeIdHyp_A={BAD_CAST"nodeId", nodeIdHyp_AH};
#endif

static struct attrXml healthMonitorDevH_A={BAD_CAST"healthMonitorDevice", HealthMonitorDevH_AH};

static void  XmHypervisor_NH0(xmlNodePtr node) {
    xmc.hpv.physicalMemoryAreasOffset=xmc.noPhysicalMemoryAreas;
    noMemAreaPtr=&xmc.hpv.noPhysicalMemoryAreas;
    SetupDefaultHpvHmActions(xmc.hpv.hmTab);
    hmTabPtr=xmc.hpv.hmTab;    
    tracePtr=&xmc.hpv.trace;
    traceNoLPtr=&xmcNoL.hpv.trace;
    tracePtr->dev.id=XM_DEV_INVALID_ID;
    cPartition=-2;
}

static void  XmHypervisor_NH2(xmlNodePtr node) {
    CheckHpvMemAreaFlags();
}
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
static struct nodeXml xmHypervisor_N={BAD_CAST"XMHypervisor",  XmHypervisor_NH0, 0, XmHypervisor_NH2, (struct attrXml *[]){&consoleH_A, &nodeIdHyp_A, &healthMonitorDevH_A, 0}, (struct nodeXml *[]){&hypMemArea_N, &healthMonitor_N, &traceHyp_N, &ttnocLink_N, 
0}};
#else
static struct nodeXml xmHypervisor_N={BAD_CAST"XMHypervisor",  XmHypervisor_NH0, 0, XmHypervisor_NH2, (struct attrXml *[]){&consoleH_A, &healthMonitorDevH_A, 0}, (struct nodeXml *[]){&hypMemArea_N, &healthMonitor_N, &traceHyp_N, 0}};
#endif

static void  HwDescription_NH2(xmlNodePtr node) {
    //CheckMemBlockReg();	
}

static struct nodeXml hwDescription_N={BAD_CAST"HwDescription", 0, 0, HwDescription_NH2, 0, (struct nodeXml *[]){&processorTable_N, &memoryLayout_N, &devices_N, 0}};

static void VersionSysDesc_AH(xmlNodePtr node, const xmlChar *val) {
    unsigned int version, subversion, revision;

    sscanf((char *)val, "%u.%u.%u", &version, &subversion, &revision);
    xmc.fileVersion=XMC_SET_VERSION(version, subversion, revision);
    xmcNoL.fileVersion=node->line;
}

static struct attrXml versionSysDesc_A={BAD_CAST"version", VersionSysDesc_AH};

static void NameSysDesc_AH(xmlNodePtr node, const xmlChar *val) {
    xmc.nameOffset=AddString((char *)val);
}

static struct attrXml nameSysDesc_A={BAD_CAST"name", NameSysDesc_AH};

static void SystemDescription_NH0(xmlNodePtr node) {
    int e;
    memset(&xmc, 0, sizeof(struct xmc));
    memset(&xmcNoL, 0, sizeof(struct xmcNoL));
    xmc.hpv.consoleDev.id=XM_DEV_INVALID_ID;
    xmc.hpv.hmDev.id=XM_DEV_INVALID_ID;
    xmc.hpv.trace.dev.id=XM_DEV_INVALID_ID;
    for (e=0; e<CONFIG_NO_HWIRQS; e++)
	xmc.hpv.hwIrqTab[e].owner=-1;
}

static void SystemDescription_NH2(xmlNodePtr node) {
    CheckMaxNoKThreads();
#ifdef CONFIG_CYCLIC_SCHED
    CheckCyclicPlanPartitionId();
    CheckCyclicPlanVCpuId();
    //CheckSmpCyclicRestrictions();
#endif
#ifdef CONFIG_FP_SCHED
    CheckFPVCpuId();
#endif
    CheckPartNotAllocToMoreThanACpu();

    CheckIoPorts();
    LinkDevices();
    LinkChannels2Ports();
    CheckIpviTab();
    ProcessIpviTab();
    SetupHwIrqMask();   
#ifdef CONFIG_CYCLIC_SCHED
    HmCheckExistMaintenancePlan();
#endif
    SortPhysMemAreas();
    SortMemRegions();
#ifdef CONFIG_FP_SCHED
    SortFpTab();
    free(fpSchedTabNoL);
#endif
}
static struct nodeXml systemDescription_N={BAD_CAST"SystemDescription", SystemDescription_NH0, 0, SystemDescription_NH2, (struct attrXml *[]){&versionSysDesc_A, &nameSysDesc_A, 0}, (struct nodeXml *[]){&hwDescription_N, &xmHypervisor_N, &residentSw_N, &partitionTable_N, &channels_N, 0}};

struct nodeXml *rootHandlers[]={&systemDescription_N, 0};

//static struct attrXml _A={BAD_CAST"", 0};
//static struct nodeXml _N={BAD_CAST"", 0, 0, (struct attrXml *[]){0}, (struct nodeXml *[]){0}};
