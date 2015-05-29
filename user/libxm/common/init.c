/*
 * $FILE: init.c
 *
 * Initialisation of the libxm
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>

struct libXmParams libXmParams;

__stdcall void init_libxm(partitionControlTable_t *partCtrlTab){
    xm_s32_t e=XM_get_vcpuid();

    libXmParams.partCtrlTab[e]=(partitionControlTable_t *)((xmAddress_t)partCtrlTab+(partCtrlTab->partCtrlTabSize*e));
    libXmParams.partMemMap[e]=(struct xmPhysicalMemMap *)((xmAddress_t)libXmParams.partCtrlTab[e]+sizeof(partitionControlTable_t));
    libXmParams.commPortBitmap[e]=(xmWord_t *)((xmAddress_t)libXmParams.partMemMap[e]+libXmParams.partCtrlTab[e]->noPhysicalMemAreas*sizeof(struct xmPhysicalMemMap));

    init_batch();
    init_arch_libxm();
}

