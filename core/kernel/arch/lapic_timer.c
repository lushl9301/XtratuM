/*
 * $FILE: lapic_timer.c
 *
 * Local & IO Advanced Programming Interrupts Controller (APIC)
 * IOAPIC, A.K.A. i82093AA
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <assert.h>
#include <boot.h>
#include <stdc.h>
#include <kdevice.h>
#include <ktimer.h>
#include <virtmm.h>
#include <vmmap.h>
#include <arch/asm.h>
#include <arch/apic.h>
#include <arch/processor.h>
#include <arch/pic.h>
#include <arch/io.h>

RESERVE_HWIRQ(LAPIC_TIMER_IRQ);

static timerHandler_t lApicHandler[CONFIG_NO_CPUS];
static hwTimer_t lApicTimer[CONFIG_NO_CPUS];
static hwTime_t lApicTimerHz[CONFIG_NO_CPUS];

#define CALIBRATE_TIME      10000
#define CALIBRATE_MULT      100
__VBOOT xm_u32_t CalibrateLApicTimer(void) {
    xm_u64_t start, stop;
    xm_u32_t t1 = 0xffffffff, t2;

    stop = GetSysClockUsec();
    while ((start=GetSysClockUsec()) == stop);
    LApicWrite(APIC_TMICT, t1);
    do {
        stop=GetSysClockUsec();
    } while ((stop-start) < (CALIBRATE_TIME-1));
    while (GetSysClockUsec()==stop);
    t2 = LApicRead(APIC_TMCCT);

    return (t1-t2)*CALIBRATE_MULT;
}

static void TimerIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
    if (lApicHandler[GET_CPU_ID()])
	(*lApicHandler[GET_CPU_ID()])();
}

static xm_s32_t InitLApicTimer(void) {
    localCpu_t *cpu=GET_LOCAL_CPU();

    // LAPIC Timer divisor set to 4
    LApicWrite(APIC_TDCR, APIC_TDR_DIV_32);
    lApicTimerHz[GET_CPU_ID()]=CalibrateLApicTimer();
    lApicTimer[GET_CPU_ID()].freqKhz=lApicTimerHz[GET_CPU_ID()]/1000;
    cpu->globalIrqMask&=~(1<<LAPIC_TIMER_IRQ);
    SetIrqHandler(LAPIC_TIMER_IRQ, TimerIrqHandler, 0);
    LApicWrite(APIC_LVTT, (LAPIC_TIMER_IRQ+FIRST_EXTERNAL_VECTOR)|APIC_LVT_MASKED);
    LApicWrite(APIC_TMICT, 0);
    LApicWrite(APIC_EOI, 0);
    HwEnableIrq(LAPIC_TIMER_IRQ);
    lApicTimer[GET_CPU_ID()].flags|=HWTIMER_ENABLED|PER_CPU;

    return 1;
}

static void SetLApicTimer(xmTime_t interval) {
    hwTime_t apicTmict=(interval*lApicTimerHz[GET_CPU_ID()])/USECS_PER_SEC;

    HwEnableIrq(LAPIC_TIMER_IRQ);
    LApicWrite(APIC_TMICT, (xm_u32_t)apicTmict);
}

static xmTime_t GetMaxIntervalLApic(void) {
    return 1000000LL; // 1s
}

static xmTime_t GetMinIntervalLApic(void) {
    return 50LL; // 50usec
}

static timerHandler_t SetTimerHandlerLApic(timerHandler_t TimerHandler) {
    timerHandler_t OldLApicUserHandler=lApicHandler[GET_CPU_ID()];
  
    lApicHandler[GET_CPU_ID()]=TimerHandler;

    return OldLApicUserHandler;
}

static void ShutdownLApicTimer(void) {
    lApicTimer[GET_CPU_ID()].flags&=~HWTIMER_ENABLED;
}

static hwTimer_t lApicTimer[CONFIG_NO_CPUS]={[0 ... (CONFIG_NO_CPUS-1)]={
        .name="LApic timer",
        .flags=0,
        .InitHwTimer=InitLApicTimer,
        .SetHwTimer=SetLApicTimer,
        .GetMaxInterval=GetMaxIntervalLApic,
        .GetMinInterval=GetMinIntervalLApic,
        .SetTimerHandler=SetTimerHandlerLApic,
        .ShutdownHwTimer=ShutdownLApicTimer,
    }
};

hwTimer_t *GetSysHwTimer(void) {
    return &lApicTimer[GET_CPU_ID()];
}

