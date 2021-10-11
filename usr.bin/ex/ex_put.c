/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_put.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.13 */
#endif

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Terminal driving and line formatting routines.
 * Basic motion optimizations are done here as well
 * as formatting of lines (printing of control characters,
 * line numbering and the like).
 */

/*
 * The routines outchar, putchar and pline are actually
 * variables, and these variables point at the current definitions
 * of the routines.  See the routine setflav.
 * We sometimes make outchar be routines which catch the characters
 * to be printed, e.g. if we want to see how long a line is.
 * During open/visual, outchar and putchar will be set to
 * routines in the file ex_vput.c (vputchar, vinschar, etc.).
 */
int	(*Outchar)() = termchar;
int	(*Putchar)() = normchar;
int	(*Pline)() = normline;

int (*
setlist(t))()
	bool t;
{
	register int (*P)();

	listf = t;
	P = Putchar;
	Putchar = t ? listchar : normchar;
	return (P);
}

int (*
setnumb(t))()
	bool t;
{
	register int (*P)();

	numberf = t;
	P = Pline;
	Pline = t ? numbline : normline;
	return (P);
}

/*
 * Format c for list mode; leave things in common
 * with normal print mode to be done by normchar.
 */
listchar(c)
	register short c;
{

	c &= (TRIM|QUOTE);
	switch (c) {

	case '\t':
	case '\b':
		outchar('^');
		c = ctlof(c);
		break;

	case '\n':
		break;

	case '\n' | QUOTE:
		outchar('$');
		break;

	default:
		if(c & QUOTE)
			break;
		if (c < ' ' && c != '\n' || c == DELETE)
			outchar('^'), c = ctlof(c);
	}
	normchar(c);
}

/*
 * Format c for printing.  Handle funnies of upper case terminals
 * and hazeltines which don't have ~.
 */
normchar(c)
	register short c;
{
	register char *colp;

	c &= (TRIM|QUOTE);
	if (c == '~' && tilde_glitch) {
		normchar('\\');
		c = '^';
	}
	if (c & QUOTE)
		switch (c) {

		case ' ' | QUOTE:
		case '\b' | QUOTE:
			break;

		case QUOTE:
			return;

		default:
			c &= TRIM;
		}
	else if (c < ' ' && (c != '\b' || !over_strike) && c != '\n' && c != '\t' || c == DELETE)
		putchar('^'), c = ctlof(c);
	else if (c >= 0200 && !isprint(c)) {
		outchar('\\');
		outchar(((c >> 6) & 07) + '0');
		outchar(((c >> 3) & 07) + '0');
		outchar((c & 07) + '0');
		return;
	} else if (UPPERCASE)
		if (isupper(c)) {
			outchar('\\');
			c = tolower(c);
		} else {
			colp = "({)}!|^~'`";
			while (*colp++)
				if (c == *colp++) {
					outchar('\\');
					c = colp[-2];
					break;
				}
		}
	outchar(c);
}

/*
 * Print a line with a number.
 */
numbline(i)
	int i;
{

	if (shudclob)
		slobber(' ');
	printf("%6d  ", i);
	normline();
}

/*
 * Normal line output, no numbering.
 */
normline()
{
	register char *cp;

	if (shudclob)
		slobber(linebuf[0]);
	/* pdp-11 doprnt is not reentrant so can't use "printf" here
	   in case we are tracing */
	for (cp = linebuf; *cp;)
		putchar(*cp++);
	if (!inopen)
		putchar('\n' | QUOTE);
}

/*
 * Given c at the beginning of a line, determine whether
 * the printing of the line will erase or otherwise obliterate
 * the prompt which was printed before.  If it won't, do it now.
 */
slobber(c)
	int c;
{

	shudclob = 0;
	switch (c) {

	case '\t':
		if (Putchar == listchar)
			return;
		break;

	default:
		return;

	case ' ':
	case 0:
		break;
	}
	if (over_strike)
		return;
	flush();
	putch(' ');
	tputs(cursor_left, 0, putch);
}

/*
 * The output buffer is initialized with a useful error
 * message so we don't have to keep it in data space.
 */
static	short linb[66];
short *linp = linb;

/*
 * Phadnl records when we have already had a complete line ending with \n.
 * If another line starts without a flush, and the terminal suggests it,
 * we switch into -nl mode so that we can send linefeeds to avoid
 * a lot of spacing.
 */
static	bool phadnl;

/*
 * Indirect to current definition of putchar.
 */
putchar(c)
	int c;
{

	(*Putchar)(c);
}

/*
 * Termchar routine for command mode.
 * Watch for possible switching to -nl mode.
 * Otherwise flush into next level of buffering when
 * small buffer fills or at a newline.
 */
termchar(c)
	int c;
{

	if (pfast == 0 && phadnl)
		pstart();
	if (c == '\n')
		phadnl = 1;
	else if (linp >= &linb[63])
		flush1();
	*linp++ = c;
	if (linp >= &linb[63]) {
		fgoto();
		flush1();
	}
}

flush()
{

	flush1();
	flush2();
}

/*
 * Flush from small line buffer into output buffer.
 * Work here is destroying motion into positions, and then
 * letting fgoto do the optimized motion.
 */
flush1()
{
	register short *lp;
	register short c;

	*linp = 0;
	lp = linb;
	while (*lp)
		switch (c = *lp++) {

		case '\r':
			destline += destcol / columns;
			destcol = 0;
			continue;

		case '\b':
			if (destcol)
				destcol--;
			continue;

		case ' ':
			destcol++;
			continue;

		case '\t':
			destcol += value(vi_TABSTOP) - destcol % value(vi_TABSTOP);
			continue;

		case '\n':
			destline += destcol / columns + 1;
			if (destcol != 0 && destcol % columns == 0)
				destline--;
			destcol = 0;
			continue;

		default:
			fgoto();
			for (;;) {
				if (auto_right_margin == 0 && outcol == columns)
					fgoto();
				c &= TRIM;
				putch(c);
				if (c == '\b') {
					outcol--;
					destcol--;
				} else if (c >= ' ' && c != DELETE) {
					outcol++;
					destcol++;
					if (eat_newline_glitch && outcol % columns == 0)
						putch('\r'), putch('\n');
				}
				c = *lp++;
				if (c <= ' ')
					break;
			}
			--lp;
			continue;
		}
	linp = linb;
}

flush2()
{

	fgoto();
	flusho();
	pstop();
}

/*
 * Sync the position of the output cursor.
 * Most work here is rounding for terminal boundaries getting the
 * column position implied by wraparound or the lack thereof and
 * rolling up the screen to get destline on the screen.
 */
fgoto()
{
	register int l, c;

	if (destcol > columns - 1) {
		destline += destcol / columns;
		destcol %= columns;
	}
	if (outcol > columns - 1) {
		l = (outcol + 1) / columns;
		outline += l;
		outcol %= columns;
		if (auto_right_margin == 0) {
			while (l > 0) {
				if (pfast)
					tputs(carriage_return, 0, putch);
				tputs(cursor_down, 0, putch);
				l--;
			}
			outcol = 0;
		}
		if (outline > lines - 1) {
			destline -= outline - (lines - 1);
			outline = lines - 1;
		}
	}
	if (destline > lines - 1) {
		l = destline;
		destline = lines - 1;
		if (outline < lines - 1) {
			c = destcol;
			if (pfast == 0 && (!cursor_address || holdcm))
				destcol = 0;
			fgoto();
			destcol = c;
		}
		while (l > lines - 1) {
			/*
			 * The following linefeed (or simulation thereof)
			 * is supposed to scroll up the screen, since we
			 * are on the bottom line.
			 *
			 * Superbee glitch:  in the middle of the screen we
			 * have to use esc B (down) because linefeed messes up
			 * in "Efficient Paging" mode (which is essential in
			 * some SB's because CRLF mode puts garbage
			 * in at end of memory), but you must use linefeed to
			 * scroll since down arrow won't go past memory end.
			 * I turned this off after receiving Paul Eggert's
			 * Superbee description which wins better.
			 */
			if (scroll_forward /* && !beehive_glitch */ && pfast)
				tputs(scroll_forward, 0, putch);
			else
				putch('\n');
			l--;
			if (pfast == 0)
				outcol = 0;
		}
	}
	if (destline < outline && !(cursor_address && !holdcm || cursor_up || cursor_home))
		destline = outline;
	if (cursor_address && !holdcm)
		if (plod(costCM) > 0)
			plod(0);
		else
			tputs(tparm(cursor_address, destline, destcol), 0, putch);
	else
		plod(0);
	outline = destline;
	outcol = destcol;
}

/*
 * Tab to column col by flushing and then setting destcol.
 * Used by "set all".
 */
gotab(col)
	int col;
{

	flush1();
	destcol = col;
}

/*
 * Move (slowly) to destination.
 * Hard thing here is using home cursor on really deficient terminals.
 * Otherwise just use cursor motions, hacking use of tabs and overtabbing
 * and backspace.
 */

static int plodcnt, plodflg;

plodput(c)
{

	if (plodflg)
		plodcnt--;
	else
		putch(c);
}

plod(cnt)
{
	register int i, j, k;
	register int soutcol, soutline;

	plodcnt = plodflg = cnt;
	soutcol = outcol;
	soutline = outline;
	/*
	 * Consider homing and moving down/right from there, vs moving
	 * directly with local motions to the right spot.
	 */
	if (cursor_home) {
		/*
		 * i is the cost to home and tab/space to the right to
		 * get to the proper column.  This assumes cursor_right costs
		 * 1 char.  So i+destcol is cost of motion with home.
		 */
		if (tab && value(vi_HARDTABS))
			i = (destcol / value(vi_HARDTABS)) + (destcol % value(vi_HARDTABS));
		else
			i = destcol;
		/*
		 * j is cost to move locally without homing
		 */
		if (destcol >= outcol) {	/* if motion is to the right */
			if (value(vi_HARDTABS)) {
				j = destcol / value(vi_HARDTABS) - outcol / value(vi_HARDTABS);
				if (tab && j)
					j += destcol % value(vi_HARDTABS);
				else
					j = destcol - outcol;
			} else
				j = destcol - outcol;
		} else
			/* leftward motion only works if we can backspace. */
			if (outcol - destcol <= i && (cursor_left))
				i = j = outcol - destcol; /* cheaper to backspace */
			else
				j = i + 1; /* impossibly expensive */

		/* k is the absolute value of vertical distance */
		k = outline - destline;
		if (k < 0)
			k = -k;
		j += k;

		/*
		 * Decision.  We may not have a choice if no cursor_up.
		 */
		if (i + destline < j || (!cursor_up && destline < outline)) {
			/*
			 * Cheaper to home.  Do it now and pretend it's a
			 * regular local motion.
			 */
			tputs(cursor_home, 0, plodput);
			outcol = outline = 0;
		} else if (cursor_to_ll) {
			/*
			 * Quickly consider homing down and moving from there.
			 * Assume cost of cursor_to_ll is 2.
			 */
			k = (lines - 1) - destline;
			if (i + k + 2 < j && (k<=0 || cursor_up)) {
				tputs(cursor_to_ll, 0, plodput);
				outcol = 0;
				outline = lines - 1;
			}
		}
	} else
		/*
		 * No home and no up means it's impossible, so we return an
		 * incredibly big number to make cursor motion win out.
		 */
		if (!cursor_up && destline < outline)
			return (500);
	if (tab && value(vi_HARDTABS))
		i = destcol % value(vi_HARDTABS)
		    + destcol / value(vi_HARDTABS);
	else
		i = destcol;
/*
	if (back_tab && outcol > destcol && (j = (((outcol+7) & ~7) - destcol - 1) >> 3)) {
		j *= (k = strlen(back_tab));
		if ((k += (destcol&7)) > 4)
			j += 8 - (destcol&7);
		else
			j += k;
	} else
*/
		j = outcol - destcol;
	/*
	 * If we will later need a \n which will turn into a \r\n by
	 * the system or the terminal, then don't bother to try to \r.
	 */
	if ((NONL || !pfast) && outline < destline)
		goto dontcr;
	/*
	 * If the terminal will do a \r\n and there isn't room for it,
	 * then we can't afford a \r.
	 */
	if (!carriage_return && outline >= destline)
		goto dontcr;
	/*
	 * If it will be cheaper, or if we can't back up, then send
	 * a return preliminarily.
	 */
	if (j > i + 1 || outcol > destcol && !cursor_left) {
		/*
		 * BUG: this doesn't take the (possibly long) length
		 * of carriage_return into account.
		 */
		if (carriage_return) {
			tputs(carriage_return, 0, plodput);
			outcol = 0;
		} else if (newline) {
			tputs(newline, 0, plodput);
			outline++;
			outcol = 0;
		}
	}
dontcr:
	/* Move down, if necessary, until we are at the desired line */
	while (outline < destline) {
		j = destline - outline;
		if (j > costDP && parm_down_cursor) {
			/* Win big on Tek 4025 */
			tputs(tparm(parm_down_cursor, j), j, plodput);
			outline += j;
		}
		else {
			outline++;
			if (cursor_down && pfast)
				tputs(cursor_down, 0, plodput);
			else
				plodput('\n');
		}
		if (plodcnt < 0)
			goto out;
		if (NONL || pfast == 0)
			outcol = 0;
	}
	if (back_tab)
		k = strlen(back_tab);	/* should probably be cost(back_tab) and moved out */
	/* Move left, if necessary, to desired column */
	while (outcol > destcol) {
		if (plodcnt < 0)
			goto out;
		if (back_tab && !insmode && outcol - destcol > 4+k) {
			tputs(back_tab, 0, plodput);
			outcol--;
			if (value(vi_HARDTABS))
				outcol -= outcol % value(vi_HARDTABS); /* outcol &= ~7; */
			continue;
		}
		j = outcol - destcol;
		if (j > costLP && parm_left_cursor) {
			tputs(tparm(parm_left_cursor, j), j, plodput);
			outcol -= j;
		}
		else {
			outcol--;
			tputs(cursor_left, 0, plodput);
		}
	}
	/* Move up, if necessary, to desired row */
	while (outline > destline) {
		j = outline - destline;
		if (parm_up_cursor && j > 1) {
			/* Win big on Tek 4025 */
			tputs(tparm(parm_up_cursor, j), j, plodput);
			outline -= j;
		}
		else {
			outline--;
			tputs(cursor_up, 0, plodput);
		}
		if (plodcnt < 0)
			goto out;
	}
	/*
	 * Now move to the right, if necessary.  We first tab to
	 * as close as we can get.
	 */
	if (value(vi_HARDTABS) && tab && !insmode && destcol - outcol > 1) {
		/* tab to right as far as possible without passing col */
		for (;;) {
			i = tabcol(outcol, value(vi_HARDTABS));
			if (i > destcol)
				break;
			if (tab)
				tputs(tab, 0, plodput);
			else
				plodput('\t');
			outcol = i;
		}
		/* consider another tab and then some backspaces */
		if (destcol - outcol > 4 && i < columns && cursor_left) {
			tputs(tab, 0, plodput);
			outcol = i;
			/*
			 * Back up.  Don't worry about parm_left_cursor because
			 * it's never more than 4 spaces anyway.
			 */
			while (outcol > destcol) {
				outcol--;
				tputs(cursor_left, 0, plodput);
			}
		}
	}
	/*
	 * We've tabbed as much as possible.  If we still need to go
	 * further (not exact or can't tab) space over.  This is a
	 * very common case when moving to the right with space.
	 */
	while (outcol < destcol) {
		j = destcol - outcol;
		if (j > costRP && parm_right_cursor) {
			/*
			 * This probably happens rarely, if at all.
			 * It seems mainly useful for ANSI terminals
			 * with no hardware tabs, and I don't know
			 * of any such terminal at the moment.
			 */
			tputs(tparm(parm_right_cursor, j), j, plodput);
			outcol += j;
		}
		else {
			/*
			 * move one char to the right.  We don't use right
			 * because it's better to just print the char we are
			 * moving over.  There are various exceptions, however.
			 * If !inopen, vtube contains garbage.  If the char is
			 * a null or a tab we want to print a space.  Other
			 * random chars we use space for instead, too.
			 */
			if (!inopen || vtube[outline]==NULL ||
				(i=vtube[outline][outcol]) < ' ')
				i = ' ';
			if(i & QUOTE)	/* no sign extension on 3B */
				i = ' ';
			if (insmode && cursor_right)
				tputs(cursor_right, 0, plodput);
			else
				plodput(i);
			outcol++;
		}
		if (plodcnt < 0)
			goto out;
	}
out:
	if (plodflg) {
		outcol = soutcol;
		outline = soutline;
	}
	return(plodcnt);
}

/*
 * An input line arrived.
 * Calculate new (approximate) screen line position.
 * Approximate because kill character echoes newline with
 * no feedback and also because of long input lines.
 */
noteinp()
{

	outline++;
	if (outline > lines - 1)
		outline = lines - 1;
	destline = outline;
	destcol = outcol = 0;
}

/*
 * Something weird just happened and we
 * lost track of what's happening out there.
 * Since we can't, in general, read where we are
 * we just reset to some known state.
 * On cursor addressable terminals setting to unknown
 * will force a cursor address soon.
 */
termreset()
{

	endim();
	if (enter_ca_mode)
		putpad(enter_ca_mode);	
	destcol = 0;
	destline = lines - 1;
	if (cursor_address) {
		outcol = UKCOL;
		outline = UKCOL;
	} else {
		outcol = destcol;
		outline = destline;
	}
}

/*
 * Low level buffering, with the ability to drain
 * buffered output without printing it.
 */
char	*obp = obuf;

draino()
{

	obp = obuf;
}

flusho()
{
	if (obp != obuf) {
		write(1, obuf, obp - obuf);
#ifdef TRACE
		if (trace)
			fwrite(obuf, 1, obp-obuf, trace);
#endif
		obp = obuf;
	}
}

putnl()
{

	putchar('\n');
}

putS(cp)
	char *cp;
{

	if (cp == NULL)
		return;
	while (*cp)
		putch(*cp++);
}


putch(c)
	int c;
{

#ifdef OLD3BTTY		
	if(c == '\n')	/* Fake "\n\r" for '\n' til fix in 3B firmware */
		putch('\r');	/* vi does "stty -icanon" => -onlcr !! */
#endif
	*obp++ = c;
	if (obp >= &obuf[sizeof obuf])
		flusho();
}

/*
 * Miscellaneous routines related to output.
 */

/*
 * Put with padding
 */
putpad(cp)
	char *cp;
{

	flush();
	tputs(cp, 0, putch);
}

/*
 * Set output through normal command mode routine.
 */
setoutt()
{

	Outchar = termchar;
}

/*
 * Printf (temporarily) in list mode.
 */
/*VARARGS2*/
lprintf(cp, dp)
	char *cp, *dp;
{
	register int (*P)();

	P = setlist(1);
	printf(cp, dp);
	Putchar = P;
}

/*
 * Newline + flush.
 */
putNFL()
{

	putnl();
	flush();
}

/*
 * Try to start -nl mode.
 */
pstart()
{

	if (NONL)
		return;
 	if (!value(vi_OPTIMIZE))
		return;
	if (ruptible == 0 || pfast)
		return;
	fgoto();
	flusho();
	pfast = 1;
	normtty++;
#ifndef USG
	tty.sg_flags = normf & ~(ECHO|XTABS|CRMOD);
#else
	tty = normf;
	tty.c_oflag &= ~(ONLCR|TAB3);
	tty.c_lflag &= ~ECHO;
#endif
	saveterm();
	sTTY(2);
}

/*
 * Stop -nl mode.
 */
pstop()
{

	if (inopen)
		return;
	phadnl = 0;
	linp = linb;
	draino();
	normal(normf);
	pfast &= ~1;
}

/*
 * Prep tty for open mode.
 */
ttymode
ostart()
{
	ttymode f;

	/*
	if (!intty)
		error("Open and visual must be used interactively");
	*/
	gTTY(2);
	normtty++;
#ifndef USG
	f = tty.sg_flags;
	tty.sg_flags = (normf &~ (ECHO|XTABS|CRMOD)) |
#ifdef CBREAK
							CBREAK;
#else
							RAW;
#endif
#ifdef TIOCGETC
	ttcharoff();
#endif
#else
	f = tty;
	tty = normf;
	tty.c_iflag &= ~ICRNL;
	tty.c_lflag &= ~(ECHO|ICANON);
	tty.c_oflag &= ~(TAB3|ONLCR);
	tty.c_cc[VMIN] = 1;
	tty.c_cc[VTIME] = 1;
	ttcharoff();
#endif
	sTTY(2);
	tostart();
	pfast |= 2;
	saveterm();
	return (f);
}

/* actions associated with putting the terminal in open mode */
tostart()
{
	putpad(cursor_visible);
	putpad(keypad_xmit);
	if (!value(vi_MESG)) {
		if (ttynbuf[0] == 0) {
			register char *tn;
			if ((tn=ttyname(2)) == NULL &&
			    (tn=ttyname(1)) == NULL &&
			    (tn=ttyname(0)) == NULL)
				ttynbuf[0] = 1;
			else
				strcpy(ttynbuf, tn);
		}
		if (ttynbuf[0] != 1) {
			struct stat sbuf;
			stat(ttynbuf, &sbuf);
			ttymesg = FMODE(sbuf) & 0777;
			chmod(ttynbuf, 0600);
		}
	}
}

/*
 * Turn off start/stop chars if they aren't the default ^S/^Q.
 * This is so people who make esc their start/stop don't lose.
 * We always turn off quit since datamedias send ^\ for their
 * right arrow key.
 */
#ifdef USG
ttcharoff()
{
#ifdef TCGETS
	tty.c_cc[VQUIT] = '\0';
	if (tty.c_cc[VSTART] != CTRL(q))
		tty.c_cc[VSTART] = '\0';
	if (tty.c_cc[VSTOP] != CTRL(s))
		tty.c_cc[VSTOP] = '\0';
	tty.c_cc[VSUSP] = '\0';
	tty.c_cc[VDSUSP] = '\0';
	tty.c_cc[VDISCARD] = '\0';
	tty.c_cc[VLNEXT] = '\0';
#else /*TCGETS */
	tty.c_cc[VQUIT] = '\377';
#endif /*TCGETS */
}
#else /*USG*/
#ifdef TIOCGETC
ttcharoff()
{
	nttyc.t_quitc = '\377';
	if (nttyc.t_startc != CTRL(q))
		nttyc.t_startc = '\377';
	if (nttyc.t_stopc != CTRL(s))
		nttyc.t_stopc = '\377';
#ifdef TIOCLGET
	nlttyc.t_suspc = '\377';	/* ^Z */
	nlttyc.t_dsuspc = '\377';	/* ^Y */
	nlttyc.t_flushc = '\377';	/* ^O */
	nlttyc.t_lnextc = '\377';	/* ^V */
#endif /*TIOCLGET */
}
#endif /*TIOCGETC*/
#endif /*USG*/

/*
 * Stop open, restoring tty modes.
 */
ostop(f)
	ttymode f;
{

#ifndef USG
	pfast = (f & CRMOD) == 0;
#else
	pfast = (f.c_oflag & ONLCR) == 0;
#endif
	termreset(), fgoto(), flusho();
	normal(f);
	tostop();
}

/* Actions associated with putting the terminal in the right mode. */
tostop()
{
	putpad(clr_eos);
	putpad(cursor_normal);
	putpad(keypad_local);
	if (!value(vi_MESG) && ttynbuf[0]>1)
		chmod(ttynbuf, ttymesg);
}

#ifndef CBREAK
/*
 * Into cooked mode for interruptibility.
 */
vcook()
{

	tty.sg_flags &= ~RAW;
	sTTY(2);
}

/*
 * Back into raw mode.
 */
vraw()
{

	tty.sg_flags |= RAW;
	sTTY(2);
}
#endif

/*
 * Restore flags to normal state f.
 */
normal(f)
	ttymode f;
{

	if (normtty > 0) {
		setty(f);
		normtty--;
	}
}

/*
 * Straight set of flags to state f.
 */
ttymode
setty(f)
	ttymode f;
{
	int isnorm = 0;
#ifndef USG
	register int ot = tty.sg_flags;
#else
	ttymode ot;
	ot = tty;
#endif

#ifndef USG
	if (f == normf) {
		nttyc = ottyc;
		isnorm = 1;
#ifdef TIOCLGET
		nlttyc = olttyc;
#endif
	} else
		ttcharoff();
	tty.sg_flags = f;
#else
	if (tty.c_lflag & ICANON)
		ttcharoff();
	else
		isnorm = 1;
	tty = f;
#endif
	sTTY(2);
	if (!isnorm)
		saveterm();
	return (ot);
}

gTTY(i)
	int i;
{

#ifndef USG
	ignore(gtty(i, &tty));
#ifdef TIOCGETC
	ioctl(i, TIOCGETC, &ottyc);
	nttyc = ottyc;
#endif
#ifdef TIOCGLTC
	ioctl(i, TIOCGLTC, &olttyc);
	nlttyc = olttyc;
#endif
#else
#ifdef TCGETS
	ioctl(i, TCGETS, &tty);
#else
	ioctl(i, TCGETA, &tty);
#endif
#endif
}

/*
 * sTTY: set the tty modes on file descriptor i to be what's
 * currently in global "tty".  (Also use nttyc if needed.)
 */
sTTY(i)
	int i;
{

#ifndef USG

#ifdef TIOCSETN
	/* Don't flush typeahead if we don't have to */
	ioctl(i, TIOCSETN, &tty);
#else
	/* We have to.  Too bad. */
	stty(i, &tty);
#endif

#ifdef TIOCGETC
	/* Update the other random chars while we're at it. */
	ioctl(i, TIOCSETC, &nttyc);
#endif
#ifdef TIOCSLTC
	ioctl(i, TIOCSLTC, &nlttyc);
#endif

#else
	/* USG 3 very simple: just set everything */
#ifdef TCGETS
	ioctl(i, TCSETSW, &tty);
#else
	ioctl(i, TCSETAW, &tty);
#endif
#endif
}

/*
 * Print newline, or blank if in open/visual
 */
noonl()
{

	putchar(Outchar != termchar ? ' ' : '\n');
}
