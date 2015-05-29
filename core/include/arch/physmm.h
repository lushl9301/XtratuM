/*
 * $FILE: physmm.h
 *
 * Physical memory manager
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_PHYSMM_H_
#define _XM_ARCH_PHYSMM_H_

#define LOW_MEMORY_START_ADDR   0x0
#define LOW_MEMORY_END_ADDR     0x100000

extern xm_u8_t ReadByPassMmuByte(void *pAddr);
extern xm_u32_t ReadByPassMmuWord(void *pAddr);
extern void WriteByPassMmuWord(void *pAddr, xm_u32_t val);

#endif
