/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)curserr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

#include 	"curses_inc.h"

char	*curs_err_strings[] =
{
    "I don't know how to deal with your \"%s\" terminal",
    "I need to know a more specific terminal type than \"%s\"",	/* unknown */
#ifdef	DEBUG
    "malloc returned NULL in function \"%s\"",
#else	/* DEBUG */
    "malloc returned NULL",
#endif	/* DEBUG */
};

void
curserr()
{
    fprintf(stderr, "Sorry, ");
    fprintf(stderr, curs_err_strings[curs_errno], curs_parm_err);
    fprintf(stderr, ".\r\n");
}
