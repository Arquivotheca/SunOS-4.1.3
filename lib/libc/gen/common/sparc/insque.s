!	.seg	"data"
!	.asciz	"@(#)insque.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>

/*
 * insque(entryp, predp)
 *
 * Insert entryp after predp in a doubly linked list.
 */
	ENTRY(insque)
	ld	[%o1], %g1		! predp->forw
	st	%o1, [%o0 + 4]		! entryp->back = predp
	st	%g1, [%o0]		! entryp->forw =  predp->forw
	st	%o0, [%o1]		! predp->forw = entryp
	retl
	st	%o0, [%g1 + 4]		! predp->forw->back = entryp
