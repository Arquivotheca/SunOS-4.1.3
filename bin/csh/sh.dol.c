/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.dol.c 1.1 92/07/30 SMI; from UCB 5.3 3/29/86";
#endif

#include "sh.h"
#include "sh.tconst.h"

/*
 * C shell
 */

/*
 * These routines perform variable substitution and quoting via ' and ".
 * To this point these constructs have been preserved in the divided
 * input words.  Here we expand variables and turn quoting via ' and " into
 * QUOTE bits on characters (which prevent further interpretation).
 * If the `:q' modifier was applied during history expansion, then
 * some QUOTEing may have occurred already, so we dont "trim()" here.
 */

int	Dpeekc, Dpeekrd;		/* Peeks for DgetC and Dreadc */
tchar	*Dcp, **Dvp;			/* Input vector for Dreadc */

#define	DEOF	-1

#define	unDgetC(c)	Dpeekc = c

#define QUOTES		(_Q|_Q1|_ESC)	/* \ ' " ` */

/*
 * The following variables give the information about the current
 * $ expansion, recording the current word position, the remaining
 * words within this expansion, the count of remaining words, and the
 * information about any : modifier which is being applied.
 */
tchar	*dolp;			/* Remaining chars from this word */
tchar	**dolnxt;		/* Further words */
int	dolcnt;			/* Count of further words */
tchar	dolmod;			/* : modifier character */
int	dolmcnt;		/* :gx -> 10000, else 1 */

/*
 * Fix up the $ expansions and quotations in the
 * argument list to command t.
 */
Dfix(t)
	register struct command *t;
{
	register tchar **pp;
	register tchar *p;

#ifdef TRACE
	tprintf("TRACE- Dfix()\n");
#endif
	if (noexec)
		return;
	/* Note that t_dcom isn't trimmed thus !...:q's aren't lost */
	for (pp = t->t_dcom; p = *pp++;)
		while (*p)
			if (cmap(*p++, _DOL|QUOTES)) {	/* $, \, ', ", ` */
				Dfix2(t->t_dcom);	/* found one */
				blkfree(t->t_dcom);
				t->t_dcom = gargv;
				gargv = 0;
				return;
			}
}

/*
 * $ substitute one word, for i/o redirection
 */
tchar *
Dfix1(cp)
	register tchar *cp;
{
	tchar *Dv[2];

#ifdef TRACE
	tprintf("TRACE- Dfix1()\n");
#endif
	if (noexec)
		return (0);
	Dv[0] = cp; Dv[1] = NOSTR;
	Dfix2(Dv);
	if (gargc != 1) {
		setname(cp);
		bferr("Ambiguous");
	}
	cp = savestr(gargv[0]);
	blkfree(gargv), gargv = 0;
	return (cp);
}

/*
 * Subroutine to do actual fixing after state initialization.
 */
Dfix2(v)
     tchar **v;
{
	tchar *agargv[GAVSIZ];

#ifdef TRACE
	tprintf("TRACE- Dfix2()\n");
#endif
	ginit(agargv);			/* Initialize glob's area pointers */
	Dvp = v; Dcp = S_ /* "" */;/* Setup input vector for Dreadc */
	unDgetC(0); unDredc(0);		/* Clear out any old peeks (at error) */
	dolp = 0; dolcnt = 0;		/* Clear out residual $ expands (...) */
	while (Dword())
		continue;
	gargv = copyblk(gargv);
}

/*
 * Get a word.  This routine is analogous to the routine
 * word() in sh.lex.c for the main lexical input.  One difference
 * here is that we don't get a newline to terminate our expansion.
 * Rather, DgetC will return a DEOF when we hit the end-of-input.
 */
Dword()
{
	register int c, c1;
	tchar wbuf[BUFSIZ];
	register tchar *wp = wbuf;
	register int i = BUFSIZ - 4;
	register bool dolflg;
	bool sofar = 0;

#ifdef TRACE
	tprintf("TRACE- Dword()\n");
#endif
loop:
	c = DgetC(DODOL);
	switch (c) {

	case DEOF:
deof:
		if (sofar == 0)
			return (0);
		/* finish this word and catch the code above the next time */
		unDredc(c);
		/* fall into ... */

	case '\n':
		*wp = 0;
		goto ret;

	case ' ':
	case '\t':
		goto loop;

	case '`':
		/* We preserve ` quotations which are done yet later */
		*wp++ = c, --i;
	case '\'':
	case '"':
		/*
		 * Note that DgetC never returns a QUOTES character
		 * from an expansion, so only true input quotes will
		 * get us here or out.
		 */
		c1 = c;
		dolflg = c1 == '"' ? DODOL : 0;
		for (;;) {
			c = DgetC(dolflg);
			if (c == c1)
				break;
			if (c == '\n' || c == DEOF)
				error("Unmatched %c", c1);
			if ((c & (QUOTE|TRIM)) == ('\n' | QUOTE))
				--wp, ++i;
			if (--i <= 0)
				goto toochars;
			switch (c1) {

			case '"':
				/*
				 * Leave any `s alone for later.
				 * Other chars are all quoted, thus `...`
				 * can tell it was within "...".
				 */
				*wp++ = c == '`' ? '`' : c | QUOTE;
				break;

			case '\'':
				/* Prevent all further interpretation */
				*wp++ = c | QUOTE;
				break;

			case '`':
				/* Leave all text alone for later */
				*wp++ = c;
				break;
			}
		}
		if (c1 == '`')
			*wp++ = '`', --i;
		goto pack;		/* continue the word */

	case '\\':
		c = DgetC(0);		/* No $ subst! */
		if (c == '\n' || c == DEOF)
			goto loop;
		c |= QUOTE;
		break;
#ifdef MBCHAR /* Could be a space char from aux. codeset. */
        default:
		if(isauxsp(c))goto loop;
#endif/*MBCHAR*/
	}
	unDgetC(c);
pack:
	sofar = 1;
	/* pack up more characters in this word */
	for (;;) {
		c = DgetC(DODOL);
		if (c == '\\') {
			c = DgetC(0);
			if (c == DEOF)
				goto deof;
			if (c == '\n')
				c = ' ';
			else
				c |= QUOTE;
		}
		if (c == DEOF)
			goto deof;
		if (cmap(c, _SP|_NL|_Q|_Q1)
		    ||isauxsp(c)) {		/* sp \t\n'"` or aux. sp*/
			unDgetC(c);
			if (cmap(c, QUOTES))
				goto loop;
			*wp++ = 0;
			goto ret;
		}
		if (--i <= 0)
toochars:
			error("Word too long");
		*wp++ = c;
	}
ret:
	Gcat(S_ /* "" */, wbuf);
	return (1);
}

/*
 * Get a character, performing $ substitution unless flag is 0.
 * Any QUOTES character which is returned from a $ expansion is
 * QUOTEd so that it will not be recognized above.
 */
DgetC(flag)
	register int flag;
{
	register int c;

top:
	if (c = Dpeekc) {
		Dpeekc = 0;
		return (c);
	}
	if (lap) {
		c = *lap++ & (QUOTE|TRIM);
		if (c == 0) {
			lap = 0;
			goto top;
		}
quotspec:
		if (cmap(c, QUOTES))
			return (c | QUOTE);
		return (c);
	}
	if (dolp) {
		if (c = *dolp++ & (QUOTE|TRIM))
			goto quotspec;
		if (dolcnt > 0) {
			setDolp(*dolnxt++);
			--dolcnt;
			return (' ');
		}
		dolp = 0;
	}
	if (dolcnt > 0) {
		setDolp(*dolnxt++);
		--dolcnt;
		goto top;
	}
	c = Dredc();
	if (c == '$' && flag) {
		Dgetdol();
		goto top;
	}
	return (c);
}

tchar *nulvec[] = { 0 };
struct	varent nulargv = { nulvec, S_argv, 0 };

/*
 * Handle the multitudinous $ expansion forms.
 * Ugh.
 */
Dgetdol()
{
	register tchar *np;
	register struct varent *vp;
	tchar name[20];
	int c, sc;
	int subscr = 0, lwb = 1, upb = 0;
	bool dimen = 0, bitset = 0;
	tchar wbuf[BUFSIZ];

#ifdef TRACE
	tprintf("TRACE- Dgetdol()\n");
#endif
	dolmod = dolmcnt = 0;
	c = sc = DgetC(0);
	if (c == '{')
		c = DgetC(0);		/* sc is { to take } later */
	if ((c & TRIM) == '#')
		dimen++, c = DgetC(0);		/* $# takes dimension */
	else if (c == '?')
		bitset++, c = DgetC(0);		/* $? tests existence */
	switch (c) {
	
	case '$':
		if (dimen || bitset)
			goto syntax;		/* No $?$, $#$ */
		setDolp(doldol);
		goto eatbrac;

	case '<'|QUOTE:
		if (dimen || bitset)
			goto syntax;		/* No $?<, $#< */
		for (np = wbuf; read_(OLDSTD, np, 1) == 1; np++) {
			if (np >= &wbuf[BUFSIZ-1])
				error("$< line too long");
			if (*np <= 0 || *np == '\n')
				break;
		}
		*np = 0;
		/*
		 * KLUDGE: dolmod is set here because it will
		 * cause setDolp to call domod and thus to copy wbuf.
		 * Otherwise setDolp would use it directly. If we saved
		 * it ourselves, no one would know when to free it.
		 * The actual function of the 'q' causes filename
		 * expansion not to be done on the interpolated value.
		 */
		dolmod = 'q';
		dolmcnt = 10000;
		setDolp(wbuf);
		goto eatbrac;

	case DEOF:
	case '\n':
		goto syntax;

	case '*':
		(void) strcpy_(name, S_argv);
		vp = adrof(S_argv);
		subscr = -1;			/* Prevent eating [...] */
		break;

	default:
		np = name;
		if (digit(c)) {
			if (dimen)
				goto syntax;	/* No $#1, e.g. */
			subscr = 0;
			do {
				subscr = subscr * 10 + c - '0';
				c = DgetC(0);
			} while (digit(c));
			unDredc(c);
			if (subscr < 0)
				goto oob;
			if (subscr == 0) {
				if (bitset) {
					dolp = file ? S_1/*"1"*/ : S_0/*"0"*/;
					goto eatbrac;
				}
				if (file == 0)
					error("No file for $0");
				setDolp(file);
				goto eatbrac;
			}
			if (bitset)
				goto syntax;
			vp = adrof(S_argv);
			if (vp == 0) {
				vp = &nulargv;
				goto eatmod;
			}
			break;
		}
		if (!alnum(c))
			goto syntax;
		for (;;) {
			*np++ = c;
			c = DgetC(0);
			if (!alnum(c))
				break;
			if (np >= &name[sizeof(name)/sizeof(*name) - 2])
syntax:
				error("Variable syntax");
		}
		*np++ = 0;
		unDredc(c);
		vp = adrof(name);
	}
	if (bitset) {
		/* getenv() to getenv_(), because 'name''s type is now tchar * */
		/* no need to xalloc */
		dolp = (vp || getenv_(name)) ? S_1 /*"1"*/ : S_0/*"0"*/;
		goto eatbrac;
	}
	if (vp == 0) {
		/* getenv() to getenv_(), because 'name''s type is now tchar * */
		/* no need to xalloc */
		np = getenv_(name);
		if (np) {
			addla(np);
			goto eatbrac;
		}
		udvar(name);
		/*NOTREACHED*/
	}
	c = DgetC(0);
	upb = blklen(vp->vec);
	if (dimen == 0 && subscr == 0 && c == '[') {
		np = name;
		for (;;) {
			c = DgetC(DODOL);	/* Allow $ expand within [ ] */
			if (c == ']')
				break;
			if (c == '\n' || c == DEOF)
				goto syntax;
			if (np >= &name[sizeof(name)/sizeof(*name) - 2])
				goto syntax;
			*np++ = c;
		}
		*np = 0, np = name;
		if (dolp || dolcnt)		/* $ exp must end before ] */
			goto syntax;
		if (!*np)
			goto syntax;
		if (digit(*np)) {
			register int i = 0;

			while (digit(*np))
				i = i * 10 + *np++ - '0';
/*			if ((i < 0 || i > upb) && !any(*np, "-*")) {*/
			if ((i < 0 || i > upb) && (*np!='-') && (*np!='*')) {
oob:
				setname(vp->v_name);
				error("Subscript out of range");
			}
			lwb = i;
			if (!*np)
				upb = lwb, np = S_AST/*"*"*/;
		}
		if (*np == '*')
			np++;
		else if (*np != '-')
			goto syntax;
		else {
			register int i = upb;

			np++;
			if (digit(*np)) {
				i = 0;
				while (digit(*np))
					i = i * 10 + *np++ - '0';
				if (i < 0 || i > upb)
					goto oob;
			}
			if (i < lwb)
				upb = lwb - 1;
			else
				upb = i;
		}
		if (lwb == 0) {
			if (upb != 0)
				goto oob;
			upb = -1;
		}
		if (*np)
			goto syntax;
	} else {
		if (subscr > 0)
			if (subscr > upb)
				lwb = 1, upb = 0;
			else
				lwb = upb = subscr;
		unDredc(c);
	}
	if (dimen) {
		tchar *cp = putn(upb - lwb + 1);

		addla(cp);
		xfree(cp);
	} else {
eatmod:
		c = DgetC(0);
		if (c == ':') {
			c = DgetC(0), dolmcnt = 1;
			if (c == 'g')
				c = DgetC(0), dolmcnt = 10000;
			if (!any(c, S_htrqxe))
				error("Bad : mod in $");
			dolmod = c;
			if (c == 'q')
				dolmcnt = 10000;
		} else
			unDredc(c);
		dolnxt = &vp->vec[lwb - 1];
		dolcnt = upb - lwb + 1;
	}
eatbrac:
	if (sc == '{') {
		c = Dredc();
		if (c != '}')
			goto syntax;
	}
}

setDolp(cp)
	register tchar *cp;
{
	register tchar *dp;

#ifdef TRACE
	tprintf("TRACE- setDolp()\n");
#endif
	if (dolmod == 0 || dolmcnt == 0) {
		dolp = cp;
		return;
	}
	dp = domod(cp, dolmod);
	if (dp) {
		dolmcnt--;
		addla(dp);
		xfree(dp);
	} else
		addla(cp);
	dolp = S_/*""*/;
}

unDredc(c)
	int c;
{

	Dpeekrd = c;
}

Dredc()
{
	register int c;

	if (c = Dpeekrd) {
		Dpeekrd = 0;
		return (c);
	}
	if (Dcp && (c = *Dcp++))
		return (c&(QUOTE|TRIM));
	if (*Dvp == 0) {
		Dcp = 0;
		return (DEOF);
	}
	Dcp = *Dvp++;
	return (' ');
}

Dtestq(c)
	register int c;
{

	if (cmap(c, QUOTES))
		gflag = 1;
}

/*
 * Form a shell temporary file (in unit 0) from the words
 * of the shell input up to a line the same as "term".
 * Unit 0 should have been closed before this call.
 */
heredoc(term)
     tchar *term;
{
	register int c;
	tchar *Dv[2];
	tchar obuf[BUFSIZ], lbuf[BUFSIZ], mbuf[BUFSIZ];
	int ocnt, lcnt, mcnt;
	register tchar *lbp, *obp, *mbp;
	tchar **vp;
	bool quoted;

#ifdef TRACE
	tprintf("TRACE- heredoc()\n");
#endif
	if (creat_(shtemp, 0600) < 0)
		Perror(shtemp);
	(void) close(0);
	if (open_(shtemp, 2) < 0) {
		int oerrno = errno;

		(void) unlink_(shtemp);
		errno = oerrno;
		Perror(shtemp);
	}
	(void) unlink_(shtemp);			/* 0 0 inode! */
	Dv[0] = term; Dv[1] = NOSTR; gflag = 0;
	trim(Dv); rscan(Dv, Dtestq); quoted = gflag;
	ocnt = BUFSIZ; obp = obuf;
	for (;;) {
		/*
		 * Read up a line
		 */
		lbp = lbuf; lcnt = BUFSIZ - 4;
		for (;;) {
			c = readc(1);		/* 1 -> Want EOF returns */
			if (c < 0) {
				setname(term);
				bferr("<< terminator not found");
			}
			if (c == '\n')
				break;
			if (c &= TRIM) {
				*lbp++ = c;
				if (--lcnt < 0) {
					setname(S_LESLES/*"<<"*/);
					error("Line overflow");
				} 
			}
		}
		*lbp = 0;

		/*
		 * Compare to terminator -- before expansion
		 */
		if (eq(lbuf, term)) {
			(void) write_(0, obuf, BUFSIZ - ocnt);
			(void) lseek(0, (off_t)0, 0);
			return;
		}

		/*
		 * If term was quoted or -n just pass it on
		 */
		if (quoted || noexec) {
			*lbp++ = '\n'; *lbp = 0;
			for (lbp = lbuf; c = *lbp++;) {
				*obp++ = c;
				if (--ocnt == 0) {
					(void) write_(0, obuf, BUFSIZ);
					obp = obuf; ocnt = BUFSIZ;
				}
			}
			continue;
		}

		/*
		 * Term wasn't quoted so variable and then command
		 * expand the input line
		 */
		Dcp = lbuf; Dvp = Dv + 1; mbp = mbuf; mcnt = BUFSIZ - 4;
		for (;;) {
			c = DgetC(DODOL);
			if (c == DEOF)
				break;
			if ((c &= TRIM) == 0)
				continue;
			/* \ quotes \ $ ` here */
			if (c =='\\') {
				c = DgetC(0);
/*				if (!any(c, "$\\`"))*/
				if ((c!='$')&&(c!='\\')&&(c!='`'))
					unDgetC(c | QUOTE), c = '\\';
				else
					c |= QUOTE;
			}
			*mbp++ = c;
			if (--mcnt == 0) {
				setname(S_LESLES/*"<<"*/);
				bferr("Line overflow");
			}
		}
		*mbp++ = 0;

		/*
		 * If any ` in line do command substitution
		 */
		mbp = mbuf;
		if (any('`', mbp)) {
			/*
			 * 1 arg to dobackp causes substitution to be literal.
			 * Words are broken only at newlines so that all blanks
			 * and tabs are preserved.  Blank lines (null words)
			 * are not discarded.
			 */
			vp = dobackp(mbuf, 1);
		} else
			/* Setup trivial vector similar to return of dobackp */
			Dv[0] = mbp, Dv[1] = NOSTR, vp = Dv;

		/*
		 * Resurrect the words from the command substitution
		 * each separated by a newline.  Note that the last
		 * newline of a command substitution will have been
		 * discarded, but we put a newline after the last word
		 * because this represents the newline after the last
		 * input line!
		 */
		for (; *vp; vp++) {
			for (mbp = *vp; *mbp; mbp++) {
				*obp++ = *mbp & TRIM;
				if (--ocnt == 0) {
					(void) write_(0, obuf, BUFSIZ);
					obp = obuf; ocnt = BUFSIZ;
				}
			}
			*obp++ = '\n';
			if (--ocnt == 0) {
				(void) write_(0, obuf, BUFSIZ);
				obp = obuf; ocnt = BUFSIZ;
			}
		}
		if (pargv)
			blkfree(pargv), pargv = 0;
	}
}
