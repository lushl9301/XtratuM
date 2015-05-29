/*
 * $FILE: leon_uart.c
 *
 * LEON UART as defined in Datasheet LEON AT697E
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
#ifdef CONFIG_LEON3
#include <boot.h>
#include <arch/leon.h>
#include <objdir.h>
#include <processor.h>
#include <queue.h>
#include <rsvmem.h>
#include <spinlock.h>

#if defined(CONFIG_LEON3)
#define UART1_IRQ 2
#define UART2_IRQ 3
#endif

#define AHB_STTS_REG 0x4
#define   AHB_STTS_FE_B 0x40
#define   AHB_STTS_OV_B 0x10
#define   AHB_STTS_BR_B 0x08
#define   AHB_STTS_TH_B 0x04
#define   AHB_STTS_TS_B 0x02
#define   AHB_STTS_DR_B 0x01

#define DATA_REG 0x0
#define STTS_REG 0x4
#define   STTS_RF_B 0x400
#define   STTS_TF_B 0x200
#define   STTS_RH_B 0x100
#define   STTS_TH_B 0x80
#define   STTS_FE_B 0x40
#define   STTS_PE_B 0x20
#define   STTS_OV_B 0x10
#define   STTS_BR_B 0x08
#define   STTS_TE_B 0x04
#define   STTS_TS_B 0x02
#define   STTS_DR_B 0x01
#define CTRL_REG 0x8
#define   CTRL_EC_B 0x100
#define   CTRL_LB_B 0x080
#define   CTRL_FL_B 0x040
#define   CTRL_PE_B 0x020
#define   CTRL_PS_B 0x010
#define   CTRL_TI_B 0x008
#define   CTRL_RI_B 0x004
#define   CTRL_TE_B 0x002
#define   CTRL_RE_B 0x001
#define SCLR_REG 0xC
#define   SCLR_VAL_M 0xfff

#define MASK_PIN_OUTPUT_UART1 (0xa000)
#define MASK_PIN_INPUT_UART1 ~(0x5000)
//#define MASK_PIN_OUTPUT_UART2 (0xa00)
//#define MASK_PIN_INPUT_UART2 ~(0x500)

#define MASK_PIN_OUTPUT_DEFAULT_UART MASK_PIN_OUTPUT_UART1
#define MASK_PIN_INPUT_DEFAULT_UART MASK_PIN_INPUT_UART1

#ifdef CONFIG_DEV_UART_1
RESERVE_IOPORTS(LEON_UART1_BASE, 4);
RESERVE_HWIRQ(UART1_IRQ);
#endif

#ifdef CONFIG_DEV_UART_2
RESERVE_IOPORTS(LEON_UART2_BASE, 4);
RESERVE_HWIRQ(UART2_IRQ);
#endif

static kDevice_t uartTab[CONFIG_DEV_NO_UARTS];

#if defined(CONFIG_DEV_UART_1) || defined(CONFIG_DEV_UART_2)
static struct uartInfo {
    xm_u32_t base;
    xm_s32_t irq;
} uartInfo[]={
    [0]={
	.base=LEON_UART1_BASE,
        .irq=UART1_IRQ,
    },
    [1]={
	.base=LEON_UART2_BASE, 
        .irq=UART2_IRQ,
    },
};

static spinLock_t uartSLock=SPINLOCK_INIT;


static inline void _WriteUart(xm_u32_t base, xm_s32_t a) {
    xm_s32_t loop=0;
#ifdef CONFIG_UART_THROUGH_DSU
    while(!(LoadIoReg(base+STTS_REG)&STTS_TE_B));
#else
    while(!(LoadIoReg(base+STTS_REG)&STTS_TE_B)&&(loop<CONFIG_UART_TIMEOUT)) loop++;
#endif
    StoreIoReg(DATA_REG+base, a);
    loop=0;
#ifdef CONFIG_UART_THROUGH_DSU
    while(!(LoadIoReg(base+STTS_REG)&STTS_TS_B));
#else
    while(!(LoadIoReg(base+STTS_REG)&STTS_TS_B)&&(loop<CONFIG_UART_TIMEOUT)) loop++;
#endif
}

static xm_s32_t WriteUart(const kDevice_t *kDev, xm_u8_t *buffer, xm_s32_t len) {
    xm_s32_t e;
    SpinLock(&uartSLock);
    for (e=0; e<len; e++) {
	_WriteUart(uartInfo[kDev->subId].base, buffer[e]);
	if (buffer[e]=='\n')
           _WriteUart(uartInfo[kDev->subId].base, '\r');
    }
    SpinUnlock(&uartSLock);
    return len;
}

static xm_s32_t __InitUart(xm_u32_t base, xm_u32_t baudrate, xm_u32_t cpuKhz) {
#ifdef DEV_UART_FLOWCONTROL
    xm_u32_t ctrl=CTRL_TE_B|CTRL_RE_B|CTRL_FL_B;
#else
    xm_u32_t ctrl=CTRL_TE_B|CTRL_RE_B;
#endif

    StoreIoReg(STTS_REG+base, 0);
#ifdef CONFIG_UART_THROUGH_DSU
    ctrl|=CTRL_LB_B|CTRL_FL_B;
#endif
#ifndef CONFIG_UART_THROUGH_DSU
    StoreIoReg(CTRL_REG+base, ctrl);
#endif
    StoreIoReg(SCLR_REG+base, (((cpuKhz*1000*10)/(baudrate*8))-5)/10);

    return 0;
}

#endif

static const kDevice_t *GetUart(xm_u32_t subId) {
    if ((subId>=CONFIG_DEV_NO_UARTS)||!xmcTab.deviceTab.uart[subId].baudRate)
	return 0;   
    return &uartTab[subId];
}

xm_s32_t InitUart(void) {
    extern xm_u32_t GetCpuKhz(void);

    memset(uartTab, 0, sizeof(kDevice_t)*CONFIG_DEV_NO_UARTS);
    uartTab[1].subId=1;

#ifdef CONFIG_DEV_UART_1
    uartTab[0].Write=WriteUart;    
    if (xmcTab.deviceTab.uart[0].baudRate)
        __InitUart(uartInfo[0].base, xmcTab.deviceTab.uart[0].baudRate, GetCpuKhz());
#endif

#ifdef CONFIG_DEV_UART_2
    uartTab[1].Write=WriteUart;    
    if (xmcTab.deviceTab.uart[1].baudRate)
        __InitUart(uartInfo[1].base, xmcTab.deviceTab.uart[1].baudRate, GetCpuKhz());
#endif

    GetKDevTab[XM_DEV_UART_ID]=GetUart;
    return 0;
}

#ifdef CONFIG_DEV_UART_MODULE
XM_MODULE("UART", InitUart, DRV_MODULE);
#else
REGISTER_KDEV_SETUP(InitUart);
#endif

#endif

#ifdef CONFIG_EARLY_OUTPUT
#ifdef CONFIG_EARLY_UART1
static xm_s32_t dflEarlyUart=0;
#elif CONFIG_EARLY_UART2
static xm_s32_t dflEarlyUart=1;
#endif

void SetupEarlyOutput(void) {
    __InitUart(uartInfo[dflEarlyUart].base, CONFIG_EARLY_UART_BAUDRATE, CONFIG_EARLY_CPU_MHZ*1000);
}

void EarlyPutChar(xm_u8_t c) {    
    _WriteUart(uartInfo[dflEarlyUart].base, c);
/*
    if (c=='\n')
        _WriteUart(uartInfo[dflEarlyUart].base, '\r');
*/
}
#endif
