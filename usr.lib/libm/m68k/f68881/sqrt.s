 	.data
|	.asciz	"@(#)sqrt.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

ENTRY(sqrt)			| Nominal entry point.
	fsqrtd	sp@,fp0
	fmoved	fp0,sp@
	moveml	sp@+,d0/d1
	RET
