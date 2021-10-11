#ifndef	lint
static char	sccsid[] = "@(#)ex2-1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 2-1, page 11
 */
#include <cgidefs.h>

main()
{
    Ccoor   ll,		/* coord of lower-left corner of rectangle   */
	    ur,		/* coord of upper-right corner of rectangle  */
	    center;	/* coord of center of circle                 */
    Cint    name1,	/* name of first CGI view surface            */
	    name2,	/* name of second CGI view surface           */
	    radius;	/* radius of circle                          */
    Cvwsurf device1,	/* 1st CGI device struct (see NORMAL_VWSURF) */
	    device2;	/* 2nd CGI device struct (see NORMAL_VWSURF) */
    static  Cchar   *scrn_name = "/dev/fb";
    char    *toolposition = "0,0,250,250,0,0,250,250,0";

    ll.x = ll.y = 5000;			/* set rectangle coordinates */
    ur.x = ur.y = 15000;

    center.x = center.y = 10000;	/* set circle coordinates */
    radius = 5000;

    NORMAL_VWSURF(device1, PIXWINDD);	/* set up a default window        */
    NORMAL_VWSURF(device2, PIXWINDD);	/* set up a 2nd default window    */
    device2.flags = VWSURF_NEWFLG;	/* don't take over current window */
    device2.ptr = &toolposition;	/* set position and size of 2nd   */

    open_cgi();				/* open CGI and view surfaces */
    open_vws(&name1, &device1);
    open_vws(&name2, &device2);

    rectangle(&ll, &ur);	/* rectangle draws on both devices */
    deactivate_vws(name2);	/* only display one is acive now */
    circle(&center, radius);	/* draw circle on device one only */
    activate_vws(name2);	/* both displays active again */
    circle(&center, 2*radius);	/* circle draws on both devices */

    sleep(20);

    close_vws(name1);		/* close view surfaces and CGI */
    close_vws(name2);
    close_cgi();
}
