/*
 * $FILE: checks.c
 *
 * checks implementation
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#define _RSV_PHYS_PAGES_
#define _RSV_HW_IRQS_

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <xm_inc/arch/ginfo.h>
#include <xm_inc/arch/paging.h>

#include "limits.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"

void CheckAllMemReg(void) {
    xmAddress_t a, b;
    int e, line;
    
    a=xmcMemRegTab[0].startAddr;
    line=xmcMemRegTabNoL[0].startAddr;
    for (e=1; e<xmc.noRegions; e++) {
        b=xmcMemRegTab[e].startAddr;
        if (a>=b)
            LineError(line, "memory region (0x%x) unsorted", a);
        a=b;
        line=xmcMemRegTabNoL[e].startAddr;
    }
        
}

void CheckAllMemAreas(struct xmcMemoryArea *mA, struct xmcMemoryAreaNoL *mANoL, int len) {
    xmAddress_t a, b;
    int e, line;
    
    a=mA[0].startAddr;
    line=mANoL[0].startAddr;
    for (e=1; e<len; e++) {
        b=mA[e].startAddr;
        if (a>=b)
            LineError(line, "memory area (0x%x) unsorted", a);
        a=b;
        line=mANoL[e].startAddr;
    }
}

void CheckMemoryRegion(int region) {
    xmAddress_t s0, e0, s1, e1;
    int e;
    s0=xmcMemRegTab[region].startAddr;
    e0=s0+xmcMemRegTab[region].size-1;
    if (s0&(PAGE_SIZE-1))
	LineError(xmcMemRegTabNoL[region].startAddr, "memory region start address (0x%x) shall be aligned to 0x%x", s0, PAGE_SIZE);
    if (xmcMemRegTab[region].size&(PAGE_SIZE-1))
	LineError(xmcMemRegTabNoL[region].size, "memory region size (%d) is not multiple of %d", xmcMemRegTab[region].size, PAGE_SIZE);
    for (e=0; e<xmc.noRegions-1; e++) {
	s1=xmcMemRegTab[e].startAddr;
	e1=s1+xmcMemRegTab[e].size-1;
	
	if ((s0>=e1)||(s1>=e0))
	    continue;
	LineError(xmcMemRegTabNoL[region].line, "memory region [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemRegTabNoL[e].line);
	}
}

int CheckPhysMemArea(int memArea) {
    extern xm_s32_t noRsvPhysPages;
    xmAddress_t s0, e0, s1, e1;
    int e, found;
   
    s0=xmcMemAreaTab[memArea].startAddr;
    e0=s0+xmcMemAreaTab[memArea].size-1;

    if (s0&(PAGE_SIZE-1))
	LineError(xmcMemAreaTabNoL[memArea].startAddr, "memory area start address (0x%x) shall be aligned to 0x%x", s0, PAGE_SIZE);
    if (xmcMemAreaTab[memArea].size&(PAGE_SIZE-1))
	LineError(xmcMemAreaTab[memArea].size, "memory area size (%d) is not multiple of %d", xmcMemAreaTab[memArea].size, PAGE_SIZE);
    
    for (e=0; e<noRsvPhysPages; e++) {
        s1=rsvPhysPages[e].address;
        e1=s1+(rsvPhysPages[e].noPag*PAGE_SIZE)-1;
        if (!((e1<s0)||(s1>=e0)))
            LineError(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] overlaps a memory area [0x%x - 0x%x] reserved for XM", s0, e0, s1, e1);

    }

    for (e=0, found=-1; e<xmc.noRegions; e++) {
	s1=xmcMemRegTab[e].startAddr;
	e1=s1+xmcMemRegTab[e].size-1;
	if ((s0>=s1)&&(e0<=e1))
	    found=e;
    }
    if (found<0)
	LineError(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] is not covered by any memory region", s0, e0);

    for (e=0; e<xmc.noPhysicalMemoryAreas-1; e++) {
	s1=xmcMemAreaTab[e].startAddr;
	e1=s1+xmcMemAreaTab[e].size-1;
	if ((s0>=e1)||(s1>=e0))
	    continue;

        if ((xmcMemAreaTab[e].flags&xmcMemAreaTab[memArea].flags&XM_MEM_AREA_SHARED)==XM_MEM_AREA_SHARED)
	    //if (xmcMemAreaTabNoL[memArea].partitionId!=xmcMemAreaTabNoL[e].partitionId)
            	continue;
	LineError(xmcMemAreaTabNoL[memArea].line, "memory area [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemAreaTabNoL[e].line);
    }

    return found;
}

void CheckMemAreaPerPart(void) {
    xmAddress_t s0, e0, s1, e1;
    int i, j, offset;

    for (i=0; i<xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas-1; i++) {
        offset=xmcPartitionTab[C_PARTITION].physicalMemoryAreasOffset;
        s0=xmcMemAreaTab[i+offset].mappedAt;
        e0=s0+xmcMemAreaTab[i+offset].size-1;
        for (j=i+1; j<xmcPartitionTab[C_PARTITION].noPhysicalMemoryAreas; j++) {
            s1=xmcMemAreaTab[j+offset].mappedAt;
            e1=s1+xmcMemAreaTab[j+offset].size-1;
            if ((s0>=e1)||(s1>=e0))
                continue;
            LineError(xmcMemAreaTabNoL[i+offset].line, "virtual memory area [0x%x - 0x%x] overlaps [0x%x - 0x%x] (line %d)", s0, e0, s1, e1, xmcMemAreaTabNoL[j+offset].line);
        }
    }
}

void CheckHwIrq(int line, int lineNo) {
    int e;
    for (e=0; e<noRsvHwIrqs; e++)
	if (line==rsvHwIrqs[e])
	    LineError(lineNo, "hw interrupt line %d reserved for XM", rsvHwIrqs[e]);
}

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
void CheckNoNodeId(int nodeId, int lineNo) {
    if (nodeId>=CONFIG_TTNOC_NODES)
       LineError(lineNo, "Node Id %d incorrect. Maximun number of nodeId is %d", nodeId,CONFIG_TTNOC_NODES);
    if (nodeId==xmc.hpv.nodeId)
       LineError(lineNo, "Node Id %d incorrect. Id is equal to the host nodeId %d. It should identify an external node", nodeId,xmc.hpv.nodeId);
}
#endif

void CheckPortName(int port, int partition) {
    int e, offset;
    offset=xmcPartitionTab[partition].commPortsOffset;
    for (e=0; e<xmcPartitionTab[partition].noPorts-1; e++)
	if (!strcmp(&strTab[xmcCommPortTab[e+offset].nameOffset], &strTab[xmcCommPortTab[port].nameOffset]))
	    LineError(xmcCommPortTabNoL[port].name, "port name \"%s\" duplicated (line %d)", &strTab[xmcCommPortTab[port].nameOffset], xmcCommPortTabNoL[e+offset].name);
}

void CheckHpvMemAreaFlags(void) {
    xm_u32_t flags;
    int e, line;

    for (e=0; e<xmc.hpv.noPhysicalMemoryAreas; e++) {
	flags=xmcMemAreaTab[e+xmc.hpv.physicalMemoryAreasOffset].flags;
	line=xmcMemAreaTabNoL[e+xmc.hpv.physicalMemoryAreasOffset].flags;
	if (flags&XM_MEM_AREA_SHARED)
	    LineError(line, "\"shared\" flag not permitted");
	
	if (flags&XM_MEM_AREA_READONLY)
	    LineError(line, "\"read-only\" flag not permitted");
	
	if (flags&XM_MEM_AREA_UNMAPPED)
	    LineError(line, "\"unmaped\" flag not permitted");
    }
}

#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
void CheckUartId(int uartId, int line) {
    if ((uartId<0)||(uartId>=CONFIG_DEV_NO_UARTS))
	LineError(line, "invalid uart id %d", uartId);
}
#endif

#ifdef CONFIG_CYCLIC_SCHED
void CheckSchedCyclicPlan(struct xmcSchedCyclicPlan *plan, struct xmcSchedCyclicPlanNoL *planNoL) {
    xm_u32_t t;
    int e;
    
    for (t=0, e=0; e<plan->noSlots; e++) {
	if (t>xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec) 
	    LineError(xmcSchedCyclicSlotTabNoL[plan->slotsOffset+e].sExec, "slot %d ([%lu - %lu] usec) overlaps slot %d ([%lu - %lu] usec)", e, xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec, e-1, xmcSchedCyclicSlotTab[plan->slotsOffset+e-1].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e-1].eExec);
	if ((t=xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec)>plan->majorFrame)
	    LineError(xmcSchedCyclicSlotTabNoL[plan->slotsOffset+e].eExec, "last slot ([%lu - %lu] usec) overlaps major frame (%lu usec)", xmcSchedCyclicSlotTab[plan->slotsOffset+e].sExec, xmcSchedCyclicSlotTab[plan->slotsOffset+e].eExec, plan->majorFrame);

    }
}

void CheckCyclicPlanPartitionId(void) {    
    struct xmcSchedCyclicPlan *plan;
    int j, k;
    
    for (j=0; j<xmc.noSchedCyclicPlans; j++) {
        plan=&xmcSchedCyclicPlanTab[j];
        for (k=0; k<plan->noSlots; k++) {
            if ((xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId<0)||(xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId>=xmc.noPartitions))
                LineError(xmcSchedCyclicSlotTabNoL[k+plan->slotsOffset].partitionId, "incorrect partition id (%d)", xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId);
        }
    }
}
#endif


#if defined(CONFIG_CYCLIC_SCHED) && CONFIG_NO_CPUS>1
void CheckSmpCyclicRestrictions(void) {
#if 0
    int e, i, noSchedCyclicPlans;
    xmTime_t t;
    noSchedCyclicPlans=xmc.hpv.cpuTab[0].noSchedCyclicPlans;
    for (e=1; e<xmc.hpv.noCpus; e++)
        if (xmc.hpv.cpuTab[e].schedPolicy==CYCLIC_SCHED)
            if (noSchedCyclicPlans!=xmc.hpv.cpuTab[e].noSchedCyclicPlans)
                LineError(xmcNoL.hpv.cpuTab[e].plan, "All processors shall define the same number of plans");

    for (e=0; e<xmc.hpv.noCpus; e++) {
        if (xmc.hpv.cpuTab[e].schedPolicy==CYCLIC_SCHED) {
            t=xmcSchedCyclicPlanTab[0].majorFrame;
            for (i=1; i<xmc.hpv.cpuTab[e].noSchedCyclicPlans; i++) {
                if (t!=xmcSchedCyclicPlanTab[i+xmc.hpv.cpuTab[e].schedCyclicPlansOffset].majorFrame) {
                    LineError(xmcSchedCyclicPlanTabNoL[i+xmc.hpv.cpuTab[e].schedCyclicPlansOffset].majorFrame, "Plan's major frame (%d) does not match with the one of the plan (%d:%d)", xmcSchedCyclicPlanTab[i+xmc.hpv.cpuTab[e].schedCyclicPlansOffset].majorFrame, );
                }
            }
        }
    }
#endif
}
#endif

void CheckPartNotAllocToMoreThanACpu(void) {
#ifdef CONFIG_CYCLIC_SCHED
    struct xmcSchedCyclicPlan *plan;
    struct xmcSchedCyclicSlot *slot;
    int i, j;
#endif
    int e;

    vCpuTab=malloc(xmc.noPartitions*sizeof(struct vCpu2Cpu *));        
    for (e=0; e<xmc.noPartitions; e++) {
        vCpuTab[e]=malloc(xmcPartitionTab[e].noVCpus*sizeof(struct vCpu2Cpu));
        memset(vCpuTab[e], -1, xmcPartitionTab[e].noVCpus*sizeof(struct vCpu2Cpu));
    }
    
#ifdef CONFIG_CYCLIC_SCHED
    for (e=0; e<xmc.hpv.noCpus; e++) {
        if (xmc.hpv.cpuTab[e].schedPolicy==CYCLIC_SCHED) {
            for (i=0; i<xmc.hpv.cpuTab[e].noSchedCyclicPlans; i++) {
                plan=&xmcSchedCyclicPlanTab[i+xmc.hpv.cpuTab[e].schedCyclicPlansOffset];
                for (j=0; j<plan->noSlots; j++) {
                    slot=&xmcSchedCyclicSlotTab[j+plan->slotsOffset];
                    if (vCpuTab[slot->partitionId][slot->vCpuId].cpu==-1) {
                        vCpuTab[slot->partitionId][slot->vCpuId].cpu=e;
                        vCpuTab[slot->partitionId][slot->vCpuId].line=xmcSchedCyclicSlotTabNoL[j+plan->slotsOffset].vCpuId;
                        continue;
                    }
                    if (vCpuTab[slot->partitionId][slot->vCpuId].cpu!=e)
                        LineError(xmcSchedCyclicSlotTabNoL[j+plan->slotsOffset].vCpuId, "vCpu (%d, %d) assigned to CPU %d and %d (line %d)", slot->partitionId, slot->vCpuId, vCpuTab[slot->partitionId][slot->vCpuId].cpu, e, vCpuTab[slot->partitionId][slot->vCpuId].line);
                }
            }
        }
    }
#endif

#ifdef CONFIG_FP_SCHED
    for (e=0; e<xmc.noFpEntries; e++) {        
        if (vCpuTab[fpSchedTab[e].partitionId][fpSchedTab[e].vCpuId].cpu==-1) {
            vCpuTab[fpSchedTab[e].partitionId][fpSchedTab[e].vCpuId].cpu=fpSchedTab[e].cpuId;
            vCpuTab[fpSchedTab[e].partitionId][fpSchedTab[e].vCpuId].line=fpSchedTabNoL[e].priority;
        } else {
            LineError(fpSchedTabNoL[e].priority, "vCpu (%d, %d) assigned to CPU %d and %d (line %d)", fpSchedTab[e].partitionId, fpSchedTab[e].vCpuId, vCpuTab[fpSchedTab[e].partitionId][fpSchedTab[e].vCpuId].cpu, fpSchedTab[e].cpuId, vCpuTab[fpSchedTab[e].partitionId][fpSchedTab[e].vCpuId].line);
        }
    }
#endif
}

#ifdef CONFIG_CYCLIC_SCHED
void CheckCyclicPlanVCpuId(void) {
    struct xmcSchedCyclicPlan *plan;
    int j, k;

    for (j=0; j<xmc.noSchedCyclicPlans; j++) {
        plan=&xmcSchedCyclicPlanTab[j];
        for (k=0; k<plan->noSlots; k++) {
            if ((xmcSchedCyclicSlotTab[k+plan->slotsOffset].vCpuId<0)||(xmcSchedCyclicSlotTab[k+plan->slotsOffset].vCpuId>=xmcPartitionTab[xmcSchedCyclicSlotTab[k+plan->slotsOffset].partitionId].noVCpus))
                LineError(xmcSchedCyclicSlotTabNoL[k+plan->slotsOffset].vCpuId, "incorrect vCpu id (%d)", xmcSchedCyclicSlotTab[k+plan->slotsOffset].vCpuId);
        }
    }
}
#endif

#ifdef CONFIG_FP_SCHED
void CheckFPVCpuId(void) {
    int e;
    for (e=0; e<xmc.noFpEntries; e++) {        
        /*if (fpSchedTab[e].partitionId>=xmc.noPartitions)
            LineError(fpSchedTabNoL[e].partitionId, "incorrect partition id (%d)", fpSchedTab[e].partitionId);
        */
        if (fpSchedTab[e].vCpuId>=xmcPartitionTab[fpSchedTab[e].partitionId].noVCpus)
            LineError(fpSchedTabNoL[e].vCpuId, "incorrect vCpu id (%d)", fpSchedTab[e].vCpuId);
    }
}
#endif

void CheckPartitionName(char *name, int line) {
    int e;
    for (e=0; e<xmc.noPartitions; e++)
	if (!strcmp(&strTab[xmcPartitionTab[e].nameOffset], name))
	    LineError(line, "partition name \"%s\" duplicated (line %d)", name, xmcPartitionTabNoL[e].name);
}

void CheckMaxNoKThreads(void) {
    int e, noKThreads=xmc.noPartitions;
    for (e=0; e<xmc.noPartitions; e++)
        noKThreads+=xmcPartitionTab[e].noVCpus;

    if (noKThreads>CONFIG_MAX_NO_KTHREADS)
        EPrintF("XtratuM only supports a maximum of %d kthreads, the configuration file defines a total of %d kthreads", CONFIG_MAX_NO_KTHREADS, noKThreads);
}

void CheckIpviTab(void) {
    struct srcIpvi *src;
    struct dstIpvi *dst;
    struct srcIpviNoL *srcNoL;
    struct dstIpviNoL *dstNoL;
    int i, j;

    for (i=0; i<noSrcIpvi; i++) {
        src=&srcIpviTab[i];
        srcNoL=&srcIpviTabNoL[i];
        if ((src->id<0)||(src->id>=xmc.noPartitions))
            LineError(srcNoL->id, "source partition %d doesn't exist", src->id);
        for (j=0; j<src->noDsts; j++) {
            dst=&src->dst[j];
            dstNoL=&srcNoL->dst[j];
            if ((dst->id<0)||(dst->id>=xmc.noPartitions))
                LineError(dstNoL->id, "destination partition %d doesn't exist", dst->id);
        }
    }
}

