	.data
|	.asciz	"@(#)Vswitch.s 1.1 92/07/30 SMI"
	.even
	.text

|	Copyright (c) 1987 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Vswitch)
RTENTRY(_vswitchfp_)
	moveml	a0/a1/d0,sp@-
	JBSR(Vinit, a0)
	JBSR(float_switch,a0)
	moveml	sp@+,a0/a1/d0
	RET
