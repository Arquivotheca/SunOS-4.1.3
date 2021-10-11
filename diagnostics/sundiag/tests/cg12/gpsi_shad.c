
#ifndef lint
static  char sccsid[] = "@(#)gpsi_shad.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

/**********************************************************************/
char *
gpsi_shad()
/**********************************************************************/
 
{

    char *shaded_cylinder();
    char *errmsg;

    func_name = "gpsi_shad";
    TRACE_IN

    /* extract test images from tar file */
    (void)xtract(GPSI_SHAD_CHK);

    /* clear screen */
    clear_all();

    /* clear context */
    errmsg = ctx_set(0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    (void)shaded_cylinder();

    errmsg = chksum_verify(GPSI_SHAD_CHK);

    TRACE_OUT
    return errmsg;
}

