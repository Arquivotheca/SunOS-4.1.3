#ifndef lint
static	char sccsid[] = "@(#)size.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:size.c	1.10"
/*
 * This file contains code for the crash functions:  size, findslot, and
 * findaddr.  The size table for RFS and Streams structures is located in
 * sizenet.c
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/types.h"
#include "sys/user.h"
#include "sys/mount.h"
#include "sys/buf.h"
#include "sys/callout.h"
#include "sys/conf.h"
#include "sys/time.h"
#include <sys/vnode.h>
#include <ufs/inode.h>
#include "sys/fcntl.h"
#include "sys/proc.h"
#include "vm/page.h"
#include "crash.h"

struct sizetable {
	char *name;
	char *symbol;
	unsigned size;
};

struct sizetable siztab[] = {
	"page","pages",sizeof (struct page),
	"proc","proc",sizeof (struct proc),
	NULL,NULL,NULL
};	


/* get size from size tables */
unsigned
getsizetab(name)
char *name;
{
	unsigned size = 0;
	struct sizetable *st;
	extern unsigned getsizenetab();

	for (st = siztab; st->name; st++) {
		if (!(strcmp(st->name,name))) {
			size = st->size;
			break;
		}
	if (!size)
		size = getsizenetab(name);
	}
	return(size);
}

/* get arguments for size function */
int
getsize()
{
	int c;
	char *all = "";
	int hex = 0;

	optind = 1;
	while ((c = getopt(argcnt,args,"xw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'x' : 	hex = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		do {
			prsize(args[optind++],hex);
		} while(args[optind]);
	}
	else prsize(all,hex);
}

/* print size */
int
prsize(name,hex)
char *name;
int hex;
{
	unsigned size;
	struct sizetable *st;
	int i;

	if (!strcmp("",name)) {
		for (st = siztab,i = 0; st->name; st++,i++) {
			if (!(i & 3))
				fprintf(fp,"\n");
			fprintf(fp,"%-15s",st->name);
		}
		prsizenet(name);
	}
	else {
		size = getsizetab(name);
		if (size) {
			if (hex)
				fprintf(fp,"0x%x\n",size);
			else fprintf(fp,"%d\n",size);
		}
		else error("%s does not match in sizetable\n",name);
	}
}

/* get arguments for findaddr function */
int
getfindaddr()
{
	int c;
	int slot;
	char *name;

	optind = 1;
	while ((c = getopt(argcnt,args,"w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (args[optind]) {
		name = args[optind++];
		if (args[optind]) {
			if ((slot = (int)strcon(args[optind],'d')) == -1)
				error("\n");
			prfindaddr(name,slot);
		}
		else longjmp(syn,0);
	}
	else longjmp(syn,0);
}

/* print address */
int
prfindaddr(name,slot)
char *name;
int slot;
{
	unsigned size = 0;
	struct nlist *sp;
	struct sizetable *st;
	char symbol[10];
	int addr;

	symbol[0] = '\0';
	for (st = siztab; st->name; st++) 
		if (!(strcmp(st->name,name))) {
			strcpy(symbol,st->symbol);
			size = st->size;
			break;
		}
	if (symbol[0] == '\0') 
		getnetsym(name,symbol,&size);
	if (symbol[0] == '\0')
		error("no match for %s in sizetable\n",name);
	if (!(sp = symsrch(symbol)))
		error("no match for %s in symbol table\n",name);
	kvm_read(kd, sp->n_value, &addr, sizeof addr);
	fprintf(fp,"%8x\n", addr + size * slot);
}
