
#ifndef lint
static  char sccsid[] = "@(#)gpsi_depth_cueing.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <pixrect/gp1cmds.h>
#include <esd.h>

/**********************************************************************/
char *
gpsi_depth_cueing()
/**********************************************************************/
 
{
    char * enterprise();
    char *dc_vector_lissa();
    char *errmsg;
    double ri;

    func_name = "gpsi_depth_cueing";
    TRACE_IN

    /* extract test images from tar file */
    (void)xtract(GPSI_DEPTH_CUEING_CHK);

    /* clear all planes */
    clear_all();

    errmsg = ctx_set(CTX_SET_DEPTH_CUE, GP1_DEPTH_CUE_ON,
		    CTX_SET_HIDDEN_SURF, GP1_ZBALL, 
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    ri = 14.5;
    errmsg = dc_vector_lissa(ri);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    errmsg = chksum_verify(GPSI_DEPTH_CUEING_CHK);
    TRACE_OUT
    return errmsg;
}

