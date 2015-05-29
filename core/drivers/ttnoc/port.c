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
* @file port.c
* @author Christian Paukovits
* @date August 5th, 2009
* @version August 6th, 2009
* @brief implementation of auxiliary functions for ports
*
* This file implements the auxiliary functions for the management of "port"
* objects, which embody the architectural ports. Basically, it abstracts
* from the implementation respectively organization of the collection of
* ttsoc_port object, which can be arranged in an array or linked list.
* Additionally, it implements the "get"/"set" functions of retrieving and
* storing port configuration information.
******************************************************************************/

#ifdef _XM_KERNEL_
#include <drivers/ttnocports.h>
#else

/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"
#include "lowlevel_io.h"

/* includes of generic source code parts of the driver (same directory) */
#include "error_codes.h"
#include "port.h"
#include "mapping.h"
#endif

/**
 * This array of ttsoc_port objects embraces all supported architectural ports.
 * All access is encapsulated by "get" functions.
 */

static ttsoc_port sysports[TISS_NUMBER][TTSOC_NR_PORTS];

/**
 * @return the logic value (true or false) as an char
 * @param id the (port) identifier to be checked
 * @brief check the validity of a port identifier
 *
 * The local auxiliary function conducts a check, whether a given port
 * identifier is valid or not.
 * 
 * For performance reasons this function should be inline.
 *
 * @note This version of the driver, which uses an array of ttsoc_port objects,
 * ensures that the "id" is less than the number of supported architectural
 * ports; this is the upper bound. As the port identifier is assumed to be
 * unsigned, the lower bound check (greater equal 0) is not necessary.
 */

inline char checkPortID(ttsoc_portid id)
{
    return (id < TTSOC_NR_PORTS ? 1 : 0);
}

/**
 * @return the address of the ttsoc_port object of the associated port
 * @param id the identifier of the port to be found
 * @brief find the ttsoc_port object of the port identified by "id"
 *
 * This function's main purpose is the encapsulation of the access to the
 * array of architectural ports sysports. It checks the validity of the "id"
 * parameter, and then makes a simple look-up (array access) in that array.
 * In case of an invalid id that exceeds the array boundaries, a 0-pointer
 * is returned.
 * 
 * For performance reasons this function should be inlined.
 */

inline ttsoc_port *ttsoc_getPort(ttsoc_tiss_base ni, ttsoc_portid id)
{
    return ( checkPortID(id) ? &sysports[ni][id] : 0);
}

/**
 * @return the index of the ttsoc_port object in the array sysports
 * @param ptr pointer to a given ttsoc_port object
 * @brief determines the port identifier from a pointer
 *
 * This function is the inverse function to ttsoc_getPort(). It finds the
 * corresponding index in the array sysports, where the ttsoc_port objects
 * are kept.
 * 
 * The function uses C-style pointer arithmetics. There is no explicit error
 * treatment applicable.
 *
 * For performance reasons this function should be inlined.
 */

inline ttsoc_portid ttsoc_getPortID(ttsoc_tiss_base ni, ttsoc_port *ptr)
{
    return (ttsoc_portid) (ptr - sysports[ni]);
}

/**
 * @return an error code
 * @param id identifier of the port to retrieve information from
 * @brief reads the Port Configuration Memory and writes back into the
 * proper object within sysports
 * 
 * This functions retrieves configuration information considering ports from
 * the Port Configuration Memory (within the TISS). These settings are then
 * assigned to the ttsoc_port object within sysports, which is addressed by
 * means of the parameter "id".
 * 
 * On success the function returns TTSOC_ERRCODE_SUCCESS. The possible error
 * that can occure are:
 * - TTSOC_ERRCODE_INVALID_PORTID ... the given port identifier is invalid
 * 
 * @note There are some entities in the ttsoc_port structure, which cannot be
 * found in the Port Configuration Memory. They are stored in the Burst
 * Configuration Memory, consequently there is no direct access from the
 * host-side. 
 * As the driver requires information from the "invisible" memories, they have
 * to be stored in the ttsoc_port structure. It is in the scope of the user
 * to supply the same values for these entities as they reside in the TISS's
 * internal memories.
 * The entities under consideration are
 * - msgsize ... the size of a message measured in multiples of ttsoc_pi_word
 * - dir ... direction of the message, whether incoming or outgoing
 */

ttsoc_errcode ttsoc_getPortCfg(ttsoc_tiss_base ni, ttsoc_portid id)
{
    ttsoc_port *port;   /* pointer to the current ttsoc_port object */
    
    /* find the ttsoc_port object in sysports that is given by id */
    port = ttsoc_getPort(ni, id);
    
    /* if there exists no port with the given identifier */
    if( !port )
        /* abort the function with an error code */
        return TTSOC_ERRCODE_INVALID_PORTID;

    /* fetch the configuration information from the Port Configuration Memory */
    /* use basic read operation into the corresponding name space */
    port->config.value = ttsoc_ci_read(ni, TTSOC_NS_PCFG, id);
    
    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}

/**
 * @return an error code
 * @param id identifier of the port to commit configuration information
 * @brief writes the settings from the ttsoc_port object into Port
 * Configuration Memory
 * 
 * The function grabs the configuration data, which are locally stored in the
 * "config" entity of the ttsoc_port structure, and writes the data to the
 * Port Configuration Memory.
 * 
 * On success the function returns TTSOC_ERRCODE_SUCCESS. The possible error
 * that can occure are:
 * - TTSOC_ERRCODE_INVALID_PORTID ... the given port identifier is invalid
 *
 * @note There are some entities in the ttsoc_port structure, which cannot be
 * found in the Port Configuration Memory. They are stored in the Burst
 * Configuration Memory, consequently there is no direct access from the
 * host-side. 
 * As the driver requires information from the "invisible" memories, they have
 * to be stored in the ttsoc_port structure. It is in the scope of the user
 * to supply the same values for these entities as they reside in the TISS's
 * internal memories.
 * The entities under consideration are
 * - msgsize ... the size of a message measured in multiples of ttsoc_pi_word
 * - dir ... direction of the message, whether incoming or outgoing
 */

ttsoc_errcode ttsoc_setPortCfg(ttsoc_tiss_base ni, ttsoc_portid id)
{
    ttsoc_port *port;   /* pointer to the current ttsoc_port object */
    
    /* find the ttsoc_port object in sysports that is given by id */
    port = ttsoc_getPort(ni, id);
    
    /* if there exists no port with the given identifier */
    if( !port )
        /* abort the function with an error code */
        return TTSOC_ERRCODE_INVALID_PORTID;

    /* store the configuration information into the Port Configuration Memory */
    /* use basic write operation into the corresponding name space */
    ttsoc_ci_write(ni, TTSOC_NS_PCFG, id, port->config.value);
    
    /* exit function successfully */
    return TTSOC_ERRCODE_SUCCESS;
}
