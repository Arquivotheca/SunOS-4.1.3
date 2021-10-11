|	.data
|	.asciz	"@(#)libm2d.S 1.1 92/07/30 SMI"
|	.even
|	.text

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

#define ENTRY2(func) \
	HEADER(func) ; \
	movel	a0@(4),sp@- ; \
	movel	a0@,sp@- ; \
	moveml	d0/d1,sp@- ; \
	jsr	_/**/func ; \
	lea	sp@(16),sp ; \
	RET

ENTRY2(atan2)
ENTRY2(pow)
