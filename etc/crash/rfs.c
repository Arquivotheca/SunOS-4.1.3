#ifndef lint
static	char sccsid[] = "@(#)rfs.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	 All Rights Reserved 	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any  	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:rfs.c	1.16"
/*
* This file contains code for the crash functions: adv, gdp, rcvd, sndd.
*/

#include "stdio.h"
#include "sys/param.h"
#include "a.out.h"
#include "sys/types.h"
#include "sys/file.h"
#include "sys/proc.h"
#include "sys/stat.h"
#include "rfs/rfs_misc.h"
#include "rfs/sema.h"
#define RFS
#include "sys/mount.h"
#include "rfs/comm.h"
#include "rfs/cirmgr.h"
#include "sys/mount.h"
#include "rfs/nserve.h"
#include "rfs/adv.h"
#include "sys/var.h"
#include "sys/stream.h"
#include "crash.h"

int advv, rcvdv, gdpv, snddv;
static struct nlist *Nadv;		/* pointers	*/
struct nlist *Nrcvd,*Nsndd, *Maxgdp;
extern int nmblock, procv, nproc;

/* check for rfs activity */
int
checkboot()
{
	int bootstate;
	struct nlist *boot;

	if (!(boot = symsrch("bootstate"))) 
		error("bootstate not found in symbol table\n");

	readmem((long)boot->n_value, 1, -1, (char *)&bootstate,
		sizeof bootstate, "bootstate");

	if (bootstate != DU_UP) {
		if (bootstate == DU_DOWN)
			prerrmes("rfs not started\n\n");
		else prerrmes("rfs in process of starting\n\n");
	}
}


/* get arguments for adv function */
int
getadv()
{
	int slot = -1;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	int nadv;

	readsym("advertise", &advv, sizeof(int));
	if (!Nadv)
		if (!(Nadv = symsrch("nadvertise"))) 
			error("cannot determine size of advertise table\n");
	readmem((long)Nadv->n_value, 1, -1, (char *)&nadv,
		sizeof nadv, "size of advertise table");
	readsym("rcvd", &rcvdv, sizeof(int));
	if (!Nrcvd)
		if (!(Nrcvd = symsrch("nrcvd"))) 
			error("cannot determine size of receive descriptor table\n");
	optind = 1;
	while((c = getopt(argcnt, args, "epw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default :	longjmp(syn, 0);
		}
	}
	checkboot();
	fprintf(fp, "ADVERTISE TABLE SIZE = %d\n", nadv);
	fprintf(fp, "SLOT  CNT       NAME     RCVD    CLIST FLAGS\n");
	if (args[optind]) {
		all = 1;
		do {
			getargs(nadv, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					pradv(all, slot, phys, addr);
			else {
				if (arg1 < nadv)
					slot = arg1;
				else addr = arg1;
				pradv(all, slot, phys, addr);
			}
			slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < nadv; slot++)
		pradv(all, slot, phys, addr);
}


/* print advertise table */
int
pradv(all, slot, phys, addr)
int all, slot, phys;
long addr;
{
	struct advertise advbuf;
	int nrcvd;

	readbuf(addr, (long)(advv + slot * sizeof advbuf), phys, -1,
		(char *)&advbuf, sizeof advbuf, "advertise table");
	readmem((long)Nrcvd->n_value, 1, -1, (char *)&nrcvd,
		sizeof nrcvd, "size of receive descriptor table");
	if ((advbuf.a_flags == A_FREE) && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, (long)advv, sizeof advbuf, phys);
	fprintf(fp, "%4d %4u %-14s",
		slot,
		advbuf.a_count,
		advbuf.a_name);
	slot = ((long)advbuf.a_queue - (long)rcvdv) / sizeof (struct rcvd);
	if ((slot >= 0) && (slot < nrcvd))
		fprintf(fp, " %4d", slot);
	else fprintf(fp, "  - ");
	fprintf(fp, " %8x  ", advbuf.a_clist);
	fprintf(fp, " %s%s%s",
		advbuf.a_flags & A_RDONLY ? " ro" : " rw",
		advbuf.a_flags & A_CLIST ? " cl" : "",
		advbuf.a_flags & A_MODIFY ? " md" : "");
	if (all)
		fprintf(fp, "%s", advbuf.a_flags & A_INUSE ? " use" : "");
	fprintf(fp, "\n");
}

/* get arguments for gdp function */
int
getgdp()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	char *heading = "SLOT    QUEUE     FILE MNT SYSID FLAG\n";
	int maxgdp;

	readsym("gdp", &gdpv, sizeof(int));
	if (!Maxgdp)
		if (!(Maxgdp = symsrch("maxgdp"))) 
			error("cannot determine size of gift descriptor table\n");
	readmem((long)Maxgdp->n_value, 1, -1, (char *)&maxgdp,
		sizeof maxgdp, "size of gift descriptor table");
	optind = 1;
	while((c = getopt(argcnt, args, "efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'p' :	phys = 1;
					break;
			case 'w' :	redirect();
					break;
			default :	longjmp(syn, 0);
		}
	}
	checkboot();
	fprintf(fp, "GDP MAX SIZE = %d\n", maxgdp);
	if (!full)
		fprintf(fp, "%s", heading);
	if (args[optind]) {
		all = 1;
		do {
			getargs(maxgdp, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prgdp(full, all, slot, phys, addr, heading);
			else {
				if (arg1 < maxgdp)
					slot = arg1;
				else addr = arg1;
				prgdp(full, all, slot, phys, addr, heading);
			}
			slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < maxgdp; slot++)
		prgdp(full, all, slot, phys, addr, heading);
}

/* print gdp table */
int
prgdp(full, all, slot, phys, addr, heading)
int full, all, slot, phys;
long addr;
char *heading;
{
	struct gdp gdpbuf;
	char temp[MAXDNAME + 1];

	readbuf(addr, (long)(gdpv + slot * sizeof gdpbuf), phys, -1,
		(char *)&gdpbuf, sizeof gdpbuf, "gdp structures");
	if (!gdpbuf.queue && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, (long)gdpv, sizeof gdpbuf, phys);
	if (full)
		fprintf(fp, "%s", heading);
	fprintf(fp, "%4d", slot);
	fprintf(fp, " %8x %8x  %2d  %4x %s%s%s\n",
		gdpbuf.queue,
		gdpbuf.file,
		gdpbuf.mntcnt,
		gdpbuf.sysid,
		(gdpbuf.flag & GDPDISCONN) ? " dis" : "",
		(gdpbuf.flag & GDPRECOVER) ? " rec" : "",
		(gdpbuf.flag & GDPCONNECT) ? " con" : "");
	if (full) {
		fprintf(fp, "\tHET VER   UID   GID       TIME\n");
		fprintf(fp, "\t%3x  %2d %5d %5d %10d\n",
			gdpbuf.hetero,
			gdpbuf.version,
			gdpbuf.idmap[0],
			gdpbuf.idmap[1],
			gdpbuf.time);
		strncpy(temp, gdpbuf.token.t_uname, MAXDNAME);
		fprintf(fp, "\tTOKEN ID: %4x  TOKEN NAME: %s\n",
			gdpbuf.token.t_id,
			temp);
		fprintf(fp, "\tmsg header: %8x idata: %8x hlen: %5d dlen: %5d\n",
			gdpbuf.hdr,
			gdpbuf.idata,
			gdpbuf.hlen,
			gdpbuf.dlen);
		fprintf(fp, "\tmax tidu size: %5d\n", gdpbuf.maxpsz);
		fprintf(fp, "\n");
	}
}

/* get arguments for rcvd function */
int
getrcvd()
{
	int slot = -1;
	int full = 0;
	int all = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	int nrcvd;
	char *heading = "SLOT ACNT QTYPE I/SD FILE QCNT RCNT STATE\n";

	readsym("rcvd", &rcvdv, sizeof(int));
	if (!Nrcvd)
		if (!(Nrcvd = symsrch("nrcvd"))) 
			error("cannot determine size of receive descriptor table\n");
	readsym("sndd", &snddv, sizeof(int));
	if (!Nsndd)
		if (!(Nsndd = symsrch("nsndd"))) 
			error("cannot determine size of send descriptor table\n");
	blockinit();
	optind = 1;
	while((c = getopt(argcnt, args, "efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default :	longjmp(syn, 0);
		}
	}
	checkboot();
	readmem((long)Nrcvd->n_value, 1, -1, (char *)&nrcvd,
		sizeof nrcvd, "size of receive descriptor table");
	fprintf(fp, "RECEIVE DESCRIPTOR TABLE SIZE = %d\n", nrcvd);
	if (!full)
		fprintf(fp, "%s", heading);
	if (args[optind]) {
		all = 1;
		do {
			getargs(nrcvd, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prrcvd(all, full, slot, phys, addr,
						heading, nrcvd);
			else {
				addr = arg1;
				prrcvd(all, full, slot, phys, addr, heading, nrcvd);
			}
			slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < nrcvd; slot++)
		prrcvd(all, full, slot, phys, addr, heading, nrcvd);
}

/* print rcvd table */
int
prrcvd(all, full, slot, phys, addr, heading, size)
int all, full, slot, phys, size;
long addr;
char *heading;
{
	struct rcvd rcvdbuf;
	struct rd_user userbuf;
	struct rd_user *next;
	int nsndd;

	readbuf(addr, (long)(rcvdv + slot * sizeof rcvdbuf), phys, -1,
		(char *)&rcvdbuf, sizeof rcvdbuf, "received descriptor table");
	readmem((long)Nsndd->n_value, 1, -1, (char *)&nsndd,
		sizeof nsndd, "size of send descriptor table");
	if (((rcvdbuf.rd_stat == 0) || (rcvdbuf.rd_stat & RDUNUSED)) && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, (long)rcvdv, sizeof rcvdbuf, phys);
	if (full) {
		streaminit();
    	fprintf(fp, "%s", heading);
	}
	fprintf(fp, "%4d %4u   %c",
		slot,
		rcvdbuf.rd_act_cnt,
		(rcvdbuf.rd_qtype & GENERAL) ? 'G' : 'S');
		fprintf(fp, " %s",
			(rcvdbuf.rd_qtype & RDTEXT) ? "text" : "  ");
	fprintf(fp, " %8x %4u %4u",
		rcvdbuf.rd_file,
		rcvdbuf.rd_qcnt,
		rcvdbuf.rd_refcnt);
	fprintf(fp, " %s%s",
		(rcvdbuf.rd_stat & RDUSED) ? " used" : "",
		(rcvdbuf.rd_stat & RDLINKDOWN) ? " ldown" : "");
	fprintf(fp, "\n");
	if (full) {
		fprintf(fp, "\tMSERVE QSIZE CONID SNDD NEXT     QSLP    MTIME    RHEAD    RTAIL\n");
		fprintf(fp, "\t %5d %5d %5u",
			rcvdbuf.rd_max_serv,
			rcvdbuf.rd_qsize,
			rcvdbuf.rd_connid);
		slot = ((long)rcvdbuf.rd_sdnack - (long)snddv) /
			sizeof (struct sndd);
		if ((slot >= 0) && (slot < nsndd))
			fprintf(fp, " %4d", slot);
		else fprintf(fp, " - ");
		slot = ((long)rcvdbuf.rd_next - (long)rcvdv) /
			sizeof (struct rcvd);
		if ((slot >= 0) && (slot < size))
			fprintf(fp, "   %4d", slot);
		else fprintf(fp, "     - ");
		fprintf(fp, " %8x %8x", rcvdbuf.rd_qslp, rcvdbuf.rd_mtime);
		fprintf(fp, " %8x %8x", rcvdbuf.rd_rcvdq.qc_head,
			rcvdbuf.rd_rcvdq.qc_tail);
		if (rcvdbuf.rd_user_list) {
			fprintf(fp, "\n\tUSER_LIST: %8x", rcvdbuf.rd_user_list);
			if (rcvdbuf.rd_qtype & GENERAL) {
				next = rcvdbuf.rd_user_list;
				if (next)
					fprintf(fp, "\n\t\t   QUEUE SRMNT  ICNT  FCNT  RCNT  WCNT   NEXT\n");
				while(next) {
					readmem((long)next, 1, -1, (char *)&userbuf,
						sizeof userbuf, "user list");
					fprintf(fp, "\t\t%8x %5d",
						userbuf.ru_queue,
						userbuf.ru_srmntindx);
					fprintf(fp, " %5d %5d %5d %5d",
						userbuf.ru_icount,
						userbuf.ru_fcount,
						userbuf.ru_frcnt,
						userbuf.ru_fwcnt);
					next = userbuf.ru_next;
					if (next)
						fprintf(fp, " %8x\n", next);
				else fprintf(fp, "   -  \n");
				}
			}
			else fprintf(fp, "\n");
		}
		else fprintf(fp, "\n");
		fprintf(fp, "\n");
	}
}

/* get arguments for sndd function */
int
getsndd()
{
	int slot = -1;
	int all = 0;
	int full = 0;
	int phys = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	int nsndd;

	readsym("sndd", &snddv, sizeof(int));
	if (!Nsndd)
		if (!(Nsndd = symsrch("nsndd"))) 
			error("cannot determine size of send descriptor table\n");
	optind = 1;
	while((c = getopt(argcnt, args, "efpw:")) !=EOF) {
		switch(c) {
			case 'e' :	all = 1;
					break;
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			default :	longjmp(syn, 0);
		}
	}
	checkboot();
	readmem((long)Nsndd->n_value, 1, -1, (char *)&nsndd,
		sizeof nsndd, "size of send descriptor table");
	fprintf(fp, "SEND DESCRIPTOR TABLE SIZE = %d\n", nsndd);
	fprintf(fp, "SLOT RCNT SNDX MNDX CONID COPY PROC   SQUE NEXT   MODE STATE\n");
	if (args[optind]) {
		all = 1;
		do {
			getargs(nsndd, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					prsndd(all, full, slot, phys, addr, nsndd);
			else {
				if (arg1 < nsndd)
					slot = arg1;
				else addr = arg1;
				prsndd(all, full, slot, phys, addr, nsndd);
			}
			slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < nsndd; slot++)
		prsndd(all, full, slot, phys, addr, nsndd);
}

/* print sndd function */
int
prsndd(all, full, slot, phys, addr, size)
int all, full, slot, phys, size;
long addr;
{
	struct sndd snddbuf;

	readbuf(addr, (long)(snddv + slot * sizeof snddbuf), phys, -1,
		(char *)&snddbuf, sizeof snddbuf, "send descriptor table");
	if (((snddbuf.sd_stat == 0) || (snddbuf.sd_stat & SDUNUSED)) && !all)
		return;
	if (addr > -1) 
		slot = getslot(addr, (long)snddv, sizeof snddbuf, phys);
	fprintf(fp, "%4d %4u %4u %4u %5u %4u",
		slot,
		snddbuf.sd_refcnt,
		snddbuf.sd_sindex,
		snddbuf.sd_mntindx,
		snddbuf.sd_connid,
		snddbuf.sd_copycnt);
	slot = ((long)snddbuf.sd_srvproc - (long)procv) / sizeof (struct proc);
	if ((slot >= 0) && (slot < nproc))
		fprintf(fp, " %4d", slot);
	else fprintf(fp, " - ");
	fprintf(fp, " %8x", snddbuf.sd_queue);
	slot = ((long)snddbuf.sd_next - (long)snddv) / sizeof (struct sndd);
	if ((slot >= 0) && (slot < size))
		fprintf(fp, " %4d", slot);
	else fprintf(fp, " - ");
	fprintf(fp, " %s%s%s%03o",
		snddbuf.sd_mode & S_ISUID ? "u" : "-",
		snddbuf.sd_mode & S_ISGID ? "g" : "-",
		snddbuf.sd_mode & S_ISVTX ? "v" : "-",
		snddbuf.sd_mode & 0777);
	fprintf(fp, "%s%s%s",
		(snddbuf.sd_stat & SDUSED) ? " used" : "",
		(snddbuf.sd_stat & SDSERVE) ? " serve" : "",
		(snddbuf.sd_stat & SDLINKDOWN) ? " ldown" : "");
	fprintf(fp, "\n");
	if (full) {
		fprintf(fp, "\t    TEMP  FHANDLE   OFF COUNT VCODE\n");
		fprintf(fp, "\t%8x %8x  %4d  %4d  %4d\n",
			snddbuf.sd_temp,
			snddbuf.sd_fhandle,
			snddbuf.sd_offset,
			snddbuf.sd_count,
			snddbuf.sd_vcode);
	}
}
