/*
 * $FILE: xmpack
 *
 * Create a pack holding the image of XM and partitions to be written
 * into the ROM
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
#include <xm_inc/arch/arch_types.h>
#include <xm_inc/xmef.h>

#include <xm_inc/compress.h>
#include <xm_inc/digest.h>

#include <endianess.h>
#include <xmpack.h>

#define PRINT_PREF

static struct xmefContainerHdr xmefContainerHdr;
static xm_u8_t *fileData=0, *fileImg=0;
static xm_u32_t fileDataLen=0;
static struct xmefFile *xmefFileTab=0;

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

struct xmefPartition *partitionTab=0, *hypervisor;

static struct file {
    char *name;
    unsigned int offset;
} *fileTab=0;

static int noFiles=0, noPartitions=0;

static inline int AddFile(char *fName, unsigned int offset) {
    int p=noFiles++;
    DO_REALLOC(fileTab, noFiles*sizeof(struct file));
    fileTab[p].name=strdup(fName);
    fileTab[p].offset=offset;
    return p;
}

static char *strTab=0;
static int strTabLen=0;

static inline int AddString(char *str) {
    int offset;
    offset=strTabLen;
    strTabLen+=strlen(str)+1;
    DO_REALLOC(strTab, strTabLen*sizeof(char));
    strcpy(&strTab[offset], str);

    return offset;
}

static void GetHypervisorInfo(char *in) {
    char *ptr, *savePtr=0, *atStr, *file;
    int e, d, pos=0;
    unsigned int at;
    int xmcFile=0;

    for (ptr=optarg, atStr=0, e=0; (file=strtok_r(ptr, ":", &savePtr)); ptr=0, e++) {
	d=(file[0]=='#')?1:0;
	if (!d) {
	    atStr=strpbrk(file, "@");
	    if (atStr) {
		*atStr=0;
		atStr++;
		at=strtoul(atStr, 0, 16);
	    } else
		at=0;
	    pos=AddFile(file, at);    
	} else {
	    d=strtoul(&file[1], 0, 10);
	    if ((d>=0)&&(d<noFiles)) {
		pos=d;
	    } else
		EPrintF("There isn't any file at position %d", d);
	}

	if (!e) {
            hypervisor->id=-1;
	    hypervisor->file=pos;
	    hypervisor->noCustomFiles=0;
	} else {
	    if ((hypervisor->noCustomFiles+1)>CONFIG_MAX_NO_CUSTOMFILES)
		EPrintF("Only %d customisation files are permitted", CONFIG_MAX_NO_CUSTOMFILES);	    
	    hypervisor->customFileTab[hypervisor->noCustomFiles]=pos;
	    if (!hypervisor->noCustomFiles)
		xmcFile++;
	    hypervisor->noCustomFiles++;	    
	}
    }
    if (!xmcFile)
	EPrintF("XM configuration (xmc) file is missed");
}

static void GetPartitionInfo(char *in) {
    char *ptr, *savePtr=0, *atStr, *file;
    struct xmefPartition *partition;
    int e, d, id=-1, pos=0;
    unsigned int at;

    noPartitions++;
    DO_REALLOC(partitionTab, noPartitions*sizeof(struct xmefPartition));
    partition=&partitionTab[noPartitions-1];

    for (ptr=optarg, atStr=0, e=0; (file=strtok_r(ptr, ":", &savePtr)); ptr=0, e++) {
	if (id<0) {
	    if (((id=strtoul(file, 0, 10))<0))
		EPrintF("Partition ID shall be a positive integer");

	    partition->id=id;
	    continue;
	}
	d=(file[0]=='#')?1:0;
	if (!d) {
	    atStr=strpbrk(file, "@");
	    if (atStr) {
		*atStr=0;
		atStr++;
		at=strtoul(atStr, 0, 16);
	    } else
		at=0;
	    pos=AddFile(file, at);
	} else {
	    d=strtoul(&file[1], 0, 10);
	    if ((d>=0)&&(d<noFiles))
		pos=d;
	    else
		EPrintF("There isn't any file at position %d", d);
	}

	if (e==1) {
	    partition->file=pos;
	    partition->noCustomFiles=0;
	} else {
	    if ((partition->noCustomFiles+1)>CONFIG_MAX_NO_CUSTOMFILES)
		EPrintF("Only %d customisation files are permitted", CONFIG_MAX_NO_CUSTOMFILES);

	    partition->customFileTab[partition->noCustomFiles]=pos;
	    partition->noCustomFiles++;
	}
    }
}

static void WriteContainerToFile(char *file) {
    int fd, fpos, pos=0, e, i, size, fileSize;
    struct digestCtx digest_state;
    unsigned char *img;

    if ((fd=open(file, O_CREAT|O_TRUNC|O_RDWR, S_IRUSR|S_IWUSR|S_IRGRP))<0)
        EPrintF("File \"%s\" couldn't be created", file);
    
    xmefContainerHdr.noPartitions=RWORD(xmefContainerHdr.noPartitions);
    xmefContainerHdr.noFiles=RWORD(xmefContainerHdr.noFiles);
    xmefContainerHdr.fileDataLen=RWORD(xmefContainerHdr.fileDataLen);

    for (e=0; e<noPartitions; e++) {
	partitionTab[e].id=RWORD(partitionTab[e].id);
	partitionTab[e].file=RWORD(partitionTab[e].file);
	for (i=0; i<partitionTab[e].noCustomFiles; i++) {
	    partitionTab[e].customFileTab[i]=RWORD(partitionTab[e].customFileTab[i]);
	}
	partitionTab[e].noCustomFiles=RWORD(partitionTab[e].noCustomFiles);
    }

    lseek(fd, sizeof(struct xmefContainerHdr), SEEK_SET);
    pos=sizeof(struct xmefContainerHdr);
    pos=ALIGNTO(pos, 8);
    lseek(fd, pos, SEEK_SET);
    xmefContainerHdr.partitionTabOffset=RWORD(pos);
    DO_WRITE(fd, partitionTab, sizeof(struct xmefPartition)*noPartitions);
    pos+=sizeof(struct xmefPartition)*noPartitions;
    
    pos=ALIGNTO(pos, 8);
    lseek(fd, pos, SEEK_SET);
    xmefContainerHdr.strTabOffset=RWORD(pos);
    xmefContainerHdr.strLen=RWORD(strTabLen);
    DO_WRITE(fd, strTab, strTabLen);
    pos+=strTabLen;

    pos=ALIGNTO(pos, 8);
    xmefContainerHdr.fileTabOffset=pos;
    pos+=sizeof(struct xmefFile)*noFiles;
    fpos=pos=ALIGNTO(pos, 8);
    
    for (e=0, size=0; e<noFiles; e++) {
	if (fileTab[e].offset) {
	    if (pos>fileTab[e].offset)
		EPrintF("Container couldn't be built with the specified offsets\n");
	    pos=fileTab[e].offset;
	    if (!e) fpos=pos;
	}
	pos=ALIGNTO(pos, PAGE_SIZE);
	xmefFileTab[e].offset=RWORD(pos);
	lseek(fd, pos, SEEK_SET);
	DO_WRITE(fd, &fileData[size], sizeof(xm_u8_t)*xmefFileTab[e].size);
	pos+=xmefFileTab[e].size;
	size+=xmefFileTab[e].size;
	xmefFileTab[e].size=RWORD(xmefFileTab[e].size);	
        xmefFileTab[e].nameOffset=RWORD(xmefFileTab[e].nameOffset);
    }
    
    xmefContainerHdr.fileDataOffset=RWORD(pos);
    lseek(fd, xmefContainerHdr.fileTabOffset, SEEK_SET);
    xmefContainerHdr.fileTabOffset=RWORD(xmefContainerHdr.fileTabOffset);
    DO_WRITE(fd, xmefFileTab, sizeof(struct xmefFile)*noFiles);    
    for (e=0; e<XM_DIGEST_BYTES; e++)
	xmefContainerHdr.digest[e]=0;
    fileSize=lseek(fd, 0, SEEK_END);
    if (fileSize&3){ // Filling the container with padding
	xm_u32_t padding=0;
	DO_WRITE(fd, &padding, 4-(fileSize&3));
	fileSize=lseek(fd, 0, SEEK_END);
    }
    xmefContainerHdr.flags=XMEF_CONTAINER_DIGEST;
    xmefContainerHdr.flags=RWORD(xmefContainerHdr.flags);
    xmefContainerHdr.fileSize=RWORD(fileSize);

    lseek(fd, 0, SEEK_SET);
    DO_WRITE(fd, &xmefContainerHdr, sizeof(struct xmefContainerHdr));

    lseek(fd, 0, SEEK_SET);
    img=malloc(fileSize);
    
    if (read(fd, img, fileSize)!=fileSize)
	EPrintF("Unable to read the container file");
    
    DigestInit(&digest_state);
    DigestUpdate(&digest_state, img, fileSize);
    DigestFinal(xmefContainerHdr.digest, &digest_state);
    free(img);
    /*for (e=0; e<XM_DIGEST_BYTES; e++)
	fprintf(stderr, "%02x", xmefContainerHdr.digest[e]);
    fprintf(stderr, " %s\n", file);
    */
    lseek(fd, 0, SEEK_SET);
    DO_WRITE(fd, &xmefContainerHdr, sizeof(struct xmefContainerHdr));
    close(fd);
}

static void DoBuild(int argc, char **argv) {
    extern void DoCheck2Build(int argc, char **argv);
    char *outFile=0;
    int opt, hyp=0, fd, e, ptr, r;
    char buffer[4096];

    DoCheck2Build(argc, argv);
    memset(&xmefContainerHdr, 0, sizeof(struct xmefContainerHdr));
    xmefContainerHdr.signature=RWORD(XM_PACKAGE_SIGNATURE);
    xmefContainerHdr.version=RWORD(XM_SET_VERSION(XMPACK_VERSION, XMPACK_SUBVERSION, XMPACK_REVISION));
    noPartitions++;
    DO_REALLOC(partitionTab, sizeof(struct xmefPartition));
    hypervisor=&partitionTab[0];
    while ((opt=getopt(argc, argv, "p:h:")) != -1) {
	switch (opt) {
	case 'p':
	    GetPartitionInfo(optarg);
	    break;
	case 'h':
	    if (!hyp) 
		hyp=1;
	    else
		EPrintF("XM hypervisor has been alredy defined");
	    
	    GetHypervisorInfo(optarg);
	    break;
	default: 
    	    fprintf(stderr, USAGE);
	    exit(2);
	}
    }

    if (!hyp)
	EPrintF("XM hypervisor missed");

    if ((argc-optind)!=1) {
	fprintf(stderr, USAGE);
	exit(2);
    }
    
    outFile=argv[optind];

    DO_REALLOC(xmefFileTab, sizeof(struct xmefFile)*noFiles);
    for (e=0; e<noFiles; e++) {
        char *fileName;
	if ((fd=open(fileTab[e].name, O_RDONLY))<0)
            EPrintF("File \"%s\" couldn't be opened", fileTab[e].name);
	xmefFileTab[e].size=lseek(fd, 0, SEEK_END);
	xmefFileTab[e].offset=fileDataLen;
        fileName=basename(strdup(fileTab[e].name));
        xmefFileTab[e].nameOffset=AddString(fileName);
	ptr=fileDataLen;
	fileDataLen+=xmefFileTab[e].size;
	//fileDataLen=ALIGNTO(fileDataLen, 8);
	DO_REALLOC(fileData, fileDataLen*sizeof(xm_u8_t));
	lseek(fd, 0, SEEK_SET);
        while((r=read(fd, buffer, 4096))) {
            memcpy(&fileData[ptr], buffer, r);
            ptr+=4096;
	}
	close(fd);
    }
    
    xmefContainerHdr.noPartitions=noPartitions;
    xmefContainerHdr.noFiles=noFiles;
    xmefContainerHdr.fileDataLen=fileDataLen;
    WriteContainerToFile(outFile);
}

static void ReadContainerFromFile(char *file) {
    xm_u8_t digest[XM_DIGEST_BYTES];
    struct digestCtx digest_state;
    int fd, r, e, i, fileSize;
    unsigned char *img;

    if ((fd=open(file, O_RDONLY))<0)
	EPrintF("File \"%s\" couldn't be opened", file);
    
    fileSize=lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);
    img=malloc(fileSize);
    
    if (read(fd, img, fileSize)!=fileSize)
	EPrintF("Unable to read %s", file);

    for (e=0; e<XM_DIGEST_BYTES; e++)
	((struct xmefContainerHdr *)img)->digest[e]=0;
    
    DigestInit(&digest_state);
    DigestUpdate(&digest_state, img, fileSize);
    DigestFinal((xm_u8_t *)digest, &digest_state);
    free(img);
    
    lseek(fd, 0, SEEK_SET);
    r=read(fd, &xmefContainerHdr, sizeof(struct xmefContainerHdr));
    xmefContainerHdr.signature=RWORD(xmefContainerHdr.signature);
    xmefContainerHdr.version=RWORD(xmefContainerHdr.version);
    xmefContainerHdr.flags=RWORD(xmefContainerHdr.flags);
    noPartitions=xmefContainerHdr.noPartitions=RWORD(xmefContainerHdr.noPartitions);
    xmefContainerHdr.partitionTabOffset=RWORD(xmefContainerHdr.partitionTabOffset);
    noFiles=xmefContainerHdr.noFiles=RWORD(xmefContainerHdr.noFiles);
    xmefContainerHdr.fileTabOffset=RWORD(xmefContainerHdr.fileTabOffset);
    xmefContainerHdr.fileDataOffset=RWORD(xmefContainerHdr.fileDataOffset);
    fileDataLen=xmefContainerHdr.fileDataLen=RWORD(xmefContainerHdr.fileDataLen);
    xmefContainerHdr.fileSize=RWORD(xmefContainerHdr.fileSize);
    xmefContainerHdr.strTabOffset=RWORD(xmefContainerHdr.strTabOffset);
    xmefContainerHdr.strLen=RWORD(xmefContainerHdr.strLen);
    strTabLen=xmefContainerHdr.strLen;
    
    if (xmefContainerHdr.signature!=XM_PACKAGE_SIGNATURE)
	EPrintF("\"%s\" is not a valid package", file);

    if (xmefContainerHdr.flags&XMEF_CONTAINER_DIGEST) {
	fprintf(stderr, "Digest: ");
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    fprintf(stderr, "%02x", digest[e]);
	fprintf(stderr, " %s ", file);
	for (e=0; e<XM_DIGEST_BYTES; e++)
	    if (digest[e]!=xmefContainerHdr.digest[e]) {
		fprintf(stderr, "\n");
		EPrintF("Incorrect digest: container corrupted\n");
	    }
	fprintf(stderr, " Ok\n");
    }
    DO_REALLOC(fileImg, xmefContainerHdr.fileSize);
    lseek(fd, 0, SEEK_SET);
    r=read(fd, fileImg, xmefContainerHdr.fileSize);
    DO_REALLOC(fileData, fileDataLen*sizeof(xm_u8_t));
    DO_REALLOC(partitionTab, noPartitions*sizeof(struct xmefPartition));
    DO_REALLOC(xmefFileTab, noFiles*sizeof(struct xmefFile));
    lseek(fd, xmefContainerHdr.partitionTabOffset, SEEK_SET);
    r=read(fd, partitionTab, noPartitions*sizeof(struct xmefPartition));    
    lseek(fd, xmefContainerHdr.fileTabOffset, SEEK_SET);
    r=read(fd, xmefFileTab, noFiles*sizeof(struct xmefFile));
    lseek(fd, xmefContainerHdr.fileDataOffset, SEEK_SET);
    r=read(fd, fileData, fileDataLen*sizeof(xm_u8_t));
    lseek(fd, xmefContainerHdr.strTabOffset, SEEK_SET);
    DO_MALLOC(strTab, strTabLen);
    r=read(fd, strTab, strTabLen);
    // Process all structures' content
    for (e=0; e<noPartitions; e++) {
	partitionTab[e].id=RWORD(partitionTab[e].id);
	partitionTab[e].file=RWORD(partitionTab[e].file);
	partitionTab[e].noCustomFiles=RWORD(partitionTab[e].noCustomFiles);
	for (i=0; i<partitionTab[e].noCustomFiles; i++)
	    partitionTab[e].customFileTab[i]=RWORD(partitionTab[e].customFileTab[i]);
    }
    for (e=0; e<noFiles; e++) {
	xmefFileTab[e].offset=RWORD(xmefFileTab[e].offset);
	xmefFileTab[e].size=RWORD(xmefFileTab[e].size);
	xmefFileTab[e].nameOffset=RWORD(xmefFileTab[e].nameOffset);
    }
}

#define _SP "    "

static void DoList(int argc, char **argv) {
    char *file=argv[argc-1];
    xm_s32_t e, i;
    if (argc!=2) {
	fprintf(stderr, USAGE);
	exit(0);
    }

    ReadContainerFromFile(file);
    //CheckContainer();

    fprintf(stderr, "<Container file=\"%s\" version=\"%d.%d.%d\" flags=\"0x%x\">\n", file, XM_GET_VERSION(xmefContainerHdr.version), XM_GET_SUBVERSION(xmefContainerHdr.version), XM_GET_REVISION(xmefContainerHdr.version), xmefContainerHdr.flags);
    for (e=0; e<xmefContainerHdr.noPartitions; e++) {
	if (!e)
	    fprintf(stderr, _SP"<XMHypervisor ");
	else
	    fprintf(stderr, _SP"<Partition id=\"%d\" ", partitionTab[e].id);
	
	fprintf(stderr, "file=\"%d\"", partitionTab[e].file);
	fprintf(stderr, ">\n");
	for (i=0; i<partitionTab[e].noCustomFiles; i++) {
	    fprintf(stderr, _SP _SP"<CustomFile ");
	    fprintf(stderr, "file=\"%d\" ", partitionTab[e].customFileTab[i]);
	    fprintf(stderr, "/>\n");
	}
	if (!e)
	    fprintf(stderr, _SP"</XMHypervisor>\n");
	else
	    fprintf(stderr, _SP"</Partition>\n");
    }
    fprintf(stderr, _SP"<FileTable>\n");
    for (e=0; e<xmefContainerHdr.noFiles; e++) {
	fprintf(stderr, _SP _SP"<File entry=\"%d\" name=\"%s\" ", e, &strTab[xmefFileTab[e].nameOffset]);
	fprintf(stderr, "offset=\"0x%"PRINT_PREF"x\" ", xmefFileTab[e].offset);
	fprintf(stderr, "size=\"%"PRINT_PREF"d\" ",  xmefFileTab[e].size);
	fprintf(stderr, "/>\n");
    }
    fprintf(stderr, _SP"</FileTable>\n");
    fprintf(stderr, "</Container>\n");
}

extern void DoCheck(int argc, char **argv);

int main(int argc, char **argv) {
    char **_argv;
    int e, i;
    if (argc<2){
	fprintf(stderr, USAGE);
	exit(2);
    }
    
    _argv=malloc(sizeof(char *)*(argc-1));
    for (e=0, i=0; e<argc; e++) {
	if (e==1) continue;
	_argv[i]=strdup(argv[e]);
	i++;
    }

    if (!strcasecmp(argv[1], "list")) {
	DoList(argc-1, _argv);
	exit(0);
    }    
	
    if (!strcasecmp(argv[1], "build")) {
	DoBuild(argc-1, _argv);
	exit(0);
    }
    
    if (!strcasecmp(argv[1], "check")) {
	DoCheck(argc-1, _argv);
	exit(0);
    } 

    fprintf(stderr, USAGE);
    return 0;
}
