/******************************************************************************
* This file is part of TTSoC-NG.
* Copyright (C) 2009, Christian Paukovits
*
* TTSoC-NG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* TTSoC-NG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with TTSoC-NG.  If not, see <http://www.gnu.org/licenses/>.
******************************************************************************/

/**************************************************************************//**
* @file regfile.h
* @author Christian Paukovits
* @date August 5th, 2009
* @version August 5th, 2009
* @brief associated header file with regfile.c
*
* This header file contains the declarations of global functions, which are
* offered to the user to execute the access towards the TISS's Register File.
******************************************************************************/

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"

/** the pattern associated with the watchdog life sign / heart beat
 * @note constant must be adapted in case of resizing of the data type */
#define TTSOC_WATCHDOG_LIFESIGN ((ttsoc_ci_word) 0x55555555)

extern inline ttsoc_timestamp ttsoc_getGlobalTime(ttsoc_tiss_base ni);
extern inline void ttsoc_getTimerSettings(ttsoc_tiss_base ni, ttsoc_timestamp *, ttsoc_timestamp *);
extern inline void ttsoc_setTimerSettings(ttsoc_tiss_base ni, ttsoc_timestamp, ttsoc_timestamp);
extern void inline ttsoc_lifesign(ttsoc_tiss_base ni);
extern inline ttsoc_ci_word ttsoc_getErrorStatus(ttsoc_tiss_base ni);
extern inline void ttsoc_setErrorStatus(ttsoc_tiss_base ni, ttsoc_ci_word);
