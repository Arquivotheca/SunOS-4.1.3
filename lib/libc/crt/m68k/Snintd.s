	.data
|	.asciz	"@(#)Snintd.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

#ifdef PIC
ENTER(Snintd)
	movl	a2,sp@-
	JBSR(Fnintd,a2)
	movl	sp@+,a2
	RET

ENTER(Srintd)
	movl	a2,sp@-
	JBSR(Frintd,a2)
	movl	sp@+,a2
	RET
#else

ENTER(Snintd)
	jmp	Fnintd

ENTER(Srintd)
	jmp	Frintd
#endif PIC
