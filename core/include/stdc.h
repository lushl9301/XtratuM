/*
 * $FILE: stdc.h
 *
 * KLib's standard c functions definition
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_STDC_H_
#define _XM_STDC_H_

#ifdef _XM_KERNEL_
#include <linkage.h>

#ifndef __SCHAR_MAX__
#define __SCHAR_MAX__   127
#endif
#ifndef __SHRT_MAX__
#define __SHRT_MAX__    32767
#endif
#ifndef __INT_MAX__
#define __INT_MAX__     2147483647
#endif

#ifndef __LONG_MAX__
#define __LONG_MAX__    2147483647L
#endif

#define INT_MIN (-1 - INT_MAX)
#define INT_MAX (__INT_MAX__)
#define UINT_MAX (INT_MAX * 2U + 1U)

#define LONG_MIN (-1L - LONG_MAX)
#define LONG_MAX ((__LONG_MAX__) + 0L)
#define ULONG_MAX (LONG_MAX * 2UL + 1UL)

#define LLONG_MAX 9223372036854775807LL
#define LLONG_MIN (-LLONG_MAX - 1LL)

/* Maximum value an `unsigned long long int' can hold.  (Minimum is 0.)  */
#define ULLONG_MAX 18446744073709551615ULL

static inline xm_s32_t isdigit(xm_s32_t ch) {
    return (xm_u32_t)(ch - '0') < 10u;
}

static inline xm_s32_t isspace(xm_s32_t ch) {
    return (xm_u32_t)(ch - 9) < 5u  ||  ch == ' ';
}

static inline xm_s32_t isxdigit(xm_s32_t ch) {
    return (xm_u32_t)(ch - '0') < 10u  ||
	(xm_u32_t)((ch | 0x20) - 'a') <  6u;
}

static inline xm_s32_t isalnum (xm_s32_t ch) {
    return (xm_u32_t)((ch | 0x20) - 'a') < 26u  ||
	(xm_u32_t)(ch - '0') < 10u;
}

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v, l) __builtin_va_arg(v,l)

#undef NULL
#define NULL ((void *) 0)

#undef OFFSETOF
#ifdef __compiler_offsetof
#define OFFSETOF(_type, _member) __compiler_offsetof(_type,_member)
#else
#define OFFSETOF(_type, _member) ((xmSize_t) &((_type *)0)->_member)
#endif

#define EOF (-1)

extern xm_s32_t kprintf(const char *, ...);
extern xm_s32_t eprintf(const char *, ...);
extern xm_s32_t vprintf(const char *fmt, va_list args);
extern xm_s32_t sprintf(char *s, char const *fmt, ...);
extern xm_s32_t snprintf(char *s, xm_s32_t n, const char *fmt, ...);
extern void *memmove(void *, const void *, xmSize_t);
extern unsigned long strtoul(const char *, char **, xm_s32_t);
extern long strtol(const char *, char **, xm_s32_t);
extern xm_s64_t strtoll(const char *nPtr, char **endPtr, xm_s32_t base);
extern xm_u64_t strtoull(const char *ptr, char **endPtr, xm_s32_t base);
extern char *basename(char *path);
extern xm_s32_t memcmp(const void *, const void *, xmSize_t);
extern void *memcpy(void *, const void *, xmSize_t);
extern void *MemCpyPhys(void *dst, const void *src, xm_u32_t count);
extern void *memset(void *, xm_s32_t, xmSize_t);
extern char *strcat(char *, const char *);
extern char *strncat(char *s, const char *t, xmSize_t n);
extern char *strchr(const char *, xm_s32_t);
extern xm_s32_t strcmp(const char *, const char *);
extern xm_s32_t strncmp(const char *, const char *, xmSize_t);
extern char *strcpy(char *, const char *);
extern char *strncpy(char *dest, const char *src, xmSize_t n);
extern xmSize_t strlen(const char *);
extern char *strrchr(const char *, xm_s32_t);
extern char *strstr(const char *, const char *);

// Non-standard functions
typedef void (*WrMem_t)(xm_u32_t *, xm_u32_t);
typedef xm_u32_t (*RdMem_t)(xm_u32_t *);

static inline void UnalignMemCpy(xm_u8_t *dst, xm_u8_t *src, xmSSize_t size, RdMem_t SrcR, RdMem_t DstR, WrMem_t DstW) {
    xm_u32_t srcW, dstW;
    xm_s32_t c1, c2, e;    

    for (e=0, c1=(xm_u32_t)src&0x3, c2=(xm_u32_t)dst&0x3; e<size; 
         src++, dst++, c1=(c1+1)&0x3, c2=(c2+1)&0x3, e++) {
        srcW=SrcR((xm_u32_t *)((xm_u32_t)src&~0x3));
#ifdef CONFIG_TARGET_LITTLE_ENDIAN
        dstW=srcW&(0xff<<((c1&0x3)<<3));
        dstW>>=((c1&0x3)<<3);
        dstW<<=((c2&0x3)<<3);

        dstW|=(DstR((xm_u32_t *)((xm_u32_t)dst&~0x3))&~(0xff<<((c2&0x3)<<3)));
#else
        dstW=srcW&(0xff000000>>((c1&0x3)<<3));
        dstW<<=((c1&0x3)<<3);
        dstW>>=((c2&0x3)<<3);

        dstW|=(DstR((xm_u32_t *)((xm_u32_t)dst&~0x3))&~(0xff000000>>((c2&0x3)<<3)));
#endif
        DstW((xm_u32_t *)((xm_u32_t)dst&~0x3), dstW);
    }
}

#define FILL_TAB(x) [x]=#x
#define PWARN kprintf
#endif
#endif
