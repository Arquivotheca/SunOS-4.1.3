#ifndef lint
static  char sccsid[] = "@(#)tocsub.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * various toc subroutines
 *
 */

#include "mktp.h"

/*
 * newstring - make a new string with malloc
 */
char *
newstring(s)
char *s;
{
	extern char *malloc(), *strcpy();
	register char *r;

	if(!s)
		return(s);
	else
	{
		r = malloc((unsigned)strlen(s)+1);
		if(!r)
		{
			(void) fprintf(stderr,"malloc error in newstring\n");
			exit(1);
		}
		r = strcpy(r,s);
		return(r);
	}
}

/*
 * remove_toc_copies - remove all toc copies, and make a fresh one
 *		at the head of the table (just like it is when it
 *		leaves parse_input)
 *
 */

void
remove_toc_copies()
{
	void remove_entry(), Set_Size_of_Entry();
	toc_entry onetoc;
	register toc_entry *eptr, *ep1;
	char found = 0;

	/*
	 * 1) remove ALL toc entries
	 */

	for(eptr = entries; eptr < ep; eptr++)
		if(IS_TOC(eptr))
		{
			if(!found)
			{
				dup_entry(&onetoc,eptr);
				found = 1;
			}
			remove_entry(eptr);
		}

	/*
	 * 2) Make enough room at head of table for TOC entry
	 *
	 */

	for(eptr = ep, ep1 = ep-1; ep1 >= entries; eptr--, ep1--)
	{
		bzero((char *) eptr, sizeof (*eptr));
		*eptr = *ep1;
	}
	ep += 1;

        /*
         * 3) Make dummy entry for the xdr toc itself (entries[0])
         *
         */

	eptr = entries;
	bzero((char *) eptr, sizeof (*eptr));
	dup_entry(eptr,&onetoc);
	destroy_entry(&onetoc);
	Set_Size_of_Entry(eptr,(u_long) 4096);
	Set_New_Vol(eptr,0);
}

/*
 * remove_entry - destroy an entry, and shuffle table
 *	to cover hole
 */

void
remove_entry(ent)
toc_entry *ent;
{
	register toc_entry *ep1, *ep2;
	void destroy_entry();

	destroy_entry(ent);

	for(ep1 = ent, ep2 = ent+1; ep2 < ep; ep1++, ep2++)
	{
		*ep1 = *ep2;
		bzero((char *) ep2, sizeof (*ep2));
	}
	ep--;
	bzero((char *) ep, sizeof (*ep));
}

/*
 * destroy_entry - destroy an toc_entry structure
 *	use the XDR_FREE op of xdr to free up the
 *	malloc'ed strings in an xdr structure.
 *
 */

void
destroy_entry(eptr)
toc_entry *eptr;
{
	auto XDR x;
	register FILE *op = fopen("/dev/null","w");

	xdrstdio_create(&x,op,XDR_FREE);
	if(!xdr_toc_entry(&x,eptr))
	{
		(void) fprintf(stderr,
			"destroy_entry- unable to drop entry into garbage\n");
		exit(1);
	}
	xdr_destroy(&x);
	(void) fclose(op);
	bzero((char *)eptr,sizeof (*eptr));
}

void
destroy_all_entries()
{
	extern toc_entry entries[], *ep;
	auto XDR x;
	register FILE *op = fopen("/dev/null","w");
	register toc_entry *eptr;


	xdrstdio_create(&x,op,XDR_FREE);
	for(eptr = entries; eptr < ep; eptr++)
	{
		if(!xdr_toc_entry(&x,eptr))
		{
			(void) fprintf(stderr,
"destroy_all_entries- unable to drop entry into garbage\n");
			exit(1);
		}
	}
	xdr_destroy(&x);
	(void) fclose(op);
	bzero((char *)entries,NENTRIES * sizeof (entries[0]));
}

/*
 * dup_entry - duplicate an entire toc_entry structure
 *
 *	The idea is to use xdr to do our duplication for
 *	us. We create an xdr encoding location, encode to it,
 *	then turn around and decode from it into the new area.
 *	That way, xdr will handle memory allocation for us...
 */

void
dup_entry(to,from)
register toc_entry *to, *from;
{
	auto XDR x;
	char localbuf[16*1024]; /* I hope that this big enough... */

	bzero((char *)to,sizeof (*to));
	xdrmem_create(&x,localbuf,16*1024,XDR_ENCODE);
	if(!xdr_toc_entry(&x,from))
	{
		(void) fprintf(stderr,"dup_entry- unable send to temp area\n");
		exit(-1);
	}
	xdr_destroy(&x);
	xdrmem_create(&x,localbuf,16*1024,XDR_DECODE);
	if(!xdr_toc_entry(&x,to))
	{
		(void) fprintf(stderr,
			"dup_entry- unable retrieve from temp area\n");
		exit(-1);
	}
	xdr_destroy(&x);
}
