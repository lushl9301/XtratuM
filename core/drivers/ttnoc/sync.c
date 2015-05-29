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
* @file sync.c
* @author Christian Paukovits
* @date August 6th, 2009
* @version September 7th, 2009
* @brief implementation of port synchronization
* 
* This file contains the functions that realize the semantics of the port
* synchronization. These functions are the "backend", usually they are
* used by the send/receive functions intended for usage by the user. However,
* it is also possible for the user to invoke them directly and deal with the
* low-level port synchronization.
*
******************************************************************************/

#ifdef _XM_KERNEL_
#include <drivers/ttnocports.h>
#else
#include <stdio.h>

/* plattform-specific includes (visible due to include path in build flow) */
//#include "avalon_tiss.h"
#include "ttsoc_types.h"
#include "lowlevel_io.h"

/* includes of generic source code parts of the driver (same directory) */
#include "error_codes.h"
#include "port.h"
#include "mapping.h"
#include "memories.h"
#include "sync.h"
#endif
/**
 * @return an error code
 * @param id the identifier of the port to operate on
 * @param msgptr a pointer to the pointer of the message buffer
 * @param blocking wait until a new message has become ready
 * @brief determines the address of the current message to be operated
 * 
 * This function calculates the address (in the CPUs address space) of the
 * beginning of the current message to be operated. We assume that that
 * message resides in the Port Memory.
 * The function realizes the port synchronization protocol. Additionally, it
 * distinguishes between the semantics of the messages, so that this function
 * is generic with respect to message type (sporadic/periodic) and port
 * synchronization mode (implicit/explicit). 
 * 
 * In case of an incoming sporadic message, blocking indicates that the
 * functions polls until the arrival of a new message when the queue is empty.
 * For all other directions and message types blocking mode is not supported.
 * 
 * In order to use blocking mode that parameter must be different from 0
 * (preferably 1). In case of 0, blocking mode is deactivated.
 * 
 * The information concerning the arrival of a new message is derived from the
 * information contained in the Port Synchronization Memory for the given port.
 * 
 * On success the function returns TTSOC_ERRCODE_SUCCESS.
 * The possible erros that can occure are:
 * - TTSOC_ERRCODE_INVALID_PORTID ... the given port identifier is invalid
 * - TTSOC_ERRCODE_QUEUE_FULL ... the message queue is full (sporadic only)
 * - TTSOC_ERRCODE_QUEUE_EMPTY ... the message queue is empty (sporadic only)
 * - TTSOC_ERRCODE_BUSY ... an outgoing periodic message is currently
 * transmitted and the shadow buffer is also locked
 * 
 * @attention For incoming periodic message the NBW protocol can not be
 * considered here! It can only be applied in the receive function or 
 * explicitly by user code .
 */

ttsoc_errcode ttsoc_getMsgPtr(ttsoc_tiss_base ni, ttsoc_portid id, void **msgptr, char blocking)
{
    ttsoc_register pflag;   /* buffer for port synchronization flags */
    ttsoc_port *port;       /* pointer to the current ttsoc_port object */
    ttsoc_pi_word *base = 0;    /* byte-aligned port base address */
	// ttsoc_pi_word  base;
	 
    /* find the ttsoc_port object in sysports that is given by id */
    port = ttsoc_getPort(ni, id);
    
    /* if there exists no port with the given identifier */
    if( !port )
        /* abort the function with an error code */
        return TTSOC_ERRCODE_INVALID_PORTID;

    /* calculate byte-aligned port address address (from CPU point of view) */
    //base = (ttsoc_pi_word *) ( ((unsigned int) port->config.pcfg.BaseAddr) + TISS_PI_0);

	 switch(ni)
	 {
			case 0: base=(ttsoc_pi_word *) ( ((unsigned int) port->config.pcfg.BaseAddr) *
	 										   sizeof(ttsoc_pi_word) + TISS_PI_0);
			//base = (unsigned int)port->config.pcfg.BaseAddr + TISS_PI_0;
			break;
			case 1: base=(ttsoc_pi_word *) ( ((unsigned int) port->config.pcfg.BaseAddr) *
	 										   sizeof(ttsoc_pi_word) + TISS_PI_1);
			//base = (unsigned int)port->config.pcfg.BaseAddr + TISS_PI_1;
			break;
			default:
			break;
	 }
    /* fetch the current value from the Port Synchronization Memory */
    pflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);
    
    /* if we operate a port with periodic messages */
    if( port->config.pcfg.MT == 0 )
    {
        /* the direction is outgoing */
        if( port->dir == outgoing )
        {
            /* the outgoing periodic message uses explicity synchronization */
            if( port->config.pcfg.PS == 0 )
            {
                /* check whether the active buffer is currently sent */
                if( pflag.psync.host_addr != pflag.psync.tiss_addr )
                {
                    /* set the msgptr to 0, indicating an error */
                    *msgptr = 0;
                    /* exit function with error code */
                    return TTSOC_ERRCODE_BUSY;
                }
                /* port not busy, continue with normal operation */

                /* calculate the beginning of the current message */
                /* if the currently active buffer is the lower half ...*/
                if( pflag.psync.tiss_addr == 0 )
					 {
                    /* ... then the shadow buffer is the upper half */
                    *msgptr = (ttsoc_pi_word *) (base + port->msgsize);
					 }
                else
                    /* ... otherwise the shadow buffer is the lower half */
                    *msgptr = (ttsoc_pi_word *) base;
                
            }
            /* the outgoing periodc message uses implicit synchronization */
            else
                /* simply assign the port base address */
                *msgptr = (ttsoc_pi_word *) base;
        }
        /* the direction is incoming */
        else{
			/* the synchronization mode is not relevant for incoming periodic
			 * messages; simply assign the port base address */
			*msgptr = (ttsoc_pi_word *) base;
	}
                
    }
    /* we operate a port with sporadic messages */
    else
    {
        /* the synchronization mode is not relevant for periodic messages */
        /* we have to use the ring buffer message queue anyway */

        /* the direction is outgoing */
        if( port->dir == outgoing )
        {
            /* check whether the message queue is full */
            if( (pflag.psync.host_addr == pflag.psync.tiss_addr) &&
                (pflag.psync.host_toof != pflag.psync.tiss_toof) )
            {
                /* set the msgptr to 0, indicating an error */
                *msgptr = 0;
                /* exit function with error code */
                return TTSOC_ERRCODE_QUEUE_FULL;
            }

        }
        /* the direction is incoming */
        else
        {
            /* when using blocking mode */
            if( blocking )
            {
                /* loop until queue is no longer empty */
                while((pflag.psync.host_addr==pflag.psync.tiss_addr)&&
                      (pflag.psync.host_toof==pflag.psync.tiss_toof))
                {
                    /* re-read current value form Port Synchronization Memory */
                    pflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC , id);
                }
            }
            /* no blocking mode used */
            else
            {
                /* check whether the queue is empty */
                if((pflag.psync.host_addr == pflag.psync.tiss_addr) &&
                   (pflag.psync.host_toof == pflag.psync.tiss_toof) )
                {
                    /* set the msgptr to 0, indicating an error */
                    *msgptr = 0;
                    /* exit function with error code */
                    return TTSOC_ERRCODE_QUEUE_EMPTY;
                }
            }
        }

        /* calculate the beginning of the current event message;
		 * considering time stamps */
        *msgptr = ((ttsoc_pi_word *) base) + ((unsigned int)pflag.psync.host_addr);
    }
    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}

/**
 * @return an error code
 * @param id the identifier of the port to operate on
 * @brief sets the port synchronization flags after consuming/completing
 * a message
 * 
 * This function handles the port synchronization flags in the Port
 * Synchronization Memory for a given port. This is necessary, after the user
 * application has consumed a message from an incoming port, and after the user
 * application has completely placed a message in an outgoing port.
 * 
 * Moreover, the function applies the synchronization protocol from the
 * host-side. Additionally, it distinguishes between the semantics of the
 * messages, so that this function is generic with respect to message type
 * (sporadic/periodic) and port synchronization mode (implicit/explicit). 
 * 
 * On success the function returns TTSOC_ERRCODE_SUCCESS.
 * The possible erros that can occure are:
 * - TTSOC_ERRCODE_INVALID_PORTID ... the given port identifier is invalid
 */

ttsoc_errcode ttsoc_iterMsgPtr(ttsoc_tiss_base ni, ttsoc_portid id)
{
    ttsoc_register pflag;   /* buffer for port synchronization flags */
    ttsoc_port *port;       /* pointer to the current ttsoc_port object */

    /* find the ttsoc_port object in sysports that is given by id */
    port = ttsoc_getPort(ni, id);
    
    /* if there exists no port with the given identifier */
    if( !port )
        /* abort the function with an error code */
        return TTSOC_ERRCODE_INVALID_PORTID;

    /* fetch the current value from the Port Synchronization Memory */
    pflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);

    /* if we operate a port with periodic messages */
    if( port->config.pcfg.MT == 0 )
    {
        /* if the have outgoing messages with explicit synchronization */
        /* NOTE: incoming and outgoing messages with implicit synchronization
         * are irrelevant here
         */
        if( port->dir == outgoing && port->config.pcfg.PS == 0 )
        {
            /* toggle the buffer halves to switch active/shadow buffer */
            pflag.psync.host_addr = (pflag.psync.host_addr ? 0:port->msgsize);
            /* write the new value into the Port Synchronization Memory */
            ttsoc_ci_write(ni, TTSOC_NS_PSYNC, id, pflag.value);
        }
    }
    /* we operate a port with sporadic messages */
    else
    {
        /* calculate new value of host-side pointer in the ring buffer */
        /* also consider time stamping */
        pflag.psync.host_addr = pflag.psync.host_addr +
                                port->msgsize + (port->config.pcfg.TS ?
								                 TTSOC_TIMESTAMP_SIZE : 0);
        
        /* detect an overflow of the message queue */
        if( pflag.psync.host_addr == port->config.pcfg.QLength )
        {
            /* reset the host-side address */
            pflag.psync.host_addr = 0;
            /* toggle the "toggle on overflow" field */
            pflag.psync.host_toof = (pflag.psync.host_toof ? 0 : 1);
        }

        /* write the new value into the Port Synchronization Memory */
        ttsoc_ci_write(ni, TTSOC_NS_PSYNC, id, pflag.value);
    }

    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}
