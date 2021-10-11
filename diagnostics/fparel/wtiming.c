/*
 *  static char     fpasccsid[] = "@(#)wtiming.c 1.1 2/14/86 Copyright Sun Microsystems";
 */ 
#include <sys/types.h> 
#include "fpa.h"


/*
 *
 * state_reg = 0;
 * for (i=0; i<1000; i++)
 *
 *  command register instructions - all operations DP
 *  set registers to the following values:
 *     1 = 4 
 *     2 = 8 
 *     3 = 16 
 *     4 = 32     
 *     5 = 0
 *     6 = 0
 *     7 = 0
 *     8 = 0
 *
 *  send instructions to do the following:
 *     reg8 = (e**reg4) - 1      
 *     reg7 = (e**reg3) - 1      
 *     reg6 = (e**reg2) - 1      
 *     reg5 = (e**reg1) - 1      
 *     
 *  read following registers in order specified:
 *     reg5
 *     reg6
 *     reg7
 *     reg8
 *
 *  verify that registers read contain correct value
 *
 *
 *  extended - do all with DP
 *  repeat above process, but use pivot operations instead of (e**x)-1
 *  use reg1 - reg8 as operands and reg9 - reg12 for results
 *
 *  DP short
 *  repeat process with square to test MULT and div to test ALU
 *
 *  SP short
 *  repeat process with square to test MULT and div to test ALU
 */

timing()
{

	u_long i, val1_msw, val1_lsw, val2_msw, val2_lsw, val3_msw, val3_lsw;
	u_long val4_msw, val4_lsw;
	u_long *expo ;


   
        /* clear the pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	expo = (u_long *)0xE0000824; /* address for exponential operation */
	/* set the imask register = 0 */
	*(u_long *)FPA_IMASK_PTR = 0x0;

	for (i = 0; i < 1000; i++) {
		*(u_long *)REGISTER_ONE_MSW = 0x40100000; /* d1ecimal 4 */
		*(u_long *)REGISTER_ONE_LSW = 0x00000000;
		*(u_long *)REGISTER_TWO_MSW = 0x40200000; /* decimal 8 */
		*(u_long *)REGISTER_TWO_LSW = 0x00000000;
		*(u_long *)REGISTER_THREE_MSW = 0x40300000; /* decimal 16 */
		*(u_long *)REGISTER_THREE_LSW = 0x00000000;
		*(u_long *)REGISTER_FOUR_MSW = 0x40400000;  /* decimal 32 */
		*(u_long *)REGISTER_FOUR_LSW = 0x00000000;
		*(u_long *)REGISTER_FIVE_MSW = 0x0000;
		*(u_long *)REGISTER_FIVE_LSW = 0x0000;
		*(u_long *)REGISTER_SIX_MSW = 0x0000;
		*(u_long *)REGISTER_SIX_LSW = 0x0000;
		*(u_long *)REGISTER_SEVEN_MSW = 0x0000;
		*(u_long *)REGISTER_SEVEN_LSW = 0x0000;
 		*(u_long *)REGISTER_EIGHT_MSW = 0x0000;
		*(u_long *)REGISTER_EIGHT_LSW = 0x0000;
		*expo = 0x108; /* reg8 = reg4 op */
		*expo = 0xC7; /* reg7 = reg3 op */
		*expo = 0x86; /* reg6 = reg2 op */
		*expo = 0x45; /* reg5 = reg1 op */
		
		val4_msw = *(u_long *)REGISTER_FIVE_MSW;
		val4_lsw = *(u_long *)REGISTER_FIVE_LSW;
		val3_msw = *(u_long *)REGISTER_SIX_MSW;
		val3_lsw = *(u_long *)REGISTER_SIX_LSW;
		val2_msw = *(u_long *)REGISTER_SEVEN_MSW;
		val2_lsw = *(u_long *)REGISTER_SEVEN_LSW;
		val1_msw = *(u_long *)REGISTER_EIGHT_MSW;
		val1_lsw = *(u_long *)REGISTER_EIGHT_LSW;

		if (val1_msw != 0x42D1F43F) 
				return(-1);
	
		if (val1_lsw != 0xCC4B65EC) 
                                return(-1); 
                if (val2_msw != 0x4160F2EB) 
                                return(-1);    
                      
                if (val2_lsw != 0xB0A80020)  
                                return(-1);    
                      
                if (val3_msw != 0x40A747EA)    
                                return(-1);    
                      
                if (val3_lsw != 0x7D470C6E)    
                                return(-1);    
                      
                if (val4_msw != 0x404ACC90)           
                                return(-1);     
                if (val4_lsw != 0x2E273A58)            
                               return(-1);      
					
	/* extended instructions */
		*(u_long *)REGISTER_TWO_MSW = 0x40100000;
		*(u_long *)REGISTER_TWO_LSW = 0x00000000;  /* decimal 4 */
		*(u_long *)REGISTER_THREE_MSW = 0x40000000;
		*(u_long *)REGISTER_THREE_LSW = 0x00000000; /* decimal 2 */

		*(u_long *)REGISTER_SIX_MSW = 0x40080000;
		*(u_long *)REGISTER_SIX_LSW = 0x00000000;  /* decimal 3 */
		*(u_long *)REGISTER_FIVE_MSW = 0x40300000;
		*(u_long *)REGISTER_FIVE_LSW = 0x00000000; /* decimal 16 */

		*(u_long *)REGISTER_NINE_MSW = 0x40180000;
		*(u_long *)REGISTER_NINE_LSW = 0x00000000; /* decimal 6 */
		*(u_long *)REGISTER_EIGHT_MSW = 0x40000000;
		*(u_long *)REGISTER_EIGHT_LSW = 0x00000000; /* decimal 2 */

		*(u_long *)REGISTER_TWELVE_MSW = 0x40080000;
		*(u_long *)REGISTER_TWELVE_LSW = 0x00000000; /* decimal 3 */
		*(u_long *)REGISTER_ELEVEN_MSW = 0x40000000;
		*(u_long *)REGISTER_ELEVEN_LSW = 0x00000000; /* decimal 2 */
		*(u_long *)0xE0001814 = 0x40000000;
		*(u_long *)0xE0001988 = 0x00000000;
		*(u_long *)0xE00018B4 = 0x40000000;
		*(u_long *)0xE0001AA0 = 0x00000000;
		*(u_long *)0xE000194C = 0x40000000; 
                *(u_long *)0xE0001C38 = 0x00000000; 
                *(u_long *)0xE00019E4 = 0x40000000;  
                *(u_long *)0xE0001DD0 = 0x00000000;
			
		val1_msw = *(u_long *)REGISTER_ONE_MSW;
		val1_lsw = *(u_long *)REGISTER_ONE_LSW;
		val2_msw = *(u_long *)REGISTER_FOUR_MSW;
		val2_lsw = *(u_long *)REGISTER_FOUR_LSW;
		val3_msw = *(u_long *)REGISTER_SEVEN_MSW;
		val3_lsw = *(u_long *)REGISTER_SEVEN_LSW;
		val4_msw = *(u_long *)REGISTER_TEN_MSW;
		val4_lsw = *(u_long *)REGISTER_TEN_LSW;

		if (val1_msw != 0x40240000) 
                                return(-1);
                
		if (val1_lsw != 0x00000000) 
                                return(-1); 
		if (val2_msw != 0x40240000)  
                                return(-1);  
                if (val2_lsw != 0x00000000) 
                                return(-1);    
                if (val3_msw != 0x40240000)   
                                return(-1);    
                if (val3_lsw != 0x00000000)  
                                return(-1);    
                if (val4_msw != 0x40240000)   
                                return(-1);    
                if (val4_lsw != 0x00000000)   
                               return(-1);

		*(u_long *)REGISTER_TWO_MSW = 0x40100000;
                *(u_long *)REGISTER_TWO_LSW = 0x00000000;  /* decimal 4 */
                *(u_long *)REGISTER_THREE_MSW = 0x40140000;
		*(u_long *)REGISTER_THREE_LSW = 0x00000000; /* decimal 5 */
		
		*(u_long *)REGISTER_SIX_MSW = 0x40000000;
		*(u_long *)REGISTER_SIX_LSW = 0x00000000;  /* decimal 2 */
		*(u_long *)REGISTER_FIVE_MSW = 0x40000000;
                *(u_long *)REGISTER_FIVE_LSW = 0x00000000; /* decimal 2 */

		*(u_long *)REGISTER_NINE_MSW = 0x40000000;
                *(u_long *)REGISTER_NINE_LSW = 0x00000000; /* decimal 2 */		
		*(u_long *)REGISTER_EIGHT_MSW = 0x40080000;
                *(u_long *)REGISTER_EIGHT_LSW = 0x00000000; /* decimal 3 */

		*(u_long *)REGISTER_TWELVE_MSW = 0x40000000;
                *(u_long *)REGISTER_TWELVE_LSW = 0x00000000; /* decimal 2 */
		*(u_long *)REGISTER_ELEVEN_MSW = 0x40080000;
		*(u_long *)REGISTER_ELEVEN_LSW = 0x00000000; /* decimal 3 */
		*(u_long *)0xE0001A14 = 0x40000000;
		*(u_long *)0xE0001988 = 0x00000000;
                *(u_long *)0xE0001AB4 = 0x401C0000;
		*(u_long *)0xE0001AA0 = 0x00000000;  
                *(u_long *)0xE0001B4C = 0x40100000;  
                *(u_long *)0xE0001C38 = 0x00000000;    
                *(u_long *)0xE0001BE4 = 0x40300000;
		*(u_long *)0xE0001DD0 = 0x00000000;
         
                val1_msw = *(u_long *)REGISTER_ONE_MSW;
                val1_lsw = *(u_long *)REGISTER_ONE_LSW;
                val2_msw = *(u_long *)REGISTER_FOUR_MSW;
                val2_lsw = *(u_long *)REGISTER_FOUR_LSW;
                val3_msw = *(u_long *)REGISTER_SEVEN_MSW;
                val3_lsw = *(u_long *)REGISTER_SEVEN_LSW;
                val4_msw = *(u_long *)REGISTER_TEN_MSW;
                val4_lsw = *(u_long *)REGISTER_TEN_LSW;
 
                if (val1_msw != 0x40240000)
                                return(-1);
                if (val1_lsw != 0x00000000)
                                return(-1); 
                if (val2_msw != 0x40240000)  
                                return(-1);  
                if (val2_lsw != 0x00000000) 
                                return(-1);    
                if (val3_msw != 0x40240000)   
                                return(-1);    
                if (val3_lsw != 0x00000000)  
                                return(-1);    
                if (val4_msw != 0x40240000)    
                                return(-1);    
                if (val4_lsw != 0x00000000)  
                                 return(-1);   

		*(u_long *)REGISTER_TWO_MSW = 0x40000000;
                *(u_long *)REGISTER_TWO_LSW = 0x00000000;  /* decimal 2 */
		*(u_long *)REGISTER_THREE_MSW = 0x40180000;
                *(u_long *)REGISTER_THREE_LSW = 0x00000000; /* decimal 6 */
	
		*(u_long *)REGISTER_SIX_MSW = 0x3FF00000;
                *(u_long *)REGISTER_SIX_LSW = 0x00000000;  /* decimal 1 */
		*(u_long *)REGISTER_FIVE_MSW = 0x40100000;
                *(u_long *)REGISTER_FIVE_LSW = 0x00000000; /* decimal 4 */

		*(u_long *)REGISTER_NINE_MSW = 0x3FF00000; 
		*(u_long *)REGISTER_NINE_LSW = 0x00000000; /* decimal 1 */
		*(u_long *)REGISTER_EIGHT_MSW = 0x40180000; 
		*(u_long *)REGISTER_EIGHT_LSW = 0x00000000; /* decimal 6 */
		*(u_long *)0xE0001C14 = 0x40000000;
                *(u_long *)0xE0001988 = 0x00000000;
                *(u_long *)0xE0001CB4 = 0x40000000;
                *(u_long *)0xE0001AA0 = 0x00000000;   
                *(u_long *)0xE0001D4C = 0x40000000;    
                *(u_long *)0xE0001C38 = 0x00000000;   

		val1_msw = *(u_long *)REGISTER_ONE_MSW;
                val1_lsw = *(u_long *)REGISTER_ONE_LSW;
                val2_msw = *(u_long *)REGISTER_FOUR_MSW;
                val2_lsw = *(u_long *)REGISTER_FOUR_LSW;
                val3_msw = *(u_long *)REGISTER_SEVEN_MSW;
                val3_lsw = *(u_long *)REGISTER_SEVEN_LSW;

		if (val1_msw != 0x40240000)
                                return(-1);
                if (val1_lsw != 0x00000000)
                                return(-1); 
                if (val2_msw != 0x40240000)  
                                return(-1);  
                if (val2_lsw != 0x00000000) 
                                return(-1);    
                if (val3_msw != 0x40240000)   
                                return(-1);    
                if (val3_lsw != 0x00000000)  
                                return(-1);    

	/* for DP short  square*/
		*(u_long *)REGISTER_ONE_MSW = 0x0;
		*(u_long *)REGISTER_ONE_LSW = 0x0;
		*(u_long *)REGISTER_TWO_MSW = 0x0;
		*(u_long *)REGISTER_TWO_LSW = 0x0; 
		*(u_long *)REGISTER_THREE_MSW = 0x0;
		*(u_long *)REGISTER_THREE_LSW = 0x0;
		*(u_long *)REGISTER_FOUR_MSW = 0x0; 
		*(u_long *)REGISTER_FOUR_LSW = 0x0;
		*(u_long *)0xE000030C = 0x40000000;  /* reg1 = square of (operand) */
		*(u_long *)0xE0001000 = 0x00000000;  /* square  for decimal 2 */
		*(u_long *)0xE0000314 = 0x40080000;  /* reg2 */
		*(u_long *)0xE0001000 = 0x00000000;  /* square  for decimal 3 */
 		*(u_long *)0xE000031C = 0x40100000;  /* reg3 */
		*(u_long *)0xE0001000 = 0x00000000;  /* square  for decimal 4 */
		*(u_long *)0xE0000324 = 0x3FF00000;  /* reg4 */
		*(u_long *)0xE0001000 = 0x00000000;  /* square  for decimal 1 */

                val1_msw = *(u_long *)REGISTER_ONE_MSW;
                val1_lsw = *(u_long *)REGISTER_ONE_LSW;
                val2_msw = *(u_long *)REGISTER_TWO_MSW;
                val2_lsw = *(u_long *)REGISTER_TWO_LSW;
                  
                val3_msw = *(u_long *)REGISTER_THREE_MSW;
		val3_lsw = *(u_long *)REGISTER_THREE_LSW;
                val4_msw = *(u_long *)REGISTER_FOUR_MSW;
		val4_lsw = *(u_long *)REGISTER_FOUR_LSW;
	
		if (val1_msw != 0x40100000)
                                return(-1);
                if (val1_lsw != 0x00000000) 
                                return(-1); 
                if (val2_msw != 0x40220000)  
                                return(-1);  
                if (val2_lsw != 0x00000000)  
                                return(-1);    
                if (val3_msw != 0x40300000)   
                                return(-1);    
                if (val3_lsw != 0x00000000)   
                                return(-1);    
                if (val4_msw != 0x3FF00000)    
                                return(-1);    
                if (val4_lsw != 0x00000000)   
                               return(-1);
		
                *(u_long *)REGISTER_ONE_MSW = 0x40180000; /* 6 */
                *(u_long *)REGISTER_ONE_LSW = 0x0;
                *(u_long *)REGISTER_TWO_MSW = 0x40180000; /* 6 */
                *(u_long *)REGISTER_TWO_LSW = 0x0; 
                *(u_long *)REGISTER_THREE_MSW = 0x40200000; /* 8 */
                *(u_long *)REGISTER_THREE_LSW = 0x0;
                *(u_long *)REGISTER_FOUR_MSW = 0x40240000; /* 10 */ 
                *(u_long *)REGISTER_FOUR_LSW = 0x0;
		*(u_long *)0xE000050C = 0x40080000; /* reg1 */
		*(u_long *)0xE0001000 = 0x00000000; /* reg1 = reg1 /operand(3) */	
		*(u_long *)0xE0000514 = 0x40000000; /* reg2 = reg2 / operand */
		*(u_long *)0xE0001000 = 0x00000000;  /* divide for op = 2 */
		*(u_long *)0xE000051C = 0x40100000; /* reg3 */
		*(u_long *)0xE0001000 = 0x00000000;  /* divide for op = 4*/
		*(u_long *)0xE0000524 = 0x40000000; /* reg4 */
		*(u_long *)0xE0001000 = 0x00000000;  /* divide for op = 2 */

		val1_msw = *(u_long *)REGISTER_ONE_MSW;
                val1_lsw = *(u_long *)REGISTER_ONE_LSW;
                val2_msw = *(u_long *)REGISTER_TWO_MSW;
                val2_lsw = *(u_long *)REGISTER_TWO_LSW;
                  
                val3_msw = *(u_long *)REGISTER_THREE_MSW;
                val3_lsw = *(u_long *)REGISTER_THREE_LSW;
                val4_msw = *(u_long *)REGISTER_FOUR_MSW;
                val4_lsw = *(u_long *)REGISTER_FOUR_LSW;
         
		if (val1_msw != 0x40000000) 
                                return(-1);
                if (val1_lsw != 0x00000000) 
                                return(-1); 
                if (val2_msw != 0x40080000)   
                                return(-1);  
                if (val2_lsw != 0x00000000)  
                                return(-1);    
                if (val3_msw != 0x40000000)    
                                return(-1);    
                if (val3_lsw != 0x00000000)   
                                return(-1);    
                if (val4_msw != 0x40140000)    
                                return(-1);    
                if (val4_lsw != 0x00000000)   
                               return(-1);

	/* single precision short */
		*(u_long *)REGISTER_ONE_MSW = 0x00000000; /* 1 */
		*(u_long *)REGISTER_TWO_MSW = 0x00000000; /* 2 */
		*(u_long *)REGISTER_THREE_MSW = 0x00000000; /* 3 */
		*(u_long *)REGISTER_FOUR_MSW = 0x00000000;  /* 4 */
		*(u_long *)0xE0000308 = 0x3F800000; /* reg1 = square of 1 */
		*(u_long *)0xE0000310 = 0x40000000; /* reg2 = square of 2 */
		*(u_long *)0xE0000318 = 0x40400000; /* reg3 = square of 3 */
		*(u_long *)0xE0000320 = 0x40800000; /* reg4 = square of 4 */

		val1_msw = *(u_long *)REGISTER_ONE_MSW;
		val2_msw = *(u_long *)REGISTER_TWO_MSW;
		val3_msw = *(u_long *)REGISTER_THREE_MSW;
		val4_msw = *(u_long *)REGISTER_FOUR_MSW;

		if (val1_msw != 0x3F800000)
                                return(-1);
		if (val2_msw != 0x40800000)   
                                return(-1); 
		if (val3_msw != 0x41100000)   
                                return(-1);    
		if (val4_msw != 0x41800000)    
                               return(-1);

		*(u_long *)REGISTER_ONE_MSW = 0x40C00000; /* 6 */
                *(u_long *)REGISTER_TWO_MSW = 0x40C00000; /* 6 */
		*(u_long *)REGISTER_THREE_MSW = 0x41000000; /* 8 */
                *(u_long *)REGISTER_FOUR_MSW = 0x41800000;  /* 16 */
		*(u_long *)0xE0000508 = 0x40400000; /* reg1 = reg1 div 3 */
		*(u_long *)0xE0000510 = 0x40000000; /* reg2 = reg2 div 2 */
		*(u_long *)0xE0000518 = 0x40800000; /* reg3 = reg3 div 4 */
		*(u_long *)0xE0000520 = 0x40800000; /* reg4 = reg4 div 4 */	

		val1_msw = *(u_long *)REGISTER_ONE_MSW;
		val2_msw = *(u_long *)REGISTER_TWO_MSW;
		val3_msw = *(u_long *)REGISTER_THREE_MSW;
		val4_msw = *(u_long *)REGISTER_FOUR_MSW;

		if (val1_msw != 0x40000000) 
                                return(-1);
                if (val2_msw != 0x40400000) 
                                return(-1);  
                if (val3_msw != 0x40000000) 
                                return(-1);    
		if (val4_msw != 0x40800000) 
                             return(-1);    
	}
	        /* clear the pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
	return(0);
}			
