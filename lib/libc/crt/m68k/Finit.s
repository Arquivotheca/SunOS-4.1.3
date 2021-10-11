        .data
|        .asciz  "@(#)Finit.s 1.1 92/07/30 Copyr 1985 Sun Micro"
        .even
        .text

|       Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Finit)
#ifdef PIC
	PIC_SETUP(a0)
	movl	a0@(_finitfp_:w),a0
	jmp	a0@
#else
	jmp 	_finitfp_
#endif
