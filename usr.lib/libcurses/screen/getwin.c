/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)getwin.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

#define		SEPARATE_READ	6

/* Read a window that was stored by putwin. */

WINDOW	*
getwin(filep)
FILE	*filep;
{
    short		*save_fch, win_nums[SEPARATE_READ], maxy, maxx, nelt;
    register	WINDOW	*win = NULL;
    register	chtype	**ecp, **wcp;

    /* read everything from _cury to _bkgd inclusive */

    nelt = sizeof(WINDOW) - sizeof(win->_y) - sizeof(win->_parent) -
	   sizeof(win->_parx) - sizeof(win->_pary) -
	   sizeof(win->_ndescs) - sizeof(win->_delay) -
	   (SEPARATE_READ * sizeof(short));

    if ((fread((char *) win_nums, sizeof(short), SEPARATE_READ, filep) != SEPARATE_READ) ||
	((win = _makenew(maxy = win_nums[2], maxx = win_nums[3], win_nums[4], win_nums[5])) == NULL))
    {
	goto err;
    }

    if (_image(win) == ERR)
    {
	win = (WINDOW *) NULL;
	goto err;
    }
    save_fch = win->_firstch;

    if (fread(&(win->_flags), 1, nelt, filep) != nelt)
	goto err;

    win->_firstch = save_fch;
    win->_lastch = save_fch + maxy;

    /* read the image */
    wcp = win->_y;
    ecp = wcp + maxy;

    while (wcp < ecp)
	if (fread((char *) *wcp++, sizeof(chtype), maxx, filep) != maxx)
	{
err :
	    if (win != NULL)
		(void) delwin(win);
	    return ((WINDOW *) NULL);
	}

    win->_cury = win_nums[0];
    win->_curx = win_nums[1];
    win->_use_idl = win->_use_keypad = FALSE;

    return (win);
}
