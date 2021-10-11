/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)run.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.5 */
#endif

#define tempfree(x)	if (istemp(x)) tfree(x); else

#define	execute(p)	(p == NULL ? true : (isvalue(p) ? (Cell *)((p)->narg[0]) : r_execute(p)))
/* #define	execute(p) r_execute(p) */

#define DEBUG
#include	"awk.h"
#include	<math.h>
#include	"y.tab.h"
#include	<stdio.h>
#include	<ctype.h>
#include	<setjmp.h>

extern double modf();

jmp_buf env;

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

extern	Awkfloat r_getfval();
extern	uchar	*r_getsval();
extern	Cell	*r_execute(), *fieldel(), *dopa2(), *gettemp(), *copycell();
extern	FILE	*openfile(), *redirect();
extern	fa	*makedfa();
extern	double	errcheck();

#define PA2NUM	29
int	pairstack[PA2NUM], paircnt;
Node	*winner = NULL;
Cell	*tmps;

static Cell	truecell	={ OBOOL, BTRUE, 0, 0, 1.0, NUM };
Cell	*true	= &truecell;
static Cell	falsecell	={ OBOOL, BFALSE, 0, 0, 0.0, NUM };
Cell	*false	= &falsecell;
static Cell	breakcell	={ OJUMP, JBREAK, 0, 0, 0.0, NUM };
Cell	*jbreak	= &breakcell;
static Cell	contcell	={ OJUMP, JCONT, 0, 0, 0.0, NUM };
Cell	*jcont	= &contcell;
static Cell	nextcell	={ OJUMP, JNEXT, 0, 0, 0.0, NUM };
Cell	*jnext	= &nextcell;
static Cell	exitcell	={ OJUMP, JEXIT, 0, 0, 0.0, NUM };
Cell	*jexit	= &exitcell;
static Cell	retcell		={ OJUMP, JRET, 0, 0, 0.0, NUM };
Cell	*jret	= &retcell;
static Cell	tempcell	={ OCELL, CTEMP, 0, 0, 0.0, NUM };

Node	*curnode = NULL;	/* the node being executed, for debugging */

run(a) Node *a;
{
	execute(a);
	closeall();
}

Cell *r_execute(u) Node *u;
{
	register Cell *(*proc)();
	register Cell *x;
	register Node *a;

	for (a = u; ; a = a->nnext) {
		curnode = a;
		if (isvalue(a) || a->ntype == NFIELD) {
			x = (Cell *) (a->narg[0]);
			x->ctype = OCELL;
			if ((x->tval & FLD) && !donefld)
				fldbld();
			else if ((x->tval & REC) && !donerec)
				recbld();
			return(x);
		}
		if (notlegal(a->nobj))	/* probably a Cell* but too risky to print */
			error(FATAL, "illegal statement");
		proc = proctab[a->nobj-FIRSTTOKEN];
		x = (*proc)(a->narg, a->nobj);
		if ((x->tval & FLD) && !donefld)
			fldbld();
		else if ((x->tval & REC) && !donerec)
			recbld();
		if (isexpr(a))
			return(x);
		/* a statement, goto next statement */
		if (isjump(x))
			return(x);
		if (a->nnext == (Node *)NULL)
			return(x);
		tempfree(x);
	}
}


Cell *program(a, n) register Node **a;
{
	register Cell *x;

	if (setjmp(env) != 0)
		goto ex;
	if (a[0]) {		/* BEGIN */
		x = execute(a[0]);
		if (isexit(x))
			return(true);
		if (isjump(x))
			error(FATAL, "unexpected break, continue or next");
		tempfree(x);
	}
  loop:
	if (a[1] || a[2])
		while (getrec(record) > 0) {
			x = execute(a[1]);
			if (isexit(x))
				break;
			tempfree(x);
		}
  ex:
	if (setjmp(env) != 0)
		goto ex1;
	if (a[2]) {		/* END */
		x = execute(a[2]);
		if (iscont(x))	/* read some more */
			goto loop;
		if (isbreak(x) || isnext(x))
			error(FATAL, "unexpected break or next");
		tempfree(x);
	}
  ex1:
	return(true);
}

#define	FRAME	100
struct Frame {
	int nargs;	/* number of arguments in this call */
	Cell *fcncell;	/* pointer to Cell for function */
	Cell **args;	/* pointer to array of arguments after execute */
	Cell *retval;	/* return value */
} frame[FRAME];

struct Frame *fp = frame;	/* frame pointer. bottom level unused */

Cell *call(a, n) Node **a;
{
	int i, ncall, ndef;
	Node *x;
	Cell **args, **oargs, *y, *z, *fcn;
	uchar *s;

	fcn = execute(a[0]);	/* the function itself */
	s = fcn->nval;
	if (!isfunc(fcn))
		error(FATAL, "calling undefined function %s", s);
	for (ncall = 0, x = a[1]; x != NULL; x = x->nnext)	/* args in call */
		ncall++;
	ndef = (int) fcn->fval;			/* args in defn */
	dprintf("calling %s, %d args (%d in defn), fp=%d\n", s, ncall, ndef, fp-frame);
	args = (Cell **) Calloc(ndef>ncall ? ndef : ncall, sizeof (Cell *));
	oargs = (Cell **) Calloc(ndef>ncall ? ndef : ncall, sizeof (Cell *));
	for (i = 0, x = a[1]; x != NULL; i++, x = x->nnext) {	/* get call args */
		dprintf("evaluate args[%d], fp=%d:\n", i, fp-frame);
		y = execute(x);
		oargs[i] = y;
		dprintf("args[%d]: %s %f <%s>, t=%o\n",
			   i, y->nval, y->fval, isarr(y) ? "(array)" : (char*) y->sval, y->tval);
		if (isfunc(y))
			error(FATAL, "can't use function %s as argument", y->nval);
		if (!isarr(y)) {
			args[i] = copycell(y);
		} else {
			args[i] = y;	/* arrays by ref */
		}
		tempfree(y);
	}
	for ( ; i < ndef; i++) {	/* add null args for ones not provided */
		args[i] = gettemp();
		args[i]->sval = (uchar *) "";
		args[i]->tval = STR|NUM|DONTFREE;
		args[i]->csub = CCOPY;
	}
	fp++;	/* now ok to up frame */
	if (fp >= frame+FRAME)
		error(FATAL, "function %s nested too deeply", s);
	fp->fcncell = fcn;
	fp->args = args;
	fp->nargs = ndef;	/* number defined with (can be more than call) */
	fp->retval = gettemp();

	dprintf("start exec of %s, fp=%d\n", s, fp-frame);
	y = execute((Node *)(fcn->sval));	/* execute body */
	dprintf("finished exec of %s, fp=%d\n", s, fp-frame);

	for (i = 0; i < ndef; i++) {
		Cell *t = fp->args[i];
		if (isarr(t)) {
			if (t->csub == CCOPY) {
				if (i >= ncall) {
					freesymtab(t);
					t->csub = CTEMP;
				} else {
					oargs[i]->tval = t->tval;
					oargs[i]->tval &= ~(STR|NUM|DONTFREE);
					oargs[i]->sval = t->sval;
				}
			}
		} else
			t->csub = CTEMP;
		tempfree(t);
	}
	xfree(oargs);
	if (isexit(y) || isnext(y))
		return y;
	z = fp->retval;			/* return value */
	dprintf("%s returns %g |%s| %o\n", s, getfval(z), getsval(z), z->tval);
	xfree(fp->args);
	fp--;
	tempfree(fcn);
	return(z);
}

Cell *copycell(x)	/* make a copy of a cell in a temp */
	Cell *x;
{
	Cell *y;

	y = gettemp();
	y->csub = CCOPY;	/* prevents freeing until call is over */
	y->nval = x->nval;
	y->sval = x->sval ? tostring(x->sval) : NULL;
	y->fval = x->fval;
	y->tval = x->tval & ~(CON|FLD|REC);	/* copy is not constant or field */
	return y;
}

Cell *arg(a) Node **a;
{
	int n;

	n = (int) a[0];	/* argument number, counting from 0 */
	dprintf("arg(%d), fp->nargs=%d\n", n, fp->nargs);
	if (n+1 > fp->nargs)
		error(FATAL, "argument #%d of function %s was not supplied",
			n+1, fp->fcncell->nval);
	return fp->args[n];
}

Cell *jump(a, n) Node **a;
{
	register Cell *y;

	switch (n) {
	case EXIT:
		if (a[0] != NULL) {
			y = execute(a[0]);
			errorflag = getfval(y);
			tempfree(y);
		}
		longjmp(env, 1);
	case RETURN:
		if (a[0] != NULL) {
			y = execute(a[0]);
			if ((y->tval & (STR|NUM)) == (STR|NUM)) {
				setsval(fp->retval, getsval(y));
				fp->retval->fval = getfval(y);
				fp->retval->tval |= NUM;
			}
			else if (y->tval & STR)
				setsval(fp->retval, getsval(y));
			else if (y->tval & NUM)
				setfval(fp->retval, getfval(y));
			tempfree(y);
		}
		return(jret);
	case NEXT:
		return(jnext);
	case BREAK:
		return(jbreak);
	case CONTINUE:
		return(jcont);
	default:
		error(FATAL, "illegal jump type %d", n);
	}
}

Cell *getline(a, n) Node **a; int n;
{
	/* a[0] is variable, a[1] is operator, a[2] is filename */
	register Cell *r, *x;
	uchar buf[RECSIZE];
	FILE *fp;

	fflush(stdout);	/* in case someone is waiting for a prompt */
	r = gettemp();
	if (a[1] != NULL) {		/* getline < file */
		x = execute(a[2]);		/* filename */
		if ((int) a[1] == '|')	/* input pipe */
			a[1] = (Node *) LE;	/* arbitrary flag */
		fp = openfile((int) a[1], getsval(x));
		tempfree(x);
		if (fp == NULL)
			n = -1;
		else
			n = readrec(buf, sizeof(buf), fp);
		if (n <= 0) {
			;
		} else if (a[0] != NULL) {	/* getline var <file */
			setsval(execute(a[0]), buf);
		} else {			/* getline <file */
			if (!(recloc->tval & DONTFREE))
				xfree(recloc->sval);
			strcpy(record, buf);
			recloc->sval = record;
			recloc->tval = REC | STR | DONTFREE;
			donerec = 1; donefld = 0;
		}
	} else {			/* bare getline; use current input */
		if (a[0] == NULL)	/* getline */
			n = getrec(record);
		else {			/* getline var */
			n = getrec(buf);
			setsval(execute(a[0]), buf);
		}
	}
	setfval(r, (Awkfloat) n);
	return r;
}

Cell *getnf(a,n) register Node **a;
{
	if (donefld == 0)
		fldbld();
	return (Cell *) a[0];
}

Cell *array(a,n) register Node **a;
{
	register Cell *x, *y, *z;
	register uchar *s;
	register Node *np;
	uchar buf[RECSIZE];

	x = execute(a[0]);	/* Cell* for symbol table */
	buf[0] = 0;
	for (np = a[1]; np; np = np->nnext) {
		y = execute(np);	/* subscript */
		s = getsval(y);
		strcat(buf, s);
		if (np->nnext)
			strcat(buf, *SUBSEP);
		tempfree(y);
	}
	if (!isarr(x)) {
		dprintf("making %s into an array\n", x->nval);
		if (freeable(x))
			xfree(x->sval);
		x->tval &= ~(STR|NUM|DONTFREE);
		x->tval |= ARR;
		x->sval = (uchar *) makesymtab();
	}
	z = setsymtab(buf, "", 0.0, STR|NUM, x->sval);
	z->ctype = OCELL;
	z->csub = CVAR;
	tempfree(x);
	return(z);
}

Cell *delete(a, n) Node **a;
{
	Cell *x, *y;
	Node *np;
	uchar buf[RECSIZE], *s;

	x = execute(a[0]);	/* Cell* for symbol table */
	buf[0] = 0;
	for (np = a[1]; np; np = np->nnext) {
		y = execute(np);	/* subscript */
		s = getsval(y);
		strcat(buf, s);
		if (np->nnext)
			strcat(buf, *SUBSEP);
		tempfree(y);
	}
	freeelem(x, buf);
	tempfree(x);
	return true;
}

Cell *intest(a, n) Node **a;
{
	register Cell *x, *ap, *k;
	Node *p;
	char buf[100];
	uchar *s;

	ap = execute(a[1]);	/* array name */
	if (!isarr(ap))
		error(FATAL, "%s is not an array", ap->nval);
	buf[0] = 0;
	for (p = a[0]; p; p = p->nnext) {
		x = execute(p);	/* expr */
		s = getsval(x);
		strcat(buf, s);
		tempfree(x);
		if (p->nnext)
			strcat(buf, *SUBSEP);
	}
	k = lookup(buf, (Cell **) ap->sval);
	tempfree(ap);
	if (k == NULL)
		return(false);
	else
		return(true);
}


Cell *matchop(a,n) Node **a;
{
	register Cell *x, *y;
	register uchar *s, *t;
	register int i;
	static struct fa *pfa = NULL;
	static uchar *prevre = NULL;
	extern int match(), pmatch();
	int (*mf)() = match, mode = 0;

	if (n == MATCHFCN) {
		mf = pmatch;
		mode = 1;
	}
	x = execute(a[1]);
	s = getsval(x);
	if (a[0] == 0)
		i = (*mf)(a[2], s);
	else {
		y = execute(a[2]);
		t = getsval(y);
		if (pfa == NULL) {
			pfa = makedfa(reparse(t), mode);
			prevre = tostring(t);
		} else if (strcmp(t, prevre) != 0) {
			xfree(prevre);
			prevre = tostring(t);
			freefa(pfa);
			pfa = makedfa(reparse(t), mode);
		}
		i = (*mf)(pfa, s);
		tempfree(y);
	}
	tempfree(x);
	if (n == MATCHFCN) {
		int start = patbeg - s + 1;
		if (patlen < 0)
			start = 0;
		setfval(rstartloc, (Awkfloat) start);
		setfval(rlengthloc, (Awkfloat) patlen);
		x = gettemp();
		x->tval = NUM;
		x->fval = start;
		return x;
	} else if (n == MATCH && i == 1 || n == NOTMATCH && i == 0)
		return(true);
	else
		return(false);
}


Cell *boolop(a,n) Node **a;
{
	register Cell *x, *y;
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
}

Cell *relop(a,n) Node **a;
{
	register int i;
	register Cell *x, *y;
	Awkfloat j;

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
}

tfree(a) register Cell *a;
{
	if (freeable(a))
		xfree(a->sval);
	a->tval = 0;
	a->ctype = OCELL;
	a->cnext = tmps;
	tmps = a;
}

Cell *gettemp()
{	int i;
	register Cell *x;

	if (!tmps) {
		tmps = (Cell *) Calloc(100, sizeof(Cell));
		if (!tmps)
			error(FATAL, "no space for temporaries");
		for(i = 1; i < 100; i++)
			tmps[i-1].cnext = &tmps[i];
		tmps[i-1].cnext = 0;
	}
	x = tmps;
	tmps = x->cnext;
	*x = tempcell;
	return(x);
}

Cell *indirect(a,n) Node **a;
{
	register Cell *x;
	register int m;
	register uchar *s;
	Cell *fieldadr();

	x = execute(a[0]);
	m = getfval(x);
	if (m == 0 && !isnumber(s = getsval(x)))	/* suspicion! */
		error(FATAL, "illegal field $(%s)", s);
	tempfree(x);
	x = fieldadr(m);
	x->ctype = OCELL;
	x->csub = CFLD;
	return(x);
}

Cell *substr(a, nnn) Node **a;
{
	register int k, m, n;
	register uchar *s;
	int temp;
	register Cell *x, *y;

	x = execute(a[0]);
	s = getsval(x);
	k = strlen(s) + 1;
	if (k <= 1) {
		tempfree(x);
		x = gettemp();
		setsval(x, "");
		return(x);
	}
	y = execute(a[1]);
	m = getfval(y);
	if (m <= 0)
		m = 1;
	else if (m > k)
		m = k;
	tempfree(y);
	if (a[2] != 0) {
		y = execute(a[2]);
		n = getfval(y);
		tempfree(y);
	}
	else
		n = k - 1;
	if (n < 0)
		n = 0;
	else if (n > k - m)
		n = k - m;
	dprintf("substr: m=%d, n=%d, s=%s\n", m, n, s);
	y = gettemp();
	temp = s[n+m-1];	/* with thanks to John Linderman */
	s[n+m-1] = '\0';
	setsval(y, s + m - 1);
	s[n+m-1] = temp;
	tempfree(x);
	return(y);
}

Cell *sindex(a, nnn) Node **a;
{
	register Cell *x, *y, *z;
	register uchar *s1, *s2, *p1, *p2, *q;
	Awkfloat v = 0.0;

	x = execute(a[0]);
	s1 = getsval(x);
	y = execute(a[1]);
	s2 = getsval(y);

	z = gettemp();
	for (p1 = s1; *p1 != '\0'; p1++) {
		for (q=p1, p2=s2; *p2 != '\0' && *q == *p2; q++, p2++)
			;
		if (*p2 == '\0') {
			v = (Awkfloat) (p1 - s1 + 1);	/* origin 1 */
			break;
		}
	}
	tempfree(x);
	tempfree(y);
	setfval(z, v);
	return(z);
}

uchar *format(buf, s, a) uchar *buf, *s; Node *a;
{
	uchar fmt[RECSIZE], tbuf[300], *savbuf;
	register uchar *p, *t, *os, *ep, *str;
	register Cell *x;
	int flag = 0;
	Awkfloat xf;

	os = s;
	p = savbuf = buf;
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
			if (islower(*s) && *s != 'l')
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
		case 'o': case 'x': case 'u':
			flag = *(s-1) == 'l' ? 2 : 3;
			break;
		case 's':
			flag = 4;
			break;
		case 'c':
			flag = 5;
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
			error(FATAL, "not enough args in printf(%s)", os);
		x = execute(a);
		a = a->nnext;
		if (flag != 4)	/* watch out for converting to numbers! */
			xf = getfval(x);
		/* 
		 * Get the string to check length; %s is the usual problem; 
		 * other conversions can cause overrun if they occur when
		 * the buffer is almost filled
		 */
		switch (flag) {
		case 1:	sprintf(tbuf, fmt, xf); break;
		case 2:	sprintf(tbuf, fmt, (long) xf); break;
		case 3:	sprintf(tbuf, fmt, (int) xf); break;
		case 4:	str = getsval(x); break;
		case 5:	sprintf(tbuf, fmt, (int) xf); break;
		}
		if (flag != 4) {
			if (strlen(tbuf) > sizeof(tbuf))
				error(FATAL, "formatted item %s... too long",
					tbuf);
			str = tbuf;
		}
		/* 
		 * If string overruns the buffer, reallocate;
		 * consider length of format string
		 * can't use realloc, originial buffer passed in is static
		 */
		if (ep < p + strlen(str) + strlen(s)) {
			int newlen, oldlen;
			uchar *newbuf;

			oldlen = p - buf;
			/* Add RECSIZE for additional space */
			newlen = oldlen + strlen(str) + RECSIZE;
			newbuf = (uchar *) malloc(newlen);
			if (newbuf == NULL)
			    error(FATAL, "out of format space");
			strncpy(newbuf, buf, oldlen);
			p = newbuf + oldlen;
			if (buf != savbuf)
				free(buf);  
			buf = newbuf;
			ep = buf + newlen;
		}
		/* Transfer string to buffer */
		if (flag == 4)
			sprintf(p, fmt, str); 
		else
			strcpy(p, str);
		tempfree(x);
		p += strlen(p);
		s++;
	}
	*p = '\0';
	return buf;
}

Cell *asprintf(a,n) Node **a;
{
	register Cell *x;
	register Node *y;
	uchar buf[RECSIZE], *bufp;

	y = a[0]->nnext;
	x = execute(a[0]);
	bufp = format(buf, getsval(x), y);
	tempfree(x);
	x = gettemp();
	if (bufp == buf)
		x->sval = tostring(buf);
	else
		x->sval = bufp;
	x->tval = STR;
	return(x);
}

Cell *aprintf(a,n) Node **a;
{
	FILE *fp;
	register Cell *x;
	register Node *y;
	uchar buf[RECSIZE], *bufp;

	y = a[0]->nnext;
	x = execute(a[0]);
	bufp = format(buf, getsval(x), y);
	tempfree(x);
	if (a[1] == NULL)
		fputs(bufp, stdout);
	else {
		fp = redirect((int)a[1], a[2]);
		fputs(bufp, fp);
		fflush(fp);
	}
	if (bufp != buf)
		free(bufp);
	return(true);
}

Cell *arith(a,n) Node **a;
{
	Awkfloat i, j;
	double v;
	register Cell *x, *y, *z;

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
			error(FATAL, "division by zero in mod");
		modf(i/j, &v);
		i = i - j * v;
		break;
	case UMINUS:
		i = -i;
		break;
	case POWER:
		i = errcheck(pow(i, j), "pow");
		break;
	default:
		error(FATAL, "illegal arithmetic operator %d", n);
	}
	setfval(z, i);
	return(z);
}

Cell *incrdecr(a, n) Node **a;
{
	register Cell *x, *z;
	register int k;
	Awkfloat xf;

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

Cell *assign(a,n) Node **a;
{
	register Cell *x, *y;
	Awkfloat xf, yf;
	double v;

	x = execute(a[0]);
	y = execute(a[1]);
	if (n == ASSIGN) {	/* ordinary assignment */
		if ((y->tval & (STR|NUM)) == (STR|NUM)) {
			setsval(x, getsval(y));
			x->fval = getfval(y);
			x->tval |= NUM;
		}
		else if (y->tval & STR)
			setsval(x, getsval(y));
		else if (y->tval & NUM)
			setfval(x, getfval(y));
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
			error(FATAL, "division by zero in /=");
		xf /= yf;
		break;
	case MODEQ:
		if (yf == 0)
			error(FATAL, "division by zero in %=");
		modf(xf/yf, &v);
		xf = xf - yf * v;
		break;
	case POWEQ:
		xf = errcheck(pow(xf, yf), "pow");
		break;
	default:
		error(FATAL, "illegal assignment operator %d", n);
		break;
	}
	tempfree(y);
	setfval(x, xf);
	return(x);
}

Cell *cat(a,q) Node **a;
{
	register Cell *x, *y, *z;
	register int n1, n2;
	register uchar *s;

	x = execute(a[0]);
	y = execute(a[1]);
	getsval(x);
	getsval(y);
	n1 = strlen(x->sval);
	n2 = strlen(y->sval);
	s = (uchar *) Malloc(n1 + n2 + 1);
	if (s == NULL)
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

Cell *pastat(a,n) Node **a;
{
	register Cell *x;

	if (a[0] == 0)
		x = execute(a[1]);
	else {
		x = execute(a[0]);
		if (istrue(x)) {
			tempfree(x);
			x = execute(a[1]);
		}
	}
	return x;
}

Cell *dopa2(a,n) Node **a;
{
	register Cell *x;
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

Cell *split(a,nnn) Node **a;
{
	Cell *x, *y, *ap;
	register uchar *s;
	register int sep;
	uchar *t, temp, num[5], *fs;
	int n, tempstat;

	y = execute(a[0]);
	s = getsval(y);
	if (a[2] == 0)
		fs = *FS;
	else {
		x = execute(a[2]);
		fs = getsval(x);
	}
	sep = *fs;
	ap = execute(a[1]);	/* array name */
	freesymtab(ap);
	dprintf("split: s=|%s|, a=%s, sep=|%s|\n", s, ap->nval, fs);
	ap->tval &= ~STR;
	ap->tval |= ARR;
	ap->sval = (uchar *) makesymtab();

	n = 0;
	if (*s != '\0' && strlen(fs) > 1) {	/* reg expr */
		static fa *pfa = NULL;
		static uchar *prevfs = NULL;
		if (pfa == NULL) {	/* first time thru */
			pfa = makedfa(reparse(fs), 1);
			prevfs = tostring(fs);
		} else if (strcmp(fs, prevfs) != 0) {	/* new fa needed for new FS */
			freefa(pfa);
			Free(prevfs);
			pfa = makedfa(reparse(fs), 1);
			prevfs = tostring(fs);
		}
		if (nematch(pfa,s)) {
			tempstat = pfa->initstat;
			pfa->initstat = 2;
			do {
				n++;
				sprintf(num, "%d", n);
				temp = *patbeg;
				*patbeg = '\0';
				if (isnumber(s))
					setsymtab(num, s, atof(s), STR|NUM, ap->sval);
				else
					setsymtab(num, s, 0.0, STR, ap->sval);
				*patbeg = temp;
				s = patbeg + patlen;
				if ((*(patbeg+patlen-1) == 0) || (*s == 0)) {
					n++;
					sprintf(num, "%d", n);
					setsymtab(num, "", 0.0, STR, ap->sval);
					pfa->initstat = tempstat;
					goto spdone;
				}
			} while(nematch(pfa,s));
		}
		n++;
		sprintf(num, "%d", n);
		if (isnumber(s))
			setsymtab(num, s, atof(s), STR|NUM, ap->sval);
		else
			setsymtab(num, s, 0.0, STR, ap->sval);
	} else if (sep == ' ') {
		for (n = 0; ; ) {
			while (*s == ' ' || *s == '\t' || *s == '\n')
				s++;
			if (*s == 0)
				break;
			n++;
			t = s;
			do
				s++;
			while (*s!=' ' && *s!='\t' && *s!='\n' && *s!='\0');
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, t, atof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, t, 0.0, STR, ap->sval);
			*s = temp;
			if (*s != 0)
				s++;
		}
	} else if (*s != 0) {
		for (;;) {
			n++;
			t = s;
			while (*s != sep && *s != '\n' && *s != '\0')
				s++;
			temp = *s;
			*s = '\0';
			sprintf(num, "%d", n);
			if (isnumber(t))
				setsymtab(num, t, atof(t), STR|NUM, ap->sval);
			else
				setsymtab(num, t, 0.0, STR, ap->sval);
			*s = temp;
			if (*s++ == 0)
				break;
		}
	}
spdone:
	tempfree(ap);
	tempfree(y);
	if (a[2] != 0)
		tempfree(x);
	x = gettemp();
	x->tval = NUM;
	x->fval = n;
	return(x);
}

Cell *condexpr(a,n) Node **a;
{
	register Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	} else {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

Cell *ifstat(a,n) Node **a;
{
	register Cell *x;

	x = execute(a[0]);
	if (istrue(x)) {
		tempfree(x);
		x = execute(a[1]);
	} else if (a[2] != 0) {
		tempfree(x);
		x = execute(a[2]);
	}
	return(x);
}

Cell *whilestat(a,n) Node **a;
{
	register Cell *x;

	for (;;) {
		x = execute(a[0]);
		if (!istrue(x))
			return(x);
		tempfree(x);
		x = execute(a[1]);
		if (isbreak(x)) {
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
	}
}

Cell *dostat(a,n) Node **a;
{
	register Cell *x;

	for (;;) {
		x = execute(a[0]);
		if (isbreak(x)) {
			x = true;
			return(x);
		}
		if (isnext(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
		x = execute(a[1]);
		if (!istrue(x))
			return(x);
		tempfree(x);
	}
}

Cell *forstat(a,n) Node **a;
{
	register Cell *x;

	x = execute(a[0]);
	tempfree(x);
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
		if (isnext(x) || isexit(x) || isret(x))
			return(x);
		tempfree(x);
		x = execute(a[2]);
		tempfree(x);
	}
}

int	indepth	= 0;	/* depth of for (i in ...) statements */

Cell *instat(a, n) Node **a;
{
	register Cell *x, *vp, *arrayp, *cp, *ncp, **tp;
	int i;

	vp = execute(a[0]);
	arrayp = execute(a[1]);
	if (!isarr(arrayp))
		error(FATAL, "%s is not an array", arrayp->nval);
	tp = (Cell **) arrayp->sval;
	tempfree(arrayp);
	indepth++;
	for (i = 0; i < MAXSYM; i++) {	/* this routine knows too much */
		for (cp = tp[i]; cp != NULL; cp = ncp) {
			setsval(vp, cp->nval);
			ncp = cp->cnext;
			x = execute(a[2]);
			if (isbreak(x)) {
				tempfree(vp);
				x = true;
				indepth--;
				return(x);
			}
			if (isnext(x) || isexit(x) || isret(x)) {
				tempfree(vp);
				indepth--;
				return(x);
			}
			tempfree(x);
		}
	}
	indepth--;
	return true;
}

Cell *bltin(a,n) Node **a;
{
	register Cell *x, *y;
	Awkfloat u;
	register int t;

	t = (int) a[0];
	x = execute(a[1]);
	switch (t) {
	case FLENGTH:
		u = (Awkfloat) strlen(getsval(x)); break;
	case FLOG:
		u = errcheck(log(getfval(x)), "log"); break;
	case FINT:
		modf(getfval(x), &u); break;
	case FEXP:
		u = errcheck(exp(getfval(x)), "exp"); break;
	case FSQRT:
		u = errcheck(sqrt(getfval(x)), "sqrt"); break;
	case FSIN:
		u = sin(getfval(x)); break;
	case FCOS:
		u = cos(getfval(x)); break;
	case FATAN:
		y = execute(a[1]->nnext);
		u = atan2(getfval(x), getfval(y));
		tempfree(y);
		break;
	case FSYSTEM:
		fflush(stdout);		/* in case something buffered already */
		u = (Awkfloat) system(getsval(x)) / 256;
		break;
	case FRAND:
		u = (Awkfloat) rand() / 32767.0; break;	/* 2^31-1 better on some systems? */
	case FSRAND:
		if (x->tval & REC)
			u = (Awkfloat) srand(time(0));
		else
			u = (Awkfloat) srand((int)getfval(x));
		break;
	default:
		error(FATAL, "illegal function type %d", t); break;
	}
	tempfree(x);
	x = gettemp();
	setfval(x, u);
	return(x);
}

Cell *print(a,n) Node **a;
{
	register Node *x;
	register Cell *y;
	FILE *fp;

	if (a[1] == 0)
		fp = stdout;
	else
		fp = redirect((int)a[1], a[2]);
	for (x=a[0]; x!=NULL; x=x->nnext) {
		y = execute(x);
		fputs(getsval(y), fp);
		tempfree(y);
		if (x->nnext == NULL)
			fputs(*ORS, fp);
		else
			fputs(*OFS, fp);
	}
	if (a[1] != 0)
		fflush(fp);
	return(true);
}

Cell *nullproc() {}


#define FILENUM	15
struct
{
	FILE	*fp;
	uchar	*fname;
	int	mode;	/* '|', 'a', 'w' */
} files[FILENUM];
FILE *popen();

FILE *redirect(a, b)
	Node *b;
{
	FILE *fp;
	Cell *x;
	uchar *fname;

	x = execute(b);
	fname = getsval(x);
	fp = openfile(a, fname);
	if (fp == NULL)
		error(FATAL, "can't open file %s", fname);
	tempfree(x);
	return fp;
}

FILE *openfile(a, s)
	uchar *s;
{
	register int i, m;
	register FILE *fp;

	if (*s == '\0')
		error(FATAL, "null file name in print or getline");
	for (i=0; i < FILENUM; i++)
		if (files[i].fname && strcmp(s, files[i].fname) == 0)
			if (a == files[i].mode || a==APPEND && files[i].mode==GT)
				return files[i].fp;
	for (i=0; i < FILENUM; i++)
		if (files[i].fp == 0)
			break;
	if (i >= FILENUM)
		error(FATAL, "%s makes too many open files", s);
	m = a;
	if (a == GT) {
		fp = fopen(s, "w");
	} else if (a == APPEND) {
		fp = fopen(s, "a");
		m = GT;	/* so can mix > and >> */
	} else if (a == '|') {	/* output pipe */
		fp = popen(s, "w");
	} else if (a == LE) {	/* input pipe */
		fp = popen(s, "r");
	} else if (a == LT) {	/* getline <file */
		fp = fopen(s, "r");
	} else
		error(FATAL, "illegal redirection");
	if (fp != NULL) {
		files[i].fname = tostring(s);
		files[i].fp = fp;
		files[i].mode = m;
	}
	return fp;
}


Cell *closefile(a) Node **a;
{
	register Cell *x;
	int i;

	x = execute(a[0]);
	getsval(x);
	for (i = 0; i < FILENUM; i++)
		if (files[i].fname && strcmp(x->sval, files[i].fname) == 0) {
			if (files[i].mode == '|' || files[i].mode == LE)
				pclose(files[i].fp);
			else
				fclose(files[i].fp);
			xfree(files[i].fname);
			files[i].fname = NULL;	/* watch out for ref thru this */
			files[i].fp = NULL;
		}
	return(x);
}

closeall()
{
	int i;

	for (i = 0; i < FILENUM; i++)
		if (files[i].fp) {
			if (files[i].mode == '|' || files[i].mode == LE)
				pclose(files[i].fp);
			else
				fclose(files[i].fp);
		}
}

Cell *sub(a, nnn) Node **a;
{
	register uchar *sptr, *pb, *q;
	register Cell *x, *y;
	uchar buf[RECSIZE], *t;
	fa *pfa;

	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(reparse(getsval(y)), 1);
		tempfree(y);
	}
	if (pmatch(pfa, t)) {
		pb = buf;
		sptr = t;
		while (sptr < patbeg)
			*pb++ = *sptr++;
		y = execute(a[2]);	/* replacement string */
		sptr = getsval(y);
		while (*sptr != 0)
			if (*sptr == '\\' && *(sptr+1) == '&') {
				sptr++;		/* skip \, */
				*pb++ = *sptr++; /* add & */
			} else if (*sptr == '&') {
				sptr++;
				for (q = patbeg; q < patbeg+patlen; )
					*pb++ = *q++;
			} else
				*pb++ = *sptr++;
		*pb = '\0';
		tempfree(y);
		sptr = patbeg + patlen;
		if ((patlen == 0 && *patbeg) || (patlen && *(sptr-1)))
			while (*pb++ = *sptr++)
				;
		setsval(x, buf);
		tempfree(x);
		return (true);
	}
	tempfree(x);
	return (false);
}

Cell *gsub(a, nnn) Node **a;
{
	register Cell *x, *y;
	register uchar *rptr, *sptr, *t, *pb;
	uchar buf[RECSIZE];
	register fa *pfa;
	int mflag, tempstat, num;

	mflag = 0;	/* if mflag == 0, can replace empty string */
	num = 0;
	x = execute(a[3]);	/* target string */
	t = getsval(x);
	if (a[0] == 0)
		pfa = (fa *) a[1];	/* regular expression */
	else {
		y = execute(a[1]);
		pfa = makedfa(reparse(getsval(y)), 1);
		tempfree(y);
	}
	if (pmatch(pfa, t)) {
		tempstat = pfa->initstat;
		pfa->initstat = 2;
		pb = buf;
		y = execute(a[2]);	/* replacement string */
		rptr = getsval(y);
		do {
/*
uchar *p;
int i;
printf("target string: %s, *patbeg = %o, patlen = %d\n", t, *patbeg, patlen);
printf("	match found: ");
p=patbeg;
for (i=0; i<patlen; i++)
	printf("%c", *p++);
printf("\n");
*/
			num++;
			if (patlen == 0) {	/* matched empty string */
				if (mflag == 0) {	/* can replace empty */
					sptr = rptr;
					while (*sptr != 0)
						if (*sptr == '\\' && *(sptr+1) == '&') {
							sptr++;
							*pb++ = *sptr++;
						} else if (*sptr == '&') {
							uchar *q;
							sptr++;
							for (q = patbeg; q < patbeg+patlen; )
								*pb++ = *q++;
						} else
							*pb++ = *sptr++;
				}
				if (*t == 0)	/* at end */
					goto done;
				*pb++ = *t++;
				mflag = 0;
			}
			else {	/* matched nonempty string */
				sptr = t;
				while (sptr < patbeg)
					*pb++ = *sptr++;
				sptr = rptr;
				while (*sptr != 0)
					if (*sptr == '\\' && *(sptr+1) == '&') {
						sptr++;
						*pb++ = *sptr++;
					} else if (*sptr == '&') {
						uchar *q;
						sptr++;
						for (q = patbeg; q < patbeg+patlen; )
							*pb++ = *q++;
					} else
						*pb++ = *sptr++;
				t = patbeg + patlen;
				if ((*(t-1) == 0) || (*t == 0))
					goto done;
				mflag = 1;
			}
		} while (pmatch(pfa,t));
		sptr = t;
		while (*pb++ = *sptr++)
			;
	done:	tempfree(y);
		*pb = '\0';
		setsval(x, buf);
		pfa->initstat = tempstat;
	}
	tempfree(x);
	x = gettemp();
	x->tval = NUM;
	x->fval = num;
	return(x);
}
