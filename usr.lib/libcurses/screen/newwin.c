/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)newwin.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

/* allocate space for and set up defaults for a new _window */

#include	"curses_inc.h"

WINDOW	*
newwin(nlines, ncols, by, bx)
register	int	nlines, ncols, by, bx;
{
    register	WINDOW	*win;
    int		counter = 0;

    if (nlines <= 0)
	nlines = LINES - by;
    if (ncols <= 0)
	ncols = COLS - bx;

    if ((by < 0) || (bx < 0) ||
	((win = _makenew(nlines, ncols, by, bx)) == (WINDOW *) NULL) ||
	(_image(win) == ERR))
    {
	return ((WINDOW *) NULL);
    }

    while (counter < nlines)
    {
	memSset(&win->_y[counter][0], (chtype) ' ', ncols);
#ifdef	_VR3_COMPAT_CODE
	if (_y16update)
	{
	    int	i = ncols;

	    while (i--)
		win->_y16[counter][i] = (_ochtype) ' ';
	}
#endif	/* _VR3_COMPAT_CODE */
	counter++;
    }

    win->_yoffset = SP->Yabove;
    return (win);
}

_image(win)
register	WINDOW	*win;
{
    register	int	i, nlines = win->_maxy,
#ifdef	_VR3_COMPAT_CODE
			oscols = win->_maxx * sizeof (_ochtype),
#endif	/* _VR3_COMPAT_CODE */
			scols = win->_maxx * sizeof (chtype);
    register	chtype	**_y = win->_y;
#ifdef	_VR3_COMPAT_CODE
    register	_ochtype	**_y16 = win->_y16;
#endif	/* _VR3_COMPAT_CODE */
    
    for (i = 0; i < nlines; i++)
    {
#ifdef	_VR3_COMPAT_CODE
	if (((_y[i] = (chtype *) malloc((unsigned) scols)) == NULL)
	    || ((_y16update) &&
	    ((_y16[i] = (_ochtype *) malloc((unsigned) oscols)) == NULL)))
#else	/* _VR3_COMPAT_CODE */
	if ((_y[i] = (chtype *) malloc((unsigned) scols)) == NULL)
#endif	/* _VR3_COMPAT_CODE */
	{
	    register	int	j;

	    curs_errno = CURS_BAD_MALLOC;
#ifdef	DEBUG
	    strcpy(curs_parm_err, "_image");
#endif	/* DEBUG */
#ifdef	_VR3_COMPAT_CODE
	    for (j = 0; j <= i; j++)
	    {
		if (_y[j] != NULL)
		    free((char *) _y[j]);
		if ((_y16update) && (_y16[j] != NULL))
		    free((char *) _y16[j]);
	    }
#else	/* _VR3_COMPAT_CODE */
	    for (j = 0; j < i; j++)
		free((char *) _y[j]);
#endif	/* _VR3_COMPAT_CODE */

	    free((char *) win->_firstch);
	    free((char *) win->_y);
#ifdef	_VR3_COMPAT_CODE
	    free((char *) win->_y16);
#endif	/* _VR3_COMPAT_CODE */
	    free((char *) win);
	    return (ERR);
	}
    }
    return (OK);
}
