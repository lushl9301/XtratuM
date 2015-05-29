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
* @file system_parameter.h
* @author Christian Paukovits
* @date July 29th, 2009
* @version August 5th, 2009
* @brief system-wide parameters present in the underlying hardware design
*
* This file contains constant, which embody parameters of the underlying
* hardware design. The counterpart in VHDL code is system_parameter.vhd
* The values in this file must correlate with the constants in that file.
*
* @note For detailed description of each constant here also consult the
* corresponding VHDL file.
******************************************************************************/

#ifndef __TTSOC__SYSTEM_PARAMETER_H__
#define __TTSOC__SYSTEM_PARAMETER_H__

/** @name ported constants
*
*   These constant are the C-equivalent of system_parameter.vhd
*   Furthermore, they are the basis for the derived constants in the next
*   block.
*/

/*@{*/

/** Width in bits of the data words in the Port Memory */
#define TTSOC_PI_WORD_WIDTH 32
/** Number of supported periods */
#define TTSOC_NR_PERIODS 4
/** Index of the period bit of the highest period */
#define TTSOC_MSB_PERIODBIT 20
/** Index of the macro tick bit */
#define TTSOC_MACROTICK_BIT 5
/** Width of the phase offset */
#define TTSOC_PHASESLICE_WIDTH 12
/** Distance between two period bits ("period delta") */
#define TTSOC_PERIOD_DELTA 1
/** Top value of Receive Window Counter */
#define TTSOC_RECVWIN_TOPVALUE 10

/*@}*/

/** @name derived constants
*
*   These constants are derived from the "ported constants" in the prior block.
*   They are the work-around for the lack of a logarithm dualis (ld) function
*   that should be applied on the "ported constants" (as in the VHDL code).
*   Consequently, any modification to constants from the prior block implies an
*   adaption of derived constants here.
*/

/*@{*/

#define TTSOC_PERIOD_WIDTH 2    /**< ld(TTSOC_NR_PERIODS) */

/*@}*/

#endif
