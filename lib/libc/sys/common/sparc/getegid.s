!
!	"@(#)getegid.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

	PSEUDO(getegid,getgid)
	retl			/* egid = getegid(); */
	mov	%o1, %o0
