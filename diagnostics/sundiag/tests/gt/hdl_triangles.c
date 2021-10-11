
#ifndef lint
static  char sccsid[] = "@(#)hdl_triangles.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_triangles()
/**********************************************************************/

{
    char *getfname();

    char *errmsg;


    func_name = "hdl_triangles";
    TRACE_IN

    (void)clear_all();

    xtract_hdl(AA_TRIANGLES_CHK);

    errmsg = exec_dl(getfname(AA_TRIANGLES_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(FLAT_TRIANGLES_CHK);

    errmsg = exec_dl(getfname(FLAT_TRIANGLES_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(GOURAUD_TRIANGLES_CHK);

    errmsg = exec_dl(getfname(GOURAUD_TRIANGLES_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(TRIANGLES_CHK);

    errmsg = exec_dl(getfname(TRIANGLES_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(TRIANGLES_CHK);
    errmsg = chksum_verify(TRIANGLES_CHK);

    TRACE_OUT
    return errmsg;
}
