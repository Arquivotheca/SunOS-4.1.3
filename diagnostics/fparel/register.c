
/*
 *  static char     fpasccsid[] = "@(#)register.c 1.1 2/14/86 Copyright Sun Microsystems";
 */ 
#include <sys/types.h>
#include "fpa.h"


/*
 * ================================================
 * immediate errors register test subroutine
 * ================================================
 */

ierr_test()
{
        register u_long         i,j, tmp_data, test_data;
	u_long *ptr;

	ptr = (u_long *)FPA_IERR_PTR;

        for (i = 0; i < 256; i++) {

  	         test_data = (i << 16) & 0xFF0000;
          	 *ptr = test_data;
                 tmp_data = *ptr & IERR_MASK;
          	 if (test_data != tmp_data) 
                       		return(-1);
                	
        	
	}
        return(0);
}

/*
 * ================================================
 * fpa inexact error mask register test subroutine
 * ================================================
 */
   
imask_test()
{
	u_long	*ptr, i, temp_val;

	ptr = (u_long *)FPA_IMASK_PTR;
	
	*ptr = 0;
	if ((*ptr & 0x1) != 0)  
                       		return(-1);
		

		*ptr = 0x1;
		if ((*ptr & 0x1) != 0x1) 
                       		return(-1);
		
	return(0);	
}
/*
 * ================================================
 * load pointer register test subroutine
 * ================================================
 */

ldptr_test()
{
        register u_long  *ram_ptr, i,j, tmp_data, test_data;

		ram_ptr = (u_long *)FPA_RAM_ACC;
	        test_data = ROTATE_DATA;
        	for (i=0 ; i < CNT_ROTATE ; i++) {

			/* you need to mask the unused bits */
                	test_data &= 0xFFFF; 
                	*ram_ptr = test_data ;
                	tmp_data = *ram_ptr & 0xFFFF; /*  & LPTR_MASK; */
                	if (test_data != tmp_data) 

                       			return(-1);
                	
                	test_data = test_data << 1;
        	}
	return(0);
}



