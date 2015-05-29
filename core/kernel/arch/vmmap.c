/*
 * $FILE: vmmap.c
 *
 * Virtual memory map management
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
#include <rsvmem.h>
#include <stdc.h>
#include <vmmap.h>
#include <virtmm.h>
#include <physmm.h>
#include <smp.h>
#include <arch/xm_def.h>
#include <arch/processor.h>

void __VBOOT SetupVmMap(xmAddress_t *stFrameArea, xm_s32_t *noFrames) {
    xmAddress_t st, end, page;
    xmAddress_t *rsvPages;
    xm_s32_t e, noPages;
    xm_u32_t *pgTable;

    st=xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].startAddr;
    end=st+xmcPhysMemAreaTab[xmcTab.hpv.physicalMemoryAreasOffset].size-1;

    *stFrameArea=ROUNDUP(_PHYS2VIRT(end+1), LPAGE_SIZE);
    *noFrames=((XM_VMAPEND-*stFrameArea)+1)/PAGE_SIZE;

    pgTable = (xm_u32_t *)_PHYS2VIRT(SaveCr3());
    for (page=_PHYS2VIRT(st); page<_PHYS2VIRT(end); page+=LPAGE_SIZE) {
        pgTable[VA2PtdL1(page)]=(_VIRT2PHYS(page)&LPAGE_MASK)|_PG_ARCH_PRESENT|_PG_ARCH_PSE|_PG_ARCH_RW|_PG_ARCH_GLOBAL;
        pgTable[VA2PtdL1(_VIRT2PHYS(page))]=(_VIRT2PHYS(page)&LPAGE_MASK)|_PG_ARCH_PRESENT|_PG_ARCH_PSE|_PG_ARCH_RW|_PG_ARCH_GLOBAL;
    }
    //memset((void *)((xmAddress_t)pgTable+PAGE_SIZE), 0, PTDL2SIZE);
    //pgTable[VA2PtdL1(*stFrameArea)]=(_VIRT2PHYS((xmAddress_t)pgTable+PAGE_SIZE)&PAGE_MASK)|_PG_ARCH_RW|_PG_ARCH_PRESENT;
    FlushTlb(); /* XXX: The XtratuM mappings have changed. This is required to update the TLB entries */

    noPages=(((XM_VMAPEND-*stFrameArea)+1)>>PTDL1_SHIFT);
    GET_MEMAZ(rsvPages, PTDL2SIZE*noPages, PTDL2SIZE);
    //memset(rsvPages, 0, PTDL2SIZE*noPages);
    for (e=VA2PtdL1(*stFrameArea); (e<PTDL1ENTRIES)&&(noPages>0); e++) {
        ASSERT(noPages>=0);
        pgTable[e]=(_VIRT2PHYS(rsvPages)&PAGE_MASK)|_PG_ARCH_PRESENT|_PG_ARCH_RW;
        rsvPages=(xmAddress_t *)((xmAddress_t)rsvPages+PTDL2SIZE);
        noPages--;
    }
    FlushTlbGlobal();
}

void SetupPtdL1(xmWord_t *ptdL1, kThread_t *k) {
    xm_s32_t l1e, e;

    l1e = VA2PtdL1(CONFIG_XM_OFFSET);
    for (e = l1e; e < PTDL1ENTRIES; e++)
        ptdL1[e] = _pgTables[e];
}

xm_u32_t VmArchAttr2Attr(xm_u32_t entry) {
    xm_u32_t flags=entry&(PAGE_SIZE-1), attr=0;

    if (flags&_PG_ARCH_PRESENT) attr|=_PG_ATTR_PRESENT;
    if (flags&_PG_ARCH_USER) attr|=_PG_ATTR_USER;
    if (flags&_PG_ARCH_RW) attr|=_PG_ATTR_RW;
    if (!(flags&_PG_ARCH_PCD)) attr|=_PG_ATTR_CACHED;
    return attr|(flags&~(_PG_ARCH_PRESENT|_PG_ARCH_USER|_PG_ARCH_RW|_PG_ARCH_PCD));
}

xm_u32_t VmAttr2ArchAttr(xm_u32_t flags) {
    xm_u32_t attr=0;

    if (flags&_PG_ATTR_PRESENT) attr|=_PG_ARCH_PRESENT;        
    if (flags&_PG_ATTR_USER) attr|=_PG_ARCH_USER;  
    if (flags&_PG_ATTR_RW) attr|=_PG_ARCH_RW;
    if (!(flags&_PG_ATTR_CACHED)) attr|=_PG_ARCH_PCD;
    return attr|(flags&0xffff);
}

xm_s32_t VmMapUserPage(partition_t *k, xmWord_t *ptdL1, xmAddress_t pAddr, xmAddress_t vAddr, xm_u32_t flags, xmAddress_t (*alloc)(struct xmcPartition *, xmSize_t, xm_u32_t, xmAddress_t *, xmSSize_t *), xmAddress_t *pool, xmSSize_t *poolSize) {
    struct physPage *pagePtdL2;
    xmAddress_t pT;
    xmWord_t *pPtdL2;
    xm_s32_t l1e, l2e, e;

    ASSERT(!(pAddr&(PAGE_SIZE-1)));
    ASSERT(!(vAddr&(PAGE_SIZE-1)));
    ASSERT(vAddr<CONFIG_XM_OFFSET);
    l1e=VA2PtdL1(vAddr);
    l2e=VA2PtdL2(vAddr);
    if (!(ptdL1[l1e]&_PG_ARCH_PRESENT)) {
        pT=alloc(k->cfg, PTDL2SIZE, PTDL2SIZE, pool, poolSize);
        if (!(pagePtdL2=PmmFindPage(pT, k, 0))) {
            return -1;
        }
        pagePtdL2->type=PPAG_PTDL2;
        PPagIncCounter(pagePtdL2);
        pPtdL2=VCacheMapPage(pT, pagePtdL2);
        for (e=0; e<PTDL2ENTRIES; ++e) {
            pPtdL2[e]=0;
        }
        ptdL1[l1e]=pT|_PG_ARCH_PRESENT|_PG_ARCH_RW;
    } else {
        pT=ptdL1[l1e]&PAGE_MASK;
        pagePtdL2=PmmFindPage(pT, k, 0);
        ASSERT(pagePtdL2);
        ASSERT(pagePtdL2->type==PPAG_PTDL2);
        ASSERT(pagePtdL2->counter>0);
        pPtdL2=VCacheMapPage(pT, pagePtdL2);
    }
    pPtdL2[l2e]=(pAddr&PAGE_MASK)|VmAttr2ArchAttr(flags);  
    VCacheUnlockPage(pagePtdL2);

    return 0;
}

void VmMapPage(xmAddress_t pAddr, xmAddress_t vAddr, xmWord_t flags) {    
    xmAddress_t *ptdLx;
    //xmAddress_t *pgTab;
    ASSERT(!(pAddr&(PAGE_SIZE-1)));
    ASSERT(!(vAddr&(PAGE_SIZE-1)));
    ASSERT(vAddr>=CONFIG_XM_OFFSET);

    //pgTab = //(xm_u32_t *)_PHYS2VIRT(SaveCr3());
    ASSERT((_pgTables[VA2PtdL1(vAddr)]&_PG_ARCH_PRESENT)==_PG_ARCH_PRESENT);

    ptdLx=(xmAddress_t *)_PHYS2VIRT(_pgTables[VA2PtdL1(vAddr)]&PAGE_MASK);
    //  ASSERT((ptdLx[VA2PtdL2(vAddr)]&_PG_ARCH_PRESENT)!=_PG_ARCH_PRESENT);
    ptdLx[VA2PtdL2(vAddr)]=pAddr|VmAttr2ArchAttr(flags);
    FlushTlbEntry(vAddr);
}

#if 0
void VmUnmapPage(xmAddress_t vAddr) {
    xmAddress_t *ptdLx;

    ASSERT(!(vAddr&(PAGE_SIZE-1)));
    ASSERT(vAddr>=CONFIG_XM_OFFSET);
    ASSERT((_pgTables[VA2PtdL1(vAddr)]&_PG_ARCH_PRESENT)==_PG_ARCH_PRESENT);
    ptdLx=(xmAddress_t *)_PHYS2VIRT(_pgTables[VA2PtdL1(vAddr)]&PAGE_MASK);
    ptdLx[VA2PtdL2(vAddr)]=0;
    FlushTlbEntry(vAddr);
}
#endif
