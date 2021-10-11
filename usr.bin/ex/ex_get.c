/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_get.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.8 */
#endif

#include "ex.h"
#include "ex_tty.h"

/*
 * Input routines for command mode.
 * Since we translate the end of reads into the implied ^D's
 * we have different flavors of routines which do/don't return such.
 */
static	bool junkbs;
short	lastc = '\n';

ignchar()
{
	ignore(getchar());
}

getchar()
{
	register int c;

	do
		c = getcd();
	while (!globp && c == CTRL(d));
	return (c);
}

getcd()
{
	register int c;
	extern short slevel;

again:
	c = getach();
	if (c == EOF)
		return (c);
	if (!inopen && slevel==0)
		if (!globp && c == CTRL(d))
			setlastchar('\n');
		else if (junk(c)) {
			checkjunk(c);
			goto again;
		}
	return (c);
}

peekchar()
{

	if (peekc == 0)
		peekc = getchar();
	return (peekc);
}

peekcd()
{
	if (peekc == 0)
		peekc = getcd();
	return (peekc);
}

int verbose;
getach()
{
	register int c, i, prev;
	static char inline[BUFSIZ];

	c = peekc;
	if (c != 0) {
		peekc = 0;
		return (c);
	}
	if (globp) {
		if (*globp)
			return ((unsigned char)*globp++);
		globp = 0;
		return (lastc = EOF);
	}
top:
	if (input) {
		if(c = (unsigned char)*input++)
			return (lastc = c);
		input = 0;
	}
	flush();
	if (intty) {
		c = read(0, inline, sizeof inline - 4);
		if (c < 0)
			return (lastc = EOF);
		if (c == 0 || inline[c-1] != '\n')
			inline[c++] = CTRL(d);
		if (inline[c-1] == '\n')
			noteinp();
		prev = 0;
		/* remove nulls from input buffer */
		for (i = 0; i < c; i++)
			if(inline[i] != 0)
				inline[prev++] = inline[i];
		inline[prev] = 0;
		input = inline;
		goto top;
	}
	c = read(0, inline, sizeof inline - 1);
	if(c <= 0)
		return(lastc = EOF);
	if (verbose)
		write(2, inline, c);
	inline[c] = '\0';
	input = inline;
	goto top;
}

/*
 * Input routine for insert/append/change in command mode.
 * Most work here is in handling autoindent.
 */
static	short	lastin;

gettty()
{
	register int c = 0;
	register char *cp = genbuf;
	char hadup = 0;
	int numbline();
	extern int (*Pline)();
	int offset = Pline == numbline ? 8 : 0;
	int ch;

	if (intty && !inglobal) {
		if (offset) {
			holdcm = 1;
			printf("  %4d  ", lineDOT() + 1);
			flush();
			holdcm = 0;
		}
		if (value(vi_AUTOINDENT) ^ aiflag) {
			holdcm = 1;
			if (value(vi_LISP))
				lastin = lindent(dot + 1);
			gotab(lastin + offset);
			while ((c = getcd()) == CTRL(d)) {
				if (lastin == 0 && isatty(0) == -1) {
					holdcm = 0;
					return (EOF);
				}
				lastin = backtab(lastin);
				gotab(lastin + offset);
			}
			switch (c) {

			case '^':
			case '0':
				ch = getcd();
				if (ch == CTRL(d)) {
					if (c == '0')
						lastin = 0;
					if (!over_strike) {
						putchar('\b' | QUOTE);
						putchar(' ' | QUOTE);
						putchar('\b' | QUOTE);
					}
					gotab(offset);
					hadup = 1;
					c = getchar();
				} else
					ungetchar(ch);
				break;

			case '.':
				if (peekchar() == '\n') {
					ignchar();
					noteinp();
					holdcm = 0;
					return (EOF);
				}
				break;

			case '\n':
				hadup = 1;
				break;
			}
		}
		flush();
		holdcm = 0;
	}
	if (c == 0)
		c = getchar();
	while (c != EOF && c != '\n') {
		if (cp > &genbuf[LBSIZE - 2])
			error("Input line too long");
		*cp++ = c;
		c = getchar();
	}
	if (c == EOF) {
		if (inglobal)
			ungetchar(EOF);
		return (EOF);
	}
	*cp = 0;
	cp = linebuf;
	if ((value(vi_AUTOINDENT) ^ aiflag) && hadup == 0 && intty && !inglobal) {
		lastin = c = smunch(lastin, genbuf);
		for (c = lastin; c >= value(vi_TABSTOP); c -= value(vi_TABSTOP))
			*cp++ = '\t';
		for (; c > 0; c--)
			*cp++ = ' ';
	}
	CP(cp, genbuf);
	if (linebuf[0] == '.' && linebuf[1] == 0)
		return (EOF);
	return (0);
}

/*
 * Crunch the indent.
 * Hard thing here is that in command mode some of the indent
 * is only implicit, so we must seed the column counter.
 * This should really be done differently so as to use the whitecnt routine
 * and also to hack indenting for LISP.
 */
smunch(col, ocp)
	register int col;
	char *ocp;
{
	register char *cp;

	cp = ocp;
	for (;;)
		switch (*cp++) {

		case ' ':
			col++;
			continue;

		case '\t':
			col += value(vi_TABSTOP) - (col % value(vi_TABSTOP));
			continue;

		default:
			cp--;
			CP(ocp, cp);
			return (col);
		}
}

char	*cntrlhm =	"^H discarded\n";

checkjunk(c)
	char c;
{

	if (junkbs == 0 && c == '\b') {
		write(2, cntrlhm, 13);
		junkbs = 1;
	}
}

line *
setin(addr)
	line *addr;
{

	if (addr == zero)
		lastin = 0;
	else
		getline(*addr), lastin = smunch(0, linebuf);
}
