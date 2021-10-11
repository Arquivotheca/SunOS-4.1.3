!
!	"@(#)getppid.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

	PSEUDO(getppid,getpid)
	retl			/* ppid = getppid(); */
	mov	%o1, %o0
