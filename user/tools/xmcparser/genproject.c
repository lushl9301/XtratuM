/*
 * $FILE: genproject.c
 *
 * Generates the project skeleton
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <sys/stat.h>
#include <sys/types.h>
#include <stdarg.h>
#include <libgen.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "xmc.h"
#include "common.h"

#include "prj-skel/makefile.root.h"
#include "prj-skel/rules.mk.h"
#include "prj-skel/config.mk.h"

#include "prj-skel/makefile.partition.h"
#include "prj-skel/boot.sparcv8.S.h"
#include "prj-skel/partition.cfg.h"
#include "prj-skel/main.c.h"
#include "prj-skel/traps.c.h"
#include "prj-skel/lds.h"

//extern char *xmDir;

#define BUFFER_LEN 512
/*
static inline void CpFile(char *dst, char *src) {
    char buffer[BUFFER_LEN];
    int fdSrc, fdDst, len, wr;
    if ((fdSrc=open(src, O_RDONLY))<0) {
	fprintf(stderr, "%s cannot be opened\n", src);
	exit(-1);
    }
    if ((fdDst=open(dst, O_WRONLY|O_CREAT, 0644))<0) {
	fprintf(stderr, "%s cannot be opened\n", dst);
	exit(-1);
    }
    while((len=read(fdSrc, buffer, BUFFER_LEN)))
	wr=write(fdDst, buffer, len);
    close(fdSrc);
    close(fdDst);
}
*/
static inline void CopyFile(char *dst, char *src) {
    char buffer[BUFFER_LEN];
    int fdSrc, fdDst, len, wr;
    char *dstFile;
    printf("src: %s dst: %s \n", src, dst);
    if ((fdSrc=open(src, O_RDONLY))<0)
	EPrintF("\"%s\" couldn't be opened\n", src);

    if (dst[strlen(dst)-1]!='/')
	dstFile=dst;
    else {
	char *tmp=strdup(src), *srcFile=tmp;
	srcFile=basename(srcFile);
	printf("bn: %s\n", srcFile);
	dstFile=malloc(strlen(dst)+strlen(srcFile)+1);
	sprintf(dstFile, "%s%s", dst, srcFile);
	free(tmp);
    }

    if ((fdDst=open(dstFile, O_WRONLY|O_CREAT, 0644))<0) {
	fprintf(stderr, "%s couldn't be created\n", dst);
	exit(-1);
    }
    while((len=read(fdSrc, buffer, BUFFER_LEN)))
	wr=write(fdDst, buffer, len);
    close(fdSrc);
    close(fdDst);

}

static inline char *CDirPath(char *path, char *str) {
    char *b;

    DO_MALLOC(b, strlen(path)+strlen(str)+3);
    sprintf(b, "%s/%s/", path, str);

    return b;
}

static inline char *CatDirPath(char *path, char *str) {
/*    char *b;
    //b=strdup(path);
    b=malloc(b, strlen(path)+strlen(str)+3);
    sprintf(b, "%s/%s/", path, str);

    return b;    */
}

static inline void CreateFile(char *path, char *file, char *content, ...) {
    va_list args;
    char *filePrj;
    FILE *f;

    filePrj=CatPath(path, file);
    f=fopen(filePrj, "w+");
    va_start(args, content);
    vfprintf(f, content, args);
    va_end(args);
    fclose(f);
    free(filePrj);
}

void GenerateProject(char *inFile, char *root) {
    char *prjFolder=0, *partFolder, *filePrj, *tmp, *tmp2, *partList, *xmpackPartList;
    int e;

    prjFolder=CDirPath(root, &strTab[xmc.nameOffset]);
    if (mkdir(prjFolder, 0744)) {
	fprintf(stderr, "Folder %s couldn't be created\n", prjFolder);
	exit(-1);
    }
    
    CopyFile(prjFolder, inFile);
    
    /* for (xmpackPartList=0, partList=0, e=0; e<xmc.noPartitions; e++) {
	if (!partList) {
	    DO_REALLOC(xmpackPartList, 2*strlen(&strTab[xmcPartitionTab[e].nameOffset])+13);
	    DO_REALLOC(partList, strlen(&strTab[xmcPartitionTab[e].nameOffset])+1);
	    strcpy(partList, &strTab[xmcPartitionTab[e].nameOffset]);
	    sprintf(xmpackPartList, "-b %s.bin:%s.cfg\n", )
	    strcpy(xmpackPartList, "-b ");
	    strcat(xmpackPartList, &strTab[xmcPartitionTab[e].nameOffset]);
	    strcat(xmpackPartList, ".bin:");
	    strcat(xmpackPartList, &strTab[xmcPartitionTab[e].nameOffset]);
	    strcat(xmpackPartList, ".cfg");
	} else {
	    DO_REALLOC(partList, strlen(&strTab[xmcPartitionTab[e].nameOffset])+ strlen(partList)+2);
	    DO_REALLOC(xmpackPartList, 2*strlen(&strTab[xmcPartitionTab[e].nameOffset])+14);
	    strcat(partList, " ");
	    strcat(partList, &strTab[xmcPartitionTab[e].nameOffset]);
	    strcat(xmpackPartList, " -b ");
	    strcat(xmpackPartList, &strTab[xmcPartitionTab[e].nameOffset]);
	    strcat(xmpackPartList, ".bin:");
	    strcat(xmpackPartList, &strTab[xmcPartitionTab[e].nameOffset]);
	    strcat(xmpackPartList, ".cfg");
	}
    */
    return 0;
	partFolder=CatPath(prjFolder, &strTab[xmcPartitionTab[e].nameOffset]);
	if (mkdir(partFolder, 0744)) {
	    fprintf(stderr, "Folder %s cannot be created\n", partFolder);
	    exit(-1);
	}
	CreateFile(partFolder, "Makefile", makefilePartitionFile,  &strTab[xmcPartitionTab[e].nameOffset],  &strTab[xmcPartitionTab[e].nameOffset]);
	CreateFile(partFolder, "boot.S", bootFile);
	CreateFile(partFolder, "main.c", mainFile);
	CreateFile(partFolder, "traps.c", trapsFile);
	CreateFile(partFolder, "partition.lds", ldsFile, xmcPartitionTab[e].binary.loadPhysAddr);
	tmp2=strdup(&strTab[xmcPartitionTab[e].nameOffset]);
	tmp2=realloc(tmp2, strlen(tmp2)+5);
	strcat(tmp2, ".cfg");
	CreateFile(partFolder, tmp2, partitionCfgFile);
	free(tmp2);
	free(partFolder);
    }
    CreateFile(prjFolder, "Makefile", makefileRootFile, partList, xmpackPartList);
    //CreateFile(prjFolder, "config.mk", configMkRootFile, xmDir);
    CreateFile(prjFolder, "rules.mk", rulesMkRootFile);
    free(prjFolder);
    free(partList);
}
