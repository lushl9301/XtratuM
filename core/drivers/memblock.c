/*
 * $FILE: memblock.c
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <rsvmem.h>
#include <kdevice.h>
#include <spinlock.h>
#include <sched.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/physmm.h>
#include <drivers/memblock.h>

static kDevice_t *memBlockTab=0;
static struct memBlockData *memBlockData;

static xm_s32_t ResetMemBlock(const kDevice_t *kDev) {
    memBlockData[kDev->subId].pos=0;
    return 0;
}

static xm_s32_t ReadMemBlock(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
    xm_u8_t *ptr;
    xm_s32_t e;
    ASSERT(buffer);
    if (((xmSSize_t)xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size-(xmSSize_t)memBlockData[kDev->subId].pos-len)<0)
	len=xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size-memBlockData[kDev->subId].pos;
    
    if (len<=0) {
        return 0;
    }
    ptr=(xm_u8_t *)(memBlockData[kDev->subId].addr+memBlockData[kDev->subId].pos);
    
    for (e=0; e<len; e++, ptr++) {
#ifdef CONFIG_ARCH_MMU_BYPASS
        buffer[e]=ReadByPassMmuByte(ptr);
#else
#ifdef CONFIG_x86
        buffer[e]=*ptr;
#endif
#endif
        if (!(e&0x7f)) {
            PreemptionOn();
            PreemptionOff();
        }
    }
    //memcpy(buffer, (xm_u8_t *)(memBlockData[kDev->subId].vAddr+memBlockData[kDev->subId].pos), len);
    memBlockData[kDev->subId].pos+=len;
    return len;
}

static xm_s32_t WriteMemBlock(const kDevice_t *kDev, xm_u8_t *buffer, xmSSize_t len) {
#ifdef CONFIG_ARCH_MMU_BYPASS
    static xm_u32_t RdVMem(xm_u32_t *vAddr) {
        return *vAddr;
    }
#endif
    xm_u8_t *ptr;
    ASSERT(buffer);
    if (((xmSSize_t)xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size-(xmSSize_t)memBlockData[kDev->subId].pos-len)<0)
	len=xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size-memBlockData[kDev->subId].pos;
    
    if (len<=0)
        return 0;

    ptr=(xm_u8_t *)(memBlockData[kDev->subId].addr+memBlockData[kDev->subId].pos);

#ifdef CONFIG_ARCH_MMU_BYPASS
    UnalignMemCpy(ptr, buffer, len, RdVMem, (RdMem_t)ReadByPassMmuWord, (WrMem_t)WriteByPassMmuWord);
#else
#ifdef CONFIG_x86
    memcpy(ptr, buffer, len);
#endif
#endif
    //  memcpy((xm_u8_t *)(memBlockData[kDev->subId].vAddr+memBlockData[kDev->subId].pos), buffer, len);
    memBlockData[kDev->subId].pos+=len;
  
    return len;
}

static xm_s32_t SeekMemBlock(const kDevice_t *kDev, xm_u32_t offset, xm_u32_t whence) {
    xm_s32_t off=offset;

    switch((whence)) {
    case DEV_SEEK_START:	
	break;
    case DEV_SEEK_CURRENT:
	off+=memBlockData[kDev->subId].pos;
	break;
    case DEV_SEEK_END:
	off+=xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size;
	break;
    }
    if (off<0) off=0;
    if (off>xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size) 
	off=xmcPhysMemAreaTab[memBlockData[kDev->subId].cfg->physicalMemoryAreasOffset].size;
    memBlockData[kDev->subId].pos=off;

    return off;
}

static const kDevice_t *GetMemBlock(xm_u32_t subId) {  
    return &memBlockTab[subId];
}

xm_s32_t __VBOOT InitMemBlock(void) {
#if defined(CONFIG_MMU)&&!defined(CONFIG_ARCH_MMU_BYPASS)
    xm_s32_t i, noPages;
#endif
    xm_s32_t e;

    GET_MEMZ(memBlockTab, sizeof(kDevice_t)*xmcTab.deviceTab.noMemBlocks);
    GET_MEMZ(memBlockData, sizeof(struct memBlockData)*xmcTab.deviceTab.noMemBlocks);
    for (e=0; e<xmcTab.deviceTab.noMemBlocks; e++) {
        memBlockTab[e].subId=e;
        memBlockTab[e].Reset=ResetMemBlock;
        memBlockTab[e].Write=WriteMemBlock;
        memBlockTab[e].Read=ReadMemBlock;
        memBlockTab[e].Seek=SeekMemBlock;
	memBlockData[e].cfg=&xmcMemBlockTab[e];
#if defined(CONFIG_MMU)&&!defined(CONFIG_ARCH_MMU_BYPASS)
	// Mapping on virtual memory
	noPages=SIZE2PAGES(xmcPhysMemAreaTab[xmcMemBlockTab[e].physicalMemoryAreasOffset].size);
	if (!(memBlockData[e].addr=VmmAlloc(noPages))) {
            cpuCtxt_t ctxt;
            GetCpuCtxt(&ctxt);
	    SystemPanic(&ctxt, "[InitMemBlock] System is out of free frames\n");
        }
	for (i=0; i<(noPages*PAGE_SIZE); i+=PAGE_SIZE)
            VmMapPage(xmcPhysMemAreaTab[xmcMemBlockTab[e].physicalMemoryAreasOffset].startAddr+i, memBlockData[e].addr+i, _PG_ATTR_PRESENT|_PG_ATTR_RW);
#else
	memBlockData[e].addr=xmcPhysMemAreaTab[xmcMemBlockTab[e].physicalMemoryAreasOffset].startAddr;
#endif
    }

    GetKDevTab[XM_DEV_LOGSTORAGE_ID]=GetMemBlock;

    return 0;
}

#ifdef CONFIG_DEV_MEMBLOCK_MODULE
XM_MODULE("MemoryBlock", InitMemBlock, DRV_MODULE);
#else
REGISTER_KDEV_SETUP(InitMemBlock);
#endif

