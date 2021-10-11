|       .data
|       .asciz  "@(#)libm3s.S 1.1 92/07/30 SMI"
|       .even
|       .text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#ifdef FSWITCH
#define HEADER(func) RTENTRY(V/**/func/**/s)
#endif
#ifdef FSOFT
#define HEADER(func) RTENTRY(F/**/func/**/s)
#endif
#ifdef FSKY
#define HEADER(func) RTENTRY(S/**/func/**/s)
#endif
#ifdef F68881
#define HEADER(func) RTENTRY(M/**/func/**/s)
#endif
#ifdef FFPA
#define HEADER(func) RTENTRY(W/**/func/**/s)
#endif

#define ENTRYM(func,func81) \
	HEADER(func) ; \
        f/**/func81/**/s d0,fp0 ; \
        fmoves  fp0,d0 ; \
	RET

#define ENTRYW(func,func81) ENTRYM(func,func81)

#ifdef FFPA
#undef ENTRYW
#define ENTRYW(func,funcfpa) \
	HEADER(func) ; \
        fpmoves d0,fpa0 ; \
        fp/**/funcfpa/**/s fpa0,fpa0 ; \
        fpmoves fpa0,d0 ; \
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
