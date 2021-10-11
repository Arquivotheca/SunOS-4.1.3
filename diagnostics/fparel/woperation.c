/*
 *  static char     fpasccsid[] = "@(#)woperation.c 1.1 2/14/86 Copyright Sun Microsystems";
 */
#include <sys/types.h> 
#include "fpa.h"


/*
 * Weitek operation 
 */

w_op_test()
{
	u_long	temp_val;
	u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
	u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;
	u_long  *opcode_addr;


        /* clear the pipe */
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	/* set the IMASK register = 0 */
	*(u_long *)FPA_IMASK_PTR = 0x0;

	w_op_com_sp();
	w_op_com_dp();
	w_op_ext_opr();
	w_op_ext_sp();
	w_op_ext_dp();
	w_op_short_sp();
	w_op_short_doublep();	

	/* clear the pipe */ 
        *(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
	return(0);

}	   
w_op_com_sp()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;

	*(u_long *)REGISTER_ONE_MSW = 0x3F800000; /* 1 */	
	*(u_long *)REGISTER_TWO_MSW = 0x3F800000; /* 1 */
	*(u_long *)REGISTER_THREE_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_FOUR_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_SIX_MSW = 0x3F800000; /* 1 */
	*(u_long *)REGISTER_SEVEN_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_EIGHT_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_TEN_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_ELEVEN_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_TWELVE_MSW = 0x40000000; /* 2 */

	*(u_long *)0xE0000880 = 0x40;	/* reg0 <- reg1 */
	*(u_long *)0xE0000888 = 0x10030081; /* reg1 <- reg3 + (reg2 * reg4) */
	*(u_long *)0xE0000890 = 0x20070185; /* reg5 <- reg7 - (reg6 * reg8) */
	*(u_long *)0xE0000898 = 0x300B0289; /* reg9 <- (-reg11) + (reg10 * reg12) */
	
	val1_msw = *(u_long *)REGISTER_ZERO_MSW;
	val2_msw = *(u_long *)REGISTER_ONE_MSW;
	val3_msw = *(u_long *)REGISTER_FIVE_MSW;
	val4_msw = *(u_long *)REGISTER_NINE_MSW;

	*(u_long *)REGISTER_TWO_MSW = 0x40000000; /* 2 */ 
        *(u_long *)REGISTER_THREE_MSW = 0x40000000; /* 2 */ 
        *(u_long *)REGISTER_FOUR_MSW = 0x3F800000; /* 1 */ 
        *(u_long *)REGISTER_SIX_MSW = 0x40800000; /* 4 */ 
        *(u_long *)REGISTER_SEVEN_MSW = 0x40400000; /* 3 */ 
        *(u_long *)REGISTER_EIGHT_MSW = 0x40000000; /* 2 */ 
        *(u_long *)REGISTER_TEN_MSW = 0x3F800000; /* 1 */ 
        *(u_long *)REGISTER_ELEVEN_MSW = 0x40400000; /* 3 */ 
        *(u_long *)REGISTER_TWELVE_MSW = 0x40800000; /* 4 */ 
	
	*(u_long *)0xE00008A0 = 0x10030081; /* reg1 <- reg3 * (reg2 + reg4) */
	*(u_long *)0xE00008A8 = 0x20070185; /* reg5 <- reg7 * (reg6 - reg8) */	
	*(u_long *)0xE00008B0 = 0x300B0289; /* reg9 <- reg11 * (-reg10 + reg12) */

	val1_lsw = *(u_long *)REGISTER_ONE_MSW;
	val2_lsw = *(u_long *)REGISTER_FIVE_MSW; 
	val3_lsw = *(u_long *)REGISTER_NINE_MSW;   

	if (val1_msw != 0x3F800000) 
                           return(-1);
        
	if (val2_msw != 0x40A00000) 
                          return(-1); 
         
	if (val3_msw != 0x3F800000)   
                          return(-1);    
              
        if (val4_msw != 0x40400000)    
                          return(-1);
            
        if (val1_lsw != 0x40C00000) 
                          return(-1); 
         
        if (val2_lsw != 0x40C00000)  
                         return(-1);    
              
        if (val3_lsw != 0x41100000)   
                        return(-1);
            
	return(0);

}
w_op_com_dp()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;
	
        *(u_long *)REGISTER_ONE_MSW = 0x3FF00000; /* 1 */       
	*(u_long *)REGISTER_ONE_LSW = 0x0;
        *(u_long *)REGISTER_TWO_MSW = 0x3FF00000; /* 1 */
	*(u_long *)REGISTER_TWO_LSW = 0x0;
        *(u_long *)REGISTER_THREE_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_THREE_LSW = 0x0;
        *(u_long *)REGISTER_FOUR_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_FOUR_LSW = 0x0;
        *(u_long *)REGISTER_SIX_MSW = 0x3FF00000; /* 1 */
	*(u_long *)REGISTER_SIX_LSW = 0x0;
        *(u_long *)REGISTER_SEVEN_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_SEVEN_LSW = 0x0;
        *(u_long *)REGISTER_EIGHT_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_EIGHT_LSW = 0x0;
        *(u_long *)REGISTER_TEN_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_TEN_LSW = 0x0;
        *(u_long *)REGISTER_ELEVEN_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_ELEVEN_LSW = 0x0;
        *(u_long *)REGISTER_TWELVE_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_TWELVE_LSW = 0x0;

        *(u_long *)0xE0000884 = 0x40;   /* reg0 <- reg1 */
        *(u_long *)0xE000088C = 0x10030081; /* reg1 <- reg3 + (reg2 * reg4) */
        *(u_long *)0xE0000894 = 0x20070185; /* reg5 <- reg7 - (reg6 * reg8) */
        *(u_long *)0xE000089C = 0x300B0289; /* reg9 <- (-reg11) + (reg10 * reg12) */
        
        val1_msw = *(u_long *)REGISTER_ZERO_MSW;
	val1_lsw = *(u_long *)REGISTER_ZERO_LSW;
        val2_msw = *(u_long *)REGISTER_ONE_MSW;
	val2_lsw = *(u_long *)REGISTER_ONE_LSW;
        val3_msw = *(u_long *)REGISTER_FIVE_MSW;
	val3_lsw = *(u_long *)REGISTER_FIVE_LSW;
        val4_msw = *(u_long *)REGISTER_NINE_MSW;
	val4_lsw = *(u_long *)REGISTER_NINE_LSW;
	 

        if (val1_msw != 0x3FF00000) 
                     return(-1);
        
        if (val1_lsw != 0x00000000) 
                     return(-1); 
         
        if (val2_msw != 0x40140000)   
                      return(-1);  
          
        if (val2_lsw != 0x00000000)  
                     return(-1);    
              
        if (val3_msw != 0x3FF00000)    
                     return(-1);    
              
        if (val3_lsw != 0x00000000)   
                     return(-1);    
              
        if (val4_msw != 0x40080000)    
                     return(-1);    
               
        if (val4_lsw != 0x00000000)   
                     return(-1);
            

	*(u_long *)REGISTER_TWO_MSW = 0x40000000; /* 2 */
        *(u_long *)REGISTER_TWO_LSW = 0x0;
        *(u_long *)REGISTER_THREE_MSW = 0x40000000; /* 2 */
        *(u_long *)REGISTER_THREE_LSW = 0x0;
        *(u_long *)REGISTER_FOUR_MSW = 0x3FF00000; /* 1 */
        *(u_long *)REGISTER_FOUR_LSW = 0x0;
        *(u_long *)REGISTER_SIX_MSW = 0x40100000; /* 4 */
        *(u_long *)REGISTER_SIX_LSW = 0x0;
        *(u_long *)REGISTER_SEVEN_MSW = 0x40080000; /* 3 */
        *(u_long *)REGISTER_SEVEN_LSW = 0x0;
        *(u_long *)REGISTER_EIGHT_MSW = 0x40000000; /* 2 */
        *(u_long *)REGISTER_EIGHT_LSW = 0x0;
        *(u_long *)REGISTER_TEN_MSW = 0x3FF00000; /* 1*/
        *(u_long *)REGISTER_TEN_LSW = 0x0;
        *(u_long *)REGISTER_ELEVEN_MSW = 0x40080000; /* 3 */
        *(u_long *)REGISTER_ELEVEN_LSW = 0x0;
        *(u_long *)REGISTER_TWELVE_MSW = 0x40100000; /* 4 */
        *(u_long *)REGISTER_TWELVE_LSW = 0x0;

        *(u_long *)0xE00008A4 = 0x10030081; /* reg1 <- reg3 * (reg2 + reg4) */
        *(u_long *)0xE00008AC = 0x20070185; /* reg5 <- reg7 * (reg6 - reg8) */   
        *(u_long *)0xE00008B4 = 0x300B0289; /* reg9 <- reg11 * (-reg10 + reg12) */

        val2_msw = *(u_long *)REGISTER_ONE_MSW;
        val2_lsw = *(u_long *)REGISTER_ONE_LSW;
        val3_msw = *(u_long *)REGISTER_FIVE_MSW;
        val3_lsw = *(u_long *)REGISTER_FIVE_LSW;
        val4_msw = *(u_long *)REGISTER_NINE_MSW;
        val4_lsw = *(u_long *)REGISTER_NINE_LSW;
         
        if (val2_msw != 0x40180000)   
                      return(-1);  
          
        if (val2_lsw != 0x00000000)  
                     return(-1);    
              
        if (val3_msw != 0x40180000)    
                     return(-1);    
              
        if (val3_lsw != 0x00000000)   
                     return(-1);    
                 
        if (val4_msw != 0x40220000)    
                     return(-1);    
               
        if (val4_lsw != 0x00000000)   
                           return(-1);
        
}
w_op_ext_opr()
{
        u_long  temp_val;
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;

        *(u_long *)REGISTER_TWO_MSW = 0x40000000; /* 2 */
        *(u_long *)REGISTER_FOUR_MSW = 0x40400000; /* 3 */
        *(u_long *)REGISTER_SIX_MSW = 0x40800000; /* 4 */
        *(u_long *)REGISTER_EIGHT_MSW = 0x3F800000; /* 1 */
        *(u_long *)REGISTER_TEN_MSW = 0x40800000; /* 4 */
        *(u_long *)REGISTER_TWELVE_MSW = 0xC0000000; /*-2 */

        *(u_long *)0xE0001010 = 0x3F800000; /* 1 *//* reg1 <- op2 + (reg2 * op1) */
        *(u_long *)0xE0001808 = 0x40800000; /* 4  op2 */
        *(u_long *)0xE00010A0 = 0x3F800000; /* 1 */ /* reg3 <- op2 - (reg4 * op1) */
        *(u_long *)0xE0001818 = 0x41100000; /* 9 op2 */
        *(u_long *)0xE0001130 = 0x40000000; /* 2 *//* reg5 <- (-op2) + (reg6 * op1) */
        *(u_long *)0xE0001828 = 0x40000000; /* 2 op2 */
        *(u_long *)0xE00011C0 = 0x3F800000; /* 1 *//* reg7 <- op2 * (reg8 + op1) */
        *(u_long *)0xE0001838 = 0x40400000; /* 3 op2 */
        *(u_long *)0xE0001250 = 0x3F800000; /* 1 *//* reg9 <- op2 * (reg10 - op1) */
        *(u_long *)0xE0001848 = 0x40000000; /* 2 op2 */
        *(u_long *)0xE00012E0 = 0x3F800000; /* 1 */ /* reg11 <- op2 * (-reg12 + op1) */
        *(u_long *)0xE0001858 = 0x40000000; /* 2  op2 */

        val1_msw = *(u_long *)REGISTER_ONE_MSW;
        val2_msw = *(u_long *)REGISTER_THREE_MSW;
        val3_msw = *(u_long *)REGISTER_FIVE_MSW;
        val4_msw = *(u_long *)REGISTER_SEVEN_MSW;
        val1_lsw = *(u_long *)REGISTER_NINE_MSW;
        val2_lsw = *(u_long *)REGISTER_ELEVEN_MSW;
 
       if (val1_msw != 0x40C00000) 
                          return(-1);

        if (val2_msw != 0x40C00000)
                          return(-1); 
         
        if (val3_msw != 0x40C00000)  
                          return(-1);    
              
        if (val4_msw != 0x40C00000)   
                          return(-1);
            
        if (val1_lsw != 0x40C00000) 
                          return(-1); 
         
        if (val2_lsw != 0x40C00000)  
                         return(-1);    
              

}
w_op_ext_sp()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;

        *(u_long *)REGISTER_TWO_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_FOUR_MSW = 0x41000000; /* 8 */
	*(u_long *)REGISTER_SIX_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_EIGHT_MSW = 0x41100000; /* 9 */
        *(u_long *)REGISTER_TEN_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_TWELVE_MSW = 0x40400000; /* 3 */

	*(u_long *)0xE0001310 = 0x40400000; /* 3 */ /* reg1 <- reg2 + operand */
	*(u_long *)0xE0001808 = 0x0;
	*(u_long *)0xE00013A0 = 0x40000000; /* 2*/ /* reg3 <- reg4 - operand */
	*(u_long *)0xE0001818 = 0x0;
	*(u_long *)0xE0001430 = 0x40000000; /* 2 */ /* reg5 <- reg6 * operand */
	*(u_long *)0xE0001828 = 0x0;
	*(u_long *)0xE00014C0 = 0x40400000; /* 3 */ /* reg7 <- reg8 / operand */
	*(u_long *)0xE0001838 = 0x0;
	*(u_long *)0xE0001550 = 0x41100000; /* 9 */ /* reg9 <- operand - reg10 */
	*(u_long *)0xE0001848 = 0x0;
	*(u_long *)0xE00015E0 = 0x41100000; /* 9 */ /* reg11 <- operand / reg12 */
	*(u_long *)0xE0001858 = 0x0;

	val1_msw = *(u_long *)REGISTER_ONE_MSW;
	val1_lsw = *(u_long *)REGISTER_THREE_MSW;
	val2_msw = *(u_long *)REGISTER_FIVE_MSW;
	val2_lsw = *(u_long *)REGISTER_SEVEN_MSW;
	val3_msw = *(u_long *)REGISTER_NINE_MSW;
	val3_lsw = *(u_long *)REGISTER_ELEVEN_MSW;

	if (val1_msw != 0x40C00000)
                           return(-1);
        
        if (val1_lsw != 0x40C00000) 
                           return(-1); 
              
        if (val2_msw != 0x40C00000) 
                          return(-1); 
         
	if (val2_lsw != 0x40400000)  
                          return(-1);  
              
	if (val3_msw != 0x40C00000)   
                          return(-1);    
              
	if (val3_lsw != 0x40400000)   
                          return(-1);          
              
	
}
w_op_ext_dp()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;
	u_long  val5_msw, val5_lsw, val6_msw, val6_lsw;

        *(u_long *)REGISTER_TWO_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_TWO_LSW = 0x0;
        *(u_long *)REGISTER_FOUR_MSW = 0x40200000; /* 8 */
	*(u_long *)REGISTER_FOUR_LSW = 0x0;
        *(u_long *)REGISTER_SIX_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_SIX_LSW = 0x0;
        *(u_long *)REGISTER_EIGHT_MSW = 0x40220000; /* 9 */
	*(u_long *)REGISTER_EIGHT_LSW = 0x0;
        *(u_long *)REGISTER_TEN_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_TEN_LSW = 0x0;
        *(u_long *)REGISTER_TWELVE_MSW = 0x40080000; /* 3*/
	*(u_long *)REGISTER_TWELVE_LSW = 0x0;

        *(u_long *)0xE0001314 = 0x40080000; /* 3*/ /* reg1 <- reg2 + operand */
        *(u_long *)0xE000180C = 0x0;
        *(u_long *)0xE00013A4 = 0x40000000; /* 2 */ /* reg3 <- reg4 - operand */
        *(u_long *)0xE000181C = 0x0;
        *(u_long *)0xE0001434 = 0x40000000; /* 2 */ /* reg5 <- reg6 * operand */
        *(u_long *)0xE000182C = 0x0;
        *(u_long *)0xE00014C4 = 0x40080000; /* 3*/ /* reg7 <- reg8 / operand */
        *(u_long *)0xE000183C = 0x0;
        *(u_long *)0xE0001554 = 0x40220000; /* 9 */ /* reg9 <- operand - reg10 */
        *(u_long *)0xE000184C = 0x0;
        *(u_long *)0xE00015E4 = 0x40220000; /* 9 */ /* reg11 <- operand / reg12 */
        *(u_long *)0xE000185C = 0x0;

        val1_msw = *(u_long *)REGISTER_ONE_MSW;
	val1_lsw = *(u_long *)REGISTER_ONE_LSW;
	val2_msw = *(u_long *)REGISTER_THREE_MSW;
	val2_lsw = *(u_long *)REGISTER_THREE_LSW;
	val3_msw = *(u_long *)REGISTER_FIVE_MSW;
	val3_lsw = *(u_long *)REGISTER_FIVE_LSW;
	val4_msw = *(u_long *)REGISTER_SEVEN_MSW;
	val4_lsw = *(u_long *)REGISTER_SEVEN_LSW;
	val5_msw = *(u_long *)REGISTER_NINE_MSW;
	val5_lsw = *(u_long *)REGISTER_NINE_LSW;
	val6_msw = *(u_long *)REGISTER_ELEVEN_MSW;
	val6_lsw = *(u_long *)REGISTER_ELEVEN_LSW;


        if (val1_msw != 0x40180000)
                     return(-1);
        
        if (val1_lsw != 0x00000000)
                     return(-1); 
         
        if (val2_msw != 0x40180000)  
                      return(-1);  
          
        if (val2_lsw != 0x00000000) 
                     return(-1);   
              
        if (val3_msw != 0x40180000)    
                     return(-1);    
              
        if (val3_lsw != 0x00000000)   
                     return(-1);    
              
        if (val4_msw != 0x40080000)    
                     return(-1);    
               
        if (val4_lsw != 0x00000000)   
                     return(-1);
            
        if (val5_msw != 0x40180000)    
                     return(-1);    
               
        if (val5_lsw != 0x00000000)   
                     return(-1);
            
        if (val6_msw != 0x40080000)    
                     return(-1);    
               
        if (val6_lsw != 0x00000000)   
                     return(-1);
            
	
}
w_op_short_sp()
{
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;
        u_long  val5_msw, val5_lsw, val6_msw, val6_lsw;
        u_long  val7_msw, val7_lsw, val8_msw, val8_lsw;
        u_long  val9_msw, val9_lsw, val10_msw, val10_lsw;
        u_long  val11_msw, val11_lsw, val12_msw, val12_lsw;
	u_long  temp_res; 
	
	*(u_long *)REGISTER_SEVEN_MSW = 0x3F800000; /* 1 */
	*(u_long *)REGISTER_EIGHT_MSW = 0x40C00000; /* 6 */
	*(u_long *)REGISTER_NINE_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_TEN_MSW = 0x41100000; /* 9 */
	*(u_long *)REGISTER_ELEVEN_MSW = 0x40400000; /* 3 */
	*(u_long *)REGISTER_TWELVE_MSW = 0x40400000; /* 3 */

	*(u_long *)0xE0000088 = 0x3F800000; /* 1  reg1 <- negate operand */	
        *(u_long *)0xE0000110 = 0x40400000; /* 3 *//* reg2 <- absolute value operand */
        *(u_long *)0xE0000198 = 0x4;/* 4*/ /* reg3 <- float operand */
        *(u_long *)0xE0000220 = 0x41800000; /* 16 */ /* reg4 <- fix operand */
        *(u_long *)0xE00002A8 = 0x41100000; /* 9 *//* reg5 <- convert (dp to sp) operand */
        *(u_long *)0xE0000330 = 0x40800000; /* 4 */ /* reg6 <- square of operand */
        *(u_long *)0xE00003B8 = 0x3F800000; /* 1 */ /* reg7 <- reg7 + operand */
        *(u_long *)0xE0000440 = 0x40000000; /* 2*/ /* reg8 <- reg8 - operand */
        *(u_long *)0xE00004C8 = 0x40000000; /* 2 */ /* reg9 <- reg9 * operand */
	*(u_long *)0xE0000550 = 0x40400000; /* 3 *//* reg10 <- reg10 / operand */
        *(u_long *)0xE00005D8 = 0x41100000; /* 9 */ /* reg11 <- operand - reg11 */
        *(u_long *)0xE0000660 = 0x41100000; /* 9 */ /* reg12 <- operand / reg12 */

        val1_msw = *(u_long *)REGISTER_ONE_MSW;
        val2_msw = *(u_long *)REGISTER_TWO_MSW;
        val3_msw = *(u_long *)REGISTER_THREE_MSW;
        val4_msw = *(u_long *)REGISTER_FOUR_MSW;
        val5_msw = *(u_long *)REGISTER_FIVE_MSW;
        val6_msw = *(u_long *)REGISTER_SIX_MSW;
        val7_msw = *(u_long *)REGISTER_SEVEN_MSW;
        val8_msw = *(u_long *)REGISTER_EIGHT_MSW;
        val9_msw = *(u_long *)REGISTER_NINE_MSW;
        val10_msw = *(u_long *)REGISTER_TEN_MSW;
        val11_msw = *(u_long *)REGISTER_ELEVEN_MSW;
        val12_msw = *(u_long *)REGISTER_TWELVE_MSW;

        if (val1_msw != 0xBF800000) 
                     return(-1);
	
        if (val2_msw != 0x40400000) 
                     return(-1);
        
        if (val3_msw != 0x40800000) 
                     return(-1);
        
        if (val4_msw != 0x10) 
                     return(-1);
        
        if (val5_msw != 0x40220000) 
                     return(-1);
        
        if (val6_msw != 0x41800000) 
                     return(-1);
        
        if (val7_msw != 0x40000000) 
                     return(-1);
        
        if (val8_msw != 0x40800000) 
                     return(-1);
        
        if (val9_msw != 0x40C00000) 
                     return(-1);
        
        if (val10_msw != 0x40400000) 
                     return(-1);
        
        if (val11_msw != 0x40C00000) 
                     return(-1);
        
        if (val12_msw != 0x40400000) 
                     return(-1);
        
        *(u_long *)0xE0000680 = 0x3F800000; /* operand compare with 0 */

        temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC);
	val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */ 
        val1_lsw = temp_res & 0xF; /* get the decoded value */ 
        if (val1_msw !=0x2 )
                     return(-1);
             
        if (val1_lsw != 0x0) 
                     return(-1); 
           

	*(u_long *)REGISTER_ONE_MSW = 0x3F800000; /* 1 */
        *(u_long *)0xE0000708 = 0x3F800000; /* reg1 compare with operand */
        temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC);
	val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */
	val1_lsw = temp_res & 0xF; /* get the decoded value */ 
	if (val1_msw !=0x0 )
                     return(-1);       
            
	if (val1_lsw != 0x4) 
                     return(-1);
           
	*(u_long *)REGISTER_ONE_MSW = 0x40000000; /* 2 */  
        *(u_long *)0xE0000788 = 0x40400000; /* reg1 compaare magnitude with operrand */
        temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC);  
	val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */ 
        val1_lsw = temp_res & 0x1F; /* get the decoded value */ 
        if (val1_msw != 0x1) 
                     return(-1);
             
        if (val1_lsw != 0x19) 
                     return(-1); 
           
	 
	return(0);      
}

w_op_short_doublep()
{
	u_long  temp_res;
        u_long  val1_msw, val1_lsw, val2_msw, val2_lsw;
        u_long  val3_msw, val3_lsw, val4_msw, val4_lsw;
        u_long  val5_msw, val5_lsw, val6_msw, val6_lsw;
	u_long  val7_msw, val7_lsw, val8_msw, val8_lsw;
	u_long  val9_msw, val9_lsw, val10_msw, val10_lsw;
	u_long  val11_msw, val11_lsw, val12_msw, val12_lsw; 

	*(u_long *)REGISTER_SEVEN_MSW = 0x3FF00000; /* 1 */
	*(u_long *)REGISTER_SEVEN_LSW = 0x0;
        *(u_long *)REGISTER_EIGHT_MSW = 0x40180000; /* 6 */
	*(u_long *)REGISTER_EIGHT_LSW = 0x0;
        *(u_long *)REGISTER_NINE_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_NINE_LSW = 0x0;
        *(u_long *)REGISTER_TEN_MSW = 0x40220000; /* 9 */
	*(u_long *)REGISTER_TEN_LSW = 0x0;
        *(u_long *)REGISTER_ELEVEN_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_ELEVEN_LSW = 0x0;
        *(u_long *)REGISTER_TWELVE_MSW = 0x40080000; /* 3 */
	*(u_long *)REGISTER_TWELVE_LSW = 0x0;

	*(u_long *)0xE000008C = 0x3FF00000; /* 1 */ /* reg1 <- negate operand */
	*(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000114 = 0x40080000; /* 3 */ /* reg2 <- absolute value operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE000019C = 0x4; /* 4 */ /* reg3 <- float operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000224 = 0x40300000;/* 16 */ /* reg4 <- fix operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE00002AC = 0x40220000;  /* 9 *//* reg5 <- convert (dp to sp) operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000334 = 0x40100000; /* 4 */ /* reg6 <- square of operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE00003BC = 0x3FF00000; /* 1 */ /* reg7 <- reg7 + operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000444 = 0x40000000; /* 2 */ /* reg8 <- reg8 - operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE00004CC = 0x40000000; /* 2*/ /* reg9 <- reg9 * operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000554 = 0x40080000; /* 3 */ /* reg10 <- reg10 / operand */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE00005DC = 0x40220000; /* 9 */ /* reg11 <- operand - reg11 */
        *(u_long *)0xE0001000 = 0x0;
	*(u_long *)0xE0000664 = 0x40220000; /* 9 */ /* reg12 <- operand / reg12 */
        *(u_long *)0xE0001000 = 0x0;


	val1_msw = *(u_long *)REGISTER_ONE_MSW;
	val1_lsw = *(u_long *)REGISTER_ONE_LSW;
	val2_msw = *(u_long *)REGISTER_TWO_MSW;
	val2_lsw = *(u_long *)REGISTER_TWO_LSW;
	val3_msw = *(u_long *)REGISTER_THREE_MSW;
	val3_lsw = *(u_long *)REGISTER_THREE_LSW;
	val4_msw = *(u_long *)REGISTER_FOUR_MSW;
	val4_lsw = *(u_long *)REGISTER_FOUR_LSW;
	val5_msw = *(u_long *)REGISTER_FIVE_MSW;
	val5_lsw = *(u_long *)REGISTER_FIVE_LSW;
	val6_msw = *(u_long *)REGISTER_SIX_MSW;
	val6_lsw = *(u_long *)REGISTER_SIX_LSW;
	val7_msw = *(u_long *)REGISTER_SEVEN_MSW;
	val7_lsw = *(u_long *)REGISTER_SEVEN_LSW;
	val8_msw = *(u_long *)REGISTER_EIGHT_MSW;
	val8_lsw = *(u_long *)REGISTER_EIGHT_LSW;
	val9_msw = *(u_long *)REGISTER_NINE_MSW;
	val9_lsw = *(u_long *)REGISTER_NINE_LSW;
	val10_msw = *(u_long *)REGISTER_TEN_MSW;
	val10_lsw = *(u_long *)REGISTER_TEN_LSW;
	val11_msw = *(u_long *)REGISTER_ELEVEN_MSW;
	val11_lsw = *(u_long *)REGISTER_ELEVEN_LSW;
	val12_msw = *(u_long *)REGISTER_TWELVE_MSW;
	val12_lsw = *(u_long *)REGISTER_TWELVE_LSW;

        if (val1_msw != 0xBFF00000) 
                     return(-1);
        
        if (val1_lsw != 0x00000000) 
                     return(-1); 
         
        if (val2_msw != 0x40080000) 
                     return(-1);
        
        if (val2_lsw != 0x00000000) 
                     return(-1); 
         
        if (val3_msw != 0x40100000) 
                     return(-1);
        
        if (val3_lsw != 0x00000000) 
                     return(-1); 
        
        if (val4_msw != 0x10) 
                     return(-1);
        
        if (val4_lsw != 0x00000000) 
                     return(-1); 
        
        if (val5_msw != 0x41100000) 
                     return(-1);
        
        if (val5_lsw != 0x00000000) 
                     return(-1); 
        

	if (val6_msw != 0x40300000) 
                     return(-1);
        
        if (val6_lsw != 0x00000000) 
                     return(-1); 
        
        if (val7_msw != 0x40000000) 
                     return(-1);
        
        if (val7_lsw != 0x00000000) 
                     return(-1); 
         
        if (val8_msw != 0x40100000) 
                     return(-1);
        
        if (val8_lsw != 0x00000000) 
                     return(-1); 
        
        if (val9_msw != 0x40180000) 
                     return(-1);
        
        if (val9_lsw != 0x00000000) 
                     return(-1); 
         
        if (val10_msw != 0x40080000) 
                     return(-1);
        
        if (val10_lsw != 0x00000000) 
                     return(-1); 
         
        if (val11_msw != 0x40180000)
                     return(-1);
        
        if (val11_lsw != 0x00000000) 
                     return(-1); 
         
        if (val12_msw != 0x40080000)
                     return(-1);
        
        if (val12_lsw != 0x00000000) 
                     return(-1); 
         

	*(u_long *)0xE0000684 = 0x3FF00000; /* operand compare with 0 */
	*(u_long *)0xE0001000 = 0x0;

	temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC);
        val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */ 
        val1_lsw = temp_res & 0xF; /* get the decoded value */ 
        if (val1_msw !=0x2 ) 
                     return(-1);
             
        if (val1_lsw != 0x0)  
                     return(-1); 
           

	
	*(u_long *)REGISTER_ONE_MSW = 0x3FF00000; /* 1 */
	*(u_long *)REGISTER_ONE_LSW = 0x0;
	*(u_long *)0xE000070C = 0x3FF00000; /* 1*/ /* reg1 compare with operand */
	*(u_long *)0xE0001000 = 0x0; 
	temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC); 
        val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */
        val1_lsw = temp_res & 0xF; /* get the decoded value */ 
        if (val1_msw !=0x0 ) 
                     return(-1);       
            
        if (val1_lsw != 0x4) 
                     return(-1);
           

	*(u_long *)REGISTER_ONE_MSW = 0x40000000; /* 2 */
	*(u_long *)REGISTER_ONE_LSW = 0x0; 
	*(u_long *)0xE000078C = 0x40080000;  /* 3 *//* reg1 compaare magnitude with operrand */
	*(u_long *)0xE0001000 = 0x0; 	
	temp_res = *(u_long *)(FPA_BASE + FPA_WSTATUSC);  
	val1_msw = (temp_res & 0xF00) >> 8; /* get the values of status register */ 
        val1_lsw = temp_res & 0x1F; /* get the decoded value */ 
        if (val1_msw != 0x1)  
                     return(-1);
             
        if (val1_lsw != 0x19) 
                     return(-1); 
           
	
	return(0);	
}
			
