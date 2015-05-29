/*
 * $FILE: stdio.c
 *
 * Standard buffered input/output
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

/*
 * - printf() based on sprintf() from gcctest9.c Volker Oth
 * - Changes made by Holger Klabunde
 * - Changes made by Martin Thomas for the efsl debug output
 * - Changes made by speiro for lpc2000 devices for PaRTiKle
 */

#include <stdc.h>
#include <spinlock.h>
#include <objects/console.h>

#define SCRATCH 20
#define USE_LONG	// %lx, %Lu and so on, else only 16 bit
			// %integer is allowed

#define USE_STRING			// %s, %S Strings as parameters
#define USE_CHAR	// %c, %C Chars as parameters
#define USE_INTEGER // %i, %I Remove this format flag. %d, %D does the same
#define USE_HEX		// %x, %X Hexadecimal output

//#define USE_UPPERHEX	// %x, %X outputs A,B,C... else a,b,c...

#ifndef USE_HEX
# undef USE_UPPERHEX		// ;)
#endif

//#define USE_UPPER // uncommenting this removes %C,%D,%I,%O,%S,%U,%X and %L..
// only lowercase format flags are used
#define PADDING					//SPACE and ZERO padding

typedef struct{
    xm_s32_t (*_putC)(xm_s32_t c, void *a);
    void *a;
} fPrint_t;

static void __PrintFmt(fPrint_t *fp, const char *fmt, va_list args) {
    xm_u8_t scratch[SCRATCH];
    xm_u8_t fmtFlag;
    xm_u16_t base;
    xm_u8_t *ptr;
    xm_u8_t issigned=0;
  
#ifdef USE_LONG
    xm_u8_t islong=0;
    xm_u8_t isvlong=0;
    xm_u64_t u_val=0;
    xm_s64_t s_val=0;
#else
    xm_u32_t u_val=0;
    xm_s32_t s_val=0;
#endif
  
    xm_u8_t fill;
    xm_u8_t width;

    for (;;) {
	while ((fmtFlag = *(fmt++)) != '%') {			 // Until '%' or '\0' 
	    if (!fmtFlag){ return; }
	    fp->_putC(fmtFlag, fp->a);
	}
    
	issigned=0; //default unsigned
	base = 10;
    
	fmtFlag = *fmt++; //get char after '%'
    
#ifdef PADDING
	width=0; //no formatting
	fill=0;	 //no formatting
    
	if(fmtFlag=='0' || fmtFlag==' ') { //SPACE or ZERO padding	?      
	    fill=fmtFlag;
	    fmtFlag = *fmt++; //get char after padding char
	    while(fmtFlag>='0' && fmtFlag<='9') {
		width = 10*width + (fmtFlag-'0');
		fmtFlag = *fmt++; //get char after width char
	    }
	}
#endif
    
#ifdef USE_LONG
	islong=0; //default int value
	isvlong=0;
#ifdef USE_UPPER
	if(fmtFlag=='l' || fmtFlag=='L') //Long value 
#else
	    if(fmtFlag=='l') //Long value 
#endif      
	    {
		islong=1;
		fmtFlag = *fmt++; //get char after 'l' or 'L'
		if (fmtFlag=='l'){
		    isvlong = 1;
		    fmtFlag = *fmt++; //get char after 'l' or 'L'
		}
	    }
#endif
    
	switch (fmtFlag) {
#ifdef USE_CHAR
	case 'c':
#ifdef USE_UPPER
	case 'C':
#endif
	    fmtFlag=va_arg(args, xm_s32_t);
	    // no break -> run into default
#endif
      
	default:
	    fp->_putC(fmtFlag, fp->a);
	    continue;
      
#ifdef USE_STRING
#ifdef USE_UPPER
	case 'S':
#endif
	case 's':
	    ptr=(xm_u8_t*)va_arg(args, char *);
	    while(*ptr) { fp->_putC(*ptr, fp->a); ptr++; }
	    continue;
#endif
      
#ifdef USE_OCTAL
	case 'o':
#ifdef USE_UPPER
	case 'O':
#endif
	    base = 8;
	    fp->_putC('0', fp->a);
	    goto CONVERSION_LOOP;
#endif
	
#ifdef USE_INTEGER //don't use %i, is same as %d
	case 'i':
#ifdef USE_UPPER
	case 'I':
#endif
#endif
	case 'd':
#ifdef USE_UPPER
	case 'D':
#endif
	    issigned=1;
	    // no break -> run into next case
	case 'u':
#ifdef USE_UPPER
	case 'U':
#endif
	
	    //don't insert some case below this if USE_HEX is undefined !
	    //or put			 goto CONVERSION_LOOP;	before next case.
#ifdef USE_HEX
	    goto CONVERSION_LOOP;
	case 'x':
#ifdef USE_UPPER
	case 'X':
#endif
	    base = 16;
#endif

	CONVERSION_LOOP:
	
	    if(issigned) { //Signed types
	  
#ifdef USE_LONG
		if(isvlong) { s_val = va_arg(args, xm_s64_t); }
		else if(islong) { s_val = va_arg(args, xm_s32_t); }
		else { s_val = va_arg(args, xm_s32_t); }
#else
		s_val = va_arg(args, xm_s32_t);
#endif
	  
		if(s_val < 0) { //Value negativ ?
		    s_val = - s_val; //Make it positiv
		    fp->_putC('-', fp->a);		 //Output sign
		}
	    
		if(!isvlong)
		    u_val = (xm_u32_t)s_val;
		else
		    u_val = (xm_u64_t)s_val;
	    } else {//Unsigned types
#ifdef USE_LONG
		if(isvlong) {u_val = va_arg(args, xm_u64_t); }
		else if(islong) { u_val = va_arg(args, xm_u32_t); }
		else { u_val = va_arg(args, xm_u32_t); }
#else
		u_val = va_arg(args, xm_u32_t);
#endif
	    }
	
	    ptr = scratch + SCRATCH;
	    *--ptr = 0;
	    do {
		char ch = u_val % base + '0';
#ifdef USE_HEX
		if (ch > '9') {
		    ch += 'a' - '9' - 1;
#ifdef USE_UPPERHEX
		    ch-=0x20;
#endif
		}
#endif					
		*--ptr = ch;
		u_val /= base;
	  
#ifdef PADDING
		if(width) width--; //calculate number of padding chars
#endif
	    } while (u_val);
	
#ifdef PADDING
	    while(width--) *--ptr = fill; //insert padding chars					
#endif
	
	    while(*ptr) { fp->_putC(*ptr, fp->a); ptr++; }
	}
    }
}

static xm_s32_t PrintFPutC(xm_s32_t c, void *a){
    xm_u32_t *nc=(xm_u32_t*)a;
    ConsolePutChar(c);
    (*nc)++;
    return 1;
}

static spinLock_t vpSpin=SPINLOCK_INIT;
xm_s32_t vprintf(const char *fmt, va_list args) {
    xm_s32_t nc=0;
    fPrint_t fp = {PrintFPutC, (void*)&nc};
    xmWord_t flags;
    
    SpinLockIrqSave(&vpSpin, flags);
    __PrintFmt(&fp, fmt, args);
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

typedef struct Sdata{
    char *s;
    xm_s32_t *nc;
} Sdata;

static xm_s32_t SPrintFPutC(xm_s32_t c, void *a){
    Sdata *sd = (Sdata*) a;
  
    (*sd->s++)=c;
    (*sd->nc)++;
    return 1;
}

xm_s32_t sprintf(char *s, char const *fmt, ...){
    xm_s32_t nc=0;
    Sdata sd = {s, &nc};
    fPrint_t fp = {SPrintFPutC, (void*)&sd};
    va_list args;
    xmWord_t flags;
    
    SpinLockIrqSave(&vpSpin, flags);
    va_start(args, fmt);
    __PrintFmt(&fp, fmt, args);
    va_end(args);
    s[nc]=0;
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

typedef struct Sndata {
    char *s;
    xm_s32_t *n;	// s size
    xm_s32_t *nc;
} Sndata;

static xm_s32_t SNPrintFPutC(xm_s32_t c, void *a){
    Sndata *snd = (Sndata*) a;

    if (*snd->n > *snd->nc){
	if (snd->s) {
	    snd->s[(*snd->nc)] = c;
	    (*snd->nc)++;
	    snd->s[(*snd->nc)]='\0';
	}
    }
    return 1;
}

xm_s32_t snprintf(char *s, xm_s32_t n, const char *fmt, ...){
    xm_s32_t nc=0;
    Sndata snd = {s, &n, &nc};
    fPrint_t fp = {SNPrintFPutC, (void*)&snd};
    va_list args;
    xmWord_t flags;
    
    SpinLockIrqSave(&vpSpin, flags);
    va_start(args, fmt);
    __PrintFmt(&fp, fmt, args);
    va_end(args);
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

xm_s32_t kprintf(const char *format,...) {
    va_list argPtr;
    xm_s32_t n;
    //xmWord_t flags;

    //HwSaveFlagsCli(flags);
    va_start(argPtr, format);
    n=vprintf(format, argPtr);
    va_end(argPtr);
    //HwRestoreFlags(flags);
    
    return n;
}

#ifdef CONFIG_EARLY_OUTPUT

static xm_s32_t EarlyPrintFPutC(xm_s32_t c, void *a){
    extern void EarlyPutChar(xm_u8_t c);
    xm_u32_t *nc=(xm_u32_t*)a;
    EarlyPutChar(c);
    (*nc)++;
    return 1;
}

static xm_s32_t EarlyVPrintf(const char *fmt, va_list args) {
    xm_s32_t nc=0;
    fPrint_t fp = {EarlyPrintFPutC, (void*)&nc};
    xmWord_t flags;
    
    SpinLockIrqSave(&vpSpin, flags);
    __PrintFmt(&fp, fmt, args);
    SpinUnlockIrqRestore(&vpSpin, flags);
    return nc;
}

#endif

xm_s32_t eprintf(const char *format,...) {
#ifdef CONFIG_EARLY_OUTPUT
    va_list argPtr;
    xm_s32_t n;

    va_start(argPtr, format);
    n=EarlyVPrintf(format, argPtr);
    va_end(argPtr);
    return n;
#else
    return 0;
#endif
}
