/*
 * $FILE: rsvmem.c
 *
 * Reserve memory
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <stdlib.h>
#include <string.h>
#include <common.h>

static struct block {
    unsigned int size; 
    int align; 
    char *comment;
} *blockTab=0;

static int noBlocks=0;

void RsvBlock(unsigned int size, int align, char *comment) {
    if (!size||!align) return;
    noBlocks++;
    DO_REALLOC(blockTab, noBlocks*sizeof(struct block));
    blockTab[noBlocks-1].size=size;
    blockTab[noBlocks-1].align=align;
    blockTab[noBlocks-1].comment=strdup(comment);
}

#define MEM_BLOCK(size, align, comment)	do {	\
    if (size) { \
        fprintf(oFile, \
"\n__asm__ (/* %s */ \\\n" \
"         \".section .data.memobj\\n\\t\" \\\n" \
"         \".long 1f\\n\\t\" \\\n" \
"         \".long %d\\n\\t\" \\\n" \
"         \".long %d\\n\\t\" \\\n" \
"         \".section .bss.mempool\\n\\t\" \\\n" \
"         \".align %d\\n\\t\" \\\n" \
"         \"1:.zero %d\\n\\t\" \\\n" \
"         \".previous\\n\\t\");\n", comment, align, size, align, size); \
    } \
} while(0)

static int Cmp(struct block *b1, struct block *b2) {
    if (b1->align>b2->align) return 1;
    if (b1->align<b2->align) return -1;
    return 0;
}

void PrintBlocks(FILE *oFile) {
    int e;
    qsort(blockTab, noBlocks, sizeof(struct block), (int(*)(const void *, const void *))Cmp);
    for (e=0; e<noBlocks; e++)
        MEM_BLOCK(blockTab[e].size,  blockTab[e].align,  blockTab[e].comment);
}
