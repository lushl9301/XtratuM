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
* @file mapping.h
* @author Christian Paukovits
* @date August 3rd, 2009
* @version August 5th, 2009
* @brief mappings of registers addresses to address offsets
*
* This file defines the macros, which reflect the mapping between registers,
* e.g. of the Register File, and their numeric address offsets. Note that
* this mapping correlates with the hardware mapping as specified in
* registermap.vhd
* Furthermore, we define the namespaces at the Control Interface, which is also
* reconstructed form registermap.vhd
*
* @note For detailed description of each constant here also consult the
* corresponding VHDL file.
******************************************************************************/

#ifndef __TTSOC__MAPPING_H___
#define __TTSOC__MAPPING_H___

/** @name namespaces at the Control Interface */

/*@{*/

/** namespace of the Port Configuration Memory */
#define TTSOC_NS_PCFG 0
/** namespace of the Port Synchronization Memory */
#define TTSOC_NS_PSYNC 1
/** namespace of the Register File */
#define TTSOC_NS_REGFILE 2

/*@}*/

/** @name register mappings in the TISS-side Register File */

/*@{*/

/** lower half of the time base's counter vector */
#define TTSOC_REG_TIME_L 0
/** upper half of the time_base's counter vector */
#define TTSOC_REG_TIME_U 1
/** watchdog life sign register */
#define TTSOC_REG_WDLIFESIGN 2
/** Error & Status Register plus watchdog period, Host Mode etc. */
#define TTSOC_REG_ERRSTATUS 3
/** lower half of timer pattern register of generic timer service */
#define TTSOC_REG_TIMERPAT_L 4
/** upper half of timer pattern register of generic timer service */
#define TTSOC_REG_TIMERPAT_U 5
/** lower half of the timer mask register of generic timer service */
#define TTSOC_REG_TIMERMSK_L 6
/** upper half of the timer mask register of generic timer service */
#define TTSOC_REG_TIMERMSK_U 7
/** status register of interrupt handling */
#define TTSOC_REG_IRQ_STATUS 8
/** mask register of interrupt handling */
#define TTSOC_REG_IRQ_MASK 9
/** offset where the interrupt flag registers begin */
#define TTSOC_REG_IRQ_FLAGS 10

/*@}*/

#endif
