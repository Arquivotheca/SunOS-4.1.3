#ifndef	lint
static char	sccsid[] = "@(#)ex3-1.1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 3-1.1, page 39
 */
#include <cgidefs.h>

#define NPTS	4

main()
{
    Cint	name;
    Cvwsurf	device;
    Ccoor	list[NPTS];		/* list of coords. */
    Ccoorlist	points;			/* structure for list of coords. */

    NORMAL_VWSURF(device, PIXWINDD);

    open_cgi();
    open_vws(&name, &device);

    interior_style(SOLIDI, ON);		/* solid fill with edge */
    list[0].x = list[0].y = 10000;
    list[1].x = 10000;
    list[1].y = 20000;
    list[2].x = list[2].y = 20000;
    list[3].x = 20000;
    list[3].y = 10000;
    points.ptlist = list;
    points.n = NPTS;
    partial_polygon(&points, CLOSE);	/* draw closed polygon */

    list[0].x = list[0].y = 12500;
    list[1].x = 12500;
    list[1].y = 17500;
    list[2].x = list[2].y = 17500;
    list[3].x = 17500;
    list[3].y = 12500;
    points.ptlist = list;		/*[Redundant, but good practice]*/
    points.n = NPTS;
    polygon(&points);			/* cut a hole in it */

    sleep(10);

    close_vws(name);
    close_cgi();
}
