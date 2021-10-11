
#ifndef lint
static  char sccsid[] = "@(#)fe_i860.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

/**********************************************************************/
char *
fe_i860()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "fe_i860";
    TRACE_IN

    xtract_hdl(I860_PROCESSOR);

    /* Initialize the system */
    if (!open_hawk()) {

	TRACE_OUT
	return DLXERR_SYSTEM_INITIALIZATION;
    }

    errmsg = exec_dl(getfname(I860_PROCESSOR, HDL_CHK));

    /* close system */
    (void)close_hawk();

    TRACE_OUT
    return errmsg;
}

