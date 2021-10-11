 	.data
|	.asciz	"@(#)Wlibmd.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRYM(func,func81) \
	RTENTRY(W/**/func/**/d) ; \
	moveml	d0/d1,sp@- ; \
	f/**/func81/**/d sp@,fp0 ; \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1 ; \
	RET

#define ENTRYW(func,funcfpa) \
	RTENTRY(W/**/func/**/d) ; \
	fpmoved	d0:d1,fpa0 ; \
	fp/**/funcfpa/**/d fpa0,fpa0 ; \
	fpmoved	fpa0,d0:d1 ; \
	RET

ENTRYW(cos,cos)
ENTRYW(sin,sin)
ENTRYM(tan,tan)
ENTRYM(acos,acos)
ENTRYM(asin,asin)
ENTRYW(atan,atan)
ENTRYM(cosh,cosh)
ENTRYM(sinh,sinh)
ENTRYM(tanh,tanh)
ENTRYW(exp,etox)
ENTRYW(exp1,etoxm1)
ENTRYM(pow2,twotox)
ENTRYM(pow10,tentox)
ENTRYW(log,logn)
ENTRYW(log1,lognp1)
ENTRYM(log2,log2)
ENTRYM(log10,log10)
