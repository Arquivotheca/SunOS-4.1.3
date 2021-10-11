/*	@(#)libm.il 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

|   @(#)RELEASE libm.il, 4.1 alpha 9 

#ifdef FSWITCH
#define PREFIX V
#define PLIBM(f,g) ; \
	.inline	_F/**/g,4 ; \
	movl	sp@+,d0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	V/**/f/**/d,0 ; \
	movl	d1,sp@- ; \
	movl	d0,sp@- ; \
	jsr	_/**/g ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	V/**/f/**/s,0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end
#endif
#ifdef FSOFT
#define PREFIX F
#define PLIBM(f,g) ; \
	.inline	_F/**/g,4 ; \
	movl	sp@+,d0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	F/**/f/**/d,0 ; \
	movl	d1,sp@- ; \
	movl	d0,sp@- ; \
	jsr	_/**/g ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	F/**/f/**/s,0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end
#endif
#ifdef FSKY
#define PREFIX S
#define PLIBM(f,g) ; \
	.inline	_F/**/g,4 ; \
	movl	sp@+,d0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	S/**/f/**/d,0 ; \
	movl	d1,sp@- ; \
	movl	d0,sp@- ; \
	jsr	_/**/g ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end ; \
	.inline	S/**/f/**/s,0 ; \
	movl	d0,sp@- ; \
	lea	sp@,a0 ; \
	movl	a0,sp@- ; \
	jsr	_r_/**/g/**/_ ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end
#endif
#ifdef F68881
#define PREFIX M
#define PLIBM(f,g)
#endif
#ifdef FFPA
#define PREFIX W
#define PLIBM(f,g)
#endif

/*	Templates for [VFSMW]f[sd] libm functions.	*/

	PLIBM(sin,sin)
	PLIBM(cos,cos)
	PLIBM(tan,tan)
	PLIBM(acos,acos)
	PLIBM(asin,asin)
	PLIBM(atan,atan)
	PLIBM(cosh,cosh)
	PLIBM(sinh,sinh)
	PLIBM(tanh,tanh)
	PLIBM(exp,exp)
	PLIBM(pow10,exp10)
	PLIBM(pow2,exp2)
	PLIBM(log,log)
	PLIBM(log10,log10)
	PLIBM(log2,log2)

	.inline	_pow_ri,8
	movl	sp@+,a0
	movl	a0@,d0
	movl	sp@+,a0
	movl	a0@,d1
	jsr	PREFIX/**/powis
	.end

	.inline	_pow_di,8
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	movl	sp@+,a0
	jsr	PREFIX/**/powid
	.end

/*	templates for documented libm routines. */

	.inline	_abs,4			/* int abs(x) int x . */
        movl   sp@+,d0
        jpl     1f
        negl    d0
1:
        .end

	.inline	_fp_class,8	/* enum fp_class_type fp_class(x) double x . */
	movl	sp@+,a0		| a0 gets upperx
	movl	sp@+,a1		| a1 gets lowerx
	moveq	#2,d0		| d0 gets 2 - normal case
	movl	a0,d1		| d1 gets upperx
	andl	#0x7ff00000,d1
	jne	5f
	cmpl	#0,a1
	jne	2f		| Branch if subnormal.
	moveq	#0,d0		| d0 gets 0 - zero
	movl	a0,d1		| d1 gets upperx
	andl	#0x7fffffff,d1	| d1 gets abs(upperx)
	jeq	1f
2:
	moveq	#1,d0		| d0 gets 1 - subnormal
	jra	1f
5:
	cmpl	#0x7ff00000,d1  | compare upperx exponent to max
	jne	1f
4:
	cmpl	#0,a1
	jne	3f		| Branch if nan.
	movl	a0,d1		| d1 gets upperx
	andl	#0xfffff,d1
	jne	3f
	movl	#3,d0
	jra	1f
3:
	moveq	#4,d0		| d0 gets 4 - quiet nan
	movl	a0,d1		| d1 gets upperx
	btst	#19,d1
	jne	1f
	moveq	#5,d0		| d0 gets 5 - signaling nan
1:
	.end

	.inline	_copysign,16		/* double copysign(x,y) double x,y. */
	movl	sp@+,d0
	movl	sp@+,d1
	andl	#0x7fffffff,d0
       	tstl    sp@+
        jpl     1f
        orl    	#0x80000000,d0
1:
	movl	sp@+,a0
	.end

	.inline	_d_copysign_,8		/* double copysign(x,y) double x,y. */
	movl	sp@+,a0
	movl	sp@+,a1
	movl	a0@+,d0
	movl	a0@,d1
	andl	#0x7fffffff,d0
       	tstb    a1@
        jpl     1f
        orl    	#0x80000000,d0
1:
	.end

	.inline	_r_copysign_,8		/* double copysign(x,y) double x,y. */
	movl	sp@+,a0
	movl	sp@+,a1
	movl	a0@,d0
	andl	#0x7fffffff,d0
       	tstb    a1@
        jpl     1f
        orl    	#0x80000000,d0
1:
	.end

	.inline	_fabs,8			/* double fabs(x) double x . */
	movl	sp@+,d0
	movl	sp@+,d1
	andl	#0x7fffffff,d0
	.end

	.inline	_r_fabs_,4
	movl	sp@+,a0
	movl	a0@,d0
	andl	#0x7fffffff,d0
	.end

	.inline	_finite,8		/* int finite(x) double x . */
	movl	sp@+,d1
	movl	sp@+,d0
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	slt	d0
	andl	#1,d0
	.end

	.inline	_id_finite_,4		/* int finite(x) double x . */
	movl	sp@+,a0
	movl	a0@,d1
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	slt	d0
	andl	#1,d0
	.end

	.inline	_ir_finite_,4		/* int finite(x) double x . */
	movl	sp@+,a0
	movl	a0@,d1
	andl	#0x7fffffff,d1
	cmpl	#0x7f800000,d1
	slt	d0
	andl	#1,d0
	.end

	.inline	_isnormal,8
	movl	sp@+,d1
	movl	sp@+,d0			| Throw this away!
	clrl	d0			| Return zero for not normal.
	andl	#0x7ff00000,d1
	jeq	1f			| Branch if zero/subnormal.
	cmpl	#0x7ff00000,d1
	jeq	1f			| Branch if inf/nan.
	moveq	#1,d0			| Return one for normal.
1:
	.end

	.inline	_id_isnormal_,4
	movl	sp@+,a0
	movl	a0@,d1			| Throw this away!
	clrl	d0			| Return zero for not normal.
	andl	#0x7ff00000,d1
	jeq	1f			| Branch if zero/subnormal.
	cmpl	#0x7ff00000,d1
	jeq	1f			| Branch if inf/nan.
	moveq	#1,d0			| Return one for normal.
1:
	.end

	.inline	_ir_isnormal_,4
	movl	sp@+,a0
	movl	a0@,d1			| Throw this away!
	clrl	d0			| Return zero for not normal.
	andl	#0x7f800000,d1
	jeq	1f			| Branch if zero/subnormal.
	cmpl	#0x7f800000,d1
	jeq	1f			| Branch if inf/nan.
	moveq	#1,d0			| Return one for normal.
1:
	.end


	.inline	_iszero,8		/* int iszero(x) double x . */
	movl	sp@+,d1
	clrl	d0			| Assume x is nonzero.
	tstl	sp@+
	jne	1f			| Branch if not zero.
	andl	#0x7fffffff,d1
	jne	1f
	moveq	#1,d0			| Return 1 for zero.
1:
	.end

	.inline	_id_iszero_,4		/* int iszero(x) double x . */
	movl	sp@+,a0
	movl	a0@+,d1
	clrl	d0			| Return 0 for non-zero.
	tstl	a0@
	jne	1f			| Branch if not zero.
	andl	#0x7fffffff,d1
	jne	1f
	moveq	#1,d0			| Return 1 for zero.
1:
	.end

	.inline	_ir_iszero_,4		/* int iszero(x) double x . */
	movl	sp@+,a0
	movl	a0@,d1
	andl	#0x7fffffff,d1
	seq	d0
	andl	#1,d0
	.end

	.inline	_isinf,8		/* int isinf(x) double x . */
	movl	sp@+,d1
	clrl	d0			| Assume not inf.
	tstl	sp@+
	jne	1f			| Branch if not inf.
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	jne	1f
	moveq	#1,d0
1:
	.end

	.inline	_id_isinf_,4		/* int isinf(x) double x . */
	movl	sp@+,a0
	clrl	d0
	movl	a0@+,d1
	tstl	a0@
	jne	1f			| Branch if not inf.
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	jne	1f
	moveq	#1,d0
1:
	.end

	.inline	_ir_isinf_,4		/* int isinf(x) double x . */
	movl	sp@+,a0
	movl	a0@,d0
	andl	#0x7fffffff,d0
	cmpl	#0x7f800000,d0
	seq	d0
	andl	#1,d0
	.end

	.inline	_isnan,8		/* int isnan(x) double x . */
	movl	sp@+,a0
	movl	sp@+,a1
	clrl	d0
	movl	a0,d1
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	jlt	1f
	jgt	2f
	movl	a0,d1
	andl	#0xfffff,d1
	movl	a1,d0
	orl	d0,d1
2:
	sne	d0
	andl	#1,d0
1:
	.end
	
	.inline	_id_isnan_,4		/* int isnan(x) double x . */
	movl	sp@+,a0
	clrl	d0
	movl	a0@,d1
	andl	#0x7fffffff,d1
	cmpl	#0x7ff00000,d1
	jlt	1f
	jgt	2f
	movl	a0@+,d1
	andl	#0xfffff,d1
	movl	a0@,d0
	orl	d0,d1
2:
	sne	d0
	andl	#1,d0
1:
	.end
	
	.inline	_ir_isnan_,4		/* int isnan(x) double x . */
	movl	sp@+,a0
	movl	a0@,d0
	andl	#0x7fffffff,d0
	cmpl	#0x7f800000,d0
	sgt	d0
	andl	#1,d0
1:
	.end
	
	.inline	_issubnormal,8		/* int issubnormal(x) double x . */
	movl	sp@+,a0
	movl	sp@+,a1
	clrl	d0			| Assume not subnormal - return 0.
	movl	a0,d1
	andl	#0x7ff00000,d1
	jne	1f			| Branch if not minimum exponent.
	movl	a0,d1
	andl	#0xfffff,d1
	movl	a1,d0
	orl	d0,d1
	sne	d0
	andl	#1,d0
1:
	.end
	
	.inline	_id_issubnormal_,4	/* int issubnormal(x) double x . */
	movl	sp@+,a0
	clrl	d0			| Assume not subnormal - return 0.
	movl	a0@,d1
	andl	#0x7ff00000,d1
	jne	1f			| Branch if not minimum exponent.
	movl	a0@+,d1
	andl	#0xfffff,d1
	movl	a0@,d0
	orl	d0,d1
	sne	d0
	andl	#1,d0
1:
	.end

	.inline	_ir_issubnormal_,4	/* int issubnormal(x) double x . */
	movl	sp@+,a0
	clrl	d0			| Assume not subnormal - return 0.
	movl	a0@,d1
	andl	#0x7fffffff,d1
	jeq	1f			| Branch if zero.
	cmpl	#0x00800000,d1
	slt	d0
	andl	#1,d0
1:
	.end
	
	.inline	_signbit,8		/* int signbit(x) double x . */
	movl	sp@+,d0
	movl	sp@+,d1
	roll	#1,d0
	andl	#1,d0			| Return one for negative, zero for positive.
	.end

	.inline	_id_signbit_,4		/* int signbit(x) double x . */
	movl	sp@+,a0
	movw	a0@,d0
	rolw	#1,d0
	andl	#1,d0			| Return one for negative, zero for positive.
	.end

	.inline	_ir_signbit_,4
	movl	sp@+,a0
	movw	a0@,d0
	rolw	#1,d0
	andl	#1,d0			| Return one for negative, zero for positive.
	.end	

	.inline	_min_subnormal,0
	movl	#0x0,d0
	movl	#0x1,d1
	.end

	.inline	_d_min_subnormal_,0
	movl	#0x0,d0
	movl	#0x1,d1
	.end

	.inline	_r_min_subnormal_,0
	movl	#0x1,d0
	.end

	.inline	_max_subnormal,0
	movl	#0x000fffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_d_max_subnormal_,0
	movl	#0x000fffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_r_max_subnormal_,0
	movl	#0x007fffff,d0
	.end

	.inline	_min_normal,0
	movl	#0x00100000,d0
	movl	#0x0,d1
	.end

	.inline	_d_min_normal_,0
	movl	#0x00100000,d0
	movl	#0x0,d1
	.end

	.inline	_r_min_normal_,0
	movl	#0x00800000,d0
	.end

	.inline	_max_normal,0
	movl	#0x7fefffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_d_max_normal_,0
	movl	#0x7fefffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_r_max_normal_,0
	movl	#0x7f7fffff,d0
	.end

	.inline	_infinity,0
	movl	#0x7ff00000,d0
	movl	#0x0,d1
	.end

                        /* POSIX defines HUGE_VAL in math.h as __infinity() */
        .inline ___infinity,0
        movl    #0x7ff00000,d0
        movl    #0x0,d1
        .end

	.inline	_d_infinity_,0
	movl	#0x7ff00000,d0
	movl	#0x0,d1
	.end

	.inline	_r_infinity_,0
	movl	#0x7f800000,d0
	.end

	.inline	_signaling_nan,0
	movl	#0x7ff00000,d0
	movl	#0x1,d1
	.end

	.inline	_d_signaling_nan_,0
	movl	#0x7ff00000,d0
	movl	#0x1,d1
	.end

	.inline	_r_signaling_nan_,0
	movl	#0x7f800001,d0
	.end

	.inline	_quiet_nan,0
	movl	#0x7fffffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_d_quiet_nan_,0
	movl	#0x7fffffff,d0
	movl	#0xffffffff,d1
	.end

	.inline	_r_quiet_nan_,0
	movl	#0x7fffffff,d0
	.end

	.inline	_ilogb,8	| int ilogb ( double x ) ;
	movl	sp@+,d0
	movl	sp@+,d1
	movl	d0,a0		| a0 preserves d0 in case call is necessary
	clrw	d0
	swap	d0
	lsrw	#4,d0		| biased exponent is now right-aligned
	andw	#0x7ff,d0	| garbage cleared.
	jeq	1f		| Branch if zero or subnormal.
	subl	#0x3ff,d0	| d0 gets unbiased exponent.
	cmpw	#0x400,d0
	jne	2f		| Jump if not inf or nan - d0 contains result.
1:
	movl	d1,sp@-
	movl	a0,sp@-		| Restore stack.
	jsr	_ilogb		| Function to handle hard cases.
2:
	.end

	.inline	_id_ilogb_,4	| int id_ilogb_ ( double *x ) ;
	movl	sp@+,a0
	clrl	d0
	movw	a0@,d0
	lsrw	#4,d0		| biased exponent is now right-aligned
	andw	#0x7ff,d0	| garbage cleared.
	jeq	1f		| Branch if zero or subnormal.
	subl	#0x3ff,d0	| d0 gets unbiased exponent.
	cmpw	#0x400,d0
	jne	2f		| Jump if not inf or nan - d0 contains result.
1:
	movl	a0@(4),sp@-
	movl	a0@,sp@-	| Restore stack.
	jsr	_ilogb		| Function to handle hard cases.
2:
	.end

	.inline	_ir_ilogb_,4	| int ir_ilogb_ ( float *x ) ;
	movl	sp@+,a0
	clrl	d0
	movw	a0@,d0
	lsrw	#7,d0		| biased exponent is now right-aligned
	andw	#0xff,d0	| garbage cleared.
	jeq	1f		| Branch if zero or subnormal.
	subl	#0x7f,d0	| d0 gets unbiased exponent.
	cmpw	#0x80,d0
	jne	2f		| Jump if not inf or nan - d0 contains result.
1:
	movl	a0,sp@-		| Restore stack.
	jsr	_ir_ilogb_	| Function to handle hard cases.
2:
	.end

/*	templates for undocumented libF77 routines	*/

/*****	begin from 3.2 fsoft.il *****/

|
| i_conv_c(cp,len,ip)
|       char *cp;
|       int len;
|       int *ip;
|
        .inline _i_conv_c,12
        movl    sp@+,a0
        movl    sp@+,d0
        movl    sp@+,a1
        movb    a1@(3),a0@
        .end
 
|
| c_conv_i(cp)
|       char *cp;
|
        .inline _c_conv_i,4
        movl    sp@+,a0
        movb    a0@,d0
#ifdef FFPA
        extbl   d0		| Only works on 68020!
#else
        extw    d0
        extl    d0
#endif
        .end

	.inline	_c_abs,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@,d1
	jsr	PREFIX/**/length2s
	.end

        .inline	__Fc_conv_f,8 	| convert complex  to float
        movl    sp@+,d0         | pick up argument
        movl    sp@+,d1         | ignore this
        .end

	.inline	__Fc_neg,8
	movl	sp@+,d0		| pick up real part of argument
	movl	sp@+,d1		| pick up imag part of argument
	eorl	#0x80000000,d0	| change sign
	eorl	#0x80000000,d1	| change sign
	.end

        .inline __Fc_add,16
        movl    sp@(4),d0       | pick up imaginary part of 1st argument
        movl    sp@(12),d1      | pick up imaginary part of 2nd argument
        jsr     PREFIX/**/adds  | add them and
        movl    d0,sp@(12)      | save result
        movl    sp@+,d0         | pick up real part of 1st argument
        movl    sp@+,d1         | Skip arg.
        movl    sp@+,d1         | pick up real part of 2nd argument
        jsr     PREFIX/**/adds   | add them
        movl    sp@+,d1         | load imag result in return register
        .end

        .inline __Fc_minus,16
        movl    sp@(4),d0       | pick up imaginary part of 1st argument
        movl    sp@(12),d1      | pick up imaginary part of 2nd argument
        jsr     PREFIX/**/subs  | add them and
        movl    d0,sp@(12)      | save result
        movl    sp@+,d0         | pick up real part of 1st argument
        movl    sp@+,d1         | Skip arg.
        movl    sp@+,d1         | pick up real part of 2nd argument
        jsr     PREFIX/**/subs  | add them
        movl    sp@+,d1         | load imag result in return register
        .end

|/* Convert float to complex */
|
|void
|_Ff_conv_c(c, x)
|        complex        *c;
|        FLOATPARAMETER  x;
|{
|        c->real = FLOATPARAMETERVALUE(x);
|        c->imag = 0.0;
|}

	.inline	__Ff_conv_c,4
	movl	sp@+,d0		| pick up argument
	movl	sp@+,a0		| a0 points to result c.
	movl	d0,a0@+		| Rc gets x.
	clrl	a0@		| Ic gets 0.
	.end

|/* Convert int to complex */
| 
|void
|_Fi_conv_c(c, i)
|        complex        *c;
|        int             i;
|{
|        c->real = (float) i;
|        c->imag = 0.0;
|}

	.inline	__Fi_conv_c,4
	movl	sp@+,d0		| pick up argument
	jsr     PREFIX/**/flts
	movl	sp@+,a0         | a0 points to result c.
	movl	d0,a0@+		| Rc gets x.
	clrl	a0@		| Ic gets 0.
	.end

		| 	convert complex  to int
	.inline	__Fc_conv_i,8
	movl	sp@+,d0		| pick up argument
	movl	sp@+,d1		| ignore this
	jsr     PREFIX/**/ints
	.end

		|	convert complex  to double
	.inline	__Fc_conv_d,8
	movl    sp@+,d0   	| pick up argument
	movl	sp@+,d1		| ignore this
	jsr     PREFIX/**/stod
	.end

|/* Convert double to complex */
| 
|void
|_Fd_conv_c(c, x)
|        complex        *c;
|        double          x;
|{
|        c->real = (float) (x);
|        c->imag = 0.0;
|}
 
	.inline	__Fd_conv_c,8
	movl    sp@+,d0   
	movl    sp@+,d1   	| pick up argument
	jsr     PREFIX/**/dtos
	movl	sp@+,a0         | a0 points to result c.
	movl	d0,a0@+		| Rc gets x.
	clrl	a0@		| Ic gets 0.
	.end

|	Double Precision Complex libF77 routines

|
| void
| d_cmplx(resp,dp1,dp2)
|	register dcomplex *resp;
|	register double *dp1, *dp2;
| {
| 	resp->real = *dp1;
| 	resp->imag = *dp2;
| }
| 
	.inline	_z_abs,4
	movl	sp@+,a0
	movl	a0@+,d0
	movl	a0@+,d1
	jsr	PREFIX/**/length2d
	.end

|
| Fz_neg(c, a)
| dcomplex *c, *a;
| {
| 	c->dreal = - a->dreal;
| 	c->dimag = - a->dimag;
| }
|
	.inline	__Fz_neg,8
        movl    sp@+,a0         | a0 = c;
        movl    sp@+,a1         | a1 = a;
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        eorb    #0x80,a0@(-4)
        movl    a1@+,a0@+       | c->dreal = - a->dreal;
        movl    a1@+,a0@+       | c->imag = - a->imag;
        eorb    #0x80,a0@(-4)
        movl    a1@,a0@         | c->imag = - a->imag;
	.end

|
| _Ff_conv_z(dp,f)	
|	dcomplex *dp;
|	union {float f; int i;}f;
| {
|	dp->real = (double)f.f;
|	dp->imag = 0.0;
| }
|
	.inline	__Ff_conv_z,8
	movl	sp@(4),d0
	jsr	PREFIX/**/stod
	movl	sp@+,a0
	movl	d0,a0@+
	movl	d1,a0@+
	clrl    a0@+
	clrl    a0@
	movl	sp@+,d0			| Throw away second argument.
	.end

|
| float
| Fz_conv_f(dp)		
|	dcomplex *dp;
| {
|	return (float)dp->dreal;
| }
|
	.inline	__Fz_conv_f,4
	movl    sp@+,a0   
	movl	a0@+,d0
	movl	a0@,d1
	jsr	PREFIX/**/dtos
	.end

|
| int
| Fz_conv_i(dp)		
|	dcomplex *dp;
| {
|	return (int)dp->dreal;
| }
|
	.inline	__Fz_conv_i,4
	movl    sp@+,a0   
	movl	a0@+,d0
	movl	a0@,d1
	jsr     PREFIX/**/intd
	.end

|
| Fi_conv_z(dp,i)	
|	dcomplex *dp;
|	int i;
| {
|	dp->dreal = (double)i;
|	dp->dimag = 0.0;
| }
|

	.inline	__Fi_conv_z,8
	movl	sp@(4),d0
	jsr     PREFIX/**/fltd
	movl	sp@+,a0
	movl	d0,a0@+
	movl	sp@+,d0		| Trash.
	movl	d1,a0@+
	clrl    a0@+
	clrl    a0@
	.end

|
| double
| Fz_conv_d(dp)		
|	dcomplex *dp;
| {
|	return dp->dreal;
| }
|
	.inline	__Fz_conv_d,4
	movl    sp@+,a0
	movl    a0@+,d0
	movl    a0@,d1
	.end

|
| Fd_conv_z(dp,d)	
|	dcomplex *dp;
|	double d;
| {
|	dp->dreal = d;
|	dp->dimag = 0.0;
| }
|
	.inline	__Fd_conv_z,12
	movl    sp@+,a0   
	movl    sp@+,a0@+
	movl    sp@+,a0@+
	clrl    a0@+
	clrl    a0@
	.end

|
| double
| Fz_conv_c(dp)         
|       dcomplex *dp;
| {
|       union { double d; complex c; } sleaze;
|       sleaze.cval.real = (float)dp->dreal;
|       sleaze.cval.imag = (float)dp->dimag;
|       return sleaze.dval;
| }
|

        .inline __Fz_conv_c,4
	movl	sp@,a0		| pick up real part of argument
	movl	a0@(8),d0
	movl	a0@(12),d1
	jsr	PREFIX/**/dtos
	movl	sp@+,a0		| Restore a0.
	movl	d0,sp@-		| Save second part of result.
	movl	a0@+,d0
	movl	a0@,d1
	jsr	PREFIX/**/dtos
	movl	sp@+,d1		| Restore second part of result.
	.end

		| 	convert complex to double complex 
		|	arg1 is address of result; arg2 is complex as a double

	.inline	__Fc_conv_z,12
	movl	sp@(4),d0	| pick up real part of argument
	jsr	PREFIX/**/stod
	movl	sp@,a0		| get address of result
	movl	d0,a0@+
	movl	d1,a0@
	movl	sp@(8),d0	| pick up imaginary part of argument
	jsr	PREFIX/**/stod
	movl	sp@+,a0		| get address of result
	movl	d0,a0@(8)
	movl	d1,a0@(12)
	lea	sp@(8),sp	| Bypass second argument.
	.end

/*****	end from 3.2 fsoft.il *****/

        .inline	_c_cmplx,12
        movl    sp@+,a1
        movl    sp@+,a0
        movl    sp@+,d0
        movl    a0@,a1@+
        movl    d0,a0
        movl    a0@,a1@
        .end

        .inline _d_atn2,8	| Implements ATAN2(DOUBLE).
	movl	sp@+,a0		| a0 gets address of first argument.
	movl	sp@+,a1		| a1 gets address of second argument.
        movl	a1@+,d0		| Pop second argument.
        movl	a1@,d1		| Pop second argument.
	movl	d1,sp@-
	movl	d0,sp@-
        movl	a0@+,d0		| Pop first argument.
        movl	a0@,d1		| Pop first argument.
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_atan2
	movl	sp@+,a0
	movl	sp@+,a0
	movl	sp@+,a0
	movl	sp@+,a0
	.end
	
|
| void
| d_cmplx(resp,dp1,dp2)
|       register dcomplex *resp.
|       register double *dp1, *dp2.
| {
|       resp->real = *dp1.
|       resp->imag = *dp2.
| }
|
        .inline	_d_cmplx,12
        movl    sp@+,a1
        movl    sp@+,a0
        movl    sp@+,d0
        movl    a0@+,a1@+
        movl    a0@,a1@+
        movl    d0,a0
        movl    a0@+,a1@+
        movl    a0@,a1@
        .end
 
        .inline	_d_cnjg,8
        movl    sp@+,a0
        movl    sp@+,a1
        movl    a1@+,a0@+
        movl    a1@+,a0@+
        movl    a1@+,a0@+
        movl    a1@,a0@
        eorl    #0x80000000,a0@(-4)
        .end
 
        .inline	_d_imag,4
        movl    sp@+,a0
        movl   a0@(8),d0
        movl   a0@(12),d1
        .end
 
        .inline	_d_sign,8
        movl   sp@+,a0
        movl   a0@+,d0
	andl	#0x7fffffff,d0
        movl   a0@,d1
        movl   sp@+,a0
        cmpl    #0x80000000,a0@
        jhi     2f              | Branch if y < -0.
        jcs     1f              | Branch if y >= +0.
        tstl    a0@(4)
        jeq     1f              | Branch if y is -0.
2:
	orl	#0x80000000,d0
1:
        .end

	.inline _i_nint,4	| Implements NINT(REAL).
	movl	sp@+,a0		| a0 gets address of argument.
	movl	a0@,d0		| d0 gets argument.
	jsr	PREFIX/**/stod
	movl	d1,sp@-
	movl	d0,sp@-
	jsr	_nint
	movl	sp@+,d1		| pop junk.
	movl	sp@+,d1		| pop junk.
	.end	

	.inline _i_dnnt,4	| Implements NINT(DOUBLE).
	movl	sp@+,a0		| a0 gets address of argument.
	movl	a0@(4),sp@-
	movl	a0@,sp@-
	jsr	_nint
	movl	sp@+,d1		| pop junk.
	movl	sp@+,d1		| pop junk.
	.end	

	.inline _d_pow_,8	| double _d_pow_ ( double *x, *y )
	movl	sp@+,a0		| Load addresses of arguments.
	movl	sp@+,a1
	movl	a1@+,d0		| Pop argument.
	movl	a1@,sp@-	| Push argument.
	movl	d0,sp@-
	movl	a0@+,d0
	movl	a0@,sp@-
	movl	d0,sp@-
	jsr	_pow
	movl	sp@+,a0		| Remove arguments.
	movl	sp@+,a0
	movl	sp@+,a0
	movl	sp@+,a0
	.end

        .inline _pow_dd,8	| Implements DOUBLE**DOUBLE.
	movl	sp@+,a0		| Load addresses of arguments.
	movl	sp@+,a1
	movl	a1@+,d0		| Pop argument.
	movl	a1@,sp@-	| Push argument.
	movl	d0,sp@-
	movl	a0@+,d0
	movl	a0@,sp@-
	movl	d0,sp@-
	jsr	_pow
	movl	sp@+,a0		| Remove arguments.
	movl	sp@+,a0
	movl	sp@+,a0
	movl	sp@+,a0
	.end
	
        .inline _pow_rr,8	| Implements REAL**REAL.
        jsr     _r_pow_
        movl	sp@+,a0		| Pop junk.
        movl	sp@+,a0		| Pop junk.
	.end

        .inline _r_atn2,8	| Implements ATAN2(REAL).
	jsr	_r_atan2_
        movl	sp@+,a0		| Pop junk.
        movl	sp@+,a0		| Pop junk.
	.end

        .inline	_r_cnjg,8
        movl    sp@+,a0
        movl    sp@+,a1
        movl    a1@+,a0@+
        movl    a1@,a0@
        eorb    #0x80,a0@
        .end

        .inline	_r_imag,4
        movl    sp@+,a0
        movl   a0@(4),d0
        .end

        .inline	_r_sign,8
        movl   sp@+,a0
        movl   a0@,d0
	andl	#0x7fffffff,d0
        movl   sp@+,a0
        cmpl    #0x80000000,a0@
        jls     1f              | Branch if y >= +0.
	orl	#0x80000000,d0
1:
        .end

/*	Templates for libm "R" routines for single-precision Fortran, Pascal, Modula-2 	*/
/*	Templates for libm "D" routines for double-precision Fortran			*/

#define D_F(X) /* double d_X_( double x ) */ \
	.inline _d_/**/X/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@(4),sp@- ; \
	movl	a0@,sp@- ; \
	jsr	_/**/X ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end

#define ID_F(X) /* int id_X_( double x ) */ \
	.inline _id_/**/X/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@(4),sp@- ; \
	movl	a0@,sp@- ; \
	jsr	_/**/X ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end

#define D_F2(X) /* double d_X_( double x,y ) */ \
	.inline _d_/**/X/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a1 ; \
	movl	a1@(4),sp@- ; \
	movl	a1@,sp@- ; \
	movl	a0@(4),sp@- ; \
	movl	a0@,sp@- ; \
	jsr	_/**/X ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end

#define D_FIX(X) /* double d_X_( int n, double y ) */ \
	.inline _d_/**/X/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a1 ; \
	movl	a1@(4),sp@- ; \
	movl	a1@,sp@- ; \
	movl	a0@,sp@- ; \
	jsr	_/**/X ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end

#define D_FXI(X) /* double d_X_( double n, int y ) */ \
	.inline _d_/**/X/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a1 ; \
	movl	a1@,sp@- ; \
	movl	a0@(4),sp@- ; \
	movl	a0@,sp@- ; \
	jsr	_/**/X ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
	.end

	D_F(acos)
	D_F(acosh)
	D_F(cos)
	D_F(cosh)
	D_F(asin)
	D_F(asinh)
	D_F(sin)
	D_F(sinh)
	D_F(atan)
	D_F(atanh)
	D_F(tan)
	D_F(tanh)
	D_F(cbrt)
	D_F(erf)
	D_F(erfc)
	D_F(exp)
	D_F(exp2)
	D_F(exp10)
	D_F(expm1)
	D_F(j0)
	D_F(j1)
	D_F(lgamma)
	D_F(log)
	D_F(log1p)
	D_F(log2)
	D_F(log10)
	D_F(logb)
	D_F(sqrt)
	D_F(y0)
	D_F(y1)
	
	ID_F(fp_class)
	ID_F(ilogb)

	D_F2(atan2)
	D_F2(nextafter)
	D_F2(scalb)

	D_FIX(jn)
	D_FIX(yn)

#ifdef FSKY
#define CDRI(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     S/**/g/**/d ; \
        .end ; \
	.inline	_id_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	S/**/g/**/d ; \
	.end ; \
	.inline	_ir_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	S/**/g/**/s ; \
	.end
#define CDRX(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     S/**/g/**/d ; \
        .end ; \
	.inline	_d_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	S/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	S/**/g/**/s ; \
	.end
#define CDRXX(f,g) ; \
	.inline	_/**/f,16 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     S/**/g/**/d ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	S/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	S/**/g/**/s ; \
	.end
#define CDRXI(f,g) ; \
	.inline	_/**/f,12 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     S/**/g/**/d ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	S/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	S/**/g/**/s ; \
	.end
#endif
#ifdef FSOFT
#define CDRI(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     F/**/g/**/d ; \
        .end ; \
	.inline	_id_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	F/**/g/**/d ; \
	.end ; \
	.inline	_ir_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	F/**/g/**/s ; \
	.end
#define CDRX(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     F/**/g/**/d ; \
        .end ; \
	.inline	_d_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	F/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	F/**/g/**/s ; \
	.end
#define CDRXX(f,g) ; \
	.inline	_/**/f,16 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     F/**/g/**/d ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	F/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	F/**/g/**/s ; \
	.end
#define CDRXI(f,g) ; \
	.inline	_/**/f,12 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     F/**/g/**/d ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	F/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	F/**/g/**/s ; \
	.end
#endif
#ifdef FSWITCH
#define CDRI(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     V/**/g/**/d ; \
        .end ; \
	.inline	_id_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	V/**/g/**/d ; \
	.end ; \
	.inline	_ir_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	V/**/g/**/s ; \
	.end
#define CDRX(f,g) ; \
	.inline	_/**/f,8 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	jsr     V/**/g/**/d ; \
        .end ; \
	.inline	_d_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	jsr	V/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	jsr	V/**/g/**/s ; \
	.end
#define CDRXX(f,g) ; \
	.inline	_/**/f,16 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     V/**/g/**/d ; \
	movl	sp@+,a0 ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	V/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	V/**/g/**/s ; \
	.end
#define CDRXI(f,g) ; \
	.inline	_/**/f,12 ; \
	movl	sp@+,d0 ; \
	movl	sp@+,d1 ; \
	lea	sp@,a0 ; \
	jsr     V/**/g/**/d ; \
	movl	sp@+,a0 ; \
        .end ; \
	.inline	_d_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@+,d0 ; \
	movl	a0@,d1 ; \
	movl	sp@+,a0 ; \
	jsr	V/**/g/**/d ; \
	.end ; \
	.inline	_r_/**/f/**/_,8 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d0 ; \
	movl	sp@+,a0 ; \
	movl	a0@,d1 ; \
	jsr	V/**/g/**/s ; \
	.end
#endif

#ifndef CDRI
#define CDRI(f,g)
#define CDRX(f,g)
#define CDRXX(f,g)
#define CDRXI(f,g)
#endif

	CDRI(nint,nint)
	CDRI(irint,rint)
	CDRX(aint,aint)
	CDRX(anint,anint)
	CDRX(rint,arint)
	CDRX(ceil,ceil)
	CDRX(floor,floor)
	CDRX(sqrt,sqrt)
	CDRXX(hypot,length2)
	CDRXX(fmod,mod)
	CDRXX(remainder,rem)
	CDRXI(scalbn,scalei)

