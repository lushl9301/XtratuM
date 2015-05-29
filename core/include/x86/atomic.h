/*
 * $FILE: atomic.h
 *
 * atomic operations
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_ATOMIC_H_
#define _XM_ARCH_ATOMIC_H_

typedef struct { 
    volatile xm_u32_t val; 
} xmAtomic_t;

//#ifdef _XM_KERNEL_

#define XMAtomicSet(v,i) (((v)->val)=(i))

#define XMAtomicGet(v) ((v)->val)

#define XMAtomicClearMask(mask, addr) \
  __asm__ __volatile__("andl %0,%1" \
  : : "r" (~(mask)),"m" (*addr) : "memory")

#define XMAtomicSetMask(mask, addr) \
  __asm__ __volatile__("orl %0,%1" \
  : : "r" (mask),"m" (*(addr)) : "memory")

static inline void XMAtomicInc(xmAtomic_t *v) {
  __asm__ __volatile__("incl %0" :"+m" (v->val));
}

static inline void XMAtomicDec(xmAtomic_t *v) {
  __asm__ __volatile__("decl %0":"+m" (v->val));
}

static inline xm_s32_t XMAtomicDecAndTest(xmAtomic_t *v) {
  xm_u8_t c;
  
  __asm__ __volatile__("decl %0; sete %1"
		       :"+m" (v->val), "=qm" (c)
		       : : "memory");
  return c!=0;
}

//#endif
#endif
