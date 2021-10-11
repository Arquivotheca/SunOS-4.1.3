
#ifndef lint
static  char sccsid[] = "@(#)hdl_depth_cueing.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_depth_cueing()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_depth_cueing";
    TRACE_IN

    xtract_hdl(DEPTH_CUEING_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(DEPTH_CUEING_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(DEPTH_CUEING_CHK);
    errmsg = chksum_verify(DEPTH_CUEING_CHK);

    TRACE_OUT
    return errmsg;

}

