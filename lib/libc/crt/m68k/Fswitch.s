	.data
|	.asciz	"@(#)Fswitch.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Fswitch)
RTENTRY(_fswitchfp_)
	moveml	a0/a1/d0,sp@-
	JBSR(Finit,a0)
	JBSR(float_switch,a0)
	moveml	sp@+,a0/a1/d0
	RET
