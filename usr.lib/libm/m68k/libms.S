|       .data
|       .asciz  "@(#)libms.S 1.1 92/07/30 SMI"
|       .even
|       .text
 
|	Copyright (c) 1987 by Sun Microsystems, Inc.

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

#define ENTRYS(func,x) \
	HEADER(func) ; \
	movel	d0,sp@- ; \
	pea	sp@ ; \
	jsr	_r_/**/x/**/_ ; \
	addql	#8,sp ; \
	RET

ENTRYS(acos,acos)
ENTRYS(asin,asin)
ENTRYS(cosh,cosh)
ENTRYS(sinh,sinh)
ENTRYS(tanh,tanh)
ENTRYS(log10,log10)
ENTRYS(pow2,exp2)
ENTRYS(pow10,exp10)
ENTRYS(exp1,expm1)
ENTRYS(log1,log1p)
ENTRYS(log2,log2)


#ifdef FSKY			/* special -fsky cases */
#undef ENTRYS
#include "Sdefs.h"
#define ENTRYS(func,FUNC) \
	RTENTRY(S/**/func/**/s) ; \
        movl    __skybase,SKYBASE ; \
        movw    #S_S/**/FUNC,SKYBASE@(-OPERAND) ; \
        movl    d0,SKYBASE@ ; \
        IORDY ; \
        movl    SKYBASE@,d0 ; \
	RET

ENTRYS(cos,COS)
ENTRYS(sin,SIN)
ENTRYS(tan,TAN)
ENTRYS(atan,ATAN)
ENTRYS(exp,EXP)
ENTRYS(log,LOG)
#else
ENTRYS(cos,cos)
ENTRYS(sin,sin)
ENTRYS(tan,tan)
ENTRYS(atan,atan)
ENTRYS(exp,exp)
ENTRYS(log,log)
#endif
