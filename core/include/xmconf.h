/*
 * $FILE: xmconf.h
 *
 * Config parameters for both, XM and partitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XMCONF_H_
#define _XMCONF_H_

#include __XM_INCFLD(arch/xmconf.h)
#include __XM_INCFLD(devid.h)
#include __XM_INCFLD(linkage.h)
#include __XM_INCFLD(xmef.h)

typedef struct {
    xm_u32_t id:16, subId:16;
} xmDev_t;

struct xmcHmSlot {
    xm_u32_t action:31, log:1;
  
// Logging
#define XM_HM_LOG_DISABLED 0
#define XM_HM_LOG_ENABLED 1

// Actions
//@% <track id="hm-action-list"> 
#define XM_HM_AC_IGNORE 0
#define XM_HM_AC_SHUTDOWN 1
#define XM_HM_AC_PARTITION_COLD_RESET 2
#define XM_HM_AC_PARTITION_WARM_RESET 3
#define XM_HM_AC_HYPERVISOR_COLD_RESET 4
#define XM_HM_AC_HYPERVISOR_WARM_RESET 5
#define XM_HM_AC_SUSPEND 6
#define XM_HM_AC_PARTITION_HALT 7
#define XM_HM_AC_HYPERVISOR_HALT 8
#define XM_HM_AC_PROPAGATE 9
#define XM_HM_AC_SWITCH_TO_MAINTENANCE 10
#define XM_HM_MAX_ACTIONS 11
//@% </track id="hm-action-list"> */
};

// Events
//@% <track id="hm-ev-xm-triggered">

// Raised by XM when an internal grave error is detected
#define XM_HM_EV_INTERNAL_ERROR 0 // affects XM

// Raised by XM when an internal error is detected but the system can be recovered
#define XM_HM_EV_SYSTEM_ERROR  1

// Raised by the partition (through the traces)
#define XM_HM_EV_PARTITION_ERROR 2

// Raised by the timer
#define XM_HM_EV_WATCHDOG_TIMER 3

// Raised when a partition uses the FP without having permissions
#define XM_HM_EV_FP_ERROR 4

// Try to access to the XM's memory space
#define XM_HM_EV_MEM_PROTECTION 5

// Unexpected trap raised
#define XM_HM_EV_UNEXPECTED_TRAP 6

#define XM_HM_MAX_GENERIC_EVENTS 7
//@% </track id="hm-ev-xm-triggered">

struct xmcCommPort {
    xm_u32_t nameOffset;
    xm_s32_t channelId;
#define XM_NULL_CHANNEL -1
    xm_s32_t direction;
#define XM_SOURCE_PORT 0x2
#define XM_DESTINATION_PORT 0x1
    xm_s32_t type;
#define XM_SAMPLING_PORT 0
#define XM_QUEUING_PORT 1
#define XM_TTNOC_PORT 2
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    xmDev_t devId;
#endif
};

#ifdef CONFIG_CYCLIC_SCHED
//@% <track id="sched-cyclic-slot"> 
struct xmcSchedCyclicSlot {
    xmId_t id;
    xmId_t partitionId;
    xmId_t vCpuId;
    xm_u32_t sExec; // offset (usec)
    xm_u32_t eExec; // offset+duration (usec)
};
//@% </track id="sched-cyclic-slot"> 

struct xmcSchedCyclicPlan {
    xm_u32_t nameOffset;
    xmId_t id;
    xm_u32_t majorFrame; // in useconds
    xm_s32_t noSlots;
#ifdef CONFIG_PLAN_EXTSYNC
    xm_s32_t extSync; // -1 means no sync
#endif
    xm_u32_t slotsOffset;
};
#endif

//@% <track id="doc-xmc-memory-area">
struct xmcMemoryArea {
    xm_u32_t nameOffset;
    xmAddress_t startAddr;
    xmAddress_t mappedAt;
    xmSize_t size;
#define XM_MEM_AREA_SHARED (1<<0)
#define XM_MEM_AREA_UNMAPPED (1<<1)
#define XM_MEM_AREA_READONLY (1<<2)
#define XM_MEM_AREA_UNCACHEABLE (1<<3)
#define XM_MEM_AREA_ROM (1<<4)
#define XM_MEM_AREA_FLAG0 (1<<5)
#define XM_MEM_AREA_FLAG1 (1<<6)
#define XM_MEM_AREA_FLAG2 (1<<7)
#define XM_MEM_AREA_FLAG3 (1<<8)
#define XM_MEM_AREA_TAGGED (1<<9)
#define XM_MEM_AREA_IOMMU (1<<10)
    xm_u32_t flags;
    xm_u32_t memoryRegionOffset;
};
//@% </track id="doc-xmc-memory-area">

struct xmcRsw {
    xm_s32_t noPhysicalMemoryAreas;    
    xm_u32_t physicalMemoryAreasOffset;
    //xmAddress_t entryPoint;
};

struct xmcTrace {
    xmDev_t dev;
    xm_u32_t bitmap;
};

struct xmcPartition {
    xmId_t id;
    xm_u32_t nameOffset;
    xm_u32_t flags;
#define XM_PART_SYSTEM 0x100
#define XM_PART_FP 0x200
    xm_u32_t noVCpus;
    xm_u32_t hwIrqs;
    xm_s32_t noPhysicalMemoryAreas;
    xm_u32_t physicalMemoryAreasOffset;
    xmDev_t consoleDev;    
    struct xmcPartitionArch arch;
    xm_u32_t commPortsOffset;
    xm_s32_t noPorts;
    struct xmcHmSlot hmTab[XM_HM_MAX_EVENTS];
    xm_u32_t ioPortsOffset;
    xm_s32_t noIoPorts;
    struct xmcTrace trace;
    struct xmcPartIpvi {
        xm_u32_t dstOffset;
        xm_s32_t noDsts;
    } ipviTab[CONFIG_XM_MAX_IPVI];
};

/* <track id="test-channel-struct"> */
struct xmcCommChannel {
#define XM_SAMPLING_CHANNEL 0
#define XM_QUEUING_CHANNEL 1
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#define XM_TTNOC_CHANNEL 2
#endif
    xm_s32_t type;
  
    union {
	struct {
	    xm_s32_t maxLength;
	    xm_s32_t maxNoMsgs;
	} q;
	struct {
	    xm_s32_t maxLength;
	    xm_u32_t validPeriod;
            xm_s32_t noReceivers;
	} s;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
	struct {
	    xm_s32_t maxLength;
	    xm_u32_t validPeriod;
            xm_s32_t noReceivers;
            xmId_t nodeId;
	} t;
#endif
    };
};
/* </track id="test-channel-struct"> */

struct xmcMemoryRegion {
    xmAddress_t startAddr;
    xmSize_t size;
#define XMC_REG_FLAG_PGTAB (1<<0)
#define XMC_REG_FLAG_ROM (1<<1)
    xm_u32_t flags;
};

struct xmcHwIrq {
    xm_s32_t owner;
#define XM_IRQ_NO_OWNER -1
};

struct xmcHpv {
    xm_s32_t noPhysicalMemoryAreas;
    xm_u32_t physicalMemoryAreasOffset;
    xm_s32_t noCpus;
    struct _cpu {
	xmId_t id;
	xm_u32_t features; // Enable/disable features
	xm_u32_t freq; // KHz
#define XM_CPUFREQ_AUTO 0
        
#define CYCLIC_SCHED 0
#define FP_SCHED 1
        xm_u32_t schedPolicy;
#ifdef CONFIG_CYCLIC_SCHED
        xm_u32_t schedCyclicPlansOffset;
        xm_s32_t noSchedCyclicPlans;
#endif
#ifdef CONFIG_FP_SCHED
        xm_u32_t schedFpTabOffset;
        xm_s32_t noFpEntries;
#endif
    } cpuTab[CONFIG_NO_CPUS];
    struct xmcHpvArch arch;
    struct xmcHmSlot hmTab[XM_HM_MAX_EVENTS];
    xmDev_t hmDev;
    xmDev_t consoleDev;
    xmId_t nodeId;
    struct xmcHwIrq hwIrqTab[CONFIG_NO_HWIRQS];  
    struct xmcTrace trace;
};

#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
struct xmcMemBlock {
    xm_u32_t physicalMemoryAreasOffset;
};
#endif

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
struct xmcTTnocSlot {
    xmAddress_t ttsocId;
    xmSize_t size;
    xm_s32_t type;
    xmId_t nodeId;
    xmDev_t devId;
};
#endif
struct xmcDevice {
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
    xmAddress_t memBlocksOffset;
    xm_s32_t noMemBlocks;
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    xmAddress_t ttnocSlotOffset;
    xm_s32_t noTTnocSlots;
#endif
#if defined(CONFIG_DEV_UART)||defined(CONFIG_DEV_UART_MODULE)
    struct xmcUartCfg {
	xm_u32_t baudRate;
    } uart[CONFIG_DEV_NO_UARTS];
#endif
#ifdef CONFIG_DEV_VGA
    struct xmcVgaCfg {
	
    } vga;
#endif
};

struct xmcRsvMem {
    void *obj;
    xm_u32_t usedAlign;
#define RSV_MEM_USED 0x80000000
    xm_u32_t size;
} __PACKED;

struct xmcBootPart {
#define XM_PART_BOOT 0x1
    xm_u32_t flags;
    xmAddress_t hdrPhysAddr;
    xmAddress_t entryPoint;
    xmAddress_t imgStart;
    xmSize_t imgSize;
    xm_u32_t noCustomFiles;
    struct xefCustomFile customFileTab[CONFIG_MAX_NO_CUSTOMFILES];
};

struct xmcRswInfo {
    xmAddress_t entryPoint;
};

#ifdef CONFIG_FP_SCHED
struct xmcFpSched {
    xmId_t partitionId;
    xmId_t vCpuId;
    xm_u32_t priority;
};
#endif

struct xmcVCpu{
    xmId_t cpu;
};

#define XMC_VERSION 3
#define XMC_SUBVERSION 0
#define XMC_REVISION 0

struct xmc {
#define XMC_SIGNATURE 0x24584d43 // $XMC
    xm_u32_t signature;
    xm_u8_t digest[XM_DIGEST_BYTES];
    xmSize_t dataSize;
    xmSize_t size;
// Reserved(8).VERSION(8).SUBVERSION(8).REVISION(8)
#define XMC_SET_VERSION(_ver, _subver, _rev) ((((_ver)&0xFF)<<16)|(((_subver)&0xFF)<<8)|((_rev)&0xFF))
#define XMC_GET_VERSION(_v) (((_v)>>16)&0xFF)
#define XMC_GET_SUBVERSION(_v) (((_v)>>8)&0xFF)
#define XMC_GET_REVISION(_v) ((_v)&0xFF)
    xm_u32_t version;
    xm_u32_t fileVersion;
    xmAddress_t rsvMemTabOffset;
    xmAddress_t nameOffset;
    struct xmcHpv hpv;
    struct xmcRsw rsw;
    xmAddress_t partitionTabOffset;    
    xm_s32_t noPartitions;
    xmAddress_t bootPartitionTabOffset;
    xmAddress_t rswInfoOffset;
    xmAddress_t memoryRegionsOffset;
    xm_u32_t noRegions;
#ifdef CONFIG_CYCLIC_SCHED
    xmAddress_t schedCyclicSlotsOffset;
    xm_s32_t noSchedCyclicSlots;
    xmAddress_t schedCyclicPlansOffset;
    xm_s32_t noSchedCyclicPlans;
#endif
    xmAddress_t commChannelTabOffset;
    xm_s32_t noCommChannels;
    xmAddress_t physicalMemoryAreasOffset;
    xm_s32_t noPhysicalMemoryAreas;
    xmAddress_t commPortsOffset;
    xm_s32_t noCommPorts;
    xmAddress_t ioPortsOffset;
    xm_s32_t noIoPorts;
    xmAddress_t ipviDstOffset;
    xm_s32_t noIpviDsts;
#ifdef CONFIG_FP_SCHED
    xmAddress_t fpSchedTabOffset;
    xm_s32_t noFpEntries;
#endif
    xmAddress_t vCpuTabOffset;
    xmAddress_t stringsOffset;
    xm_s32_t stringTabLength;
    struct xmcDevice deviceTab;
} __PACKED;

#ifdef _XM_KERNEL_
extern const struct xmc xmcTab;
extern struct xmcPartition *xmcPartitionTab;
extern struct xmcBootPart *xmcBootPartTab;
extern struct xmcRswInfo *xmcRswInfo;
extern struct xmcMemoryRegion *xmcMemRegTab;
extern struct xmcCommChannel *xmcCommChannelTab;
extern struct xmcMemoryArea *xmcPhysMemAreaTab;
extern struct xmcCommPort *xmcCommPorts;
extern struct xmcIoPort *xmcIoPortTab;
#ifdef CONFIG_CYCLIC_SCHED
extern struct xmcSchedCyclicSlot *xmcSchedCyclicSlotTab;
extern struct xmcSchedCyclicPlan *xmcSchedCyclicPlanTab;
#endif
extern xm_u8_t *xmcDstIpvi;
extern xm_s8_t *xmcStringTab;
#ifdef CONFIG_FP_SCHED
extern struct xmcFpSched *xmcFpSchedTab;
#endif
extern struct xmcVCpu *xmcVCpuTab;
extern struct xmcRsvMem *xmcRsvMemTab;
#if defined(CONFIG_DEV_MEMBLOCK)||defined(CONFIG_DEV_MEMBLOCK_MODULE)
extern struct xmcMemBlock *xmcMemBlockTab;
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
extern struct xmcTTnocSlot *xmcTTnocSlotTab;
#endif

#endif

#endif
