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

#ifndef _XM_ARCH_SPINLOCK_H_
#define _XM_ARCH_SPINLOCK_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/asm.h>

typedef struct {
    volatile xm_u32_t lock;
} archSpinLock_t;

typedef struct {
    volatile xm_u8_t mask;
} archBarrierMask_t;

#define __ARCH_SPINLOCK_UNLOCKED {1}
#define __ARCH_BARRIER_MASK_INIT {0}

static inline void __ArchBarrierWriteMask(archBarrierMask_t *bm, xm_u8_t bitMask) {
#ifdef CONFIG_SMP
    __asm__ __volatile__("lock; or %0,%1\n\t" : : "r" (bitMask), "m"(*(&bm->mask)) : "memory" );
#endif
}

static inline int __ArchBarrierCheckMask(archBarrierMask_t *bm, xm_u8_t mask) {
#ifdef CONFIG_SMP
   if (bm->mask==mask)
      return 0;
   return -1;
#else
   return 0;
#endif
}

static inline void __ArchSpinLock(archSpinLock_t *lock) {
#ifdef CONFIG_SMP
    __asm__ __volatile__("\n1:\t"					\
			 "cmpl $1, %0\n\t"				\
			 "je 2f\n\t"					\
			 "pause\n\t"\
			 "jmp 1b\n\t"\
			 "2:\t"						\
			 "mov $0, %%eax\n\t"					\
			 "xchg %%eax, %0\n\t"				\
			 "cmpl $1, %%eax\n\t"					\
			 "jne 1b\n\t"					\
			 "3:\n\t" : "+m" (lock->lock) : : "eax", "memory");
#else
    lock->lock=0;
#endif
}

static inline void __ArchSpinUnlock(archSpinLock_t *lock) {
#ifdef CONFIG_SMP
    xm_s8_t oldval=1;  
    __asm__ __volatile__("xchgb %b0, %1" : "=q" (oldval), "+m" (lock->lock) : "0" (oldval) : "memory");
#else
    lock->lock=1;
#endif
}

static inline xm_s32_t __ArchSpinTryLock(archSpinLock_t *lock) {
    xm_s8_t oldval;
#ifdef CONFIG_SMP
    __asm__ __volatile__ ("xchgb %b0,%1" :"=q" (oldval), "+m" (lock->lock) :"0" (0) : "memory");
#else
    oldval=lock->lock;
    lock->lock=1;
#endif
    return oldval>0;
}

#define __ArchSpinIsLocked(x) (*(volatile xm_s8_t *)(&(x)->lock)<=0)

#define HwSaveFlagsCli(flags) { \
    HwSaveFlags(flags); \
    HwCli();  \
}

static inline xm_s32_t HwIsSti(void) {
    xmWord_t flags;
    HwSaveFlags(flags);
    return (flags&0x200);
}

#endif

