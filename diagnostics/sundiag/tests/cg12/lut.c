
#ifndef lint
static  char sccsid[] = "@(#)lut.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <suntool/sunview.h>
#include <esd.h>

extern
char		*lut_test();

extern
char		*sprintf();

extern
char		*device_name;

/**********************************************************************/
char *
lut()
/**********************************************************************/

{
    static char errtxt[256];
    char *errmsg;
    Pixrect *pr;
    int group;

    func_name = "lut";
    TRACE_IN

    pr = pr_open(device_name);
    if (pr == NULL) {
	(void)sprintf(errtxt, errmsg_list[19],
					    device_name);
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
	(void)pr_close(pr);
	TRACE_OUT
	return errmsg;
    }

    pr_set_plane_group(pr, group);
    (void)pr_close(pr);
    TRACE_OUT
    return (char *)0;
    
}
