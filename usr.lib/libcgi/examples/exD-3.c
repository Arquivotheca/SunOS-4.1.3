#ifndef	lint
static char	sccsid[] = "@(#)exD-3.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example D-3, page 139
 */
#include <cgidefs.h>
#include <stdio.h>

#define	NCOLORS	64
#define	LCOLOR	(NCOLORS-1)
#define C_STEP	(256/NCOLORS)
#define	MIN	0
#define	MAX	10000
#define STEP	((MAX-MIN)/NCOLORS)

typedef	unsigned char	Color;

static	Ccoor	vpll	= { MIN, MIN };	/* lower left corner */
static	Ccoor	vpur	= { MAX, MAX };	/* upper right corner */

main()
{
	int	name;			/* view surface name */
	Cvwsurf	device;			/* view surface device */       
	Ccoorlist line;			/* line coordinate list */
	Ccoor	points[2];		/* point list */
	int	i;			/* position counter */
	Ccentry	clist;			/* color map list */
	Color	red[NCOLORS];		/* red color map */
	Color	green[NCOLORS];		/* green color map */
	Color	blue[NCOLORS];		/* blue color map */

	NORMAL_VWSURF(device, PIXWINDD);/* select output device */
	device.cmapsize = NCOLORS;	/* # of colors in MY map */
	strcpy(device.cmapname, "mymap"); /* copy name of MY map */
	open_cgi();			/* initilize cgi */
	open_vws(&name,&device);	/* open view surface */
	vdc_extent(&vpll,&vpur);	/* reset vdc space */

	line_width_specification_mode(ABSOLUTE);
					/* set the line attributes */
	line_width(1.0);

	/* set background color (loc[0]) to all black */
	red[0] = green[0] = blue[0] = 0;
	/* set cursor color (loc[LAST]) to green */
	red[LCOLOR] = blue[LCOLOR] = 0;  green[LCOLOR] = 255;

	for(i=1; i<LCOLOR; i++) {	/* set up the color map */
		red[i] = (i*C_STEP);
		green[i] = 128;		/* goes from blue to red */
		blue[i] = 256-(i*C_STEP);
	}
	clist.n = NCOLORS;
	clist.ra = red;
	clist.ga = green;
	clist.ba = blue;

	color_table(0,&clist);

	line.n = 2;			/* draw a line */
	line.ptlist = points;
	points[0].y = MIN;
	points[1].y = MAX;
	for( i=1; i<LCOLOR; i++) {
		line_color(i);
		points[0].x = (i*STEP);
		points[1].x = (i*STEP);
		polyline(&line);
	}
	sleep(10);
							
	close_vws(name);
	close_cgi();
}
