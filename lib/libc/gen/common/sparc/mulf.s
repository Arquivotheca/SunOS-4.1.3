!	.seg	"data"
!	.asciz  "@(#)mulf.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.seg	"text"

#include <machine/asm_linkage.h>

	ENTRY(mulf)
	save	%sp, -SA(WINDOWSIZE), %sp
	mov	%i0, %o0
	call	.umul		! this will give us the high-order bits in %o1
	mov	%i1, %o1
	add	%o1, 1, %o1
	srl	%o1, 1, %i0
	ret
	restore
