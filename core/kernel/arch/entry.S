/*
 * $FILE: entry.S
 *
 * Interrupts' code
 *
 * $VERSION$
 *
 * $AUTHOR$
 *
 * $LICENSE:
 * COPYRIGHT (c) Fent Innovative Software Solutions S.L.
 *     Read LICENSE.txt file for the license terms.
 */

#include <linkage.h>
#include <hypercalls.h>
#include <ktimer.h>
#include <arch/asm_offsets.h>
#include <arch/irqs.h>
#include <arch/processor.h>
#include <arch/segments.h>
#include <arch/xm_def.h>

#ifdef CONFIG_DEBUG
#define ROLLBACK_STACK movl $0, %ebp
#else
#define ROLLBACK_STACK
#endif

.macro GET_CPUCTXT _ctxt
        call __ArchGetLocalId
        lea (%eax, %eax, 2), %eax
        mov localSchedInfo+_CKTHREAD_OFFSET(,%eax, 8), %eax
        mov _IRQCPUCTXT_OFFSET+_CTRL_OFFSET(%eax), \_ctxt\()
 .endm

.macro SET_CPUCTXT _ctxt
        call __ArchGetLocalId
        lea (%eax, %eax, 2), %eax
        mov localSchedInfo+_CKTHREAD_OFFSET(,%eax, 8), %eax
        mov \_ctxt\(), _IRQCPUCTXT_OFFSET+_CTRL_OFFSET(%eax)
.endm
        
.macro SET_WP
        movl %cr0, %eax
        orl $(_CR0_WP), %eax
        movl %eax, %cr0
.endm
	
.macro CLEAR_WP
        movl %cr0, %eax
        andl $(~(_CR0_WP)), %eax
        movl %eax, %cr0
.endm

.macro HW_SAVE_REGS
        pushl %gs
        pushl %fs
        pushl %es
        pushl %ds
        pushl %eax
        pushl %ebp
        pushl %edi
        pushl %esi
        pushl %edx
        pushl %ecx
        pushl %ebx
.endm

.macro HW_RESTORE_REGS
        popl %ebx
        popl %ecx
        popl %edx
        popl %esi
        popl %edi
        popl %ebp
        popl %eax
1:      popl %ds
2:      popl %es
3:      popl %fs
4:      popl %gs
.section .gp_ex_tab, "a"
        ASM_ALIGN
        .long 1b
        .long 2b
        .long 3b
        .long 4b
.previous
.endm

.macro HW_SAVE_ALL
        cld
        HW_SAVE_REGS
        movl $(DS_SEL), %edx
        movl %edx, %ds
        movl %edx, %es
        xorl %edx, %edx
        movl %edx, %fs
        movl $(PERCPU_SEL), %edx
        movl %edx,%gs
.endm
	
.macro HW_RESTORE_ALL
        HW_RESTORE_REGS
        addl $8, %esp
.endm

.macro TABLE_START section, symbol
.section .rodata.\section\(),"a"
.globl \symbol\()
ASM_ALIGN
\symbol\() :
.endm

.macro TABLE_END section
.section .rodata.\section\(),"a"
ASM_ALIGN
.long 0
.previous
.endm
	
.macro BUILD_IRQ irq
.section .rodata.irqHndl,"a"
ASM_ALIGN
.long 1f
.text
ASM_ALIGN
1:
        pushl $0
        pushl $\irq\()
        jmp CommonIrqBody
.endm

.macro BUILD_TRAP_ERRCODE trap
.section .rodata.trapHndl,"a"
ASM_ALIGN
.long 1f
.text
ASM_ALIGN
1:
        pushl $\trap\()
        cli
        jmp CommonTrapBody
.endm
	
.macro BUILD_TRAP_NOERRCODE trap
.section .rodata.trapHndl,"a"
ASM_ALIGN
.long 1f
.text
ASM_ALIGN
1:
        pushl $0
        pushl $\trap\()
        cli
        jmp CommonTrapBody
.endm

// HW irq routines	
TABLE_START irqHndl, hwIrqHndlTab
vector=0
.rept CONFIG_NO_HWIRQS
        BUILD_IRQ vector
vector=vector+1
.endr
TABLE_END irqHndl
	
// Traps routines
TABLE_START trapHndl, trapHndlTab
BUILD_TRAP_NOERRCODE 0x0
BUILD_TRAP_NOERRCODE 0x1
BUILD_TRAP_NOERRCODE 0x2
BUILD_TRAP_NOERRCODE 0x3
BUILD_TRAP_NOERRCODE 0x4 
BUILD_TRAP_NOERRCODE 0x5
BUILD_TRAP_NOERRCODE 0x6 
BUILD_TRAP_NOERRCODE 0x7
BUILD_TRAP_ERRCODE 0x8
BUILD_TRAP_NOERRCODE 0x9
BUILD_TRAP_ERRCODE 0xa 
BUILD_TRAP_ERRCODE 0xb
BUILD_TRAP_ERRCODE 0xc 
BUILD_TRAP_ERRCODE 0xd
BUILD_TRAP_ERRCODE 0xe
BUILD_TRAP_NOERRCODE 0xf
BUILD_TRAP_NOERRCODE 0x10 
BUILD_TRAP_ERRCODE 0x11
BUILD_TRAP_NOERRCODE 0x12 
BUILD_TRAP_NOERRCODE 0x13
BUILD_TRAP_ERRCODE 0x14 
BUILD_TRAP_ERRCODE 0x15
BUILD_TRAP_ERRCODE 0x16	
BUILD_TRAP_ERRCODE 0x17
BUILD_TRAP_ERRCODE 0x18 
BUILD_TRAP_ERRCODE 0x19
BUILD_TRAP_ERRCODE 0x1a 
BUILD_TRAP_ERRCODE 0x1b
BUILD_TRAP_ERRCODE 0x1c 
BUILD_TRAP_ERRCODE 0x1d
BUILD_TRAP_ERRCODE 0x1e 
BUILD_TRAP_ERRCODE 0x1f
TABLE_END trapHndl

.macro CHECK_USER_CTXT label, offset
        movl \offset\()(%esp), %eax
        andl $0x3, %eax
        testl %eax, %eax
        jz \label\()
.endm

.macro COMMON_BODY hndlr
.text
ASM_ALIGN
Common\hndlr\()Body:
        sub $4, %esp
        HW_SAVE_ALL
        ROLLBACK_STACK
        
        GET_CPUCTXT %eax
        movl %eax, _PREV_OFFSET(%esp)
        SET_CPUCTXT %esp
        
        CHECK_USER_CTXT 1f, _CS_OFFSET
        CLEAR_WP
1:      pushl %esp
        call Do\hndlr\()
        addl $4, %esp
        CHECK_USER_CTXT 1f, _CS_OFFSET
        pushl %esp
        call RaisePendIrqs
        addl $4, %esp
        SET_WP
1:
        movl _PREV_OFFSET(%esp), %ecx
        SET_CPUCTXT %ecx
        HW_RESTORE_ALL
        add $4, %esp
        iret
.endm
	
COMMON_BODY Irq
COMMON_BODY Trap

.text
ASM_ALIGN
ENTRY(AsmHypercallHndl)
        pushf
        cli
        sub $8, %esp
        HW_SAVE_ALL
        ROLLBACK_STACK
        CLEAR_WP

        movl _ECODE_OFFSET(%esp), %eax
        movl %eax, _FLAGS_OFFSET(%esp)

        GET_CPUCTXT %eax
        movl %eax, _PREV_OFFSET(%esp)
        SET_CPUCTXT %esp

        movl _AX_OFFSET(%esp), %eax
        cmpl $NR_HYPERCALLS, %eax
        jae 1f
        movl hypercallsTab(, %eax, 4), %eax
        testl %eax, %eax
        jz 1f
        call *%eax

        movl _AX_OFFSET(%esp), %ecx
        movl hypercallFlagsTab(, %ecx, 4), %ecx
        test %ecx,%ecx
        jns 2f

        movl %eax, _AX_OFFSET(%esp)
        jmp 2f
1:      pushl _AX_OFFSET(%esp)
        pushl $unknownHypercallStr
        call kprintf
        addl $0x8, %esp
        movl $-1, _AX_OFFSET(%esp)

2:
        pushl %esp
        call RaisePendIrqs
        addl $4, %esp

        movl _PREV_OFFSET(%esp), %ecx
        SET_CPUCTXT %ecx
        SET_WP
        HW_RESTORE_ALL
        add $4, %esp
        iret

ASM_ALIGN
ENTRY(AsmIRetHndl)
        pushf
        cli
        sub $8, %esp
        HW_SAVE_ALL
        ROLLBACK_STACK
        CLEAR_WP

        movl _ECODE_OFFSET(%esp), %eax
        movl %eax, _FLAGS_OFFSET(%esp)

        GET_CPUCTXT %eax
        movl %eax, _PREV_OFFSET(%esp)
        SET_CPUCTXT %esp
 
        call X86IRetSys

        pushl %esp
        call RaisePendIrqs
        addl $4, %esp

        movl _PREV_OFFSET(%esp), %ecx
        SET_CPUCTXT %ecx
        SET_WP
        HW_RESTORE_ALL
        add $4, %esp
        iret
        
ASM_ALIGN
ENTRY(UnexpectedIrq)
        pushl $0x0
        pushl $0xff
        HW_SAVE_ALL
        ROLLBACK_STACK
        CHECK_USER_CTXT 1f, _CS_OFFSET
        CLEAR_WP
1:      pushl $unexpecIrqStr
        call kprintf
        addl $4, %esp
        CHECK_USER_CTXT 1f, _CS_OFFSET
        SET_WP
1:      HW_RESTORE_ALL
        iret

.data
	
unexpecIrqStr:
    .asciz "Unexpected irq\n"
	
unknownHypercallStr:
    .asciz "Hypercall (%d) unknown\n"

unknownIpiStr:
    .asciz "IPI (%d) unkown\n"
