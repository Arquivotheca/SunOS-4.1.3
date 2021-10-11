        .data 
|        .asciz  "@(#)Flength2s.s 1.1 92/07/30 SMI"
        .even
        .text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "PIC.h"

RTENTRY(Flength2s)
        moveml  d0-d3/a2,sp@-        | Save x/y.
        JBSR(Fexpos,a2)                 | d0 gets exponent of x.
        exg	d0,d1			| d0 gets y; d1 gets expo(x).
        JBSR(Fexpos,a2)                 | d0 gets exponent of y.
        cmpl    d0,d1
        bges    1f
        movel   d0,d2     		| d2 gets scale factor.
        bras    2f
1:

        movel   d1,d2     		| d2 gets scale factor.
2:
        negl    d2        		| Reverse sign.
        movel   sp@+,d0               	| d0 get x.
        movel   d2,d1     		| d1 gets scale factor.
        JBSR(Fscaleis,a2)               | d0 get x/s.
        JBSR(Fsqrs,a2)                  | d0 get (x/s)**2
        movel   d0,d3
        movel   sp@+,d0                 | Restore y.
        movel   d2,d1     		| d1 gets address of scale factor.
        JBSR(Fscaleis,a2)               | d0 get y/s.
        JBSR(Fsqrs,a2)                  | d0 get (y/s)**2
        movel   d3,d1
        JBSR(Fadds,a2)                  | d0 get sum of squares.
        JBSR(Fsqrts,a2)
        movel	d2,d1
	negl	d1
	JBSR(Fscaleis,a2)
 	moveml	sp@+,d2/d3/a2
	RET
