
#ifndef lint
static  char sccsid[] = "@(#)video_mem.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <suntool/sunview.h>
#include <esd.h>

extern
char		*pr_test(),
		*pr_visual_test(),
		*pixrect_complete_test();

extern
char		*sprintf();

extern
char		*device_name;

static
char errtxt[256];

/**********************************************************************/
char *
video_mem()
/**********************************************************************/

{
    Pixrect *pr;
    char *errmsg;

    func_name = "video_mem";
    TRACE_IN

    pr = pr_open(device_name);
    if (pr == NULL) {
	(void)sprintf(errtxt, errmsg_list[19], device_name);
	TRACE_OUT
	return(errtxt);
    }

    errmsg = pixrect_complete_test(pr);
    if (errmsg) {
	(void)pr_close(pr);
	TRACE_OUT
	return(errmsg);
    }

    (void)pr_close(pr);
    TRACE_OUT
    return (char *)0;
}

