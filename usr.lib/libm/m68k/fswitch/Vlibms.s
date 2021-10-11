 	.data
|	.asciz	"@(#)Vlibms.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRYS(func) \
	RTENTRY(V/**/func/**/s) ; \
	movel	d0,sp@- ; \
	pea	sp@ ; \
	jsr	_F/**/func/**/_ ; \
	addql	#8,sp ; \
	RET

#define ENTRYX(func,x) \
	RTENTRY(V/**/func/**/s) ; \
	movel	d0,sp@- ; \
	pea	sp@ ; \
	jsr	_F/**/x/**/_ ; \
	addql	#8,sp ; \
	RET

ENTRYS(cos)
ENTRYS(sin)
ENTRYS(tan)
ENTRYS(acos)
ENTRYS(asin)
ENTRYS(atan)
ENTRYS(cosh)
ENTRYS(sinh)
ENTRYS(tanh)
ENTRYS(exp)
ENTRYX(exp1,expm1)
ENTRYX(pow2,exp2)
ENTRYX(pow10,exp10)
ENTRYS(log)
ENTRYX(log1,log1p)
ENTRYS(log2)
ENTRYS(log10)

#define FVENTRY(func) ; \
	RTENTRY(fv/**/func/**/is) ; \
	jsr	V/**/func/**/s ; \
	RET

FVENTRY(exp)
FVENTRY(log)
FVENTRY(pow)
FVENTRY(sin)
FVENTRY(cos)
FVENTRY(tan)
FVENTRY(atan)

