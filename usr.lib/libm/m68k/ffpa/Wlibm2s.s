 	.data
|	.asciz	"@(#)Wlibm2s.s 1.1 92/07/30 Copyr 1986 Sun Micro"
 	.even
 	.text
 
|	Copyright (c) 1986 by Sun Microsystems, Inc.

#include "DEFS.h"

#define ENTRY2(func) \
        RTENTRY(W/**/func/**/s) ; \
        moveml  d0/d1,sp@- ; \
        pea	sp@(4) ; \
        pea	sp@(4) ; \
	jsr     _F/**/func/**/_ ; \
        lea   	sp@(16),sp ; \
        RET
 
ENTRY2(atan2) 
ENTRY2(pow)
