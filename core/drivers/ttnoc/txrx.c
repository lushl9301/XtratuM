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
*
* Adapted to XM by Javier Coronel (jcoronel@fentiss.com)
* It has been performed under MultiPARTES project. 
* COPYRIGHT (c) 2013
* FentISS
*
******************************************************************************/

/**************************************************************************//**
* @file txrx.c
* @author Christian Paukovits
* @date August 6th, 2009
* @version September 14th, 2009
* @brief implementation of send/receive functions
* 
* This file realizes the send and receive functions, which are intended to be
* invoked by the user to commit messages into Port Memory (send) or to consume
* messages from Port Memory (receive).
******************************************************************************/
#ifdef XM_SRC_PARTITION
#include <xm.h>
#endif
#ifdef _XM_KERNEL_
#include <sparcv8/leon.h>
#include <drivers/ttnocports.h>
#else
#include <string.h>     /* for memcpy(), data type size_t */
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
#include "txrx.h"
#endif
/**
 * @return an error code 
 * @param id the identifier of the port to operate on
 * @param msg a pointer to the message buffer of the message to be received
 * @param blocking wait until a new message has become ready
 * @brief copy a message into Port Memory and initiate port synchronization
 * 
 * This function is intended to be used by user-specific application code.
 * Literally, it "receives" a message. That is, it copies a message from the
 * Port Memory into a message buffer in the CPUs scope. For this purpose, it
 * takes usage of the low-level port synchronization functions ttsoc_getMsgPtr()
 * and ttsoc_iterMsgPtr(). So, this functions wraps the low-level tasks of
 * copy operation and port synchronization.
 * 
 * Besides this, this function applies the Non-Blocking Write (NBW) protocol
 * for incoming periodic messages. If the TISS updates the given port during
 * execution of this function, or the function starts a fetch of the message
 * after the update has begun, the further behaviour is defined by the
 * parameter blocking.
 * 
 * If blocking is activated, which means a value greater than 0, the function
 * loops until a receive operation of an incoming periodic message has been
 * conducted successfully. In case of deactivated blocking mode, which is
 * expressed by a value of 0, the function exists with an error code, if the
 * TISS updates during the receive operation. In this scenario, the error
 * code would be TTSOC_ERRCODE_BUSY.
 *
 * For the other message types and synchronization modes, "blocking" has the
 * meaning as it is implemented by ttsoc_getMsgPtr().
 *  
 * On success the function returns TTSOC_ERRCODE_SUCCESS.
 * Other error codes are defined by the respective low-level functions are
 * propageted back to the function caller.
 * 
 * @attention The current implementation does not consider possible data caches
 * in the CPU! A platform-specific "copy" function would be required to 
 * perform the real copy operation from data memory into Port Memory.
 */

ttsoc_errcode ttsoc_receiveMsg(ttsoc_tiss_base ni, ttsoc_portid id, void *msg, char blocking)
{
    ttsoc_errcode err;      /* buffer for error codes returned by functions */
    void *portbuffer;       /* pointer to the message buffer in Port Memory */
    size_t len;             /* number of words to be copied */
    ttsoc_pi_word *dst;     /* casted pointer for destination */
    ttsoc_pi_word *src;     /* casted pointer for source */
    ttsoc_port *port;       /* pointer to the ttsoc_port object of that port */
    /* working variables for each copy operation */
    ttsoc_pi_word *dst_it;
    ttsoc_pi_word *src_it;  
    size_t len_it;
#ifdef XM_SRC_PARTITION
    int rtn;
#endif
    
    /* buffer for synchronization flags to recognize updates in progress */
    /* (incoming periodic messages only) */
    ttsoc_register oldflag; /* old value of synchronization flags */
    ttsoc_register newflag; /* new value of synchronization flags */

    /* determine the beginning of the data chunk, where the send message is to
     * reside in Port Memory */
    err = ttsoc_getMsgPtr(ni, id, &portbuffer, blocking);
   
    /* in case of an error, abort the function an propagete the error code */
    if( err != TTSOC_ERRCODE_SUCCESS )
        return err;

    /* find the ttsoc_port object for direct access to port configuration */
    port = ttsoc_getPort(ni, id);
    /* NOTE: Here there need not be a check of the return value of
     * ttsoc_getPort(). If the id had not existed, ttsoc_getMsgPtr() would
     * have indicated the error any way.
     */

    /* convert source and destination address to more specific pointers */
    src = (ttsoc_pi_word *) portbuffer;
    dst = (ttsoc_pi_word *) msg;
    /* prepare lenght of copy operation, consider time stamps */
    len = port->config.pcfg.TS ? TTSOC_TIMESTAMP_SIZE : 0;
    /* add the number of words to be copied */
    len += (size_t) port->msgsize;
    
    /* if the port to receive from contains incoming periodic messages */
    if( port->dir == incoming && port->config.pcfg.MT == 0 )
    {
        /* when using blocking mode */
        if( blocking )
        {
            /* do-while: execute the copy operation at least once */
            do
            {
                /* fetch current value from the Port Synchronization Memory */
                /* before copy operation */
                oldflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);
                /* default copy operation (see "attention" in documentation) */
                dst_it = dst;
                src_it = src;
                len_it = len;
#ifdef XM_SRC_PARTITION
            while(len_it--){
               rtn=XM_sparc_inport((unsigned int)src_it,(unsigned int *)dst_it);
               if (rtn<0){
                  printf("[P%d-receiveMsg] Error reading from 0x%x\n",XM_PARTITION_SELF,src_it);
                  break;
               }
               src++;dst++;
            }
#else
#ifdef _XM_KERNEL_
                while(len_it--){                    
                    *dst_it++ = LoadIoReg((xmAddress_t)src_it);
                    src_it++;
                }
#else
                while(len_it--)
                    *dst_it++ = *src_it++;
#endif
#endif
                /* fetch current value from the Port Synchronization Memory */
                /* after copy operation */
                newflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);
            }
            /* if fetch has started during an update operation (NBW is odd),
             * OR
             * if the old and new synchronization flags are not equal, an
             * update has occured during the copy operation
             */
            while((oldflag.psync.tiss_addr & 1) ||
				  (oldflag.psync.tiss_addr != newflag.psync.tiss_addr));
        }
        /* no blocking mode used */
        else
        {
            /* fetch the current value from the Port Synchronization Memory */
            /* before copy operation */
            oldflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);
            
            /* if an update is currently in progress (NBW is odd) */
            if( oldflag.psync.tiss_addr & 1 )
                return TTSOC_ERRCODE_BUSY;
	    
            /* default copy operation (see "attention" in documentation) */
#ifdef XM_SRC_PARTITION
            while(len--){
               rtn=XM_sparc_inport((unsigned int)src,(unsigned int *)dst);
               if (rtn<0){
                  printf("[P%d-receiveMsg] Error reading from 0x%x\n",XM_PARTITION_SELF,src);
                  break;
               }
               src++;dst++;
            }
#else
#ifdef _XM_KERNEL_
            while(len--){                    
                *dst++ = LoadIoReg((xmAddress_t)src);
                src++;
            }
#else
            while(len--)
                *dst++ = *src++;
#endif
#endif
            /* fetch the current value from the Port Synchronization Memory */
            /* after copy operation */
            newflag.value = ttsoc_ci_read(ni, TTSOC_NS_PSYNC, id);

            /* if the old and new synchronization flags (value of NBW sequencer) 
             * are not equal, an update has occured during the copy operation
             */
            if( oldflag.psync.tiss_addr != newflag.psync.tiss_addr )
                return TTSOC_ERRCODE_BUSY;
        }
    }
    /* the port to receive from contains other than incoming periodic
     * messages; do not concern about NBW protocol
     */
    else
    {
        /* default copy operation (see "attention" in function documentation) */
#ifdef XM_SRC_PARTITION
            while(len--){
               rtn=XM_sparc_inport((unsigned int)src,(unsigned int *)dst);
               if (rtn<0){
                  printf("[P%d-receiveMsg] Error reading from 0x%x\n",XM_PARTITION_SELF,src);
                  break;
               }
               src++;dst++;
            }
#else
#ifdef _XM_KERNEL_
            while(len--){                    
                *dst++ = LoadIoReg((xmAddress_t)src);
                src++;
            }
#else
            while(len--)
                *dst++ = *src++;
#endif
#endif
    }
    
    /* iterate the synchronization flags */
    err = ttsoc_iterMsgPtr(ni, id);

    /* in case of an error, abort the function an propagete the error code */
    if( err != TTSOC_ERRCODE_SUCCESS )
        return err;

    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}

/**
 * @return an error code 
 * @param id the identifier of the port to operate on
 * @param msg a pointer to the message buffer of the message to be sent
 * @param blocking wait until a new message has become ready
 * @brief copy a message into Port Memory and initiate port synchronization
 * 
 * This function is intended to be used by user-specific application code.
 * Literally, it "sends" a message. That is, it copies a message from the
 * message buffer in the CPUs scope into Port Memory. For this purpose, it
 * takes usage of the low-level port synchronization functions ttsoc_getMsgPtr()
 * and ttsoc_iterMsgPtr(). So, this functions wraps the low-level tasks of
 * copy operation and port synchronization.
 * 
 * For compatibility issues (with ttsoc_getMsgPtr()) the parameter "blocking"
 * is included. However it does not directly take any effect within that
 * function, but in ttsoc_getMsgptr().
 * 
 * On success the function returns TTSOC_ERRCODE_SUCCESS.
 * Other error codes are defined by the respective low-level functions are
 * propageted back to the function caller.
 * 
 * @attention The current implementation does not consider possible data caches
 * in the CPU! A platform-specific "copy" function would be required to 
 * perform the real copy operation from data memory into Port Memory.
 */

ttsoc_errcode ttsoc_sendMsg(ttsoc_tiss_base ni, ttsoc_portid id, void *msg, char blocking)
{
    ttsoc_errcode err;      /* buffer for error codes returned by functions */
    void *portbuffer;       /* pointer to the message buffer in Port Memory */
    size_t len;             /* number of words to be copied */
    ttsoc_pi_word *dst;     /* casted pointer for destination */
    ttsoc_pi_word *src;     /* casted pointer for source */
    ttsoc_port *port;       /* pointer to the ttsoc_port object of that port */
#ifdef XM_SRC_PARTITION
    int rtn;
#endif
    
    /* determine the beginning of the data chunk, where the send message is to
     * reside in Port Memory */
    err = ttsoc_getMsgPtr(ni, id, &portbuffer, blocking);
    
    /* in case of an error, abort the function an propagete the error code */
    if( err != TTSOC_ERRCODE_SUCCESS )
        return err;

    /* find the ttsoc_port object for direct access to port configuration */
    port = ttsoc_getPort(ni, id);
    /* NOTE: Here there need not be a check of the return value of
     * ttsoc_getPort(). If the id had not existed, ttsoc_getMsgPtr() would
     * have indicated the error any way.
     */
    
    /* convert source and destination address to more specific pointers */
    src = (ttsoc_pi_word *) msg;
    dst = (ttsoc_pi_word *) portbuffer;
    

    
    /* prepare lenght of copy operation, consider time stamps */
    len = port->config.pcfg.TS ? TTSOC_TIMESTAMP_SIZE : 0;
    /* add the number of words to be copied */
    len += (size_t) port->msgsize;
    
    /* default copy operation (see "attention" in function documentation) */
#ifdef XM_SRC_PARTITION
    while(len--){
       rtn=XM_sparc_outport((unsigned int)dst,(unsigned int)*src);
       if (rtn<0){
          printf("[P%d-ttsoc_sendMsg] Error writting on 0x%x\n",XM_PARTITION_SELF,dst);
          break;
       }
       src++;dst++;
    }
#else
#ifdef _XM_KERNEL_
    while(len--){
        StoreIoReg((xmAddress_t)dst,(xm_u32_t)*src);
        src++;dst++;
    }
#else
    while(len--)
        *dst++ = *src++;
#endif
#endif
    /* iterate the synchronization flags */
    err = ttsoc_iterMsgPtr(ni, id);

    /* in case of an error, abort the function an propagete the error code */
    if( err != TTSOC_ERRCODE_SUCCESS )
        return err;

    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}
