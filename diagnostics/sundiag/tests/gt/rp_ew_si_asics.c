
#ifndef lint
static  char sccsid[] = "@(#)rp_ew_si_asics.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <gttest.h>
#include <errmsg.h>

static char errtxt[512];

/**********************************************************************/
char *
rp_ew_si_asics()
/**********************************************************************/

{

    char *errmsg;

    func_name = "rp_ew_si_asics";
    TRACE_IN

    xtract_hdl(RENDERING_PIPELINE);

    clear_all();

    errmsg = exec_dl(getfname(RENDERING_PIPELINE, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_RENDERING_PIPELINE, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    TRACE_OUT
    return errmsg;
}

