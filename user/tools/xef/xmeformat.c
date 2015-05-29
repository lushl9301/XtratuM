/*
 * $FILE: main.c
 *
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

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <elf.h>
#include <stddef.h>
#include <xeftool.h>

#include <xm_inc/digest.h>
#include <xm_inc/compress.h>
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmef.h>

#define TOOL_NAME "xmeformat"
#define USAGE "Usage: " TOOL_NAME " [read] [build]\n"  \
    " \tread [-h|-s|-m] <input>\n" \
    " \tbuild [-m] [-o <output>] [-c] [-p <payload_file>] <input>\n"          

struct xefHdr xefHdr, *xefHdrRead;
struct xefSegment *xefSegmentTab;
struct xefRela *xefRelaTab;
struct xefRel *xefRelTab;
struct xefCustomFile *xefCustomFileTab;
xm_u8_t *image;

void EPrintF(char *fmt, ...) {
    va_list args;

    fflush(stdout);
    if(TOOL_NAME != NULL)
        fprintf(stderr, "%s: ", TOOL_NAME);

    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    if (fmt[0] != '\0' && fmt[strlen(fmt)-1] == ':')
        fprintf(stderr, " %s", strerror(errno));
    fprintf(stderr, "\n");
    exit(2); /* conventional value for failed execution */
}

#if defined(CONFIG_SPARCv8)
#define EM_ARCH EM_SPARC
#define ELF(x) Elf32_##x
#define PRINT_PREF
#endif

#if defined(CONFIG_x86)
#define EM_ARCH EM_386
#define ELF(x) Elf32_##x
#define PRINT_PREF
#endif

#ifdef FORCE_ELF32
#undef ELF
#define ELF(x) Elf32_##x
#undef EM_ARCH
#define EM_ARCH EM_386
#endif

static int ParseElfImage(int fdElf) {
    struct xmImageHdr *xmImageHdr=0;
    struct xefSegment *xefSect;
    struct xmHdr *xmHdr=0;
    struct xefCustomFile *customFileTab;
    ELF(Ehdr) eHdr;
    ELF(Phdr) *pHdr;
    ELF(Shdr) *sHdr;
    char *elfStrTab=0;
    int e;

    customFileTab=0;
    xefHdr.noSegments=0;
    xefSegmentTab=0;
    xefCustomFileTab=0;
    xefRelTab=0;
    xefRelaTab=0;
    image=0;
    xefHdr.imageLength=0;

    lseek(fdElf, 0, SEEK_SET);
    DO_READ(fdElf, &eHdr, sizeof(ELF(Ehdr)));
    if ((RHALF(eHdr.e_type)!=ET_EXEC)||(eHdr.e_machine!=RHALF(EM_ARCH))||(eHdr.e_phentsize!=RHALF(sizeof(ELF(Phdr)))))
	EPrintF("Malformed ELF header");

    // Looking for .xmHdr/.xmImageHdr sections
    sHdr=malloc(sizeof(ELF(Shdr))*RHALF(eHdr.e_shnum));
    lseek(fdElf,  RWORD(eHdr.e_shoff), SEEK_SET);
    DO_READ(fdElf, sHdr, sizeof(ELF(Shdr))*RHALF(eHdr.e_shnum));

    // Locating the string table
    for (e=0; e<RHALF(eHdr.e_shnum); e++)
	if (RWORD(sHdr[e].sh_type)==SHT_STRTAB) {
	    DO_MALLOC(elfStrTab, RWORD(sHdr[e].sh_size));
	    lseek(fdElf, RWORD(sHdr[e].sh_offset), SEEK_SET);
	    DO_READ(fdElf, elfStrTab, RWORD(sHdr[e].sh_size));	    
	    break;
	}
    if (!elfStrTab)
	EPrintF("ELF string table not found");

    xefHdr.entryPoint=eHdr.e_entry;
    for (e=0; e<RHALF(eHdr.e_shnum); e++) {
	if (RWORD(sHdr[e].sh_type)==SHT_PROGBITS) {
            if (!strcmp(&elfStrTab[RWORD(sHdr[e].sh_name)], ".xmHdr")) {
                if (RWORD(sHdr[e].sh_size)!=sizeof(struct xmHdr))
                    EPrintF("Malformed .xmHdr section");
                if (xmHdr)
                    EPrintF(".xmHdr section found twice");
                if (xmImageHdr)
                    EPrintF(".xmImageHdr section already found");
                DO_MALLOC(xmHdr, sizeof(struct xmHdr));
                lseek(fdElf, RWORD(sHdr[e].sh_offset), SEEK_SET);
                DO_READ(fdElf, xmHdr, RWORD(sHdr[e].sh_size));
                if ((RWORD(xmHdr->sSignature)!=XMEF_XM_MAGIC)&&(RWORD(xmHdr->eSignature)!=XMEF_XM_MAGIC))
                    EPrintF("Malformed .xmHdr structure");
                customFileTab=xmHdr->customFileTab;
                xefHdr.noCustomFiles=RWORD(xmHdr->noCustomFiles);
                xefHdr.xmImageHdr=(xmAddress_t)RWORD(sHdr[e].sh_addr);
                xefHdr.flags&=~XEF_TYPE_MASK;
                xefHdr.flags|=XEF_TYPE_HYPERVISOR;
            } else if (!strcmp(&elfStrTab[RWORD(sHdr[e].sh_name)], ".xmImageHdr")) {
                if (RWORD(sHdr[e].sh_size)!=sizeof(struct xmImageHdr))
                    EPrintF("Malformed .xmImageHdr section");
                if (xmHdr)
                    EPrintF(".xmHdr section already found");
                if (xmImageHdr)
                    EPrintF(".xmImageHdr section found twice");
                DO_MALLOC(xmImageHdr, sizeof(struct xmImageHdr));
                lseek(fdElf, RWORD(sHdr[e].sh_offset), SEEK_SET);
                DO_READ(fdElf, xmImageHdr, RWORD(sHdr[e].sh_size)); 
                if ((RWORD(xmImageHdr->sSignature)!=XMEF_PARTITION_MAGIC)&&(RWORD(xmImageHdr->eSignature)!=XMEF_PARTITION_MAGIC))
                    EPrintF("Malformed .xmImageHdr structure");
                customFileTab=xmImageHdr->customFileTab;
                xefHdr.noCustomFiles=RWORD(xmImageHdr->noCustomFiles);
                xefHdr.xmImageHdr=(xmAddress_t)RWORD(sHdr[e].sh_addr);
                xefHdr.flags&=~XEF_TYPE_MASK;
                xefHdr.flags|=XEF_TYPE_PARTITION;
                xefHdr.pageTable=RWORD(xmImageHdr->pageTable);
                xefHdr.pageTableSize=RWORD(xmImageHdr->pageTableSize);
            }
        }
    }
    if (!xmHdr&&!xmImageHdr)
        EPrintF("Neither .xmHdr nor .xmImageHdr found");

    DO_MALLOC(xefCustomFileTab, sizeof(struct xefCustomFile)*xefHdr.noCustomFiles);
    for (e=0; e<xefHdr.noCustomFiles; e++) {
        xefCustomFileTab[e].sAddr=RWORD(customFileTab[e].sAddr);
        xefCustomFileTab[e].size=RWORD(customFileTab[e].size);
    }

    pHdr=malloc(sizeof(ELF(Phdr))*RHALF(eHdr.e_phnum));
    lseek(fdElf, RWORD(eHdr.e_phoff), SEEK_SET);
    DO_READ(fdElf, pHdr, sizeof(ELF(Phdr))*RHALF(eHdr.e_phnum));
    
    for (e=0; e<RHALF(eHdr.e_phnum); e++) {
        if (RWORD(pHdr[e].p_type)!=PT_LOAD)
            continue;
        if (!pHdr[e].p_filesz)
            continue;
        xefHdr.noSegments++;
        DO_REALLOC(xefSegmentTab, xefHdr.noSegments*sizeof(struct xefSegment));
	xefSect=&xefSegmentTab[xefHdr.noSegments-1];
        xefSect->physAddr=RWORD(pHdr[e].p_paddr);
	xefSect->virtAddr=RWORD(pHdr[e].p_vaddr);
	xefSect->fileSize=RWORD(pHdr[e].p_filesz);
	xefSect->deflatedFileSize=RWORD(pHdr[e].p_filesz);

	xefSect->offset=xefHdr.imageLength;
	xefHdr.imageLength+=xefSect->fileSize;
	DO_REALLOC(image, xefHdr.imageLength);

	lseek(fdElf, RWORD(pHdr[e].p_offset), SEEK_SET);
	DO_READ(fdElf, &image[xefSect->offset], RWORD(pHdr[e].p_filesz));
    }
    xefHdr.deflatedImageLength=xefHdr.imageLength;

    return 0;
}

static void ParseCustomFile(int fdIn) {
    xefHdr.noSegments=1;
    xefSegmentTab=0;
    xefCustomFileTab=0;
    xefRelTab=0;
    xefRelaTab=0;
    image=0;
    xefHdr.imageLength=0;
    xefHdr.flags&=~XEF_TYPE_MASK;
    xefHdr.flags|=XEF_TYPE_CUSTOMFILE;

    // An xmImage has only one segment
    DO_REALLOC(xefSegmentTab, xefHdr.noSegments*sizeof(struct xefSegment));
    xefHdr.noCustomFiles=0;
 
    xefSegmentTab[0].physAddr=0;
    xefSegmentTab[0].virtAddr=0;    
    xefSegmentTab[0].fileSize=lseek(fdIn, 0, SEEK_END);
    xefSegmentTab[0].deflatedFileSize=xefSegmentTab[0].fileSize;
    xefSegmentTab[0].offset=xefHdr.imageLength;
    xefHdr.imageLength+=xefSegmentTab[0].fileSize;
    DO_REALLOC(image, xefHdr.imageLength);
    lseek(fdIn, 0, SEEK_SET);
    DO_READ(fdIn, image, xefSegmentTab[0].fileSize);
    xefHdr.deflatedImageLength=xefHdr.imageLength;
}

/*
static void ParseXmImage(int fdIn, xm_u32_t signature) {
    struct xmImageHdr xmImgHdr;
    struct xefCustomFile *customFileTab;
    struct xmHdr xmHdr;
    xm_s32_t e;

    xefHdr.noSegments=1;
    xefSegmentTab=0;
    xefCustomFileTab=0;
    xefRelTab=0;
    xefRelaTab=0;
    image=0;
    xefHdr.imageLength=0;

    // An xmImage has only one segment
    DO_REALLOC(xefSegmentTab, xefHdr.noSegments*sizeof(struct xefSegment));
    lseek(fdIn, 0, SEEK_SET);
    DO_READ(fdIn, &xmImgHdr, sizeof(struct xmImageHdr));

    if (signature==XMEF_XM_MAGIC) {
	lseek(fdIn, 0, SEEK_SET);
	DO_READ(fdIn, &xmHdr, sizeof(struct xmHdr));
	xefHdr.noCustomFiles=RWORD(xmHdr.noCustomFiles);
	customFileTab=xmHdr.customFileTab;
    } else {
	xefHdr.noCustomFiles=RWORD(xmImgHdr.noCustomFiles);
	customFileTab=xmImgHdr.customFileTab;
    }
    DO_MALLOC(xefCustomFileTab, sizeof(struct xefCustomFile)*xefHdr.noCustomFiles);
    xefSegmentTab[0].physAddr=RWORD(xmImgHdr.sAddr);
    xefSegmentTab[0].virtAddr=RWORD(xmImgHdr.sAddr);
    xefSegmentTab[0].fileSize=lseek(fdIn, 0, SEEK_END);
    xefSegmentTab[0].deflatedFileSize=xefSegmentTab[0].fileSize;
    xefSegmentTab[0].offset=xefHdr.imageLength;
    xefHdr.imageLength+=xefSegmentTab[0].fileSize;
    DO_REALLOC(image, xefHdr.imageLength);
    lseek(fdIn, 0, SEEK_SET);
    DO_READ(fdIn, image, xefSegmentTab[0].fileSize);
    xefHdr.deflatedImageLength=xefHdr.imageLength;


    for (e=0; e<xefHdr.noCustomFiles; e++) {
	xefCustomFileTab[e].sAddr=RWORD(customFileTab[e].sAddr);
	xefCustomFileTab[e].size=RWORD(customFileTab[e].size);
    }
}
*/

static xm_s32_t CRead(void *b, xmSize_t s, void *d) {
    memcpy(b, *(xm_u8_t **)d, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static xm_s32_t CWrite(void *b, xmSize_t s, void *d) {
    memcpy(*(xm_u8_t **)d, b, s);
    *(xm_u8_t **)d+=s;
    return s;
}

static void CSeek(xmSSize_t offset, void *d) {    
    *(xm_u8_t **)d+=offset;  
}

static void CompressImage(void) {
    xm_u32_t cLen, len;
    xm_u8_t *cImg;
    xm_s32_t e, size;
    size=xefHdr.imageLength*3;
    DO_MALLOC(cImg, size);
    for (e=0, cLen=0, len=0; e<xefHdr.noSegments; e++) {
        xm_u8_t *ptrImg=&image[len], *ptrCImg=&cImg[cLen];
	if ((xefSegmentTab[e].deflatedFileSize=Compress(xefSegmentTab[e].fileSize, size, CRead, &ptrImg, CWrite, &ptrCImg, CSeek))<=0)
	    EPrintF("Unable to perform compression\n");
	xefSegmentTab[e].offset=cLen;
	cLen+=xefSegmentTab[e].deflatedFileSize;
	len+=xefSegmentTab[e].fileSize;
	size-=cLen;
    }
    xefHdr.deflatedImageLength=cLen;
    free(image);
    image=cImg;
}

static void WriteXefImage(char *output, int fdOut) {
    int pos=0, e, fileSize;
    struct digestCtx digest_state;
    xm_u8_t *img;

    pos=sizeof(struct xefHdr);
    pos=ALIGNTO(pos, 8);
    xefHdr.segmentTabOffset=pos;
    for (e=0; e<xefHdr.noSegments; e++) {
	xefSegmentTab[e].physAddr=RWORD(xefSegmentTab[e].physAddr);
	xefSegmentTab[e].virtAddr=RWORD(xefSegmentTab[e].virtAddr);
	xefSegmentTab[e].fileSize=RWORD(xefSegmentTab[e].fileSize);
	xefSegmentTab[e].deflatedFileSize=RWORD(xefSegmentTab[e].deflatedFileSize);
	xefSegmentTab[e].offset=RWORD(xefSegmentTab[e].offset);
    }

    lseek(fdOut, xefHdr.segmentTabOffset, SEEK_SET);
    DO_WRITE(fdOut, xefSegmentTab, sizeof(struct xefSegment)*xefHdr.noSegments);

    pos+=sizeof(struct xefSegment)*xefHdr.noSegments;
    pos=ALIGNTO(pos, 8);
    xefHdr.customFileTabOffset=pos;
    for (e=0; e<xefHdr.noCustomFiles; e++) {
	xefCustomFileTab[e].sAddr=RWORD(xefCustomFileTab[e].sAddr);
	xefCustomFileTab[e].size=RWORD(xefCustomFileTab[e].size);
    }
    lseek(fdOut, xefHdr.customFileTabOffset, SEEK_SET);
    DO_WRITE(fdOut, xefCustomFileTab, sizeof(struct xefCustomFile)*xefHdr.noCustomFiles);
    pos+=sizeof(struct xefCustomFile)*xefHdr.noCustomFiles;
    pos=ALIGNTO(pos, 8);
    xefHdr.imageOffset=pos;
    lseek(fdOut, xefHdr.imageOffset, SEEK_SET);
    DO_WRITE(fdOut, image, xefHdr.deflatedImageLength);    

    xefHdr.flags|=XEF_DIGEST;
#ifdef CONFIG_SPARCv8
    xefHdr.flags|=XEF_ARCH_SPARCv8;
#endif
    xefHdr.flags=RWORD(xefHdr.flags);
    fileSize=lseek(fdOut, 0, SEEK_END);
    if (fileSize&3){ // Filling the xef with padding
	xm_u32_t padding=0;
	DO_WRITE(fdOut, &padding, 4-(fileSize&3));
	fileSize=lseek(fdOut, 0, SEEK_END);
    }

    xefHdr.fileSize=RWORD(fileSize);
    xefHdr.segmentTabOffset=RWORD(xefHdr.segmentTabOffset);
    xefHdr.noSegments=RWORD(xefHdr.noSegments);
    xefHdr.imageLength=RWORD(xefHdr.imageLength);
    xefHdr.deflatedImageLength=RWORD(xefHdr.deflatedImageLength);
    xefHdr.customFileTabOffset=RWORD(xefHdr.customFileTabOffset);
    xefHdr.noCustomFiles=RWORD(xefHdr.noCustomFiles);
    xefHdr.imageOffset=RWORD(xefHdr.imageOffset);
    xefHdr.xmImageHdr=RWORD(xefHdr.xmImageHdr);
    xefHdr.pageTable=RWORD(xefHdr.pageTable);
    xefHdr.pageTableSize=RWORD(xefHdr.pageTableSize);
    lseek(fdOut, 0, SEEK_SET);
    DO_WRITE(fdOut, &xefHdr,  sizeof(xefHdr));
    
    DO_MALLOC(img, fileSize);
    lseek(fdOut, 0, SEEK_SET);
    DO_READ(fdOut, img, fileSize);
    DigestInit(&digest_state);
    DigestUpdate(&digest_state, img, fileSize);
    DigestFinal(xefHdr.digest, &digest_state);
    free(img);
    for (e=0; e<XM_DIGEST_BYTES; e++)
        fprintf(stderr, "%02x", xefHdr.digest[e]);

    fprintf(stderr, " %s\n", output);
    
    lseek(fdOut, 0, SEEK_SET);
    DO_WRITE(fdOut, &xefHdr,  sizeof(xefHdr));
}

static int DoBuild(int argc, char **argv) {
    int fdIn, fdOut, opt, compressed=0, customFile=0;
    char *output, stdOutput[]="a.out", *payload=0;
    xm_u32_t signature;
    output=stdOutput;

    while ((opt=getopt(argc, argv, "o:cp:m")) != -1) {
        switch (opt) {
        case 'o':
	    DO_MALLOC(output, strlen(optarg)+1);
            strcpy(output, optarg);
            break;
	case 'c':
	    compressed=1;
	    break;
	case 'p':
	    DO_MALLOC(payload, strlen(optarg)+1);
            strcpy(payload, optarg);
	    break;
        case 'm':
            customFile=1;
            break;
        default: /* ? */
	    fprintf(stderr, USAGE);
	    return -2;
        }
    }

    if ((argc-optind)!=1) {
	fprintf(stderr, USAGE);
	return -2;
    }

    if ((fdIn=open(argv[optind], O_RDONLY))<0)
	EPrintF("Unable to open %s\n", argv[optind]);

    memset(&xefHdr, 0, sizeof(struct xefHdr));
    xefHdr.signature=RWORD(XEF_SIGNATURE);
    xefHdr.version=RWORD(XM_SET_VERSION(XEF_VERSION, XEF_SUBVERSION, XEF_REVISION));

    if (customFile)
        ParseCustomFile(fdIn);
    else {
        DO_READ(fdIn, &signature, sizeof(xm_u32_t));
        switch(signature) {
        case ELFMAG3<<24|ELFMAG2<<16|ELFMAG1<<8|ELFMAG0:
            ParseElfImage(fdIn);
            break;
/*    case XMEF_XM_MAGIC:
      case XMEF_PARTITION_MAGIC:
      ParseXmImage(fdIn, signature);
      break;
*/
        default:
            EPrintF("Signature 0x%x unknown\n", signature);
        }
    }
    if (payload) {
        int e, bytes, fdPayload;
        if ((fdPayload=open(payload, O_RDONLY))<0)
            EPrintF("Unable to open payload file %s\n", payload);
        bytes=read(fdPayload, &xefHdr.payload, XM_PAYLOAD_BYTES);
        close(fdPayload);
        for(e=0; e<XM_PAYLOAD_BYTES; e++) 
            if (!((e+1)%8))
                fprintf(stderr, "%02x\n", xefHdr.payload[e]);
            else
                fprintf(stderr, "%02x ", xefHdr.payload[e]);
    }
    
    if (compressed) {
        CompressImage();
        xefHdr.flags|=XEF_COMPRESSED;
    }
    
    if ((fdOut=open(output, O_CREAT|O_RDWR|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP))<0)
        EPrintF("Unable to open %s\n", output);   
        
    WriteXefImage(output, fdOut);
    return 0;
}

#define TAB "  "

static void PrintHeader(void) {
    int e;
    fprintf(stderr, "XEF header:\n");
    fprintf(stderr, TAB"signature: 0x%x\n", RWORD(xefHdrRead->signature));
    fprintf(stderr, TAB"version: %d.%d.%d\n", XM_GET_VERSION(RWORD(xefHdrRead->version)), XM_GET_SUBVERSION(RWORD(xefHdrRead->version)), XM_GET_REVISION(RWORD(xefHdrRead->version)));
    fprintf(stderr, TAB"flags: ");
    if(RWORD(xefHdrRead->flags)&XEF_DIGEST)
	fprintf(stderr, "XEF_DIGEST ");
    if(RWORD(xefHdrRead->flags)&XEF_COMPRESSED)
	fprintf(stderr, "XEF_COMPRESSED ");    

    switch(RWORD(xefHdrRead->flags)&XEF_TYPE_MASK) {
    case XEF_TYPE_HYPERVISOR:
        fprintf(stderr, "XEF_TYPE_HYPERVISOR ");
        break;
    case XEF_TYPE_PARTITION:
        fprintf(stderr, "XEF_TYPE_PARTITION ");
        break;
    case XEF_TYPE_CUSTOMFILE:
        fprintf(stderr, "XEF_TYPE_CUSTOMFILE ");
      break;
    }
    switch(RWORD(xefHdrRead->flags)&XEF_ARCH_MASK) {
    case XEF_ARCH_SPARCv8:
        fprintf(stderr, "XEF_ARCH_SPARCv8 ");
        break;
    }
    fprintf(stderr, "\n");
    if (RWORD(xefHdrRead->flags)&XEF_DIGEST) {
	fprintf(stderr, TAB"digest: ");
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    fprintf(stderr, "%02x", xefHdrRead->digest[e]);
	fprintf(stderr, "\n");
    }
    fprintf(stderr, TAB"payload: ");
    for(e=0; e<XM_PAYLOAD_BYTES; e++) 
	if ((e+1)!=XM_PAYLOAD_BYTES&&!((e+1)%8))
	    fprintf(stderr, "%02x\n"TAB"         ", xefHdrRead->payload[e]);
	else
	    fprintf(stderr, "%02x ", xefHdrRead->payload[e]);
    fprintf(stderr, "\n");
    fprintf(stderr, TAB"file size: %"PRINT_PREF"u\n", RWORD(xefHdrRead->fileSize));
    fprintf(stderr, TAB"segment table offset: %"PRINT_PREF"u\n", RWORD(xefHdrRead->segmentTabOffset));
    fprintf(stderr, TAB"no. segments: %d\n", RWORD(xefHdrRead->noSegments));
    fprintf(stderr, TAB"customFile table offset: %"PRINT_PREF"u\n", RWORD(xefHdrRead->customFileTabOffset));
    fprintf(stderr, TAB"no. customFiles: %d\n", RWORD(xefHdrRead->noCustomFiles));
    fprintf(stderr, TAB"image offset: %"PRINT_PREF"d\n", RWORD(xefHdrRead->imageOffset));
    fprintf(stderr, TAB"image length: %"PRINT_PREF"d\n", RWORD(xefHdrRead->imageLength));
    if (RWORD(xefHdrRead->flags)&XEF_TYPE_PARTITION)
        fprintf(stderr, TAB"page table: [0x%"PRINT_PREF"x - 0x%"PRINT_PREF"x]\n", RWORD(xefHdrRead->pageTable), RWORD(xefHdrRead->pageTable)+RWORD(xefHdrRead->pageTableSize));
    fprintf(stderr, TAB"XM image's header: 0x%"PRINT_PREF"x\n", RWORD(xefHdrRead->xmImageHdr));
    fprintf(stderr, TAB"entry point: 0x%"PRINT_PREF"x\n", RWORD(xefHdrRead->entryPoint));
    if (RWORD(xefHdrRead->flags)&XEF_COMPRESSED)
	fprintf(stderr, TAB"compressed image length: %"PRINT_PREF"d (%.2f%%)\n", RWORD(xefHdrRead->deflatedImageLength), 100.0*(float)RWORD(xefHdrRead->deflatedImageLength)/(float)RWORD(xefHdrRead->imageLength));
    
}

static void PrintSegments(void) {
    int e;
    fprintf(stderr, "Segment table: %d segments\n", RWORD(xefHdrRead->noSegments));
    for (e=0; e<RWORD(xefHdrRead->noSegments); e++) {
	fprintf(stderr, TAB"segment %d\n", e);
	fprintf(stderr, TAB TAB"physical address: 0x%x\n", RWORD(xefSegmentTab[e].physAddr));
	fprintf(stderr, TAB TAB"virtual address: 0x%"PRINT_PREF"x\n", RWORD(xefSegmentTab[e].virtAddr));        
	fprintf(stderr, TAB TAB"file size: %"PRINT_PREF"d\n", RWORD(xefSegmentTab[e].fileSize));
	if (RWORD(xefHdrRead->flags)&XEF_COMPRESSED)
	    fprintf(stderr, TAB TAB"compressed file size: %"PRINT_PREF"d (%.2f%%)\n", RWORD(xefSegmentTab[e].deflatedFileSize), 100.0*(float)RWORD(xefSegmentTab[e].deflatedFileSize)/(float)RWORD(xefSegmentTab[e].fileSize));
    }
}

static void PrintCustomFiles(void) {
    int e;
    fprintf(stderr, "CustomFile table: %d customFiles\n", RWORD(xefHdrRead->noCustomFiles));
    for (e=0; e<RWORD(xefHdrRead->noCustomFiles); e++) {
	fprintf(stderr, TAB"customFile %d\n", e);
	fprintf(stderr, TAB TAB"address: 0x%"PRINT_PREF"x\n", RWORD(xefCustomFileTab[e].sAddr));
	if (!RWORD(xefCustomFileTab[e].size))
	    fprintf(stderr, TAB TAB"undefined file size\n");
	else
	    fprintf(stderr, TAB TAB"file size: %"PRINT_PREF"d\n", RWORD(xefCustomFileTab[e].size));
    }
}

static void ParseXef(char *file, int fdIn) {
    struct digestCtx digest_state;
    int fileSize, e;
    unsigned char *img;
    static xm_u8_t digest[XM_DIGEST_BYTES];

    fileSize=lseek(fdIn, 0, SEEK_END);
    DO_MALLOC(img, fileSize);
    lseek(fdIn, 0, SEEK_SET);
    DO_READ(fdIn, img, fileSize);
    xefHdrRead=(struct xefHdr *)img;
    xefSegmentTab=(struct xefSegment *)(img+RWORD(xefHdrRead->segmentTabOffset));
    xefCustomFileTab=(struct xefCustomFile *)(img+RWORD(xefHdrRead->customFileTabOffset));
    if(xefHdrRead->signature!=RWORD(XEF_SIGNATURE))
	EPrintF("Not a XEF file. Wrong signature (0x%x)\n", xefHdrRead->signature);
    if(xefHdrRead->fileSize!=RWORD(fileSize))
	EPrintF("Wrong file sile %d - expected %d\n", RWORD(xefHdrRead->fileSize), fileSize);
    DigestInit(&digest_state);
    DigestUpdate(&digest_state, img, offsetof(struct xefHdr, digest));
    DigestUpdate(&digest_state, (xm_u8_t *)digest, XM_DIGEST_BYTES);
    DigestUpdate(&digest_state, &img[offsetof(struct xefHdr, payload)], fileSize-offsetof(struct xefHdr, payload));
    DigestFinal(digest, &digest_state);
    for (e=0; e<XM_DIGEST_BYTES; e++)
	if (digest[e]!=xefHdrRead->digest[e])
	    EPrintF("Wrong digest - file corrupted?\n");

    if ((RWORD(xefHdrRead->segmentTabOffset)>fileSize)||((RWORD(xefHdrRead->segmentTabOffset)+RWORD(xefHdrRead->noSegments)*sizeof(struct xefSegment))>fileSize))
	EPrintF("Segment table beyond file?\n");
    if ((RWORD(xefHdrRead->imageOffset)>fileSize)||((RWORD(xefHdrRead->imageOffset)+RWORD(xefHdrRead->deflatedImageLength))>fileSize))
	EPrintF("Image beyond file?\n");
    if ((RWORD(xefHdrRead->customFileTabOffset)>fileSize)||((RWORD(xefHdrRead->customFileTabOffset)+RWORD(xefHdrRead->noCustomFiles)*sizeof(struct xefCustomFile))>fileSize))
	EPrintF("CustomFile table beyond file?\n");
}

static int DoRead(int argc, char **argv) {
    int header=0, fdIn, opt, segments=0, customFiles=0;

    if (argc<2) {
	fprintf(stderr, USAGE);
	return -2;
    }

    while ((opt=getopt(argc, argv, "hsm")) != -1) {
        switch (opt) {
	case 'h':
	    header=1;
	    break;
	case 's':
	    segments=1;
	    break;
	case 'm':
	    customFiles=1;
	    break;
        default: /* ? */
	    fprintf(stderr, USAGE);
	    return -2;
        }
    }

    if ((argc-optind)!=1) {
	fprintf(stderr, USAGE);
	return -2;
    }

    if ((fdIn=open(argv[optind], O_RDONLY))<0)
	EPrintF("Unable to open %s\n", argv[optind]);

    ParseXef(argv[optind], fdIn);
    if (!header&&!segments&&!customFiles) {
	fprintf(stderr, USAGE);
	return -2;
    }

    if(header)
	PrintHeader();
    if(segments)
	PrintSegments();
    if(customFiles)
	PrintCustomFiles();
    close(fdIn);

    return 0;
}

int main(int argc, char **argv) {
    char **_argv;
    int e, i;

    if (argc<2) {
	fprintf(stderr, USAGE);
	return -2;
    }
    
    _argv=malloc(sizeof(char *)*(argc-1));
    for (e=0, i=0; e<argc; e++) {
	if (e==1) continue;
	_argv[i]=strdup(argv[e]);
	i++;
    }

    if (!strcasecmp(argv[1], "read")) {
	DoRead(argc-1, _argv);
	exit(0);
    }
    if (!strcasecmp(argv[1], "build")) {
	DoBuild(argc-1, _argv);
	exit(0);
    }
    
    fprintf(stderr, USAGE);
    return 0;
}
