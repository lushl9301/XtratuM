/*
 * $FILE: status.h
 *
 * System/partition status
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_STATUS_H_
#define _XM_OBJ_STATUS_H_

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
/* <track id="system-status-remote"> */
typedef struct {
   xm_u32_t nodeId;
   xm_u32_t state;
//#define XM_STATUS_IDLE 0x0
//#define XM_STATUS_READY 0x1
//#define XM_STATUS_SUSPENDED 0x2
//#define XM_STATUS_HALTED 0x3
   xm_u32_t noPartitions;
   xm_u32_t noSchedPlans;
   xm_u32_t currentPlan;
} xmSystemStatusRemote_t;
/* </track id="system-status-remote"> */

/* <track id="partition-status-remote"> */
typedef struct {
   xm_u32_t state;
//#define XM_STATUS_IDLE 0x0
//#define XM_STATUS_READY 0x1
//#define XM_STATUS_SUSPENDED 0x2
//#define XM_STATUS_HALTED 0x3
} xmPartitionStatusRemote_t;
/* </track id="partition-status"> */
#endif

/* <track id="system-status"> */
typedef struct {
    xm_u32_t resetCounter;
    /* Number of HM events emitted. */
    xm_u64_t noHmEvents;                /* [[OPTIONAL]] */
    /* Number of HW interrupts received. */
    xm_u64_t noIrqs;                    /* [[OPTIONAL]] */
    /* Current major cycle interation. */
    xm_u64_t currentMaf;                /* [[OPTIONAL]] */
    /* Total number of system messages: */
    xm_u64_t noSamplingPortMsgsRead;    /* [[OPTIONAL]] */
    xm_u64_t noSamplingPortMsgsWritten; /* [[OPTIONAL]] */
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    xm_u64_t noTTnocPortMsgsRead;    /* [[OPTIONAL]] */
    xm_u64_t noTTnocPortMsgsWritten; /* [[OPTIONAL]] */
#endif
    xm_u64_t noQueuingPortMsgsSent;     /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsReceived; /* [[OPTIONAL]] */
} xmSystemStatus_t;
/* </track id="system-status"> */

//@ \void{<track id="plan-status">}
typedef struct {
    xmTime_t switchTime;
    xm_s32_t next;
    xm_s32_t current;
    xm_s32_t prev;
} xmPlanStatus_t;
//@ \void{</track id="plan-status">} 



/* <track id="partition-status"> */
typedef struct {
    /* Current state of the partition: ready, suspended ... */
    xm_u32_t state;
#define XM_STATUS_IDLE 0x0
#define XM_STATUS_READY 0x1
#define XM_STATUS_SUSPENDED 0x2
#define XM_STATUS_HALTED 0x3

    /*By compatibility with ARINC*/
    xm_u32_t opMode; 
#define XM_OPMODE_IDLE 0x0
#define XM_OPMODE_COLD_RESET 0x1
#define XM_OPMODE_WARM_RESET 0x2
#define XM_OPMODE_NORMAL 0x3

    /* Number of virtual interrupts received. */
    xm_u64_t noVIrqs;                   /* [[OPTIONAL]] */
    /* Reset information */
    xm_u32_t resetCounter;
    xm_u32_t resetStatus;
    xmTime_t execClock;
    /* Total number of partition messages: */
    xm_u64_t noSamplingPortMsgsRead;    /* [[OPTIONAL]] */
    xm_u64_t noSamplingPortMsgsWritten; /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsSent;     /* [[OPTIONAL]] */
    xm_u64_t noQueuingPortMsgsReceived; /* [[OPTIONAL]] */   
} xmPartitionStatus_t;
/* </track id="partition-status"> */

/* <track id="vcpuid-status"> */
typedef struct {
    /* Current state of the virtual CPUs: ready, suspended ... */
    xm_u32_t state;
//#define XM_STATUS_IDLE 0x0
//#define XM_STATUS_READY 0x1
//#define XM_STATUS_SUSPENDED 0x2
//#define XM_STATUS_HALTED 0x3

   /*Only for debug*/
    xm_u32_t opMode;  
//#define XM_OPMODE_IDLE 0x0
//#define XM_OPMODE_COLD_RESET 0x1
//#define XM_OPMODE_WARM_RESET 0x2
//#define XM_OPMODE_NORMAL 0x3
} xmVirtualCpuStatus_t;
/* </track id="vcpuid-status"> */


typedef struct {
    xmAddress_t pAddr;
    xm_u32_t unused: 2, 
        type:3, 
        counter:27;
} xmPhysPageStatus_t;

#define XM_GET_SYSTEM_STATUS 0x0
#define XM_GET_SCHED_PLAN_STATUS 0x1
#define XM_SET_PARTITION_OPMODE 0x2
#define XM_GET_PHYSPAGE_STATUS 0x3
#define XM_GET_VCPU_STATUS 0x4
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#define XM_GET_NODE_STATUS 0x5
#define XM_GET_NODE_PARTITION 0x6
#endif

union statusCmd {
    union {
	xmSystemStatus_t system;
	xmPartitionStatus_t partition;
        xmVirtualCpuStatus_t vcpu;
        xmPlanStatus_t plan;
        xmPhysPageStatus_t physPage;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
        xmSystemStatusRemote_t nodeStatus;
        xmPartitionStatusRemote_t nodePartition;
#endif
    } status;
    xm_u32_t opMode;
};

#ifdef _XM_KERNEL_
extern xmSystemStatus_t systemStatus;
extern xmPartitionStatus_t *partitionStatus;
#endif
#endif
