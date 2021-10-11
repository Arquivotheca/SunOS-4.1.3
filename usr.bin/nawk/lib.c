/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)lib.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.6 */
#endif

#define DEBUG
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include "awk.h"
#include "y.tab.h"

#define	getfval(p)	(((p)->tval & (ARR|FLD|REC|NUM)) == NUM ? (p)->fval : r_getfval(p))
#define	getsval(p)	(((p)->tval & (ARR|FLD|REC|STR)) == STR ? (p)->sval : r_getsval(p))

extern	Awkfloat r_getfval();
extern	uchar	*r_getsval();

FILE	*infile	= NULL;
uchar	*file	= (uchar*) "";
uchar	recdata[RECSIZE];
uchar	*record	= recdata;
uchar	fields[RECSIZE];

#define	MAXFLD	101	/* one greater than the maximum number of fields */
int	donefld;	/* 1 = implies rec broken into fields */
int	donerec;	/* 1 = record is valid (no flds have changed) */

Cell emptyfld = { OCELL, CFLD, NULL, (uchar*) "", 0.0, FLD|STR|DONTFREE };

Cell fldtab[MAXFLD] = {		/* room for fields */
	{ OCELL, CFLD, (uchar*) "$0", recdata, 0.0, REC|STR|DONTFREE},
};
int	maxfld	= 0;	/* last used field */
int	argno	= 1;	/* current input argument number */

void initfld()
{	int i;

	for (i = 1; i < MAXFLD; i++)
		fldtab[i] = emptyfld;
}

initgetrec()
{
	infile = stdin;
	*FILENAME = file = (uchar*) "-";
}

getrec(buf)
	uchar *buf;
{
	uchar *getargv();
	int c;
	extern Awkfloat *ARGC;

	dprintf("RS=<%s>, FS=<%s>\n", *RS, *FS);
	donefld = 0;
	donerec = 1;
	buf[0] = 0;
	while (argno < *ARGC || infile == stdin) {
		dprintf("argno=%d, file=|%s|\n", argno, file);
		if (infile == NULL) {	/* have to open a new file */
			file = getargv(argno);
			if (*file == '\0') {	/* it's been zapped */
				argno++;
				continue;
			}
			if (isclvar(file)) {	/* a var=value arg */
				setclvar(file);
				argno++;
				continue;
			}
			*FILENAME = file;
			dprintf("opening file %s\n", file);
			if (*file == '-' && *(file+1) == '\0')
				infile = stdin;
			else if ((infile = fopen(file, "r")) == NULL)
				error(FATAL, "can't open %s", file);
			setfval(fnrloc, 0.0);
		}
		c = readrec(buf, RECSIZE, infile);
		if (c != 0 || buf[0] != '\0') {	/* normal record */
			if (buf == record) {
				if (!(recloc->tval & DONTFREE))
					xfree(recloc->sval);
				recloc->sval = record;
				recloc->tval = REC | STR | DONTFREE;
			}
			setfval(nrloc, nrloc->fval+1);
			setfval(fnrloc, fnrloc->fval+1);
			return 1;
		}
		/* EOF arrived on this file; set up next */
		if (infile != stdin)
			fclose(infile);
		infile = NULL;
		argno++;
	}
	return 0;	/* true end of file */
}

readrec(buf, bufsize, inf)	/* read one record into buf */
	uchar *buf;
	int bufsize;
	FILE *inf;
{
	register int sep, c;
	register uchar *rr, *er;

	if ((sep = **RS) == 0) {
		sep = '\n';
		while ((c=getc(inf)) == '\n' && c != EOF)	/* skip leading \n's */
			;
		if (c != EOF)
			ungetc(c, inf);
	}
	er = buf + bufsize;
	for (rr = buf; ; ) {
		for (; (c=getc(inf)) != sep && c != EOF && rr < er; *rr++ = c)
			;
		if (rr >= er)
			error(FATAL, "input record `%.20s...' too long", buf);
		if (**RS == sep || c == EOF)
			break;
		if ((c = getc(inf)) == '\n' || c == EOF) /* 2 in a row */
			break;
		*rr++ = '\n';
		*rr++ = c;
	}
	if (rr > er)
		error(FATAL, "input record `%.20s...' too long", buf);
	*rr = 0;
	dprintf("readrec saw <%s>, returns %d\n", buf, c == EOF && rr == buf ? 0 : 1);
	return c == EOF && rr == buf ? 0 : 1;
}

uchar *getargv(n)	/* get ARGV[n] */
	int n;
{
	Cell *x;
	uchar *s, temp[10];
	extern Cell **ARGVtab;

	sprintf(temp, "%d", n);
	x = setsymtab(temp, "", 0.0, STR, ARGVtab);
	s = getsval(x);
	dprintf("getargv(%d) returns |%s|\n", n, s, NULL);
	return s;
}

setclvar(s)	/* set var=value from s */
uchar *s;
{
	uchar *p;
	Cell *q;

	for (p=s; *p != '='; p++)
		;
	*p++ = 0;
	q = setsymtab(s, p, 0.0, STR, symtab);
	setsval(q, p);
	if (isnumber(q->sval)) {
		q->fval = atof(q->sval);
		q->tval |= NUM;
	}
	dprintf("command line set %s to |%s|\n", s, p, NULL);
}


fldbld()
{
	register uchar *r, *fr, sep;
	Cell *p, *q;
	int i;

	if (donefld)
		return;
	if (!(recloc->tval & STR))
		getsval(recloc);
	r = recloc->sval;	/* was record! */
	fr = fields;
	i = 0;	/* number of fields accumulated here */
	if (strlen(*FS) > 1) {	/* it's a regular expression */
		i = refldbld(r, *FS);
	} else if ((sep = **FS) == ' ') {
		for (i = 0; ; ) {
			while (*r == ' ' || *r == '\t' || *r == '\n')
				r++;
			if (*r == 0)
				break;
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | DONTFREE;
			do
				*fr++ = *r++;
			while (*r != ' ' && *r != '\t' && *r != '\n' && *r != '\0');
			*fr++ = 0;
		}
		*fr = 0;
	} else if (*r != 0) {	/* if 0, it's a null field */
		for (;;) {
			i++;
			if (i >= MAXFLD)
				break;
			if (!(fldtab[i].tval & DONTFREE))
				xfree(fldtab[i].sval);
			fldtab[i].sval = fr;
			fldtab[i].tval = FLD | STR | DONTFREE;
			while (*r != sep && *r != '\n' && *r != '\0')	/* \n always a separator */
				*fr++ = *r++;
			*fr++ = 0;
			if (*r++ == 0)
				break;
		}
		*fr = 0;
	}
	if (i >= MAXFLD)
		error(FATAL, "record `%.20s...' has too many fields", record);
	/* clean out junk from previous record */
	cleanfld(i, maxfld);
	maxfld = i;
	donefld = 1;
	for (p = fldtab+1; p <= fldtab+maxfld; p++) {
		if(isnumber(p->sval)) {
			p->fval = atof(p->sval);
			p->tval |= NUM;
		}
	}
	setfval(nfloc, (Awkfloat) maxfld);
	if (dbg)
		for (p = fldtab; p <= fldtab+maxfld; p++)
			printf("field %d: |%s|\n", p-fldtab, p->sval);
}

cleanfld(n1, n2)	/* clean out fields n1..n2 inclusive */
{
	static uchar *nullstat = (uchar *) "";
	register Cell *p, *q;

	for (p = &fldtab[n2], q = &fldtab[n1]; p > q; p--) {
		if (!(p->tval & DONTFREE))
			xfree(p->sval);
		p->tval = FLD | STR | DONTFREE;
		p->sval = nullstat;
	}
}

newfld(n)	/* add field n (after end) */
{
	if (n >= MAXFLD)
		error(FATAL, "creating too many fields", record);
	cleanfld(maxfld, n);
	maxfld = n;
	setfval(nfloc, (Awkfloat) n);
}

refldbld(rec, fs)	/* build fields from reg expr in FS */
	uchar *rec, *fs;
{
	fa *makedfa();
	uchar *fr;
	int i, tempstat;
	static fa *pfa = NULL;
	static uchar *prevfs = NULL;

	fr = fields;
	*fr = '\0';
	if (*rec == '\0')
		return 0;
	if (pfa == NULL) {	/* first time thru */
		pfa = makedfa(reparse(fs), 1);
		prevfs = tostring(fs);
	} else if (strcmp(fs, prevfs) != 0) {	/* new fa needed for new FS */
		freefa(pfa);
		Free(prevfs);
		pfa = makedfa(reparse(fs), 1);
		prevfs = tostring(fs);
	}
	dprintf("into refldbld, rec = <%s>, pat = <%s>\n", rec, fs);
	tempstat = pfa->initstat;
	for (i = 1; i < MAXFLD; i++) {
		if (!(fldtab[i].tval & DONTFREE))
			xfree(fldtab[i].sval);
		fldtab[i].tval = FLD | STR | DONTFREE;
		fldtab[i].sval = fr;
		dprintf("refldbld: i=%d\n", i);
		if (nematch(pfa, rec)) {
			pfa->initstat = 2;
			dprintf("match %s (%d chars)\n", patbeg, patlen);
			strncpy(fr, rec, patbeg-rec);
			fr += patbeg - rec + 1;
			*(fr-1) = '\0';
			rec = patbeg + patlen;
		} else {
			dprintf("no match %s\n", rec);
			strcpy(fr, rec);
			pfa->initstat = tempstat;
			break;
		}
	}
	return i;		
}

recbld()
{
	int i;
	register uchar *r, *p;
	static uchar rec[RECSIZE];

	if (donerec == 1)
		return;
	r = rec;
	for (i = 1; i <= *NF; i++) {
		p = getsval(&fldtab[i]);
		while (*r = *p++)
			r++;
		if (i < *NF)
			for (p = *OFS; *r = *p++; )
				r++;
	}
	*r = '\0';
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	recloc->tval = REC | STR | DONTFREE;
	recloc->sval = record = rec;
	dprintf("in recbld FS=%o, recloc=%o\n", **FS, recloc, NULL);
	if (r > record + RECSIZE)
		error(FATAL, "built giant record `%.20s...'", record);
	dprintf("recbld = |%s|\n", record, NULL, NULL);
	donerec = 1;
}

Cell *fieldadr(n)
{
	if (n < 0 || n >= MAXFLD)
		error(FATAL, "trying to access field %d", n);
	return(&fldtab[n]);
}

int	errorflag	= 0;

yyerror(s, a1, a2, a3, a4, a5, a6, a7)
	uchar *s;
{
	extern uchar *cmdname, *curfname;
	static int been_here = 0;

	if (been_here++ > 2)
		return;
	fprintf(stderr, "%s: ", cmdname);
	fprintf(stderr, s, a1, a2, a3, a4, a5, a6, a7);
	fprintf(stderr, " at source line %d", lineno);
	if (curfname != NULL)
		fprintf(stderr, " in function %s", curfname);
	fprintf(stderr, "\n");
	errorflag = 2;
	eprint();
}

fpecatch()
{
	error(FATAL, "floating point exception");
}

bracecheck()
{
	extern int bracecnt, brackcnt, parencnt;
	int c;

	while ((c = input()) != EOF && c != '\0')
		bclass(c);
	bcheck2(bracecnt, '{', '}');
	bcheck2(brackcnt, '[', ']');
	bcheck2(parencnt, '(', ')');
}

bcheck2(n, c1, c2)
{
	if (n == 1)
		fprintf(stderr, "\t1 extra %c\n", c1);
	else if (n > 1)
		fprintf(stderr, "\t%d extra %c's\n", n, c1);
	else if (n == -1)
		fprintf(stderr, "\t1 extra %c\n", c2);
	else if (n < -1)
		fprintf(stderr, "\t%d extra %c's\n", -n, c2);
}

error(f, s, a1, a2, a3, a4, a5, a6, a7)
{
	extern Node *curnode;
	extern uchar *cmdname;

	fprintf(stderr, "%s: ", cmdname);
	fprintf(stderr, s, a1, a2, a3, a4, a5, a6, a7);
	fprintf(stderr, "\n");
	if (NR && *NR > 0) {
		fprintf(stderr, " input record number %g", *FNR);
		if (strcmp(*FILENAME, "-") != 0)
			fprintf(stderr, ", file %s", *FILENAME);
		fprintf(stderr, "\n");
	}
	if (curnode)
		fprintf(stderr, " source line number %d\n", curnode->lineno);
	else if (lineno)
		fprintf(stderr, " source line number %d\n", lineno);
	eprint();
	if (f) {
		if (dbg)
			abort();
		exit(2);
	}
}

eprint()	/* try to print context around error */
{
	uchar *p, *q;
	int c;
	static int been_here = 0;
	extern int compile_time;
	extern uchar ebuf[300], *ep;

	if (compile_time == 0 || been_here++ > 0)
		return;
	p = ep - 1;
	if (p > ebuf && *p == '\n')
		p--;
	for ( ; p > ebuf && *p != '\n' && *p != '\0'; p--)
		;
	while (*p == '\n')
		p++;
	fprintf(stderr, " context is\n\t");
	for (q=ep-1; q>=p && *q!=' ' && *q!='\t' && *q!='\n'; q--)
		;
	while (p < q)
		putc(*p++, stderr);
	fprintf(stderr, " >>> ");
	while (p < ep)
		putc(*p++, stderr);
	fprintf(stderr, " <<< ");
	if (*ep)
		while ((c = input()) != '\n' && c != '\0' && c != EOF) {
			putc(c, stderr);
			bclass(c);
		}
	putc('\n', stderr);
	ep = ebuf;
}

bclass(c)
{
	switch (c) {
	case '{': bracecnt++; break;
	case '}': bracecnt--; break;
	case '[': brackcnt++; break;
	case ']': brackcnt--; break;
	case '(': parencnt++; break;
	case ')': parencnt--; break;
	}
}

double errcheck(x, s)
	double x;
	uchar *s;
{
	extern int errno;

	if (errno == EDOM) {
		errno = 0;
		error(!FATAL, "%s argument out of domain", s);
		x = 1;
	} else if (errno == ERANGE) {
		errno = 0;
		error(!FATAL, "%s result out of range", s);
		x = 1;
	}
	return x;
}

PUTS(s) uchar *s; {
	dprintf("%s\n", s, NULL, NULL);
}

isclvar(s)	/* is s of form var=something? */
	char *s;
{
	for ( ; *s; s++)
		if (!isalnum(*s))
			break;
	return *s == '=';
}

#define	MAXEXPON	38	/* maximum exponenet for fp number */

isnumber(s)
register uchar *s;
{
	register d1, d2;
	int point;
	uchar *es;

	d1 = d2 = point = 0;
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(0);	/* empty stuff isn't number */
	if (*s == '+' || *s == '-')
		s++;
	if (!isdigit(*s) && *s != '.')
		return(0);
	if (isdigit(*s)) {
		do {
			d1++;
			s++;
		} while (isdigit(*s));
	}
	if(d1 >= MAXEXPON)
		return(0);	/* too many digits to convert */
	if (*s == '.') {
		point++;
		s++;
	}
	if (isdigit(*s)) {
		d2++;
		do {
			s++;
		} while (isdigit(*s));
	}
	if (!(d1 || point && d2))
		return(0);
	if (*s == 'e' || *s == 'E') {
		s++;
		if (*s == '+' || *s == '-')
			s++;
		if (!isdigit(*s))
			return(0);
		es = s;
		do {
			s++;
		} while (isdigit(*s));
		if (s - es > 2)
			return(0);
		else if (s - es == 2 && 10 * (*es-'0') + *(es+1)-'0' >= MAXEXPON)
			return(0);
	}
	while (*s == ' ' || *s == '\t' || *s == '\n')
		s++;
	if (*s == '\0')
		return(1);
	else
		return(0);
}
