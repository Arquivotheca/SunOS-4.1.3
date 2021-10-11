
#ifndef lint
static  char sccsid[] = "@(#)dsp.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

/**********************************************************************/
char *
dsp()
/**********************************************************************/

{
    int result;
    short cmd;
    short mode;


    func_name = "dsp";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    cmd = GP1_DIAG;
    mode = DSP_TEST;
    result = post_diag_test(cmd, mode, 0, 0, 0, 0, 0, 0, 0, 0);

    if (result) {
	int errindex;
	if (result >= 1 && result <= 4) {
	    errindex = 40 + result;
	} else {
	    errindex = 16;
	}
	TRACE_OUT
	return(errmsg_list[errindex]);
    }

    TRACE_OUT
    return(char *)0;
}
