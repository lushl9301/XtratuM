/*
 * $FILE: partition.c
 *
 * Fent Innovative Software Solutions
 *
 * $LICENSE:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include <string.h>
#include <stdio.h>
#include <xm.h>
#include <irqs.h>

#ifdef CONFIG_SPARCv8
#define __DIV(n, d, r) __asm__ __volatile__ ("udiv %1, %2, %0\n\t" : "=r" (r) : "r" (n), "r" (d))
#endif

#define PRINT(...) do { \
        printf("[%d] ", XM_PARTITION_SELF); \
        printf(__VA_ARGS__); \
} while (0)

xm_s32_t control = 1;
xm_u32_t excRet;

void DivideExceptionHandler(trapCtxt_t *ctxt)                                       /* XAL trap API */
{
    PRINT("#Divide Exception propagated, ignoring...\n");
    PRINT("Halting\n");
    XM_halt_partition(XM_PARTITION_SELF);
}

void PartitionMain(void)
{
    volatile xm_s32_t val = 0;

    XM_idle_self();

#ifdef CONFIG_x86
    InstallTrapHandler(DIVIDE_ERROR, DivideExceptionHandler); /* Install timer handler */
#endif
#ifdef CONFIG_SPARCv8
    InstallTrapHandler(DIVISION_BY_ZERO, DivideExceptionHandler); /* Install timer handler */
#endif

    PRINT("Dividing by zero...\n");

#ifdef CONFIG_SPARCv8
    __DIV(10,val,val);
#else
    val = 10 / val;
#endif

    PRINT("Halting\n");
    XM_halt_partition(XM_PARTITION_SELF);
}
