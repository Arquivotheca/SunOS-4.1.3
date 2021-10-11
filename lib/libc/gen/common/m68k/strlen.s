	.data
/*	.asciz "@(#)strlen.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

| Usage: strlen(s)
| Returns the number of
| non-NULL bytes in string argument.

ENTRY(strlen)
	movl	PARAM,a0	| s
	movl	a0,a1		| pointer for scanning

	moveq	#-1,d1		| count of 65535
| The following loop runs in loop mode on 68010
1$:	tstb	a1@+		| find the terminating null for s1
	dbeq	d1,1$
	bne	1$		| not zero yet - keep going

	subql	#1,a1		| point to the null
	subl	a0,a1		| get the length
	movl	a1,d0		| and return it
	RET
