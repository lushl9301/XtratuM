/*
 * $FILE: xm.h
 *
 * Guest header file
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_H_
#define _LIB_XM_H_

#ifdef _XM_KERNEL_
#error Guest file, do not include.
#endif

#include <xm_inc/config.h>
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmef.h>
#include <xmhypercalls.h>

#ifndef __ASSEMBLY__

#include <xm_inc/hypercalls.h>
#include <xm_inc/guest.h>

extern struct libXmParams {
    partitionControlTable_t *partCtrlTab[CONFIG_MAX_NO_VCPUS];
    struct xmPhysicalMemMap *partMemMap[CONFIG_MAX_NO_VCPUS];
    xmWord_t *commPortBitmap[CONFIG_MAX_NO_VCPUS];
} libXmParams;

extern __stdcall void init_libxm(partitionControlTable_t *partCtrlTab);

static inline struct xmPhysicalMemMap *XM_get_partition_mmap(void) {    
    return libXmParams.partMemMap[XM_get_vcpuid()];
}

static inline partitionControlTable_t *XM_get_PCT(void) {
    return libXmParams.partCtrlTab[XM_get_vcpuid()];
    //return (partitionControlTable_t *)((xm_u8_t *)libXmParams.partCtrlTab+XM_get_vcpuid()*libXmParams.partCtrlTab->partCtrlTabSize);
 }

static inline partitionControlTable_t *XM_get_PCT0(void) {
    return libXmParams.partCtrlTab[0];
}

static inline xmWord_t *XM_get_commport_bitmap(void) {    
    return libXmParams.commPortBitmap[XM_get_vcpuid()];
}

static inline xmId_t XM_get_number_vcpus(void){
    return XM_get_PCT0()->noVCpus;
}


#include <comm.h>
#include <hm.h>
#include <hypervisor.h>
#include <trace.h>
#include <status.h>

#endif

#endif
