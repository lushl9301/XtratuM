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
* @file regfile.h
* @author Christian Paukovits
* @date August 5th, 2009
* @version August 5th, 2009
* @brief access functions for TISS's Register File
*
* Herein the global function, which perform the read and write access on the
* TISS's Register File, are implemented. The according global functions are
* declared in registerfile.h.
* 
* Because these functions are quite simple, they should be implemented inline
* for performance reasons.
******************************************************************************/

#ifdef _XM_KERNEL_
#include <drivers/ttnocports.h>
#else
/* plattform-specific includes (visible due to include path in build flow) */
#include "ttsoc_types.h"
#include "lowlevel_io.h"

/* includes of generic source code parts of the driver (same directory) */
#include "mapping.h"
#include "regfile.h"
#endif
/**
 * @return the current value of the global time (i.e. time stamp)
 * @brief read the value of the counter vector that embodies the global time
 *
 * This functions reads the lower and upper half of the time base's counter
 * vector. These entities are mapped into the Register File and can be found
 * at the offset given in mapping.h (c.f. registermap.vhd).
 *
 * It is a feature by the TISS to keep the value of the counter vector
 * consistent during access. Therefore, this function is reduced to load
 * the lower and upper half into a variable of the corresponding type
 * "ttsoc_timestamp". That variable is returned by copy-by-value.
 */

inline ttsoc_timestamp ttsoc_getGlobalTime(ttsoc_tiss_base ni)
{
    /* the local working variable */
    ttsoc_timestamp instant;
    
    /* read the lower half of the time base's counter vector */
    instant.Lower = ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIME_L);
    /* read the upper half of the time base's counter vector */
    /* apply shift operation by the half width of the data type*/
    /* "/ 2 * 8" => "<<2" */
    instant.Upper = ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIME_U);

    return instant;
}

/**
 * @param pPattern pointer to the object which stores the timer pattern
 * @param pMask pointer to the object which stores the timer mask
 * @brief deliver the current settings of the generic timer service
 * 
 * This functions reads the timer pattern and timer mask registers from the
 * Register File. As both entities are time stamps, there are 4 load operations
 * involved. The values are directly stored into objects in the user space by
 * means of pointer dereferentiation.
 */

inline void ttsoc_getTimerSettings(ttsoc_tiss_base ni, ttsoc_timestamp *pPattern, ttsoc_timestamp *pMask)
{
    /* read the lower half of the timer pattern */
    pPattern->Lower = ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERPAT_L);
    /* read the upper half of the timer pattern */
    /* same shift operation as in ttsoc_getGlobalTime() */
    pPattern->Upper = ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERPAT_U) ;

    /* read the lower half of the timer mask */
    pMask->Lower = ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERMSK_L);
    /* read the upper half of the timer pattern */
    /* same shift operation as in ttsoc_getGlobalTime() */
    pMask->Upper= ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERMSK_U);
}

/**
 * @param pattern new value of the timer pattern
 * @param mask new value of the timer mask
 * @brief updates the settings of the generic timer service
 *
 * This functions writes new values of timer pattern and timer mask registers
 * into the Register File. As both entities are time stamps, there are 4 store
 * operations involved. The new values are delivered copy-by-value in the
 * parameters of this function.
 */

inline void ttsoc_setTimerSettings(ttsoc_tiss_base ni, ttsoc_timestamp pattern, ttsoc_timestamp mask)
{
    /* write the lower half of the timer pattern */
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERPAT_L, pattern.Lower);
    /* write the upper half of the timer pattern */
    /* apply shilft operation to extract the upper half of the time stamp */
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERPAT_U, pattern.Upper);

    /* write the lower half of the timer mask */
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERMSK_L, mask.Lower);
    /* write the upper half of the timer mask */
    /* apply shilft operation to extract the upper half of the time stamp */
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_TIMERMSK_U, mask.Upper);
}

/**
 * @brief signal the life sign / heart beat to the watchdog
 * 
 * This functions issues the life sign or also called heart beat to the
 * watchdog that resides in the TISS. For that purpose, the function writes
 * the special pattern into the appropriate register of the Register File.
 * 
 * The life sign is defined in regfile.h as a preprocessor constant.
 */

inline void ttsoc_lifesign(ttsoc_tiss_base ni)
{
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_WDLIFESIGN, TTSOC_WATCHDOG_LIFESIGN);
}

/**
 * @return raw (uninterpreted) value of the Error & Status Register
 * @brief read the Error & Status Register
 * 
 * This functions encapsulates the read access to the Error & Status Register
 * of the TISS's Register File. The resulting value is not interpreted, i.e.
 * casted to the data type ttsoc_register, which contains a description of the
 * Error & Status Register. This "interpretation" (cast) has be executed
 * in the user scope.
 */

inline ttsoc_ci_word ttsoc_getErrorStatus(ttsoc_tiss_base ni)
{
    return ttsoc_ci_read(ni, TTSOC_NS_REGFILE, TTSOC_REG_ERRSTATUS);
}

/**
 * @param new raw (uninterpreted) value for the Error & Status Register
 * @brief sets modifyable parts of the Error Status Register
 * 
 * This function conducts a write access to the Error & Status Register of the
 * TISS's Register File. Even though the user might handle an interpreted,
 * i.e. casted to ttsoc_register, version, we use the raw value, which
 * regards the bitfields as plane data word. This conversion is easily
 * achieved, as long as ttsoc_register is implemented as union data type.
 * 
 * @note Most of the fields in the Error & Status Register are not writeable
 * from the host-side at all. Consequently, only the writeable fields are
 * taken, while the others are skipped within the TISS.  
 */
inline void ttsoc_setErrorStatus(ttsoc_tiss_base ni, ttsoc_ci_word word)
{
    ttsoc_ci_write(ni, TTSOC_NS_REGFILE, TTSOC_REG_ERRSTATUS, word);
}

