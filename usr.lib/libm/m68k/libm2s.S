|       .data
|       .asciz  "@(#)libm2s.S 1.1 92/07/30 SMI"
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

#define ENTRY2(func) \
        HEADER(func) ; \
        moveml  d0/d1,sp@- ; \
        pea	sp@(4) ; \
        pea	sp@(4) ; \
	jsr     _r_/**/func/**/_ ; \
        lea   	sp@(16),sp ; \
        RET
 
ENTRY2(atan2) 

#ifdef FSKY
#include "Sdefs.h"
RTENTRY(Spows)
        movl    __skybase,SKYBASE
        movw    #S_SPOW,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    d1,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
#else
ENTRY2(pow)
#endif
