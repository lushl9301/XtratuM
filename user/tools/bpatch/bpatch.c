/*
 * $FILE: bpatch.c
 *
 * Binary patcher
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#if (__BYTE_ORDER== __LITTLE_ENDIAN)
#define TO_LITTLE_ENDIAN(i) i
#define TO_BIG_ENDIAN(i) ((i&0xff)<<24)+((i&0xff00)<<8)+((i&0xff0000)>>8)+((i>>24)&0xff)
#else
#define TO_LITTLE_ENDIAN(i) ((i&0xff)<<24)+((i&0xff00)<<8)+((i&0xff0000)>>8)+((i>>24)&0xff)
#define TO_BIG_ENDIAN(i) i
#endif

#define USAGE "USAGE:\nbatch -l|-b <offset>:<val>:<size> <file_in> <file_out>\n"

#define BLOCK_SIZE 512
static char buffer[BLOCK_SIZE];

int main(int argc, char *argv[]) {
    unsigned long offset, val;
    int t, fdIn, fdOut, size, w, little;
    if (argc!=5) {
	fprintf(stderr, USAGE);
	exit(-1);
    }

    if (!strcmp(argv[1], "-l")) {
	little=1;
    } else if (!strcmp(argv[1], "-b")) {
	little=0;
    } else {
	fprintf(stderr, USAGE);
	exit(-1);
    }
    
    sscanf(argv[2], "%lx:%lx:%d", &offset, &val, &t);
    
    fprintf(stderr, "Patching \"%s\" 0x%lx:0x%lx:%d\n", argv[3], offset, val, t);
 
    // Copy the file
    if ((fdIn=open(argv[3], O_RDWR))<0) {
	fprintf(stderr, "file \"%s\" cannot be opened\n", argv[3]);
	exit(-1);
    }
    if (strcmp(argv[3], argv[4])) {
	if ((fdOut=open(argv[4], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP))<0) {
	    fprintf(stderr, "file \"%s\" cannot be created\n", argv[4]);
	    exit(-1);
	}
	while((size=read(fdIn, buffer, BLOCK_SIZE))>0)
	    w=write(fdOut, buffer, size);
    } else
	fdOut=fdIn;

    if (little) {
	val=TO_LITTLE_ENDIAN(val);
    } else {
	val=TO_BIG_ENDIAN(val);
    }
    
    lseek(fdOut, offset, SEEK_SET);
    if (write(fdOut, &val, t)<t){
	fprintf(stderr, "error writting 0x%lx at offset 0x%lx\n", val, offset);
	exit(-1);
    }
    close(fdIn);
    close(fdOut);

    return 0;
}
