        .data
|       .asciz  "@(#)Mdefault.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1986 by Sun Microsystems, Inc.

#include "fpcrtdefs.h"

RTENTRY(_Mdefault)                       | Initializes 68881 for default modes.
        fmoveml threezeros,fpcr/fpsr/fpiar
        RET
threezeros: .long	0,0,0
