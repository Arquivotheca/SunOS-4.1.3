/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)ll_refresh.c 1.1 92/07/30 SMI"; /* from S5R3 1.4.1.2 */
#endif

#include "curses.ext"

extern int InputPending;
extern int outchcount;

int didntdobotright;	/* writechars didn't output char in bot right corner */
static int ll_first, ll_last, ll_clear;	/* bounds of area touched */
struct line *_line_alloc();

/*
 * Optimally make the screen (which currently looks like SP->cur_body)
 * look like SP->std_body.  If use_idl is 1, this routine will call
 * out all its horses, including some code to figure out
 * how to use insert/delete line.  If use_idl is 0, or if the terminal
 * does not have insert/delete line, a simpler scheme will
 * be used, and the insert/delete line features of the terminal will not
 * be used.
 *
 * While the original intent of this parameter was speed (insert/delete
 * line was slow) the parameter currently is there to avoid annoying
 * the user with unnecessary insert/delete lines.
 */
int
_ll_refresh (use_idl)
{
	register int n;

#ifdef DEBUG
	if(outf) {
		fprintf(outf, "_ll_refresh(%d)\n", use_idl);
		fprintf(outf, "phys cursor at (%d,%d), want it at (%d,%d)\n",
			SP->phys_y, SP->phys_x, SP->virt_y, SP->virt_x);
	}
#endif

	if (_ta_check())
		return 0;

	outchcount = 0;

	/* update soft labels */
	{
		register struct slkdata *SLK = SP->slk;
		register int i;
		extern int _outch();
		int dooutput;

		if (SLK && (SLK->fl_changed || SP->doclear)) {
			dooutput = SLK->window == NULL;
			if (SP->doclear)
				for (i = 0; i < 8; i++) {
					if (dooutput)
						tputs(tparm(plab_norm, i+1, SLK->label[i]), 1, _outch);
					SLK->changed[i] = FALSE;
					strcpy(SLK->scrlabel[i], SLK->label[i]);
				}
			else
				for (i = 0; i < 8; i++)
					if (SLK->changed[i]) {
						if (strcmp(SLK->label[i], SLK->scrlabel[i]) != 0) {
							if (dooutput)
								tputs(tparm(plab_norm, i+1, SLK->label[i]), 1,_outch);
							strcpy(SLK->scrlabel[i], SLK->label[i]);
						}
						SLK->changed[i] = FALSE;
					}
			tputs(label_on, 1, _outch);
			SLK->fl_changed = FALSE;
		}
	}

#ifdef DEBUG
	if (outf) fprintf(outf, "virt cursor at y=%d, x=%d\n",
		SP->virt_y, SP->virt_x);
#endif

	if (SP->doclear) {
#ifdef DEBUG
		if(outf) fprintf(outf, "SP->doclear, clearing screen\n");
#endif
		_reset ();
		SP->doclear = 0;
		for (n = 1; n <= lines; n++) {
			if (SP->cur_body[n] != SP->std_body[n])
				_line_free (SP->cur_body[n]);
			SP->cur_body[n] = 0;
		}
		ll_first = 1;
		ll_last = lines;
	} else if (!SP->fl_changed ||
		 (_find_bounds(), ll_first > lines)) {
		if (!InputPending && SP->virt_x >= 0 && SP->virt_y >= 0)
			_pos(SP->virt_y, SP->virt_x);
		__cflush();
		return outchcount;
	}

	_trim_trailing_blanks();
	if (magic_cookie_glitch > 0)
		_toss_cookies();
#ifdef DEBUG
	_refr_dump(use_idl);
#endif
	_check_clreos();
	SP->check_input = SP->baud / 2400;

	/* Choose between two updating algorithms. */
	if (ll_first < ll_last && use_idl && _cost(ilfixed) < INFINITY) {
#ifdef DEBUG
		if(outf) fprintf(outf, "use_idl\n");
#endif
		_fix_hash();
		_chk_scroll();
		_sl_upd();
	} else
		_f_upd();

	if (ll_clear > 0) {
		_pos(ll_clear-1, 0);
		tputs(clr_eos, 1, _outch);
	}

	if (didntdobotright)
		_fix_bot_right();

	if (!ceol_standout_glitch) {
		_hlmode(0);
		_sethl();
	}
#ifdef DEBUG
	if(outf) fprintf(outf, "at end, phys SP->curptr at (%d,%d), want SP->curptr at (%d,%d)\n",
		SP->phys_y, SP->phys_x, SP->virt_y, SP->virt_x);
#endif
	if (!InputPending) {
		if (SP->virt_x >= 0 && SP->virt_y >= 0)
			_pos (SP->virt_y, SP->virt_x);
		SP->fl_changed = FALSE;
	}
	__cflush();
#ifdef DEBUG
	if(outf) {
		fprintf(outf, "end of _ll_refresh, InputPending %d\n", InputPending);
		fflush(outf);
	}
#endif
	return outchcount;
}

/*
 * Find the topmost and bottommost line that has been touched since
 * the last refresh.  After this, we need look no further than these
 * bounds.  When only one line has been touched, this can save a good
 * deal of CPU time.  Results are stored in globals ll_first and ll_last;
 */
static
_find_bounds()
{
	register struct line **std_body = SP->std_body;
	register struct line **cur_body = SP->cur_body;
	register struct line *dln;
	register int first = 1, last = lines;

	for ( ; first <= last ; first++) {
		dln = std_body[first];
		if (dln && dln != cur_body[first])
			break;
	}

	for ( ; last > first ; last--) {
		dln = std_body[last];
		if (dln && dln != cur_body[last])
			break;
	}

	ll_first = first;
	ll_last = last;
#ifdef DEBUG
	if (outf)
		fprintf(outf, "find_bounds finds first %d last %d\n",
			first, last);
#endif
}

/*
 * If there is typeahead waiting, don't refresh now.
 */
static
_ta_check()
{
	if (SP->check_fd >= 0)
		InputPending = _chk_input();
	else
		InputPending = 0;
	if (InputPending) {
#ifdef DEBUG
		if (outf) fprintf(outf, "InputPending %d, aborted ll_refresh at start\n", InputPending);
#endif
		return 1;
	}

#ifdef		NONSTANDARD
	input_wait();
#endif		/* NONSTANDARD */
	return 0;
}

static
_trim_trailing_blanks()
{
	register chtype *p;
	register int i, n;
	register struct line *dln, *pln;
	register int r_last = ll_last;
	register struct line **std_body = SP->std_body;
	register struct line **cur_body = SP->cur_body;

	/* Get rid of trailing blanks on all lines */
	for (n = ll_first; n <= r_last; n++) {
		dln = std_body[n];
		pln = cur_body[n];
		if (dln && dln != pln) {
			if (i = dln->length) {
				p = dln->body + i;
				while (*--p == ' ' && --i > 0)
					;
				dln->length = i;
			}
		}

		if (pln) {
			register chtype *p;

			if (i = pln->length) {
				p = pln->body + i;
				while (*--p == ' ' && --i > 0)
					;
				pln->length = i;
			}
		}
	}
}

/*
 * Check if we can use clr_eos. If so, set ll_clear to line
 * to do clr_eos from and ll_last 1 less than that.
 */
_check_clreos()
{
	register int r_bot, r_last, r_first;
	register struct line **std_body = SP->std_body;
	register int i;

	ll_clear = 0;
	if ((std_body[ll_last] && std_body[ll_last]->length != 0) || !clr_eos)
		return;
	/* check if everything below ll_last is blank */
	for (r_bot = lines, r_last = ll_last + 1; r_last <= r_bot; r_last++)
		if (std_body[r_last] && std_body[r_last]->length != 0)
			return;
	/* check how much of ll_first..ll_last is blank */
	r_last = ll_last - 1;	/* checked ll_last above */
	r_first = ll_first;
	for ( ; r_last >= r_first; r_last--)
		if (std_body[r_last] && std_body[r_last]->length != 0)
			break;
	/* save info for later and clear out cur_body */
	ll_clear = r_last + 1;
	for (i = ll_last; i > r_last; i--) {
		if (SP->cur_body[i] != SP->std_body[i])
			_line_free (SP->cur_body[i]);
		SP->cur_body[i] = SP->std_body[i];
	}
	ll_last = r_last;
}

/*
 * Make sure all the hash functions are the same.
 */
static
_fix_hash()
{
	register int n;
	register int r_last = ll_last;

	for (n = ll_first; n <= r_last; n++) {
		if (SP->cur_body[n] == 0)
			SP->cur_body[n] = _line_alloc();
		if (SP->std_body[n] == 0)
			SP->std_body[n] = SP->cur_body[n];
		else
			_comphash (SP->std_body[n]);
		_comphash (SP->cur_body[n]);
	}
}

/*
 * Count number of matches if we scroll 1 line and if we
 * don't scroll at all.  This is primarily useful for the
 * case where we scroll the whole screen.  Scrolling a portion
 * of the screen will be handled by the ins/del line routines,
 * although a special case here might buy some CPU speed.
 */
static
_chk_scroll()
{
	register int i, j, n;

	if (ll_first > 1 || ll_last < lines)
		return;		/* Not full screen change, no scroll */

	for (i=1,n=0,j=0; i<lines; i++) {
		if (SP->cur_body[i+1]->hash == SP->std_body[i]->hash)
			n++;
		if (SP->cur_body[i]->hash == SP->std_body[i]->hash)
			j++;
	}
	if (n > lines-3 && n > j) {
		_window(0, lines-1, 0, columns-1);
		_scrollf(1);
		_line_free(SP->cur_body[1]);
		for (i=1; i<lines; i++) {
			/* Copy line with two references since they
			 * are no longer the same row on screen. */
			if (SP->cur_body[i+1] == SP->std_body[i+1]) {
				struct line *p;
				int l;
				chtype *b1, *b2;
				p = _line_alloc ();
				p->length = l = SP->cur_body[i+1]->length;
				p->hash = SP->cur_body[i+1]->hash;
				b1 = &(p->body[0]);
				b2 = &(SP->cur_body[i+1]->body[0]);
				memcpy((char *)b1, (char *)b2,
					l*(int)sizeof(chtype));
				SP->std_body[i+1] = p;
			}
			SP->cur_body[i] = SP->cur_body[i+1];
		}
		SP->cur_body[lines] = _line_alloc();
	}
}

/*
 * Slower update, considering insert/delete line.
 */
static
_sl_upd()
{
	register int i, j;
	register int r_last = ll_last;

	i = ll_first;
	/*
	 * Break the screen (from ll_first to ll_last) into clumps of
	 * lines that are different.  Thus we ignore the ones that
	 * are identical.
	 */
	for (;;) {
		while (i<=r_last && SP->cur_body[i]==SP->std_body[i])
			i++;
		if (i > r_last)
			break;
		for (j=i; j <= r_last &&
			  SP->cur_body[j] != SP->std_body[j]; j++)
			;
		j--;
#ifdef DEBUG
		if(outf) fprintf(outf, "window from %d to %d\n", i, j);
#endif
		/* i thru j is a window of different lines. */
		if (i == j) {
			_id_char(SP->cur_body[i], SP->std_body[i], i-1);
			if (SP->cur_body[i] != SP->std_body[i])
				_line_free (SP->cur_body[i]);
			SP->cur_body[i] = SP->std_body[i];
		} else {
			_window(i-1, j-1, 0, columns-1);
			_setwind();   /* Force action for moves, etc */
			_id_line(i, j);
		}
		i = j+1;
	}
}

/*
 * Fast update - don't consider insert/delete line.
 */
static
_f_upd()
{
	register int n;
	register int r_last = ll_last;

#ifdef DEBUG
	if(outf) fprintf(outf, "Fast Update, lines %d\n", lines);
#endif
	_window(0, lines-1, 0, columns-1);
	_setwind();   /* Force action for moves, etc */
	for (n = ll_first; n <= r_last; n++)
		if (SP->std_body[n] != SP->cur_body[n]) {
			_id_char (SP->cur_body[n], SP->std_body[n], n-1);
			if (SP->cur_body[n] != SP->std_body[n])
				_line_free (SP->cur_body[n]);
			SP->cur_body[n] = SP->std_body[n];
		}
}

/*
 * Didn't output char in bottom right corner of screen.
 * Remember this fact so that next time when it's higher
 * on the screen, we'll fix it up.
 */
static
_fix_bot_right()
{
	int holdvx, holdvy;
#ifdef DEBUG
	if (outf)
		fprintf(outf,
		"didntdobotright so setting SP->cur_body[%d]->body[%d] from '%c' to space.\n",
		lines, columns-1, SP->cur_body[lines]->body[columns-1]);
#endif

	holdvx = SP->virt_x;
	holdvy = SP->virt_y;
	/*
	 * This code in effect marks the last line dirty
	 * so that the next time it will get fixed.  It also
	 * splits the line back into virt/phys so we don't
	 * clobber the virtual part too.
	 */
	_ll_move(lines-1, columns-1);
	SP->cur_body[lines]->body[columns-1] = ' ';
	didntdobotright = 0;
	/* Now restore the cursor we clobbered. */
	_ll_move(holdvy, holdvx);
}

#ifdef DEBUG
/*
 * Debugging dump of old and new screens.  We only dump the parts that
 * have changed to cut down on the quantity of output.
 */
static
_refr_dump(use_idl)
int use_idl;
{
	register int i, j, n;
	register int r_last = ll_last;

	if (!outf) return;
	fprintf(outf, "what we have:\n");
	fprintf(outf, "pointer : hash,      len:    body\n");
	for (i=ll_first; i<=r_last; i++) {
		fprintf(outf, "%8x: ", SP->cur_body[i]);
		if (SP->cur_body[i] == NULL) {
			fprintf(outf, "()\n");
		} else {
			fprintf(outf, "%8x, ", SP->cur_body[i]->hash);
			fprintf(outf, "%4d:", SP->cur_body[i]->length);
			for (j=0; j<SP->cur_body[i]->length; j++) {
				n = SP->cur_body[i]->body[j];
				if (n & A_ATTRIBUTES)
					putc('\'', outf);
				n &= 0177;
				fprintf(outf, "%c", n>=' ' ? n : '.');
			}
			fprintf(outf, "\n");
		}
	}
	fprintf(outf, "what we want:\n");
	for (i=ll_first; i<=r_last; i++) {
		fprintf(outf, "%8x: ", SP->std_body[i]);
		if (SP->std_body[i] == NULL) {
			fprintf(outf, "()\n");
		} else {
			fprintf(outf, "%8x, ", SP->cur_body[i]->hash);
			fprintf(outf, "%4d:", SP->std_body[i]->length);
			for (j=0; j<SP->std_body[i]->length; j++) {
				n = SP->std_body[i]->body[j];
				if (n & A_ATTRIBUTES)
					putc('\'', outf);
				n &= 0177;
				fprintf(outf, "%c", n>=' ' ? n : '.');
			}
			fprintf(outf, "\n");
		}
	}
	fprintf(outf, "----\n");
	fflush(outf);
}
#endif

/*
 * This routine is only needed on terminals with the "magic cookie"
 * braindamage glitch.  The problem is that the shortsighted designers of
 * these terminals were being cheap.  They didn't allocate 16 bits for
 * each character (7 for the character and 9 for attributes) but instead
 * created some reserved "magic cookie" characters to tell the scan
 * routine "you should change attributes now".  This would be fine except
 * that these cookies take up a space in memory, and usually display as a
 * blank.  This makes it impossible to display what the user really
 * wanted, if he is using attributes for underlining, bold, etc.  It also
 * requires special handling, even if the characters don't take up a
 * space, because the terminals behave differently than terminals
 * with a graphic rendition "mode".
 *
 * One approach to this problem is to make everybody pay the price of
 * this stupidity, forcing the programmer to allocate a blank space
 * when attributes are changed.  This works cleanly but I consider it
 * unacceptable.
 *
 * My approach is to simulate what the programmer (who wasn't thinking
 * about these turkey terminals) wanted as closely as possible.  If there
 * is a desired blank in there, we use that slot.  If not, we shove the
 * rest of the line to the right one space.  (When several attributes
 * are changed on one line, this can result in losing several characters
 * from the right of the line.)
 *
 * This routine looks for places in SP->std_body where shoving to the
 * right is needed, and does the required shoving.
 *
 * Note that the ceol_standout_glitch is even harder to handle than the
 * magic cookie glitch, because there is no reasonable way to get rid
 * of a cookie once it's up on the screen.  And since the cookies don't
 * take up a space, you expect to be able to do better.  That problem
 * is handled elsewhere.
 */
static
_toss_cookies()
{
	register int i, j, len;
	register struct line *dsi;
	register chtype *b;

#ifdef DEBUG
	if(outf) fprintf(outf, "_toss_cookies\n");
#endif
	for (i=1; i<=lines; i++) {
		dsi = SP->std_body[i];
		if (dsi && dsi != SP->cur_body[i]) {
			len = dsi->length;
			b = dsi->body;
			for (j=0; j<len; j++) {
				if (b[j]&A_ATTRIBUTES) {
#ifdef DEBUG
					if(outf) fprintf(outf, "_shove, line %d, char %d, val %o\n", i, j, b[j]);
#endif
					dsi->length = _shove(b, len, i);
					break;
				}
			}
		}
	}
}

/*
 * Shove right in body as much as necessary.
 * Note that we give the space the same attributes as the upcoming
 * character, to force the cookie to be placed on the space.
 */
/* ARGSUSED */
static
_shove(body, len, lno)
register chtype *body;
register int len, lno;
{
	register int j, k, prev = 0;
	register int curscol = SP->virt_x, cursincr = 0, shoved = 0;
	static chtype buf[256];

#ifdef DEBUG
	if(outf) fprintf(outf, "_shove('");
	_prstr(body, len);
	if(outf) fprintf(outf, "', %d, %d), SP->virt_x %d\n", len, lno, SP->virt_x);
#endif
	for (j=0, k=0; j<len; ) {
		if ((body[j]&A_ATTRIBUTES) != prev) {
			shoved++;
			if ((body[j]&A_CHARTEXT) == ' ') {
				/* Using an existing space */
				buf[j] = ' ' | body[j+1]&A_ATTRIBUTES;
			} else if ((body[j-1]&A_CHARTEXT) == ' ') {
				/* Using previous existing space */
				buf[j-1] = ' ' | body[j]&A_ATTRIBUTES;
			} else {
				/* A space is inserted here. */
				buf[k++] = ' ' | body[j]&A_ATTRIBUTES;
				if (j < curscol)
					cursincr++;
			}
		}
#ifdef DEBUG
		if(outf) fprintf(outf, "j %d, k %d, prev %o, new %o\n",
			j, k, prev, body[j] & A_ATTRIBUTES);
#endif
		prev = body[j] & A_ATTRIBUTES;
		buf[k++] = body[j++];
	}
	if (shoved) {
		/* k is 1 more than the last column of the line */
		if (k > columns)
			k = columns;
		if (buf[k-1]&A_ATTRIBUTES) {
			if (k < columns)
				k++;
			buf[k-1] = ' ';	/* All attributes off */
		}
		memcpy((char *)body, (char *)buf, k * (int)sizeof(chtype));
		len = k;
	}
	if (cursincr && lno == SP->virt_y+1)
		SP->virt_x += cursincr;
#ifdef DEBUG
	if(outf) fprintf(outf, "returns '");
	_prstr(body, len);
	if(outf) fprintf(outf, "', len %d, SP->virt_x %d\n", len, SP->virt_x);
#endif
	return len;
}

#ifdef DEBUG
static
_prstr(result, len)
chtype *result;
int len;
{
	register chtype *cp;

	if (!outf)
		return;

	for (cp=result; *cp && cp < result+len; cp++)
		if (*cp >= ' ' && *cp <= '~')
			fprintf(outf, "%c", *cp);
		else
			fprintf(outf, "<%o,%c>",
				*cp&A_ATTRIBUTES, *cp&A_CHARTEXT);
}
#endif
