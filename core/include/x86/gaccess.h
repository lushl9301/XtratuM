/*
 * $FILE: gaccess.h
 *
 * Guest shared info
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_ARCH_GACCESS_H_
#define _XM_ARCH_GACCESS_H_

#ifndef _XM_KERNEL_
#error Kernel file, do not include.
#endif

#include <arch/xm_def.h>
#include <arch/asm.h>
/*
#define __ArchCheckGPhysAddr(__cfg, __addr, __size) ({ \
    xm_s32_t __e, __r=0; \
    xmAddress_t __a, __b, __c, __d;   \
    __c=(xmAddress_t)(__addr); \
    __d=__c+(xmAddress_t)(__size); \
    for (__e=0; __e<__cfg->noPhysicalMemoryAreas; __e++) { \
        __a=xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].startAddr; \
        __b=__a+xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].size; \
       if ((__c>=__a) &&(__b>__d)) \
            break; \
    } \
    if (__e>=__cfg->noPhysicalMemoryAreas) \
        __r=-1; \
    __r; \
})

#define __ArchCheckGMappedAtAddr(__cfg, __addr, __size) ({ \
    xm_s32_t __e, __r=0; \
    xmAddress_t __a, __b, __c, __d;   \
    __c=(xmAddress_t)(__addr); \
    __d=__c+(xmAddress_t)(__size); \
    for (__e=0; __e<__cfg->noPhysicalMemoryAreas; __e++) { \
        __a=xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].mappedAt; \
        __b=__a+xmcPhysMemAreaTab[__cfg->physicalMemoryAreasOffset+__e].size; \
       if ((__c>=__a) &&(__b>__d)) \
            break; \
    } \
    if (__e>=__cfg->noPhysicalMemoryAreas) \
        __r=-1; \
    __r; \
})
*/

#define __archGParam
#define __ArchCheckGParam(__param, __size, __align) ({ \
    xm_s32_t __r=-1; \
    if (!((__align - 1) & __param)) \
        if ((xmAddress_t)__param < CONFIG_XM_OFFSET) \
            if (CONFIG_XM_OFFSET-(xmAddress_t)(__param) >= size) \
                __r=0; \
    __r; \
)}


/*
#define __ArchCheckGParam(__ctxt, __param, __size) ({ \
    xm_s32_t __r=0; \
    if ((xmAddress_t)(__param)<(XM_VMAPEND+1)) \
        if (((xmAddress_t)(__param)+(__size))>=CONFIG_XM_OFFSET) __r=-1; \
    __r; \
})
*/
#endif
