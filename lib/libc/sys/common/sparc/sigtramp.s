!
!	"@(#)sigtramp.s 1.1 92/07/30"
!       Copyright (c) 1987 by Sun Microsystems, Inc.
!
	.seg	"text"

#include <machine/asm_linkage.h>
#include <machine/psl.h>
#include <sys/signal.h>
#include "PIC.h"

	.global	__sigtramp, __sigfunc
__sigtramp:
	!
	! On entry sp points to:
	! 	0 - 63: window save area
	!	64: signal number
	!	68: signal code
	!	72: pointer to sigcontext
	!	76: addr parameter
	!
	! A sigcontext looks like:
	!	00: on signal stack flag
	!	04: old signal mask
	!	08: old sp
	!	12: old pc
	!	16: old npc
	!	20: old psr
	!	24: old g1
	!	28: old o0
	!
	! We are also in the old window. We first do a save in which we
	! bump the sp down by enough to have an area to save the old globals,
	! the old floating point registers and the normal frame.
	! This also gives us a fresh set of registers.
	!
	save	%sp, -(SA(MINFRAME)+SA(40*4)), %sp ! make frame on (new) stack
	ld	[%fp + 72], %o2		! get ptr to sigcontext
	set	PSR_EF, %l0
	ld	[%o2 + 20], %o0		! get psr
	mov	%y, %l1			! save y
	btst	%l0, %o0		! is FPU enabled?
	bz	1f			! if not skip FPU save
	ld	[%fp + 64], %o0		! get signal number
	cmp	%o0, SIGFPE		! check if sigfpe
	be,a	1f			! don't save fp registers if sigfpe
	mov	0, %l0			! unset PSR_EF bit, for later check
					! so we don't restore them also

	std	%f0, [%sp + SA(MINFRAME)+(0*4)]	! save all fpu registers.
	std	%f2, [%sp + SA(MINFRAME)+(2*4)]
	std	%f4, [%sp + SA(MINFRAME)+(4*4)]
	std	%f6, [%sp + SA(MINFRAME)+(6*4)]
	std	%f8, [%sp + SA(MINFRAME)+(8*4)]
	std	%f10, [%sp + SA(MINFRAME)+(10*4)]
	std	%f12, [%sp + SA(MINFRAME)+(12*4)]
	std	%f14, [%sp + SA(MINFRAME)+(14*4)]
	std	%f16, [%sp + SA(MINFRAME)+(16*4)]
	std	%f18, [%sp + SA(MINFRAME)+(18*4)]
	std	%f20, [%sp + SA(MINFRAME)+(20*4)]
	std	%f22, [%sp + SA(MINFRAME)+(22*4)]
	std	%f24, [%sp + SA(MINFRAME)+(24*4)]
	std	%f26, [%sp + SA(MINFRAME)+(26*4)]
	std	%f28, [%sp + SA(MINFRAME)+(28*4)]
	std	%f30, [%sp + SA(MINFRAME)+(30*4)]
	st	%fsr, [%sp + SA(MINFRAME)+(32*4)] ! save old fsr
1:
	st	%l1, [%sp + SA(MINFRAME)+(33*4)] ! store copy of %y
	std	%g2, [%sp + SA(MINFRAME)+(34*4)] ! save globals
	std	%g4, [%sp + SA(MINFRAME)+(36*4)]
	std	%g6, [%sp + SA(MINFRAME)+(38*4)]
#ifdef PIC
	PIC_SETUP(o5)
	ld	[%o5 + __sigfunc], %g1
#else
	set	__sigfunc, %g1		! get array of function ptrs
#endif
	ld	[%fp + 68], %o1		! get code
	sll	%o0, 2, %g2		! scale signal number for index
	ld	[%g1 + %g2], %g1	! get func
	call	%g1			! (*_sigfunc[sig])(sig,code,&sc,addr)
	ld	[%fp + 76], %o3		! get addr

	!
	! Restore state.
	!
	ld	[%fp + 72], %i0		! get ptr to sigcontext
	ld	[%sp + SA(MINFRAME)+(33*4)], %l1 ! restore y
	ld	[%i0 + 20], %o0		! get psr
	mov	%l1, %y
	btst	%l0, %o0		! is FPU enabled?
	bz	2f			! if not skip FPU restore
	ldd	[%sp + SA(MINFRAME)+(34*4)], %g2 ! delay slot, restore globals

	ldd	[%sp + SA(MINFRAME)+(0*4)], %f0	! restore all fpu registers.
	ldd	[%sp + SA(MINFRAME)+(2*4)], %f2
	ldd	[%sp + SA(MINFRAME)+(4*4)], %f4
	ldd	[%sp + SA(MINFRAME)+(6*4)], %f6
	ldd	[%sp + SA(MINFRAME)+(8*4)], %f8
	ldd	[%sp + SA(MINFRAME)+(10*4)], %f10
	ldd	[%sp + SA(MINFRAME)+(12*4)], %f12
	ldd	[%sp + SA(MINFRAME)+(14*4)], %f14
	ldd	[%sp + SA(MINFRAME)+(16*4)], %f16
	ldd	[%sp + SA(MINFRAME)+(18*4)], %f18
	ldd	[%sp + SA(MINFRAME)+(20*4)], %f20
	ldd	[%sp + SA(MINFRAME)+(22*4)], %f22
	ldd	[%sp + SA(MINFRAME)+(24*4)], %f24
	ldd	[%sp + SA(MINFRAME)+(26*4)], %f26
	ldd	[%sp + SA(MINFRAME)+(28*4)], %f28
	ldd	[%sp + SA(MINFRAME)+(30*4)], %f30
	ld	[%sp + SA(MINFRAME)+(32*4)], %fsr	! restore old fsr
2:
	ldd	[%sp + SA(MINFRAME)+(36*4)], %g4
	ldd	[%sp + SA(MINFRAME)+(38*4)], %g6
	restore	%g0, 139, %g1		! sigcleanup system call
	t	0
	unimp	0			! just in case it returns
	/*NOTREACHED*/
