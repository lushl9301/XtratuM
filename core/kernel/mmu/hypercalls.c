/*
 * $FILE: hypercalls.c
 *
 * XM's hypercalls
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
#include <gaccess.h>
#include <physmm.h>
#include <processor.h>
#include <sched.h>
#include <smp.h>
#include <spinlock.h>
#include <stdc.h>
#include <hypercalls.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/physmm.h>

#ifdef CONFIG_VMM_UPDATE_HYPERCALLS
static xm_s32_t UpdatePtd(struct physPage *pagePtd, xmAddress_t pAddr, xmAddress_t *val) {
    xmWord_t *vPtd=VCacheMapPage(pAddr, pagePtd), oldVal;
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;

    if (!IS_VALID_PTD_PTR(pagePtd->type, pAddr))
        return XM_INVALID_PARAM;

    //oldVal=*vPtd;
    oldVal=ReadByPassMmuWord(vPtd);
    VCacheUnlockPage(pagePtd);
    
    if (IS_PTD_PRESENT(*val)) {
	if (!(page=PmmFindPage(GET_PTD_ADDR(*val), GetPartition(sched->cKThread), 0))) {
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_INVLD_PTDE, 1, (xmWord_t *)val);
#endif
	    return XM_INVALID_PARAM;
	}

	if (!IS_VALID_PTD_ENTRY(page->type)) {
#ifdef CONFIG_AUDIT_EVENTS
            RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_INVLD_PTDE, 1, (xmWord_t *)val);
#endif
	    return XM_INVALID_PARAM;
	}
	PPagIncCounter(page);
    }
    
    if (IS_PTD_PRESENT(oldVal)) {
        if (!(page=PmmFindPage(GET_PTD_ADDR(oldVal), GetPartition(sched->cKThread), 0))) {
	    return XM_INVALID_PARAM;
	}
	ASSERT(IS_VALID_PTD_ENTRY(page->type));
	ASSERT(page->counter>0);
	PPagDecCounter(page);
    }

    return XM_OK;
}

static xm_s32_t UpdatePte(struct physPage *pagePte, xmAddress_t pAddr, xmAddress_t *val) {
    xmWord_t *vPte=VCacheMapPage(pAddr, pagePte), oldVal;
    localSched_t *sched=GET_LOCAL_SCHED();
    kThread_t *k=sched->cKThread;
    struct physPage *page;
    xm_u32_t areaFlags, attr;
    xm_s32_t isPCtrlTab=0;
    
    //oldVal=*vPte;
    oldVal=ReadByPassMmuWord(vPte);
    VCacheUnlockPage(pagePte);
    if (IS_PTE_PRESENT(*val)) {
	if (!(page=PmmFindPage(GET_PTE_ADDR(*val), GetPartition(k), &areaFlags)))
            if (!(isPCtrlTab=IsPCtrlTabPg(GET_PTE_ADDR(*val), k->ctrl.g)))
                if (!PmmFindAddr(GET_PTE_ADDR(*val), GetPartition(k), &areaFlags)) {
                    return XM_INVALID_PARAM;
                }

        attr=VmArchAttr2Attr(*val);

	if (areaFlags&XM_MEM_AREA_READONLY)
            attr&=~_PG_ATTR_RW;

	if (areaFlags&XM_MEM_AREA_UNCACHEABLE)
            attr&=~_PG_ATTR_CACHED;
        
	if (page) {
	    if (page->type!=PPAG_STD)
                attr&=~_PG_ATTR_RW;
	    PPagIncCounter(page);
	} else
            if (isPCtrlTab)
                attr&=~_PG_ATTR_RW;
        *val=(*val& _PG_ARCH_ADDR)|VmAttr2ArchAttr(attr);
    }
    
    if (IS_PTE_PRESENT(oldVal)) {
	if (!(page=PmmFindPage(GET_PTE_ADDR(oldVal), GetPartition(k), 0))) {
            if (!IsPCtrlTabPg(GET_PTE_ADDR(oldVal), k->ctrl.g))
                if (!PmmFindAddr(GET_PTE_ADDR(oldVal), GetPartition(k), 0)) {
                    return XM_INVALID_PARAM;
                }
        }
	if (page) {
	    ASSERT(page->counter>0);
	    PPagDecCounter(page);
	}        
    }

    return XM_OK;
}

static void UnsetPtd(xmAddress_t pAddr, struct physPage *pagePtd, xm_u32_t type) {
    xmWord_t *vPtd=VCacheMapPage(pAddr, pagePtd);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;
    xm_s32_t e;
    
    for (e=0; e<GET_USER_PTD_ENTRIES(type); e++) {
        xmWord_t vPtdVal=ReadByPassMmuWord(&vPtd[e]);
	if (IS_PTD_PRESENT(vPtdVal)) {
            if (!(page=PmmFindPage(GET_PTD_ADDR(vPtdVal), GetPartition(sched->cKThread), 0)))
		return;
	    ASSERT(IS_VALID_PTD_ENTRY(page->type));
	    ASSERT(page->counter>0);
	    PPagDecCounter(page);
	}
    }
    VCacheUnlockPage(pagePtd);
}

static void UnsetPte(xmAddress_t pAddr, struct physPage *pagePte, xm_u32_t type) {
    xmWord_t *vPte=VCacheMapPage(pAddr, pagePte);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;
    xm_s32_t e;
    
    for (e=0; e<GET_USER_PTE_ENTRIES(type); e++) {
        xmWord_t vPteVal=ReadByPassMmuWord(&vPte[e]);
	if (IS_PTE_PRESENT(vPteVal)) {
            if (!(page=PmmFindPage(GET_PTE_ADDR(vPteVal), GetPartition(sched->cKThread), 0)))
		return;
            ASSERT(IS_VALID_PTE_ENTRY(page->type));
	    ASSERT(page->counter>0);
	    PPagDecCounter(page);
	}
    }
    VCacheUnlockPage(pagePte);
}

static void SetPtd(xmAddress_t pAddr, struct physPage *pagePtd, xm_u32_t type) {
    xmWord_t *vPtd=VCacheMapPage(pAddr, pagePtd);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;
    xm_s32_t e;    

    for (e=0; e<GET_USER_PTD_ENTRIES(type); e++) {
        xmWord_t vPtdVal=ReadByPassMmuWord(&vPtd[e]);
	if (IS_PTD_PRESENT(vPtdVal)) {
            if (!(page=PmmFindPage(GET_PTD_ADDR(vPtdVal), GetPartition(sched->cKThread), 0)))
		return;

	    if (!IS_VALID_PTD_ENTRY(page->type)) {
#ifdef CONFIG_AUDIT_EVENTS
                RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_INVLD_PTDE, 1, (xmWord_t *)&vPtdVal);
#endif
		WriteByPassMmuWord(&vPtd[e], SET_PTD_NOT_PRESENT(vPtdVal));
		continue;
	    }
	    PPagIncCounter(page);
	}
    }
    CLONE_XM_PTD_ENTRIES(type, vPtd);

    VCacheUnlockPage(pagePtd);
}

static void SetPte(xmAddress_t pAddr, struct physPage *pagePte, xm_u32_t type) {
    xmWord_t *vPte=VCacheMapPage(pAddr, pagePte);
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;
    xm_u32_t areaFlags;
    xm_s32_t e;
    
    for (e=0; e<GET_USER_PTE_ENTRIES(type); e++) {
        xmWord_t vPteVal=ReadByPassMmuWord(&vPte[e]);
	if (IS_PTE_PRESENT(vPteVal)) {
	    if (!(page=PmmFindPage(GET_PTE_ADDR(vPteVal), GetPartition(sched->cKThread), &areaFlags)))
		if (!PmmFindAddr(GET_PTE_ADDR(vPteVal), GetPartition(sched->cKThread), &areaFlags)) {
		    return;
		}
	    if (areaFlags&XM_MEM_AREA_READONLY) 
                vPteVal=SET_PTE_RONLY(vPteVal);
	    
	    if (areaFlags&XM_MEM_AREA_UNCACHEABLE)
		vPteVal=SET_PTE_UNCACHED(vPteVal);

	    if (page) {
		if (page->type!=PPAG_STD)
                    vPteVal=SET_PTE_RONLY(vPteVal);
		PPagIncCounter(page);
	    }
            WriteByPassMmuWord(&vPte[e], vPteVal);
	}
    }
    VCacheUnlockPage(pagePte);
}

#endif

static xm_s32_t (* const UpdatePPag32HndlTab[NR_PPAG])(struct physPage *, xmAddress_t, xmAddress_t *) = {
#ifdef CONFIG_VMM_UPDATE_HYPERCALLS
    [PPAG_STD]=0,
    [PPAG_PTDL1]=UpdatePtd,
#if PTD_LEVELS>2
    [PPAG_PTDL2]=UpdatePtd,
    [PPAG_PTDL3]=UpdatePte,
#else
    [PPAG_PTDL2]=UpdatePte,
#endif
#else
    [PPAG_STD]=0,
    [PPAG_PTDL1]=0,
    [PPAG_PTDL2]=0,
#if PTD_LEVELS>2
    [PPAG_PTDL3]=0,
#endif
#endif
};

static void (*const UnsetPPagTypeHndlTab[NR_PPAG])(xmAddress_t, struct physPage *, xm_u32_t)={
#ifdef CONFIG_VMM_UPDATE_HYPERCALLS
    [PPAG_STD]=0,
    [PPAG_PTDL1]=UnsetPtd,
#if PTD_LEVELS>2
    [PPAG_PTDL2]=UnsetPtd,
    [PPAG_PTDL3]=UnsetPte,
#else
    [PPAG_PTDL2]=UnsetPte,
#endif
#else
    [PPAG_STD]=0,
    [PPAG_PTDL1]=0,
    [PPAG_PTDL2]=0,
#if PTD_LEVELS>2
    [PPAG_PTDL3]=0,
#endif
#endif
};

static void (* const SetPPagTypeHndlTab[NR_PPAG])(xmAddress_t, struct physPage *, xm_u32_t)={
#ifdef CONFIG_VMM_UPDATE_HYPERCALLS
    [PPAG_STD]=0, //SetStdPg,
    [PPAG_PTDL1]=SetPtd,
#if PTD_LEVELS>2
    [PPAG_PTDL2]=SetPtd,
    [PPAG_PTDL3]=SetPte,
#else
    [PPAG_PTDL2]=SetPte,
#endif
#else
    [PPAG_STD]=0,
    [PPAG_PTDL1]=0,
    [PPAG_PTDL2]=0,
#if PTD_LEVELS>2
    [PPAG_PTDL3]=0,
#endif
#endif
};

__hypercall xm_s32_t UpdatePage32Sys(xmAddress_t pAddr, xm_u32_t val) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;
    xm_u32_t addr;

    ASSERT(!HwIsSti());
    if (pAddr&3)
        return XM_INVALID_PARAM;

    if (!(page=PmmFindPage(pAddr, GetPartition(sched->cKThread), 0))) {
	return XM_INVALID_PARAM;
    }

    if (UpdatePPag32HndlTab[page->type])
	if (UpdatePPag32HndlTab[page->type](page, pAddr, &val)<0)
	    return XM_INVALID_PARAM;

    addr=(xmAddress_t)VCacheMapPage(pAddr, page);
    WriteByPassMmuWord((void *)addr, val);
    VCacheUnlockPage(page);
    //FlushTlb();
    return XM_OK;
}

__hypercall xm_s32_t SetPageTypeSys(xmAddress_t pAddr, xm_u32_t type) {
    localSched_t *sched=GET_LOCAL_SCHED();
    struct physPage *page;

    ASSERT(!HwIsSti());
    if (type>=NR_PPAG)
	return XM_INVALID_PARAM;

    if (!(page=PmmFindPage(pAddr, GetPartition(sched->cKThread), 0)))
	return XM_INVALID_PARAM;
    
    if (type!=page->type) {
        if (page->counter) {
#ifdef CONFIG_AUDIT_EVENTS
            xmWord_t auditArgs[2];
            auditArgs[0]=pAddr;
            auditArgs[1]=page->counter;
            RaiseAuditEvent(TRACE_VMM_MODULE, AUDIT_VMM_PPG_CNT, 2, auditArgs);
#endif
            return XM_OP_NOT_ALLOWED;
        }
        if (UnsetPPagTypeHndlTab[page->type])
            UnsetPPagTypeHndlTab[page->type](pAddr, page, type);
        
        if (SetPPagTypeHndlTab[type])
            SetPPagTypeHndlTab[type](pAddr, page, type);
        
        page->type=type;
        return XM_OK;
    }

    return XM_NO_ACTION;
}

__hypercall xm_s32_t InvldTlbSys(xmWord_t val) {
    if (val==-1)
	FlushTlb();
    else
	FlushTlbEntry(val);

    return XM_OK;
}
