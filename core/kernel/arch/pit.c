/*
 * $FILE: pit.c
 *
 * pit driver
 *
 * $VERSION: 1.0
 *
 * $AUTHOR$
 *
 * $LICENSE:  
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 *
 */
#ifdef CONFIG_PC_PIT_TIMER
#include <assert.h>
#include <boot.h>
#include <kdevice.h>
#include <ktimer.h>
#include <smp.h>
#include <stdc.h>
#include <processor.h>
#include <arch/io.h>

// Definitions
#define PIT_IRQ_NR 0
#define PIT_HZ 1193182UL
#define PIT_KHZ 1193UL
#define PIT_ACCURATELY 60
#define PIT_MODE 0x43
#define PIT_CH0 0x40
#define PIT_CH2 0x42

// Read-back mode constants
#define PIT_LATCH_CNT0 0xD2
#define PIT_LATCH_CNT2 0xD8
#define PIT_LATCH_CNT0_2 0xDA

RESERVE_HWIRQ(PIT_IRQ_NR);
RESERVE_IOPORTS(0x40, 4);

#ifdef CONFIG_PC_PIT_CLOCK
#define PIT_PERIOD_USEC 1000
#define PIT_PERIOD ((PIT_PERIOD_USEC*PIT_HZ)/USECS_PER_SEC)
static struct pitClockData {
    volatile xm_u32_t ticks;
} pitClockData;
#endif

static hwTimer_t pitTimer;
static timerHandler_t pitHandler;

static inline void SetPitTimerHwt(xm_u16_t pitCounter) {
#ifndef CONFIG_PC_PIT_CLOCK
    // ONESHOOT_MODE
    OutBP(0x30, PIT_MODE);
    OutBP(pitCounter&0xff,PIT_CH0);
    OutBP(pitCounter>>8, PIT_CH0);
#endif
}

static void TimerIrqHandler(cpuCtxt_t *ctxt, void *irqData) {
#ifdef CONFIG_PC_PIT_CLOCK
    pitClockData.ticks++;
#endif
    if (pitHandler)
        (*pitHandler)();
    HwEnableIrq(PIT_IRQ_NR);
}

static xm_s32_t InitPitTimer(void) {
    localCpu_t *cpu=GET_LOCAL_CPU();

    cpu->globalIrqMask&=~(1<<PIT_IRQ_NR);
    SetIrqHandler(PIT_IRQ_NR, TimerIrqHandler, 0);
#ifdef CONFIG_PC_PIT_CLOCK
    OutBP(0x34, PIT_MODE);
    OutBP(PIT_PERIOD&0xff, PIT_CH0);
    OutBP(PIT_PERIOD>>8, PIT_CH0);
#else
    // setting counter 0 in oneshot mode
    OutBP(0x30, PIT_MODE);
#endif
    pitTimer.flags|=HWTIMER_ENABLED;
    HwEnableIrq(PIT_IRQ_NR);
    
    return 1;
}

static void SetPitTimer(xmTime_t interval) {
    xm_u16_t pitCounter=(interval*PIT_HZ)/USECS_PER_SEC;
    SetPitTimerHwt(pitCounter);
}

static xmTime_t GetPitTimerMaxInterval(void) {
    return 1000000; //(0xF0*USECS_PER_SEC)/PIT_HZ;
}

static xmTime_t GetPitTimerMinInterval(void) {
    return PIT_ACCURATELY;
}

static timerHandler_t SetPitTimerHandler(timerHandler_t TimerHandler) {
    timerHandler_t OldPitUserHandler=pitHandler;
    pitHandler=TimerHandler;
    return OldPitUserHandler;
}

static void PitTimerShutdown(void) {
    pitTimer.flags&=~HWTIMER_ENABLED;
    HwDisableIrq(PIT_IRQ_NR);
    SetIrqHandler(PIT_IRQ_NR, TimerIrqHandler, 0);
}

static hwTimer_t pitTimer={
#ifdef CONFIG_PC_PIT_CLOCK
    .name="i8253p timer",
#else
    .name="i8253 timer",
#endif
    .flags=0,
    .freqKhz=PIT_KHZ,
    .InitHwTimer=InitPitTimer,
    .SetHwTimer=SetPitTimer,
    .GetMaxInterval=GetPitTimerMaxInterval,
    .GetMinInterval=GetPitTimerMinInterval,
    .SetTimerHandler=SetPitTimerHandler,
    .ShutdownHwTimer=PitTimerShutdown,
};

/*hwTimer_t *sysHwTimer=&pitTimer;*/

hwTimer_t *GetSysHwTimer(void) {
    return &pitTimer;
}

#ifdef CONFIG_PC_PIT_CLOCK

static hwClock_t pitClock;

static xm_s32_t InitPitClock(void) {    
    pitClockData.ticks=0;
    pitClock.flags|=HWCLOCK_ENABLED;
    return 1;
}

static hwTime_t ReadPitClock(void) {
    hwTime_t cTime, t;
    xm_u32_t cnt;
    OutBP(PIT_LATCH_CNT0, PIT_MODE);
    cnt=InBP(PIT_CH0);
    cnt|=InBP(PIT_CH0)<<8;
    ASSERT(cnt<=PIT_PERIOD);
    t=pitClockData.ticks;
    cTime=PIT_PERIOD-cnt;
    return cTime+t*PIT_PERIOD;
}

static xmTime_t ReadPitClockUsec(void) {
    return HwTime2Duration(ReadPitClock(), PIT_HZ);
}

static hwClock_t pitClock={
    .name="PIT clock",
    .flags=0,
    .freqKhz=PIT_KHZ,
    .InitClock=InitPitClock,
    .GetTimeUsec=ReadPitClockUsec,
    .ShutdownClock=0,
};

hwClock_t *sysHwClock=&pitClock;

#endif

#endif


