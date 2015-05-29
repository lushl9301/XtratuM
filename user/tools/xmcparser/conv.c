/*
 * $FILE: conv.c
 *
 * conversion helper functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "limits.h"
#include "common.h"
#include "parser.h"
#include "checks.h"
#include "xmc.h"

#include <xm_inc/audit.h>

xm_u32_t ToVersion(char *s) {
    xm_u32_t version, subversion, revision;

    sscanf(s, "%u.%u.%u", &version, &subversion, &revision);
    return XMC_SET_VERSION(version, subversion, revision);
}

xm_u32_t ToU32(char *s, int base) {
    xm_u32_t u;

    u=strtoul(s, 0, base);
    return u;
}

int ToYNTF(char *s, int line) {
     if (!strcasecmp(s, "yes")||!strcasecmp(s, "true"))
         return 1;
     
     if (!strcasecmp(s, "no")||!strcasecmp(s, "false"))
         return 0;
     
     LineError(line, "expected yes|no|true|false");
     return 0;
}

xm_u32_t ToFreq(char *s) {
    double d;
    sscanf(s, "%lf", &d);
    if(strcasestr(s, "MHz")) {
	d*=1000.0;
    } /*else if(strcasestr(b, "KHz")) {
	d*=1000.0;
	} */
    if (d>=UINT_MAX)
        EPrintF("Frequency exceeds MAX_UINT");
    return (xm_u32_t)d;
}

xm_u32_t ToTime(char *s) {
    double d;
    sscanf(s, "%lf", &d); 
    if(strcasestr(s, "us")) {
    } else if(strcasestr(s, "ms")) {
	d*=1000.0;
    } else  if(strcasestr(s, "s")) {
	d*=1000000.0;
    }
    if (d>=UINT_MAX)
        EPrintF("Time exceeds MAX_UINT");
    return (xm_u32_t)d;
}

xmSize_t ToSize(char *s) {
    double d;
    sscanf(s, "%lf", &d);  
    if(strcasestr(s, "MB")) {
	d*=(1024.0*1024.0);
    } else if(strcasestr(s, "KB")) {
	d*=1024.0;
    } /*else  if(strcasestr(s, "B")) {
	} */

    if (d>=UINT_MAX)
	EPrintF("Size exceeds MAX_UINT");
    
    return (xmSize_t)d;
}

xm_u32_t ToCommPortType(char *s, int line) {
    if (!strcmp(s, "queuing"))
	return XM_QUEUING_PORT;

    if (!strcmp(s, "sampling"))
	return XM_SAMPLING_PORT;

    if (!strcmp(s, "ttnoc"))
	return XM_TTNOC_PORT;

    LineError(line, "expected a valid communication port type");
    return 0;
}

struct attr2flags {
    char attr[32];
    xm_u32_t flag;
};

#define NO_PARTITION_FLAGS 3
static struct attr2flags partitionFlagsTab[NO_PARTITION_FLAGS]={
    [0]={
	.attr="system",
	.flag=XM_PART_SYSTEM,
    },
    [1]={
	.attr="fp",
	.flag=XM_PART_FP,
    },
    [2]={
	.attr="none",
	.flag=0x0,
    },
};

#ifdef CONFIG_AUDIT_EVENTS
#define NO_BITMASK_TRACE_HYP 4
static struct attr2flags bitmaskTraceHypTab[NO_BITMASK_TRACE_HYP]={
    [0]={
        .attr="HYP_IRQS",
        .flag=TRACE_BM_IRQ_MODULE,
    },
    [1]={
        .attr="HYP_HCALLS",
        .flag=TRACE_BM_HCALLS_MODULE,
    },
    [2]={
        .attr="HYP_SCHED",
        .flag=TRACE_BM_SCHED_MODULE,
    },
    [3]={
        .attr="HYP_HM",
        .flag=TRACE_BM_HM_MODULE,
    },
};
#endif

#define NO_PHYMEM_AREA_FLAGS 11
static struct attr2flags physMemAreaFlagsTab[NO_PHYMEM_AREA_FLAGS]={
    [0]={
	.attr="unmapped",
	.flag=XM_MEM_AREA_UNMAPPED,
    },
    [1]={
	.attr="shared",
	.flag=XM_MEM_AREA_SHARED,
    },
    [2]={
	.attr="read-only",
	.flag=XM_MEM_AREA_READONLY,
    },
    [3]={
	.attr="uncacheable",
	.flag=XM_MEM_AREA_UNCACHEABLE,
    },
    [4]={
	.attr="rom",
	.flag=XM_MEM_AREA_ROM,
    },
    [5]={
	.attr="flag0",
	.flag=XM_MEM_AREA_FLAG0,
    },
    [6]={
	.attr="flag1",
	.flag=XM_MEM_AREA_FLAG1,
    },
    [7]={
	.attr="flag2",
	.flag=XM_MEM_AREA_FLAG2,
    },
    [8]={
	.attr="flag3",
	.flag=XM_MEM_AREA_FLAG3,
    },
    [9]={
        .attr="iommu",
        .flag=XM_MEM_AREA_IOMMU,
    },
    [10]={
	.attr="none",
	.flag=0x0,
    },
};

void ProcessIdList(char *s, void (*CallBack)(int, char *), int line) {
    char *tmp, *tmp1;

    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;
        
        CallBack(line, tmp);

        if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
     }
}

static xm_u32_t ToFlags(char *s, struct attr2flags *attr2flags, int noElem, int line) {
    xm_u32_t flags=0, found=0;
    char *tmp, *tmp1;
    int e;

    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;
	for (e=0; e<noElem; e++)
	    if (!strcasecmp(tmp, attr2flags[e].attr)) {
		flags|=attr2flags[e].flag;
		found=1;
	    }
	if (!found)
	    LineError(line, "expected valid flag (%s)\n", tmp);
	found=0;
	
	if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
    }

    return flags;
}

xm_u32_t ToPartitionFlags(char *s, int line) {
    return ToFlags(s, partitionFlagsTab, NO_PARTITION_FLAGS, line);
}

xm_u32_t ToBitmaskTraceHyp(char *s, int line) {
#ifdef CONFIG_AUDIT_EVENTS
    return ToFlags(s, bitmaskTraceHypTab, NO_BITMASK_TRACE_HYP, line);
#else
    return 0;
#endif
}

xm_u32_t ToPhysMemAreaFlags(char *s, int line) {
    return ToFlags(s, physMemAreaFlagsTab, NO_PHYMEM_AREA_FLAGS, line);
}

xm_u32_t ToCommPortDirection(char *s, int line) {
    if (!strcmp(s, "source"))
	return XM_SOURCE_PORT;

    if (!strcmp(s, "destination"))
	return XM_DESTINATION_PORT;
	    
    LineError(line, "expected a valid communication port direction");
    return 0;
}

void ToHwIrqLines(char *s, int lineNo) {
    char *tmp, *tmp1;
    int line;
    
    for (tmp=s, tmp1=strstr(tmp, " "); *tmp;) {
        if (tmp1) *tmp1=0;

	line=atoi(tmp);
	if (line>=CONFIG_NO_HWIRQS)
	    LineError(lineNo, "invalid hw interrupt line (%d)", line);
	if (xmc.hpv.hwIrqTab[line].owner!=XM_IRQ_NO_OWNER)
	    LineError(lineNo, "hw interrupt line (%d) already assigned (line %d)", line, xmcNoL.hpv.hwIrqTab[line]);
	
	CheckHwIrq(line, lineNo);
	xmc.hpv.hwIrqTab[line].owner=xmc.noPartitions-1;
	xmcNoL.hpv.hwIrqTab[line]=lineNo;
	if (tmp1)
            tmp=tmp1+1;
        else
            break;
	tmp1=strstr(tmp, " ");
    }
}

