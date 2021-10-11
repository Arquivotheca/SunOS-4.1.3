/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)putp.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.9 */
#endif

/*
 * Handy functions to put out a string with padding.
 * These make two assumptions:
 *	(1) Output is via stdio to stdout through putchar.
 *	(2) There is no count of affected lines.  Thus, this
 *	    routine is only valid for certain capabilities,
 *	    i.e. those that don't have *'s in the documentation.
 */
#include	"curses_inc.h"

/*
 * Routine to act like putchar for passing to tputs.
 * _outchar should really be a void since it's used by tputs
 * and tputs doesn't look at return code.  However, tputs also has the function
 * pointer declared as returning an int so we didn't change it.
 */
_outchar(ch)
char	ch;
{
    (void) putchar(ch);
}

/* Handy way to output a string. */

putp(str)
char	*str;
{
    extern	int	_outchar();

    return (tputs(str, 1, _outchar));
}

/* Handy way to output video attributes. */

vidattr(newmode)
chtype	newmode;
{
    extern	int	_outchar();

    return (vidputs(newmode, _outchar));
}
