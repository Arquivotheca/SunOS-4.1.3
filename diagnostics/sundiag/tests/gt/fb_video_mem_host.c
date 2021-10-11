
#ifndef lint
static  char sccsid[] = "@(#)fb_video_mem_host.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <suntool/sunview.h>
#include <gttest.h>
#include <errmsg.h>

extern
char		*pr_test(),
		*pr_visual_test(),
		*pixrect_complete_test();

extern
char		*sprintf();

extern
char		*device_name;

extern
Pixrect		*open_pr_device();

static
char errtxt[256];

/**********************************************************************/
char *
fb_video_mem_host()
/**********************************************************************/

{
    Pixrect *pr;
    char *errmsg;
    int wind;


    func_name = "fb_video_mem_host";
    TRACE_IN

    pr = open_pr_device();
    if (pr == NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN, device_name);

	TRACE_OUT
	return(errtxt);
    }

    (void)get_available_plane_groups(pr);
    (void)include_plane_group(PIXPG_ZBUF);
    /*
    (void)exclude_plane_group(PIXPG_CURSOR_ENABLE);
    (void)exclude_plane_group(PIXPG_CURSOR);
    */

    errmsg = pixrect_complete_test(pr);
    if (errmsg) {

	TRACE_OUT
	return(errmsg);
    }


    TRACE_OUT
    return (char *)0;
}

