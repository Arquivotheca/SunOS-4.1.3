!	.seg	"data"
!	.asciz	"@(#)ptr_call.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.seg	"text"

/*
 * Indirect procedure call.
 *	just jump to whatever's in %g1
 */
	.global	.ptr_call
.ptr_call:
	jmp	%g1
	nop
