/*
 *static char     frefsccsid[] = "@(#)fpu-test.s 1.1 7/30/92 Copyright Sun Microsystems"; 
 */
!****************************************************************************
! 
!
!****************************************************************************
! Function:	This is the test module for Sundiag.  All of the test and
!		assembler level diagnostic code is here.  All of the inter-
!		face code is in the fpu-if.s module.
!****************************************************************************

        .seg    "text"

	.global	_datap_add
	.global _datap_mult
	.global _datap_add_dp
	.global _datap_mult_dp
	.global _timing_add_sp
	.global _timing_mult_sp
	.global _timing_add_dp
	.global _timing_mult_dp
	.global result_msw
	.global result_lsw
	.global _clear_regs
	.global _register_test
	.global _move_regs
	.global _branches
	.global _trap_remove
	.global _fpu_exc	
	.global _int_float_e

	.seg    "data"
        .align  8
temp:		.skip 4
temp1:		.skip 4
temp2:		.skip 4
wdp: 		.skip 4		! storage location for a single precision
wdp1:  		.skip 4		! storage location for a double precision
wdp2:   	.skip 4		! storage for extended

dp_result: 	.skip 4
result_msw: 	.skip 4
result_lsw: 	.skip 4
 
 
	.seg	"text"

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	This routine test the data path of weitek adder for single 
!		precision.
! Calling:	i0 = value
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
!		f0 = value
!		f1 = 0
!	add =   f2 = value
!
_datap_add:
	save    %sp, -88, %sp	! save the stack frame
	set	wdp, %l0	! get a memory address
	set	wdp1, %l1	! .. one for the result
	set	0x0, %l3	! .. get a zero
	st	%l3, [%l1]	! .... and store it in memory
	st	%i0, [%l0]	! .... store the value passed 
	ld	[%l0], %f0	! .... put the passed value into f0
	ld	[%l1], %f1	! .... put value 0 into reg f1
	fadds   %f0, %f1, %f2   ! ...... add zero and value into f2
	fcmps	%f0, %f2	! .... check the value passed and added value 
	nop			! .... delay
	fbe	datap_ok	! .. if they are equal
	nop			! .... delay
datap_no:
	st	%f2, [%l1]	! return the result on error
datap_ok:
	ld	[%l1], %i0	! then return a zero
	ret			! .... delay
	restore


!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
! 
! This routine test the data path of weitek multiplier for single precision
!              f0 = value
!              f1 = 1 
!      mult =  f2 = f0 * f1
!
_datap_mult:
	save    %sp, -88, %sp
        set     wdp, %l0
        set     wdp1, %l1
        set     0x3F800000, %l3		!put value 1 into memoruy
	st      %l3, [%l1] 
        st      %i0, [%l0]      !store the value passed into memory location
        ld      [%l0], %f0      ! put the passed value into f0 
        ld      [%l1], %f1      ! put value 1 into reg f1 
        fmuls   %f0, %f1, %f2	! multiply value with 1 , it has to be same
	fcmps   %f0, %f2
	nop
	fbne	datap_no
	nop
	set	0x0, %l3
	st	%l3, [%l1]
	ba	datap_ok
	nop


!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
!	This routine tests the data path of the weitek multiplier for 
!	double precision 
!
!              f0 = msw value
!		f1 = lsw value
!		f2 = 0
!		f3 = 0
!	add =   f4 = f0 + f2
!
_datap_add_dp:
	save    %sp, -88, %sp  
        set     wdp, %l0       
        set     wdp1, %l1  
	set	wdp2, %l2
	set	result_msw, %l4
	set	result_lsw,%l5
	set	0x0, %l3	!put value 0 into memoruy      
        st      %l3, [%l1]
	st	%i0, [%l0]	! msw of value
        st	%i1, [%l2]	! lsw of value
	ld	[%l0], %f0	! put the msw into f0
	ld	[%l2], %f1	! put the lsw into f1
	ld	[%l1], %f2	! put 0 into f2
	ld	[%l1], %f3	! put 0 into f3
	faddd   %f0, %f2, %f4	! add value + 0 into f4
	fcmpd   %f0, %f4	! now compare the result
	nop
	fbe	datap_ok	! good
	nop
!*	stfd	%f4, [%l4]
	st	%f4, [%l4]
	st	%f5, [%l5]
	nop
	set	0x1, %l3
	st	%l3, [%l1]
	ba	datap_ok	! no good but code is already set		
	nop

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
!  This routine tests the data path of the weitek multiplier for 
!  double precision      
!
!              f0 = msw value 
!              f1 = lsw value 
!              f2 = 0
!              f3 = 0
!	mult = f4 = f0 * f2
!
_datap_mult_dp:
	save    %sp, -88, %sp  
        set     wdp, %l0       
        set     wdp1, %l1      
        set     wdp2, %l2      
        set     result_msw, %l4
	set	result_lsw, %l5
        set     0x3FF00000, %l3 ! put value 1  into memoruy      
        st      %l3, [%l1]     
        st      %i0, [%l0]      ! msw of value 
        st      %i1, [%l2]      ! lsw of value 
        ld      [%l0], %f0      ! put the msw into f0  
        ld      [%l2], %f1      ! put the lsw into f1  
        ld      [%l1], %f2      ! put 1 into f2
	set	0x0, %l3
	st	%l3, [%l1]
        ld      [%l1], %f3	! put 0 into f3, i.e f2 = 0x3ff00000 dp of 1 
	fmuld	%f0, %f2, %f4   ! mult value * 1 into f4
	fcmpd   %f0, %f4	! now compare the result
	nop 
        fbe     datap_ok        ! good
	nop 
!*        stfd    %f4, [%l4]       
	st	%f4, [%l4]
	st	%f5, [%l5]
        nop
        set     0x1, %l3         
        st      %l3, [%l1]     
        ba      datap_ok        ! no good but code is already set 
	nop
!
! for add routine all the f registers from 0 - 19 will be filled with numbers
! and the result should be 10.
!
_timing_add_sp:
	save    %sp, -88, %sp	! save the registers, stacck
        set     wdp, %l0
	set	wdp1, %l1
	set	wdp2, %l2
	set	0x0, %l3
	set     0x3f800000, %l4 ! put value 1 
	set	0x41200000, %l5	! put value 10 into local 5
	st	%l5, [%l0]
	st	%l4, [%l1]
	st	%l3, [%l2]
	ld	[%l0], %f31	! register 31 has 10
	ld	[%l1], %f30	! register 30 has 1

	ld	[%l2], %f0	! reg 0 has 0
	fmovs   %f31, %f1	! reg1 has 10
	fsubs   %f31, %f30, %f18! reg 18 has 9
	fmovs   %f18, %f3	! reg 3 has 9
	fmovs   %f30, %f2	! reg 2 has 1
	fmovs   %f30, %f19	! reg 19 has 1
	fsubs   %f18, %f19, %f16! reg 16 has 8
	fmovs   %f16, %f5	! reg 5 has 8
	fsubs   %f31, %f16, %f17! reg 17 has 2
	fmovs	%f17, %f4	! reg 4 has 2
	fsubs   %f16, %f30, %f14! reg 14 has 7 
	fmovs   %f14, %f7	! reg 7 has 7
	fsubs   %f31, %f14, %f15! reg 15 has 3
	fmovs	%f15, %f6	! reg 6 has 3
	fsubs   %f14, %f30, %f12! reg 12 has 6
	fmovs	%f12, %f9	! reg 9 has 6
	fsubs   %f31, %f12, %f13! reg 13 has 4
	fmovs	%f13, %f8	! reg 8 has 4
	fsubs   %f12, %f30, %f10! reg 10 has 5
	fmovs	%f10, %f11	! reg 11 has 5

	fadds	%f0, %f1, %f20		! reg 0 + reg 1 = reg 20 = 10
	fadds   %f2, %f3, %f21		! reg 2 + reg 3 = reg 21 = 10
	fadds   %f4, %f5, %f22		! reg 4 + reg 5 = reg 22 = 10
	fadds   %f6, %f7, %f23		! reg 6 + reg 7 = reg 23 = 10
	fadds   %f8, %f9, %f24		! reg 8 + reg 9 = reg 24 = 10
	fadds   %f10, %f11, %f25	! reg 10 + reg 11 = reg 25 = 10
	fadds   %f12, %f13, %f26	! reg 12 + reg 13 = reg 26 = 10
	fadds   %f14, %f15, %f27	! reg 14 + reg 15 = reg 27 = 10
	fadds   %f16, %f17, %f28	! reg 16 + reg 17 = reg 28 = 10
	fadds   %f18, %f19, %f29	! reg 18 + reg 19 = reg 29 = 10
!
treg20:					!  Now additions are done check it out
	fcmps	%f31, %f20
	nop
	fbe	treg21			! go to check next one
	nop
r20:
	st	%f20, [%l2]
	ba	done
	nop
treg21:
	fcmps	%f31, %f21
	nop
	fbe	treg22
	nop
r21:
	st	%f21, [%l2]      
        ba      done
        nop
treg22:	
	fcmps   %f31, %f22
	nop
	fbe	treg23
	nop
r22:
	st	%f22, [%l2]            
        ba      done   
        nop    
treg23:
	fcmps	%f31, %f23
	nop
	fbe     treg24 
        nop 
r23:
        st      %f23, [%l2]             
        ba      done    
        nop        
treg24:   
        fcmps   %f31, %f24 
	nop
        fbe     treg25  
        nop  
r24:
        st      %f24, [%l2]              
        ba      done     
        nop         
treg25:    
        fcmps   %f31, %f25  
	nop
        fbe     treg26   
        nop   
r25:
        st      %f25, [%l2]               
        ba      done      
        nop          
treg26:     
        fcmps   %f31, %f26   
	nop
        fbe     treg27    
        nop    
r26:
        st      %f26, [%l2]                
        ba      done       
        nop           
treg27:      
        fcmps   %f31, %f27    
	nop
        fbe     treg28     
        nop     
r27:
        st      %f27, [%l2]                 
        ba      done        
        nop            
treg28:       
        fcmps   %f31, %f28     
	nop
        fbe     treg29      
        nop      
r28:
        st      %f28, [%l2]                  
        ba      done         
        nop             
treg29:        
        fcmps   %f31, %f28      
	nop
        fbe     reggood
	nop
r29:
	st	%f29, [%l2]
	nop
done:
reggood:
	ld	[%l2], %i0
	ret
        restore
!
!	for mult routine all the f registers from 0 - 19 will be filled 
!	with numbers and the result should be the number.
!
_timing_mult_sp:
	save    %sp, -88, %sp           ! save the registers, stacck
        set     wdp, %l0
        set     wdp1, %l1
        set     wdp2, %l2
        set     0x0, %l3
	set	0x3f800000, %l4         ! put value 1 
        set     0x41200000, %l5         ! put value 10 into local 5
        st      %l5, [%l0]
        st      %l4, [%l1]
        st      %l3, [%l2]
        ld      [%l0], %f31             ! register 31 has 10
        ld      [%l1], %f1              ! register 1 has 1
	fmovs   %f1, %f3
	fmovs   %f1, %f5
	fmovs   %f1, %f7
	fmovs   %f1, %f9
	fmovs   %f1, %f11		! register 1, 3, 5, 7, 9, 11, 13, 15, 17, 19
	fmovs   %f1, %f13		! has a value of 1
	fmovs   %f1, %f15
	fmovs   %f1, %f17
	fmovs   %f1, %f19		!
	fmovs	%f1, %f0
	fmovs   %f31, %f18		! reg 18 has 10
	fsubs	%f31, %f0, %f16		! reg 16  has 9
	fsubs   %f16, %f0, %f14		! reg 14 has 8
	fsubs   %f14, %f0, %f12		! reg 12 has 7
	fsubs   %f12, %f0, %f10		! reg 10 has 6
	fsubs   %f10, %f0, %f8		! reg 8 has 5
	fsubs   %f8, %f0, %f6		! reg 6 has 4
	fsubs   %f6, %f0, %f4		! reg 4 has 3
	fsubs   %f4, %f0, %f2		! reg 2 has 2

	fmuls   %f0, %f1, %f20          ! reg 0 * reg 1 = reg 20 = 1
        fmuls   %f2, %f3, %f21          ! reg 2 * reg 3 = reg 21 = 2 
        fmuls   %f4, %f5, %f22          ! reg 4 * reg 5 = reg 22 = 3 
        fmuls   %f6, %f7, %f23          ! reg 6 * reg 7 = reg 23 = 4 
        fmuls   %f8, %f9, %f24          ! reg 8 * reg 9 = reg 24 = 5 
        fmuls   %f10, %f11, %f25        ! reg 10 * reg 11 = reg 25 = 6 
        fmuls   %f12, %f13, %f26        ! reg 12 * reg 13 = reg 26 = 7 
        fmuls   %f14, %f15, %f27        ! reg 14 * reg 15 = reg 27 = 8  
        fmuls   %f16, %f17, %f28        ! reg 16 * reg 17 = reg 28 = 9 
        fmuls   %f18, %f19, %f29        ! reg 18 * reg 19 = reg 29 = 10

	fcmps	%f0, %f20
	nop
	fbe	m21
	nop
	ba	r20
	nop
m21:
	fcmps	%f2, %f21
	nop
	fbe	m22
	nop
	ba	r21
	nop
m22:
	fcmps   %f4, %f22
	nop
	fbe 	m23
	nop
	ba	r22
	nop
m23:
	fcmps	%f6, %f23
	nop
	fbe	m24
	nop
	ba	r23
	nop
m24:
	fcmps	%f8, %f24
	nop
	fbe	m25
	nop
	ba	r24
	nop
m25:
	fcmps	%f10, %f25
	nop
	fbe	m26
	nop
	ba	r25
	nop
m26:
	fcmps	%f12, %f26
	nop
	fbe	m27
	nop
	ba	r26
	nop
m27:
	fcmps	%f14, %f27
	nop
	fbe	m28
	nop
	ba	r27
	nop
m28:
	fcmps	%f16, %f28
	nop
	fbe	m29
	nop
	ba	r28
	nop
m29:
	fcmps	%f18, %f29
	nop
	fbe	reggood
	nop	
	ba	r29
	nop
!
!	same thing for double precision
!
_timing_add_dp:
	save    %sp, -88, %sp           ! save the registers, stacck
        set     wdp, %l0
        set     wdp1, %l1
        set     wdp2, %l2
        set     0x0, %l3
	set	0x3ff00000, %l4         ! put value 1 
        set     0x40240000, %l5         ! put value 10 into local 5
        st      %l5, [%l0]
        st      %l4, [%l1]
        st      %l3, [%l2]
	ld	[%l0], %f30             ! register 30 has 10
	fmovs   %f30, %f2		! reg 2 has 10
	ld	[%l2], %f0		! reg 0 has 0
	ld	[%l1], %f4		! reg 4 has 1
	fsubd	%f30, %f4, %f6		! reg 6 has 9
	fsubd	%f6, %f4, %f10		! reg 10 has 8
	fsubd   %f30, %f10, %f8		! reg 8 has 2
	fsubd	%f10, %f4, %f14		! reg 14 has 7
	fsubd   %f30, %f14, %f12	! reg 12 has 3
	fsubd	%f14, %f4, %f18		! reg 18 has 6
	fsubd	%f30, %f18, %f16	! reg 16 has 4
!
	faddd	%f0, %f2, %f20		! reg 20 has 10
	faddd   %f4, %f6, %f22		! reg 22 has 10
	faddd   %f8, %f10, %f24		! reg 24 has 10
	faddd   %f12, %f14, %f26	! reg 26 has 10
	faddd   %f16, %f18, %f28	! reg 28 has 10
!
	fcmpd	%f30, %f20
	nop
	fbe	d22
	nop
	st	%f20, [%l2]
	ba	nogood
	nop
d22:
	fcmpd	%f30, %f22
	nop
	fbe	d24
	nop
	st	%f22, [%l2]      
        ba      nogood
        nop
d24:
	fcmpd   %f30, %f24     
	nop	
        fbe     d26
	nop
        st      %f24, [%l2]            
        ba      nogood 
        nop    
d26:
        fcmpd   %f30, %f26     
	nop
        fbe     d28    
	nop
        st      %f26, [%l2]            
        ba      nogood 
        nop    
d28:
	fcmpd	%f30, %f28
	nop
	fbe	good	
	nop
	st	%f28, [%l2]   
	nop
good:
nogood:
	ld	[%l2], %i0
	
	ret
	restore
				
!	Now for mult
!
_timing_mult_dp:
	save    %sp, -88, %sp           ! save the registers, stacck
        set     wdp, %l0
        set     wdp1, %l1
        set     wdp2, %l2
        set     0x0, %l3
        set     0x3ff00000, %l4         ! put value 1 
        set     0x40340000, %l5         ! put value 20 into local 5
        st      %l5, [%l0]
        st      %l4, [%l1]
        st      %l3, [%l2]
	ld      [%l0], %f30             ! register 30 has 20
	ld	[%l1], %f2		! register  2 has 1
	fmovs   %f30, %f0		! register  0 has 20
	faddd	%f2, %f2, %f10		! register 10 has 2
	fmovs   %f10, %f16		! register 16 has 2
	faddd	%f10, %f16, %f4		! register 4 has 4
	faddd   %f4, %f2, %f6		! register 6 has 5
	fmovs	%f6, %f12		! reg. 12 has 5
	fmovs	%f4, %f14		! reg 14 has 4
	faddd	%f12, %f6, %f18		! reg 18 has 10
	fmovs	%f18, %f8		! reg 8 has 10
!
! 	now everything is set
!
	fmuld   %f0, %f2, %f20          ! reg 20 has 20	
	fmuld	%f4, %f6, %f22          ! reg 22 has 20
	fmuld	%f8, %f10, %f24         ! reg 24 has 20
	fmuld	%f12, %f14, %f26        ! reg 26 has 20
	fmuld	%f16, %f18, %f28        ! reg 28 has 20
!
	fcmpd   %f30, %f20
	nop
	fbe	dm22
	nop
        st      %f20, [%l2]
        ba      nogood
        nop
dm22:
	fcmpd   %f30, %f22
	nop
        fbe     dm24
	nop
        st      %f22, [%l2]      
        ba      nogood
        nop
dm24:
	fcmpd   %f30, %f24     
	nop
        fbe     dm26
        nop
        st      %f24, [%l2]            
        ba      nogood 
        nop    
dm26:
        fcmpd   %f30, %f26     
	nop
        fbe     dm28    
        nop    
        st      %f26, [%l2]            
        ba      nogood 
        nop    
dm28:
        fcmpd   %f30, %f28
	nop
        fbe     good
        nop
        st      %f28, [%l2]   
        nop
	ba	nogood
	nop
!
!

	
	.seg	"text"

	.global _wadd_sp
        .global _wadd_dp
        .global _wdiv_sp
        .global _wdiv_dp
        .global _wmult_sp
        .global _wmult_dp
        .global _wsadd_addr
        .global _wdadd_addr
        .global _wsdiv_addr
        .global _wddiv_addr
        .global _wsmult_addr
        .global _wdmult_addr
        .global _actual_addr
        .global _chain_sp
        .global _chain_dp
 

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
! The following routines are for checking the weitek
! status tests.  
!	The input is : i0 = amsw
!		       i1 = bmsw or alsw (for double precision)
!	  	       i2 = bmsw (for dp)
!		       i3 = blsw (for dp)
!
!	The output is  i0 = value of FSR register
!

_wadd_sp:
	save    %sp, -88, %sp
        set     xdp, %l0
        set     xdp1, %l1
	set	xdp2, %l2	
	set	_wsadd_addr, %l3
	set	_actual_addr, %l4
	st	%l3, [%l4]
	st	%i0, [%l0]		! get the first value
	st	%i1, [%l1]		! get the second value
	ld	[%l0], %f0		! f0 has the first value
	ld 	[%l1], %f2		! f2 has the second value
	ld	[%l1], %o3
_wsadd_addr:
	fadds   %f0, %f2, %f3		! now do the instruction
	nop
	nop
	st	%fsr, [%l2]		! get the fsr value
go_back:
	nop
	ld	[%l2], %i0
	ret
	restore
	nop
!
!	same thing for add double precision
!
_wadd_dp:
	save    %sp, -88, %sp
        set     xdp, %l0
        set     xdp1, %l1
        set     xdp2, %l2 
	set     _wdadd_addr, %l3 
        set     _actual_addr, %l4 
        st      %l3, [%l4] 
	st	%i0, [%l0]              ! get the first value
        st      %i1, [%l1]              ! get the lsw of first value
	ld	[%l0], %f0
	ld	[%l1], %f1
	st      %i2, [%l0]              ! get the second value
	st      %i3, [%l1]              ! get the lsw of second value
	ld	[%l0], %f2
	ld	[%l1], %f3
_wdadd_addr:
	faddd	%f0, %f2, %f4		! now do the instruction
	nop
        nop
        st      %fsr, [%l2]             ! get the fsr value 
	ba	go_back
	nop
!
!
!	for divide single precision
!
_wdiv_sp:
	save    %sp, -88, %sp
        set     xdp, %l0
        set     xdp1, %l1
        set     xdp2, %l2        
	set     _wsdiv_addr, %l3 
        set     _actual_addr, %l4 
        st      %l3, [%l4] 
        st      %i0, [%l0]              ! get the first value
        st      %i1, [%l1]              ! get the second value
	st      %fsr, [%l2]
        ld      [%l0], %f0              ! f0 has the first value
        ld      [%l1], %f2              ! f2 has the second value 
_wsdiv_addr: 
        fdivs	%f0, %f2, %f3           ! now do the instruction
	nop
	nop
        st      %fsr, [%l2]             ! get the fsr value 
	ba	go_back
	nop
!
!
!	for divide double precision
!
_wdiv_dp:
	save    %sp, -88, %sp  
        set     xdp, %l0       
        set     xdp1, %l1      
        set     xdp2, %l2      
	set     _wddiv_addr, %l3 
        set     _actual_addr, %l4 
        st      %l3, [%l4] 
        st      %i0, [%l0]              ! get the first value  
        st      %i1, [%l1]              ! get the lsw of first value   
        ld      [%l0], %f0     
        ld      [%l1], %f1     
        st      %i2, [%l0]              ! get the second value 
        st      %i3, [%l1]              ! get the lsw of second value  
        ld      [%l0], %f2     
        ld      [%l1], %f3     
_wddiv_addr: 
        fdivd	%f0, %f2, %f4           ! now do the instruction       
        nop
        nop    
        st      %fsr, [%l2]             ! get the fsr value    
        ba      go_back
	nop
!
!
!       for multiply single precision   
!
_wmult_sp:
	save    %sp, -88, %sp 
        set     xdp, %l0 
        set     xdp1, %l1 
        set     xdp2, %l2         
	set     _wsmult_addr, %l3 
        set     _actual_addr, %l4 
        st      %l3, [%l4] 

        st      %i0, [%l0]              ! get the first value 
        st      %i1, [%l1]              ! get the second value 
        ld      [%l0], %f0              ! f0 has the first value 
        ld      [%l1], %f2              ! f2 has the second value  
_wsmult_addr:  
        fmuls   %f0, %f2, %f3           ! now do the instruction
        nop 
        nop 
        st      %fsr, [%l2]             ! get the fsr value  
        ba      go_back 
	nop
! 
! 
!       for multiply double precision 
! 
_wmult_dp: 
        save    %sp, -88, %sp   
        set     xdp, %l0        
        set     xdp1, %l1       
        set     xdp2, %l2       
	set     _wdmult_addr, %l3 
        set     _actual_addr, %l4 
        st      %l3, [%l4] 

        st      %i0, [%l0]              ! get the first value   
        st      %i1, [%l1]              ! get the lsw of first value    
        ld      [%l0], %f0      
        ld      [%l1], %f1      
        st      %i2, [%l0]              ! get the second value  
        st      %i3, [%l1]              ! get the lsw of second value   
        ld      [%l0], %f2      
        ld      [%l1], %f3      
_wdmult_addr:  
        fmuld	 %f0, %f2, %f4           ! now do the instruction         
        nop    
        nop    
        st      %fsr, [%l2]             ! get the fsr value    
        ba      go_back
	nop
!
!	
!	Chaining test.
!	 
_chain_sp:
	save    %sp, -88, %sp
        set     xdp, %l0
        set     xdp1, %l1
	st	%i0, [%l0]	! store the value
	ld	[%l0], %f0
	fitos   %f0, %f2	! convert integer into single
	fmovs   %f2, %f0	! f0 has the same value  x
	fadds	%f0, %f2, %f4   ! f4 will have 2x
	fsubs   %f4, %f0, %f6   ! f6 will have x
	fmuls   %f6, %f4, %f8   ! f8 will have (2x * x)
	fdivs   %f8, %f4, %f10  ! f10 will have (2x * x) / 2x = x
	fstoi	%f10, %f12
_ch_done:
	st	%f12, [%l1]
	ld	[%l1], %i0

	ret
        restore


!
!
_chain_dp:
	save    %sp, -88, %sp
        set     xdp, %l0
        set     xdp1, %l1
        st      %i0, [%l0]      ! store the value
        ld      [%l0], %f0
        fitod   %f0, %f2        ! convert integer into double
	fmovs   %f2, %f0        ! f0 has the same value  x
        faddd   %f0, %f2, %f4   ! f4 will have 2x
        fsubd   %f4, %f0, %f6   ! f6 will have x
        fmuld   %f6, %f4, %f8   ! f8 will have (2x * x)
        fdivd   %f8, %f4, %f10  ! f10 will have (2x * x) / 2x = x
        fdtoi   %f10, %f12
	ba	_ch_done
!
!
	nop
!

	.seg    "data"
        .align  4
xdp:
        .word 1                 /* storage location for a single precision */
xdp1:
        .word 1                 /* storage for double precision */
xdp2:
        .word 1                 /* storage for extended */
xp_result:
        .word 2
 
_actual_addr:
        .word   2
	 
	
	
	
        .seg	"text"

	
	.global	_fpu_soft_trap
	.global _fp_addr
	.global _fsr_data
	.global _donot_dq
	.global _seqerr_trap


!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
_fpu_soft_trap:
	save    %sp, -88, %sp
	set     _fwp, %l1
	set     _fp_addr, %l2 
	set	_fsr_data, %l3
	set	0x1, %l5		! set the flag that trap occured
	set	_donot_dq, %l6
	st	%l5, [%l4]		! set the flag
	st	%fsr, [%l3]		! store the fsr data
	ld	[%l6], %i0
	cmp     %i0, %l5		! check whether bit is set for
	be	no_que
	nop		
	std    %fq, [%l2]  
        std    %fq, [%l1]
!*	std    %fq, [%l1]
	st     %fsr, [%l1]
	ld      [%l1], %i0
!
no_que:	
!*	st	%fsr, [%l1]
!*	ld	[%l1], %i0 
	ret
	restore

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
!	This routine is to create the seqerror trap only 
!	just uses an floating point operation and does not
!	do anything.
!
_seqerr_trap:
	save    %sp, -88, %sp
	set	_fwp, %l1
	ld      [%l1], %f0		! just do some fpu instruction
	st	%fsr, [%l1]
	nop
	nop
	ld	[%l1], %i0

	ret
        restore

	.seg	"data"
_fp_addr:
        .word   2
_fsr_data:
        .word   2
_donot_dq:
        .word   2
_fwp:
        .word   1
_fwp1:
        .word   1

	.seg    "text"
!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		Clear all Registers
! Function:	Loads the callers value into all floating point registers.
! Calling:	in0 = Value
! Returns:	All float register = Value
! Convention:	clear_regs(0);
! Method:	Copys the user value into each floa reg in sequence.
!--------------------------------------------------------------------------
_clear_regs:
	save    %sp, -88, %sp	! save the registers, stack
        set     temp2, %l0	! load the address of temp2 in local0
	st	%i0, [%l0]	! load the value in temp2 via local0
	ld	[%l0], %f0	! .. load the value
	ld	[%l0], %f1	! .. load the value
        ld      [%l0], %f2	! .. load the value
        ld      [%l0], %f3	! .. load the value
        ld      [%l0], %f4	! .. load the value
	ld	[%l0], %f5	! .. load the value
        ld      [%l0], %f6 	! .. load the value
        ld      [%l0], %f7 	! .. load the value
        ld      [%l0], %f8 	! .. load the value
        ld      [%l0], %f9 	! .. load the value
        ld      [%l0], %f10 	! .. load the value
        ld      [%l0], %f11 	! .. load the value
        ld      [%l0], %f12 	! .. load the value
        ld      [%l0], %f13 	! .. load the value
        ld      [%l0], %f14 	! .. load the value
        ld      [%l0], %f15 	! .. load the value
        ld      [%l0], %f16 	! .. load the value
        ld      [%l0], %f17 	! .. load the value
        ld      [%l0], %f18 	! .. load the value
        ld      [%l0], %f19 	! .. load the value
        ld      [%l0], %f20 	! .. load the value
        ld      [%l0], %f21 	! .. load the value
        ld      [%l0], %f22 	! .. load the value
        ld      [%l0], %f23 	! .. load the value
        ld      [%l0], %f24 	! .. load the value
        ld      [%l0], %f25 	! .. load the value
        ld      [%l0], %f26 	! .. load the value
        ld      [%l0], %f27 	! .. load the value
        ld      [%l0], %f28 	! .. load the value
        ld      [%l0], %f29 	! .. load the value
        ld      [%l0], %f30 	! .. load the value
        ld      [%l0], %f31	! .. load the value
	ret
        restore

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
_register_test:
	save    %sp, -88, %sp
	set	temp, %l0
	set	temp1, %l1
	set	temp2,%l2
	set     0x0,  %l3               !reg has register number
	st	%i0,  [%l0]		!save the register number
	st	%i1,  [%l1]		!save the pattern to be written

	cmp	%i0, %l3		! == 0
	be	reg0
	inc	%l3
	cmp	%i0, %l3		! == 1
	be	reg1
	inc	%l3
	cmp	%i0, %l3		! == 2
	be	reg2
	inc	%l3
	cmp	%i0, %l3		! == 3
	be	reg3
	inc	%l3
	cmp	%i0, %l3		! == 4
	be	reg4
	inc	%l3
	cmp	%i0, %l3		! == 5
        be      reg5
	inc     %l3      
        cmp     %i0, %l3                ! == 5 
        be      reg6
	inc     %l3      
        cmp     %i0, %l3                ! == 7 
        be      reg7
	inc     %l3      
        cmp     %i0, %l3                ! == 8 
        be      reg8
	inc     %l3      
        cmp     %i0, %l3                ! == 9 
        be      reg9
	inc     %l3      
        cmp     %i0, %l3                ! == 10 
        be      reg10
	inc     %l3      
        cmp     %i0, %l3                ! == 11 
        be      reg11
	inc     %l3      
        cmp     %i0, %l3                ! == 12 
        be      reg12
	inc     %l3      
        cmp     %i0, %l3                ! == 13 
        be      reg13
	inc     %l3      
        cmp     %i0, %l3                ! == 14 
        be      reg14
	inc     %l3      
        cmp     %i0, %l3                ! == 15 
        be      reg15
	inc     %l3      
        cmp     %i0, %l3                ! == 16 
        be      reg16
	inc     %l3      
        cmp     %i0, %l3                ! == 17 
        be      reg17
	inc     %l3      
        cmp     %i0, %l3                ! == 18 
        be      reg18	
	inc     %l3            
        cmp     %i0, %l3                ! == 19 
        be      reg19
	inc     %l3            
        cmp     %i0, %l3                ! == 20 
        be      reg20
	inc     %l3             
        cmp     %i0, %l3                ! == 21  
        be      reg21
	inc     %l3             
        cmp     %i0, %l3                ! == 22  
        be      reg22
	inc     %l3             
        cmp     %i0, %l3                ! == 23  
        be      reg23
	inc     %l3             
        cmp     %i0, %l3                ! == 24  
        be      reg24
	inc     %l3             
        cmp     %i0, %l3                ! == 25  
        be      reg25
	inc     %l3             
        cmp     %i0, %l3                ! == 26  
        be      reg26
	inc     %l3             
        cmp     %i0, %l3                ! == 27  
        be      reg27
	inc     %l3             
        cmp     %i0, %l3                ! == 28  
        be      reg28
	inc     %l3             
        cmp     %i0, %l3                ! == 29  
        be      reg29
	inc     %l3             
        cmp     %i0, %l3                ! == 30  
        be      reg30
	inc     %l3             
        cmp     %i0, %l3                ! == 31  
	nop
	ld	[%l1], %f31
	st	%f31, [%l2]
	ba	reg_done		! done
	nop	

reg0:
	ld	[%l1], %f0
	st	%f0, [%l2]
	ba	reg_done	
	nop
reg1:
	ld	[%l1], %f1
	st	%f1, [%l2]
	ba	reg_done
	nop
reg2:
	ld	[%l1], %f2
	st	%f2, [%l2]
	ba	reg_done
	nop
reg3:
	ld      [%l1], %f3              
        st      %f3, [%l2]
        ba      reg_done               
	nop
reg4:
	ld      [%l1], %f4
        st      %f4, [%l2]
        ba      reg_done 
        nop
reg5:
	ld      [%l1], %f5
        st      %f5, [%l2]
        ba      reg_done 
        nop
reg6:
	ld      [%l1], %f6
        st      %f6, [%l2]
        ba      reg_done 
        nop
reg7:
	ld      [%l1], %f7
        st      %f7, [%l2]
        ba      reg_done 
        nop
reg8:
	ld      [%l1], %f8
        st      %f8, [%l2]
        ba      reg_done 
        nop
reg9:
	ld      [%l1], %f9
        st      %f9, [%l2]
        ba      reg_done 
        nop
reg10:
	ld      [%l1], %f10
        st      %f10, [%l2]
        ba      reg_done 
        nop
reg11:
	ld      [%l1], %f11
        st      %f11, [%l2]
        ba      reg_done 
        nop
reg12:
	ld      [%l1], %f12
        st      %f12, [%l2]
        ba      reg_done 
        nop
reg13:
	ld      [%l1], %f13
        st      %f13, [%l2]
        ba      reg_done 
        nop
reg14:
	ld      [%l1], %f14
        st      %f14, [%l2]
        ba      reg_done 
        nop
reg15:
	ld      [%l1], %f15
        st      %f15, [%l2]
        ba      reg_done 
        nop
reg16:
	ld      [%l1], %f16
        st      %f16, [%l2]
        ba      reg_done 
        nop
reg17:
	ld      [%l1], %f17
        st      %f17, [%l2]
        ba      reg_done 
        nop
reg18:
	ld      [%l1], %f18
        st      %f18, [%l2]
        ba      reg_done 
        nop
reg19:
	ld      [%l1], %f19
        st      %f19, [%l2]
        ba      reg_done 
        nop
reg20:
	ld      [%l1], %f20
        st      %f20, [%l2]
        ba      reg_done 
        nop
reg21:
	ld      [%l1], %f21
        st      %f21, [%l2]
        ba      reg_done 
        nop
reg22:
	ld      [%l1], %f22
        st      %f22, [%l2]
        ba      reg_done 
        nop
reg23:
	ld      [%l1], %f23
        st      %f23, [%l2]
        ba      reg_done 
        nop
reg24:
	ld      [%l1], %f24
        st      %f24, [%l2]
        ba      reg_done 
        nop
reg25:
	ld      [%l1], %f25
        st      %f25, [%l2]
        ba      reg_done 
        nop
reg26:
	ld      [%l1], %f26
        st      %f26, [%l2]
        ba      reg_done 
        nop
reg27:
	ld      [%l1], %f27
        st      %f27, [%l2]
        ba      reg_done 
        nop
reg28:
	ld      [%l1], %f28
        st      %f28, [%l2]
        ba      reg_done 
        nop
reg29:
	ld      [%l1], %f29
        st      %f29, [%l2]
        ba      reg_done 
        nop
reg30:
	ld      [%l1], %f30
        st      %f30, [%l2]
        ba      reg_done 
        nop
reg31:
	ld      [%l1], %f31
        st      %f31, [%l2]
reg_done:
	ld	[%l2], %i0

	ret
	restore

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		Move Registers
! Function:	Move a value thru the float registers
! Calling:	in0 = value
! Returns:	in0 = result 
! Convention:	if (result != move_regs(value)) 
!                   error(result-value);
!--------------------------------------------------------------------------
_move_regs:
	save    %sp, -88, %sp	! save the registers, stack
        set     temp2, %l0	! get the address to temp2
        set     temp, %l1	! .. and temp
        st      %i0, [%l0]	! get the callers value
        ld      [%l0], %f0	! .. into a float register
	fmovs   %f0, %f1	! copy from 1 register to the next
	fmovs   %f1, %f2	! .. to the next
	fmovs   %f2, %f3	! .. to the next
	fmovs   %f3, %f4	! .. to the next
	fmovs   %f4, %f5	! .. to the next
	fmovs   %f5, %f6	! .. to the next
	fmovs   %f6, %f7	! .. to the next
	fmovs   %f7, %f8	! .. to the next
	fmovs   %f8, %f9	! .. to the next
	fmovs   %f9, %f10	! .. to the next
	fmovs   %f10, %f11	! .. to the next
	fmovs   %f11, %f12	! .. to the next
	fmovs   %f12, %f13	! .. to the next
	fmovs   %f13, %f14	! .. to the next
	fmovs   %f14, %f15	! .. to the next
	fmovs   %f15, %f16	! .. to the next
	fmovs   %f16, %f17	! .. to the next
	fmovs   %f17, %f18	! .. to the next
	fmovs   %f18, %f19	! .. to the next
	fmovs   %f19, %f20	! .. to the next
	fmovs   %f20, %f21	! .. to the next
	fmovs   %f21, %f22	! .. to the next
	fmovs   %f22, %f23	! .. to the next
	fmovs   %f23, %f24	! .. to the next
	fmovs   %f24, %f25	! .. to the next
	fmovs   %f25, %f26	! .. to the next
	fmovs   %f26, %f27	! .. to the next
	fmovs   %f27, %f28	! .. to the next
	fmovs   %f28, %f29	! .. to the next
	fmovs   %f29, %f30	! .. to the next
	fmovs   %f30, %f31	! .. to the next
	st	%f31, [%l1]	! .... save the result
	ld	[%l1], %i0	! .. and return it to the caller
	ret
        restore

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
! 	The following routine checks the branching is done accordingly
!	to the ficc bits.
!	input	%i0 = 0 = branch unordered
!		      1 = branch greater
!		      2 = branch unordered or greater
!		      3 = branch less
!		      4 = branch unordered or less
!		      5 = branch less or greater
!		      6 = branch not equal
!		      7 = branch equal
!		      8 = branch unordered or equal
!		    . 9 = branch greater or equal
!		     10 = branch branch unordered or greater or equal
!		     11 = branch less or equal
!		     12 = branch unordered or or less or equal
!		     13 = branch ordered
!		     14 = branch always
!		     15 = branch never
!
!	ouput : %i0 = 0 = good
!		    = 1 = error
!

_branches:
	save    %sp, -88, %sp           ! save the registers, stacck
        set     temp2, %l0
        set     temp1, %l1
        set     temp, %l2
	set	0x0, %l3
        st      %l3, [%l0]		!set the result to be true
        st      %i1, [%l1]
	st	%i2, [%l2]
	ld      [%l1], %f0
        ld      [%l2], %f2
	fcmps	%f0, %f2		! compare the values  to get ficc
	nop
	cmp	%i0, %l3
	be	brn_0
	inc	%l3
	cmp	%i0, %l3
	be	brn_1
	inc     %l3 
        cmp     %i0, %l3
	be      brn_2
	inc     %l3  
        cmp     %i0, %l3 
        be      brn_3
	inc     %l3   
        cmp     %i0, %l3  
        be      brn_4 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_5 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_6 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_7 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_8 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_9 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_10 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_11 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_12 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_13 
        inc     %l3   
        cmp     %i0, %l3  
        be      brn_14 
	nop
	fbn	br_error	
	nop
br_good:
	ld	[%l0], %i0

	ret
	restore	
        
br_error:
	set	0xff, %l3  			! set the flag that it is error
	st	%l3, [%l0]
	ld      [%l0], %i0       
 
        ret
        restore	
!	
!				branch unordered
brn_0:
	fbu	br_good
	nop
	ba	br_error
	nop
!				branch greater
brn_1:
	fbg	br_good
	nop
	ba	br_error
	nop
!				branch unordered or greater
brn_2:
	fbug	br_good
	nop
	ba	br_error
	nop
!				branch less
brn_3:
	fbl	br_good
	nop
	ba	br_error
	nop
!				branch unorderd or less
brn_4:
	fbul	br_good
	nop
	ba	br_error
	nop
!				branch less or greater
brn_5:
	fblg	br_good
	nop
	ba	br_error
	nop
!				branch not equal
brn_6:
	fbne	br_good	
	nop
	ba      br_error 
	nop
!                               branch equal
brn_7:
	fbe	br_good  
        nop 
        ba      br_error  
	nop
!                               branch unordered or equal
brn_8:
	fbue	br_good   
        nop  
        ba      br_error   
	nop
!                               branch greater or equal
brn_9:
	fbge	br_good    
        nop   
        ba      br_error    
	nop
!                               branch unordered or greater or equal
brn_10:
	fbuge	br_good     
        nop    
        ba      br_error     
	nop
!                               branch less or equal
brn_11:
	fble	br_good      
        nop     
        ba      br_error      
	nop
!                               branch unordered or less or equal
brn_12:
	fbule	br_good       
        nop      
        ba      br_error       
	nop
!                               branch ordered
brn_13:
	fbo	br_good
	nop
	ba	br_error
	nop
!				branch always
brn_14:
	fba	br_good
	nop
	ba	br_error
	nop
!
!
!	


!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
!
_trap_remove:
	save    %sp, -88, %sp  
	set     temp, %l2
	std     %fq, [%l2]
	std     %fq, [%l2]
	nop
	ret
	restore

!++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
! Name:		
! Function:	
! Calling:	
! Returns:	
! Convention:	
!--------------------------------------------------------------------------
_fpu_exc:
	save    %sp, -88, %sp           ! save the registers, stacck
!*	set	_exp_trap_signal, %l0
	set	temp, %l2
	ld	[%l0], %l1
 	st	%i0, [%l0]		! put the expected interrupt level
	fsqrtx  %f0, %f4		! send an unimplemented instruction
	std     %fq, [%l2]
	std     %fq, [%l2]
	std     %fq, [%l2] 
	std     %fq, [%l2] 
	std     %fq, [%l2] 
	std     %fq, [%l2] 
	std     %fq, [%l2] 
	std     %fq, [%l2] 
	ld	[%l0], %i0		! return the interrupt level
	st	%l1, [%l0]		! put the original interrupt level

!*	save    %sp, -88, %sp
!*	fsqrtx  %f0, %f4
!*	nop	
	ret
	restore

!
!	for single to integer round 
!
!*_sin_int_rnd:
!*	save    %sp, -88, %sp           ! save the registers, stacck
!*        set     temp2, %o5
!*        set     temp, %o4
!*        st      %i0, [%o4]
!*        ld      [%o4], %f0
!*	fstoir  %f0, %f2
!*        st      %f2, [%o5]
!*        ld      [%o5], %i0

!*        ret
!*        restore
!*_doub_int_rnd:
!*	save    %sp, -88, %sp           ! save the registers, stacck 
!*        set     temp2, %o5 
!*        set     temp, %o4 
!*        st      %i0, [%o4] 
!*!        ld      [%o4], %f0 
!*        fdtoir  %f0, %f2 
!*        st      %f2, [%o5] 
!*        ld      [%o5], %i0 
 
!*        ret 
!*        restore 




