/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.lex.c 1.1 92/07/30 SMI; from UCB 5.4 3/29/86";
#endif

#include "sh.h"
#include <sgtty.h>
#include "sh.tconst.h"

/*
 * C shell
 */

/*
 * These lexical routines read input and form lists of words.
 * There is some involved processing here, because of the complications
 * of input buffering, and especially because of history substitution.
 */

tchar *word();

/*
 * Peekc is a peek characer for getC, peekread for readc.
 * There is a subtlety here in many places... history routines
 * will read ahead and then insert stuff into the input stream.
 * If they push back a character then they must push it behind
 * the text substituted by the history substitution.  On the other
 * hand in several places we need 2 peek characters.  To make this
 * all work, the history routines read with getC, and make use both
 * of ungetC and unreadc.  The key observation is that the state
 * of getC at the call of a history reference is such that calls
 * to getC from the history routines will always yield calls of
 * readc, unless this peeking is involved.  That is to say that during
 * getexcl the variables lap, exclp, and exclnxt are all zero.
 *
 * Getdol invokes history substitution, hence the extra peek, peekd,
 * which it can ungetD to be before history substitutions.
 */
tchar peekc, peekd;
tchar peekread;

tchar *exclp;			/* (Tail of) current word from ! subst */
struct	wordent *exclnxt;	/* The rest of the ! subst words */
int	exclc;			/* Count of remainig words in ! subst */
tchar *alvecp;		/* "Globp" for alias resubstitution */

/*
 * Lex returns to its caller not only a wordlist (as a "var" parameter)
 * but also whether a history substitution occurred.  This is used in
 * the main (process) routine to determine whether to echo, and also
 * when called by the alias routine to determine whether to keep the
 * argument list.
 */
bool	hadhist;

tchar getCtmp;
#define getC(f)		((getCtmp = peekc) ? (peekc = 0, getCtmp) : getC1(f))
#define	ungetC(c)	peekc = c
#define	ungetD(c)	peekd = c

lex(hp)
	register struct wordent *hp;
{
	register struct wordent *wdp;
	int c;

#ifdef TRACE
	tprintf("TRACE- lex()\n");
#endif
	lineloc = btell();
	hp->next = hp->prev = hp;
	hp->word = S_ /*""*/;
	alvecp = 0, hadhist = 0;
	do
		c = readc(0);
	while (issp(c));
	if (c == HISTSUB && intty)
		/* ^lef^rit	from tty is short !:s^lef^rit */
		getexcl(c);
	else
		unreadc(c);
	wdp = hp;
	/*
	 * The following loop is written so that the links needed
	 * by freelex will be ready and rarin to go even if it is
	 * interrupted.
	 */
	do {
		register struct wordent *new = (struct wordent *) xalloc(sizeof *wdp);

		new->word = 0;
		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = word();
	} while (wdp->word[0] != '\n');
#ifdef TRACE
	tprintf("Exiting lex()\n");
#endif
	hp->prev = wdp;
	return (hadhist);
}

prlex(sp0)
	struct wordent *sp0;
{
	register struct wordent *sp = sp0->next;

#ifdef TRACE
	tprintf("TRACE- prlex()\n");
#endif
	for (;;) {
		printf("%t", sp->word);
		sp = sp->next;
		if (sp == sp0)
			break;
		if (sp->word[0] != '\n')
			putchar(' ');
	}
}

copylex(hp, fp)
	register struct wordent *hp;
	register struct wordent *fp;
{
	register struct wordent *wdp;

#ifdef TRACE
	tprintf("TRACE- copylex()\n");
#endif
	wdp = hp;
	fp = fp->next;
	do {
		register struct wordent *new = (struct wordent *) xalloc(sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = savestr(fp->word);
		fp = fp->next;
	} while (wdp->word[0] != '\n');
	hp->prev = wdp;
}

freelex(vp)
	register struct wordent *vp;
{
	register struct wordent *fp;

#ifdef TRACE
	tprintf("TRACE- freelex()\n");
#endif
	while (vp->next != vp) {
		fp = vp->next;
		vp->next = fp->next;
		XFREE(fp->word)
		XFREE( (char *)fp)
	}
	vp->prev = vp;
}

tchar *
word()
{
	register tchar c, c1;
	register tchar *wp;
	tchar wbuf[BUFSIZ];
	register bool dolflg;
	register int i;

#ifdef TRACE
	tprintf("TRACE- word()\n");
#endif
	wp = wbuf;
	i = BUFSIZ - 4;
loop:
	while (issp(c = getC(DOALL)))
		;
	if (cmap(c, _META|_ESC)||isauxsp(c))
		switch (c) {
		case '&':
		case '|':
		case '<':
		case '>':
			*wp++ = c;
			c1 = getC(DOALL);
			if (c1 == c)
				*wp++ = c1;
			else
				ungetC(c1);
			goto ret;

		case '#':
			if (intty)
				break;
			c = 0;
			do {
				c1 = c;
				c = getC(0);
			} while (c != '\n');
			if (c1 == '\\')
				goto loop;
			/* fall into ... */

		case ';':
		case '(':
		case ')':
		case '\n':
			*wp++ = c;
			goto ret;

		case '\\':
			c = getC(0);
			if (c == '\n') {
				if (onelflg == 1)
					onelflg = 2;
				goto loop;
			}
			if (c != HIST)
				*wp++ = '\\', --i;
			c |= QUOTE;
		}
	c1 = 0;
	dolflg = DOALL;
	for (;;) {
		if (c1) {
			if (c == c1) {
				c1 = 0;
				dolflg = DOALL;
			} else if (c == '\\') {
				c = getC(0);
				if (c == HIST)
					c |= QUOTE;
				else {
					if (c == '\n')
						/*
						if (c1 == '`')
							c = ' ';
						else
						*/
							c |= QUOTE;
					ungetC(c);
					c = '\\';
				}
			} else if (c == '\n') {
				seterrc("Unmatched ", c1);
				ungetC(c);
				break;
			}
		} else if (cmap(c, _META|_Q|_Q1|_ESC)||isauxsp(c)) {
			if (c == '\\') {
				c = getC(0);
				if (c == '\n') {
					if (onelflg == 1)
						onelflg = 2;
					break;
				}
				if (c != HIST)
					*wp++ = '\\', --i;
				c |= QUOTE;
			} else if (cmap(c, _Q|_Q1)) {		/* '"` */
				c1 = c;
				dolflg = c == '"' ? DOALL : DOEXCL;
			} else if (c != '#' || !intty) {
				ungetC(c);
				break;
			}
		}
		if (--i > 0) {
			*wp++ = c;
			c = getC(dolflg);
		} else {
			seterr("Word too long");
			wp = &wbuf[1];
			break;
		}
	}
ret:
	*wp = 0;
#ifdef TRACE
	tprintf("word() returning:%t\n", wbuf);
#endif
	return (savestr(wbuf));
}

getC1(flag)
	register int flag;
{
	register tchar c;

top:
	if (c = peekc) {
		peekc = 0;
		return (c);
	}
	if (lap) {
		if ((c = *lap++) == 0)
			lap = 0;
		else {
			if (cmap(c, _META|_Q|_Q1)||isauxsp(c))
				c |= QUOTE;
			return (c);
		}
	}
	if (c = peekd) {
		peekd = 0;
		return (c);
	}
	if (exclp) {
		if (c = *exclp++)
			return (c);
		if (exclnxt && --exclc >= 0) {
			exclnxt = exclnxt->next;
			setexclp(exclnxt->word);
			return (' ');
		}
		exclp = 0;
		exclnxt = 0;
	}
	if (exclnxt) {
		exclnxt = exclnxt->next;
		if (--exclc < 0)
			exclnxt = 0;
		else
			setexclp(exclnxt->word);
		goto top;
	}
	c = readc(0);
	if (c == '$' && (flag & DODOL)) {
		getdol();
		goto top;
	}
	if (c == HIST && (flag & DOEXCL)) {
		getexcl(0);
		goto top;
	}
	return (c);
}

getdol()
{
#define	MAXNAMELEN	40 /* Max # of chars in variables ?*/
	register tchar *np;
	tchar name[MAXNAMELEN];
	register int c;
	int sc;
	bool special = 0;

#ifdef TRACE
	tprintf("TRACE- getdol()\n");
#endif
	np = name, *np++ = '$';
	c = sc = getC(DOEXCL);
	if (isspnl(c)) {
		ungetD(c);
		ungetC('$' | QUOTE);
		return;
	}
	if (c == '{')
		*np++ = c, c = getC(DOEXCL);
	if (c == '#' || c == '?')
		special++, *np++ = c, c = getC(DOEXCL);
	*np++ = c;
	switch (c) {
	
	case '<':
	case '$':
		if (special)
			goto vsyn;
		goto ret;

	case '\n':
		ungetD(c);
		np--;
		goto vsyn;

	case '*':
		if (special)
			goto vsyn;
		goto ret;

	default:
		if (digit(c)) {
/*
 * let $?0 pass for now
			if (special)
				goto vsyn;
*/
			while (digit(c = getC(DOEXCL))) {
				if (np < &name[MAXNAMELEN / 2])
					*np++ = c;
			}
		} else if (letter(c)) {
			while (letter(c = getC(DOEXCL)) || digit(c)) {
				if (np < &name[MAXNAMELEN / 2])
					*np++ = c;
			}
		}
		else
			goto vsyn;
	}
	if (c == '[') {
		*np++ = c;
		do {
			c = getC(DOEXCL);
			if (c == '\n') {
				ungetD(c);
				np--;
				goto vsyn;
			}
			if (np >= &name[MAXNAMELEN - 8])
				goto vsyn;
			*np++ = c;
		} while (c != ']');
		c = getC(DOEXCL);
	}
	if (c == ':') {
		*np++ = c, c = getC(DOEXCL);
		if (c == 'g')
			*np++ = c, c = getC(DOEXCL);
		*np++ = c;
		if (!any(c, S_htrqxe))
			goto vsyn;
	} else
		ungetD(c);
	if (sc == '{') {
		c = getC(DOEXCL);
		if (c != '}') {
			ungetC(c);
			goto vsyn;
		}
		*np++ = c;
	}
ret:
	*np = 0;
	addla(name);
	return;

vsyn:
	seterr("Variable syntax");
	goto ret;
#undef	MAXNAMELEN
}

addla(cp)
	tchar *cp;
{
	tchar buf[BUFSIZ];

#ifdef TRACE
	tprintf("TRACE- addla()\n");
#endif
	if (lap != 0 && strlen_(cp) + strlen_(lap) >= BUFSIZ - 4) {
		seterr("Expansion buf ovflo");
		return;
	}
	if (lap)
		(void) strcpy_(buf, lap);
	(void) strcpy_(labuf, cp);
	if (lap)
		(void) strcat_(labuf, buf);
	lap = labuf;
}

tchar	lhsb[32];
tchar	slhs[32];
tchar	rhsb[64];
int	quesarg;

getexcl(sc)
	tchar sc;
{
	register struct wordent *hp, *ip;
	int left, right, dol;
	register int c;

#ifdef TRACE
	tprintf("TRACE- getexcl()\n");
#endif
	if (sc == 0) {
		sc = getC(0);
		if (sc != '{') {
			ungetC(sc);
			sc = 0;
		}
	}
	quesarg = -1;
	lastev = eventno;
	hp = gethent(sc);
	if (hp == 0)
		return;
	hadhist = 1;
	dol = 0;
	if (hp == alhistp)
		for (ip = hp->next->next; ip != alhistt; ip = ip->next)
			dol++;
	else
		for (ip = hp->next->next; ip != hp->prev; ip = ip->next)
			dol++;
	left = 0, right = dol;
	if (sc == HISTSUB) {
		ungetC('s'), unreadc(HISTSUB), c = ':';
		goto subst;
	}
	c = getC(0);
	/* if (!any(c, ":^$*-%")) */		/* change needed for char -> tchar */
	if (! (c == ':' || c == '^' || c == '$' || c == '*' ||
	       c == '-' || c == '%' ))
		goto subst;
	left = right = -1;
	if (c == ':') {
		c = getC(0);
		unreadc(c);
		if (letter(c) || c == '&') {
			c = ':';
			left = 0, right = dol;
			goto subst;
		}
	} else
		ungetC(c);
	if (!getsel(&left, &right, dol))
		return;
	c = getC(0);
	if (c == '*')
		ungetC(c), c = '-';
	if (c == '-') {
		if (!getsel(&left, &right, dol))
			return;
		c = getC(0);
	}
subst:
	exclc = right - left + 1;
	while (--left >= 0)
		hp = hp->next;
	if (sc == HISTSUB || c == ':') {
		do {
			hp = getsub(hp);
			c = getC(0);
		} while (c == ':');
	}
	unreadc(c);
	if (sc == '{') {
		c = getC(0);
		if (c != '}')
			seterr("Bad ! form");
	}
	exclnxt = hp;
}

struct wordent *
getsub(en)
	struct wordent *en;
{
	register tchar *cp;
	int delim;
	register int c;
	int sc;
	bool global = 0;
	tchar orhsb[(sizeof rhsb)/(sizeof rhsb[0])];

#ifdef TRACE
	tprintf("TRACE- getsub()\n");
#endif
	exclnxt = 0;
	sc = c = getC(0);
	if (c == 'g')
		global++, c = getC(0);
	switch (c) {

	case 'p':
		justpr++;
		goto ret;

	case 'x':
	case 'q':
		global++;
		/* fall into ... */

	case 'h':
	case 'r':
	case 't':
	case 'e':
		break;

	case '&':
		if (slhs[0] == 0) {
			seterr("No prev sub");
			goto ret;
		}
		(void) strcpy_(lhsb, slhs);
		break;

/*
	case '~':
		if (lhsb[0] == 0)
			goto badlhs;
		break;
*/

	case 's':
		delim = getC(0);
		if (alnum(delim) || isspnl(delim)){
			unreadc(delim);
bads:
			lhsb[0] = 0;
			seterr("Bad substitute");
			goto ret;
		}
		cp = lhsb;
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				break;
			}
			if (c == delim)
				break;
			if (cp > &lhsb[(sizeof lhsb)/(sizeof lhsb[0]) - 2])
				goto bads;
			if (c == '\\') {
				c = getC(0);
				if (c != delim && c != '\\')
					*cp++ = '\\';
			}
			*cp++ = c;
		}
		if (cp != lhsb)
			*cp++ = 0;
		else if (lhsb[0] == 0) {
/*badlhs:*/
			seterr("No prev lhs");
			goto ret;
		}
		cp = rhsb;
		(void) strcpy_(orhsb, cp);
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				break;
			}
			if (c == delim)
				break;
/*
			if (c == '~') {
				if (&cp[strlen_(orhsb)] 
				> &rhsb[(sizeof rhsb)/(sizeof rhsb[0]) - 2])
					goto toorhs;
				(void) strcpy_(cp, orhsb);
				cp = strend(cp);
				continue;
			}
*/
			if (cp > &rhsb[(sizeof rhsb)/(sizeof rhsb[0]) - 2]) {
/*toorhs:*/
				seterr("Rhs too long");
				goto ret;
			}
			if (c == '\\') {
				c = getC(0);
				if (c != delim /* && c != '~' */)
					*cp++ = '\\';
			}
			*cp++ = c;
		}
		*cp++ = 0;
		break;

	default:
		if (c == '\n')
			unreadc(c);
		seterrc("Bad ! modifier: ", c);
		goto ret;
	}
	(void) strcpy_(slhs, lhsb);
	if (exclc)
		en = dosub(sc, en, global);
ret:
	return (en);
}

struct wordent *
dosub(sc, en, global)
	int sc;
	struct wordent *en;
	bool global;
{
	struct wordent lex;
	bool didsub = 0;
	struct wordent *hp = &lex;
	register struct wordent *wdp;
	register int i = exclc;

#ifdef TRACE
	tprintf("TRACE- dosub()\n");
#endif
	wdp = hp;
	while (--i >= 0) {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		en = en->next;
		wdp->word = global || didsub == 0 ?
		    subword(en->word, sc, &didsub) : savestr(en->word);
	}
	if (didsub == 0)
		seterr("Modifier failed");
	hp->prev = wdp;
	return (&enthist(-1000, &lex, 0)->Hlex);
}

tchar *
subword(cp, type, adid)
	tchar *cp;
	int type;
	bool *adid;
{
	tchar wbuf[BUFSIZ];
	register tchar *wp, *mp, *np;
	register int i;

#ifdef TRACE
	tprintf("TRACE- subword()\n");
#endif
	switch (type) {

	case 'r':
	case 'e':
	case 'h':
	case 't':
	case 'q':
	case 'x':
		wp = domod(cp, type);
		if (wp == 0)
			return (savestr(cp));
		*adid = 1;
		return (wp);

	default:
		wp = wbuf;
		i = BUFSIZ - 4;
		for (mp = cp; *mp; mp++)
			if (matchs(mp, lhsb)) {
				for (np = cp; np < mp;)
					*wp++ = *np++, --i;
				for (np = rhsb; *np; np++) switch (*np) {

				case '\\':
					if (np[1] == '&')
						np++;
					/* fall into ... */

				default:
					if (--i < 0)
						goto ovflo;
					*wp++ = *np;
					continue;

				case '&':
					i -= strlen_(lhsb);
					if (i < 0)
						goto ovflo;
					*wp = 0;
					(void) strcat_(wp, lhsb);
					wp = strend(wp);
					continue;
				}
				mp += strlen_(lhsb);
				i -= strlen_(mp);
				if (i < 0) {
ovflo:
					seterr("Subst buf ovflo");
					return (S_ /*""*/);
				}
				*wp = 0;
				(void) strcat_(wp, mp);
				*adid = 1;
				return (savestr(wbuf));
			}
		return (savestr(cp));
	}
}

tchar *
domod(cp, type)
	tchar *cp;
	int type;
{
	register tchar *wp, *xp;
	register int c;

#ifdef TRACE
	tprintf("TRACE- domod()\n");
#endif
	switch (type) {

	case 'x':
	case 'q':
		wp = savestr(cp);
		for (xp = wp; c = *xp; xp++)
			if (!issp(c) || type == 'q')
				*xp |= QUOTE;
		return (wp);

	case 'h':
	case 't':
		if (!any('/', cp))
			return (type == 't' ? savestr(cp) : 0);
		wp = strend(cp);
		while (*--wp != '/')
			continue;
		if (type == 'h')
			xp = savestr(cp), xp[wp - cp] = 0;
		else
			xp = savestr(wp + 1);
		return (xp);

	case 'e':
	case 'r':
		wp = strend(cp);
		for (wp--; wp >= cp && *wp != '/'; wp--)
			if (*wp == '.') {
				if (type == 'e')
					xp = savestr(wp + 1);
				else
					xp = savestr(cp), xp[wp - cp] = 0;
				return (xp);
			}
		return (savestr(type == 'e' ? S_ /*""*/ : cp));
	}
	return (0);
}

matchs(str, pat)
	register tchar *str, *pat;
{

#ifdef TRACE
	tprintf("TRACE- matchs()\n");
#endif
	while (*str && *pat && *str == *pat)
		str++, pat++;
	return (*pat == 0);
}

getsel(al, ar, dol)
	register int *al, *ar;
	int dol;
{
	register int c = getC(0);
	register int i;
	bool first = *al < 0;

#ifdef TRACE
	tprintf("TRACE- getsel()\n");
#endif
	switch (c) {

	case '%':
		if (quesarg == -1)
			goto bad;
		if (*al < 0)
			*al = quesarg;
		*ar = quesarg;
		break;

	case '-':
		if (*al < 0) {
			*al = 0;
			*ar = dol - 1;
			unreadc(c);
		}
		return (1);

	case '^':
		if (*al < 0)
			*al = 1;
		*ar = 1;
		break;

	case '$':
		if (*al < 0)
			*al = dol;
		*ar = dol;
		break;

	case '*':
		if (*al < 0)
			*al = 1;
		*ar = dol;
		if (*ar < *al) {
			*ar = 0;
			*al = 1;
			return (1);
		}
		break;

	default:
		if (digit(c)) {
			i = 0;
			while (digit(c)) {
				i = i * 10 + c - '0';
				c = getC(0);
			}
			if (i < 0)
				i = dol + 1;
			if (*al < 0)
				*al = i;
			*ar = i;
		} else
			if (*al < 0)
				*al = 0, *ar = dol;
			else
				*ar = dol - 1;
		unreadc(c);
		break;
	}
	if (first) {
		c = getC(0);
		unreadc(c);
		/* if (any(c, "-$*")) */	/* char -> tchar */
		if (c == '-' || c == '$' || c == '*')
			return (1);
	}
	if (*al > *ar || *ar > dol) {
bad:
		seterr("Bad ! arg selector");
		return (0);
	}
	return (1);

}

struct wordent *
gethent(sc)
	int sc;
{
	register struct Hist *hp;
	register tchar *np;
	register int c;
	int event;
	bool back = 0;

#ifdef TRACE
	tprintf("TRACE- gethent()\n");
#endif
	c = sc == HISTSUB ? HIST : getC(0);
	if (c == HIST) {
		if (alhistp)
			return (alhistp);
		event = eventno;
		goto skip;
	}
	switch (c) {

	case ':':
	case '^':
	case '$':
	case '*':
	case '%':
		ungetC(c);
		if (lastev == eventno && alhistp)
			return (alhistp);
		event = lastev;
		break;

	case '-':
		back = 1;
		c = getC(0);
		goto number;

	case '#':			/* !# is command being typed in (mrh) */
		return(&paraml);

	default:
		/*if (any(c, "(=~")) {*/
		if (c == '(' || c == '=' || c == '~') {
			unreadc(c);
			ungetC(HIST);
			return (0);
		}
		if (digit(c))
			goto number;
		np = lhsb;
		/* while (!any(c, ": \t\\\n}")) { */
		while (! (c == ':' || c == '\\' || isspnl(c) || c == '}')) {
			if (np < &lhsb[(sizeof lhsb)/(sizeof lhsb[0]) - 2])
				*np++ = c;
			c = getC(0);
		}
		unreadc(c);
		if (np == lhsb) {
			ungetC(HIST);
			return (0);
		}
		*np++ = 0;
		hp = findev(lhsb, 0);
		if (hp)
			lastev = hp->Hnum;
		return (&hp->Hlex);

	case '?':
		np = lhsb;
		for (;;) {
			c = getC(0);
			if (c == '\n') {
				unreadc(c);
				break;
			}
			if (c == '?')
				break;
			if (np < &lhsb[(sizeof lhsb)/(sizeof lhsb[0]) - 2])
				*np++ = c;
		}
		if (np == lhsb) {
			if (lhsb[0] == 0) {
				seterr("No prev search");
				return (0);
			}
		} else
			*np++ = 0;
		hp = findev(lhsb, 1);
		if (hp)
			lastev = hp->Hnum;
		return (&hp->Hlex);

	number:
		event = 0;
		while (digit(c)) {
			event = event * 10 + c - '0';
			c = getC(0);
		}
		if (back)
			event = eventno + (alhistp == 0) - (event ? event : 0);
		unreadc(c);
		break;
	}
skip:
	for (hp = Histlist.Hnext; hp; hp = hp->Hnext)
		if (hp->Hnum == event) {
			hp->Href = eventno;
			lastev = hp->Hnum;
			return (&hp->Hlex);
		}
	np = putn(event);
	noev(np);
	return (0);
}

struct Hist *
findev(cp, anyarg)
	tchar *cp;
	bool anyarg;
{
	register struct Hist *hp;

#ifdef TRACE
	tprintf("TRACE- findev()\n");
#endif
	for (hp = Histlist.Hnext; hp; hp = hp->Hnext) {
	 tchar *dp;
		register tchar *p, *q;
		register struct wordent *lp = hp->Hlex.next;
		int argno = 0;

		if (lp->word[0] == '\n')
			continue;
		if (!anyarg) {
			p = cp;
			q = lp->word;
			do
				if (!*p)
					return (hp);
			while (*p++ == *q++);
			continue;
		}
		do {
			for (dp = lp->word; *dp; dp++) {
				p = cp;
				q = dp;
				do
					if (!*p) {
						quesarg = argno;
						return (hp);
					}
				while (*p++ == *q++);
			}
			lp = lp->next;
			argno++;
		} while (lp->word[0] != '\n');
	}
	noev(cp);
	return (0);
}

noev(cp)
	tchar *cp;
{

#ifdef TRACE
	tprintf("TRACE- noev()\n");
#endif
	seterr2(cp, ": Event not found");
}

setexclp(cp)
	register tchar *cp;
{

#ifdef TRACE
	tprintf("TRACE- setexclp()\n");
#endif
	if (cp && cp[0] == '\n')
		return;
	exclp = cp;
}

unreadc(c)
	tchar c;
{

	peekread = c;
}

readc(wanteof)
	bool wanteof;
{
	register int c;
	static sincereal;

	if (c = peekread) {
		peekread = 0;
		return (c);
	}
top:
	if (alvecp) {
		if (c = *alvecp++)
			return (c);
		if (*alvec) {
			alvecp = *alvec++;
			return (' ');
		}
	}
	if (alvec) {
		if (alvecp = *alvec) {
			alvec++;
			goto top;
		}
		/* Infinite source! */
		return ('\n');
	}
	if (evalp) {
		if (c = *evalp++)
			return (c);
		if (*evalvec) {
			evalp = *evalvec++;
			return (' ');
		}
		evalp = 0;
	}
	if (evalvec) {
		if (evalvec ==  (tchar **)1) {
			doneinp = 1;
			reset();
		}
		if (evalp = *evalvec) {
			evalvec++;
			goto top;
		}
		evalvec =  (tchar **)1;
		return ('\n');
	}
	do {
		if (arginp ==  (tchar *) 1 || onelflg == 1) {
			if (wanteof)
				return (-1);
			exitstat();
		}
		if (arginp) {
			if ((c = *arginp++) == 0) {
				arginp =  (tchar *) 1;
				return ('\n');
			}
			return (c);
		}
reread:
		c = bgetc();
		if (c < 0) {
			struct sgttyb tty;

			if (wanteof)
				return (-1);
			/* was isatty but raw with ignoreeof yields problems */
			if (ioctl(SHIN, TIOCGETP,  (char *)&tty) == 0 &&
			    (tty.sg_flags & RAW) == 0) {
				/* was 'short' for FILEC */
				int ctpgrp;

				if (++sincereal > 25)
					goto oops;
				if (tpgrp != -1 &&
				    ioctl(FSHTTY, TIOCGPGRP,  (char *)&ctpgrp) == 0 &&
				    tpgrp != ctpgrp) {
					(void) ioctl(FSHTTY, TIOCSPGRP,
						 (char *)&tpgrp);
					(void) killpg(ctpgrp, SIGHUP);
printf("Reset tty pgrp from %d to %d\n", ctpgrp, tpgrp);
					goto reread;
				}
				if (adrof(S_ignoreeof/*"ignoreeof"*/)) {
					if (loginsh)
						printf("\nUse \"logout\" to logout.\n");
					else
						printf("\nUse \"exit\" to leave csh.\n");
					reset();
				}
				if (chkstop == 0)
					panystop(1);
			}
oops:
			doneinp = 1;
			reset();
		}
		sincereal = 0;
		if (c == '\n' && onelflg)
			onelflg--;
	} while (c == 0);
	return (c);
}

bgetc()
{
	register int buf, off, c;
#ifdef FILEC
	tchar ttyline[BUFSIZ];
	register int numleft = 0, roomleft;
#endif

#ifdef TELL
	if (cantell) {
		if (fseekp < fbobp || fseekp > feobp) {
			fbobp = feobp = fseekp;
			(void) lseek(SHIN, fseekp, 0);
		}
		if (fseekp == feobp) {
			fbobp = feobp;
			do
				c = read_(SHIN, fbuf[0], BUFSIZ);
			while (c < 0 && errno == EINTR);
			if (c <= 0)
				return (-1);
			feobp += c;
		}
		c = fbuf[0][fseekp - fbobp];
		fseekp++;
		return (c);
	}
#endif
again:
	buf = (int) fseekp / BUFSIZ;
	if (buf >= fblocks) {
		register tchar **nfbuf =
			 (tchar **) calloc((unsigned) (fblocks + 2),
				sizeof  (tchar **));

		if (fbuf) {
			(void) blkcpy(nfbuf, fbuf);
			xfree( (char *)fbuf);
		}
		fbuf = nfbuf;
		fbuf[fblocks] = (tchar *)calloc(BUFSIZ, sizeof (tchar));
		fblocks++;
		goto again;
	}
	if (fseekp >= feobp) {
		buf = (int) feobp / BUFSIZ;
		off = (int) feobp % BUFSIZ;
#ifndef FILEC
		for (;;) {
			c = read_(SHIN, fbuf[buf] + off, BUFSIZ - off);
#else
		roomleft = BUFSIZ - off;
		for (;;) {
			if (filec && intty) {
				c = numleft ? numleft : tenex(ttyline, BUFSIZ);
				if (c > roomleft) {
					/* start with fresh buffer */
					feobp = fseekp = fblocks * BUFSIZ;
					numleft = c;
					goto again;
				}
				if (c > 0)
					copy(fbuf[buf] + off, ttyline, c*sizeof(tchar));
				numleft = 0;
			} else
				c = read_(SHIN, fbuf[buf] + off, roomleft);
#endif
			if (c >= 0)
				break;
			if (errno == EWOULDBLOCK) {
				int off = 0;

				(void) ioctl(SHIN, FIONBIO,  (char *)&off);
			} else if (errno != EINTR)
				break;
		}
		if (c <= 0)
			return (-1);
		feobp += c;
#ifndef FILEC
		goto again;
#else
		if (filec && !intty)
			goto again;
#endif
	}
	c = fbuf[buf][(int) fseekp % BUFSIZ];
	fseekp++;
	return (c);
}

bfree()
{
	register int sb, i;

#ifdef TELL
	if (cantell)
		return;
#endif
	if (whyles)
		return;
	sb = (int) (fseekp - 1) / BUFSIZ;
	if (sb > 0) {
		for (i = 0; i < sb; i++)
			xfree(fbuf[i]);
		(void) blkcpy(fbuf, &fbuf[sb]);
		fseekp -= BUFSIZ * sb;
		feobp -= BUFSIZ * sb;
		fblocks -= sb;
	}
}

bseek(l)
	off_t l;
{
	register struct whyle *wp;

	fseekp = l;
#ifdef TELL
	if (!cantell) {
#endif
		if (!whyles)
			return;
		for (wp = whyles; wp->w_next; wp = wp->w_next)
			continue;
		if (wp->w_start > l)
			l = wp->w_start;
#ifdef TELL
	}
#endif
}

/* any similarity to bell telephone is purely accidental */
#ifndef btell
off_t
btell()
{

	return (fseekp);
}
#endif

btoeof()
{

	(void) lseek(SHIN, (off_t)0, 2);
	fseekp = feobp;
	wfree();
	bfree();
}

#ifdef TELL
settell()
{

	cantell = 0;
	if (arginp || onelflg || intty)
		return;
	if (lseek(SHIN, (off_t)0, 1) < 0 || errno == ESPIPE)
		return;
	fbuf =  (tchar **) calloc(2, sizeof  (tchar **));
	fblocks = 1;
	fbuf[0] = (tchar *)calloc(BUFSIZ, sizeof (tchar));
	fseekp = fbobp = feobp = lseek(SHIN, (off_t)0, 1);
	cantell = 1;
}
#endif

