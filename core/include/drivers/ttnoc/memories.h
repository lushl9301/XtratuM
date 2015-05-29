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
* @file memories.h
* @author Christian Paukovits
* @date July 30th, 2009
* @version September 14th, 2009
* @brief data structures modelling the TISS memories
*
* This file specifies data structures, which model the entries of the
* internal memories of the TISS. These are the C-equivalents of the record
* data types in VHDL, e.g. in tiss.vhd
*
* @attention The arrangement of fields within the bitfields must reflect the
* memory mappings in memorymap.vhd as well as registermap.vhd!
******************************************************************************/

#ifndef __TTSOC__MEMORIES_H___
#define __TTSOC__MEMORIES_H___

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"

/* includes of generic source code parts of the driver (same directory) */
#include "system_parameter.h"
#include "tiss_parameter.h"


/** net width/sum of entries of the Port Synchronization Memory */
//#if (TTSOC_PI_ADDR_WIDTH + 1) >= TTSOC_NBWSEQ_WIDTH
    #define TTSOC_PSYNCMEM_DATA_WIDTH ((TTSOC_PI_ADDR_WIDTH + 1)*2)
//#else
//    #define TTSOC_PSYNCMEM_DATA_WIDTH (TTSOC_NBWSEQ_WIDTH*2)
//#endif

typedef union
{
	/** modelling the entries of the Port Configuration Memory */
	struct
	{
		unsigned empty: 3;
	   /** queue length (in data words of the Port Memory) */
		unsigned QLength : TTSOC_PI_ADDR_WIDTH;
		/** port base address */
		unsigned BaseAddr : TTSOC_PI_ADDR_WIDTH;
		unsigned TS : 1;	/**< time stamp enable: 0..disabled | 1..enabled */
		unsigned IE : 1;	/**< interrupt enable: 0..disabled | 1..enabled */
		unsigned PS : 1;	/**< port synchronization: 0..explicit | 1..implicit */	
		unsigned MT : 1;	/**< message type: 0..periodic | 1..sporadic */
		unsigned PE	: 1;	/**< port enable: 0..disabled | 1..enabled */
	} pcfg;

	/** modelling the entries of the Port Synchronization Memory */ 
	/** semantics of sporadic and periodic messages */
	struct
	{
		/** - sporadic message: host-side position in message queue
		*	- outgoing periodic message: start address of current buffer
		*/
      unsigned empty : 6;	
		/** similar to host_toof, but TISS-side */
		unsigned tiss_toof	:	1;
		/** similar to host_addr, but TISS-side
         *  also included NBW sequencer for incoming periodic messages */
		unsigned tiss_addr	:	TTSOC_PI_ADDR_WIDTH;
		/** host-side toogle on overflow (sporadic messages only) */
		unsigned host_toof	:	1;		
		unsigned host_addr	:	TTSOC_PI_ADDR_WIDTH;
	} psync;

	/** pure numeric value of the Control Interface data word */
	ttsoc_ci_word value;

    /** modelling the Error & Status Register in the TISS's Register File */
    struct
    {
	     unsigned empty      :  16;
        unsigned CommDis    :   1;  /**< communication service disenable (0...enabled / 1...disabled)*/
        unsigned TimerEna   :   1;  /**< generic timer service enable */
        unsigned HostMode   :   TTSOC_HOSTMODE_WIDTH;   /**< Host Mode */		  
        unsigned WDPeriod   :   TTSOC_PERIOD_WIDTH;     /**< watchdog period */
        unsigned OvflPort   :   TTSOC_PORTID_WIDTH;     /**< overflow port */
        unsigned Ovfl       :   1;  /**< queue overflow error */
        unsigned MemErr     :   1;  /**< Memory Error (from Port Interface */
        unsigned CfgErr     :   1;  /**< port configuration error */
        unsigned CommErr    :   1;  /**< communication error (unused) */
        unsigned WDMiss     :   1;  /**< watchdog miss */		  
        unsigned RepErr     :   1;  /**< repeated error */
    } errstat;

}ttsoc_register;

#endif
