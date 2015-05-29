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
* @file error_codes.h
* @author Christian Paukovits
* @date July 29th, 2009
* @version August 6th, 2009
* @brief error codes of driver functions
*
* This file defines the error codes that are returned by the driver's
* functions.
******************************************************************************/

#ifndef __TTSOC__ERROR_CODES_H__
#define __TTSOC__ERROR_CODES_H__

#define TTSOC_ERRCODE_SUCCESS 0
#define TTSOC_ERRCODE_INVALID_PORTID -1
#define TTSOC_ERRCODE_QUEUE_FULL -2
#define TTSOC_ERRCODE_QUEUE_EMPTY -3
#define TTSOC_ERRCODE_BUSY -4

#endif
