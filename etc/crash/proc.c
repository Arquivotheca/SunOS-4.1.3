#ifndef lint
static	char sccsid[] = "@(#)proc.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:proc.c	1.15.6.3"
/*
 * This file contains code for the crash functions:  proc, defproc.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/user.h"
#include "sys/proc.h"
#include "sys/time.h"
#include "sys/ucred.h"
#include "crash.h"

#define min(a, b) (a>b? b:a)
#define SULOAD 0

extern int active;
extern struct nlist *Proc, *_Proc;	/* namelist symbol pointers */
unsigned nproc, procv;

/* get arguments for proc function */
int
getproc()
{
	int slot = Procslot;
	int full = 0;
	int phys = 0;
	int run = 0;
	unsigned addr = -1;
	unsigned arg1 = -1;
	unsigned arg2 = -1;
	int id = -1;
	int c;
	char *heading = "SLOT ST PID   PPID  PGRP   UID PRI CPU   EVENT     NAME        FLAGS\n";

	optind = 1;
	while((c = getopt(argcnt, args, "fprw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'w' :	redirect();
					break;
			case 'r' :	run = 1;
					break;
			case 'p' :	phys = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	fprintf(fp, "PROC TABLE SIZE = %d\n", nproc);
	if (!full)
		fprintf(fp, "%s", heading);
	if (args[optind]) {
		do {
			if (*args[optind] == '#') {
				if ((id = strcon(++args[optind], 'd')) == -1)
					error("\n");
				prproc(full, slot, id, phys, run, addr, heading);
			}
			else {
				getargs(nproc, &arg1, &arg2);
				if (arg1 == -1) 
					continue;
				if (arg2 != -1)
					for (slot = arg1; slot <= arg2; slot++)
						prproc(full, slot, id, phys,
							run, addr, heading);
				else {
					if (arg1 < nproc)
						slot = arg1;
					else addr = arg1;
					prproc(full, slot, id, phys, run, addr,
						heading);
				}
			}
			id = slot = addr = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else for (slot = 0; slot < nproc; slot++)
		prproc(full, slot, id, phys, run, addr, heading);
}

/* print proc table */
int
prproc(full, slot, id, phys, run, addr, heading)
int full, slot, id, phys, run;
unsigned addr;
char *heading;
{
	char ch, *typ;
	char cp[MAXCOMLEN+1];
	struct proc procbuf;
	struct user *ubp;
	struct rusage usage;
	struct ucred cred;
	int i, j, cnt;

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
	else readbuf(addr, procv+slot*sizeof procbuf, phys, -1,
		(char *)&procbuf, sizeof procbuf, "proc table");
	if (!procbuf.p_stat)
		return;
	if (run)
		if (!(procbuf.p_stat == SRUN))
			return;
	if (addr > -1) 
		slot = getslot(addr, procv, sizeof procbuf, phys, nproc);
	if (full)
		fprintf(fp, "%s", heading);
	switch (procbuf.p_stat) {
		case NULL:   ch = ' '; break;
		case SSLEEP: ch = 's'; break;
		case SWAIT:  ch = 'w'; break;
		case SRUN:   ch = 'r'; break;
		case SIDL:   ch = 'i'; break;
		case SZOMB:  ch = 'z'; break;
		case SSTOP:  ch = 't'; break;
		default:     ch = '?'; break;
	}
	if (slot == -1)
		fprintf(fp, "  - ");
	else fprintf(fp, "%4d", slot);
	fprintf(fp, " %c %5u %5u %5u %5u %3u %3u",
		ch,
		procbuf.p_pid,
		procbuf.p_ppid,
		procbuf.p_pgrp,
		procbuf.p_uid,
		procbuf.p_pri,
		procbuf.p_cpu & 0xff);
	fprintf(fp, " %8x ", procbuf.p_wchan);
	for (i = 0; i < MAXCOMLEN+2; i++)
		cp[i] = '\0';
	if (procbuf.p_stat == SZOMB)
		strcpy(cp, "<defunct>");
	if (!active && ((procbuf.p_flag & SLOAD) != SLOAD))
		strcpy(cp, "<swapped>");
	ubp = kvm_getu(kd, &procbuf);
	if (ubp) {
		strcpy(cp, ubp->u_comm);
		for(i = 0; i < 8 && cp[i]; i++) {
			if (cp[i] < 040 || cp[i] > 0176) {
				strcpy(cp, "<unprint>");
				break;
			}
		}
	}
	fprintf(fp, "%-14s", cp);
	fprintf(fp, "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n",
		procbuf.p_flag & SLOAD ? " load" : "",
		(procbuf.p_flag & (SLOAD|SULOAD)) == SULOAD ? " swapped" : "",
		procbuf.p_flag & SSYS ? " sys" : "",
		procbuf.p_flag & SLOCK ? " lock" : "",
		procbuf.p_flag & SSWAP ? " swap" : "",
		procbuf.p_flag & STRC ? " trc" : "",
 		procbuf.p_flag & SWTED ? " wted" : "",
 		procbuf.p_flag & SULOCK ? " ulock" : "",
 		procbuf.p_flag & SPAGE ? " page" : "",
 		procbuf.p_flag & SKEEP ? " keep" : "",
 		procbuf.p_flag & SOMASK ? " omask" : "",
 		procbuf.p_flag & SWEXIT ? " wexit" : "",
 		procbuf.p_flag & SPHYSIO ? " physio" : "",
 		procbuf.p_flag & SVFORK ? " vfork" : "",
 		procbuf.p_flag & SVFDONE ? " vfdone" : "",
		procbuf.p_flag & SNOVM ? " novm" : "",
		procbuf.p_flag & SPAGI ? " pagi" : "",
		procbuf.p_flag & SSEQL ? " seql" : "",
		procbuf.p_flag & SUANOM ? " uanom" : "",
		procbuf.p_flag & STIMO ? " timo" : "",
		procbuf.p_flag & SNOCLDSTOP ? " nocldstop" : "",
		procbuf.p_flag & STRACNG ? " tracng" : "",
		procbuf.p_flag & SOWEUPC ? " oweupc" : "",
		procbuf.p_flag & SSEL ? " sel" : "",
		procbuf.p_flag & SLOGIN ? " login" : "",
		procbuf.p_flag & SFAVORD ? " favord" : "",
		procbuf.p_flag & SLKDONE ? " lkdone" : "",
		procbuf.p_flag & STRCSYS ? " trcsys" : "");
	if (!full)
		return;
 	fprintf(fp, "\ttime: %d, slptime: %d, nice: %d, exit %d\n",
		procbuf.p_time,
		procbuf.p_slptime,
		procbuf.p_nice,  
		procbuf.p_xstat);  
 	fprintf(fp, "\tusrpri: %d, cpticks: %d, pctcpu: %d\n",
		procbuf.p_usrpri,
		procbuf.p_cpticks,
		procbuf.p_pctcpu);  
	fprintf(fp, "\tsig: %x, cursig: %d, suid: %d, sgid: %d\n",
		procbuf.p_sig,
		procbuf.p_cursig,
		procbuf.p_suid,
		procbuf.p_sgid);
	fprintf(fp, "\tsigmask: %x, sigignore: %x, sigcatch: %x\n",
		procbuf.p_sigmask,
		procbuf.p_sigignore,
		procbuf.p_sigcatch);
	fprintf(fp, "\ttsize: %d, dsize: %d, ssize: %d, rssize: %d, maxrss: %d, swrss: %d\n",
		procbuf.p_tsize,
		procbuf.p_dsize,
		procbuf.p_ssize,
		procbuf.p_rssize,
		procbuf.p_maxrss,
		procbuf.p_swrss);
	fprintf(fp, "\tlink: %x\trlink: %x\tnxt: %x\tprev: %x\n",
		procbuf.p_link,
		procbuf.p_rlink,
		procbuf.p_nxt,
		procbuf.p_prev);
	fprintf(fp, "\tparent: %x\tchild: %x\n",
		procbuf.p_pptr,
		procbuf.p_cptr);
	fprintf(fp, "\tolder sibling: %x\tyounger sibling: %x\n",
		procbuf.p_osptr,
		procbuf.p_ysptr);
	fprintf(fp, "\ttracer: %x\tidhash: %x\tswlocks: %d\n",
		procbuf.p_tptr,
		procbuf.p_idhash,
		procbuf.p_swlocks);
	if (procbuf.p_nxt)
		fprintf(fp, "\tnxt: %d\n",
		((unsigned)procbuf.p_nxt - procv) /sizeof (procbuf));
	fprintf(fp, "\ttimer interval: %d secs %d usecs timer value: %d secs %d usecs\n",
		procbuf.p_realtimer.it_interval.tv_sec, procbuf.p_realtimer.it_interval.tv_usec,
		procbuf.p_realtimer.it_value.tv_sec, procbuf.p_realtimer.it_value.tv_usec);
	fprintf(fp, "\taddress space: %x\n", procbuf.p_as);
	fprintf(fp, "\tsegu ptr: %x stack: %x uarea: %x\n",
		procbuf.p_segu,
		procbuf.p_stack,
		procbuf.p_uarea);
	fprintf(fp, "\tnumber threads: %d\n", procbuf.p_threadcnt);
	if (kvm_read(kd, (unsigned)procbuf.p_ru, &usage, sizeof usage) == sizeof usage) {
		fprintf(fp, "\tresource usage (at %x):\n", procbuf.p_ru);
		fprintf(fp, "\tuser time: %d secs %d usecs system time: %d secs %d usecs\n",
			usage.ru_utime.tv_sec, usage.ru_utime.tv_usec,
			usage.ru_stime.tv_sec, usage.ru_stime.tv_usec);
		fprintf(fp, "\tmax rss: %d\n\tint shared size: %d\tint unsh size: %d\tint stack size: %d\n",
			usage.ru_maxrss,
			usage.ru_ixrss,
			usage.ru_idrss,
			usage.ru_isrss);
		fprintf(fp, "\treclaims: %d\tpagefaults: %d\tswaps: %d\n",
			usage.ru_minflt,
			usage.ru_majflt,
			usage.ru_nswap);
		fprintf(fp, "\tblock inputs: %d\tblock outputs: %d\n",
			usage.ru_inblock,
			usage.ru_oublock);
		fprintf(fp, "\tmsg sent: %d\tmsg recvd: %d\n",
			usage.ru_msgsnd,
			usage.ru_msgrcv);
		fprintf(fp, "\tsignals recvd: %d\tvol cxt swtch: %d\tinvol cxt swtch: %d\n",
			usage.ru_nsignals,
			usage.ru_nvcsw,
			usage.ru_nivcsw);
	}
	if (kvm_read(kd, (unsigned)procbuf.p_cred, &cred, sizeof cred) == sizeof cred) {
		fprintf(fp, "\tcredentials (at %x):\n", procbuf.p_cred);
		fprintf(fp, "\tref: %d\n", cred.cr_ref);
		fprintf(fp, "\tuid: %d\tgid: %d\treal uid: %d\treal gid: %d\n",
			cred.cr_uid,
			cred.cr_gid,
			cred.cr_ruid,
			cred.cr_rgid);
		fprintf(fp, "\tauid: %d\taudit success: %d\taudit failure: %d\n",
			cred.cr_auid,
			cred.cr_audit.as_success,
			cred.cr_audit.as_failure);
		fprintf(fp, "\tgroups:");
		for (i = 0; i < NGROUPS; i++) {
			fprintf(fp, "%4d ", cred.cr_groups[i]);
			if ((i % 8) == 7)  {
				fprintf(fp, "\n");
				if (i < (NGROUPS - 1)) fprintf(fp, "\t       ");
			}
		}
		fprintf(fp, "\tsecurity: lvl %d\n", cred.cr_label.sl_level);
		fprintf(fp, "\tcategories:");
		for (i = 0; i < 16; i++) {
			fprintf(fp, "%3d ", cred.cr_label.sl_categories[i]);
			if ((i % 8) == 7) {
				fprintf(fp, "\n");
				if (i < 15) fprintf(fp, "\t           ");
			}
		}
	}
}

/* get arguments for defproc function */
int
getdefproc()
{
	int c;
	int proc = -1;
	int reset = 0;

	optind = 1;
	while((c = getopt(argcnt, args, "cw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'c' :	reset = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) 
		if ((proc = strcon(args[optind], 'd')) == -1)
			error("\n");
	prdefproc(proc, reset);
}

/* print results of defproc function */
int
prdefproc(proc, reset)
int proc, reset;
{

	if (reset)
		Procslot = getcurproc();
	else if (proc > -1) {
		if ((proc > nproc) || (proc < 0))
			error("%d out of range\n", proc);
		Procslot = proc;
	}
	fprintf(fp, "Procslot = %d\n", Procslot);
}
