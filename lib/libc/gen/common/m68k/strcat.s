	.data
/*	.asciz "@(#)strcat.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

| Usage: strcat(s1, s2)
| Concatenate s2 on the end of s1.  S1's space must be large enough.
| Return s1.

ENTRY(strcat)
	movl	PARAM,a0	| s1
	movl	PARAM2,a1	| s2
	movl	a0,d0		| return s1 at the end

	moveq	#-1,d1		| count of 65535
| The following loop runs in loop mode on 68010
1$:	tstb	a0@+		| find the terminating null for s1
	dbeq	d1,1$
	bne	1$		| not zero yet - keep going

	subql	#1,a0		| point to the null

	moveq	#-1,d1		| count of 65535
| The following loop runs in loop mode on 68010
2$:	movb	a1@+,a0@+	| move byte from s2 to s1
	dbeq	d1,2$
	bne	2$		| if zero byte not seen, continue

	RET
