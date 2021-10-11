#ifndef	lint
static char	sccsid[] = "@(#)ex5-2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 5-2, page 92
 */
/*	NOTE: This example should be run from a gfxtool.		    */

#include <stdio.h>
#include <cgidefs.h>

#define TEN_SECS	(10 * 1000 * 1000)
#define	LID_LOC		1
#define	MOUSE_BUTTON_1	2
#define	MOUSE_BUTTON_2	3
#define	MOUSE_BUTTON_3	4
#define	MOUSE_MOVE	5

main()
{
    static Ccoor  ipt = { 16384, 16384 };	/* initial pt */
    static Cinrep ivalue = { &ipt, NULL, 0., 0, '\0', NULL }; /* init LID */
    Cawresult	  valid;
    Cint	  name,
		  trigger;
    Cvwsurf	  device;
    char	  dummy, buf[80];

    fprintf(stderr,"Move cursor in graphics area & click mouse buttons.\n");
    fprintf(stderr, "To exit, press mouse button three (right).\n");

    NORMAL_VWSURF( device, PIXWINDD);

    open_cgi();
    open_vws( &name, &device);

    initialize_lid( IC_LOCATOR, LID_LOC, &ivalue);  /* create locator dev */
    associate(MOUSE_BUTTON_1, IC_LOCATOR, LID_LOC); /* trigger = button 1 */
    associate(MOUSE_BUTTON_2, IC_LOCATOR, LID_LOC); /* trigger = button 1 */
    associate(MOUSE_BUTTON_3, IC_LOCATOR, LID_LOC); /* trigger = button 1 */
    associate(MOUSE_MOVE, IC_LOCATOR, LID_LOC);     /* trigger = button 1 */

    do {	/* loop until get bad data or hit right mouse button  */
	request_input(IC_LOCATOR, LID_LOC, TEN_SECS,
		    &valid, &ivalue, &trigger);
	sprintf(buf, "X= %d  Y= %d  valid= %d trig= %d\n",
		    ipt.x, ipt.y, valid, trigger);
	fprintf(stderr, "%s", buf);
    }	while (valid == VALID_DATA  && trigger != MOUSE_BUTTON_3);

    dissociate(MOUSE_BUTTON_1, IC_LOCATOR, LID_LOC);
    dissociate(MOUSE_BUTTON_2, IC_LOCATOR, LID_LOC);
    dissociate(MOUSE_BUTTON_3, IC_LOCATOR, LID_LOC);
    dissociate(MOUSE_MOVE, IC_LOCATOR, LID_LOC);
    release_input_device(IC_LOCATOR, LID_LOC);	/* shut down locator  */

    close_vws(name);
    close_cgi();
}
