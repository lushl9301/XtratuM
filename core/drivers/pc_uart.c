/*
 * $FILE: pc_uart.c
 *
 * PC UART
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <kdevice.h>
#include <ktimer.h>
#if defined(CONFIG_x86)&&defined(CONFIG_DEV_UART)
#include <irqs.h>
#include <arch/io.h>
#include <drivers/pc_uart.h>

RESERVE_HWIRQ(UART_IRQ0);
RESERVE_IOPORTS(DEFAULT_PORT, 5);

#define _UART_MAX_FREQ 115200
static void __InitUart(xm_u32_t baudrate) {
    xm_u16_t div;
    if (!baudrate) return;
    if (baudrate>=_UART_MAX_FREQ)
        div=1;
    else
        div=_UART_MAX_FREQ/baudrate;

    OutB(0x00, DEFAULT_PORT+1); // Disable all interrupts
    OutB(0x80, DEFAULT_PORT+3); // Enable DLAB (set baud rate divisor)
    OutB(div&0xff, DEFAULT_PORT+0);
    OutB((div>>8)&0xff, DEFAULT_PORT+1);
    OutB(0x03, DEFAULT_PORT+3); // 8 bits, no parity, one stop bit
    OutB(0xC7, DEFAULT_PORT+2); // Enable FIFO, clear them, with 14-byte threshold
    OutB(0x0B, DEFAULT_PORT+4); // IRQs enabled, RTS/DSR set
}

static inline void PutCharUart(xm_s32_t c) {
    while (!(InB(DEFAULT_PORT+5)&0x20))
        continue;
    OutB(c, DEFAULT_PORT);
}

static xm_s32_t WriteUart(const kDevice_t *kDev, xm_u8_t *buffer, xm_s32_t len) {
#if 0
    xmTime_t now, before;

    /* TODO: ¿Where's the end of the slot? */
    before = GetSysClockUsec();
    while (!(InB(DEFAULT_PORT+5)&0x20)) {
        now = GetSysClockUsec();
        if ((now-before) >= CONFIG_UART_TIMEOUT) {
            return;
        }
    }
#endif

    xm_s32_t e;
    for (e=0; e<len; e++)
        PutCharUart(buffer[e]);
    
    return len;
}

static const kDevice_t uartDev={
    .subId=0,
    .Reset=0,
    .Write=WriteUart,
    .Read=0,
    .Seek=0,
};

static const kDevice_t *GetUart(xm_u32_t subId) {
    switch(subId) {
    case 0:
        return &uartDev;
        break;
    }

    return 0;
}

xm_s32_t InitUart(void) {
    __InitUart(xmcTab.deviceTab.uart[0].baudRate);
    GetKDevTab[XM_DEV_UART_ID]=GetUart;
    return 0;
}

REGISTER_KDEV_SETUP(InitUart);

#ifdef CONFIG_EARLY_DEV_UART
void SetupEarlyOutput(void) {
    InitUart();
}

void EarlyPutChar(xm_u8_t c) {
    PutCharUart(c);
}
#endif

#endif
