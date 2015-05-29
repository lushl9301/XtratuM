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
* @file irq.c
* @author Christian Paukovits
* @date July 31st, 2009
* @version August 31st, 2009
* @brief interrupt handling
*
* This file contains the plattform-independet code for interrupt handling.
* Basically, it contains the IRQ callback function ttsoc_master_irq(), which is
* the internal dispatcher function to further process the Master Interrupt that
* is issued at the TISS's Control Interface. Additionally, local function
* implement the given sources of interrupts according to the Control Interface
* semantics.
******************************************************************************/

#include <stdio.h>

/* plattform-specific includes (visible due to include path in build flow) */
//#include "avalon_tiss.h"
#include "ttsoc_types.h"
#include "lowlevel_io.h"

/* includes of generic source code parts of the driver (same directory) */
#include "irq.h"
#include "mapping.h"
#include "port.h"

/**
 * This (module) global variable stores the current state of the status
 * register of interrupt handling (in the TISS Register File). The variable
 * is written when the master interrupt (ttsoc_master_irq()) function reads
 * that status register. Afterwards the local specialized interrupt handling
 * functions use it to obtain the information about active interrupt sources
 */ 

volatile ttsoc_ci_word irqstatus;

/**
 * @param context pointer to the driver's internal state structure
 * @brief default interrupt handler for message complete interrupts
 * 
 * This functions iterates through all interrupt flag registers in order
 * to find the ports that have completed a message transfer, which in
 * turn triggered this interrupt source.
 * 
 * If a flag in the respective interrupt flag register is set, the associated
 * port is determined and its (user-specific) message complete interrupt
 * function is invoked.
 * 
 * Finally, the flags are reset by writing 1 to each passed interrupt flag
 * register. This occurs within an atomic write operation for the whole
 * interrupt flag register.
 */

//void ttsoc_msgcmpl_irq(void *context)
//{
//    ttsoc_ci_word finx; /* buffer for the index bits in the interrupt status register */
//    ttsoc_ci_word flags;/* buffer for the current interrupt flag register */
//    int it_finx;        /* iterator over finx */
//    int it_flags;       /* iterator over flags */
//    ttsoc_portid pid;   /* reconstructed port identifier */
//    ttsoc_port *port;   /* pointer to the current ttsoc_port object */
//
//    finx = irqstatus >> TTSOC_IRQ_SOURCES;
//
//    /* iterate over index bits in the interrupt status register */
//    for(it_finx = 0; it_finx < TTSOC_IRQ_FLAG_REGS; ++it_finx)
//    {
//        /* if the current index bit is set */
//        if( (finx >> it_finx) & 1 )
//        {
//            flags = ttsoc_ci_read(TTSOC_NS_REGFILE, TTSOC_REG_IRQ_FLAGS + it_finx);
//            
//            /* iterate over the current interrupt flag register */
//            for(it_flags = 0; it_flags < TTSOC_CI_WORD_WIDTH; ++it_flags)
//            {
//                /* if the current flag in that interrupt flag register is set */
//                if( (flags >> it_flags) & 1 )
//                {                   
//                    /* calculate the current port identifier */
//                    pid = (ttsoc_portid) (TTSOC_CI_WORD_WIDTH * it_finx + it_flags);
//                    /* reconstruct pointer to the proper ttsoc_port object */
//                    port = ttsoc_getPort(pid);
//
//                    /* if a ttsoc_port object really exists */
//                    if(port)
//                    {
//                        /* record port identifier and pointer to ttsoc_port
//                         * object in the driver's internal state structure */
//                        ((t_avalon_tiss_state *)context)->nActPort = pid;
//                        ((t_avalon_tiss_state *)context)->pActPort = ttsoc_getPort(pid);
//
//                        /* invoke the "message complete" callback function ...*/
//                        if(port->msgcmpl)
//                            /* ... and pass the context/internal state of the driver */
//                            port->msgcmpl(context); 
//                    }
//                }
//            }
//            ttsoc_ci_write(TTSOC_NS_REGFILE, TTSOC_REG_IRQ_FLAGS + it_finx, flags);
//        }
//    }
//
//    ttsoc_ci_write(TTSOC_NS_REGFILE, TTSOC_REG_IRQ_STATUS, TTSOC_IRQ_MSGCMPL_MASK);
//
//    /* invalidate the information in the driver's internal state structure */
//    ((t_avalon_tiss_state *)context)->nActPort = (ttsoc_portid) -1;
//    ((t_avalon_tiss_state *)context)->pActPort = 0;
//}

/**
 * This function pointer is the machanism of internal dispatching by the
 * master interrupt IRQ callback function. The associated interrupt sources
 * are:
 * 
 * -# error interrupt
 * -# message complete interrupt
 * -# reconfiguration instant
 * -# interrupt of the generic timer service
 *
 * The mapping of interrupt sources reflects the mapping of interrupt sources
 * in the interrupt status register in hardware (in the TISS Register File).
 */

//void (*irqhandles[TTSOC_IRQ_SOURCES])(void *context) =
//{
//    0,
//    ttsoc_msgcmpl_irq,
//    0,
//    0,
//};

/**
 * @param context pointer to the driver's internal state structure
 * @param id required by the HAL (unused)
 * @brief IRQ callback function of the TISS's Master Interrupt
 * 
 * This callback function realizes the main interrupt source that is associated
 * with the Master Interrupt generated by the TISS's Control Interface. 
 * First, it reads the whole interrupt status register of the TISS's
 * Register File into a module global variable irqstatus.
 *
 * Depending on the set and unset flags of interrupt sources (now contained in
 * irqstatus), the callback function further dispatches the execution to local
 * specialized handler functions (one for each interrupt source).
 *
 * After processing of the relevant interrupt sources, this functions resets
 * the flags associated with interrupt sources in order to release the
 * interrupt event.
 * 
 * @attention That internal dispatching is necessary, because the TISS only
 * produces one interrupt, i.e. the Master Interrupt, at the Control Interface.
 * Therefore, we have only one interrupt wire connected from the TISS (actually
 * its Avalon Wrapper) to the current CPU.
 * If we had multiple interrupt wires, we could have directly integrated the
 * local specialized interrupt handling functions into the dispatching
 * mechanism of the run-time environment.
 * 
 * @note The dispatching to the local specialized interrupt handling functions
 * is conditional with respect to the active flags in the interrupt status
 * register of the TISS's Register File. As we expect an unequally distributed
 * frequency for different interrupt sources, e.g. an "error" might happen
 * rarely, while "message complete" are the usual case, the execution time
 * of the whole interrupt handling mechanism is reduced significantly. That
 * conditional execution avoids executing unnecessary programme code.
 * 
 * @attention It might happen that this IRQ callback function is invoked twice
 * consecutively. This happens when message complete interrupts are triggered.
 * Namely, during the first access of irqstatus and the invokation of
 * ttsoc_msgcmpl_irq() another message complete interrupt could be triggered.
 * As the second run also accesses irqstatus then, the state of that variable
 * has become inconsistent between the run-time of ttsoc_master_irq() and any
 * ttsoc_msgcmpl_irq().
 * As a consequence, ttsoc_master_irq() only resets those flags that have been
 * set when it has processed its version of irqstatus. The additional flags
 * (caused by the second run) are not reset, consequently triggering another
 * invokation of ttsoc_master_irq(), which does nothing else except iterating
 * the interrupt status register and resetting it appropriately.
 */ 

//void ttsoc_master_irq(void *context, int id)
//{
//    int inx;    /* iteration variable over the interrupt status register */
//    
//    /* fetch the status register of interrupt handling */
//    irqstatus = ttsoc_ci_read(TTSOC_NS_REGFILE, TTSOC_REG_IRQ_STATUS);
//
//    /* iterate over all interrupt sources and determine the currently active */
//    for(inx = 0; inx < TTSOC_IRQ_SOURCES; ++inx)
//    {
//        /* the current interrupt source is active */
//        if( (irqstatus >> inx) & 1 )
//            /* if the function pointer is set */
//            if( irqhandles[inx] )
//                /* invoke the local specialized interrupt handler */
//                irqhandles[inx](context);
//    }
//}

/**
 * @return value of the mask register of interrupt handling
 * @brief reads the mask register of interrupt handling
 * 
 * This function encapsulates the read access towards the
 * interrupt mask register within the TISS's Register File.
 */

inline ttsoc_ci_word ttsoc_getInterruptMask(ttsoc_tiss_base ni)
{
	return ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_IRQ_MASK);
}

/**
 * @param value new value of the mask register of interrupt handling
 * @brief writes the mask register of interrupt handling
 * 
 * This function encapsulates the write access towards the
 * interrupt mask register within the TISS's Register File.
 */

inline void ttsoc_setInterruptMask(ttsoc_tiss_base ni, ttsoc_ci_word value)
{
	ttsoc_ci_write(ni,TTSOC_NS_REGFILE, TTSOC_REG_IRQ_MASK, value);
}


/**
 * @param irq number of interrupt to be set
 * @param func function pointer to the callback funtion to be installed
 * @brief sets an IRQ callback function for a given interrupt source
 *
 * This function overwrites the start-up values of the function pointer
 * array "irqhandles". The parameter "irq" states the element int the array,
 * which complies to the number of an interrupt source. The parameter "func"
 * holds the address of the new IRQ callback function that is to be associated
 * with that interrupt source.
 */

//void ttsoc_setIRQCallback(unsigned int irq, void (*func)(void *context))
//{
//	if(irq < TTSOC_IRQ_SOURCES)
//		irqhandles[irq] = func;
//}

