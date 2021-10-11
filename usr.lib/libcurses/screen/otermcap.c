/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)otermcap.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

/* Copyright (c) 1979 Regents of the University of California	*/
/* Modified to:							*/
/* 1) remember the name of the first tc= parameter		*/
/*	encountered during parsing.				*/
/* 2) handle multiple invocations of tgetent().			*/
/* 3) tskip() is now available outside of the library.		*/
/* 4) remember $TERM name for error messages.			*/
/* 5) have a larger buffer.					*/
/* 6) really fix the bug that 5) got around. This fix by	*/
/*		Marion Hakanson, orstcs!hakanson		*/


#include "otermcap.h"
#define MAXHOP	32	/* max number of tc= indirections */

#include <stdio.h>
#include <ctype.h>
/* #include "uparm.h"	- TLH all uparm.h had that was needed was E_TERMCAP */
#define E_TERMCAP "/etc/termcap"

/*
 * termcap - routines for dealing with the terminal capability data base
 *
 * BUG:		Should use a "last" pointer in tbuf, so that searching
 *		for capabilities alphabetically would not be a n**2/2
 *		process when large numbers of capabilities are given.
 * Note:	If we add a last pointer now we will screw up the
 *		tc capability. We really should compile termcap.
 *
 * Essentially all the work here is scanning and decoding escapes
 * in string capabilities.  We don't use stdio because the editor
 * doesn't, and because living w/o it is not hard.
 */

static	char *tbuf;
static	int hopcount;	/* detect infinite loops in termcap, init 0 */
char	*tskip();
char	*otgetstr();
char	*tdecode();
extern char	*strcpy(), *getenv(), *strncpy();
extern int	strlen(), strcmp();

/* Tony Hansen */
int	TLHtcfound = 0;
char	TLHtcname[16];
static	char *termname;

/*
 * Get an entry for terminal name in buffer bp,
 * from the termcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
otgetent(bp, name)
	char *bp, *name;
{
	/* Tony Hansen */
	int ret;
	TLHtcfound = 0;
	hopcount = 0;
	termname = name;
	ret = _tgetent(bp, name);
	/*
	  There is some sort of bug in the check way down below to prevent
	  buffer overflow. I really don't want to track it down, so I
	  upped the standard buffer size and check here to see if the created
	  buffer is larger than the old buffer size.
	*/
	if (strlen(bp) >= 1024)
	    (void) fprintf (stderr,
	        "tgetent(): TERM=%s: Termcap entry is too long.\n",
	        termname);
	return ret;
}

static _tgetent(bp, name)
	char *bp, *name;
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[TBUFSIZE];
	char *cp2;
	int tf;

	tbuf = bp;
	tf = 0;
#ifndef V6
	cp = getenv("TERMCAP");
	/*
	 * TERMCAP can have one of two things in it. It can be the
	 * name of a file to use instead of /etc/termcap. In this
	 * case it better start with a "/". Or it can be an entry to
	 * use so we don't have to read the file. In this case it
	 * has to already have the newlines crunched out.
	 */
	if (cp && *cp) {
		if (*cp!='/') {
			cp2 = getenv("TERM");
			if (cp2==(char *) 0 || strcmp(name,cp2)==0) {
				(void) strcpy(bp,cp);
				return(tnchktc());
			} else {
				tf = open(E_TERMCAP, 0);
			}
		} else
			tf = open(cp, 0);
	}
	if (tf==0)
		tf = open(E_TERMCAP, 0);
#else
	tf = open(E_TERMCAP, 0);
#endif
	if (tf < 0)
		return (-1);
	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read(tf, ibuf, TBUFSIZE);
				if (cnt <= 0) {
					(void) close(tf);
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+TBUFSIZE) {
				(void) fprintf (stderr,
				    "tgetent(): TERM=%s: Termcap entry too long\n",
				    termname);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (tnamatch(name)) {
			(void) close(tf);
			return(tnchktc());
		}
	}
}

/*
 * tnchktc: check the last entry, see if it's tc=xxx. If so,
 * recursively find xxx and append that entry (minus the names)
 * to take the place of the tc=xxx entry. This allows termcap
 * entries to say "like an HP2621 but doesn't turn on the labels".
 * Note that this works because of the left to right scan.
 */
tnchktc()
{
	register char *p, *q;
#define TERMNAMESIZE 16
	char tcname[TERMNAMESIZE];	/* name of similar terminal */
	char tcbuf[TBUFSIZE];
	char *holdtbuf = tbuf;
	int l;

	p = tbuf + strlen(tbuf) - 2;	/* before the last colon */
	while (*--p != ':')
		if (p<tbuf) {
			(void) fprintf (stderr,
			    "tnchktc(): TERM=%s: Bad termcap entry\n",
			    termname);
			return (0);
		}
	p++;
	/* p now points to beginning of last field */
	if (p[0] != 't' || p[1] != 'c')
		return(1);
	(void) strncpy(tcname,p+3, TERMNAMESIZE);	/* TLH */
	q = tcname;
	while (*q && *q != ':')
		q++;
	*q = 0;
	if (++hopcount > MAXHOP) {
		(void) fprintf (stderr, 
		    "tnchktc(): TERM=%s: Infinite tc= loop\n", 
		    termname);
		return (0);
	}
	if (_tgetent(tcbuf, tcname) != 1)
		return(0);
	/* Tony Hansen */
	TLHtcfound++;
	(void) strcpy (TLHtcname, tcname);

	for (q=tcbuf; *q != ':'; q++)
		;
	l = p - holdtbuf + strlen(q);
	if (l > TBUFSIZE) {
		(void) fprintf (stderr, 
		    "tnchktc(): TERM=%s: Termcap entry too long\n", 
		    termname);
		q[TBUFSIZE - (p-holdtbuf)] = 0;
	}
	(void) strcpy(p, q+1);
	tbuf = holdtbuf;
	return(1);
}

/*
 * Tnamatch deals with name matching.  The first field of the termcap
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
tnamatch(np)
	char *np;
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return(0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0))
			return (1);
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the termcap file in octal.
 */
/* static - TLH removed */ char *
tskip(bp)
	register char *bp;
{

	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':')
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
otgetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = tskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
otgetflag(id)
	char *id;
{
	register char *bp = tbuf;

	for (;;) {
		bp = tskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
			if (!*bp || *bp == ':')
				return (1);
			else if (*bp == '@')
				return(0);
		}
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
char *
otgetstr(id, area)
	char *id, **area;
{
	register char *bp = tbuf;

	for (;;) {
		bp = tskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return(0);
		if (*bp != '=')
			continue;
		bp++;
		return (tdecode(bp, area));
	}
}

/*
 * Tdecode does the grung work to decode the
 * string capability escapes.
 */
static char *
tdecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}
