#ifndef lint
static	char sccsid[] = "@(#)m4macs.c 1.1 92/07/30 SMI"; /* from S5R2 1.3 */
#endif

#include	<stdio.h>
#include	<sys/param.h>
#include	<signal.h>
#include	<sys/types.h>
#if	u3b
#include	<sys/macro.h>
#else
#include	<sys/sysmacros.h>
#endif
#include	<sys/stat.h>
#include	"m4.h"

#define arg(n)	(c<(n)? nullstr: ap[n])

dochcom(ap,c)
char	**ap;
{
	register char	*l = arg(1);
	register char	*r = arg(2);

	if (strlen(l)>MAXSYM || strlen(r)>MAXSYM)
		error("comment marker longer than %d chars",MAXSYM);
	strcpy(lcom,l);
	strcpy(rcom,*r?r:"\n");
}

docq(ap,c)
register char 	**ap;
{
	register char	*l = arg(1);
	register char	*r = arg(2);

	if (strlen(l)>MAXSYM || strlen(r)>MAXSYM)
		error("quote marker longer than %d chars", MAXSYM);

	if (c<=1 && !*l) {
		l = "`";
		r = "'";
	} else if (c==1) {
		r = l;
	}

	strcpy(lquote,l);
	strcpy(rquote,r);
}

dodecr(ap,c)
char 	**ap;
{
	pbnum(ctol(arg(1))-1);
}

dodef(ap,c)
char	**ap;
{
	def(ap,c,NOPUSH);
}

def(ap,c,mode)
register char 	**ap;
{
	register char	*s;
	register char	sc;

	if (c<1)
		return;

	s = ap[1];
	sc = *s;
	if (isalpha(sc) || sc == '_') {
		do
			sc = *++s;
		while (isalnum(sc) || sc == '_');
	}
	if (sc || s==ap[1])
		error("bad macro name");

	if (strcmp(ap[1],ap[2])==0)
		error("macro defined as itself");
	install(ap[1],arg(2),mode);
}

dodefn(ap,c)
register char	**ap;
register c;
{
	register char *d;

	while (c > 0)
		if ((d = lookup(ap[c--])->def) != NULL) {
			putbak(*rquote);
			while (*d)
				putbak(*d++);
			putbak(*lquote);
		}
}

dodiv(ap,c)
register char **ap;
{
	register int f;

	f = atoi(arg(1));
	if (f>=10 || f<0) {
		cf = NULL;
		ofx = f;
		return;
	}
	tempfile[7] = 'a'+f;
	if (ofile[f] || (ofile[f]=xfopen(tempfile,"w"))) {
		ofx = f;
		cf = ofile[f];
	}
}

/* ARGSUSED */
dodivnum(ap,c)
{
	pbnum((long) ofx);
}

/* ARGSUSED */
dodnl(ap,c)
char 	*ap;
{
	register t;

	while ((t=getchr())!='\n' && t!=EOF)
		;
}

dodump(ap,c)
char 	**ap;
{
	register struct nlist *np;
	register	i;

	if (c > 0)
		while (c--) {
			if ((np = lookup(*++ap))->name != NULL)
				dump(np->name,np->def);
		}
	else
		for (i=0; i<hshsize; i++)
			for (np=hshtab[i]; np!=NULL; np=np->next)
				dump(np->name,np->def);
}

dump(name,defnn)
register char	*name,
		*defnn;
{
	register char	*s = defnn;

	fprintf(stderr,"%s:\t",name);

	while (*s++)
		;
	--s;

	while (s>defnn)
		if (*--s&~LOW7)
			fprintf(stderr,"<%s>",barray[*s&LOW7].bname);
		else
			fputc(*s,stderr);

	fputc('\n',stderr);
}

doerrp(ap,c)
char 	**ap;
{
	if (c > 0)
		fprintf(stderr,"%s",ap[1]);
}

long	evalval;	/* return value from yacc stuff */
char	*pe;	/* used by grammar */
doeval(ap,c)
char 	**ap;
{
	register	base = atoi(arg(2));
	register	pad = atoi(arg(3));

	evalval = 0;
	if (c > 0) {
		pe = ap[1];
		if (yyparse()!=0)
			error("invalid expression");
	}
	pbnbr(evalval, base>0?base:10, pad>0?pad:1);
}

doexit(ap,c)
char	**ap;
{
	delexit(atoi(arg(1)));
}

doif(ap,c)
register char **ap;
{
	if (c < 3)
		return;
	while (c >= 3) {
		if (strcmp(ap[1],ap[2])==0) {
			pbstr(ap[3]);
			return;
		}
		c -= 3;
		ap += 3;
	}
	if (c > 0)
		pbstr(ap[1]);
}

doifdef(ap,c)
char 	**ap;
{
	if (c < 2)
		return;

	while (c >= 2) {
		if (lookup(ap[1])->name != NULL) {
			pbstr(ap[2]);
			return;
		}
		c -= 2;
		ap += 2;
	}

	if (c > 0)
		pbstr(ap[1]);
}

doincl(ap,c)
char	**ap;
{
	incl(ap,c,1);
}

incl(ap,c,noisy)
register char 	**ap;
{
	if (c>0 && strlen(ap[1])>0) {
		if (ifx >= 9)
			error("input file nesting too deep (9)");
		if ((ifile[++ifx]=fopen(ap[1],"r"))==NULL){
			--ifx;
			if (noisy)
				error(badfile);
		} else {
			ipstk[ifx] = ipflr = ip;
			setfname(ap[1]);
		}
	}
}

doincr(ap,c)
char 	**ap;
{
	pbnum(ctol(arg(1))+1);
}

doindex(ap,c)
char	**ap;
{
	register char	*subj = arg(1);
	register char	*obj  = arg(2);
	register	i;

	for (i=0; *subj; ++i)
		if (leftmatch(subj++,obj)) {
			pbnum( (long) i );
			return;
		}

	pbnum( (long) -1 );
}

leftmatch(str,substr)
register char	*str;
register char	*substr;
{
	while (*substr)
		if (*str++ != *substr++)
			return (0);

	return (1);
}

dolen(ap,c)
char 	**ap;
{
	pbnum((long) strlen(arg(1)));
}

domake(ap,c)
char 	**ap;
{
	if (c > 0)
		pbstr(mktemp(ap[1]));
}

dopopdef(ap,c)
char	**ap;
{
	register	i;

	for (i=1; i<=c; ++i)
		undef(ap[i]);
}

dopushdef(ap,c)
char	**ap;
{
	def(ap,c,PUSH);
}

doshift(ap,c)
register char	**ap;
register c;
{
	if (c <= 1)
		return;

	for (;;) {
		pbstr(rquote);
		pbstr(ap[c--]);
		pbstr(lquote);

		if (c <= 1)
			break;

		pbstr(",");
	}
}

dosincl(ap,c)
char	**ap;
{
	incl(ap,c,0);
}

dosubstr(ap,c)
register char 	**ap;
{
	char	*str;
	int	inlen, outlen;
	register	offset, ix;

	inlen = strlen(str=arg(1));
	offset = atoi(arg(2));

	if (offset<0 || offset>=inlen)
		return;

	outlen = c>=3? atoi(ap[3]): inlen;
	ix = min(offset+outlen,inlen);

	while (ix > offset)
		putbak(str[--ix]);
}

dosyscmd(ap,c)
char 	**ap;
{
	sysrval = 0;
	if (c > 0) {
		fflush(stdout);
		sysrval = system(ap[1]);
	}
}

/* ARGSUSED */
dosysval(ap,c)
char	**ap;
{
	pbnum((long) (sysrval < 0 ? sysrval :
		(sysrval >> 8) & ((1 << 8) - 1)) |
		((sysrval & ((1 << 8) - 1)) << 8));
}

dotransl(ap,c)
char 	**ap;
{
	char	*sink, *fr, *sto;
	register char	*source, *to;

	if (c<1)
		return;

	sink = ap[1];
	fr = arg(2);
	sto = arg(3);

	for (source = ap[1]; *source; source++) {
		register char	*i;
		to = sto;
		for (i = fr; *i; ++i) {
			if (*source==*i)
				break;
			if (*to)
				++to;
		}
		if (*i) {
			if (*to)
				*sink++ = *to;
		} else
			*sink++ = *source;
	}
	*sink = EOS;
	pbstr(ap[1]);
}

dotroff(ap,c)
register char	**ap;
{
	register struct nlist	*np;

	trace = 0;

	while (c > 0)
		if ((np=lookup(ap[c--]))->name)
			np->tflag = 0;
}

dotron(ap,c)
register char	**ap;
{
	register struct nlist	*np;

	trace = !*arg(1);

	while (c > 0)
		if ((np=lookup(ap[c--]))->name)
			np->tflag = 1;
}

doundef(ap,c)
char	**ap;
{
	register	i;

	for (i=1; i<=c; ++i)
		while (undef(ap[i]))
			;
}

undef(nam)
char	*nam;
{
	register struct	nlist *np, *tnp;

	if ((np=lookup(nam))->name==NULL)
		return 0;
	tnp = hshtab[hshval];	/* lookup sets hshval */
	if (tnp==np)	/* it's in first place */
		hshtab[hshval] = tnp->next;
	else {
		while (tnp->next != np)
			tnp = tnp->next;

		tnp->next = np->next;
	}
	cfree(np->name);
	cfree(np->def);
	cfree((char *) np);
	return 1;
}

doundiv(ap,c)
register char 	**ap;
{
	register int i;

	if (c<=0)
		for (i=1; i<10; i++)
			undiv(i,OK);
	else
		while (--c >= 0)
			undiv(atoi(*++ap),OK);
}

dowrap(ap,c)
char	**ap;
{
	register char	*a = arg(1);

	if (Wrapstr)
		cfree(Wrapstr);

	Wrapstr = xcalloc(strlen(a)+1,sizeof(char));
	strcpy(Wrapstr,a);
}

struct bs	barray[] = {
	dochcom,	"changecom",
	docq,		"changequote",
	dodecr,		"decr",
	dodef,		"define",
	dodefn,		"defn",
	dodiv,		"divert",
	dodivnum,	"divnum",
	dodnl,		"dnl",
	dodump,		"dumpdef",
	doerrp,		"errprint",
	doeval,		"eval",
	doexit,		"m4exit",
	doif,		"ifelse",
	doifdef,	"ifdef",
	doincl,		"include",
	doincr,		"incr",
	doindex,	"index",
	dolen,		"len",
	domake,		"maketemp",
	dopopdef,	"popdef",
	dopushdef,	"pushdef",
	doshift,	"shift",
	dosincl,	"sinclude",
	dosubstr,	"substr",
	dosyscmd,	"syscmd",
	dosysval,	"sysval",
	dotransl,	"translit",
	dotroff,	"traceoff",
	dotron,		"traceon",
	doundef,	"undefine",
	doundiv,	"undivert",
	dowrap,		"m4wrap",
	0,		0
};
