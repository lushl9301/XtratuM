/*
 * $FILE: gen_offsets.h
 *
 * ASM offsets, this file only can be included from asm-offset.c
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef __GEN_OFFSETS_H_
#define _GEN_OFFSETS_H_
#ifndef _GENERATE_OFFSETS_
#error Do not include this file
#endif

#include <objects/commports.h>
#include <objects/console.h>
#include <objects/status.h>
#include <drivers/memblock.h>
#include <arch/atomic.h>
#include <kthread.h>
#include <sched.h>
#include <kdevice.h>
#include <logstream.h>
#include <physmm.h>

static inline void GenerateOffsets(void) {
    // localSched_t
    DEFINE(cKThread, offsetof(localSched_t, cKThread),);

   // kthread_t
    DEFINE(ctrl, offsetof(kThread_t, ctrl),);
    
    // guest
    DEFINE(partCtrlTab, offsetof(struct guest, partCtrlTab),);

    // struct __kthread
    DEFINE(g, offsetof(struct __kThread, g),);
    DEFINE(kStack, offsetof(struct __kThread, kStack),);
    DEFINE(irqCpuCtxt, offsetof(struct __kThread, irqCpuCtxt),);

    // gctrl_t
    DEFINE(id, offsetof(partitionControlTable_t, id),);
    DEFINE(iFlags, offsetof(partitionControlTable_t, iFlags),);
    DEFINE(flags, offsetof(cpuCtxt_t, flags),);
    DEFINE(cs, offsetof(cpuCtxt_t, cs),);
    DEFINE(ax, offsetof(cpuCtxt_t, ax),);
    DEFINE(eCode, offsetof(cpuCtxt_t, eCode),);
    DEFINE(prev, offsetof(cpuCtxt_t, prev),);
    //DEFINE(localCpu_cpuCtxt, offsetof(localCpu_t, cpuCtxt),);
    //DEFINE(localTime_flags, offsetof(localTime_t, flags),);
    // sizeof
    DEFINE2(kthread_t,  sizeof(kThread_t), );
    DEFINE2(kthreadptr_t,  sizeof(kThread_t *), );
    DEFINE2(partition_t, sizeof(partition_t), );
    DEFINE2(struct_guest,  sizeof(struct guest), );
    DEFINE2(kdevice_t, sizeof(kDevice_t), );
    DEFINE2(struct_memblockdata, sizeof(struct memBlockData), );    
    DEFINE2(struct_console, sizeof(struct console), );
    DEFINE2(xmPartitionStatus_t, sizeof(xmPartitionStatus_t), );
    DEFINE2(struct_logstream, sizeof(struct logStream), );
    DEFINE2(union_channel, sizeof(union channel), );
    DEFINE2(struct_port, sizeof(struct port), );
    DEFINE2(struct_msg, sizeof(struct msg), );
    DEFINE2(struct_physpageptr, sizeof(struct physPage *), );
    DEFINE2(struct_physpage, sizeof(struct physPage), );
    DEFINE2(struct_scheddata, sizeof(struct schedData), );
    DEFINE2(localTime_t, sizeof(localTime_t), );
}

#endif
