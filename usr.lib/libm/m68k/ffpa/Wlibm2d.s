 	.data
|	.asciz	"@(#)Wlibm2d.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRY2(func) \
	RTENTRY(W/**/func/**/d) ; \
	movel	a0@(4),sp@- ; \
	movel	a0@,sp@- ; \
	moveml	d0/d1,sp@- ; \
	jsr	_/**/func ; \
	lea	sp@(16),sp ; \
	RET

ENTRY2(atan2)
ENTRY2(pow)
