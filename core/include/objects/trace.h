/*
 * $FILE: trace.h
 *
 * Tracing object definitions
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_TRACE_H_
#define _XM_OBJ_TRACE_H_

#include __XM_INCFLD(linkage.h)

#define XM_TRACE_UNRECOVERABLE 0x3 // This level triggers a health
                                   // monitoring fault
#define XM_TRACE_WARNING 0x2
#define XM_TRACE_DEBUG 0x1
#define XM_TRACE_NOTIFY 0x0

/* <track id="xm-trace-event"> */
struct xmTraceEvent {
#define XMTRACE_SIGNATURE 0xc33c
    xm_u16_t signature;
    xm_u16_t checksum;
    xm_u32_t opCodeH, opCodeL;

// HIGH
#define TRACE_OPCODE_SEQ_MASK (0xfffffff0<<TRACE_OPCODE_SEQ_BIT)
#define TRACE_OPCODE_SEQ_BIT 4

#define TRACE_OPCODE_CRIT_MASK (0x7<<TRACE_OPCODE_CRIT_BIT)
#define TRACE_OPCODE_CRIT_BIT 1

#define TRACE_OPCODE_SYS_MASK (0x1<<TRACE_OPCODE_SYS_BIT)
#define TRACE_OPCODE_SYS_BIT 0

// LOW
#define TRACE_OPCODE_CODE_MASK (0xffff<<TRACE_OPCODE_CODE_BIT)
#define TRACE_OPCODE_CODE_BIT 16

// 256 vcpus
#define TRACE_OPCODE_VCPUID_MASK (0xff<<TRACE_OPCODE_VCPUID_BIT)
#define TRACE_OPCODE_VCPUID_BIT 8

// 256 partitions
#define TRACE_OPCODE_PARTID_MASK (0xff<<TRACE_OPCODE_PARTID_BIT)
#define TRACE_OPCODE_PARTID_BIT 0
    xmTime_t timestamp;
#define XMTRACE_PAYLOAD_LENGTH 4
    xmWord_t payload[XMTRACE_PAYLOAD_LENGTH];
} __PACKED;

typedef struct xmTraceEvent xmTraceEvent_t;

/* </track id="xm-trace-event"> */

#define XM_TRACE_GET_STATUS 0x0
#define XM_TRACE_LOCK 0x1
#define XM_TRACE_UNLOCK 0x2
#define XM_TRACE_RESET 0x3

/* <track id="xm-trace-status"> */
typedef struct {
    xm_s32_t noEvents;
    xm_s32_t maxEvents;
    xm_s32_t currentEvent;
} xmTraceStatus_t;
/* </track id="xm-trace-status"> */

union traceCmd {
    xmTraceStatus_t status;
};

// Bitmaps
#define TRACE_BM_ALWAYS (~0x0)

#ifdef _XM_KERNEL_
extern xm_s32_t TraceWriteSysEvent(xm_u32_t bitmap, xmTraceEvent_t *event);
#endif
#endif
