	.seg	"data"
	.asciz	"@(#)ieee_std.S 1.1 92/07/30 SMI"

!	Copyright (c) 1988 by Sun Microsystems, Inc.

FRAME = -0x68
	.seg	"text"			! [marker node]

	.proc	4
	.global	_standard_arithmetic,_standard_arithmetic_
_standard_arithmetic:
_standard_arithmetic_:
	save	%sp,FRAME,%sp
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
	.global	_nonstandard_arithmetic,_nonstandard_arithmetic_
_nonstandard_arithmetic:
_nonstandard_arithmetic_:
	save	%sp,FRAME,%sp
	sethi	%hi(0x400000),%o1
	st	%fsr,[%fp-8]
	ld	[%fp-8],%o0
	or	%o0,%o1,%o0		! [OPT_COALESCE]
	st	%o0,[%fp-8]
        ld      [%fp-8],%fsr
	ret
	restore	%g0,%o0,%o0

	.proc	4
	.global	__is_nonstandard
__is_nonstandard:
	save	%sp,-72,%sp
	st	%fsr,[%sp+64]
	ld	[%sp+64],%i0
	sra	%i0,22,%i0
	and	%i0,1,%i0
	ret
	restore
