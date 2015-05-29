/*
 * $FILE: rsvmem.h
 *
 * Memory for structure initialisation
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_RSVMEM_H_
#define _XM_RSVMEM_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <stdc.h>
#include <arch/xm_def.h>

extern void InitRsvMem(void);
extern void *AllocRsvMem(xm_u32_t size, xm_u32_t align);
extern void ReleaseRsvMemByOwner(xm_s32_t owner);
#ifdef CONFIG_DEBUG
extern void RsvMemDebug(void);
#endif

#define GET_MEMA(c, s, a) do { \
    if (s) { \
        if (!(c=AllocRsvMem(s, a))) { \
            cpuCtxt_t _ctxt; \
            GetCpuCtxt(&_ctxt); \
            SystemPanic(&_ctxt, __XM_FILE__":%u: memory pool exhausted", __LINE__); \
        } \
    } else c=0;\
} while(0)

#define GET_MEM(c, s) GET_MEMA(c, s, ALIGNMENT)

#define GET_MEMAZ(c, s, a) do {\
    if (s) { \
        if (!(c=AllocRsvMem(s, a))) { \
            cpuCtxt_t _ctxt; \
            GetCpuCtxt(&_ctxt); \
            SystemPanic(&_ctxt, __XM_FILE__":%u: memory pool exhausted", __LINE__); \
        } \
        memset(c, 0, s); \
    } else c=0; \
} while(0)

#define GET_MEMZ(c, s) GET_MEMAZ(c, s, ALIGNMENT)

#endif
