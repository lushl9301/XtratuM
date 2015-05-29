#include <rsw_stdc.h>

#define SCRATCH 20	//32Bits go up to 4GB + 1 Byte for \0

//Spare some program space by making a comment of all not used format flag lines
#define USE_LONG	// %lx, %Lu and so on, else only 16 bit integer is allowed
//#define USE_OCTAL // %o, %O Octal output. Who needs this ?
#define USE_STRING			// %s, %S Strings as parameters
#define USE_CHAR	// %c, %C Chars as parameters
#define USE_INTEGER // %i, %I Remove this format flag. %d, %D does the same
#define USE_HEX		// %x, %X Hexadezimal output
#define USE_UPPERHEX	// %x, %X outputs A,B,C... else a,b,c...
#ifndef USE_HEX
# undef USE_UPPERHEX		// ;)
#endif
#define USE_UPPER // uncommenting this removes %C,%D,%I,%O,%S,%U,%X and %L..
												// only lowercase format flags are used
#define PADDING					//SPACE and ZERO padding

void *memmove(void *dest, void *src, unsigned long count) {
    xm_u8_t *tmp;
    const xm_u8_t *s;
    
    if (dest <= src) {
	tmp = dest;
	s = src;
	while (count--)
	    *tmp++ = *s++;
    } else {
	tmp = dest;
	tmp += count;
	s = src;
	s += count;
	while (count--)
	    *--tmp = *--s;
    }
    return dest;
}

void *memset(void *dst, xm_s32_t s, unsigned long count) {
    register xm_s8_t *a = dst;
    count++;
    while (--count)
	*a++ = s;
    return dst;
}

void *memcpy(void *dst, const void *src, unsigned long count) {
    register xm_s8_t *d=dst;
    register const xm_s8_t *s=src;
    ++count;
    while (--count) {
	*d = *s;
	++d; ++s;
    }
    return dst;
}

#if 0
xm_s32_t memcmp(const void *dst, const void *src, unsigned long count) {
    xm_s32_t r;
    const xm_s8_t *d=dst;
    const xm_s8_t *s=src;
    ++count;
    while (--count) {
	if ((r=(*d - *s)))
	    return r;
	++d;
	++s;
    }
    return 0;
}

xm_u32_t strlen(const xm_s8_t *s) {
    xm_u32_t i;
    if (!s) return 0;
    for (i = 0; *s; ++s) ++i;
    return i;
}
#endif

typedef struct Fprint Fprint;
struct Fprint {
    xm_s32_t (*putc)(xm_s32_t c, void *a);
    void *a;
};

void vrprintf(Fprint *fp, const char *fmt, va_list args) {
    xm_u8_t scratch[SCRATCH];
    xm_u8_t fmt_flag;
    xm_u16_t base;
    xm_u8_t *ptr;
    xm_u8_t issigned=0;
    
#ifdef USE_LONG
// #warning "use long"
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
    
    for (;;){
	while ((fmt_flag = *(fmt++)) != '%'){			 // Until '%' or '\0' 
	    if (!fmt_flag){ return; }
	    if (fp->putc) fp->putc(fmt_flag, fp->a);
	}
	
	issigned=0; //default unsigned
	base = 10;
	
	fmt_flag = *fmt++; //get char after '%'
	
#ifdef PADDING
	width=0; //no formatting
	fill=0;	 //no formatting
	
	if(fmt_flag=='0' || fmt_flag==' ') //SPACE or ZERO padding	?
	{
	    fill=fmt_flag;
	    fmt_flag = *fmt++; //get char after padding char
	    while(fmt_flag>='0' && fmt_flag<='9')
	    {
		width = 10*width + (fmt_flag-'0');
		fmt_flag = *fmt++; //get char after width char
	    }
	}
#endif
	
#ifdef USE_LONG
	islong=0; //default int value
	isvlong=0;
#ifdef USE_UPPER
	if(fmt_flag=='l' || fmt_flag=='L') //Long value 
#else
        if(fmt_flag=='l') //Long value 
#endif
	{
	    islong=1;
	    fmt_flag = *fmt++; //get char after 'l' or 'L'
	    if (fmt_flag=='l'){
		isvlong = 1;
		fmt_flag = *fmt++; //get char after 'l' or 'L'
	    }
	}
#endif
	
	switch (fmt_flag)
	{
#ifdef USE_CHAR
	case 'c':
#ifdef USE_UPPER
	case 'C':
#endif
	    fmt_flag = va_arg(args, xm_s32_t);
	    // no break -> run into default
#endif
	    
	default:
	    if (fp->putc)
		fp->putc(fmt_flag, fp->a);
	    continue;
	    
#ifdef USE_STRING
#ifdef USE_UPPER
	case 'S':
#endif
	case 's':
	    ptr = (xm_u8_t *)va_arg(args, xm_s8_t *);
	    while(*ptr) { if(fp->putc) fp->putc(*ptr, fp->a); ptr++; }
	    continue;
#endif
	    
#ifdef USE_OCTAL
	case 'o':
#ifdef USE_UPPER
	case 'O':
#endif
	    base = 8;
	    if (fp->putc) fp->putc('0', fp->a);
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
	    
	    if(issigned) //Signed types
	    {
#ifdef USE_LONG
		if(isvlong) { s_val = va_arg(args, xm_s64_t); }
		else if(islong) { s_val = va_arg(args, xm_s32_t); }
		else { s_val = va_arg(args, xm_s32_t); }
#else
		s_val = va_arg(args, xm_s32_t);
#endif
		
		if(s_val < 0) //Value negativ ?
		{
		    s_val = - s_val; //Make it positiv
		    if (fp->putc) fp->putc('-', fp->a);		 //Output sign
		}
		
		if(!isvlong)
		    u_val = (xm_u32_t)s_val;
		else
		    u_val = (xm_u64_t)s_val;
	    } else //Unsigned types
	    {
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
	    do
	    {
		xm_s8_t ch = u_val % base + '0';
#ifdef USE_HEX
		if (ch > '9')
		{
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
	    
	    while(*ptr) { if (fp->putc) fp->putc(*ptr, fp->a); ptr++; }
	}
    }
}

static xm_s32_t printfputc(xm_s32_t c, void *a){
    xm_u32_t * nc = (xm_u32_t *) a;
    if(c == '\n'){
	xputchar('\r');
	(*nc)++;
    }
    
    xputchar(c);
    (*nc)++;
    return 1;
}

xm_s32_t xprintf(const char *fmt, ...){
    xm_s32_t nc=0;
    Fprint fp = {printfputc, (void*)&nc};
    va_list args;
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);
    
    return nc;
}

#if 0
typedef struct Sdata Sdata;
struct Sdata{
    xm_s8_t *s;
    xm_s32_t *nc;
};

static xm_s32_t sprintfputc(xm_s32_t c, void *a){
    Sdata *sd = (Sdata*) a;
    
    (*sd->s++) = c;
    (*sd->nc)++;
    return 1;
}

xm_s32_t sprintf(char *s, const char *fmt, ...){
    xm_s32_t nc=0;
    Sdata sd = {s, &nc};
    Fprint fp = {sprintfputc, (void*)&sd};
    va_list args;
    
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);
    return nc;
}

typedef struct Sndata Sndata;
struct Sndata {
    xm_s8_t *s;
    xm_s32_t *n;	// s size
    xm_s32_t *nc;
};

static xm_s32_t snprintfputc(xm_s32_t c, void *a){
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
    Fprint fp = {snprintfputc, (void*)&snd};
    va_list args;
    
    va_start(args, fmt);
    vrprintf(&fp, fmt, args);
    va_end(args);
    return nc;
}
#endif
