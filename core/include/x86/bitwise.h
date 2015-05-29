/*
 * $FILE: bitwise.h
 *
 * Some basic bit operations
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_BITWISE_H_
#define _XM_ARCH_BITWISE_H_

#define ARCH_HAS_FFS

static __inline__ xm_s32_t _Ffs(xm_s32_t x) {
    xm_s32_t r;

    __asm__ __volatile__ ("bsfl %1,%0\n\t"
			  "jnz 1f\n\t"
			  "movl $-1,%0\n"
			  "1:" : "=r" (r) : "g" (x));
    return r;
}

#define ARCH_HAS_FFZ

static __inline__ xm_s32_t _Ffz(xm_s32_t x) {
    xm_s32_t r;

    __asm__ __volatile__ ("bsfl %1,%0\n\t"
			  "jnz 1f\n\t"
			  "movl $-1,%0\n"
			  "1:" : "=r" (r) : "g" (~x));
    return r;
}

#define ARCH_HAS_FLS

static __inline__ xm_s32_t _Fls(xm_s32_t x) {
    xm_s32_t r;

    __asm__ __volatile__ ("bsrl %1,%0\n\t"
			  "jnz 1f\n\t"
			  "movl $-1,%0\n"
			  "1:" : "=r" (r) : "g" (x));
    return r;
}

#define ARCH_HAS_SET_BIT

static inline void _set_bit(xm_s32_t nr, volatile xm_u32_t *addr) {
    __asm__ __volatile__ ("btsl %1,%0"
			  :"=m" ((*(volatile xm_s32_t *) addr))
			  :"Ir" (nr));
}

#define ARCH_HAS_CLEAR_BIT

static inline void _clear_bit(xm_s32_t nr, volatile xm_u32_t *addr) {
    __asm__ __volatile__ ("btrl %1,%0" :"=m" ((*(volatile xm_s32_t *) addr)):"Ir" (nr));
}

/*#define ARCH_HAS_CMPXCHG

struct __xchg_dummy { xm_u32_t a[100]; };
#define __xg(x) ((struct __xchg_dummy *)(x))

static inline xm_u32_t __cmpxchg(volatile void *ptr, xm_u32_t old,
                                      xm_u32_t new, xm_s32_t size) {
  xm_u32_t prev;
  switch (size) {
  case 1:
    __asm__ __volatile__("cmpxchgb %b1,%2"
			 : "=a"(prev)
			 : "q"(new), "m"(*__xg(ptr)), "0"(old)
			 : "memory");
    return prev;
  case 2:
    __asm__ __volatile__("cmpxchgw %w1,%2"
			 : "=a"(prev)
			 : "q"(new), "m"(*__xg(ptr)), "0"(old)
			 : "memory");
    return prev;
  case 4:
    __asm__ __volatile__("cmpxchgl %1,%2"
			 : "=a"(prev)
			 : "q"(new), "m"(*__xg(ptr)), "0"(old)
			 : "memory");
    return prev;
  }
  return old;
}
*/
#endif
