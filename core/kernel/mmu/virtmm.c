/*
 * $FILE: virtmm.c
 *
 * Virtual memory manager
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
#include <rsvmem.h>
#include <boot.h>
#include <processor.h>
#include <virtmm.h>
#include <vmmap.h>
#include <spinlock.h>
#include <stdc.h>
#include <arch/paging.h>
#include <arch/xm_def.h>

static xmAddress_t vmmStartAddr; 
static xm_s32_t noFrames;

#ifndef CONFIG_ARCH_MMU_BYPASS

xmAddress_t VmmAlloc(xm_s32_t nPag) {
    xmAddress_t vAddr;
    if ((noFrames-nPag)<0) 
	return 0;
    vAddr=vmmStartAddr;
    vmmStartAddr+=(nPag<<PAGE_SHIFT);
    noFrames-=nPag;
    ASSERT(noFrames>=0);
    return vAddr;
}

#else

#endif

xm_s32_t VmmGetNoFreeFrames(void) {
    ASSERT(noFrames>=0);
    return noFrames;
}

void __VBOOT SetupVirtMM(void) {
    xmAddress_t st, end;
    xm_u32_t flags;

    st=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].startAddr;
    end=st+xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].size-1;
    flags=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].flags;
    eprintf("XM map: [0x%"PRNT_ADDR_FMT"x - 0x%"PRNT_ADDR_FMT"x] flags: 0x%x\n", st, end, flags);
    ASSERT(st==CONFIG_XM_LOAD_ADDR);    
    SetupVmMap(&vmmStartAddr, &noFrames);
    eprintf("[VMM] Free [0x%"PRNT_ADDR_FMT"x-0x%"PRNT_ADDR_FMT"x] %d frames\n", vmmStartAddr, XM_VMAPEND, noFrames);
}
