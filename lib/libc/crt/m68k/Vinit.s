        .data
|        .asciz  "@(#)Vinit.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Vinit)
#ifdef PIC
	movl	a2,sp@-
	JBSR(_vinitfp_,a2)
	movl	sp@+,a2
	RET
#else
	jmp 	_vinitfp_
#endif PIC
