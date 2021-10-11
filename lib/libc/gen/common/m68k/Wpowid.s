        .data
|       .asciz  "@(#)Wpowid.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Wdefs.h"

one:	.double	0r1

RTENTRY(Wpowid)
        tstl    a0@
        bnes    6f              | Branch if i <> 0.
        moveml  one,d0/d1  	| Return 1.0 = x**0.
        jra    3f
6:
        LOADFPABASE
	fpmoved@1 d0:d1,fpa0         | fpa0 gets x.
        movel   a0@,d0          | d0 gets integer power i.
        bpls    1f
|        fpmoved@1 fpa0,sp@-	| Save original x.
	movel	a1@(0xc04),sp@-
	movel	a1@(0xc00),sp@-
	negl    d0             | d0 gets abs(i).
        bras    1f
 
powerloop:
        lsrl    #1,d0
        fpsqrd@1  fpa0,fpa0      | If there are n trailing 0's,
                               | this loop computes fpa0 = x**2**n.
1:
        btst    #0,d0
        beqs    powerloop
         
        fpmoved@1  fpa0,fpa1      | fpa1 gets x**2**n.
        bras    6f
         
multloop:
        fpsqrd@1  fpa1,fpa1      | Square fpa1.
        btst    #0,d0
        beqs    6f
        fpmuld@1   fpa1,fpa0    	| If a one bit, multiply result by fpa1.
6:
        lsrl    #1,d0
        bnes    multloop        | Branch if there are more 1 bits.
5:
        tstb    a0@
        bpls    2f
        fpmoved@1 one,fpa1       	| fpa1 gets 1.0.
        fpdivd@1 fpa0,fpa1        | fpa1 gets 1/x**|i|.
        fpmoved@1  fpa1,d0:d1        | Result is 1/x**|i|.
       cmpl    #-1,a0@
       beqs    8f              | Branch if x**-1; can't improve.
	tstl	d1
	bnes	8f		| Branch if result non-zero.
        tstl	d0
	beqs	1f		| Branch if result +0.
	cmpl	#0x80000000,d0
	bnes	8f		| Branch if result not -0.
1:
	moveml	sp@+,d0/d1
        movel   a2,sp@-
        JBSR(Mpowid,a2)
        movel   sp@+,a2
	bras	3f
8:
	addql	#8,sp
	bras	3f
2:
        fpmoved@1  fpa0,d0:d1        | Result is x**i.
3:
        RET
