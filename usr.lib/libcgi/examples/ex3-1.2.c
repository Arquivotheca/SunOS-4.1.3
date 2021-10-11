#ifndef	lint
static char	sccsid[] = "@(#)ex3-1.2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 3-1.2, page 41
 */
#include <cgidefs.h>

/* Indices of colors to be used.  Using the defaults. */
#define	RED	1
#define	YELLOW	2
#define	GREEN	3
#define	CYAN	4

/* definitions for circle coord's and values */
#define	MID	16000
#define	RAD	(MID / 2)
#define	MIN	(MID - RAD)
#define	MAX	(MID + RAD)

main()		/* draws four quadrants in different colors and fills */
{
    Ccoor	c1;	/* coord of center	      */
    Cint	name;	/* CGI name for view surface  */
    Cvwsurf	device;	/* view surface device struct */

    c1.x = c1.y = MID;	/* center */

    NORMAL_VWSURF( device, CGPIXWINDD);

    open_cgi();
    open_vws( &name, &device);

    perimeter_width( 1.0);		/* perimeter width 1% of VDC */

    interior_style( SOLIDI, OFF);
    fill_color( YELLOW);		/* color of quadrant 2	*/
    circular_arc_center_close( &c1, MAX, MID, MID, MAX, RAD, PIE);

    interior_style( HOLLOW, OFF);
    fill_color( GREEN);			/* color of quadrant 3	*/
    circular_arc_center_close( &c1, MID, MAX, MIN, MID, RAD, PIE);

    interior_style( SOLIDI, ON);
    fill_color( CYAN);			/* color of quadrant 4	*/
    circular_arc_center_close( &c1, MIN, MID, MID, MIN, RAD, PIE);

    interior_style( HOLLOW, ON);
    fill_color( RED);			/* color of quadrant 1	*/
    circular_arc_center_close( &c1, MID, MIN, MAX, MID, RAD, PIE);

    sleep(10);
    close_vws(name);			/* close view surface and CGI */
    close_cgi();
}
