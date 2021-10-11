|       .data
|       .asciz  "@(#)libm3d.S 1.1 92/07/30 SMI"
|       .even
|       .text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#ifdef FSWITCH
#define HEADER(func) RTENTRY(V/**/func/**/d)
#endif
#ifdef FSOFT
#define HEADER(func) RTENTRY(F/**/func/**/d)
#endif
#ifdef FSKY
#define HEADER(func) RTENTRY(S/**/func/**/d)
#endif
#ifdef F68881
#define HEADER(func) RTENTRY(M/**/func/**/d)
#endif
#ifdef FFPA
#define HEADER(func) RTENTRY(W/**/func/**/d)
#endif

#define ENTRYM(func,func81) \
	HEADER(func) ; \
	moveml	d0/d1,sp@- ; \
	f/**/func81/**/d sp@,fp0 ; \
	fmoved	fp0,sp@ ; \
	moveml	sp@+,d0/d1 ; \
	RET

#define ENTRYW(func,func81) ENTRYM(func,func81)

#ifdef FFPA
#undef ENTRYW
#define ENTRYW(func,funcfpa) \
	HEADER(func) ; \
	fpmoved	d0:d1,fpa0 ; \
	fp/**/funcfpa/**/d fpa0,fpa0 ; \
	fpmoved	fpa0,d0:d1 ; \
	RET
#endif

ENTRYW(cos,cos)
ENTRYW(sin,sin)
ENTRYW(atan,atan)
ENTRYW(exp,etox)
ENTRYW(exp1,etoxm1)
ENTRYW(log,logn)
ENTRYW(log1,lognp1)
ENTRYM(tan,tan)
ENTRYM(acos,acos)
ENTRYM(asin,asin)
ENTRYM(cosh,cosh)
ENTRYM(sinh,sinh)
ENTRYM(tanh,tanh)
ENTRYM(pow2,twotox)
ENTRYM(pow10,tentox)
ENTRYM(log2,log2)
ENTRYM(log10,log10)
