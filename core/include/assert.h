/*
 * $FILE: assert.h
 *
 * Assert definition
 *
 * $VERSION$
 * 
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ASSERT_H_
#define _XM_ASSERT_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <processor.h>
#include <spinlock.h>

#ifdef CONFIG_DEBUG

#define ASSERT(exp) do { \
    if (!(exp)) { \
        cpuCtxt_t _ctxt; \
        GetCpuCtxt(&_ctxt); \
        SystemPanic(&_ctxt, __XM_FILE__":%u: failed assertion `"#exp"'\n", __LINE__); \
    } \
    ((void)0); \
} while(0)

#define ASSERT_LOCK(exp, lock) do { \
    if (!(exp)) { \
        cpuCtxt_t _ctxt; \
        SpinUnlock((lock)); \
        GetCpuCtxt(&_ctxt); \
        SystemPanic(&_ctxt, __XM_FILE__":%u: failed assertion `"#exp"'\n", __LINE__); \
    } \
    ((void)0); \
} while(0)

#else

#define ASSERT(exp) ((void)0)
#define ASSERT_LOCK(exp, sLock) ((void)0)
#endif

#endif
