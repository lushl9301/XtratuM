/*
 * $FILE: digest.h
 *
 * MD5 algorithm
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_MD5_H_
#define _XM_MD5_H_

/* Data structure for MD5 (Message Digest) computation */
struct digestCtx {
    xm_u8_t in[64];
    xm_u32_t buf[4];
    xm_u32_t bits[2];
};

extern void DigestInit(struct digestCtx *mdContext);
extern void DigestUpdate(struct digestCtx *ctx, const xm_u8_t *buf, xm_u32_t len);
extern void DigestFinal(xm_u8_t digest[16], struct digestCtx *ctx);

#endif
