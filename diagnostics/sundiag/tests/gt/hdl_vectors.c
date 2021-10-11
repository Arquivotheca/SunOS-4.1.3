
#ifndef lint
static  char sccsid[] = "@(#)hdl_vectors.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_vectors()
/**********************************************************************/
/* vectors generation test */

{
    char *getfname();
    char *errmsg;



    func_name = "hdl_vectors";
    TRACE_IN

    (void)clear_all();

    xtract_hdl(VECTORS_CHK);
    errmsg = exec_dl(getfname(VECTORS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(AA_VECTORS_CHK);
    errmsg = exec_dl(getfname(AA_VECTORS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(WIDE_VECTORS_CHK);
    errmsg = exec_dl(getfname(WIDE_VECTORS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(TEXTURED_VECTORS_CHK);
    errmsg = exec_dl(getfname(TEXTURED_VECTORS_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(VECTORS_CHK);
    errmsg = chksum_verify(VECTORS_CHK);

    TRACE_OUT
    return errmsg;
}

