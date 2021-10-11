 	.data
|	.asciz	"@(#)base_conv.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

|	68000 assembly language version of base_primitives.c.

#include "DEFS.h"

        .text
        .globl  __fourdigits
__fourdigits:
        LINK
	MCOUNT
        moveq   #3,d1
        movl    PARAM2,a0		| a0 gets address of string.
        addl	#4,a0
	movl    PARAM,d0		| d0 gets base-10000 digit.
L16:
        divu	#10,d0			| d0 gets rem : quo.
        swap	d0			| d0 gets quo : rem.
	addb	#48,d0			| d0 gets quo : digit.
        movb    d0,a0@-			| write digit.
	clrw	d0
	swap	d0			| d0 gets 0 : quo.
	dbra    d1,L16
        RET

        .text
        .globl  __quorem10000
__quorem10000:
        LINK   
	MCOUNT
        movl    PARAM,d0		| d0 gets argument.
        divu    #10000,d0		| d0 gets rem : quo.
        movl	d0,d1
	andl	#0xffff,d0		| d0 gets 0 : quo.
	clrw	d1
	swap	d1			| d1 gets 0 : rem.
	movl    PARAM2,a0		| a0 gets address of remainder.
        movl    d1,a0@
        RET

	.text
	.globl	__mul_10000
__mul_10000:
	link	a6,#-16
	MCOUNT
	moveml	#8416,sp@
	movl	a6@(16),a0
	movl	a0@,d7			| d7 gets carry.
	movl	a6@(8),a5		| a5 gets pb.
	movw	a6@(14),d5		| d5 gets i.
	jle	L14
	subqw	#1,d5
L17:
	movl	a5@+,d0			| d0 gets *pb.
	mulu	#10000,d0		| d0 gets 10000 * bi.
	addl	d7,d0			| d0 gets 10000 * bi + carry.
	movl	d0,d7			| d7 gets carry/bi.
	clrw	d7
	swap	d7			| d7 gets carry.
	movw	d0,a5@(-2)		| Store bi.
	dbra	d5,L17
	jra	L18
L14:
	negw	d5
	subqw	#1,d5
	addqw	#2,a5			| Point to lower order 16 bits.
L21:
	movw	a5@,d0			| d0 gets *pb.
	mulu	#10000,d0		| d0 gets 10000 * bi.
	addl	d7,d0			| d0 gets 10000 * bi + carry.
	movl	d0,d7			| d7 gets carry/bi.
	clrw	d7
	swap	d7			| d7 gets carry.
	movw	d0,a5@			| Store bi.
	subql	#4,a5
	dbra	d5,L21
L18:
	movl	a6@(16),a0
	movl	d7,a0@
	moveml	a6@(-16),#8416
	unlk	a6
	rts

	.text
	.globl	__mul_65536
__mul_65536:
	link	a6,#-16
	MCOUNT
	moveml	#8416,sp@
	movl	a6@(16),a0
	movl	a0@,d7
	movl	a6@(8),a5
	movw	a6@(14),d5
	jle	L25
	subqw	#1,d5
L28:
	movl	a5@+,d0			| d0 gets bi.
	swap	d0			| d0 gets 2**16 * bi.
	movew	d7,d0			| d0 gets 2**16 * bi + carry.
	divu	#10000,d0		| d0 gets bi | carry.
	movw	d0,d7			| d7 gets bi | carry.
	swap	d0			| d0 gets carry | bi.
	movew	d0,a5@(-2)
	dbra	d5,L28
	jra	L29
L25:
	negw	d5
	subqw	#1,d5
	addqw	#2,a5
L32:
        movw    a5@,d0                 	| d0 gets bi.
        swap    d0                      | d0 gets 2**16 * bi.
        movew   d7,d0                   | d0 gets 2**16 * bi + carry.
        divu    #10000,d0               | d0 gets bi | carry.
        movw    d0,d7                   | d7 gets bi | carry.
        swap    d0                      | d0 gets carry | bi.
        movew   d0,a5@
	subql	#4,a5
	dbra	d5,L32
L29:
	movl	a6@(16),a0
	movl	d7,a0@
	moveml	a6@(-16),#8416
	unlk	a6
	rts

#ifdef originalcversion

/*
 * Machine-independent versions of base conversion primitives.
 * Routines to multiply buffers by 2**16 or 10**4. Base 10**4 buffers have
 * b[i] < 10000, carry in and out < 65536. Base 2**16 buffers have b[i] <
 * 65536, carry in and out < 10000. If n is positive, b[0]..b[n-1] are
 * processed; if n is negative, b[0]..b[n+1] are processed. 
 */

void 
_fourdigits(t, d)
	unsigned        t;
	char            d[4];

/* Converts t < 10000 into four ascii digits at *pc.	 */

{
	register short  i;

	i = 3;
	do {
		d[i] = '0' + t % 10;
		t = t / 10;
	}
	while (--i != -1);
}

unsigned 
_quorem10000(u, pr)
	unsigned        u;
	unsigned       *pr;
{
	*pr = u % 10000;
	return (u / 10000);
}

void 
_mul_10000(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-2**16 buffer by 10000. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb++;
			carry = t >> 16;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb * 10000) + carry) & 0xffff;
			pb--;
			carry = t >> 16;
		}
		while (--i != -1);
	}
	*c = carry;
}

void 
_mul_65536(b, n, c)
	unsigned       *b;
	int             n;
	unsigned       *c;
{
	/* Multiply base-10**4 buffer by 65536. */

	register unsigned carry, t;
	register short int i;
	register unsigned *pb;

	carry = *c;
	pb = b;
	if ((i = n) > 0) {
		i--;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb++;
			carry = t / 10000;
		}
		while (--i != -1);
	} else {
		i = -i - 1;
		do {
			*pb = (t = (*pb << 16) | carry) % 10000;
			pb--;
			carry = t / 10000;
		}
		while (--i != -1);
	}
	*c = carry;
}
#endif
