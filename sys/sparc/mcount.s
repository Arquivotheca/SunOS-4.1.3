	.seg	"data"			
	.asciz	"@(#)mcount.s 1.1 92/07/30 Copyr 1987 Sun Micro"
	.align	4

!	Copyright (c) 1986 by Sun Microsystems, Inc.

#include <machine/asm_linkage.h>


/* don't mess with FROMPC and SELFPC, or you won't be able to return */
#define	FROMPC		%i7
#define	SELFPC		%o7

/*
 *  register usage:  we're in the same window as the calling routine, so we
 *	only use the first 6 output registers, and there's lots of sharing
 */

#define PROFIL		%o5

#define FROM_P		%o4

#define TOS_P		%o3
#define SCRATCH3	TOS_P

#define TOINDEX		%o2
#define PREVTOP		TOINDEX
#define SCRATCH2	TOINDEX

#define TOP		%o1
#define	SCRATCH1	TOP
#define	PROF_P		TOP

#define	SCRATCH0	%o0

/* some offsets */
#define	T_SELF		0
#define	T_CNT		4
#define	T_LINK		8


	.seg	"text"
	.proc	0
	.global mcount

mcount:

#ifdef	MULTIPROCESSOR

! only do mcount work if inside the kernel lock.

	mov	%tbr, %o0
	sethi	%hi(_klock), %o1
	ld	[%o1+%lo(_klock)], %o1
	srl	%o0, 20, %o0
	and	%o0, 3, %o0
	set	0xFF000008, %o2
	or	%o0, %o2, %o0
	cmp	%o0, %o1
	bne	out
	nop

#endif	MULTIPROCESSOR

!  Make sure that we're profiling, and that we aren't recursively called

	sethi	%hi(_tolimit), SCRATCH0		!Make sure we haven't run out
	ld	[SCRATCH0+%lo(_tolimit)], SCRATCH0
	sethi	%hi(_profiling), PROF_P		!moved up to avoid lockout
	tst	SCRATCH0
	be	out
	sethi	%hi(_s_lowpc), SCRATCH3		!moved up to avoid lockout
	ld	[PROF_P+%lo(_profiling)], PROFIL
	sethi	%hi(_s_textsize), SCRATCH0	!moved up to avoid lockout
	tst	PROFIL
	bne	out
	add	PROFIL, 1, SCRATCH2		! increment profiling
	st	SCRATCH2, [PROF_P+%lo(_profiling)]


!  Make sure (frompc-lowpc) > textsize

	ld	[SCRATCH3+%lo(_s_lowpc)], SCRATCH3
	ld	[SCRATCH0+%lo(_s_textsize)], SCRATCH0
	sub	FROMPC, SCRATCH3, FROM_P
	cmp	FROM_P, SCRATCH0
	bgu,a	out
	st	PROFIL, [PROF_P+%lo(_profiling)]   !delay slot; clear profiling


! from_p = &froms[(frompc-s_lowpc)/(HASHFRACTION*sizeof(short))]

	sethi	%hi(_froms), SCRATCH0
	ld	[SCRATCH0+%lo(_froms)], SCRATCH0
	sra	FROM_P, 2, FROM_P		!divide by 4 then multiply by
	sll	FROM_P, 1, FROM_P		!two; MUST be even
	add	SCRATCH0, FROM_P, FROM_P
	lduh	[FROM_P], TOINDEX		! toindex = *from_p
	sethi	%hi(_tos), TOS_P		!for later; locked out anyway
	tst	TOINDEX
	bne,a	oldarc
	sll	TOINDEX, 2, TOINDEX		! delay slot


! This is the first time we've seen a call from here; get new tostruct

	ld	[TOS_P+%lo(_tos)], TOS_P	! &tos[0]
	sethi	%hi(_tolimit), SCRATCH1
	lduh	[TOS_P+T_LINK], TOINDEX		!tos[0].link(last used struct)
	ld	[SCRATCH1+%lo(_tolimit)], SCRATCH0
	inc	TOINDEX				!increment to free struct
	sth	TOINDEX, [TOS_P+T_LINK]
	cmp	TOINDEX, SCRATCH0		!too many?
	bge,a	overflow
	st	%g0, [SCRATCH1+%lo(_tolimit)]	!delay slot;
	

!  Initialize arc

	sth	TOINDEX, [FROM_P]		!*from_p = toindex
	sll	TOINDEX, 2, TOINDEX
	sll	TOINDEX, 1, SCRATCH0
	add	TOINDEX, SCRATCH0, TOINDEX	!toindex*sizeof(tostruct))
	add	TOS_P, TOINDEX, TOP		!top = &tos[toindex]
	st	SELFPC, [TOP+T_SELF]		!top->selfpc = selfpc (%i7)
	mov	1, SCRATCH0
	st	SCRATCH0, [TOP+T_CNT]		!top->count = 1
	b	done
	sth	%g0, [TOP+T_LINK]		!delay slot; top->link = 0


!  Not a new frompc

oldarc:
	sll	TOINDEX, 1, SCRATCH0
	ld	[TOS_P+%lo(_tos)], TOS_P
	add	TOINDEX, SCRATCH0, TOINDEX	!(toindex*sizeof(tostruct))
	add	TOS_P, TOINDEX, TOP		!top = &tos[toindex]
	ld	[TOP+T_SELF], SCRATCH0		!top->selfpc
	ld	[TOP+T_CNT], SCRATCH2	!for later; don't want to be locked out
	cmp	SCRATCH0, SELFPC		!is this the right arc?
	bne,a	chainloop
	lduh	[TOP+T_LINK], SCRATCH0		!delay slot; top->link


!  Our arc was at the head of the chain.  This is the most common case.

	inc	SCRATCH2
	b	done
	st	SCRATCH2, [TOP+T_CNT]		!delay slot; increment count


!  We have to wander down the linked list, looking for our arc.  We only get
!	here for calls like "*foo()"

chainloop:
	tst	SCRATCH0			!top->link = 0?
	bne,a	next
	mov	TOP, PREVTOP			!delay slot; prevtop = top


!  We're at the end of the chain and didn't find top->selfpc == selfpc
!	Allocate a new tostruct

	lduh	[TOS_P+T_LINK], TOINDEX		!tos[0].link (last used struct)
	sethi	%hi(_tolimit), SCRATCH1
	inc	TOINDEX				!increment to free struct
	ld	[SCRATCH1+%lo(_tolimit)], SCRATCH0
	sth	TOINDEX, [TOS_P+T_LINK]
	cmp	TOINDEX, SCRATCH0		!check for overflow
	bge,a	overflow
	st	%g0, [SCRATCH1+%lo(_tolimit)]	!delay slot;


!  Now initialize it and put it at the head of the chain

	sll	TOINDEX, 2, SCRATCH0
	sll	SCRATCH0, 1, TOP
	add	SCRATCH0, TOP, TOP		!(toindex*sizeof(tostruct))
	add	TOS_P, TOP, TOP			!top=&tos[toindex]
	st	SELFPC, [TOP+T_SELF]		!top->selfpc = selfpc
	mov	1, SCRATCH0
	st	SCRATCH0, [TOP+T_CNT]		!top->count = 1
	lduh	[FROM_P], SCRATCH0
	sth	TOINDEX, [FROM_P]		!*from_p = toindex
	b	done
	sth	SCRATCH0, [TOP+T_LINK]		!top->link = *from_p


!  Check the next arc on the chain

next:					
	sll	SCRATCH0, 2, TOP
	sll	TOP, 1, SCRATCH0
	add	TOP, SCRATCH0, TOP
	add	TOS_P, TOP, TOP			!top = &tos[top->link]
	ld	[TOP+T_SELF], SCRATCH0
	cmp	SCRATCH0, SELFPC		!is top->selfpc = selfpc?
	bne,a	chainloop
	lduh	[TOP+T_LINK], SCRATCH0		!delay slot; top->link


!  We're home.  Increment the count and move it to the head of the chain

	ld	[TOP+T_CNT], SCRATCH3
	lduh	[TOP+T_LINK], SCRATCH0
	inc	SCRATCH3
	st	SCRATCH3, [TOP+T_CNT]		!top->count++
	lduh	[PREVTOP+T_LINK], SCRATCH3	!temp1 = prevtop->link
	sth	SCRATCH0, [PREVTOP+T_LINK]	!prevtop->link = top->link
	lduh	[FROM_P], SCRATCH0		!temp2 = *from_p
	sth	SCRATCH3, [FROM_P]		!*from_p = temp1
	sth	SCRATCH0, [TOP+T_LINK]		!top->link = temp2
done:
	sethi	%hi(_profiling),PROF_P
	retl
	st	PROFIL, [PROF_P+%lo(_profiling)]	! clear profiling
out:
	retl
	nop
overflow:
	save	%sp, -SA(MINFRAME), %sp
	sethi	%hi(OVERMSG), %o0
	call	_printf, 1			!printf(OVERMSG)
	or	%o0, %lo(OVERMSG), %o0
	ret
	restore
	.seg	"data1"			
OVERMSG:
	.ascii	"mcount: tos overflow\n\0"

