!  "@(#)others.s 1.1 7/30/92  Copyright Sun Microsystems"
	.seg	"text"
	.global	_reg1m
	.global _reg1l
	.global _reg2m
	.global _reg2l
	.global _simple2
	.global _simple3

        .seg    "data"
simtemp:
	.word   2
_reg1m:
	.word 2
_reg1l:
	.word 2
_reg2m:
	.word 2
_reg2l:
	.word 2
_simt:
	.word 2
_simt1:
	.word 2
        .align  8
var:
x1      = 0x0
        .word   0x7E4663C4,0x40000000
q       = 0x8
        .word   0xF8000000,0x00000000
z       = 0x10
        .word   0x39F00000,0x00000000
w       = 0x18
        .word   0x00CE998A
        .skip   4
copy_f0 = 0x20
        .skip   8
copy_f2 = 0x28
        .skip   8
save    = 0x30
        .skip   64
exit_on_err     = 0x70
        .word   1
loop_count      = 0x74
        .word   10000000
first_failure   = 0x78
        .skip   4

!
! check x1*z*(double)w + q
!
! no input is necessary. The first evaluation of the expression is 
! saved in (f2,f3). Then the program repeat computing the expression
! in (f0,f1) loop_count many times. If a failure ((f0,f1)!=(f2,f3))
! is detected, the program will accumulate the failure and pass 
! throught label "_problem". If exit_on_err is set, the program will
! exit. The values of (f0,f1,f2,f3) will be saved in copy_f0 and
! copy_f2 in the data segment. Output o0 is set equal to the number
! of failures detected.
!
        .seg    "text"
        .global _zbug,_problem
	.global _simpletest
	.global _get_f0
        .global _get_f1
        .global _get_f2  
        .global _get_f3  
        .global _get_f4  
        .global _get_f5  
        .global _get_f6  
        .global _get_f7  
        .global _get_f8  
        .global _get_f9  
        .global _get_f10  
        .global _get_f11  
        .global _get_f12  
        .global _get_f13  
        .global _get_f14  
        .global _get_f15  
        .global _get_f16  
        .global _get_f17  
        .global _get_f18  
        .global _get_f19  
        .global _get_f20 
        .global _get_f21 
        .global _get_f22 
        .global _get_f23 
        .global _get_f24 
        .global _get_f25 
        .global _get_f26 
        .global _get_f27 
        .global _get_f28 
        .global _get_f29 
        .global _get_f30 
        .global _get_f31 

_simple3:
	save    %sp, -88, %sp
	set	_simt, %l0
	set	_simt1, %l1
	set	_reg1m, %l3
	set	_reg1l, %l4
	st	%i0, [%l0]
	st	%i1, [%l1]
	ld	[%l0], %f0
	ld	[%l1], %f1
	ld	[%l0], %f2
	ld	[%l1], %f3
	fcmpd	%f2, %f0
	nop
	nop
	fbe	sd1
	nop
	nop
	fcmps    %f2, %f0
	nop
	nop
	fbe	oksim1
	nop
	nop
	set	0x1, %i0
	ba	simgo
	nop
oksim1:
	fcmps	%f3, %f1
	nop
	nop
	fbe	sd1
	nop
	st	%f0, [%l3]
	st	%f1, [%l4]	
	set	0x2, %i0
simgo:
	ret
	restore

sd1:
	set	0x0,%i0
	ret
	restore

	

! o0 = number of failures
! o3 = current loop count
! o4 = loop count bound
_zbug:
        set     var,%o5
        st      %o0,[%o5+exit_on_err]
        std     %f4,[%o5+save]
        std     %f6,[%o5+save+0x8]
        std     %f8,[%o5+save+0x10]
        std     %f10,[%o5+save+0x18]
        std     %f12,[%o5+save+0x20]
        std     %f14,[%o5+save+0x28]
        ld      [%o5+loop_count],%o4
        set     0,%o3
        set     0,%o0
        ld      [%o5+w],%f1
        fitod   %f1,%f4
        ldd     [%o5+x1],%f6
        fmuld   %f4,%f6,%f8
        ldd     [%o5+z],%f10
        fmuld   %f8,%f10,%f12
        ldd     [%o5+q],%f14
        faddd   %f12,%f14,%f2           ! f2 = x1*z*((double) w)+q
        nop
loop:
        add     %o3,1,%o3
        cmp     %o3,%o4
        bge     done
        nop
        ld      [%o5+w],%f1
        fitod   %f1,%f4
        ld      [%o5+x1],%f6
        ld      [%o5+x1+4],%f7
        fmuld   %f4,%f6,%f8
        ld      [%o5+z],%f10
        ld      [%o5+z+4],%f11
        fmuld   %f8,%f10,%f12
        ld      [%o5+q],%f14
        ld      [%o5+q+4],%f15
        faddd   %f12,%f14,%f0           ! f2 = x1*z*((double) w)+q
        nop
        nop
        fcmpd   %f2,%f0
        nop
        nop
        fbe     loop
        nop
        nop
        std     %f0,[%o5+copy_f0]
        std     %f2,[%o5+copy_f2]
        add     %o0,1,%o0               ! failure count + 1
        st      %o3,[%o5+first_failure]
        ld      [%o5+exit_on_err],%o1
        nop
_problem: 
        tst     %o1
        bne     done
        nop
        ba      loop
        nop
done:
        ldd     [%o5+save],%f4  
        ldd     [%o5+save+0x8],%f6      
        ldd     [%o5+save+0x10],%f8     
        ldd     [%o5+save+0x18],%f10    
        ldd     [%o5+save+0x20],%f12    
        ldd     [%o5+save+0x28],%f14    
        retl
        nop



_simpletest:
	save    %sp, -88, %sp           ! save the registers, stacck
        set     simtemp, %l0
        st      %i0, [%l0]
	ld      [%l0], %f0
        ld      [%l0], %f1
        ld      [%l0], %f2
        ld      [%l0], %f3
        ld      [%l0], %f4
        ld      [%l0], %f5
        ld      [%l0], %f6 
        ld      [%l0], %f7 
        ld      [%l0], %f8 
        ld      [%l0], %f9 
        ld      [%l0], %f10 
        ld      [%l0], %f11 
        ld      [%l0], %f12 
        ld      [%l0], %f13 
        ld      [%l0], %f14 
        ld      [%l0], %f15 
        ld      [%l0], %f16 
        ld      [%l0], %f17 
        ld      [%l0], %f18 
        ld      [%l0], %f19 
        ld      [%l0], %f20 
        ld      [%l0], %f21 
        ld      [%l0], %f22 
        ld      [%l0], %f23 
        ld      [%l0], %f24 
        ld      [%l0], %f25 
        ld      [%l0], %f26 
        ld      [%l0], %f27 
        ld      [%l0], %f28 
        ld      [%l0], %f29 
        ld      [%l0], %f30 
        ld      [%l0], %f31
	ret
	restore

_get_f0:
	save    %sp, -88, %sp  
	set	simtemp, %l0
	st	%f0, [%l0]
	ld	[%l0],%i0
	ret
	restore

_get_f1:
	save    %sp, -88, %sp   
        set     simtemp, %l0 
        st      %f1, [%l0] 
        ld      [%l0],%i0  
        ret 
        restore 
 
_get_f2: 
        save    %sp, -88, %sp    
        set     simtemp, %l0  
        st      %f2, [%l0]  
        ld      [%l0],%i0   
        ret  
        restore  
  
_get_f3:  
        save    %sp, -88, %sp     
        set     simtemp, %l0   
        st      %f3, [%l0]  
        ld      [%l0],%i0    
        ret   
        restore   
   
_get_f4:   
        save    %sp, -88, %sp      
        set     simtemp, %l0    
        st      %f4, [%l0]   
        ld      [%l0],%i0     
        ret    
        restore    
    
_get_f5:    
        save    %sp, -88, %sp       
        set     simtemp, %l0     
        st      %f5, [%l0]    
        ld      [%l0],%i0      
        ret     
        restore     
     
_get_f6:     
        save    %sp, -88, %sp        
        set     simtemp, %l0      
        st      %f6, [%l0]     
        ld      [%l0],%i0       
        ret      
        restore      
      
_get_f7:      
        save    %sp, -88, %sp         
        set     simtemp, %l0       
        st      %f7, [%l0]      
        ld      [%l0],%i0        
        ret       
        restore       
       
_get_f8:       
        save    %sp, -88, %sp          
        set     simtemp, %l0        
        st      %f8, [%l0]       
        ld      [%l0],%i0         
        ret        
        restore        
        
_get_f9:        
        save    %sp, -88, %sp           
        set     simtemp, %l0         
        st      %f9, [%l0]        
        ld      [%l0],%i0          
        ret         
        restore         
         
_get_f10:         
        save    %sp, -88, %sp            
        set     simtemp, %l0          
        st      %f10, [%l0]         
        ld      [%l0],%i0           
        ret          
        restore          
          
_get_f11:          
        save    %sp, -88, %sp             
        set     simtemp, %l0           
        st      %f11, [%l0]          
        ld      [%l0],%i0            
        ret           
        restore           
           
_get_f12:           
        save    %sp, -88, %sp              
        set     simtemp, %l0            
        st      %f12, [%l0]           
        ld      [%l0],%i0             
        ret            
        restore            
            
_get_f13:            
        save    %sp, -88, %sp               
        set     simtemp, %l0             
        st      %f13, [%l0]            
        ld      [%l0],%i0              
        ret             
        restore             
             
_get_f14:             
        save    %sp, -88, %sp                
        set     simtemp, %l0              
        st      %f14, [%l0]             
        ld      [%l0],%i0               
        ret              
        restore              
              
_get_f15:              
        save    %sp, -88, %sp                 
        set     simtemp, %l0               
        st      %f15, [%l0]              
        ld      [%l0],%i0                
        ret               
        restore               
               
_get_f16:               
        save    %sp, -88, %sp                  
        set     simtemp, %l0                
        st      %f16, [%l0]               
        ld      [%l0],%i0                 
        ret                
        restore                
                
_get_f17:                
        save    %sp, -88, %sp                   
        set     simtemp, %l0                 
        st      %f17, [%l0]                
        ld      [%l0],%i0                  
        ret                 
        restore                 
                 
_get_f18:                 
        save    %sp, -88, %sp                    
        set     simtemp, %l0                  
        st      %f18, [%l0]                 
        ld      [%l0],%i0                   
        ret                  
        restore                  
                  
_get_f19:                  
        save    %sp, -88, %sp                     
        set     simtemp, %l0                   
        st      %f19, [%l0]                  
        ld      [%l0],%i0                    
        ret                   
        restore                   
                   
_get_f20:                   
        save    %sp, -88, %sp                      
        set     simtemp, %l0                    
        st      %f20, [%l0]                   
        ld      [%l0],%i0                     
        ret                    
        restore                    
                    
_get_f21:                    
        save    %sp, -88, %sp                       
        set     simtemp, %l0                     
        st      %f21, [%l0]                    
        ld      [%l0],%i0                      
        ret                     
        restore                     
                     
_get_f22:                     
        save    %sp, -88, %sp                        
        set     simtemp, %l0                      
        st      %f22, [%l0]                     
        ld      [%l0],%i0                       
        ret                      
        restore                      
                      
_get_f23:                      
        save    %sp, -88, %sp                         
        set     simtemp, %l0                       
        st      %f23, [%l0]                      
        ld      [%l0],%i0                        
        ret                       
        restore                       
                       
_get_f24:                       
        save    %sp, -88, %sp                          
        set     simtemp, %l0                        
        st      %f24, [%l0]                       
        ld      [%l0],%i0                         
        ret                        
        restore                        
                        
_get_f25:                        
        save    %sp, -88, %sp                           
        set     simtemp, %l0                         
        st      %f25, [%l0]                        
        ld      [%l0],%i0                          
        ret                         
        restore                         
                         
_get_f26:                         
        save    %sp, -88, %sp                            
        set     simtemp, %l0                          
        st      %f26, [%l0]                         
        ld      [%l0],%i0                           
        ret                          
        restore                          
                          
_get_f27:                          
        save    %sp, -88, %sp                             
        set     simtemp, %l0                           
        st      %f27, [%l0]                          
        ld      [%l0],%i0                            
        ret                           
        restore                           
                           
_get_f28:                           
        save    %sp, -88, %sp                              
        set     simtemp, %l0                            
        st      %f28, [%l0]                           
        ld      [%l0],%i0                             
        ret                            
        restore                            
                            
_get_f29:                            
        save    %sp, -88, %sp                               
        set     simtemp, %l0                             
        st      %f29, [%l0]                            
        ld      [%l0],%i0                              
        ret                             
        restore                             
                             
_get_f30:                             
        save    %sp, -88, %sp                                
        set     simtemp, %l0                              
        st      %f30, [%l0]                             
        ld      [%l0],%i0                               
        ret                              
        restore                              
                              
_get_f31:                              
        save    %sp, -88, %sp                                 
        set     simtemp, %l0                               
        st      %f31, [%l0]                              
        ld      [%l0],%i0                                
        ret                               
        restore                               
                

_simple2:
	save    %sp, -88, %sp
	set	_reg1m, %l1
	set	_reg1l, %l2
	set	_reg2m, %l3
	set	_reg2l, %l4
	set	simtemp, %l0
	st      %i0, [%l0]
        ld      [%l0], %f0
        ld      [%l0], %f1
        ld      [%l0], %f2
        ld      [%l0], %f3
        ld      [%l0], %f4
        ld      [%l0], %f5
        ld      [%l0], %f6 
        ld      [%l0], %f7 
        ld      [%l0], %f8 
        ld      [%l0], %f9 
        ld      [%l0], %f10 
        ld      [%l0], %f11 
        ld      [%l0], %f12 
        ld      [%l0], %f13 
        ld      [%l0], %f14 
        ld      [%l0], %f15 
        ld      [%l0], %f16 
        ld      [%l0], %f17 
        ld      [%l0], %f18 
        ld      [%l0], %f19 
        ld      [%l0], %f20 
        ld      [%l0], %f21 
        ld      [%l0], %f22 
        ld      [%l0], %f23 
        ld      [%l0], %f24 
        ld      [%l0], %f25 
        ld      [%l0], %f26 
        ld      [%l0], %f27 
        ld      [%l0], %f28 
        ld      [%l0], %f29 
        ld      [%l0], %f30 
        ld      [%l0], %f31

	fcmpd   %f2, %f0
	nop
	nop
	fbe     next1
	nop
	st	%f0, [%l1]
	st	%f1, [%l2]
	st	%f2, [%l3]
	st	%f3, [%l4]
	set	1, %i0
dn1:
dn2:
	ret
	restore

next1:
	fcmpd	%f6, %f4
	nop 
        nop 
        fbe     next2 
        nop 
        st      %f4, [%l1]     
        st      %f5, [%l2]        
        st      %f6, [%l3]      
        st      %f7, [%l4]
	set	2, %i0
	ba	dn1
	nop

next2:
	fcmpd   %f10, %f8 
        nop  
        nop  
        fbe     next3  
        nop  
        st      %f8, [%l1]      
        st      %f9, [%l2]         
        st      %f10, [%l3]       
        st      %f11, [%l4] 
	set	3, %i0
        ba      dn1
	nop

next3:
	fcmpd   %f14, %f12 
        nop  
        nop  
        fbe     next4  
        nop  
        st      %f12, [%l1]      
        st      %f13, [%l2]         
        st      %f14, [%l3]       
        st      %f15, [%l4] 
	set	4, %i0
        ba      dn1
	nop

next4:
	fcmpd   %f18, %f16 
        nop  
        nop  
        fbe     next5  
        nop  
        st      %f16, [%l1]      
        st      %f17, [%l2]         
        st      %f18, [%l3]       
        st      %f19, [%l4] 
	set	5, %i0
        ba      dn1
	nop

next5:
	fcmpd   %f22, %f20 
        nop  
        nop  
        fbe     next6  
        nop  
        st      %f20, [%l1]      
        st      %f21, [%l2]         
        st      %f22, [%l3]       
        st      %f23, [%l4] 
	set	6, %i0
        ba      dn1
	nop

next6:
	fcmpd   %f26, %f24 
        nop  
        nop  
        fbe     next7  
        nop  
        st      %f24, [%l1]      
        st      %f25, [%l2]         
        st      %f26, [%l3]       
        st      %f27, [%l4] 
	set	7, %i0
        ba      dn1
	nop
next7:
	fcmpd   %f30, %f28 
        nop  
        nop  
        fbe     next8  
        nop  
        st      %f28, [%l1]      
        st      %f29, [%l2]         
        st      %f30, [%l3]       
        st      %f31, [%l4] 
	set	8, %i0
        ba      dn1
	nop
next8:
	set	0x0, %i0
	ba	dn2	
	nop
