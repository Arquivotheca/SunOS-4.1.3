/*	@(#)dependent.il 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

|	Sun-3 FPA+ templates for new sqrt and complex instructions.
|	Comment out if using original Sun-3 FPA.

        .inline _hypot,16
        fpmoved sp@+,fpa0
        fphypotd sp@+,fpa0,fpa1
        fpmoved	fpa1,d0:d1
        .end

        .inline _d_hypot_,8
        movl    sp@+,a0
        fpmoved a0@,fpa0
        movl    sp@+,a1
        fphypotd a1@,fpa0,fpa1
        fpmoved	fpa1,d0:d1
        .end

        .inline _r_hypot_,8
        movl    sp@+,a0
        fpmoves a0@,fpa0
        movl    sp@+,a1
        fphypots a1@,fpa0,fpa1
        fpmoves	fpa1,d0
        .end
 
        .inline _z_abs,4
        movl    sp@+,a0
        fpmoved a0@+,fpa0
        fphypotd a0@,fpa0,fpa1
        fpmoved	fpa1,d0:d1
        .end
    
        .inline _c_abs,4
        movl    sp@+,a0
 	fpcabsd	a0@,fpa1
	fpmoves	fpa1,d0
        .end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_mult(c, a, b)
| complex *c, *a, *b
       
        .inline __Fc_mult,12
        movl    sp@+,a1			| c
        movl    sp@+,a0			| a
	fpmoved a0@,fpa1		| fpa1 gets a
        movl    sp@+,a0			| b
 	fpcmuld a0@,fpa1,fpa2		| fpa0 gets a*b
	fpmoved	fpa2,a1@		| c gets product
        .end
 
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_minus(c, a, b)
| complex *c, *a, *b
        
        .inline __Fc_minus,12
|    30         c->real = a->real - b->real
        movl    sp@+,a1			| c
        movl    sp@+,a0			| a
	fpmoved a0@,fpa0		| fpa0 gets a
        movl    sp@+,a0			| b
 	fpcsubd a0@,fpa0,fpa2		| fpa0 gets a-b
	fpmoved	fpa2,a1@		| c gets product
        .end

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_add(c, a, b)
| complex *c, *a, *b
        
| {
        .inline __Fc_add,12
|    30         c->real = a->real + b->real
        movl    sp@+,a1			| c
        movl    sp@+,a0			| a
	fpmoved a0@,fpa1		| fpa0 gets a
        movl    sp@+,a0			| b
 	fpcaddd a0@,fpa1,fpa2		| fpa0 gets a+b
	fpmoved	fpa2,a1@		| c gets product
        .end
 
|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_div(c, a, b)
| complex *c, *a, *b
        
        .inline __Fc_div,12
|    30         c->real = a->real / b->real
        movl    sp@+,a1			| c
        movl    sp@+,a0			| a
	fpmoved a0@,fpa1		| fpa0 gets a
        movl    sp@+,a0			| b
 	fpcdivd a0@,fpa1,fpa2		| fpa0 gets a/b
	fpmoved	fpa2,a1@		| c gets product
        .end
 
        .inline Wsqrts,0
        fpsqrts  d0,fpa0
        fpmoves  fpa0,d0
        .end

        .inline Wsqrtd,0
        fpsqrtd  d0:d1,fpa0
        fpmoved  fpa0,d0:d1
        .end
 
        .inline _sqrt,8
        fpsqrtd  sp@+,fpa0
        fpmoved  fpa0,d0:d1
        .end
 
        .inline _d_sqrt_,4
        movl    sp@+,a0
        fpsqrtd  a0@,fpa0
        fpmoved  fpa0,d0:d1
        .end
 
        .inline _Fsqrt,4
        fpsqrts  sp@+,fpa0
        fpmoves  fpa0,d0
        .end

        .inline _r_sqrt_,4
        movl    sp@+,a0
        fpsqrts  a0@,fpa0
        fpmoves  fpa0,d0
        .end

        .inline _d_sqrt,4
        movl    sp@+,a0
        fpsqrtd  a0@,fpa0
        fpmoved  fpa0,d0:d1
        .end
 
        .inline _r_sqrt,4
        movl    sp@+,a0
        fpsqrts  a0@,fpa0
        fpmoves  fpa0,d0
        .end

|	End new FPA+ instruction templates.	*/

/*	-ffpa templates for documented libm functions		*/

#define FPA_LIBM(f,g)  /* double _f ( double x ) ; */ \
	.inline _/**/f,8 ; \
	fpmoved sp@+,fpa0 ; \
	fp/**/g/**/d fpa0,fpa0 ; \
	fpmoved fpa0,d0:d1 ; \
	.end ; \
	.inline _d_/**/f/**/_,4 ;  /* double _d_f_ ( double *x ) ; */ \
	movl	sp@+,a0 ; \
	fpmoved	a0@,fpa0 ; \
	fp/**/g/**/d fpa0,fpa0 ; \
	fpmoved fpa0,d0:d1 ; \
	.end ; \
	.inline _r_/**/f/**/_,4 ; /* float _r_f_ (float *x ) ; */ \
	movl	sp@+,a0 ; \
	fpmoves	a0@,fpa0 ; \
	fp/**/g/**/s fpa0,fpa0 ; \
	fpmoves fpa0,d0 ; \
	.end ; \


	FPA_LIBM(sin,sin)
	FPA_LIBM(cos,cos)
	FPA_LIBM(atan,atan)
	FPA_LIBM(exp,etox)
	FPA_LIBM(expm1,etoxm1)
	FPA_LIBM(log,logn)
	FPA_LIBM(log1p,lognp1)

	.inline _anint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Wanintd
	.end

	.inline _d_anint_,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	jsr	Wanintd
	.end

	.inline _r_anint_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Wanints
	.end

	.inline _nint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Wnintd
	.end

	.inline _id_nint_,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	jsr	Wnintd
	.end

	.inline _ir_nint_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Wnints
	.end

	.inline	_irint,8
	fpmove	fpamode,d1
	andl	#0xf,d1		| Clear junk bits.
	movl	d1,d0
	andb	#0xd,d0		| Clear integer convert toward zero mode.
	fpmove	d0,fpamode
	fpdtol	sp@+,fpa0
	fpmoves	fpa0,d0
	fpmove	d1,fpamode	| Restore FPA mode.
	.end

	.inline	_id_irint_,4
	movl	sp@+,a0
	fpmove	fpamode,d1
	andl	#0xf,d1		| Clear junk bits.
	movl	d1,d0
	andb	#0xd,d0		| Clear integer convert toward zero mode.
	fpmove	d0,fpamode
	fpdtol	a0@,fpa0
	fpmoves	fpa0,d0
	fpmove	d1,fpamode	| Restore FPA mode.
	.end

	.inline	_ir_irint_,4
	movl	sp@+,a0
	fpmove	fpamode,d1
	andl	#0xf,d1		| Clear junk bits.
	movl	d1,d0
	andb	#0xd,d0		| Clear integer convert toward zero mode.
	fpmove	d0,fpamode
	fpstol	a0@,fpa0
	fpmoves	fpa0,d0
	fpmove	d1,fpamode	| Restore FPA mode.
	.end

	.inline _ceil,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Wceild
	.end

	.inline _floor,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Wfloord
	.end

	.inline _d_floor_,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	jsr	Wfloord
	.end

	.inline _d_ceil_,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	jsr	Wceild
	.end

	.inline _r_floor_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Wfloors
	.end

	.inline _r_ceil_,4
	movl	sp@+,a0
	movl	a0@,d0
	jsr	Wceils
	.end

        .inline _hypot,16
        fmoved	sp@+,fp0
	fmoved	sp@+,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movl	sp@+,d0
	movl	sp@+,d1
	.end

        .inline _d_hypot_,8
	movl	sp@+,a0
	movl	sp@+,a1
        fmoved	a0@,fp0
	fmoved	a1@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movl	sp@+,d0
	movl	sp@+,d1
	.end

        .inline _z_abs,4
        movl    sp@+,a0
        fmoved  a0@+,fp0
        fmoved  a0@,fp1
        fmulx   fp0,fp0
        fmulx   fp1,fp1
        faddx   fp1,fp0
        fsqrtx  fp0,fp0
        fmoved  fp0,sp@-
        movel   sp@+,d0
        movel   sp@+,d1
        .end
        
	.inline _i_dnnt,4
        movl   sp@+,a0
        movl   a0@+,d0
        movl   a0@,d1
        jsr     Wnintd
        .end

        .inline _i_nint,4
        movl   sp@+,a0
        movl   a0@,d0
        jsr     Wnints
        .end

/*	templates for obsolete libm functions	*/

        .inline Wlog10s,0
        flog10s	d0,fp0
	fmoves	fp0,d0
        .end

        .inline Wlog10d,0
        movl    d1,sp@-
        movl    d0,sp@-
        flog10d	sp@+,fp0
	fmoved	fp0,sp@-
	movl	sp@+,d0
	movl	sp@+,d1
        .end


	.inline	_d_dim,8
	movel	sp@+,a0
	fpmoved	a0@,fpa0
	movel	sp@+,a0
	fpsubd	a0@,fpa0
	fpmoved	fpa0,d0:d1
	tstl	d0
	jpl	1f		| Branch if positive sign bit.
	cmpl	#0xfff00000,d0
	jlt	2f		| Branch if finite.
	jgt	1f		| Branch if nan.
	tstl	d1
	jne	1f		| Branch if nan.
2:
	clrl	d0
	clrl	d1
1:
	.end

	.inline	_r_dim,8
	movel	sp@+,a0
	fpmoves	a0@,fpa0
	movel	sp@+,a0
	fpsubs	a0@,fpa0
	fpmoves	fpa0,d0
	tstl	d0
	jpl	1f		| Branch if positive sign.
	cmpl	#0xff800000,d0
	jgt	1f		| Branch if nan.
	clrl	d0
1:
	.end


	.inline	__Fz_mult,12	
	movl	sp@+,d0		| d0 = c.
	movl	sp@+,a1		| a0 = a.
	movl	sp@+,a0		| a1 = b.
	fpmoved	a1@,fpa0	| fp0 = (a->dreal *  b->dreal).
	fpmuld	a0@,fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dimag).
	fpmuld	a0@(8),fpa1
	fpsubd	fpa1,fpa0	| c->dreal = fp0 - fp1.
	exg	d0,a0
	fpmoved	fpa0,a0@
	exg	d0,a0
	fpmoved	a1@,fpa0	| fp0 = (a->dreal *  b->dimag).
	fpmuld	a0@(8),fpa0
	fpmoved	a1@(8),fpa1	| fp1 = (a->dimag *  b->dreal).
	fpmuld	a0@,fpa1
	fpaddd	fpa1,fpa0	| c->dimag = fp0 + fp1.
	movl	d0,a0
	fpmoved	fpa0,a0@(8)
	.end

	.inline	__Fz_minus,12
	movl	sp@+,d0		| d0 = c.
	movl	sp@+,a0		| a0 = a.
	movl	sp@+,a1		| a1 = b.
	fpmoved	a0@+,fpa0	| fp0 = a->dreal - b->dreal.
	fpsubd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag - b->dimag.
	fpsubd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0.
	fpmoved	fpa1,a0@		| c->dimag = fp1.
	.end

	.inline	__Fz_add,12
	movl	sp@+,d0		| d0 = c.
	movl	sp@+,a0		| a0 = a.
	movl	sp@+,a1		| a1 = b.
	fpmoved	a0@+,fpa0	| fp0 = a->dreal + b->dreal.
	fpaddd	a1@+,fpa0
	fpmoved	a0@,fpa1		| fp1 = a->dimag + b->dimag.
	fpaddd	a1@,fpa1
	movl	d0,a0
	fpmoved	fpa0,a0@+	| c->dreal = fp0.
	fpmoved	fpa1,a0@		| c->dimag = fp1.
	.end

	.inline	__Ff_conv_z,8
	movl	sp@+,a0
	fpstod	sp@+,fpa0
	fpmoved	fpa0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	__Fz_conv_f,4
	movl    sp@+,a0   
	fpdtos	a0@,fpa0
	fpmoves	fpa0,d0
	.end

	.inline	__Fz_conv_i,4
	movl    sp@+,a0   
	fpdtol	a0@,fpa0
	fpmovel	fpa0,d0
	.end

	.inline	__Fi_conv_z,8
	movl	sp@+,a0
	fpltod	sp@+,fpa0
	fpmoved	fpa0,a0@+
	clrl    a0@+
	clrl    a0@
	.end

	.inline	__Fz_conv_c,4
	movl	sp@+,a0
	fpdtos	a0@+,fpa0
	fpdtos	a0@+,fpa1
	fpmoves	fpa0,d0
	fpmoves	fpa1,d1
	.end

	.inline	Waints,0
	fintrzs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wsqrts,0
	fsqrts	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wsqrtd,0
	movl	d1,sp@-
	movl	d0,sp@-
        fmovel  fpcr,d0                 | Fetch old mode.
        movel   d0,d1                   | d1 saves old fp mode.
        andb    #0x3f,d0                | Clear rounding precision.
        orb     #0x80,d0                | Set double precision.
        fmovel  d0,fpcr                 | Set new mode.
        fsqrtd  sp@+,fp0
	fmoved	fp0,sp@-
        fmovel  d1,fpcr
	movl	sp@+,d0
	movl	sp@+,d1
	.end

        .inline _sqrt,8
        fmovel  fpcr,d0                 | Fetch old mode.
        movel   d0,d1                   | d1 saves old fp mode.
        andb    #0x3f,d0                | Clear rounding precision.
        orb     #0x80,d0                | Set double precision.
        fmovel  d0,fpcr                 | Set new mode.
        fsqrtd  sp@+,fp0
        fmoved  fp0,sp@-
        fmovel  d1,fpcr
        movl    sp@+,d0
        movl    sp@+,d1
        .end

        .inline _d_sqrt_,4
        movl    sp@+,a0
        fmovel  fpcr,d0                 | Fetch old mode.
        movel   d0,d1                   | d1 saves old fp mode.
        andb    #0x3f,d0                | Clear rounding precision.
        orb     #0x80,d0                | Set double precision.
        fmovel  d0,fpcr                 | Set new mode.
        fsqrtd  a0@,fp0
        fmoved  fp0,sp@-
        fmovel  d1,fpcr
        movl    sp@+,d0
        movl    sp@+,d1
        .end

	.inline	Wpow2s,0
	ftwotoxs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wpow10s,0
	ftentoxs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wtans,0
	ftans	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wasins,0
	fasins	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wacoss,0
	facoss	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wsinhs,0
	fsinhs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wcoshs,0
	fcoshs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wtanhs,0
	ftanhs	d0,fp0
	fmoves	fp0,d0
	.end

	.inline	Wlog10s,0
	flog10s	d0,fp0
	fmoves	fp0,d0
	.end

	.inline Waintd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fintrzd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wpow2d,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftwotoxd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wpow10d,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftentoxd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wlog10d,0
	movel	d1,sp@-
	movel	d0,sp@-
	flog10d	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wtand,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftand	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wacosd,0
	movel	d1,sp@-
	movel	d0,sp@-
	facosd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wasind,0
	movel	d1,sp@-
	movel	d0,sp@-
	fasind	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wsinhd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fsinhd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wcoshd,0
	movel	d1,sp@-
	movel	d0,sp@-
	fcoshd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline Wtanhd,0
	movel	d1,sp@-
	movel	d0,sp@-
	ftanhd	sp@+,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	.end

	.inline	_sincos,16
	fpmoved sp@+,fpa1		| Argument x.
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fpsincosd  fpa1,fpa0:fpa1
	fpmoved fpa0,a1@		| Store cos.
	fpmoved fpa1,a0@		| Store sin.
	.end

	.inline	_d_sincos_,12
	movl	sp@+,a0
	fpmoved a0@,fpa1		| Argument x.
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fpsincosd  fpa1,fpa0:fpa1
	fpmoved fpa0,a1@		| Store cos.
	fpmoved fpa1,a0@		| Store sin.
	.end

	.inline	_r_sincos_,12
	movl	sp@+,a0
	fpmoves a0@,fpa1		| Argument x.
	movl	sp@+,a0			| Address for sin.
	movl	sp@+,a1			| Address for cos.
	fpsincoss  fpa1,fpa0:fpa1
	fpmoves fpa0,a1@		| Store cos.
	fpmoves fpa1,a0@		| Store sin.
	.end

#define D_FPA(F,G) \
	.inline	_d_/**/F/**/_,4 ; \
	movl	sp@+,a0 ; \
	fp/**/G/**/d	a0@,fpa0 ; \
	fpmoved	fpa0,d0:d1 ; \
	.end

	D_FPA(cos,cos)
	D_FPA(sin,sin)
	D_FPA(atan,atan)
	D_FPA(exp,etox)
	D_FPA(expm1,etoxm1)
	D_FPA(log,logn)
	D_FPA(log1p,lognp1)

|- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
| void
| _Fc_mult(c, a, b)
| complex *c, *a, *b;		/* c = a * b */
| {
	.inline	__Fc_mult,12
	movl	sp@+,d0
	movl	sp@+,a0
	movl	sp@+,a1
|    21	  	c->real = (a->real *  b->real) - (a->imag *  b->imag);
	fpstod	a0@(4),fpa0
	fpstod	a1@(4),fpa1
	fpmuld	fpa1,fpa0		| fpa0 = a->imag * b->imag
	fpstod	a1@+,fpa2
	fpstod	a0@+,fpa3
	fpmrd	fpa3,fpa2,fpa0,fpa1	| fpa1 = (a->real*b->real) - fpa0
|    22	  	c->imag = (a->real *  b->imag) + (a->imag *  b->real);
	fpstod	a1@,fpa0
	fpmuld	fpa0,fpa3		| fpa3 = (a->real*b->imag)
	fpstod	a0@,fpa0
	fpmad	fpa0,fpa2,fpa3,fpa2	| fpa2 = a->imag*b->real + fpa3
	movl	d0,a0
	fpdtos	fpa1,fpa0
	fpdtos	fpa2,fpa3
	fpmoves	fpa0,a0@+
	fpmoves	fpa3,a0@
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
	fpmoves	a0@+,fpa2
	fpsubs	a1@+,fpa2
|    31	  	c->imag = a->imag - b->imag;
	fpmoves	a0@,fpa3
	fpsubs	a1@,fpa3
	movl	d0,a0
	fpmoves	fpa2,a0@+
	fpmoves	fpa3,a0@
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
	fpmoves	a0@+,fpa2
	fpadds	a1@+,fpa2
|    31	  	c->imag = a->imag - b->imag;
	fpmoves	a0@,fpa3
	fpadds	a1@,fpa3
	movl	d0,a0
	fpmoves	fpa2,a0@+
	fpmoves	fpa3,a0@
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
	fpstol	a0@,fpa2
	fpmovel	fpa2,d0
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
	fpltos	d0,fpa3
	fpmoves	fpa3,a0@+
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
	fpstod	a0@,fpa0
	fpmoved	fpa0,d0:d1
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
	fpmoved	sp@+,fpa0
	fpdtos	fpa0,fpa2
	fpmoves	fpa2,a0@+
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
	fpstod	a1@+,fpa0
	fpmoved	fpa0,a0@+
|   121		result->dimag = (double)c->imag;
	fpstod	a1@,fpa2
	fpmoved	fpa2,a0@
	.end
