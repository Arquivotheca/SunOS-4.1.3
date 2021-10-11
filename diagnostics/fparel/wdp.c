/*
 *	"@(#)wdp.c 1.1 7/30/92 Copyright Sun Microsystems"; 
 */ 
#include <sys/types.h>
#include "fpa.h"


/*
 * This test exercises the data bus of the Weitek chips.  To check the 
 * bus to the ALU different data values are added to zero.  For the 
 * multiplier the value is multiplied by one.  To give some test of the
 * data paths in the Weitek chips both single and double precision data
 * values are used.
 *
 * test the single precision case 
 *
 * load the microcode and mapping rams 
 *
 * the FPA users registers contain the following:
 *    reg0 = 0
 *    reg1 = 1
 *    reg2 = value under test
 *    reg3 = reg0 + reg2
 *    reg4 = reg1 * reg2
 *
 */

fpa_wd()
{
	u_long	*add, *mult;
	u_long  fshift, shift2, value_msw, value_lsw, value;
	u_long  mult_result_lsw, mult_result_msw, add_result_lsw, add_result_msw;
	u_long  add_result, mult_result;
	int	i, j;	



	/* clear the pipe */
	*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;


	/* setup reg0 and reg1 */

	*(u_long *)REGISTER_ZERO_MSW = 0;
	*(u_long *)REGISTER_ONE_MSW = 0x3f800000;
	add = (u_long *)0xe0000a80; /* command registter format */
	mult =(u_long *)0xe0000a08; /* command register format */

	/* transmit value and test */
	/* the value used is of the single floating point format */
	/* bits  0 - 22 = base value 	  */
	/* bits 23 - 30 = biased exponent */
	/* bit  31 	= sign bit 	  */

	for (i = 0; i < 2; i++) {

		for (j = 1; j < 255; j++) { 

			for (fshift = 0; fshift < 23; fshift++) {
	              
				value = (i << 31) | (j << 23) | (1 << fshift);
				*(u_long *)REGISTER_TWO_MSW = value;
				*add = 0x00020003;
				add_result = *(u_long *)REGISTER_THREE_MSW;  /*reg3*/
				if (add_result != value) 
                       				return(-1);
				
				*mult = 0x00020044; 
				mult_result = *(u_long *)REGISTER_FOUR_MSW;  /*reg4*/
				if (mult_result != value) 
                       				return(-1);
				
			}
		}
	}


	/* test the double precision case */


       /*
	* the FPA users registers contain the following:
	*    reg0 = 0
	*    reg1 = 1
	*    reg2 = value under test
	*    reg3 = reg0 + reg2
	*    reg4 = reg1 * reg2
	*/

	/* setup reg0 and reg1 */

	*(u_long *)REGISTER_ZERO_MSW = 0;              /* reg0 = 0 */
	*(u_long *)REGISTER_ZERO_LSW = 0;
   
	*(u_long *)REGISTER_ONE_MSW = 0x3ff00000;     /* reg1 = 1 */
	*(u_long *)REGISTER_ONE_LSW = 0x00000000;

	add = (u_long *)0xe0000a84;   
	mult = (u_long *)0xe0000a0c;

	/* transmit value and test */
        /* the value used is of the single floating point format */
        /* bits  0 - 51 = base value      */
        /* bits 52 - 62 = biased exponent */
        /* bit  63      = sign bit        */

	for (i = 0; i < 2; i++) {

		for (j = 1; j < 2047; j++){

			for (fshift = 0; fshift < 52; fshift++){
              
				value_lsw = (1 << fshift);
				if (fshift > 32)
					shift2 = fshift - 32;
				else 
					shift2 = 32;
				value_msw = (i << 31) | (j << 20) | (1 << shift2);
				*(u_long *)REGISTER_TWO_MSW = value_msw;
				*(u_long *)REGISTER_TWO_LSW = value_lsw;

				*add = 0x00020003;
				add_result_msw = *(u_long *)REGISTER_THREE_MSW;  /*reg3*/
				add_result_lsw = *(u_long *)REGISTER_THREE_LSW;  /*reg3*/
                 
				if (add_result_msw != value_msw)
                       			return(-1);
					
				if (add_result_lsw != value_lsw) 
                       			return(-1);
					

				*mult = 0x00020044;
				mult_result_msw = *(u_long *)REGISTER_FOUR_MSW;  /*reg4*/
				mult_result_lsw = *(u_long *)REGISTER_FOUR_LSW;  /*reg4*/

				if (mult_result_msw != value_msw) 
                       			return(-1);
				
		
				if (mult_result_lsw != value_lsw) 
                       			return(-1);
					
			}
		}
	}
	return(0);
}
