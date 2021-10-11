/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)parse.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.3 */
#endif

#define DEBUG
#include <stdio.h>
#include "awk.h"
#include "y.tab.h"

Node *nodealloc(n)
{
	register Node *x;
	x = (Node *) Malloc(sizeof(Node) + (n-1)*sizeof(Node *));
	if (x == NULL)
		error(FATAL, "out of space in nodealloc");
	x->nnext = NULL;
	x->lineno = lineno;
	return(x);
}

Node *exptostat(a) Node *a;
{
	a->ntype = NSTAT;
	return(a);
}

Node *node0(a)
{
	register Node *x;
	x = nodealloc(0);
	x->nobj = a;
	return(x);
}

Node *node1(a,b) Node *b;
{
	register Node *x;
	x = nodealloc(1);
	x->nobj = a;
	x->narg[0]=b;
	return(x);
}

Node *node2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = nodealloc(2);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	return(x);
}

Node *node3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = nodealloc(3);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	return(x);
}

Node *node4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = nodealloc(4);
	x->nobj = a;
	x->narg[0] = b;
	x->narg[1] = c;
	x->narg[2] = d;
	x->narg[3] = e;
	return(x);
}

Node *stat3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NSTAT;
	return(x);
}

Node *op2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NEXPR;
	return(x);
}

Node *op1(a,b) Node *b;
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NEXPR;
	return(x);
}

Node *stat1(a,b) Node *b;
{
	register Node *x;
	x = node1(a,b);
	x->ntype = NSTAT;
	return(x);
}

Node *op3(a,b,c,d) Node *b, *c, *d;
{
	register Node *x;
	x = node3(a,b,c,d);
	x->ntype = NEXPR;
	return(x);
}

Node *op4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NEXPR;
	return(x);
}

Node *stat2(a,b,c) Node *b, *c;
{
	register Node *x;
	x = node2(a,b,c);
	x->ntype = NSTAT;
	return(x);
}

Node *stat4(a,b,c,d,e) Node *b, *c, *d, *e;
{
	register Node *x;
	x = node4(a,b,c,d,e);
	x->ntype = NSTAT;
	return(x);
}

Node *valtonode(a, b) Cell *a;
{
	register Node *x;

	a->ctype = OCELL;
	a->csub = b;
	x = node1(0, a);
	x->ntype = b == CFLD ? NFIELD : NVALUE;
	return(x);
}

Node *rectonode()
{
	/* return valtonode(lookup("$0", symtab), CFLD); */
	return valtonode(recloc, CFLD);
}

Node *makearr(p) Node *p;
{
	Cell *cp;

	if (isvalue(p)) {
		cp = (Cell *) (p->narg[0]);
		if (!isarr(cp)) {
			xfree(cp->sval);
			cp->sval = (uchar *) makesymtab();
			cp->tval = ARR;
		}
	}
	return p;
}

Node *pa2stat(a,b,c) Node *a, *b, *c;
{
	register Node *x;
	x = node4(PASTAT2, a, b, c, (Node *) paircnt);
	paircnt++;
	x->ntype = NSTAT;
	return(x);
}

Node *linkum(a,b) Node *a, *b;
{
	register Node *c;

	if (errorflag)	/* don't link things that are wrong */
		return a;
	if (a == NULL) return(b);
	else if (b == NULL) return(a);
	for (c = a; c->nnext != NULL; c = c->nnext)
		;
	c->nnext = b;
	return(a);
}

defn(v, vl, st)	/* turn on FCN bit in definition */
	Cell *v;
	Node *st, *vl;	/* body of function, arglist */
{
	Node *p;
	int n;

	v->tval = FCN;
	v->sval = (uchar *) st;
	n = 0;	/* count arguments */
	for (p = vl; p; p = p->nnext)
		n++;
	v->fval = n;
	dprintf("defining func %s (%d args)\n", v->nval, n);
}

isarg(s)	/* is s in argument list for current function? */
	uchar *s;
{
	extern Node *arglist;
	Node *p = arglist;
	int n;

	for (n = 0; p != 0; p = p->nnext, n++)
		if (strcmp(((Cell *)(p->narg[0]))->nval, s) == 0)
			return n;
	return -1;
}
