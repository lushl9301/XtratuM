/*
 * $FILE: config.h
 *
 * Config file
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_CONFIG_H_
#define _XM_CONFIG_H_

#include __XM_INCFLD(autoconf.h)

#ifdef ASM
#define __ASSEMBLY__
#endif

// bits: (31..24)(23..16)(15..8)(7..0)
// Reserved.VERSION.SUBVERSION.REVISION
#define XM_VERSION (((CONFIG_XM_VERSION&0xFF)<<16)|((CONFIG_XM_SUBVERSION&0xFF)<<8)|(CONFIG_XM_REVISION&0xFF))

#define CONFIG_KSTACK_SIZE (CONFIG_KSTACK_KB*1024)

#define CONFIG_MAX_NO_CUSTOMFILES 3

#ifndef CONFIG_NO_CPUS
#define CONFIG_NO_CPUS 1
#endif

#if (CONFIG_ID_STRING_LENGTH&3)
#error CONFIG_ID_STRING_LENGTH must be a power of 4 (log2(32))
#endif

#ifdef CONFIG_x86
#if (CONFIG_XM_LOAD_ADDR&0x3FFFFF)
#error XtratuM must be aligned to a 4MB boundary for a x86 target
#endif
#endif

#if !defined(CONFIG_HWIRQ_PRIO_FBS)&&!defined(CONFIG_HWIRQ_PRIO_LBS)
#error "Interrupt priority order must be defined"
#endif

#ifdef __BASE_FILE__
#define __XM_FILE__ __BASE_FILE__
#else
#define __XM_FILE__ __FILE__
#endif

#endif
