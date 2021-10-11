#ifndef lint
static	char sccsid[] = "@(#)run.c 1.1 92/07/30 SMI"; /* from S5R2 */
#endif

#define DEBUG
#define tempfree(a)	{if(istemp(a)) {xfree(a->sval) ;a->tval = 0;}}

#include	"awk.def"
#include	"math.h"
#include	"awk.h"
#include	"stdio.h"

#define RECSIZE BUFSIZ

#define FILENUM	10
struct
{
	FILE *fp;
	int type;
	char *fname;
} files[FILENUM];
FILE *popen();

extern CELL *execute(), *nodetoobj(), *fieldel(), *dopa2(), *gettemp();
extern char *malloc(), *realloc();

#define PA2NUM	29
int	pairstack[PA2NUM], paircnt;
NODE	*winner = NULL;
#define	MAXTMP	20
CELL	tmps[MAXTMP];

static CELL	truecell	={ OBOOL, BTRUE, 0, 0, 0.0, NUM, 0 };
CELL	*true	= &truecell;
static CELL	falsecell	={ OBOOL, BFALSE, 0, 0, 0.0, NUM, 0 };
CELL	*false	= &falsecell;
static CELL	breakcell	={ OJUMP, JBREAK, 0, 0, 0.0, NUM, 0 };
CELL	*jbreak	= &breakcell;
static CELL	contcell	={ OJUMP, JCONT, 0, 0, 0.0, NUM, 0 };
CELL	*jcont	= &contcell;
static CELL	nextcell	={ OJUMP, JNEXT, 0, 0, 0.0, NUM, 0 };
CELL	*jnext	= &nextcell;
static CELL	exitcell	={ OJUMP, JEXIT, 0, 0, 0.0, NUM, 0 };
CELL	*jexit	= &exitcell;
static CELL	tempcell	={ OCELL, CTEMP, 0, 0, 0.0, NUM, 0 };

run(a) NODE *a;
{
	register int i;

	execute(a);

	/* Wait for children to complete if output to a pipe. */
	for (i=0; i<FILENUM; i++)
		if (files[i].fp && files[i].type == '|')
			pclose(files[i].fp);
}

CELL *execute(u) NODE *u;
{
	register CELL *(*proc)();
	register CELL *x;
	register NODE *a;
	extern char *printname[];

	if (u == NULL)
		return(true);
	for (a = u; ; a = a->nnext) {
		if (cantexec(a))
			return(nodetoobj(a));
		if (notlegal(a->nobj))
			error(FATAL, "illegal statement %o", a);
		proc = proctab[a->nobj-FIRSTTOKEN];
		x = (*proc)(a->narg, a->nobj);
		if (isfld(x))
			fldbld();
		if (isexpr(a))
			return(x);
		/* a statement, goto next statement */
		if (isjump(x))
			return(x);
		if (a->nnext == (NODE *)NULL)
			return(x);
		tempfree(x);
	}
}

CELL *program(a, n) register NODE **a;
{
	register CELL *x;

	if (a[0] != NULL) {
		x = execute(a[0]);
		if (isexit(x))
			return(true);
		if (isjump(x))
			error(FATAL, "unexpected break, continue or next");
		tempfree(x);
	}
	while (getrec()) {
		x = execute(a[1]);
		if (isexit(x)) {
			tempfree(x);
			break;
		}
		tempfree(x);
	}
	if (a[2] != NULL) {
		x = execute(a[2]);
		if (isbreak(x) || isnext(x) || iscont(x))
			error(FATAL, "unexpected break, continue or next");
		tempfree(x);
	}
	return(true);
}

CELL *getline()
{
	register CELL *x;

	x = gettemp();
	setfval(x, (awkfloat) getrec());
	return(x);
}

CELL *array(a,n) register NODE **a;
{
	register CELL *x, *y;
	extern CELL *arrayel();

	x = execute(a[1]);
	y = arrayel(a[0], x);
	tempfree(x);
	return(y);
}

CELL *arrayel(a, b) NODE *a; CELL *b;
{
	register char *s;
	register CELL *x;
	register int i;
	register CELL *y;

	s = getsval(b);
	x = (CELL *) a;
	if (!(x->tval&ARR)) {
		xfree(x->sval);
		x->tval &= ~STR;
		x->tval |= ARR;
		x->sval = (char *) makesymtab();
	}
	y = setsymtab(s, tostring(""), 0.0, STR|NUM, x->sval);
	y->ctype = OCELL;
	y->csub = CVAR;
	return(y);
}

CELL *matchop(a,n) NODE **a;
{
	register CELL *x;
	register char *s;
	register int i;

	x = execute(a[0]);
	s = getsval(x);
	tempfree(x);
	i = match(a[1], s);
	if (n == MATCH && i == 1 || n == NOTMATCH && i == 0)
		return(true);
	else
		return(false);
}

CELL *boolop(a,n) NODE **a;
{
	register CELL *x, *y;
	register int i;

	x = execute(a[0]);
	i = istrue(x);
	tempfree(x);
	switch (n) {
	case BOR:
		if (i) return(true);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case AND:
		if ( !i ) return(false);
		y = execute(a[1]);
		i = istrue(y);
		tempfree(y);
		if (i) return(true);
		else return(false);
	case NOT:
		if (i) return(false);
		else return(true);
	default:
		error(FATAL, "unknown boolean operator %d", n);
	}
	/*NOTREACHED*/
}

CELL *relop(a,n) NODE **a;
{
	register int i;
	register CELL *x, *y;
	awkfloat j;

	x = execute(a[0]);
	y = execute(a[1]);
	if (x->tval&NUM && y->tval&NUM) {
		j = x->fval - y->fval;
		i = j<0? -1: (j>0? 1: 0);
	} else {
		i = strcmp(getsval(x), getsval(y));
	}
	tempfree(x);
	tempfree(y);
	switch (n) {
	case LT:	if (i<0) return(true);
			else return(false);
	case LE:	if (i<=0) return(true);
			else return(false);
	case NE:	if (i!=0) return(true);
			else return(false);
	case EQ:	if (i == 0) return(true);
			else return(false);
	case GE:	if (i>=0) return(true);
			else return(false);
	case GT:	if (i>0) return(true);
			else return(false);
	default:
		error(FATAL, "unknown relational operator %d", n);
	}
	/*NOTREACHED*/
}


CELL *gettemp()
{
	register int i;
	register CELL *x;

	for (i=0; i<MAXTMP; i++)
		if (tmps[i].tval == 0)
			break;
	if (i == MAXTMP)
		error(FATAL, "out of temporaries in gettemp");
	tmps[i] = tempcell;
	x = &tmps[i];
	return(x);
}

CELL *indirect(a,n) NODE **a;
{
	register CELL *x;
	register int m;
	CELL *fieldadr();

	x = execute(a[0]);
	m = getfval(x);
	tempfree(x);
	x = fieldadr(m);
	x->ctype = OCELL;
	x->csub = CFLD;
	return(x);
}

CELL *substr(a, nnn) NODE **a;
{
	register int k, m, n;
	register char *s, temp;
	register CELL *x;

	x = execute(a[0]);
	s = getsval(x);
	k = strlen(s) + 1;
	tempfree(x);
	if (k <= 1) {
		x = gettemp();
		setsval(x, "");
		return(x);
	}
	x = execute(a[1]);
	m = getfval(x);
	if (m <= 0)
		m = 1;
	else if (m > k)
		m = k;
	tempfree(x);
	if (a[2] != 0) {
		x = execute(a[2]);
		n = getfval(x);
		tempfree(x);
	}
	else
		n = k - 1;
	if (n < 0)
		n = 0;
	else if (n > k - m)
		n = k - m;
	dprintf("substr: m=%d, n=%d, s=%s\n", m, n, s);
	x = gettemp();
	temp = s[n+m-1];	/* with thanks to John Linderman */
	s[n+m-1] = '\0';
	setsval(x, s + m - 1);
	s[n+m-1] = temp;
	return(x);
}

CELL *sindex(a, nnn) NODE **a;
{
	register CELL *x;
	register char *s1, *s2, *p1, *p2, *q;

	x = execute(a[0]);
	s1 = getsval(x);
	tempfree(x);
	x = execute(a[1]);
	s2 = getsval(x);
	tempfree(x);

	x = gettemp();
	for (p1 = s1; *p1 != '\0'; p1++) {
		for (q=p1, p2=s2; *p2 != '\0' && *q == *p2; q++, p2++)
			;
		if (*p2 == '\0') {
			setfval(x, (awkfloat) (p1 - s1 + 1));	/* origin 1 */
			return(x);
		}
	}
	setfval(x, 0.0);
	return(x);
}

char *format(s,a) char *s; NODE *a;
{
	register char *buf, *p, fmt[200], *t, *os, *ep, tbuf[300], *str;
	register CELL *x;
	int flag = 0;
	awkfloat xf;

	os = s;
	p = buf = (char *)malloc(RECSIZE);
	if (p == NULL)
		error(FATAL, "out of space in format");
	ep = p + RECSIZE;
	while (*s) {
		if (*s != '%') {
			*p++ = *s++;
			continue;
		}
		if (*(s+1) == '%') {
			*p++ = '%';
			s += 2;
			continue;
		}
		for (t=fmt; (*t++ = *s) != '\0'; s++)
			if (*s >= 'a' && *s <= 'z' && *s != 'l')
				break;
		*t = '\0';
		if (t >= fmt + sizeof(fmt))
			error(FATAL, "format item %.20s... too long", os);
		switch (*s) {
		case 'f': case 'e': case 'g':
			flag = 1;
			break;
		case 'd':
			flag = 2;
			if(*(s-1) == 'l') break;
			*(t-1) = 'l';
			*t = 'd';
			*++t = '\0';
			break;
		case 'o': case 'x':
			flag = *(s-1) == 'l' ? 2 : 3;
			break;
		case 'c':
			flag = 3;
			break;
		case 's':
			flag = 4;
			break;
		default:
			flag = 0;
			break;
		}
		if (flag == 0) {
			sprintf(p, "%s", fmt);
			p += strlen(p);
			continue;
		}
		if (a == NULL)
			error(FATAL, "not enough arguments in printf(%s)", os);
		x = execute(a);
		a = a->nnext;
		/* 
		 * Get the string to check length; %s is the usual problem; 
		 * other conversions can cause overrun if they occur when 
		 * the buffer is almost filled.
		 */
		if (flag == 4)	{ /* watch out for converting to numbers! */
			str = getsval(x);
		}
		else {
			xf = getfval(x);
			if (flag == 1) sprintf(tbuf, fmt, xf);
			else if (flag == 2) sprintf(tbuf, fmt, (long)xf);
			else if (flag == 3) sprintf(tbuf, fmt, (int)xf);
			if (strlen(tbuf) > sizeof(tbuf))
				error(FATAL, "formatted item %s... too long",
						tbuf);
			str = tbuf;
		}
		/* 
		 * If string overruns the buffer, reallocate;
		 * consider length of format string
		 */
		if (ep < p + strlen(str) + strlen(s)) {
			int newlen, oldlen;
	
			oldlen = p - buf;
			/* Add RECSIZE for additional space */
			newlen = oldlen + strlen(str) + RECSIZE;
			buf = realloc(buf, (unsigned) newlen);
			if (buf == NULL)
				error(FATAL, "out of format space");
			p = buf + oldlen;
			ep = buf + newlen;
		}
		/* Transfer string to buffer */
		if (flag == 4)
			sprintf(p, fmt, str);
		else
			strcpy(p, str);

		tempfree(x);
		p += strlen(p);
		if (p > ep)
			error(FATAL, "formatted string too long");
		s++;
	}
	*p = '\0';
	return(buf);
}

CELL *asprintf(a,n) NODE **a;
{
	register CELL *x;
	register NODE *y;
	register char *s;

	y = a[0]->nnext;
	x = execute(a[0]);
	s = format(getsval(x), y);
	tempfree(x);
	x = gettemp();
	x->sval = s;
	x->tval = STR;
	return(x);
}

CELL *arith(a,n) NODE **a;
{
	awkfloat i, j;
	register CELL *x, *y, *z;

	x = execute(a[0]);
	i = getfval(x);
	tempfree(x);
	if (n != UMINUS) {
		y = execute(a[1]);
		j = getfval(y);
		tempfree(y);
	}
	z = gettemp();
	switch (n) {
	case ADD:
		i += j;
		break;
	case MINUS:
		i -= j;
		break;
	case MULT:
		i *= j;
		break;
	case DIVIDE:
		if (j == 0)
			error(FATAL, "division by zero");
		i /= j;
		break;
	case MOD:
		if (j == 0)
			error(FATAL, "division by zero");
		i = i - j*(long)(i/j);
		break;
	case UMINUS:
		i = -i;
		break;
	default:
		error(FATAL, "illegal arithmetic operator %d", n);
	}
	setfval(z, i);
	return(z);
}

CELL *incrdecr(a, n) NODE **a;
{
	register CELL *x, *z;
	register int k;
	awkfloat xf;

	x = execute(a[0]);
	xf = getfval(x);
	k = (n == PREINCR || n == POSTINCR) ? 1 : -1;
	if (n == PREINCR || n == PREDECR) {
		setfval(x, xf + k);
		return(x);
	}
	z = gettemp();
	setfval(z, xf);
	setfval(x, xf + k);
	tempfree(x);
	return(z);
}


CELL *assign(a,n) NODE **a;
{
	register CELL *x, *y;
	awkfloat xf, yf;

	x = execute(a[0]);
	y = execute(a[1]);
	if (n == ASSIGN) {	/* ordinary assignment */
		if ((y->tval & (STR|NUM)) == (STR|NUM)) {
			setsval(x, y->sval);
			x->fval = y->fval;
			x->tval |= NUM;
		}
		else if (y->tval & STR)
			setsval(x, y->sval);
		else if (y->tval & NUM)
			setfval(x, y->fval);
		tempfree(y);
		return(x);
	}
	xf = getfval(x);
	yf = getfval(y);
	switch (n) {
	case ADDEQ:
		xf += yf;
		break;
	case SUBEQ:
		xf -= yf;
		break;
	case MULTEQ:
		xf *= yf;
		break;
	case DIVEQ:
		if (yf == 0)
			error(FATAL, "division by zero");
		xf /= yf;
		break;
	case MODEQ:
		if (yf == 0)
			error(FATAL, "division by zero");
		xf = xf - yf*(long)(xf/yf);
		break;
	default:
		error(FATAL, "illegal assignment operator %d", n);
		break;
	}
	tempfree(y);
	setfval(x, xf);
	return(x);
}

CELL *cat(a,q) NODE **a;
{
	register CELL *x, *y, *z;
	register int n1, n2;
	register char *s;

	x = execute(a[0]);
	y = execute(a[1]);
	getsval(x);
	getsval(y);
	n1 = strlen(x->sval);
	n2 = strlen(y->sval);
	if ((s = (char *) malloc(n1 + n2 + 1)) == NULL)
		error(FATAL, "out of space in cat");
	strcpy(s, x->sval);
	strcpy(s+n1, y->sval);
	tempfree(y);
	z = gettemp();
	z->sval = s;
	z->tval = STR;
	tempfree(x);
	return(z);
}

CELL *pastat(a,n) NODE **a;
{
	register CELL *x;

	if (a[0] == 0)
		x = true;
	else
		x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	}
	return(x);
}

CELL *dopa2(a,n) NODE **a;
{
	register CELL *x;
	register int pair;

	pair = (int) a[3];
	if (pairstack[pair] == 0) {
		x = execute(a[0]);
		if (istrue(x))
			pairstack[pair] = 1;
		tempfree(x);
	}
	if (pairstack[pair] == 1) {
		x = execute(a[1]);
		if (istrue(x))
			pairstack[pair] = 0;
		tempfree(x);
		x = execute(a[2]);
		return(x);
	}
	return(false);
}

CELL *aprintf(a,n) NODE **a;
{
	register CELL *x;

	x = asprintf(a,n);
	if (a[1] == NULL) {
		printf("%s", x->sval);
		tempfree(x);
		return(true);
	}
	redirprint(x->sval, (int)a[1], a[2]);
	return(x);
}
static int ctest[0200];

CELL *split(a,nnn) NODE **a;
{
	register CELL *x;
	register CELL *ap;
	register char *s, *p;
	char *t, temp, num[5];
	register int sep;
	int n, flag;

	x = execute(a[0]);
	s = getsval(x);
	tempfree(x);
	if (a[2] == 0)
		sep = **FS;
	else {
		x = execute(a[2]);
		sep = getsval(x)[0];
		tempfree(x);
	}
	ap = (CELL *) a[1];
	freesymtab(ap);
	dprintf("split: s=|%s|, a=%s, sep=|%c|\n", s, ap->nval, sep);
	ap->tval &= ~STR;
	ap->tval |= ARR;
	ap->sval = (char *) makesymtab();

	n = 0;
	if (sep == ' ')
		for (n = 0; ; ) {
			while (*s == ' ' || *s == '\t' || *s == '\n')
				s++;
			if (*s == 0)
				break;
			n++;
			t = s;
			ctest[' '] = ctest['\t'] = ctest['\n'] = ctest['\0'] =1;
			do
				s++;
			/*while (*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0');*/
			while(!ctest[*s]);
			ctest[' '] = ctest['\t'] = ctest['\n'] = ctest['\0'] =0;
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, tostring(t), atof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, tostring(t), 0.0, STR, ap->sval);
			*s = temp;
			if (*s != 0)
				s++;
		}
	else if (*s != 0)
		for (;;) {
			n++;
			t = s;
			ctest[sep] = ctest['\n'] = ctest['\0'] = 1;
			while(!ctest[*s])
			/*while (*s != sep && *s != '\n' && *s != '\0')*/
				s++;
			ctest[sep] = ctest['\n'] = ctest['\0'] = 0;
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, tostring(t), atof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, tostring(t), 0.0, STR, ap->sval);
			*s = temp;
			if (*s++ == 0)
				break;
		}
	x = gettemp();
	x->tval = NUM;
	x->fval = n;
	return(x);
}

CELL *ifstat(a,n) NODE **a;
{
	register CELL *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	}
	else if (a[2] != 0) {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

CELL *whilestat(a,n) NODE **a;
{
	register CELL *x;

	for (;;) {
		x = execute(a[0]);
		if (!istrue(x)) return(x);
		tempfree(x);
		x = execute(a[1]);
		if (isbreak(x)) {
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x))
			return(x);
		tempfree(x);
	}
	/*NOTREACHED*/
}

CELL *forstat(a,n) NODE **a;
{
	register CELL *x;
	register CELL *z;

	z = execute(a[0]);
	tempfree(z);
	for (;;) {
		if (a[1]!=0) {
			x = execute(a[1]);
			if (!istrue(x)) return(x);
			else tempfree(x);
		}
		x = execute(a[3]);
		if (isbreak(x)) {	/* turn off break */
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x))
			return(x);
		tempfree(x);
		z = execute(a[2]);
		tempfree(z);
	}
}

CELL *instat(a, n) NODE **a;
{
	register CELL *vp, *arrayp, *cp, **tp;
	register CELL *x;
	int i;

	vp = (CELL *) a[0];
	arrayp = (CELL *) a[1];
	if (!(arrayp->tval & ARR))
		error(FATAL, "%s is not an array", arrayp->nval);
	tp = (CELL **) arrayp->sval;
	for (i = 0; i < MAXSYM; i++) {	/* this routine knows too much */
		for (cp = tp[i]; cp != NULL; cp = cp->nextval) {
			setsval(vp, cp->nval);
			x = execute(a[2]);
			if (isbreak(x)) {
				x = true;
				return(x);
			}
			if (isnext(x) || isexit(x))
				return(x);
			tempfree(x);
		}
	}
	return (true);
}

CELL *jump(a, n) NODE **a;
{
	register CELL *y;

	switch (n) {
	case EXIT:
		if (a[0] != 0) {
			y = execute(a[0]);
			errorflag = getfval(y);
		}
		return(jexit);
	case NEXT:
		return(jnext);
	case BREAK:
		return(jbreak);
	case CONTINUE:
		return(jcont);
	default:
		error(FATAL, "illegal jump type %d", n);
	}
	/*NOTREACHED*/
}

CELL *fncn(a,n) NODE **a;
{
	register CELL *x;
	awkfloat u;
	register int t;

	t = (int) a[0];
	x = execute(a[1]);
	if (t == FLENGTH)
		u = (awkfloat) strlen(getsval(x));
	else if (t == FLOG)
		u = log(getfval(x));
	else if (t == FINT)
		u = (awkfloat) (long) getfval(x);
	else if (t == FEXP)
		u = exp(getfval(x));
	else if (t == FSQRT)
		u = sqrt(getfval(x));
	else
		error(FATAL, "illegal function type %d", t);
	tempfree(x);
	x = gettemp();
	setfval(x, u);
	return(x);
}

CELL *print(a,n) NODE **a;
{
	register NODE *x;
	register CELL *y;
	char s[RECSIZE], *str, *bp, *ep;

	s[0] = '\0';
	bp = s;
	ep = s + RECSIZE;
	for (x=a[0]; x!=NULL; x=x->nnext) {
		y = execute(x);
		str = getsval(y);
		/* allocate larger buffer if needed */
		if (ep < bp + strlen(bp) + strlen(str)) {
			int newlen;
			char *oldbp;

			newlen = strlen(bp) + strlen(str) + 1 + 1;
			oldbp = bp;
			bp = malloc((unsigned) newlen);
			if (bp == NULL)
				error(FATAL, "out of space in print");
			ep = bp + newlen;
			strcpy(bp, oldbp);
			if (oldbp != s)
				free(oldbp);
		}
		strcat(bp, str);
		tempfree(y);
		if (x->nnext == NULL)
			strcat(bp, *ORS);
		else
			strcat(bp, *OFS);
	}
	if (a[1] == 0) {
		printf("%s", bp);
		if (bp != s)
			free(bp);
		return(true);
	}
	redirprint(bp, (int)a[1], a[2]);
	if (bp != s)
		free(bp);
	return(false);
}

CELL *nullproc() {}

CELL *nodetoobj(a) NODE *a;
{
	register CELL *x;

	x= (CELL *) a->nobj;
	x->ctype = OCELL;
	x->csub = a->subtype;
	if (isfld(x))
		fldbld();
	return(x);
}

redirprint(s, a, b) char *s; NODE *b;
{
	register int i;
	register CELL *x;

	x = execute(b);
	getsval(x);
	for (i=0; i<FILENUM; i++)
		if (files[i].fp != 0 && strcmp(x->sval, files[i].fname) == 0)
			goto doit;
	for (i=0; i<FILENUM; i++)
		if (files[i].fp == 0)
			break;
	if (i >= FILENUM)
		error(FATAL, "too many output files %d", i);
	if (a == '|')	/* a pipe! */
		files[i].fp = popen(x->sval, "w");
	else if (a == APPEND)
		files[i].fp = fopen(x->sval, "a");
	else if (a == GT)
		files[i].fp = fopen(x->sval, "w");
	else 
		error(FATAL, "illegal redirection near line %d", lineno);
	if (files[i].fp == NULL)
		error(FATAL, "can't open file %s", x->sval);
	files[i].fname = tostring(x->sval);
	files[i].type = a;
doit:
	fprintf(files[i].fp, "%s", s);
#ifndef gcos
	fflush(files[i].fp);	/* in case someone is waiting for the output */
#endif
	tempfree(x);
}
