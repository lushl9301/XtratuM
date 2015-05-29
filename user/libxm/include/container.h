/*
 * $FILE: container.h
 *
 * Container definition
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _LIB_XM_CONTAINER_H_
#define _LIB_XM_CONTAINER_H_

#include <xm_inc/config.h>

struct xefContainerFile {
    struct xmefContainerHdr *hdr;
    struct xmefFile *fileTab;
    struct xmefPartition *partitionTab;
    xm_u8_t *image;
};

#define CONTAINER_OK 0
#define CONTAINER_BAD_SIGNATURE -1
#define CONTAINER_INCORRECT_VERSION -2
#define CONTAINER_UNMATCHING_DIGEST -3

extern xm_s32_t ParseXefContainer(xm_u8_t *img, struct xefContainerFile *pack);

#endif
