/*
 * $FILE: xef.h
 *
 * XM's executable format helper functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_XEF_H_
#define _LIB_XM_XEF_H_

#include <xm_inc/xmef.h>

#define XEF_OK 0
#define XEF_BAD_SIGNATURE -1
#define XEF_INCORRECT_VERSION -2
#define XEF_UNMATCHING_DIGEST -3

struct xefFile {
    struct xefHdr *hdr;
    struct xefSegment *segmentTab;
#if 0
#ifdef CONFIG_IA32
    struct xefRel *relTab;
    struct xefRela *relaTab;
#endif
#endif
    struct xefCustomFile *customFileTab;
    xm_u8_t *image;
};

extern xm_s32_t ParseXefFile(xm_u8_t *img, struct xefFile *xefFile);

extern void *LoadXefFile(struct xefFile *xefFile, int (*VAddr2PAddr)(void *, xm_s32_t, xmAddress_t, xmAddress_t *), void *mAreas, xm_s32_t noAreas);
extern void *LoadXefCustomFile(struct xefFile *xefCustomFile, struct xefCustomFile *customFile);

#endif
