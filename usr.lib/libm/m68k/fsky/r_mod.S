	.data
	.asciz	"@(#)r_mod.S 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "DEFS.h"

ENTRY(r_mod) 			| Fortran needs to call Fmods to pass fcvs.
	movel	PARAM,a0
	movel	a0@,d0
	movel	PARAM2,a0
	movel	a0@,d1
	jsr	Fmods
	RET	
