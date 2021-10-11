/*
 *  "@(#)nack2.c 1.1 7/30/92 Copyright Sun Microsystems";
 */
#include <sys/types.h>
#include "fpa.h"
#include <stdio.h>

nack2_test()
{
/*
 * send an unimplemented instruction
 * reg0 -> 0; check for bus error, check ierr = 0x20
 * read any user register, check for bus error, check ierr = 0x20;
 * write another instruction to fpa, read wstatus reg.(clear pipe)
 * write the imask register and check the bus error
 */
	u_long	*ptr1, *ptr2, *ptr3, *ptr4, *ptr5, *ptr6, *ptr7;
	int	value;


        /* Initialize  by giving the diagnostic initialize command */
        *(u_long *)DIAG_INIT_CMD = 0x0;
        *(u_long *)MODE_WRITE_REGISTER = 0x2; 

	*(u_long *)REGISTER_ZERO_MSW = 0x0;
	*(u_long *)REGISTER_ZERO_LSW = 0x0;

	ptr1 = (u_long *)0xE000096C; /* this is the unimplemented instruction */
	ptr2 = (u_long *)FPA_IERR_PTR; /* point to ierr register */
	ptr4 = (u_long *)0xE0000824; /* address for exponential operation */
	ptr5 = (u_long *)0xE0000824; /* address for exponential operation */ 
	ptr6 = (u_long *)(FPA_BASE + FPA_WSTATUSC);
	ptr7 = (u_long *)(FPA_BASE + FPA_IMASK);



	/* first the shadow register read */

	*ptr1 = 0x0;  /* register zero */	

	value = *(u_long *)0xE0000E00; /* read the zero shadow ram */
	
	/* the checking of ierr register and clearing the ierr register is done in the
	 * signal handler 
         */
	/* Pipe line read */

	/* try to read any user register */
	*ptr1 = 0x0;  /* register zero */

        value = *(u_long *)REGISTER_ONE_MSW;
	/* the checking of ierr register and clearing the ierr register is done in the 
         * signal handler  
         */ 

	/* pipe line write */

	/* send two short instructions */
	*ptr1 = 0x0;  /* register zero */	
	*ptr4 = 0x108;
	
        *ptr5 = 0xC7;    /* send third instruction, reg7 = reg3 op */
	/* the checking of ierr register and clearing the ierr register is done in the 
         * signal handler  
         */  

	/* read the control register */
	*ptr1 = 0x0;  /* register zero */
        value = *ptr6;   /* read the wstatus register */
   
 	/* the checking of ierr register and clearing the ierr register is done in the 
         * signal handler  
         */  
	/* write the control register */
        *ptr7 = 0x0;  /* write to imask register */
	/* the checking of ierr register and clearing the ierr register is done in the 
         * signal handler  
         */ 
	return(0);
}
