/*
 * $FILE: xmc.c
 *
 * xmc implementation
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "limits.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"
#include "checks.h"
#include <xm_inc/sched.h>

struct xmc xmc;
struct xmcNoL xmcNoL;
struct xmcMemoryRegion *xmcMemRegTab=0;
struct xmcMemoryRegionNoL *xmcMemRegTabNoL=0;
struct xmcMemoryArea *xmcMemAreaTab=0;
struct xmcMemoryAreaNoL *xmcMemAreaTabNoL=0;
struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab=0;
struct xmcSchedCyclicSlotNoL *xmcSchedCyclicSlotTabNoL=0;
struct xmcPartition *xmcPartitionTab=0;
struct xmcPartitionNoL *xmcPartitionTabNoL=0;
struct xmcIoPort *xmcIoPortTab=0;
struct xmcIoPortNoL *xmcIoPortTabNoL=0;
struct xmcCommPort *xmcCommPortTab=0;
struct xmcCommPortNoL *xmcCommPortTabNoL=0;
struct xmcCommChannel *xmcCommChannelTab=0;
struct xmcCommChannelNoL *xmcCommChannelTabNoL=0;
struct xmcMemBlock *xmcMemBlockTab=0;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
struct xmcTTnocSlot *xmcTTnocSlotTab=0;
#endif
struct xmcMemBlockNoL *xmcMemBlockTabNoL=0;
struct ipcPort *ipcPortTab=0;
struct ipcPortNoL *ipcPortTabNoL=0;
struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab=0;
struct xmcSchedCyclicPlanNoL *xmcSchedCyclicPlanTabNoL=0;
struct srcIpvi *srcIpviTab=0;
struct srcIpviNoL *srcIpviTabNoL=0;
int noSrcIpvi=0;
char *ipviDstTab=0;
int noIpcPorts=0;
int noTTnocLinks=0;

char *strTab=NULL;

static struct devTab {
    char *name;
    xmDev_t id;
    int line;
} *devTab=0;

static int devTabLen=0;

xmDev_t LookUpDev(char *name, int line) {
    int e;
    for (e=0; e<devTabLen; e++)
	if (!strcmp(name, devTab[e].name))
	    return devTab[e].id;
    LineError(line, "\"%s\" not found in the device table", name);
    return (xmDev_t){.id=XM_DEV_INVALID_ID, .subId=0};
}

void RegisterDev(char *name, xmDev_t id, int line) {
    int e;
    for (e=0; e<devTabLen; e++)
	if (!strcmp(name, devTab[e].name)) 
	    LineError(line, "device name already registered (line %d)", devTab[e].line);
    
    e=devTabLen++;
    DO_REALLOC(devTab, devTabLen*sizeof(struct devTab));
    devTab[e].name=strdup(name);
    devTab[e].id=id;
    devTab[e].line=line;
}

static char **devNameTab=0;
static int devNameTabLen=0;

int AddDevName(char *name) {
    int e;
    if (!strcmp(name, "NULL"))
        return XM_DEV_INVALID_ID;
    e=devNameTabLen++;    
    DO_REALLOC(devNameTab, sizeof(char *)*devNameTabLen);
    devNameTab[e]=strdup(name);
    return e;
}

void LinkDevices(void) {
#define IS_DEFINED(_x) (((_x).id!=XM_DEV_INVALID_ID)&&((_x).id<devNameTabLen))
    int e;
    if (IS_DEFINED(xmc.hpv.consoleDev))
	xmc.hpv.consoleDev=LookUpDev(devNameTab[xmc.hpv.consoleDev.id], xmcNoL.hpv.consoleDev);
    if (IS_DEFINED(xmc.hpv.trace.dev))
	xmc.hpv.trace.dev=LookUpDev(devNameTab[xmc.hpv.trace.dev.id], xmcNoL.hpv.trace.dev);
    if (IS_DEFINED(xmc.hpv.hmDev))
	xmc.hpv.hmDev=LookUpDev(devNameTab[xmc.hpv.hmDev.id], xmcNoL.hpv.hmDev);

    for (e=0; e<xmc.noPartitions; e++) {
	if (IS_DEFINED(xmcPartitionTab[e].consoleDev))
	    xmcPartitionTab[e].consoleDev=LookUpDev(devNameTab[xmcPartitionTab[e].consoleDev.id], xmcPartitionTabNoL[e].consoleDev);
	
	if (IS_DEFINED(xmcPartitionTab[e].trace.dev))
	    xmcPartitionTab[e].trace.dev=LookUpDev(devNameTab[xmcPartitionTab[e].trace.dev.id], xmcPartitionTabNoL[e].trace.dev);
    }
}

xm_u32_t AddString(char *s) {
    xm_u32_t offset;
    offset=xmc.stringTabLength;
    xmc.stringTabLength+=strlen(s)+1;
    DO_REALLOC(strTab, xmc.stringTabLength*sizeof(char));
    strcpy(&strTab[offset], s);
    return offset;
}

void LinkChannels2Ports(void) {
    struct xmcCommPort *port=0;
    int e, i;
 
    for (i=0; i<noIpcPorts; i++) {
	if (ipcPortTab[i].partitionId>=xmc.noPartitions)
	    LineError(ipcPortTabNoL[i].partitionId, "incorrect partition id (%d)", ipcPortTab[i].partitionId);	
	if (ipcPortTab[i].partitionName&&strcmp(ipcPortTab[i].partitionName, &strTab[xmcPartitionTab[ipcPortTab[i].partitionId].nameOffset]))
	    LineError(ipcPortTabNoL[i].partitionName, "partition name \"%s\" mismatches with the expected one \"%s\"", ipcPortTab[i].partitionName, &strTab[xmcPartitionTab[ipcPortTab[i].partitionId].nameOffset]);
	for (e=0; e<xmcPartitionTab[ipcPortTab[i].partitionId].noPorts; e++) {
	    port=&xmcCommPortTab[xmcPartitionTab[ipcPortTab[i].partitionId].commPortsOffset+e];
	    if (!strcmp(ipcPortTab[i].portName, &strTab[port->nameOffset])) {
		port->channelId=ipcPortTab[i].channel;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
                port->devId=ipcPortTab[i].devId;
                if (xmcCommChannelTab[port->channelId].type==XM_TTNOC_CHANNEL){
//                   int j;
//                   for (j=0;j<xmc.deviceTab.noTTnocSlots;j++){
//                       if (port->devId.subId == xmcTTnocSlotTab[j].devId.subId){
//                          xmcTTnocSlotTab[j].nodeId=xmcCommChannelTab[port->channelId].t.nodeId;
//                          xmcTTnocSlotTab[j].devId=port->devId;
//                       }
//                   }
                   if ((port->devId.subId==(xm_u16_t)-1)||(port->devId.id==(xm_u16_t)-1))
                      LineError(ipcPortTabNoL[i].portName,"Device id not defined");
                   if (port->direction==XM_DESTINATION_PORT){
                      xmcCommChannelTab[port->channelId].t.noReceivers++;  /*Current version allows only one receptor, although it could be modified to support more than 1*/
                   }
                }
#endif
                if ((xmcCommChannelTab[port->channelId].type==XM_SAMPLING_CHANNEL)&&(port->direction==XM_DESTINATION_PORT))
                    xmcCommChannelTab[port->channelId].s.noReceivers++;
		break;
	    }
	}

	if (e>=xmcPartitionTab[ipcPortTab[i].partitionId].noPorts)
	    LineError(ipcPortTabNoL[i].portName, "port \"%s\" not found", ipcPortTab[i].portName);
	if (xmcCommChannelTab[ipcPortTab[i].channel].type!=port->type)
	    LineError(xmcCommChannelTabNoL[ipcPortTab[i].channel].type, "channel type mismatches with the type of the port");
	if (ipcPortTab[i].direction!=port->direction)
	    LineError(ipcPortTabNoL[i].direction, "channel direction mismatches with the direction of the port");
    }
}

void SetupHwIrqMask(void) {
    int e, i;
    for (e=0; e<xmc.noPartitions; e++) {
        for (i=0; i<CONFIG_NO_HWIRQS; i++) {
            if (xmc.hpv.hwIrqTab[i].owner==xmcPartitionTab[e].id)
                xmcPartitionTab[e].hwIrqs|=(1<<i);
        }
    }
        
}

static int CmpMemArea(struct xmcMemoryArea *m1, struct xmcMemoryArea *m2) {
    if (m1->startAddr>m2->startAddr) return 1;
    if (m1->startAddr<m2->startAddr) return -1;
    return 0;
}

void SortPhysMemAreas(void) {
    xm_s32_t e;
    for (e=0; e<xmc.noPartitions; e++) {
        qsort(&xmcMemAreaTab[xmcPartitionTab[e].physicalMemoryAreasOffset], xmcPartitionTab[e].noPhysicalMemoryAreas, sizeof(struct xmcMemoryArea), (int(*)(const void *, const void *))CmpMemArea);
        CheckAllMemAreas(&xmcMemAreaTab[xmcPartitionTab[e].physicalMemoryAreasOffset], &xmcMemAreaTabNoL[xmcPartitionTab[e].physicalMemoryAreasOffset], xmcPartitionTab[e].noPhysicalMemoryAreas);
    }
}

static int CmpMemRegions(struct xmcMemoryRegion *m1, struct xmcMemoryRegion *m2) {
    if (m1->startAddr>m2->startAddr) return 1;
    if (m1->startAddr<m2->startAddr) return -1;
    return 0;
}

void SortMemRegions(void) {
    qsort(xmcMemRegTab, xmc.noRegions, sizeof(struct xmcMemoryRegion), (int(*)(const void *, const void *))CmpMemRegions);
    CheckAllMemReg();
}

void ProcessIpviTab(void) {
    int e, i, j;
    struct srcIpvi *src;
    struct dstIpvi *dst;

    for (e=0; e<xmc.noPartitions; e++) {
        memset(&xmcPartitionTab[e].ipviTab, 0,  sizeof(struct xmcPartIpvi)*CONFIG_XM_MAX_IPVI);
        for (src=0, i=0; i<noSrcIpvi; i++){
            src=&srcIpviTab[i];
            if (xmcPartitionTab[e].id==src->id) {
                xmcPartitionTab[e].ipviTab[src->ipviId].dstOffset=xmc.noIpviDsts;
                for (j=0; j<src->noDsts; j++) {
                    dst=&src->dst[j];
                    xmc.noIpviDsts++;
                    xmcPartitionTab[e].ipviTab[src->ipviId].noDsts++;
                    DO_REALLOC(ipviDstTab, xmc.noIpviDsts);
                    ipviDstTab[xmc.noIpviDsts-1]=dst->id;
                }
            }
        }
    }      
}

#ifdef CONFIG_FP_SCHED
struct fpSched *fpSchedTab=0;
struct fpSchedNoL *fpSchedTabNoL=0;

static int CmpFpPrio(struct fpSched *f1, struct fpSched *f2) {
    if (f1->priority<f2->priority) return -1;
    if (f1->priority>f2->priority) return 1;
    return 0;
}

void SortFpTab(void) {
    int i;
    for (i=0; i<xmc.hpv.noCpus; i++) {
        if (xmc.hpv.cpuTab[i].schedPolicy==FP_SCHED)
            qsort(&fpSchedTab[xmc.hpv.cpuTab[i].schedFpTabOffset], xmc.hpv.cpuTab[i].noFpEntries, sizeof(struct fpSched), (int(*)(const void *, const void *))CmpFpPrio);
    }
}

#endif
