#ifndef	lint
static char	sccsid[] = "@(#)ex1-1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 1-1, page 3
 */
#include <cgidefs.h>

#define	BOXPTS	5

Ccoor box[BOXPTS] = {	10000,10000 ,
			10000,20000 ,
			20000,20000 ,
			20000,10000 ,
			10000,10000 };

main()
{
    Cint	name;		/* name of CGI device (set by CGI)   */
    Cvwsurf	device;		/* device struct (see NORMAL_VWSURF) */
    Ccoorlist	boxlist;	/* struct of info for list of points */

    boxlist.n = BOXPTS;		/* set number of points              */
    boxlist.ptlist = box;	/* set pointer to list of points     */

    NORMAL_VWSURF(device, PIXWINDD);/* view surface is default window    */

    open_cgi();			/* open CGI and the view surface     */
    open_vws(&name, &device);

    polyline(&boxlist);		/* watch the '&' here!               */
    sleep(10);

    close_vws(name);		/* close up the view surface and CGI */
    close_cgi();
}
