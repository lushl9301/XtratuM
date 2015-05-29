/*
 * $FILE: xmc.h
 *
 * XMC definitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XMCDEF_H_
#define _XMCDEF_H_

#include <xm_inc/xmconf.h>

struct xmcHmSlotNoL {
    int action;
    int log;
};

struct xmcTraceNoL {
    int dev;
};

struct xmcMemoryRegionNoL {
    int line;
    int startAddr;
    int size;
    int flags;
};

struct xmcMemoryAreaNoL {
    int name;
    int line;
    int startAddr;
    int mappedAt;
    int size;
    int flags;
    int partitionId;    
};

#if defined(CONFIG_SPARCv8)
struct xmcIoPortNoL {
    union {
	struct {
	    int base;
	    int noPorts;
	} range;
	struct  {
	    int address;
	    int mask;
	} restricted;
    };
};
#endif

struct xmcSchedCyclicSlotNoL {
    int id;
    int partitionId;
    int vCpuId;
    int sExec;
    int eExec;
};

struct xmcPartitionNoL {
    int id;
    int name;
    int flags;
    int noVCpus;
//    xm_s32_t noPhysicalMemoryAreas;
//    xm_u32_t physicalMemoryAreasOffset;
    int consoleDev;
//    struct xmcPartitionArch arch;
    //   xm_u32_t commPortsOffset;
    //   xm_s32_t noPorts;
    struct xmcHmSlotNoL hmTab[XM_HM_MAX_EVENTS];
    //  xm_u32_t ioPortsOffset;
    //  xm_s32_t noIoPorts;
    struct xmcTraceNoL trace;
};

struct xmcSchedCyclicPlanNoL {
    int name;
    int majorFrame; // in useconds
    int extSync;
};

struct xmcNoL {
    int fileVersion;
    struct {
	struct {
	    int id;
	    int features; // Enable/disable features
	    int freq; // KHz
#ifdef CONFIG_CYCLIC_SCHED
            int plan;
#endif
	    /*union {
		struct {
		    struct xmcSchedCyclicPlanNoL {
			int majorFrame; // in useconds
			//int slotsOffset;
		    } planTab[CONFIG_MAX_CYCLIC_PLANS];
		} cyclic;
                } schedParams;*/
	} cpuTab[CONFIG_NO_CPUS];
	//struct xmcHmSlotNoL hmTab[XM_HM_MAX_EVENTS];
	int containerDev;
        int hmDev;
	int consoleDev;	
	int nodeId;	
        int covDev;
	int hwIrqTab[CONFIG_NO_HWIRQS];  
	struct xmcTraceNoL trace;
    } hpv;
    struct {
	//int consoleDev;
	int entryPoint;
    } rsw;
};

struct xmcMemBlockNoL {
    int line;
    int startAddr;
    int size;
};

struct xmcCommChannelNoL {
    int type;
    union {
	struct {
	    int maxLength;
	    int maxNoMsgs;
	} q;
	struct {
	    int maxLength;
	} s;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
	struct {
	    int maxLength;
	} t;
#endif
    };
    int validPeriod;
};

struct xmcCommPortNoL {
    int name;
    int direction;
    int type;
};

struct ipcPort {
    int channel;
    char *partitionName;
    char *portName;
    xm_u32_t partitionId;
    xmDev_t devId;
    int direction;
};

struct ipcPortNoL {
    int partitionName;
    int portName;
    int partitionId;
    int devId;
    int direction;
};

struct srcIpvi {
    int ipviId;
    int id;
    struct dstIpvi {
        int id;
    } *dst;
    int noDsts;
};

struct srcIpviNoL {
    int id;
    struct dstIpviNoL {
        int id;
    } *dst;
};

extern char *ipviDstTab;

extern struct srcIpviNoL *srcIpviTabNoL;
extern struct srcIpvi *srcIpviTab;
extern int noSrcIpvi;

extern struct xmc xmc;
extern struct xmcNoL xmcNoL;

extern struct xmcMemoryRegion *xmcMemRegTab;
extern struct xmcMemoryRegionNoL *xmcMemRegTabNoL;

extern struct xmcMemoryArea *xmcMemAreaTab;
extern struct xmcMemoryAreaNoL *xmcMemAreaTabNoL;

extern struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
extern struct xmcSchedCyclicSlotNoL *xmcSchedCyclicSlotTabNoL;

extern struct xmcPartition *xmcPartitionTab;
extern struct xmcPartitionNoL *xmcPartitionTabNoL;

extern struct xmcIoPort *xmcIoPortTab;
extern struct xmcIoPortNoL *xmcIoPortTabNoL;

extern struct xmcCommPort *xmcCommPortTab;
extern struct xmcCommPortNoL *xmcCommPortTabNoL;
extern struct xmcCommChannel *xmcCommChannelTab;
extern struct xmcCommChannelNoL *xmcCommChannelTabNoL;
extern struct ipcPort *ipcPortTab;
extern struct ipcPortNoL *ipcPortTabNoL;
extern int noIpcPorts;
//extern struct hmSlot hmSlot;
extern struct xmcMemBlock *xmcMemBlockTab;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern struct xmcTTnocSlot *xmcTTnocSlotTab;
extern int noTTnocLinks;
#endif
extern struct xmcMemBlockNoL *xmcMemBlockTabNoL;

extern struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
extern struct xmcSchedCyclicPlanNoL *xmcSchedCyclicPlanTabNoL;

extern char *strTab;

#define C_PARTITION (xmc.noPartitions-1)
#define C_COMM_CHANNEL (xmc.noCommChannels-1)
#define C_FPSCHED (xmc.noFpEntries-1)
#define C_HPV_PHYSMEMAREA (xmc.hpv.noPhysicalMemoryAreas-1)
#define C_PART_PHYSMEMAREA (partitionTab[C_PARTITION].noPhysicalMemoryAreas-1)
#define C_RSW_PHYSMEMAREA (xmc.rsw.noPhysicalMemoryAreas-1)
#define C_PART_VIRTMEMAREA (partitionTab[C_PARTITION].noVirtualMemoryAreas-1)
#define C_PART_IOPORT (partitionTab[C_PARTITION].noIoPorts-1)
#define C_PART_COMMPORT (partitionTab[C_PARTITION].noPorts-1)
#define C_CPU (xmc.hpv.noCpus-1)
#define C_REGION (xmc.noRegions-1)
#define C_PHYSMEMAREA (xmc.noPhysicalMemoryAreas-1)
#define C_COMMPORT (xmc.noCommPorts-1)
#define C_IOPORT (xmc.noIoPorts-1)
#define C_CYCLICSLOT (xmc.noSchedCyclicSlots-1)
#define C_CYCLICPLAN (xmc.noSchedCyclicPlans-1)
#define C_MEMORYBLOCK_DEV (xmc.deviceTab.noMemBlocks-1)
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#define C_TTNOC_PORT_DEV (xmc.deviceTab.noTTnocSlots-1)
#endif
#define C_IPCPORT (noIpcPorts-1)
#define C_TTNOCLINKS (noTTnocLinks-1)
//#define C_HPV_PLAN_SLOT (xmc.hpv.cpuTab[C_CPU].schedParams.cyclic.planTab[C_HPV_SCHED_PLAN].noSlots-1)

extern xm_u32_t AddString(char *s);
extern xmDev_t LookUpDev(char *name, int line);
extern void RegisterDev(char *name, xmDev_t id, int line);
extern int AddDevName(char *name);
extern void LinkChannels2Ports(void);
extern void LinkDevices(void);

extern void SetupDefaultHpvHmActions(struct xmcHmSlot *hmTab);
extern void SetupDefaultPartHmActions(struct xmcHmSlot *hmTab);
extern void SortPhysMemAreas(void);
extern void SortMemRegions(void);
extern void ProcessIpviTab(void);

#ifdef CONFIG_FP_SCHED

extern void SortFpTab(void);

extern struct fpSched {
    xmId_t partitionId;
    xmId_t vCpuId;
    xm_u32_t priority;
    xmId_t cpuId;
} *fpSchedTab;

extern struct fpSchedNoL {
    int id;
    int vCpuId;
    int priority;
} *fpSchedTabNoL;

#endif

#endif
