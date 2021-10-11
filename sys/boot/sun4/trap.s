/*
 *	.seg	"data"
 *	.asciz	"@(#)trap.s 1.1 92/07/30 SMI"
 *	Copyright (c) 1988 by Sun Microsystems, Inc.
 */
#include <sun4/asm_linkage.h>

#define	SAVE_ARGS	BRELOC-ARGPUSHSIZE
#define	SAVE_RESULTS	BRELOC-8

#define	TRAP_VECTOR_BASE	0xffe810d8

	.data
	.align	4
_save_args:
	.skip	4*4
_save_sp:
	.skip	4


/*
 * Make sure the vector for 'trap #0'
 */
	.globl	_set_vec
_set_vec:
	set	TRAP_VECTOR_BASE, %l0
	ld	[%l0], %l0
	set	systrap, %o0
	st	%o0, [%l0 + 0x200]	! write new vector
	nop
	nop
	retl				! done, leaf routine return
	nop

/*
 * System call handler.
 */
systrap:
	save	%sp,-WINDOWSIZE,%sp
	! This lunacy is to get around the window being lost
	! in the PROM on trap..
	set	SAVE_ARGS, %o0
	ld	[%o0+0x0], %i0
	ld	[%o0+0x4], %i1
	ld	[%o0+0x8], %i2
	ld	[%o0+0xc], %i3
	! End of lunacy
	set	_save_args, %o1
	st	%i1, [%o1+0]
	st	%i2, [%o1+4]
	st	%i3, [%o1+8]
	mov	%i0, %o0		! which syscall
	nop
	nop
	call	_syscall		! syscall(rp)
	! add	%sp, MINFRAME, %o0	! ptr to reg struct
	nop
	set	SAVE_RESULTS, %o1
	st	%o0, [%o1]
	mov	%o0, %i0
	ret
	restore
	nop
	/* end syscall */
