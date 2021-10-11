        .data
|        .asciz  "@(#)Vmode.s 1.1 92/07/30 SMI"
        .even
        .text

|       Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

VECTORED(mode)

	.text

ENTER(_fpmode_)
	movel	sp@(4),a0	| Get address of long argument.
	movel	a0@,d0		| Get long argument.
#ifdef  PIC
	PIC_SETUP(a0)
	movl	a0@(Vmode),a0
	jmp	a0@
#else
	jra	Vmode
#endif 
