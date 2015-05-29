/**************************************************************
        LZSS.C -- A Data Compression Program
        (tab = 4 spaces)
	***************************************************************
        4/6/1989 Haruhiko Okumura
        Use, distribute, and modify this program freely.
**************************************************************/

#include __XM_INCFLD(compress.h)

#define N               4096  /* size of ring buffer */
#define F                 18  /* upper limit for match_length */
#define THRESHOLD       2   /* encode string into position and length
			       if match_length is greater than this */
#define NIL               N    /* index for root of binary search trees */

static xm_u8_t textBuf[N+F-1];    /* ring buffer of size N, with extra
				      F-1 bytes to facilitate string
				      comparison */

static inline xm_s32_t ReadByte(xm_u32_t *readCnt, xm_u32_t inSize, CFunc_t Read, void *rData) {
    xm_u8_t tmp;
    if (*readCnt>=inSize) return -1;
    if (Read(&tmp, 1, rData)!=1) return -1;
    (*readCnt)++;
    return tmp&0xff;
}

static inline void WriteByte(xm_s32_t c, xm_u32_t *writeCnt, xm_u32_t outSize, CFunc_t Write, void *wData) {
    xm_u8_t tmp;
    if (*writeCnt>=outSize) return;
    tmp=c;
    if (Write(&tmp, 1, wData)!=1) return;
    (*writeCnt)++;
}

#ifndef _XM_KERNEL_
static xm_s32_t matchPosition, matchLength,  /* of longest match.  These are
						  set by the InsertNode() procedure. */
    lson[N+1], rson[N+257], dad[N+1];  /* left & right children &
					  parents -- These constitute binary search trees. */

static inline void InitTree(void) { /* initialize trees */
    xm_s32_t i;
    
    /* For i = 0 to N - 1, rson[i] and lson[i] will be the right and
       left children of node i.  These nodes need not be initialized.
       Also, dad[i] is the parent of node i.  These are initialized to
       NIL (= N), which stands for 'not used.'
       For i = 0 to 255, rson[N + i + 1] is the root of the tree
       for strings that begin with character i.  These are initialized
       to NIL.  Note there are 256 trees. */
    
    for (i = N + 1; i <= N + 256; i++) rson[i] = NIL;
    for (i = 0; i < N; i++) dad[i] = NIL;
}

static inline void InsertNode(xm_s32_t r)
/* Inserts string of length F, textBuf[r..r+F-1], into one of the
   trees (textBuf[r]'th tree) and returns the longest-match position
   and length via the global variables matchPosition and matchLength.
   If matchLength = F, then removes the old node in favor of the new
   one, because the old one will be deleted sooner.
   Note r plays double role, as tree node and position in buffer. */
{
    xm_s32_t  i, p, cmp;
    xm_u8_t  *key;
    
    cmp = 1;  key = &textBuf[r];  p = N + 1 + key[0];
    rson[r] = lson[r] = NIL;  matchLength = 0;
    for ( ; ; ) {
	if (cmp >= 0) {
	    if (rson[p] != NIL) p = rson[p];
	    else {  rson[p] = r;  dad[r] = p;  return;  }
	} else {
	    if (lson[p] != NIL) p = lson[p];
	    else {  lson[p] = r;  dad[r] = p;  return;  }
	}
	for (i = 1; i < F; i++)
	    if ((cmp = key[i] - textBuf[p + i]) != 0)  break;
	if (i > matchLength) {
	    matchPosition = p;
	    if ((matchLength = i) >= F)  break;
	}
    }
    dad[r] = dad[p];  lson[r] = lson[p];  rson[r] = rson[p];
    dad[lson[p]] = r;  dad[rson[p]] = r;
    if (rson[dad[p]] == p) rson[dad[p]] = r;
    else                   lson[dad[p]] = r;
    dad[p] = NIL;  /* remove p */
}

static inline void DeleteNode(xm_s32_t p)  /* deletes node p from tree */
{
    xm_s32_t  q;
       
    if (dad[p] == NIL) return;  /* not in tree */
    if (rson[p] == NIL) q = lson[p];
    else if (lson[p] == NIL) q = rson[p];
    else {
	q = lson[p];
	if (rson[q] != NIL) {
	    do {  q = rson[q];  } while (rson[q] != NIL);
	    rson[dad[q]] = lson[q];  dad[lson[q]] = dad[q];
	    lson[q] = lson[p];  dad[lson[p]] = q;
	}
	rson[q] = rson[p];  dad[rson[p]] = q;
    }
    dad[q] = dad[p];
    if (rson[dad[p]] == p) rson[dad[p]] = q;  else lson[dad[p]] = q;
    dad[p] = NIL;
}

xm_s32_t LZSSCompress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData) {
    xm_s32_t  i, c, len, r, s, lastMatchLength, codeBufPtr;
    xm_u8_t  codeBuf[17], mask;
    xm_u32_t readCnt=0, writeCnt=0;

    InitTree();  /* initialize trees */
    codeBuf[0] = 0;  /* codeBuf[1..16] saves eight units of code, and
			 codeBuf[0] works as eight flags, "1" representing that the unit
			 is an unencoded letter (1 byte), "0" a position-and-length pair
			 (2 bytes).  Thus, eight units require at most 16 bytes of code. */
    codeBufPtr = mask = 1;
    s = 0;  r = N - F;
    for (i = s; i < r; i++) textBuf[i] = ' ';  /* Clear the buffer with
						   any character that will appear often. */
    for (len = 0; len < F && (c = ReadByte(&readCnt, inSize, Read, rData)) != -1; len++)
	textBuf[r + len] = c;  /* Read F bytes into the last F bytes of
				   the buffer */
    if (len==0) return 0;  /* text of size zero */
    for (i = 1; i <= F; i++) InsertNode(r - i);  /* Insert the F strings,
						    each of which begins with one or more 'space' characters.  Note
						    the order in which these strings are inserted.  This way,
						    degenerate trees will be less likely to occur. */
    InsertNode(r);  /* Finally, insert the whole string just read.  The
		       global variables matchLength and matchPosition are set. */
    do {
	if (matchLength > len) matchLength = len;  /* matchLength
							may be spuriously long near the end of text. */
	if (matchLength <= THRESHOLD) {
	    matchLength = 1;  /* Not long enough match.  Send one byte. */
	    codeBuf[0] |= mask;  /* 'send one byte' flag */
	    codeBuf[codeBufPtr++] = textBuf[r];  /* Send uncoded. */
	} else {
	    codeBuf[codeBufPtr++] = (xm_u8_t) matchPosition;
	    codeBuf[codeBufPtr++] = (xm_u8_t)
		(((matchPosition >> 4) & 0xf0)
		 | (matchLength - (THRESHOLD + 1)));  /* Send position and
							  length pair. Note matchLength > THRESHOLD. */
	}
	if ((mask <<= 1) == 0) {  /* Shift mask left one bit. */
	    for (i = 0; i < codeBufPtr; i++)  /* Send at most 8 units of */
		WriteByte(codeBuf[i], &writeCnt, outSize, Write, wData);     /* code together */
	    codeBuf[0] = 0;  codeBufPtr = mask = 1;
	}
	lastMatchLength = matchLength;
	for (i = 0; i < lastMatchLength &&
		 (c = ReadByte(&readCnt, inSize, Read, rData)) != -1; i++) {
	    DeleteNode(s);    /* Delete old strings and */
	    textBuf[s] = c;        /* read new bytes */
	    if (s < F - 1) textBuf[s + N] = c;  /* If the position is
						    near the end of buffer, extend the buffer to make
						    string comparison easier. */
	    s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
	    /* Since this is a ring buffer, increment the position
	       modulo N. */
	    InsertNode(r);  /* Register the string in textBuf[r..r+F-1] */
	}
	while (i++ < lastMatchLength) {       /* After the end of text, */
	    DeleteNode(s);          /* no need to read, but */
	    s = (s + 1) & (N - 1);  r = (r + 1) & (N - 1);
	    if (--len) InsertNode(r);              /* buffer may not be empty. */
	}
    } while (len > 0);      /* until length of string to be processed is zero */
    if (codeBufPtr > 1) {  /* Send remaining code. */
	for (i = 0; i < codeBufPtr; i++) WriteByte(codeBuf[i], &writeCnt, outSize, Write, wData);
    }
    return writeCnt;
}
#endif

xm_s32_t LZSSUncompress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData) {
    xm_s32_t  i, j, k, r, c;
    xm_u32_t  flags;
    xm_u32_t readCnt=0, writeCnt=0;


    for (i = 0; i < N - F; i++) textBuf[i] = ' ';
    r = N - F;  flags = 0;
    for ( ; ; ) {
	if (((flags >>= 1) & 256) == 0) {
	    if ((c = ReadByte(&readCnt, inSize, Read, rData)) == -1) break;
	    flags = c | 0xff00;          /* uses higher byte cleverly */
	}                                                 /* to count eight */
	if (flags & 1) {
	    if ((c = ReadByte(&readCnt, inSize, Read, rData)) == -1) break;
	    WriteByte(c, &writeCnt, outSize, Write, wData); textBuf[r++] = c;  r &= (N - 1);
	} else {
	    if ((i = ReadByte(&readCnt, inSize, Read, rData)) == -1) break;
	    if ((j = ReadByte(&readCnt, inSize, Read, rData)) == -1) break;
	    i |= ((j & 0xf0) << 4);  j = (j & 0x0f) + THRESHOLD;
	    for (k = 0; k <= j; k++) {
		c = textBuf[(i + k) & (N - 1)];
		WriteByte(c, &writeCnt, outSize, Write, wData); textBuf[r++] = c;  r &= (N - 1);
	    }
	}
    }
    
    return writeCnt;
}

#ifdef _XM_KERNEL_
#define RWORD(w) w
#else
#include <endianess.h>
#endif

struct compressHdr {
#define COMPRESS_MAGIC 0x64751423
    xm_u32_t magic;
    xmSize_t size;
    xmSize_t lzSize;
};

#ifndef _XM_KERNEL_
xm_s32_t Compress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData, void (*SeekW)(xmSSize_t offset, void *wData)) {
    struct compressHdr hdr;
    xm_u32_t lzSize;

    if (outSize<=(inSize+(inSize>>1)+sizeof(struct compressHdr)))
	return COMPRESS_BUFFER_OVERRUN;

    SeekW(sizeof(struct compressHdr), wData);
    if ((lzSize=LZSSCompress(inSize, outSize-sizeof(struct compressHdr), Read, rData, Write, wData))<=0)
	return COMPRESS_ERROR_LZ;
    hdr.magic=RWORD(COMPRESS_MAGIC);
    hdr.size=RWORD(inSize);
    hdr.lzSize=RWORD(lzSize);
    SeekW(-(lzSize+sizeof(struct compressHdr)), wData);
    if (Write(&hdr, sizeof(struct compressHdr), wData)!=sizeof(struct compressHdr))
        return COMPRESS_WRITE_ERROR;
    return (lzSize+sizeof(struct compressHdr));
}
#endif

xm_s32_t Uncompress(xm_u32_t inSize, xm_u32_t outSize, CFunc_t Read, void *rData, CFunc_t Write, void *wData) {
    struct compressHdr hdr;
    xm_u32_t lzSize, size, uncomSize;

    if (Read(&hdr, sizeof(struct compressHdr), rData)!=sizeof(struct compressHdr))
        return COMPRESS_READ_ERROR;

    if(hdr.magic!=RWORD(COMPRESS_MAGIC))
	return COMPRESS_BAD_MAGIC;

    size=RWORD(hdr.size);
    lzSize=RWORD(hdr.lzSize);

    if (outSize<size)
	return COMPRESS_BUFFER_OVERRUN;
    
    uncomSize=LZSSUncompress(lzSize, outSize, Read, rData, Write, wData);
    return (size==uncomSize)?uncomSize:-1;
}
