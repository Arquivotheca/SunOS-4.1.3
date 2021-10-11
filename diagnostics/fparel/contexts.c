static char sccsid[] = "@(#)contexts.c 1.1 7/30/92 Copyright Sun Microsystems";

/*
 * ****************************************************************************
 * Option(s)       : -t Generate a functional trace.
 * Source File     : contexts.c 
 * Original Engr   : Nancy Chow
 * Date            : 10/14/88
 * Function        : This file contains the modules whose funtions are
 *		   : related to context manipulation.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * ***************/

#include <sys/types.h>
#include <sys/file.h>
#include <errno.h>
#include "fpa3x.h"
#include <sys/ioctl.h>
#include <sundev/fpareg.h>

/* ******************
 * open_new_context
 * ******************/

/* This routine opens the FPA context. If unable to open, a corresponding 
   error message is printed */

open_new_context()
{

    dev_no = open("/dev/fpa", O_RDWR);		/* open special device file */
						/* for new context */
    if (dev_no < 0) {				/* if open error */
	if (verbose)				/* verbose message display? */
	    switch (errno) {
	    	case ENXIO   : printf("Cannot find FPA.\n"); break;
		case ENOENT  : printf("Cannot find 68881.\n"); break;
		case EBUSY   : printf("No FPA context Available.\n"); break;
		case ENETDOWN: printf("Disabled FPA, could not access.\n"); 
			       break;
		case EEXIST  : printf("Duplicate Open on FPA.\n"); break; 
	    }
    }
    else
	return(0);				/* open OK */
}

/* *******************
 * close_new_context
 * *******************/

/* This routine is used to close the FPA special device file for the current
   context */

close_new_context()
{
    if (close(dev_no) < 0) {			/* close fail? */
	printf("Error while closing a context number.\n");
	printf("        descriptor number = %x, error number = %d.\n", dev_no, errno);
    }
}

/* ****************
 * other_contexts
 * ****************/

/* This routine is used to test the register file access for each of the 
   currently unused contexts. 
   Returns 0 			 - success
           0xFF 		 - test failed
	   other positive number - could not open fpa */

other_contexts(contexts)
u_long *contexts;				/* accessed contexts */
{
    
int    i;
u_long val;
u_long cxt_bit;

    for (i=1; i < MAX_CXTS; i++) { 		/* for each context */
	if (val = open_new_context())		/* context open error? */
	    return(val);
	val = *(u_long *)REG_STATE & CXT_MASK;	/* get current context */
	cxt_bit = (1 << val);			/* corresponding context */
	if (verbose)
	    printf("        Register Ram Lower Half Test, Context Number = %d\n",val);
	
	*contexts = (*contexts | cxt_bit);	/* set accessed context */
	
	if (reg_ram())				/* ram LH test fail? */
 	    return(RAM_LH_FAIL);
	close_new_context();			/* close current context */
    }
    return(0);
}

