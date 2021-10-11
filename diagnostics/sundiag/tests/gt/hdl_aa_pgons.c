
#ifndef lint
static  char sccsid[] = "@(#)hdl_aa_pgons.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_aa_pgons()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_aa_pgons";
    TRACE_IN

    xtract_hdl(AA_POLYGONS_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(AA_POLYGONS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(AA_POLYGONS_CHK);
    errmsg = chksum_verify(AA_POLYGONS_CHK);

    TRACE_OUT
    return errmsg;

}

