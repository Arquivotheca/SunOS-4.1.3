/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)huff.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

#include <stdio.h>
#ifdef FIXED
#include <math.h>
#endif
#define BYTE 8
#define QW 1		/* width of bas-q digit in bits */

/* this stuff should be local and hidden; it was made
 * accessible outside for dirty reasons: 20% faster spell
 */
#include "huff.h"
struct huff huffcode;

/* Infinite Huffman code
 *
 * Let the messages be exponentially distributed with ratio r:
 * 	P{message k} = r^k*(1-r),	k=0,1,...
 * Let the messages be coded in base q, and suppose
 * 	r^n = 1/q
 * If each decade (base q) contains n codes, then
 * the messages assigned to each decade will be q times
 * as probable as the next. Moreover the code for the tail of
 * the distribution after truncating one decade should look
 * just like the original, but longer by one leading digit q-1.
 * 	q(z+n) = z + (q-1)q^w
 * where z is first code of decade, w is width of code (in shortest
 * full decade). Examples, base 2:
 * 	r^1 = 1/2	r^5 = 1/2
 * 	0		0110
 * 	10		0111
 * 	110		1000
 * 	1110		1001
 * 	...		1010
 * 			10110
 * 	w=1,z=0		w=4,z=0110
 * Rewriting slightly
 * 	(q-1)z + q*n = (q-1)q^w
 * whence z is a multiple of q and n is a multiple of q-1. Let
 * 	z = cq, n = d(q-1)
 * We pick w to be the least integer such that
 * 	d = n/(q-1) <= q^(w-1)
 * Then solve for c
 * 	c = q^(w-1) - d
 * If c is not zero, the first decade may be preceded by
 * even shorter (w-1)-digit codes 0,1,...,c-1. Thus
 * the example code with r^5 = 1/2 becomes
 * 	000
 * 	001
 * 	010
 * 	0110
 * 	0111
 * 	1000
 * 	1001
 * 	1010
 * 	10110
 * 	...
 * 	110110
 * 	...
 * The expected number of base-q digits in a codeword is then
 *	w - 1 + r^c/(1-r^n)
 * The present routines require q to be a power of 2
 */
/* There is a lot of hanky-panky with left justification against
 * sign instead of simple left justification because
 * unsigned long is not available
 */
#define L (BYTE*(sizeof(long))-1)	/* length of signless long */
#define MASK (~(1L<<L))		/* mask out sign */

/* decode the prefix of word y (which is left justified against sign)
 * place mesage number into place pointed to by kp
 * return length (in bits) of decoded prefix or 0 if code is out of
 * range
 */
decode(y,pk)
long y;
long *pk;
{
	register l;
	long v;
	if(y < cs) {
		*pk = y >> (L+QW-w);
		return(w-QW);
	}
	for(l=w,v=v0; y>=qcs; y=(y<<QW)&MASK,v+=n)
		if((l+=QW) > L)
			return(0);
	*pk = v + (y>>(L-w));
	return(l);
}

/* encode message k and put result (right justified) into
 * place pointed to by py.
 * return length (in bits) of result,
 * or 0 if code is too long
 */
encode(k,py)
long k;
long *py;
{
	register l;
	long y;
	if(k < c) {
		*py = k;
		return(w-QW);
	}
	for(k-=c,y=1,l=w; k>=n; k-=n,y<<=QW)
		if((l+=QW) > L)
			return(0);
	*py = ((y-1)<<w) + cq + k;
	return(l);
}
/* Initialization code, given expected value of k
 * E(k) = r/(1-r) = a
 * and given base width b
 * return expected length of coded messages
 */
struct qlog {
	long p;
	double u;
};
double
huff(a)
float a;
{
	struct qlog qlog();
	struct qlog z;
	register i, q;
	long d, j;
	double r = a/(1.0 + a);
	double rc, rq;
	for(i=0,q=1,rq=r; i<QW; i++,q*=2,rq*=rq)
		continue;
	rq /= r;	/* rq = r^(q-1) */
	z = qlog(rq, 1./q, 1L, rq);
	d = z.p;
	n = d*(q-1);
	if(n!=d*(q-1))
		abort();	/*time to make n long*/
	for(w=QW,j=1; j<d; w+=QW,j*=q)
		continue;
	c = j - d;
	cq = c*q;
	cs = cq<<(L-w);
	qcs = (((long)(q-1)<<w) + cq) << (L-QW-w);
	v0 = c - cq;
#ifndef FIXED
	for(i=0,rc=1; i<c; i++,rc*=r)	/* rc = r^c */
		continue;
#else
	rc = pow(r, (double)c);		/* DAG -- bug fix (see Sep-84 SUN) */
#endif
	return(w + QW*(rc/(1-z.u) - 1));
}

struct qlog
qlog(x, y, p, u)	/*find smallest p so x^p<=y */
double x,y,u;
long p;
{
	struct qlog z;
	if(u/x <= y) {
		z.p = 0;
		z.u = 1;
	} else {
		z = qlog(x, y, p+p, u*u);
		if(u*z.u/x > y) {
			z.p += p;
			z.u *= u;
		}
	}
	return(z);
}

whuff()
{
	fwrite((char*)&huffcode,sizeof(huffcode),1,stdout);
}

rhuff(f)
FILE *f;
{
	return(read(fileno(f), (char*)&huffcode, sizeof(huffcode)) == sizeof(huffcode));
}
