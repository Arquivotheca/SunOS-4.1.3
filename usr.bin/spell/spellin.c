/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)spellin.c 1.1 92/07/30 SMI"; /* from S5R3 1.3 */
#endif

#include <stdio.h>
#include "hash.h"

#define S (BYTE*sizeof(long))
#define B (BYTE*sizeof(unsigned))
unsigned *table;
int index[NI];
unsigned wp;		/* word pointer*/
int bp =B;	/* bit pointer*/
int ignore;
int extra;

/*	usage: hashin N
	where N is number of words in dictionary
	and standard input contains sorted, unique
	hashed words in octal
*/
main(argc,argv)
char **argv;
{
	long h,k,d;
	register i;
	long count;
	long w;
	long x;
	int t,u;
	extern float huff();
	double atof();
	double z;
	extern char *malloc();
	k = 0;
	u = 0;
	if(argc!=2) {
		fprintf(stderr,"spellin: arg count\n");
		exit(1);
	}
	table = (unsigned*)malloc(ND*sizeof(*table)); 
	if(table==0) {
		fprintf(stderr, "spellin: no space for table\n");
		exit(1);
	}
	z = huff((1L<<HASHWIDTH)/atof(argv[1]));
	fprintf(stderr, "spellin: expected code widths = %f\n", z);
	for(count=0; scanf("%lo", &h) == 1; ++count) {
		if((t=h>>(HASHWIDTH-INDEXWIDTH)) != u) {
			if(bp!=B)
				wp++;
			bp = B;
			while(u<t)
				index[++u] = wp;
			k =  (long)t<<(HASHWIDTH-INDEXWIDTH);
		}
		d = h-k;
		k = h;
		for(;;) {
			for(x=d;;x/=2) {
				i = encode(x,&w);
				if(i>0)
					break;
			}
			if(i>B) {
				if(!(
				   append((unsigned)(w>>(i-B)), B)&&
				   append((unsigned)(w<<(B+B-i)), i-B)))
					ignore++;
			} else
				if(!append((unsigned)(w<<(B-i)), i))
					ignore++;
			d -= x;
			if(d>0)
				extra++;
			else
				break;
		}
	}
	if(bp!=B)
		wp++;
	while(++u<NI)
		index[u] = wp;
	whuff();
	fwrite((char*)index, sizeof(*index), NI, stdout);
	fwrite((char*)table, sizeof(*table), wp, stdout);
	fprintf(stderr, "spellin: %ld items, %d ignored, %d extra, %u words occupied\n",
		count,ignore,extra,wp);
	count -= ignore;
	fprintf(stderr, "spellin: %f table bits/item, ", 
		((float)BYTE*wp)*sizeof(*table)/count);
	fprintf(stderr, "%f table+index bits\n",
		BYTE*((float)wp*sizeof(*table) + sizeof(index))/count);
	return(0);
}

append(w, i)
register unsigned w;
register i;
{
	while(wp<ND-1) {
		table[wp] |= w>>(B-bp);
		i -= bp;
		if(i<0) {
			bp = -i;
			return(1);
		}
		w <<= bp;
		bp = B;
		wp++;
	}
	return(0);
}
