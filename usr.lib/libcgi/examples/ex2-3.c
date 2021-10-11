#ifndef	lint
static char	sccsid[] = "@(#)ex2-3.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 2-3, page 25
 */
#include <cgidefs.h>

Ccoor box[5] = { 10000,10000 ,
				10000,20000 ,
				20000,20000 ,
				20000,10000 ,
				10000,10000 };
Ccoorlist boxlist;
Cint	name;
extern Cint redraw();
Cvwsurf device;
Cint	color_index;

main()
{

	color_index = 1;
	boxlist.n = 5;
	boxlist.ptlist = box;
	NORMAL_VWSURF(device, PIXWINDD);

	open_cgi();
	open_vws(&name, &device);
	set_up_sigwinch(name, redraw);

	polyline(&boxlist);
	sleep(30);

	close_vws(name);
	close_cgi();
}

Cint redraw()
{
	clear_view_surface(name, ON, 0);
	line_color((++color_index>7) ? color_index=1 : color_index);
	polyline(&boxlist);
}
