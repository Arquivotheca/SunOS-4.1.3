#ifndef lint
static  char sccsid[] = "@(#)life.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems Inc.
 */

#include <stdio.h>
#include "life.h"

#define MAXINT 0x7fffffff
#define MININT 0x80000000
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define STACK 2000		/* max number of points added at any one
				   step */

static int addx[STACK];
static int addy[STACK];
static int stack;

struct point {
    unsigned char p_color;
    int p_y;			/* x from row pointer */
    short p_delete;
    struct point *p_nxt;
};
struct row {
    int r_x;
    struct point *r_point;
    struct row *r_nxt;
} *row;

addpoint(x,y)
{
	struct row *rw, *oldrw, *newrw;

	if (row == NULL) {
		row = (struct row *)malloc(sizeof(struct row));
		row->r_x = x;
		row->r_point = NULL;
		addtorow(row, y);
		row->r_nxt = NULL;
		return;
	}			
	if (x < row->r_x) {
		rw = (struct row *)malloc(sizeof(struct row));
		rw->r_x = x;
		rw->r_point = NULL;
		addtorow(rw, y);
		rw->r_nxt = row;
		row = rw;
		return;
	}
	for(rw = row, oldrw = row; rw != NULL; oldrw = rw, rw = rw->r_nxt) {
		if (rw->r_x == x) {
			addtorow(rw, y);
			return;
		}
		if (x < rw->r_x)
			break;
	}
	newrw = (struct row *)malloc(sizeof(struct row));
	newrw->r_x = x;
	newrw->r_point = NULL;
	addtorow(newrw, y);
	newrw->r_nxt = rw;
	oldrw->r_nxt = newrw;
}

addtorow(rw, y)
	struct row *rw;
{
	struct point *pt, *oldpt, *newpt;
	
	if (rw->r_point == NULL) {
		pt = (struct point *)malloc(sizeof(struct point));
		pt->p_color = INITCOLOR;
		pt->p_y = y;
		pt->p_delete = 0;
		pt->p_nxt = NULL;
		rw->r_point = pt;
		return;
	}			
	if (y < rw->r_point->p_y) {
		pt = (struct point *)malloc(sizeof(struct point));
		pt->p_color = INITCOLOR;
		pt->p_y = y;
		pt->p_delete = 0;
		pt->p_nxt = rw->r_point;
		rw->r_point = pt;
		return;
	}
	for(pt = rw->r_point, oldpt = pt; pt != NULL;
	    oldpt = pt, pt = pt->p_nxt) {
		if (pt->p_y == y)
			return;
		if (y < pt->p_y)
			break;
	}
	newpt = (struct point *)malloc(sizeof(struct point));
	newpt->p_color = INITCOLOR;
	newpt->p_y = y;
	newpt->p_delete = 0;
	newpt->p_nxt = pt;
	oldpt->p_nxt = newpt;
}

deletepoint(x,y)
{
	struct row *rw, *oldrw;
	struct point *pt, *oldpt;
	
	for (rw = row, oldrw = row; rw != NULL; oldrw = rw, rw = rw->r_nxt) {
		if (rw->r_x == x)
			break;
	}
	if (rw == NULL) {
		fprintf(stderr, "tried to delete nonexistent point\n");
		exit(1);
	}
	for (pt = rw->r_point, oldpt = pt; pt != NULL;
	    oldpt = pt, pt = pt->p_nxt) {
		if (pt->p_y == y)
			break;
	}
	if (pt == NULL) {
		fprintf(stderr, "tried to delete nonexistent point\n");
		exit(1);
	}
	if (oldpt == pt) {
		rw->r_point = pt->p_nxt;
		if (rw->r_point == NULL) {
			deleterow(oldrw, rw);			
		}
	}
	else
		oldpt->p_nxt = pt->p_nxt;
	free(pt);
}

deleterow(oldrw, rw)
    struct row *oldrw, *rw;
{
	if (oldrw == rw) {
		if (rw->r_nxt == NULL)
			row = NULL;
		else
			row = rw->r_nxt;
	}
	else
		oldrw->r_nxt = rw->r_nxt;
	free(rw);
}

/* 
 * for debugging
 */
printall()
{
	struct row *rw;
	struct point *pt;
	
	for (rw = row; rw != NULL; rw = rw->r_nxt)
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
			printf("%d %d (%d)\n", rw->r_x, pt->p_y, pt->p_delete);
}

printrw()
{
	struct row *rw;
	
	for (rw = row; rw != NULL; rw = rw->r_nxt)
		printf("%d 0x%x\n", rw->r_x, rw->r_point);
}

printpt(rw)
	struct row *rw;
{
	struct point *pt;
	
	for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
		printf("%d %d (%d)\n", rw->r_x, pt->p_y, pt->p_delete);
}

zerolist()
{
	struct row *rw;
	struct point *pt;

	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
			free(pt);
		free(rw);
	}
	row = NULL;
}

refreshboard()
{
	struct row *rw;
	struct point *pt;
	int i,j;
	
	for(i = 0; i < rightedge; i++)
	for(j = 0; j < bottomedge; j++)
		board[i][j] = 0;
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
			i = rw->r_x + leftoffset;
			j = pt->p_y + upoffset;
			if (i >= 0 && i < rightedge && j >= 0
			    && j < bottomedge)
				board[i][j] = pt->p_color;
		}
	}
}

/* 
 * called from lifetool
 */
newgen()
{
	struct row *oldrw, *rw;
	struct point *pt, *oldpt;
	int i,j;

	for (rw = row, oldrw = rw; rw != NULL;
	    oldrw = rw, rw = rw->r_nxt) {
		for (pt = rw->r_point, oldpt = pt; pt != NULL;
		    oldpt = pt, pt = pt->p_nxt)
			pt->p_delete = nbors(oldrw, rw, oldpt, pt);
	}

	newstuff();
	lock();
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt){
			if (pt->p_delete >=4 || pt->p_delete <=1) {
				/* 
				 * should delete pt directly
				 */
				i = rw->r_x;
				j = pt->p_y;
				deletepoint(i,j);
				i+= leftoffset;
				j += upoffset;
				if (i >= 0 && i < rightedge && j >= 0
				    && j < bottomedge && board[i][j]) {
					erase_stone(i, j);
					board[i][j] = 0;
				    }
			}
		}
	}

	if (color) {
		for (rw = row; rw != NULL; rw = rw->r_nxt)
			for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt)
				if (pt->p_color < CMSIZE-2)
					pt->p_color++;
	}

	while(stack > 0){
		stack--;
		i = addx[stack];
		j = addy[stack];
		addpoint(i, j);
		i+= leftoffset;
		j += upoffset;
		if (i >= 0 && i < rightedge && j >= 0
		    && j < bottomedge && board[i][j] == 0) {
			paint_stone(i, j, INITCOLOR);
			board[i][j] = 1;
		}
	}

	if (color) {
		for (rw = row; rw != NULL; rw = rw->r_nxt) {
			for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
				i = rw->r_x + leftoffset;
				j = pt->p_y + upoffset;
				if (i >= 0 && i < rightedge && j >= 0
				    && j<bottomedge && pt->p_color > INITCOLOR)
					paint_stone(i, j, pt->p_color);
			}
		}
	}

	unlock();
}

nbors(oldrw, rw, oldpt, pt)
	struct row *rw, *oldrw;
	struct point *pt, *oldpt;
{
	struct point *tmppt;
	int x, y;
	int cnt = 0;

	x = rw->r_x;
	y = pt->p_y;
	if (oldpt != pt && oldpt->p_y == y-1)
		cnt++;
	if (pt->p_nxt != NULL && pt->p_nxt->p_y == y+1)
		cnt++;
	if (rw != oldrw && oldrw->r_x == x-1) {
		for(tmppt = oldrw->r_point; tmppt != NULL;
		    tmppt = tmppt->p_nxt) {
			if (tmppt->p_y == y-1)
				cnt++;
			if (tmppt->p_y == y)
				cnt++;
			if (tmppt->p_y == y+1) {
				cnt++;
				break;
			}
			if (tmppt->p_y >= y+1)
				break;
		}
	}
	if (rw->r_nxt != NULL && rw->r_nxt->r_x == x+1) {
		for(tmppt = rw->r_nxt->r_point; tmppt != NULL;
		    tmppt = tmppt->p_nxt) {
			if (tmppt->p_y == y-1)
				cnt++;
			if (tmppt->p_y == y)
				cnt++;
			if (tmppt->p_y == y+1) {
				cnt++;
				break;
			}
			if (tmppt->p_y >= y+1)
				break;
		}
	}
	return cnt;
}

struct xy {
	int x,y;
	short cnt;
	struct xy *nxt;		/* next in this bucket */
	struct xy *chain;	/* for chaining nonzero entries together */
};

#define HASHSIZE 32

struct xy *candidates;
struct xy *hash[HASHSIZE][HASHSIZE];

#define addxy(X,Y) \
	xx = (X) % HASHSIZE; \
	yy = (Y) % HASHSIZE; \
	p = hash[xx][yy]; \
	for (;;) { \
		if (p == NULL) { \
			p = (struct xy *)newmalloc(); \
			p->x = (X); \
			p->y = (Y); \
			p->cnt = 1; \
			p->nxt = hash[xx][yy]; \
			hash[xx][yy] = p; \
			break; \
		} \
		if (p->x == (X) && p->y == (Y)) { \
			p->cnt++; \
			if (p->cnt == 3) { \
				p->chain = candidates; \
				candidates = p; \
			} \
			break; \
		} \
		p = p->nxt; \
	}

newstuff()
{
	register struct row *rw;
	register struct point *pt;
	register struct xy *p;
	register unsigned int x, y;
	register unsigned int xx,yy;
	static int firsttime = 1;

	if (firsttime) {
		firsttime = 0;
		initchain();
	}
	candidates = NULL;
	bzero(hash, HASHSIZE * HASHSIZE * sizeof(struct xy *));
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		x = rw->r_x;
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
			y = pt->p_y;
			addxy(x-1, y-1);
			addxy(x-1, y);
			addxy(x-1, y+1);
			addxy(x, y-1);
			addxy(x, y+1);
			addxy(x+1, y-1);
			addxy(x+1, y);
			addxy(x+1, y+1);
		}
	}
	findbirths();
}

findbirths()
{
	register struct xy *p;
	
	for (p = candidates; p != NULL; p = p->chain) {
		if (p->cnt == 3)
			addtostack(p->x, p->y);
	}
	restorespace();
}

addtostack(i, j)
{
	addx[stack] = i;
	addy[stack++] = j;
	if (stack >= STACK) {
		fprintf(stderr, "overflowed stack\n");
		exit(1);
	}
}

printhashdata()
{
	int x, y;
	struct xy *p;
	
	for(x = 0; x < HASHSIZE; x++)
	for(y = 0; y < HASHSIZE; y++)
	for (p = hash[x][y]; p != NULL; p = p->nxt) {
		printf("%d %d (%d)\n", p->x, p->y, p->cnt);
	}
}

struct chain {
	struct xy ch_xy;
	struct chain *ch_nxt;
} *chain, *headchain, *endchain;

initchain()
{
	headchain = (struct chain *)malloc(sizeof(struct chain));
	headchain->ch_nxt = NULL;
	endchain = headchain;
	chain = headchain;
}

restorespace()
{
	chain = headchain;
}

newmalloc()
{
	struct chain *ch;
	
	chain = chain->ch_nxt;
	if (chain == NULL) {
		ch = (struct chain *)malloc(sizeof(struct chain));
		ch->ch_nxt = NULL;
		endchain->ch_nxt = ch;
		endchain = ch;
		chain = ch;
	}
	return (int)(chain);
}

box(leftp, rightp, upp, downp)
	int *leftp, *rightp, *upp, *downp;
{
	register struct row *rw;
	register struct point *pt;
	register int maxy, miny, maxx, minx;
	
	maxx = maxy = MININT;
	minx = miny = MAXINT;
	for (rw = row; rw != NULL; rw = rw->r_nxt) {
		if (rw->r_x < minx)
			minx = rw->r_x;
		if (rw->r_x > maxx)
			maxx = rw->r_x;
		for (pt = rw->r_point; pt != NULL; pt = pt->p_nxt) {
			if (pt->p_y < miny)
				miny = pt->p_y;
			if (pt->p_y > maxy)
				maxy = pt->p_y;
		}
	}
	*leftp = minx;
	*rightp = maxx;
	*upp = miny;
	*downp = maxy;
}
