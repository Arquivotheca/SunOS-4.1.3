/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)derwin.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.2 */
#endif

#include	"curses_inc.h"

/*
 *	Make a  derived window of an existing window. The two windows
 *	share the same character image.
 *		orig:	the original window
 *		nl, nc:	numbers of lines and columns
 *		by, bx:	coordinates for upper-left corner of the derived
 *			window in the coord system of the parent window.
 */

WINDOW	*
derwin(orig, num_lines, nc, by, bx)
WINDOW	*orig;
int	num_lines, nc, by, bx;
{
    register	int	y;
    register	WINDOW	*win = (WINDOW *) NULL, *par;
    register	chtype	**w_y, **o_y;
#ifdef	_VR3_COMPAT_CODE
    register	_ochtype	**w_y16, **o_y16;
#endif	/* _VR3_COMPAT_CODE */

    /* make sure window fits inside the original one */
    if (by < 0 || (by + num_lines) > orig->_maxy || bx < 0 || (bx + nc) > orig->_maxx)
	goto done;
    if (nc == 0)
	nc = orig->_maxx - bx;
    if (num_lines == 0)
	num_lines = orig->_maxy - by;

    /* Create the window skeleton */
    if ((win = _makenew(num_lines, nc, by + orig->_begy, bx + orig->_begx)) == NULL)
	goto done;

    /* inheritance */
    win->_parx = bx;
    win->_pary = by;
    win->_bkgd = orig->_bkgd;
    win->_attrs = orig->_attrs;
    w_y = win->_y;
    o_y = orig->_y;

#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
    {
	 int	hby = by;

         w_y16 = win ->_y16;
         o_y16 = orig->_y16;

         for (y = 0; y < num_lines; y++, hby++)
	     w_y16[y] = o_y16[hby] + bx;
    }
#endif	/* _VR3_COMPAT_CODE */

    for (y = 0; y < num_lines; y++, by++)
	w_y[y] = o_y[by] + bx;

    win->_yoffset = orig->_yoffset;

    /* update descendant number of ancestors */
    win->_parent = orig;
    for (par = win->_parent; par != NULL; par = par->_parent)
	par->_ndescs += 1;

done:
    return (win);
}
