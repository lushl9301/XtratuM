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
* @file irq.h
* @author Christian Paukovits
* @date July 31st, 2009
* @version August 25th, 2009
* @brief header file associated with interrupt handling
*
* This file is the corresponding header file to irq.c, which implements the
* plattform-independent interrupt handling.
******************************************************************************/

#ifndef __TTSOC__IRQ_H__
#define __TTSOC__IRQ_H__

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"

/* includes of generic source code parts of the driver (same directory) */
#include "tiss_parameter.h"

#if TTSOC_NR_PORTS % TTSOC_CI_WORD_WIDTH
    #define TTSOC_IRQ_FLAG_REGS (TTSOC_NR_PORTS / TTSOC_CI_WORD_WIDTH + 1)
#else
    #define TTSOC_IRQ_FLAG_REGS (TTSOC_NR_PORTS / TTSOC_CI_WORD_WIDTH)
#endif

/* ordering of interrupt sources */
#define TTSOC_IRQ_ERROR		0
#define TTSOC_IRQ_MSGCMPL	1
#define TTSOC_IRQ_RECONF	2
#define TTSOC_IRQ_TIMER		3
#define TTSOC_IRQ_INDEX(x)  (TTSOC_IRQ_SOURCES+x)

/* bit masks to pinpoint the mask bits of interrupt sources */
#define TTSOC_IRQ_ERROR_MASK	(1<<TTSOC_IRQ_ERROR)
#define TTSOC_IRQ_MSGCMPL_MASK	(1<<TTSOC_IRQ_MSGCMPL)
#define TTSOC_IRQ_RECONF_MASK	(1<<TTSOC_IRQ_RECONF)
#define TTSOC_IRQ_TIMER_MASK	(1<<TTSOC_IRQ_TIMER)

/* bit masks to pinpoint the mas bits of the interrupt indexes */
#define TTSOC_IRQ_INDEX_MASK(x) (1<<TTSOC_IRQ_INDEX(x))

/* declaration of global function that is the "master" IRQ callback function */
//extern void ttsoc_master_irq(void *context, int id);
/* declaration of global get/set function to handle the interrupt masks */
extern inline ttsoc_ci_word ttsoc_getInterruptMask(ttsoc_tiss_base ni);
extern inline void ttsoc_setInterruptMask(ttsoc_tiss_base ni, ttsoc_ci_word);
/* declaration of set function to customize IRQ callback functions */
//extern void ttsoc_setIRQCallback(unsigned int, void (*)(void *));

#endif
