/*
 * $FILE: mem.h
 *
 * memory object definitions
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_MEM_H_
#define _XM_OBJ_MEM_H_

#define XM_OBJ_MEM_CPY_AREA 0x1

union memCmd {
	struct cpyArea {
            xmId_t dstId; 
            xmAddress_t dstAddr;
            xmId_t srcId;
            xmAddress_t srcAddr;
            xmSSize_t size;
        } cpyArea;
};

/*#define XM_MEM_GET_PHYSMEMMAP 0x1

union memCmd {
    struct getPhysMemMap {
	struct xmcMemoryArea *areas;
	xm_s32_t noAreas;
    } physMemMap;
};
*/
#endif
