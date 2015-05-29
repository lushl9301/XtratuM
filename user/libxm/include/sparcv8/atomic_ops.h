/*
 * $FILE: atomic_ops.h
 *
 * Atomic operations (architecture dependent)
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _ATOMIC_OPS_H_
#define _ATOMIC_OPS_H_

#ifndef __ASSEMBLY__
#include <xm_inc/arch/atomic.h>

static inline xm_u32_t XMAtomicGet(xmAtomic_t *v) {
    return v->val;
}

static inline void XMAtomicClearMask(xm_u32_t mask, xmAtomic_t *v) {
    XM_sparc_atomic_and((xm_u32_t *)&(v->val), ~mask);
}

static inline void XMAtomicSetMask(xm_u32_t mask, xmAtomic_t *v) {
    XM_sparc_atomic_or((xm_u32_t *)&v->val, mask);
}
#endif

#endif
