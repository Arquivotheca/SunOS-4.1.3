
#ifndef lint
static  char sccsid[] = "@(#)animation.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>


extern char *gpsi_animation();

static char *errmsg;

/**********************************************************************/
char *
animation()
/**********************************************************************/
/* generates animation for visual check */

{
    func_name = "animation";
    TRACE_IN

    if(!open_gp()) {
	TRACE_OUT
	return(errmsg_list[1]);
    }

    errmsg = gpsi_animation();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return(char *)0;
}
