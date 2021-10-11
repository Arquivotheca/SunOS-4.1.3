!	@(#)kprof.s 1.1 92/07/30 SMI
!	Copyright (c) 1986 by Sun Microsystems, Inc.

	.seg	"text"
	.align	4

#include <machine/psl.h>
#include <machine/intreg.h>
#include "assym.s"

	.seg	"data"
	.global	_kpri, _kticks, _kprimin4

_kpri:
	.word 0, 0, 0, 0, 0, 0, 0, 0
	.word 0, 0, 0, 0, 0, 0, 0, 0
_kticks:
	.word 0
_kprimin4:
	.word 0*4			! Lowest prio to trace PC of, times 4.

	.seg	"text"
	.global	kprof
/*
 * Kernel profiling interrupt, called once it is determined
 * a valid clock interrupt at level 14 is pending.
 *
 * Use the local registers in the trap window and return.
 * entry state:	%l0 = psr
 *		%l1 = pc
 *		%l2 = npc
 */

kprof:
	and	%l0, PSR_PIL, %l4	! priority(times 4) into %l4
	srl	%l4, PSR_PIL_BIT-2, %l4
	set	_kpri, %l6		! address of priority buckets
	ld	[%l6 + %l4], %l7	! increment priority bucket
	inc	%l7
	st	%l7, [%l6 + %l4]

	sethi	%hi(_kprimin4), %l7	! minimum priority to profile
	lduh	[%l7 + %lo(_kprimin4)], %l7
	cmp	%l4, %l7		! if current priority < min*4, return
	bl	kdone
	btst	PSR_PS, %l0		! check pS bit
	bz	kdone			! not in supervisor mode, return
	sethi	%hi(_profiling), %l7
	ld	[%l7 + %lo(_profiling)], %l7
	cmp	%l7, 2			! if profiling > two levels deep
	bge	kdone			! forget about it here

	sethi	%hi(_s_lowpc), %l7
	ld	[%l7 + %lo(_s_lowpc)], %l7
	subcc	%l1, %l7, %l3		! subtract out origin
	blu	kdone			! if pc < origin, return
	sethi	%hi(_s_textsize), %l7
	ld	[%l7 + %lo(_s_textsize)], %l7
	cmp	%l3, %l7
	bge	kdone			! if pc > text, return
	sethi	%hi(_kcount), %l7
	ld	[%l7 + %lo(_kcount)], %l4 ! address of kernel counters array
	srl	%l3, 2, %l3		! divide offset by 2
	sll	%l3, 1, %l3
	lduh	[%l4 + %l3], %l7	! increment a bucket
	inc	%l7
	sth	%l7, [%l4 + %l3]
kdone:
	!
	! reset pending interrupt and return from trap
	!
	mov	%l0, %psr		! reinstall PSR_CC
	set	INTREG_ADDR, %l6
	ldub	[%l6], %l7		! read interrupt register
	bclr	IR_ENA_CLK14, %l7
	stb	%l7, [%l6]		! reset interrupt
	bset	IR_ENA_CLK14, %l7
	stb	%l7, [%l6]		! re-enable interrupt
	jmp	%l1			! return from trap
	rett	%l2
