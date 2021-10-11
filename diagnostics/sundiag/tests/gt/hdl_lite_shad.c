
#ifndef lint
static  char sccsid[] = "@(#)hdl_lite_shad.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
hdl_lite_shad()
/**********************************************************************/

{
    char *getfname();
    char *errmsg;


    func_name = "hdl_lite_shad";
    TRACE_IN

    (void)clear_all();

    xtract_hdl(LIGHTED_SUN_LOGO);
    errmsg = exec_dl(getfname(LIGHTED_SUN_LOGO, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(LIGHTING_SHADING_CHK);
    errmsg = exec_dl(getfname(LIGHTING_SHADING_CHK, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(MOLEC1);
    errmsg = exec_dl(getfname(MOLEC1, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_hdl(BLENDER_C);
    errmsg = exec_dl(getfname(BLENDER_C, HDL_CHK));
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    xtract_chk(LIGHTING_SHADING_CHK);
    errmsg = chksum_verify(LIGHTING_SHADING_CHK);

    TRACE_OUT
    return errmsg;

}

