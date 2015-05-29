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
* @file tiss_parameter.h
* @author Christian Paukovits
* @date July 29th, 2009
* @version September 14th, 2009
* @brief TISS-specific parameters present in the underlying hardware design
*
* This file contains constant, which embody parameters of a TISS of the
* underlying hardware design. The counterpart in VHDL code is
* tiss_parameter.vhd. The values in this file must correlate with the constants
* in that file.
*
* @note For detailed description of each constant here also consult the
* corresponding VHDL file.
******************************************************************************/

#ifndef __TTSOC__TISS_PARAMETER_H___
#define __TTSOC__TISS_PARAMETER_H___

/** @name ported constants
*
*	These constant are the C-equivalent of tiss_parameter.vhd
*	Furthermore, they are the basis for the derived constants in the next
*	block.
*/

/*@{*/

/** Number of provided architectural ports */
#define TTSOC_NR_PORTS 16
/** Number of addressable data words at the Port Interface
 * (i.e. data words in the Port Memory that is attached to a given TISS). */
#define TTSOC_PI_WORD_NUMBER 4096
/** Number of entries in the Time-Triggered Communication Schedule */
#define TTSOC_TTCOMMSCHED_SIZE 32
/** Number of entries in the Burst Configuration Memory */
#define TTSOC_BCFGMEM_SIZE 16
/** Number of data words in the Routing Information Memory */
#define TTSOC_RIMEM_SIZE 4
/** Maximum number of routing flits per burst */
#define TTSOC_MAX_RIFLITS 2
/** Number of supported Host Modes */
#define TTSOC_NR_HOSTMODES 4

/*@}*/

/** @name derived constants
*
*	These constants are derived from the "ported constants" in the prior block.
*	They are the work-around for the lack of a logarithm dualis (ld) function
*	that should be applied on the "ported constants" (as in the VHDL code).
*	Consequently, any modification to constants from the prior block implies an
*	adaption of derived constants here.
*/

/*@{*/

/** Width of the port identifier and address bus of Port Configuration and
*	Port Synchronization Memory.
*/
#define TTSOC_PORTID_WIDTH 4			/* ld(TTSOC_NR_PORTS) */ 
/** Width of the addresses at the Port Interface resp. Port Memory */
#define TTSOC_PI_ADDR_WIDTH 12			/* ld(TTSOC_PI_WORD_NUMBER) */
/** Width of the address bus of the Time-Triggered Communication Schedule */
#define TTSOC_TTCOMMSCHED_ADDR_WIDTH 5	/* ld(TTSOC_TTCOMMSCHED_SIZE) */
/** Width of the address bus of the Burst Configuration Memory */
#define TTSOC_BCFGMEM_ADDR_WIDTH 4		/* ld(TTSOC_BCFGMEM_SIZE) */
/** Width of the addresses in the Routing Information Memory */
#define TTSOC_RIMEM_ADDR_WIDTH	2		/* ld(TTSOC_RIMEM_SIZE) */
/** Width of the length value of routing information */
#define TTSOC_RILEN_WIDTH 2				/* ld(TTSOC_MAX_RIFLITS + 1) */
/** Width of the Host Mode field in the Register File */
#define TTSOC_HOSTMODE_WIDTH 2			/* ld(TTSOC_NR_HOSTMODES) */
/** Width of the Control Interfaces address bus (withouth namespace) */
#define TTSOC_CI_ADDR_WIDTH 4           /* max(ld(REG_NRREGS), PORTID_WIDTH); */
/** Width of the namespace part of the address at the Control Interface */
#define TTSOC_CI_NS_WIDTH 2             /* ld(NS_NR_HOST) */

/*@}*/

/** @name auxiliary constants */

/*@{*/

/** number of interrupt sources in the TISS, required by irq.c */
#define TTSOC_IRQ_SOURCES 4
/** number of data words occupied by a time stamp of the global time */
#define TTSOC_TIMESTAMP_SIZE 2

/*@}*/


#endif
