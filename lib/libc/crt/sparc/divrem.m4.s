!	.seg	"data"
!	.asciz	"@(#)divrem.m4.s 1.1 92/07/30 Copyr 1986 Sun Micro"
	.seg	"text"
/*
 * divison/remainder
 *
 * Input is:
 *	dividend -- the thing being divided
 * divisor  -- how many ways to divide
 * Important parameters:
 *	N -- how many bits per iteration we try to get
 *		as our current guess: define(N, 4)
 *	WORDSIZE -- how many bits altogether we're talking about:
 *		obviously: define(WORDSIZE, 32)
 * A derived constant:
 *	TOPBITS -- how many bits are in the top "decade" of a number:
 *		define(TOPBITS, eval( WORDSIZE - N*((WORDSIZE-1)/N) ) )
 * Important variables are:
 *	Q -- the partial quotient under development -- initally 0
 *	R -- the remainder so far -- initially == the dividend
 *	ITER -- number of iterations of the main division loop will
 *		be required. Equal to CEIL( lg2(quotient)/N )
 *		Note that this is log_base_(2^N) of the quotient.
 *	V -- the current comparand -- initially divisor*2^(ITER*N-1)
 * Cost:
 *	current estimate for non-large dividend is 
 *		CEIL( lg2(quotient) / N ) x ( 10 + 7N/2 ) + C
 *	a large dividend is one greater than 2^(31-TOPBITS) and takes a 
 *	different path, as the upper bits of the quotient must be developed 
 *	one bit at a time.
 */

#include <machine/trap.h>
#include <machine/asm_linkage.h>

define(dividend, `%o0')
define(divisor,	`%o1')
define(Q,	`%o2')
define(R,	`%o3')
define(ITER,	`%o4')
define(V,	`%o5')
define(SIGN,	`%g1')
define(T,	`%g2')	! working variable
define(SC,	`%g3')

/*
 * this is the recursive definition of how we develop quotient digits.
 * it takes three important parameters:
 *	$1 -- the current depth, 1<=$1<=N
 *	$2 -- the current accumulation of quotient bits
 *	N  -- max depth
 * We add a new bit to $2 and either recurse or 
 * insert the bits in the quotient.
 * Dynamic input:
 *	R -- current remainder
 *	Q -- current quotient
 *	V -- current comparand
 *	cc -- set on current value of R
 * Dynamic output:
 * R', Q', V', cc'
 */

define(DEVELOP_QUOTIENT_BITS,
`	!depth $1, accumulated bits $2
	bl	L.$1.eval(2^N+$2)
	srl	V,1,V
	! remainder is positive
	subcc	R,V,R
	ifelse( $1, N,
	`	b	9f
		add	Q, ($2*2+1), Q
	',`	DEVELOP_QUOTIENT_BITS( incr($1), `eval(2*$2+1)')
	')
L.$1.eval(2^N+$2):	! remainder is negative
	addcc	R,V,R
	ifelse( $1, N,
	`	b	9f
		add	Q, ($2*2-1), Q
	',`	DEVELOP_QUOTIENT_BITS( incr($1), `eval(2*$2-1)')
	')
	ifelse( $1, 1, `9:')
')


ifelse( ANSWER, `quotient', `
	RTENTRY(.udiv)		! UNSIGNED DIVIDE
	b	divide
	mov	0,SIGN		! result always positive

	RTENTRY(.div)		! SIGNED DIVIDE
	orcc	divisor,dividend,%g0 ! are either dividend or divisor negative
	bge	divide		! if not, skip this junk
	xor	divisor,dividend,SIGN ! record sign of result in sign of SIGN
		tst	divisor
		bge	2f
		tst	dividend
	!	divisor < 0
		bge	divide
		neg	divisor
	2:
	!	dividend < 0
		neg	dividend
	!	FALL THROUGH
',`
	RTENTRY(.urem)		! UNSIGNED REMAINDER
	b	divide
	mov	0,SIGN		! result always positive

	RTENTRY(.rem)		! SIGNED REMAINDER
	orcc	divisor,dividend,%g0 ! are either dividend or divisor negative
	bge	divide		! if not, skip this junk
	mov	dividend,SIGN	! record sign of result in sign of SIGN
		tst	divisor
		bge	2f
		tst	dividend
	!	divisor < 0
		bge	divide
		neg	divisor
	2:
	!	dividend < 0
		neg	dividend
	!	FALL THROUGH
')

divide:
!	compute size of quotient, scale comparand
	orcc	divisor,%g0,V	! movcc	divisor,V
	bnz	0f		! if divisor != 0
	mov	dividend,R
	ba	zero_divide
	nop
0:
	cmp     R,V
	blu     got_result ! if R<V already, there's no point in continuing
	mov	0,Q
	sethi	%hi(1<<(WORDSIZE-TOPBITS-1)),T
	cmp	R,T
	blu	not_really_big
	mov	0,ITER
	!
	! here, the dividend is >= 2^(31-N) or so. We must be careful here, as
	! our usual N-at-a-shot divide step will cause overflow and havoc. The
	! total number of bits in the result here is N*ITER+SC, where SC <= N.
	! compute ITER, in an unorthodox manner: know we need to Shift V into
	!	the top decade: so don't even bother to compare to R.
	1:
		cmp	V,T
		bgeu	3f
		mov	1,SC
		sll	V,N,V
		b	1b
		inc	ITER
	! now compute SC
	2:	addcc	V,V,V
		bcc	not_too_big ! bcc	not_too_big
		add	SC,1,SC
			!
			! here if the divisor overflowed when Shifting
			! this means that R has the high-order bit set
			! restore V and subtract from R
			sll	T,TOPBITS,T ! high order bit
			srl	V,1,V ! rest of V
			add	V,T,V
			b	do_single_div
			sub	SC,1,SC
	not_too_big:
	3:	cmp	V,R
		blu	2b
		nop
		be	do_single_div
		nop
	! V > R: went too far: back up 1 step
	!	srl	V,1,V
	!	dec	SC
	! do single-bit divide steps
	!
	! we have to be careful here. We know that R >= V, so we can do the
	! first divide step without thinking. BUT, the others are conditional,
	! and are only done if R >= 0. Because both R and V may have the high-
	! order bit set in the first step, just falling into the regular 
	! division loop will mess up the first time around. 
	! So we unroll slightly...
	do_single_div:
		deccc	SC
		bl	end_regular_divide
		nop
		sub	R,V,R
		mov	1,Q
		b,a	end_single_divloop
	single_divloop:
		sll	Q,1,Q
		bl	1f
		srl	V,1,V
		! R >= 0
		sub	R,V,R
		b	2f
		inc	Q
	1:	! R < 0
		add	R,V,R
		dec	Q
	2:	
	end_single_divloop:
		deccc	SC
		bge	single_divloop
		tst	R
		b,a	end_regular_divide

not_really_big:
1:	
	sll	V,N,V
	cmp	V,R
	bleu	1b
	inccc	ITER
	be	got_result
	dec	ITER
do_regular_divide:

!	do the main division iteration
	tst	R
!	fall through into divide loop
divloop:
	sll	Q,N,Q
	DEVELOP_QUOTIENT_BITS( 1, 0 )
end_regular_divide:
	deccc	ITER
	bge	divloop
	tst	R
	bl,a	got_result
ifelse( ANSWER, `quotient',
`	dec	Q
',`	add	R,divisor,R
')

got_result:
	tst	SIGN
	bl,a	1f
ifelse( ANSWER, `quotient',
`	neg	%o2	! quotient  <- -Q
',`	neg	%o3	! remainder <- -R
')
1:
	retl
ifelse( ANSWER, `quotient',
`	mov	%o2,%o0	! quotient  <-  Q
',`	mov	%o3,%o0	! remainder <-  R
')
	
zero_divide:
	ta	ST_DIV0		! divide by zero trap
	retl			! if handled, ignored, return
	mov	0, %o0
