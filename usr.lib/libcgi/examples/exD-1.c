#ifndef	lint
static char	sccsid[] = "@(#)exD-1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example D-1, page 137
 */
#include <cgidefs.h>

Ccoorlist martinilist;
Ccoor glass_coords[10] = { 0,0,
						-10,0,
						-1,1,
						-1,20,
						-15,35,
						15,35,
						1,20,
						1,1,
						10,0,
						0,0 };
Ccoor water_coords[2] = { -12,33,
						12,33 };
Ccoor vpll = { -50,-10 };
Ccoor vpur = { 50,80 };

main()
{
	Cvwsurf device;
	Cint name;

	NORMAL_VWSURF(device, PIXWINDD);

	open_cgi();
	open_vws(&name, &device);
	vdc_extent(&vpll, &vpur);

	martinilist.ptlist = glass_coords;
	martinilist.n = 10;
	polyline(&martinilist);
	martinilist.ptlist = water_coords;
	martinilist.n = 2;
	polyline(&martinilist);

	sleep(10);
	close_vws(name);
	close_cgi();
}
