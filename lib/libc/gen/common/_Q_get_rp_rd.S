#ifdef sparc
!
! @(#)_Q_get_rp_rd.S 1.1 92/07/30 SMI
!
! Copyright (c) 1989 by Sun Microsystems, Inc.
!
#endif

#include "SYS.h"

#ifdef sparc
	.global __Q_get_rp_rd, _fp_precision, _fp_direction
__Q_get_rp_rd:
#ifdef PIC
	PIC_SETUP(o5)
	ld     [%o5+_fp_direction],%o3
#else
	set     _fp_direction,%o3
#endif
	set     0xc0000000,%o4          ! mask of rounding direction bits
        st      %fsr,[%sp+0x44]
        ld      [%sp+0x44],%o0          ! o0 = fsr
        and     %o0,%o4,%o1
        srl     %o1,30,%o1
	st	%o1,[%o3]
#ifdef PIC
	ld     [%o5+_fp_precision],%o3
#else
	set     _fp_precision,%o3
#endif
	set     0x30000000,%o4
	and     %o0,%o4,%o1
	srl     %o1,28,%o1
        retl
	st	%o1,[%o3]
#endif


#ifdef mc68020
	.globl __Q_get_rp_rd, _fp_direction, _fp_precision
__Q_get_rp_rd:
#ifdef PIC
	PIC_SAVE(a2,a2)
	PIC_SETUP(a1)
#endif
	movl	d2,a0
        fmovel  fpcr,d0
        lsrl    #4,d0
        moveq   #3,d2
        andl    d2,d0
#ifdef PIC
	movl	a1@(_fp_direction),a2
	movl	d0,a2@
#else
	movl	d0,_fp_direction
#endif
        fmovel  fpcr,d0
        lsrl    #6,d0
        moveq   #3,d2
        andl    d2,d0
        movl    a0,d2
#ifdef PIC
	movl	a1@(_fp_precision),a2
	movl	d0,a2@
	PIC_RESTORE(a2,a2)
#else
	movl	d0,_fp_precision
#endif
        RET
#endif


#ifdef i386
	.global	_Q_get_rp_rd, fp_direction, fp_precision
_Q_get_rp_rd:
	pushl   %ebp
        movl    %esp,%ebp
        subl    $4,%esp
        fstcw   -4(%ebp)
        movl    -4(%ebp),%eax
        andl    $0xc00,%eax
        shrl    $10,%eax
	movl	%eax,fp_direction
        movl    -4(%ebp),%eax
	shrl    $8,%eax
	andl    $0x3,%eax
	movl	%eax,fp_precision
        leave
        ret
#endif
