/*
 * $FILE: start.S
 *
 * The execution of XM starts from this file (start symbol)
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
#include <xmef.h>
#include <hypercalls.h>
#include <arch/asm_offsets.h>
#include <arch/irqs.h>
#include <arch/paging.h>
#include <arch/processor.h>
#include <arch/segments.h>
#include <arch/xm_def.h>

.section .boot.text, "ax"

ASM_ALIGN

ENTRY(start)
ENTRY(_start)
        xorl %eax, %eax
        movl $_VIRT2PHYS(_sbss), %edi
        movl $_VIRT2PHYS(_ebss), %ecx
        subl %edi, %ecx
        rep
        stosb

        movl $_VIRT2PHYS(_sdata), %esi /* Backup data for cold reset */
        movl $_VIRT2PHYS(_edata), %ecx
        movl $_VIRT2PHYS(_scdata), %edi
        movl $_VIRT2PHYS(_cpdata), %eax
        subl %esi, %ecx
        mov (%eax), %ebx
        testl %ebx, %ebx
        jz 1f

        movl $_VIRT2PHYS(_scdata), %esi
        movl $_VIRT2PHYS(_sdata), %edi

1:      rep movsb
        movl $1, (%eax)

        lgdt _gdtDesc
        
        ljmp $(EARLY_CS_SEL), $1f
1:      mov $(EARLY_DS_SEL), %eax
        mov %eax, %ds
        mov %eax, %es
        mov %eax, %ss
        xorl %eax, %eax
        mov %eax, %fs
        mov %eax, %gs
       
        mov $_VIRT2PHYS(__idle_kthread)+CONFIG_KSTACK_SIZE, %esp
        call BootDetectCpuFeat

        mov %eax, _VIRT2PHYS(cpuFeatures)
        
        andl $_DETECTED_I586|_PSE_SUPPORT|_PGE_SUPPORT, %eax
        xorl $_DETECTED_I586|_PSE_SUPPORT|_PGE_SUPPORT, %eax

        jnz __BootHaltSystem

        movl $_VIRT2PHYS(_pgTables), %eax
        call BootInitPgTab

#ifdef CONFIG_SMP
        movb $0x0, _VIRT2PHYS(aspReady)
smpStart32:
#endif

        movl $(_CR4_PSE|_CR4_PGE),%eax
        mov %eax, %cr4

        movl $_VIRT2PHYS(_pgTables), %eax
        movl %eax, %cr3
        movl %cr0,%eax
        orl $0x80000000,%eax
        movl %eax,%cr0
        jmp StartXM
        
__BootHaltSystem:
        cli
        hlt
        jmp __BootHaltSystem

PAGE_ALIGN
ENTRY(_Reset2)
        mov    0x4(%esp),%edx
        cli
        mov    %cr0,%eax
        and    $0x7fffffff,%eax
        mov    %eax,%cr0
        xor    %eax,%eax
        mov    %eax,%cr3
        jmp    *%edx

#ifdef CONFIG_SMP
.code16
	
ENTRY(smpStart16)
rBase=.
        wbinvd          # Needed for NUMA-Q should be harmless for others
        mov %cs, %ax    # Code and data in the same place
        mov %ax, %ds

        cli             # We should be safe anyway

        lidtl smpIdtDescr-rBase        # load idt with 0, 0
        lgdtl smpGdtDescr-rBase        # load gdt with whatever is appropriate
        xor %ax, %ax
        inc %ax                        # protected mode (PE) bit
        lmsw %ax                       # into protected mode
        ljmpl $(EARLY_CS_SEL), $1f

.code32
1:
        mov $(EARLY_DS_SEL), %eax
        mov %eax, %ds
        mov %eax, %es
        xorl %eax, %eax
        mov %eax, %fs
        mov %eax, %gs

        /* Setup an initial stack pointer */
        mov $_smpStack+64, %esp

        ljmp $(EARLY_CS_SEL), $1f
1:
        jmp smpStart32

smpGdtDescr:
        .word EARLY_XM_GDT_ENTRIES*8-1
        .long _VIRT2PHYS(earlyGdtTab)

smpIdtDescr:
        .word 0                                # idt limit = 0
        .long 0                                # idt base = 0L
ENTRY(smpStart16End)
#endif        /*CONFIG_SMP*/
        
.section .boot.data, "ax"
        .word 0
_resetAddr:
        .long 0
_gdtDesc:
        .word EARLY_XM_GDT_ENTRIES*8-1
        .long _VIRT2PHYS(earlyGdtTab)
_smpStack:
        .zero 64

.section .kbuild_info, "a"
.ascii "BUILD_TIME="TO_STR(BUILD_TIME)"\n"
.ascii "BUILD_IDR="TO_STR(BUILD_IDR)"\n"
.previous
