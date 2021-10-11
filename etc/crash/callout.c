#ifndef lint
static	char sccsid[] = "@(#)callout.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:callout.c	1.8"
/*
 * This file contains code for the crash function callout.
 */

#include "stdio.h"
#include "a.out.h"
#include "sys/types.h"
#include "sys/callout.h"
#include "crash.h"

struct nlist *Callout;			/* namelist symbol pointer */
extern char *strtbl;			/* pointer to string table */

/* get arguments for callout function */
int
getcallout()
{
	int c;

	if (!Callout)
		if (!(Callout = symsrch("callout")))
			error("callout not found in symbol table\n");

	optind = 1;
	while ((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp,"ADDR       FUNCTION        ARGUMENT   TIME\n");
	if (args[optind]) 
		longjmp(syn,0);
	else prcallout();
}

/* print callout table */
int
prcallout()
{
	struct nlist *sp;
	extern struct nlist *findsym();
	struct callout callbuf;
	int next;
	char *name;

	if (kvm_read(kd, Callout->n_value, (char *)&next, sizeof next) != sizeof next) 
		error("read error on callout table\n");
	for(;;) {
		if (kvm_read(kd, next, (char *)&callbuf, sizeof callbuf) != sizeof callbuf) 
			error("read error on callout table\n");
		if (!callbuf.c_next)	
			return;
		if (!(sp = findsym((unsigned long)callbuf.c_func)))
			error("%8x does not match in symbol table\n",
				callbuf.c_func);
		name = strtbl + sp->n_un.n_strx;
		fprintf(fp,"%8x   ", next);
		fprintf(fp,"%-15s", name);
		fprintf(fp," %8lx  %5u\n", 
			callbuf.c_arg,
			callbuf.c_time);
		next = (int)callbuf.c_next;
	}
}
