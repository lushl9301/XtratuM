/*
 * $FILE: kthread.c
 *
 * Kernel and Guest context
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
#include <kthread.h>
#include <physmm.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <xmef.h>
#include <arch/xm_def.h>

void SetupPctMm(partitionControlTable_t *partCtrlTab, kThread_t *k) {
    partCtrlTab->arch._ARCH_PTDL1_REG=k->ctrl.g->kArch.ptdL1;
}
