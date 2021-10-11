	.data
	.asciz	"@(#)kprof.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/* 
 * Kernel profiling -- done at level 7 so nearly all code gets profiled.
 */
#include "assym.s"
#include <machine/psl.h>

	.globl	_kprimin4, _kpri, _kticks, kprof
	.data
_kprimin4: .word 0*4		| Lowest prio to trace PC of, times 4.
_kpri:	  .long 0, 0, 0, 0, 0, 0, 0, 0
_kticks: .word 0
	.text

/*
 * The profiling routine.  This is entered on NMI (level 7
 * non-maskable interrupt).  We quickly profile things then return.
 * Called with VOR, PC, SR, <d0,d1,a0,a1> already saved on the stack
 * below the return address.
 */
kprof:
	movw	sp@((4*4)+4),d0	| SR stacked by interrupt
	andw	#SR_INTPRI,d0	| Current priority
	lsrw	#6,d0		| ...as 0-7, times size of bucket (4)
	lea	_kpri,a0
	addql	#1,a0@(0,d0:w)	| Bump priority bucket at A0 + D0
	cmpw	_kprimin4,d0
	jlt	kdone
	btst	#SR_SMODE_BIT-8,sp@((4*4)+4) | were we in supervisor mode?
	jeq	kdone		| no, all done
	movl	sp@((4*4+2)+4),d0	| PC stacked by interrupt
	cmpl	#2,_profiling
	jge	kdone
	movl	_kcount,a0
	subl	_s_lowpc,d0
	jlt	kdone
	cmpl	_s_textsize,d0
	jge	kdone
	lsrl	#2,d0
	lsll	#1,d0
	addl	d0,a0
kover:
	addqw	#1,a0@		| Bump some bucket corresp. to PC
kdone:
	rts
