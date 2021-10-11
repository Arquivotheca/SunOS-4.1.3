 	.data
|	.asciz	"@(#)Slibms.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"
#include "Sdefs.h"

#define ENTRYF(func) \
	RTENTRY(S/**/func/**/s) ; \
	movel	d0,sp@- ; \
	pea	sp@ ; \
	jsr	_F/**/func/**/_ ; \
	addql	#8,sp ; \
	RET

#define ENTRYX(func,x) \
	RTENTRY(S/**/func/**/s) ; \
	movel	d0,sp@- ; \
	pea	sp@ ; \
	jsr	_F/**/x/**/_ ; \
	addql	#8,sp ; \
	RET

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
ENTRYF(acos)
ENTRYF(asin)
ENTRYS(atan,ATAN)
ENTRYF(cosh)
ENTRYF(sinh)
ENTRYF(tanh)
ENTRYS(exp,EXP)
ENTRYX(exp1,expm1)
ENTRYX(pow2,exp2)
ENTRYX(pow10,exp10)
ENTRYS(log,LOG)
ENTRYX(log1,log1p)
ENTRYF(log2)
ENTRYF(log10)
