	.seg	"data"
	.asciz	"@(#)underflow.S 1.1 92/07/30 SMI"

!	Copyright (c) 1987 by Sun Microsystems, Inc.

	.seg	"text"			! [marker node]

	.proc	4
	.global	_gradual_underflow_
_gradual_underflow_:
	save	%sp,-80,%sp
	sethi	%hi(0xffbfffff),%o1
	or	%o1,%lo(0xffbfffff),%o1	! [OPT_CONSTPROP]
	st	%fsr,[%fp-8]
	ld	[%fp-8],%o0
	and	%o0,%o1,%o0		! [OPT_COALESCE]
	st	%o0,[%fp-8]
        ld      [%fp-8],%fsr
	ret
	restore	%g0,%o0,%o0

	.proc	4
	.global	_abrupt_underflow_
_abrupt_underflow_:
	save	%sp,-80,%sp
	sethi	%hi(0x400000),%o1
	st	%fsr,[%fp-8]
	ld	[%fp-8],%o0
	or	%o0,%o1,%o0		! [OPT_COALESCE]
	st	%o0,[%fp-8]
        ld      [%fp-8],%fsr
	ret
	restore	%g0,%o0,%o0
