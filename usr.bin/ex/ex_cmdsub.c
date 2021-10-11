/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* Copyright (c) 1981 Regents of the University of California */

#ifndef lint
static	char *sccsid = "@(#)ex_cmdsub.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.14 */
#endif

#include "ex.h"
#include "ex_argv.h"
#include "ex_temp.h"
#include "ex_tty.h"
#include "ex_vis.h"

/*
 * Command mode subroutines implementing
 *	append, args, copy, delete, join, move, put,
 *	shift, tag, yank, z and undo
 */

bool	endline = 1;
line	*tad1;
static	jnoop();

/*
 * Append after line a lines returned by function f.
 * Be careful about intermediate states to avoid scramble
 * if an interrupt comes in.
 */
append(f, a)
	int (*f)();
	line *a;
{
	register line *a1, *a2, *rdot;
	int nline;

	nline = 0;
	dot = a;
	if (a == (line *) 0)
		return; /* because we would be appending prior to opening
			 * a file! 
			 */
	if(FIXUNDO && !inopen && f!=getsub) {
		undap1 = undap2 = dot + 1;
		undkind = UNDCHANGE;
	}
	while ((*f)() == 0) {
		if (truedol >= endcore) {
			if (morelines() < 0) {
				if (FIXUNDO && f == getsub) {
					undap1 = addr1;
					undap2 = addr2 + 1;
				}
				error("Out of memory@- too many lines in file");
			}
		}
		nline++;
		a1 = truedol + 1;
		a2 = a1 + 1;
		dot++;
		undap2++;
		dol++;
		unddol++;
		truedol++;
		for (rdot = dot; a1 > rdot;)
			*--a2 = *--a1;
		*rdot = 0;
		putmark(rdot);
		if (f == gettty) {
			dirtcnt++;
			TSYNC();
		}
	}
	return (nline);
}

appendnone()
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		undap1 = undap2 = addr1;
	}
}

/*
 * Print out the argument list, with []'s around the current name.
 */
pargs()
{
	register char **av = argv0, *as = args0;
	register int ac;

	for (ac = 0; ac < argc0; ac++) {
		if (ac != 0)
			putchar(' ' | QUOTE);
		if (ac + argc == argc0 - 1)
			printf("[");
		lprintf("%s", as);
		if (ac + argc == argc0 - 1)
			printf("]");
		as = av ? *++av : strend(as) + 1;
	}
	noonl();
}

/*
 * Delete lines; two cases are if we are really deleting,
 * more commonly we are just moving lines to the undo save area.
 */
delete(hush)
	bool hush;
{
	register line *a1, *a2;

	nonzero();
	if(FIXUNDO) {
		void (*dsavint)();

#ifdef UNDOTRACE
		if (trace)
			vudump("before delete");
#endif
		change();
		dsavint = signal(SIGINT, SIG_IGN);
		undkind = UNDCHANGE;
		a1 = addr1;
		squish();
		a2 = addr2;
		if (a2++ != dol) {
			reverse(a1, a2);
			reverse(a2, dol + 1);
			reverse(a1, dol + 1);
		}
		dol -= a2 - a1;
		unddel = a1 - 1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
		pkill[0] = pkill[1] = 0;
		signal(SIGINT, dsavint);
#ifdef UNDOTRACE
		if (trace)
			vudump("after delete");
#endif
	} else {
		register line *a3;
		register int i;

		change();
		a1 = addr1;
		a2 = addr2 + 1;
		a3 = truedol;
		i = a2 - a1;
		unddol -= i;
		undap2 -= i;
		dol -= i;
		truedol -= i;
		do
			*a1++ = *a2++;
		while (a2 <= a3);
		a1 = addr1;
		if (a1 > dol)
			a1 = dol;
		dot = a1;
	}
	if (!hush)
		killed();
}

deletenone()
{

	if(FIXUNDO) {
		undkind = UNDCHANGE;
		squish();
		unddel = addr1;
	}
}

/*
 * Crush out the undo save area, moving the open/visual
 * save area down in its place.
 */
squish()
{
	register line *a1 = dol + 1, *a2 = unddol + 1, *a3 = truedol + 1;

	if(FIXUNDO) {
		if (inopen == -1)
			return;
		if (a1 < a2 && a2 < a3)
			do
				*a1++ = *a2++;
			while (a2 < a3);
		truedol -= unddol - dol;
		unddol = dol;
	}
}

/*
 * Join lines.  Special hacks put in spaces, two spaces if
 * preceding line ends with '.', or no spaces if next line starts with ).
 */
static	int jcount, jnoop();

join(c)
	int c;
{
	register line *a1;
	register char *cp, *cp1;

	cp = genbuf;
	*cp = 0;
	for (a1 = addr1; a1 <= addr2; a1++) {
		getline(*a1);
		cp1 = linebuf;
		if (a1 != addr1 && c == 0) {
			while (*cp1 == ' ' || *cp1 == '\t')
				cp1++;
			if (*cp1 && cp > genbuf && cp[-1] != ' ' && cp[-1] != '\t') {
				if (*cp1 != ')') {
					*cp++ = ' ';
					if (cp[-2] == '.')
						*cp++ = ' ';
				}
			}
		}
		while (*cp++ = *cp1++)
			if (cp > &genbuf[LBSIZE-2])
				error("Line overflow|Result line of join would be too long");
		cp--;
	}
	strcLIN(genbuf);
	delete(0);
	jcount = 1;
	if (FIXUNDO)
		undap1 = undap2 = addr1;
	ignore(append(jnoop, --addr1));
	if (FIXUNDO)
		vundkind = VMANY;
}

static
jnoop()
{

	return(--jcount);
}

/*
 * Move and copy lines.  Hard work is done by move1 which
 * is also called by undo.
 */
int	getcopy();

vi_move()
{
	register line *adt;
	bool iscopy = 0;

	if (Command[0] == 'm') {
		setdot1();
		markpr(addr2 == dot ? addr1 - 1 : addr2 + 1);
	} else {
		iscopy++;
		setdot();
	}
	nonzero();
	adt = address((char*)0);
	if (adt == 0)
		serror("%s where?|%s requires a trailing address", Command);
	donewline();
	move1(iscopy, adt);
	killed();
}

move1(cflag, addrt)
	int cflag;
	line *addrt;
{
	register line *adt, *ad1, *ad2;
	int nlines;

	adt = addrt;
	nlines = (addr2 - addr1) + 1;
	if (cflag) {
		tad1 = addr1;
		ad1 = dol;
		ignore(append(getcopy, ad1++));
		ad2 = dol;
	} else {
		ad2 = addr2;
		for (ad1 = addr1; ad1 <= ad2;)
			*ad1++ &= ~01;
		ad1 = addr1;
	}
	ad2++;
	if (adt < ad1) {
		if (adt + 1 == ad1 && !cflag && !inglobal)
			error("That move would do nothing!");
		dot = adt + (ad2 - ad1);
		if (++adt != ad1) {
			reverse(adt, ad1);
			reverse(ad1, ad2);
			reverse(adt, ad2);
		}
	} else if (adt >= ad2) {
		dot = adt++;
		reverse(ad1, ad2);
		reverse(ad2, adt);
		reverse(ad1, adt);
	} else
		error("Move to a moved line");
	change();
	if (!inglobal)
		if(FIXUNDO) {
			if (cflag) {
				undap1 = addrt + 1;
				undap2 = undap1 + nlines;
				deletenone();
			} else {
				undkind = UNDMOVE;
				undap1 = addr1;
				undap2 = addr2;
				unddel = addrt;
				squish();
			}
		}
}

getcopy()
{

	if (tad1 > addr2)
		return (EOF);
	getline(*tad1++);
	return (0);
}

/*
 * Put lines in the buffer from the undo save area.
 */
getput()
{

	if (tad1 > unddol)
		return (EOF);
	getline(*tad1++);
	tad1++;
	return (0);
}

put()
{
	register int cnt;

	if (!FIXUNDO)
		error("Cannot put inside global/macro");
	cnt = unddol - dol;
	if (cnt && inopen && pkill[0] && pkill[1]) {
		pragged(1);
		return;
	}
	tad1 = dol + 1;
	ignore(append(getput, addr2));
	undkind = UNDPUT;
	notecnt = cnt;
	netchange(cnt);
}

/*
 * A tricky put, of a group of lines in the middle
 * of an existing line.  Only from open/visual.
 * Argument says pkills have meaning, e.g. called from
 * put; it is 0 on calls from putreg.
 */
pragged(kill)
	bool kill;
{
	extern char *cursor;
	register char *gp = &genbuf[cursor - linebuf];

	/*
	 * Assume the editor has:
	 *
	 *	cursor is on 'c'
	 *
	 *	file is:	1) abcd
	 *			2) efgh
	 *
	 *	undo area:	3) 1
	 *			4) 2
	 *			5) 3
	 */

	if (!kill)
		getDOT();
	
	/*
	 * Copy "abcd" into genbuf.
	 * Note that gp points to 'c'.
	 */

	strcpy(genbuf, linebuf);
	
	/*
	 * Get last line of undo area ("3") into linebuf.
	 */

	getline(*unddol);
	if (kill)
		*pkill[1] = 0;
	

	/*
	 * Concatenate trailing end of current line
	 * into the last line of undo area:
	 *	linebuf = "3cd"
	 */

	strcat(linebuf, gp);
	/*
	 * Replace the last line with what is now in linebuf.
	 * So unddol = "3cd"
	 */

	putmark(unddol);

	/*
	 * Get the first line of the undo save area into linebuf.
	 * So linebuf = "1"
	 */

	getline(dol[1]);
	if (kill)
		strcLIN(pkill[0]);
	
	/*
	 * Copy the first line of the undo save area
	 * over what is pointed to by sp.
	 *	genbuf = "ab1"
	 */

	strcpy(gp, linebuf);
	
	/*
	 * Now copy genbuf back into linebuf.
	 *	linebuf = "ab1"
	 */

	strcLIN(genbuf);

	/*
	 * Now put linebuf back into the first line
	 * of the undo save area.
	 */

	putmark(dol+1);

	/*
	 * Prepare to perform an undo which will actually
	 * do a put of multiple lines in the middle of
	 * the current line.
	 */

	undkind = UNDCHANGE;
	undap1 = dot;
	undap2 = dot + 1;
	unddel = dot - 1;
	undo(1);
}

/*
 * Shift lines, based on c.
 * If c is neither < nor >, then this is a lisp aligning =.
 */
shift(c, cnt)
	int c;
	int cnt;
{
	register line *addr;
	register char *cp;
	char *dp;
	register int i;

	if(FIXUNDO)
		save12(), undkind = UNDCHANGE;
	cnt *= value(vi_SHIFTWIDTH);
	for (addr = addr1; addr <= addr2; addr++) {
		dot = addr;
		if (c == '=' && addr == addr1 && addr != addr2)
			continue;
		getDOT();
		i = whitecnt(linebuf);
		switch (c) {

		case '>':
			if (linebuf[0] == 0)
				continue;
			cp = genindent(i + cnt);
			break;

		case '<':
			if (i == 0)
				continue;
			i -= cnt;
			cp = i > 0 ? genindent(i) : genbuf;
			break;

		default:
			i = lindent(addr);
			getDOT();
			cp = genindent(i);
			break;
		}
		if (cp + strlen(dp = vpastwh(linebuf)) >= &genbuf[LBSIZE - 2])
			error("Line too long|Result line after shift would be too long");
		CP(cp, dp);
		strcLIN(genbuf);
		putmark(addr);
	}
	killed();
}

/*
 * Find a tag in the tags file.
 * Most work here is in parsing the tags file itself.
 */
tagfind(quick)
	bool quick;
{
	char cmdbuf[BUFSIZ];
	char filebuf[FNSIZE];
	char dirbuf[FNSIZE];
	char tagfbuf[128];
	register int c, d;
	bool samef = 1;
	int tfcount = 0;
	int omagic, tl;
	char *fn, *fne;
	struct stat sbuf;
#ifdef FASTTAG
	int iof;
	char iofbuf[MAXBSIZE];
	long mid;	/* assumed byte offset */
	long top, bot;	/* length of tag file */
#endif
	char *dirend;
	extern char *rindex();

	omagic = value(vi_MAGIC);
	tl = value(vi_TAGLENGTH);
	if (!skipend()) {
		register char *lp = lasttag;

		while (!iswhite(peekchar()) && !endcmd(peekchar()))
			if (lp < &lasttag[sizeof lasttag - 2])
				*lp++ = getchar();
			else
				ignchar();
		*lp++ = 0;
		if (!endcmd(peekchar()))
badtag:
			error("Bad tag|Give one tag per line");
	} else if (lasttag[0] == 0)
		error("No previous tag");
	c = getchar();
	if (!endcmd(c))
		goto badtag;
	if (c == EOF)
		ungetchar(c);
	clrstats();

	/*
	 * Loop once for each file in tags "path".
	 */
	CP(tagfbuf, svalue(vi_TAGS));
	fne = tagfbuf - 1;
	while (fne) {
		fn = ++fne;
		while (*fne && *fne != ' ')
			fne++;
		if (*fne == 0)
			fne = 0;	/* done, quit after this time */
		else
			*fne = 0;	/* null terminate filename */
		/*
		 * Save pathname of tags file (to trailing '/') in dirbuf.
		 */
		dirend = rindex(strcpy(dirbuf, fn), '/');
		if (dirend != NULL)
			*(++dirend) = '\0';
#ifdef FASTTAG
		iof = topen(fn, iofbuf);
		if (iof == -1)
			continue;
		tfcount++;
		fstat(iof, &sbuf);
		top = sbuf.st_size;
		if (top == 0L )
			top = -1L;
		bot = 0L;
		while (top >= bot) {
#else
		/*
		 * Avoid stdio and scan tag file linearly.
		 */
		io = open(fn, 0);
		if (io<0)
			continue;
		tfcount++;
		if (fstat(io, &sbuf) < 0)
			bsize = LBSIZE;
		else {
			bsize = min(sbuf.st_blksize, MAXBSIZE);
			if (bsize <= 0)
				bsize = LBSIZE;
		}
		while (getfile() == 0) {
#endif
			/* loop for each tags file entry */
			register char *cp = linebuf;
			register char *lp = lasttag;
			char *oglobp;

#ifdef FASTTAG
			mid = (top + bot) / 2;
			tseek(iof, mid);
			if (mid > 0)	/* to get first tag in file to work */
				/* scan to next \n */
				if(tgets(linebuf, sizeof linebuf, iof)==NULL)
					goto goleft;
			/* get the line itself */
			if(tgets(linebuf, sizeof linebuf, iof)==NULL)
				goto goleft;
#endif
			while (*cp && *lp == *cp)
				cp++, lp++;
			/*
			 * This if decides whether there is a tag match.
			 *  A positive taglength means that a
			 *  match is found if the tag given matches at least
			 *  taglength chars of the tag found.
			 *  A taglength of greater than 511 means that a
			 *  match is found even if the tag given is a proper
			 *  prefix of the tag found.  i.e. "ab" matches "abcd"
			 */
#ifdef FASTTAG
			if (*lp == 0 && (iswhite((unsigned char)*cp)
			    || tl > 511 || tl > 0 && lp-lasttag >= tl)) {
				/*
				 * Found a match.  Force selection to be
				 *  the first possible.
				 */
				if ( mid == bot  &&  mid == top ) {
					; /* found first possible match */
				}
				else {
					/* postpone final decision. */
					top = mid;
					continue;
				}
			}
			else {
				if (*lp > *cp)
					bot = mid + 1;
				else
goleft:
					top = mid - 1;
				continue;
			}
#else
			if (*lp == 0 && (iswhite((unsigned char)*cp)
			    || tl > 511 || tl > 0 && lp-lasttag >= tl)) {
				; /* Found it. */
			}
			else {
				/* Not this tag.  Try the next */
				continue;
			}
#endif
			/*
			 * We found the tag.  Decode the line in the file.
			 */
#ifdef FASTTAG
			tclose(iof);
#else
			close(io);
#endif
			/* Rest of tag if abbreviated */
			while (!iswhite((unsigned char)*cp))
				cp++;

			/* name of file */
			while (*cp && iswhite((unsigned char)*cp))
				cp++;
			if (!*cp)
badtags:
				serror("%s: Bad tags file entry", lasttag);
			lp = filebuf;
			while (*cp && *cp != ' ' && *cp != '\t') {
				if (lp < &filebuf[sizeof filebuf - 2])
					*lp++ = *cp;
				cp++;
			}
			*lp++ = 0;

			if (*cp == 0)
				goto badtags;
			if (dol != zero) {
				/*
				 * Save current position in 't for ^^ in visual.
				 */
				names['t'-'a'] = *dot &~ 01;
				if (inopen) {
					extern char *ncols['z'-'a'+2];
					extern char *cursor;

					ncols['t'-'a'] = cursor;
				}
			}
#ifdef TAG_STACK 
			if (*savedfile) {
				savetag(savedfile);
			}
#endif 
			strcpy(cmdbuf, cp);

			/*
			 * If target file has a relative pathname and
			 * doesn't exist, try prepending the current
			 * tagfile's pathname to it.
			 */
			if ((*filebuf != '/') && (dirend != NULL) &&
			    access(filebuf,0) &&
			    (strlen(filebuf)+strlen(dirbuf) < sizeof filebuf)) {
				strcat(dirbuf, filebuf);
				strcpy(filebuf, dirbuf);
				*dirend = '\0';
			}

			if (strcmp(filebuf, savedfile) || !edited) {
				char cmdbuf2[sizeof filebuf + 10];

				/* Different file.  Do autowrite & get it. */
				if (!quick) {
					ckaw();
					if (chng && dol > zero) {
#ifdef TAG_STACK
						unsavetag();
#endif
						error("No write@since last change (:tag! overrides)");
					}
				}
				oglobp = globp;
				strcpy(cmdbuf2, "e! ");
				strcat(cmdbuf2, filebuf);
				globp = cmdbuf2;
				d = peekc; ungetchar(0);
				commands(1, 1);
				peekc = d;
				globp = oglobp;
				value(vi_MAGIC) = omagic;
				samef = 0;
			}

			/*
			 * Look for pattern in the current file.
			 */
			oglobp = globp;
			globp = cmdbuf;
			d = peekc; ungetchar(0);
			if (samef)
				markpr(dot);
			/*
			 * BUG: if it isn't found (user edited header
			 * line) we get left in nomagic mode.
			 */
			value(vi_MAGIC) = 0;
			commands(1, 1);
			peekc = d;
			globp = oglobp;
			value(vi_MAGIC) = omagic;
			return;
		}	/* end of "for each tag in file" */

		/*
		 * No such tag in this file.  Close it and try the next.
		 */
#ifdef FASTTAG
		tclose(iof);
#else
		close(io);
#endif
	}	/* end of "for each file in path" */
	if (tfcount <= 0)
		error("No tags file");
	else
		serror("%s: No such tag@in tags file", lasttag);
}

/*
 * Save lines from addr1 thru addr2 as though
 * they had been deleted.
 */
yank()
{

	if (!FIXUNDO)
		error("Can't yank inside global/macro");
	save12();
	undkind = UNDNONE;
	killcnt(addr2 - addr1 + 1);
}

/*
 * z command; print windows of text in the file.
 *
 * If this seems unreasonably arcane, the reasons
 * are historical.  This is one of the first commands
 * added to the first ex (then called en) and the
 * number of facilities here were the major advantage
 * of en over ed since they allowed more use to be
 * made of fast terminals w/o typing .,.22p all the time.
 */
bool	zhadpr;
bool	znoclear;
short	zweight;

zop(hadpr)
	int hadpr;
{
	register int c, nlines, op;
	bool excl;

	zhadpr = hadpr;
	notempty();
	znoclear = 0;
	zweight = 0;
	excl = exclam();
	switch (c = op = getchar()) {

	case '^':
		zweight = 1;
	case '-':
	case '+':
		while (peekchar() == op) {
			ignchar();
			zweight++;
		}
	case '=':
	case '.':
		c = getchar();
		break;

	case EOF:
		znoclear++;
		break;

	default:
		op = 0;
		break;
	}
	if (isdigit(c)) {
		nlines = c - '0';
		for(;;) {
			c = getchar();
			if (!isdigit(c))
				break;
			nlines *= 10;
			nlines += c - '0';
		}
		if (nlines < lines)
			znoclear++;
		value(vi_WINDOW) = nlines;
		if (op == '=')
			nlines += 2;
	} else
		nlines = op == EOF ? value(vi_SCROLL) : excl ? lines - 1 : 2*value(vi_SCROLL);
	if (inopen || c != EOF) {
		ungetchar(c);
		donewline();
	}
	addr1 = addr2;
	if (addr2 == 0 && dot < dol && op == 0)
		addr1 = addr2 = dot+1;
	setdot();
	zop2(nlines, op);
}

zop2(nlines, op)
	register int nlines;
	register int op;
{
	register line *split;

	split = NULL;
	switch (op) {

	case EOF:
		if (addr2 == dol)
			error("\nAt EOF");
	case '+':
		if (addr2 == dol)
			error("At EOF");
		addr2 += nlines * zweight;
		if (addr2 > dol)
			error("Hit BOTTOM");
		addr2++;
	default:
		addr1 = addr2;
		addr2 += nlines-1;
		dot = addr2;
		break;

	case '=':
	case '.':
		znoclear = 0;
		nlines--;
		nlines >>= 1;
		if (op == '=')
			nlines--;
		addr1 = addr2 - nlines;
		if (op == '=')
			dot = split = addr2;
		addr2 += nlines;
		if (op == '.') {
			markDOT();
			dot = addr2;
		}
		break;

	case '^':
	case '-':
		addr2 -= nlines * zweight;
		if (addr2 < one)
			error("Hit TOP");
		nlines--;
		addr1 = addr2 - nlines;
		dot = addr2;
		break;
	}
	if (addr1 <= zero)
		addr1 = one;
	if (addr2 > dol)
		addr2 = dol;
	if (dot > dol)
		dot = dol;
	if (addr1 > addr2)
		return;
	if (op == EOF && zhadpr) {
		getline(*addr1);
		putchar('\r' | QUOTE);
		shudclob = 1;
	} else if (znoclear == 0 && clear_screen != NOSTR && !inopen) {
		flush1();
		vclear();
	}
	if (addr2 - addr1 > 1)
		pstart();
	if (split) {
		plines(addr1, split - 1, 0);
		splitit();
		plines(split, split, 0);
		splitit();
		addr1 = split + 1;
	}
	plines(addr1, addr2, 0);
}

static
splitit()
{
	register int l;

	for (l = columns > 80 ? 40 : columns / 2; l > 0; l--)
		putchar('-');
	putnl();
}

plines(adr1, adr2, movedot)
	line *adr1;
	register line *adr2;
	bool movedot;
{
	register line *addr;

	pofix();
	for (addr = adr1; addr <= adr2; addr++) {
		getline(*addr);
		pline(lineno(addr));
		if (inopen)
			putchar('\n' | QUOTE);
		if (movedot)
			dot = addr;
	}
}

pofix()
{

	if (inopen && Outchar != termchar) {
		vnfl();
		setoutt();
	}
}

/*
 * Command level undo works easily because
 * the editor has a unique temporary file
 * index for every line which ever existed.
 * We don't have to save large blocks of text,
 * only the indices which are small.  We do this
 * by moving them to after the last line in the
 * line buffer array, and marking down info
 * about whence they came.
 *
 * Undo is its own inverse.
 */
undo(c)
	bool c;
{
	register int i, k;
	register line *jp, *kp, *j;
	line *dolp1, *newdol, *newadot;

#ifdef UNDOTRACE
	if (trace)
		vudump("before undo");
#endif
	if (inglobal && inopen <= 0)
		error("Can't undo in global@commands");
	
	/*
	 * Unless flag indicates a forced undo, make sure
	 * there really was a change before trying to undo it.
	 */

	if (!c)
		somechange();
	
	/*
	 * Update change flags.
	 */

	pkill[0] = pkill[1] = 0;
	change();
	if (undkind == UNDMOVE) {
 		/*
		 * Command to be undone is a move command.
		 * This is handled as a special case by noting that
		 * a move "a,b m c" can be inverted by another move.
		 */
		if ((i = (jp = unddel) - undap2) > 0) {
			/*
			 * when c > b inverse is a+(c-b),c m a-1
			 */
			addr2 = jp;
			addr1 = (jp = undap1) + i;
			unddel = jp-1;
		} else {
			/*
			 * when b > c inverse is  c+1,c+1+(b-a) m b
			 */
			addr1 = ++jp;
			addr2 = jp + ((unddel = undap2) - undap1);
		}
		kp = undap1;
		move1(0, unddel);
		dot = kp;
		Command = "move";
		killed();
	} else {
		int cnt;

		newadot = dot;
		cnt = lineDOL();
		newdol = dol;
		dolp1 = dol + 1;
		/*
		 * Command to be undone is a non-move.
		 * All such commands are treated as a combination of
		 * a delete command and a append command.
		 * We first move the lines appended by the last command
		 * from undap1 to undap2-1 so that they are just before the
		 * saved deleted lines.
		 *
		 * Assume the editor has:
		 *
		 * 	cursor is on 'c'
		 *
		 *	(just change lines 5-8)
		 *
		 *	file is:	1) ab
		 *			2) cd
		 *			3) ef
		 *			4) gh
		 *	undap1:		5) 12
		 *			6) 34
		 *			7) 56
		 *			8) 78
		 *	undap2:		9) qr
		 *		       10) st
		 *		       11) uv
		 *		       12) wx
		 *	dol:	       13) yz
		 *
		 *	    UNDO AREA:
		 *	dol+1:		5) ij
		 *			6) kl
		 *			7) mn
		 *	unddol:		8) op
		 */

		/*
		 * If this is a change (not a delete/put),
		 * then we must move the text between undap1 and undap2
		 * and it must not be at the bottom of the file
		 */
		
		if ((i = (kp = undap2) - (jp = undap1)) > 0) {
			if (kp != dolp1) {

		/*
		 * FILE:     LINE    INITIAL   REV1   REV2   REV3
		 *
		 *	      1)       ab	ab     ab     ab
		 *	      2)       cd       cd     cd     cd
		 *            3)       ef       ef     ef     ef
		 * unddel:    4)       gh       gh     gh     gh
		 * undap1:    5)       12       78     78     qr
		 *            6)       34       56     56     st
		 *            7)       56       34     34     uv
		 *            8)       78       12     12     wx
		 * undap2:    9)       qr       qr     yz     yz
		 *           10)       st       st     wx     12
		 *           11)       uv       uv     uv     34
		 *           12)       wx       wx     st     56
		 * dol:      13)       yz       yz     qr     78
		 *
		 *	UNDO AREA:
		 * dol+1:     5)       ij       ij     ij     ij
		 *            6)       kl       kl     kl     kl
		 *	      7)       mn       mn     mn     mn
		 * unddol:    8)       op       op     op     op
		 */

				reverse(jp, kp);
				reverse(kp, dolp1);
				reverse(jp, dolp1);
			}
			/*
			 * Unddel, the line just before the spot where this
			 * test was deleted, may have moved. Account for 
			 * this in restoration of saved deleted lines.
			 */
			if (unddel >= jp)
				unddel -= i;
			
			/*
			 * The last line (dol) may have changed,
			 * account for this.
			 */
			 newdol -= i;
			
			/*
			 * For the case where no lines are restored, dot
			 * is the line before the first line deleted.
			 */
			dot = jp-1;
		}
		/*
		 * Now put the deleted lines, if any, back where they were.
		 * Basic operation is: dol+1,unddol m unddel
		 */
		if (undkind == UNDPUT) {
			unddel = undap1 - 1;
			squish();
		}
		
		/*
		 * Set jp to the line where deleted text is to be added.
		 */
		jp = unddel + 1;
		
		/*
		 * Set kp to end of undo save area.
		 *
		 * If there is any deleted text to be added, do reverses.
		 */
		
		if ((i = (kp = unddol) - dol) > 0) {
			
			/*
			 * If deleted lines are not to be appended
			 * to the bottom of the file...
			 */
			 
			 if (jp != dolp1) {
				/*
				 * FILE:   LINE   START   REV1   REV2   REV3
				 *          1)     ab      ab     ab     ab
				 *          2)     cd      cd     cd     cd
				 *          3)     ef      ef     ef     ef
				 * unddel:  4)     gh      gh     gh     gh
				 * undap1:  5)     qr      78     78     ij
				 *          6)     st      56     56     kl
				 *          7)     uv      34     34     mn
				 *          8)     wx      12     12     op
				 * undap2:  9)     yz      yz     yz     qr
				 *         10)     12      wx     wx     st
				 *         11)     34      uv     uv     uv
				 *         12)     56      st     st     wx
				 * dol:    13)     78      qr     qr     yz
				 *
				 * UNDO AREA:
				 * dol+1:  5)      ij      ij     op     12
				 *         6)      kl      kl     mn     34
				 *         7)      mn      mn     kl     56
				 * unddol: 8)      op      op     ij     78
				 */
				 
				 reverse(jp, dolp1);
				reverse(dolp1, ++kp);
				reverse(jp, kp);
			}
			/*
			 * Account for possible forward motion of the target
			 * (where the deleted lines were restored) for after
			 * restoration of the deleted lines.
			 */
			if (undap1 >= jp)
				undap1 += i;
			/*
			 * Dot is the first resurrected line.
			 */
			dot = jp;
			
			/*
			 * Account for a shift in the last line (dol).
			 */
			 
			 newdol += i;
		}
		/*
		 * Clean up so we are invertible
		 */
		unddel = undap1 - 1;
		undap1 = jp;
		undap2 = jp + i;
		dol = newdol;
		netchHAD(cnt);
		if (undkind == UNDALL) {
			dot = undadot;
			undadot = newadot;
		} else
			undkind = UNDCHANGE;
 		/*
 		 * Now relocate all marks for lines that were modified,
 		 * since the marks point to lines whose address has 
 		 * been modified from the save area to the current 
 		 * area
 		 */
 			
 		for (j=unddol; j> dol; j--) 
 			for (k=0; k<=25; k++) 
 				if (names[k] == *(j)) 
 					names[k]= *((undap1+(j-dolp1)) );
	}
	/*
	 * Defensive programming - after a munged undadot.
	 * Also handle empty buffer case.
	 */
	if ((dot <= zero || dot > dol) && dot != dol)
		dot = one;
#ifdef UNDOTRACE
	if (trace)
		vudump("after undo");
#endif
}

/*
 * Be (almost completely) sure there really
 * was a change, before claiming to undo.
 */
somechange()
{
	register line *ip, *jp;

	switch (undkind) {

	case UNDMOVE:
		return;

	case UNDCHANGE:
		if (undap1 == undap2 && dol == unddol)
			break;
		return;

	case UNDPUT:
		if (undap1 != undap2)
			return;
		break;

	case UNDALL:
		if (unddol - dol != lineDOL())
			return;
		for (ip = one, jp = dol + 1; ip <= dol; ip++, jp++)
			if ((*ip &~ 01) != (*jp &~ 01))
				return;
		break;

	case UNDNONE:
		error("Nothing to undo");
	}
	error("Nothing changed|Last undoable command didn't change anything");
}

/*
 * Map command:
 * map src dest
 */
mapcmd(un, ab)
	int un;	/* true if this is unmap command */
	int ab;	/* true if this is abbr command */
{
	char lhs[100], rhs[100];	/* max sizes resp. */
	register char *p;
	register int c;		/* char --> int */
	char *dname;
	struct maps *mp;	/* the map structure we are working on */

	mp = ab ? abbrevs : exclam() ? immacs : arrows;
	if (skipend()) {
		int i;

		/* print current mapping values */
		if (peekchar() != EOF)
			ignchar();
		if (un)
			error("Missing lhs");
		if (inopen)
			pofix();
		for (i=0; i< MAXNOMACS && mp[i].mapto; i++)
			if (mp[i].cap) {
				lprintf("%s", mp[i].descr);
				putchar('\t');
				lprintf("%s", mp[i].cap);
				putchar('\t');
				lprintf("%s", mp[i].mapto);
				putNFL();
			}
		return;
	}

	ignore(skipwh());
	for (p=lhs; ; ) {
		c = getchar();
		if (c == CTRL(v)) {
			c = getchar();
		} else if (!un && any(c, " \t")) {
			/* End of lhs */
			break;
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			if (un) {
				donewline();
				*p = 0;
				addmac(lhs, NOSTR, NOSTR, mp);
				return;
			} else
				error("Missing rhs");
		}
		*p++ = c;
	}
	*p = 0;

	if (skipend())
		error("Missing rhs");
	for (p=rhs; ; ) {
		c = getchar();
		if (c == CTRL(v)) {
			c = getchar();
		} else if (endcmd(c) && c!='"') {
			ungetchar(c);
			break;
		}
		*p++ = c;
	}
	*p = 0;
	donewline();
	/*
	 * Special hack for function keys: #1 means key f1, etc.
	 * If the terminal doesn't have function keys, we just use #1.
	 */
	if (lhs[0] == '#') {
		char *fnkey;
		char *fkey();
		char funkey[3];

		fnkey = fkey(lhs[1] - '0');
		funkey[0] = 'f'; funkey[1] = lhs[1]; funkey[2] = 0;
		if (fnkey)
			strcpy(lhs, fnkey);
		dname = funkey;
	} else {
		dname = lhs;
	}
	addmac(lhs,rhs,dname,mp);
}

/*
 * Add a macro definition to those that already exist. The sequence of
 * chars "src" is mapped into "dest". If src is already mapped into something
 * this overrides the mapping. There is no recursion. Unmap is done by
 * using NOSTR for dest.  Dname is what to show in listings.  mp is
 * the structure to affect (arrows, etc).
 */
addmac(src,dest,dname,mp)
	register char *src, *dest, *dname;
	register struct maps *mp;
{
	register int slot, zer;

#ifdef UNDOTRACE
	if (trace)
		fprintf(trace, "addmac(src='%s', dest='%s', dname='%s', mp=%x\n", src, dest, dname, mp);
#endif
	if (dest && mp==arrows) {
		/*
		 * Prevent tail recursion. We really should be
		 * checking to see if src is a suffix of dest
		 * but this makes mapping involving escapes that
		 * is reasonable mess up.
		 */
		if (src[1] == 0 && src[0] == dest[strlen(dest)-1])
			error("No tail recursion");
		/*
		 * We don't let the user rob himself of ":", and making
		 * multi char words is a bad idea so we don't allow it.
		 * Note that if user sets mapinput and maps all of return,
		 * linefeed, and escape, he can hurt himself. This is
		 * so weird I don't bother to check for it.
		 */
		if (isalpha((unsigned char)src[0]) && src[1] || any(src[0],":"))
			error("Too dangerous to map that");
	}
	else if (dest) {
		/* check for tail recursion in input mode: fussier */
		if (eq(src, dest+strlen(dest)-strlen(src)))
			error("No tail recursion");
	}
	/*
	 * If the src were null it would cause the dest to
	 * be mapped always forever. This is not good.
	 */
	if (src == NOSTR || src[0] == 0)
		error("Missing lhs");

	/* see if we already have a def for src */
	zer = -1;
	for (slot=0; slot < MAXNOMACS && mp[slot].mapto; slot++) {
		if (mp[slot].cap) {
			if (eq(src, mp[slot].cap) || eq(src, mp[slot].mapto))
				break;	/* if so, reuse slot */
		} else {
			zer = slot;	/* remember an empty slot */
		}
	}
	
	if (slot >= MAXNOMACS)
		error("Too many macros");

	if (dest == NOSTR) {
		/* unmap */
		if (mp[slot].cap) {
			mp[slot].cap = NOSTR;
			mp[slot].descr = NOSTR;
		} else {
			error("Not mapped|That macro wasn't mapped");
		}
		return;
	}

	/* reuse empty slot, if we found one and src isn't already defined */
	if (zer >= 0 && mp[slot].mapto == 0)
		slot = zer;

	/* if not, append to end */
	if (msnext == 0)	/* first time */
		msnext = mapspace;
	/* Check is a bit conservative, we charge for dname even if reusing src */
	if (msnext - mapspace + strlen(dest) + strlen(src) + strlen(dname) + 3 > MAXCHARMACS)
		error("Too much macro text");
	CP(msnext, src);
	mp[slot].cap = msnext;
	msnext += strlen(src) + 1;	/* plus 1 for null on the end */
	CP(msnext, dest);
	mp[slot].mapto = msnext;
	msnext += strlen(dest) + 1;
	if (dname) {
		CP(msnext, dname);
		mp[slot].descr = msnext;
		msnext += strlen(dname) + 1;
	} else {
		/* default descr to string user enters */
		mp[slot].descr = src;
	}
}

/*
 * Implements macros from command mode. c is the buffer to
 * get the macro from.
 */
cmdmac(c)
char c;
{
	char macbuf[BUFSIZ];
	line *ad, *a1, *a2;
	char *oglobp;
	short pk;
	bool oinglobal;

	lastmac = c;
	oglobp = globp;
	oinglobal = inglobal;
	pk = peekc; peekc = 0;
	if (inglobal < 2)
		inglobal = 1;
	regbuf(c, macbuf, sizeof(macbuf));
	a1 = addr1; a2 = addr2;
	for (ad=a1; ad<=a2; ad++) {
		globp = macbuf;
		dot = ad;
		commands(1,1);
	}
	globp = oglobp;
	inglobal = oinglobal;
	peekc = pk;
}

char *
vgetpass(prompt)
char *prompt;
{
	register char *p;
	register c;
	static char pbuf[9];
	char *getpass();

	/* In ex mode, let the system hassle with setting no echo */
	if (!inopen)
		return getpass(prompt);
	printf("%s", prompt); flush();
	for (p=pbuf; (c = getkey())!='\n' && c!=EOF && c!='\r';) {
		if (p < &pbuf[8])
			*p++ = c;
	}
	*p = '\0';
	return(pbuf);
}

#ifdef TAG_STACK
#define TSTACKSIZE 20
struct tagstack {
	line *tag_line;
	char *tag_file;
} tagstack[TSTACKSIZE];
static int tag_depth = 0;

static char tag_buf[ 1024 ];
static char *tag_end = tag_buf;

savetag( name )	/* saves location where we are */
char *name;
{
	if( !value(vi_TAGSTACK) )
		return;
	if(tag_depth >= TSTACKSIZE) {
		error("Tag stack too deep.");
	}
	if( strlen( name ) + 1 + tag_end >= &tag_buf[1024]) {
		error("Too many tags.");
	}
	tagstack[tag_depth].tag_line = dot;
	tagstack[tag_depth++].tag_file = tag_end;
	while(*tag_end++ = *name++)
		;
}

/*
 * Undo a "savetag".
 */
unsavetag()
{
	if (!value(vi_TAGSTACK))
		return;
	if (tag_depth > 0)
		tag_end = tagstack[--tag_depth].tag_file;
}

poptag(quick)	/* puts us back where we came from */
bool quick;
{
	char cmdbuf[100];
	char *oglobp;
	int d;

	if( !value(vi_TAGSTACK) ) {	/* reset the stack */
		tag_end = tag_buf;
		d = tag_depth;
		tag_depth = 0;
		if(d == 0)
			error("Tagstack not enabled.");
		else
			return;
	}
	if( !tag_depth )
		error("Tagstack empty.");
	
	/* change to old file */
	if (strcmp(tagstack[tag_depth-1].tag_file, savedfile) ) {
		if (!quick) {
			ckaw();
			if (chng && dol > zero)
				error("No write@since last change (:pop! overrides)");
		}
		oglobp = globp;
		strcpy(cmdbuf, "e ");
		strcat(cmdbuf, tagstack[tag_depth-1].tag_file);
		globp = cmdbuf;
		d = peekc; ungetchar(0);
		commands(1, 1);
		peekc = d;
		globp = oglobp;
	}
		markpr(dot);
	/* set line number */
	dot = tagstack[--tag_depth].tag_line;
	tag_end = tagstack[tag_depth].tag_file;
}
#endif
