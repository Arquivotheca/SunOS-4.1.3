/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tran.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.4 */
#endif

#define	DEBUG
#include <stdio.h>
#include "awk.h"
#include "y.tab.h"

Cell *symtab[MAXSYM];	/* symbol table pointers */

uchar	**FS;		/* initial field sep */
uchar	**RS;		/* initial record sep */
uchar	**OFS;		/* output field sep */
uchar	**ORS;		/* output record sep */
uchar	**OFMT;		/*output format for numbers*/
Awkfloat *NF;		/* number of fields in current record */
Awkfloat *NR;		/* number of current record */
Awkfloat *FNR;		/* number of current record in current file */
uchar	**FILENAME;	/* current filename argument */
Awkfloat *ARGC;		/* number of arguments from command line */
uchar	**SUBSEP;	/* subscript separator for a[i,j,k]; default \034 */
Awkfloat *RSTART;	/* start of re matched with ~; origin 1 (!) */
Awkfloat *RLENGTH;	/* length of same */

Cell	*recloc;	/* location of record */
Cell	*nrloc;		/* NR */
Cell	*nfloc;		/* NF */
Cell	*fnrloc;	/* FNR */
Cell	**ARGVtab;	/* symbol table containing ARGV[...] */
Cell	*rstartloc;	/* RSTART */
Cell	*rlengthloc;	/* RLENGTH */

Cell	*nullloc;
Node	*nullnode;	/* zero&null, converted into a node for comparisons */

syminit(ac, av)
	uchar *av[];
{
	extern Node *valtonode();
	extern Cell fldtab[];

	arginit(ac, av);
	initfld();
	setsymtab("0", "0", 0.0, NUM|STR|CON, symtab);
	/* this is used for if(x)... tests: */
	nullloc = setsymtab("$zero&null", "", 0.0, NUM|STR|CON, symtab);
	nullnode = valtonode(nullloc, CCON);
	/* recloc = setsymtab("$0", record, 0.0, REC|STR|DONTFREE, symtab); */
	recloc = &fldtab[0];
	/* dprintf("recloc %o lookup %o\n", recloc, lookup("$0", symtab), NULL); */
	FS = &setsymtab("FS", " ", 0.0, STR, symtab)->sval;
	RS = &setsymtab("RS", "\n", 0.0, STR, symtab)->sval;
	OFS = &setsymtab("OFS", " ", 0.0, STR, symtab)->sval;
	ORS = &setsymtab("ORS", "\n", 0.0, STR, symtab)->sval;
	OFMT = &setsymtab("OFMT", "%.6g", 0.0, STR, symtab)->sval;
	FILENAME = &setsymtab("FILENAME", "", 0.0, STR, symtab)->sval;
	nfloc = setsymtab("NF", "", 0.0, NUM, symtab);
	NF = &nfloc->fval;
	nrloc = setsymtab("NR", "", 0.0, NUM, symtab);
	NR = &nrloc->fval;
	fnrloc = setsymtab("FNR", "", 0.0, NUM, symtab);
	FNR = &fnrloc->fval;
	SUBSEP = &setsymtab("SUBSEP", "\034", 0.0, STR, symtab)->sval;
	rstartloc = setsymtab("RSTART", "", 0.0, NUM, symtab);
	RSTART = &rstartloc->fval;
	rlengthloc = setsymtab("RLENGTH", "", 0.0, NUM, symtab);
	RLENGTH = &rlengthloc->fval;
}

arginit(ac, av)
	int ac;
	uchar *av[];
{
	Cell *cp, **makesymtab();
	int i;
	uchar temp[5];

	ARGC = &setsymtab("ARGC", "", (Awkfloat) ac, NUM, symtab)->fval;
	cp = setsymtab("ARGV", "", 0.0, ARR, symtab);
	ARGVtab = makesymtab();
	cp->sval = (uchar *) ARGVtab;
	for (i = 0; i < ac; i++) {
		sprintf(temp, "%d", i);
		if (isnumber(*av))
			setsymtab(temp, *av, atof(*av), STR|NUM, cp->sval);
		else
			setsymtab(temp, *av, 0.0, STR, cp->sval);
		av++;
	}
}


Cell **makesymtab()
{
	Cell **cp;

	cp = (Cell **) Calloc(MAXSYM, sizeof(Cell *));
	if (cp == NULL)
		error(FATAL, "out of space in makesymtab");
	return(cp);
}

freesymtab(ap)	/* free symbol table */
	Cell *ap;
{
	Cell *cp, **tp;
	int i;

	if (!isarr(ap))
		return;
	tp = (Cell **) ap->sval;
	if (tp == NULL)
		return;
	for (i = 0; i < MAXSYM; i++) {
		for (cp = tp[i]; cp != NULL; cp = cp->cnext) {
			xfree(cp->nval);
			xfree(cp->sval);
			Free(cp);
		}
	}
	xfree(tp);
}

freeelem(ap, s)		/* free elem s from ap (i.e., ap["s"] */
	Cell *ap;
	uchar *s;
{
	Cell **tab, *p, *prev = NULL;
	int h;
	
	tab = (Cell **) ap->sval;
	h = hash(s);
	for (p = tab[h]; p != NULL; prev = p, p = p->cnext)
		if (strcmp(s, p->nval) == 0) {
			if (prev == NULL)	/* 1st one */
				tab[h] = p->cnext;
			else			/* middle somewhere */
				prev->cnext = p->cnext;
			if (!(p->tval&DONTFREE))
				xfree(p->sval);
			Free(p->nval);
			Free(p);
			return;
		}
}

Cell *setsymtab(n, s, f, t, tab)
	uchar *n, *s;
	Awkfloat f;
	unsigned t;
	Cell **tab;
{
	register h;
	register Cell *p;
	Cell *lookup();

	if (n != NULL && (p = lookup(n, tab)) != NULL) {
		dprintf("setsymtab found %o: n=%s", p, p->nval, NULL);
		dprintf(" s=\"%s\" f=%g t=%o\n", p->sval, p->fval, p->tval);
		return(p);
	}
	p = (Cell *) Malloc(sizeof(Cell));
	if (p == NULL)
		error(FATAL, "symbol table overflow at %s", n);
	p->nval = tostring(n);
	p->sval = s ? tostring(s) : tostring("");
	p->fval = f;
	p->tval = t;
	h = hash(n);
	p->cnext = tab[h];
	tab[h] = p;
	dprintf("setsymtab set %o: n=%s", p, p->nval, NULL);
	dprintf(" s=\"%s\" f=%g t=%o\n", p->sval, p->fval, p->tval);
	return(p);
}

hash(s)	/* form hash value for string s */
register uchar *s;
{
	register int hashval;

	for (hashval = 0; *s != '\0'; s++)
		hashval = (*s + 31 * hashval) % MAXSYM;
	return hashval;
}

Cell *lookup(s, tab)	/* look for s in tab */
register uchar *s;
Cell **tab;
{
	register Cell *p, *prev = NULL;
	int h;
	extern int indepth;

	h = hash(s);
	for (p = tab[h]; p != NULL; prev = p, p = p->cnext) {
		if (strcmp(s, p->nval) == 0) {
			if (prev != NULL && indepth == 0) {
				prev->cnext = p->cnext;
				p->cnext = tab[h];
				tab[h] = p;
			}
			return(p);	/* found it */
		}
	}
	return(NULL);	/* not found */
}

Awkfloat setfval(vp, f)
	register Cell *vp;
	Awkfloat f;
{
	extern Cell fldtab[];

	if (vp->tval & ARR)
		error(FATAL, "illegal reference to array %s", vp->nval);
	if ((vp->tval & (NUM | STR)) == 0)
		error(FATAL, "funny variable %o: n=%s s=\"%s\" f=%g t=%o",
			vp, vp->nval, vp->sval, vp->fval, vp->tval);
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf("setting field %d to %g\n", vp-fldtab, f);
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~STR;	/* mark string invalid */
	vp->tval |= NUM;	/* mark number ok */
	dprintf("setfval %o: %s = %g, t=%o\n", vp, vp->nval, f, vp->tval);
	return vp->fval = f;
}

uchar *setsval(vp, s)
register Cell *vp;
uchar *s;
{
	if (vp->tval & ARR)
		error(FATAL, "illegal reference to array %s", vp->nval);
	if ((vp->tval & (NUM | STR)) == 0)
		error(FATAL, "funny variable %o: n=%s s=\"%s\" f=%g t=%o",
			vp, vp->nval, vp->sval, vp->fval, vp->tval);
	if (vp->tval & FLD) {
		donerec = 0;	/* mark $0 invalid */
		if (vp-fldtab > *NF)
			newfld(vp-fldtab);
		dprintf("setting field %d to %s\n", vp-fldtab, s);
	} else if (vp->tval & REC) {
		donefld = 0;	/* mark $1... invalid */
		donerec = 1;
	}
	vp->tval &= ~NUM;
	vp->tval |= STR;
	if (!(vp->tval&DONTFREE))
		xfree(vp->sval);
	vp->tval &= ~DONTFREE;
	dprintf("setsval %o: %s = \"%s\", t=%o\n", vp, vp->nval, s, vp->tval);
	return(vp->sval = tostring(s));
}

Awkfloat r_getfval(vp)
register Cell *vp;
{
	if (vp->tval & ARR)
		error(FATAL, "illegal reference to array %s", vp->nval);
	if ((vp->tval & (NUM | STR)) == 0)
		error(FATAL, "funny variable %o: n=%s s=\"%s\" f=%g t=%o",
			vp, vp->nval, vp->sval, vp->fval, vp->tval);
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if (!isnum(vp)) {	/* not a number */
		vp->fval = atof(vp->sval);	/* best guess */
		if (isnumber(vp->sval) && !(vp->tval&CON))
			vp->tval |= NUM;	/* make NUM only sparingly */
	}
	dprintf("getfval %o: %s = %g, t=%o\n", vp, vp->nval, vp->fval, vp->tval);
	return(vp->fval);
}

uchar *r_getsval(vp)
register Cell *vp;
{
	uchar s[100];

	if (vp->tval & ARR)
		error(FATAL, "illegal reference to array %s", vp->nval);
	if ((vp->tval & (NUM | STR)) == 0)
		error(FATAL, "funny variable %o: n=%s s=\"%s\" f=%g t=%o",
			vp, vp->nval, vp->sval, vp->fval, vp->tval);
	if ((vp->tval & FLD) && donefld == 0)
		fldbld();
	else if ((vp->tval & REC) && donerec == 0)
		recbld();
	if ((vp->tval & STR) == 0) {
		if (!(vp->tval&DONTFREE))
			xfree(vp->sval);
		if ((long)vp->fval == vp->fval)
			sprintf(s, "%.20g", vp->fval);
		else
			sprintf(s, *OFMT, vp->fval);
		vp->sval = tostring(s);
		vp->tval &= ~DONTFREE;
		vp->tval |= STR;
	}
	dprintf("getsval %o: %s = \"%s\", t=%o\n", vp, vp->nval, vp->sval, vp->tval);
	return(vp->sval);
}

uchar *tostring(s)
register uchar *s;
{
	register uchar *p;

	p = Malloc(strlen(s)+1);
	if (p == NULL)
		error(FATAL, "out of space in tostring on %s", s);
	strcpy(p, s);
	return(p);
}

#ifdef MYALLOC

long	stamp;

uchar *Malloc(n)
	int n;
{
	uchar *p;

	p = (uchar *) malloc(n);
	fprintf(stderr, "%6d a %d %d\n", ++stamp, p, n);
	return p;
}

uchar *Calloc(n, sz)
	int n, sz;
{
	uchar *p;

	p = (uchar *) calloc(n, sz);
	fprintf(stderr, "%6d a %d %d (%d %d)\n", ++stamp, p, n*sz, n, sz);
	return p;
}

Free(p)
	uchar *p;
{
	fprintf(stderr, "%6d f %d\n", ++stamp, p);
	free(p);
}

#endif
