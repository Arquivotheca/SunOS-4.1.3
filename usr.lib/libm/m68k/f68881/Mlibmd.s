 	.data
|	.asciz	"@(#)Mlibmd.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRYD(func,func81) \
	RTENTRY(M/**/func/**/d) ; \
	moveml	d0/d1,sp@- ; \
	f/**/func81/**/d sp@,fp0 ; \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1 ; \
	RET

ENTRYD(cos,cos)
ENTRYD(sin,sin)
ENTRYD(tan,tan)
ENTRYD(acos,acos)
ENTRYD(asin,asin)
ENTRYD(atan,atan)
ENTRYD(cosh,cosh)
ENTRYD(sinh,sinh)
ENTRYD(tanh,tanh)
ENTRYD(exp,etox)
ENTRYD(exp1,etoxm1)
ENTRYD(pow2,twotox)
ENTRYD(pow10,tentox)
ENTRYD(log,logn)
ENTRYD(log1,lognp1)
ENTRYD(log2,log2)
ENTRYD(log10,log10)
