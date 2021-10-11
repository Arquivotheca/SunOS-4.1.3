#ifndef	lint
static char	sccsid[] = "@(#)exD-2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example D-2, page 138
 */
#include <cgidefs.h>
#define DEVNUM	1			/* device number */
#define MOUSE_POSITION	5	/* trigger number */
#define TIMEOUT (5 * 1000 * 1000)	/* timeout in microseconds */

Ccoor ulc = {1000, 2000};
Ccoor lrc = {2000, 1000};

main()
{
	Cint name;
	Cvwsurf device;
	Cawresult stat;
	Cinrep sample;	/* device measure value */
	Ccoor samp;		/* LOCATOR's x,y position */
	Cint trigger;	/* trigger number */

	NORMAL_VWSURF(device, PIXWINDD);
	sample.xypt = &samp;
	samp.x = 0;
	samp.y = 27000;

	open_cgi();
	open_vws(&name, &device);
	set_global_drawing_mode(XOR);
	initialize_lid(IC_LOCATOR, DEVNUM, &sample);
	associate(MOUSE_POSITION, IC_LOCATOR, DEVNUM);
	rectangle(&lrc, &ulc);	/* draw first rectangle */
		/* wait TIMEOUT micro-seconds for input and check the status */
	while (request_input(IC_LOCATOR, DEVNUM, TIMEOUT,
		&stat, &sample, &trigger), (stat == VALID_DATA)) {
		if ((sample.xypt->x != ulc.x) || (sample.xypt->y != lrc.y) ) {
			rectangle(&lrc, &ulc);
			lrc.y = sample.xypt->y;	/* move to new location */
			lrc.x = (sample.xypt->x + 1000);
			ulc.x = sample.xypt->x;
			ulc.y = (sample.xypt->y + 1000);
			rectangle(&lrc, &ulc);
		}
	}
	dissociate(MOUSE_POSITION, IC_LOCATOR, DEVNUM);
	release_input_device(IC_LOCATOR, DEVNUM);
	close_vws(name);
	close_cgi();
}
