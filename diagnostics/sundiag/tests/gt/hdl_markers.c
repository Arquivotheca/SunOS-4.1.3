
#ifndef lint
static  char sccsid[] = "@(#)hdl_markers.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_markers()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_markers";
    TRACE_IN

    xtract_hdl(MARKERS_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(MARKERS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(MARKERS_CHK);
    errmsg = chksum_verify(MARKERS_CHK);

    TRACE_OUT
    return errmsg;

}

