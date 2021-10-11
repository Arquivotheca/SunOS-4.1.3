/*
 *	.seg	"data"
 *	.asciz	"@(#)trap.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include <machine/asm_linkage.h>
#include <machine/reg.h>
#include <machine/psl.h>

/*
 * Make sure the vector for 'trap #0' is installed. Kadb in particular
 * depends on this.
 *
 * set_vec(vbr)
 *	int vbr;
 */
	ENTRY(set_vec)
	bclr	0xfff, %o0		! set tt field to 0x80 for trap #0
	or	%o0, 0x800, %o0
	set	trap_0, %o1
	ldd	[%o1], %o2		! copy prototype to trap table
	std	%o2, [%o0]
	ldd	[%o1 + 8], %o2
	std	%o2, [%o0 + 8]
	mov	%psr, %o0		! enable traps
	or	%o0, PSR_ET, %o0
	mov	%o0, %psr
	nop
	retl
	mov	%g0, %o0

	.align 8
trap_0:	sethi	%hi(systrap), %l7
	or	%l7, %lo(systrap), %l7
	jmp	%l7
	mov	%psr, %l0
	.align 4

/*
 * System call handler. We get here after a 't 0' instruction, so we're
 * trapped somewhere in a syscall stub. We won't rett directly back to the
 * caller; instead we'll make it look like we simply called syscall().
 * Syscall()'s return will pass control back to the stub right after
 * the trap instruction.
 */
systrap:
!
! When we enter:
!   %i0 : syscall code
!   %i1 : arg 0
!   %i2 : arg 1
!   %i3 : arg 2
!   %l1 : trapped %pc
!   %l2 : trapped %npc
!
! Decrement the return address so the system call returns to the right place.
!
	sub	%l1, 4, %i7
!
! Push the args, then leave room for a window overflow. This must be
! based on CALLER'S current stack pointer (our frame pointer), not our
! current sp, since we want out of this window.
!
	st	%i1, [%fp + 0]
	st	%i2, [%fp + 4]
	st	%i3, [%fp + 8]
	mov	%fp, %i1
	add	%fp, -SA(MINFRAME), %fp
!
! Set up addresses so that when we leave the trap control passes to syscall().
!
	set	_syscall, %l1
	add	%l1, 4, %l2
	jmp	%l1
	rett	%l2
