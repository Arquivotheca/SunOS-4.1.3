	.data
/*	.asciz	"@(#)insque.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(insque)
	movl	PARAM,a0
	movl	PARAM2,a1
	movl	a1@,a0@
	movl	a1,a0@(4)
	movl	a0,a1@
	movl	a0@,a1
	movl	a0,a1@(4)
	RET
