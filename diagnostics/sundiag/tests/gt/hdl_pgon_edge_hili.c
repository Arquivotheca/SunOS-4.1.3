
#ifndef lint
static  char sccsid[] = "@(#)hdl_pgon_edge_hili.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_pgon_edge_hili()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_pgon_edge_hili";
    TRACE_IN

    xtract_hdl(POLYGON_EDGE_HILITE_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(POLYGON_EDGE_HILITE_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(POLYGON_EDGE_HILITE_CHK);
    errmsg = chksum_verify(POLYGON_EDGE_HILITE_CHK);

    TRACE_OUT
    return errmsg;

}

