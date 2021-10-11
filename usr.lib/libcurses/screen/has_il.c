/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)has_il.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.3 */
#endif

#include "curses_inc.h"

/* Query: does the terminal have insert/delete line? */

has_il()
{
    return (((insert_line || parm_insert_line) && 
	(delete_line || parm_delete_line)) || change_scroll_region);
}
