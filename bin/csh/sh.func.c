/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley Software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char *sccsid = "@(#)sh.func.c 1.1 92/07/30 SMI; from UCB 5.3 5/13/86";
#endif

#include "sh.h"
#include <sys/ioctl.h>
#include <locale.h>	/* For LC_ALL */
#include "sh.tconst.h"
/*
 * C shell
 */

struct biltins *
isbfunc(t)
	struct command *t;
{
	register tchar *cp = t->t_dcom[0];
	register struct biltins *bp, *bp1, *bp2;
	int dolabel(), dofg1(), dobg1();

	static struct biltins label = { S_/*""*/, dolabel, 0, 0 };
	static struct biltins foregnd = { S_Pjob /*"%job"*/, dofg1, 0, 0 };
	static struct biltins backgnd = { S_PjobAND /*"%job &"*/, dobg1, 0, 0 };
#ifdef TRACE
	tprintf("TRACE- isbfunc()\n");
#endif
	if (lastchr(cp) == ':') {
		label.bname = cp;
		return (&label);
	}
	if (*cp == '%') {
		if (t->t_dflg & FAND) {
			t->t_dflg &= ~FAND;
			backgnd.bname = cp;
			return (&backgnd);
		}
		foregnd.bname = cp;
		return (&foregnd);
	}
	/*
	 * Binary search
	 * Bp1 is the beginning of the current search range.
	 * Bp2 is one past the end.
	 */
	for (bp1 = bfunc, bp2 = bfunc + nbfunc; bp1 < bp2;) {
		register i;

		bp = bp1 + (bp2 - bp1 >> 1);
		if ((i = *cp - *bp->bname) == 0 &&
		    (i = strcmp_(cp, bp->bname)) == 0)
			return bp;
		if (i < 0)
			bp2 = bp;
		else
			bp1 = bp + 1;
	}
	return (0);
}

func(t, bp)
	register struct command *t;
	register struct biltins *bp;
{
	int i;

#ifdef TRACE
	tprintf("TRACE- func()\n");
#endif
	xechoit(t->t_dcom);
	setname(bp->bname);
	i = blklen(t->t_dcom) - 1;
	if (i < bp->minargs)
		bferr("Too few arguments");
	if (i > bp->maxargs)
		bferr("Too many arguments");
	(*bp->bfunct)(t->t_dcom, t);
}

dolabel()
{
#ifdef TRACE
	tprintf("TRACE- dolabel()\n");
#endif

}

doonintr(v)
	tchar **v;
{
	register tchar *cp;
	register tchar *vv = v[1];

#ifdef TRACE
	tprintf("TRACE- doonintr()\n");
#endif
	if (parintr == SIG_IGN)
		return;
	if (setintr && intty)
		bferr("Can't from terminal");
	cp = gointr, gointr = 0, xfree(cp);
	if (vv == 0) {
		if (setintr)
			(void) sigblock(sigmask(SIGINT));
		else
			(void) signal(SIGINT, SIG_DFL);
		gointr = 0;
	} else if (eq((vv = strip(vv)), S_MINUS/*"-"*/)) {
		(void) signal(SIGINT, SIG_IGN);
		gointr = S_MINUS/*"-"*/;
	} else {
		gointr = savestr(vv);
		(void) signal(SIGINT, pintr);
	}
}

donohup()
{

#ifdef TRACE
	tprintf("TRACE- donohup()\n");
#endif
	if (intty)
		bferr("Can't from terminal");
#ifdef	INTR_UNSET
	if (setintr == 0) {
#endif
		(void) signal(SIGHUP, SIG_IGN);
#ifdef CC
		submit(getpid());
#endif
#ifdef	INTR_UNSET
	}
#endif
}

dozip()
{

	;
}

prvars()
{
#ifdef TRACE
	tprintf("TRACE- prvars()\n");
#endif

	plist(&shvhed);
}

doalias(v)
	register tchar **v;
{
	register struct varent *vp;
	register tchar *p;

#ifdef TRACE
	tprintf("TRACE- doalias()\n");
#endif
	v++;
	p = *v++;
	if (p == 0)
		plist(&aliases);
	else if (*v == 0) {
		vp = adrof1(strip(p), &aliases);
		if (vp)
			blkpr(vp->vec), printf("\n");
	} else {
		if (eq(p, S_alias/*"alias"*/) || eq(p, S_unalias/*"unalias"*/)) {
			setname(p);
			bferr("Too dangerous to alias that");
		}
		set1(strip(p), saveblk(v), &aliases);
	}
}

unalias(v)
	tchar **v;
{

#ifdef TRACE
	tprintf("TRACE- unalias()\n");
#endif
	unset1(v, &aliases);
}

dologout()
{

#ifdef TRACE
	tprintf("TRACE- dologout()\n");
#endif
	islogin();
	goodbye();
}

dologin(v)
	tchar **v;
{

	char *v_;	/* work */
#ifdef TRACE
	tprintf("TRACE- dologin()\n");
#endif
	islogin();
	rechist();
	(void) signal(SIGTERM, parterm);
	if (v[1] != NULL)			
		v_ = tstostr(NULL, v[1]);	/* No need to free */
	else
		v_ = 0;
	execl("/bin/login", "login", v_, 0);
	untty();
	exit(1);
}

#ifdef NEWGRP
donewgrp(v)
	tchar **v;
{

	char *v_;	/* work */
#ifdef TRACE
	tprintf("TRACE- donewgrp()\n");
#endif
	if (chkstop == 0 && setintr)
		panystop(0);
	(void) signal(SIGTERM, parterm);

	if (v[1] != NULL)
		v_ = tstostr(NOSTR, v[1]);	/* No need to free */
	else
		v_ = 0;
	execl("/bin/newgrp", "newgrp", v_, 0);
	execl("/usr/bin/newgrp", "newgrp", v_, 0);
	untty();
	exit(1);
}
#endif

islogin()
{

#ifdef TRACE
	tprintf("TRACE- islogin()\n");
#endif
	if (chkstop == 0 && setintr)
		panystop(0);
	if (loginsh)
		return;
	error("Not login shell");
}

doif(v, kp)
	tchar **v;
	struct command *kp;
{
	register int i;
	register tchar **vv;

#ifdef TRACE
	tprintf("TRACE- doif()\n");
#endif
	v++;
	i = exp(&v);
	vv = v;
	if (*vv == NOSTR)
		bferr("Empty if");
	if (eq(*vv, S_then /*"then"*/)) {
		if (*++vv)
			bferr("Improper then");
		setname(S_then/*"then"*/);
		/*
		 * If expression was zero, then scan to else,
		 * otherwise just fall into following code.
		 */
		if (!i)
			search(ZIF, 0);
		return;
	}
	/*
	 * Simple command attached to this if.
	 * Left shift the node in this tree, munging it
	 * so we can reexecute it.
	 */
	if (i) {
		lshift(kp->t_dcom, vv - kp->t_dcom);
		reexecute(kp);
		donefds();
	}
}

/*
 * Reexecute a command, being careful not
 * to redo i/o redirection, which is already set up.
 */
reexecute(kp)
	register struct command *kp;
{

#ifdef TRACE
	tprintf("TRACE- reexecute()\n");
#endif
	kp->t_dflg &= FSAVE;
	kp->t_dflg |= FREDO;
	/*
	 * If tty is still ours to arbitrate, arbitrate it;
	 * otherwise dont even set pgrp's as the jobs would
	 * then have no way to get the tty (we can't give it
	 * to them, and our parent wouldn't know their pgrp, etc.
	 */
	execute(kp, tpgrp > 0 ? tpgrp : -1);
}

doelse()
{

#ifdef TRACE
	tprintf("TRACE- doelse()\n");
#endif
	search(ZELSE, 0);
}

dogoto(v)
tchar **v;
{
	register struct whyle *wp;
	tchar *lp;
#ifdef TRACE
	tprintf("TRACE- dogoto()\n");
#endif

	/*
	 * While we still can, locate any unknown ends of existing loops.
	 * This obscure code is the WORST result of the fact that we
	 * don't really parse.
	 */
	for (wp = whyles; wp; wp = wp->w_next)
		if (wp->w_end == 0) {
			search(ZBREAK, 0);
			wp->w_end = btell();
		} else
			bseek(wp->w_end);
	search(ZGOTO, 0, lp = globone(v[1]));
	xfree(lp);
	/*
	 * Eliminate loops which were exited.
	 */
	wfree();
}

doswitch(v)
	register tchar **v;
{
	register tchar *cp, *lp;

#ifdef TRACE
	tprintf("TRACE- doswitch()\n");
#endif
	v++;
	if (!*v || *(*v++) != '(')
		goto syntax;
	cp = **v == ')' ? S_/*""*/ : *v++;
	if (*(*v++) != ')')
		v--;
	if (*v)
syntax:
		error("Syntax error");
	search(ZSWITCH, 0, lp = globone(cp));
	xfree(lp);
}

dobreak()
{

#ifdef TRACE
	tprintf("TRACE- dobreak()\n");
#endif
	if (whyles)
		toend();
	else
		bferr("Not in while/foreach");
}

doexit(v)
	tchar **v;
{

#ifdef TRACE
	tprintf("TRACE- doexit()\n");
#endif
	if (chkstop == 0)
		panystop(0);
	/*
	 * Don't DEMAND parentheses here either.
	 */
	v++;
	if (*v) {
		set(S_status/*"status"*/, putn(exp(&v)));
		if (*v)
			bferr("Expression syntax");
	}
	btoeof();
	if (intty)
		(void) close(SHIN);
}

doforeach(v)
	register tchar **v;
{
	register tchar *cp;
	register struct whyle *nwp;

#ifdef TRACE
	tprintf("TRACE- doforeach()\n");
#endif
	v++;
	cp = strip(*v);
	while (*cp && alnum(*cp))
		cp++;
	if (*cp || strlen_(*v) >= 20 || !letter(**v))
		bferr("Invalid variable");
	cp = *v++;
	if (v[0][0] != '(' || v[blklen(v) - 1][0] != ')')
		bferr("Words not ()'ed");
	v++;
	gflag = 0, tglob(v);
	v = glob(v);
	if (v == 0)
		bferr("No match");
	nwp = (struct whyle *) calloc(1, sizeof *nwp);
	nwp->w_fe = nwp->w_fe0 = v; gargv = 0;
	nwp->w_start = btell();
	nwp->w_fename = savestr(cp);
	nwp->w_next = whyles;
	whyles = nwp;
	/*
	 * Pre-read the loop so as to be more
	 * comprehensible to a terminal user.
	 */
	if (intty)
		preread_();
	doagain();
}

dowhile(v)
	tchar **v;
{
	register int status;
	register bool again = whyles != 0 && whyles->w_start == lineloc &&
	    whyles->w_fename == 0;

#ifdef TRACE
	tprintf("TRACE- dowhile()\n");
#endif
	v++;
	/*
	 * Implement prereading here also, taking care not to
	 * evaluate the expression before the loop has been read up
	 * from a terminal.
	 */
	if (intty && !again)
		status = !exp0(&v, 1);
	else
		status = !exp(&v);
	if (*v)
		bferr("Expression syntax");
	if (!again) {
		register struct whyle *nwp = (struct whyle *) calloc(1, sizeof (*nwp));

		nwp->w_start = lineloc;
		nwp->w_end = 0;
		nwp->w_next = whyles;
		whyles = nwp;
		if (intty) {
			/*
			 * The tty preread
			 */
			preread_();
			doagain();
			return;
		}
	}
	if (status)
		/* We ain't gonna loop no more, no more! */
		toend();
}

preread_()
{
#ifdef TRACE
	tprintf("TRACE- preread()\n");
#endif

	whyles->w_end = -1;
	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
	search(ZBREAK, 0);
	if (setintr)
		(void) sigblock(sigmask(SIGINT));
	whyles->w_end = btell();
}

doend()
{

#ifdef TRACE
	tprintf("TRACE- doend()\n");
#endif
	if (!whyles)
		bferr("Not in while/foreach");
	whyles->w_end = btell();
	doagain();
}

docontin()
{
#ifdef TRACE
	tprintf("TRACE- docontin()\n");
#endif

	if (!whyles)
		bferr("Not in while/foreach");
	doagain();
}

doagain()
{

#ifdef TRACE
	tprintf("TRACE- doagain()\n");
#endif
	/* Repeating a while is simple */
	if (whyles->w_fename == 0) {
		bseek(whyles->w_start);
		return;
	}
	/*
	 * The foreach variable list actually has a spurious word
	 * ")" at the end of the w_fe list.  Thus we are at the
	 * of the list if one word beyond this is 0.
	 */
	if (!whyles->w_fe[1]) {
		dobreak();
		return;
	}
	set(whyles->w_fename, savestr(*whyles->w_fe++));
	bseek(whyles->w_start);
}

dorepeat(v, kp)
	tchar **v;
	struct command *kp;
{
	register int i, omask;

#ifdef TRACE
	tprintf("TRACE- dorepeat()\n");
#endif
	i = getn(v[1]);
	if (setintr)
		omask = sigblock(sigmask(SIGINT)) & ~sigmask(SIGINT);
	lshift(v, 2);
	while (i > 0) {
		if (setintr)
			(void) sigsetmask(omask);
		reexecute(kp);
		--i;
	}
	donefds();
	if (setintr)
		(void) sigsetmask(omask);
}

doswbrk()
{

#ifdef TRACE
	tprintf("TRACE- doswbrk()\n");
#endif
	search(ZBRKSW, 0);
}

srchx(cp)
	register tchar *cp;
{
	register struct srch *sp, *sp1, *sp2;
	register i;

#ifdef TRACE
	tprintf("TRACE- srchx()\n");
#endif
	/*
	 * Binary search
	 * Sp1 is the beginning of the current search range.
	 * Sp2 is one past the end.
	 */
	for (sp1 = srchn, sp2 = srchn + nsrchn; sp1 < sp2;) {
		sp = sp1 + (sp2 - sp1 >> 1);
		if ((i = *cp - *sp->s_name) == 0 &&
		    (i = strcmp_(cp, sp->s_name)) == 0)
			return sp->s_value;
		if (i < 0)
			sp2 = sp;
		else
			sp1 = sp + 1;
	}
	return (-1);
}

tchar Stype;
tchar *Sgoal;

/*VARARGS2*/
search(type, level, goal)
     int type;
     register int level;
     tchar *goal;
{
	tchar wordbuf[BUFSIZ];
	register tchar *aword = wordbuf;
	register tchar *cp;

#ifdef TRACE
	tprintf("TRACE- search()\n");
#endif
	Stype = type; Sgoal = goal;
	if (type == ZGOTO)
		bseek((off_t)0);
	do {
		if (intty && fseekp == feobp)
			printf("? "), flush();
		aword[0] = 0;
		(void) getword(aword);
		switch (srchx(aword)) {

		case ZELSE:
			if (level == 0 && type == ZIF)
				return;
			break;

		case ZIF:
			while (getword(aword))
				continue;
			if ((type == ZIF || type == ZELSE) && eq(aword, S_then/*"then"*/))
				level++;
			break;

		case ZENDIF:
			if (type == ZIF || type == ZELSE)
				level--;
			break;

		case ZFOREACH:
		case ZWHILE:
			if (type == ZBREAK)
				level++;
			break;

		case ZEND:
			if (type == ZBREAK)
				level--;
			break;

		case ZSWITCH:
			if (type == ZSWITCH || type == ZBRKSW)
				level++;
			break;

		case ZENDSW:
			if (type == ZSWITCH || type == ZBRKSW)
				level--;
			break;

		case ZLABEL:
			if (type == ZGOTO && getword(aword) && eq(aword, goal))
				level = -1;
			break;

		default:
			if (type != ZGOTO && (type != ZSWITCH || level != 0))
				break;
			if (lastchr(aword) != ':')
				break;
			aword[strlen_(aword) - 1] = 0;
			if (type == ZGOTO && eq(aword, goal) || type == ZSWITCH && eq(aword, S_default))
				level = -1;
			break;

		case ZCASE:
			if (type != ZSWITCH || level != 0)
				break;
			(void) getword(aword);
			if (lastchr(aword) == ':')
				aword[strlen_(aword) - 1] = 0;
			cp = strip(Dfix1(aword));
			if (Gmatch(goal, cp))
				level = -1;
			xfree(cp);
			break;

		case ZDEFAULT:
			if (type == ZSWITCH && level == 0)
				level = -1;
			break;
		}
		(void) getword(NOSTR);
	} while (level >= 0);
}

getword(wp)
	register tchar *wp;
{
	register int found = 0;
	register int c, d;
#ifdef TRACE
	tprintf("TRACE- getword()\n");
#endif

	c = readc(1);
	d = 0;
	do {
		while (issp(c))
			c = readc(1);
		if (c == '#')
			do
				c = readc(1);
			while (c >= 0 && c != '\n');
		if (c < 0)
			goto past;
		if (c == '\n') {
			if (wp)
				break;
			return (0);
		}
		unreadc(c);
		found = 1;
		do {
			c = readc(1);
			if (c == '\\' && (c = readc(1)) == '\n')
				c = ' ';
			if (c == '\'' || c == '"')
				if (d == 0)
					d = c;
				else if (d == c)
					d = 0;
			if (c < 0)
				goto past;
			if (wp)
				*wp++ = c;
		} while ((d || !issp(c)) && c != '\n');
/*WAS:		} while ((d || c != ' ' && c != '\t') && c != '\n');*/
	} while (wp == 0);
	unreadc(c);
	if (found)
		*--wp = 0;
	return (found);

past:
	switch (Stype) {

	case ZIF:
		bferr("then/endif not found");

	case ZELSE:
		bferr("endif not found");

	case ZBRKSW:
	case ZSWITCH:
		bferr("endsw not found");

	case ZBREAK:
		bferr("end not found");

	case ZGOTO:
		setname(Sgoal);
		bferr("label not found");
	}
	/*NOTREACHED*/
}

toend()
{

#ifdef TRACE
	tprintf("TRACE- toend()\n");
#endif
	if (whyles->w_end == 0) {
		search(ZBREAK, 0);
		whyles->w_end = btell() - 1;
	} else
		bseek(whyles->w_end);
	wfree();
}

wfree()
{
	long o = btell();

#ifdef TRACE
	tprintf("TRACE- wfree()\n");
#endif
	while (whyles) {
		register struct whyle *wp = whyles;
		register struct whyle *nwp = wp->w_next;

		if (o >= wp->w_start && (wp->w_end == 0 || o < wp->w_end))
			break;
		if (wp->w_fe0)
			blkfree(wp->w_fe0);
		if (wp->w_fename)
			xfree(wp->w_fename);
		xfree( (char *)wp);
		whyles = nwp;
	}
}

doecho(v)
	tchar **v;
{

#ifdef TRACE
	tprintf("TRACE- doecho()\n");
#endif
	echo(' ', v);
}

doglob(v)
	tchar **v;
{

#ifdef TRACE
	tprintf("TRACE- doglob()\n");
#endif
	echo(0, v);
	flush();
}

echo(sep, v)
	tchar sep;
	register tchar **v;
{
	register tchar *cp;
	int nonl = 0;

#ifdef TRACE
	tprintf("TRACE- echo()\n");
#endif
	if (setintr)
		(void) sigsetmask(sigblock(0) & ~sigmask(SIGINT));
	v++;
	if (*v == 0)
		return;
	gflag = 0, tglob(v);
	if (gflag) {
		v = glob(v);
		if (v == 0)
			bferr("No match");
	} else
		trim(v);
	if (sep == ' ' && *v && !strcmp_(*v, S_n/*"-n"*/))
		nonl++, v++;
	while (cp = *v++) {
		register int c;

		while (c = *cp++)
			putchar(c | QUOTE);
		if (*v)
			putchar(sep | QUOTE);
	}
	if (sep && nonl == 0)
		putchar('\n');
	else
		flush();
	if (setintr)
		(void) sigblock(sigmask(SIGINT));
	if (gargv)
		blkfree(gargv), gargv = 0;
}

char **environ; 

/* Check if the environment variable vp affects this csh's behavior
 * and therefore we should call setlocale() or not.
 */
static bool
islocalevar(vp)
	tchar *vp;
{      
	static tchar *categories_we_care[]
	    ={S_LANG, S_LC_CTYPE, S_LC_MESSAGES, NOSTR};
	tchar **p=categories_we_care;

	do{
		if( strcmp_(vp, *p)==0 ) return 1;
	}while(*(++p));
	return 0;
}

dosetenv(v)
	register tchar **v;
{
	tchar *vp, *lp;

#ifdef TRACE
	tprintf("TRACE- dosetenv()\n");
#endif
	v++;
	if ((vp = *v++) == 0) {
		register char **ep;

		if (setintr)
			(void) sigsetmask(sigblock(0) & ~ sigmask(SIGINT));
		for (ep = environ; *ep; ep++)
			printf("%s\n", *ep);
		return;
	}
	if ((lp = *v++) == 0)
		lp = S_/*""*/;
	setenv(vp, lp = globone(lp));
	if (eq(vp, S_PATH/*"PATH"*/)) {
		importpath(lp);
		dohash();
	}else if(islocalevar(vp)) {
		if(!setlocale(LC_ALL, "")){
			error("Cannot find the locale named %t - Ignored", lp);
		}
	}

	xfree(lp);
}

dounsetenv(v)
	register tchar **v;
{
	bool	locale_changed=0;
#ifdef TRACE
	tprintf("TRACE- dounsetenv()\n");
#endif
	v++;
	do{
		unsetenv(*v);
		if(islocalevar(*v++)) locale_changed=1;
	}while (*v);
	if(locale_changed) setlocale(LC_ALL, "");/* Hope no error! */
}

setenv(name, val)
	tchar *name, *val;
{
	register char **ep = environ;
	register tchar *cp; 
	register char *dp;
	register tchar *ep_;	/* temporary */
	char *blk[2], **oep = ep;

#ifdef TRACE
	/* tprintf("TRACE- setenv(%t, %t)\n", name, val); */
	/* printf("IN setenv args = (%t)\n", val); */
#endif
	for (; *ep; ep++) {
#ifdef MBCHAR
		for (cp = name, dp = *ep; *cp && *dp; cp++){
			/* This loop compares two chars in different
			 * representations, EUC (as char *) and wchar_t 
			 * (in tchar), and ends when they are different.
			 */
			wchar_t	dwc;
			int	n;

			n=mbtowc(&dwc, dp, MB_CUR_MAX);
			if(n<=0) break; /* Illegal multibyte. */
			dp+=n; /* Advance to next multibyte char. */
			if(dwc == (wchar_t)*cp) continue;
			else break; 
		}
#else/*!MBCHAR*/
		for (cp = name, dp = *ep; *cp && (char)*cp == *dp; cp++, dp++)
			continue;
#endif/*!MBCHAR*/
		if (*cp != 0 || *dp != '=')
			continue;
		cp = strspl(S_EQ/*"="*/, val);
		xfree(*ep);
		ep_ = strspl(name, cp);	/* ep_ is xalloc'ed */
		xfree(cp);
		/* Trimming is not needed here.
		 * trim();
		 */
		*ep = tstostr(NULL, ep_);
		xfree(ep_);			/* because temp.  use */
		return;
	}
	ep_ = strspl(name, S_EQ/*"="*/); /* ep_ is xalloc'ed */
	blk[0] = tstostr(NULL, ep_);
	blk[1] = 0;
	xfree(ep_);	
	environ = blkspl_(environ, blk);
	xfree( (void *)oep);
	setenv(name, val);
}

unsetenv(name)
	tchar *name;
{
	register char **ep = environ;
	register tchar *cp;
	register char *dp;
	char **oep = ep;
	char *cp_;	/* tmp use */
	static cnt = 0;	/* delete counter */

#ifdef TRACE
	tprintf("TRACE- unsetenv()\n");
#endif
	for (; *ep; ep++) {
#ifdef MBCHAR
		for (cp = name, dp = *ep; *cp && *dp; cp++){
			/* This loop compares two chars in different
			 * representations, EUC (as char *) and wchar_t 
			 * (in tchar), and ends when they are different.
			 */
			wchar_t	dwc;
			int	n;

			n=mbtowc(&dwc, dp, MB_CUR_MAX);
			if(n<=0) break; /* Illegal multibyte. */
			dp+=n; /* Advance to next multibyte char. */
			if(dwc == (wchar_t)*cp) continue;
			else break; 
		}
#else/*!MBCHAR*/
		for (cp = name, dp = *ep; *cp && (char)*cp == *dp; cp++, dp++)
			continue;
#endif/*!MBCHAR*/
		if (*cp != 0 || *dp != '=')
			continue;
		cp_ = *ep;
		*ep = 0;
		environ = blkspl_(environ, ep+1);
		*ep = cp_;
		xfree(cp_);
		xfree((void *)oep);
		return;
	}
}

doumask(v)
	register tchar **v;
{
	register tchar *cp = v[1];
	register int i;

#ifdef TRACE
	tprintf("TRACE- dounmask()\n");
#endif
	if (cp == 0) {
		i = umask(0);
		(void) umask(i);
		printf("%o\n", i);
		return;
	}
	i = 0;
	while (digit(*cp) && *cp != '8' && *cp != '9')
		i = i * 8 + *cp++ - '0';
	if (*cp || i < 0 || i > 0777)
		bferr("Improper mask");
	(void) umask(i);
}


struct limits {
	int	limconst;
	tchar *limname;
	int	limdiv;
	tchar *limscale;
} limits[] = {
	RLIMIT_CPU,	S_cputime/*"cputime"*/,1,S_seconds/*"seconds"*/,
	RLIMIT_FSIZE,	S_filesize/*"filesize"*/,1024,S_kbytes/*"kbytes"*/,
	RLIMIT_DATA,	S_datasize/*"datasize"*/,1024,S_kbytes/*"kbytes"*/,
	RLIMIT_STACK,	S_stacksize/*"stacksize"*/,1024,S_kbytes/*"kbytes"*/,
	RLIMIT_CORE,	S_coredumpsize/*"coredumpsize"*/,1024,S_kbytes/*"kbytes"*/,
	RLIMIT_RSS,	S_memorysize/*"memoryuse"*/,	1024,S_kbytes/*"kbytes*/,
#ifdef RLIMIT_NOFILE	/* SunOS 4.1 and later. */
	RLIMIT_NOFILE,	S_descriptors,	1,	S_,
#endif
	-1,		0,
};

struct limits *
findlim(cp)
	tchar *cp;
{
	register struct limits *lp, *res;

#ifdef TRACE
	tprintf("TRACE- findlim()\n");
#endif
	res = 0;
	for (lp = limits; lp->limconst >= 0; lp++)
		if (prefix(cp, lp->limname)) {
			if (res)
				bferr("Ambiguous");
			res = lp;
		}
	if (res)
		return (res);
	bferr("No such limit");
	/*NOTREACHED*/
}

dolimit(v)
	register tchar **v;
{
	register struct limits *lp;
	register int limit;
	tchar hard = 0;

#ifdef TRACE
	tprintf("TRACE- dolimit()\n");
#endif
	v++;
	if (*v && eq(*v, S_h/*"-h"*/)) {
		hard = 1;
		v++;
	}
	if (*v == 0) {
		for (lp = limits; lp->limconst >= 0; lp++)
			plim(lp, hard);
		return;
	}
	lp = findlim(v[0]);
	if (v[1] == 0) {
		plim(lp,  hard);
		return;
	}
	limit = getval(lp, v+1);
	if (setlim(lp, hard, limit) < 0)
		error(NOSTR);
}

getval(lp, v)
	register struct limits *lp;
	tchar **v;
{
	register float f;
	double atof_();
	tchar *cp = *v++;

#ifdef TRACE
	tprintf("TRACE- getval()\n");
#endif
	f = atof_(cp);
	while (digit(*cp) || *cp == '.' || *cp == 'e' || *cp == 'E')
		cp++;
	if (*cp == 0) {
		if (*v == 0)
			return ((int)(f+0.5) * lp->limdiv);
		cp = *v;
	}
	switch (*cp) {

	case ':':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		return ((int)(f * 60.0 + atof_(cp+1)));

	case 'h':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		limtail(cp, S_hours/*"hours"*/);
		f *= 3600.;
		break;

	case 'm':
		if (lp->limconst == RLIMIT_CPU) {
			limtail(cp, S_minutes/*"minutes"*/);
			f *= 60.;
			break;
		}
	case 'M':
		if (lp->limconst == RLIMIT_CPU)
			goto badscal;
		*cp = 'm';
		limtail(cp, S_megabytes/*"megabytes"*/);
		f *= 1024.*1024.;
		break;

	case 's':
		if (lp->limconst != RLIMIT_CPU)
			goto badscal;
		limtail(cp, S_seconds/*"seconds"*/);
		break;

	case 'k':
		if (lp->limconst == RLIMIT_CPU)
			goto badscal;
		limtail(cp, S_kbytes/*"kbytes"*/);
		f *= 1024;
		break;

	case 'u':
		limtail(cp, S_unlimited/*"unlimited"*/);
		return (RLIM_INFINITY);

	default:
badscal:
		bferr("Improper or unknown scale factor");
	}
	return ((int)(f+0.5));
}

limtail(cp, str0)
	tchar *cp, *str0;
{
	register tchar *str = str0;
#ifdef TRACE
	tprintf("TRACE- limtail()\n");
#endif

	while (*cp && *cp == *str)
		cp++, str++;
	if (*cp)
		error("Bad scaling; did you mean ``%t''?", str0);
}

plim(lp, hard)
	register struct limits *lp;
	tchar hard;
{
	struct rlimit rlim;
	u_long limit;

#ifdef TRACE
	tprintf("TRACE- plim()\n");
#endif
	printf("%t \t", lp->limname);
	(void) getrlimit(lp->limconst, &rlim);
	limit = hard ? rlim.rlim_max : rlim.rlim_cur;
	if (limit == RLIM_INFINITY)
		printf("unlimited");
	else if (lp->limconst == RLIMIT_CPU)
		psecs((long)limit);
	else
		printf("%ld %t", limit / lp->limdiv, lp->limscale);
	printf("\n");
}

dounlimit(v)
	register tchar **v;
{
	register struct limits *lp;
	int err = 0;
	tchar hard = 0;
#ifdef TRACE
	tprintf("TRACE- dounlimit()\n");
#endif

	v++;
	if (*v && eq(*v, S_h/*"-h"*/)) {
		hard = 1;
		v++;
	}
	if (*v == 0) {
		for (lp = limits; lp->limconst >= 0; lp++)
			if (setlim(lp, hard, (int)RLIM_INFINITY) < 0)
				err++;
		if (err)
			error(NULL);
		return;
	}
	while (*v) {
		lp = findlim(*v++);
		if (setlim(lp, hard, (int)RLIM_INFINITY) < 0)
			error(NULL);
	}
}

setlim(lp, hard, limit)
	register struct limits *lp;
	tchar hard;
{
	struct rlimit rlim;

#ifdef TRACE
	tprintf("TRACE- setlim()\n");
#endif
	(void) getrlimit(lp->limconst, &rlim);
	if (hard)
		rlim.rlim_max = limit;
  	else if (limit == RLIM_INFINITY && geteuid() != 0)
 		rlim.rlim_cur = rlim.rlim_max;
 	else
 		rlim.rlim_cur = limit;
	if (setrlimit(lp->limconst, &rlim) < 0) {
		printf("%t: %t: Can't %s%s limit\n", bname, lp->limname,
		    limit == RLIM_INFINITY ? "remove" : "set",
		    hard ? " hard" : "");
		return (-1);
	}
	return (0);
}

dosuspend()
{
	int ldisc, ctpgrp;
	void (*old)();

#ifdef TRACE
	tprintf("TRACE- dosuspend()\n");
#endif
	if (loginsh)
		error("Can't suspend a login shell (yet)");
	untty();
	old = signal(SIGTSTP, SIG_DFL);
	(void) kill(0, SIGTSTP);
	/* the shell stops here */
	(void) signal(SIGTSTP, old);
	if (tpgrp != -1) {
retry:
		(void) ioctl(FSHTTY, TIOCGPGRP,  (char *)&ctpgrp);
		if (ctpgrp != opgrp) {
			old = signal(SIGTTIN, SIG_DFL);
			(void) kill(0, SIGTTIN);
			(void) signal(SIGTTIN, old);
			goto retry;
		}
		(void) ioctl(FSHTTY, TIOCSPGRP,  (char *)&shpgrp);
		(void) setpgrp(0, shpgrp);
	}
	(void) ioctl(FSHTTY, TIOCGETD,  (char *)&oldisc);
	if (oldisc != NTTYDISC) {
		printf("Switching to new tty driver...\n");
		ldisc = NTTYDISC;
		(void) ioctl(FSHTTY, TIOCSETD,  (char *)&ldisc);
	}
}

doeval(v)
	tchar **v;
{
	tchar **oevalvec = evalvec;
	tchar *oevalp = evalp;
	jmp_buf osetexit;
	int reenter;
	tchar **gv = 0;

#ifdef TRACE
	tprintf("TRACE- doeval()\n");
#endif
	v++;
	if (*v == 0)
		return;
	gflag = 0, tglob(v);
	if (gflag) {
		gv = v = glob(v);
		gargv = 0;
		if (v == 0)
			error("No match");
		v = copyblk(v);
	} else
		trim(v);
	getexit(osetexit);
	reenter = 0;
	setexit();
	reenter++;
	if (reenter == 1) {
		evalvec = v;
		evalp = 0;
		process(0);
	}
	evalvec = oevalvec;
	evalp = oevalp;
	doneinp = 0;
	if (gv)
		blkfree(gv);
	resexit(osetexit);
	if (reenter >= 2)
		error(NULL);
}
