
#ifndef lint
static  char sccsid[] = "@(#)depth_cueing.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>


static char *errmsg;

/**********************************************************************/
char *
depth_cueing()
/**********************************************************************/
/* tests depth cueing capability */

{
    char *gpsi_depth_cueing();

    func_name = "depth_cueing";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_depth_cueing();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
