!
!	"@(#)ptrace.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

#define	SYS_ptrace	26

	.global	_errno

	ENTRY(ptrace)
#ifdef PIC
	PIC_SETUP(o5)
	ld	[%o5 + _errno], %g1
	clr	[%g1]
#else
	sethi	%hi(_errno), %g1
	clr	[%g1 + %lo(_errno)]
#endif
	mov	SYS_ptrace, %g1
	t	0
	CERROR(o5);
	RET
