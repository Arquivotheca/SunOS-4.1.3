#ifndef	lint
static char	sccsid[] = "@(#)ex2-2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif	lint

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
/* SunCGI REFERENCE MANUAL, Rev. A, 9 May 1988, PN 800-1786-10 -- SunOS 4.0
 * Example 2-2, page 19
 */
#include <cgidefs.h>

main()
{
    Cvwsurf device;
    Cint name;
    Ccoor dv1, dv2, lower, upper;

    NORMAL_VWSURF(device, PIXWINDD);

    open_cgi();
    open_vws(&name, &device);
    dv1.x = dv1.y = 0;
    dv2.x = dv2.y = 200;	/* coord. space (0<x|y<200) */
    vdc_extent(&dv1, &dv2);

    lower.x = lower.y = 30;	/* rectangle coordinates */
    upper.x = upper.y = 70;
    rectangle(&upper, &lower);	/* draw initial rectangle */
    sleep(4);

    dv1.x = dv1.y = 0;
    dv2.x = dv2.y = 100;	/* coord. space (0<x|y<100) */
    vdc_extent(&dv1, &dv2);	/* center rectangle */
    rectangle(&upper, &lower);
    sleep(4);

    dv1.x = dv1.y = 20;
    dv2.x = dv2.y = 80;		/* coord. space (20<x|y<80) */
    vdc_extent(&dv1, &dv2);	/* enlarge rectangle */
    rectangle(&upper, &lower);
    sleep(20);

    close_vws(name);
    close_cgi();
}
