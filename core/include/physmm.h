/*
 * $FILE: physmm.h
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

#ifndef _XM_PHYSMM_H_
#define _XM_PHYSMM_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <kthread.h>
#include <processor.h>
#include <hypercalls.h>
#include <arch/paging.h>
#include <arch/physmm.h>

#ifdef CONFIG_MMU

struct physPage {
    struct dynListNode listNode;
#ifndef CONFIG_ARCH_MMU_BYPASS
    xmAddress_t vAddr;
#endif
    xm_u32_t mapped:1, unlocked:1, type:3, counter:27;
    spinLock_t lock;
};

#else
struct physPage {
};
#endif

static inline void PPagIncCounter(struct physPage *page) {
    xm_u32_t cnt;
        
    SpinLock(&page->lock);
    cnt=page->counter;
    page->counter++;
    SpinUnlock(&page->lock);

    if (cnt==~0) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
        PartitionPanic(&ctxt, __XM_FILE__":%u: counter overflow", __LINE__);
    }
}

static inline void PPagDecCounter(struct physPage *page) {
    xm_u32_t cnt;
        
    SpinLock(&page->lock);
    cnt=page->counter;
    page->counter--;
    SpinUnlock(&page->lock);

    if (!cnt) {
        cpuCtxt_t ctxt;
        GetCpuCtxt(&ctxt);
        PartitionPanic(&ctxt, __XM_FILE__":%u: counter underflow", __LINE__);
    }
}

extern void SetupPhysMM(void);
extern struct physPage *PmmFindPage(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags);
extern struct physPage *PmmFindAnonymousPage(xmAddress_t pAddr);
extern xm_s32_t PmmFindAddr(xmAddress_t pAddr, partition_t *p, xm_u32_t *flags);
extern xm_s32_t PmmFindArea(xmAddress_t pAddr, xmSSize_t size, partition_t *k, xm_u32_t *flags);
extern void PmmResetPartition(partition_t *p);
extern void *VCacheMapPage(xmAddress_t pAddr, struct physPage *page);
extern void VCacheUnlockPage(struct physPage *page);
extern xmAddress_t EnableByPassMmu(xmAddress_t addr, partition_t * p, struct physPage **);
extern inline void DisableByPassMmu(struct physPage *);

#endif
