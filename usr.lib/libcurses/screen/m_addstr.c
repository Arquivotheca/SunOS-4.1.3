/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_addstr.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

# include	"curses.ext"
# include	<signal.h>

/*
 *	This routine adds a string starting at (_cury,_curx)
 *
 */
m_addstr(str)
register char	*str;
{
# ifdef DEBUG
	if(outf) fprintf(outf, "M_ADDSTR(\"%s\")\n", str);
# endif
	while (*str)
		if (m_addch((chtype) *str++) == ERR)
			return ERR;
	return OK;
}
