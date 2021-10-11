#include <stdio.h>
#include "var.h"

#define NENV	300
extern char	**environ;

static char	**saveenv;
static VARBLOCK	firstvar;
static char            Nullstr[] = "";

extern char	*copstr(),
		*malloc(),
		*copys();

/*
 * This routine is called just before and exec. 
 */

void
setenv()
{
	register char	**ea;
	register int    nenv = 0;
	register char	*p;
	char	**es;
	VARBLOCK        vp;
	int             length;

	if (firstvar == 0)
		return;

	es = ea = (char	**) malloc((unsigned)NENV * sizeof(*ea));
	if (es == (char **)0)
		fatal("Cannot alloc mem for envp.");

	for (vp = firstvar; vp != 0; vp = vp->nextvar)
		if (vp->envflg)	/* is this an environment variable? */
		{
			if (++nenv >= NENV)
				fatal("Too many env parameters.");
			length = strlen(vp->varname) + strlen(vp->varval) + 2;
			if ((*ea = malloc((unsigned)length)) == (char *)0)
				fatal("Cannot alloc mem for env.");
			p = copstr(*ea++, vp->varname);
			p = copstr(p, "=");
			p = copstr(p, vp->varval);
		}
	*ea = (char *)0;	/* terminate list of string pointers */

	if (nenv > 0)
	{
		saveenv = environ;
		environ = es;
	}
#ifdef DEBUG
	if (IS_ON(DBUG))
		(void) printf("nenv = %d\n", nenv);
#endif DEBUG
}

/* Through away the environment created by setenv() and restore the
 * original environment.
 */
void
rstenv()
{
	register char	**ea;

	if (saveenv == 0)
		return;
	for (ea = environ; *ea; ea++)
		free(*ea);
	free((char *)environ);
	environ = saveenv;
	saveenv = 0;
}


/*
 * Called in main If a string like "CC=" occurs then CC is not put in
 * environment. This is because there is no good way to remove a variable
 * from the environment within the shell. 
 */

void
readenv()
{
	register char	**ea;
	register char	*p;

	/* each "name=value" in the environment, where name != "SHELL" &&
	 * value != NULL is processed as if it were read from the makefile.
	 */
	for(ea = environ; *ea; ++ea)
	{
		for (p = *ea; *p && *p != '='; p++)	/* skip to '=' */
			continue;
		if (*p == '=' && *(p+1) && (strncmp(*ea, "SHELL=", 6) != 0))
			(void)eqsign(*ea);
	}
}

void
fcallyacc(file)
	FILE	*file;
{
	extern FILE	*yyin;
	extern int	yylineno;
	int	savelineno;
	FILE	*savefile;

	savefile = yyin;
	savelineno = yylineno;
	yyin = file;

	yyparse();

	yyin = savefile;
	yylineno = savelineno;
}

void
callyacc(str)
	register unsigned char	*str;
{
	FILE	scratchfile;
	unsigned char	*tempstr;

	scratchfile._flag = _IOSTRG | _IOREAD;
	scratchfile._cnt = scratchfile._bufsiz = strlen((char *)str) + 1;

	tempstr = (unsigned char *)malloc(scratchfile._bufsiz+1);
	strcpy(tempstr, str);

	scratchfile._ptr = tempstr;	/* buffer supplied by caller */
	scratchfile._base = tempstr;
	tempstr += scratchfile._bufsiz;	/* point to '\0' */
	*tempstr-- = '\0';
	*tempstr = '\n';		/* parser needs newlines */

	fcallyacc(&scratchfile);
	free(scratchfile._base);
}

int
eqsign(a)
	register char	*a;
{
	register char	*p;

	for (p = ":=$\n\t"; *p; p++)
		if (any(a, *p))
		{
			callyacc((unsigned char *)a);
			return (YES);
		}
	return (NO);
}


VARBLOCK
varptr(v)
	register char	*v;
{
	register VARBLOCK vp;

	if ((vp = srchvar(v)) != NO)
		return (vp);

	vp = ALLOC(varblock);
	vp->nextvar = firstvar;
	firstvar = vp;
	vp->varname = copys(v);
	vp->varval = copys("");
	return (vp);
}

VARBLOCK
srchvar(vname)
	register char	*vname;
{
	register VARBLOCK vp;

	for (vp = firstvar; vp != 0; vp = vp->nextvar)
		if (equal(vname, vp->varname))
			return (vp);
	return (NO);
}

void
setvar(v, s)
	register char	*v,
	                *s;
{
	register VARBLOCK p;

	p = varptr(v);		/* find or allocate v */
	s = (s ? s : Nullstr);	/* NULL becomes "" */
	if (p->noreset == NO)	/* don't assign if var is read only */
	{
		if (IS_ON(EXPORT))		/* do children see this? */
			p->envflg = YES;	/*  yes */
		if( p->varval )
			free(p->varval);
		p->varval = copys(s);			/* set value */
		if (IS_ON(INARGS) || IS_ON(ENVOVER))	/* make read only? */
			p->noreset = YES;		/*  yes, read only */
		else
			p->noreset = NO;		/* else, read/write */
#ifdef DEBUG
		if (IS_ON(DBUG))
			(void) printf("setvar: %s = %s noreset = %d envflg = %d Mflags = 0%o\n",
			    v, p->varval, p->noreset, p->envflg, Mflags);

		if (p->used && !amatch(v, "[@*<?!%]"))
			if (IS_ON(DBUG))
				(void) fprintf(stderr, "Warning: %s changed after being used\n", v);
#endif DEBUG
	}
}
