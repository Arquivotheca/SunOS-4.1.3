/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static char *sccsid = "@(#)ex_vget.c 1.1 92/07/30 SMI"; 
#endif

#include "ex.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Input routines for open/visual.
 * We handle upper case only terminals in visual and reading from the
 * echo area here as well as notification on large changes
 * which appears in the echo area.
 */

/*
 * Return the key.
 */
ungetkey(c)
	int c;		/* char --> int */
{

	if (Peekkey != ATTN)
		Peekkey = c;
}

/*
 * Unget the string of characters st.  If append is 1, append it to the
 * existing unget buffer, otherwise copy it into the unget buffer.
 */
unget(st, append)
char *st;
int append;
{
	char tmpbuf[BUFSIZ];

	if ((append ? strlen(vungetbuf) : 0) + strlen(st) > VBSIZE)
		error("Unget buffer overflowed");
	if (append)
		strcat(vungetbuf, st);
	else
		strcpy(vungetbuf, st);
	vungetp = vungetbuf;
}

/*
 * Return a keystroke, but never a ^@.
 */
getkey()
{
	register int c;		/* char --> int */

	do {
		c = getbr();
		if (c==0)
			beep();
	} while (c == 0);
	return (c);
}

/*
 * Tell whether next keystroke would be a ^@.
 */
peekbr()
{

	Peekkey = getbr();
	return (Peekkey == 0);
}

short	precbksl;

/*
 * Get a keystroke, including a ^@.
 * If an key was returned with ungetkey, that
 * comes back first.  Next comes unread input (e.g.
 * from repeating commands with .), and finally new
 * keystrokes.
 *
 * The hard work here is in mapping of \ escaped
 * characters on upper case only terminals.
 */
getbr()
{
	unsigned char ch;
	register int c, d;
	register unsigned char *colp;
	int cnt;
	static unsigned char Peek2key;
	extern short slevel, ttyindes;

getATTN:
	if (Peekkey) {
		c = Peekkey;
		Peekkey = 0;
		return (c);
	}
	if (Peek2key) {
		c = Peek2key;
		Peek2key = 0;
		return (c);
	}
	if (vungetp) {
		if (*vungetp)
			return ((unsigned char)*vungetp++);
		vungetp = 0;
	}
	if (vglobp) {
		if (*vglobp)
			return (lastvgk = (unsigned char)*vglobp++);
		lastvgk = 0;
		return (ESCAPE);
	}
	if (vmacp) {
		if (*vmacp)
			return((unsigned char)*vmacp++);
		/* End of a macro or set of nested macros */
		vmacp = 0;
		if (inopen == -1)	/* don't mess up undo for esc esc */
			vundkind = VMANY;
		inopen = 1;	/* restore old setting now that macro done */
		vch_mac = VC_NOTINMAC;
	}
	flusho();
again:
	if ((c=read(slevel == 0 ? 0 : ttyindes, (char *)&ch, 1)) != 1) {
		if (errno == EINTR)
			goto getATTN;
		error("Input read error");
	}
	c = ch;
	if (beehive_glitch && slevel==0 && c == ESCAPE) {
		if (read(0, (char *)&Peek2key, 1) != 1)
			goto getATTN;
		switch (Peek2key) {
		case 'C':	/* SPOW mode sometimes sends \EC for space */
			c = ' ';
			Peek2key = 0;
			break;
		case 'q':	/* f2 -> ^C */
			c = CTRL(c);
			Peek2key = 0;
			break;
		case 'p':	/* f1 -> esc */
			Peek2key = 0;
			break;
		}
	}

	/*
	 * The algorithm here is that of the UNIX kernel.
	 * See the description in the programmers manual.
	 */
	if (UPPERCASE) {
		if (isupper(c))
			c = tolower(c);
		if (c == '\\') {
			if (precbksl < 2)
				precbksl++;
			if (precbksl == 1)
				goto again;
		} else if (precbksl) {
			d = 0;
			if (islower(c))
				d = toupper(c);
			else {
				colp = (unsigned char *)"({)}!|^~'~";
				while (d = *colp++)
					if (d == c) {
						d = *colp++;
						break;
					} else
						colp++;
			}
			if (precbksl == 2) {
				if (!d) {
					Peekkey = c;
					precbksl = 0;
					c = '\\';
				}
			} else if (d)
				c = d;
			else {
				Peekkey = c;
				precbksl = 0;
				c = '\\';
			}
		}
		if (c != '\\')
			precbksl = 0;
	}
#ifdef TRACE
	if (trace) {
		if (!techoin) {
			tfixnl();
			techoin = 1;
			fprintf(trace, "*** Input: ");
		}
		tracec(c);
	}
#endif
	lastvgk = 0;
	return (c);
}

/*
 * Get a key, but if a delete, quit or attention
 * is typed return 0 so we will abort a partial command.
 */
getesc()
{
	register int c;

	c = getkey();
	switch (c) {

	case CTRL(v):
	case CTRL(q):
		c = getkey();
		return (c);

	case ATTN:
	case QUIT:
		ungetkey(c);
		return (0);

	case ESCAPE:
		return (0);
	}
	return (c);
}

/*
 * Peek at the next keystroke.
 */
peekkey()
{

	Peekkey = getkey();
	return (Peekkey);
}

/*
 * Read a line from the echo area, with single character prompt c.
 * A return value of 1 means the user blewit or blewit away.
 */
readecho(c)
	char c;
{
	register char *sc = cursor;
	register int (*OP)();
	bool waste;
	register int OPeek;

	if (WBOT == WECHO)
		vclean();
	else
		vclrech(0);
	splitw++;
	vgoto(WECHO, 0);
	putchar(c);
	vclreol();
	vgoto(WECHO, 1);
	cursor = linebuf; linebuf[0] = 0; genbuf[0] = c;
	if (peekbr()) {
		if (!INS[0] || (unsigned char)INS[128] == 0200) {
			INS[128] = 0;
			goto blewit;
		}
		vglobp = INS;
	}
	OP = Pline; Pline = normline;
	ignore(vgetline(0, genbuf + 1, &waste, c));
	if (Outchar == termchar)
		putchar('\n');
	vscrap();
	Pline = OP;
	if (Peekkey != ATTN && Peekkey != QUIT && Peekkey != CTRL(h)) {
		cursor = sc;
		vclreol();
		return (0);
	}
blewit:
	OPeek = Peekkey==CTRL(h) ? 0 : Peekkey; Peekkey = 0;
	splitw = 0;
	vclean();
	vshow(dot, NOLINE);
	vnline(sc);
	Peekkey = OPeek;
	return (1);
}

/*
 * A complete command has been defined for
 * the purposes of repeat, so copy it from
 * the working to the previous command buffer.
 */
setLAST()
{

	if (vungetp || vglobp || vmacp)
		return;
	lastreg = vreg;
	lasthad = Xhadcnt;
	lastcnt = Xcnt;
	*lastcp = 0;
	CP(lastcmd, workcmd);
}

/*
 * Gather up some more text from an insert.
 * If the insertion buffer oveflows, then destroy
 * the repeatability of the insert.
 */
addtext(cp)
	char *cp;
{

	if (vglobp)
		return;
	addto(INS, cp);
	if ((unsigned char)INS[128] == 0200)
		lastcmd[0] = 0;
}

setDEL()
{

	setBUF(DEL);
}

/*
 * Put text from cursor upto wcursor in BUF.
 */
setBUF(BUF)
	register char *BUF;
{
	register int c;
	register char *wp = wcursor;

	c = *wp;
	*wp = 0;
	BUF[0] = 0;
	addto(BUF, cursor);
	*wp = c;
}

addto(buf, str)
	register char *buf, *str;
{

	if ((unsigned char)buf[128] == 0200)
		return;
	if (strlen(buf) + strlen(str) + 1 >= VBSIZE) {
		buf[128] = 0200;
		return;
	}
	ignore(strcat(buf, str));
	buf[128] = 0;
}

/*
 * Note a change affecting a lot of lines, or non-visible
 * lines.  If the parameter must is set, then we only want
 * to do this for open modes now; return and save for later
 * notification in visual.
 */
noteit(must)
	bool must;
{
	register int sdl = destline, sdc = destcol;

	if (notecnt < 1 || !must && state == VISUAL)
		return (0);
	splitw++;
	if (WBOT == WECHO)
		vmoveitup(1, 1);
	vigoto(WECHO, 0);
	printf("%d %sline", notecnt, notesgn);
	if (notecnt > 1)
		putchar('s');
	if (*notenam) {
		printf(" %s", notenam);
		if (*(strend(notenam) - 1) != 'e')
			putchar('e');
		putchar('d');
	}
	vclreol();
	notecnt = 0;
	if (state != VISUAL)
		vcnt = vcline = 0;
	splitw = 0;
	if (state == ONEOPEN || state == CRTOPEN)
		vup1();
	destline = sdl; destcol = sdc;
	return (1);
}

/*
 * Ring or beep.
 * If possible, flash screen.
 */
beep()
{

	if (flash_screen && value(vi_FLASH))
		vputp(flash_screen, 0);
	else if (bell)
		vputp(bell, 0);
}

/*
 * Map the command input character c,
 * for keypads and labelled keys which do cursor
 * motions.  I.e. on an adm3a we might map ^K to ^P.
 * DM1520 for example has a lot of mappable characters.
 */

map(c,maps,commch)
	register int c;
	register struct maps *maps;
	char commch;		/* indicate if in append/insert/replace mode */
{
	register int d;
	register char *p, *q;
	char b[10];	/* Assumption: no keypad sends string longer than 10 */
	char *st;

	/*
	 * Mapping for special keys on the terminal only.
	 * BUG: if there's a long sequence and it matches
	 * some chars and then misses, we lose some chars.
	 *
	 * For this to work, some conditions must be met.
	 * 1) Keypad sends SHORT (2 or 3 char) strings
	 * 2) All strings sent are same length & similar
	 * 3) The user is unlikely to type the first few chars of
	 *    one of these strings very fast.
	 * Note: some code has been fixed up since the above was laid out,
	 * so conditions 1 & 2 are probably not required anymore.
	 * However, this hasn't been tested with any first char
	 * that means anything else except escape.
	 */
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"map(%c): ",c);
#endif
	/*
	 * If c==0, the char came from getesc typing escape.  Pass it through
	 * unchanged.  0 messes up the following code anyway.
	 */
	if (c==0)
		return(0);

	b[0] = c;
	b[1] = 0;
	for (d=0; d < MAXNOMACS && maps[d].mapto; d++) {
#ifdef MDEBUG
		if (trace)
			fprintf(trace,"\ntry '%s', ",maps[d].cap);
#endif
		if (p = maps[d].cap) {
			for (q=b; *p; p++, q++) {
#ifdef MDEBUG
				if (trace)
					fprintf(trace,"q->b[%d], ",q-b);
#endif
				if (*q==0) {
					/*
					 * Is there another char waiting?
					 *
					 * This test is oversimplified, but
					 * should work mostly. It handles the
					 * case where we get an ESCAPE that
					 * wasn't part of a keypad string.
					 */
					if ((c=='#' ? peekkey() : fastpeekkey()) == 0) {
#ifdef MDEBUG
						if (trace)
							fprintf(trace,"fpk=0: will return '%c'",c);
#endif
						/*
						 * Nothing waiting.  Push back
						 * what we peeked at & return
						 * failure (c).
						 *
						 * We want to be able to undo
						 * commands, but it's nonsense
						 * to undo part of an insertion
						 * so if in input mode don't.
						 */
#ifdef MDEBUG
						if (trace)
							fprintf(trace, "Call macpush, b %d %d %d\n", b[0], b[1], b[2]);
#endif
						macpush(&b[1],maps == arrows);
#ifdef MDEBUG
						if (trace)
							fprintf(trace, "return %d\n", c);	
#endif
						return(c);
					}
					*q = getkey();
					q[1] = 0;
				}
				if (*p != *q)
					goto contin;
			}
			macpush(maps[d].mapto,maps == arrows);
			/*
			 * For all macros performed within insert,
			 * append, or replacement mode, we must end
			 * up returning back to that mode when we
			 * return (except that append will become
			 * insert for <home> key, so cursor is not
			 * in second column).
			 *
			 * In order to preserve backward movement
			 * when leaving insert mode, an 'l' must be
			 * done to compensate for the left done by
			 * the <esc> (except when cursor is already
			 * in the first column: i.e., outcol = 0).
			 */
			 if ((maps == immacs) 
			 && strcmp(maps[d].descr, maps[d].cap)) {
				switch (commch) {
				  case 'R':
					if (!strcmp(maps[d].descr, "home"))
						st = "R";
					else
						if (outcol == 0)
							st = "R";
						else
							st = "lR"; 
					break;
				  case 'i':
					if (!strcmp(maps[d].descr, "home"))
						st = "i";
					else
						if (outcol == 0)
							st = "i";
						else
							st = "li"; 
					break;
				  case 'a':
					if (!strcmp(maps[d].descr, "home"))
						st = "i";
					else
						st = "a";
					break;
				  default:
					st = "i";
				}
				if(strlen(vmacbuf)  + strlen(st) > BUFSIZ) 
					error("Macro too long@ - maybe recursive?");
				else
					/* 
					 * Macros such as function keys are
					 * performed by leaving the insert,
					 * replace, or append mode, executing 
					 * the proper cursor movement commands
					 * and returning to the mode we are
					 * currently in (commch).
					 */
					strcat(vmacbuf, st);
			}
			c = getkey();
#ifdef MDEBUG
			if (trace)
				fprintf(trace,"Success: push(%s), return %c",maps[d].mapto, c);
#endif
			return(c);	/* first char of map string */
			contin:;
		}
	}
#ifdef MDEBUG
	if (trace)
		fprintf(trace,"Fail: push(%s), return %c", &b[1], c);
#endif
	macpush(&b[1],0);
	return(c);
}

/*
 * Push st onto the front of vmacp. This is tricky because we have to
 * worry about where vmacp was previously pointing. We also have to
 * check for overflow (which is typically from a recursive macro)
 * Finally we have to set a flag so the whole thing can be undone.
 * canundo is 1 iff we want to be able to undo the macro.  This
 * is false for, for example, pushing back lookahead from fastpeekkey(),
 * since otherwise two fast escapes can clobber our undo.
 */
macpush(st, canundo)
char *st;
int canundo;
{
	char tmpbuf[BUFSIZ];

	if (st==0 || *st==0)
		return;
#ifdef MDEBUG
	if (trace)
		fprintf(trace, "macpush(%s), canundo=%d\n",st,canundo);
#endif
	if ((vmacp ? strlen(vmacp) : 0) + strlen(st) > BUFSIZ)
		error("Macro too long@ - maybe recursive?");
	if (vmacp) {
		strcpy(tmpbuf, vmacp);
		if (!FIXUNDO)
			canundo = 0;	/* can't undo inside a macro anyway */
	}
	strcpy(vmacbuf, st);
	if (vmacp)
		strcat(vmacbuf, tmpbuf);
	vmacp = vmacbuf;
	/* arrange to be able to undo the whole macro */
	if (canundo) {
#ifdef notdef
		otchng = tchng;
		vsave();
		saveall();
		inopen = -1;	/* no need to save since it had to be 1 or -1 before */
		vundkind = VMANY;
#endif
		vch_mac = VC_NOCHANGE;
	}
}

#ifdef UNDOTRACE
visdump(s)
char *s;
{
	register int i;

	if (!trace) return;

	fprintf(trace, "\n%s: basWTOP=%d, basWLINES=%d, WTOP=%d, WBOT=%d, WLINES=%d, WCOLS=%d, WECHO=%d\n",
		s, basWTOP, basWLINES, WTOP, WBOT, WLINES, WCOLS, WECHO);
	fprintf(trace, "   vcnt=%d, vcline=%d, cursor=%d, wcursor=%d, wdot=%d\n",
		vcnt, vcline, cursor-linebuf, wcursor-linebuf, wdot-zero);
	for (i=0; i<TUBELINES; i++)
		if (vtube[i] && *vtube[i])
			fprintf(trace, "%d: '%s'\n", i, vtube[i]);
	tvliny();
}

vudump(s)
char *s;
{
	register line *p;
	char savelb[1024];

	if (!trace) return;

	fprintf(trace, "\n%s: undkind=%d, vundkind=%d, unddel=%d, undap1=%d, undap2=%d,\n",
		s, undkind, vundkind, lineno(unddel), lineno(undap1), lineno(undap2));
	fprintf(trace, "  undadot=%d, dot=%d, dol=%d, unddol=%d, truedol=%d\n",
		lineno(undadot), lineno(dot), lineno(dol), lineno(unddol), lineno(truedol));
	fprintf(trace, "  [\n");
	CP(savelb, linebuf);
	fprintf(trace, "linebuf = '%s'\n", linebuf);
	for (p=zero+1; p<=truedol; p++) {
		fprintf(trace, "%o ", *p);
		getline(*p);
		fprintf(trace, "'%s'\n", linebuf);
	}
	fprintf(trace, "]\n");
	CP(linebuf, savelb);
}
#endif

/*
 * Get a count from the keyed input stream.
 * A zero count is indistinguishable from no count.
 */
vgetcnt()
{
	register int c, cnt;

	cnt = 0;
	for (;;) {
		c = getkey();
		if (!isdigit(c))
			break;
		cnt *= 10, cnt += c - '0';
	}
	ungetkey(c);
	Xhadcnt = 1;
	Xcnt = cnt;
	return(cnt);
}

/*
 * fastpeekkey is just like peekkey but insists the character come in
 * fast (within 1 second). This will succeed if it is the 2nd char of
 * a machine generated sequence (such as a function pad from an escape
 * flavor terminal) but fail for a human hitting escape then waiting.
 */
fastpeekkey()
{
	void trapalarm();
	register int c;

	/*
	 * If the user has set notimeout, we wait forever for a key.
	 * If we are in a macro we do too, but since it's already
	 * buffered internally it will return immediately.
	 * In other cases we force this to die in 1 second.
	 * This is pretty reliable (VMUNIX rounds it to .5 - 1.5 secs,
	 * but UNIX truncates it to 0 - 1 secs) but due to system delays
	 * there are times when arrow keys or very fast typing get counted
	 * as separate.  notimeout is provided for people who dislike such
	 * nondeterminism.
	 */
		if (value(vi_TIMEOUT) && inopen >= 0) {
			signal(SIGALRM, trapalarm);
			setalarm();
		}
	CATCH
		c = peekkey();
		cancelalarm();
	ONERR
		c = 0;
	ENDCATCH
	/* Should have an alternative method based on select for 4.2BSD */
	return(c);
}

static int ftfd;
struct requestbuf {
	short time;
	short signo;
};

/*
 * Arrange for SIGALRM to come in shortly, so we don't
 * hang very long if the user didn't type anything.  There are
 * various ways to do this on different systems.
 */
setalarm()
{
	char ftname[20];
	struct requestbuf rb;

#ifdef FTIOCSET
	/*
	 * Use nonstandard "fast timer" to get better than
	 * one second resolution.  We must wait at least
	 * 1/15th of a second because some keypads don't
	 * transmit faster than this.
	 */

	/* Open ft psuedo-device - we need our own copy. */
	if (ftfd == 0) {
		strcpy(ftname, "/dev/ft0");
		while (ftfd <= 0 && ftname[7] <= '~') {
			ftfd = open(ftname, 0);
			if (ftfd <= 0)
				ftname[7] ++;
		}
	}
	if (ftfd <= 0) {	/* Couldn't open a /dev/ft? */
		alarm(1);
	} else {
		rb.time = 6;	/* 6 ticks = 100 ms > 67 ms. */
		rb.signo = SIGALRM;
		ioctl(ftfd, FTIOCSET, &rb);
	}
#else
	/*
	 * No special capabilities, so we use alarm, with 1 sec. resolution.
	 */
	alarm(1);
#endif
}

/*
 * Get rid of any impending incoming SIGALRM.
 */
cancelalarm()
{
	struct requestbuf rb;
#ifdef FTIOCSET
	if (ftfd > 0) {
		rb.time = 0;
		rb.signo = SIGALRM;
		ioctl(ftfd, FTIOCCANCEL, &rb);
	}
#endif
	alarm(0);	/* Have to do this whether or not FTIOCSET */
}

void
trapalarm() {
	alarm(0);
	if (vcatch)
		longjmp(vreslab,1);
}
