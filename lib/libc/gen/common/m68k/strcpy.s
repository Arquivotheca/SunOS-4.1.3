	.data
/*	.asciz "@(#)strcpy.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

| Usage: strcpy(s1, s2)
| Copy string s2 to s1.  s1 must be large enough.
| return s1

ENTRY(strcpy)
	movl	PARAM,a0	| s1
	movl	PARAM2,a1	| s2
	movl	a0,d0		| return s1 at the end

	moveq	#-1,d1		| count of 65535
| The following loop runs in loop mode on 68010
1$:	movb	a1@+,a0@+	| move byte from s2 to s1
	dbeq	d1,1$
	bne	1$		| if zero byte seen, done

	RET
