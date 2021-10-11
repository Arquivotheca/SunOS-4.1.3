
#ifndef lint
static  char sccsid[] = "@(#)lines.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

extern char *gpsi_3D_lines();

static char *errmsg;

/**********************************************************************/
char *
lines()
/**********************************************************************/
/* tests vector generation capability */

{
    func_name = "lines";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_3D_lines();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
