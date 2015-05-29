/*
 * $FILE: hdr.c
 *
 * XM header
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */


#include <xm.h>
#include <config.h>
#include <xm_inc/arch/paging.h>

static xm_u8_t _pageTable[PAGE_SIZE*CONFIG_SIZE_PAGE_TABLE] __attribute__((aligned(PAGE_SIZE))) __attribute__ ((section(".bss.noinit")));

struct xmImageHdr xmImageHdr __XMIHDR = {
    .sSignature=XMEF_PARTITION_MAGIC,
    .compilationXmAbiVersion=XM_SET_VERSION(XM_ABI_VERSION, XM_ABI_SUBVERSION, XM_ABI_REVISION),
    .compilationXmApiVersion=XM_SET_VERSION(XM_API_VERSION, XM_API_SUBVERSION, XM_API_REVISION),
    .pageTable=(xmAddress_t)_pageTable,
    .pageTableSize=CONFIG_SIZE_PAGE_TABLE*PAGE_SIZE,
    .noCustomFiles=0,
#if 0    
    .customFileTab={[0]=(struct xefCustomFile){
            .sAddr=(xmAddress_t)0x40105bc0,
            .size=0,
        },
    },
#endif
    .eSignature=XMEF_PARTITION_MAGIC,
};

