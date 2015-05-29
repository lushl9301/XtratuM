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
* @file avalon_tiss_driver.c
* @author Christian Paukovits
* @date August 3rd, 2009
* @version August 4th, 2009
* @brief plattform-specific wrapping code for low-level register access
*
* This file implements the inline functions and macros, which wrap the
* low-level access to the TISS's Register File via the Control Interface.
* The purpose is to abstract from the platform-specific operations and to
* create an uniform I/O layer within the driver.
******************************************************************************/


//#include <xio.h>
#ifdef XM_SRC_PARTITION
#include <xm.h>
#include <stdio.h>
#endif
#ifdef _XM_KERNEL_
#include <sparcv8/leon.h>
#include <drivers/ttnocports.h>
#else
#include "lowlevel_io.h"
#endif

/**
 * @param ns name space to read from
 * @param off address offset to be read within that name space
 * @return the raw (uninterpreted) data from an entity within the TISS
 * @brief read a data word via the Control Interface
 *
 * This is the platform-specific function that performs the basic read
 * operation via the Control Interface. The name space determines the memory
 * component within the TISS to be read. The offset specifies the relative
 * offset within the memory component.
 *
 * The plattform-specific code manifests in the basic I/O macros, which are
 * given by the platform, i.e. the HAL.
 *
 * In general, this function should be inlined.
 */

inline ttsoc_ci_word ttsoc_ci_read(ttsoc_tiss_base ni, ttsoc_ci_addr ns, ttsoc_ci_addr off)
{
  #ifdef XM_SRC_PARTITION
	xm_u32_t lreg = 0;
	xm_u32_t data;
	int rtn;
	
	switch(ni)
	{
		case 0: lreg = TISS_CI_0;
		break;
		case 1: lreg = TISS_CI_1;
		break;
	}
	lreg+=((ns<<4)|off)*4;
	rtn=XM_sparc_inport(lreg,&data);
	if (rtn<0)
	  printf("[P%d-ttsoc_ci_read] Error reading from 0x%x\n",XM_PARTITION_SELF,lreg);
	return (ttsoc_ci_word)data;
  #else
     #ifdef _XM_KERNEL_
        xm_u32_t lreg = 0;
        xm_u32_t data;
        int rtn;

        switch(ni)
        {
                case 0: lreg = TISS_CI_0;
                break;
                case 1: lreg = TISS_CI_1;
                break;
        }
        lreg+=((ns<<4)|off)*4;
        return LoadIoReg((xmAddress_t)lreg);
      #else
	int *lreg=0;
	
	switch(ni)
	{
		case 0: lreg = (int *) TISS_CI_0;
		break;
		case 1: lreg = (int *) TISS_CI_1;
		break;
	}
	return lreg[ (ns << 4) | off ];
      #endif
  #endif
}

/**
 *	@param ns name space to write to
 *	@param off address offset to be written within that name space
 *	@brief write a data word via the Control Interface
 *
 *	This is the platform-specific function that performs the basic write
 *	operation via the Control Interface. The name space determines the memory
 *	component within the TISS to be written. The offset specifies the relative
 *	offset within the memory component.
 *
 *	The plattform-specific code manifests in the basic I/O macros, which are
 *	given by the platform, i.e. the HAL.
 *
 *	In general, this function should be inlined.
 */

inline void ttsoc_ci_write(ttsoc_tiss_base ni, ttsoc_ci_addr ns, ttsoc_ci_addr off, ttsoc_ci_word data)
{
  #ifdef XM_SRC_PARTITION
	xm_u32_t lreg = 0;
	int rtn;
	
	switch(ni)
	{
		case 0: lreg = TISS_CI_0;
		break;
		case 1: lreg = TISS_CI_1;
		break;
	}
	lreg+=((ns<<4)|off)*4;
	rtn=XM_sparc_outport(lreg,(xm_u32_t)data);
	if (rtn<0)
	  printf("[P%d-ttsoc_ci_write] Error writting on 0x%x\n",XM_PARTITION_SELF,lreg);
  #else
    #ifdef _XM_KERNEL_
        xm_u32_t lreg = 0;
        int rtn;

        switch(ni)
        {
                case 0: lreg = TISS_CI_0;
                break;
                case 1: lreg = TISS_CI_1;
                break;
        }
        lreg+=((ns<<4)|off)*4;
        StoreIoReg((xmAddress_t)lreg,(xm_u32_t)data);
    #else
	int *lreg=0;
	switch(ni)
	{
		case 0: lreg = (int *) TISS_CI_0;
		break;
		case 1: lreg = (int *) TISS_CI_1;
		break;
	}
	lreg[ (ns << 4) | off ] = data;
     #endif
  #endif
}
