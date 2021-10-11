/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)m_addch.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

# include	"curses.ext"

/*
 *	mini.c contains versions of curses routines for minicurses.
 *	They work just like their non-mini counterparts but draw on
 *	std_body rather than stdscr.  This cuts down on overhead but
 *	restricts what you are allowed to do - you can't get stuff back
 *	from the screen and you can't use multiple windows or things
 *	like insert/delete line (the logical ones that affect the screen).
 *	All this but multiple windows could probably be added if somebody
 *	wanted to mess with it.
 *
 */

m_addch(c)
register chtype		c;
{
	register int		x, y;
	char *uctr;
	register char rawc = c & A_CHARTEXT;

#ifdef DEBUG
	if (outf) fprintf(outf, "m_addch: [(%d,%d)] ", stdscr->_cury, stdscr->_curx);
#endif
	x = stdscr->_curx;
	y = stdscr->_cury;
# ifdef DEBUG
	if (c == rawc)
		if(outf) fprintf(outf, "'%c'", rawc);
	else
		if(outf) fprintf(outf, "'%c' %o, raw %o", c, c, rawc);
# endif
	if (y >= stdscr->_maxy || x >= stdscr->_maxx || y < 0 || x < 0) {
		return ERR;
	}
	switch (rawc) {
	  case '\t':
	  {
		register int newx;

		for (newx = x + (8 - (x & 07)); x < newx; x++)
			if (m_addch(' ') == ERR)
				return ERR;
		return OK;
	  }
	  default:
		if (rawc < ' ' || rawc > '~') {
			uctr = unctrl(rawc);
			if (m_addch((chtype)uctr[0]|(c&A_ATTRIBUTES)) == ERR)
			    return ERR;
			return m_addch((chtype)uctr[1]|(c&A_ATTRIBUTES));
		}
		if (stdscr->_attrs) {
#ifdef DEBUG
			if(outf) fprintf(outf, "(attrs %o, %o=>%o)", stdscr->_attrs, c, c | stdscr->_attrs);
#endif
			c |= stdscr->_attrs;;
		}
		/* This line actually puts it out. */
		SP->virt_x++;
		*(SP->curptr++) = c;
		x++;
		if (x >= stdscr->_maxx) {
			x = 0;
new_line:
			if (++y >= stdscr->_maxy)
				if (stdscr->_scroll) {
					_ll_refresh(stdscr->_use_idl);
					stdscr->_need_idl = TRUE;
					_scrdown();
					--y;
				}
				else {
# ifdef DEBUG
					int i;
					if(outf) fprintf(outf,
					    "ERR because (%d,%d) > (%d,%d)\n",
					    x, y, stdscr->_maxx, stdscr->_maxy);
					if(outf) fprintf(outf, "line: '");
					if(outf) for (i=0; i<stdscr->_maxy; i++)
						fprintf(outf, "%c", stdscr->_y[y-1][i]);
					if(outf) fprintf(outf, "'\n");
# endif
					return ERR;
				}
			_ll_move(y, x);
		}
# ifdef FULLDEBUG
		if(outf) fprintf(outf, "ADDCH: 2: y = %d, x = %d, firstch = %d, lastch = %d\n", y, x, stdscr->_firstch[y], stdscr->_lastch[y]);
# endif
		break;
	  case '\n':
# ifdef DEBUG
		if (outf) fprintf(outf, "newline, y %d, lengths %d->%d, %d->%d, %d->%d\n", y, y, SP->std_body[y]->length, y+1, SP->std_body[y+1]->length, y+2, SP->std_body[y+2]->length);
# endif
		if (SP->std_body[y+1])
			SP->std_body[y+1]->length = x;
		x = 0;
		goto new_line;
	  case '\r':
		x = 0;
		break;
	  case '\b':
		if (--x < 0)
			x = 0;
		break;
	}
	stdscr->_curx = x;
	stdscr->_cury = y;
#ifdef DEBUG
	if (outf) fprintf(outf, " => (%d,%d)]\n", stdscr->_cury, stdscr->_curx);
#endif
	return OK;
}
