
#ifndef lint
static  char sccsid[] = "@(#)clip_hid.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
clip_hid()
/**********************************************************************/
/* tests clipping and hidden surface removal capability */

{
    char *gpsi_clip_hid();

    func_name = "clip_hid";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_clip_hid();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
