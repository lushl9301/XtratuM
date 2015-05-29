/*
 * $FILE: vmmap.c
 *
 * Virtual memory map
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
#include <kthread.h>
#include <physmm.h>
#include <rsvmem.h>
#include <sched.h>
#include <vmmap.h>
#include <arch/physmm.h>
#include <arch/paging.h>

static inline xmAddress_t VAddr2PAddr(struct xmcMemoryArea *mAreas, xm_s32_t noAreas, xmAddress_t vAddr) {
    xm_s32_t e;
    for (e=0; e<noAreas; e++)
        if ((mAreas[e].mappedAt<=vAddr)&&(((mAreas[e].mappedAt+mAreas[e].size)-1)>=vAddr))
            return vAddr-mAreas[e].mappedAt+mAreas[e].startAddr;
    return -1;
}

static xmAddress_t AllocMem(struct xmcPartition *cfg, xmSize_t size, xm_u32_t align, xmAddress_t *pool, xmSSize_t *maxSize) {
    xmAddress_t addr;
    xm_s32_t e;
    
    if (*pool&(align-1)) {
        *maxSize-=align-(*pool&(align-1));
        *pool=align+(*pool&~(align-1));
    }

    addr=*pool;
    *pool+=size;
    if ((*maxSize-=size)<0){
        kprintf("[SetupPageTable] partition page table couldn't be created\n");
        return ~0;
    }

    for (e=0; e<size; e+=sizeof(xm_u32_t)) {
        WriteByPassMmuWord((void *)(addr+e), 0);
    }

    addr=VAddr2PAddr(&xmcPhysMemAreaTab[cfg->physicalMemoryAreasOffset], cfg->noPhysicalMemoryAreas, addr);
    return addr;
}

static inline int SetupLdr(partition_t *p, xmWord_t *pPtdL1, xmAddress_t at, xmAddress_t pgTb, xmSize_t size){
    extern xm_u8_t _sldr[], _eldr[];
    xmAddress_t addr,vAddr=0, a, b;
    struct physPage *page;
    void *stack;
    xmWord_t attr;
    xm_s32_t i;

    ASSERT(((xmAddress_t)_eldr-(xmAddress_t)_sldr)<=256*1024);
    ASSERT(xmcBootPartTab[p->cfg->id].noCustomFiles<=CONFIG_MAX_NO_CUSTOMFILES);

    /*Partition Loader Stack*/
    if (!(p->vLdrStack)){
       GET_MEMA(stack, 18*PAGE_SIZE,PAGE_SIZE);
       p->vLdrStack=(xmAddress_t)stack;
    }
    else
       stack=(void *)p->vLdrStack;

    a=_VIRT2PHYS(stack);
    b=a+(18*PAGE_SIZE)-1;
    vAddr=at-18*PAGE_SIZE;

    for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
        if (VmMapUserPage(p, pPtdL1, addr, vAddr, _PG_ATTR_PRESENT|_PG_ATTR_USER|_PG_ATTR_CACHED|_PG_ATTR_RW, AllocMem, &pgTb,&size)<0){
           kprintf("[SetupLdr(P%d)] Error mapping the Partition Loader Stack\n",p->cfg->id);
           return -1;
        }
    }

    /*Partition Loader code*/
    a=(xmAddress_t)_sldr;
    b=a+(256*1024)-1;
    vAddr=at;
    for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
        if (VmMapUserPage(p, pPtdL1, addr, vAddr, _PG_ATTR_PRESENT|_PG_ATTR_USER|_PG_ATTR_CACHED, AllocMem, &pgTb,&size)<0){
           kprintf("[SetupLdr(P%d)] Error mapping the Partition Loader Code\n",p->cfg->id);
           return -1;
        }
    }

    /*Mapping partition image from container*/
    a = xmcBootPartTab[p->cfg->id].imgStart;
    b = a+(xmcBootPartTab[p->cfg->id].imgSize)-1;
//    vAddr=at+256*1024;
    vAddr=a;
    p->imgStart=vAddr;
    for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
        if (VmMapUserPage(p, pPtdL1, addr, vAddr, _PG_ATTR_PRESENT|_PG_ATTR_USER|_PG_ATTR_CACHED, AllocMem, &pgTb,&size)<0){
           kprintf("[SetupLdr(P%d)] Error mapping the Partition image from container\n",p->cfg->id);
           return -1;
        }
    }


    for (i=0; i<xmcBootPartTab[p->cfg->id].noCustomFiles; i++){
        a=xmcBootPartTab[p->cfg->id].customFileTab[i].sAddr;
        b=a+xmcBootPartTab[p->cfg->id].customFileTab[i].size-1;
        for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
            if (VmMapUserPage(p, pPtdL1, addr, vAddr, _PG_ATTR_PRESENT|_PG_ATTR_USER|_PG_ATTR_CACHED, AllocMem, &pgTb,&size)<0){
               kprintf("[SetupLdr(P%d)] Error mapping the CustomFile(%d) image from container\n",p->cfg->id,i);
               return -1;
            }
        }
    }

    return 0;
}

xmAddress_t SetupPageTable(partition_t *p, xmAddress_t pgTb, xmSize_t size) {
    xmAddress_t addr, vAddr=0, a, b, pT;
    xmWord_t *pPtdL1, attr;
    struct physPage *pagePtdL1, *page;
    xm_s32_t e;

    if ((pT=AllocMem(p->cfg, PTDL1SIZE, PTDL1SIZE, &pgTb, &size))==~0) {
        PWARN("(%d) Unable to create page table (out of memory)\n", p->cfg->id);
        return ~0;
    }

    if (!(pagePtdL1=PmmFindPage(pT, p, 0))) {
        PWARN("(%d) Page 0x%x does not belong to this partition\n", p->cfg->id, pT);
        return ~0;
    }

    pagePtdL1->type=PPAG_PTDL1;

    // Incremented because it is load as the initial page table
    PPagIncCounter(pagePtdL1);    
    pPtdL1=VCacheMapPage(pT, pagePtdL1);
    ASSERT(PTDL1SIZE<=PAGE_SIZE);
    for (e=0; e<PTDL1ENTRIES; e++)
        WriteByPassMmuWord(&pPtdL1[e], 0);

    for (e=0; e<p->cfg->noPhysicalMemoryAreas; e++) {
        if (xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNMAPPED)
 	    continue;

        a=xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].startAddr;
        b=a+xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].size-1;
        vAddr=xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].mappedAt;
        for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
            attr=_PG_ATTR_PRESENT|_PG_ATTR_USER;
            
            if (!(xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNCACHEABLE))
                attr|=_PG_ATTR_CACHED;
            
            if (!(xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_READONLY))
                attr|=_PG_ATTR_RW;

            if (VmMapUserPage(p, pPtdL1, addr, vAddr, attr, AllocMem, &pgTb, &size)<0)
                return ~0;
	}
    }


    attr=_PG_ATTR_PRESENT|_PG_ATTR_USER;
    ASSERT(p->pctArraySize);
    for (vAddr=XM_PCTRLTAB_ADDR, addr=(xmAddress_t)_VIRT2PHYS(p->pctArray);
         addr<((xmAddress_t)_VIRT2PHYS(p->pctArray)+p->pctArraySize);
         addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
        if (VmMapUserPage(p, pPtdL1, addr, vAddr, attr, AllocMem, &pgTb, &size)<0)
            return ~0;
    }

//    xmAddress_t vAddrLdr=CONFIG_XM_OFFSET+16*1024*1024;
    xmAddress_t vAddrLdr=XM_PCTRLTAB_ADDR-256*1024;
    if (SetupLdr(p, pPtdL1,vAddrLdr,pgTb, size)<0)
       return ~0;

    attr=_PG_ATTR_PRESENT|_PG_ATTR_USER;
    // Set appropriate permissions
    for (e=0; e<p->cfg->noPhysicalMemoryAreas; e++) {
        if (xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNMAPPED)
            continue;

        a=xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].startAddr;
        b=a+xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].size-1;
        vAddr=xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].mappedAt;
        for (addr=a; (addr>=a)&&(addr<b); addr+=PAGE_SIZE, vAddr+=PAGE_SIZE) {
            if ((page=PmmFindPage(addr, p, 0))) {
                PPagIncCounter(page);
                if (xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNCACHEABLE)
                    attr&=~_PG_ATTR_CACHED;
                else
                    attr|=_PG_ATTR_CACHED;

                if (page->type!=PPAG_STD) {
                    if (VmMapUserPage(p, pPtdL1, addr, vAddr, attr, AllocMem, &pgTb, &size)<0)
                        return ~0;
                }
            }
        }
    }

    VCacheUnlockPage(pagePtdL1);
  return pT;
}

