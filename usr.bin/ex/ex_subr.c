/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_subr.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.15 */
#endif

#include "ex.h"
#include "ex_re.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Random routines, in alphabetical order.
 */

any(c, s)
	int c;
	register char *s;
{
	register int x;

	while (x = *s++)
		if (x == c)
			return (1);
	return (0);
}

backtab(i)
	register int i;
{
	register int j;

	if (value(vi_SHIFTWIDTH))
		j = i % value(vi_SHIFTWIDTH);
	if (j == 0)
		j = value(vi_SHIFTWIDTH);
	i -= j;
	if (i < 0)
		i = 0;
	return (i);
}

change()
{

	tchng++;
	chng = tchng;
}

/*
 * Column returns the number of
 * columns occupied by printing the
 * characters through position cp of the
 * current line.
 */
column(cp)
	register char *cp;
{

	if (cp == 0)
		cp = &linebuf[LBSIZE - 2];
	return (qcolumn(cp, (char *) 0));
}

/*
 * Ignore a comment to the end of the line.
 * This routine eats the trailing newline so don't call donewline().
 */
comment()
{
	register int c;

	do {
		c = getchar();
	} while (c != '\n' && c != EOF);
	if (c == EOF)
		ungetchar(c);
}

Copy(to, from, size)
	register char *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

copyw(to, from, size)
	register line *from, *to;
	register int size;
{

	if (size > 0)
		do
			*to++ = *from++;
		while (--size > 0);
}

copywR(to, from, size)
	register line *from, *to;
	register int size;
{

	while (--size >= 0)
		to[size] = from[size];
}

ctlof(c)
	int c;
{

	return (c == DELETE ? '?' : c | ('A' - 1));
}

dingdong()
{

	if (flash_screen && value(vi_FLASH))
		putpad(flash_screen);
	else if (value(vi_ERRORBELLS))
		putpad(bell);
}

fixindent(indent)
	int indent;
{
	register int i;
	register char *cp;

	i = whitecnt(genbuf);
	cp = vpastwh(genbuf);
	if (*cp == 0 && i == indent && linebuf[0] == 0) {
		genbuf[0] = 0;
		return (i);
	}
	CP(genindent(i), cp);
	return (i);
}

filioerr(cp)
	char *cp;
{
	register int oerrno = errno;

	lprintf("\"%s\"", cp);
	errno = oerrno;
	syserror(1);
}

char *
genindent(indent)
	register int indent;
{
	register char *cp;

	for (cp = genbuf; indent >= value(vi_TABSTOP); indent -= value(vi_TABSTOP))
		*cp++ = '\t';
	for (; indent > 0; indent--)
		*cp++ = ' ';
	return (cp);
}

getDOT()
{

	getline(*dot);
}

line *
getmark(c)
	register int c;
{
	register line *addr;
	
	for (addr = one; addr <= dol; addr++)
		if (names[c - 'a'] == (*addr &~ 01)) {
			return (addr);
		}
	return (0);
}

getn(cp)
	register char *cp;
{
	register int i = 0;

	while (isdigit((unsigned char)*cp))
		i = i * 10 + *cp++ - '0';
	if (*cp)
		return (0);
	return (i);
}

ignnEOF()
{
	register int c = getchar();

	if (c == EOF)
		ungetchar(c);
	else if (c=='"')
		comment();
}

iswhite(c)
	int c;
{

	return (c == ' ' || c == '\t');
}

junk(c)
	register int c;
{

	if (c && !value(vi_BEAUTIFY))
		return (0);
	if (c >= ' ' && c != DELETE)
		return (0);
	switch (c) {

	case '\t':
	case '\n':
	case '\f':
		return (0);

	default:
		return (1);
	}
}

killed()
{

	killcnt(addr2 - addr1 + 1);
}

killcnt(cnt)
	register int cnt;
{

	if (inopen) {
		notecnt = cnt;
		notenam = notesgn = "";
		return;
	}
	if (!notable(cnt))
		return;
	printf("%d lines", cnt);
	if (value(vi_TERSE) == 0) {
		printf(" %c%s", Command[0] | ' ', Command + 1);
		if (Command[strlen(Command) - 1] != 'e')
			putchar('e');
		putchar('d');
	}
	putNFL();
}

lineno(a)
	line *a;
{

	return (a - zero);
}

lineDOL()
{

	return (lineno(dol));
}

lineDOT()
{

	return (lineno(dot));
}

markDOT()
{

	markpr(dot);
}

markpr(which)
	line *which;
{

	if ((inglobal == 0 || inopen) && which <= endcore) {
		names['z'-'a'+1] = *which & ~01;
		if (inopen)
			ncols['z'-'a'+1] = cursor;
	}
}

markreg(c)
	register int c;
{

	if (c == '\'' || c == '`')
		return ('z' + 1);
	if (c >= 'a' && c <= 'z')
		return (c);
	return (0);
}

/*
 * Mesg decodes the terse/verbose strings. Thus
 *	'xxx@yyy' -> 'xxx' if terse, else 'xxx yyy'
 *	'xxx|yyy' -> 'xxx' if terse, else 'yyy'
 * All others map to themselves.
 */
char *
mesg(str)
	register char *str;
{
	register char *cp;

	str = strcpy(genbuf, str);
	for (cp = str; *cp; cp++)
		switch (*cp) {

		case '@':
			if (value(vi_TERSE))
				*cp = 0;
			else
				*cp = ' ';
			break;

		case '|':
			if (value(vi_TERSE) == 0)
				return (cp + 1);
			*cp = 0;
			break;
		}
	return (str);
}

/*VARARGS2*/
merror(seekpt, i)
#ifdef VMUNIX
	char *seekpt;
#else
#ifdef lint
	char *seekpt;
#else
	int seekpt;
#endif
#endif
	int i;
{
	register char *cp = linebuf;

	if (seekpt == 0)
		return;
	merror1(seekpt);
	if (*cp == '\n')
		putnl(), cp++;
	if (inopen > 0 && clr_eol)
		vclreol();
	if (enter_standout_mode && exit_bold)
		putpad(enter_standout_mode);
	printf(mesg(cp), i);
	if (enter_standout_mode && exit_bold)
		putpad(exit_bold);
}

merror1(seekpt)
#ifdef VMUNIX
	char *seekpt;
#else
#ifdef lint
	char *seekpt;
#else
	int seekpt;
#endif
#endif
{

#ifdef VMUNIX
	strcpy(linebuf, seekpt);
#else
	lseek(erfile, (long) seekpt, 0);
	if (read(erfile, linebuf, 128) < 2)
		CP(linebuf, "ERROR");
#endif
}

#define MAXDATA (56*1024)
morelines()
{
	register char *end;

	if ((int) sbrk(1024 * sizeof (line)) == -1) {
		if (endcore >= (line *) MAXDATA)
			return -1;
		end = (char *) MAXDATA;
		/*
		 * Ask for end+2 sice we want end to be the last used location.
		 */
		while (brk(end+2) == -1)
			end -= 64;
		if (end <= (char *) endcore)
			return -1;
		endcore = (line *) end;
	} else {
		endcore += 1024;
	}
	return (0);
}

nonzero()
{

	if (addr1 == zero) {
		notempty();
		error("Nonzero address required@on this command");
	}
}

notable(i)
	int i;
{

	return (hush == 0 && !inglobal && i > value(vi_REPORT));
}


notempty()
{

	if (dol == zero)
		error("No lines@in the buffer");
}


netchHAD(cnt)
	int cnt;
{

	netchange(lineDOL() - cnt);
}

netchange(i)
	register int i;
{
	register char *cp;

	if (i > 0)
		notesgn = cp = "more ";
	else
		notesgn = cp = "fewer ", i = -i;
	if (inopen) {
		notecnt = i;
		notenam = "";
		return;
	}
	if (!notable(i))
		return;
	printf(mesg("%d %slines@in file after %s"), i, cp, Command);
	putNFL();
}

putmark(addr)
	line *addr;
{

	putmk1(addr, putline());
}

putmk1(addr, n)
	register line *addr;
	int n;
{
	register line *markp;
	register oldglobmk;

	oldglobmk = *addr & 1;
	*addr &= ~1;
	for (markp = (anymarks ? names : &names['z'-'a'+1]);
	  markp <= &names['z'-'a'+1]; markp++)
		if (*markp == *addr)
			*markp = n;
	*addr = n | oldglobmk;
}

char *
plural(i)
	long i;
{

	return (i == 1 ? "" : "s");
}

int	qcount();
short	vcntcol;

qcolumn(lim, gp)
	register char *lim, *gp;
{
	register int x;
	int (*OO)();

	OO = Outchar;
	Outchar = qcount;
	vcntcol = 0;
	if (lim != NULL)
		x = lim[1], lim[1] = 0;
	pline(0);
	if (lim != NULL)
		lim[1] = x;
	if (gp)
		while (*gp)
			putchar(*gp++);
	Outchar = OO;
	return (vcntcol);
}

int
qcount(c)
	int c;
{

	if (c == '\t') {
		vcntcol += value(vi_TABSTOP) - vcntcol % value(vi_TABSTOP);
		return;
	}
	vcntcol++;
}

reverse(a1, a2)
	register line *a1, *a2;
{
	register line t;

	for (;;) {
		t = *--a2;
		if (a2 <= a1)
			return;
		*a2 = *a1;
		*a1++ = t;
	}
}

save(a1, a2)
	line *a1;
	register line *a2;
{
	register int more;

	if (!FIXUNDO)
		return;
#ifdef UNDOTRACE
	if (trace)
		vudump("before save");
#endif
	undkind = UNDNONE;
	undadot = dot;
	more = (a2 - a1 + 1) - (unddol - dol);
	while (more > (endcore - truedol))
		if (morelines() < 0)
			error("Out of memory@saving lines for undo - try using ed");
	if (more)
		(*(more > 0 ? copywR : copyw))(unddol + more + 1, unddol + 1,
		    (truedol - unddol));
	unddol += more;
	truedol += more;
	copyw(dol + 1, a1, a2 - a1 + 1);
	undkind = UNDALL;
	unddel = a1 - 1;
	undap1 = a1;
	undap2 = a2 + 1;
#ifdef UNDOTRACE
	if (trace)
		vudump("after save");
#endif
}

save12()
{

	save(addr1, addr2);
}

saveall()
{

	save(one, dol);
}

span()
{

	return (addr2 - addr1 + 1);
}

sync()
{

	chng = 0;
	tchng = 0;
	xchng = 0;
}


skipwh()
{
	register int wh;

	wh = 0;
	while (iswhite(peekchar())) {
		wh++;
		ignchar();
	}
	return (wh);
}

/*VARARGS2*/
smerror(seekpt, cp)
#ifdef lint
	char *seekpt;
#else
	int seekpt;
#endif
	char *cp;
{

	errcnt++;
	merror1(seekpt);
	if (inopen && clr_eol)
		vclreol();
	if (enter_standout_mode && exit_bold)
		putpad(enter_standout_mode);
	lprintf(mesg(linebuf), cp);
	if (enter_standout_mode && exit_bold)
		putpad(exit_bold);
}

char *
strend(cp)
	register char *cp;
{

	while (*cp)
		cp++;
	return (cp);
}

strcLIN(dp)
	char *dp;
{

	CP(linebuf, dp);
}

/*
 * A system error has occurred that we need to perror.
 * danger is true if we are unsure of the contents of
 * the file or our buffer, e.g. a write error in the
 * middle of a write operation, or a temp file error.
 */
syserror(danger)
int danger;
{
	register int e = errno;
	extern int sys_nerr;
	extern char *sys_errlist[];

	dirtcnt = 0;
	putchar(' ');
	if (danger)
		edited = 0;	/* for temp file errors, for example */
	if (e >= 0 && e <= sys_nerr)
		error(sys_errlist[e]);
	else
		error("System error %d", e);
}

/*
 * Return the column number that results from being in column col and
 * hitting a tab, where tabs are set every ts columns.  Work right for
 * the case where col > columns, even if ts does not divide columns.
 */
tabcol(col, ts)
int col, ts;
{
	int offset, result;

	if (col >= columns) {
		offset = columns * (col/columns);
		col -= offset;
	} else
		offset = 0;
	result = col + ts - (col % ts) + offset;
	return (result);
}

char *
vfindcol(i)
	int i;
{
	register char *cp;
	register int (*OO)() = Outchar;

	Outchar = qcount;
	ignore(qcolumn(linebuf - 1, NOSTR));
	for (cp = linebuf; *cp && vcntcol < i; cp++)
		putchar(*cp);
	if (cp != linebuf)
		cp--;
	Outchar = OO;
	return (cp);
}

char *
vskipwh(cp)
	register char *cp;
{

	while (iswhite((unsigned char)*cp) && cp[1])
		cp++;
	return (cp);
}


char *
vpastwh(cp)
	register char *cp;
{

	while (iswhite((unsigned char)*cp))
		cp++;
	return (cp);
}

whitecnt(cp)
	register char *cp;
{
	register int i;

	i = 0;
	for (;;)
		switch (*cp++) {

		case '\t':
			i += value(vi_TABSTOP) - i % value(vi_TABSTOP);
			break;

		case ' ':
			i++;
			break;

		default:
			return (i);
		}
}

#ifdef lint
Ignore(a)
	char *a;
{

	a = a;
}

Ignorf(a)
	int (*a)();
{

	a = a;
}
#endif

markit(addr)
	line *addr;
{

	if (addr != dot && addr >= one && addr <= dol)
		markDOT();
}

/*
 * The following code is defensive programming against a bug in the
 * pdp-11 overlay implementation.  Sometimes it goes nuts and asks
 * for an overlay with some garbage number, which generates an emt
 * trap.  This is a less than elegant solution, but it is somewhat
 * better than core dumping and losing your work, leaving your tty
 * in a weird state, etc.
 */
int _ovno;
void
onemt()
{
	int oovno;

	signal(SIGEMT, onemt);
	oovno = _ovno;
	/* 2 and 3 are valid on 11/40 type vi, so */
	if (_ovno < 0 || _ovno > 3)
		_ovno = 0;
	error("emt trap, _ovno is %d @ - try again");
}

/*
 * When a hangup occurs our actions are similar to a preserve
 * command.  If the buffer has not been [Modified], then we do
 * nothing but remove the temporary files and exit.
 * Otherwise, we sync the temp file and then attempt a preserve.
 * If the preserve succeeds, we unlink our temp files.
 * If the preserve fails, we leave the temp files as they are
 * as they are a backup even without preservation if they
 * are not removed.
 */
void
onhup()
{

	/*
	 * USG tty driver can send multiple HUP's!!
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	if (chng == 0) {
		cleanup(1);
		exit(++errcnt);
	}
	if (setexit() == 0) {
		if (preserve()) {
			cleanup(1);
			exit(++errcnt);
		}
	}
	exit(++errcnt);
}

/*
 * Similar to onhup.  This happens when any random core dump occurs,
 * e.g. a bug in vi.  We preserve the file and then generate a core.
 */
oncore(sig)
int sig;
{
	static int timescalled = 0;

	/*
	 * USG tty driver can send multiple HUP's!!
	 */
	signal(SIGINT, SIG_IGN);
	signal(SIGHUP, SIG_IGN);
	signal(sig, SIG_DFL);	/* Insure that we don't catch it again */
	if (timescalled++ == 0 && chng && setexit() == 0) {
		if (inopen)
			vsave();
		preserve();
		write(1, "\r\nYour file has been preserved\r\n", 32);
	}
	if (timescalled < 2) {
		normal(normf);
		cleanup(2);
		kill(getpid(), sig);	/* Resend ourselves the same signal */
		/* We won't get past here */
	}
	exit(++errcnt);
}

/*
 * An interrupt occurred.  Drain any output which
 * is still in the output buffering pipeline.
 * Catch interrupts again.  Unless we are in visual
 * reset the output state (out of -nl mode, e.g).
 * Then like a normal error (with the \n before Interrupt
 * suppressed in visual mode).
 */
void
onintr()
{
#ifndef CBREAK
	signal(SIGINT, onintr);
#else
	signal(SIGINT, inopen ? vintr : onintr);
#endif
	cancelalarm();
	draino();
	if (!inopen) {
		pstop();
		setlastchar('\n');
#ifdef CBREAK
	}
#else
	} else
		vraw();
#endif
	error("\nInterrupt" + (inopen!=0));
}

/*
 * If we are interruptible, enable interrupts again.
 * In some critical sections we turn interrupts off,
 * but not very often.
 */
setrupt()
{

	if (ruptible) {
#ifndef CBREAK
		signal(SIGINT, onintr);
#else
		signal(SIGINT, inopen ? vintr : onintr);
#endif
#ifdef SIGTSTP
		if (dosusp)
			signal(SIGTSTP, onsusp);
#endif
	}
}

preserve()
{

#ifdef VMUNIX
	tflush();
#endif
	synctmp();
	pid = fork();
	if (pid < 0)
		return (0);
	if (pid == 0) {
		close(0);
		dup(tfile);
		execlp(EXPRESERVE, "expreserve", (char *) 0);
		exit(++errcnt);
	}
	waitfor();
	if (rpid == pid && status == 0)
		return (1);
	return (0);
}

#ifndef V6
exit(i)
	int i;
{

#ifdef TRACE
	if (trace)
		fclose(trace);
#endif
	_exit(i);
}
#endif

#ifdef SIGTSTP
/*
 * We have just gotten a susp.  Suspend and prepare to resume.
 */
void
onsusp()
{
	ttymode f;
	int savenormtty;

	f = setty(normf);
	vnfl();
	putpad(exit_ca_mode);
	flush();
	resetterm();
	savenormtty = normtty;
	normtty = 0;

	(void) sigsetmask(0);
	signal(SIGTSTP, SIG_DFL);
	kill(0, SIGTSTP);

	/* the pc stops here */

	signal(SIGTSTP, onsusp);
	normtty = savenormtty;
	vcontin(0);
	setty(f);
	if (!inopen)
		error(0);
	else {
		if (vcnt < 0) {
			vcnt = -vcnt;
			if (state == VISUAL)
				vclear();
			else if (state == CRTOPEN)
				vcnt = 0;
		}
		vdirty(0, lines);
		vrepaint(cursor);
	}
}
#endif
