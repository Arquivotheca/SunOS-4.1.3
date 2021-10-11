#ifndef lint
static	char sccsid[] = "@(#)stream.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:stream.c	1.15.3.1"
/*
 * This file contains code for the crash functions:  stream, queue, mblock,
 * mbfree, dblock, dbfree, strstat, linkblk, qrun.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/time.h"
#include "sys/proc.h"
#include <sys/vnode.h>
#include <ufs/inode.h>
#include "sys/mount.h"
#include "sys/poll.h"
#include "sys/stream.h"
#include "sys/strstat.h"
#include "sys/stropts.h"
#include "crash.h"

struct nlist *Mbfree, *Dbfree, *Strst, *Qhead,
	*Linkblk, *Nmuxlink, *Strbufsizes;	/* namelist symbol pointers */
extern struct nlist *Inode, *Proc;
unsigned dbfreev, streamv, mbfreev;
unsigned nstrbufszv, strbufstatv;
mblk_t *prmess();
dblk_t *prdblk();

int
blockinit()
{

	readsym("dbfreelist", &dbfreev, sizeof(int));
	readsym("mbfreelist", &mbfreev, sizeof(int));
}

/* get arguments for stream function */
int
getstream()
{
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *heading = "    ADDR      WRQ IOCB    VNODE  PGRP      IOCID IOCWT WOFF ERR FLAG\n";

	readsym("allstream", &streamv, sizeof(int));
	optind = 1;
	while ((c = getopt(argcnt, args, "efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (!full)
		fprintf(fp, "%s", heading);
	if (args[optind]) {
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 == -1)
				continue;
			else addr = arg1;
			prstream(full, phys, addr, heading);
			addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else prstream(full, phys, addr, heading);
}

/* print streams table */
int
prstream(full, phys, addr, heading)
int full, phys;
unsigned long addr;
char *heading;
{
	struct stdata strm;
	struct strevent evbuf;
	struct strevent *next;
	int all = 0;

	if (addr == -1) {
		all = 1;
		addr = streamv;
	}
	while (1) {
		readbuf(addr, addr, phys, -1,
			(char *)&strm, sizeof strm, "stream table");
		if (!strm.sd_wrq && !all) 
			continue;
		if (full)
			fprintf(fp, "%s", heading);
		else fprintf(fp, "%8x ", addr);
		fprintf(fp, "%8x ", strm.sd_wrq);
		if (strm.sd_iocblk)
			fprintf(fp, "%8x ", strm.sd_iocblk);
		else
			fprintf(fp, "   - ");
		fprintf(fp, "%8x ", strm.sd_vnode);
		fprintf(fp, "%5d %10d %5d %4d %3o ", strm.sd_pgrp, strm.sd_iocid,
			strm.sd_iocwait, strm.sd_wroff, strm.sd_error);
		fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
			((strm.sd_flag & IOCWAIT) ? "iocw " : ""),
			((strm.sd_flag & RSLEEP) ? "rslp " : ""),
			((strm.sd_flag & WSLEEP) ? "wslp " : ""),
			((strm.sd_flag & STRPRI) ? "pri " : ""),
			((strm.sd_flag & STRHUP) ? "hup " : ""),
			((strm.sd_flag & STWOPEN) ? "stwo " : ""),
			((strm.sd_flag & STPLEX) ? "plex " : ""),
			((strm.sd_flag & RMSGDIS) ? "mdis " : ""),
			((strm.sd_flag & RMSGNODIS) ? "mnds " : ""),
			((strm.sd_flag & STRERR) ? "err " : ""),
			((strm.sd_flag & STRTIME) ? "sttm " : ""),
			((strm.sd_flag & STR2TIME) ? "s2tm " : ""),
			((strm.sd_flag & STR3TIME) ? "s3tm " : ""),
			((strm.sd_flag & NBIO) ? "nbio " : ""),
			((strm.sd_flag & ASYNC) ? "asyn " : ""),
			((strm.sd_flag & OLDNDELAY) ? "oldn " : ""),
			((strm.sd_flag & STRTOSTOP) ? "tstp " : ""),
			((strm.sd_flag & TIMEDOUT) ? "tout " : ""),
			((strm.sd_flag & RCOLL) ? "rcol " : ""),
			((strm.sd_flag & WCOLL) ? "wcol " : ""),
			((strm.sd_flag & ECOLL) ? "ecol " : ""));
		if (full) {
			fprintf(fp, "read proc %8x write proc %8x except proc %8x\n",
				strm.sd_selr,
				strm.sd_selw,
				strm.sd_sele);
			fprintf(fp, "\t  strtab rcnt vmin vtime deficit\n");
			fprintf(fp, "\t%8x   %2d %4d %4d %4d\n",
				strm.sd_strtab,
				strm.sd_pushcnt,
				strm.sd_vmin,
				strm.sd_vtime,
				strm.sd_deficit);
			fprintf(fp, "\tSIGFLAGS:  %s%s%s%s\n",
				((strm.sd_sigflags & S_INPUT) ? " input" : ""),
				((strm.sd_sigflags & S_HIPRI) ? " hipri" : ""),
				((strm.sd_sigflags & S_OUTPUT) ? " output" : ""),
				((strm.sd_sigflags & S_MSG) ? " msg" : ""));
			fprintf(fp, "\tSIGLIST:\n");
			next = strm.sd_siglist;
			while (next) {
				readmem((long)next, 1, -1, (char *)&evbuf,
					sizeof evbuf, "stream event buffer");
				fprintf(fp, "\t\tPROC:  %3d   %s%s%s%s\n",
					((long)evbuf.se_procp-Proc->n_value)/
						sizeof(struct proc),
					((evbuf.se_events & S_INPUT) ? " input" : ""),
					((evbuf.se_events & S_HIPRI) ? " hipri" : ""),
					((evbuf.se_events & S_OUTPUT) ? " output" : ""),
					((evbuf.se_events & S_MSG) ? " msg" : ""));
				next = evbuf.se_next;	
			}
			fprintf(fp, "\tPOLLFLAGS:  %s%s%s\n",
				((strm.sd_pollflags & POLLIN) ? " in" : ""),
				((strm.sd_pollflags & POLLPRI) ? " pri" : ""),
				((strm.sd_pollflags & POLLOUT) ? " out" : ""));
			fprintf(fp, "\tPOLLIST:\n");
			next = strm.sd_pollist;
			while (next) {
				readmem((long)next, 1, -1, (char *)&evbuf,
					sizeof evbuf, "stream event buffer");
				fprintf(fp, "\t\tPROC:  %3d   %s%s%s\n",
					((long)evbuf.se_procp-Proc->n_value)/
						sizeof(struct proc),
					((evbuf.se_events & POLLIN) ? " in" : ""),
					((evbuf.se_events & POLLPRI) ? " pri" : ""),
					((evbuf.se_events & POLLOUT) ? " out" : ""));
				next = evbuf.se_next;	
			}
			fprintf(fp, "\n");
		}
		if (!all) break;
		addr = (int)strm.sd_next;
		if (!addr) break;
	}
}

/* get arguments for queue function */
int
getqueue()
{
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "pw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "    INFO     NEXT     LINK     PTR  RCNT HEAD TAIL MINP MAXP HIWT LOWT FLAG\n");
	if (args[optind]) {
		do {
			getargs(0xffffffff, &arg1, &arg2);
			if (arg1 == -1)
				continue;
			else addr = arg1;
			prqueue(phys, addr);
			addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
}

/* print queue structure */
int
prqueue(phys, addr)
int phys;
long addr;
{
	queue_t que;

	readbuf(addr, addr, phys, -1,
		(char *)&que, sizeof que, "queue slot");
	fprintf(fp, "%8x %8x %8x",
		que.q_qinfo,
		que.q_next,
		que.q_link);
	fprintf(fp, "%8x ", que.q_ptr);
	fprintf(fp, " %4d ", que.q_count);
	if (que.q_first)
		fprintf(fp, "%8x ", que.q_first);
	else fprintf(fp, "   - ");
	if (que.q_last)
		fprintf(fp, "%8x ", que.q_last);
	else fprintf(fp, "   - ");
	fprintf(fp, "%4d %4d %4d %4d ",
		que.q_minpsz,
		que.q_maxpsz,
		que.q_hiwat,
		que.q_lowat);
	fprintf(fp, "%s%s%s%s%s%s%s\n",
		((que.q_flag & QENAB) ? "en " : ""),
		((que.q_flag & QWANTR) ? "wr " : ""),
		((que.q_flag & QWANTW) ? "ww " : ""),
		((que.q_flag & QFULL) ? "fl " : ""),
		((que.q_flag & QREADR) ? "rr " : ""),
		((que.q_flag & QUSE) ? "us " : ""),
		((que.q_flag & QNOENB) ? "ne " : ""));
}

/* get arguments for mblock function */
int
getmess()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "MBLKADDR     NEXT     CONT     PREV	RPTR     WPTR    DATAB\n");
	if (args[optind]) {
		all = 1;
		do {
			if ((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prmess(all, addr, phys, 0);
		} while (args[optind]);
	}
}

/* print mblock table */
mblk_t *
prmess(all, addr, phys, fflag)
int all, phys;
long addr;
int fflag;
{
	mblk_t mblk;

	readbuf(addr, addr, phys, -1, (char *)&mblk, sizeof mblk,
	    "message block");
	if (!mblk.b_datap && !all)
		return (NULL);
	if (mblk.b_datap && fflag) {
		fprintf(fp,"mbfreelist modified while examining - discontinuing search\n");
		return (NULL);
	}
	fprintf(fp, "%8x ", addr);
	if (mblk.b_next)
		fprintf(fp, "%8x ", mblk.b_next);
	else fprintf(fp, "       - ");
	if (mblk.b_cont)
		fprintf(fp, "%8x ", mblk.b_cont);
	else fprintf(fp, "       - ");
	if (mblk.b_prev)
		fprintf(fp, "%8x ", mblk.b_prev);
	else fprintf(fp, "       - ");
	fprintf(fp, "%8x %8x ", mblk.b_rptr, mblk.b_wptr);
	if (mblk.b_datap)
		fprintf(fp, "%8x\n", mblk.b_datap);
	else fprintf(fp, "       -\n");
	return (mblk.b_next);
}

/* get arguments for mbfree function */
int
getmbfree()
{
	int c;

	blockinit();
	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "MBLKADDR     NEXT     CONT     PREV	RPTR     WPTR    DATAB\n");
	if (args[optind]) 
		longjmp(syn, 0);
	prmbfree();
}

/* print mblock free list */
int
prmbfree()
{
	mblk_t *m;

	m = (mblk_t *)mbfreev;
	while (m)
		m = prmess(1, m, 0, 1);
}

/* get arguments for dblock function */
int
getdblk()
{
	int all = 0;
	int phys = 0;
	long addr = -1;
	int c;

	optind = 1;
	while((c = getopt(argcnt,args,"epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn,0);
		}
	}
	fprintf(fp, "DBLKADDR  SIZE  RCNT TYPE         BASE    LIMIT    FRTNP\n");
	if (args[optind]) {
		all = 1;
		do {
			if ((addr = strcon(args[optind++], 'h')) == -1)
				error("\n");
			(void) prdblk(all, addr, phys, 0);
		} while (args[optind]);
	}
}

/* print dblock table */
dblk_t *
prdblk(all, addr, phys, fflag)
int all, phys;
long addr;
int fflag;
{
	dblk_t dblk;

	readbuf(addr, addr, phys, -1, (char *)&dblk, sizeof dblk,
	    "data block");
	if (!dblk.db_ref && !all)
		return(NULL);
	if (dblk.db_ref && fflag) {
		fprintf(fp,"dbfreelist modified while examining - discontinuing search\n");
		return (NULL);
	}

	fprintf(fp, "%8x", addr);
	fprintf(fp, " %5u ", dblk.db_size);
	fprintf(fp, " %4d ", dblk.db_ref);
	switch (dblk.db_type) {
		case M_DATA: fprintf(fp, "data     "); break;
		case M_PROTO: fprintf(fp, "proto    "); break;
		case M_BREAK: fprintf(fp, "break    "); break;
		case M_PASSFP: fprintf(fp, "passfs   "); break;
		case M_SIG: fprintf(fp, "sig      "); break;
		case M_DELAY: fprintf(fp, "delay    "); break;
		case M_CTL: fprintf(fp, "ctl      "); break;
		case M_IOCTL: fprintf(fp, "ioctl    "); break;
		case M_SETOPTS: fprintf(fp, "setopts  "); break;
		case M_IOCACK: fprintf(fp, "iocack   "); break;
		case M_IOCNAK: fprintf(fp, "iocnak   "); break;
		case M_PCPROTO: fprintf(fp, "pcproto  "); break;
		case M_PCSIG: fprintf(fp, "pcsig    "); break;
		case M_FLUSH: fprintf(fp, "flush    "); break;
		case M_STOP: fprintf(fp, "stop     "); break;
		case M_START: fprintf(fp, "start    "); break;
		case M_HANGUP: fprintf(fp, "hangup   "); break;
		case M_ERROR: fprintf(fp, "error    "); break;
		case M_STOPI: fprintf(fp, "stopi    "); break;
		case M_STARTI: fprintf(fp, "starti    "); break;
		case M_UNHANGUP: fprintf(fp, "unhangup    "); break;
		default: fprintf(fp, " -       ");
	}
	if (dblk.db_base)
		fprintf(fp, "%8x ", dblk.db_base);
	else fprintf(fp, "       - ");
	if (dblk.db_lim)
		fprintf(fp, "%8x ", dblk.db_lim);
	else fprintf(fp, "       - ");
	if (dblk.db_frtnp)
		fprintf(fp, "%8x\n", dblk.db_frtnp);
	else fprintf(fp, "       -\n");
	return(dblk.db_freep);
}

/* get arguments for dbfree function */
int
getdbfree()
{
	int c;

	optind = 1;
	blockinit();
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "DBLKADDR  SIZE  RCNT TYPE         BASE    LIMIT    FREEP\n");
	prdbfree();
}

/* print dblock free list */
int
prdbfree()
{
	dblk_t *d;

	d = (dblk_t *)dbfreev;
	while (d)
		d = prdblk(1, d, 0, 1);
}

/* get arguments for qrun function */
int
getqrun()
{
	int c;

	if (!Qhead)
		if (!(Qhead = symsrch("qhead")))
			error("qhead not found in symbol table\n");
	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	prqrun();
}

/* print qrun information */
int
prqrun()
{
	queue_t que, *q;

	readsym("qhead", &q, sizeof(queue_t *));
	fprintf(fp, "Queue slots scheduled for service: ");
	while (q) {
		fprintf(fp, "%8x ", q);
		readmem((long)q, 1, -1, (char *)&que,
			sizeof que, "scanning queue list");
		q = que.q_link;
	}
	fprintf(fp, "\n");
}


/* initialization for namelist symbols */
int
streaminit()
{
	static int strinit = 0;

	if (strinit)
		return;
	readsym("allstream", &streamv, sizeof(int));

	readsym("nstrbufsz", &nstrbufszv, sizeof(int));
	readsym("strbufstat", &strbufstatv, sizeof(int));
	
	if (!Strst)
		if (!(Strst = symsrch("strst")))
			error("strst not found in symbol table\n");
	if (!Strbufsizes)
		if (!(Strbufsizes = symsrch("strbufsizes")))
			error("strbufsizes not found in symbol table\n");
	if (!Mbfree)
		if (!(Mbfree = symsrch("mbfreelist")))
			error("mbfreelist not found in symbol table\n");
	if (!Dbfree)
		if (!(Dbfree = symsrch("dbfreelist")))
			error("dbfreelist not found in symbol table\n");
	if (!Qhead)
		if (!(Qhead = symsrch("qhead")))
			error("qhead not found in symbol table\n");
	if (!Strst)
		if (!(Strst = symsrch("strst")))

	strinit = 1;
}


/* get arguments for strstat function */
int
getstrstat()
{
	int c;

	streaminit();
	optind = 1;
	while ((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	prstrstat();
}

/*
 * Streams allocation statistics code.
 */
struct strstat	strst;
alcdat		*strbufstat;

/* print stream statistics */
int
prstrstat()
{
	int i;
	register u_int size;
	register struct strstat *s = &strst;
	register u_int *strbufsizes;
	register alcdat *sbs;
	char buf[50];


	fprintf(fp, "ITEM             ");
	fprintf(fp, "CONFIG   ALLOCATED         TOTAL         MAX      FAIL\n");

	/*
	 * Maximize the probability of getting an internally consistent
	 * snapshot by gathering all data before printing anything.
	 */
	if (nstrbufszv == 0 || strbufstatv == 0)
		return;

	/*
	 * Grab the strst structure (which contains overall STREAMS
	 * statistics).
	 */
	readmem((long)Strst->n_value, 1, -1, (char *)&strst, sizeof strst,
	    "strst table");

	/*
	 * Allocate space for the strbufsizes array and pull it in.
	 */
	size = sizeof *strbufsizes * nstrbufszv;
	strbufsizes = (u_int *) malloc(size);
	if (strbufsizes == NULL) {
		error("bad malloc of strbufsizes\n");
		return;
	}
	readmem((long)Strbufsizes->n_value, 1, -1, (char *)strbufsizes,
	    size, "strbufsizes array");

	/*
	 * Allocate space for the strbufstat array and pull it in.  Note that
	 * it contains two extra entries at the beginning for external buffers
	 * and buffers embedded in dblks.  Also, since the kernel itself
	 * dynamically allocates space for this array, we have to go through
	 * an extra level of indirection here.
	 */
	size = sizeof *sbs * (2 + nstrbufszv);
	sbs = (alcdat *) malloc(size);
	if (sbs == NULL) {
		error("bad malloc of strbfstat array\n");
		return;
	}	
	readbuf(strbufstatv, strbufstatv, 0, -1, (char *)sbs,
	    size, "strbufstat array");

	/*
	 * Display overall STREAMS data.
	 */

	pf_strstat("streams", &s->stream);
	pf_strstat("queues", &s->queue);
	pf_strstat("mblocks", &s->mblock);
	pf_strstat("dblocks", &s->dblock);

	/*
	 * Display buffer-related data.
	 */
	fprintf(fp, "streams buffers:\n");
	pf_strstat("external", &sbs[0]);
	pf_strstat("within-dblk", &sbs[1]);
	for (i = 0; i < nstrbufszv - 1; i++) {
		(void) sprintf(buf, "size <= %5d", strbufsizes[i]);
		pf_strstat(buf, &sbs[i + 2]);
	}
	(void) sprintf(buf, "size >  %5d", strbufsizes[nstrbufszv - 2]);
	pf_strstat(buf, &sbs[nstrbufszv + 1]);
}

/*
 * Print a line of streams allocation information, as recorded
 * in the (alcdat *) given as argument.
 */
pf_strstat(label, alp)
char *label;
alcdat *alp;
{
	fprintf(fp, "%s%*s      %6d       %7d      %6d     %5d\n",
		label,
		23 - strlen(label), "-", /* Move to column 24 */
		alp->use, alp->total, alp->max, alp->fail);
}

/* get arguments for linkblk function */
int
getlinkblk()
{
	int c;
	int slot = -1;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg2 = -1;
	long arg1 = -1;
	int nmuxlink;

	if (!Linkblk)
		if (!(Linkblk = symsrch("linkblk")))
			error("linkblk not found in symbol table\n");
	if (!Nmuxlink)
		if (!(Nmuxlink = symsrch("nmuxlink")))
			error("nmuxlink not found in symbol table\n");
	optind = 1;
	while ((c = getopt(argcnt, args, "epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	readmem((long)Nmuxlink->n_value, 1, -1, (char *)&nmuxlink,
		sizeof nmuxlink, "linkblk table size");
	fprintf(fp, "LINKBLK TABLE SIZE = %d\n", nmuxlink);
	fprintf(fp, "SLOT   QTOP     QBOT   INDEX\n");
	if (args[optind]) {
		all = 1;
		do {
			getargs(nmuxlink, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prlinkblk(all, slot, phys, addr, nmuxlink);
			else {
				if (arg1 < nmuxlink)
					slot = arg1;
				else addr = arg1;
				prlinkblk(all, slot, phys, addr, nmuxlink);
			}
			slot = addr = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else for (slot = 0; slot < nmuxlink; slot++)
		prlinkblk(all, slot, phys, addr, nmuxlink);	
}

/* print linkblk table */
int
prlinkblk(all, slot, phys, addr, max)
int all, slot, phys, max;
long addr;
{
	struct linkblk linkbuf;

	readbuf(addr, (long)(Linkblk->n_value + slot * sizeof linkbuf), phys, -1,
		(char *)&linkbuf, sizeof linkbuf, "linkblk table");
	if (!linkbuf.l_qtop && !all)
		return;
	if (addr > -1)
		slot = getslot(addr, (long)Linkblk->n_value, sizeof linkbuf, phys,
			max);
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
		fprintf(fp, " %8x", linkbuf.l_qtop);
		fprintf(fp, " %8x", linkbuf.l_qbot);
		fprintf(fp, "   %3d\n", linkbuf.l_index);
}
