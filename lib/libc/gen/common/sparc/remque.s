!	.seg	"data"
!	.asciz	"@(#)remque.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>

/*
 * remque(entryp)
 *
 * Remove entryp from a doubly linked list
 */
	ENTRY(remque)
	ld	[%o0], %g1		! entryp->forw
	ld	[%o0 + 4], %g2		! entryp->back
	st	%g1, [%g2]		! entryp->back = entryp->forw
	retl
	st	%g2, [%g1 + 4]		! entryp->forw = entryp->back
