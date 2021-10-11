#ifndef lint
static	char sccsid[] = "@(#)map.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:map.c	1.4"
/*
 * This file contains code for the crash function map.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/types.h"
#include "sys/map.h"
#include "crash.h"

/* get arguments for map function */
int
getmap()
{
	struct nlist *sp;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind])
		do {
			if (sp = symsrch(args[optind])) {
				fprintf(fp, "\n%s:\n", args[optind]);
				prmap(sp);
			}
			else fprintf(fp, "%s not found in symbol table\n",
				args[optind]);
		} while(args[++optind]);
	else longjmp(syn, 0);
}

/* print map */
int
prmap(name)
struct nlist *name;
{
	struct map mbuf;
	struct mapent mbuf1;
	unsigned units = 0, seg = 0;
	int addr;

	kvm_read(kd, name->n_value, &addr, sizeof addr);
	readmem(addr, 1, -1, (char *)&mbuf,
		sizeof mbuf, "map table");
	addr += sizeof (struct map);

	fprintf(fp, "MAPSIZE: %u\tSLEEP VALUE: %u\n",
		mbuf.m_free,
		mbuf.m_want);
	fprintf(fp, "\nSIZE    ADDRESS\n");
	for (;;addr += sizeof(struct mapent)) {
	        if (kvm_read(kd, addr, (char *)&mbuf1, sizeof(mbuf1)) != sizeof(mbuf1)) 
			error("read error on %s map table\n", name);
		if (!mbuf1.m_size) {
			fprintf(fp, "%u SEGMENTS, %u UNITS\n",
				seg,
				units);
			return;
		}
		fprintf(fp, "%4u   %8x\n",
			mbuf1.m_size,
			mbuf1.m_addr);
		units += mbuf1.m_size;
		seg++;
	}
}
