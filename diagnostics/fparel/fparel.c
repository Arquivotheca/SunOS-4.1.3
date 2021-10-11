static char sccsid[] = "@(#)fparel.c 1.1 7/30/92 Copyright Sun Microsystems"; 

/*
 * ****************************************************************************
 * Source File     : fparel.c
 * Original Engr   : Nancy Chow
 * Date            : 10/12/88
 * Function        : This file contains the data structure definitions used
 *		   : globally by the fparel modules.
 * Revision #1 Engr: 
 * Date            : 
 * Change(s)       :
 * ****************************************************************************
 */

/* ***************
 * Include Files
 * ***************/

#include "fpa3x.h"

int     sig_err_flag; 		/* expect or not expect buserr */
int     seg_sig_flag;		/* expect or not expect seg violation */ 
int	dev_no;			/* file desc of current context */
int     verbose = FALSE;	/* verbose message display? */
int	no_times = 1;		/* test pass count */

