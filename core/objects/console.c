/*
 * $FILE: console.c
 *
 * Object console
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
#include <rsvmem.h>
#include <boot.h>
#include <hypercalls.h>
#include <sched.h>
#include <spinlock.h>
#include <objdir.h>
#include <stdc.h>

#include <objects/console.h>

static struct console xmCon, *partitionConTab;
static spinLock_t consoleLock=SPINLOCK_INIT;

void ConsolePutChar(xm_u8_t c) {
    if (xmCon.dev) {
	if (KDevWrite(xmCon.dev, &c, 1)!=1) {
	    KDevSeek(xmCon.dev, 0, DEV_SEEK_START);
	    KDevWrite(xmCon.dev, &c, 1);
	}
    }
}

static inline xm_s32_t WriteMod(struct console *con, xm_u8_t *b) {
    if (KDevWrite(con->dev, b, 1)!=1) {
        KDevSeek(con->dev, 0, DEV_SEEK_START);
        if (KDevWrite(con->dev, b, 1)!=1)
            return 0;
    }
    return 1;
}

static xm_s32_t WriteConsoleObj(xmObjDesc_t desc, xm_u8_t *__gParam buffer, xmSize_t length) {
    localSched_t *sched=GET_LOCAL_SCHED();
    xmId_t partId=OBJDESC_GET_PARTITIONID(desc);
    struct console *con;
    xm_s32_t e;

    if (partId!=KID2PARTID(sched->cKThread->ctrl.g->id))
	return XM_PERM_ERROR;

    if (CheckGParam(buffer, length, 1, PFLAG_NOT_NULL)<0)
        return XM_INVALID_PARAM;

    // Only strings of a maximum of 128 bytes are allowed
    if (length>128)
        return XM_INVALID_PARAM;

    con=(partId==XM_HYPERVISOR_ID)?&xmCon:&partitionConTab[partId];

    SpinLock(&consoleLock);
    for (e=0; e<length; e++) {
	PreemptionOn();
	PreemptionOff();
        if (!WriteMod(con, &buffer[e])) {
            SpinUnlock(&consoleLock);
            return e;
        }
    }
    SpinUnlock(&consoleLock);
    return length;    
}

static const struct object consoleObj={
    .Write=(writeObjOp_t)WriteConsoleObj,
};

xm_s32_t __VBOOT SetupConsole(void) {
    xm_s32_t e;

    GET_MEMZ(partitionConTab, sizeof(struct console)*xmcTab.noPartitions);
    xmCon.dev=LookUpKDev(&xmcTab.hpv.consoleDev);
    objectTab[OBJ_CLASS_CONSOLE]=&consoleObj;
    for (e=0; e<xmcTab.noPartitions; e++) {
	partitionConTab[e].dev=LookUpKDev(&xmcPartitionTab[e].consoleDev);
    }
    return 0;
}

REGISTER_OBJ(SetupConsole);
