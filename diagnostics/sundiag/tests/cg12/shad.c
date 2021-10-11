
#ifndef lint
static  char sccsid[] = "@(#)shad.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

extern char *gpsi_shad();

static char *errmsg;

/**********************************************************************/
char *
shad()
/**********************************************************************/
/* tests lighting and shading capability */

{
    func_name = "shad";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_shad();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
