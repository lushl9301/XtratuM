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
* @file avalon_tiss_driver.h
* @author Christian Paukovits
* @date August 7th, 2009
* @version August 7th, 2009
* @brief central header file of the Avalon TISS driver
*
* This file collects all platform-independent and platform-specific header
* files of the TISS driver. In the end, the user application need only
* include this file and need not take care of each header file individually.
******************************************************************************/

#ifndef __TTSOC__AVALON_TISS_DRIVER_H__
#define __TTSOC__AVALON_TISS_DRIVER_H__

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"
#include "lowlevel_io.h"


/* includes of generic source code parts of the driver */
#include "system_parameter.h"
#include "tiss_parameter.h"
#include "error_codes.h"
#include "mapping.h"
#include "memories.h"
#include "irq.h"
#include "port.h"
#include "regfile.h"
#include "sync.h"
#include "txrx.h"

#endif
