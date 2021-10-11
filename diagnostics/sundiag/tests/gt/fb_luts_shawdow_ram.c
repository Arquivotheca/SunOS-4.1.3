
#ifndef lint
static  char sccsid[] = "@(#)fb_luts_shawdow_ram.c 1.1 92/07/30 Copyr 1990 Sun Micro";
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
char		*lut_test();

extern
char		*sprintf();

extern
char		*device_name;

extern
Pixrect		*open_pr_device();

/**********************************************************************/
char *
fb_luts_shawdow_ram()
/**********************************************************************/

{
    static char errtxt[256];
    char *errmsg;
    Pixrect *pr;
    int group;


    func_name = "fb_luts_shawdow_ram";
    TRACE_IN

    pr = open_pr_device();
    if (pr == NULL) {
	(void)sprintf(errtxt, DLXERR_OPEN, device_name);

	TRACE_OUT
	return(errtxt);
    }

    /* save current plane group */
    group = pr_get_plane_group(pr);
    /* set 24-bit plane */
    pr_set_plane_group(pr, PIXPG_24BIT_COLOR);

    errmsg = lut_test(pr);
    if (errmsg) {
	pr_set_plane_group(pr, group);
	close_pr_device();

	TRACE_OUT
	return errmsg;
    }

    pr_set_plane_group(pr, group);
    close_pr_device();

    TRACE_OUT
    return (char *)0;
    
}
