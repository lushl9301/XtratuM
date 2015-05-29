/*
 * $FILE: string.c
 *
 * String related functions
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <stdc.h>
#include <arch/xm_def.h>

void *memset(void *dst, xm_s32_t s, xmSize_t count) {
    register xm_s8_t *a=dst;
    count++;
    while (--count)
	*a++=s;
    return dst;
}

#ifndef __ARCH_MEMCPY

void *memcpy(void *dst, const void *src, xmSize_t count) {
    register xm_s8_t *d=dst;
    register const xm_s8_t *s=src;

    ++count;
    while (--count) {
	*d=*s;
	++d; ++s;
    }
    return dst;
}

void *MemCpyPhys(void *dst, const void *src, xm_u32_t count) {
    return 0;
}

#endif

xm_s32_t memcmp(const void *dst, const void *src, xmSize_t count) {
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

char *strcpy(char *dst, const char *src) {
    char *aux = dst;
    while ((*aux++ = *src++));
    return dst;
}

char *strncpy(char *dest, const char *src, xmSize_t n) {
    xm_s32_t j;

    memset(dest,0,n);

    for (j=0; j<n && src[j]; j++)
	dest[j]=src[j];

    if (j>=n)
	dest[n-1]=0;

    return dest;
}

char *strcat(char *s, const char* t) {
    char *dest=s;
    s+=strlen(s);
    for (;;) {
	if (!(*s=*t)) break; ++s; ++t;
    }
    return dest;
}

char *strncat(char *s, const char *t, xmSize_t n) {
  char *dest=s;
  register char *max;
  s+=strlen(s);
  if ((max=s+n)==s) goto fini;
  for (;;) {
    if (!(*s = *t)) break; if (++s==max) break; ++t;
  }
  *s=0;
fini:
  return dest;
}

xm_s32_t strcmp(const char *s, const char *t) {
    char x;

    for (;;) {
	x = *s; if (x != *t) break; if (!x) break; ++s; ++t;
    }
    return ((xm_s32_t)x)-((xm_s32_t)*t);
}

xm_s32_t strncmp(const char *s1, const char *s2, xmSize_t n) {
    register const xm_u8_t *a=(const xm_u8_t*)s1;
    register const xm_u8_t *b=(const xm_u8_t*)s2;
    register const xm_u8_t *fini=a+n;

    while (a < fini) {
	register xm_s32_t res = *a-*b;
	if (res) return res;
	if (!*a) return 0;
	++a; ++b;
    }
    return 0;
}

xmSize_t strlen(const char *s) {
    xm_u32_t i;
    if (!s) return 0;
    for (i = 0; *s; ++s) ++i;
    return i;
}

char *strrchr(const char *t, xm_s32_t c) {
    char ch;
    const char *l=0;

    ch = c; 
    for (;;) {
	if (*t == ch) l=t; 
	if (!*t) return (char*)l; ++t;
    }
  
    return (char*)l;
}

char *strchr(const char *t, xm_s32_t c) {
    register char ch;

    ch = c;
    for (;;) {
	if (*t == ch) break; if (!*t) return 0; ++t;
    }
    return (char*)t;
}

char *strstr(const char *haystack, const char *needle) {
    xmSize_t nl=strlen(needle);
    xmSize_t hl=strlen(haystack);
    xm_s32_t i;
    if (!nl) goto found;
    if (nl>hl) return 0;
    for (i=hl-nl+1; i; --i) {
	if (*haystack==*needle && !memcmp(haystack,needle,nl))
	found:
	    return (char*)haystack;
	++haystack;
    }
    return 0;
}

void *memmove(void *dst, const void *src, xmSize_t count) {
    xm_s8_t *a = dst;
    const xm_s8_t *b = src;
    if (src!=dst)
    {
	if (src>dst)
	{
	    while (count--) *a++ = *b++;
	}
	else
	{
	    a+=count-1;
	    b+=count-1;
	    while (count--) *a-- = *b--;
	}
    }
    return dst;
}
