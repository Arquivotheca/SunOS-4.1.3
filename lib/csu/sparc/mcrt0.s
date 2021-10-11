!
!	.asciz "@(#)mcrt0.s 1.1 92/07/30 SMI"
!
! Copyright (c) 1986 by Sun Microsystems, Inc.
!
!	mcrt0.s for the Sparc Architecture
!

#include <machine/asm_linkage.h>
#ifdef	FIX_ALIGN
#include <machine/trap.h>
#endif	FIX_ALIGN

	.seg	"data"
	.global	_environ
_environ:
	.word	0
	.seg	"text"
	.global	_on_exit
	.global	____Argv

!
! Start up a C program.
! On entry the stack frame looks like:
!
!	 _______________________  <- USRSTACK
!	|	    :		|
!	|  arg and env strings	|
!	|	    :		|
!	|-----------------------|
!	|	    0		|
!	|-----------------------|
!	|	    :		|
!	|  ptrs to env strings	|
!	|-----------------------|
!	|	    0		|
!	|-----------------------|
!	|	    :		|
!	|  ptrs to arg strings  |
!	|   (argc = # of wds)	|
!	|-----------------------|
!	|	   argc		|
!	|-----------------------|
!	|    window save area	|
!	|	 (16 wds)	|
!	|_______________________| <- %sp
!
	.seg	"text"
	.align	4
	.global	start
start:
	mov	0, %fp			! stack frame link 0 in main -- for dbx
#ifdef	FIX_ALIGN
	ta	ST_FIX_ALIGN		! enable kernel alignment trap handler
#endif	FIX_ALIGN
	add	%sp, WINDOWSIZE, %l0	! &argc
	add	%l0, 4, %l1		! argv
	ld	[%l0], %l0		! argc
	sethi	%hi(____Argv), %o1
	st	%l1, [%o1 + %lo(____Argv)] ! prog name for profiling
	sll	%l0, 2, %l2		! argc * sizeof(char *)
	add	%l2, 4, %l2		! skip 0 at end of arg ptrs
	add	%l1, %l2, %l2		! environment ptr
	sethi	%hi(_environ), %o1
	st	%l2, [%o1 + %lo(_environ)] ! store in _environ
_$eprol:
	sethi	%hi(_$eprol), %o0	! lowpc
	or	%o0, %lo(_$eprol), %o0
	sethi	%hi(_etext), %o1	! highpc
	or	%o1, %lo(_etext), %o1
!
!	before we make this call, we have to:
!		force 0 mod 8 %sp alignment,
!		and make an arg save area for monstartup, on_exit, and main.
!
	andn	%sp, 07, %sp		! force 0 mod 8 stack alignment
	call	_monstartup		! monstartup(lowpc, highpc)
	sub	%sp, SA(ARGPUSHSIZE+4), %sp ! delay slot, leave room to push args
	sethi	%hi(stopmon), %o0
	or	%o0, %lo(stopmon), %o0
	call	_on_exit		! on_exit( stopmon, NULL )
	mov	0, %o1			! delay slot
	call	start_float		! initialize floating point
	nop
	mov	%l0, %o0		! argc
	mov	%l1, %o1		! argv
	mov	%l2, %o2		! envp
	sethi	%hi(_environ), %o3
	call	_main			! main( argc, argv, environ )
	st	%o2, [%o3 + %lo(_environ)] !just in case it got munged
	call	_exit			! exit(main(...))
	nop				! delay slot
	call	__exit			! we should never get here...
	nop				! delay slot

	.global	stopmon
stopmon:
	save	%sp, -SA(MINFRAME), %sp
	call	_monitor,1
	mov	0, %o0			! delay slot
	mov	0, %i0
	ret
	restore
