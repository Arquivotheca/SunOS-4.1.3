#ifndef lint
static	char sccsid[] = "@(#)sizenet.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:sizenet.c	1.4"
/*
 * This file contains code for the crash function size.  The RFS and
 * Streams tables and sizes are listed here to allow growth and not
 * overrun the compiler symbol table.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/mount.h"
#include "sys/time.h"
#include <sys/vnode.h>
#include <ufs/inode.h>
#include "sys/stream.h"
#include "crash.h"


struct sizenetable {
	char *name;
	char *symbol;
	unsigned size;
};
struct sizenetable sizntab[] = {
	"datab", "dblock", sizeof (struct datab),
	"dblk", "dblock", sizeof (struct datab),
	"dblock", "dblock", sizeof (struct datab),
#ifdef	notdef
	/*
	 * XXX:	further surgery required for dynamic streams allocation.
	 */
	"dbalcst", "dballoc", sizeof (struct dbalcst),
	"dballoc", "dballoc", sizeof (struct dbalcst),
#endif	notdef
	"linkblk", "linkblk", sizeof (struct linkblk),
	"mblk", "mblock", sizeof (struct msgb),
	"mblock", "mblock", sizeof (struct msgb),
	"msgb", "mblock", sizeof (struct msgb),
	"queue", "queue", sizeof (struct queue),
	"stdata", "streams", sizeof (struct stdata),
	"streams", "streams", sizeof (struct stdata),
	NULL, NULL, NULL
};	

/* get size from size table */
unsigned
getsizenetab(name)
char *name;
{
	unsigned size = 0;
	struct sizenetable *st;

	for (st = sizntab; st->name; st++) {
		if (!(strcmp(st->name, name))) {
			size = st->size;
			break;
		}
	}
	return(size);
}

/* print size */
int
prsizenet(name)
char *name;
{
	struct sizenetable *st;
	int i;

	if (!strcmp("", name)) {
		for (st = sizntab, i = 0; st->name; st++, i++) {
			if (!(i & 3))
				fprintf(fp, "\n");
			fprintf(fp, "%-15s", st->name);
		}
		fprintf(fp, "\n");
	}
}

/* get symbol name and size */
int
getnetsym(name, symbol, size)
char *name;
char *symbol;
unsigned *size;
{
	struct sizenetable *st;

	for (st = sizntab; st->name; st++) 
		if (!(strcmp(st->name, name))) {
			strcpy(symbol, st->symbol);
			*size = st->size;
			break;
		}
}
