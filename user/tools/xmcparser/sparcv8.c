/*
 * $FILE: sparcv8.c
 *
 * architecture dependent stuff
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#define _RSV_IO_PORTS_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <xm_inc/arch/paging.h>
#include <xm_inc/guest.h>
#include <xm_inc/arch/ginfo.h>

#include "conv.h"
#include "common.h"
#include "parser.h"
#include "xmc.h"

xm_u32_t ToRegionFlags(char *s) {
    xm_u32_t flags=0;
    if(!strcasecmp(s, "stram")) {
        flags=XMC_REG_FLAG_PGTAB;
    } else if(!strcasecmp(s, "sdram")) {
        flags=XMC_REG_FLAG_PGTAB;
    } else if(!strcasecmp(s, "rom")) {
        flags=XM_MEM_AREA_UNMAPPED|XMC_REG_FLAG_ROM;
    } else {
	EPrintF("Expected valid region type (%s)\n", s);
    }
    return flags;
}

static void BaseRange_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].range.base=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].range.base=node->line;
    xmcIoPortTab[C_IOPORT].type=XM_IOPORT_RANGE;
}

static struct attrXml baseRange_A={BAD_CAST"base", BaseRange_AH};

static void NoPortsRange_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].range.noPorts=ToU32((char *)val, 10);
    xmcIoPortTabNoL[C_IOPORT].range.noPorts=node->line;
}

static struct attrXml noPortsRange_A={BAD_CAST"noPorts", NoPortsRange_AH};

static void IoPort_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].noIoPorts++;
    xmc.noIoPorts++;
    DO_REALLOC(xmcIoPortTab, xmc.noIoPorts*sizeof(struct xmcIoPort));
    DO_REALLOC(xmcIoPortTabNoL, xmc.noIoPorts*sizeof(struct xmcIoPortNoL));
    memset(&xmcIoPortTab[C_IOPORT], 0, sizeof(struct xmcIoPort));
    memset(&xmcIoPortTabNoL[C_IOPORT], 0, sizeof(struct xmcIoPortNoL));
}

static struct nodeXml rangeIoPort_N={BAD_CAST"Range", IoPort_NH0, 0, 0, (struct attrXml *[]){&baseRange_A, &noPortsRange_A, 0}, 0};

static void AddressRestricted_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].restricted.address=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].restricted.address=node->line;
    xmcIoPortTab[C_IOPORT].type=XM_RESTRICTED_IOPORT;
}

static struct attrXml addressRestricted_A={BAD_CAST"address", AddressRestricted_AH};

static void MaskRestricted_AH(xmlNodePtr node, const xmlChar *val) {
    xmcIoPortTab[C_IOPORT].restricted.mask=ToU32((char *)val, 16);
    xmcIoPortTabNoL[C_IOPORT].restricted.mask=node->line;
}

static struct attrXml maskRestricted_A={BAD_CAST"mask", MaskRestricted_AH};

static struct nodeXml restrictedIoPort_N={BAD_CAST"Restricted", IoPort_NH0, 0, 0, (struct attrXml *[]){&addressRestricted_A, &maskRestricted_A, 0}, 0};

static void IoPorts_NH0(xmlNodePtr node) {
    xmcPartitionTab[C_PARTITION].ioPortsOffset=xmc.noIoPorts;
}

struct nodeXml ioPorts_N={BAD_CAST"IoPorts", IoPorts_NH0, 0, 0, 0, (struct nodeXml *[]){&rangeIoPort_N, &restrictedIoPort_N, 0}};

void GenerateIoPortTab(FILE *oFile) {
    int i;
    fprintf(oFile, "const struct xmcIoPort xmcIoPortTab[] = {\n");
    for (i=0; i<xmc.noIoPorts; i++) {
	fprintf(oFile, ADDNTAB(1, "[%d] = {\n"), i);
	fprintf(oFile, ADDNTAB(2, ".type = "));
	if (xmcIoPortTab[i].type==XM_IOPORT_RANGE) {
	    fprintf(oFile, "XM_IOPORT_RANGE,\n");
	    fprintf(oFile, ADDNTAB(2, "{.range.base = 0x%x,\n"), xmcIoPortTab[i].range.base);
	    fprintf(oFile, ADDNTAB(2, ".range.noPorts = %d, },\n"), xmcIoPortTab[i].range.noPorts);
	} else {
	    fprintf(oFile, "XM_RESTRICTED_IOPORT,\n");
	    fprintf(oFile, ADDNTAB(3, "{.restricted.address = 0x%x,\n"), xmcIoPortTab[i].restricted.address);
	    fprintf(oFile, ADDNTAB(3, ".restricted.mask = 0x%x,\n"), xmcIoPortTab[i].restricted.mask);
	    fprintf(oFile, ADDNTAB(2, "},\n"));
	}
	fprintf(oFile, ADDNTAB(1, "},\n"));
    }
    fprintf(oFile, "};\n\n");
}

void CheckIoPorts(void) {
    xmAddress_t a0, a1, b0, b1;
    int e, line0, line1, iP;

    for (iP=0; iP<xmc.noIoPorts; iP++) {	
	if (xmcIoPortTab[iP].type==XM_IOPORT_RANGE) {
	    b0=xmcIoPortTab[iP].range.base;
	    b1=b0+xmcIoPortTab[iP].range.noPorts*sizeof(xm_u32_t);
	    line0=xmcIoPortTabNoL[iP].range.base;
	} else {
	    b0=xmcIoPortTab[iP].restricted.address;
	    line0=xmcIoPortTabNoL[iP].restricted.address;
	    b1=b0+sizeof(xm_u32_t);
	}
	
	for (e=0; e<noRsvIoPorts; e++) {
	    a0=rsvIoPorts[e].base;
	    a1=a0+rsvIoPorts[e].offset*sizeof(xm_u32_t);
	    if (!((a0>=b1)||(a1<=b0))) {
		LineError(line0, "io-port [0x%lx:%d] reserved by XM",
			  rsvIoPorts[e].base, rsvIoPorts[e].offset);
	    }
	}
    
	for (e=iP+1; e<xmc.noIoPorts; e++) {
	    if (xmcIoPortTab[e].type==XM_IOPORT_RANGE) {
		a0=xmcIoPortTab[e].range.base;
		a1=a0+xmcIoPortTab[e].range.noPorts*sizeof(xm_u32_t);
		line1=xmcIoPortTabNoL[e].range.base;
	    } else {
		a0=xmcIoPortTab[e].restricted.address;
		a1=a0+sizeof(xm_u32_t);
		line1=xmcIoPortTabNoL[e].restricted.address;
	    }
	    if (!((a0>=b1)||(a1<=b0)))
		LineError(line1, "io-port [0x%lx:%d] already assigned (line %d)", b0, b1-b0, line0);
	}
    }
}

#ifdef CONFIG_MPU
void ArchMpuRsvMem(FILE *oFile) {
}
#endif

#ifdef CONFIG_MMU
#define PTDL1_SHIFT 24
#define PTDL2_SHIFT 18
#define PTDL3_SHIFT 12
#define PTDL1SIZE 1024
#define PTDL2SIZE 256
#define PTDL3SIZE 256

#define PTDL1ENTRIES (PTDL1SIZE>>2)
#define PTDL2ENTRIES (PTDL2SIZE>>2)
#define PTDL3ENTRIES (PTDL3SIZE>>2)
#define XM_VMAPEND 0xfeffffff
#define VA2PtdL1(x) (((x)&0xff000000)>>PTDL1_SHIFT)
#define VA2PtdL2(x) (((x)&0xfc0000)>>PTDL2_SHIFT)
#define VA2PtdL3(x) (((x)&0x3f000)>>PTDL3_SHIFT)
#define ROUNDUP(r, v) ((((~(r)) + 1)&((v)-1))+(r))

void ArchMmuRsvMem(FILE *oFile) {
/*    xmAddress_t a, b, addr;
    unsigned int **ptdL1[PTDL1ENTRIES], **ptdL2, *ptdL3;
    xm_s32_t e, i, l1e, l2e, l3e, l2c=0, l3c=0;
    // Memory Areas
    for (e=0; e<xmc.noPartitions; e++) {
        RsvBlock(PTDL1SIZE, PTDL1SIZE, "ptdL1");
        memset(ptdL1, 0, sizeof(unsigned int **)*PTDL1ENTRIES);
        for (i=0; i<xmcPartitionTab[e].noPhysicalMemoryAreas; i++) {
            if (xmcMemAreaTab[i+xmcPartitionTab[e].physicalMemoryAreasOffset].flags&XM_MEM_AREA_UNMAPPED)
                continue;
            a=xmcMemAreaTab[i+xmcPartitionTab[e].physicalMemoryAreasOffset].startAddr;
            b=a+xmcMemAreaTab[i+xmcPartitionTab[e].physicalMemoryAreasOffset].size;
            for (addr=a; addr<b; addr+=PAGE_SIZE) {
                l1e=VA2PtdL1(addr);
                l2e=VA2PtdL2(addr);
                l3e=VA2PtdL3(addr);
                if (!ptdL1[l1e]) {
                    l2c++;
                    DO_MALLOC(ptdL2, PTDL2SIZE);                
                    memset(ptdL2, 0, PTDL2SIZE);
                    ptdL1[l1e]=ptdL2;
                } else
                    ptdL2=ptdL1[l1e];
                
                if (!ptdL2[l2e]) {
                    l3c++;
                    DO_MALLOC(ptdL3, PTDL3SIZE);
                    memset(ptdL3, 0, PTDL3SIZE);
                    ptdL2[l2e]=ptdL3;           
                } else
                    ptdL3=ptdL2[l2e];       
            }
        }
    }

    for (e=0; e<l2c; e++)
        RsvBlock(PTDL2SIZE, PTDL2SIZE, "ptdL2");
    for (e=0; e<l3c; e++)
        RsvBlock(PTDL3SIZE, PTDL3SIZE, "ptdL3");*/
}

void ArchLdrRsvMem(FILE *oFile) {
    xm_s32_t i;
    for (i=0; i<xmc.noPartitions; i++) {
/*        RsvBlock(PTDL2SIZE, PTDL2SIZE, "ptdL2Ldr");*/
        RsvBlock(18*PAGE_SIZE, PAGE_SIZE, "Ldr stack");
    }
}

#endif
