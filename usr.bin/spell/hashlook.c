/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)hashlook.c 1.1 92/07/30 SMI"; /* from S5R3 1.7 */
#endif

#include <stdio.h>
#include "hash.h"
#include "huff.h"

unsigned *table;
int index[NI];

#define B (BYTE*sizeof(unsigned))
#define L (BYTE*sizeof(long)-1)
#define MASK (~(1L<<L))

#ifdef pdp11	/*sizeof(unsigned)==sizeof(long)/2 */
#define fetch(wp,bp)\
	(((((long)wp[0]<<B)|wp[1])<<(B-bp))|(wp[2]>>bp))
#else 		/*sizeof(unsigned)==sizeof(long)*/
#define fetch(wp,bp) ((wp[0]<<(B-bp))|(wp[1]>>bp))
#endif

hashlook(s)
char *s;
{
	long h;
	long t;
	register bp;
	register unsigned *wp;
	int i;
	long sum;
	unsigned *tp;

	h = hash(s);
	t = h>>(HASHWIDTH-INDEXWIDTH);
	wp = &table[index[t]];
	tp = &table[index[t+1]];
	bp = B;
	sum = (long)t<<(HASHWIDTH-INDEXWIDTH);
	for(;;) {
		{/*	this block is equivalent to
			 bp -= decode((fetch(wp,bp)>>1)&MASK, &t);*/
			long y;
			long v;
#if u3b || u3b5	|| u3b2 || sparc || i386
			/*
			 * shift 32 on those machines leaves destination
			 * unchanged
			 */
			if (bp == 0)
				y = 0;
			else
				y = wp[0] << (B - bp);
			if (bp < 32)
				y |= (wp[1] >> bp);
			y = (y >> 1) & MASK;
#else
			y = (fetch(wp,bp)>>1) & MASK;
#endif
			if(y < cs) {
				t = y >> (L+1-w);
				bp -= w-1;
			}
			else {
				for(bp-=w,v=v0; y>=qcs; y=(y<<1)&MASK,v+=n)
					bp -= 1;
				t = v + (y>>(L-w));
			}
		}
		while(bp<=0) {
			bp += B;
			wp++;
		}
		if(wp>=tp&&(wp>tp||bp<B))
			return(0);
		sum += t;
		if(sum<h)
			continue;
		return(sum==h);
	}
}


prime(argc,argv)
char **argv;
{
	register FILE *f;
	register fd;
	extern char *malloc();
	if(argc <= 1)
		return(0);
#ifdef pdp11	/* because of insufficient address space for buffers*/
	fd = dup(0);
	close(0);
	if(open(argv[1], 0) != 0)
		return(0);
	f = stdin;
	if(rhuff(f)==0
	|| read(fileno(f), (char *)index, NI*sizeof(*index)) != NI*sizeof(*index)
	|| (table = (unsigned*)malloc(index[NI-1]*sizeof(*table))) == 0
	|| read(fileno(f), (char*)table, sizeof(*table)*index[NI-1])
	   != index[NI-1]*sizeof(*table))
		return(0);
	close(0);
	if(dup(fd) != 0)
		return(0);
	close(fd);
#else
	if((f = fopen(argv[1], "ri")) == NULL)
		return(0);
	if(rhuff(f)==0
	|| fread((char*)index, sizeof(*index),  NI, f) != NI
	|| (table = (unsigned*)malloc(index[NI-1]*sizeof(*table))) == 0
	|| fread((char*)table, sizeof(*table), index[NI-1], f)
	   != index[NI-1])
		return(0);
	fclose(f);
#endif
	hashinit();
	return(1);
}
