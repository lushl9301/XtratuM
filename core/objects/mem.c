/*
 * $FILE: mem.c
 *
 * System physical memory
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
#include <hypercalls.h>
#include <objdir.h>
#include <physmm.h>
#include <sched.h>
#include <stdc.h>

#include <objects/mem.h>

static inline xm_s32_t CopyArea(xmAddress_t dstAddr, xmId_t dstId, xmAddress_t srcAddr, xmId_t srcId, xmSSize_t size) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xm_u32_t flags;

    if (size<=0) return 0;
    if (dstId!=KID2PARTID(sched->cKThread->ctrl.g->id))
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;

    if (srcId!=KID2PARTID(sched->cKThread->ctrl.g->id))
        if (!(GetPartition(sched->cKThread)->cfg->flags&XM_PART_SYSTEM))
            return XM_PERM_ERROR;
    
    if (dstId!=UNCHECKED_ID) {
        if ((dstId<0)||(dstId>=xmcTab.noPartitions))
            return XM_INVALID_PARAM;

        if (!PmmFindArea(dstAddr, size, (dstId!=UNCHECKED_ID)?&partitionTab[dstId]:0, &flags))
            return XM_INVALID_PARAM;

        if (flags&XM_MEM_AREA_READONLY)
            return XM_INVALID_PARAM;
    }
    
    if (srcId!=UNCHECKED_ID) {
        if ((srcId<0)||(srcId>=xmcTab.noPartitions))
            return XM_INVALID_PARAM;

        if (!PmmFindArea(srcAddr, size, (srcId!=UNCHECKED_ID)?&partitionTab[srcId]:0, &flags))
            return XM_INVALID_PARAM;

    }
    
    if (size<=0) return 0;

    UnalignMemCpy((void *)dstAddr, (void*)srcAddr, size, (RdMem_t)ReadByPassMmuWord, (RdMem_t)ReadByPassMmuWord, (WrMem_t)WriteByPassMmuWord);
    
    return size;
}

static xm_s32_t CtrlMem(xmObjDesc_t desc, xm_u32_t cmd, union memCmd *__gParam args) {
    if (!args)
	return XM_INVALID_PARAM;

    if (CheckGParam(args, sizeof(union memCmd), 4, PFLAG_NOT_NULL)<0) 
        return XM_INVALID_PARAM;

    switch(cmd) {
    case XM_OBJ_MEM_CPY_AREA:
        return CopyArea(args->cpyArea.dstAddr, args->cpyArea.dstId, args->cpyArea.srcAddr, args->cpyArea.srcId, args->cpyArea.size);
     }

    return XM_INVALID_PARAM;
}

static const struct object memObj={
    .Ctrl=(ctrlObjOp_t)CtrlMem,
};

xm_s32_t __VBOOT SetupMem(void) {    
    objectTab[OBJ_CLASS_MEM]=&memObj;
    return 0;
}

REGISTER_OBJ(SetupMem);

