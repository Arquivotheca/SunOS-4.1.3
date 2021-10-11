	.data
|	.asciz	"@(#)Fpowis.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Fpowis)
        tstl   	d1
        bnes    1f              | Branch if i <> 0.
        movel   #0x3f800000,d0  | Return 1.0 = x**0.
        jra     3f
1:
	moveml	d2-d5/a2,sp@-	| Save d2-d5.
				| d0 will be the result.
				| d2 will be |i|.
	movel	d1,d5		| d5 gets i.
        movel	d5,d2
	bpls    4f
        movel	d0,sp@-		| Save argument x.
	negl    d2              | d2 gets abs(i).
        bras    4f
 
dpowerloop:
       				| If there are n trailing 0's,
                                | this loop computes d0/d1 = x**2**n.
	JBSR(Fsqrs,a2)
        lsrl    #1,d2		| While U Wait.
4:
        btst    #0,d2
        beqs    dpowerloop
         
        movel	d0,d4		| d4 gets x**2**n.
        bras    6f
         
dmultloop:
	JBSR(Fsqrs,a2)
        btst    #0,d2
        beqs    6f
	movel	d0,d3 		| d3 saves x**2**n.
	movel	d4,d1
	JBSR(Fmuls,a2)		| d0/d1 gets answer so far.
	movel	d0,d4 		| d4 gets answer so far.
	movel	d3,d0		| d0 gets x**2**n.
6:       
        lsrl    #1,d2
        bnes    dmultloop       | Branch if there are more 1 bits.
5:       
        tstl    d5
        bpls    2f		| Branch if non-negative power.
	movel	#0x3f800000,d0 	| d0/d1 gets 1.0.
	movel	d4,d1
	JBSR(Fdivs,a2)		| d0 gets 1.0/f.
        cmpl 	#-1,d5
        beqs    8f              | Branch if x**-1; can't improve.
	movel	d0,d2
	andl	#0x7fffffff,d2
	bnes	8f		| Branch if x**-i was not infinity.
        negl    d5              | d5 gets abs(i).
        movel   sp@,d0          | Restore original argument.
        movel   d5,d2
        lsrl    #1,d2           | d2 gets -i/2.
        movel   d2,d1         	| d1 gets |i|/2.
        bsr     Fpowis          | Compute x**|i|/2.
        movel   d0,d1 		| d1 gets x**-i/2.
        movel   #0x3f800000,d0  | d0 gets 1.0.
        JBSR(Fdivs,a2)           | d0 gets x**i/2.
        JBSR(Fsqrs,a2)           | d0 gets x**i-{0 or 1}.
        lsll    #1,d2
        cmpl    d2,d5
        beqs    8f              | Branch if d2 = i/2 exactly.
        movel   sp@,d1
        JBSR(Fdivs,a2)	 	| Otherwise,a0 divide by x again.
8:
        addql   #4,sp           | Bypass original argument x.
        bras    7f
2:       
	movel	d4,d0		|Final result.
7:
	moveml	sp@+,d2-d5/a2
3:
	RET
