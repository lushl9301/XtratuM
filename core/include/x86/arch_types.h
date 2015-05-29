/*
 * $FILE: arch_types.h
 *
 * Types defined by the architecture
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_TYPES_H_
#define _XM_ARCH_TYPES_H_

#ifndef __ASSEMBLY__
/*  <track id="x86-basic-types"> */
// Basic types
typedef unsigned char xm_u8_t;
#define MAX_U8 0xFF
typedef char xm_s8_t;
#define MAX_S8 0x7F
typedef unsigned short xm_u16_t;
#define MAX_U16 0xFFFF
typedef short xm_s16_t;
#define MAX_S16 0x7FFF
typedef unsigned int xm_u32_t;
#define MAX_U32 0xFFFFFFFF
typedef int xm_s32_t;
#define MAX_S32 0x7FFFFFFF
typedef unsigned long long xm_u64_t;
#define MAX_U64 0xFFFFFFFFFFFFFFFFULL
typedef long long xm_s64_t;
#define MAX_S64 0x7FFFFFFFFFFFFFFFLL
/*  </track id="x86-basic-types"> */

/*  <track id="x86-extended-types"> */
// Extended types
#define XM_LOG2_WORD_SZ 5
typedef xm_s64_t xmTime_t;
#define MAX_XMTIME MAX_S64
typedef long xmLong_t;
typedef xm_u32_t xmWord_t;
typedef xm_u32_t xmId_t;
typedef xm_u32_t xmAddress_t;
typedef xm_u16_t xmIoAddress_t;
typedef xm_u32_t xmSize_t;
typedef xm_s32_t xmSSize_t;
/*  </track id="x86-extended-types"> */

#define PRNT_ADDR_FMT "l"
#define PTR2ADDR(x) ((xmWord_t)x)
#define ADDR2PTR(x) ((void *)((xmWord_t)x))

#ifdef _XM_KERNEL_

// Extended internal types
typedef xm_s64_t hwTime_t;

#endif /*_XM_KERNEL_*/

#endif /*__ASSEMBLY__*/
#endif /*_XM_ARCH_TYPES_H_*/
