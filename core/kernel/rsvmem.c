/*
 * $FILE: rsvmem.c
 *
 * Memory for structures
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xmconf.h>
#include <assert.h>
#include <processor.h>

void InitRsvMem(void) {
    xm_s32_t e;
    for (e=0; xmcRsvMemTab[e].obj; e++)
	xmcRsvMemTab[e].usedAlign&=~RSV_MEM_USED;
}

void *AllocRsvMem(xm_u32_t size, xm_u32_t align) {
    xm_s32_t e;
    for (e=0; xmcRsvMemTab[e].obj; e++) {        
	if (!(xmcRsvMemTab[e].usedAlign&RSV_MEM_USED)&&((xmcRsvMemTab[e].usedAlign&~RSV_MEM_USED)==align)&&(xmcRsvMemTab[e].size==size)) {
            xmcRsvMemTab[e].usedAlign|=RSV_MEM_USED;
	    return (void *)((xmAddress_t)xmcRsvMemTab[e].obj+(xmAddress_t)&xmcTab);
	}
    }
    return 0;
}

#ifdef CONFIG_DEBUG
void RsvMemDebug(void) {
    xm_s32_t e;
    for (e=0; xmcRsvMemTab[e].obj; e++)
	if (!(xmcRsvMemTab[e].usedAlign&RSV_MEM_USED))
            PWARN("RsvMem not used %d:%d:0x%x\n", xmcRsvMemTab[e].usedAlign&~RSV_MEM_USED, xmcRsvMemTab[e].size, xmcRsvMemTab[e].obj);
    //SystemPanic("RsvMem not used %d:%d:%x\n", xmcRsvMemTab[e].align,xmcRsvMemTab[e].size, xmcRsvMemTab[e].obj);
}
#endif

