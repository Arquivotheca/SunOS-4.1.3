	.data
|	.asciz	"@(#)lmodt.s 1.1 92/07/30 Copyr 1983 Sun Micro"
	.even
	.text

|	Copyright (c) 1983 by Sun Microsystems, Inc.

#include "PIC.h"
#include "DEFS.h"
	.globl	lmodt, ulmodt
div_neg_a:
	| the dividend is negative: make it positive;
	| make sure the divisor is positive, too
	| remainder takes sign of dividend
	negl	d0
	tstl	d1
	bges	1$
	    negl	d1
1$:	bsrs	div_recall
	negl	d0
	rts
divide_by_zero:
	| the divisor is zero. if we're going to dump core,
	| lets put down a link, so that adb will grot our core image
	link	a6,#0
	divu	d1, d0	| BOOM!
	unlk	a6
	rts

ulmodt:	| ENTRYPOINT FOR UNSIGNED REMAINDERING
	| arguments in d0, d1
	| return remainder in d0, garbage in d1
	LINK
	RTMCOUNT
#ifdef PROF
	unlk a6
#endif PROF
	tstl	d1
	beqs	divide_by_zero
	bras	div_recall

lmodt:	| ENTRYPOINT FOR SIGNED REMAINDERING
	| receive arguments in d0, d1
	| return remainder in d0, garbage in d1
	LINK
	RTMCOUNT
#ifdef PROF
	unlk a6
#endif PROF
	tstl	d0
	blts	div_neg_a	| dont't deal with signs here
	tstl	d1
	beqs	divide_by_zero
	bges	div_recall
	    negl	d1	| sign of divisor unimportant
div_recall:
#define REM 1
#include "divide.include"
