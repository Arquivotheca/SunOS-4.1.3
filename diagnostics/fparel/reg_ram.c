/*
 *      static char     fpasccsid[] = "@(#)reg_ram.c 1.1 7/30/92 Sun right Sun Microsystems";
 */

#include <sys/types.h>
#include "fpa.h"



		
/*
 * This routine checks the ram registers for present context
 */
reg_ram()
{
	u_long  *ptr, *ptr1, value1, value2, i, j, pattern, pattern1;

	ptr = (u_long *)0xE0000C00; /* starting address for register ram for present context */
	pattern = 0x1;
	pattern1 = 0x1;

	for (j = 0; j < 32; j++)
	{
		ptr = (u_long *)0xE0000C00; 
		for (i = 0; i < 32; i++)
		{
			*ptr = pattern;
			ptr += 1;
			*ptr = pattern1;
			ptr += 1;
		}

		ptr = (u_long *)0xE0000C00;
		for (i = 0; i < 32; i ++)
		{
			value1 = *ptr; /* for most significant word */
			ptr += 1;  
			value2 = *ptr; /* for least significant word */
			ptr += 1;

			if (value1 != pattern)
				return(-1);
			if (value2 != pattern1)
				return(-1);
		}
		pattern = (pattern << 1);
		pattern1 = (pattern1 << 1);
	}
	return(0);
}

/* 
 * The following routine tests the shadow ram for the
 * present context
 */


shadow_ram()
{
	u_long   *ptr, *ptr1, i, j;

	ptr1 = (u_long *)SHADOW_RAM_START; /* pointer to the start of the shadow ram */


	ptr = (u_long *)0xE0000C00; /* starting address for register ram for */
                                    /* present context */
	for (i = 0; i < 16; i += 2)
        {
                *ptr = i + 1; /* for most significant word */
		ptr += 1;
 
                *ptr = i;  /* for least significant word */
		ptr += 1;
        }
		
	for (i = 0; i < 16; i +=2)
	{

		if (*ptr1 != (i+1)) /* higher word of shadow ram */
			return(-1);
		
		ptr1 += 1;
		if (*ptr1 != i) /* lower word of shadow ram */
			return(-1);
		
		ptr1 += 1;
	}
	return(0);
}
 

