/*
 *  static char     fpasccsid[] = "@(#)wbranching.c 1.1 2/14/86 Copyright Sun Microsystems";
 */ 
#include <sys/types.h> 
#include "fpa.h"



w_jump_cond_test()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;


        /* clear the pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
	*(u_long *)MODE_WRITE_REGISTER = 0x2;

	/* set the IMASK register to 0 */
	*(u_long *)FPA_IMASK_PTR = 0x0;

	*(u_long *)REGISTER_ONE_MSW = 0x01000000; /*  */
	*(u_long *)REGISTER_TWO_MSW = 0x81000000; 
	*(u_long *)REGISTER_THREE_MSW = 0x3F800000; 
        *(u_long *)REGISTER_FOUR_MSW = 0xBF800000; 
	*(u_long *)REGISTER_FIVE_MSW = 0x40000000;
	*(u_long *)REGISTER_SIX_MSW = 0x70000000; 
	*(u_long *)REGISTER_SEVEN_MSW = 0xF0000000; 
	
	*(u_long *)0xE0000800 = 0x41;  /* reg1 <- sine(reg1) */
	*(u_long *)0xE0000800 = 0x82;  /* reg2 <- sine(reg2) */
	*(u_long *)0xE0000800 = 0xC3;  /* reg3 <- sine(reg3) */
	*(u_long *)0xE0000800 = 0x104; /* reg4 <- sine(reg4) */

	*(u_long *)0xE0000818 = 0x145; /* reg5 <- atan(reg5) */
	*(u_long *)0xE0000818 = 0x186; /* reg6 <- atan(reg6) */
	*(u_long *)0xE0000818 = 0x1C7; /* reg7 <- atan(reg7) */

	val1_msw = *(u_long *)REGISTER_ONE_MSW;
	val1_lsw = *(u_long *)REGISTER_TWO_MSW;
	val2_msw = *(u_long *)REGISTER_THREE_MSW;
	val2_lsw = *(u_long *)REGISTER_FOUR_MSW;
	val3_msw = *(u_long *)REGISTER_FIVE_MSW;
	val3_lsw = *(u_long *)REGISTER_SIX_MSW;
	val4_msw = *(u_long *)REGISTER_SEVEN_MSW;

	if (val1_msw != 0x01000000) 
                           return(-1);
        
        if (val1_lsw != 0x81000000) 
                     return(-1); 
         
        if (val2_msw != 0x3F576AA4)   
                      return(-1);  
          
        if (val2_lsw != 0xBF576AA4)  
                     return(-1);    
              
        if (val3_msw != 0x3F8DB70D) 
                     return(-1);     
              
        if (val3_lsw != 0x3FC90FDB)   
                     return(-1);    
        if (val4_msw != 0xBFC90FDB)    
                     return(-1);    
               

	  /* clear the pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;

	return(0);
}	

