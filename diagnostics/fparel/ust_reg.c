
/*
 *  static char     fpasccsid[] = "@(#)ust_reg.c 1.1 2/14/86 Copyright Sun Microsystems";
 */
#include <sys/types.h>
#include "fpa.h"


u_long	wstatus_res[] = {

	0xa004,
	0xa119,
	0xa200,
	0xa300,
	0x2400,
	0x2500,
	0x2600,
	0x2700,
	0x2800,
	0x2900,
	0x2a00,
	0x2b00,
	0x2c00,
	0x2d00,
	0x2e00,
	0x2f02
};

u_long	wstatus_res1[] = {

	0xa004,
	0xa119,
	0xa200,
	0x2300,
	0x2400,
	0x2500,
	0x2600,
	0x2700,
	0x2800,
	0x2900,
	0x2a00,
	0x2b00,
	0x2c00,
	0x2d00,
	0x2e00,
	0x2f02
};
/*
 *	The following test is for testing the micro store type registers
 *		those are pointer - 5, loop counter, mode register, wstat register
 *
 */

 
  

test_mode_reg()
{

	int	i;
	u_long	*ptr1, *ptr2;


	/* set the IMASK register = 0 */
	*(u_long *)FPA_IMASK_PTR = 0x0;

	/* clear pipe */
	ptr1 = (u_long *)FPA_STATE_PTR;
/*	*ptr1 = 0x40; */
	ptr2 = (u_long *)FPA_CLEAR_PIPE_PTR;
        *ptr2 = 0x0;

	ptr1 = (u_long *)(FPA_BASE + FPA_MODE3_0C);
	ptr2 = (u_long *)(FPA_BASE + FPA_MODE3_0S);

	for (i = 0; i < 0x10; i++) {

		/* write into the mode reg. E00008D0 */

		*(u_long *)MODE_WRITE_REGISTER = i;
		/* now read at two places, mode-reg stable and mode reg clear 
	           mode_reg stable = 0xE0000F38, mode_reg clear = 0xE0000FB8
                 */

		if ((*ptr1 & 0x0F) != i) 
                       		return(-1);
		
		if ((*ptr2 & 0x0F) != i) 
                       		return(-1);
		
	}
	/* clear hard pipe */

	*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
	return(0);
}

test_wstatus_reg() 
{

	int	i, result;
	u_long	*ptr1, *ptr2, res1, res2;
	u_long  *st_reg_ptr;

	
		/* we test wstatus read stable and clear by writing 4bit pattern to 
		 * 0xE0000958 location.  because there are only 4 bits that matters */
		/* and check this with IMASK bit = 0 (errors are disabled)
		  		       IMASK bit = 1 (errors are enabled)  */


	st_reg_ptr = (u_long *)FPA_STATE_PTR;
/*	*st_reg_ptr = 0x40; */

	ptr1 = (u_long *)FPA_IMASK_PTR;
	*ptr1 = 0x0;                   /* IMASK bit is 0 */

	ptr1 = (u_long *)(FPA_BASE + FPA_WSTATUSC);	/* wstatus clear - 0FBC */
	ptr2 = (u_long *)(FPA_BASE + FPA_WSTATUSS);	/* wstatus stable - 0F3c */
	for (i = 0; i < 0x10; i++) {

		*(u_long *)WSTATUS_WRITE_REGISTER = (i << 8);

		res1 = *ptr1 & 0xEF1F;
		res2 = *ptr2 & 0xEF1F;

		if (res1 != wstatus_res[i]) 
                       		return(-1);
		
		if (res2 != wstatus_res[i])    
                       		return(-1);
	}

	ptr1 = (u_long *)FPA_IMASK_PTR;
        *ptr1 =  0x1;			/* IMASK bit is 1 */
 
        ptr1 = (u_long *)(FPA_BASE + FPA_WSTATUSC);   /* wstatus clear */

	for (i = 0; i < 0x10; i++) {
 
                *(u_long *)WSTATUS_WRITE_REGISTER = (i << 8);
 
                res1 = *ptr1 & 0xEF1F;    
                res2 = *ptr2 & 0xEF1F;  
 
                if (res1 != wstatus_res1[i])   
                       		return(-1);
                if (res2 != wstatus_res1[i])  
                       		return(-1);
        }
	/*clear hard pipe */
	*(u_long *)FPA_CLEAR_PIPE_PTR = 0x0;
	/*  do ttest using unimplemented instruction */
	/* do the valid bit test by clearing the pipe, write the status and check */
/*	st_reg_ptr = 0x0;*/	
	return(0);
}						
