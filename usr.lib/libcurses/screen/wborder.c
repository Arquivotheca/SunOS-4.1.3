/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)wborder.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

/*
 *	Draw a box around a window.
 *
 *	ls : the character and attributes used for the left side.
 *	rs : right side.
 *	ts : top side.
 *	bs : bottom side.
 */

#define	_LEFTSIDE	variables[0]
#define	_RIGHTSIDE	variables[1]
#define	_TOPSIDE	variables[2]
#define	_BOTTOMSIDE	variables[3]
#define	_TOPLEFT	variables[4]
#define	_TOPRIGHT	variables[5]
#define	_BOTTOMLEFT	variables[6]
#define	_BOTTOMRIGHT	variables[7]

static	char	acs_values[] =
		{
		    'x', /* ACS_VLINE */
		    'x', /* ACS_VLINE */
		    'q', /* ACS_HLINE */
		    'q', /* ACS_HLINE */
		    'l', /* ACS_ULCORNER */
		    'k', /* ACS_URCORNER */
		    'm', /* ACS_LLCORNER */
		    'j' /* ACS_LRCORNER */
		};

wborder(win, ls, rs, ts, bs, tl, tr, bl, br)
register	WINDOW	*win;
chtype		ls, rs, ts, bs, tl, tr, bl, br;
{
    register	int	i, endy = win->_maxy - 1, endx = win->_maxx - 2;
    register	chtype	**_y = win->_y;	/* register version */
    chtype		*line_ptr, variables[8];

    _LEFTSIDE = ls;
    _RIGHTSIDE = rs;
    _TOPSIDE = ts;
    _BOTTOMSIDE = bs;
    _TOPLEFT = tl;
    _TOPRIGHT = tr;
    _BOTTOMLEFT = bl;
    _BOTTOMRIGHT = br;

    for (i = 0; i < 8; i++)
    {
	if (_CHAR(variables[i]) == 0)
	    variables[i] = acs_map[acs_values[i]];
	variables[i] = _WCHAR(win, variables[i]) | _ATTR(variables[i]);
    }

    /* do top and bottom edges and corners */
    memSset((line_ptr = &_y[0][1]), _TOPSIDE, endx);
    *(--line_ptr) = _TOPLEFT;
    line_ptr[++endx] = _TOPRIGHT;

    memSset((line_ptr = &_y[endy][1]), _BOTTOMSIDE, --endx);
    *--line_ptr = _BOTTOMLEFT;
    line_ptr[++endx] = _BOTTOMRIGHT;
#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
    {
	(*_y16update)(win, 1, ++endx, 0, 0);
	(*_y16update)(win, 1, endx--, endy, 0);
    }
#endif	/* _VR3_COMPAT_CODE */

    /* left and right edges */
    while (--endy > 0)
    {
	_y[endy][0] = _LEFTSIDE;
	_y[endy][endx] = _RIGHTSIDE;
#ifdef	_VR3_COMPAT_CODE
	if (_y16update)
	{
	    win->_y16[endy][0] = _TO_OCHTYPE(_LEFTSIDE);
	    win->_y16[endy][endx] = _TO_OCHTYPE(_RIGHTSIDE);
	}
#endif	/* _VR3_COMPAT_CODE */
    }
    return (wtouchln((win), 0, (win)->_maxy, TRUE));
}
