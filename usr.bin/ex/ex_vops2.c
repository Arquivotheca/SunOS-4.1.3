/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static  char *sccsid = "@(#)ex_vops2.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.13 */
#endif

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Low level routines for operations sequences,
 * and mostly, insert mode (and a subroutine
 * to read an input line, including in the echo area.)
 */
extern char	*vUA1, *vUA2;		/* extern; also in ex_vops.c */
extern char	*vUD1, *vUD2;		/* extern; also in ex_vops.c */

/*
 * Obleeperate characters in hardcopy
 * open with \'s.
 */
bleep(i, cp)
	register int i;
	char *cp;
{

	i -= column(cp);
	do
		putchar('\\' | QUOTE);
	while (--i >= 0);
	rubble = 1;
}

/*
 * Common code for middle part of delete
 * and change operating on parts of lines.
 */
vdcMID()
{
	register char *cp;

	squish();
	setLAST();
	if (FIXUNDO)
		vundkind = VCHNG, CP(vutmp, linebuf);
	if (wcursor < cursor)
		cp = wcursor, wcursor = cursor, cursor = cp;
	vUD1 = vUA1 = vUA2 = cursor; vUD2 = wcursor;
	return (column(wcursor - 1));
}

/*
 * Take text from linebuf and stick it
 * in the VBSIZE buffer BUF.  Used to save
 * deleted text of part of line.
 */
takeout(BUF)
	char *BUF;
{
	register char *cp;

	if (wcursor < linebuf)
		wcursor = linebuf;
	if (cursor == wcursor) {
		beep();
		return;
	}
	if (wcursor < cursor) {
		cp = wcursor;
		wcursor = cursor;
		cursor = cp;
	}
	setBUF(BUF);
	if ((unsigned char)BUF[128] == 0200)
		beep();
}

/*
 * Are we at the end of the printed representation of the
 * line?  Used internally in hardcopy open.
 */
ateopr()
{
	register int i, c;
	register short *cp = vtube[destline] + destcol;

	for (i = WCOLS - destcol; i > 0; i--) {
		c = *cp++;
		if (c == 0) {
			/*
			 * Optimization to consider returning early, saving
			 * CPU time.  We have to make a special check that
			 * we aren't missing a mode indicator.
			 */
			if (destline == WECHO && destcol < WCOLS-11 && vtube[WECHO][WCOLS-20])
				return 0;
			return (1);
		}
		if (c != ' ' && (c & QUOTE) == 0)
			return (0);
	}
	return (1);
}

/*
 * Append.
 *
 * This routine handles the top level append, doing work
 * as each new line comes in, and arranging repeatability.
 * It also handles append with repeat counts, and calculation
 * of autoindents for new lines.
 */
bool	vaifirst;
bool	gobbled;
char	*ogcursor;

static int 	INSCDCNT; /* number of ^D's (backtabs) in insertion buffer */

static int 	inscdcnt; /* 
			   * count of ^D's (backtabs) not seen yet when doing
		 	   * repeat of insertion
			   */

vappend(ch, cnt, indent)
	int ch;		/* char --> int */
	int cnt, indent;
{
	register int i;
	register char *gcursor;
	bool escape;
	int repcnt, savedoomed;
	short oldhold = hold;

	/*
	 * Before a move in hardopen when the line is dirty
	 * or we are in the middle of the printed representation,
	 * we retype the line to the left of the cursor so the
	 * insert looks clean.
	 */

	if (ch != 'o' && state == HARDOPEN && (rubble || !ateopr())) {
		rubble = 1;
		gcursor = cursor;
		i = *gcursor;
		*gcursor = ' ';
		wcursor = gcursor;
		vmove();
		*gcursor = i;
	}
	vaifirst = indent == 0;

	/*
	 * Handle replace character by (eventually)
	 * limiting the number of input characters allowed
	 * in the vgetline routine.
	 */
	if (ch == 'r')
		repcnt = 2;
	else
		repcnt = 0;

	/*
	 * If an autoindent is specified, then
	 * generate a mixture of blanks to tabs to implement
	 * it and place the cursor after the indent.
	 * Text read by the vgetline routine will be placed in genbuf,
	 * so the indent is generated there.
	 */
	if (value(vi_AUTOINDENT) && indent != 0) {
		gcursor = genindent(indent);
		*gcursor = 0;
		vgotoCL(qcolumn(cursor - 1, genbuf));
	} else {
		gcursor = genbuf;
		*gcursor = 0;
		if (ch == 'o')
			vfixcurs();
	}

	/*
	 * Prepare for undo.  Pointers delimit inserted portion of line.
	 */
	vUA1 = vUA2 = cursor;

	/*
	 * If we are not in a repeated command and a ^@ comes in
	 * then this means the previous inserted text.
	 * If there is none or it was too long to be saved,
	 * then beep() and also arrange to undo any damage done
	 * so far (e.g. if we are a change.)
	 */
	switch (ch) {
	case 'r':
		break;
	case 'a':
		vshowmode("APPEND MODE");
		break;
	case 's':
		vshowmode("SUBSTITUTE MODE");
		break;
	case 'c':
		vshowmode("CHANGE MODE");
		break;
	case 'R':
		vshowmode("REPLACE MODE");
		break;
	case 'o':
		vshowmode("OPEN MODE");
		break;
	case 'i':
		vshowmode("INSERT MODE");
		break;
	default:
		vshowmode("INPUT MODE");
	}
	if ((vglobp && *vglobp == 0) || peekbr()) {
		if ((unsigned char)INS[128] == 0200) {
			beep();
			if (!splitw)
				ungetkey('u');
			doomed = 0;
			hold = oldhold;
			return;
		}
		/*
		 * Unread input from INS.
		 * An escape will be generated at end of string.
		 * Hold off n^^2 type update on dumb terminals.
		 */
		vglobp = INS;
		inscdcnt = INSCDCNT;
		hold |= HOLDQIK;
	} else if (vglobp == 0) {
		/*
		 * Not a repeated command, get
		 * a new inserted text for repeat.
		 */
		INS[0] = 0;
		INS[128] = 0;
		INSCDCNT = 0;
	}

	/*
	 * For wrapmargin to hack away second space after a '.'
	 * when the first space caused a line break we keep
	 * track that this happened in gobblebl, which says
	 * to gobble up a blank silently.
	 */
	gobblebl = 0;

	/*
	 * Text gathering loop.
	 * New text goes into genbuf starting at gcursor.
	 * cursor preserves place in linebuf where text will eventually go.
	 */
	if (*cursor == 0 || state == CRTOPEN)
		hold |= HOLDROL;
	for (;;) {
		if (ch == 'r' && repcnt == 0)
			escape = 0;
		else {
			gcursor = vgetline(repcnt, gcursor, &escape, ch);

			/*
			 * After an append, stick information
			 * about the ^D's and ^^D's and 0^D's in
			 * the repeated text buffer so repeated
			 * inserts of stuff indented with ^D as backtab's
			 * can work.
			 */
			if (HADUP)
				addtext("^");
			else if (HADZERO)
				addtext("0");
			if(!vglobp)
				INSCDCNT = CDCNT;
			while (CDCNT > 0) {
				addtext("\004");
				CDCNT--;
			}
			if (gobbled)
				addtext(" ");
			addtext(ogcursor);
		}
		repcnt = 0;

		/*
		 * Smash the generated and preexisting indents together
		 * and generate one cleanly made out of tabs and spaces
		 * if we are using autoindent.
		 */
		if (!vaifirst && value(vi_AUTOINDENT)) {
			i = fixindent(indent);
			if (!HADUP)
				indent = i;
			gcursor = strend(genbuf);
		}

		/*
		 * Limit the repetition count based on maximum
		 * possible line length; do output implied
		 * by further count (> 1) and cons up the new line
		 * in linebuf.
		 */
		cnt = vmaxrep(ch, cnt);
		CP(gcursor + 1, cursor);
		do {
			CP(cursor, genbuf);
			if (cnt > 1) {
				int oldhold = hold;

				Outchar = vinschar;
				hold |= HOLDQIK;
				printf("%s", genbuf);
				hold = oldhold;
				Outchar = vputchar;
			}
			cursor += gcursor - genbuf;
		} while (--cnt > 0);
		endim();
		vUA2 = cursor;
		if (escape != '\n')
			CP(cursor, gcursor + 1);

		/*
		 * If doomed characters remain, clobber them,
		 * and reopen the line to get the display exact.
		 */
		if (state != HARDOPEN) {
			DEPTH(vcline) = 0;
			savedoomed = doomed;
			if (doomed > 0) {
				register int cind = cindent();

				physdc(cind, cind + doomed);
				doomed = 0;
			}
			i = vreopen(LINE(vcline), lineDOT(), vcline);
#ifdef TRACE
			if (trace)
				fprintf(trace, "restoring doomed from %d to %d\n", doomed, savedoomed);
#endif
			if (ch == 'R')
				doomed = savedoomed;
		}

		/*
		 * All done unless we are continuing on to another line.
		 */
		if (escape != '\n') {
			vshowmode("");
			break;
		}

		/*
		 * Set up for the new line.
		 * First save the current line, then construct a new
		 * first image for the continuation line consisting
		 * of any new autoindent plus the pushed ahead text.
		 */
		killU();
		addtext(gobblebl ? " " : "\n");
		vsave();
		cnt = 1;
		if (value(vi_AUTOINDENT)) {
			if (value(vi_LISP))
				indent = lindent(dot + 1);
			else
			     if (!HADUP && vaifirst)
				indent = whitecnt(linebuf);
			vaifirst = 0;
			strcLIN(vpastwh(gcursor + 1));
			gcursor = genindent(indent);
			*gcursor = 0;
			if (gcursor + strlen(linebuf) > &genbuf[LBSIZE - 2])
				gcursor = genbuf;
			CP(gcursor, linebuf);
		} else {
			CP(genbuf, gcursor + 1);
			gcursor = genbuf;
		}

		/*
		 * If we started out as a single line operation and are now
		 * turning into a multi-line change, then we had better yank
		 * out dot before it changes so that undo will work
		 * correctly later.
		 */
		if (FIXUNDO && vundkind == VCHNG) {
			vremote(1, yank, 0);
			undap1--;
		}

		/*
		 * Now do the append of the new line in the buffer,
		 * and update the display.  If slowopen
		 * we don't do very much.
		 */
		vdoappend(genbuf);
		vundkind = VMANYINS;
		vcline++;
		if (state != VISUAL)
			vshow(dot, NOLINE);
		else {
			i += LINE(vcline - 1);
			vopen(dot, i);
			if (value(vi_SLOWOPEN))
				vscrap();
			else
				vsync1(LINE(vcline));
		}
		switch (ch) {
		case 'r':
			break;
		case 'a':
			vshowmode("APPEND MODE");
			break;
		case 's':
			vshowmode("SUBSTITUTE MODE");
			break;
		case 'c':
			vshowmode("CHANGE MODE");
			break;
		case 'R':
			vshowmode("REPLACE MODE");
			break;
		case 'i':
			vshowmode("INSERT MODE");
			break;
		case 'o':
			vshowmode("OPEN MODE");
			break;
		default:
			vshowmode("INPUT MODE");
		}
		strcLIN(gcursor);
		*gcursor = 0;
		cursor = linebuf;
		vgotoCL(qcolumn(cursor - 1, genbuf));
	}

	/*
	 * All done with insertion, position the cursor
	 * and sync the screen.
	 */
	hold = oldhold;
	if (cursor > linebuf)
		cursor--;
	if (state != HARDOPEN)
		vsyncCL();
	else if (cursor > linebuf)
		back1();
	doomed = 0;
	wcursor = cursor;
	vmove();
}

/*
 * Subroutine for vgetline to back up a single character position,
 * backwards around end of lines (vgoto can't hack columns which are
 * less than 0 in general).
 */
back1()
{

	vgoto(destline - 1, WCOLS + destcol - 1);
}

/*
 * Get a line into genbuf after gcursor.
 * Cnt limits the number of input characters
 * accepted and is used for handling the replace
 * single character command.  Aescaped is the location
 * where we stick a termination indicator (whether we
 * ended with an ESCAPE or a newline/return.
 *
 * We do erase-kill type processing here and also
 * are careful about the way we do this so that it is
 * repeatable.  (I.e. so that your kill doesn't happen,
 * when you repeat an insert if it was escaped with \ the
 * first time you did it.  commch is the command character
 * involved, including the prompt for readline.
 */
char *
vgetline(cnt, gcursor, aescaped, commch)
	int cnt;
	register char *gcursor;
	bool *aescaped;
	char commch;
{
	register int c, ch;
	register char *cp;
	int x, y, iwhite, backsl=0;
	char *iglobp;
	char cstr[2];
	int (*OO)() = Outchar;

	/*
	 * Clear the output state and counters
	 * for autoindent backwards motion (counts of ^D, etc.)
	 * Remember how much white space at beginning of line so
	 * as not to allow backspace over autoindent.
	 */
	
	*aescaped = 0;
	ogcursor = gcursor;
	flusho();
	CDCNT = 0;
	HADUP = 0;
	HADZERO = 0;
	gobbled = 0;
	iwhite = whitecnt(genbuf);
	iglobp = vglobp;

	/*
	 * Clear abbreviation recursive-use count
	 */
	abbrepcnt = 0;
	/*
	 * Carefully avoid using vinschar in the echo area.
	 */
	if (splitw)
		Outchar = vputchar;
	else {
		Outchar = vinschar;
		vprepins();
	}
	for (;;) {
		backsl = 0;
		if (gobblebl)
			gobblebl--;
		if (cnt != 0) {
			cnt--;
			if (cnt == 0)
				goto vadone;
		}
		c = getkey();
		if (c != ATTN)
			c &= 0377;
		ch = c;
		maphopcnt = 0;
		if (vglobp == 0 && Peekkey == 0 && commch != 'r')
			while ((ch = map(c, immacs, commch)) != c) {
				c = ch;
				if (!value(vi_REMAP))
					break;
				if (++maphopcnt > 256)
					error("Infinite macro loop");
			}
		if (!iglobp) {

			/*
			 * Erase-kill type processing.
			 * Only happens if we were not reading
			 * from untyped input when we started.
			 * Map users erase to ^H, kill to -1 for switch.
			 */
#ifndef USG
			if (c == tty.sg_erase)
				c = CTRL(h);
			else if (c == tty.sg_kill)
				c = -1;
#else
			if (c == tty.c_cc[VERASE])
				c = CTRL(h);
			else if (c == tty.c_cc[VKILL])
				c = -1;
#endif
			switch (c) {

			/*
			 * ^?		Interrupt drops you back to visual
			 *		command mode with an unread interrupt
			 *		still in the input buffer.
			 *
			 * ^\		Quit does the same as interrupt.
			 *		If you are a ex command rather than
			 *		a vi command this will drop you
			 *		back to command mode for sure.
			 */
			case ATTN:
			case QUIT:
				ungetkey(c);
				goto vadone;

			/*
			 * ^H		Backs up a character in the input.
			 *
			 * BUG:		Can't back around line boundaries.
			 *		This is hard because stuff has
			 *		already been saved for repeat.
			 */
			case CTRL(h):
bakchar:
				cp = gcursor - 1;
				if (cp < ogcursor) {
					if (splitw) {
						/*
						 * Backspacing over readecho
						 * prompt. Pretend delete but
						 * don't beep.
						 */
						ungetkey(c);
						goto vadone;
					}
					beep();
					continue;
				}
				if (splitw) {
					/* do a hard backspace if
					 * we are on the echo line.
					 * This includes spacing over control
					 * chars and tabs which may be there
					 * Crude, but avoids the coredump in
					 * vbackup.
					 */
					vputp(cursor_left, 0); 
					destcol--; outcol--;
					if (iscntrl(*cp)) {
						if (*cp == '\t') {
							*cp = 0;
							x = outcol - qcolumn(cursor - 1, genbuf); 
						}
						else 
							x = 1;
						for (y=x; y>0 ; --y)  {
							vputp(cursor_left, 0);
							destcol--;
							outcol--;
						}
					} 
				}	
				goto vbackup;

			/*
			 * ^W		Back up a white/non-white word.
			 */
			case CTRL(w):
				ch = 0;
				wdkind = 1;
				for (cp = gcursor; cp > ogcursor
				    && isspace((unsigned char)cp[-1]); cp--)  {
					if (cp[0] != 0 && iscntrl(cp[0]))
						ch++;
					continue;
				}
						
				for (c = wordch(cp - 1);
				    cp > ogcursor && wordof(c, cp - 1); cp--) {
					if (iscntrl(cp[-1]) ||
						(cp[0] != 0 && iscntrl(cp[0])) )
						ch++;
					continue;
				}	
				if (splitw) {
					
					/* go left enough to get to 
				 	 * start of previous word
				 	 */
					for (c=0 ; c<gcursor-cp+ch ; c++){
						vputp(cursor_left,0);
						destcol--;
						outcol--;
					}
				}
				goto vbackup;

			/*
			 * users kill	Kill input on this line, back to
			 *		the autoindent.
			 */
			case -1:
				ch = -1;
				cp = ogcursor;
vbackup:
				if (cp == gcursor) {
					beep();
					continue;
				}
				endim();
				*cp = 0;
				c = cindent();
				/* the vgoto rules just don't cope with 
				 * splits on the echo line
				 */
				if (!splitw || (splitw && ch == -1))
					vgotoCL(qcolumn(cursor - 1, genbuf));
				if (doomed >= 0)
					doomed += c - cindent();
				gcursor = cp;
				continue;

			/*
			 * \		Followed by erase or kill
			 *		maps to just the erase or kill.
			 */
			case '\\':
				x = destcol, y = destline;
				putchar('\\');
				vcsync();
				c = getkey();
#ifndef USG
				if (c == tty.sg_erase
				    || c == tty.sg_kill)
#else
				if (c == tty.c_cc[VERASE]
				    || c == tty.c_cc[VKILL])
#endif
				{
					vgoto(y, x);
					if (doomed >= 0)
						doomed++;
					goto def;
				}
				ungetkey(c), c = '\\';
				backsl = 1;
				break;

			/*
			 * ^Q		Super quote following character
			 *		Only ^@ is verboten (trapped at
			 *		a lower level) and \n forces a line
			 *		split so doesn't really go in.
			 *
			 * ^V		Synonym for ^Q
			 */
			case CTRL(q):
			case CTRL(v):
				x = destcol, y = destline;
				if (splitw) {
				/* dont use vgoto here!  */
					outchar('^');
					outchar('\b');
				}
				else {
					putchar('^');
					vgoto(y, x);
				}
				c = getkey();
#ifdef USG
				if (c == ATTN)
					c = tty.c_cc[VINTR];
#else
#ifdef TIOCSETC
				if (c == ATTN)
					c = nttyc.t_intrc;
#endif
#endif
				if (c != NL) {
					if (doomed >= 0)
						doomed++;
					goto def;
				}
				break;
			}
		}

		/*
		 * If we get a blank not in the echo area
		 * consider splitting the window in the wrapmargin.
		 */
		if (c != NL && !splitw) {
			if (c == ' ' && gobblebl) {
				gobbled = 1;
				continue;
			}
			if (value(vi_WRAPMARGIN) &&
				(outcol >= OCOLUMNS - value(vi_WRAPMARGIN) ||
				 backsl && outcol==0) &&
				commch != 'r') {
				/*
				 * At end of word and hit wrapmargin.
				 * Move the word to next line and keep going.
				 */
				wdkind = 1;
				*gcursor++ = c;
				if (backsl)
					*gcursor++ = getkey();
				*gcursor = 0;
				/*
				 * Find end of previous word if we are past it.
				 */
				for (cp=gcursor; cp>ogcursor
				    && isspace((unsigned char)cp[-1]); cp--)
					;
				if (outcol+(backsl?OCOLUMNS:0) - (gcursor-cp) >= OCOLUMNS - value(vi_WRAPMARGIN)) {
					/*
					 * Find beginning of previous word.
					 */
					for (; cp>ogcursor
					    && !isspace((unsigned char)cp[-1]); cp--)
						;
					if (cp <= ogcursor) {
						/*
						 * There is a single word that
						 * is too long to fit.  Just
						 * let it pass, but beep for
						 * each new letter to warn
						 * the luser.
						 */
						c = *--gcursor;
						*gcursor = 0;
						beep();
						goto dontbreak;
					}
					/*
					 * Save it for next line.
					 */
					unget("\n", 0);
					unget(cp, 1);
					cp--;
				} else
					unget("\n", 0);
				/*
				 * Erase white space before the word.
				 */
				while (cp > ogcursor
				    && isspace((unsigned char)cp[-1]))
					cp--;	/* skip blank */
				gobblebl = 3;
				goto vbackup;
			}
		dontbreak:;
		}

		/*
		 * Word abbreviation mode.
		 */
		cstr[0] = c;
		if (anyabbrs && gcursor > ogcursor && !wordch(cstr) && wordch(gcursor-1)) {
				int wdtype, abno;

				cstr[1] = 0;
				wdkind = 1;
				cp = gcursor - 1;
				for (wdtype = wordch(cp - 1);
				    cp > ogcursor && wordof(wdtype, cp - 1); cp--)
					;
				*gcursor = 0;
				for (abno=0; abbrevs[abno].mapto; abno++) {
					if (eq(cp, abbrevs[abno].cap)) {
						if(abbrepcnt == 0) {
							if(reccnt(abbrevs[abno].cap, abbrevs[abno].mapto))
								abbrepcnt = 1;
							macpush(cstr, 0);
							macpush(abbrevs[abno].mapto);
							goto vbackup;
						} else
							abbrepcnt = 0;
					}
				}
		}

		switch (c) {

		/*
		 * ^M		Except in repeat maps to \n.
		 */
		case CR:
			if (vglobp)
				goto def;
			c = '\n';
			/* presto chango ... */

		/*
		 * \n		Start new line.
		 */
		case NL:
			*aescaped = c;
			goto vadone;

		/*
		 * escape	End insert unless repeat and more to repeat.
		 */
		case ESCAPE:
			if (lastvgk)
				goto def;
			goto vadone;

		/*
		 * ^D		Backtab.
		 * ^T		Software forward tab.
		 *
		 *		Unless in repeat where this means these
		 *		were superquoted in.
		 */
		case CTRL(t):
			if (vglobp)
				goto def;
			/* fall into ... */

			*gcursor = 0;
			cp = vpastwh(genbuf);
			c = whitecnt(genbuf);
			if (ch == CTRL(t)) {
				/*
				 * ^t just generates new indent replacing
				 * current white space rounded up to soft
				 * tab stop increment.
				 */
				if (cp != gcursor)
					/*
					 * BUG:		Don't hack ^T except
					 *		right after initial
					 *		white space.
					 */
					continue;
				cp = genindent(iwhite = backtab(c + value(vi_SHIFTWIDTH) + 1));
				ogcursor = cp;
				goto vbackup;
			}
			/*
			 * ^D works only if we are at the (end of) the
			 * generated autoindent.  We count the ^D for repeat
			 * purposes.
			 */
		case CTRL(d):
			/* check if ^d was superquoted in */
			if(vglobp && inscdcnt <= 0)
				goto def;
			if(vglobp)
				inscdcnt--;
			*gcursor = 0;
			cp = vpastwh(genbuf);
			c = whitecnt(genbuf);
			if (c == iwhite && c != 0)
				if (cp == gcursor) {
					iwhite = backtab(c);
					CDCNT++;
					ogcursor = cp = genindent(iwhite);
					goto vbackup;
				} else if (&cp[1] == gcursor &&
				    (*cp == '^' || *cp == '0')) {
					/*
					 * ^^D moves to margin, then back
					 * to current indent on next line.
					 *
					 * 0^D moves to margin and then
					 * stays there.
					 */
					HADZERO = *cp == '0';
					ogcursor = cp = genbuf;
					HADUP = 1 - HADZERO;
					CDCNT = 1;
					endim();
					back1();
					vputchar(' ');
					goto vbackup;
				}
			if (vglobp && vglobp - iglobp >= 2 &&
			    (vglobp[-2] == '^' || vglobp[-2] == '0')
			    && gcursor == ogcursor + 1)
				goto bakchar;
			continue;

		default:
			/*
			 * Possibly discard control inputs.
			 */
			if (!vglobp && junk(c)) {
				beep();
				continue;
			}
def:
			if (!backsl) {
				int cnt;
				putchar(c);
				flush();
			}
			if (gcursor > &genbuf[LBSIZE - 2])
				error("Line too long");
			*gcursor++ = c;
			vcsync();
			if (value(vi_SHOWMATCH) && !iglobp)
				if (c == ')' || c == '}')
					lsmatch(gcursor);
			continue;
		}
	}
vadone:
	*gcursor = 0;
	if (Outchar != termchar)
		Outchar = OO;
	endim();
	return (gcursor);
}

int	vgetsplit();
char	*vsplitpt;

/*
 * Append the line in buffer at lp
 * to the buffer after dot.
 */
vdoappend(lp)
	char *lp;
{
	register int oing = inglobal;

	vsplitpt = lp;
	inglobal = 1;
	ignore(append(vgetsplit, dot));
	inglobal = oing;
}

/*
 * Subroutine for vdoappend to pass to append.
 */
vgetsplit()
{

	if (vsplitpt == 0)
		return (EOF);
	strcLIN(vsplitpt);
	vsplitpt = 0;
	return (0);
}

/*
 * Vmaxrep determines the maximum repetition factor
 * allowed that will yield total line length less than
 * LBSIZE characters and also does hacks for the R command.
 */
vmaxrep(ch, cnt)
	char ch;
	register int cnt;
{
	register int len, replen;

	if (cnt > LBSIZE - 2)
		cnt = LBSIZE - 2;
	replen = strlen(genbuf);
	if (ch == 'R') {
		len = strlen(cursor);
		if (replen < len)
			len = replen;
		CP(cursor, cursor + len);
		vUD2 += len;
	}
	len = strlen(linebuf);
	if (len + cnt * replen <= LBSIZE - 2)
		return (cnt);
	cnt = (LBSIZE - 2 - len) / replen;
	if (cnt == 0) {
		vsave();
		error("Line too long");
	}
	return (cnt);
}

/*
 * Determine how many occurrences of word 'CAP' are in 'MAPTO'.  To be
 * considered an occurrence there must be both a nonword-prefix, a 
 * complete match of 'CAP' within 'MAPTO', and a nonword-suffix. 
 * Note that the beginning and end of 'MAPTO' are considered to be
 * valid nonword delimiters.
 */
reccnt(cap, mapto)
char *cap;
char *mapto;
{
	register int i, cnt, final;

	cnt = 0;
	final = strlen(mapto) - strlen(cap);

	for (i=0; i <= final; i++) 
	  if ((strncmp(cap, mapto+i, strlen(cap)) == 0)       /* match */
	  && (i == 0     || !wordch(&mapto[i-1]))	      /* prefix ok */
	  && (i == final || !wordch(&mapto[i+strlen(cap)])))  /* suffix ok */
		cnt++;
	return (cnt);
}

