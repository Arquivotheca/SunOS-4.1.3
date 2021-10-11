/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)trm.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

#include "curses.ext"

extern _outch();
char *tparm();

/*
 * Set the virtual insert/replacement mode to new.
 */
_insmode (new)
int new;
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_insmode(%d).\n", new);
#endif
	SP->virt_irm = new;
}

/*
 * Output the string to get us in the right highlight mode,
 * no matter what mode we are currently in.
 */
_forcehl()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_forcehl().\n");
#endif
	SP->phys_gr = -1;
	_sethl();
}

_clearhl ()
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_clearhl().\n");
#endif
	if (SP->phys_gr) {
		register oldes = SP->virt_gr;
		SP->virt_gr = 0;
		_sethl ();
		SP->virt_gr = oldes;
	}
}

_inslines (n)
{
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "_inslines(%d).\n", n);
#endif
	if (!move_standout_mode && SP->phys_gr)
		_clearhl();

	if (SP->phys_y + n >= lines && clr_eos) {
		/*
		 * Really quick -- clear to end of screen.
		 */
		tputs(clr_eos, lines-SP->phys_y, _outch);
		return;
	}

	/*
	 * We are about to shove something off the bottom of the screen.
	 * Terminals with extra memory keep this around, and it might
	 * show up again to haunt us later if we do a delete line or
	 * scroll into it, or when we exit.  So we clobber it now.  We
	 * might get better performance by somehow remembering that this
	 * stuff is down there and worrying about it if/when we have to.
	 * In particular, having to do this extra pair of SP->curptr motions
	 * is a lose.
	 */
	if (memory_below && SP->phys_bot_mgn == lines-1) {
		int save = SP->phys_y,
			savetm = SP->phys_top_mgn,
			savebm=SP->phys_bot_mgn;

		if (save_cursor && restore_cursor)
			tputs(save_cursor, 1, _outch);
		if (clr_eos) {
			_pos(lines-n, 0);
			tputs(clr_eos, n, _outch);
		} else {
			for (i=0; i<n; i++) {
				_pos(lines-n+i, 0);
				_clreol();
			}
		}
		if (save_cursor && restore_cursor)
			tputs(restore_cursor, 1, _outch);
		else
			_pos(save, 0);
		/* _pos might clobber scrolling region, put it back */
		if (savetm!=SP->phys_top_mgn || savebm!=SP->phys_bot_mgn) {
			_window(savetm, savebm, 0, columns-1);
			_setwind();
		}
	}

	/* Do the physical line deletion */
	if ((scroll_reverse || (parm_rindex && !memory_above)) &&
	    SP->phys_y == SP->des_top_mgn /* &&costSR<costAL */) {
		/*
		 * Use reverse scroll mode of the terminal, at
		 * the top of the window.  Reverse linefeed works
		 * too, since we only use it from top line of window.
		 */
		_setwind();
		if (parm_rindex && (n > 3 || !scroll_reverse) &&
		    !memory_above) {
			_pos(SP->phys_y, 0);
			tputs(tparm(parm_rindex, n), n, _outch);
			SP->ml_above -= n;
			if (SP->ml_above < 0)
				SP->ml_above = 0;
		} else
			for (i = n; i > 0; i--) {
				_pos(SP->phys_y, 0);
				tputs(scroll_reverse, 1, _outch);
				if (SP->ml_above > 0)
					SP->ml_above--;
				/*
				 * If we are at the top of the screen, and the
				 * terminal retains display above, then we
				 * should try to clear to end of line.
				 * Have to use CE since we don't remember what
				 * is actually on the line.
				 */
				if (clr_eol && memory_above)
					tputs(clr_eol, 1, _outch);
			}
	} else if (parm_insert_line && (n>1 || !insert_line)) {
		tputs(tparm(parm_insert_line, n), lines-SP->phys_y, _outch);
	} else if (change_scroll_region && !(insert_line) &&
		   save_cursor && restore_cursor &&
		   (scroll_reverse || parm_rindex)) {
		/* vt100 change scrolling region to fake AL */
		tputs(save_cursor, 1, _outch);
		tputs(	tparm(change_scroll_region,
			SP->phys_y, SP->des_bot_mgn), 1, _outch);
		/* change_scroll_region homes stupid cursor */
		tputs(restore_cursor, 1, _outch);
		if (parm_rindex && (n > 3 || !scroll_reverse))
			tputs(tparm(parm_rindex, n), n, _outch);
		else
			for (i=n; i>0; i--)	/* should do @'s */
				tputs(scroll_reverse, 1, _outch);
		/* restore scrolling region */
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);/* Once again put it back */
		SP->phys_top_mgn = SP->des_top_mgn;
		SP->phys_bot_mgn = SP->des_bot_mgn;
	} else {
		tputs(insert_line, lines - SP->phys_y, _outch);
		for (i = n - 1; i > 0; i--) {
			tputs(insert_line, lines - SP->phys_y, _outch);
		}
	}
}

_dellines (n)
{
	register int i;

#ifdef DEBUG
	if(outf) fprintf(outf, "_dellines(%d).\n", n);
#endif
	if (lines - SP->phys_y <= n && (clr_eol && n == 1 || clr_eos)) {
		tputs(clr_eos, n, _outch);
	} else
	if ((scroll_forward || parm_index) &&
	    SP->phys_y == SP->des_top_mgn /* &&costSF<costDL */) {
		/*
		 * Use forward scroll mode of the terminal, at
		 * the bottom of the window.  Linefeed works
		 * too, since we only use it from the bottom line.
		 */
		_setwind();
		if (parm_index && (n > 3 || !scroll_forward)) {
			_pos(SP->des_bot_mgn, 0);
			tputs(tparm(parm_index, n), n, _outch);
		} else	
			for (i = n; i > 0; i--) {
				_pos(SP->des_bot_mgn, 0);
				tputs(scroll_forward, 1, _outch);
			}
		SP->ml_above += n;
		_pos(SP->des_top_mgn, 0);
		if (SP->ml_above + lines > lines_of_memory)
			SP->ml_above = lines_of_memory - lines;
	} else if (parm_delete_line && (n>1 || !(delete_line))) {
		tputs(tparm(parm_delete_line, n), lines-SP->phys_y, _outch);
	}
	else if (change_scroll_region && !(delete_line) &&
		 save_cursor && restore_cursor &&
		 (scroll_forward || parm_index)) {
		/* vt100: fake delete_line by changing scrolling region */
		/* Save since change_scroll_region homes stupid cursor */
		tputs(save_cursor, 1, _outch);
		tputs(tparm(change_scroll_region,
			SP->phys_y, SP->des_bot_mgn), 1, _outch);
		/* go to bottom left corner.. */
		tputs(tparm(cursor_address, SP->des_bot_mgn, 0), 1, _outch);
		if (parm_index && (n > 3 || !scroll_forward))
			tputs(tparm(parm_index, n), n, _outch);
		else
			for (i=0; i<n; i++)	/* .. and scroll n times */
				tputs(scroll_forward, 1, _outch);
		/* restore scrolling region */
		tputs(tparm(change_scroll_region,
			SP->des_top_mgn, SP->des_bot_mgn), 1, _outch);
		tputs(restore_cursor, 1, _outch);	/* put SP->curptr back */
		SP->phys_top_mgn = SP->des_top_mgn;
		SP->phys_bot_mgn = SP->des_bot_mgn;
	}
	else {
		for (i = 0; i < n; i++)
			tputs(delete_line, lines-SP->phys_y, _outch);
	}
}

/*
 * Scroll the terminal forward n lines, bringing up blank lines from bottom.
 * This only affects the current scrolling region.
 */
_scrollf(n)
int n;
{
	register int i;

	if (scroll_forward || parm_index) {
		_setwind();
		_pos(SP->des_bot_mgn, 0);
		if (parm_index && (n > 3 || !scroll_forward))
			tputs(tparm(parm_index, n), n, _outch);
		else
			for (i=0; i<n; i++)
				tputs(scroll_forward, 1, _outch);
		SP->ml_above += n;
		if (SP->ml_above + lines > lines_of_memory)
			SP->ml_above = lines_of_memory - lines;
	} else {
		/* Braindamaged terminal can't do it, try delete line. */
		_pos(0, 0);
		_dellines(n);
		if (memory_below) {
			_pos(lines-1, 0);	/* go back to the bottom */
			_inslines(n);		/* to add blank lines */
		}
	}
}

/*
 * writechars: write the characters from *start through *end out to the
 * terminal.  Keep track of the cursor position.  Do anything terminal
 * specific that is necessary to get the job done right or more efficiently.
 */
_writechars (start, end, leftch)
register char	*start, *end;
chtype	leftch;
{
	register int c;
	register char *p;
	register int clrhlflag = 0;
	int savetm, savebm = SP->phys_bot_mgn;
	char *beginning = start;
	extern int didntdobotright;

#ifdef DEBUG
	if(outf) {
		fprintf(outf, "_writechars(%d:'", end-start+1);
		fwrite(start, sizeof (char), end-start+1, outf);
		fprintf(outf, "', leftch=0%o).\n", leftch);
	}
#endif
	_setmode ();
	_sethl();

	while (start <= end) {
#ifdef FULLDEBUG
		if(outf) fprintf(outf, "wc loop: repeat_char '%s', SP->phys_irm %d, *start '%c'\n", repeat_char, SP->phys_irm, *start);
#endif
		if (repeat_char && SP->phys_irm != 1 &&
			((p=start+1),*start==*p++) && (*start==*p++) &&
			(*start==*p++) && (*start==*p++) && p<=end) {
			/* We have a run of at least 5 characters */
			c = 5;
			while (p <= end && *start == *p)
				p++, c++;
			SP->phys_x += c;
			/* Do not assume anything about how repeat and auto
			 * margins interact.  The concept botches it.
			 */
			if (auto_right_margin) {
				int overshot = (SP->phys_x+1) - columns;
				if (overshot > 0) {
					c -= overshot;
					p -= overshot;
					SP->phys_x -= overshot;
				}
			}
#ifdef DEBUG
			if(outf) fprintf(outf, "using repeat, count %d, char '%c'\n", c, *start);
#endif
			if (*start == '~' && tilde_glitch)
				*start = '`';
			tputs(tparm(repeat_char, *start, c), c, _outch);
			start = p;
			continue;
		}
		c = *start++;
		if (c == '~' && tilde_glitch)
			c = '`';
#ifdef DEBUG
		if (outf) fprintf(outf, "c is '%c', phys_x %d, phys_y %d\n", c, SP->phys_x, SP->phys_y);
#endif
		if(SP->phys_irm == 1 && insert_character)
			tputs(insert_character, columns-SP->phys_x, _outch);
		/*
		 * If transparent_underline && !erase_overstrike,
		 * should probably do clr_eol.  No such terminal yet.
		 * We do clr_eol at the end of the line to avoid
		 * wrapping.
		 */
		if (transparent_underline && erase_overstrike &&
		   c == '_' && SP->phys_irm != 1)
			if (SP->phys_x >= columns - 1) 
		   		tputs(clr_eol, 1, _outch);
		   	else {
				_outch (' ');
				tputs(cursor_left, 1, _outch);
		    	}

		if (++SP->phys_x >= columns) {
			/*
			 * The bottom right corner of a scrolling
			 * region is just as bad as the bottom
			 * right corner of the screen. We can at 
			 * least control the scrolling region.
			 */
			if (SP->phys_y>=SP->phys_bot_mgn) {
				savetm = SP->phys_top_mgn;
				_window(0, lines-1, 0, columns-1);
				_setwind();
			}
			if (auto_right_margin) {
				if (SP->phys_gr && !move_standout_mode)
					clrhlflag = 1;

							  /* Have to on c100 anyway..*/
				if (SP->phys_y >= lines-1) { /*&& !eat_newline_glitch*/
					/*
					 * We attempted to put something
					 * in the last position of the
					 * last line.  Since this will
					 * cause a scroll (we only get
					 * here if the terminal has
					 * auto_right_margin) we refuse
					 * to put it out.
					 *
					 * Well, on second thought, if we
					 * back up a character, we can
					 * type the character, back up
					 * again, and re-INSERT the
					 * character that used to be
					 * there before.
					 *
					 * This algorithm is from Tony Hansen
					 * and Mitch Baker.
					 */
#ifdef DEBUG
					if (outf) fprintf(outf, "Avoiding lower right corner\n");
#endif
					/*
					 * For now, only do this if the
					 * terminal has cursor_left. If we
					 * want to be complete, we should
					 * probably use _pos() to do the
					 * moving around.
					 */
					if (cursor_left && (enter_insert_mode || insert_character)) {
						register int leftc, left_attrs, curr_attrs;
						/* Make sure that we can move */
						if (SP->phys_irm == 1 && !move_insert_mode) {
							_insmode(0);
							_setmode();
						}
						curr_attrs = SP->phys_gr;

						/* Find the character to the left of us. */
						/* If we just wrote out the character, use that. */
						if (beginning < (start-1)) {
							leftc = *(start - 2);
							left_attrs = curr_attrs;
						/* Otherwise, use the one passed in. */
						} else {
							left_attrs = leftch & A_ATTRIBUTES;
							leftc = leftch & A_CHARTEXT;
						}
						if (leftc == '~' && tilde_glitch)
							leftc = '`';

						/* rewrite the current char in the 79th column */
						if ((leftc != c) || (left_attrs != curr_attrs)) {
							if (curr_attrs && !move_standout_mode) {
								_hlmode(0);
								_sethl();
							}
							tputs(cursor_left, 1, _outch);
							/* Place us back in the proper hilite. */
							if (SP->phys_gr != curr_attrs) {
								_hlmode(curr_attrs);
								_sethl();
							}
							_outch((chtype) c);
						}

						/* Move again to column 79. */
						if (curr_attrs && !move_standout_mode) {
							_hlmode(0);
							_sethl();
						}
						tputs(cursor_left, 1, _outch);
						SP->phys_x--;
						/* Restore the highlighting. */
						if (left_attrs != SP->phys_gr) {
							_hlmode(left_attrs);
							_sethl();
						}

						/* Restore the 79th character. */
						if (enter_insert_mode) {
							if (SP->phys_irm == 0) {
								tputs(enter_insert_mode, 1, _outch);
								_outch((chtype) leftc);
								if (insert_padding)
									tputs(insert_padding, 1, _outch);
								tputs(exit_insert_mode, 1, _outch);
							} else {
								_outch((chtype) leftc);
								if (insert_padding)
									tputs(insert_padding, 1, _outch);
							}
						} else {
							tputs(insert_character, 1, _outch);
							_outch((chtype) leftc);
						}
					} else {			/* could not output char */
						didntdobotright = 1;
						SP->phys_x--;
						break;
					}
				} else {				/* not on bottom right corner */
					SP->phys_x = 0;
					SP->phys_y++;
					_outch((chtype) c);
				}
			} else {	/* No auto right margin */
				_outch((chtype) c);
				SP->phys_x = columns-1;
			}
		} else							/* not on right margin */
			_outch((chtype) c);

		/* Only 1 line can be affected by insert char here */
		if(SP->phys_irm == 1 && insert_padding)
			tputs(insert_padding, 1, _outch);
	}
	if (eat_newline_glitch && SP->phys_x == 0) {
		/*
		 * This handles both C100 and VT100, which are
		 * different.  We do not output carriage_return
		 * and cursor_down because it might confuse a
		 * terminal that is looking for return and linefeed.
		 */
		_outch('\r');
		_outch('\n');
	}
	/* restore the scrolling region */
	if (savebm != SP->phys_bot_mgn) {
		_window(savetm, savebm, 0, columns-1);
		_setwind();
	}
	if (clrhlflag) {
		/* Auto margin counts as motion on an HP */
		_hlmode(0);
		_sethl();
	}
}

/*
 * Output n blanks, or the equivalent.  This is done to erase text, and
 * also to insert blanks.  The semantics of this call do not define where
 * it leaves the cursor - it might be where it was before, or it might
 * be at the end of the blanks.  We will, of course, leave SP->phys_x
 * properly updated.
 */
_blanks (n)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_blanks(%d).\n", n);
#endif
	if (n == 0)
		return;
	_setmode ();
	_sethl ();
	if (SP->virt_irm==1 && parm_ich) {
		if (n == 1 && insert_character)
			tputs(insert_character, 1, _outch);
		else if (parm_ich)
			tputs(tparm(parm_ich, n), n, _outch);
		return;
	}
	if (erase_chars && SP->phys_irm != 1 && n > 5) {
		tputs(tparm(erase_chars, n), n, _outch);
		return;
	}
	if (repeat_char && SP->phys_irm != 1 && n > 5) {
		tputs(tparm(repeat_char, ' ', n), n, _outch);
		SP->phys_x += n;
		return;
	}
	while (--n >= 0) {
		if (SP->phys_irm == 1 && insert_character)
			tputs (insert_character, columns - SP->phys_x, _outch);
		if (++SP->phys_x >= columns && auto_right_margin) {
			if (SP->phys_y >= lines-1) {
				/*
				 * We attempted to put something in the last
				 * position of the last line.  Since this will
				 * cause a scroll (we only get here if the
				 * terminal has auto_right_margin) we refuse
				 * to put it out.
				 */
				SP->phys_x--;
				return;
			}
			SP->phys_x = 0;
			SP->phys_y++;
		}
		_outch (' ');
		if (SP->phys_irm == 1 && insert_padding)
			tputs (insert_padding, 1, _outch);
	}
}

_clrline(ln, p, olen)
int ln;
register chtype *p;
register int olen;
{
	register int firstnonblank;

#ifdef DEBUG
	if (outf) fprintf(outf, "_clrline(ln=%d,p=%x,olen=%d).\n", ln, p, olen);
#endif /* DEBUG */
	for (firstnonblank = 0; firstnonblank < olen; firstnonblank++, p++)
		if (*p != ' ')
			break;

	if ((magic_cookie_glitch >= 0) && (firstnonblank > 0) &&
	    (*p&A_ATTRIBUTES))
		firstnonblank -= magic_cookie_glitch;

	/* Avoid an extra _pos if possible. */
	/* Warning: This makes assumptions about how _clreol() */
	/* works. If that changes, this may have to change also. */
	if (!clr_eol && delete_line && insert_line)
		_pos(ln, 0);
	else
		_pos(ln, firstnonblank);
	_clreol();
}

/*
 * Perform clear to beginning of line. This routine is used
 * within insdelchar.c. Return where the first nonblank charcter is.
 * Note that (firstnonblank > firstdiff) for this to be useful.
 */

int
_clrbol(firstnonblank, firstdiff, ln, ob)
int firstnonblank, firstdiff, ln;
chtype *ob;
{
	register int cost_blanks, cost_clrbol, cost_ichdch, cost_erase;
#ifdef DEBUG
	if (outf) fprintf(outf, "_clrbol(firstnonblank=%d,firstdiff=%d,ln=%d,ob=%x). ", firstnonblank, firstdiff, ln, ob);
#endif /* DEBUG */
	if ((magic_cookie_glitch > 0) && (ob[firstnonblank]&A_ATTRIBUTES))
		firstnonblank -= magic_cookie_glitch;

	/* cost to overwrite with blanks */
	cost_blanks = firstnonblank - firstdiff;

	if (cost_blanks <= 4) {		/* arbitrary cutoff to save CPU */
#ifdef DEBUG
	if (outf) fprintf(outf, "too close to bother, returning firstdiff.\n");
#endif /* DEBUG */
		return firstdiff;
	}

	/* cost of clr_bol + cost to write blank */
	if (clr_bol && cursor_right)
		cost_clrbol = _cost(Clr_bol) +
			(SP->phys_irm || SP->phys_gr) ?
				_cost(Cursor_right) : 1;
	else
		cost_clrbol = INFINITY;

	/* cost to delete n chars, then insert n blanks */
	if (parm_dch && parm_ich)
		cost_ichdch = _cost(dcfixed) + _cost(dcvar) * cost_blanks +
			_cost(icfixed) + (_cost(icvar) * cost_blanks);
	else
		cost_ichdch = INFINITY;

	/* cost to erase chars */
	if (erase_chars)
		cost_erase = _cost(Erase_chars);
	else
		cost_erase = INFINITY;

	if (cost_clrbol < cost_blanks &&
	    cost_clrbol < cost_ichdch &&
	    cost_clrbol < cost_erase) {
#ifdef DEBUG
		if (outf) fprintf(outf, "use clr_bol, return firstnonblank.\n");
#endif /* DEBUG */
		_pos(ln, firstnonblank-1);
		/*
		 * There are two types of clr_bol: inclusive and
		 * exclusive. We assume that it is inclusive, which
		 * is the ANSI standard.
		 * If it were inclusive, then the blank will be
		 * blanked twice.
		 */
		tputs(clr_bol, 1, _outch);
		if (SP->phys_irm || SP->phys_gr)
			tputs(cursor_right, 1, _outch);
		else
			_outch(' ');
		SP->phys_x++;
		return firstnonblank;
	} else if (cost_ichdch < cost_blanks &&
		   cost_ichdch < cost_erase) {
#ifdef DEBUG
		if (outf) fprintf(outf, "use i/d, return firstnonblank.\n");
#endif /* DEBUG */
	        _pos(ln, firstdiff);
		_delchars(cost_blanks);
		_insmode(1);
		_blanks(cost_blanks);
		_insmode(0);
		_setmode();
		return firstnonblank;
	} else if (cost_erase < cost_blanks) {
#ifdef DEBUG
	if (outf) fprintf (outf, "using erase_chars, return firstnonblank.\n");
#endif /* DEBUG */
		_pos(ln, firstdiff);
		tputs(tparm(erase_chars, firstnonblank - firstdiff), 1, _outch);
		return firstnonblank;
	}
#ifdef DEBUG
	if (outf) fprintf(outf, "not cost effective, returning firstdiff.\n");
#endif /* DEBUG */
	return firstdiff;
}

_clreol()
{
	register int col;
	register struct line *cb;

#ifdef DEBUG
	if(outf) fprintf(outf, "_clreol().\n");
#endif
	if (!move_standout_mode)
		_clearhl ();
	if (clr_eol) {
		tputs (clr_eol, columns-SP->phys_x, _outch);
	} else if (delete_line && insert_line && SP->phys_x == 0) {
		_dellines (1);
		_inslines (1);
	} else {
		/*
		 * Should consider using delete/insert line here, too,
		 * if the part that would have to be redrawn is short,
		 * or if we have to get rid of some cemented highlights.
		 * This might win on the DTC 382 or the Teleray 1061.
		 */
		cb = SP->cur_body[SP->phys_y+1];
		for (col=SP->phys_x; col<cb->length; col++)
			if (cb->body[col] != ' ') {
				_pos(SP->phys_y, col);
				_blanks(1);
			}
		/*
		 * Could save and restore SP->curptr position, but since we
		 * keep track of what _blanks does to it, it turns out
		 * that this is wasted.  Not backspacing over the stuff
		 * cleared out is a real win on a super dumb terminal.
		 */
	}
}

/*
 * Insert n blank characters.
 */
_inschars(n)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_inschars(%d).\n", n);
#endif
	if (enter_insert_mode && SP->phys_irm == 0) {
		tputs(enter_insert_mode, 1, _outch);
		SP->phys_irm = 1;
	}
	if (parm_ich && n > 1)
		tputs(tparm(parm_ich, n), n, _outch);
	else if (insert_character)
		while (--n >= 0)
			tputs(insert_character, 1, _outch);
}

/*
 * Delete n characters.
 */
_delchars (n)
{
#ifdef DEBUG
	if(outf) fprintf(outf, "_delchars(%d).\n", n);
#endif
	if (enter_delete_mode) {
		if (strcmp(enter_delete_mode, enter_insert_mode) == 0) {
			if (SP->phys_irm == 0) {
				tputs(enter_delete_mode,1,_outch);
				SP->phys_irm = 1;
			}
		}
		else {
			if (SP->phys_irm == 1) {
				tputs(exit_insert_mode,1,_outch);
				SP->phys_irm = 0;
			}
			tputs(enter_delete_mode,1,_outch);
		}
	}
	if (parm_dch && (n > 1 || !delete_character))
		tputs(tparm(parm_dch, n), 1, _outch);
	else
		while (--n >= 0) {
			/* Only one line can be affected by our delete char */
			tputs(delete_character, 1, _outch);
		}
	if (exit_delete_mode) {
		if (strcmp(exit_delete_mode, exit_insert_mode) == 0)
			SP->phys_irm = 0;
		else
			tputs(exit_delete_mode, 1, _outch);
	}
}
