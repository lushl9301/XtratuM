/*
 * $FILE: kdevice.c
 *
 * Kernel devices
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
#include <stdc.h>
#include <processor.h>

const kDevice_t *(*GetKDevTab[NO_KDEV])(const xm_u32_t subId);

void SetupKDev(void) {
    extern xm_s32_t (*kDevSetup[])(void);    
    xm_s32_t e;
    memset(GetKDevTab, 0, sizeof(kDevice_t *(*)(const xm_u32_t subId))*NO_KDEV);
    for (e=0; kDevSetup[e]; e++)
        kDevSetup[e]();
}

const kDevice_t *LookUpKDev(const xmDev_t *dev) {
    if ((dev->id<0)||(dev->id>=NO_KDEV)||(dev->id==XM_DEV_INVALID_ID))
	return 0;

    if (GetKDevTab[dev->id])
        return GetKDevTab[dev->id](dev->subId);

    return 0;
}

