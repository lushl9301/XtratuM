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

#include <config.h>
#define MIN_STACK_FRAME 0x60
#define STACK_SIZE 8192

#ifdef CONFIG_SPARCv8
.global _start, start
_start: 
start:
        set partCtrlTabPtr, %g2
        st %g1, [%g2]

        set stack, %fp
        set STACK_SIZE-8, %g1
        add %fp, %g1, %fp
        mov %g1, %o0
        call MainLdr
        sub %fp, MIN_STACK_FRAME, %sp
        
        set partCtrlTabPtr, %g2
        ld [%g2], %g1
        jmp %o0
        nop
1:      b 1b
        nop

.common stack,STACK_SIZE,8
#endif

#ifdef CONFIG_x86
.global _start, start
_start:
start:
        movl %ebx, partCtrlTabPtr
        movl $eStack, %esp
        call MainLdr
        movl partCtrlTabPtr, %ebx
        jmp *%eax
1:
        jmp 1b
.bss
sStack:
        .zero STACK_SIZE
eStack:
#endif
