/*	@(#)proc_init.s 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

#ifdef	MULTIPROCESSOR

#include <sys/param.h>
#include <machine/vmparam.h>
#include <sys/errno.h>
#include <machine/asm_linkage.h>
#include <machine/cpu.h>
#include <machine/intreg.h>
#include <machine/mmu.h>
#include <machine/pcb.h>
#include <machine/psl.h>
#include <machine/pte.h>
#include <machine/reg.h>
#include <machine/trap.h>
#include <machine/scb.h>

#include "percpu_def.h"

#include "assym.s"

/*
 * Processor initialization
 *
 * The boot prom turns control over to us with the MMU
 * turned on and the context table set to whatever the
 * master asked for. How nice!
 */
	ENTRY(cpuX_startup)

	set	RMMU_CTP_REG, %g1	! read ctp register
	lda	[%g1]ASI_MOD, %g1	! just to verify it.

	set	PSR_S|PSR_PIL, %g1
	mov	%g1, %psr		! setup psr, disable traps
	nop ; nop

	mov	0x02, %wim		! setup wim
	mov	0, %fp			! setup frame

	set	ekernstack - SA(MINFRAME + REGSIZE), %g1
	mov	%g1, %sp		! set stack pointer

#ifndef	PC_vscb
	set	scb, %g1
#else	PC_vscb
	sethi	%hi(_cpuid), %g2
	ld	[%g2+%lo(_cpuid)], %g2
	set	VA_PERCPU+PC_vscb, %g1
	sll	%g2, PERCPU_SHIFT, %g2
	or	%g1, %g2, %g1
#endif	PC_vscb
	mov	%g1, %tbr		! set trap base

	set	_idleuarea, %l0		! point uunix at idleuarea
	sethi	%hi(_uunix), %l1	! must be before traps are enabled.
	st	%l0, [%l1+%lo(_uunix)];

	!
	! Dummy up fake user registers on the stack.
	!
	set	USRSTACK-WINDOWSIZE, %g1
	st	%g1, [%sp + MINFRAME + SP*4] ! user stack pointer
	set	PSL_USER, %l0
	st	%l0, [%sp + MINFRAME + PSR*4] ! psr
	set	USRTEXT, %g1
	st	%g1, [%sp + MINFRAME + PC*4] ! pc
	add	%g1, 4, %g1
	st	%g1, [%sp + MINFRAME + nPC*4] ! npc

! invalidate the dual (one-to-one) mapping for low memory,
! we do not need it any more.

        sethi   %hi(_cache), %l0
        ld      [%l0 + %lo(_cache)], %l0
        cmp     %l0, CACHE_PAC_E                !CC mode?
        bne	1f                              !No, don't cache the
	set	RMMU_CTP_REG, %l0
	!
	! We need to set the AC bit.
	!
	lda	[%l0]ASI_MOD, %l0	! get context table ptr
	sll	%l0, 4, %l0		! convert to paddr

        lda     [%g0]ASI_MOD, %l1       ! get MMU CSR
        set     CPU_VIK_AC, %l2         ! AC bit
        or      %l1, %l2, %l2           ! or in AC bit
        sta     %l2, [%g0]ASI_MOD       ! store new CSR
	lda	[%l0]ASI_MEM, %l0	! get level 1 table ptr

	srl	%l0, 4, %l0		! mask off dont-cares
	sll	%l0, 8, %l0		! convert to paddr
	sta	%g0, [%l0]ASI_MEM	! zero out the entry
	call	_mmu_flushall		! flush the MMU
	sta     %l1, [%g0]ASI_MOD       ! restore CSR;
	b	2f
	nop


1:	lda	[%l0]ASI_MOD, %l0	! get context table ptr
	sll	%l0, 4, %l0		! convert to paddr
	lda	[%l0]ASI_MEM, %l0	! get level 1 table ptr
	srl	%l0, 4, %l0		! mask off dont-cares
	sll	%l0, 8, %l0		! convert to paddr
	call	_mmu_flushall		! flush the MMU, after
	sta	%g0, [%l0]ASI_MEM	!   zeroing out the entry

2:	mov	%psr, %g1
	wr	%g1, PSR_ET, %psr	! enable traps
	nop ; nop

	! Call pxmain, which will return as the idle process
	! for our appropriate CPU

	call	_pxmain
	add	%sp, MINFRAME, %o0
	!
	! Proceed as if this was a normal user trap.
	! Safety; won't be here anyways
	!
	b,a	_idlework
#endif	MULTIPROCESSOR
