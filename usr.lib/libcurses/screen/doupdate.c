/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)doupdate.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5.1.7 */
#endif

#include	"curses_inc.h"

/*
 * Doupdate is a real function because _virtscr
 * is not accessible to application programs.
 */

doupdate()
{
    return (wrefresh(_virtscr));
}
