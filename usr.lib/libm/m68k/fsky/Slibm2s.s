 	.data
|	.asciz	"@(#)Slibm2s.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"
#include "Sdefs.h"

#define ENTRY2(func) \
        RTENTRY(S/**/func/**/s) ; \
        moveml  d0/d1,sp@- ; \
        pea	sp@(4) ; \
        pea	sp@(4) ; \
	jsr     _F/**/func/**/_ ; \
        lea   	sp@(16),sp ; \
        RET
 
ENTRY2(atan2) 

RTENTRY(Spows)
        movl    __skybase,SKYBASE
        movw    #S_SPOW,SKYBASE@(-OPERAND)
        movl    d0,SKYBASE@
        IORDY
        movl    d1,SKYBASE@
        IORDY
        movl    SKYBASE@,d0
        RET
