
#ifndef lint
static  char sccsid[] = "@(#)xf.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>


extern char *gpsi_xf();

static char *errmsg;

/**********************************************************************/
char *
xf()
/**********************************************************************/
/* tests transformation capability */

{
    func_name = "xf";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_xf();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
