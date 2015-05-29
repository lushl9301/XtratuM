/*
 * $FILE: spinlock.h
 *
 * Spin locks related stuffs
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 */

#ifndef _XM_SPINLOCK_H_
#define _XM_SPINLOCK_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/spinlock.h>
#include <arch/smp.h>

extern xm_u16_t __nrCpus;

typedef struct {
    archSpinLock_t archLock;
} spinLock_t;

typedef struct {
    volatile xm_s32_t v;
} barrier_t;

typedef struct {
    archBarrierMask_t bm;
} barrierMask_t;

#define SPINLOCK_INIT (spinLock_t){.archLock=__ARCH_SPINLOCK_UNLOCKED,}
#define BARRIER_INIT (barrier_t){.v=0,}
#define BARRIER_MASK_INIT (barrierMask_t){.bm=__ARCH_BARRIER_MASK_INIT,}

static inline void BarrierWriteMask(barrierMask_t *m){
    __ArchBarrierWriteMask(&m->bm,1<<GET_CPU_ID());
}

static inline int BarrierCheckMask(barrierMask_t *m){
    return __ArchBarrierCheckMask(&m->bm,(1<<__nrCpus)-1);
}

static inline void BarrierWaitMask(barrierMask_t *m){
   while(BarrierCheckMask(m));
}

static inline void BarrierWait(barrier_t *b) {
    while(b->v);
}

static inline void BarrierLock(barrier_t *b) {
    b->v=1;
}

static inline void BarrierUnlock(barrier_t *b) {
    b->v=0;
}

static inline void SpinLock(spinLock_t *s) {
    __ArchSpinLock(&s->archLock);
}

static inline void SpinUnlock(spinLock_t *s) {
    __ArchSpinUnlock(&s->archLock);
}

static inline xm_s32_t SpinIsLocked(spinLock_t *s) {
    return(xm_s32_t)__ArchSpinIsLocked(&s->archLock);
}

#define SpinLockIrqSave(s, flags) do {		\
    HwSaveFlagsCli(flags);			\
    SpinLock(s);				\
} while(0)

#define SpinUnlockIrqRestore(s, flags) do {	\
    SpinUnlock(s);				\
    HwRestoreFlags(flags);			\
} while(0)

static inline xm_s32_t SpinTryLock(spinLock_t *s) {
    return __ArchSpinTryLock(&s->archLock);
}

#endif
