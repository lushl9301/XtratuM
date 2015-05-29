/*
 * $FILE: xmhypercalls.h
 *
 * Arch hypercalls definition
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _ARCH_LIB_XM_HYPERCALLS_H_
#define _ARCH_LIB_XM_HYPERCALLS_H_

#ifdef _XM_KERNEL_
#error Guest file, do not include.
#endif

#define __STR(x) #x
#define TO_STR(x) __STR(x)
#define ASMLINK
#ifndef __ASSEMBLY__

/* <track id="DOC_HYPERCALL_MECHANISM">   */
#define __DO_XMHC "ta "TO_STR(XM_HYPERCALL_TRAP)"\n\t"
/* </track id="DOC_HYPERCALL_MECHANISM">   */

#define __DO_XMAHC "ta "TO_STR(XM_ASMHYPERCALL_TRAP)"\n\t"

#define _XM_HCALL0(_hc_nr, _r) \
    __asm__ __volatile__ ("mov %0, %%o0\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")

#define _XM_HCALL1(a0, _hc_nr, _r) \
    __asm__ __volatile__ ("mov %0, %%o0\n\t" \
			  "mov %2, %%o1\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr), "r" (a0) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")


#define _XM_HCALL2(a0, a1, _hc_nr, _r) \
    __asm__ __volatile__ ("mov %0, %%o0\n\t" \
			  "mov %2, %%o1\n\t" \
			  "mov %3, %%o2\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr), "r" (a0), "r" (a1) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")




#define _XM_HCALL3(a0, a1, a2, _hc_nr, _r) \
    __asm__ __volatile__ ("mov %0, %%o0\n\t"	\
			  "mov %2, %%o1\n\t"	\
			  "mov %3, %%o2\n\t" \
			  "mov %4, %%o3\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr), "r" (a0), "r" (a1), "r" (a2) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")

#define _XM_HCALL4(a0, a1, a2, a3, _hc_nr, _r)	\
    __asm__ __volatile__ ("mov %0, %%o0\n\t"	\
			  "mov %2, %%o1\n\t"	\
			  "mov %3, %%o2\n\t" \
			  "mov %4, %%o3\n\t" \
                          "mov %5, %%o4\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr), "r" (a0), "r" (a1), "r" (a2), "r" (a3) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")

#define _XM_HCALL5(a0, a1, a2, a3, a4, _hc_nr, _r)	\
    __asm__ __volatile__ ("mov %0, %%o0\n\t"	\
			  "mov %2, %%o1\n\t"	\
			  "mov %3, %%o2\n\t" \
			  "mov %4, %%o3\n\t" \
                          "mov %5, %%o4\n\t" \
                          "mov %6, %%o5\n\t" \
			  __DO_XMHC \
			  "mov %%o0, %0\n\t" : "=r" (_r) : "0" (_hc_nr), "r" (a0), "r" (a1), "r" (a2), "r" (a3), "r" (a4) : \
			  "o0", "o1", "o2", "o3", "o4", "o5",       "o7")

#define xm_hcall0(_hc) \
ASMLINK void XM_##_hc(void) { \
    xm_s32_t _r ; \
    _XM_HCALL0(_hc##_nr, _r); \
}

#define xm_hcall0r(_hc) \
ASMLINK xm_s32_t XM_##_hc(void) { \
    xm_s32_t _r ; \
    _XM_HCALL0(_hc##_nr, _r); \
    return _r; \
}

#define xm_hcall1(_hc, _t0, _a0) \
ASMLINK void XM_##_hc(_t0 _a0) { \
    xm_s32_t _r ; \
    _XM_HCALL1(_a0, _hc##_nr, _r); \
}

#define xm_hcall1r(_hc, _t0, _a0) \
ASMLINK xm_s32_t XM_##_hc(_t0 _a0) { \
    xm_s32_t _r ; \
    _XM_HCALL1(_a0, _hc##_nr, _r); \
    return _r; \
}

#define xm_hcall2(_hc, _t0, _a0, _t1, _a1) \
ASMLINK void XM_##_hc(_t0 _a0, _t1 _a1) { \
    xm_s32_t _r ; \
    _XM_HCALL2(_a0, _a1, _hc##_nr, _r);	\
}

#define xm_hcall2r(_hc, _t0, _a0, _t1, _a1) \
ASMLINK xm_s32_t XM_##_hc(_t0 _a0, _t1 _a1)  { \
    xm_s32_t _r ; \
    _XM_HCALL2(_a0, _a1, _hc##_nr, _r); \
    return _r; \
}

#define xm_hcall3(_hc, _t0, _a0, _t1, _a1, _t2, _a2) \
ASMLINK void XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2) { \
    xm_s32_t _r ; \
    _XM_HCALL3(_a0, _a1, _a2, _hc##_nr, _r); \
}

#define xm_hcall3r(_hc, _t0, _a0, _t1, _a1, _t2, _a2) \
ASMLINK xm_s32_t XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2) { \
    xm_s32_t _r ; \
    _XM_HCALL3(_a0, _a1, _a2, _hc##_nr, _r); \
    return _r; \
}

#define xm_hcall4(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3) \
ASMLINK void XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3) { \
    xm_s32_t _r ; \
    _XM_HCALL4(_a0, _a1, _a2, _a3, _hc##_nr, _r); \
}

#define xm_hcall4r(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3) \
ASMLINK xm_s32_t XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3) { \
    xm_s32_t _r ; \
    _XM_HCALL4(_a0, _a1, _a2, _a3, _hc##_nr, _r); \
    return _r; \
}

#define xm_hcall5(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3, _t4, _a4) \
ASMLINK void XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3, _t4 _a4) { \
    xm_s32_t _r ; \
    _XM_HCALL5(_a0, _a1, _a2, _a3, _a4, _hc##_nr, _r);	\
}

#define xm_hcall5r(_hc, _t0, _a0, _t1, _a1, _t2, _a2, _t3, _a3, _t4, _a4) \
ASMLINK xm_s32_t XM_##_hc(_t0 _a0, _t1 _a1, _t2 _a2, _t3 _a3, _t4 _a4) { \
    xm_s32_t _r ; \
    _XM_HCALL5(_a0, _a1, _a2, _a3, _a4, _hc##_nr, _r);	\
    return _r; \
}

static inline void XM_sparc_flush_regwin(void) {
    __asm__ __volatile__("mov "TO_STR(sparc_flush_regwin_nr)", %%o0\n\t" \
			 __DO_XMAHC:::"o0");
}

static inline xm_u32_t XM_sparc_get_psr(void) {
    xm_u32_t ret;
    __asm__ __volatile__("mov "TO_STR(sparc_get_psr_nr)", %%o0\n\t" \
			 __DO_XMAHC \
			 "mov %%o0, %0\n\t" : "=r" (ret): : "o0");
    return ret;
}

static inline void XM_sparc_set_psr(xm_u32_t flags) {
    __asm__ __volatile__("mov "TO_STR(sparc_set_psr_nr)", %%o0\n\t" \
			 "mov %0, %%o1\n\t" \
			 __DO_XMAHC :: "r"(flags) : "o0", "o1");
}

static inline void XM_sparc_set_pil(void) {
    __asm__ __volatile__("mov "TO_STR(sparc_set_pil_nr)", %%o0\n\t" \
			 __DO_XMAHC::: "o0");  \
}

static inline void XM_sparc_clear_pil(void) {
    __asm__ __volatile__("mov "TO_STR(sparc_clear_pil_nr)", %%o0\n\t" \
                         __DO_XMAHC::: "o0"); \
}

extern xm_s32_t XM_sparc_atomic_and(xm_u32_t *, xm_u32_t);
extern xm_s32_t XM_sparc_atomic_or(xm_u32_t *, xm_u32_t);
extern xm_s32_t XM_sparc_atomic_add(xm_u32_t *, xm_u32_t);
extern xm_s32_t XM_sparc_inport(xm_u32_t, xm_u32_t *);
extern xm_s32_t XM_sparc_outport(xm_u32_t, xm_u32_t);

#else

/* <track id="DOC_HYPERCALL_MECHANISM">  */
#define __XM_HC ta XM_HYPERCALL_TRAP ; nop
/* </track id="DOC_HYPERCALL_MECHANISM">   */
#define __XM_AHC ta XM_ASMHYPERCALL_TRAP ; nop

#endif

#endif
