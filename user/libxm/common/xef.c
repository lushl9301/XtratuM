/*
 * $FILE: xef.c
 *
 * XEF
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xef.h>
#include <xm_inc/digest.h>
#include <xm_inc/compress.h>
#include <xm_inc/xmef.h>

#include <endianess.h>

#undef OFFSETOF
#ifdef __compiler_offsetof
#define OFFSETOF(_type, _member) __compiler_offsetof(_type,_member)
#else
#ifdef HOST
#define OFFSETOF(_type, _member) (unsigned long)(&((_type *)0)->_member)
#else
#define OFFSETOF(_type, _member) PTR2ADDR(&((_type *)0)->_member)
#endif
#endif

extern void *memcpy(void *dest, const void *src, unsigned long n);

void InitXefParser() {
    
}

xm_s32_t ParseXefFile(xm_u8_t *img, struct xefFile *xefFile) {
    xm_u8_t digest[XM_DIGEST_BYTES];
    struct digestCtx digestState;
    xm_s32_t e;

    xefFile->hdr=(struct xefHdr *)img;
    if (RWORD(xefFile->hdr->signature)!=XEF_SIGNATURE)
	return XEF_BAD_SIGNATURE;

    if ((XM_GET_VERSION(RWORD(xefFile->hdr->version))!=XEF_VERSION)&&
	(XM_GET_SUBVERSION(RWORD(xefFile->hdr->version))!=XEF_SUBVERSION))
	return XEF_INCORRECT_VERSION;

    xefFile->segmentTab=(struct xefSegment *)(img+RWORD(xefFile->hdr->segmentTabOffset));
    xefFile->customFileTab=(struct xefCustomFile *)(img+RWORD(xefFile->hdr->customFileTabOffset));
#ifdef CONFIG_IA32
    xefFile->relTab=(struct xefRel *)(img+RWORD(xefFile->hdr->relOffset));
    xefFile->relaTab=(struct xefRela *)(img+RWORD(xefFile->hdr->relaOffset));
#endif
    xefFile->image=img+RWORD(xefFile->hdr->imageOffset);
    if (RWORD(xefFile->hdr->flags)&XEF_DIGEST) {
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    digest[e]=0;
	DigestInit(&digestState);
	DigestUpdate(&digestState, img, OFFSETOF(struct xefHdr, digest));
	DigestUpdate(&digestState, (xm_u8_t *)digest, XM_DIGEST_BYTES);
	DigestUpdate(&digestState, &img[(xm_u32_t)OFFSETOF(struct xefHdr, payload)], RWORD(xefFile->hdr->fileSize)-(xm_u32_t)OFFSETOF(struct xefHdr, payload));
	DigestFinal(digest, &digestState);

	for (e=0; e<XM_DIGEST_BYTES; e++) {
	    if (digest[e]!=xefFile->hdr->digest[e])
		return XEF_UNMATCHING_DIGEST;	
	}
    }

    return XEF_OK;
}

#ifndef HOST
static xm_s32_t URead(void *b, xmSize_t s, void *d) {
    memcpy(b, *(xm_u8_t **)d, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static xm_s32_t UWrite(void *b, xmSize_t s, void *d) {
    memcpy(*(xm_u8_t **)d, b, s);
    *(xm_u8_t **)d+=s;
    return s;
}

void *LoadXefFile(struct xefFile *xefFile, int (*VAddr2PAddr)(void *, xm_s32_t, xmAddress_t, xmAddress_t *), void *mAreas, xm_s32_t noAreas) {
    xm_s32_t e;
    xmAddress_t addr;
    if (xefFile->hdr->noSegments<=0)
        return 0;

    for (e=0; e<xefFile->hdr->noSegments; e++) {
	if (xefFile->hdr->flags&XEF_COMPRESSED) {
            xm_u8_t *rPtr, *wPtr;
            addr=xefFile->segmentTab[e].physAddr;
            if (VAddr2PAddr)
                VAddr2PAddr(mAreas, noAreas, addr, &addr);
            rPtr=(xm_u8_t *)&xefFile->image[xefFile->segmentTab[e].offset];
            wPtr=(xm_u8_t *)ADDR2PTR(addr);//xefFile->segmentTab[e].physAddr);
	    if (Uncompress(xefFile->segmentTab[e].deflatedFileSize, xefFile->segmentTab[e].fileSize, URead, &rPtr, UWrite, &wPtr)<0)
		return 0;
	} else {
            addr=xefFile->segmentTab[e].physAddr;
            if (VAddr2PAddr)
                VAddr2PAddr(mAreas, noAreas, addr, &addr);
	    memcpy(ADDR2PTR(addr),//xefFile->segmentTab[e].physAddr), 
                   &xefFile->image[xefFile->segmentTab[e].offset], xefFile->segmentTab[e].fileSize);
        }
    }

    addr=xefFile->hdr->xmImageHdr;
    if (VAddr2PAddr)
        VAddr2PAddr(mAreas, noAreas, addr, &addr);
    return ADDR2PTR(addr);
}

void *LoadXefCustomFile(struct xefFile *xefCustomFile, struct xefCustomFile *customFile) {
    xm_s32_t e;

    if (xefCustomFile->hdr->noSegments<=0)
        return 0;

    for (e=0; e<xefCustomFile->hdr->noSegments; e++) {
	if (xefCustomFile->hdr->flags&XEF_COMPRESSED) {
            xm_u8_t *rPtr, *wPtr;
            rPtr=(xm_u8_t *)&xefCustomFile->image[xefCustomFile->segmentTab[e].offset];
            wPtr=(xm_u8_t *)ADDR2PTR(customFile->sAddr);
	    if (Uncompress(xefCustomFile->segmentTab[e].deflatedFileSize, xefCustomFile->segmentTab[e].fileSize, URead, &rPtr, UWrite, &wPtr)<0)
		return 0;
	} else {
	    memcpy(ADDR2PTR(customFile->sAddr), &xefCustomFile->image[xefCustomFile->segmentTab[e].offset], xefCustomFile->segmentTab[e].fileSize);
        }
    }
  
    return ADDR2PTR(customFile->sAddr);
}
#endif

