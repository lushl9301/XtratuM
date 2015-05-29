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
