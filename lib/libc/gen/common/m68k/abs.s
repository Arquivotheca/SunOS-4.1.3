	.data
/*	.asciz	"@(#)abs.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(abs)
	movl	PARAM,d0
	bges	1$
	negl	d0
1$:
	RET
