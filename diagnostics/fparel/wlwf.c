/*
 *      static char     fpasccsid[] = "@(#)wlwf.c 1.1 2/14/86 Copyright Sun Microsystems"; *
 *      Copyright (c) 1985 by Sun Microsystems, Inc 
 *
 */
#include <sys/types.h>
#include "fpa.h"


struct fvalue {
	u_long addr;
	u_long reg2;
	u_long reg3;
	u_long result;
};


struct fvalue fval[] = {

	0xE0000A00, 0x40800000, 0x40400000, 0x3F800000,
		 /* single subtract, f32 - f32, 4 - 3 = 1 */
	0xE0000A04, 0x40100000, 0x40080000, 0x3FF00000,
		/* double subtract, f32/64 - f32/64, 4 - 3 = 1 */
	0xE0000A10, 0xC0000000, 0x40000000, 0x40800000,
		 /* single magnitude of difference, |f32 - f32|, |-2 - 2| = 4 */
	0xE0000A20, 0xC0800000, 0xC0000000, 0x40000000,
		/* single subtract of magnitudes, |f32| - |f32|, |-4| - |-2| = 2 */
	0xE0000A40, 0x40000000, 0x00000000, 0xC0000000,
		/* single negate, -f32 + 0 , 2 = -2*/
	0xE0000A80, 0x40000000, 0x40400000, 0x40A00000,
		/* single add, f32 + f32, 2 + 3 = 5 */
	0x0,	    0x0,	0x0,	    0x0	    /* end of table */
};

wlwf_test()
{
	u_long	*ptr1, *ptr2, *ptr3, *op_ptr, *ptr1_lsw, *ptr2_lsw, *ptr3_lsw;
	u_long  i, res1;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	ptr1 = (u_long *)REGISTER_ONE_MSW;
	ptr1_lsw = (u_long *)REGISTER_ONE_LSW;
	ptr2 = (u_long *)REGISTER_TWO_MSW;
	ptr2_lsw = (u_long *)REGISTER_TWO_LSW;
	ptr3 = (u_long *)REGISTER_THREE_MSW;
	ptr3_lsw = (u_long *)REGISTER_THREE_LSW;

	for ( i = 0; fval[i].addr != 0x0; i++) {

		*ptr1 = 0x0;
		*ptr1_lsw = 0x0;
		*ptr2 = 0x0;
		*ptr2_lsw = 0x0;
		*ptr3 = 0x0;
		*ptr3_lsw = 0x0;

		op_ptr = (u_long *)fval[i].addr;
		*ptr2 = fval[i].reg2;
		*ptr3 = fval[i].reg3;
	
		*op_ptr = 0x30081; /* reg1 <- reg2 op reg3 */
	
		res1 = *ptr1;  /* read the result */
		if (res1 != fval[i].result) 
			return(-1);
		
	}
	return(0);
}
		
			
