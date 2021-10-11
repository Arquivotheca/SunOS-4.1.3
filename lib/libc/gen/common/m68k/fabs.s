	.data
/*	.asciz	"@(#)fabs.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(fabs)
	movl	PARAM2,d1
	movl	PARAM,d0
	bclr	#31,d0
	RET
