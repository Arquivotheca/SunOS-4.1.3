	.data
|	.asciz	"@(#)Fpowid.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Fpowid)
        tstl    a0@
        bnes    1f              | Branch if i <> 0.
        movel   #0x3ff00000,d0  | Return 1.0 = x**0.
        clrl   	d1  		| Return 1.0 = x**0.
        jra     3f
1:
        link	a6,#-8
temp	=	-8		| Double temporary.

	moveml	d2-d5/a2,sp@-	| Save d2-d5.
				| d0/d1 will be the result.
				| ARG2PTR@ will be i.
				| d2 will be |i|.
	movel	a0@,d5		| d5 gets i.
        movel	d5,d2
	bpls    4f
        moveml	d0/d1,sp@-	| Save argument x.
	negl    d2              | d2 gets abs(i).
        bras    4f
 
dpowerloop:
       				| If there are n trailing 0's,
                                | this loop computes d0/d1 = x**2**n.
	JBSR(Fsqrd,a2)
        lsrl    #1,d2		| While U Wait.
4:
        btst    #0,d2
        beqs    dpowerloop
         
        moveml	d0/d1,a6@(temp)	| temp gets x**2**n.
        bras    6f
         
dmultloop:
	JBSR(Fsqrd,a2)
        btst    #0,d2
        beqs    6f
	movel	d0,d3
	movel	d1,d4		| d3/d4 saves x**2**n.
	lea	a6@(temp),a0
	JBSR(Fmuld,a2)		| d0/d1 gets answer so far.
	moveml	d0/d1,a6@(temp) | a6@(temp) gets answer so far.
	movel	d3,d0
	movel	d4,d1
6:       
        lsrl    #1,d2
        bnes    dmultloop        | Branch if there are more 1 bits.
5:       
        tstl    d5
        bpls    2f
	movel	#0x3ff00000,d0 	 | d0/d1 gets 1.0.
	clrl	d1
	lea	a6@(temp),a0
	JBSR(Fdivd,a2)
	cmpl	#-1,d5
	beqs	8f		| Branch if x**-1; can't improve.
	movel	d0,d2
	andl	#0x7fffffff,d2
	orl	d1,d2
	bnes	8f		| Branch if x**-i was not infinity.
	negl	d5		| d5 gets abs(i).
	moveml	sp@,d0/d1	| Restore original argument.
	movel	d5,d2
	lsrl	#1,d2		| d2 gets -i/2.
	movel	d2,sp@-		| Stack |i|/2.
	lea	sp@,a0
	bsr	Fpowid		| Compute x**|i|/2.
	addql	#4,sp		| Bypass |i|/2.
	moveml	d0/d1,a6@(temp)	| Temp gets x**-i/2.
	lea	a6@(temp),a0
	movel	#0x3ff00000,d0 	 | d0/d1 gets 1.0.
	clrl	d1
	JBSR(Fdivd,a2)		| d0/d1 gets x**i/2.
	JBSR(Fsqrd,a2)		| d0/d1 gets x**i-{0 or 1}.
	lsll	#1,d2
	cmpl	d2,d5
	beqs	8f		| Branch if d2 = i/2 exactly.
	lea	sp@,a0
	JBSR(Fdivd,a2)		| Otherwise divide by x again.
8:
	addql	#8,sp		| Bypass original argument x.
	bras	7f
2:       
	moveml	a6@(temp),d0/d1	|Final result.
7:
	moveml	sp@+,d2-d5/a2
	unlk	a6
3:
	RET
