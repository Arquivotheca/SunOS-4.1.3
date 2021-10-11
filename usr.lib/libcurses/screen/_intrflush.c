/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)_intrflush.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.2 */
#endif

#define		NOMACROS
#include	"curses_inc.h"

intrflush(win, flag)
WINDOW	*win;
int	flag;
{
    _setqiflush(flag);
    return (OK);
}
