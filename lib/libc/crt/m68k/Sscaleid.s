	.data
|	.asciz	"@(#)Sscaleid.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Sscaleid)
#ifdef PIC
	movl	a2,sp@-
	JBSR(Fscaleid,a2)
	movl	sp@+,a2
	RET
#else
	jmp	Fscaleid
#endif

