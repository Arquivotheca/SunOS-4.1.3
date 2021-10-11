#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)draw.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <rpcsvc/ether.h>
#include "traffic.h"
#define MAXINT 0x7fffffff
#define MININT 0x80000000
#define NSITES 1000

int debug;
/* 
 *  global variables
 */

static struct etherhmem 	*(*dsthtablep)[MAXSPLIT][HASHSIZE];
static struct etherhmem 	*(*dsthtable1p)[MAXSPLIT][HASHSIZE];
static struct etherhmem 	*(*dstdeltahtablep)[MAXSPLIT][HASHSIZE];
static struct etherhmem 	*(*srchtablep)[MAXSPLIT][HASHSIZE];
static struct etherhmem 	*(*srchtable1p)[MAXSPLIT][HASHSIZE];
static struct etherhmem 	*(*srcdeltahtablep)[MAXSPLIT][HASHSIZE];

#define dsthtable (*dsthtablep)
#define dsthtable1 (*dsthtable1p)
#define dstdeltahtable (*dstdeltahtablep)
#define srchtable (*srchtablep)
#define srchtable1 (*srchtable1p)
#define srcdeltahtable (*srcdeltahtablep)

static unsigned long	bucket[MAXSPLIT][NBUCKETS/2];
static unsigned long	bucket1[MAXSPLIT][NBUCKETS/2];
static int		prevbar[MAXSPLIT][MAXBARS];
static int		prevbar1[MAXSPLIT][MAXBARS];
static unsigned long	prototable[MAXSPLIT][NPROTOS];
static unsigned long	prototable1[MAXSPLIT][NPROTOS];

static int		deltacnt[MAXSPLIT];
static struct timeval	deltatime[MAXSPLIT];
static int		first[MAXSPLIT];
static int		lasttm[MAXSPLIT]; /* last time bar1 updated */
static int		oldbcast[MAXSPLIT];
static int		pos[MAXSPLIT];
static int		savepktcnt[MAXSPLIT];
static int		savepktcnt1[MAXSPLIT];
static struct timeval	savetime[MAXSPLIT];
static struct timeval	savetime1[MAXSPLIT];
static struct rank	rank[NSITES], rank1[NSITES];
static struct etherstat etherstat;
static struct etheraddrs ea;

/* 
 *  procedure variables
 */
int traf_compar();
struct timeval timersub();

/* 
 *  pixrects
 */
static short traf_gray80_data[] = {
#include <images/square_80.pr>
};
mpr_static(traf_gray80_patch, 15, 15, 1, traf_gray80_data);

static short traf_gray25_data[] = {
#include <images/square_25.pr>
};
mpr_static(traf_gray25_patch, 16, 16, 1, traf_gray25_data);

struct pixrect	*proof_pr1 = &traf_gray80_patch;
struct pixrect	*proof_pr = &traf_gray25_patch;

/* malloc space for arrays */
trf_allocate()
{
	dsthtablep = (struct etherhmem * (*)[MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));
	dsthtable1p = (struct etherhmem * (*)[MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));
	dstdeltahtablep = (struct etherhmem * (*)[MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));

	srchtablep = (struct etherhmem * (*) [MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));
	srchtable1p = (struct etherhmem * (*) [MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));
	srcdeltahtablep = (struct etherhmem * (*) [MAXSPLIT][HASHSIZE])
	    malloc(MAXSPLIT*HASHSIZE*sizeof(char *));
}

draw(k)
{
	int i, j;
	struct etherstat *ep;
	struct timeval tm;
	
	if (!running)
		return;
	ep = &etherstat;
	switch (mode[k]) {
		case DISPLAY_LOAD: {
			int  wd, ht, diff, left, go, step;
			static struct timeval oldtime[MAXSPLIT];
			static int oldht[MAXSPLIT];
			static unsigned oldbytes[MAXSPLIT];
		
			bzero(&etherstat, sizeof(etherstat));
			if (mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS,
			    ETHERSTATPROC_GETDATA,xdr_void, 0,
			    xdr_etherstat, &etherstat))
				return;
			diff = 1000*(ep->e_time.tv_sec - oldtime[k].tv_sec);
			diff += (ep->e_time.tv_usec - oldtime[k].tv_usec)/1000;
			if (diff == 0)
				break;
			ht = (8*(ep->e_bytes - oldbytes[k]))/(10*diff);
			if (ht > 999)
				ht = 999;
			/* add 1 so won't rest on bottom of graph */
			ht  = TOPGAP + curht[k] - (ht*curht[k])/1000 - 1;
			wd = tswrect[k].r_width;
			if (first[k]) {
				go = 0;
				first[k] = 0;
			}
			else {
				go = 1;
			}
			step = (int)panel_get_value(speed_item[k]);
			if (pos[k] + step < wd) {
				if (go)
					pw_vector(pixwin[k], pos[k], oldht[k],
					    pos[k] + step, ht, PIX_SRC, 1);
			}
			else if (pos[k] < wd) {
				pw_vector(pixwin[k], pos[k], oldht[k],
				    wd-1, ht, PIX_SRC, 1);
			}
			else {
				left = LEFTMARGIN*marginfontwd;
				lock(k);
				pw_copy(pixwin[k], left, TOPGAP, wd,
				    curht[k] + 1, PIX_SRC, pixwin[k],
				    /*curht[k] + debug, PIX_SRC, pixwin[k], */
				    left + step, TOPGAP);
				pw_writebackground(pixwin[k], wd-step, TOPGAP,
				    step, curht[k]+1, PIX_CLR);
				pw_vector(pixwin[k], wd-1-step, oldht[k], wd-1,
				    ht, PIX_SRC, 1);
				if (gridon[k])
					drawgridstub(k, step);
				unlock(k);
			}
			pos[k] += step;
			oldht[k] = ht;
			oldtime[k] = ep->e_time;
			oldbytes[k] = ep->e_bytes;
			break;
		}
		case DISPLAY_SIZE:
			bzero(&etherstat, sizeof(etherstat));
			if (mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS,
			    ETHERSTATPROC_GETDATA,xdr_void, 0,
			    xdr_etherstat, &etherstat))
				return;
			for (i = 0; i < NBUCKETS/2; i++)
				bucket[k][i] = (ep->e_size[2*i]
				    + ep->e_size[2*i+1]) - bucket[k][i];
			gettimeofday(&tm, 0);
 			if (tm.tv_sec > lasttm[k] + timeout1[k]) {
				for (i = 0; i < NBUCKETS/2; i++)
					bucket1[k][i] =
					    ep->e_size[2*i] + ep->e_size[2*i+1]
					    - bucket1[k][i];
				lasttm[k] = tm.tv_sec;
				drawbars1(etherstat.e_packets-savepktcnt1[k],
				    timersub(etherstat.e_time, savetime1[k]),
				    k, bucket1[k], sizeof(bucket1[k][0]),
				    NBUCKETS/2);
				savepktcnt1[k] = etherstat.e_packets;
				savetime1[k] = etherstat.e_time;
				for (i = 0; i < NBUCKETS/2; i++)
					bucket1[k][i] = ep->e_size[2*i] + 
					    ep->e_size[2*i+1];
			}
			drawbars(etherstat.e_packets-savepktcnt[k],
			    timersub(etherstat.e_time, savetime[k]),
			    k, bucket[k], sizeof(bucket[k][0]),
			    NBUCKETS/2);
			for (i = 0; i < NBUCKETS/2; i++)
				bucket[k][i] = ep->e_size[2*i]
				    + ep->e_size[2*i+1];
			savepktcnt[k] = etherstat.e_packets;
			savetime[k] = etherstat.e_time;
			break;
		case DISPLAY_PROTO:
			bzero(&etherstat, sizeof(etherstat));
			if (mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS,
			    ETHERSTATPROC_GETDATA,xdr_void, 0,
			    xdr_etherstat, &etherstat))
				return;
			for (i = 0; i < NPROTOS; i++)
				prototable[k][i] =
				    ep->e_proto[i] - prototable[k][i];
			gettimeofday(&tm, 0);
			if (tm.tv_sec > lasttm[k] + timeout1[k]) {
				for (i = 0; i < NPROTOS; i++)
					prototable1[k][i] =
					    ep->e_proto[i] - prototable1[k][i];
				lasttm[k] = tm.tv_sec;
				drawbars1(etherstat.e_packets - savepktcnt1[k],
				    timersub(etherstat.e_time, savetime1[k]),
				    k, prototable1[k],
				    sizeof(prototable1[k][0]), NPROTOS);
				savetime1[k] = etherstat.e_time;
				savepktcnt1[k] = etherstat.e_packets;
				for (i = 0; i < NPROTOS; i++)
					prototable1[k][i] = ep->e_proto[i];
			}
			drawbars(etherstat.e_packets-savepktcnt[k],
			    timersub(etherstat.e_time, savetime[k]), k,
			    prototable[k], sizeof(prototable[k][0]), NPROTOS);
			for (i = 0; i < NPROTOS; i++)
				prototable[k][i] = ep->e_proto[i];
			savepktcnt[k] = etherstat.e_packets;
			savetime[k] = etherstat.e_time;
			break;
		case DISPLAY_SRC:
			bzero(&ea, sizeof(ea));
			if (mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS,
			    ETHERSTATPROC_GETSRCDATA,xdr_void, 0,
			    xdr_etheraddrs, &ea))
				return;
			j = loadrank(rank1, srchtable[k]);
			qsort(rank1, j, sizeof(rank1[0]), traf_compar);
			drawnames(k, rank, rank1);
			drawbars(ea.e_packets - savepktcnt[k],
			    timersub(ea.e_time, savetime[k]),
			    k, rank,  sizeof(rank[0]), NUMNAMES);
			gettimeofday(&tm, 0);
			if (tm.tv_sec > lasttm[k] + timeout1[k]) {
				lasttm[k] = tm.tv_sec;
				loadrank1(rank, srchtable1[k],
				    srcdeltahtable[k]);
				drawbars1(ea.e_packets - savepktcnt1[k],
				    timersub(ea.e_time, savetime1[k]),
				    k, rank, sizeof(rank[0]), NUMNAMES);
				deltacnt[k] = ea.e_packets - savepktcnt1[k];
				deltatime[k] =
				    timersub(ea.e_time, savetime1[k]);
				savepktcnt1[k] = ea.e_packets;
				savetime1[k] = ea.e_time;
				updatetable(srchtable1[k]);
			}
			else {
				loadfromdelta(rank, srcdeltahtable[k]);
				drawbars1(deltacnt[k], deltatime[k],
				    k, rank, sizeof(rank[0]), NUMNAMES);
			}
			savepktcnt[k] = ea.e_packets;
			savetime[k] = ea.e_time;
			updatetable(srchtable[k]);
			freeargs(xdr_etheraddrs, &ea);
			break;
		case DISPLAY_DST:
			bzero(&ea, sizeof(ea));
			if (mycallrpc(host, ETHERSTATPROG, ETHERSTATVERS,
			    ETHERSTATPROC_GETDSTDATA,xdr_void, 0,
			    xdr_etheraddrs, &ea))
				return;
			j = loadrank(rank1, dsthtable[k]);
			rank1[j].addr = 0;
			rank1[j].cnt = ea.e_bcast - oldbcast[k];
			j++;
			oldbcast[k] = ea.e_bcast;
			qsort(rank1, j, sizeof(rank1[0]), traf_compar);
			drawnames(k, rank, rank1);
			drawbars(ea.e_packets - savepktcnt[k],
			    timersub(ea.e_time, savetime[k]),
			    k, rank, sizeof(rank[0]), NUMNAMES);
			gettimeofday(&tm, 0);
			if (tm.tv_sec > lasttm[k] + timeout1[k]) {
				lasttm[k] = tm.tv_sec;
				loadrank1(rank, dsthtable1[k],
				    dstdeltahtable[k]);
				drawbars1(ea.e_packets - savepktcnt1[k],
				    timersub(ea.e_time, savetime1[k]),
				    k, rank, sizeof(rank[0]), NUMNAMES);
				deltacnt[k] = ea.e_packets - savepktcnt1[k];
				deltatime[k] =
				    timersub(ea.e_time, savetime1[k]);
				savetime1[k] = ea.e_time;
				savepktcnt1[k] = ea.e_packets;
				updatetable(dsthtable1[k]);
			}
			else {
				loadfromdelta(rank, dstdeltahtable[k]);
				drawbars1(deltacnt[k], deltatime[k],
				    k, rank, sizeof(rank[0]), NUMNAMES);
			}
			savepktcnt[k] = ea.e_packets;
			savetime[k] = ea.e_time;
			updatetable(dsthtable[k]);
			freeargs(xdr_etheraddrs, &ea);
			break;
	}
}

/* 
 * divide graph into cnt equal parts, and imagine a dotted line
 * drawn to divide the parts.  Then BETWEEN is space between
 * dotted line and leftmost bar, OVERLAP is space between leftbar
 * and right one, right edge of right bar is flush with dotted line.
 */
#define BETWEEN 15
#define OVERLAP 10

/* 
 * draws leftmost bar: assumes right bar has value
 * given by prevbar1
 */
drawbars(pkts, tm, k, p, offset, cnt)
	struct timeval tm;
	char p[];
{
	int ht, wd, i, j, j1, z, w, left;
	double time;
	
	left = LEFTMARGIN*marginfontwd;
	wd = (tswrect[k].r_width - left)/cnt;
	nopkts(pkts == 0);
	time = tm.tv_sec + tm.tv_usec/1000000.0;
	lock(k);
    again:
	for (i = 0; i < cnt; i++) {
		if (traf_absolute[k]) {
			ht = (*(unsigned *)&p[offset*i])/time;
			if (ht > maxabs[k]) {
				maxabs[k] = ((ht+INITMAXABS-1)/INITMAXABS)
				    *INITMAXABS;
				drawleftmargin(k);
				goto again;
			}
			ht = ht*curht[k]/maxabs[k];
		}
		else if (pkts == 0)
			ht = 0;
		else
			ht = (*(unsigned *)&p[offset*i]*curht[k])/pkts;
		j = prevbar[k][i];
		/* 
		 * left hand part of left bar
		 */
		if (j > ht)
			pw_replrop(pixwin[k], left+i*wd + BETWEEN,
			    TOPGAP + curht[k] - j,
			    OVERLAP, j - ht, PIX_SRC ^ PIX_DST,
			    proof_pr, left + i*wd + BETWEEN, curht[k] - j);
		else if (j < ht)
			pw_replrop(pixwin[k], left+i*wd + BETWEEN,
			    TOPGAP + curht[k] - ht,
			    OVERLAP, ht - j, PIX_SRC ^ PIX_DST,
			    proof_pr, left + i*wd + BETWEEN, curht[k] - ht);
		/* 
		 * overlap of 2 bars
		 */
		j1 = prevbar1[k][i];
		if (j >= j1) {
			if (j > ht)
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - j,
				    wd - 2*OVERLAP - BETWEEN, j-ht,
				    PIX_SRC ^ PIX_DST, proof_pr,
				    left + i*wd + BETWEEN+OVERLAP, curht[k]-j);
			else if (j < ht)
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - ht,
				    wd-2*OVERLAP - BETWEEN, ht-j,
				    PIX_SRC ^ PIX_DST, proof_pr,
				    left + i*wd+BETWEEN+OVERLAP, curht[k]-ht);
			if (ht < j1) {
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - j1,
				    wd - 2*OVERLAP-BETWEEN, j1-ht,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left + i*wd + BETWEEN+OVERLAP,curht[k]-j1);
			}
		}
		else {
			if (ht < j) {
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - j,
				    wd - 2*OVERLAP - BETWEEN, j-ht,
				    PIX_SRC ^ PIX_DST, proof_pr,
				    left + i*wd + BETWEEN+OVERLAP, curht[k]-j);
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - j,
				    wd - 2*OVERLAP - BETWEEN, j-ht,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left + i*wd + BETWEEN+OVERLAP, curht[k]-j);
			}
			else {
				z = min(ht - j, j1 - j);
				w = min(ht, j1);
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - w,
				    wd - 2*OVERLAP - BETWEEN, z,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left + i*wd + BETWEEN+OVERLAP, curht[k]-w);
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - ht,
				    wd-2*OVERLAP - BETWEEN, ht-j,
				    PIX_SRC ^ PIX_DST, proof_pr,
				    left + i*wd+BETWEEN+OVERLAP, curht[k]-ht);
			}
		}
		prevbar[k][i] = ht;
	}
	unlock(k);
}

/* 
 * draws rightmost bar: assumes left bar has value
 * given by prevbar
 */
drawbars1(pkts, tm, k, p, offset, cnt)
	struct timeval tm;
	char p[];
{
	int wd, i, j, j1, ht1, z, left;
	float time;
	
	left = LEFTMARGIN*marginfontwd;
	wd = (tswrect[k].r_width - left)/cnt;
	time = tm.tv_sec + tm.tv_usec/1000000.0;
	lock(k);
   again:
	for (i = 0; i < cnt; i++) {
		if (traf_absolute[k]) {
			ht1 = (*(unsigned *)&p[offset*i])/time;
			if (ht1 > maxabs[k]) {
				maxabs[k] = ((ht1+INITMAXABS-1)/INITMAXABS)
				    *INITMAXABS;
				drawleftmargin(k);
				goto again;
			}
			ht1 = ht1*curht[k]/maxabs[k];
		}
		else if (pkts == 0)
			ht1 = 0;
		else
			ht1 = (*(unsigned *)&p[offset*i]*curht[k])/pkts;
		j = prevbar[k][i];
		j1 = prevbar1[k][i];
		/* 
		 * overlap of 2 bars
		 */
		if (j >= j1) {
			if (ht1 > j) {
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - ht1,
				    wd-2*OVERLAP-BETWEEN, ht1-j,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left+i*wd + BETWEEN+OVERLAP, curht[k]-ht1);
			}
		}
		else {
			if (ht1 > j1) {
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - ht1,
				    wd-2*OVERLAP-BETWEEN, ht1-j1,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left+i*wd + BETWEEN+OVERLAP, curht[k]-ht1);
			}
			else if (ht1 < j1) {
				z = min(j1 - ht1, j1 - j);
				pw_replrop(pixwin[k],left+i*wd+BETWEEN+OVERLAP,
				    TOPGAP + curht[k] - j1,
				    wd - 2*OVERLAP - BETWEEN, z,
				    PIX_SRC ^ PIX_DST, proof_pr1,
				    left+i*wd + BETWEEN+OVERLAP, curht[k]-j1);
			}
		}
		/* 
		 * right hand part of right bar
		 */
		if (j1 > ht1)
			pw_replrop(pixwin[k], left + (i+1)*wd - OVERLAP,
			    TOPGAP + curht[k] - j1,
			    OVERLAP, j1 - ht1, PIX_SRC^PIX_DST,
			    proof_pr1, left + (i+1)*wd-OVERLAP, curht[k] - j1);
		else if (j1 < ht1)
			pw_replrop(pixwin[k], left+(i+1)*wd-OVERLAP,
			    TOPGAP + curht[k] - ht1,
			    OVERLAP, ht1 - j1, PIX_SRC ^ PIX_DST,
			    proof_pr1, left + (i+1)*wd-OVERLAP, curht[k]-ht1);
		prevbar1[k][i] = ht1;
	}
	unlock(k);
}

/* 
 * load rank[] with values of ea.e_addrs - table
 * so that rank[] can be sorted
 */
loadrank(rk, table)
	struct rank rk[];
	struct etherhmem *table[];
{	
	int i, j, x;
	struct etherhmem *p, *q;
	
	j = 0;
	for (i = 0; i < HASHSIZE; i++) {
		for (p = ea.e_addrs[i]; p != NULL; p = p->ht_nxt) {
			x = p->ht_addr & 0xff;
			for (q = table[x]; q != NULL; q = q->ht_nxt) {
				if (q->ht_addr == p->ht_addr) {
					rk[j].cnt = p->ht_cnt - q->ht_cnt;
					rk[j].addr = p->ht_addr;
					goto nextround;
				}
			}
			/* first time seen */
			rk[j].cnt = p->ht_cnt;
			rk[j].addr = p->ht_addr;
		    nextround:;
			j++;
			if (j >= NSITES) {
				fprintf(stderr, "overflowed rank table\n");
				return (j-1);
			}
		}
	}
	return (j);
}

loadfromdelta(rk, table)
	struct rank rk[];
	struct etherhmem *table[];
{
	int i, x;
	struct etherhmem *p;

	for (i = 0; i < NUMNAMES; i++) {
		x = rk[i].addr;
		for (p = table[x & 0xff]; p != NULL; p = p->ht_nxt) {
			if (p->ht_addr == x) {
				rk[i].cnt = p->ht_cnt;
				break;
			}
		}
		if (p == NULL)
			rk[i].cnt = 0;
	}
}

/* 
 * load rank[].cnt with values of ea.e_addrs - table
 * but use only first NUMNAMES entries of rank[]
 * Also, update delta table for all entries
 */
loadrank1(rk, table, deltatable)
	struct rank rk[];
	struct etherhmem *table[];
	struct etherhmem *deltatable[];
{	
	int i, x;
	struct etherhmem *p, *q;
	
	for (i = 0; i < HASHSIZE; i++) {
		for (p = ea.e_addrs[i]; p != NULL; p = p->ht_nxt) {
			x = p->ht_addr;
			for (q = table[x & 0xff]; q != NULL; q = q->ht_nxt) {
				if (q->ht_addr == x) {
					addtohtable(p->ht_cnt - q->ht_cnt, x,
					    deltatable);
					break;
				}
			}
			if (q == NULL)	/* first time seen */
				addtohtable(p->ht_cnt, x, deltatable);
		}
	}
	for (i = 0; i < NUMNAMES; i++) {
		x = rk[i].addr;
		for (p = deltatable[x & 0xff]; p != NULL; p = p->ht_nxt) {
			if (p->ht_addr == x)
				rk[i].cnt = p->ht_cnt;
		}
	}
}

addtohtable(cnt, addr, table)
	struct etherhmem *table[];
{
	int x;
	struct etherhmem *q;

	x = addr & 0xff;
	for (q = table[x]; q != NULL; q = q->ht_nxt) {
		if (q->ht_addr == addr) {
			q->ht_cnt = cnt;
			return;
		}
	}
	q = (struct etherhmem *)malloc(sizeof(struct etherhmem));
	q->ht_addr = addr;
	q->ht_cnt = cnt;
	q->ht_nxt = table[x];
	table[x] = q;
}

updatetable(table)
	struct etherhmem *table[];
{	
	int i;
	struct etherhmem *p;
	
	for (i = 0; i < HASHSIZE; i++) {
		for (p = ea.e_addrs[i]; p != NULL; p = p->ht_nxt)
			addtohtable(p->ht_cnt, p->ht_addr, table);
	}
}

traf_clearstate(i)
{
	deltacnt[i] = 0;
	deltatime[i].tv_sec = 0;
	deltatime[i].tv_usec = 0;
	first[i] = 1;
	lasttm[i] = 0;
	oldbcast[i] = 0;
	maxabs[i] = INITMAXABS;
	pos[i] = LEFTMARGIN*marginfontwd;
	savepktcnt[i] = 0;
	savepktcnt1[i] = 0;
	savetime[i].tv_sec = 0;
	savetime[i].tv_usec = 0;
	savetime1[i].tv_sec = 0;
	savetime1[i].tv_usec = 0;

	bzero(bucket[i], sizeof(bucket[i]));
	bzero(bucket1[i], sizeof(bucket1[i]));
	bzero(dsthtable[i], sizeof(dsthtable[i]));
	bzero(dsthtable1[i], sizeof(dsthtable1[i]));
	bzero(prevbar[i], sizeof(prevbar[0]));
	bzero(prevbar1[i], sizeof(prevbar1[0]));
	bzero(prototable[i], sizeof(prototable[i]));
	bzero(prototable1[i], sizeof(prototable1[i]));
	bzero(srchtable[i], sizeof(srchtable[i]));
	bzero(srcdeltahtable[i], sizeof(srcdeltahtable[i]));
	bzero(srchtable1[i], sizeof(srchtable1[i]));
	zerooldaddrs(i);
}

/* 
 *  t1 - t2
 */
struct timeval
timersub(t1, t2)
	struct timeval t1, t2;
{
	if (t1.tv_usec < t2.tv_usec) {
		t1.tv_sec--;
		t1.tv_usec += 1000000;
	}
	t1.tv_sec -= t2.tv_sec;
	t1.tv_usec -= t2.tv_usec;
	return (t1);
}
