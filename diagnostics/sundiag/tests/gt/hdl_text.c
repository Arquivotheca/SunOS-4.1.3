
#ifndef lint
static  char sccsid[] = "@(#)hdl_text.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_text()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_text";
    TRACE_IN

    xtract_hdl(TEXT_CHK);

    (void)clear_all();

    errmsg = exec_dl(getfname(TEXT_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(TEXT_CHK);
    errmsg = chksum_verify(TEXT_CHK);

    TRACE_OUT
    return errmsg;

}

