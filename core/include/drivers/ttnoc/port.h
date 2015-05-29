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
* @file port.h
* @author Christian Paukovits
* @date August 5th, 2009
* @version August 6th, 2009
* @brief declarations of data structures and auxiliary functions for ports
*
* This is the header file associated with port.c. It contains the declaration
* of the data structure ttsoc_port, which models an architectural port.
* Besides this, it declares the global functions that are used to access
* objects of the ttsoc_port type.
******************************************************************************/

#ifndef __TTSOC__PORT_H__
#define __TTSOC__PORT_H__

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"

/* includes of generic source code parts of the driver (same directory) */
#include "memories.h"

typedef enum
{
    incoming = 0,
    outgoing = 1,
}ttsoc_direction;    

typedef struct
{
    ttsoc_register config;      /**< duplication of port configuration */
    void (*msgcmpl)(void *);    /**< callback function on "message complete" */
    ttsoc_direction dir;        /**< direction of transmission (as in BCFG) */
    ttsoc_ci_addr msgsize;      /**< size of message (as in BCFG) */
}ttsoc_port;    

extern inline ttsoc_port *ttsoc_getPort(ttsoc_tiss_base ni, ttsoc_portid);
extern inline ttsoc_portid ttsoc_getPortID(ttsoc_tiss_base ni, ttsoc_port *);
extern ttsoc_errcode ttsoc_getPortCfg(ttsoc_tiss_base ni, ttsoc_portid);
extern ttsoc_errcode ttsoc_setPortCfg(ttsoc_tiss_base ni, ttsoc_portid);

#endif
