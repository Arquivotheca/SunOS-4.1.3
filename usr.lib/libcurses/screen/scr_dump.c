/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)scr_dump.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.7 */
#endif

#include	"curses_inc.h"

/*
 * Dump a screen image to a file. This routine and scr_reset
 * can be used to communicate the screen image across processes.
 */

scr_dump(file)
char	*file;
{
    int		rv;
    FILE	*filep;

    if ((filep = fopen(file,"w")) == NULL)
    {
#ifdef	DEBUG
	if (outf)
	    (void) fprintf (outf, "scr_dump: cannot open \"%s\".\n", file);
#endif	/* DEBUG */
	return (ERR);
    }
    rv = scr_ll_dump(filep);
    fclose(filep);
    return (rv);
}
