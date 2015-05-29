/*
 * $FILE: audit.h
 *
 * Audit events definition
 *
 * $VERSION$
 * 
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_AUDIT_H_
#define _XM_AUDIT_H_

#ifdef CONFIG_AUDIT_EVENTS
#include __XM_INCFLD(objects/trace.h)

#define TRACE_NO_MODULES (5)
#define TRACE_BM_IRQ_MODULE (1<<TRACE_IRQ_MODULE)
#define TRACE_IRQ_MODULE 0
  #define AUDIT_IRQ_RAISED 0x1
  #define AUDIT_TRAP_RAISED 0x2
  #define AUDIT_IRQ_EMUL 0x3
#define TRACE_BM_HCALLS_MODULE (1<<TRACE_HCALLS_MODULE)
#define TRACE_HCALLS_MODULE 0x1
  #define AUDIT_HCALL_BEGIN 0x1
  #define AUDIT_HCALL_BEGIN2 0x2
  #define AUDIT_HCALL_END 0x3
  #define AUDIT_ASMHCALL 0x4 // sparc
#define TRACE_BM_SCHED_MODULE (1<<TRACE_SCHED_MODULE)
#define TRACE_SCHED_MODULE 0x2
  #define AUDIT_SCHED_PART_SUSPEND 0x1
  #define AUDIT_SCHED_PART_RESUME 0x2
  #define AUDIT_SCHED_PART_HALT 0x3
  #define AUDIT_SCHED_PART_RESET 0x4
  #define AUDIT_SCHED_PART_SHUTDOWN 0x5
  #define AUDIT_SCHED_PART_IDLE 0x6
  #define AUDIT_SCHED_HYP_HALT 0x7
  #define AUDIT_SCHED_HYP_RESET 0x8
  #define AUDIT_SCHED_PLAN_SWITCH_REQ 0x9
  #define AUDIT_SCHED_PLAN_SWITCH_DONE 0xa
  #define AUDIT_SCHED_CONTEXT_SWITCH 0xb

  #define AUDIT_SCHED_VCPU_SUSPEND 0xc
  #define AUDIT_SCHED_VCPU_RESUME 0xd
  #define AUDIT_SCHED_VCPU_HALT 0xe
  #define AUDIT_SCHED_VCPU_RESET 0xf

#define TRACE_BM_HM_MODULE (1<<TRACE_HM_MODULE)
#define TRACE_HM_MODULE 0x3
  #define AUDIT_HM_EVENT_RAISED 0x1
  #define AUDIT_HM_HPV_ACTION 0x2
  #define AUDIT_HM_PART_ACTION 0x3

#define TRACE_BM_VMM_MODULE (1<<TRACE_VMM_MODULE)
#define TRACE_VMM_MODULE 0x4
  #define AUDIT_VMM_PPG_DOESNT_BELONG 0x1
  #define AUDIT_VMM_PPG_DOESNT_EXIST 0x2
  #define AUDIT_VMM_PPG_CNT 0x3
  #define AUDIT_VMM_INVLD_PTDE 0x4

#ifdef _XM_KERNEL_
#include <assert.h>

extern void RaiseAuditEvent(xm_u32_t module, xm_u32_t event, xm_s32_t payloadLen, xmWord_t *payload);
extern xm_s32_t IsAuditEventMasked(xm_u32_t module);
#endif
#endif
#endif
