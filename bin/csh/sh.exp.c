/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.exp.c 1.1 92/07/30 SMI; from UCB 5.3 6/23/85";
#endif

#include "sh.h"
#include "sh.tconst.h"

/*
 * C shell
 */

#define IGNORE	1	/* in ignore, it means to ignore value, just parse */
#define NOGLOB	2	/* in ignore, it means not to globone */

#define	ADDOP	1
#define	MULOP	2
#define	EQOP	4
#define	RELOP	8
#define	RESTOP	16
#define	ANYOP	31

#define	EQEQ	1
#define	GTR	2
#define	LSS	4
#define	NOTEQ	6
#define EQMATCH 7
#define NOTEQMATCH 8

exp(vp)
	register tchar ***vp;
{
#ifdef TRACE
	tprintf("TRACE- exp()\n");
#endif

	return (exp0(vp, 0));
}

exp0(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register int p1 = exp1(vp, ignore);
#ifdef TRACE
	tprintf("TRACE- exp0()\n");
#endif
	
#ifdef EDEBUG
	etraci("exp0 p1", p1, vp);
#endif
	if (**vp && eq(**vp, S_BARBAR /*"||"*/)) {
		register int p2;

		(*vp)++;
		p2 = exp0(vp, (ignore&IGNORE) || p1);
#ifdef EDEBUG
		etraci("exp0 p2", p2, vp);
#endif
		return (p1 || p2);
	}
	return (p1);
}

exp1(vp, ignore)
	register tchar ***vp;
{
	register int p1 = exp2(vp, ignore);

#ifdef TRACE
	tprintf("TRACE- exp1()\n");
#endif
#ifdef EDEBUG
	etraci("exp1 p1", p1, vp);
#endif
	if (**vp && eq(**vp, S_ANDAND /*"&&" */)) {
		register int p2;

		(*vp)++;
		p2 = exp1(vp, (ignore&IGNORE) || !p1);
#ifdef EDEBUG
		etraci("exp1 p2", p2, vp);
#endif
		return (p1 && p2);
	}
	return (p1);
}

exp2(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register int p1 = exp2a(vp, ignore);

#ifdef TRACE
	tprintf("TRACE- exp2()\n");
#endif
#ifdef EDEBUG
	etraci("exp3 p1", p1, vp);
#endif
	if (**vp && eq(**vp, S_BAR /*"|" */)) {
		register int p2;

		(*vp)++;
		p2 = exp2(vp, ignore);
#ifdef EDEBUG
		etraci("exp3 p2", p2, vp);
#endif
		return (p1 | p2);
	}
	return (p1);
}

exp2a(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register int p1 = exp2b(vp, ignore);

#ifdef TRACE
	tprintf("TRACE- exp2a()\n");
#endif
#ifdef EDEBUG
	etraci("exp2a p1", p1, vp);
#endif
	if (**vp && eq(**vp, S_HAT /*"^" */)) {
		register int p2;

		(*vp)++;
		p2 = exp2a(vp, ignore);
#ifdef EDEBUG
		etraci("exp2a p2", p2, vp);
#endif
		return (p1 ^ p2);
	}
	return (p1);
}

exp2b(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register int p1 = exp2c(vp, ignore);

#ifdef TRACE
	tprintf("TRACE- exp2b()\n");
#endif
#ifdef EDEBUG
	etraci("exp2b p1", p1, vp);
#endif
	if (**vp && eq(**vp, S_AND /*"&"*/)) {
		register int p2;

		(*vp)++;
		p2 = exp2b(vp, ignore);
#ifdef EDEBUG
		etraci("exp2b p2", p2, vp);
#endif
		return (p1 & p2);
	}
	return (p1);
}

exp2c(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register tchar *p1 = exp3(vp, ignore);
	register tchar *p2;
	register int i;

#ifdef TRACE
	tprintf("TRACE- exp2c()\n");
#endif
#ifdef EDEBUG
	etracc("exp2c p1", p1, vp);
#endif
	if (i = isa(**vp, EQOP)) {
		(*vp)++;
		if (i == EQMATCH || i == NOTEQMATCH)
			ignore |= NOGLOB;
		p2 = exp3(vp, ignore);
#ifdef EDEBUG
		etracc("exp2c p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (i) {

		case EQEQ:
			i = eq(p1, p2);
			break;

		case NOTEQ:
			i = !eq(p1, p2);
			break;

		case EQMATCH:
			i = Gmatch(p1, p2);
			break;

		case NOTEQMATCH:
			i = !Gmatch(p1, p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (i);
	}
	i = egetn(p1);
	xfree(p1);
	return (i);
}

tchar *
exp3(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register tchar *p1, *p2;
	register int i;

#ifdef TRACE
	tprintf("TRACE- exp3()\n");
#endif
	p1 = exp3a(vp, ignore);
#ifdef EDEBUG
	etracc("exp3 p1", p1, vp);
#endif
	if (i = isa(**vp, RELOP)) {
		(*vp)++;
		if (**vp && eq(**vp, S_EQ /*"=" */))
			i |= 1, (*vp)++;
		p2 = exp3(vp, ignore);
#ifdef EDEBUG
		etracc("exp3 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (i) {

		case GTR:
			i = egetn(p1) > egetn(p2);
			break;

		case GTR|1:
			i = egetn(p1) >= egetn(p2);
			break;

		case LSS:
			i = egetn(p1) < egetn(p2);
			break;

		case LSS|1:
			i = egetn(p1) <= egetn(p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

tchar *
exp3a(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register tchar *p1, *p2, *op;
	register int i;

#ifdef TRACE
	tprintf("TRACE- exp3a()\n");
#endif
	p1 = exp4(vp, ignore);
#ifdef EDEBUG
	etracc("exp3a p1", p1, vp);
#endif
	op = **vp;
	/* if (op && any(op[0], "<>") && op[0] == op[1]) { */
	if (op && (op[0] == '<' || op[0] == '>') && op[0] == op[1]) {
		(*vp)++;
		p2 = exp3a(vp, ignore);
#ifdef EDEBUG
		etracc("exp3a p2", p2, vp);
#endif
		if (op[0] == '<')
			i = egetn(p1) << egetn(p2);
		else
			i = egetn(p1) >> egetn(p2);
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

tchar *
exp4(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register tchar *p1, *p2;
	register int i = 0;

#ifdef TRACE
	tprintf("TRACE- exp4()\n");
#endif
	p1 = exp5(vp, ignore);
#ifdef EDEBUG
	etracc("exp4 p1", p1, vp);
#endif
	if (isa(**vp, ADDOP)) {
		register tchar *op = *(*vp)++;

		p2 = exp4(vp, ignore);
#ifdef EDEBUG
		etracc("exp4 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (op[0]) {

		case '+':
			i = egetn(p1) + egetn(p2);
			break;

		case '-':
			i = egetn(p1) - egetn(p2);
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

tchar *
exp5(vp, ignore)
	register tchar ***vp;
	bool ignore;
{
	register tchar *p1, *p2;
	register int i = 0;

#ifdef TRACE
	tprintf("TRACE- exp5()\n");
#endif
	p1 = exp6(vp, ignore);
#ifdef EDEBUG
	etracc("exp5 p1", p1, vp);
#endif
	if (isa(**vp, MULOP)) {
		register tchar *op = *(*vp)++;

		p2 = exp5(vp, ignore);
#ifdef EDEBUG
		etracc("exp5 p2", p2, vp);
#endif
		if (!(ignore&IGNORE)) switch (op[0]) {

		case '*':
			i = egetn(p1) * egetn(p2);
			break;

		case '/':
			i = egetn(p2);
			if (i == 0)
				error("Divide by 0");
			i = egetn(p1) / i;
			break;

		case '%':
			i = egetn(p2);
			if (i == 0)
				error("Mod by 0");
			i = egetn(p1) % i;
			break;
		}
		xfree(p1), xfree(p2);
		return (putn(i));
	}
	return (p1);
}

tchar *
exp6(vp, ignore)
	register tchar ***vp;
{
	int ccode, i;
	register tchar *cp, *dp, *ep;

#ifdef TRACE
	tprintf("TRACE- exp6()\n");
#endif
	if (**vp == 0)
		bferr("Expression syntax");
	if (eq(**vp, S_EXAS /* "!" */)) {
		(*vp)++;
		cp = exp6(vp, ignore);
#ifdef EDEBUG
		etracc("exp6 ! cp", cp, vp);
#endif
		i = egetn(cp);
		xfree(cp);
		return (putn(!i));
	}
	if (eq(**vp, S_TIL /*"~" */)) {
		(*vp)++;
		cp = exp6(vp, ignore);
#ifdef EDEBUG
		etracc("exp6 ~ cp", cp, vp);
#endif
		i = egetn(cp);
		xfree(cp);
		return (putn(~i));
	}
	if (eq(**vp, S_LPAR /*"(" */)) {
		(*vp)++;
		ccode = exp0(vp, ignore);
#ifdef EDEBUG
		etraci("exp6 () ccode", ccode, vp);
#endif
		if (*vp == 0 || **vp == 0 || ***vp != ')')
			bferr("Expression syntax");
		(*vp)++;
		return (putn(ccode));
	}
	if (eq(**vp, S_LBRA /* "{" */)) {
		register tchar **v;
		struct command faket;
		tchar *fakecom[2];

		faket.t_dtyp = TCOM;
		faket.t_dflg = 0;
		faket.t_dcar = faket.t_dcdr = faket.t_dspr = (struct command *)0;
		faket.t_dcom = fakecom;
		fakecom[0] = S_BRAPPPBRA /*"{ ... }" */;
		fakecom[1] = NOSTR;
		(*vp)++;
		v = *vp;
		for (;;) {
			if (!**vp)
				bferr("Missing }");
			if (eq(*(*vp)++, S_RBRA /*"}" */))
				break;
		}
		if (ignore&IGNORE)
			return (S_ /*""*/);
		psavejob();
		if (pfork(&faket, -1) == 0) {
			*--(*vp) = 0;
			evalav(v);
			exitstat();
		}
		pwait();
		prestjob();
#ifdef EDEBUG
		etraci("exp6 {} status", egetn(value("status")), vp);
#endif
		return (putn(egetn(value(S_status /*"status" */)) == 0));
	}
	if (isa(**vp, ANYOP))
		return (S_ /*""*/);
	cp = *(*vp)++;
	if (*cp == '-' && any(cp[1], S_erwxfdzo /*"erwxfdzo" */)) {
		struct stat stb;

		if (cp[2] != '\0')
			bferr("Malformed file inquiry");

		/*
		 * Detect missing file names by checking for operator
		 * in the file name position.  However, if an operator
		 * name appears there, we must make sure that there's
		 * no file by that name (e.g., "/") before announcing
		 * an error.  Even this check isn't quite right, since
		 * it doesn't take globbing into account.
		 */
		if (isa(**vp, ANYOP) && stat_(**vp, &stb))
			bferr("Missing file name");
		dp = *(*vp)++;

		if (ignore&IGNORE)
			return (S_ /*""*/);
		ep = globone(dp);
		switch (cp[1]) {

		case 'r':
			i = !access_(ep, 4);
			break;

		case 'w':
			i = !access_(ep, 2);
			break;

		case 'x':
			i = !access_(ep, 1);
			break;

		default:
			if (stat_(ep, &stb)) {
				xfree(ep);
				return (S_0 /*"0"*/);
			}
			switch (cp[1]) {

			case 'f':
				i = (stb.st_mode & S_IFMT) == S_IFREG;
				break;

			case 'd':
				i = (stb.st_mode & S_IFMT) == S_IFDIR;
				break;

			case 'z':
				i = stb.st_size == 0;
				break;

			case 'e':
				i = 1;
				break;

			case 'o':
				i = stb.st_uid == uid;
				break;
			}
		}
#ifdef EDEBUG
		etraci("exp6 -? i", i, vp);
#endif
		xfree(ep);
		return (putn(i));
	}
#ifdef EDEBUG
	etracc("exp6 default", cp, vp);
#endif
	return (ignore&NOGLOB ? savestr(cp) : globone(cp));
}

evalav(v)
	register tchar **v;
{
	struct wordent paraml;
	register struct wordent *hp = &paraml;
	struct command *t;
	register struct wordent *wdp = hp;
	
#ifdef TRACE
	tprintf("TRACE- evalav()\n");
#endif
	set(S_status /*"status" */, S_0 /*"0"*/);
	hp->prev = hp->next = hp;
	hp->word = S_ /*""*/;
	while (*v) {
		register struct wordent *new = (struct wordent *) calloc(1, sizeof *wdp);

		new->prev = wdp;
		new->next = hp;
		wdp->next = new;
		wdp = new;
		wdp->word = savestr(*v++);
	}
	hp->prev = wdp;
	alias(&paraml);
	t = syntax(paraml.next, &paraml, 0);
	if (err)
		error(err);
	execute(t, -1);
	freelex(&paraml), freesyn(t);
}

isa(cp, what)
	register tchar *cp;
	register int what;
{

#ifdef TRACE
	tprintf("TRACE- isa()\n");
#endif
	if (cp == 0)
		return ((what & RESTOP) != 0);
	if (cp[1] == 0) {
		if (what & ADDOP && (*cp == '+' || *cp == '-'))
			return (1);
		if (what & MULOP && (*cp == '*' || *cp == '/' || *cp == '%'))
			return (1);
		if (what & RESTOP && (*cp == '(' || *cp == ')' || *cp == '!' ||
				      *cp == '~' || *cp == '^' || *cp == '"'))
			return (1);
	} else if (cp[2] == 0) {
		if (what & RESTOP) {
			if (cp[0] == '|' && cp[1] == '&')
				return (1);
			if (cp[0] == '<' && cp[1] == '<')
				return (1);
			if (cp[0] == '>' && cp[1] == '>')
				return (1);
		}
		if (what & EQOP) {
			if (cp[0] == '=') {
				if (cp[1] == '=')
					return (EQEQ);
				if (cp[1] == '~')
					return (EQMATCH);
			} else if (cp[0] == '!') {
				if (cp[1] == '=')
					return (NOTEQ);
				if (cp[1] == '~')
					return (NOTEQMATCH);
			}
		}
	}
	if (what & RELOP) {
		if (*cp == '<')
			return (LSS);
		if (*cp == '>')
			return (GTR);
	}
	return (0);
}

egetn(cp)
	register tchar *cp;
{

#ifdef TRACE
	tprintf("TRACE- egetn()\n");
#endif
	if (*cp && *cp != '-' && !digit(*cp))
		bferr("Expression syntax");
	return (getn(cp));
}

/* Phew! */

#ifdef EDEBUG
etraci(str, i, vp)
	tchar *str;
	int i;
	tchar ***vp;
{

	printf("%s=%d\t", str, i);
	blkpr(*vp);
	printf("\n");
}

etracc(str, cp, vp)
	tchar *str, *cp;
	tchar ***vp;
{

	printf("%s=%s\t", str, cp);
	blkpr(*vp);
	printf("\n");
}
#endif
