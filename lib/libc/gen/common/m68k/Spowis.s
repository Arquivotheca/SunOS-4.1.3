	.data
|	.asciz	"@(#)Spowis.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.even
	.text

|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Spowis)
        moveml	d2-d3/a2,sp@-	| Save d2/d3.
#ifdef PIC
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@, SKYBASE
#else
	movl	__skybase,SKYBASE 
#endif
        tstl    d1
        bnes    1f              | Branch if i <> 0.
        movel   #0x3f800000,d0  | Return 1.0 = x**0.
        jra    3f
1:

				| d0 will be the result.
				| d1 will be i.
				| d2 will be |i|.
				| d3 will be x**2**n.
	movel	d1,d2
        bpls    4f
        movel d0,sp@-         | Save argument x.
        negl    d2              | d2 gets abs(i).
        bras    4f
 
powerloop:
       				| If there are n trailing 0's,
                                | this loop computes d0 = x**2**n.
	FMULS(d0,d0,d0)
        lsrl    #1,d2		| While U Wait.
4:
        btst    #0,d2
        beqs    powerloop
         
        movel	d0,d3		| d3 gets x**2**n.
        bras    6f
         
multloop:
	FMULS(d3,d3,d3)
        btst    #0,d2
        beqs    6f
	FMULS(d0,d3,d0)
6:       
        lsrl    #1,d2
        bnes    multloop        | Branch if there are more 1 bits.
5:       
        tstl    d1
        bpls    2f
	FDIVS(#0x3f800000,d0,d0)
         cmpl    #-1,d1
         beqs    8f              | Branch if x**-1; can't improve.
       movel   d0,d2
       andl    #0x7fffffff,d2
       bnes    8f              | Branch if x**-i was not infinity.
        movel   sp@,d0          | Restore original argument.
        movel   d1,d3
        negl    d3              | d3 gets -i.
        movel	d3,d2
	lsrl    #1,d2           | d2 gets -i/2.
        movel   d2,d1                 | d1 gets |i|/2.
        bsr     Spowis          | Compute x**|i|/2.
	FDIVS(#0x3f800000,d0,d0)  | d0 gets x**i/2
	FMULS(d0,d0,d0)		| d0 gets x**i-{0 or 1}.
        lsll    #1,d2
        cmpl    d2,d3
        beqs    8f              | Branch if d2 = i/2 exactly.
	FDIVS(d0,sp@,d0)	| Otherwise divide by x again.
8:
         addql   #4,sp           | Bypass original argument x.

2:       
7:
3:
	moveml	sp@+,d2-d3/a2
	RET
