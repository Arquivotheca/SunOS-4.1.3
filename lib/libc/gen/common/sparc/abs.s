!	.seg	"data"
!	.asciz	"@(#)abs.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.seg	"text"

#include <sun4/asm_linkage.h>

	ENTRY(abs)
	tst	%o0
	bl,a	1f
	neg	%o0
1:
	retl
	nop
