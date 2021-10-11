 	.data
|	.asciz	"@(#)Msqrtd.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

RTENTRY(Msqrtd)			| Secret entry point.
	moveml	d0/d1,sp@-
	fsqrtd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
