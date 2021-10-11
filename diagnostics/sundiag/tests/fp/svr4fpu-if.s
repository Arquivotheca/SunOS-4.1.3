/* static char  frefsccsid[] = "@(#)svr4fpu-if.s 1.1 7/30/92 Copyright Sun Microsystems"; */
!=============================================================================
! 	File: fpu.s					Date:
!=============================================================================
!
!  Objective:
!
!	This module provides an interface between diagnostic application
!	programs and the Floating Point Co-Processor.
!
!	Each routine is callable from C and is responsible for setting
!	up the floating point registers, moving parameters, initiating
!	the co-processor operation and moving the results into the 
!	correct registers.
!
! Open Issues:
!
!	1) Double precision values are passed as 32 bit (single)
!	2) Extended precision operations are Invalid Operations!.
!
! History:
!
!	BDS  04/28/88  	Adding comments to source.
!			Revising code for compatability with TI & Weitek.
!			Restructuring into an interface module.
!			.. Test code is now in fpu-test.s
!
!===========================================================================
	.seg    ".data"
	.align	8
temp:	.skip 4			/* storage location for a single precision */
temp1:	.skip 4			/* storage for double precision */
temp2:	.skip 4			/* storage for extended */

	.seg	".text"
	.global	int_float_s
	.global int_float_d
	.global float_int_s
	.global float_int_d
	.global convert_sp_dp
	.global convert_dp_sp
	.global negate_value
	.global absolute_value
	.global square_sp 
	.global square_dp
	.global add_sp
	.global add_dp
	.global sub_sp
	.global sub_dp
	.global mult_sp
	.global mult_dp
	.global div_sp
	.global div_dp
!*	.global set_psr 
	.global get_psr 
	.global set_fsr 
	.global get_fsr
	.global cmp_s
	.global cmp_d
	.global int_float_e
	.global cmp_s_ex
	.global cmp_d_ex

!**************************************************************************
!*  			PSR & FSR Register Interface			 *
!**************************************************************************

!--------------------------------------------------------------------------
! Name:		Get Program Status Register
! Function:	return a copy of the psr to the user
! Calling:	none
! Returns:	i0 = psr contents
! Convention:	psr_value = get_psr() ** 
!--------------------------------------------------------------------------
get_psr:
	save	%sp, -88, %sp	! save the registers & stack frame
	mov	%psr, %i0	! .. load the PSR
	nop			! .. delay
	nop			! .. delay
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Get the Floating point Status Register
! Function:	return a copy of the FSR to caller
! Calling:	none
! Returns:	i0 = fsr contents
! Convention:	fsr_value = get_fsr() ** 
!--------------------------------------------------------------------------
get_fsr:
	save	%sp, -88, %sp	! save the registers & stack frame
	set	temp2, %l0	! .. set the address of the result holder
	st	%fsr, [%l0]	! .. set the contents of the FSR register
	ld	[%l0], %i0	! .. return the fsr to caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Set Floating point Status Register
! Function:	Set the FSR
! Calling:	i0 = value to write to fsr
! Returns:	none
! Convention:	set_fsr(get_fsr() ** || 0x1001) ** 
!--------------------------------------------------------------------------
set_fsr:
	save	%sp, -88, %sp	! save the registers & stack frame
	set	temp2, %l0 	! .. set the address of the result holder
	st	%i0, [%l0]	! .. save the value in memory
	ld	[%l0], %fsr	! .. get the contents of the FSR register
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!**************************************************************************
!*			Data Conversion Functions			 *
!**************************************************************************

!--------------------------------------------------------------------------
! Name:		Integer to Float (Single)
! Function:	Convert an integer value to a single precision floating point 
!		value
! Calling:	in0 = value to convert
! Returns:	in0 = converted value
! Convention:	Real = int_float_s(Int) ** 
!--------------------------------------------------------------------------
int_float_s:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %o5	! .. set the address of the result holder
	set	temp, %o4	! .. set address of temp. mem reg
	st	%i0, [%o4]	! .. put the passed value into memory
	ld	[%o4], %f0	! .. get the value from memory into FPU register
	fitos   %f0, %f2	! .. get the integer into float into fpu r1
	st	%f2, [%o5]	! .. store into the location
	ld	[%o5], %i0	! .. put the value for return
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Integer to Float (double)
! Function:	Convert an integer value to a double precision floating point 
!		value
! Calling:	in0 = value to convert
! Returns:	in0 = converted value
! Convention:	Real = int_float_d(Int) ** 
!--------------------------------------------------------------------------
int_float_d:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %o5	! .. get the address of temp2
	set	temp, %o4	! .. get the address of temp
	st	%i0, [%o4]	! .. get the user value
	ld	[%o4], %f0	! .. into the float register
	fitod   %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%o5]	! .. save the result
	ld	[%o5], %i0	! .. and return it to caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Integer to Float Exception 
! Function:	This routine is to create the exception for creating an 
! Warning:	** UNIMPLEMENTED INSTRUCTION TRAP (EXTENDED PRECISION) ** **
! Calling:	
! Returns:	
! Convention:	Q) ** Is fitox implemented or not ?
!--------------------------------------------------------------------------
int_float_e:
	save	%sp, -88, %sp   ! save the registers, stack
	set	temp2, %o5	! .. Get the address of temp2
	set	temp, %o4	! .. Get the address of temp
	st	%i0, [%o4]	! .. Get the users value
	ld	[%o4], %f0	! .. into the float register
	fitox	%f0, %f4	! .... create an unimplemented instruction
	st	%fsr, [%o5]	! .. return the fsr value 
	ld	[%o5], %i0	! to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		float to integer (single)
! Function:	Convert a real value to an integer
! Calling:	in0 = Value
! Returns:	in0 = Value
! Convention:	Int = float_int_s(real) ** 
!--------------------------------------------------------------------------
float_int_s:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %o5	! .. get the address of temp2
	set	temp, %o4	! .... and temp
	st	%i0, [%o4]	! .. get the users value
	ld	[%o4], %f0	! .. into the float register
	fstoi   %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%o5]	! .. save the result
	ld	[%o5], %i0	! .. and return it to the user
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Float to Integer conversion (double)
! Function:	Convert a real value to an integer
! Calling:	in0 = value
! Returns:	in0 = value
! Convention:	Int = float_int_d(real) ** 
!--------------------------------------------------------------------------
float_int_d:
	save	%sp, -88, %sp	! save the registers, stack 
	set	temp2, %o5 	! .. get the address of temp2
	set	temp, %o4 	! .. and temp
	std     %i0, [%o4] 	! .. get the callers value
	ldd     [%o4], %f0 	! .. into the float register
	fdtoi   %f0, %f2 	! .... have the fpu perform the operation
	std     %f2, [%o5] 	! .. save the result
	ldd     [%o5], %i0 	! .... and return it to caller
	ret 			! Delayed return (get user ret addr)
	restore 		! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Convert Single to double precision
! Function:	<as the name says>
! Calling:	in0 = value
! Returns:	in0 = result
! Convention:	result = convert_sp_dp(value) ** 
!--------------------------------------------------------------------------
convert_sp_dp:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp, %l1	! .. get the address of temp
	st	%i0, [%l0]	! .. get the callers value
	ld	[%l0], %f0	! .. into the float register
	fstod   %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%l1]	! .. save the result
	ld	[%l1], %i0	! .... and return it to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Convert Double to Single precision
! Function:	..
! Calling:	in0 = double precision value
! Returns:	in0 = result
! Convention:	result = convert_dp_sp(value) ** 
!--------------------------------------------------------------------------
convert_dp_sp:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp, %l1	! .. and temp
	st	%i0, [%l0]	! .. get the users value
	ld	[%l0], %f0	! .. move it to a float register
	fdtos	%f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%l1]	! .. save the result
	ld	[%l1], %i0	! .... and return it to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Convert Single Precision to Extended Precision
! Function:	<see above>
! Warning:	** UNIMPLEMENTED INSTRUCTION TRAP (EXTENDED PRECISION) ** 
! Calling:	in0 = number to convert
! Returns:	in0 = extended result (64bits)
! Convention:	
!--------------------------------------------------------------------------
convert_sp_ext:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! point to the temp storage
	set	temp, %l1	! .. the result temp
	st	%i0, [%l0]	! .. copy the .data to temp memory
	ld	[%l0], %f0	! .... load from memory to FPU
	fstox	%f0, %f4	! .... have the fpu perform the operation
	st	%f4, [%l1]	! .. copy the 32 bit result into memory
	ld	[%l1], %i0	! then copy to the return registers
	ret 			! .. return to caller
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Negate a value
! Function:	Compliments the Sign bit
! Calling:	in0 = number to cross her
! Returns:	in0 = result
! Convention:	result = negate_value(value) ** 
!--------------------------------------------------------------------------
negate_value:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp, %l1	! .. and of temp
	st	%i0, [%l0]	! .. get the callers value
	ld	[%l0], %f0	! .. into the float register
	fnegs   %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%l1]	! .. save the result
	ld	[%l1], %i0 	! .... and return it to the caller
	ret 			! Delayed return (get user ret addr)
	restore 		! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Absolute Value
! Function:	Convert a value to its absolute value (clears sign bit)
! Calling:	in0 = value
! Returns:	in0 = result
! Convention:	result = absolute_value(value) ** 
!--------------------------------------------------------------------------
absolute_value:
	save	%sp, -88, %sp	! save the registers, stack 
	set	temp2, %l0 	! .. get the address of temp2
	set	temp, %l1 	! .. and temp
	st	%i0, [%l0] 	! .. get the users value
	ld	[%l0], %f0 	! .. into a float register
	fabss	%f0, %f2 	! .... have the fpu perform the operation
	st	%f2, [%l1] 	! .. save the result
	ld	[%l1], %i0  	! .... and return it to caller
	ret  			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!**************************************************************************
!*				Arithmetic Functions			 *
!**************************************************************************

!--------------------------------------------------------------------------
! Name:		Square Single
! Function:	Calculate the Square of a Single precision value
! Calling:	in0 = value
! Returns:	in0 = result
! Convention:	result = square_sp(value) ** 
!--------------------------------------------------------------------------
square_sp:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp, %l1	! .. and temp
	st	%i0, [%l0]	! .. get the callers value
	ld	[%l0], %f0	! .. into the float register
	fsqrts  %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%l1]	! .. save the result
	ld	[%l1], %i0	! .... and return it to caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Square (double)
! Function:	Calculate the Square of a double precision value
! Calling:	in0 = value
! Returns:	in0 = result
! Convention:	result = square_dp(value) ** 
!--------------------------------------------------------------------------
square_dp:
	save	%sp, -88, %sp	! save the registers, stack 
	set	temp2, %l0 	! .. get the address of temp2
	set	temp, %l1 	! .. and temp
	st	%i0, [%l0] 	! .. get the callers value
	ld	[%l0], %f0 	! .. into a float register
	fsqrtd  %f0, %f2	! .... have the fpu perform the operation
	st	%f2, [%l1] 	! .. save the result
	ld	[%l1], %i0 	! .... and return it to the caller
	ret 			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Add single precision
! Function:	Add two values
! Calling:	in0 = value1,  in1 = value2
! Returns:	in0 = result
! Convention:	result = add_sp(value1,value2); 
!--------------------------------------------------------------------------
add_sp:
	save	%sp, -88, %sp	! save the registers, stack  
	set	temp2, %l0  	! .. get the address of temp2
	set	temp1, %l1	! .. and temp1
	set	temp, %l2	! .. and temp
	st	%i0, [%l0]	! .. get the users value1
	st	%i1, [%l1]	! .. and value2
	ld	[%l0], %f0	! .. into the float registers
	ld	[%l1], %f2	! ......
	fadds   %f0, %f2, %f4	! .... have the fpu perform the operation
	st	%f4, [%l2]	! .. save the result
	ld	[%l2], %i0	! .... and return it to caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Add double precision 
! Function:	Add two 64 bit values
! Calling:	in0 = value1, in1 = value2
! Returns:	in0.1 = result
! Convention:	result = add_dp(value1,value2); 
!--------------------------------------------------------------------------
add_dp:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0   	! .. get the address of temp2
	set	temp1, %l1 	! .. and temp1
	set	temp, %l2 	! .. and temp
	st	%i0, [%l0] 	! .. get the user value1
	st	%i1, [%l1]	! .. get the user value2
	ld	[%l0], %f0 	! .. set them in float registers
	ld	[%l1], %f2	! .... both values
	faddd	%f0, %f2, %f4	! .... have the fpu perform the operation
	st	%f4, [%l2] 	! .. save the result
	ld	[%l2], %i0 	! .... and return it to the caller
	ret 			! Delayed return (get user ret addr)
	restore 		! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Subtract Single Precision
! Function:	Subtract two single precision values from each other
! Calling:	in0 = Value1, in1 = value2
! Returns:	in0 = result
! Convention:	result = sub_sp(value1, value2);
!--------------------------------------------------------------------------
sub_sp:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0   	! set the address of the result holder
	set	temp1, %l1 	! .. get the address of temp1 (holder)
	set	temp, %l2 	! .. get the address of temp
	st	%i0, [%l0] 	! .. save the value in memory
	st	%i1, [%l1] 	! .. save the value in memory
	ld	[%l0], %f0 	! .. load the fpu register
	ld	[%l1], %f2 	! .. load the fpu register
	fsubs	%f0, %f2, %f4 	! .... have the fpu perform the operation
	st	%f4, [%l2] 	! .. save the result
	ld	[%l2], %i0 	! .. return the result to the caller
	ret 			! Delayed return (get user ret addr)
	restore 		! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Subtract Double Precision
! Function:	Subtract two double precision values
! Calling:	in0 = Value1, in1 = Value2
! Returns:	in0 = Result
! Convention:	Result = sub_dp(Value1,Value2);
!--------------------------------------------------------------------------
sub_dp:
	save	%sp, -88, %sp	! save the registers, stack    
	set	temp2, %l0    	! set the address of the result holder
	set	temp1, %l1  	! .. get the address of temp1 (holder)
	set	temp, %l2  	! .. get the address of temp
	st	%i0, [%l0]  	! .. save the value in memory
	st	%i1, [%l1] 	! .. save the value in memory
	ld	[%l0], %f0  	! .. load the fpu register
	ld	[%l1], %f2 	! .. load the fpu register
	fsubd	%f0, %f2, %f4 	! .... have the fpu perform the operation
	st	%f4, [%l2]  	! .. save the result
	ld	[%l2], %i0  	! .. return the result to the caller
	ret  			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Multiply Single Precision
! Function:	Multiply two single precision values
! Calling:	in0 = Value1, in1 = value2
! Returns:	in0 = Result
! Convention:	Result = mult_sp(Value1,Value2);
!--------------------------------------------------------------------------
mult_sp:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp1, %l1	! .. and temp1
	set	temp, %l2	! .. and temp
	st	%i0, [%l0]	! .. Get the callers value1 into temp2
	st	%i1, [%l1]	! .. Get the callers value2 into temp1
	ld	[%l0], %f0	! .. then load Value1
	ld	[%l1], %f2	! .. and Value2
	fmuls   %f0, %f2, %f4	! .... have the fpu perform the operation
	st	%f4, [%l2]	! .. save the result
	ld	[%l2], %i0	! .... and return it to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Multiply Double Precision
! Function:	Multiply two values and return the result
! Calling:	i0 = value1, i1 = value2
! Returns:	i0 = result
! Convention:	result = mul_dp(value1, value2); 
!--------------------------------------------------------------------------
mult_dp:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0     	! set the address of the result holder
	set	temp1, %l1     	! .. get the address of temp1 (holder)
	set	temp, %l2	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0	! .. load the fpu register
	ld	[%l1], %f2	! .. load the fpu register
	fmuld   %f0, %f2, %f4  	! .... have the fpu perform the operation
	st	%f4, [%l2]	! .. save the result
	ld	[%l2], %i0	! .. return the result to the caller
	ret    			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Divide Single Precision
! Function:	Divide two value and return the result
! Calling:	i0 = value1, i1 = value2
! Returns:	i0 = result
! Convention:	result = div_sp(value1, value2); 
!--------------------------------------------------------------------------
div_sp:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0	! .. get the address of temp2
	set	temp1, %l1     	! .. get the address of temp1 (holder)
	set	temp, %l2	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0     	! .. load the fpu register
	ld	[%l1], %f2     	! .. load the fpu register
!-------
!  f4 = fdiv(f0,f2);
!  if ( (fsr && 0x000c000) == 0)	/* see if Weitek fpu		 */
!	then fsr = 0;			/* .. if so then clear the flags */
!-------
	fdivs   %f0, %f2, %f4  	! .... have the fpu perform the operation
	st	%fsr, [%l0]	! .. copy the fsr into it
	ld	[%l0], %l4	! .... then into a register
	set	0x0c000, %l0	! .. get the FPU Type mask
	and	%l4, %l0, %l4	! .... isolate the fpu type
	bne	div_sp_done	! ...... exit if not a weitek (ie.. 0)
	fmovs	%f3, %f3	! .... code required by Weitek
div_sp_done:
	st	%f4, [%l2]	! .. save the result
	ld	[%l2], %i0	! .. return the result to the caller
	ret    			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Divide Double Precision
! Function:	Divide two value and return the result
! Calling:	i0 = value1, i1 = value2
! Returns:	i0 = result
! Convention:	result = div_dp(value1, value2); 
!--------------------------------------------------------------------------
div_dp:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0     	! .. get the address of temp2
	set	temp1, %l1     	! .. get the address of temp1 (holder)
	set	temp, %l2	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0     	! .. load the fpu register
	ld	[%l1], %f2     	! .. load the fpu register
!-------
!  f4 = fdiv(f0,f2);
!  if ( (fsr && 0x000c000) == 0)	/* see if Weitek fpu		 */
!	then fsr = 0;			/* .. if so then clear the flags */
!-------
	fdivd   %f0, %f2, %f4  	! .... have the fpu perform the operation
	st	%fsr, [%l0]	! .. copy the fsr into it
	ld	[%l0], %l4	! .... then into a register
	set	0x0c000, %l0	! .. get the FPU Type mask
	and	%l4, %l0, %l4	! .... isolate the fpu type
	bne	div_dp_done	! ...... exit if not a weitek (ie.. 0)
	fmovs	%f3, %f3	! .... code required by Weitek
div_dp_done:
	st	%f4, [%l2]	! .. save the result
	ld	[%l2], %i0	! .. return the result to the caller
	ret    			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!**************************************************************************
!*			Data Comparison Functions			 *
!**************************************************************************

!--------------------------------------------------------------------------
! Name:		Compare Single Precision Values
! Function:	Compare two values and return the FSR flags
! Calling:	i0 = value1, i2 = value2
! Returns:	i0 = flags
! Convention:	flagsresult  = cmp_s(value1, value2);
!--------------------------------------------------------------------------
cmp_s:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp1, %l1	! .. get the address of temp1 (holder)
	set	temp, %l2	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0	! .. load the fpu register
	ld	[%l1], %f2	! .. load the fpu register
	fcmps	%f0, %f2	! .... have the fpu perform the operation
	nop			! .. delay
	st	%fsr, [%l2]	! .. get the contents of the FSR register
	ld	[%l2], %i0	! .. return the result to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Compare double Precision Values
! Function:	Compare two values and return the FSR flags
! Calling:	i0 = value1, i2 = value2
! Returns:	i0 = flags
! Convention:	flagsresult  = cmp_d(value1, value2);
!--------------------------------------------------------------------------
cmp_d:	
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0     	! .. get the address of temp2
	set	temp1, %l1     	! .. get the address of temp1 (holder)
	set	temp, %l2 	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0     	! .. load the fpu register
	ld	[%l1], %f2	! .. load the fpu register
	fcmpd   %f0, %f2	! .... have the fpu perform the operation
	nop			! .. delay
	st	%fsr, [%l2]	! .. get the contents of the FSR register
	ld	[%l2], %i0	! .. return the result to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Compare Single to EXtended precision values
! Function:	Compare two values and return the FSR flags
! Warning:	** UNIMPLEMENTED INSTRUCTION TRAP (EXTENDED PRECISION) ** 
! Calling:	i0 = value1, i2 = value2
! Returns:	i0 = flags
! Convention:	flagsresult  = cmp_s_ex(value1, value2);
!--------------------------------------------------------------------------
cmp_s_ex:
	save	%sp, -88, %sp	! save the registers, stack
	set	temp2, %l0	! .. get the address of temp2
	set	temp1, %l1	! .. get the address of temp
	set	temp, %l2	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0	! .. load the fpu register
	ld	[%l1], %f2	! .. load the fpu register
	fcmpes  %f0, %f2	! .... have the fpu perform the operation
	nop			! .. delay
	st	%fsr, [%l2]	! .. get the contents of the FSR register
	ld	[%l2], %i0	! .. return the result to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window

!--------------------------------------------------------------------------
! Name:		Compare Double to EXtended precision values
! Function:	Compare two values and return the FSR flags
! Warning:	** UNIMPLEMENTED INSTRUCTION TRAP (EXTENDED PRECISION) ** 
! Calling:	i0 = value1, i2 = value2
! Returns:	i0 = flags
! Convention:	flagsresult  = cmp_d_ex(value1, value2);
!--------------------------------------------------------------------------
cmp_d_ex:
	save	%sp, -88, %sp	! save the registers, stack   
	set	temp2, %l0     	! .. get the address of temp2
	set	temp1, %l1     	! .. get the address of temp1 (holder)
	set	temp, %l2  	! .. get the address of temp
	st	%i0, [%l0]	! .. save the value in memory
	st	%i1, [%l1]	! .. save the value in memory
	ld	[%l0], %f0     	! .. load the fpu register
	ld	[%l1], %f2	! .. load the fpu register
	fcmped  %f0, %f2	! .... have the FPU do it
	nop			! .. delay
	st	%fsr, [%l2]	! .. get the contents of the FSR register
	ld	[%l2], %i0	! .. return the result to the caller
	ret			! Delayed return (get user ret addr)
	restore			! .. restore the frame window


