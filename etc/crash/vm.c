#ifndef lint
static	char sccsid[] = "@(#)vm.c 1.1 92/07/30 SMI";
#endif

/*
 * This file contains code for the crash functions:  as, seg, segdata, pmgrp,
 * ctx, pment
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/time.h"
#include "sys/proc.h"
#include "machine/vm_hat.h"
#include "vm/as.h"
#include "vm/seg.h"
#include "vm/seg_dev.h"
#include "vm/seg_map.h"
#include "vm/seg_vn.h"
#include "vm/page.h"
#include "machine/seg_kmem.h"
#include "machine/param.h"
#include "machine/mmu.h"
#include "crash.h"

extern int nproc, procv;
int prvndata(), prmapdata(), prdevdata(), prkmemdata();
extern char *strtbl;			/* pointer to string table */

struct segops {
	char *name;
	int  value;
	int (*print)();
	char *heading;
};
struct segops segops[] = {
	{"vn", 0, prvndata, " L PP  P MP TY    VNODE    OFF  IX      AMP VP     CRED SWR"},
	{"map", 0, prmapdata, "    SMAP   SMAPFR  PR  WNT  HASHSZ   HASH"},
	{"dev", 0, prdevdata, "           FUNC OFFSET    DEV  PP   P  MP    VPAGE"},
	{"kmem", 0, NULL, NULL},
	{"u", 0, NULL, NULL},
	{0, 0}
};

extern unsigned npages, base;
extern struct page *pages, *epages;
unsigned pmev, pmgrpv, ctxv, npmgrps, nctxs;

/* get arguments for as function */
int
getas()
{
	int slot = Procslot;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int id = -1;
	int c;
#if defined(sun2) || defined(sun3) || defined(sun4) || defined(sun4c)
	char *heading = "  PID       AS  L W KEEP   SEGPTR  SEGLAST   RSS     CTX    PMGRP\n";
#endif
#if defined (sun3x) || defined (sun4m)
	char *heading = "  PID       AS  L W KEEP   SEGPTR  SEGLAST   RSS V       L1       L2       L3\n";
#endif

	optind = 1;
	while((c = getopt(argcnt,args,"pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			if(*args[optind] == '#') {
				if((id = strcon(++args[optind],'d')) == -1)
					error("\n");
				pras(slot, id, phys, addr);
			}
			else {
				getargs(nproc, &arg1, &arg2);
				if (arg1 == -1) 
					continue;
				if (arg2 != -1)
					for (slot = arg1; slot <= arg2; slot++)
						pras(slot, id, phys, addr);
				else {
					if(arg1 < nproc)
						slot = arg1;
					else addr = arg1;
					pras(slot, id, phys, addr);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < nproc; slot++)
		pras(slot, id, phys, addr);
}

/* print address spaces */
int
pras(slot, id, phys, addr)
int slot, id, phys;
unsigned addr;
{
	char ch, *typ;
	struct proc procbuf;
	struct user *ubp;
	int i, j, cnt;
	struct as asbuf;

	if (addr == -1) {
		if (id != -1) {
			for (slot = 0; slot < nproc; slot++) {
				readmem(procv + slot * sizeof procbuf, 1, 
					slot, (char *)&procbuf, sizeof procbuf, 
						"proc table");
					if (procbuf.p_pid == id) 
						break;
			}
			if (slot == nproc) {
				fprintf(fp, "%d not valid process id\n", id);
				return;
			}
		}
		else readbuf(addr, procv + slot * sizeof procbuf, phys, -1, 
				(char *)&procbuf, sizeof procbuf, "proc table");
		if (!procbuf.p_stat)
			return;
		if (procbuf.p_pid == 0) procbuf.p_as = (struct as *)(symsrch("kas")->n_value);
		if (procbuf.p_as == NULL) return;
		readmem(procbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
		fprintf(fp,"%5u %8x ",
			procbuf.p_pid,
			procbuf.p_as);
	}
	else {
		readmem(addr, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
		fprintf(fp,"      %8x ", addr);
	}
	fprintf(fp,"%2u %2u %3u %8x %8x %5d",
		asbuf.a_lock,
		asbuf.a_want,
		asbuf.a_keepcnt,
		asbuf.a_segs,
		asbuf.a_seglast,
		asbuf.a_rss);
#if defined(sun2) || defined(sun3) || defined(sun4) || defined(sun4c)
	fprintf(fp,"%8x %8x\n",
		asbuf.a_hat.hat_ctx,
		asbuf.a_hat.hat_pmgrps);
#endif
#if defined (sun3x)
	fprintf(fp,"%2u %8x %8x %8x\n",
		asbuf.a_hat.hat_valid,
		asbuf.a_hat.hat_l1pfn,
		asbuf.a_hat.hat_l2pts,
		asbuf.a_hat.hat_l2pts);
#endif
#if defined (sun4m)
	fprintf(fp,"%2u %8x %8x %8x\n",
		asbuf.a_hat.hat_ptvalid,
		asbuf.a_hat.hat_l1pt,
		asbuf.a_hat.hat_l2pts,
		asbuf.a_hat.hat_l2pts);
#endif
}

/* get arguments for seg function */
int
getseg()
{
	int slot = Procslot;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int id = -1;
	int c;
	char *heading = "  PID LK     BASE   SIZE       AS     NEXT     PREV      OPS     DATA\n";

	optind = 1;
	while((c = getopt(argcnt,args,"pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	if (segops[0].value == 0) init_segops();
	fprintf(fp,"%s",heading);
	if(args[optind]) {
		do {
			if(*args[optind] == '#') {
				if((id = strcon(++args[optind],'d')) == -1)
					error("\n");
				prseg(slot, id, phys, addr);
			}
			else {
				getargs(nproc, &arg1, &arg2);
				if (arg1 == -1) 
					continue;
				if (arg2 != -1)
					for (slot = arg1; slot <= arg2; slot++)
						prseg(slot, id, phys, addr);
				else {
					if(arg1 < nproc)
						slot = arg1;
					else addr = arg1;
					prseg(slot, id, phys, addr);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for (slot = 0; slot < nproc; slot++)
		prseg(slot, id, phys, addr);
}

/* print segments */
int
prseg(slot, id, phys, addr)
int slot, id, phys;
unsigned addr;
{
	char ch, *typ;
	struct proc procbuf;
	struct user *ubp;
	int i, j, cnt;
	struct as asbuf;
	struct seg segbuf;
	int segaddr;
	register struct segops *sp;

	if (addr == -1) {
		if (id != -1) {
			for (slot = 0; slot < nproc; slot++) {
				readmem(procv + slot * sizeof procbuf, 1, 
					slot, (char *)&procbuf, sizeof procbuf, 
						"proc table");
					if (procbuf.p_pid == id) 
						break;
			}
			if (slot == nproc) {
				fprintf(fp, "%d not valid process id\n", id);
				return;
			}
		}
		else readbuf(addr, procv + slot * sizeof procbuf, phys, -1, 
				(char *)&procbuf, sizeof procbuf, "proc table");
		if (!procbuf.p_stat) return;
		if (procbuf.p_pid == 0) procbuf.p_as = (struct as *)(symsrch("kas")->n_value);
		if (procbuf.p_as == NULL) return;
		readmem(procbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
		readmem(asbuf.a_segs, 1, -1, (char *)&segbuf, sizeof segbuf, "segment");
		segaddr = (int)asbuf.a_segs;
	}
	else {
		readmem(addr, 1, -1, (char *)&segbuf, sizeof segbuf, "segment");
		segaddr = addr;
	}
	for(;;) {
		register struct segops *sp = segops;
		char ops[8];

		while (sp->name) {
			if ((int)segbuf.s_ops == sp->value) break;
			sp++;
		}
		if (sp->name) sprintf(ops, "%s", sp->name);
			else sprintf(ops, "%x", segbuf.s_ops);
		if (addr == -1) fprintf(fp, "%5u ", procbuf.p_pid);
			else fprintf(fp, "      ");
		fprintf(fp, "%2x %8x %6x %8x %8x %8x %8s %8x\n",
			segbuf.s_lock.dummy,
			segbuf.s_base,
			segbuf.s_size,
			segbuf.s_as,
			segbuf.s_next,
			segbuf.s_prev,
			ops,
			segbuf.s_data);
		if (segbuf.s_next == (struct seg *)segaddr) break;
		readmem(segbuf.s_next, 1, -1, (char *)&segbuf, sizeof segbuf, "segment");
	}
}

/* get arguments for segdata function */
int
getsegdata()
{
	int slot = Procslot;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int id = -1;
	int c;
	char *type;
	register struct segops *sp = segops;

	optind = 1;
	while((c = getopt(argcnt,args,"pw:t:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 't' :	type = optarg;
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}

	while (sp->name) {
		if (!strcmp(sp->name, type)) break;
		sp++;
	}
	if ((sp->name == NULL) || (sp->print == NULL)) {
		fprintf(fp, "valid data types are: ");
		for (sp = segops; sp->name; sp++)
			if (sp->print) fprintf(fp, "%s ", sp->name);
		fprintf(fp, "\n");
		return;
	}

	fprintf(fp, "  PID      SEG ");
	fprintf(fp,"%s\n", sp->heading);
	if (segops[0].value == 0) init_segops();
	if(args[optind]) {
		do {
			if(*args[optind] == '#') {
				if((id = strcon(++args[optind],'d')) == -1)
					error("\n");
				prsegdata(slot, id, phys, addr, sp);
			}
			else {
				getargs(nproc, &arg1, &arg2);
				if (arg1 == -1) 
					continue;
				if (arg2 != -1)
					for (slot = arg1; slot <= arg2; slot++)
						prsegdata(slot, id, phys, addr, sp);
				else {
					if(arg1 < nproc)
						slot = arg1;
					else addr = arg1;
					prsegdata(slot, id, phys, addr, sp);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		}while(args[++optind]);
	}
	else for (slot = 0; slot < nproc; slot++)
		prsegdata(slot, id, phys, addr, sp);
}

/* print segment data */
int
prsegdata(slot, id, phys, addr, sp)
int slot, id, phys;
unsigned addr;
register struct segops *sp;
{
	char ch, *typ;
	struct proc procbuf;
	struct user *ubp;
	int i, j, cnt;
	struct as asbuf;
	struct seg segbuf;
	int saddr;

	if (id != -1) {
		for (slot = 0; slot < nproc; slot++) {
			readmem(procv + slot * sizeof procbuf, 1, 
				slot, (char *)&procbuf, sizeof procbuf, 
					"proc table");
				if (procbuf.p_pid == id) 
					break;
		}
		if (slot == nproc) {
			fprintf(fp, "%d not valid process id\n", id);
			return;
		}
	}
	else readbuf(addr, procv + slot * sizeof procbuf, phys, -1, 
			(char *)&procbuf, sizeof procbuf, "proc table");
	if (!procbuf.p_stat)
		return;
	if (addr > -1) 
		slot = getslot(addr, procv, sizeof procbuf, phys);
	if (procbuf.p_pid == 0) procbuf.p_as = (struct as *)(symsrch("kas")->n_value);
	if (procbuf.p_as == NULL) return;
	readmem(procbuf.p_as, 1, -1, (char *)&asbuf, sizeof asbuf, "address space");
	saddr = (int)asbuf.a_segs;
	readmem(saddr, 1, -1, (char *)&segbuf, sizeof segbuf, "segment");
	for(;;) {
		if ((int)segbuf.s_ops == sp->value) (*(sp->print))(&procbuf, saddr, &segbuf);
		if (segbuf.s_next == asbuf.a_segs) break;
		saddr = (int)segbuf.s_next;
		readmem(saddr, 1, -1, (char *)&segbuf, sizeof segbuf, "segment");
	}
}

prvndata(pp, saddr, sp)
struct proc *pp;
register struct seg *sp;
{
	struct segvn_data sdbuf;

	readmem(sp->s_data, 1, -1, (char *)&sdbuf, sizeof sdbuf, "segment data");
	fprintf(fp, "%5u %8x %2x %2x %2x %2x %2x %8x %6x %3x %8x %2x %8x %3d\n",
		pp->p_pid,
		saddr,
		sdbuf.lock.dummy,
		sdbuf.pageprot,
		sdbuf.prot,
		sdbuf.maxprot,
		sdbuf.type,
		sdbuf.vp,
		sdbuf.offset,
		sdbuf.anon_index,
		sdbuf.amp,
		sdbuf.vpage,
		sdbuf.cred,
		sdbuf.swresv >> PAGESHIFT);
}

prmapdata(pp, saddr, sp)
struct proc *pp;
register struct seg *sp;
{
	struct segmap_data sdbuf;

	readmem(sp->s_data, 1, -1, (char *)&sdbuf, sizeof sdbuf, "segment data");
	fprintf(fp, "%5u %8x %8x %8x %3x %3x %4x %8x\n",
		pp->p_pid,
		saddr,
		sdbuf.smd_sm,
		sdbuf.smd_free,
		sdbuf.smd_prot,
		sdbuf.smd_want,
		sdbuf.smd_hashsz,
		sdbuf.smd_hash);
}

prdevdata(pp, saddr, sp)
struct proc *pp;
register struct seg *sp;
{
	struct segdev_data sdbuf;
	extern struct nlist *findsym();
	struct nlist *np;
	char tmp[15];

	readmem(sp->s_data, 1, -1, (char *)&sdbuf, sizeof sdbuf, "segment data");
	if (!(np = findsym((unsigned)sdbuf.mapfunc)))
		sprintf(tmp, "%x", sdbuf.mapfunc);
	else sprintf(tmp, "%15s", strtbl + np->n_un.n_strx);
	fprintf(fp, "%5u %8x %15s %6x %3d,%2d %3x %3x %3x %8x\n",
		pp->p_pid,
		saddr,
		tmp,
		sdbuf.offset,
		(sdbuf.dev & 0xff00) >> 8,
		sdbuf.dev & 0xff,
		sdbuf.pageprot,
		sdbuf.prot,
		sdbuf.maxprot,
		sdbuf.vpage);
}

init_segops()
{
	register struct segops *sp = segops;
	struct nlist *np;
	char tmp[80];

	while (sp->name) {
		sprintf(tmp, "_seg%s_ops", sp->name);
		np = symsrch(tmp);
		if (np) sp->value = np->n_value;
		sp++;
	}
}

#if defined(sun2) || defined(sun3) || defined(sun4) || defined(sun4c)
getpmgrp()
{
	int slot = -1;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	
	pmgrpv = symsrch("pmgrps")->n_value;
#if defined(sun2) || defined(sun3)
	npmgrps = NPMGRPS;
#endif
#ifdef sparc
	readmem(symsrch("npmgrps")->n_value, 1, -1, (char *)&npmgrps, sizeof npmgrps, "number of contexts");
#endif	sparc
	pmev = symsrch("pments")->n_value;
	fprintf(fp, "PMGRP TABLE SIZE = %d\n", npmgrps);
	fprintf(fp, "%s", "SLOT L   NUM KEEP       AS     BASE      NXT PREV  PME\n");
	if (args[optind]) {
		do {
			getargs(npmgrps, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prpmgrp(slot, phys, addr);
			else {
				if (arg1 < npmgrps)
					slot = arg1;
				else addr = arg1;
				prpmgrp(slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < npmgrps; slot++)
		prpmgrp(slot, phys, addr);
}

#ifdef sparc
/*
 * XXX - max. no of PM entries; should use per-CPU table.
 */
#ifndef	MNPMENTS
#define MNPMENTS	8192
#endif
#endif sparc

/* print pmgrp table */
int
prpmgrp(slot, phys, addr)
int slot, phys;
unsigned addr;
{
	register i;
	struct pmgrp pbuf;

	readbuf(addr, pmgrpv + slot * sizeof pbuf, phys, -1,
		(char *)&pbuf, sizeof pbuf, "pmgrp table");
	if (addr > -1) 
		slot = getslot(addr, pmgrpv, sizeof pbuf, phys,
			npmgrps);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
	fprintf(fp, " %1u %5u %4d %8x %8x",
		pbuf.pmg_lock,
		pbuf.pmg_num,
		pbuf.pmg_keepcnt,
		pbuf.pmg_as,
		pbuf.pmg_base);
	slot = ((unsigned)pbuf.pmg_next - pmgrpv) / sizeof pbuf;
	if ((slot >= 0) && (slot < npmgrps))
		fprintf(fp, "\t%4d", slot);
	else fprintf(fp, "\t  - ");
	slot = ((unsigned)pbuf.pmg_prev - pmgrpv) / sizeof pbuf;
	if ((slot >= 0) && (slot < npmgrps))
		fprintf(fp, " %4d", slot);
	else fprintf(fp, "   - ");
	slot = ((unsigned)pbuf.pmg_pme - pmev) / sizeof(struct pment);
	if ((slot >= 0) && (slot < MNPMENTS))
		fprintf(fp, " %4d", slot);
	else fprintf(fp, "   - ");
	fprintf(fp, "\n");
}

getctx()
{
	int slot = -1;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	
	ctxv = symsrch("ctxs")->n_value;
#if defined(sun2) || defined(sun3)
	nctxs = NCTXS;
#endif
#ifdef sparc
	readmem(symsrch("nctxs")->n_value, 1, -1, (char *)&nctxs, sizeof nctxs, "number of contexts");
#endif	sparc
	fprintf(fp, "CONTEXT TABLE SIZE = %d\n", nctxs);
	fprintf(fp, "%s", "SLOT L NUM  TIME       AS\n");
	if (args[optind]) {
		do {
			getargs(nctxs, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prctx(slot, phys, addr);
			else {
				if (arg1 < nctxs)
					slot = arg1;
				else addr = arg1;
				prctx(slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < nctxs; slot++)
		prctx(slot, phys, addr);
}

/* print context table */
int
prctx(slot, phys, addr)
int slot, phys;
unsigned addr;
{
	register i;
	struct ctx cbuf;

	readbuf(addr, ctxv + slot * sizeof cbuf, phys, -1,
		(char *)&cbuf, sizeof cbuf, "context table");
	if (addr > -1) 
		slot = getslot(addr, ctxv, sizeof cbuf, phys,
			nctxs);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
	fprintf(fp, " %1u %3u %5d %8x",
		cbuf.c_lock,
		cbuf.c_num,
		cbuf.c_time,
		cbuf.c_as);
	fprintf(fp, "\n");
}

getpment()
{
	int slot = -1;
	int phys = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	
	readsym("pages", &pages, sizeof pages);
	readsym("epages", &epages, sizeof epages);
	npages = epages - pages;

	pmev = symsrch("pments")->n_value;
	fprintf(fp, "PMENT TABLE SIZE = %d\n", MNPMENTS);
	fprintf(fp, "%s", "SLOT   PAGE   NEXT FLAGS\n");
	if (args[optind]) {
		do {
			getargs(MNPMENTS, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prpment(slot, phys, addr);
			else {
				if (arg1 < MNPMENTS)
					slot = arg1;
				else addr = arg1;
				prpment(slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < MNPMENTS; slot++)
		prpment(slot, phys, addr);
}

/* print pment table */
int
prpment(slot, phys, addr)
int slot, phys;
unsigned addr;
{
	register i;
	struct pment pbuf;

	readbuf(addr, pmev + slot * sizeof pbuf, phys, -1,
		(char *)&pbuf, sizeof pbuf, "pment table");

	if (!pbuf.pme_valid) return;
	if (addr > -1) 
		slot = getslot(addr, pmev, sizeof pbuf, phys,
			MNPMENTS);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);

	slot = pbuf.pme_page - pages;
	if ((slot >= 0) && (slot < npages))
		fprintf(fp, "\t%4d", slot);
	else fprintf(fp, "\t  - ");
	fprintf(fp, " %5u ", pbuf.pme_next);
	fprintf(fp, "%s%s%s",
		pbuf.pme_intrep ? "i" : " ",
		pbuf.pme_nosync ? "n" : " ",
		pbuf.pme_valid ? "v" : " ");
	fprintf(fp, "\n");
}
#else
getpmgrp()
{
	notvalid();
}

getctx()
{
	notvalid();
}

getpment()
{
	notvalid();
}
#endif
