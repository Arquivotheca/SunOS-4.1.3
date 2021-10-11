!
!	"@(#)cerror.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

	.seg	"text"
	.global cerror, _errno
cerror:
#ifdef PIC
	PIC_SETUP(o5)
	ld	[%o5 + _errno], %g1
	st	%o0, [%g1]
#else
	sethi	%hi(_errno), %g1
	st	%o0, [%g1 + %lo(_errno)]
#endif
	retl
	mov	-1, %o0
