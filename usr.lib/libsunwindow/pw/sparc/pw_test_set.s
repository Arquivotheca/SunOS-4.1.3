! sccsid[] = "@(#)pw_test_set.s 1.1 92/07/30";
!
!
!  Copyright (c) 1986 by Sun Microsystems, Inc.
!
!
!/* return true if byte/word already set. */
!int
!pw_test_set(lock_flags)
!	int	*lock_flags;
!
	.seg	"text"
	.global	_pw_test_set
!_pw_test_set:
	! "real" routines have a "save" here, but we can get along without.
	!!
	!! NOTE:
	!!	ldst (load word, store ones) only avaliable on Fab-1 chip.
	!!	ldstub (load unsigned byte, store ones) only available on
	!!		not-Fab-1 chips.
	!!
!	ldst	[%o0],%o1	!change to ldstub as appropriate
!	addcc	%o1,%g0,%o0	! mov-and-set-cc
!	bne,a	1f
!	mov	1,%o0		! annulled if == 0
!1:	!end up here anyway
!	jmp	%o7+8
!	nop
!
!	this can be considerably simplified if we just want to return
!	non-zero if the location is set to non-zero. We do the atomic
!	load & store ones, and return the old value in %o0:
!_pw_test_set:
!	ldst	[%o0],%o0	! do load/store
!	jmp	%o7+8		! return
!	nop
!
!	On Fab-2 chips, we'll be able to do even better: we'll
!	put the ldstub in the delay slot of the return instruction.
!	We cannot do this for Fab-1 because of defects in the interlock
!	logic:
!
_pw_test_set:
	jmp	%o7+8		! return
	ldstub	[%o0],%o0	! do load/store in delay slot

