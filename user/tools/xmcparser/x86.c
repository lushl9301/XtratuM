/*
 * $FILE: x86.c
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
#include <xm_inc/arch/segments.h>
#include <xm_inc/arch/ginfo.h>

#include "parser.h"
#include "conv.h"
#include "common.h"
#include "xmc.h"

#define _PHYS2VIRT(x) ((xm_u32_t)(x)+CONFIG_XM_OFFSET-CONFIG_XM_LOAD_ADDR)

xm_u32_t ToRegionFlags(char *s) {
    xm_u32_t flags=0;
    if(!strcasecmp(s, "ram")) {
        flags=XMC_REG_FLAG_PGTAB;
    } else if(!strcasecmp(s, "rom")) {
        flags=XMC_REG_FLAG_ROM;
    } else {
	EPrintF("Expected valid region type (%s)\n", s);
    }
    return flags;
}

static xm_u32_t ioPortBase, noIoPorts;

static void BaseRange_AH(xmlNodePtr node, const xmlChar *val) {
    ioPortBase=ToU32((char *)val, 16);
}

static struct attrXml baseRange_A={BAD_CAST"base", BaseRange_AH};

static void NoPortsRange_AH(xmlNodePtr node, const xmlChar *val) {
    noIoPorts=ToU32((char *)val, 10);
    xmcPartitionTab[C_PARTITION].noIoPorts+=noIoPorts;
}

static struct attrXml noPortsRange_A={BAD_CAST"noPorts", NoPortsRange_AH};

static void IoPortRange_NH1(xmlNodePtr node) {
    xm_u32_t port, a, b, c, d;
    xm_s32_t e;

    c=ioPortBase;
    d=c+noIoPorts-1;
    for (e=0; e<noRsvIoPorts; e++) {
        a=rsvIoPorts[e].base;
        b=a+rsvIoPorts[e].offset-1;
        if (!((d<a)||(c>=b)))
            LineError(node->line, "io-ports [0x%lx:%d] reserved for XM ([0x%lx:%d])", ioPortBase, noIoPorts, rsvIoPorts[e].base, rsvIoPorts[e].offset);
    }

    for (port=c; port<d; port++)
        xmcIoPortTab[C_IOPORT].map[port/32]&=~(1<<(port%32));
    ioPortBase=0;
    noIoPorts=0;
}

static struct nodeXml rangeIoPort_N={BAD_CAST"Range", 0, IoPortRange_NH1, 0, (struct attrXml *[]){&baseRange_A, &noPortsRange_A, 0}, 0};

static void IoPortRestricted_NH0(xmlNodePtr node) {
    LineError(node->line, "Restricted IoPorts not supported");
}

static struct nodeXml restrictedIoPort_N={BAD_CAST"Restricted", IoPortRestricted_NH0, 0, 0, (struct attrXml *[]){0}, 0};

static void IoPorts_NH0(xmlNodePtr node) {
    xm_s32_t e;
    xmcPartitionTab[C_PARTITION].ioPortsOffset=xmc.noIoPorts;
    xmc.noIoPorts++;
    DO_REALLOC(xmcIoPortTab, xmc.noIoPorts*sizeof(struct xmcIoPort));
    memset(&xmcIoPortTab[C_IOPORT], 0, sizeof(struct xmcIoPort));
    for (e=0; e<2048; e++)
        xmcIoPortTab[C_IOPORT].map[e]=~0;
        //xmcIoPortTab[C_IOPORT].map[2047]=0xff000000;
}

struct nodeXml ioPorts_N={BAD_CAST"IoPorts", IoPorts_NH0, 0, 0, 0, (struct nodeXml *[]){&rangeIoPort_N, &restrictedIoPort_N, 0}};

void GenerateIoPortTab(FILE *oFile) {
    int i, j, f;    
    fprintf(oFile, "const struct xmcIoPort xmcIoPortTab[] = {\n");
    for (i=0; i<xmc.noIoPorts; i++) {
        fprintf(oFile, ADDNTAB(1, "[%d] = { .map = {\n"), i);
        xmcIoPortTab[i].map[2047]|=0xff000000;
        for (f=0, j=1; j<2048; j++) {
            if (xmcIoPortTab[i].map[f]!=xmcIoPortTab[i].map[j]) {
                if (f!=(j-1))
                    fprintf(oFile, ADDNTAB(2, "[%d ... %d] = 0x%x,\n"), f, j-1, xmcIoPortTab[i].map[f]);
                else
                    fprintf(oFile, ADDNTAB(2, "[%d] = 0x%x,\n"), f, xmcIoPortTab[i].map[f]);
                f=j;
            }
        }
        if (f!=2047)
            fprintf(oFile, ADDNTAB(2, "[%d ... 2047] = 0x%x,\n"), f, xmcIoPortTab[i].map[f]);
        else
            fprintf(oFile, ADDNTAB(2, "[2047] = 0x%x,\n"), xmcIoPortTab[i].map[f]);	
        fprintf(oFile, ADDNTAB(1, "}, },\n"));
    }
    fprintf(oFile, "};\n\n");
}

void CheckIoPorts(void) {
}

#ifdef CONFIG_MPU
void ArchMpuRsvMem(FILE *oFile) {
}
#endif

#define ROUNDUP(r, v) ((((~(r)) + 1)&((v)-1))+(r))

void ArchMmuRsvMem(FILE *oFile) {
    xmAddress_t end;

    end=ROUNDUP(_PHYS2VIRT(xmcMemAreaTab[xmc.hpv.physicalMemoryAreasOffset].startAddr)+xmcMemAreaTab[xmc.hpv.physicalMemoryAreasOffset].size, LPAGE_SIZE);
    RsvBlock((((XM_VMAPEND-end)+1)>>PTDL1_SHIFT)*PTDL2SIZE, PTDL2SIZE, "canon-ptdL2");
}

void ArchLdrRsvMem(FILE *oFile) {
    xm_s32_t i;
    for (i=0; i<xmc.noPartitions; i++) {
/*        RsvBlock(PTDL2SIZE, PTDL2SIZE, "ptdL2Ldr");*/
        RsvBlock(18*PAGE_SIZE, PAGE_SIZE, "Ldr stack");
    }
}
