#ifndef	lint
static char	sccsid[] = "@(#)ex4-6.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 4-6, page 66
 */
#include <cgidefs.h>

#define	BOXPTS		5
#define	PAT_ROWS	4
#define	PAT_COLS	4
#define	PAT_SIZE	(PAT_ROWS * PAT_COLS)
#define	PAT_INDEX	2

#define	PAT_DX		250
#define	PAT_DY		250

Ccoor box[BOXPTS] = {	10000,10000 ,
			10000,20000 ,
			20000,20000 ,
			20000,10000 ,
			10000,10000 };

/*Cint pattern[PAT_SIZE] = {  50,  75, 100, 125,
			   150,   0,   0, 175,
			   200,   0,   0, 225,
			   250, 275, 300, 325 };*/
Cint pattern[PAT_SIZE] = { 6, 1, 1, 2,
			   7, 0, 0, 3,
			   7, 0, 0, 3,
			   6, 5, 5, 4 };

main()
{
    Cint	name;
    Cvwsurf	device;
    Ccoorlist	boxlist;

    boxlist.n = BOXPTS;
    boxlist.ptlist = box;
    NORMAL_VWSURF( device, PIXWINDD);

    open_cgi();
    open_vws( &name, &device);

    interior_style( PATTERN, ON);
    pattern_table( PAT_INDEX, PAT_ROWS, PAT_COLS, pattern);
    pattern_index( PAT_INDEX);
    pattern_size( PAT_DX, PAT_DY);
    polygon( &boxlist);

    sleep(10);

    close_vws( name);
    close_cgi();
}
