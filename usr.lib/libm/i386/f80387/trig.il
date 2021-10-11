!
! @(#)trig.il 1.1 92/07/30 SMI
! Copyright (c) 1988 by Sun Microsystems, Inc.
!
! Warning: 
!	This inline expansion templates for 387 trig functions never
!	be used by default. As a matter of fact it is NOT recommended
!	unless the user is ABSOLUTELY certain that the input argument 
!	will NEVER exceed 2^63 in magnitude.  This is because the 387 
!	instructions fsin,fcos,ftan will return the SAME input x as 
!	answer if |x|>2^63 (horrible, isn't it).
!		

	.inline sin,2
	fldl    0(%esp) 
	fsin 
	.end
		
	.inline cos,2
	fldl    0(%esp) 
	fcos
	.end
									 
	.inline tan,2
	fldl    0(%esp) 
	fptan
	fstp	%st(0)
	.end

	.inline d_sin,1
	movl    0(%esp),%eax
	fldl	0(%eax)
	fsin
	.end

	.inline d_cos,1
	movl    0(%esp),%eax
	fldl	0(%eax)
	fcos
	.end

	.inline d_tan,1
	movl    0(%esp),%eax
	fldl	0(%eax)
	fptan
	fstp	%st(0)
	.end

	.inline r_sin,1
	movl    0(%esp),%eax
	flds	0(%eax)
	fsin
	.end

	.inline r_cos,1
	movl    0(%esp),%eax
	flds	0(%eax)
	fcos
	.end

	.inline r_tan,1
	movl    0(%esp),%eax
	flds	0(%eax)
	fptan
	fstp	%st(0)
	.end
