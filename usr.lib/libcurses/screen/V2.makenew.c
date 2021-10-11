/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)V2.makenew.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

#include "curses_inc.h"

#ifdef _VR2_COMPAT_CODE
extern WINDOW *_makenew();

WINDOW *
makenew(num_lines, num_cols, begy, begx)
int	num_lines, num_cols, begy, begx;
{
	return _makenew(num_lines, num_cols, begy, begx);
}
#endif
