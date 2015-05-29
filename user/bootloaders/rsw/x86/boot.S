/*
 * $FILE: boot.S
 *
 * Generic boot code, sets up a valid stack and jumps to the C code
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include "multiboot.h"

.text
.align 4

.global start, _start, _halt_system

_start:
start:
        jmp _multiboot_entry
.align 4

_multiboot_header:
/* magic */
.long __MULTIBOOT_HEADER_MAGIC
/* flags */
.long __MULTIBOOT_HEADER_FLAGS
/* checksum */
.long -(__MULTIBOOT_HEADER_MAGIC+__MULTIBOOT_HEADER_FLAGS)

_multiboot_entry:
        xorl %eax, %eax
        movl $_sbss, %edi
        movl $_ebss, %ecx
        subl %edi, %ecx
        rep
        stosb

        mov $_estack, %esp
        call RSwMain

_halt_system:
        cli
        hlt
        jmp _halt_system

.bss

_stack:
	.zero (CONFIG_STACK_SIZE*1024)
_estack:

.previous
