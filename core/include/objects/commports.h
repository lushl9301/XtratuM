/*
 * $FILE: commports.h
 *
 * Communication port object definitions
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_COMMPORTS_H_
#define _XM_OBJ_COMMPORTS_H_
// Commands
#define XM_COMM_CREATE_PORT 0x0
#define XM_COMM_GET_PORT_STATUS 0x1
#define XM_COMM_GET_PORT_INFO 0x2

#ifdef _XM_KERNEL_
#include <kthread.h>
#include <spinlock.h>
#include <kdevice.h>
#else
#ifndef __gParam
#define __gParam
#endif
#endif

#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)

/* <track id="ttnoc-status-type"> */
typedef struct {
    xmTime_t timestamp;
    xm_u32_t lastMsgSize;
    xm_u32_t noMsgs;       // Current number of messages.
    xm_u32_t flags;
#define XM_COMM_EMPTY_PORT 0x0
#define XM_COMM_CONSUMED_MSG (1<<XM_COMM_PORT_STATUS_B)
#define XM_COMM_NEW_MSG (2<<XM_COMM_PORT_STATUS_B)
} xmTTnocPortStatus_t;
/* </track id="ttnoc-status-type"> */

//@ \void{<track id="ttnoc-info-type">}
typedef struct {
    char *__gParam portName;
#define XM_INFINITE_TIME ((xm_u32_t)-1)
    xmTime_t  validPeriod; // Refresh period.
    xm_u32_t  maxMsgSize;  // Max message size.
    xm_u32_t direction;
} xmTTnocPortInfo_t;
//@ \void{</track id="ttnoc-info-type">}
#endif

/* <track id="sampling-status-type"> */
typedef struct {
    xmTime_t timestamp;
    xm_u32_t lastMsgSize;
    xm_u32_t  flags;
#define XM_COMM_PORT_STATUS_B 1
#define XM_COMM_PORT_STATUS_M 0x3
#define XM_COMM_EMPTY_PORT 0x0
#define XM_COMM_CONSUMED_MSG (1<<XM_COMM_PORT_STATUS_B)
#define XM_COMM_NEW_MSG (2<<XM_COMM_PORT_STATUS_B)
} xmSamplingPortStatus_t;
/* </track id="sampling-status-type"> */

//@ \void{<track id="sampling-info-type">}
typedef struct {
    char *__gParam portName;
#define XM_INFINITE_TIME ((xm_u32_t)-1)
    xmTime_t  validPeriod; // Refresh period.
    xm_u32_t  maxMsgSize;  // Max message size.
    xm_u32_t direction;
} xmSamplingPortInfo_t;
//@ \void{</track id="sampling-info-type">}

/* <track id="queuing-status-type"> */
typedef struct {
    xm_u32_t noMsgs;       // Current number of messages.
} xmQueuingPortStatus_t;
/* </track id="queuing-status-type"> */

//@ \void{<track id="queuing-info-type">}
typedef struct {
    char *__gParam portName;
    xm_u32_t maxMsgSize; // Max message size.
    xm_u32_t maxNoMsgs; // Max number of messages.
    xm_u32_t direction;
    xmTime_t validPeriod;
} xmQueuingPortInfo_t;
//@ \void{</track id="queuing-info-type">}

union samplingPortCmd {
    struct createSCmd {
	char *__gParam portName; 
	xm_u32_t maxMsgSize; 
	xm_u32_t direction;
        xmTime_t validPeriod;
    } create;
    xmSamplingPortStatus_t status;
    xmSamplingPortInfo_t info;
};
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
union ttnocPortCmd {
    struct createTCmd {
	char *__gParam portName; 
	xm_u32_t maxMsgSize; 
	xm_u32_t direction;
        xmTime_t validPeriod;
    } create;
    xmTTnocPortStatus_t status;
    xmTTnocPortInfo_t info;
};
#endif

union queuingPortCmd {
    struct createQCmd {
	char *__gParam portName; 
	xm_u32_t maxNoMsgs;
	xm_u32_t maxMsgSize;
	xm_u32_t direction;
    } create;
    xmQueuingPortStatus_t status;
    xmQueuingPortInfo_t info;
};

#define XM_COMM_MSG_VALID 0x1

#ifdef _XM_KERNEL_

union channel {
    struct {
	char *buffer;
	xm_s32_t length;
	xmTime_t timestamp;
        partition_t **receiverTab;
        xm_s32_t *receiverPortTab;
        xm_s32_t noReceivers;
        partition_t *sender;
        xm_s32_t senderPort;
        spinLock_t lock;
    } s;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    struct {
//        char *buffer;
        xm_s32_t length;
        xmTime_t timestamp;
        partition_t **receiverTab;
        xm_s32_t *nodeId;
        xm_s32_t *receiverPortTab;
        xm_s32_t noReceivers;
        partition_t *sender;
        xm_s32_t senderPort;
        spinLock_t lock;
    } t;
#endif
    struct {
	struct msg {
	    struct dynListNode listNode;
	    char *buffer;
	    xm_s32_t length;
	    xmTime_t timestamp;
	} *msgPool;
	struct dynList freeMsgs, recvMsgs;
	xm_s32_t usedMsgs;
	partition_t *receiver;
        xm_s32_t receiverPort;
        partition_t *sender;
        xm_s32_t senderPort;
        spinLock_t lock;
    } q;
};

struct port {   
    xm_u32_t flags;
#define COMM_PORT_OPENED 0x1
#define COMM_PORT_EMPTY 0x0
#define COMM_PORT_NEW_MSG 0x2
#define COMM_PORT_CONSUMED_MSG 0x4
#define COMM_PORT_MSG_MASK 0x6
    xmId_t partitionId;
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
    kDevice_t *ttnocDev;
#endif
    spinLock_t lock;
};

#endif

#endif
