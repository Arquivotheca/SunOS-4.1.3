!       .data
!       .asciz  "@(#)fpuversion4sub.s 1.1 92/07/30 SMI"
!       .even
!       .text

!       Copyright (c) 1988 by Sun Microsystems, Inc.

        .seg    "text"
        .global _getfsr
_getfsr:
        sethi   %hi(-72),%g1
        add     %g1,%lo(-72),%g1
        save    %sp,%g1,%sp
        st      %fsr,[%sp+64]
        ld      [%sp+64],%i0
        ret
        restore
  
