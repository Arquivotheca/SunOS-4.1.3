	.data
|	.asciz	"@(#)Ssqrts.s 1.1 92/07/30 SMI"
	.even
	.text


|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"
#include "Sdefs.h"

RTENTRY(Ssqrts)
#ifdef PIC
	movl	a2,sp@-
	PIC_SETUP(a2)
	movl	a2@(__skybase:w),a2
	movl	a2@, SKYBASE
	bsr	Ssqrts__
	movl	sp@+,a2
	RET
Ssqrts__:
#else
	movl	__skybase,SKYBASE 
#endif
	movw	#S_SSQRT,SKYBASE@(-OPERAND)
	movl	d0,SKYBASE@
	IORDY
	movl	SKYBASE@,d0
	RET
