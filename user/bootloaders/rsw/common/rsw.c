/*
 * $FILE: rsw.c
 *
 * A boot rsw
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <xm.h>
#include <container.h>
#include <xef.h>
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmconf.h>
#include <xm_inc/compress.h>
#include <xm_inc/digest.h>
#include <rsw_stdc.h>

void HaltSystem(void) {
    extern void _halt_system(void);
    xprintf("[RSW] System Halted.\n");
    _halt_system();
}

void DoTrap(xm_u32_t noTrap) {
    xprintf("[RSW] Unexpected trap: %d\n", noTrap);
    HaltSystem();
}

static int XmcVAddr2PAddr(struct xmcMemoryArea *mAreas, xm_s32_t noAreas, xmAddress_t vAddr, xmAddress_t *pAddr) {
    xm_s32_t e;
    for (e=0; e<noAreas; e++) {
        if ((mAreas[e].mappedAt<=vAddr)&&(((mAreas[e].mappedAt+mAreas[e].size)-1)>=vAddr)) {
            *pAddr=vAddr-mAreas[e].mappedAt+mAreas[e].startAddr;
            return 0;
        }
    }
    return -1;
}


extern xmAddress_t hpvEntryPoint[];

void RSwMain(void) {
    extern xm_u8_t *xmefContainerPtr;
    extern void start(void);
    struct xefContainerFile container;
    struct xmcBootPart *xmcBootPartTab;
    struct xefFile xefFile, xefCustomFile;
    struct xmHdr *xmHdr;
    struct xmc *xmc;
    struct xmcPartition *xmcPartitions;
    struct xmcMemoryArea *xmcMemArea;
    xm_s32_t ret, min, e, i;

    InitOutput();
    xprintf("[RSW] Start Resident Software\n");
    // Parse container
    if ((ret=ParseXefContainer(xmefContainerPtr, &container))!=CONTAINER_OK) {
	xprintf("[RSW] Error %d when parsing container file\n", ret);
	HaltSystem();
    }

    // Loading XM and the XML configuration file    
    if ((ret=ParseXefFile((xm_u8_t *)(container.fileTab[container.partitionTab[0].file].offset+xmefContainerPtr), &xefFile))!=XEF_OK) {
	xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	HaltSystem();
    }

    // Checking xml configuration file's digestion correctness
    if ((ret=ParseXefFile((xm_u8_t *)(container.fileTab[container.partitionTab[0].customFileTab[0]].offset+xmefContainerPtr), &xefCustomFile))!=XEF_OK) {
	xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	HaltSystem();
    }

    xmc=LoadXefCustomFile(&xefCustomFile, &xefFile.customFileTab[0]);

    if (xmc->signature!=XMC_SIGNATURE) {
        xprintf("[RSW] XMC signature not found\n");
	HaltSystem();
    }    

    xmHdr=LoadXefFile(&xefFile, 0, 0, 0);
    hpvEntryPoint[0]=xefFile.hdr->entryPoint;
    if ((xmHdr->sSignature!=XMEF_XM_MAGIC)||(xmHdr->eSignature!=XMEF_XM_MAGIC)) {
	xprintf("[RSW] XtratuM signature not found\n");
	HaltSystem();
    }

    // Loading additional custom files
    min=(container.partitionTab[0].noCustomFiles>xmHdr->noCustomFiles)?xmHdr->noCustomFiles:container.partitionTab[0].noCustomFiles;
    
    for (e=1; e<min; e++) {
        if ((ret=ParseXefFile((xm_u8_t *)(container.fileTab[container.partitionTab[0].customFileTab[e]].offset+xmefContainerPtr), &xefCustomFile))!=XEF_OK) {
            xprintf("[RSW] Error %d when parsing XEF file\n", ret);
            HaltSystem();
        }
        LoadXefCustomFile(&xefCustomFile, &xefFile.customFileTab[e]);
    }

    xmcBootPartTab=(struct xmcBootPart *)((xmAddress_t)xmc+xmc->bootPartitionTabOffset);
    xmcPartitions=(struct xmcPartition *)((xmAddress_t)xmc+xmc->partitionTabOffset);
    xmcMemArea=(struct xmcMemoryArea *)((xmAddress_t)xmc+xmc->physicalMemoryAreasOffset);
    // Loading partitions
    for (e=1; e<container.hdr->noPartitions; e++) {
        struct xmImageHdr *partHdr;
        if ((ret=ParseXefFile((xm_u8_t *)(container.fileTab[container.partitionTab[e].file].offset+xmefContainerPtr), &xefFile))!=XEF_OK) {
	    xprintf("[RSW] Error %d when parsing XEF file\n", ret);
	    HaltSystem();
	}
        partHdr=LoadXefFile(&xefFile, (int (*)(void *, xm_s32_t, xmAddress_t, xmAddress_t *))XmcVAddr2PAddr, &xmcMemArea[xmcPartitions[container.partitionTab[e].id].physicalMemoryAreasOffset], xmcPartitions[container.partitionTab[e].id].noPhysicalMemoryAreas);
        if ((partHdr->sSignature!=XMEF_PARTITION_MAGIC)&&
            (partHdr->eSignature!=XMEF_PARTITION_MAGIC)) {
	    xprintf("[RSW] Partition signature not found (0x%x)\n", partHdr->sSignature);
	    HaltSystem();
	}
        xmcBootPartTab[container.partitionTab[e].id].flags|=XM_PART_BOOT;
        xmcBootPartTab[container.partitionTab[e].id].hdrPhysAddr=(xmAddress_t)partHdr;
        xmcBootPartTab[container.partitionTab[e].id].entryPoint=xefFile.hdr->entryPoint;
        xmcBootPartTab[container.partitionTab[e].id].imgStart=(xmAddress_t)xefFile.hdr;
        xmcBootPartTab[container.partitionTab[e].id].imgSize=xefFile.hdr->fileSize;
        // Loading additional custom files
        min=(container.partitionTab[e].noCustomFiles>partHdr->noCustomFiles)?partHdr->noCustomFiles:container.partitionTab[e].noCustomFiles;

        if (min>CONFIG_MAX_NO_CUSTOMFILES){
                xprintf("[RSW] Error when parsing XEF custom files. noCustomFiles > CONFIG_MAX_NO_CUSTOMFILES\n");
                HaltSystem();
        }

        xmcBootPartTab[container.partitionTab[e].id].noCustomFiles=min;

        for (i=0; i<min; i++) {
            if ((ret=ParseXefFile((xm_u8_t *)(container.fileTab[container.partitionTab[e].customFileTab[i]].offset+xmefContainerPtr), &xefCustomFile))!=XEF_OK) {
                xprintf("[RSW] Error %d when parsing XEF file\n", ret);
                HaltSystem();
            }
            xmcBootPartTab[container.partitionTab[e].id].customFileTab[i].sAddr=(xmAddress_t)xefCustomFile.hdr;
            xmcBootPartTab[container.partitionTab[e].id].customFileTab[i].size=xefCustomFile.hdr->imageLength;
            LoadXefCustomFile(&xefCustomFile, &xefFile.customFileTab[i]);
        }
    }

    xprintf("[RSW] Starting XM at 0x%x\n", hpvEntryPoint[0]);
    ((void (*)(void))ADDR2PTR(hpvEntryPoint[0]))();
}

