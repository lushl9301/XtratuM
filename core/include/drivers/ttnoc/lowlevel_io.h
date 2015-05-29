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
* @file lowlevel_io.h
* @author Christian Paukovits
* @date July 31st, 2009
* @version September 15th, 2009
* @brief plattform-specific wrapping code for low-level register access
*
* This file declares inline functions and macros, which wrap the low-level
* access to the TISS's Register File via the Control Interface. The purpose
* is to abstract from the platform-specific operations and to create an
* uniform I/O layer within the driver.
******************************************************************************/

#ifndef __TTSOC__LOWLEVEL_IO__H__
#define __TTSOC__LOWLEVEL_IO__H__

//#include "system.h"
//#include "xparameters.h"

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"
/* includes of generic source code parts of the driver */
#include "tiss_parameter.h"

/** @name preparation of base addresses
 * 
 * This block defines auxiliary macros to introduce the plattform-independent
 * base addresses of Port Interface and Control Interface. Basically, these are
 * preprocessor "hacks", which use string concatenation. The goal is to create
 * the proper component name as used in the SOPC builder, and to reuse the
 * base address macros from system.h
 */
/*@{*/

/** define default values for base address macro for Port Interface and Control
 *  Interface with respect to the identifier of the current CPU */


#define TISS_CI_0 0xFFF60000//XPAR_TTSOC_CI_WRAPPER_0_MEM0_BASEADDR
#define TISS_PI_0 0xFFF64000//XPAR_TTSOC_PI_WRAPPER_0_MEM0_BASEADDR

#define TISS_CI_1 0
#define TISS_PI_1 0

#define TISS_NUMBER 1

/*@}*/

/** @name address calculation and access functions (Control Interface only) */

/*@{*/

/** namespace-aware calculation of addresses at the Control Interface */

#define TTSOC_CI_CALCADDR(ns, reg) ( ((ns) << TTSOC_CI_ADDR_WIDTH) | (reg) )

extern inline ttsoc_ci_word ttsoc_ci_read(ttsoc_tiss_base ni, ttsoc_ci_addr ns, ttsoc_ci_addr reg);
extern inline void ttsoc_ci_write(ttsoc_tiss_base ni, ttsoc_ci_addr ns, ttsoc_ci_addr reg, ttsoc_ci_word data);

/*@}*/

#endif
