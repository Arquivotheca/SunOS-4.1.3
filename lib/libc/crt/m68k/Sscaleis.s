	.data
|	.asciz	"@(#)Sscaleis.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Sscaleis)
#ifdef PIC
	movl	a2,sp@-
	JBSR(Fscaleis,a2)
	movl	sp@+,a2
	RET
#else
	jmp	Fscaleis
#endif
