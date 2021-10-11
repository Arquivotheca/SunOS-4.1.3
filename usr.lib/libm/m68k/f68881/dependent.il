/*	@(#)dependent.il 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

	.inline _pow_rr,8	| float _pow_rr( float *x, *y )
	movl	sp@+,a0		| a0 gets address of x.
	movl	sp@+,a1		| a1 gets address of y.
	fmovel	fpsr,d1		| d1 gets caller's status.
	fmovel	fpcr,d0		| d0 gets caller's modes.
	fmovel	#0,fpcr		| Set default modes. 	
	flogns	a0@,fp0		| fp0 gets log(x).
	fmuls	a1@,fp0		| fp0 gets y*log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	fmovel	d0,fpcr		| Restore caller's modes.
	btst	#9,d0	
	jne	1f		| Go the long way if inexact trap is intended.
	btst	#3,d1
	jeq	1f		| Go the long way if accrued inexact bit was off.
	fmovel	fpsr,d0
	andl	#0x07000000,d0	| Get Z/I/NaN condition code bits.
	jeq	2f		| Go the long way if result is zero, infinity, or NaN.
1:
	fmovel	d1,fpsr		| Restore caller's status.
	movl	a1,sp@- 	| Push parameters for _r_pow_.
	movl	a0,sp@-
	jsr	_r_pow_		| Compute carefully.
	fmoves	d0,fp0
	movl	sp@+,a0		| Clear parameters for _r_pow_.
	movl	sp@+,a0
2:
	fmoves	fp0,d0
	.end

	.inline _r_pow_,8	| float _r_pow_( float *x, *y )
	movl	sp@+,a0		| a0 gets address of x.
	movl	sp@+,a1		| a1 gets address of y.
	fmovel	fpsr,d1		| d1 gets caller's status.
	fmovel	fpcr,d0		| d0 gets caller's modes.
	fmovel	#0,fpcr		| Set default modes. 	
	flogns	a0@,fp0		| fp0 gets log(x).
	fmuls	a1@,fp0		| fp0 gets y*log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	fmovel	d0,fpcr		| Restore caller's modes.
	btst	#9,d0	
	jne	1f		| Go the long way if inexact trap is intended.
	btst	#3,d1
	jeq	1f		| Go the long way if accrued inexact bit was off.
	fmovel	fpsr,d0
	andl	#0x07000000,d0	| Get Z/I/NaN condition code bits.
	jeq	2f		| Go the long way if result is zero, infinity, or NaN.
1:
	fmovel	d1,fpsr		| Restore caller's status.
	movl	a1,sp@- 	| Push parameters for _r_pow_.
	movl	a0,sp@-
	jsr	_r_pow_		| Compute carefully.
	fmoves	d0,fp0
	movl	sp@+,a0		| Clear parameters for _r_pow_.
	movl	sp@+,a0
2:
	fmoves	fp0,d0
	.end

	.inline _pow_dd,8	| double _pow_dd( double *x, *y )
	movl	sp@+,a0		| a0 gets address of x.
	movl	sp@+,a1		| a1 gets address of y.
	fmovel	fpsr,d1		| d1 gets caller's status.
	fmovel	fpcr,d0		| d0 gets caller's modes.
	fmovel	#0,fpcr		| Set default modes. 	
	flognd	a0@,fp0		| fp0 gets log(x).
	fmuld	a1@,fp0		| fp0 gets y*log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	fmovel	d0,fpcr		| Restore caller's modes.
	btst	#9,d0	
	jne	1f		| Go the long way if inexact trap is intended.
	btst	#3,d1
	jeq	1f		| Go the long way if accrued inexact bit was off.
	fmovel	fpsr,d0
	andl	#0x07000000,d0	| Get Z/I/NaN condition code bits.
	jeq	2f		| Go the long way if result is zero, infinity, or NaN.
1:
	fmovel	d1,fpsr		| Restore caller's status.
	movl	a1@(4),sp@-	| Push parameters for _pow.
	movl	a1@,sp@-
	movl	a0@(4),sp@-
	movl	a0@,sp@-
	jsr	_pow		| Compute carefully.
	movl	sp@+,a0		| Clear parameters for _pow.
	movl	sp@+,a0
	movl	d0,sp@
	movl	d1,sp@(4)
	fmoved	sp@+,fp0
2:
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline _d_pow_,8	| double _pow_dd( double *x, *y )
	movl	sp@+,a0		| a0 gets address of x.
	movl	sp@+,a1		| a1 gets address of y.
	fmovel	fpsr,d1		| d1 gets caller's status.
	fmovel	fpcr,d0		| d0 gets caller's modes.
	fmovel	#0,fpcr		| Set default modes. 	
	flognd	a0@,fp0		| fp0 gets log(x).
	fmuld	a1@,fp0		| fp0 gets y*log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	fmovel	d0,fpcr		| Restore caller's modes.
	btst	#9,d0	
	jne	1f		| Go the long way if inexact trap is intended.
	btst	#3,d1
	jeq	1f		| Go the long way if accrued inexact bit was off.
	fmovel	fpsr,d0
	andl	#0x07000000,d0	| Get Z/I/NaN condition code bits.
	jeq	2f		| Go the long way if result is zero, infinity, or NaN.
1:
	fmovel	d1,fpsr		| Restore caller's status.
	movl	a1@(4),sp@-	| Push parameters for _pow.
	movl	a1@,sp@-
	movl	a0@(4),sp@-
	movl	a0@,sp@-
	jsr	_pow		| Compute carefully.
	movl	sp@+,a0		| Clear parameters for _pow.
	movl	sp@+,a0
	movl	d0,sp@
	movl	d1,sp@(4)
	fmoved	sp@+,fp0
2:
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline _pow,16		| double _pow( double x, y )
	fmoved	sp@+,fp1	| fp1 gets x.
	fmovel	fpsr,d1		| d1 gets caller's status.
	fmovel	fpcr,d0		| d0 gets caller's modes.
	fmovel	#0,fpcr		| Set default modes. 	
	flognx	fp1,fp0		| fp0 gets log(x).
	fmuld	sp@,fp0		| fp0 gets y*log(x).
	fetoxx	fp0,fp0		| fp0 gets x**y.
	fmovel	d0,fpcr		| Restore caller's modes.
	btst	#9,d0	
	jne	1f		| Go the long way if inexact trap is intended.
	btst	#3,d1
	jeq	1f		| Go the long way if accrued inexact bit was off.
	fmovel	fpsr,d0
	andl	#0x07000000,d0	| Get Z/I/NaN condition code bits.
	jeq	2f		| Go the long way if result is zero, infinity, or NaN.
1:
	fmovel	d1,fpsr		| Restore caller's status.
	fmoved	fp1,sp@-	| Push parameters for _pow.
	jsr	_pow		| Compute carefully.
	movl	sp@+,a0		| Clear parameters for _pow.
	movl	sp@+,a0
	movl	d0,sp@
	movl	d1,sp@(4)
	fmoved	sp@,fp0
2:
	fmoved	fp0,sp@
	movl	sp@+,d0
	movl	sp@+,d1
	.end

/*	-f68881 templates for documented libm functions		*/

	.inline	_acos,8
	facosd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_aint,8
	fintrzd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline _anint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Manintd
	.end

	.inline	_asin,8
	fasind	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_atan,8
	fatand	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_atanh,8
	fatanhd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_atanh_,4
	movl	sp@+,a0
	fatanhs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_ceil,8
	fmovel	fpcr,d1
	movl	d1,d0
	orb	#0x30,d0	| Turn on round to +inf.
	fmovel	d0,fpcr
	fintd	sp@+,fp0
	fmoved	fp0,sp@-
	fmovel	d1,fpcr
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_floor,8
	fmovel	fpcr,d1
	movl	d1,d0
	andb	#0xcf,d0
	orb	#0x20,d0	| Turn on round to -inf.
	fmovel	d0,fpcr
	fintd	sp@+,fp0
	fmoved	fp0,sp@-
	fmovel	d1,fpcr
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_d_ceil_,4
	movl	sp@+,a0
	fmovel	fpcr,d1
	movl	d1,d0
	orb	#0x30,d0	| Turn on round to +inf.
	fmovel	d0,fpcr
	fintd	a0@,fp0
	fmoved	fp0,sp@-
	fmovel	d1,fpcr
	movl	sp@+,d0
	movl	sp@+,d1
	.end
	
	.inline	_d_floor_,4
	movl	sp@+,a0
	fmovel	fpcr,d1
	movl	d1,d0
	andb	#0xcf,d0
	orb	#0x20,d0	| Turn on round to -inf.
	fmovel	d0,fpcr
	fintd	a0@,fp0
	fmoved	fp0,sp@-
	fmovel	d1,fpcr
	movl	sp@+,d0
	movl	sp@+,d1
	.end
	
	.inline	_r_ceil_,4
	movl	sp@+,a0
	fmovel	fpcr,d1
	movl	d1,d0
	orb	#0x30,d0	| Turn on round to +inf.
	fmovel	d0,fpcr
	fints	a0@,fp0
	fmoves	fp0,d0
	fmovel	d1,fpcr
	.end
	
	.inline	_r_floor_,4
	movl	sp@+,a0
	fmovel	fpcr,d1
	movl	d1,d0
	andb	#0xcf,d0
	orb	#0x20,d0	| Turn on round to -inf.
	fmovel	d0,fpcr
	fints	a0@,fp0
	fmoves	fp0,d0
	fmovel	d1,fpcr
	.end
	.inline	_cos,8
	fcosd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_cosh,8
	fcoshd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_exp,8
	fetoxd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_expm1,8
	fetoxm1d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_expm1_,4
	movl	sp@+,a0
	fetoxm1s	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_exp2,8
	ftwotoxd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_exp10,8
	ftentoxd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

        .inline _hypot,16
        fmoved	sp@+,fp0
        fmulx   fp0,fp0
	fmoved	sp@+,fp1
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movl	sp@+,d0
	movl	sp@+,d1
	.end

        .inline _d_hypot_,8
        movl	sp@+,a0
	fmoved	a0@,fp0
	movl	sp@+,a0
        fmulx   fp0,fp0
	fmoved	a0@,fp1
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movl	sp@+,d0
	movl	sp@+,d1
	.end
       
	.inline _r_hypot_,8
        movl	sp@+,a0
	fmoves	a0@,fp0
	movl	sp@+,a0
	fmoves	a0@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoves  fp0,d0
	.end

	.inline	_log,8
	flognd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_log1p,8
	flognp1d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_log1p_,4
	movl	sp@+,a0
	flognp1s	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_log2,8
	flog2d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_log2_,4
	movl	sp@+,a0
	flog2s	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_log10,8
	flog10d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline _nint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Mnintd
	.end

	.inline	_sin,8
	fsind	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_sinh,8
	fsinhd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_sqrt,8
	fsqrtd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_tan,8
	ftand	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_tanh,8
	ftanhd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_rint,8
	fintd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_d_rint_,4
	movl	sp@+,a0
	fintd	a0@,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_rint_,4
	movl	sp@+,a0
	fints	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_irint,8
	fintd	sp@+,fp0
	fmovel	fp0,d0
	.end

	.inline	_id_irint_,4
	movl	sp@+,a0
	fintd	a0@,fp0
	fmovel	fp0,d0
	.end

	.inline	_ir_irint_,4
	movl	sp@+,a0
	fints	a0@,fp0
	fmovel	fp0,d0
	.end

	.inline	_fmod,16
	fmoved	sp@+,fp0		| fp0 gets first argument
	fmodd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_remainder,16
	fmoved	sp@+,fp0		| fp0 gets first argument
	fremd	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_ldexp,12
	fmoved	sp@+,fp0
	fscalel	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_scalbn,12
	fmoved	sp@+,fp0
	fscalel	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_d_scalbn_,8
	movl	sp@+,a0
	fmoved	a0@,fp0
	movl	sp@+,a0
	fscalel	a0@,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_r_scalbn_,8
	movl	sp@+,a0
	fmoves	a0@,fp0
	movl	sp@+,a0
	fscalel	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_d_fmod_,8
	movl	sp@+,a0
	fmoved	a0@,fp0
	movl	sp@+,a0
	fmodd	a0@,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_d_remainder_,8
	movl	sp@+,a0
	fmoved	a0@,fp0
	movl	sp@+,a0
	fremd	a0@,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

	.inline	_ir_nint_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Mnints
	.end

	.inline	_r_acos_,4
	movl	sp@+,a0
	facoss	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_aint_,4
	movl	sp@+,a0
	fintrzs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_anint_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Manints
	.end

	.inline	_r_asin_,4
	movl	sp@+,a0
	fasins	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_atan_,4
	movl	sp@+,a0
	fatans	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_cos_,4
	movl	sp@+,a0
	fcoss	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_cosh_,4
	movl	sp@+,a0
	fcoshs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_exp10_,4
	movl	sp@+,a0
	ftentoxs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_exp2_,4
	movl	sp@+,a0
	ftwotoxs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_exp_,4
	movl	sp@+,a0
	fetoxs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_fmod_,8
	movl	sp@+,a0
	fmoves	a0@,fp0
	movl	sp@+,a0
	fmods	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_remainder_,8
	movl	sp@+,a0
	fmoves	a0@,fp0
	movl	sp@+,a0
	frems	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_log10_,4
	movl	sp@+,a0
	flog10s	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_log_,4
	movl	sp@+,a0
	flogns	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_sin_,4
	movl	sp@+,a0
	fsins	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_sinh_,4
	movl	sp@+,a0
	fsinhs	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_sqrt_,4
	movl	sp@+,a0
	fsqrts	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_tan_,4
	movl	sp@+,a0
	ftans	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	_r_tanh_,4
	movl	sp@+,a0
	ftanhs	a0@,fp0
	fmoves	fp0,d0
	.end

/*	-f68881 templates for libF77 entry points	*/

	.inline	__Fz_add,12
	movl	sp@+,d0		| d0 = c.
	movl	sp@+,a0		| a0 = a.
	movl	sp@+,a1		| a1 = b.
	fmoved	a0@+,fp0	| fp0 = a->dreal + b->dreal.
	faddd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag + b->dimag.
	faddd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0.
	fmoved	fp1,a0@		| c->dimag = fp1.
	.end

	.inline	__Fz_conv_c,4
	movl	sp@+,a0
	fmoved	a0@+,fp0
	fmoved	a0@+,fp1
	fmoves	fp0,d0
	fmoves	fp1,d1
	.end

	.inline	__Fz_conv_f,4
	movl    sp@+,a0   
	fmoved	a0@,fp0
	fmoves	fp0,d0
	.end

	.inline	__Fz_conv_i,4
	movl    sp@+,a0   
	fintrzd	a0@,fp0
	fmovel	fp0,d0
	.end

	.inline	__Fz_minus,12
	movl	sp@+,d0		| d0 = c.
	movl	sp@+,a0		| a0 = a.
	movl	sp@+,a1		| a1 = b.
	fmoved	a0@+,fp0	| fp0 = a->dreal - b->dreal.
	fsubd	a1@+,fp0
	fmoved	a0@,fp1		| fp1 = a->dimag - b->dimag.
	fsubd	a1@,fp1
	movl	d0,a0
	fmoved	fp0,a0@+	| c->dreal = fp0.
	fmoved	fp1,a0@		| c->dimag = fp1.
	.end

        .inline __Fz_mult,12
        movl    sp@+,d0         | d0 = c.
        movl    sp@+,a1         | a0 = a.
        movl    sp@+,a0         | a1 = b.
        fmoved  a1@,fp0         | fp0 = (a->dreal *  b->dreal).
        fmuld   a0@,fp0
        fmoved  a1@(8),fp1      | fp1 = (a->dimag *  b->dimag).
        fmuld   a0@(8),fp1
        fsubx   fp1,fp0         | c->dreal = fp0 - fp1.
        exg     d0,a0
        fmoved  fp0,a0@
        exg     d0,a0
        fmoved  a1@,fp0         | fp0 = (a->dreal *  b->dimag).
        fmuld   a0@(8),fp0
        fmoved  a1@(8),fp1      | fp1 = (a->dimag *  b->dreal).
        fmuld   a0@,fp1
        faddx   fp1,fp0         | c->dimag = fp0 + fp1.
        movl    d0,a0
        fmoved  fp0,a0@(8)
        .end
 
	.inline	_c_abs,4
	movl	sp@+,a0
	fmoves	a0@+,fp0
	fmulx	fp0,fp0
	fmoves	a0@,fp1
	fmulx	fp1,fp1
	faddx	fp1,fp0
	fsqrtx	fp0,fp0
	fmoves	fp0,d0
	.end

	.inline	_d_dim,8
        movl   sp@+,a0
        fmoved  a0@,fp0
        movl   sp@+,a0
        fsubd   a0@,fp0
	fjugt	1f		| Accept difference if ? or >0.
	clrl	d0		| Return +0 if difference is <= 0.
	clrl	d1
	jra	2f
1:
        fmoved  fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
2:
	.end

	.inline	_d_mod,8
        movl   sp@+,a0
        fmoved  a0@,fp0
        movl   sp@+,a0
        fmodd   a0@,fp0
        fmoved  fp0,sp@-
        movl   sp@+,d0
        movl   sp@+,d1
 	.end

        .inline _i_dnnt,4
        movl   sp@+,a0
        movl   a0@+,d0
        movl   a0@,d1
        jsr     Mnintd
        .end

        .inline _id_nint_,4
        movl   sp@+,a0
        movl   a0@+,d0
        movl   a0@,d1
        jsr     Mnintd
        .end

        .inline _i_nint,4
        movl   sp@+,a0
        movl   a0@,d0
        jsr     Mnints
        .end

        .inline _r_atn2,8
        movl    sp@+,a0         | a0 gets address of first argument.
        movl    sp@+,a1         | a1 gets address of second argument.
        fmoves  a1@,fp0
        fmoved  fp0,sp@-
        fmoves  a0@,fp1
        fmoved  fp1,sp@-
        jsr     _atan2
        movl    sp@+,a0         | Pop junk.
        movl    sp@+,a0         | Pop junk.
        movl    sp@+,a0         | Pop junk.
        movl    sp@+,a0         | Pop junk.
        movl    d1,sp@-
        movl    d0,sp@-
        fmoved  sp@+,fp0
        fmoves  fp0,d0
        .end
 
	.inline	_r_dim,8
        movl   sp@+,a0
        fmoves 	a0@,fp0
        movl   sp@+,a0
        fsubs  	a0@,fp0
	fjugt	1f
	clrl	d0		| Return 0 if diff <=0.
	jra	2f
1:
        fmoves 	fp0,d0		| Accept difference if ? or >0.
2:
	.end

	.inline	_r_mod,8
        movl   sp@+,a0
        fmoves  a0@,fp0
        movl   sp@+,a0
        fmods   a0@,fp0
        fmoves  fp0,d0
 	.end

	.inline	_z_abs,4
	movl	sp@+,a0
	fmoved	a0@+,fp0
	fmoved	a0@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

/*	templates for undocumented libm functions	*/

#ifndef FFPA

/* 
 * _swapRD(rd) 	exchanges rd with the current rounding direction.
 * _swapRP(rp) 	exchanges rp with the current rounding precision.
 * _swapTE(ex)  exchanges ex with the current exception trap enable bits.
 * _swapEX(ex) 	exchanges ex with the current accrued exception. 
 *		Here ex is a five bit value where each bit (cf. enum 
 *		fp_exception_type in <sys/ieeefloatingpoint.h>) corresponds 
 *		to either an exception-occurred accrued status flag or an
 *		exception trap enable flag (0 off,1 on):
 * 			bit 0 :  inexact flag
 * 			bit 1 :  division by zero flag
 * 			bit 2 :  underflow flag
 * 			bit 3 :  overflow flag
 * 			bit 4 :  invalid flag
 */

	.inline	__swapRD,4
	movl	sp@+,d1
	movl	d1,_fp_direction
	lsll	#4,d1
	movl	d2,a0
	fmovel	fpcr,d2
	movl	d2,d0
	andl	#0xFFFFFFCF,d0
	orl	d1,d0
	fmovel	d0,fpcr
	movl	d2,d0
	lsrl	#4,d0
	moveq	#3,d2
	andl	d2,d0
	movl	a0,d2
	.end

	.inline	__swapRP,4
	movl	sp@+,d1
	movl	d1,_fp_precision
	lsll	#6,d1
	movl	d2,a0
	fmovel	fpcr,d2
	movl	d2,d0
	andl	#0xFFFFFF3F,d0
	orl	d1,d0
	fmovel	d0,fpcr
	movl	d2,d0
	lsrl	#6,d0
	moveq	#3,d2
	andl	d2,d0
	movl	a0,d2
	.end

	.inline	__swapEX,4
	movl	sp@+,d1
	movl	d1,_fp_accrued_exceptions
	movl	d2,a0
	fmovel	fpsr,d2
	movl	d2,d0
	andl	#0xFFFFFF07,d0
	lsll	#3,d1
	orl	d1,d0
	fmovel	d0,fpsr
	movl	d2,d0
	lsrl	#3,d0
	moveq	#31,d2
	andl	d2,d0
	movl	a0,d2
	.end

	.inline	__swapTE,4
        movl    sp@+,d1
        movl	d6,a0
	movl	d7,a1
        moveq   #9,d7
        lsll    d7,d1
        btst    d7,d1
        jeq     1f
        orw     #0x100,d1
1:
        btst    #13,d1
        jeq     1f
        orw     #0xc000,d1
1:
        fmovel  fpcr,d6
        movl    d6,d0
        andl    #0xFFFFC1FF,d0
        orl     d1,d0
        fmovel  d0,fpcr
        movl    d6,d0
        lsrl    d7,d0
        moveq   #31,d1
        andl    d1,d0
        movl	a0,d6
	movl	a1,d7
	.end
#endif

/*	templates for obsolete libm functions	*/

        .inline Mlog10s,0
        flog10s	d0,fp0
	fmoves	fp0,d0
        .end

        .inline Mlog10d,0
        movl    d1,sp@-
        movl    d0,sp@-
        flog10d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
        .end


	.inline	__Fz_eq,8
	movl	sp@+,a0		| a0 -> x
	movl	sp@+,a1		| a1 -> y
	fmoved	a0@+,fp0	| Re x
	fcmpd	a1@+,fp0	| Re y
	clrl	d0		| x == y
	fjneq	1f
	fmoved	a0@,fp0		| Im x
	fcmpd	a1@,fp0		| Im y
	fjneq	1f
	moveq	#1,d0		| x != y
1:
	.end

	.inline	__Fz_ne,8
	movl	sp@+,a0		| a0 -> x
	movl	sp@+,a1		| a1 -> y
	fmoved	a0@+,fp0	| Re x
	fcmpd	a1@+,fp0	| Re y
	moveq	#1,d0		| x != y
	fjneq	1f
	fmoved	a0@,fp0		| Im x
	fcmpd	a1@,fp0		| Im y
	fjneq	1f
	clrl	d0		| x == y
1:
	.end

        .inline _d_anint_,4
        movl   sp@+,a0
        movl   a0@+,d0
        movl   a0@,d1
        jsr     Manintd
        .end

	.inline	_sincos,16
	fsincosd  sp@+,fp0:fp1
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fmoved fp1,a0@		| Store sin.
	fmoved fp0,a1@		| Store cos.
	.end

	.inline	_d_sincos_,12
	movl	sp@+,a0
	fsincosd  a0@,fp0:fp1
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fmoved fp0,a1@		| Store cos.
	fmoved fp1,a0@		| Store sin.
	.end

	.inline	_r_sincos_,12
	movl	sp@+,a0
	fsincoss  a0@,fp0:fp1
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fmoves fp0,a1@		| Store cos.
	fmoves fp1,a0@		| Store sin.
	.end

#define D_81(F,G) \
	.inline	_d_/**/F/**/_,4 ; \
	movl	sp@+,a0 ; \
	f/**/G/**/d	a0@,fp0 ; \
	fmoved	fp0,sp@- ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	.end

	D_81(cos,cos)
	D_81(sin,sin)
	D_81(tan,tan)
	D_81(cosh,cosh)
	D_81(sinh,sinh)
	D_81(tanh,tanh)
	D_81(acos,acos)
	D_81(asin,asin)
	D_81(atan,atan)
	D_81(atanh,atanh)
	D_81(aint,intrz)
	D_81(exp,etox)
	D_81(expm1,etoxm1)
	D_81(exp10,tentox)
	D_81(exp2,twotox)
	D_81(log,logn)
	D_81(log1p,lognp1)
	D_81(log10,log10)
	D_81(log2,log2)
	D_81(sqrt,sqrt)

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_mult(c, a, b)
| complex *c, *a, *b;		/* c = a * b */
| {
	.inline	__Fc_mult,12
	movl	sp@+,d0
	movl	sp@+,a0
	movl	sp@+,a1
|    21		c->real = (a->real *  b->real) - (a->imag *  b->imag);
	fmoves	a0@,fp0
	fmuls	a1@,fp0
	fmoves	a0@(4),fp1
	fmuls	a1@(4),fp1
	fsubx	fp1,fp0
	exg	d0,a0
	fmoves	fp0,a0@
	exg	d0,a0
|    22		c->imag = (a->real *  b->imag) + (a->imag *  b->real);
	fmoves	a0@,fp0
	fmuls	a1@(4),fp0
	fmoves	a0@(4),fp1
	fmuls	a1@,fp1
	faddx	fp1,fp0
	movl	d0,a0
	fmoves	fp0,a0@(4)
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_minus(c, a, b)
| complex *c, *a, *b;		/* c = a - b */
| {
	.inline	__Fc_minus,12
|    30	 	c->real = a->real - b->real;
	movl	sp@+,d0
	movl	sp@+,a0
	movl	sp@+,a1
	fmoves	a0@+,fp0
	fsubs	a1@+,fp0
|    31	  	c->imag = a->imag - b->imag;
	fmoves	a0@,fp1
	fsubs	a1@,fp1
	movl	d0,a0
	fmoves	fp0,a0@+
	fmoves	fp1,a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_add(c, a, b)
| complex *c, *a, *b;		/* c = a + b */
| {
	.inline	__Fc_add,12
|    30	 	c->real = a->real - b->real;
	movl	sp@+,d0
	movl	sp@+,a0
	movl	sp@+,a1
	fmoves	a0@+,fp0
	fadds	a1@+,fp0
|    31	  	c->imag = a->imag - b->imag;
	fmoves	a0@,fp1
	fadds	a1@,fp1
	movl	d0,a0
	fmoves	fp0,a0@+
	fmoves	fp1,a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_neg(c, a)
| complex *c, *a;		/* c = -a */
| {
	.inline	__Fc_neg,8
|    48	  	c->real = - a->real;
	movl	sp@+,a0
	movl	sp@+,a1
	movl	a1@+,d0
	bchg	#31,d0
	movl	d0,a0@+
|    49	  	c->imag = - a->imag;
	movl	a1@,d0
	bchg	#31,d0
	movl	d0,a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Ff_conv_c(c, x)	/* convert float to complex */
| complex *c;
| FLOATPARAMETER x;
| {
	.inline	__Ff_conv_c,8
|    59		c->real = FLOATPARAMETERVALUE(x);
	movl	sp@+,a0
	movl	sp@+,a0@+
|    60		c->imag = 0.0;
	clrl	a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| FLOATFUNCTIONTYPE
| _Fc_conv_f( c )		/* Convert complex to float */
| complex *c;
| {
	.inline	__Fc_conv_f,4
|    69		RETURNFLOAT(c->real);
	movl	sp@+,a0
	movl	a0@,d0
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| int
| _Fc_conv_i (c)			/* Convert complex to int */
| complex *c;
| {
	.inline	__Fc_conv_i,4
|    78		return (int)c->real;
	movl	sp@+,a0
	fmoves	a0@,fp0
	fmovel	fp0,d0
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void     
| _Fi_conv_c (c,i)		/* Convert int to complex */
| complex *c;
| int i;   
| {        
	.inline	__Fi_conv_c,8
|    88		c->real = (float)i;
	movl	sp@+,a0
	movl	sp@+,d0
	fmovel	d0,fp0
	fmoves	fp0,a0@+
|    89		c->imag = 0.0;
	clrl	a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| double
| _Fc_conv_d ( c )		/* Convert complex to double */
| complex *c;
| {
	.inline	__Fc_conv_d,4
|    98		return (double)c->real;
	movl	sp@+,a0
	fmoves	a0@,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fd_conv_c (c, x)		/* Convert double to complex */
| complex *c;
| double x;
| {
	.inline	__Fd_conv_c,8
|   109		c->real = (float)(x);
	movl	sp@+,a0
	fmoved	sp@+,fp0
	fmoves	fp0,a0@+
|   110		c->imag = 0.0;
	clrl	a0@
	.end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_conv_z( result, c )	/* Convert complex to double complex */
| dcomplex *result;
| complex *c;
| {
	.inline	__Fc_conv_z,8
|   120		result->dreal = (double)c->real;
	movl	sp@+,a0
	movl	sp@+,a1
	fmoves	a1@+,fp0
	fmoved	fp0,a0@+
|   121		result->dimag = (double)c->imag;
	fmoves	a1@,fp0
	fmoved	fp0,a0@
	.end
