/*
 *	"@(#)ptr_incdec.c 1.1 7/30/92 Copyright Sun Microsystems"; 
 *
 */

#include <sys/types.h>
#include "fpa.h"
#include "fpa3x.h"

u_long ptr_cmd[] =
{
	0x10005,
	0x20046,
	0x30087,
	0x400C8,
	0x50109,
	0x6014A,
	0x7018B,
	0x801CC,
	0x9020D,
	0xA024E,
	0xB028F,
	0xC02D0,
	0xD0311,
	0xE0352,
	0xF0393,
	0x1003D4,
	0x110415,
	0x120456,
	0x130497,
	0x1404D8,
	0x150519,
	0x16055A,
	0x17059B,
	0x1805DC,
	0x19061D,
	0x1A065E,
	0x1B069F
};
u_long val[] = {

	0x3FF00000, /* for dp 1 */
	0x40000000, /* for dp 2 */
	0x40080000, /* for dp 3 */
	0x40100000, /* for dp 4 */
	0x40140000  /* for dp 5 */
};

ptr_incdec_test()
{
	u_long	i, j, k, m, n, res1;
	u_long  *ptr, *ptr2, *ptr3;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;


	ptr = (u_long *)0xE00009B0; 	/* for transposing */

	for (j = 0; j < 32; j++)        /* clear registers */
                {
                         ptr2 = (u_long *)INS_REGFC_M(j);
                         *ptr2 = 0x0;
                         ptr2++;
                        *ptr2 = 0x0;
                }

	for (j = 0; j <= 15; j++)	/* fill 4x4 matrix */
	{
		*(u_long *)INS_REGFC_M(j) = j;
			
		k = (j+16) & 0x1F; 	/* for 0 - 31 */	
		*(u_long *)INS_REGFC_M(k) = 0x100;
	}			
	*ptr = 0;			/* reg 0 is element 1x1 */

	/* now read the transposed values */
	for (m = 0, j = 0; m < 4; m++)
	{
		for (n = 0; n < 4; n++)
		{
			ptr2 = (u_long *)INS_REGFC_M(j);
	                k = (j+16) & 0x1F; 	/* for 0 - 31 */
                        ptr3 = (u_long *)INS_REGFC_M(k);
			res1 =  m + (n*4);

			if (*ptr2 != res1) 
				return(-1);
				
			if (*ptr3 != 0x100) 
				return(-1);

					/* check LSH of registers */
			if (*(ptr2 + 1) != 0 || *(ptr3 + 1) != 0)
				return(-1);
				
			j++;
		}
	}

	/* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2;

	ptr = (u_long *)0xE00008CC; 		/* for dot product */

	for (i = 0; i <= 26; i++)
	{
		ptr3 = (u_long *)INS_REGFC_M(i+5);

		for (j = 0; j < 32; j++)	/* clear registers */
	        {
       		         ptr2 = (u_long *)INS_REGFC_M(j);
               		 *ptr2 = 0x0;
			 ptr2++;
			*ptr2 = 0x0;
        	}

		for (j = 0; j <= 4; j++)
			*(u_long *)INS_REGFC_M(i+j) = val[j];
		
		*ptr = ptr_cmd[i]; 	/* send the instruction */
		if (*ptr3 != 0x40440000)	/* check MSH of result */
			return(-1);  
		if (*(ptr3 + 1) != 0)		/* check LSH of result */
			return(-1);
		
	}
	return(0);
}
