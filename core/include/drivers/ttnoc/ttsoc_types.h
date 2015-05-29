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
* @file ttsoc_types.h
* @author Christian Paukovits & Mikel Azkarate-askasua
* @date July 29th, 2009
* @version September 28th, 2009
* @brief typedefs of datatypes associated with TTSoC-specific entities
*
* This file contains "symbolic" type definitions (typedefs) for datatypes,
* which are used for specific entities given by the TTSoC Architecture, e.g.
* the type used to embody the identifier of architectural ports.
*
* @attention This file is plattform-specific because of the size of native
* datatypes, e.g. size of integer vs. long.
*
* @attention The types must be aligned to the parameters defined in 
* system_parameter.h and tiss_parameter.h, or even to parameters that are
* derived from them in VHDL code, e.g. in tiss.vhd
******************************************************************************/

#ifndef __TTSOC__TTSOC_TYPES_H__
#define __TTSOC__TTSOC_TYPES_H__

//#include <alt_types.h>
//#include <xbasic_types.h>

typedef struct
{
	unsigned long Upper;
	unsigned long Lower;
} Xuint64;

//#define XUINT64_MSW(x) ((x).Upper)
//#define XUINT64_LSW(x) ((x).Lower)

/** the type of "internal" error codes produced by the driver's functions */
typedef signed char ttsoc_errcode;	/* does not need correlation */

/** the type that equals the size of the NoC flit 
  * @note The size of flits is the same as data words at the Port Interface */
typedef unsigned long ttsoc_flit;
/** the type that models generic data words at the Control Interface */
typedef unsigned long ttsoc_ci_word;
/** auxiliary constant because we can't use "sizeof" in preprocessor constants */
#define TTSOC_CI_WORD_WIDTH 32
/** the type of data words at the Port Interface resp. in the Port Memory */
typedef ttsoc_flit ttsoc_pi_word;
/** the numeric (not pointer) type that embodies addresses at the Port
  * Interface (also used for message sizes) */
typedef unsigned short ttsoc_pi_addr;
/** the type of the numeric identifier of architectural ports */
typedef unsigned char ttsoc_portid;
/** the numeric (not pointer) type of address at the Control Interface */
typedef unsigned short ttsoc_ci_addr;
/** the type of a time stamp */
typedef Xuint64 ttsoc_timestamp; 

typedef unsigned char ttsoc_tiss_base;

#endif
