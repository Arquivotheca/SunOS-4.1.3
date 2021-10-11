#ifndef lint
static char sccsid[] = "@(#)mtest.c	1.1 (Sun) 7/30/92";
#endif

/*
 * a continuous test of random sequences of malloc(), free(), and realloc()
 */

#include <stdio.h>

#define LISTSIZE 256
#define NACTIONS 4
#define MAXSIZE	 1024 

#ifdef sparc
#define MALIGN	sizeof(double)
#else
#define MALIGN	sizeof(long)
#endif

/* format of data blocks */
#define LEN 0
#define CHK 1
#define DATA 2

typedef int* ptr;
typedef enum { Free, Malloc, Valloc, Realloc } actions;
typedef enum { Vacant, Occupied } slottype;

ptr	freelist[LISTSIZE];	/* items freed since last allocation call */
ptr	busylist[LISTSIZE];	/* allocated items */

extern long random();
extern getpagesize();
extern char *valloc(), *malloc(), *realloc();

actions act;

/*VARARGS1*/
error(s,x1,x2,x3,x4)
	char *s;
{
	(void)fprintf(stderr,s,x1,x2,x3,x4);
	(void)fflush(stderr);
	abort();
}

actions
nextaction()
{
	return (actions)(random() % NACTIONS);
}

flip()
{
	return (random() & 1);
}

clearfree()
{
	register int n;
	for (n = 0; n < LISTSIZE; n++)
		freelist[n] = NULL;
}

clearbusy()
{
	register int n, *p;
	for (n = 0; n < LISTSIZE; n++) {
		p = busylist[n];
		if (p != NULL) {
			free((char*)p);
			busylist[n] = NULL;
		}
	}
}

int
mksize(min, max)
{
	return ( random() % (max-min+1) ) + min;
}

ptr *
findslot(list,type)
	register ptr list[];
	register slottype type;
{
	register k,n;
	n = (random() % LISTSIZE);
	for ( k = (n+1) % LISTSIZE ; k != n ; k = (k+1) % LISTSIZE ) {
		if (type == Vacant) {
			/* need an empty slot */
			if (list[k] == NULL)
				return list + k;
		} else {
			/* need an occupied slot */
			if (list[k] != NULL)
				return list + k;
		}
	}
	return NULL;
}

filldata(list, n)
	register int *list;
	int n;
{
	register k;
	long x, chksum;

	chksum = 0;
	if (n > MAXSIZE) {
		error("filldata: invalid length (%d)\n", n);
	}
	for (k = DATA; k < n; k++) {
		x = random();
		list[k] = x;
		chksum += x;
	}
	list[LEN] = n;
	list[CHK] = chksum;
}

checkdata(list)
	register int list[];
{
	register k,n;
	long chksum;
	n = list[LEN];
	if (n > MAXSIZE) {
		error("checkdata: invalid length (%d)\n", n);
	}
	chksum = 0;
	for (k = DATA; k < n; k++) {
		chksum += list[k];
	}
	if (chksum != list[CHK]) {
		error("checkdata: invalid checksum\n");
	}
}

main(argc,argv)
	int argc;
	char *argv[];
{
	int i, n;
	int size;
	ptr *pp;
	ptr p;
	enum { Freelist, Busylist } which;
	int interval;
	int pagesize = getpagesize();

	if (argc != 3) {
		n = 4000;
		interval = 400;
		(void)fprintf(stderr, "mtest: n = %d interval = %d\n", n, interval);
	} else {
		n = atoi(argv[1]);
		interval = atoi(argv[2]);
	}

#ifdef	DEBUG
	malloc_debug(1);
#endif	DEBUG

	for(i = 0; i < n; i++) {
		
		if (i % interval == 0 ) {
			prfree();
			mallocmap();
		}

		switch (act = nextaction()) {

		case Free:
			/*
			 * Find a previously allocated data block,
			 * check its contents, and free it.
			 */
			pp = findslot(busylist,Occupied);
			if (pp != NULL) {
				p = *pp;
				checkdata(p);
				free((char*)p);
				*pp = NULL;
				pp = findslot(freelist, Vacant);
				if (pp == NULL) {
					clearfree();
					pp = findslot(freelist, Vacant);
				}
				*pp = p;
			}
			continue;

		case Valloc:
			/*
			 * Find a place to store a pointer to some data,
			 * allocate a random size block, initialize it,
			 * and stash it.
			 */
			pp = findslot(busylist, Vacant);
			if (pp != NULL) {
				size = mksize(DATA,MAXSIZE);
				p = (ptr)valloc((unsigned)size*sizeof(int));
				if ((int)p % pagesize) {
					error("valloc: misaligned (%#x)\n", p);
				}
				filldata(p,size);
				*pp = p;
			}
			clearfree();
			continue;

		case Malloc:
			/*
			 * Find a place to store a pointer to some data,
			 * allocate a random size block, initialize it,
			 * and stash it.
			 */
			pp = findslot(busylist, Vacant);
			if (pp == NULL) {
				clearbusy();
			} else {
				size = mksize(DATA,MAXSIZE);
				p = (ptr)malloc((unsigned)size*sizeof(int));
				if ((int)p % MALIGN) {
					error("malloc: misaligned (%#x)\n", p);
				}
				filldata(p,size);
				*pp = p;
			}
			clearfree();
			continue;

		case Realloc:
			/*
			 * similer to Malloc, but sometimes try reallocating
			 * an object freed since the last call to malloc()
			 * or realloc().
			 */
			if (flip()) {
				/* try reallocating something in the freelist */
				pp = findslot(freelist, Occupied);
				which = Freelist;
			} else {
				/* try reallocating something that's busy */
				pp = findslot(busylist, Occupied);
				which = Busylist;
			}
			if (pp != NULL) {
				p = *pp;
				checkdata(p);
				size = mksize(DATA,MAXSIZE);
				if (size < p[LEN]) {
					checkdata(p);	/* shrinking */
					filldata(p,size);
				}
				p = (ptr)realloc((char*)p,
					(unsigned)size*sizeof(int));
				checkdata(p);	/* check old data */
				if (which == Freelist) {
					*pp = NULL; /* no longer free */
					pp = findslot(busylist, Vacant);
					if (pp == NULL) {
						/*
						 * no empty slots in busy list;
						 * make one up.
						 */
						free((char*)busylist[0]);
						pp = &busylist[0];
					}
				}
				*pp = p;
			}
			clearfree();
			continue;

		} /*switch*/
	} /*for*/
}
