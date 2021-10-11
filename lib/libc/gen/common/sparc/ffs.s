!	.seg	"data"
!	.asciz	"@(#)ffs.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>

	ENTRY(ffs)
	tst	%o0		! if zero, done
	bz	2f
	clr	%o1		! delay slot, return zero if no bit set
1:
	inc	%o1		! bit that will get checked
	btst	1, %o0
	be	1b		! if bit is zero, keep checking
	srl	%o0, 1, %o0	! shift input right until we hit a 1 bit
2:
	retl
	mov	%o1, %o0	! return value is in o1
