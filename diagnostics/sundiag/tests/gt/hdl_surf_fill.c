
#ifndef lint
static  char sccsid[] = "@(#)hdl_surf_fill.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_surf_fill()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_surf_fill";
    TRACE_IN

    xtract_hdl(SURFACE_FILL_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(SURFACE_FILL_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(SURFACE_FILL_CHK);
    errmsg = chksum_verify(SURFACE_FILL_CHK);

    TRACE_OUT
    return errmsg;

}

