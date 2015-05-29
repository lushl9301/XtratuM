/*
 * $FILE: pc_vga.c
 *
 * ia32 PC screen driver
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */
#ifdef CONFIG_DEV_VGA
#include <boot.h>
#include <rsvmem.h>
#include <kdevice.h>
#include <spinlock.h>
#include <sched.h>
#include <stdc.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/physmm.h>
#include <drivers/vga_text.h>

#define COLUMNS 80
#define LINES 25
#define ATTRIBUTE 7

#define BUFFER_SIZE (COLUMNS*LINES*2)

static const kDevice_t textVga;
static const kDevice_t *GetTextVga(xm_u32_t subId) {
    switch (subId) {
        case 0:
            return &textVga;
            break;
    }

    return 0;
}

static xm_u8_t *buffer;
static xm_s32_t xPos, yPos;

static inline void ClearTextVga(void) {
    xm_s32_t pos;

    xPos = yPos = 0;
    for (pos = 0; pos < COLUMNS * LINES; pos++)
        ((xm_u16_t *) buffer)[pos] = (ATTRIBUTE << 8);

    VgaSetStartAddr(0);
    VgaSetCursorPos(0);
}

static inline xm_u8_t *VgaMapTextVga(void) {
    xmAddress_t addr, p;
    xm_u32_t noPages;

    noPages = SIZE2PAGES(BUFFER_SIZE);
    addr = VmmAlloc(noPages);
    for (p = 0; p < (noPages * PAGE_SIZE); p += PAGE_SIZE)
        VmMapPage(TEXT_VGA_ADDRESS + p, addr + p, _PG_ATTR_PRESENT | _PG_ATTR_RW);

    return (xm_u8_t *) addr;
}

xm_s32_t InitTextVga(void) {
    GetKDevTab[XM_DEV_VGA_ID] = GetTextVga;
    buffer = VgaMapTextVga();

#ifndef CONFIG_EARLY_DEV_VGA
    ClearTextVga();
#endif

    return 0;
}

static inline void PutCharTextVga(xm_s32_t c) {
    xm_s32_t pos;

    if (c == '\t') {
        xPos += 3;
        if (xPos >= COLUMNS)
            goto newline;
        VgaSetCursorPos((xPos+yPos*COLUMNS));
        return;
    }

    if (c == '\n' || c == '\r') {
newline:
        xPos = 0;
        yPos++;
        if (yPos == LINES) {
            memcpy((xm_u8_t *) buffer, (xm_u8_t *)&buffer[COLUMNS*2], (LINES-1) * COLUMNS*2);
            for (pos=0; pos < COLUMNS; pos++)
                ((xm_u16_t *) buffer)[pos + (LINES-1) * COLUMNS] = (ATTRIBUTE << 8);
            yPos--;
        }

        VgaSetCursorPos((xPos+yPos*COLUMNS));
        return;
    }

    buffer[(xPos + yPos * COLUMNS) * 2] = c & 0xFF;
    buffer[(xPos + yPos * COLUMNS) * 2 + 1] = ATTRIBUTE;

    xPos++;
    if (xPos >= COLUMNS)
        goto newline;

    VgaSetCursorPos(xPos+yPos*COLUMNS);
}

static xm_s32_t WriteTextVga(const kDevice_t *kDev, xm_u8_t *buffer, xm_s32_t len) {
    xm_s32_t e;
    for (e = 0; e < len; e++)
        PutCharTextVga(buffer[e]);

    return len;
}

static const kDevice_t textVga = { .subId = 0, .Reset = 0, .Write = WriteTextVga, .Read = 0, .Seek = 0, };

REGISTER_KDEV_SETUP(InitTextVga);

#ifdef CONFIG_EARLY_DEV_VGA
xm_s32_t EarlyInitTextVga(void) {

    buffer = (xm_u8_t *)TEXT_VGA_ADDRESS;
    ClearTextVga();

    return 0;
}

void SetupEarlyOutput(void) {
    EarlyInitTextVga();
}

void EarlyPutChar(xm_u8_t c) {
    PutCharTextVga(c);
}
#endif

#endif
