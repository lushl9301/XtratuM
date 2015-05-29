/*
 * $FILE: checksum.h
 *
 * checksum algorithm
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_CHECKSUM_H_
#define _XM_CHECKSUM_H_

#ifdef _XM_KERNEL_
#include <assert.h>
#define RHALF(x) x
#else
#include <endianess.h>
#define ASSERT(a)
#endif

static inline xm_u16_t CalcCheckSum(xm_u16_t *buffer, xm_s32_t size) {
    xm_u16_t sum=0;
    ASSERT(!(size&0x1));
    size>>=1;
    while (size--) {
        sum+=*buffer++;
        if (sum&0xffff0000) {
            sum&=0xffff;
            sum++;
        }
    }
    return ~(sum&0xffff);
}

static inline xm_s32_t IsValidCheckSum(xm_u16_t *buffer, xm_s32_t size) {
    xm_u16_t sum=0;
    ASSERT(!(size&0x1));
    size>>=1;
    while (size--) {
        sum+=RHALF(*buffer);
        buffer++;
        if (sum&0xffff0000) {
            sum&=0xffff;
            sum++;
        }
    }
    return (sum==0xffff)?1:0;
}

#endif
