#ifndef lint
static	char sccsid[] = "@(#)page.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:page.c	1.10.2.3"
/*
 * This file contains code for the crash functions: page
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "vm/page.h"
#include "crash.h"

unsigned npages, base;
struct page *pages, *epages;

/* get arguments for page function */
int
getpage()
{
	int slot = -1;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	struct memseg memseg;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	/* collect system variables to calculate size */
	readsym("memseg", &memseg, sizeof memseg);
	pages = memseg.pages;
	epages = memseg.epages;
	base = memseg.pages_base;
	npages = epages - pages;

	fprintf(fp,"PAGE TABLE SIZE: %d START: %d\n", npages, base);
	fprintf(fp,"SLOT IO  KP LK   VNODE    OFF  HASH NEXT PREV  VPN  VPP      MAP FLAGS\n");
	if(args[optind]) {
		do {
			getargs((int)npages, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prpage(slot, phys, addr, npages);
			else {
				if (arg1 < npages)
					slot = arg1;
				else addr = arg1;
				prpage(slot, phys, addr, npages);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < npages; slot++)
		prpage(slot, phys, addr, npages);
}

/* print page table */
int
prpage(slot, phys, addr, size)
int slot, phys;
long addr;
unsigned size;
{
	struct page pbuf;
	unsigned page;
	int next, prev, hash, vpnext, vpprev;

	readsym("pages", &page, sizeof page);
	readbuf(addr, (long)(page + slot * sizeof (struct page)), phys, -1, 
		(char *)&pbuf, sizeof pbuf, "page table");
	/* calculate page entry number of pointers */
	next = ((long)pbuf.p_next - page)/sizeof pbuf;
	if ((next < 0) || (next > size)) next = -1;
	prev = ((long)pbuf.p_prev - page)/sizeof pbuf;
	if ((prev < 0) || (prev > size)) prev = -1;
	vpnext = ((long)pbuf.p_vpnext - page)/sizeof pbuf;
	if ((vpnext < 0) || (vpnext > size)) vpnext = -1;
	vpprev = ((long)pbuf.p_vpprev - page)/sizeof pbuf;
	if ((vpprev < 0) || (vpprev > size)) vpprev = -1;
	hash = ((long)pbuf.p_hash - page)/sizeof pbuf;
	if ((hash < 0) || (hash > size)) hash = -1;
	if (addr > -1) 
		slot = getslot(addr, (long)page, sizeof pbuf, phys, size);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
	fprintf(fp, " %2d %3d %3d %8x %6x ", 
		pbuf.p_nio,
		pbuf.p_keepcnt, 
		pbuf.p_lckcnt, 
		pbuf.p_vnode, 
		pbuf.p_offset); 
	if (hash == -1)
		fprintf(fp, "    -");
	else fprintf(fp, " %4d", hash);
	if (next == -1)
		fprintf(fp, "    -");
	else fprintf(fp, " %4d", next);
	if (prev == -1)
		fprintf(fp, "    -");
	else fprintf(fp, " %4d", prev);
	if (vpnext == -1)
		fprintf(fp, "    -");
	else fprintf(fp, " %4d", vpnext);
	if (vpprev == -1)
		fprintf(fp, "    -");
	else fprintf(fp, " %4d", vpprev);
	fprintf(fp, " %8x   ", pbuf.p_mapping);
	fprintf(fp, "%s%s%s%s%s%s%s%s%s%s\n", 
		pbuf.p_lock ? "l" : "",
		pbuf.p_want ? "w" : "",
		pbuf.p_free ? "f" : "",
		pbuf.p_intrans ? "i" : "",
		pbuf.p_gone ? "g" : "",
		pbuf.p_mod ? "m" : "",
		pbuf.p_ref ? "r" : "",
		pbuf.p_pagein ? "p" : "",
		pbuf.p_nc ? "n" : "",
		pbuf.p_age ? "a" : "");
}
