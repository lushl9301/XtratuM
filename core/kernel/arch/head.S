/*
 * $FILE: head.S
 *
 * Entry point of the XM in the x86 architecture version
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
#include <arch/irqs.h>
#include <arch/asm_offsets.h>
#include <arch/segments.h>
#include <arch/xm_def.h>

.section .vboot.text, "ax"
.code32

ENTRY(StartXM)
        lidt idtDesc

        lss _sstack, %esp

        call SetupCr
        call SetupGdt

#ifdef CONFIG_SMP
		movb aspReady, %al
        cmpb $0x0, %al
        je 1f

        cld
        pushl $0
        popf
        orb $0x80, aspReady
#ifdef CONFIG_DEBUG
        movl $0, %ebp
#endif /*CONFIG_DEBUG*/

        call InitSecondaryCpu

1:      movb $0x1, aspReady
#endif /*CONFIG_SMP*/
        push $__idle_kthread /* Idle kthread */
        pushl $0             /* CPU 0 ID     */
        call Setup

.text
ENTRY(__HaltSystem)
        cli
        hlt
        jmp __HaltSystem

.data
PAGE_ALIGN
        .word 0
ENTRY(idtDesc)
        .word IDT_ENTRIES*8-1
        .long _VIRT2PHYS(idtTab)
        
#ifdef CONFIG_SMP
.section .vboot.data, "ax"
ENTRY(aspReady)
        .byte 0
//ENTRY(_smpPgTab)
//        .long _pgTables
#endif /*CONFIG_SMP*/
ENTRY(_sstack)
		.long __idle_kthread+CONFIG_KSTACK_SIZE
        .word EARLY_DS_SEL
.data
ENTRY(earlyGdtTab)
        .quad 0x0000000000000000 /* NULL  descriptor */
        .quad 0x00cf9b000000ffff /* 1<<3 XM's code segment R0 */
        .quad 0x00cf93000000ffff /* 2<<3 XM's data segment R0 */


.bss
PAGE_ALIGN
ENTRY(_pgTables)
        .zero 2*PAGE_SIZE
        //.zero ((0x3ff-(CONFIG_XM_OFFSET>>PTDL1_SHIFT))+1)*PAGE_SIZE

ENTRY(xmPhys)
        .zero PAGE_SIZE

ENTRY(idtTab)
        .zero IDT_ENTRIES*8
        
ENTRY(__idle_kthread)
        .zero CONFIG_KSTACK_SIZE*CONFIG_NO_CPUS
