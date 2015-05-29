/*
 * $FILE: xmlcheck.c
 *
 * Validates a partition xef against a xml xef file
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <ctype.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <features.h>
#include <xef.h>
#include <xm_inc/xmconf.h>
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmef.h>
#include <xm_inc/compress.h>
#include <xm_inc/digest.h>

#include <endianess.h>
#include <xmpack.h>

struct file2Proc {
    char *name;
    struct xefFile xef;
};

static xm_u8_t *LoadFile(char *name) {
    xm_s32_t fileSize;
    xm_u8_t *buffer;
    int fd;
    if ((fd=open(name, O_RDONLY))<0)
        EPrintF("File \"%s\" couldn't be opened", name);

    fileSize=lseek(fd, 0, SEEK_END);
    DO_MALLOC(buffer, fileSize);
    lseek(fd, 0, SEEK_SET);
    DO_READ(fd, buffer, fileSize);    
    return buffer;
}

static xm_s32_t URead(void *b, xmSize_t s, void *d) {
    memcpy(b, *(xm_u8_t **)d, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static xm_s32_t UWrite(void *b, xmSize_t s, void *d) {
    memcpy(*(xm_u8_t **)d, b, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static struct xmc *ParseXmc(struct file2Proc *xmcFile) {
    struct xmc *xmcTab=0;
    xm_u8_t *uimg;
    if ((RWORD(xmcFile->xef.hdr->flags)&XEF_TYPE_MASK)!=XEF_TYPE_CUSTOMFILE)
        EPrintF("File \"%s\": expecting a XML customization file", xmcFile->name);
    
    if (RWORD(xmcFile->xef.hdr->flags)&XEF_COMPRESSED) {
        xm_s32_t ptr, e;
        DO_MALLOC(uimg, RWORD(xmcFile->xef.hdr->imageLength));
        for (ptr=0, e=0; e<RWORD(xmcFile->xef.hdr->noSegments); e++, ptr+=RWORD(xmcFile->xef.segmentTab[e].fileSize)) {
            xm_u8_t *rPtr, *wPtr;
            rPtr=(xm_u8_t *)&xmcFile->xef.image[RWORD(xmcFile->xef.segmentTab[e].offset)];
            wPtr=(xm_u8_t *)&uimg[ptr];
            Uncompress(RWORD(xmcFile->xef.segmentTab[e].deflatedFileSize), RWORD(xmcFile->xef.segmentTab[e].fileSize), URead, &rPtr, UWrite, &wPtr);
        }
        xmcTab=(struct xmc *)uimg;
    } else {
        xmcTab=(struct xmc *)xmcFile->xef.image;
    }
    
    if (!xmcTab||xmcTab->signature!=RWORD(XMC_SIGNATURE))
	EPrintF("File \"%s\": invalid XMC signature (0x%x)", xmcFile->name, xmcTab->signature);    

    return xmcTab;   
}

static void CheckPartition(int partId, struct file2Proc *part, struct file2Proc *customTab, int noCustom, struct xmc *xmcTab) {
    struct xmcPartition *xmcPartitionTab;
    struct xmcMemoryArea *xmcMemArea;
    xmAddress_t a, b, c, d;
    int noMemAreas;
    int e, s, hyp=0, i;
    xmcMemArea=(struct xmcMemoryArea *)&((char *)xmcTab)[RWORD(xmcTab->physicalMemoryAreasOffset)];
    xmcPartitionTab=(struct xmcPartition *)&((char *)xmcTab)[RWORD(xmcTab->partitionTabOffset)];
   
    if (((RWORD(part->xef.hdr->flags)&XEF_TYPE_MASK)!=XEF_TYPE_PARTITION)&&
        ((RWORD(part->xef.hdr->flags)&XEF_TYPE_MASK)!=XEF_TYPE_HYPERVISOR))
        EPrintF("File \"%s\": XM or a partition expected", part->name);
    
    for (e=0; e<noCustom; e++)
        if ((RWORD(customTab[e].xef.hdr->flags)&XEF_TYPE_MASK)!=XEF_TYPE_CUSTOMFILE)
            EPrintF("File \"%s\": customization file expected ", customTab[e].name);

    if (noCustom!=RWORD(part->xef.hdr->noCustomFiles))
        //EPrintF
        fprintf(stderr, "File \"%s\": %d customization files expected, %d provided\n", part->name, RWORD(part->xef.hdr->noCustomFiles), noCustom);
   
    if ((RWORD(part->xef.hdr->flags)&XEF_TYPE_MASK)==XEF_TYPE_HYPERVISOR) {
        xmcMemArea=&xmcMemArea[RWORD(xmcTab->hpv.physicalMemoryAreasOffset)];
        noMemAreas=RWORD(xmcTab->hpv.noPhysicalMemoryAreas);
        hyp=1;
    } else {
        if ((partId<0)||(partId>=RWORD(xmcTab->noPartitions)))
            EPrintF("PartitionId %d: not valid", partId);
        xmcMemArea=&xmcMemArea[RWORD(xmcPartitionTab[partId].physicalMemoryAreasOffset)];
        noMemAreas=RWORD(xmcPartitionTab[partId].noPhysicalMemoryAreas);
    }

    for (s=0; s<RWORD(part->xef.hdr->noSegments); s++) {
        c=RWORD(part->xef.segmentTab[s].physAddr);
        d=c+RWORD(part->xef.segmentTab[s].fileSize);
        for (e=0; e<noMemAreas; e++) {
            a=RWORD(xmcMemArea[e].startAddr);
            b=a+RWORD(xmcMemArea[e].size);
            if ((c>=a)&&(d<=b))
                break;
        }
        if (e>=noMemAreas)
            EPrintF("Partition \"%s\" (%d): segment %d [0x%x- 0x%x] does not fit (XMC)", part->name, partId, s, c, d);
    }

    for (e=0; e<noCustom; e++) {
        if (RWORD(part->xef.customFileTab[e].size)) {
            if (RWORD(customTab[e].xef.hdr->imageLength)>RWORD(part->xef.customFileTab[e].size))
                EPrintF("Customization file \"%s\": too big (%d bytes), partition \"%s\" only reserves %d bytes", customTab[e].name, RWORD(customTab[e].xef.hdr->imageLength), part->name, RWORD(part->xef.customFileTab[e].size));
        }
        c=RWORD(part->xef.customFileTab[e].sAddr);        
        if (hyp&&(e==0)) { // XML
            d=c+RWORD(xmcTab->size);
        } else
            d=c+RWORD(customTab[e].xef.hdr->imageLength);

        for (i=0; i<noMemAreas; i++) {
            if (hyp) {
                a=RWORD(xmcMemArea[i].startAddr);
                b=a+RWORD(xmcMemArea[i].size);
            } else {
                a=RWORD(xmcMemArea[i].mappedAt);
                b=a+RWORD(xmcMemArea[i].size);
            }
            if ((c>=a)&&(d<=b))
                break;
        }

        if (i>=noMemAreas)
            EPrintF("Customization file \"%s\" (%d): [0x%x- 0x%x] does not fit (XMC)", customTab[e].name, e, c, d);
    }
}

static void GetPartitionInfo(char *in, int *partitionId, struct file2Proc *part, struct file2Proc **customTab, int *noCustomFiles) {
    char *ptr, *savePtr=0, *atStr, *file;
    int e, id=-1;
    
    for (ptr=in, atStr=0, e=0; (file=strtok_r(ptr, ":", &savePtr)); ptr=0, e++) {
	if (id<0) {
	    if (((id=strtoul(file, 0, 10))<0))
		EPrintF("Partition ID shall be a positive integer");

	    *partitionId=id;
	    continue;
	}
	
        if (e==1) {
            part->name=strdup(file);
	    *noCustomFiles=0;
	} else {
	    if (((*noCustomFiles)+1)>CONFIG_MAX_NO_CUSTOMFILES)
		EPrintF("Only %d customisation files are permitted", CONFIG_MAX_NO_CUSTOMFILES);
            (*noCustomFiles)++;
            DO_REALLOC((*customTab), (*noCustomFiles)*sizeof(struct file2Proc));
            (*customTab)[(*noCustomFiles)-1].name=strdup(file);
	}
    }
}

static void GetHypervisorInfo(char *in, struct file2Proc *part, struct file2Proc **customTab, int *noCustomFiles) {
    char *ptr, *savePtr=0, *atStr, *file;
    int e;

    for (ptr=in, atStr=0, e=0; (file=strtok_r(ptr, ":", &savePtr)); ptr=0, e++) {
        if (e==0) {
            part->name=strdup(file);
	    *noCustomFiles=0;
	} else {
	    if (((*noCustomFiles)+1)>CONFIG_MAX_NO_CUSTOMFILES)
		EPrintF("Only %d customisation files are permitted", CONFIG_MAX_NO_CUSTOMFILES);
            (*noCustomFiles)++;
            DO_REALLOC((*customTab), (*noCustomFiles)*sizeof(struct file2Proc));
            (*customTab)[(*noCustomFiles)-1].name=strdup(file);
	}
    }
}

void DoCheck(int argc, char **argv) {
    struct file2Proc xmc, part, *customTab=0;
    int ret, noCustom=0, e, i, partId=0;
    struct xmc *xmcTab;
    if (argc<4){
	fprintf(stderr, USAGE);
	exit(2);
    }

    xmc.name=strdup(argv[1]);
    if ((ret=ParseXefFile(LoadFile(xmc.name), &xmc.xef))!=XEF_OK)
        EPrintF("Error loading XEF file \"%s\": %d", xmc.name, ret);
    xmcTab=ParseXmc(&xmc);

    for (e=2; e<argc; e++) {
        if (!strcmp(argv[e], "-h")) {
            GetHypervisorInfo(argv[++e], &part, &customTab, &noCustom);
        } else if (!strcmp(argv[e], "-p")) {
            GetPartitionInfo(argv[++e], &partId, &part, &customTab, &noCustom);
        } else {
            fprintf(stderr, "Ignoring unexpected argument (%s)\n", argv[e]);
            continue;
        }
        if ((ret=ParseXefFile(LoadFile(part.name), &part.xef))!=XEF_OK)
            EPrintF("Error loading XEF file \"%s\": %d", part.name, ret);
        if (noCustom) {
            for (i=0; i<noCustom; i++) {
                if ((ret=ParseXefFile(LoadFile(customTab[i].name), &customTab[i].xef))!=XEF_OK)
                    EPrintF("Error loading XEF file \"%s\": %d", customTab[i].name, ret);
            }
        }
        fprintf(stderr, "> %s ... ", part.name);
        CheckPartition(partId, &part, customTab, noCustom, xmcTab);
        fprintf(stderr, "ok\n");
    }
}

void DoCheck2Build(int argc, char **argv) {
    struct file2Proc xmc, part, *customTab=0;
    int ret, noCustom=0, e, i, partId=0;
    struct xmc *xmcTab=0;
    char **_argv;

    DO_MALLOC(_argv, argc*sizeof(char *));
    
    for (e=0; e<argc; e++)
        _argv[e]=strdup(argv[e]);

    for (e=0; e<argc; e++) {
        if (!strcmp(_argv[e], "-h")) {
            GetHypervisorInfo(_argv[++e], &part, &customTab, &noCustom);
            if (noCustom<1)
                EPrintF("Hypervisor requires at least one custom file");
            xmc.name=strdup(customTab[0].name);
        } else if (!strcmp(_argv[e], "-p")) {
            GetPartitionInfo(_argv[++e], &partId, &part, &customTab, &noCustom);
        } else {
            continue;
        }
        if ((ret=ParseXefFile(LoadFile(part.name), &part.xef))!=XEF_OK)
            EPrintF("Error loading XEF file \"%s\": %d", part.name, ret);
        if (noCustom) {
            for (i=0; i<noCustom; i++) {
                if ((ret=ParseXefFile(LoadFile(customTab[i].name), &customTab[i].xef))!=XEF_OK)
                    EPrintF("Error loading XEF file \"%s\": %d", customTab[i].name, ret);
            }
        }
        if ((ret=ParseXefFile(LoadFile(xmc.name), &xmc.xef))!=XEF_OK)
            EPrintF("Error loading XEF file \"%s\": %d", xmc.name, ret);
        xmcTab=ParseXmc(&xmc);
        fprintf(stderr, "> %s ... ", part.name);
        CheckPartition(partId, &part, customTab, noCustom, xmcTab);
        fprintf(stderr, "ok\n");
    }
    for (e=0; e<argc; e++)
        free(_argv[e]);
    free(_argv);
}
