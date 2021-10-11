#ident "@(#)mmu_crueltt.s	1.1 8/6/90 SMI"

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc
 */

/*

	Adapted from @(#)crueltt.s	1.16	6/9/89
	trap table and window over/under flow trap handlers
	for use on Verilog description of custom sunrise.

	swk added some stuff to the reset handler to deal
	with enabling the mmu.

	6/22	swk	Changed T_reset to T_RESET so mpsas 
			parsing of sym.dat file will work. 	
	6/25	swk	Added switch to disable ITBR if wanted.

*/


!----------------------------------------------------------------------
! Link in trap handling routines by #defining the entry point before
! #including crueltt.


#ifndef IAE_HANDLER
#ifdef	iae_handler
#define IAE_HANDLER	iae_handler
#else
#define IAE_HANDLER	TRAP(unimplemented_trap)
#endif
#endif

#ifndef IAER_HANDLER
#ifdef	iaer_handler
#define IAER_HANDLER	iaer_handler
#else
#define IAER_HANDLER	TRAP(unimplemented_trap)
#endif
#endif

#ifndef DAE_HANDLER
#ifdef	dae_handler
#define DAE_HANDLER	dae_handler
#else
#define DAE_HANDLER	TRAP(unimplemented_trap)
#endif
#endif

#ifndef DAER_HANDLER
#ifdef	daer_handler
#define DAER_HANDLER	daer_handler
#else
#define DAER_HANDLER	TRAP(unimplemented_trap)
#endif
#endif

#ifndef INT14_HANDLER
#define INT14_HANDLER	int_handler
#endif

#ifndef INT15_HANDLER
#define INT15_HANDLER	int_handler
#endif

#ifndef UNIMP_HANDLER
#define UNIMP_HANDLER	unimplemented_trap
#endif

#ifndef PRIV_HANDLER
#define PRIV_HANDLER	unimplemented_trap
#endif

#ifndef FPDIS_HANDLER
#define FPDIS_HANDLER	unimplemented_trap
#endif

#ifndef CPDIS_HANDLER
#define CPDIS_HANDLER	unimplemented_trap
#endif

#ifndef OFL_HANDLER
#define OFL_HANDLER	win_overflow
#endif

#ifndef UFL_HANDLER
#define UFL_HANDLER	win_underflow
#endif

#ifndef MALGN_HANDLER
#define MALGN_HANDLER	unimplemented_trap
#endif

#ifndef FPX_HANDLER
#define FPX_HANDLER	unimplemented_trap
#endif

#ifndef CPX_HANDLER
#define CPX_HANDLER	unimplemented_trap
#endif

#ifndef TAGOF_HANDLER
#define TAGOF_HANDLER	unimplemented_trap
#endif

! register file parity error handler
#ifndef RFE_HANDLER
#define RFE_HANDLER	unimplemented_trap
#endif


#ifndef SWTRAP_0
#define SWTRAP_0	set_super
#endif

#ifndef SWTRAP_1
#define SWTRAP_1	unimplemented_trap
#endif

#ifndef SWTRAP_2
#define SWTRAP_2	unimplemented_trap
#endif

#ifndef SWTRAP_3
#define SWTRAP_3	unimplemented_trap
#endif

#ifndef SW_TRAP_HANDLER
#define other_swtraps	unimplemented_trap
#endif

!----------------------------------------------------------------------

#include "exc_handlers.s"

#define TRAP(label) b label; rd %psr, %l0; nop; nop
	.global address_0
	.global _start
address_0:
T_reset:			TRAP(T_RESET)			! 0
T_instr_access_exception:	IAE_HANDLER			! 1
T_unimplemented_instruction:	TRAP(UNIMP_HANDLER)	        ! 2
T_privileged_instruction:	TRAP(PRIV_HANDLER)	        ! 3
T_fp_disabled:			TRAP(FPDIS_HANDLER)	        ! 4
T_window_overflow:		TRAP(OFL_HANDLER)		! 5
T_window_underflow:		TRAP(UFL_HANDLER)		! 6
T_mem_addr_not_aligned:		TRAP(MALGN_HANDLER)	        ! 7
T_fp_exception:			TRAP(FPX_HANDLER)               ! 8
T_data_access_exception:	DAE_HANDLER			! 9
T_tag_overflow:			TRAP(TAGOF_HANDLER)	        ! 10

				TRAP(unimplemented_trap)        ! 11
				TRAP(unimplemented_trap)        ! 12
				TRAP(unimplemented_trap)        ! 13
				TRAP(unimplemented_trap)        ! 14
				TRAP(unimplemented_trap)        ! 15
				TRAP(unimplemented_trap)        ! 16


T_int_1:			int_handler			! 17
T_int_2:			int_handler			! 18
T_int_3:			int_handler			! 19
T_int_4:			int_handler			! 20
T_int_5:			int_handler			! 21
T_int_6:			int_handler			! 22
T_int_7:			int_handler			! 23
T_int_8:			int_handler			! 24
T_int_9:			int_handler			! 25
T_int_10:			int_handler			! 26
T_int_11:			int_handler			! 27
T_int_12:			int_handler			! 28
T_int_13:			int_handler			! 29
T_int_14:			INT14_HANDLER			! 30
T_int_15:			INT15_HANDLER			! 31

T_rferr:                        TRAP(RFE_HANDLER)               ! 32
T_iaerr:			IAER_HANDLER			! 33
                                TRAP(unimplemented_trap)
                                TRAP(unimplemented_trap)
T_cp_disabled:                  TRAP(CPDIS_HANDLER)             ! 36
                                TRAP(unimplemented_trap)
                                TRAP(unimplemented_trap)
                                TRAP(unimplemented_trap)
T_cp_exception:                 TRAP(CPX_HANDLER)               ! 40
T_daerr:                        DAER_HANDLER			! 41

				! unused hardware trap vectors    42 - 127

				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)
				TRAP(unimplemented_trap)

software_traps:							! 128 - 255

				TRAP(SWTRAP_0)
				TRAP(SWTRAP_1)
				TRAP(SWTRAP_2)
				TRAP(SWTRAP_3)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)
				TRAP(other_swtraps)

/*************beginning of reset handler ************************/
T_RESET:	mov 0xfa0, %psr
		mov	0, %tbr

/****************************************************************
 *      Should we initialize the MMU?
 *      if so, we need to flush the entire TLB
 *      (updated for sunergy)
 ****************************************************************/
#ifndef NO_INITIALIZE_MMU
	set	MSFSR_VADDR,%g1 		! Read SFSR to clear mmu faults
	lda	[%g1]MMU_REG_ASI,%g0		! Clear the M-fsr
	set	MENTIRE_FLUSH_VADDR,%g1
	sta	%g0,[%g1]MMU_FLUSH_ASI		! Demap all entries
#endif
/****************************************************************
 * By default the following modes are set in the processor control 
 * register (PCR).  Set "NO_ENABLE_MMU" if you don't want
 * the defaults; define "DISABLE_ITBR" if you want ITBR disabled:
 *
 *	EN	1       MMU-enabled
 *	NF	0       NoFault-disabled
 *	IT	0	ITBR-enabled	
 *	SBAE	0000	Sbus arbitration disabled for all devices
 *	DE  	0   	Dcache-disabled
 *	IE	0	Icache-disabled
 *	rsvd	00
 *	PE	0	Parity checking disabled
 *	VE	0	Video disabled
 *	BM	0	Boot mode disabled
 *	RC	00	Refresh control set to 0
 *	PC	0	Even parity on memory interface
 *	CG	0	Color graphics - monochrome
 *	CC	0	Color CRT - monochrome     
 *	MV	0	Internal Memory Bus View - disabled
 *	DV	0	Data Bus View - disabled
 *	AV	0	Address Bus View - disabled
 *
 *****************************************************************/
#ifndef	DISABLE_ITBR
#define	MPCR_DATA	0x00000001
#endif
	
#ifndef MPCR_DATA
#define MPCR_DATA	0x00000005
#endif

#ifndef NO_ENABLE_MMU
	set	MCXR_VADDR,%g1		! initialize the context register   
	set	USER_CONTEXT_NUMBER,%g2	! note that we're originally in 
	sta	%g2, [%g1] MMU_REG_ASI	! context 0...this changes us to 1

	set	MCTPR_VADDR,%g1		! initialize the ctpr register	
	set	CTP_VALUE,%g2		! CTP_VALUE is set to 0xf0000000	
	sta	%g2, [%g1] MMU_REG_ASI

	set 	MPCR_VADDR,%g1		! turn on the mmu with defaults
	set 	MPCR_DATA, %g2
	sta 	%g2, [%g1] MMU_REG_ASI
#endif
 
	b _start
	mov	0, %wim

/********** end of reset trap handler ****************************/


unimplemented_trap:
		b	unimplemented_trap;
		nop

lastWindowMask:	.word 0

win_overflow:
	! a simple window overflow handler.
	! This handler does not check for page faults,
	! It assumes memory on the stack is resident.
	! This code cannot be interrupted.

	! This code not recommended for use on real hardware

		rd	%wim, %l3	! save wim before clearing it
		mov	0, %wim
		nop
		nop
		nop

computeLastWindowMask:
		set	lastWindowMask, %l5	! may have mask in memory
		ld	[%l5], %l4
		cmp	%l4, 0
		bne	haveLastWindowMask
		nop

		! must figure out how many windows there are.
		! This only has to be done once.

		mov	%g1, %l4		! free up three globals
		mov	%g2, %l5
		mov	%g3, %l6
		mov	%l0, %g1		! save psr in g1 for access
						! from another window
		and	%g1, 0x1f, %l7		! get CWP
		mov	1, %g2
		sll	%g2, %l7, %g2		! window mask for trap window

1:		restore				! increment CWP until
		rd	%psr, %g3		! it wraps around to zero.
		and	%g3, 0x1f, %g3		! extract CWP
		cmp	%g3, 0
		be	1f
		nop
		b	1b
		sll	%g2, 1, %g2

1:		wr	%g1, %psr		! restore trap window
		nop
		nop
		nop
		mov	%l4, %g1		! restore g1
		mov	%g2, %l4		! lastWindowMask
		mov	%l5, %g2		! restore g2 and g3
		mov	%l6, %g3
		set	lastWindowMask, %l5
		st	%l4, [%l5]		! save mask in memory

haveLastWindowMask:

		save			! decrement CWP to enter window to save
		
		st	%l0, [%sp + 0]	! save locals onto the stack
		st	%l1, [%sp + 4]
		st	%l2, [%sp + 8]
		st	%l3, [%sp + 12]
		st	%l4, [%sp + 16]
		st	%l5, [%sp + 20]
		st	%l6, [%sp + 24]
		st	%l7, [%sp + 28]

		st	%i0, [%sp + 32]	! save in's onto the stack
		st	%i1, [%sp + 36]
		st	%i2, [%sp + 40]
		st	%i3, [%sp + 44]
		st	%i4, [%sp + 48]
		st	%i5, [%sp + 52]
		st	%i6, [%sp + 56]	! frame pointer
		st	%i7, [%sp + 60]	! return address

		restore			! return to trap window

		cmp	%l3, 1		! %l3 contains the saved WIM
		be	rotate_right	! do we have to rotate around the end?
		srl	%l3, 1, %l3	! SHIFT WIM mask

restoreWIM:
		wr	%l3, %wim	! put in new wim
		nop			! need 3 instruction delay
		jmp	%l1		! and return from trap
		rett	%l2

rotate_right:
		mov	%l4, %wim	! put in new wim. from lastWindowMask
		jmp	%l1		! and return from trap
		rett	%l2



win_underflow:
	
		rd	%wim, %l3	! save WIM before clearing it
		mov	0, %wim
		nop
		nop
		nop

		set	lastWindowMask, %l5	! get mask from memory
		ld	[%l5], %l4

		restore			! move to window which needs restoring
		restore

		ld	[%sp + 0], %l0	! restore locals from stack
		ld	[%sp + 4], %l1
		ld	[%sp + 8], %l2
		ld	[%sp + 12], %l3
		ld	[%sp + 16], %l4
		ld	[%sp + 20], %l5
		ld	[%sp + 24], %l6
		ld	[%sp + 28], %l7

		ld	[%sp + 32], %i0	! restore in's from the stack
		ld	[%sp + 36], %i1
		ld	[%sp + 40], %i2
		ld	[%sp + 44], %i3
		ld	[%sp + 48], %i4
		ld	[%sp + 52], %i5
		ld	[%sp + 56], %i6
		ld	[%sp + 60], %i7

		save			! return to trap window
		save

		cmp	%l3, %l4	! %l3 contains saved WIM
		be	rotate_left	! do we have to rotate around the end?
		sll	%l3, 1, %l3	! SHIFT WIM mask

restoreWIMu:
		mov	%l3, %wim	! put in new wim
		jmp	%l1		! and return from trap
		rett	%l2

rotate_left:
		mov	1, %wim		! put in new wim
		jmp	%l1		! and return from trap
		rett	%l2

set_super:
		set	0xc0, %l7	! sets S and PS
		mov	%psr, %l6
		or	%l7, %l6, %l7
		mov	%l7, %psr
		nop ; nop ; nop ; nop

		jmp	%l2
		rett	%l2 + 4
