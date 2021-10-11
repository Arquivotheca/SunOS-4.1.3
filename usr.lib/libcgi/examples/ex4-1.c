#ifndef	lint
static char	sccsid[] = "@(#)ex4-1.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * Example 4-1, page 57
 */
#include <cgidefs.h>

#define	BOXPTS		5
#define	NUMATTRS	18
#define	BUNDLE_INDEX	2

Ccoor box[BOXPTS] = {	10000,10000 ,
			10000,20000 ,
			20000,20000 ,
			20000,10000 ,
			10000,10000 };
	
Cbunatt bundle = {	DASHED_DOTTED, 1., 4, X, 6., 4,
			PATTERN, 1, 1, 2, DOTTED, 1.5, 1,
			STICK, CHARACTER, 1.3, 0.05, 1 };

main()
{
    Cint	i,			/* counter variable	*/
		name;
    Cvwsurf	device;
    Ccoorlist	boxlist;		/* structure of coords.	*/
    Cflaglist	flags;			/* structure of ASF's	*/

    boxlist.n = BOXPTS;
    boxlist.ptlist = box;

    NORMAL_VWSURF(device, PIXWINDD);

    open_cgi();
    open_vws(&name, &device);

    /* allocate room for array of ASF's (one for each attribute)	*/
    flags.value = (Casptype *) malloc( NUMATTRS*sizeof(Casptype) );
    flags.num = (Cint *) malloc( NUMATTRS*sizeof(Cint) );

    /* set all the ASF's to "BUNDLED", and set the appropriate flag #'s	*/
    for (i = 0; i < NUMATTRS; i++) {
	flags.value[i] = BUNDLED;
	flags.num[i] = i;
    }
    flags.n = NUMATTRS;

    /* define our bundle which contains a setting for every attribute	*/
    define_bundle_index( BUNDLE_INDEX, &bundle);
    set_aspect_source_flags( &flags);
    polyline_bundle_index( BUNDLE_INDEX); /* select our polyline bundle	*/

    polyline( &boxlist);
    sleep(10);

    close_vws(name);			/* close view surface and CGI	*/
    close_cgi();
}
