/*
 * $FILE: devid.h
 *
 * Devices Ids
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#ifndef _XM_DEVID_H_
#define _XM_DEVID_H_

#define XM_DEV_INVALID_ID 0xFFFF

#define XM_DEV_LOGSTORAGE_ID 0
#define XM_DEV_UART_ID 1
#define XM_DEV_VGA_ID 2
#if defined(CONFIG_EXT_SYNC_MPT_IO)||defined(CONFIG_PLAN_EXTSYNC)
#define XM_DEV_SPARTAN6_EXTSYNC_ID 3
#endif
#if defined(CONFIG_DEV_TTNOC)||defined(CONFIG_DEV_TTNOC_MODULE)
#define XM_DEV_TTNOC_ID 4
#endif

#define NO_KDEV 7

#endif
