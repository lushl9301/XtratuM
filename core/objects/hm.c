/*
 * $FILE: hm.c
 *
 * Health Monitor
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <checksum.h>
#include <kthread.h>
#include <hypercalls.h>
#include <logstream.h>
#include <rsvmem.h>
#include <stdc.h>
#include <xmconf.h>
#include <sched.h>
#include <objects/hm.h>
#ifdef CONFIG_OBJ_STATUS_ACC
#include <objects/status.h>
#endif

extern xm_u32_t resetStatusInit[];

static struct logStream hmLogStream;
static xm_s32_t hmInit=0;
static xm_u32_t seq=0;

static xm_s32_t ReadHmLog(xmObjDesc_t desc, xmHmLog_t *__gParam log, xm_u32_t size) {
    xm_s32_t e, noLogs=size/sizeof(xmHmLog_t);
    localSched_t *sched=GET_LOCAL_SCHED();

    if (OBJDESC_GET_PARTITIONID(desc)!=XM_HYPERVISOR_ID)
	return XM_INVALID_PARAM;

    if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (CheckGParam(log, size, 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;

    if (!log||!noLogs)
	return XM_INVALID_PARAM;

    for (e=0; e<noLogs; e++)
	if (LogStreamGet(&hmLogStream, &log[e])<0)
	    return e*sizeof(xmHmLog_t);
    
    return noLogs*sizeof(xmHmLog_t);
}

static xm_s32_t SeekHmLog(xmObjDesc_t desc, xm_u32_t offset, xm_u32_t whence) {
    localSched_t *sched=GET_LOCAL_SCHED();
    if (OBJDESC_GET_PARTITIONID(desc)!=XM_HYPERVISOR_ID)
	return XM_INVALID_PARAM;

    if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;

    if (LogStreamSeek(&hmLogStream, offset, whence)<0)
        return XM_INVALID_PARAM;
    return XM_OK;
}

static xm_s32_t CtrlHmLog(xmObjDesc_t desc, xm_u32_t cmd, union hmCmd *__gParam args) {
    localSched_t *sched=GET_LOCAL_SCHED();
    if (OBJDESC_GET_PARTITIONID(desc)!=XM_HYPERVISOR_ID)
	return XM_INVALID_PARAM;

    if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
        return XM_PERM_ERROR;
    if (CheckGParam(args, sizeof(union hmCmd), 4, PFLAG_NOT_NULL|PFLAG_RW)<0) return XM_INVALID_PARAM;
    switch(cmd) {
    case XM_HM_GET_STATUS:
	args->status.noEvents=hmLogStream.ctrl.elem;
	args->status.maxEvents=hmLogStream.info.maxNoElem;
	args->status.currentEvent=hmLogStream.ctrl.d;
	return XM_OK;
    case XM_HM_LOCK_EVENTS:
        LogStreamLock(&hmLogStream);
        return XM_OK;
    case XM_HM_UNLOCK_EVENTS:
        LogStreamUnlock(&hmLogStream);
        return XM_OK;
    case XM_HM_RESET_EVENTS:
        LogStreamInit(&hmLogStream, LookUpKDev(&xmcTab.hpv.hmDev), sizeof(xmHmLog_t));
        return XM_OK;
    }
    return XM_INVALID_PARAM;
}

static const struct object hmObj={
    .Read=(readObjOp_t)ReadHmLog,
    .Seek=(seekObjOp_t)SeekHmLog,
    .Ctrl=(ctrlObjOp_t)CtrlHmLog,
};

xm_s32_t __VBOOT SetupHm(void) {
    LogStreamInit(&hmLogStream, LookUpKDev(&xmcTab.hpv.hmDev), sizeof(xmHmLog_t));
    objectTab[OBJ_CLASS_HM]=&hmObj;
    hmInit=1;
    return 0;
}

#ifdef CONFIG_OBJ_HM_VERBOSE
xm_s8_t *hmEvent2Str[XM_HM_MAX_EVENTS]={
    FILL_TAB(XM_HM_EV_INTERNAL_ERROR),
    FILL_TAB(XM_HM_EV_SYSTEM_ERROR),
    FILL_TAB(XM_HM_EV_PARTITION_ERROR),
    FILL_TAB(XM_HM_EV_WATCHDOG_TIMER),
    FILL_TAB(XM_HM_EV_FP_ERROR),
    FILL_TAB(XM_HM_EV_MEM_PROTECTION),
    FILL_TAB(XM_HM_EV_UNEXPECTED_TRAP),
#ifdef CONFIG_x86
    FILL_TAB(XM_HM_EV_X86_DIVIDE_ERROR),
    FILL_TAB(XM_HM_EV_X86_DEBUG),
    FILL_TAB(XM_HM_EV_X86_NMI_INTERRUPT),
    FILL_TAB(XM_HM_EV_X86_BREAKPOINT),
    FILL_TAB(XM_HM_EV_X86_OVERFLOW),
    FILL_TAB(XM_HM_EV_X86_BOUND_RANGE_EXCEEDED),
    FILL_TAB(XM_HM_EV_X86_INVALID_OPCODE),
    FILL_TAB(XM_HM_EV_X86_DEVICE_NOT_AVAILABLE),
    FILL_TAB(XM_HM_EV_X86_DOUBLE_FAULT),
    FILL_TAB(XM_HM_EV_X86_COPROCESSOR_SEGMENT_OVERRUN),
    FILL_TAB(XM_HM_EV_X86_INVALID_TSS),
    FILL_TAB(XM_HM_EV_X86_SEGMENT_NOT_PRESENT),
    FILL_TAB(XM_HM_EV_X86_STACK_FAULT),
    FILL_TAB(XM_HM_EV_X86_GENERAL_PROTECTION),
    FILL_TAB(XM_HM_EV_X86_PAGE_FAULT),
    FILL_TAB(XM_HM_EV_X86_X87_FPU_ERROR),
    FILL_TAB(XM_HM_EV_X86_ALIGNMENT_CHECK),
    FILL_TAB(XM_HM_EV_X86_MACHINE_CHECK),
    FILL_TAB(XM_HM_EV_X86_SIMD_FLOATING_POINT),
#endif
#ifdef CONFIG_SPARCv8
    FILL_TAB(XM_HM_EV_SPARC_WRITE_ERROR),
    FILL_TAB(XM_HM_EV_SPARC_INSTR_ACCESS_MMU_MISS),
    FILL_TAB(XM_HM_EV_SPARC_INSTR_ACCESS_ERROR),
    FILL_TAB(XM_HM_EV_SPARC_REGISTER_HARDWARE_ERROR),
    FILL_TAB(XM_HM_EV_SPARC_INSTR_ACCESS_EXCEPTION),
    FILL_TAB(XM_HM_EV_SPARC_PRIVILEGED_INSTR),
    FILL_TAB(XM_HM_EV_SPARC_ILLEGAL_INSTR),
    FILL_TAB(XM_HM_EV_SPARC_FP_DISABLED),
    FILL_TAB(XM_HM_EV_SPARC_CP_DISABLED),
    FILL_TAB(XM_HM_EV_SPARC_UNIMPLEMENTED_FLUSH),
    FILL_TAB(XM_HM_EV_SPARC_WATCHPOINT_DETECTED),
    FILL_TAB(XM_HM_EV_SPARC_MEM_ADDR_NOT_ALIGNED),
    FILL_TAB(XM_HM_EV_SPARC_FP_EXCEPTION),
    FILL_TAB(XM_HM_EV_SPARC_CP_EXCEPTION),
    FILL_TAB(XM_HM_EV_SPARC_DATA_ACCESS_ERROR),
    FILL_TAB(XM_HM_EV_SPARC_DATA_ACCESS_MMU_MISS),
    FILL_TAB(XM_HM_EV_SPARC_DATA_ACCESS_EXCEPTION),
    FILL_TAB(XM_HM_EV_SPARC_TAG_OVERFLOW),
    FILL_TAB(XM_HM_EV_SPARC_DIVIDE_EXCEPTION),
#endif
};

xm_s8_t *hmLog2Str[2]={
    FILL_TAB(XM_HM_LOG_DISABLED),
    FILL_TAB(XM_HM_LOG_ENABLED),
};

xm_s8_t *hmAction2Str[XM_HM_MAX_ACTIONS]={
    FILL_TAB(XM_HM_AC_IGNORE),
    FILL_TAB(XM_HM_AC_SHUTDOWN),
    FILL_TAB(XM_HM_AC_PARTITION_COLD_RESET),
    FILL_TAB(XM_HM_AC_PARTITION_WARM_RESET),
    FILL_TAB(XM_HM_AC_HYPERVISOR_COLD_RESET),
    FILL_TAB(XM_HM_AC_HYPERVISOR_WARM_RESET),
    FILL_TAB(XM_HM_AC_SUSPEND),
    FILL_TAB(XM_HM_AC_PARTITION_HALT),
    FILL_TAB(XM_HM_AC_HYPERVISOR_HALT),
    FILL_TAB(XM_HM_AC_PROPAGATE),
    FILL_TAB(XM_HM_AC_SWITCH_TO_MAINTENANCE),
};

#endif

REGISTER_OBJ(SetupHm);

xm_s32_t HmRaiseEvent(xmHmLog_t *log) {
    xm_s32_t propagate=0;
#ifdef CONFIG_CYCLIC_SCHED
    xm_s32_t  oldPlanId;
#endif
    cpuCtxt_t ctxt;
    xm_u32_t eventId, system;
    xmId_t partitionId;
    xmTime_t cTime;
#ifdef CONFIG_AUDIT_EVENTS
    xmWord_t auditArgs[3];
#endif
#ifdef CONFIG_OBJ_HM_VERBOSE
    xm_s32_t e;
#endif

    if (!hmInit) return 0;
#ifdef CONFIG_AUDIT_EVENTS
    auditArgs[0]=log->opCodeL;
    auditArgs[1]=log->opCodeH;
    auditArgs[2]=log->cpuCtxt.pc;
    RaiseAuditEvent(TRACE_HM_MODULE, AUDIT_HM_EVENT_RAISED, 3, auditArgs);
#endif
    cTime=GetSysClockUsec();
    log->signature=XM_HMLOG_SIGNATURE;
    eventId=(log->opCodeL&HMLOG_OPCODE_EVENT_MASK)>>HMLOG_OPCODE_EVENT_BIT;
    ASSERT((eventId>=0)&&(eventId<XM_HM_MAX_EVENTS));   
    partitionId=(log->opCodeL&HMLOG_OPCODE_PARTID_MASK)>>HMLOG_OPCODE_PARTID_BIT;
    system=(log->opCodeH&HMLOG_OPCODE_SYS_MASK)?1:0;
#ifdef CONFIG_OBJ_STATUS_ACC
    systemStatus.noHmEvents++;
#endif
    log->timestamp=cTime;
#ifdef CONFIG_OBJ_HM_VERBOSE
    kprintf("[HM] %lld:", log->timestamp);

    if ((eventId<XM_HM_MAX_EVENTS)&&hmEvent2Str[eventId])
        kprintf("%s ", hmEvent2Str[eventId]);
    else
        kprintf("unknown ");
    
    kprintf("(%d)", eventId);

    if (system)
        kprintf(":SYS");
    else
        kprintf(":PART");

    kprintf("(%d)\n", partitionId);

    if (!(log->opCodeH&HMLOG_OPCODE_VALID_CPUCTXT_MASK)) {
        for (e=0; e<XM_HMLOG_PAYLOAD_LENGTH; e++)
            kprintf("0x%lx ", log->payload[e]);
        kprintf("\n");
    } else
        PrintHmCpuCtxt(&log->cpuCtxt);   
#endif
    if (system) {
#ifdef CONFIG_OBJ_HM_VERBOSE
        kprintf("[HM] ");
        if ((xmcTab.hpv.hmTab[eventId].action<XM_HM_MAX_ACTIONS)&&hmAction2Str[xmcTab.hpv.hmTab[eventId].action])
            kprintf("%s", hmAction2Str[xmcTab.hpv.hmTab[eventId].action]);
        else
            kprintf("unknown");
        kprintf("(%d) ", xmcTab.hpv.hmTab[eventId].action);
        
        kprintf("%s\n", hmLog2Str[xmcTab.hpv.hmTab[eventId].log]);
#endif
#ifdef CONFIG_AUDIT_EVENTS
        auditArgs[0]=((xmcTab.hpv.hmTab[eventId].log)?1<<31:0)|xmcTab.hpv.hmTab[eventId].action;
        RaiseAuditEvent(TRACE_HM_MODULE, AUDIT_HM_HPV_ACTION, 1, auditArgs);
#endif
	if (xmcTab.hpv.hmTab[eventId].log) {
            xm_u32_t tmpSeq;
            log->checksum=0;
            tmpSeq=seq++;
            log->opCodeH&=~HMLOG_OPCODE_SEQ_MASK;
            log->opCodeH|=tmpSeq<<HMLOG_OPCODE_SEQ_BIT;
            log->checksum=CalcCheckSum((xm_u16_t *)log, sizeof(struct xmHmLog));
            LogStreamInsert(&hmLogStream, log);
        }
	switch(xmcTab.hpv.hmTab[eventId].action) {
	case XM_HM_AC_IGNORE:
	    // Doing nothing
	    break;
	case XM_HM_AC_HYPERVISOR_COLD_RESET:
            resetStatusInit[0]=(XM_HM_RESET_STATUS_MODULE_RESTART<<XM_HM_RESET_STATUS_USER_CODE_BIT)|(eventId&XM_HM_RESET_STATUS_EVENT_MASK);
            ResetSystem(XM_COLD_RESET);
	    break;
	case XM_HM_AC_HYPERVISOR_WARM_RESET:
            resetStatusInit[0]=(XM_HM_RESET_STATUS_MODULE_RESTART<<XM_HM_RESET_STATUS_USER_CODE_BIT)|(eventId&XM_HM_RESET_STATUS_EVENT_MASK);
            ResetSystem(XM_WARM_RESET);
	    break;
#ifdef CONFIG_CYCLIC_SCHED
        case XM_HM_AC_SWITCH_TO_MAINTENANCE:
            SwitchSchedPlan(1, &oldPlanId);
	    MakePlanSwitch(cTime, &GET_LOCAL_SCHED()->data->cyclic);
            Schedule();
	    break;
#endif
        case XM_HM_AC_HYPERVISOR_HALT:
	    HaltSystem();
	    break;
	default:
            GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt, "Unknown health-monitor action %d\n", xmcTab.hpv.hmTab[eventId].action);
	}
    } else {
#ifdef CONFIG_OBJ_HM_VERBOSE
        kprintf("[HM] ");
        if ((partitionTab[partitionId].cfg->hmTab[eventId].action<XM_HM_MAX_ACTIONS)&&hmAction2Str[partitionTab[partitionId].cfg->hmTab[eventId].action])
            kprintf("%s", hmAction2Str[partitionTab[partitionId].cfg->hmTab[eventId].action]);
        else
            kprintf("unknown");

        kprintf("(%d) ", partitionTab[partitionId].cfg->hmTab[eventId].action);
        kprintf("%s\n", hmLog2Str[partitionTab[partitionId].cfg->hmTab[eventId].log]);
#endif
#ifdef CONFIG_AUDIT_EVENTS
        auditArgs[0]=((partitionTab[partitionId].cfg->hmTab[eventId].log)?1<<31:0)|partitionTab[partitionId].cfg->hmTab[eventId].action;
        RaiseAuditEvent(TRACE_HM_MODULE, AUDIT_HM_PART_ACTION, 1, auditArgs);
#endif
        if (partitionTab[partitionId].cfg->hmTab[eventId].log) {
            xm_u32_t tmpSeq;
            log->checksum=0;
            tmpSeq=seq++;
            log->opCodeH&=~HMLOG_OPCODE_SEQ_MASK;
            log->opCodeH|=tmpSeq<<HMLOG_OPCODE_SEQ_BIT;
            log->checksum=CalcCheckSum((xm_u16_t *)log, sizeof(struct xmHmLog));
	    LogStreamInsert(&hmLogStream, log);
        }

	ASSERT(partitionId<xmcTab.noPartitions);
        switch(partitionTab[partitionId].cfg->hmTab[eventId].action) {
	case XM_HM_AC_IGNORE:
	    // Doing nothing
	    break;
	case XM_HM_AC_SHUTDOWN:
            SHUTDOWN_PARTITION(partitionId);
	    break;
	case XM_HM_AC_PARTITION_COLD_RESET:
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d cold reseted\n", partitionId);
#endif
	    if (ResetPartition(&partitionTab[partitionId], XM_COLD_RESET, eventId)<0) {
                HALT_PARTITION(partitionId);
                Schedule();
            }
                
	    break;
	case XM_HM_AC_PARTITION_WARM_RESET:
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d warm reseted\n", partitionId);
#endif
	    if (ResetPartition(&partitionTab[partitionId], XM_WARM_RESET, eventId)<0) {
                HALT_PARTITION(partitionId);
                Schedule();
            }
            
	    break;
	case XM_HM_AC_HYPERVISOR_COLD_RESET:
            resetStatusInit[0]=(XM_HM_RESET_STATUS_MODULE_RESTART<<XM_HM_RESET_STATUS_USER_CODE_BIT)|(eventId&XM_HM_RESET_STATUS_EVENT_MASK);
            ResetSystem(XM_COLD_RESET);
	    break;
	case XM_HM_AC_HYPERVISOR_WARM_RESET:
            resetStatusInit[0]=(XM_HM_RESET_STATUS_MODULE_RESTART<<XM_HM_RESET_STATUS_USER_CODE_BIT)|(eventId&XM_HM_RESET_STATUS_EVENT_MASK);
            ResetSystem(XM_WARM_RESET);
	    break;
	case XM_HM_AC_SUSPEND:
	    ASSERT(partitionId!=XM_HYPERVISOR_ID);
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d suspended\n", partitionId);
#endif
            SUSPEND_PARTITION(partitionId);
	    Schedule();
	    break;
        case XM_HM_AC_PARTITION_HALT:
	    ASSERT(partitionId!=XM_HYPERVISOR_ID);
#ifdef CONFIG_OBJ_HM_VERBOSE
	    kprintf("[HM] Partition %d halted\n", partitionId);
#endif
            HALT_PARTITION(partitionId);
	    Schedule();
	    break;
        case XM_HM_AC_HYPERVISOR_HALT:
	    HaltSystem();
	    break;
#ifdef CONFIG_CYCLIC_SCHED
        case XM_HM_AC_SWITCH_TO_MAINTENANCE:
            SwitchSchedPlan(1, &oldPlanId);
	    MakePlanSwitch(cTime, &GET_LOCAL_SCHED()->data->cyclic);
            Schedule();
	    break;
#endif
	case XM_HM_AC_PROPAGATE:
	    propagate=1;
	    break;
	default:
            GetCpuCtxt(&ctxt);
            SystemPanic(&ctxt, "Unknown health-monitor action %d\n", partitionTab[partitionId].cfg->hmTab[eventId].action);
	}
    }

    return propagate;
}
