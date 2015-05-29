/*
 * $FILE: memblock.h
 *
 * Memory block driver
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_MEMBLOCK_DRV_H_
#define _XM_MEMBLOCK_DRV_H_

#include <xmconf.h>
struct memBlockData {
    xm_s32_t pos;
    xmAddress_t addr;
    struct xmcMemBlock *cfg;
};

#endif
