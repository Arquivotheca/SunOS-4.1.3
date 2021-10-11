/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)delwin.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.6 */
#endif

#include	"curses_inc.h"

/* This routine deletes a _window and releases it back to the system. */

delwin(win)
register	WINDOW	*win;
{
    register	int	i;
    WINDOW		*par;

    /* If we have children don't delte the window. */
    if (win->_ndescs > 0)
	return (ERR);

    /*
     * If window is a pad, delete the padwin associated with it.
     * NOTE: We shouldn't care that the recursive call will decrement
     * ndescs for this window, since the window will be deleted anyhow.
     */

    if (win->_padwin)
    {
	win->_padwin->_maxy = win->_maxy;
	(void) delwin(win->_padwin);
    }
    if (win->_parent == NULL)
    {
	/* Delete all the memory associated with this window. */
	for (i = win->_maxy; i-- > 0;)
	{
	    free((char *) win->_y[i]);
#ifdef	_VR3_COMPAT_CODE
	    if (_y16update)
		free((char *) win->_y16[i]);
#endif	/* _VR3_COMPAT_CODE */
	}
    }
    else
    {
	/*
	 * We are a subwin and we don't want to delete the memory since
	 * it's shared by other windows.  We do want to decrement the
	 * descendant count so that if there are no children left to a
	 * particular window winsdelln.c will run in fast mode (look there).
	 */
	for (par = win->_parent; par != NULL; par = par->_parent)
	    par->_ndescs--;
    }

#ifdef	_VR3_COMPAT_CODE
    if (_y16update)
	free((char *) win->_y16);
#endif	/* _VR3_COMPAT_CODE */

    free((char *) win->_y);
    free((char *) win->_firstch);
    free((char *) win);
    return (OK);
}
