/*
 * $FILE: panic.c
 *
 * Code executed in a panic situation
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <kthread.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <objects/hm.h>

void SystemPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmHmLog_t hmLog;
    va_list vl;
    ASSERT(fmt);

    HwCli();

    kprintf("System FATAL ERROR:\n");
    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);
    kprintf("\n");
    if (sched&&sched->cKThread&&ctxt)
	DumpState(ctxt);
/*
  #ifdef CONFIG_DEBUG
  else {
  xmWord_t ebp=SaveBp();
  StackBackTrace(ebp);
  }
  #endif
*/  
    hmLog.opCodeH=0;
    hmLog.opCodeL=0;
    if (sched&&(sched->cKThread!=sched->idleKThread)) {
        hmLog.opCodeL|=KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT;
    }
       
    hmLog.opCodeL|=XM_HM_EV_INTERNAL_ERROR<<HMLOG_OPCODE_EVENT_BIT;
    hmLog.opCodeH|=HMLOG_OPCODE_SYS_MASK;
    
    if (sched&&sched->cKThread&&ctxt) {
        hmLog.opCodeH|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
	CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);        
    } else
        memset(hmLog.payload, 0, XM_HMLOG_PAYLOAD_LENGTH*sizeof(xmWord_t));

    HmRaiseEvent(&hmLog);

    SmpHaltAll();
    HaltSystem();
}

void PartitionPanic(cpuCtxt_t *ctxt, xm_s8_t *fmt, ...) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmHmLog_t hmLog;
    va_list vl;
    ASSERT(fmt);

    kprintf("Partition PANIC [");

    if (sched->cKThread->ctrl.g)
        kprintf("0x%x:id(%d)]:\n", sched->cKThread, GetPartition(sched->cKThread)->cfg->id);
    else
	kprintf("0x%x:IDLE]:\n", sched->cKThread);

    va_start(vl, fmt);
    vprintf(fmt, vl);
    va_end(vl);
    kprintf("\n");
    if (ctxt)
	DumpState(ctxt);
/*
#ifdef CONFIG_DEBUG
    else {
	xmWord_t ebp=SaveBp();
	StackBackTrace(ebp);
    }
#endif
*/
    if (sched->cKThread==sched->idleKThread)
	HaltSystem();

    hmLog.opCodeH=0;
    hmLog.opCodeL=0;
    
    if (sched->cKThread!=sched->idleKThread) {
	hmLog.opCodeL|=KID2PARTID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_PARTID_BIT;
        hmLog.opCodeL|=KID2VCPUID(sched->cKThread->ctrl.g->id)<<HMLOG_OPCODE_VCPUID_BIT;
    }
    
    hmLog.opCodeL|=XM_HM_EV_INTERNAL_ERROR<<HMLOG_OPCODE_EVENT_BIT;

    if (ctxt) {
        hmLog.opCodeH|=HMLOG_OPCODE_VALID_CPUCTXT_MASK;
	CpuCtxt2HmCpuCtxt(ctxt, &hmLog.cpuCtxt);        
    } else
        memset(hmLog.payload, 0, XM_HMLOG_PAYLOAD_LENGTH*sizeof(xmWord_t));
    HmRaiseEvent(&hmLog);
    
    // Finish current kthread
    SetKThreadFlags(sched->cKThread, KTHREAD_HALTED_F);
    if (sched->cKThread==sched->idleKThread)
	SystemPanic(ctxt, "Idle thread triggered a PANIC???");
    Schedule();
    SystemPanic(ctxt, "[PANIC] This line should not be reached!!!");
}
