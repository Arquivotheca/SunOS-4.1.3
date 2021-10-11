!
!	"@(#)sbrk.s 1.1 92/07/30"
!       Copyright (c) 1986 by Sun Microsystems, Inc.
!
	.seg	"text"

#include "SYS.h"

#define	SYS_brk		17
#define ALIGNSIZE	8

	.global	curbrk
	.global _end
	.seg	"data"
	.align	4
curbrk:	.word	_end
	.seg	"text"

	ENTRY(sbrk)
	add	%o0, (ALIGNSIZE-1), %o0	! round up request to align size
	andn	%o0, (ALIGNSIZE-1), %o0
#ifdef PIC
	PIC_SETUP(o5)
	ld	[%o5 + curbrk], %g1
	ld	[%g1], %o3
#else
	sethi	%hi(curbrk), %o2
	ld	[%o2 + %lo(curbrk)], %o3
#endif
	add	%o3, (ALIGNSIZE-1), %o3	! round up curbrk to align size
	andn	%o3, (ALIGNSIZE-1), %o3
	add	%o3, %o0, %o0		! new break setting = request + curbrk
	mov	%o0, %o4		! save it
	mov	SYS_brk, %g1
	t	0
	CERROR(o5)
#ifdef PIC
	PIC_SETUP(o5)
	ld	[%o5 + curbrk], %g1
	st	%o4, [%g1]
#else
	st	%o4, [%o2 + %lo(curbrk)] ! store new break in curbrk
#endif
	retl
	mov	%o3, %o0		! return old break
