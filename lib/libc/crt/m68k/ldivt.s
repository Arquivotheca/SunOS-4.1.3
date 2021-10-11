	.data
|	.asciz	"@(#)ldivt.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.
#include "PIC.h"
#include "DEFS.h"
	.globl	ldivt, uldivt
div_neg_a:
	| the dividend is negative: make it positive;
	| if the divisor is also negative, then do a positive division
	| else quotient is negative.
	negl	d0
	tstl	d1
	bges	1$
	    negl	d1
	    bras	div_recall
1$:	bsrs	div_recall
	negl	d0
	rts
div_neg_b:
	| the dividend is positive, but the divisor is negative.
	| do negative division.
	negl	d1
	bsrs	div_recall
	negl	d0
	rts
divide_by_zero:
	| the divisor is zero. if we're going to dump core,
	| lets put down a link, so that adb will grot our core image
	link	a6,#0
	divu	d1, d0	| BOOM!
	unlk	a6
	rts

uldivt:	| ENTRYPOINT FOR UNSIGNED DIVISION
	| arguments in d0, d1
	| return quotient in d0, garbage in d1
	LINK
	RTMCOUNT
#ifdef PROF
	unlk a6
#endif PROF
	tstl	d1
	beqs	divide_by_zero
	bras	div_recall

ldivt:	| ENTRYPOINT FOR SIGNED DIVISION
	| receive arguments in d0, d1
	| return quotient in d0, garbage in d1
	LINK
	RTMCOUNT
#ifdef PROF
	unlk a6
#endif PROF
	tstl	d0
	blts	div_neg_a	| dont't deal with signs here
	tstl	d1
	blts	div_neg_b
	beqs	divide_by_zero
div_recall:
#define QUOT	1
#include "divide.include"
