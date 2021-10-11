	.data
|	.asciz	"@(#)Mswitch.s 1.1 92/07/30 Copyr 1985 Sun Micro"
	.even
	.text

|	Copyright (c) 1985 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

ENTER(Mswitch)
RTENTRY(_mswitchfp_)
	moveml	a0/a1/d0,sp@-
	JBSR(Minit,a0)
	tstw	d0
	beqs	1f
	JBSR(float_switch,a0)
	moveml	sp@+,a0/a1/d0
	RET
1:
	pea	end-begin
	pea	begin
	pea	2
	JBSR(_write,a0)
	pea	99
	JBSR(__exit,a0)
begin:
	.asciz	"MC68881 floating point not available -- program requires it\012"
end:
