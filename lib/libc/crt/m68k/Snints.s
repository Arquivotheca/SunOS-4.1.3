	.data
|	.asciz	"@(#)Snints.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

#ifdef PIC
ENTER(Snints)
	movl	a2,sp@-
	JBSR(Fnints,a2)
	movl	sp@+,a2
	RET

ENTER(Srints)
	movl	a2,sp@-
	JBSR(Frints,a2)
	movl	sp@+,a2
	RET
#else
ENTER(Snints)
	jmp	Fnints
ENTER(Srints)
	jmp	Frints
#endif PIC
