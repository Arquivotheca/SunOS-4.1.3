	.data
|	.asciz	"@(#)Spowid.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text


|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Spowid)
        moveml	d2-d5/a2,sp@-	| Save d2-d5.
#ifdef PIC
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@,SKYBASE
#else
	movl	__skybase,SKYBASE 
#endif
        tstl    ARG2PTR@
        bnes    1f              | Branch if i <> 0.
        movel   #0x3ff00000,d0  | Return 1.0 = x**0.
        clrl   	d1  		| Return 1.0 = x**0.
        jra     3f
1:
				| d0/d1 will be the result.
				| ARG2PTR@ will be i.
				| d2 will be |i|.
				| d3/d4 will be x**2**n.
	movel	ARG2PTR@,d5
        movel	d5,d2
	bpls    4f
        moveml        d0/d1,sp@-      | Save argument x.
        negl    d2              | d2 gets abs(i).
        bras    4f
 
dpowerloop:
       				| If there are n trailing 0's,
                                | this loop computes d0/d1 = x**2**n.
	FMULD(d0,d1,d0,d1,d0,d1)
        lsrl    #1,d2		| While U Wait.
4:
        btst    #0,d2
        beqs    dpowerloop
         
        movel	d0,d3		| d3 gets x**2**n.
        movel	d1,d4		| d4 gets x**2**n.
        bras    6f
         
dmultloop:
	FMULD(d3,d4,d3,d4,d3,d4)
        btst    #0,d2
        beqs    6f
	FMULD(d0,d1,d3,d4,d0,d1)
6:       
        lsrl    #1,d2
        bnes    dmultloop        | Branch if there are more 1 bits.
5:       
        tstl    d5
        jpl    2f
	FDIVD(#0x3ff00000,#0,d0,d1,d0,d1)
        cmpl    #-1,d5
       beqs    8f              | Branch if x**-1; can't improve.
       movel   d0,d2
       andl    #0x7fffffff,d2
       orl     d1,d2
       bnes    8f              | Branch if x**-i was not infinity.
       negl    d5              | d5 gets abs(i).
       moveml  sp@,d0/d1       | Restore original argument.
       movel   d5,d2
       lsrl    #1,d2           | d2 gets -i/2.
       movel   d2,sp@-         | Stack |i|/2.
       lea     sp@,a0
       bsr     Spowid          | Compute x**|i|/2.
       addql   #4,sp           | Bypass |i|/2.
	FDIVD(#0x3ff00000,#0,d0,d1,d0,d1)	| d0/d1 gets x**i/2.
	FMULD(d0,d1,d0,d1,d0,d1)	| d0/d1 gets x**i-{0 or 1}.
       lsll    #1,d2
       cmpl    d2,d5
       beqs    8f              | Branch if d2 = i/2 exactly.
	FDIVD(d0,d1,sp@,sp@(4),d0,d1) | Otherwise divide by x again.
 8:
       addql   #8,sp           | Bypass original argument x.
2:       
7:
3:
	moveml	sp@+,d2-d5/a2
	RET
