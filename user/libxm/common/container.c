/*
 * $FILE: container.c
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

#include <container.h>
#include <xm_inc/xmef.h>
#include <xm_inc/digest.h>

#define OFFSETOF(_type, _member) ((xmSize_t)PTR2ADDR(&((_type *)0)->_member))

xm_s32_t ParseXefContainer(xm_u8_t *img, struct xefContainerFile *c) {
    xm_u8_t digest[XM_DIGEST_BYTES];
    struct digestCtx digestState;
    xm_s32_t e;

    c->hdr=(struct xmefContainerHdr *)img;
    if (c->hdr->signature!=XM_PACKAGE_SIGNATURE)
	return CONTAINER_BAD_SIGNATURE;

    if ((XM_GET_VERSION(c->hdr->version)!=XMPACK_VERSION)&&
	(XM_GET_SUBVERSION(c->hdr->version)!=XMPACK_SUBVERSION))
	return CONTAINER_INCORRECT_VERSION;
    c->fileTab=(struct xmefFile *)(img+c->hdr->fileTabOffset);
    c->partitionTab=(struct xmefPartition *)(img+c->hdr->partitionTabOffset);
    c->image=img;

    if (c->hdr->flags&XMEF_CONTAINER_DIGEST) {
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    digest[e]=0;	
	DigestInit(&digestState);
	DigestUpdate(&digestState, img, OFFSETOF(struct xmefContainerHdr, digest));
	DigestUpdate(&digestState, digest, XM_DIGEST_BYTES);
	DigestUpdate(&digestState, &img[OFFSETOF(struct xmefContainerHdr, fileSize)], c->hdr->fileSize-OFFSETOF(struct xmefContainerHdr, fileSize));
	DigestFinal(digest, &digestState);
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    if (digest[e]!=c->hdr->digest[e])
		return CONTAINER_UNMATCHING_DIGEST;
    }

    return CONTAINER_OK;
}
