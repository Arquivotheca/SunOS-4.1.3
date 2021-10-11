        .data
|       .asciz  "@(#)Wpowis.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"
#include "PIC.h"

one:	.single	0r1

RTENTRY(Wpowis)
#ifdef PIC
	movl	a2,sp@-
	bsr	Wpowis__
	movl	sp@+,a2
	RET
Wpowis__:
#endif
	tstl    d1
        bnes    6f              | Branch if i <> 0.
        movel   one,d0  	| Return 1.0 = x**0.
        jra    3f
6:
        LOADFPABASE
        fpmoves@1 d0,fpa0         | fpa0 gets x.
        movel   d1,d0          | d0 gets integer power i.
        bpls    1f
        fpmoves@1 fpa0,sp@-	| Save original argument x.
	negl    d0             | d0 gets abs(i).
        bras    1f
 
powerloop:
        lsrl    #1,d0
        fpsqrs@1  fpa0,fpa0      | If there are n trailing 0's,
                               | this loop computes fpa0 = x**2**n.
1:
        btst    #0,d0
        beqs    powerloop
         
        fpmoves@1  fpa0,fpa1      | fpa1 gets x**2**n.
        bras    6f
         
multloop:
        fpsqrs@1  fpa1,fpa1      | Square fpa1.
        btst    #0,d0
        beqs    6f
        fpmuls@1   fpa1,fpa0    	| If a one bit, multiply result by fpa1.
6:
        lsrl    #1,d0
        bnes    multloop        | Branch if there are more 1 bits.
5:
        tstl    d1
        bpls    2f
        fpmoves@1 one,fpa1       	| fpa1 gets 1.0.
        fpdivs@1 fpa0,fpa1        | fpa1 gets 1/x**|i|.
        fpmoves@1  fpa1,d0        | Result is 1/x**|i|.
	cmpl	#-1,d1
	beqs	8f		| Branch if power is -1.
	tstl	d0
	beqs	1f		| Branch if result is +0.
	cmpl	#0x80000000,d0
	bnes	8f		| Branch if result not -0.
1:
        movel	sp@+,d0		| Restore argument x.
	JBSR(Mpowis,a2)		| Recompute.
	bras    3f
8:
	addql	#4,sp
	bras	3f
2:
        fpmoves@1  fpa0,d0        | Result is x**i.
3:
        RET
