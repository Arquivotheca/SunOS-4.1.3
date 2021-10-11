
!	.asciz	"@(#)libm.il 1.1 92/07/30 SMI"

!	Copyright (c) 1989 by Sun Microsystems, Inc.
!	@(#)RELEASE libm.il, 4.1 alpha 9 

! x0 = 0x44	! shadow area of %o0
! x1 = 0x48	! shadow area of %o1
! x2 = 0x4c	! shadow area of %o2
! x3 = 0x50	! shadow area of %o3
! x4 = 0x54	! shadow area of %o4
! x5 = 0x58	! shadow area of %o5
! 
! List of inline expansion libm functions
! __swapEX, __swapRD, __swapRP, __swapTE
! _fp_class, _ir_fp_class, _copysign, _r_copysign_
! _finite,  _ir_finite_
! _signbit, _ir_signbit
! _isinf, _isnan, _isnormal, _issubnormal, _iszero
! _abs, _fabs, _r_fabs_
!! _irint, _ir_irint_
!
! _Facos, _Faint, _Fanint, _Fasin, _Fatan, _Fcos, _Fcosh, _Fexp, _Fexp10
! _Fexp2, _Flog, _Flog10, _Fnint, _Fsin, _Fsinh, _Ftan, _Ftanh
!
! ___infinity	 (request by POSIX)
! 
! List of inline expansion libF77 functions
! _i_nint, _i_dnnt 
!
! __Fc_minus, __Fc_add, __Fc_neg,
! __Ff_conv_c, __Fc_conv_f, __Fc_conv_i, __Fi_conv_c
! __Fc_conv_d, __Fd_conv_c
!
! __Fc_conv_z
! __Fz_minus, __Fz_add, __Fz_neg
! __Ff_conv_z, __Fz_conv_f, __Fz_conv_i, __Fi_conv_z
! __Fz_conv_d, __Fd_conv_z, __Fz_conv_c

! _c_abs, z_abs, _c_cmplx, _d_cmplx, _r_cnjg, _d_cnjg
! _r_dim, _d_dim, _r_imag, _d_imag, _r_sign, _d_sign

! __Fc_mult, __Fz_mult
! 

!	begin fsqrt[sd] inline expansion templates
!	remove if fsqrt[sd] instructions aren't in hardware

        .inline _Fsqrt,1
        st      %o0,[%sp+0x44]
        ld      [%sp+0x44],%f0
	fsqrts  %f0,%f0
	.end

        .inline _sqrt,2
        std     %o0,[%sp+0x48]          ! store to 8-aligned address
        ldd     [%sp+0x48],%f0
        fsqrtd  %f0,%f0
	.end

        .inline _r_sqrt_,1
        ld      [%o0],%f0
        fsqrts  %f0,%f0
	.end

        .inline _d_sqrt_,1
        ld      [%o0],%f0
        ld      [%o0+4],%f1
        fsqrtd  %f0,%f0
        .end

	.inline	_r_hypot_,2
	ld	[%o0],%o4
	sethi	0x1fffff,%o5
	or	%o5,1023,%o5
	and	%o4,%o5,%o4
	sethi	0x1fe000,%o3
	cmp	%o4,%o3
	bne	2f
	ld	[%o0],%f0	! load result with first argument
	ba	5f		! first argument is inf
	fabss	%f0,%f0
2:		
	ld	[%o1],%o4
	sethi	0x1fffff,%o5
	or	%o5,1023,%o5
	and	%o4,%o5,%o4
	sethi	0x1fe000,%o3
	cmp	%o4,%o3
	bne	4f
	nop
	ld	[%o1],%f0	! second argument inf
	ba	5f
	fabss	%f0,%f0
4:
	fstod	%f0,%f0
	ld	[%o1],%f3
	fmuld	%f0,%f0,%f0
	fstod	%f3,%f2
	fmuld	%f2,%f2,%f2
	faddd	%f2,%f0,%f0
	fsqrtd	%f0,%f0
	fdtos	%f0,%f0
5:
	.end

	.inline	_c_abs,1
	ld	[%o0],%o4
	sethi	0x1fffff,%o5
	or	%o5,1023,%o5
	and	%o4,%o5,%o4
	sethi	0x1fe000,%o3
	cmp	%o4,%o3
	bne	2f
	ld	[%o0],%f0
	ba	5f
	fabss	%f0,%f0
2:		
	ld	[%o0+4],%o4
	sethi	0x1fffff,%o5
	or	%o5,1023,%o5
	and	%o4,%o5,%o4
	sethi	0x1fe000,%o3
	cmp	%o4,%o3
	bne	4f
	nop
	ld	[%o0+4],%f0
	ba	5f
	fabss	%f0,%f0
! store to 8-aligned address
4:
	fstod	%f0,%f0
	ld	[%o0+4],%f3
	fmuld	%f0,%f0,%f0
	fstod	%f3,%f2
	fmuld	%f2,%f2,%f2
	faddd	%f2,%f0,%f0
	fsqrtd	%f0,%f0
	fdtos	%f0,%f0
5:
	.end

!	remove if fsqrt[sd] instructions aren't in hardware
!	end fsqrt[sd] inline expansion templates

! alternate definition for those who don't want fsqrt instructions
	.inline	_c_abs,1
	add	%o0,4,%o1
	call	_r_hypot_,2
	nop
	.end

	.inline	_min_subnormal,0
	set	0x0,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_min_subnormal_,0
	set	0x0,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_min_subnormal_,0
	set	0x1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_max_subnormal,0
	set	0x000fffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_max_subnormal_,0
	set	0x000fffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_max_subnormal_,0
	set	0x007fffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_min_normal,0
	set	0x00100000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x0,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_min_normal_,0
	set	0x00100000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x0,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_min_normal_,0
	set	0x00800000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_max_normal,0
	set	0x7fefffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_max_normal_,0
	set	0x7fefffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_max_normal_,0
	set	0x7f7fffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	___infinity,0
        set      0x7ff00000,%o0
        set      0x0,%o1
        std      %o0,[%sp+0x48]
        ldd      [%sp+0x48],%f0
        .end

	.inline	_infinity,0
        set      0x7ff00000,%o0
        set      0x0,%o1
        std      %o0,[%sp+0x48]
        ldd      [%sp+0x48],%f0
        .end

	.inline	_d_infinity_,0
	set	0x7ff00000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x0,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_infinity_,0
	set	0x7f800000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_signaling_nan,0
	set	0x7ff00000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_signaling_nan_,0
	set	0x7ff00000,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0x1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_signaling_nan_,0
	set	0x7f800001,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_quiet_nan,0
	set	0x7fffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_d_quiet_nan_,0
	set	0x7fffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	set	0xffffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f1
	.end

	.inline	_r_quiet_nan_,0
	set	0x7fffffff,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	__swapEX,1
	set     _fp_accrued_exceptions,%o3
	st      %o0,[%o3]
	and     %o0,0x1f,%o1
	sll     %o1,5,%o1               ! shift input to aexc bit location
	st      %fsr,[%sp+0x44]
	ld      [%sp+0x44],%o0          ! o0 = fsr
	andn    %o0,0x3e0,%o2
	or      %o1,%o2,%o1             ! o1 = new fsr
	st      %o1,[%sp+0x44]
	ld      [%sp+0x44],%fsr
	srl     %o0,5,%o0
	and     %o0,0x1f,%o0
	.end

	.inline	__swapRD,1
	set     _fp_direction,%o3
	and	%o0,0x3,%o0
	st      %o0,[%o3]
	sll     %o0,30,%o1              ! shift input to RD bit location
	st      %fsr,[%sp+0x44]
	ld      [%sp+0x44],%o0          ! o0 = fsr
	set     0xc0000000,%o4          ! mask of rounding direction bits
	andn    %o0,%o4,%o2
	or      %o1,%o2,%o1             ! o1 = new fsr
	st      %o1,[%sp+0x44]
	ld      [%sp+0x44],%fsr
	srl     %o0,30,%o0
	and     %o0,0x3,%o0
	.end

	.inline	__swapRP,1
	set     _fp_precision,%o3
	and	%o0,0x3,%o0
	st      %o0,[%o3]
	sll     %o0,28,%o1              ! shift input to RP bit location
	st      %fsr,[%sp+0x44]
	ld      [%sp+0x44],%o0          ! o0 = fsr
	set     0x30000000,%o4          ! mask of rounding precision bits
	andn    %o0,%o4,%o2
	or      %o1,%o2,%o1             ! o1 = new fsr
	st      %o1,[%sp+0x44]
	ld      [%sp+0x44],%fsr
	srl     %o0,28,%o0
	and     %o0,0x3,%o0
	.end

	.inline	__swapTE,1
	and	%o0,0x1f,%o0
	sll     %o0,23,%o1              ! shift input to TEM bit location
	st      %fsr,[%sp+0x44]
	ld      [%sp+0x44],%o0            ! o0 = fsr
	set     0x0f800000,%o4          ! mask of TEM (Trap Enable Mode bits)
	andn    %o0,%o4,%o2
	or      %o1,%o2,%o1             ! o1 = new fsr
	st      %o1,[%sp+0x48]
	ld      [%sp+0x48],%fsr
	srl     %o0,23,%o0
	and     %o0,0x1f,%o0
	.end

	.inline	_fp_class,2
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	orcc	%o0,%o1,%g0
	bne	1f
	sethi	%hi(0x7ff00000),%o2
	ba	2f			! x is 0
	mov	0,%o0
1:	andcc	%o0,%o2,%g0
	bne	1f
	cmp	%o0,%o2
	ba	2f			! x is subnormal
	mov	1,%o0
1:	bge	1f
	andn	%o0,%o2,%o0
	ba	2f			! x is normal
	mov	2,%o0
1:	orcc	%o0,%o1,%g0
	bne	1f
	sethi	%hi(0x00080000),%o2
	ba	2f			! x is infinity
	mov	3,%o0
1:	andcc	%o0,%o2,%g0
	bne	2f
	mov	4,%o0			! x is quiet NaN
	mov	5,%o0			! x is signaling NaN
2:
	.end

	.inline	_ir_fp_class,1
	ld	[%o0],%o0
	sethi	%hi(0x80000000),%o2
	andncc	%o0,%o2,%o0
	bne	1f
	sethi	%hi(0x7f800000),%o2
	ba	2f			! x is 0
	mov	0,%o0
1:	andcc	%o0,%o2,%g0
	bne	1f
	cmp	%o0,%o2
	ba	2f			! x is subnormal
	mov	1,%o0
1:	bge	1f
	andn	%o0,%o2,%o0
	ba	2f			! x is normal
	mov	2,%o0
1:	bg	1f
	sethi	%hi(0x00400000),%o2
	ba	2f			! x is infinity
	mov	3,%o0
1:	andcc	%o0,%o2,%g0
	bne	2f
	mov	4,%o0			! x is quiet NaN
	mov	5,%o0			! x is signaling NaN
2:
	.end

	.inline	_copysign,4
        set     0x80000000,%o3
        and     %o2,%o3,%o2
        andn    %o0,%o3,%o0
        or      %o0,%o2,%o0
        std      %o0,[%sp+0x48]
        ldd     [%sp+0x48],%f0
        .end

	.inline	_r_copysign_,2
	ld	[%o0],%o0
	ld	[%o1],%o1
	set	0x80000000,%o2
	andn	%o0,%o2,%o0
	and	%o1,%o2,%o1
	or	%o0,%o1,%o0
	st	%o0,[%sp+0x44]
	ld	[%sp+0x44],%f0
	.end

	.inline	_finite,2
	set	0x7ff00000,%o1
	and	%o0,%o1,%o0
	cmp	%o0,%o1
	bne	1f
	mov	1,%o0
	mov	0,%o0
1:
	.end

	.inline	_ir_finite_,1
	ld	[%o0],%o0
	set	0x7f800000,%o1
	and	%o0,%o1,%o0
	cmp	%o0,%o1
	bne	1f
	mov	1,%o0
	mov	0,%o0
1:
	.end

	.inline	_signbit,1
	srl	%o0,31,%o0
	.end

	.inline	_ir_signbit_,1
	ld	[%o0],%o0
	srl	%o0,31,%o0
	.end

	.inline	_ir_isinf_,1		! used in r_hypot_
	ld	[%o0],%o0		
	sethi	%hi(0x7fffffff),%o1
	or	%o1,%lo(0x7fffffff),%o1	! [internal]
	and	%o0,%o1,%o0
	sethi	%hi(0x7f800000),%o3
	cmp	%o0,%o3
	bne,a	1f
	mov	0,%o0
	mov	1,%o0
1:
	.end

	.inline	_isinf,2
	tst	%o1
	bne	1f
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	sethi	%hi(0x7ff00000),%o2
	cmp	%o0,%o2
	be	2f
	mov	1,%o0
1:
	mov	0,%o0
2:
	.end

	.inline	_isnan,2
	sethi	%hi(0x7ff00000),%o2
	and	%o0,%o2,%o3
	cmp	%o3,%o2
	bne	1f
	sethi	%hi(0xfff00000),%o2
	andn	%o0,%o2,%o0
	orcc	%o0,%o1,%g0
	bne	2f
	mov	1,%o0
1:
	mov	0,%o0
2:
	.end

	.inline	_isnormal,2
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	sethi	%hi(0x7ff00000),%o2
	cmp	%o0,%o2
	bge	1f
	sethi	%hi(0x00100000),%o2
	cmp	%o0,%o2
	bge	2f
	mov	1,%o0
1:
	mov	0,%o0
2:
	.end

	.inline	_issubnormal,2
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	sethi	%hi(0x00100000),%o2
	cmp	%o0,%o2
	bge	1f
	orcc	%o0,%o1,%g0
	bne	2f
	mov	1,%o0
1:
	mov	0,%o0
2:
	.end

	.inline	_iszero,2
	sethi	%hi(0x80000000),%o2
	andn	%o0,%o2,%o0
	orcc	%o0,%o1,%g0
	be	1f
	mov	1,%o0
	mov	0,%o0
1:
	.end

        .inline _abs,1          /* integer abs */
        subcc   %g0,%o0,%g0     /* compute 0-o0 */
        ble     1f              /* nop if 0 <= o0 */
        nop
        sub     %g0,%o0,%o0     /* o0 := -o0 */
1:
        .end

	.inline	_fabs,2
        sll     %o0,1,%o0
        srl     %o0,1,%o0
        std     %o0,[%sp+0x48]
        ldd     [%sp+0x48],%f0
	.end

	.inline	_r_fabs_,1
	ld	[%o0],%f0
	fabss	%f0,%f0
	.end

!
! _Facos, _Faint, _Fanint, _Fasin, _Fatan, _Fcos, _Fcosh, _Fexp, _Fexp10
! _Fexp2, _Flog, _Flog10, _Fnint, _Fsin, _Fsinh, _Ftan, _Ftanh
!
	.inline	_Facos,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_acos_,1
	nop
	.end

	.inline	_Faint,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_aint_,1
	nop
	.end

	.inline	_Fanint,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_anint_,1
	nop
	.end

	.inline	_Fasin,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_asin_,1
	nop
	.end

	.inline	_Fatan,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_atan_,1
	nop
	.end

	.inline	_Fcos,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_cos_,1
	nop
	.end

	.inline	_Fcosh,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_cosh_,1
	nop
	.end

	.inline	_Fexp,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_exp_,1
	nop
	.end

	.inline	_Fexp10,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_exp10_,1
	nop
	.end

	.inline	_Fexp2,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_exp2_,1
	nop
	.end

	.inline	_Flog,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_log_,1
	nop
	.end

	.inline	_Flog10,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_log10_,1
	nop
	.end

	.inline	_Fnint,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_ir_nint_,1
	nop
	.end

	.inline	_Fsin,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_sin_,1
	nop
	.end

	.inline	_Fsinh,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_sinh_,1
	nop
	.end

	.inline	_Ftan,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_tan_,1
	nop
	.end

	.inline	_Ftanh,1
	st	%o0,[%sp+0x44]
	add	%sp,0x44,%o0
	call	_r_tanh_,1
	nop
	.end


!
! libF77 _i_nint and _i_dnnt
!
	.inline	_i_nint,1
	call	_ir_nint_,1
	nop
	.end

	.inline	_i_dnnt,1
	ld	[%o0+4],%o1
	ld	[%o0],%o0
	call	_nint,2
	nop
	.end

	.inline	__Fz_minus,3
	ld	[%o1],%f0
	ld	[%o1+0x4],%f1
	ld	[%o2],%f4
	ld	[%o2+0x4],%f5
	fsubd	%f0,%f4,%f0
	ld	[%o1+8],%f2
	ld	[%o1+0xc],%f3
	ld	[%o2+8],%f6
	ld	[%o2+0xc],%f7
	fsubd	%f2,%f6,%f2
	st	%f0,[%o0+0x0]
	st	%f1,[%o0+0x4]
	st	%f2,[%o0+0x8]
	st	%f3,[%o0+0xc]
	.end

	.inline	__Fz_add,3
	ld	[%o1],%f0
	ld	[%o1+0x4],%f1
	ld	[%o2],%f4
	ld	[%o2+0x4],%f5
	faddd	%f0,%f4,%f0
	ld	[%o1+8],%f2
	ld	[%o1+0xc],%f3
	ld	[%o2+8],%f6
	ld	[%o2+0xc],%f7
	faddd	%f2,%f6,%f2
	st	%f0,[%o0+0x0]
	st	%f1,[%o0+0x4]
	st	%f2,[%o0+0x8]
	st	%f3,[%o0+0xc]
	.end

	.inline	__Fz_neg,2
	ld	[%o1],%f0
	fnegs	%f0,%f0
	ld	[%o1+0x4],%f1
	st	%f1,[%o0+0x4]
	ld	[%o1+8],%f2
	fnegs	%f2,%f2
	ld	[%o1+0xc],%f3
	st	%f3,[%o0+0xc]
	st	%f0,[%o0]
	st	%f2,[%o0+0x8]
	.end

	.inline	__Ff_conv_z,2
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%f0
	fstod	%f0,%f0
	st	%g0,[%o0+0x8]
	st	%g0,[%o0+0xc]
	st	%f1,[%o0+0x4]
	st	%f0,[%o0]
	.end

	.inline	__Fz_conv_f,1
	ld	[%o0],%f0
	ld	[%o0+4],%f1
	fdtos	%f0,%f0
	.end

	.inline	__Fz_conv_i,1
	ld	[%o0],%f0
	ld	[%o0+4],%f1
	fdtoi	%f0,%f0
	st	%f0,[%sp+0x44]
	ld	[%sp+0x44],%o0
	.end

	.inline	__Fi_conv_z,2
	st	%o1,[%sp+0x44]
	ld	[%sp+0x44],%f0
	fitod	%f0,%f0
	st	%g0,[%o0+0x8]
	st	%g0,[%o0+0xc]
	st	%f1,[%o0+0x4]
	st	%f0,[%o0]
	.end

	.inline	__Fz_conv_d,1
	ld	[%o0],%f0
	ld	[%o0+4],%f1
	.end

	.inline	__Fd_conv_z,3
	st	%o1,[%o0]
	st	%o2,[%o0+0x4]
	st	%g0,[%o0+0x8]
	st	%g0,[%o0+0xc]
	.end

	.inline	__Fz_conv_c,1
	ld	[%o0],%f0
	ld	[%o0+4],%f1
	fdtos	%f0,%f0
	ld	[%o0+0x8],%f2
	ld	[%o0+0xc],%f3
	fdtos	%f2,%f1
	.end

	.inline	_z_abs,1
	ld	[%o0+0xc],%o3
	ld	[%o0+0x8],%o2
	ld	[%o0+0x4],%o1
	ld	[%o0],%o0
	call	_hypot,4
	nop
	.end

	.inline	_c_cmplx,3
	ld	[%o1],%o1
	st	%o1,[%o0]
	ld	[%o2],%o2
	st	%o2,[%o0+4]
	.end

	.inline	_d_cmplx,3
	ld	[%o1],%f0
	st	%f0,[%o0]
	ld	[%o1+4],%f1
	st	%f1,[%o0+4]
	ld	[%o2],%f0
	st	%f0,[%o0+0x8]
	ld	[%o2+4],%f1
	st	%f1,[%o0+0xc]
	.end

	.inline	_r_cnjg,2
	ld	[%o1+0x4],%f1
	fnegs	%f1,%f1
	ld	[%o1],%f0
	st	%f0,[%o0]
	st	%f1,[%o0+4]
	.end

	.inline	_d_cnjg,2
	ld	[%o1+0x8],%f0
	fnegs	%f0,%f0
	ld	[%o1+0xc],%f1
	st	%f1,[%o0+0xc]
	ld	[%o1+0x0],%f1
	st	%f1,[%o0+0x0]
	ld	[%o1+0x4],%f1
	st	%f1,[%o0+0x4]
	st	%f0,[%o0+0x8]
	.end

	.inline	_r_dim,2
	ld	[%o0],%f2
	ld	[%o1],%f4
	fcmps	%f2,%f4
	st	%g0,[%sp+0x48]
	ld	[%sp+0x48],%f0
	fbule	1f
	nop
	fsubs	%f2,%f4,%f0
1:
	.end

	.inline	_d_dim,2
	ld	[%o0],%f2
	ld	[%o0+4],%f3
	ld	[%o1],%f4
	ld	[%o1+4],%f5
	fcmpd	%f2,%f4
	st	%g0,[%sp+0x48]
	ld	[%sp+0x48],%f0
	ld	[%sp+0x48],%f1
	fbule	1f
	nop
	fsubd	%f2,%f4,%f0
1:
	.end

	.inline	_r_imag,1
	ld	[%o0+4],%f0
	.end

	.inline	_d_imag,1
	ld	[%o0+8],%f0
	ld	[%o0+0xc],%f1
	.end

	.inline	_r_sign,2
	ld	[%o0],%f0
	fabss	%f0,%f0
	ld	[%o1],%o2
	sethi	%hi(0x80000000),%o3
	cmp	%o2,%o3
	beq	1f
	tst	%o2
	bge	1f
	nop
	fnegs	%f0,%f0
1:
	.end

	.inline	_d_sign,2
	ld	[%o0],%f0
	fabss	%f0,%f0
	ld	[%o0+4],%f1
	ld	[%o1],%o2
	ld	[%o1+4],%o3
	sethi	%hi(0x80000000),%o4
	andn	%o2,%o4,%o4
	orcc	%o3,%o4,%g0
	beq	1f
	tst	%o2
	bge	1f
	nop
	fnegs	%f0,%f0
1:
	.end

	.inline	__Fz_mult,3
	ld	[%o1],%f0
	ld	[%o1+0x4],%f1
	ld	[%o2],%f4
	ld	[%o2+0x4],%f5
	fmuld	%f0,%f4,%f8	! f8 = r1*r2
	ld	[%o1+0x8],%f2
	ld	[%o1+0xc],%f3
	ld	[%o2+0x8],%f6
	ld	[%o2+0xc],%f7
	fmuld	%f2,%f6,%f10	! f10= i1*i2
	fsubd	%f8,%f10,%f12	! f12= r1*r2-i1*i2
	st	%f12,[%o0]	
	st	%f13,[%o0+4]	
	fmuld	%f0,%f6,%f14	! f14= r1*i2
	fmuld	%f2,%f4,%f16	! f16= r2*i1
	faddd	%f14,%f16,%f2	! f2 = r1*i2+r2*i1
	st	%f2,[%o0+8]	
	st	%f3,[%o0+12]	
	.end

!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! __Fc_mult(c, a, b)
! complex *c, *a, *b;		/* c = a * b */
! {
	.inline	__Fc_mult,3
!    21	  	c->real = (a->real *  b->real) - (a->imag *  b->imag);
	ld	[%o1+4],%f0
	ld	[%o2+4],%f1
	fstod	%f0,%f2		! f2 = a->imag
	fstod	%f1,%f4		! f4 = b->imag
	fmuld	%f2,%f4,%f6	! f6 = (a->imag *  b->imag)
	ld	[%o1],%f0
	ld	[%o2],%f1
	fstod	%f0,%f8		! f8 = a->real
	fstod	%f1,%f10	! f10 = b->real
	fmuld	%f8,%f10,%f0
	fsubd	%f0,%f6,%f6
!    22	  	c->imag = (a->real *  b->imag) + (a->imag *  b->real);
	fmuld	%f2,%f10,%f2	! f2 = a->imag * b->real
	fmuld	%f8,%f4,%f4	! f4 = a->real * b->imag
	faddd	%f2,%f4,%f4
	fdtos	%f6,%f0
	fdtos	%f4,%f1
	st	%f0,[%o0]
	st	%f1,[%o0+4]
	.end


!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! __Fc_minus(c, a, b)
! complex *c, *a, *b;		/* c = a - b */
! {
	.inline	__Fc_minus,3
!    30	 	c->real = a->real - b->real;
	ld	[%o1],%f0
	ld	[%o2],%f1
	fsubs	%f0,%f1,%f2
!    31	  	c->imag = a->imag - b->imag;
	ld	[%o1+4],%f3
	ld	[%o2+4],%f4
	fsubs	%f3,%f4,%f5
	st	%f2,[%o0]
	st	%f5,[%o0+4]
	.end


!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Fc_add(c, a, b)
! complex *c, *a, *b;		/* c = a + b */
! {
	.inline	__Fc_add,3
!    39	 	c->real = a->real + b->real;
	ld	[%o1],%f0
	ld	[%o2],%f1
	fadds	%f0,%f1,%f2
!    40	 	c->imag = a->imag + b->imag;
	ld	[%o1+4],%f3
	ld	[%o2+4],%f4
	fadds	%f3,%f4,%f5
	st	%f2,[%o0]
	st	%f5,[%o0+4]
	.end


!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Fc_neg(c, a)
! complex *c, *a;		/* c = -a */
! {
	.inline	__Fc_neg,2
!    48	  	c->real = - a->real;
	ld	[%o1],%f0
	fnegs	%f0,%f1
!    49	  	c->imag = - a->imag;
	ld	[%o1+4],%f2
	fnegs	%f2,%f3
	st	%f1,[%o0]
	st	%f3,[%o0+4]
	.end


!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Ff_conv_c(c, x)	/* convert float to complex */
! complex *c;
! FLOATPARAMETER x;
! {
	.inline	__Ff_conv_c,2
!    59		c->real = x;
	st	%o1,[%o0]
!    60		c->imag = 0.0;
	st	%g0,[%o0+4]
	.end



!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! FLOATFUNCTIONTYPE
! _Fc_conv_f( c )		/* Convert complex to float */
! complex *c;
! {
	.inline	__Fc_conv_f,1
!    69  	RETURNFLOAT(c->real);
	ld	[%o0],%f0
	.end


!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! int
! _Fc_conv_i (c)			/* Convert complex to int */
! complex *c;
! {
	.inline	__Fc_conv_i,1
!    78		return (int)c->real;
	ld	[%o0],%f0
	fstoi	%f0,%f1
	st	%f1,[%sp+68]
	ld	[%sp+68],%o0
	.end

	 
!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void     
! _Fi_conv_c (c,i)		/* Convert int to complex */
! complex *c;
! int i;   
! {        
	.inline	__Fi_conv_c,2
!    88		c->real = (float)i;
	st	%o1,[%sp+68]
	ld	[%sp+68],%f0
	fitos	%f0,%f1
	st	%f1,[%o0]
!    89		c->imag = 0.0;
	st	%g0,[%o0+4]
	.end

 
!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! double
! _Fc_conv_d ( c )		/* Convert complex to double */
! complex *c;
! {
	.inline	__Fc_conv_d,1
!    98		return (double)c->real;
	ld	[%o0],%f2
	fstod	%f2,%f0
	.end

 
!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Fd_conv_c (c, x)		/* Convert double to complex */
! complex *c;
! double x;
	.inline	__Fd_conv_c,2
	st	%o1,[%sp+72]
	st	%o2,[%sp+76]
!   109		c->real = (float)(x);
	ldd	[%sp+72],%f0
	fdtos	%f0,%f1
	st	%f1,[%o0]
!   110		c->imag = 0.0;
	st	%g0,[%o0+4]
	.end

 
!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Fc_conv_z( result, c )	/* Convert complex to double complex */
! dcomplex *result;
! complex *c;
! {
	.inline	__Fc_conv_z,2
!   120		result->dreal = (double)c->real;
	ld	[%o1],%f0
	fstod	%f0,%f2
	st	%f2,[%o0]
	st	%f3,[%o0+4]
!   121		result->dimag = (double)c->imag;
	ld	[%o1+4],%f3
	fstod	%f3,%f4
	st	%f4,[%o0+8]
	st	%f5,[%o0+12]
	.end

!- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
! void
! _Fc_div(c, a, b)
! complex *c, *a, *b;		/* c = a * b */
! {
	.inline	__Fc_div,3
	ld	[%o2+4],%o3
	sethi	%hi(0x7fffffff),%o4
	or	%o4,%lo(0x7fffffff),%o4	! [internal]
	andcc	%o3,%o4,%g0
	bne,a	1f
	ld	[%o2],%f6		! f6 gets reb
	ld	[%o1],%f0
	ld	[%o2],%f1
	fdivs	%f0,%f1,%f0
	st	%f0,[%o0]
	ld	[%o2],%f4
	ld	[%o1+4],%f3
	fdivs	%f3,%f4,%f3
	b	2f
	st	%f3,[%o0+4]
1:					! [internal]
	fstod	%f6,%f8			! f8/9 gets reb
	ld	[%o2+4],%f19		! f19 gets imb
	ld	[%o1+4],%f13		! f13 gets ima
	fstod	%f13,%f24		! f24/5 gets ima
	fstod	%f19,%f10		! f10/11 gets imb
	fmuld	%f8,%f8,%f16		! f16/17 gets reb**2 
	ld	[%o1],%f19		! f19 gets rea
	fmuld	%f24,%f10,%f0		! f0/f1 gets ima*imb
	fstod	%f19,%f26		! f26/7 gets rea
	fmuld	%f10,%f10,%f12		! f12/13 gets imb**2
	faddd	%f12,%f16,%f12		! f12/13 gets reb**2+imb**2
	fmuld	%f26,%f8,%f2		! f2/3 gets rea*reb
	faddd	%f2,%f0,%f2		! f2/3 gets rea*reb+ima*imb
	fdivd	%f2,%f12,%f2		! f2/3 gets imc
	fmuld	%f24,%f8,%f24		! f24/5 gets ima*reb
	fmuld	%f26,%f10,%f10		! f10/11 gets rea*imb
	fsubd	%f24,%f10,%f10		! f10/11 gets ima*reb-rea*imb
	fdtos	%f2,%f7			! f7 gets imc
	fdivd	%f10,%f12,%f12		! f12 gets rec
	fdtos	%f12,%f15		! f15 gets rec
	st	%f7,[%o0]
	st	%f15,[%o0+4]
2:
	.end
