/*
 * $FILE: hypercalls.c
 *
 * XM system calls definitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xmhypercalls.h>
#include <xm_inc/hypercalls.h>
#include <hypervisor.h>

xm_lazy_hcall2(update_page32, xmAddress_t, pAddr, xm_u32_t, val);
xm_lazy_hcall2(set_page_type, xmAddress_t, pAddr, xm_u32_t, type);



