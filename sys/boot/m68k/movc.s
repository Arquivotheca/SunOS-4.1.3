        .data
        .asciz  "@(#)movc.s 1.1 92/07/30 Copyr 1986 Sun Micro"
        .even
        .text

|       Copyright (c) 1984 by Sun Microsystems, Inc.

#include "asm_linkage.h"

ret:    rts

| Block copy with possibly overlapped operands
        ENTRY(ovbcopy)
        movl    sp@(4),a0       | from
        movl    sp@(8),a1       | to
        movl    sp@(12),d0      | get count
        jle     ret             | return if not positive
        cmpl    a0,a1           | check direction of copy
        jgt     bwd             | do it backwards
| Here if from > to - copy bytes forward
        jra     2$
| Loop mode byte copy
1$:     movb    a0@+,a1@+
2$:     dbra    d0,1$
        rts
| Here if from < to - copy bytes backwards
bwd:    addl    d0,a0           | get end of from area
        addl    d0,a1           | get end of to area
        jra     2$              | enter loop
| Loop mode byte copy
1$:     movb    a0@-,a1@-
2$:     dbra    d0,1$
        rts
