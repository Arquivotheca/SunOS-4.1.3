
#ifndef lint
static  char sccsid[] = "@(#)egret_test.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <suntool/sunview.h>
#include <esd.h>

extern
char		*pixrect_complete_test(),
		*lut_test();

extern
char		*sprintf();

extern
char		*device_name;

/**********************************************************************/
void
egret_test()
/**********************************************************************/

{
    void gpsi();
    static char errtxt[256];

    Pixrect *pr;
    char *errmsg;
    int group;

#ifdef NOT_TESTING /* Skip the pixrect test for now to accelerate */
    pr = pr_open(device_name);
    if (pr == NULL) {
	(void)sprintf(errtxt, "Can't open Pixrect Device %s.\n",
					    device_name);
	tmessage("Frame Buffer Test: ");
	error_report(errtxt);
	return;
    }

    /* Pixrect test */
    tmessage("Frame Buffer Pixrect Test: ");
    errmsg = pixrect_complete_test(pr);
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

    /* LUT test */
    tmessage("Lookup Tables: ");
    /* save current plane group */
    group = pr_get_plane_group(pr);
    /* set 24-bit plane */
    pr_set_plane_group(pr, PIXPG_24BIT_COLOR);
    errmsg = lut_test(pr);
    /* retore original plane */
    pr_set_plane_group(pr, group);
    if (errmsg) {
	error_report(errmsg);
    } else {
	pmessage("ok.\n");
    }

    (void)pr_close(pr);
#endif NOT_TESTING

    /* GPSI */
    gpsi();

}
