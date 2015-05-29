/*
 * $FILE: xmcbuild.c
 *
 * Compile the c code
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common.h"

#include <endianess.h>
#include <xm_inc/digest.h>
#include <xm_inc/xmconf.h>

//#define CMD_TMPL "%s/xmcbuild.sh %s %s"
//#define CMD2_TMPL "xmcbuild.sh %s %s"

#define CONFIG_FILE "xmconfig"

#define CONFIG_PATH "%s/"CONFIG_FILE

#define BUFFER_SIZE 256
#define CMD ". %s ;  ${TARGET_CC} ${TARGET_CFLAGS_ARCH} -x c %s --include xm_inc/config.h --include xm_inc/arch/arch_types.h %s -o %s -Wl,--entry=0x0,-T%s\n"
#define CFLAGS "-O2 -Wall -I${XTRATUM_PATH}/user/libxm/include -D\"__XM_INCFLD(_fld)=<xm_inc/_fld>\" -I${XTRATUM_PATH}/include -nostdlib -nostdinc"

static char ldsContent[]="OUTPUT_FORMAT(\"binary\")\n" \
    "SECTIONS\n" \
    "{\n" \
    "         . = 0x0;\n" \
    "         .data ALIGN (8) : {\n" \
    "      	     *(.rodata.hdr)\n" \
    "    	     *(.rodata)\n" \
    "    	     *(.data)\n" \
    "                _memObjTab = .;\n" \
    "                *(.data.memobj)\n" \
    "                LONG(0);\n" \
    "        }\n" \
    "\n" \
    "        _dataSize = .;\n"   \
   "\n" \
    "        .bss ALIGN (8) : {\n" \
    "                *(.bss)\n" \
    "    	     *(.bss.mempool)\n" \
    "    	}\n" \
    "\n" \
    "    	_xmcSize = .;\n" \
    "\n" \
    " 	/DISCARD/ : {\n" \
    "	   	*(.text)\n" \
    "    	*(.note)\n" \
    "	    	*(.comment*)\n" \
    "	}\n" \
    "}\n";

static void WriteLdsFile(char *ldsFile) {    
    int fd=mkstemp(ldsFile);

    if ((fd<0)||(write(fd, ldsContent, strlen(ldsContent))!=strlen(ldsContent)))
	EPrintF("unable to create the lds file\n");
    close(fd);
}

#define DEV_INC_PATH "/user/libxm/include"
#define INST_INC_PATH "/include"

void ExecXmcBuild(char *path, char *in, char *out) {
    char *xmPath, *configPath, *sysCmd;
    char ldsFile[]="ldsXXXXXX";
    
    if (!(xmPath=getenv("XTRATUM_PATH")))
        EPrintF("The XTRATUM_PATH enviroment variable must be set\n");
    
    DO_MALLOC(configPath, strlen(CONFIG_PATH)+strlen(xmPath)+1);
    sprintf(configPath, CONFIG_PATH, xmPath);
    WriteLdsFile(ldsFile);
    DO_MALLOC(sysCmd, strlen(CMD)+strlen(configPath)+strlen(CFLAGS)+strlen(in)+strlen(out)+strlen(ldsFile)+1);
    sprintf(sysCmd, CMD, configPath, CFLAGS, in, out, ldsFile);
    fprintf(stderr, "%s", sysCmd);
    if (system(sysCmd)) fprintf(stderr, "Error building xmc file\n");
    unlink(ldsFile);
}

void CalcDigest(char *out) {
    xm_u8_t digest[XM_DIGEST_BYTES];
    struct digestCtx digestState;
    xm_u32_t signature;
    xmSize_t dataSize;
    xm_u8_t *buffer;
    int fd;

    memset(digest, 0, XM_DIGEST_BYTES);
    if ((fd=open(out, O_RDWR))<0)
        EPrintF("File %s cannot be opened\n", out);

    fsync(fd);
    DO_READ(fd, &signature, sizeof(xm_u32_t));
    if (RWORD(signature)!=XMC_SIGNATURE)
        EPrintF("File signature unknown (%x)\n", signature);

    lseek(fd, offsetof(struct xmc, dataSize), SEEK_SET);
    
    DO_READ(fd, &dataSize, sizeof(xmSize_t));
    dataSize=RWORD(dataSize);
    DO_MALLOC(buffer, dataSize);
    lseek(fd, 0, SEEK_SET);
    DO_READ(fd, buffer, dataSize);
    DigestInit(&digestState);
    DigestUpdate(&digestState, buffer, offsetof(struct xmc, digest));
    DigestUpdate(&digestState, (xm_u8_t *)digest, XM_DIGEST_BYTES);
    DigestUpdate(&digestState, &buffer[offsetof(struct xmc, dataSize)], dataSize-offsetof(struct xmc, dataSize));
    DigestFinal(digest, &digestState);
    free(buffer);

    lseek(fd, offsetof(struct xmc, digest), SEEK_SET);
    DO_WRITE(fd, digest, XM_DIGEST_BYTES);
    close(fd);
}
