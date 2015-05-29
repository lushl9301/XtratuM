/*
 * $FILE: physmm.c
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

#include <physmm.h>
#include <arch/physmm.h>

xm_u8_t ReadByPassMmuByte(void *pAddr) {
    struct physPage *page;
    xm_u8_t *vAddr, val;

    if ((xmAddress_t)pAddr > CONFIG_XM_OFFSET)
        return *(xm_u8_t *)pAddr;
    page = PmmFindAnonymousPage((xmAddress_t)pAddr);
    vAddr = VCacheMapPage((xmAddress_t)pAddr, page);
    val = *vAddr;
    VCacheUnlockPage(page);

    return val;
}

xm_u32_t ReadByPassMmuWord(void *pAddr) {
    struct physPage *page;
    xm_u32_t *vAddr, val;

    if ((xmAddress_t)pAddr > CONFIG_XM_OFFSET)
        return *(xm_u32_t *)pAddr;
    page = PmmFindAnonymousPage((xmAddress_t)pAddr);
    vAddr = VCacheMapPage((xmAddress_t)pAddr, page);
    val = *vAddr;
    VCacheUnlockPage(page);

    return val;
}

void WriteByPassMmuWord(void *pAddr, xm_u32_t val) {
    struct physPage *page;
    xm_u32_t *vAddr;

    if ((xmAddress_t)pAddr > CONFIG_XM_OFFSET) {
        *(xm_u32_t *)pAddr = val;
    } else {
        page = PmmFindAnonymousPage((xmAddress_t)pAddr);
        vAddr = VCacheMapPage((xmAddress_t)pAddr, page);
        *vAddr = val;
        VCacheUnlockPage(page);
    }
}

xmAddress_t EnableByPassMmu(xmAddress_t addr, partition_t *p, struct physPage **page) {
    *page=PmmFindPage(addr, p, 0);
    addr=(xmAddress_t)VCacheMapPage(addr, *page);

    return addr;
}

inline void DisableByPassMmu(struct physPage *page) {
    VCacheUnlockPage(page);
}
