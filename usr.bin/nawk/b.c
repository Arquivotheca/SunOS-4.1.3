/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)b.c 1.1 92/07/30 SMI"; /* from S5R3.1 2.5 */
#endif

#define	DEBUG

#include "awk.h"
#include <ctype.h>
#include <stdio.h>
#include "y.tab.h"

#define	HAT	(NCHARS-1)	/* matches ^ in regular expr */
				/* NCHARS is 2**n */

extern Node *op2();
#define MAXLIN 256

#define type(v)	v->nobj
#define left(v)	v->narg[0]
#define right(v)	v->narg[1]
#define parent(v)	v->nnext

#define LEAF	case CCL: case NCCL: case CHAR: case DOT: case FINAL: case ALL:
#define UNARY	case STAR: case PLUS: case QUEST:

/* encoding in tree Nodes:
	leaf (CCL, NCCL, CHAR, DOT, FINAL, ALL): left is index, right contains value or pointer to value
	unary (STAR, PLUS, QUEST): left is child, right is null
	binary (CAT, OR): left and right are children
	parent contains pointer to parent
*/


uchar	chars[MAXLIN];
int	setvec[MAXLIN];
int	tmpset[MAXLIN];
Node	*point[MAXLIN];

int	rtok;		/* next token in current re */
int	rlxval;
uchar	*prestr;	/* current position in current re */
uchar	*lastre;	/* origin of last re */

static	int setcnt;
static	int poscnt;

uchar	*patbeg;
int	patlen;

fa *makedfa(p, anchor)	/* returns dfa for tree pointed to by p */
	Node *p;	/* anchor = 1 for anchored matches, else 0 */
	int anchor;
{
	Node *p1;
	fa *f;

	p1 = op2(CAT, op2(STAR, op2(ALL, (Node *) 0, (Node *) 0), (Node *) 0), p);
		/* put ALL STAR in front of reg.  exp. */
	p1 = op2(CAT, p1, op2(FINAL, (Node *) 0, (Node *) 0));
		/* put FINAL after reg.  exp. */

	poscnt = 0;
	penter(p1);	/* enter parent pointers and leaf indices */
	if ((f = (fa *) Calloc(1, sizeof(fa) + poscnt*sizeof(rrow))) == NULL)
		overflo("no room for fa");
	f->accept = poscnt-1;	/* penter has computed number of positions in re */
	cfoll(f, p1);	/* set up follow sets */
	freetr(p1);
/*
	{
		int i;
		int j;
		printf("retab %o:\n", f->re);
		for (i=0; i<=f->accept; i++) {
			printf("%d: type: %d\tlval: %d\tlfollow:",i,f->re[i].ltype,f->re[i].lval);
			for (j = 0; j <= (f->re[i].lfollow)[0]; j++)
				printf(" %d", f->re[i].lfollow[j]);
			printf("\n");
		}
	}
*/
	if ((f->posns[0] = (int *) Calloc(1, *(f->re[0].lfollow)*sizeof(int))) == NULL)
			overflo("out of space in makedfa");
	if ((f->posns[1] = (int *) Calloc(1, sizeof(int))) == NULL)
		overflo("out of space in makedfa");
	*f->posns[1] = 0;
	f->initstat = makeinit(f, anchor);
	f->reset = 0;
	return f;
}

int makeinit(f, anchor)
	fa *f;
	int anchor;
{
	register i, k;

	f->curstat = 2;
	f->out[2] = 0;
	k = *(f->re[0].lfollow);
/*	printf("k = %d\n",k);	*/
	xfree(f->posns[2]);			
	if ((f->posns[2] = (int *) Calloc(1, (k+1)*sizeof(int))) == NULL)
		overflo("out of space in makeinit");
	for (i=0; i<=k; i++) {
		(f->posns[2])[i] = (f->re[0].lfollow)[i];
/*		printf("f->posns[2][%d] = %d\n",i, (f->posns[2])[i]);	*/
	}
	if ((f->posns[2])[1] == f->accept)
		f->out[2] = 1;
	for (i=0; i<NCHARS; i++)
		f->gototab[2][i] = 0;
	f->curstat = cgoto(f, 2, HAT);
	if (anchor) {
		*f->posns[2] = k-1;	/* leave out position 0 */
/*		printf("anchor: k = %d\n",k);	*/			
		for (i=0; i<k; i++) {
			(f->posns[0])[i] = (f->posns[2])[i];
/*			printf("f->posns[0][%d] = %d\n", i, (f->posns[0])[i]);	*/
		}

		f->out[0] = f->out[2];
		if (f->curstat != 2)
			--(*f->posns[f->curstat]);
	}
	return f->curstat;
}

penter(p)	/* set up parent pointers and leaf indices */
	Node *p;
{
	switch(type(p)) {
	LEAF
		left(p) = (Node *) poscnt;
		point[poscnt++] = p;
		break;
	UNARY
		penter(left(p));
		parent(left(p)) = p;
		break;
	case CAT:
	case OR:
		penter(left(p));
		penter(right(p));
		parent(left(p)) = p;
		parent(right(p)) = p;
		break;
	default:
		error(FATAL, "unknown type %d in penter\n", type(p));
		break;
	}
}

freetr(p)	/* free parse tree */
	Node *p;
{
	switch(type(p)) {
	LEAF
		xfree(p);
		break;
	UNARY
		freetr(left(p));
		xfree(p);
		break;
	case CAT:
	case OR:
		freetr(left(p));
		freetr(right(p));
		xfree(p);
		break;
	default:
		error(FATAL, "unknown type %d in freetr", type(p));
		break;
	}
}

uchar *cclenter(p)
	register uchar *p;
{
	register int i, c;
	uchar *op;

	op = p;
	i = 0;
	while ((c = *p++) != 0) {
		if (c == '-' && i > 0 && chars[i-1] != 0) {
			if (*p != 0) {
				c = chars[i-1];
				while (c < *p) {
					if (i >= MAXLIN)
						overflo("character class too big");
					chars[i++] = ++c;
				}
				p++;
				continue;
			}
		}
		if (i >= MAXLIN)
			overflo("character class too big");
		chars[i++] = c;
	}
	chars[i++] = '\0';
	dprintf("cclenter: in = |%s|, out = |%s|\n", op, chars, NULL);
	xfree(op);
	return(tostring(chars));
}

overflo(s)
	uchar *s;
{
	error(FATAL, "regular expression too big: %s", s);
}

cfoll(f, v)	/* enter follow set of each leaf of vertex v into lfollow[leaf] */
	fa *f;
	register Node *v;
{
	register int i;
	register int *p;

	switch(type(v)) {
	LEAF
		f->re[(int) left(v)].ltype = type(v);
		f->re[(int) left(v)].lval = (int) right(v);
		for (i=0; i<=f->accept; i++)
			setvec[i] = 0;
		setcnt = 0;
		follow(v);	/* computes setvec and setcnt */
		if ((p = (int *) Calloc(1, (setcnt+1)*sizeof(int))) == NULL)
			overflo("follow set overflow");
		f->re[(int) left(v)].lfollow = p;
		*p = setcnt;
		for (i = f->accept; i >= 0; i--)
			if (setvec[i] == 1) *++p = i;
		break;
	UNARY
		cfoll(f,left(v));
		break;
	case CAT:
	case OR:
		cfoll(f,left(v));
		cfoll(f,right(v));
		break;
	default:
		error(FATAL, "unknown type %d in cfoll", type(v));
	}
}

first(p)		/* collects initially active leaves of p into setvec */
	register Node *p;	/* returns 0 or 1 depending on whether p matches empty string */
{
	register int b;

	switch(type(p)) {
	LEAF
		if (setvec[(int) left(p)] != 1) {
			setvec[(int) left(p)] = 1;
			setcnt++;
		}
		if (type(p) == CCL && (*(uchar *) right(p)) == '\0')
			return(0);		/* empty CCL */
		else return(1);
	case PLUS:
		if (first(left(p)) == 0) return(0);
		return(1);
	case STAR:
	case QUEST:
		first(left(p));
		return(0);
	case CAT:
		if (first(left(p)) == 0 && first(right(p)) == 0) return(0);
		return(1);
	case OR:
		b = first(right(p));
		if (first(left(p)) == 0 || b == 0) return(0);
		return(1);
	}
	error(FATAL, "unknown type %d in first\n", type(p));
	return(-1);
}

follow(v)
	Node *v;		/* collects leaves that can follow v into setvec */
{
	Node *p;

	if (type(v) == FINAL)
		return;
	p = parent(v);
	switch (type(p)) {
	case STAR:
	case PLUS:
		first(v);
		follow(p);
		return;

	case OR:
	case QUEST:
		follow(p);
		return;

	case CAT:
		if (v == left(p)) {	/* v is left child of p */
			if (first(right(p)) == 0) {
				follow(p);
				return;
			}
		}
		else		/* v is right child */
			follow(p);
		return;
	}
}

member(c, s)	/* is c in s? */
	register uchar c, *s;
{
	while (*s)
		if (c == *s++)
			return(1);
	return(0);
}


match(f, p)
	register fa *f;
	register uchar *p;
{
	register int s, ns;

	s = f->reset?makeinit(f,0):f->initstat;
	if (f->out[s])
		return(1);
	do {
/*
		int i, k;
		printf("match: p = %o, *p = %c\n", p, *p);
		printf("state %d: ", s);
		k = *f->posns[s];
		for (i=0; i<=k; i++)
			printf(" %d", (f->posns[s])[i]);
		printf("	out = %d\n", f->out[s]);
		printf("	g[%d][%c] = ", s, *p);
*/
		if (ns=f->gototab[s][*p])
			s=ns;
		else
			s=cgoto(f,s,*p);
/*
		printf("%d\n", s);	
*/
		if (f->out[s])
			return(1);
	} while (*p++ != 0);
	return(0);
}

pmatch(f, p)
	register fa *f;
	register uchar *p;
{
	register s, ns;
	register uchar *q;
	int i, k;

	s = f->reset?makeinit(f,1):f->initstat;
	patbeg = p;
	patlen = -1;
	do {
		q = p;
		do {
/*
			printf("pmatch: p = %o, *p = %c, q = %o, *q = %c\n", p, *p, q, *q);
			printf("state %d: ", s);
			k = *f->posns[s];
			for (i=0; i<=k; i++)
				printf(" %d", (f->posns[s])[i]);
			printf("	out = %d\n", f->out[s]);
*/
			if (f->out[s])		/* final state */
				patlen = q-p;
/*			printf("	g[%d][%c] = ", s, *q);	*/
			if (ns=f->gototab[s][*q])
				s=ns;
			else
				s=cgoto(f,s,*q);
/*			printf("%d\n", s);	*/
			if (s==1)	/* no transition */
				if (patlen >= 0) {
					patbeg = p;
					return(1);
				}
				else
					goto nextin;	/* no match */
		} while (*q++ != 0);
		if (f->out[s])
			patlen	= q-p;
		if (patlen >=0 ) {
			patbeg = p;
			return(1);
		}
	nextin:
		s = 2;
		if (f->reset) {
			for (i=2; i<=f->curstat; i++)
				Free(f->posns[i]);
			k = *f->posns[0];			
			if ((f->posns[2] = (int *) Calloc(1, (k+1)*sizeof(int))) == NULL)
				overflo("out of space in pmatch");
			for (i=0; i<=k; i++)
				(f->posns[2])[i] = (f->posns[0])[i];
			f->initstat = f->curstat = 2;
			f->out[2] = f->out[0];
			for (i=0; i<NCHARS; i++)
				f->gototab[2][i] = 0;
		}
	} while (*p++ != 0);
	return (0);
}

nematch(f, p)
	register fa *f;
	register uchar *p;
{
	register int s, ns;
	register uchar *q;
	int i, k;

	s = f->reset?makeinit(f,1):f->initstat;
	patlen = -1;
	while (*p) {
		q = p;
		do {
			if (f->out[s])		/* final state */
				patlen = q-p;
			if (ns=f->gototab[s][*q])
				s=ns;
			else
				s=cgoto(f,s,*q);
			if (s==1)	/* no transition */
				if (patlen > 0) {
					patbeg = p;
					return(1);
				}
				else
					goto nnextin;	/* no nonempty match */
		} while (*q++ != 0);
		if (f->out[s])
			patlen	= q-p;
		if (patlen >0 ) {
			patbeg = p;
			return(1);
		}
	nnextin:
		s = 2;
		if (f->reset) {
			for (i=2; i<=f->curstat; i++)
				Free(f->posns[i]);
			k = *f->posns[0];			
			if ((f->posns[2] = (int *) Calloc(1, (k+1)*sizeof(int))) == NULL)
				overflo("out of state space");
			for (i=0; i<=k; i++)
				(f->posns[2])[i] = (f->posns[0])[i];
			f->initstat = f->curstat = 2;
			f->out[2] = f->out[0];
			for (i=0; i<NCHARS; i++)
				f->gototab[2][i] = 0;
		}
	p++;
	}
	return (0);
}

Node *regexp(), *primary(), *concat(), *alt(), *unary();

Node *reparse(p)
	uchar *p;
{
	/* parses regular expression pointed to by p */
	/* uses relex() to scan regular expression */
	Node *np;

	dprintf("reparse <%s>\n", p);
	lastre = prestr = p;	/* prestr points to string to be parsed */
	rtok = relex();
	if (rtok == '\0')
		error(FATAL, "empty regular expression");
	np = regexp();
	if (rtok == '\0')
		return(np);
	else
		error(FATAL, "syntax error in regular expression %s at %s", lastre, prestr);
}

Node *regexp()
{
	return (alt(concat(primary())));
}

Node *primary()
{
	Node *np;

	switch (rtok) {
	case CHAR:
		np = op2(CHAR, (Node *) 0, rlxval);
		rtok = relex();
		return (unary(np));
	case ALL:
		rtok = relex();
		return (unary(op2(ALL, (Node *) 0, (Node *) 0)));
	case DOT:
		rtok = relex();
		return (unary(op2(DOT, (Node *) 0, (Node *) 0)));
	case CCL:
		np = op2(CCL, (Node *) 0, cclenter(rlxval));
		rtok = relex();
		return (unary(np));
	case NCCL:
		np = op2(NCCL, (Node *) 0, cclenter(rlxval));
		rtok = relex();
		return (unary(np));
	case '^':
		rtok = relex();
		return (unary(op2(CHAR, (Node *) 0, HAT)));
	case '$':
		rtok = relex();
		return (unary(op2(CHAR, (Node *) 0, (Node *) 0)));
	case '(':
		rtok = relex();
		if (rtok == ')') {	/* special pleading for () */
			rtok = relex();
			return unary(op2(CCL, (Node *) 0, tostring("")));
		}
		np = regexp();
		if (rtok == ')') {
			rtok = relex();
			return (unary(np));
		}
		else
			error(FATAL, "syntax error in regular expression %s at %s", lastre, prestr);
	default:
		error(FATAL, "illegal primary in regular expression %s at %s", lastre, prestr);
	}
}

Node *concat(np)
	Node *np;
{
	switch (rtok) {
	case CHAR: case DOT: case ALL: case CCL: case NCCL: case '$': case '(':
		return (concat(op2(CAT, np, primary())));
	default:
		return (np);
	}
}

Node *alt(np)
	Node *np;
{
	if (rtok == OR) {
		rtok = relex();
		return (alt(op2(OR, np, concat(primary()))));
	}
	return (np);
}

Node *unary(np)
	Node *np;
{
	switch (rtok) {
	case STAR:
		rtok = relex();
		return (unary(op2(STAR, np, (Node *) 0)));
	case PLUS:
		rtok = relex();
		return (unary(op2(PLUS, np, (Node *) 0)));
	case QUEST:
		rtok = relex();
		return (unary(op2(QUEST, np, (Node *) 0)));
	default:
		return (np);
	}
}

relex()		/* lexical analyzer for reparse */
{
	extern int rlxval;
	register int c;
	uchar cbuf[150];
	int clen, cflag;

	switch (c = *prestr++) {
	case '|': return OR;
	case '*': return STAR;
	case '+': return PLUS;
	case '?': return QUEST;
	case '.': return DOT;
	case '\0': prestr--; return '\0';
	case '^':
	case '$':
	case '(':
	case ')':
		return c;
	case '\\':
		if ((c = *prestr++) == 't')
			c = '\t';
		else if (c == 'n')
			c = '\n';
		else if (c == 'f')
			c = '\f';
		else if (c == 'r')
			c = '\r';
		else if (c == 'b')
			c = '\b';
		else if (c == '\\')
			c = '\\';
		else if (isdigit(c)) {
			int n = c - '0';
			if (isdigit(*prestr)) {
				n = 8 * n + *prestr++ - '0';
				if (isdigit(*prestr))
					n = 8 * n + *prestr++ - '0';
			}
			c = n;
		} /* else it's now in c */
		rlxval = c;
		return CHAR;
	default:
		rlxval = c;
		return CHAR;
	case '[': 
		clen = 0;
		if (*prestr == '^') {
			cflag = 1;
			prestr++;
		}
		else
			cflag = 0;
		for (;;) {
			if ((c = *prestr++) == '\\') {
				if ((c = *prestr++) == 't')
					cbuf[clen++] = '\t';
				else if (c == 'n')
					cbuf[clen++] = '\n';
				else if (c == 'f')
					cbuf[clen++] = '\f';
				else if (c == 'r')
					cbuf[clen++] = '\r';
				else if (c == 'b')
					cbuf[clen++] = '\b';
				else if (c == '\\')
					cbuf[clen++] = '\\';
				else if (isdigit(c)) {
					int n = c - '0';
					if (isdigit(*prestr)) {
						n = 8 * n + *prestr++ - '0';
						if (isdigit(*prestr))
							n = 8 * n + *prestr++ - '0';
					}
					cbuf[clen++] = n;
				} else
					cbuf[clen++] = c;
			} else if (c == ']') {
				cbuf[clen] = 0;
				rlxval = (int) tostring(cbuf);
				if (cflag == 0)
					return CCL;
				else
					return NCCL;
			} else if (c == '\n') {
				error(FATAL, "newline in character class %s...", lastre);
			} else if (c == '\0') {
				error(FATAL, "nonterminated character class %s", lastre);
			} else
				cbuf[clen++] = c;
		}
	}
}

int cgoto(f, s, c)
	fa *f;
	int s, c;
{
	register int i, j, k;
	register int *p, *q;

	for (i=0; i<=f->accept; i++)
		setvec[i] = 0;
	setcnt = 0;
	/* compute positions of gototab[s,c] into setvec */
	p = f->posns[s];
	for (i=1; i<=*p; i++) {
/*		printf("cgoto: state = %d, p[%d] = %d\n",s, i, p[i]);	*/
		if ((k = f->re[p[i]].ltype) != FINAL) {
			if (k == CHAR && c == f->re[p[i]].lval
				|| k == DOT && c != 0 && c != HAT
				|| k == ALL && c != 0
				|| k == CCL && member(c, (uchar *) f->re[p[i]].lval)
				|| k == NCCL && !member(c, (uchar *) f->re[p[i]].lval) && c != 0 && c != HAT) {
					q = f->re[p[i]].lfollow;
					for (j=1; j<=*q; j++) {
						if (setvec[q[j]] == 0) {
							setcnt++;
							setvec[q[j]] = 1;
						}
					}
				}
		}
	}
	/* determine if setvec is a previous state */
	tmpset[0] = setcnt;
	j = 1;
/*	printf("posns: %d", setcnt );	*/
	for (i = f->accept; i >= 0; i--)
		if (setvec[i]) {
/*			printf(" %d", i);	*/
			tmpset[j++] = i;
		}
/*	printf("\n");	*/
	/* tmpset == previous state? */
	for (i=1; i<= f->curstat; i++) {
		p = f->posns[i];
		if ((k = tmpset[0]) != p[0])
			goto different;
		for (j = 1; j <= k; j++)
			if (tmpset[j] != p[j])
				goto different;
		/* setvec is state i */
		f->gototab[s][c] = i;
/*		printf("old: g[%d][%c] = %d\n", s, c, i);	*/
		return i;
	different:;
	}

	/* add tmpset to current set of states */
	if (f->curstat >= NSTATES-1) {
		f->curstat = 2;
		f->reset = 1;
		for (i=2; i<NSTATES; i++)
			Free(f->posns[i]);
	}
	else
		++(f->curstat);
	for (i=0; i<NCHARS; i++)
		f->gototab[f->curstat][i] = 0;
/*	printf("setcnt = %d\n", setcnt);	*/
	if ((p = (int *) Calloc(1, (setcnt+1)*sizeof(int))) == NULL)
		overflo("out of space in cgoto");
/*	printf("f->curstat = %d\n", f->curstat);	*/

	f->posns[f->curstat] = p;
	f->gototab[s][c] = f->curstat;
	for (i = 0; i <= setcnt; i++)
		p[i] = tmpset[i];
/*	printf("new: g[%d, %c] = %d\n", s, c, f->curstat);	*/
	if (setvec[f->accept])
		f->out[f->curstat] = 1;
	else
		f->out[f->curstat] = 0;
	return f->curstat;
}


freefa(f)
	struct fa *f;
{

	register int i;

	for (i=0; i<=f->curstat; i++)
		Free(f->posns[i]);
	for (i=0; i<=f->accept; i++)
		Free(f->re[i].lfollow);
	Free(f);
}

