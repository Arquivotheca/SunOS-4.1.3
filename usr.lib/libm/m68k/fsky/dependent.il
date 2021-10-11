/*	@(#)dependent.il 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*	-fsky inline expansion file.	*/

	.inline	_Fc_minus,16	
	movel	__skybase,a0
	movew	#0x1048,a0@(-4)
	movel	sp@+,a0@
	movel	sp@(4),a0@
	movel	sp@+,a0@
	movel	sp@+,d0
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

	.inline	_Fc_add,16	
	movel	__skybase,a0
	movew	#0x1047,a0@(-4)
	movel	sp@+,a0@
	movel	sp@(4),a0@
	movel	sp@+,a0@
	movel	sp@+,d0
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

		| 	convert complex  to int
	.inline	_Fc_conv_i,8
        movl    __skybase,a1
        movw    #0x1027,a1@(-4)
        movl    sp@+,a1@
	movl	sp@+,d1		| ignore this
        movl    a1@,d0
	.end

		| 	convert int to complex 
	.inline	_Fi_conv_c,4
	movl    __skybase,a1
        movw    #0x1024,a1@(-4)
        movl    sp@+,a1@
        movl    a1@,d0
	clrl    d1
	.end

		|	convert complex  to double
	.inline	_Fc_conv_d,8
        movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    sp@+,a1@
	movl	sp@+,d1		| ignore this
        movl    a1@,d0
        movl    a1@,d1
 	.end

		|	convert double  to complex
	.inline	_Fd_conv_c,8
        movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    sp@+,a1@
        movl    sp@+,a1@
        movl    a1@,d0
	clrl	d1
	.end

	.inline	_d_dim,8
        movl    __skybase,a1
        movw    #0x1008,a1@(-4)
	movel	sp@+,a0
        movl    a0@+,a1@
        movl    a0@,a1@
	movel	sp@+,a0
        movl    a0@+,a1@
        movl    a0@,a1@
1:      tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
        movl    a1@,d1
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

	.inline	_d_mod,8
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	movel	sp@+,a0
	jsr	Fmodd
	.end

	.inline	_i_dnnt,4
	movel	sp@+,a0
	movel	a0@+,d0
	movel	a0@,d1
	jsr	Fnintd
	.end
	
	.inline	_i_nint,4
	movel	sp@+,a0
	movel	a0@,d0
	jsr	Fnints
	.end

	.inline	_r_dim,8
        movl    __skybase,a1
        movw    #0x1007,a1@(-4)
	movel	sp@+,a0
        movel   a0@,a1@
	movel	sp@+,a0
	movel	a0@,a1@
1:
        tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
	jpl	1f		| Branch if positive sign.
	cmpl	#0xff800000,d0
	jgt	1f		| Branch if nan.
	clrl	d0
1:
	.end

        .inline _r_mod,8	| Can't pass fcvs with Sky FFP mod op code.
        movel   sp@+,a0
        movel   sp@+,a1
        movel   a0@,d0
        movel   a1@,d1
        jsr     Fmods		| So use -fsoft version.
        .end

        .inline _r_fmod_,8
        movl    sp@+,a0
        movl    __skybase,a1
        movw    #0x1030,a1@(-4)
        movl    a0@,d1                  | d1 gets x.
        movl    d1,a1@
        movl    sp@+,a0
        movl    a0@,a0                  | a0 gets y.
        movl    a0,a1@
1:
        tstw    a1@(-2)
        jge     1b 
        movl    a1@,d0                  | d0 gets mod.
        eorl    d0,d1                   | d1 gets sign(mod) # sign(x)
        jpl    4f                      | Branch if sign(mod)=sign(x).
        movl    a0,d1
        eorl    d0,d1                   | d1 gets sign(mod) # sign(y)
        jpl    2f                      | Branch if sign(mod) = sign(y)
        movw    #0x1001,a1@(-4)
        jra    3f
2:
        movw    #0x1007,a1@(-4)
3:
        movl    d0,a1@             | Add or subtract to get sign.
        movl    a0,a1@
1:
        tstw    a1@(-2)
        jge     1b
        movl    a1@,d0
4:
        .end
 
	.inline	_Fc_mult,16
	movel	__skybase,a0
	movew	#0x1011,a0@(-4)
	movel	sp@+,a0@
	movel	sp@+,a0@
	movel	sp@+,a0@
	movel	sp@+,a0@
1:
	tstw	a0@(-2)
	jge	1b
	movel	a0@,d0
	movel	a0@,d1
	.end

	.inline	_ceil,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Fceild
	.end

	.inline	_floor,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Ffloord
	.end

	.inline	_rint,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Farintd
	.end

	.inline	_sqrt,8
	movl	sp@+,d0
	movl	sp@+,d1
	jsr	Fsqrtd
	.end

	.inline	_drem,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fremd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

	.inline	_fmod,16
	movl	sp@+,d0
	movl	sp@+,d1
	lea	sp@,a0
	jsr	Fmodd
	movel	sp@+,a0
	movel	sp@+,a0
	.end

#define SLIBM(f,g) ; \
	.inline	_r_/**/f/**/_,4 ; \
	movl	sp@+,a0 ; \
         movl    __skybase,a1 ; \
        movw    #/**/g,a1@(-4) ; \
        movl    a0@,a1@ ; \
1: 	tstw    a1@(-2) ; \
        jge    1b ; \
        movl    a1@,d0 ; \
	.end ; \
	.inline	S/**/f/**/s,0 ; \
         movl    __skybase,a1 ; \
        movw    #/**/g,a1@(-4) ; \
        movl    d0,a1@ ; \
1: 	tstw    a1@(-2) ; \
        jge    1b ; \
        movl    a1@,d0 ; \
	.end	 ; \
	.inline	_F/**/f/**/,4 ; \
         movl    __skybase,a1 ; \
        movw    #/**/g,a1@(-4) ; \
        movl    sp@+,a1@ ; \
1: 	tstw    a1@(-2) ; \
        jge    1b ; \
        movl    a1@,d0 ; \
	.end

	SLIBM(cos,0x1028)
	SLIBM(sin,0x1029)
	SLIBM(tan,0x102a)
	SLIBM(atan,0x102b)
	SLIBM(exp,0x102c)
	SLIBM(log,0x102d)
	SLIBM(sqrt,0x102f)

	.inline	Spows,0
        movl    __skybase,a1
        movw    #0x102e,a1@(-4)
        movl    d0,a1@
1: 	tstw    a1@(-2)
        jge    	1b
        movl    d1,a1@
2: 	tstw    a1@(-2)
        jge     2b
        movl    a1@,d0
	.end

	.inline	_r_pow_,8
	movl	sp@+,a0
        movl    __skybase,a1
        movw    #0x102e,a1@(-4)
        movl    a0@,a1@
1: 	tstw    a1@(-2)
        jge     1b
	movl	sp@+,a0
        movl    a0@,a1@
2: 	tstw    a1@(-2)
        jge     2b
        movl    a1@,d0
	.end

	.inline	_pow_rr,8
	movl	sp@+,a0
        movl    __skybase,a1
        movw    #0x102e,a1@(-4)
        movl    a0@,a1@
1: 	tstw    a1@(-2)
        jge     1b
	movl	sp@+,a0
        movl    a0@,a1@
2: 	tstw    a1@(-2)
        jge     2b
        movl    a1@,d0
	.end

	.inline	Ssqrs,0
        movl    __skybase,a1
        movw    #0x100b,a1@(-4)
        movl    d0,a1@
        movl    d0,a1@
        movl    a1@,d0
	.end

	.inline Sdtos,0
	movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    d0,a1@
        movl    d1,a1@
        movl    a1@,d0
	.end

	.inline	Sstod,0
	movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    d0,a1@
        movl    a1@,d0
        movl    a1@,d1
	.end

	.inline	Ssqrd,0
	movl    __skybase,a1
        movw    #0x100c,a1@(-4)
        movl    d0,a1@
        movl    d1,a1@
        movl    d0,a1@
        movl    d1,a1@
1:      tstw    a1@(-2)
        jge    1b
        movl    a1@,d0
        movl    a1@,d1
	.end

	.inline	Ssqrtd,0
	jsr	Fsqrtd
	.end

        .inline _Fc_conv_z,12
        movl    sp@+,a0          | get address of result
        movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    sp@+,a1@	| real part of argument
        movl    a1@,a0@+
        movl    a1@,a0@+
        movw    #0x1042,a1@(-4)
        movl    sp@+,a1@	| imaginary part of argument
        movl    a1@,a0@+
        movl    a1@,a0@+
        .end

        .inline __Fz_conv_c,4
        movl    sp@+,a0          | pick up real part of argument
        movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    a0@+,a1@
        movl    a0@+,a1@
        movl    a1@,d0
        movw    #0x1043,a1@(-4)
        movl    a0@+,a1@
        movl    a0@+,a1@
        movl    a1@,d1
        .end

        .inline _Ff_conv_z,8
	movl	sp@+,a0
        movl    __skybase,a1
        movw    #0x1042,a1@(-4)
        movl    sp@+,a1@
        movl    a1@,a0@+
        movl    a1@,a0@+
        clrl    a0@+
        clrl    a0@
        .end

        .inline __Fz_conv_f,4
        movl    sp@+,a0
        movl    __skybase,a1
        movw    #0x1043,a1@(-4)
        movl    a0@+,a1@
        movl    a0@,a1@
        movl    a1@,d0
        .end

        .inline __Fz_conv_i,4
        movl    sp@+,a0
        movl    __skybase,a1
        movw    #0x1045,a1@(-4)
        movl    a0@+,a1@
        movl    a0@,a1@
        movl    a1@,d0
        .end

        .inline _Fi_conv_z,8
        movl    sp@+,a0
        movl    __skybase,a1
        movw    #0x1044,a1@(-4)
        movl    sp@+,a1@
        movl    a1@,a0@+
        movl    a1@,a0@+
        clrl    a0@+
        clrl    a0@
        .end
 
	.inline	_r_hypot_,8
        movl	sp@+,a0
	movl    __skybase,a1
        movw    #0x101d,a1@(-4)
        movl    a0@,a1@
        movl	sp@+,a0
	movl    a0@,a1@
1:
	tstw	a1@(-2)
	jge	1b
        movl    a1@,d0
        movw    #0x102f,a1@(-4)
        movl    d0,a1@
1:
	tstw	a1@(-2)
	jge	1b
        movl    a1@,d0
        .end
 
	.inline	_c_abs,4
        movl	sp@+,a0
	movl    __skybase,a1
        movw    #0x101d,a1@(-4)
        movl    a0@+,a1@
	movl    a0@,a1@
1:
	tstw	a1@(-2)
	jge	1b
        movl    a1@,d0
        movw    #0x102f,a1@(-4)
        movl    d0,a1@
1:
	tstw	a1@(-2)
	jge	1b
        movl    a1@,d0
        .end
        
	.inline __Fz_add,12
        movl    sp@+,d0         | d0 = c.
        movl    sp@+,a0         | a0 = a.
        movl    sp@+,d1         | d1 = b.
	movl    __skybase,a1
	movw	#0x1002,a1@(-4)
	movl	a0@+,a1@
	movl	a0@+,a1@
	exg	a0,d1		| a0 = b . d1 = a .
	movl	a0@+,a1@
	movl	a0@+,a1@
	exg	a0,d0		| a0 = c . d0 = b .
	movl	a1@,a0@+
	movl	a1@,a0@+
	movw	#0x1002,a1@(-4)
	exg	a0,d1		| a0 = a . d1 = c .
	movl	a0@+,a1@
	movl	a0@,a1@
	movl	d0,a0		| a0 = b . d0 = b .
	movl	a0@+,a1@
	movl	a0@,a1@
	movl	d1,a0		| a0 = c . d1 = c .
	movl	a1@,a0@+
	movl	a1@,a0@
        .end

        .inline __Fz_minus,12
        movl    sp@+,d0         | d0 = c.
        movl    sp@+,a0         | a0 = a.
        movl    sp@+,d1         | d1 = b.
	movl    __skybase,a1
	movw	#0x1008,a1@(-4)
	movl	a0@+,a1@
	movl	a0@+,a1@
	exg	a0,d1		| a0 = b . d1 = a .
	movl	a0@+,a1@
	movl	a0@+,a1@
	exg	a0,d0		| a0 = c . d0 = b .
	movl	a1@,a0@+
	movl	a1@,a0@+
	movw	#0x1008,a1@(-4)
	exg	a0,d1		| a0 = a , d1 = c ,
	movl	a0@+,a1@
	movl	a0@,a1@
	movl	d0,a0		| a0 = b , d0 = b ,
	movl	a0@+,a1@
	movl	a0@,a1@
	movl	d1,a0		| a0 = c , d1 = c ,
	movl	a1@,a0@+
	movl	a1@,a0@
        .end

	.inline	__Fz_mult,12
	movl	sp@+,a0			| a0 gets address of result c.
	movl	sp@+,a1			| a1 gets address of operand a.
	movl	sp@+,d0			| d0 gets address of operand b.
	movl	a3,sp@-			| Save original a3 on stack.

	movl	__skybase,a3		| a3 gets skybase.
	movw	#0x100c,a3@(-4)		| d0/d1 get Im(a)*Im(b).
	movl	a1@(8),a3@
	movl	a2,sp@-			| Save original a2 on stack.
	movl	a1@(12),a3@
	movl	d0,a2			| a2 gets address of operand b.
	movl	a2@(8),a3@
	movl	a2@(12),a3@
1:	tstw	a3@(-2)
	jge	1b
	movl	a3@,d0
	movl	a3@,d1

	eorl	#0x80000000,d0		| d0/d1 gets -Im(a)*Im(b).

	movw	#0x1018,a3@(-4)		| Compute Re(a)*Re(b) - Im(a)*Im(b).
	movl	a1@,a3@
	movl	a1@(4),a3@
	movl	a2@,a3@
	movl	a2@(4),a3@
	movl	d0,a3@
	movl	d1,a3@
1:      tstw    a3@(-2)   
        jge     1b    
	movl	a3@,a0@+		| Store Re(c).
	movl	a3@,a0@+

	movw	#0x1034,a3@(-4)		| Reg 0 gets Re(a).
	movl	a1@+,a3@
	movl	a1@+,a3@		| a1 now points to Im(a).
1:	movw	a3@(-2),d0
	btst	#14,d0
	jeq	1b

	movw	#0x1010,a3@(-4)		| Reg 0 gets Reg 0 * Im(b).
	movl	a2@(8),a3@
	movl	a2@(12),a3@
1:	movw	a3@(-2),d0
	btst	#14,d0
	jeq	1b

	movw	#0x101c,a3@(-4)		| Reg 0 gets Im(a) * Re(b) + Reg 0.
	movl	a1@+,a3@
	movl	a1@,a3@
	movl	a2@+,a3@
	movl	a2@,a3@
1:	movw	a3@(-2),d0
	btst	#14,d0
	jeq	1b
 
	movw	#0x103a,a3@(-4)		| Store reg0 to Im(c).
	movl	a3@,a0@+
	movl	a3@,a0@

	movl	sp@+,a2			| Restore a3 and a2.
	movl	sp@+,a3
	.end
