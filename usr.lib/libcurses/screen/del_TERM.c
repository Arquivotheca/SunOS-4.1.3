/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)del_TERM.c 1.1 92/07/30 SMI"; /* from S5R3 1.5 */
#endif

#include "curses.ext"

/*
 * Relinquish the storage associated with "oldterminal".
 */
extern TERMINAL _first_term;
extern int _called_before;
extern char _frst_tblstr[];

int del_curterm(oldterminal)
register TERMINAL *oldterminal;
{
	if (oldterminal) {
		if (oldterminal == &_first_term) {
			/* next setupterm can reuse static areas */
			_called_before = FALSE;
			if (oldterminal->_strtab != _frst_tblstr)
				free((char *)oldterminal->_strtab);
		} else {
			free((char *)oldterminal->_bools);
			free((char *)oldterminal->_nums);
			free((char *)oldterminal->_strs);
			free((char *)oldterminal->_strtab);
			free((char *)oldterminal);
		}
		return OK;
	} else
		return ERR;
}
