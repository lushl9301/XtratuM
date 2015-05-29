/*
 * $FILE: common.h
 *
 * Common definitions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include "stdio.h"

#define TAB "    "
#define ADD1TAB(x)      TAB x
#define ADD2TAB(x)      TAB ADD1TAB(x)
#define ADD3TAB(x)      TAB ADD2TAB(x)
#define ADD4TAB(x)      TAB ADD3TAB(x)
#define ADD5TAB(x)      TAB ADD4TAB(x)
#define ADD6TAB(x)      TAB ADD5TAB(x)
#define ADD7TAB(x)      TAB ADD6TAB(x)
#define ADD8TAB(x)      TAB ADD7TAB(x)
#define ADD9TAB(x)      TAB ADD8TAB(x)
#define ADD10TAB(x)     TAB ADD9TAB(x)
/* ADDNTAB(n,x), where n is a digit and x a string */
#define ADDNTAB(n,x)    ADD ## n ## TAB(x)

#define DO_REALLOC(p, s) do { \
    if (!(p=realloc(p, s))) { \
        EPrintF("Memory pool out of memory"); \
    } \
} while(0)

#define DO_MALLOC(p, s) do { \
    if (!(p=malloc(s))) { \
        EPrintF("Memory pool out of memory"); \
    } \
} while(0)

#define DO_MALLOCZ(p, s) do { \
    if (!(p=malloc(s))) { \
        EPrintF("Memory pool out of memory"); \
    } \
    memset(p, 0, s); \
} while(0)

#define DO_WRITE(fd, b, s) do {\
    if (write(fd, b, s)!=s) { \
        EPrintF("Error writting to file"); \
    } \
} while(0)

#define DO_READ(fd, b, s) do {\
    if (read(fd, b, s)!=s) { \
        EPrintF("Error reading from file"); \
    } \
} while(0)

extern void LineError(int nLine, char *fmt, ...);
extern void EPrintF(char *fmt, ...);
extern void GenerateCFile(FILE *oFile);
extern void ExecXmcBuild(char *path, char *in, char *out);
extern void CalcDigest(char *out);
extern char *inFile;
extern void SetupHwIrqMask(void);

//#define ALIGN(size, align) ((((~(size))+1)&((align)-1))+(size))
extern void RsvBlock(unsigned int size, int align, char *comment);
extern void PrintBlocks(FILE *oFile);

struct vCpu2Cpu{
     int line;
     xm_s32_t cpu;
} **vCpuTab;

#define TAGGED_MEM_BLOCK(tag, size, align, comment) do { \
    if (size) { \
        fprintf(oFile, \
"\n__asm__ (/* %s */ \\\n" \
"         \".section .bss.mempool\\n\\t\" \\\n" \
"         \".align %d\\n\\t\" \\\n" \
"         \"%s:.zero %d\\n\\t\" \\\n" \
"         \".previous\\n\\t\");\n", comment, align, tag, size); \
    } \
} while(0)

#endif
