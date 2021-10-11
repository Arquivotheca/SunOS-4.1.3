
#ifndef lint
static  char sccsid[] = "@(#)hdl_splines.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <errmsg.h>
#include <gttest.h>

/**********************************************************************/
char *
hdl_splines()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_splines";
    TRACE_IN

    xtract_hdl(SPLINE_CURVES_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(SPLINE_CURVES_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(SPLINE_CURVES_CHK);
    errmsg = chksum_verify(SPLINE_CURVES_CHK);

    TRACE_OUT
    return errmsg;

}

