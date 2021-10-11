
#ifndef lint
static  char sccsid[] = "@(#)rp_shared_ram.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
rp_shared_ram()
/**********************************************************************/

{
    char *errmsg;

    func_name = "rp_shared_ram";
    TRACE_IN

    xtract_hdl(RP_SHARED_RAM);

    errmsg = exec_dl(getfname(RP_SHARED_RAM, HDL_CHK));

    if (errmsg) {
	sprintf(errtxt, DLXERR_SU_SHARED_RAM, errmsg);
	TRACE_OUT;
	return errtxt;
    }

    TRACE_OUT
    return (char *)0;
}

