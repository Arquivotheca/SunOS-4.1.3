/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)killchar.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.1.1.4 */
#endif

/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

char
killchar()
{
#ifdef SYSV
    return (SHELLTTY.c_cc[VKILL]);
#else
    return (SHELLTTY.sg_kill);
#endif
}
