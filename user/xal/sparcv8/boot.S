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

/* <track id="PARTITION_BOOT_CODE"> */
#include <xm.h>
#include <xm_inc/arch/asm_offsets.h>
	
#define MIN_STACK_FRAME 0x60
#define MAIN_VCPU 0x0
	
.text
.align 8

.global start, _start

_start:	
start:
        set get_vcpuid_nr, %o0 // vCpuId
	__XM_HC
        mov %o0, %g2

        cmp %o0, MAIN_VCPU
        bne 2f
        nop

	/* Zero out our BSS section. */
	set _sbss, %o0
	set _ebss, %o1

1:
	st %g0, [%o0]
	subcc %o0, %o1, %g0
	bl 1b
	add %o0, 0x4, %o0

2:	
	set __stack, %fp

        inc %g2
        
        sll %g2, 0xd, %g2
        add %g2, %fp, %fp
        
        mov %g1, %o0
        call init_libxm
        sub %fp, MIN_STACK_FRAME, %sp

	! Set up TBR
	set sparc_write_tbr_nr, %o0
	set _traptab, %o1
	__XM_HC

        call SetupIrqs
        sub %fp, MIN_STACK_FRAME, %sp
        
	call PartitionMain
	sub %fp, MIN_STACK_FRAME, %sp

        set libXmParams, %o0
        ld    [%o0], %o1
        ld    [%o1 + _IDPART_OFFSET], %o1
        mov halt_partition_nr, %o0
        __XM_HC

1:	b 1b
	nop

CommonTrapHandler:
        sub %fp, 48+MIN_STACK_FRAME, %sp
	std %l0, [%sp+40+MIN_STACK_FRAME]
	std %l2, [%sp+32+MIN_STACK_FRAME]
	std %g6, [%sp+24+MIN_STACK_FRAME]
	std %g4, [%sp+16+MIN_STACK_FRAME]
	std %g2, [%sp+8+MIN_STACK_FRAME]
	st %g1, [%sp+4+MIN_STACK_FRAME]
	rd %y, %g5
	st %g5, [%sp+MIN_STACK_FRAME]	

	set trapHandlersTab, %g1
	sll %l3, 2, %g2
	ld [%g1+%g2], %g1
	
	cmp %g1, 0
	be 1f
	nop

	mov %sp, %o0
	
	call %g1
	add %o0, MIN_STACK_FRAME, %o0

1:	
	ld [%sp+MIN_STACK_FRAME], %g1
        wr %g1, %y
        ld [%sp+4+MIN_STACK_FRAME], %g1
        ldd [%sp+8+MIN_STACK_FRAME], %g2
        ldd [%sp+16+MIN_STACK_FRAME], %g4
        ldd [%sp+24+MIN_STACK_FRAME], %g6
        ldd [%sp+32+MIN_STACK_FRAME], %l2
        ldd [%sp+40+MIN_STACK_FRAME], %l0
	
        restore
        save
	mov sparc_iret_nr, %o0
	__XM_AHC
        
.macro BUILD_TRAP trapNr
	b CommonTrapHandler
	mov \trapNr\(), %l3
	nop
	nop
.endm
	
.macro BUILD_TRAP_BLOCK stTrapNr endTrapNr
vector=\stTrapNr\()
.rept \endTrapNr\()-\stTrapNr\()+1
	/*\noTraps\() */
	BUILD_TRAP vector
vector=vector+1
.endr
.endm

.align 4096
.global _traptab
_traptab:
! + 0x00: reset
t_reset: b start
	nop
	nop
	nop

	BUILD_TRAP_BLOCK 0x1 0xff

.data
.align 8
__stack:
        .fill (STACK_SIZE/4)*CONFIG_MAX_NO_VCPUS,4,0

.previous

.global lPartitionControlTable
.align 4096
lPartitionControlTable:
        .zero 4096
