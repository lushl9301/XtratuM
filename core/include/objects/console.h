/*
 * $FILE: console.h
 *
 * Console definition
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_OBJ_CONSOLE_H_
#define _XM_OBJ_CONSOLE_H_

#ifdef _XM_KERNEL_
#include <kdevice.h>

extern void ConsoleInit(const kDevice_t *kDev);
extern void ConsolePutChar(xm_u8_t c);

struct console {
    const kDevice_t *dev;
};

#endif
#endif
