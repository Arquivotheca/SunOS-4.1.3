#ifndef	lint
static char	sccsid[] = "@(#)ex5-3.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 5-3, page 95
 */
/*	NOTE: This example should be run from a gfxtool.		    */
#include <cgidefs.h>

#define	XBASE	1.0
#define	YBASE	0.0
#define	XUP	0.0
#define	YUP	1.0
#define TEN_SECONDS (10 * 1000 * 1000)

main()
{
    Cint	name;
    Cvwsurf	device;
    Cawresult	valid;
    Ccoor	point,
    		coord;
    Cdevoff	devclass = IC_STRING;
    Ceqflow 	overflow;
    Cinrep	ivalue;
    Cint	devnum = 1,
		replost,
		time_stamp,
		timeout = TEN_SECONDS,
		tracktype = 2,
		trigger = 1;
    Cmesstype	message_link;
    Cqtype	qstat;

    NORMAL_VWSURF( device, PIXWINDD);
    point.x = point.y = 16384;
    coord.x = coord.y = 4000;
    ivalue.xypt = &point;
    ivalue.string = "Return from await_event";

    open_cgi();
    open_vws( &name, &device);

    /* insert a text string as a prompt for event input */
    character_height( 1000);
    text_font_index( 1);

    /* set string precision */
    text_precision( STRING);

    /* set the alignment to RIGHT and TOP , so the entered text
     * matches the prompt */
    text_alignment( RIGHT, TOP);

    /* set the orientation to the default */
    character_orientation( XBASE, YBASE, XUP, YUP);

    /* issue a text primitive. This action will set the
     * text concatenation point.  */
    text( &coord, "Enter string: ");

    initialize_lid( devclass, devnum, &ivalue);
    associate( trigger, devclass, devnum);
    track_on( devclass, devnum, tracktype, (Ccoorpair *)0, &ivalue);
    enable_events( devclass, devnum);

    await_event( timeout, &valid, &devclass, &devnum, &ivalue,
	    &message_link, &replost, &time_stamp, &qstat, &overflow);
    printf( "%s\n", ivalue.string);
    disable_events( IC_STRING, devnum);
    dissociate( trigger, IC_STRING, devnum);
    release_input_device( IC_STRING, devnum);

    close_vws( name);
    close_cgi();
}
