/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)hashcheck.c 1.1 92/07/30 SMI"; /* from S5R3 1.2 */
#endif

#include <stdio.h>
#include "hash.h"
long fetch();
int index[NI];
unsigned *table;
unsigned wp;
int bp;
#define U (BYTE*sizeof(unsigned))
#define L (BYTE*sizeof(long))

main()
{
	int i;
	long v;
	long a;
	extern char *malloc();
	rhuff(stdin);
	fread((char*)index, sizeof(*index), NI, stdin);
	table = (unsigned*)malloc(index[NI-1]*sizeof(*table));
	fread((char*)table, sizeof(*table), index[NI-1], stdin);
	for(i=0;i<NI-1;i++) {
		bp = U;
		v = (long)i<<(HASHWIDTH-INDEXWIDTH);
		for(wp=index[i];wp<index[i+1]; ) {
			if(wp==index[i]&&bp==U)
				a = fetch();
			else {
				a = fetch();
				if(a==0)
					break;
			}
			if(wp>index[i+1]||
				wp==index[i+1]&&bp<U)
				break;
			v += a;
			printf("%.9lo\n",v);
		}
	}
}

long fetch()
{
	long w;
	long y = 0;
	int empty = L;
	int i = bp;
	int tp = wp;
	while(empty>=i) {
		empty -= i;
		i = U;
		y |= (long)table[tp++] << empty;
	}
	if(empty>0)
		y |= table[tp]>>i-empty;
	i = decode((y>>1)&((1L<<(BYTE*sizeof(y)-1))-1), &w);
	bp -= i;
	while(bp<=0) {
		bp += U;
		wp++;
	}
	return(w);
}
