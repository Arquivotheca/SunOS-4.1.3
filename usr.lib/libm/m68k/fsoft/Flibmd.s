 	.data
|	.asciz	"@(#)Flibmd.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRYD(func) \
	RTENTRY(F/**/func/**/d) ; \
	moveml	d0/d1,sp@- ; \
	jsr	_/**/func ; \
	addql	#8,sp ; \
	RET

#define ENTRYX(func,x) \
	RTENTRY(F/**/func/**/d) ; \
	moveml	d0/d1,sp@- ; \
	jsr	_/**/x ; \
	addql	#8,sp ; \
	RET

ENTRYD(cos)
ENTRYD(sin)
ENTRYD(tan)
ENTRYD(acos)
ENTRYD(asin)
ENTRYD(atan)
ENTRYD(cosh)
ENTRYD(sinh)
ENTRYD(tanh)
ENTRYD(exp)
ENTRYX(exp1,expm1)
ENTRYX(pow2,exp2)
ENTRYX(pow10,exp10)
ENTRYD(log)
ENTRYX(log1,log1p)
ENTRYD(log2)
ENTRYD(log10)
