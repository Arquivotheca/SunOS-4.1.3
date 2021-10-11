|       .data
|       .asciz  "@(#)libmd.S 1.1 92/07/30 SMI"
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

#define ENTRYD(func) \
	HEADER(func) ; \
	moveml	d0/d1,sp@- ; \
	jsr	_/**/func ; \
	addql	#8,sp ; \
	RET

#define ENTRYX(func,x) \
	HEADER(func) ; \
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
