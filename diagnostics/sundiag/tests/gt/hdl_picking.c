
#ifndef lint
static  char sccsid[] = "@(#)hdl_picking.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_picking()
/**********************************************************************/

{
    static char errtxt[256];

    char *getfname();
    char *chk_prb();
    char *errmsg;


    func_name = "hdl_picking";
    TRACE_IN

    (void)clear_all();

    xtract_hdl(PICK_DETECT_CHK);

    errmsg = exec_dl(getfname(PICK_DETECT_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = chk_prb();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    (void)clear_all();

    xtract_hdl(PICK_ECHO_CHK);

    errmsg = exec_dl(getfname(PICK_ECHO_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(PICK_ECHO_CHK);
    errmsg = chksum_verify(PICK_ECHO_CHK);

    if (errmsg) {
	(void)sprintf(errtxt, DLXERR_PICK_ECHO, errmsg);
	TRACE_OUT
	return errtxt;
    }

    TRACE_OUT
    return (char *)0;

}

