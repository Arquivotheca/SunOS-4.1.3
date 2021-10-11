!
!	"@(#)fp_save.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

/*
 * FPSAVESIZE is the minimum block size to allocate on
 * the stack for storage of the floating point state.
 * The actual size of the block that gets used will
 * either FPSAVESIZE or FPSAVESIZE+4 depending on the
 * alignment of the stack pointer.
 */
#define FPSAVESIZE	0x84		

/*
 * Floating point save. (for user signal handling)
 * In order to use 'std' we have to insure that that the
 * destination block address is double word aligned
 */
	ENTRY(fp_save)
	mov	FPSAVESIZE, %g1
	btst	0x7, %sp		! check for double word alignment
	bz,a	1f			! sp aligned at 4 by default
	add	%g1, 4, %g1		! add 4 to align sp on 8 byte boudary
1:	sub	%sp, %g1, %sp		! allocate block on stack
	st	%fsr, [%sp + 0x80]	! synchronize and save state
	st	%g1, [%sp+ FPSAVESIZE]	! size of allocated block
	std	%f0, [%sp + 0x0]	! save floating point registers
	std	%f2, [%sp + 0x8]
	std	%f4, [%sp + 0x10]
	std	%f6, [%sp + 0x18]
	std	%f8, [%sp + 0x20]
	std	%f10, [%sp + 0x28]
	std	%f12, [%sp + 0x30]
	std	%f14, [%sp + 0x38]
	std	%f16, [%sp + 0x40]
	std	%f18, [%sp + 0x48]
	std	%f20, [%sp + 0x50]
	std	%f22, [%sp + 0x58]
	std	%f24, [%sp + 0x60]
	std	%f26, [%sp + 0x68]
	std	%f28, [%sp + 0x70]
	retl
	std	%f30, [%sp + 0x78]	! delay slot, store last fp reg

/*
 * Floating point restore. (for user signal handling)
 * Restore fpu state and deallocate storage from stack
 */
	ENTRY(fp_restore)
	ldd	[%sp + 0x0], %f0
	ldd	[%sp + 0x8], %f2
	ldd	[%sp + 0x10], %f4
	ldd	[%sp + 0x18], %f6
	ldd	[%sp + 0x20], %f8
	ldd	[%sp + 0x28], %f10
	ldd	[%sp + 0x30], %f12
	ldd	[%sp + 0x38], %f14
	ldd	[%sp + 0x40], %f16
	ldd	[%sp + 0x48], %f18
	ldd	[%sp + 0x50], %f20
	ldd	[%sp + 0x58], %f22
	ldd	[%sp + 0x60], %f24
	ldd	[%sp + 0x68], %f26

	ldd	[%sp + 0x70], %f28
	ldd	[%sp + 0x78], %f30
	ld	[%sp + 0x80], %fsr
	ld	[%sp + FPSAVESIZE], %g1
	retl
	add	%sp, %g1, %sp		! dealy slot, deallocate block on stack
