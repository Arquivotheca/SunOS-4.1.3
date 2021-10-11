/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_move.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

# include	"curses.ext"

/*
 * Move to given location on stdscr.  Update curses notion of where we
 * are (stdscr). This could be a macro
 * and originally was, but if you do
 *	move(line++, 0)
 * it will increment line twice, which is a lose.
 *
 * how about?
 *	#define move(r,c)	(stdscr->_cury = r, stdscr->_curx=c,\
 *				    _ll_move(stdscr->_cury, stdscr->_curx))
 * or?
 *	#define move(r,c)	_ll_move(stdscr->_cury=r, stdscr->_curx=c)
 *
 */
m_move(row, col)
int row, col;
{
	stdscr->_cury=row;
	stdscr->_curx=col;
	_ll_move(row,col);
}
