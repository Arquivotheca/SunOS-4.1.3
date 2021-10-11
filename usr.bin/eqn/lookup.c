#ifndef lint
static	char sccsid[] = "@(#)lookup.c 1.1 92/07/30 SMI"; /* from UCB 4.1 83/02/11 */
#endif

# include "e.h"
#include "e.def"

#define	TBLSIZE	100

tbl	*keytbl[TBLSIZE];	/* key words */
tbl	*restbl[TBLSIZE];	/* reserved words */
tbl	*deftbl[TBLSIZE];	/* user-defined names */

struct {
	char	*key;
	int	keyval;
} keyword[]	={
	"sub", 	SUB, 
	"sup", 	SUP, 
	".EN", 	EOF, 
	"from", 	FROM, 
	"to", 	TO, 
	"sum", 	SUM, 
	"hat", 	HAT, 
	"vec", VEC, 
	"dyad", DYAD, 
	"dot", 	DOT, 
	"dotdot", 	DOTDOT, 
	"bar", 	BAR, 
	"tilde", 	TILDE, 
	"under", 	UNDER, 
	"prod", 	PROD, 
	"int", 	INT, 
	"integral", 	INT, 
	"union", 	UNION, 
	"inter", 	INTER, 
	"pile", 	PILE, 
	"lpile", 	LPILE, 
	"cpile", 	CPILE, 
	"rpile", 	RPILE, 
	"over", 	OVER, 
	"sqrt", 	SQRT, 
	"above", 	ABOVE, 
	"size", 	SIZE, 
	"font", 	FONT, 
	"fat", FAT, 
	"roman", 	ROMAN, 
	"italic", 	ITALIC, 
	"bold", 	BOLD, 
	"left", 	LEFT, 
	"right", 	RIGHT, 
	"delim", 	DELIM, 
	"define", 	DEFINE, 

#ifdef	NEQN	/* make ndefine synonym for define, tdefine a no-op */

	"tdefine",	TDEFINE,
	"ndefine",	DEFINE,

#else		/* tdefine = define, ndefine = no-op */

	"tdefine", 	DEFINE, 
	"ndefine", 	NDEFINE, 

#endif

	"gsize", 	GSIZE, 
	".gsize", 	GSIZE, 
	"gfont", 	GFONT, 
	"include", 	INCLUDE, 
	"up", 	UP, 
	"down", 	DOWN, 
	"fwd", 	FWD, 
	"back", 	BACK, 
	"mark", 	MARK, 
	"lineup", 	LINEUP, 
	"matrix", 	MATRIX, 
	"col", 	COL, 
	"lcol", 	LCOL, 
	"ccol", 	CCOL, 
	"rcol", 	RCOL, 
	0, 	0
};

struct {
	char	*res;
	char	*resval;
} resword[]	={
	">=",	"\\(>=",
	"<=",	"\\(<=",
	"==",	"\\(==",
	"!=",	"\\(!=",
	"+-",	"\\(+-",
	"->",	"\\(->",
	"<-",	"\\(<-",
	"inf",	"\\(if",
	"infinity",	"\\(if",
	"partial",	"\\(pd",
	"half",	"\\f1\\(12\\fP",
	"prime",	"\\f1\\(fm\\fP",
	"dollar",	"\\f1$\\fP",
	"nothing",	"",
	"times",	"\\(mu",
	"del",	"\\(gr",
	"grad",	"\\(gr",
#ifdef	NEQN
	"<<",	"<<",
	">>",	">>",
	"approx",	"~\b\\d~\\u",
	"cdot",	"\\v'-.5'.\\v'.5'",
	"...",	"...",
	",...,",	",...,",
#else
	"<<",	"<\\h'-.3m'<",
	">>",	">\\h'-.3m'>",
	"approx",	"\\v'-.2m'\\z\\(ap\\v'.25m'\\(ap\\v'-.05m'",
	"cdot",	"\\v'-.3m'.\\v'.3m'",
	"...",	"\\v'-.3m'\\ .\\ .\\ .\\ \\v'.3m'",
	",...,",	",\\ .\\ .\\ .\\ ,\\|",
#endif

	"alpha",	"\\(*a",
	"beta",	"\\(*b",
	"gamma",	"\\(*g",
	"GAMMA",	"\\(*G",
	"delta",	"\\(*d",
	"DELTA",	"\\(*D",
	"epsilon",	"\\(*e",
	"EPSILON",	"\\f1E\\fP",
	"omega",	"\\(*w",
	"OMEGA",	"\\(*W",
	"lambda",	"\\(*l",
	"LAMBDA",	"\\(*L",
	"mu",	"\\(*m",
	"nu",	"\\(*n",
	"theta",	"\\(*h",
	"THETA",	"\\(*H",
	"phi",	"\\(*f",
	"PHI",	"\\(*F",
	"pi",	"\\(*p",
	"PI",	"\\(*P",
	"sigma",	"\\(*s",
	"SIGMA",	"\\(*S",
	"xi",	"\\(*c",
	"XI",	"\\(*C",
	"zeta",	"\\(*z",
	"iota",	"\\(*i",
	"eta",	"\\(*y",
	"kappa",	"\\(*k",
	"rho",	"\\(*r",
	"tau",	"\\(*t",
	"omicron",	"\\(*o",
	"upsilon",	"\\(*u",
	"UPSILON",	"\\(*U",
	"psi",	"\\(*q",
	"PSI",	"\\(*Q",
	"chi",	"\\(*x",
	"and",	"\\f1and\\fP",
	"for",	"\\f1for\\fP",
	"if",	"\\f1if\\fP",
	"Re",	"\\f1Re\\fP",
	"Im",	"\\f1Im\\fP",
	"sin",	"\\f1sin\\fP",
	"cos",	"\\f1cos\\fP",
	"tan",	"\\f1tan\\fP",
	"sec",  "\\f1sec\\fP",
	"csc",  "\\f1csc\\fP",
	"arc",	"\\f1arc\\fP",
	"asin", "\\f1asin\\fP",
	"acos", "\\f1acos\\fP",
	"atan", "\\f1atan\\fP",
	"asec", "\\f1asec\\fP",
	"acsc", "\\f1acsc\\fP",
	"sinh",	"\\f1sinh\\fP",
	"coth",	"\\f1coth\\fP",
	"tanh",	"\\f1tanh\\fP",
	"cosh",	"\\f1cosh\\fP",
	"lim",	"\\f1lim\\fP",
	"log",	"\\f1log\\fP",
	"max",	"\\f1max\\fP",
	"min",	"\\f1min\\fP",
	"ln",	"\\f1ln\\fP",
	"exp",	"\\f1exp\\fP",
	"det",	"\\f1det\\fP",
	0,	0
};

tbl *lookup(tblp, name, defn)	/* find name in tbl. if defn non-null, install */
tbl **tblp;
char *name, *defn;
{
	register tbl *p;
	register int h;
	register char *s = name;
	char *malloc();

	for (h = 0; *s != '\0'; )
		h += *s++;
	h %= TBLSIZE;

	for (p = tblp[h]; p != NULL; p = p->next)
		if (strcmp(name, p->name) == 0) {	/* found it */
			if (defn != NULL)
				p->defn = defn;
			return(p);
		}
	/* didn't find it */
	if (defn == NULL)
		return(NULL);
	p = (tbl *) malloc(sizeof (tbl));
	if (p == NULL)
		error(FATAL, "out of space in lookup");
	p->name = name;
	p->defn = defn;
	p->next = tblp[h];
	tblp[h] = p;
	return(p);
}

init_tbl()	/* initialize all tables */
{
	int i;

	for (i = 0; keyword[i].key != NULL; i++)
		lookup(keytbl, keyword[i].key, keyword[i].keyval);
	for (i = 0; resword[i].res != NULL; i++)
		lookup(restbl, resword[i].res, resword[i].resval);
}
