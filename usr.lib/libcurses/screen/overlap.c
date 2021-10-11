/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)overlap.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

#include	"curses_inc.h"

/*
 * This routine writes Srcwin on Dstwin.
 * Only the overlapping region is copied.
 */

_overlap(Srcwin, Dstwin, Overlay)
register	WINDOW	*Srcwin, *Dstwin;
int			Overlay;
{
    register	int	sby, sbx, sey, sex, dby, dbx, dey, dex,
			top, bottom, left, right;

#ifdef	DEBUG
    if (outf)
	fprintf(outf, "OVERWRITE(0%o, 0%o);\n", Srcwin, Dstwin);
#endif	/* DEBUG */

    sby = Srcwin->_begy;	dby = Dstwin->_begy;
    sbx = Srcwin->_begx;	dbx = Dstwin->_begx;
    sey = sby + Srcwin->_maxy;	dey = dby + Dstwin->_maxy;
    sex = sbx + Srcwin->_maxx;	dex = dbx + Dstwin->_maxx;

    if (sey < dby || sby > dey || sex < dbx || sbx > dex)
	return (ERR);

    top = _MAX(sby, dby);	bottom = _MIN(sey, dey);
    left = _MAX(sbx, dbx);	right = _MIN(sex, dex);

    sby = top - sby;		sbx = left - sbx;
    dey = bottom - dby - 1;	dex = right - dbx - 1;
    dby = top - dby;		dbx = left - dbx;

    return (copywin(Srcwin, Dstwin, sby, sbx, dby, dbx, dey, dex, Overlay));
}
