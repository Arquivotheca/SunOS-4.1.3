	.data
/*	.asciz	"@(#)ffs.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

ENTRY(ffs)
	movl	PARAM,d0
	beqs	return
	moveq	#-1,d1
loop:
	lsrl	#1,d0
	dbcs	d1,loop
	movl	d1,d0
	negl	d0
return:	RET
