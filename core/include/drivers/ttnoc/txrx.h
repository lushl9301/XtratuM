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
* @file txrx.h
* @author Christian Paukovits
* @date August 6th, 2009
* @version August 6th, 2009
* @brief associated header file with rxtx.c
* 
* This file contains the declarations of the global send/receive functions
******************************************************************************/

#ifndef __TTSOC__TXRX_H__
#define __TTSOC__TXRX_H__

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"
//#include <xm.h>

typedef xmSize_t size_t;

extern ttsoc_errcode ttsoc_receiveMsg(ttsoc_tiss_base ni, ttsoc_portid, void *, char);
extern ttsoc_errcode ttsoc_sendMsg(ttsoc_tiss_base ni, ttsoc_portid, void *, char);

#endif
