/*
 * $FILE: tsc.c
 *
 * TSC driver
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

#ifdef CONFIG_TSC_CLOCK
#include <ktimer.h>
#include <processor.h>
#include <stdc.h>

static hwClock_t tscClock;

static xm_s32_t InitTscClock(void) {    
    tscClock.freqKhz=cpuKhz;
    tscClock.flags|=HWCLOCK_ENABLED;
    return 1;
}

static xmTime_t ReadTscClockUsec(void) {
    hwTime_t tscTime=RdTscLL();
    return HwTime2Duration(tscTime, tscClock.freqKhz*1000);
}

#if 0
static hwTime_t ReadTscClock(void) {
    hwTime_t tscTime=RdTscLL();
    return tscTime;
}
#endif

static hwClock_t tscClock={
    .name="TSC clock",
    .flags=0,
    .freqKhz=0,
    .InitClock=InitTscClock,
    .GetTimeUsec=ReadTscClockUsec,
    .ShutdownClock=0,
};

hwClock_t *sysHwClock=&tscClock;

#endif
