	.data
/*	.asciz "@(#)strncpy.s 1.1 92/07/30 SMI"	*/
	.text

#include "DEFS.h"

| Usage: strncpy(s1, s2, n)
| Copy s2 to s1, truncating or null-padding to always copy n bytes
| return s1

ENTRY(strncpy)
	movl	PARAM,a0	| s1
	movl	PARAM2,a1	| s2
	movl	PARAM3,d1	| n
	movl	a0,d0		| return s1 at the end

	jra	2$		| enter byte move loop
| The following loop runs in loop mode on 68010
1$:	movb	a1@+,a0@+	| move byte from s2 to s1
2$:	dbeq	d1,1$
	beq	3$		| if zero byte seen, do null-padding
	clrw	d1		| still more bytes to move
	subql	#1,d1
	bccs	1$		| if count exhausted, done

	RET

3$:	clrl	d0		| move a zero byte
	jra	5$		| enter byte clear loop
| The following loop runs in loop mode on 68010
4$:	movb	d0,a0@+		| clear byte in s1
5$:	dbra	d1,4$
	clrw	d1		| still more bytes to clear
	subql	#1,d1
	bccs	4$		| if count exhausted, done

	movl	PARAM,d0	| return s1 at the end
	RET
