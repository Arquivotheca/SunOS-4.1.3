        .data
|	.asciz  "@(#)Sinit.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Sinit)
#ifdef PIC
	movl	a2,sp@-
	JBSR(_sinitfp_,a2)
	movl	sp@+,a2
	RET
#else PIC
	jmp 	_sinitfp_
#endif PIC
