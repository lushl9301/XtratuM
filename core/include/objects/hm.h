/*
 * $FILE: hm.h
 *
 * Health Monitor definitions
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_HM_H_
#define _XM_OBJ_HM_H_

#include __XM_INCFLD(arch/irqs.h)

struct hmCpuCtxt {
    xm_u32_t pc;
    xm_u32_t nPc;
    xm_u32_t psr;
};

/* <track id="xm-hm-log-msg"> */
struct xmHmLog {
#define XM_HMLOG_SIGNATURE 0xfecf
    xm_u16_t signature;
    xm_u16_t checksum;

    xm_u32_t opCodeH, opCodeL;
    // HIGH
#define HMLOG_OPCODE_SEQ_MASK (0xfffffff0<<HMLOG_OPCODE_SEQ_BIT)
#define HMLOG_OPCODE_SEQ_BIT 4
   // bits 2 and 3 free

#define HMLOG_OPCODE_VALID_CPUCTXT_MASK (0x1<<HMLOG_OPCODE_VALID_CPUCTXT_BIT)
#define HMLOG_OPCODE_VALID_CPUCTXT_BIT 1
#define HMLOG_OPCODE_SYS_MASK (0x1<<HMLOG_OPCODE_SYS_BIT)
#define HMLOG_OPCODE_SYS_BIT 0

// LOW

#define HMLOG_OPCODE_EVENT_MASK (0xffff<<HMLOG_OPCODE_EVENT_BIT)
#define HMLOG_OPCODE_EVENT_BIT 16

// 256 vcpus
#define HMLOG_OPCODE_VCPUID_MASK (0xff<<HMLOG_OPCODE_VCPUID_BIT)
#define HMLOG_OPCODE_VCPUID_BIT 8

// 256 partitions
#define HMLOG_OPCODE_PARTID_MASK (0xff<<HMLOG_OPCODE_PARTID_BIT)
#define HMLOG_OPCODE_PARTID_BIT 0

    xmTime_t timestamp;
    union {
#define XM_HMLOG_PAYLOAD_LENGTH 4
        struct hmCpuCtxt cpuCtxt;
        xmWord_t payload[XM_HMLOG_PAYLOAD_LENGTH];
    };
}  __PACKED; 

typedef struct xmHmLog xmHmLog_t;
/* </track id="xm-hm-log-msg"> */

#define XM_HM_GET_STATUS 0x0
#define XM_HM_LOCK_EVENTS 0x1
#define XM_HM_UNLOCK_EVENTS 0x2
#define XM_HM_RESET_EVENTS 0x3

/* <track id="xm-hm-log-status"> */
typedef struct {
    xm_s32_t noEvents;
    xm_s32_t maxEvents;
    xm_s32_t currentEvent;
} xmHmStatus_t;
/* </track id="xm-hm-log-status"> */

union hmCmd {
    xmHmStatus_t status;
};

/*Value of the reset status in PCT when the partition is reset via HM*/
#define XM_RESET_STATUS_PARTITION_NORMAL_START    0
#define XM_RESET_STATUS_PARTITION_RESTART    1
#define XM_HM_RESET_STATUS_MODULE_RESTART    2
#define XM_HM_RESET_STATUS_PARTITION_RESTART 3
#define XM_HM_RESET_STATUS_USER_CODE_BIT 16
#define XM_HM_RESET_STATUS_EVENT_MASK 0xffff


#ifdef _XM_KERNEL_
#include <arch/asm.h>
#include <stdc.h>

extern xm_s32_t HmRaiseEvent(xmHmLog_t *log);
static inline void RaiseHmPartEvent(xm_u32_t eventId, xmId_t partId, xmId_t vCpuId, xm_u32_t system) {
    xmHmLog_t hmLog;
    memset(&hmLog, 0, sizeof(xmHmLog_t));
    hmLog.opCodeL=
        (eventId<<HMLOG_OPCODE_EVENT_BIT)|
        (partId<<HMLOG_OPCODE_PARTID_BIT)|
        (vCpuId<<HMLOG_OPCODE_VCPUID_BIT);
    hmLog.opCodeH=system?HMLOG_OPCODE_SYS_MASK:0;
    //SaveCpuHmCtxt(&hmLog.cpuCtxt);
    HmRaiseEvent(&hmLog);
}

#endif
#endif
