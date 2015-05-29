/*
 * $FILE: ldr.c
 *
 * Partition loader code
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#undef _XM_KERNEL_
#include <xef.h>
#include <xm.h>

partitionControlTable_t *partCtrlTabPtr;

static int VAddr2PAddr(void *mAreas, xm_s32_t noAreas, xmAddress_t vAddr, xmAddress_t *pAddr) {
    *pAddr=vAddr;
    return 0;
}

xmAddress_t MainLdr(void) {
    struct xefFile xefFile,xefCustomFile;
    struct xmImageHdr *partHdr;
    xm_s32_t ret,i;

    xm_u8_t *img=(xm_u8_t *)partCtrlTabPtr->imgStart;
    if ((ret=ParseXefFile(img, &xefFile))!=XEF_OK)
        return 0;
    partHdr=LoadXefFile(&xefFile, VAddr2PAddr, 0, 0);

    img=(xm_u8_t *)((partCtrlTabPtr->imgStart+xefFile.hdr->fileSize)&(~(PAGE_SIZE-1)))+PAGE_SIZE;

    for (i=0; i<partHdr->noCustomFiles; i++){
            if ((ret=ParseXefFile((xm_u8_t *)img, &xefCustomFile))!=XEF_OK)
               return 0;
            LoadXefCustomFile(&xefCustomFile, &xefFile.customFileTab[i]);
    }

    return xefFile.hdr->entryPoint;
}

