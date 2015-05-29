/*
 * $FILE: physmm.c
 *
 * Physical memory manager
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
#include <list.h>
#include <rsvmem.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/paging.h>
#include <arch/physmm.h>
#include <arch/xm_def.h>

static struct dynList cacheLRU;
static struct physPage **physPageTab;

struct physPage *PmmFindAnonymousPage(xmAddress_t pAddr) {
    xm_s32_t l, r, c;
    xmAddress_t a, b;

    pAddr&=PAGE_MASK;
    for (l=0, r=xmcTab.noRegions-1; l<=r; ) {
        c=(l+r)>>1;
        a=xmcMemRegTab[c].startAddr;
        b=a+xmcMemRegTab[c].size-1;
        if (pAddr<a) {
            r=c-1;
        } else {
            if (pAddr>b) {
                l=c+1;
            } else {
                ASSERT((pAddr>=a)&&((pAddr+PAGE_SIZE-1)<=b));
                if (!(xmcMemRegTab[c].flags&XMC_REG_FLAG_PGTAB)) return 0;

                return &physPageTab[c][(pAddr-a)>>PAGE_SHIFT];
            }
        }
    }

#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_PPG_DOESNT_EXIST, 1, (xmWord_t *)&pAddr);
#endif

    return 0;
}

struct physPage *PmmFindPage(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags) {
    struct xmcMemoryArea *memArea;    
    struct xmcPartition *cfg;
    xm_s32_t l, r, c;
    xmAddress_t a, b;
    ASSERT(p);
    pAddr&=PAGE_MASK;
    cfg=p->cfg;
    for (l=0, r=cfg->noPhysicalMemoryAreas-1; l<=r; ) {
        c=(l+r)>>1;
        memArea=&xmcPhysMemAreaTab[c+cfg->physicalMemoryAreasOffset];
        a=memArea->startAddr;
        b=a+memArea->size-1;
        if (pAddr<a) {
            r=c-1;
        } else {
            if (pAddr>b) {
                l=c+1;
            } else {
                struct xmcMemoryRegion *memRegion=
                    &xmcMemRegTab[memArea->memoryRegionOffset];
                ASSERT((pAddr>=a)&&((pAddr+PAGE_SIZE-1)<=b));
                if (!(memRegion->flags&XMC_REG_FLAG_PGTAB))
                    return 0;
                if (flags)
                    *flags=memArea->flags;
                
                return &physPageTab[memArea->memoryRegionOffset][(pAddr-memRegion->startAddr)>>PAGE_SHIFT];
            }                
        }
    }

#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_PPG_DOESNT_BELONG, 1, (xmWord_t *)&pAddr);
#endif

    return 0;
}

xm_s32_t PmmFindAddr(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags) {
    struct xmcMemoryArea *memArea;
    struct xmcPartition *cfg;
    xm_s32_t l, r, c;
    xmAddress_t a, b;

    pAddr&=PAGE_MASK;
    cfg=p->cfg;

    for (l=0, r=cfg->noPhysicalMemoryAreas-1; l<=r; ) {
        c=(l+r)>>1;
        memArea=&xmcPhysMemAreaTab[c+cfg->physicalMemoryAreasOffset];
        a=memArea->startAddr;
        b=a+memArea->size-1;
        if (pAddr<a) {
            r=c-1;
        } else {
            if (pAddr>b) {
                l=c+1;
            } else {
                ASSERT((pAddr>=a)&&((pAddr+PAGE_SIZE-1)<=b));
                if (flags)
                    *flags=memArea->flags;
                return 1;
            }
        }
    }

    PWARN("Page 0x%x does not belong to %d\n", pAddr, cfg->id);
#ifdef CONFIG_AUDIT_EVENTS
    RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_PPG_DOESNT_BELONG, 1, (xmWord_t *)&pAddr);
#endif

    return 0;
}

xm_s32_t PmmFindArea(xmAddress_t pAddr, xmSSize_t size, partition_t *p, xm_u32_t *flags) {
    struct xmcMemoryArea *memArea;
    struct xmcPartition *cfg;
    xm_s32_t l, r, c;
    xmAddress_t a, b;

    if (p) {
        cfg=p->cfg;

        for (l=0, r=cfg->noPhysicalMemoryAreas-1; l<=r; ) {
            c=(l+r)>>1;
            memArea=&xmcPhysMemAreaTab[c+cfg->physicalMemoryAreasOffset];
            a=memArea->startAddr;
            b=a+memArea->size-1;
            if (pAddr<a) {
                r=c-1;
            } else {
                if ((pAddr+size-1)>b) {
                l=c+1;
                } else {
                    ASSERT((pAddr>=a)&&((pAddr+size-1)<=b));
                    if (flags)
                        *flags=memArea->flags;
                    return 1;
                }
            }
        }
    } else {
        for (l=0, r=xmcTab.noRegions-1; l<=r; ) {
            c=(l+r)>>1;
            a=xmcMemRegTab[c].startAddr;
            b=a+xmcMemRegTab[c].size-1;
            if (pAddr<a) {
                r=c-1;
            } else {
                if ((pAddr+size-1)>b) {
                l=c+1;
                } else {
                    ASSERT((pAddr>=a)&&((pAddr+size-1)<=b));
                    if (flags)
                        *flags=xmcMemRegTab[c].flags;
                    return 1;
                }
            }
        }
    }

    return 0;
}

void PmmResetPartition(partition_t *p) {
    struct xmcMemoryRegion *memRegion;
    struct xmcMemoryArea *memArea;
    struct physPage *page;
    xmAddress_t addr;
    xm_s32_t e;
    
    for (e=0; e<p->cfg->noPhysicalMemoryAreas; e++) {
        memArea=&xmcPhysMemAreaTab[e+p->cfg->physicalMemoryAreasOffset];
        memRegion=&xmcMemRegTab[memArea->memoryRegionOffset];
        if (memRegion->flags&XMC_REG_FLAG_PGTAB)
            for (addr=memArea->startAddr; addr<memArea->startAddr+memArea->size; addr+=PAGE_SIZE) {
                page=&physPageTab[memArea->memoryRegionOffset][(addr-memRegion->startAddr)>>PAGE_SHIFT];
                SpinLock(&page->lock);
                page->type=PPAG_STD;
                page->counter=0;
                SpinUnlock(&page->lock);

                if (page->mapped)
                    VCacheUnlockPage(page);
            }
    }
}

#ifndef CONFIG_ARCH_MMU_BYPASS

//#ifdef CONFIG_SMP
//#error "XXX: not SMP safe"
//#endif

void *VCacheMapPage(xmAddress_t pAddr, struct physPage *page) {
    if (page->mapped)
        return (void *)(page->vAddr+(pAddr&(PAGE_SIZE-1)));

    if (VmmGetNoFreeFrames()<=0) {
        struct physPage *victimPage;
        // Unmapping the last mapped page
        victimPage=DynListRemoveTail(&cacheLRU);
        ASSERT(victimPage);
        victimPage->unlocked=0;
        victimPage->mapped=0;
        page->vAddr=victimPage->vAddr;
        victimPage->vAddr=~0;
    } else
        page->vAddr=VmmAlloc(1);

    page->mapped=1;
    VmMapPage(pAddr&PAGE_MASK, page->vAddr, _PG_ATTR_PRESENT|_PG_ATTR_RW|_PG_ATTR_CACHED);
    return (void *)(page->vAddr+(pAddr&(PAGE_SIZE-1)));
}

void VCacheUnlockPage(struct physPage *page) {
    ASSERT(page&&page->mapped);
    if (!page->unlocked) {
	page->unlocked=1;
	DynListInsertHead(&cacheLRU, &page->listNode);
    }
}

#else

void *VCacheMapPage(xmAddress_t pAddr, struct physPage *page) {
    return (void *)pAddr;
}

void VCacheUnlockPage(struct physPage *page) {
}

#endif

void SetupPhysMM(void) {
    xm_s32_t e, i;
    
    DynListInit(&cacheLRU);
    GET_MEMZ(physPageTab, sizeof(struct physPage *)*xmcTab.noRegions);
    for (e=0; e<xmcTab.noRegions; e++) {
	ASSERT(!(xmcMemRegTab[e].size&(PAGE_SIZE-1))&&!(xmcMemRegTab[e].startAddr&(PAGE_SIZE-1)));
	if (xmcMemRegTab[e].flags&XMC_REG_FLAG_PGTAB) {
	    GET_MEMZ(physPageTab[e], sizeof(struct physPage)*(xmcMemRegTab[e].size/PAGE_SIZE));
            for (i=0; i<xmcMemRegTab[e].size/PAGE_SIZE; i++)
                physPageTab[e][i].lock=SPINLOCK_INIT;
        } else {
	    physPageTab[e]=0;
        }
    }
}


