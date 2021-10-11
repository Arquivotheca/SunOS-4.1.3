#ifndef lint
static	char sccsid[] = "@(#)u.c 1.1 92/07/30 SMI";
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)crash-3b2:u.c	1.20.6.1"
/*
 * This file contains code for the crash functions:  user, pcb, stack,
 * trace, and kfp.
 */

#include "sys/param.h"
#include "a.out.h"
#include "stdio.h"
#include "signal.h"
#include "sys/sysmacros.h"
#include "sys/types.h"
#include "sys/user.h"
#include "sys/session.h"
#include "sys/acct.h"
/* XXX */
#define KERNEL 
#include "sys/file.h"
#undef KERNEL
#include "sys/proc.h"
#include "sys/time.h"
#include "sys/dirent.h"
#include <sys/vnode.h>
#include <ufs/inode.h>
#include "machine/frame.h"
#include "machine/vmparam.h"
#include "crash.h"
#ifdef sparc
#include "struct.h"
#endif

#define min(a, b) (a>b ? b:a)
#define CALL_INDIRECT -1 /* flag from calltarget for an indirect call */
#define SAVE_INS (5*4)

static is_leaf_proc();
static calltarget();

extern struct user *ubp;		/* ublock pointer */
extern int active;			/* active system flag */
struct sess sess;			/* session (ctty) buffer */
struct proc procbuf;			/* proc entry buffer */
extern	char	*strtbl;		/* pointer to string table */
int	*stk_bptr;			/* stack pointer */
extern	struct	nlist	*Proc, *File,
	*Inode, *Panic;	/* namelist symbol pointers */
extern char *ctime();
extern struct	nlist	*findsym();
extern long lseek();
extern char *malloc();
void free();
int *temp;
extern int nproc, procv;
char *sigstr[4] = {"DFL", "IGN", "CATCH", "HOLD"};
char kstack[KERNSTACK];
int kstack1[KERNSTACK];

/* read ublock and kernel stack into buffer */
int
getublock(slot)
int slot;
{
	
	readmem((long)(procv + slot * sizeof procbuf), 1,
		slot, (char *)&procbuf, sizeof procbuf,
			"proc table");
	kvm_read(kd, procbuf.p_sessp, (char*)&sess, sizeof sess);
	ubp = kvm_getu(kd, &procbuf);
	kvm_read(kd, procbuf.p_stack - KERNSTACK, kstack, KERNSTACK);
	return((ubp == NULL) ? -1 : 0);
}

/* get arguments for user function */
int
getuser()
{
	int slot = Procslot;
	int full = 0;
	int all = 0;
	long arg1 = -1;
	long arg2 = -1;
	int c;

	optind = 1;
	while ((c = getopt(argcnt, args, "efw:")) !=EOF) {
		switch(c) {
			case 'f' :	full = 1;
					break;
			case 'e' :	all = 1;
					break;
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			getargs(nproc, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (slot = arg1; slot <= arg2; slot++)
					pruser(full, slot);
			else pruser(full, arg1);
			slot = arg1 = arg2 = -1;
		} while (args[++optind]);
	}
	else if (all) {
		for (slot = 0; slot < nproc; slot++) 
			pruser(full, slot);
	}
	else pruser(full, slot);
}

/* print ublock */
int
pruser(full, slot)
int full, slot;
{
	register  int  i, j;
	struct ucwd cdir;
	char tmp[MAXNAMLEN+1];
	int filev, getofile = 0, nfiles;
	struct file **ofile;
	char  *pofile;

	if (getublock(slot) == -1)
		return;
	if (slot == -1)
		slot = getcurproc();
	fprintf(fp, "PER PROCESS USER AREA FOR PROCESS %d\n", slot);
	fprintf(fp, "\tcommand: %s", ubp->u_comm);
	fprintf(fp, "\tproc ptr: %x", ubp->u_procp);
	fprintf(fp, "\tsess ptr: %x", procbuf.p_sessp);
	fprintf(fp, "\ttty ptr: %x\n", sess.s_ttyp);
	if (sess.s_ttyp != 0)
		fprintf(fp, "\tcntrl tty: %3u, %-3u\n",
			major(sess.s_ttyd),
			minor(sess.s_ttyd));
	else fprintf(fp, "\tno cntrl tty\n");
	if (ubp->u_start.tv_sec)
		fprintf(fp, "\tstart: %s", ctime(&ubp->u_start.tv_sec));
	if (kvm_read(kd, (unsigned)ubp->u_cwd, cdir, sizeof cdir) == sizeof cdir) {
		fprintf(fp, "\tcurrent directory structure (at %x):\n", ubp->u_cwd);
		fprintf(fp, "\tref %x len %x\n", cdir.cw_ref, cdir.cw_len);
		if (kvm_read(kd, (unsigned)cdir.cw_dir, tmp, sizeof tmp) == sizeof tmp)
			fprintf(fp, "\tcurrent directory: %s", tmp);
		if (kvm_read(kd, (unsigned)cdir.cw_root, tmp, sizeof tmp) == sizeof tmp)
			fprintf(fp, "\troot directory: %s", tmp);
		fprintf(fp, "\n");
	}
	fprintf(fp, "\tvnodes: current directory: %x", ubp->u_cdir);
	fprintf(fp, " root directory: %x", ubp->u_rdir);
	fprintf(fp, " tty: %x", sess.s_vp);

	readsym("file", &filev, sizeof filev);

	if ((int)ubp->u_ofile !=
		((int)procbuf.p_uarea + ((int)ubp->u_ofile_arr - (int)ubp)))
		getofile = 1;
	nfiles = ubp->u_lastfile + 1;
	if (getofile) {
		ofile = (struct file **)malloc(nfiles * sizeof(struct file *));
		pofile = (char *)malloc(nfiles * sizeof(char));
		kvm_read(kd, (unsigned)ubp->u_ofile, ofile, nfiles * sizeof(struct file *));
		kvm_read(kd, (unsigned)ubp->u_pofile, pofile, nfiles * sizeof(char));
	}
	else {
		ofile = ubp->u_ofile_arr;
		pofile = ubp->u_pofile_arr;
	}
	if (ubp->u_lastfile != -1) {
		fprintf(fp, "\n\tOPEN FILES AND POFILE FLAGS:\n");
		for (i = 0, j = 1; i < nfiles; i++) {
			if (ofile[i] != 0) {
				if (!(j++ & 3))
					fprintf(fp, "\n");
				fprintf(fp, "\t[%d]: F#%d, ",
					i,
					((unsigned)ofile[i] -
					filev)/sizeof (struct file));
				fprintf(fp, " %s%s", 
					pofile[i] & UF_EXCLOSE ? "x" : "",
					pofile[i] & UF_FDLOCK ? "l" : "");
			}
		}
	}
	if (getofile) {
		free(ofile);
		free(pofile);
	}
	fprintf(fp, "\n\tofile: %x pofile: %x lastfile: %d cmask: %4.4o\n",
		ubp->u_ofile,
		ubp->u_pofile,
		ubp->u_lastfile,
		ubp->u_cmask);

	fprintf(fp, "SIGNALS:");
	fprintf(fp, "\n\tSIGNAL DISPOSITION:");
	for (i = 0; i < NSIG; i++) {
		register unsigned tmp;

		if (!(i & 3))
			fprintf(fp, "\n\t");
		fprintf(fp, "%4d: ", i+1);
		tmp = (int)ubp->u_signal[i];
		if (tmp < 4) 
			fprintf(fp, "%8s", sigstr[tmp]);
		else fprintf(fp, "%8x", tmp);
	}
	fprintf(fp, "\n\tSIGNAL MASKS:");
	for (i = 0; i < NSIG; i++) {
		if (!(i & 3))
			fprintf(fp, "\n\t");
		fprintf(fp, "%4d: ", i+1);
		fprintf(fp, "%-8x", ubp->u_sigmask[i]);
	}
	fprintf(fp, "\n\tsignals on stack: %x intr syscalls: %x reset: %x\n",
		ubp->u_sigonstack,
		ubp->u_sigintr,
		ubp->u_sigreset);
	fprintf(fp, "\told mask: %x\n", ubp->u_oldmask);
	fprintf(fp, "\ttrap code: %x trap addr: %x\n",
		ubp->u_code,
		ubp->u_addr);
	fprintf(fp, "\tsigstack onstack: %x sigstack sp: %x\n",
		ubp->u_onstack,
		ubp->u_sigsp);
	if (full) {
		fprintf(fp, "\tacct flag: %s%s%s%s%s\n",
			ubp->u_acflag & AFORK ? "fork" : "exec",
			ubp->u_acflag & ASU ? " su-user" : "",
			ubp->u_acflag & ACOMPAT ? " compat" : "",
			ubp->u_acflag & ACORE ? " core" : "",
			ubp->u_acflag & AXSIG ? " killed-by-signal" : "");
		fprintf(fp, "\tlofault: %x error: %d\n",
			ubp->u_lofault,
			ubp->u_error);
		fprintf(fp, "\teosys: %d ar0: %x",
			ubp->u_eosys,
			ubp->u_ar0);
		fprintf(fp, "\tap: %x rval1: %x, rval2: %x\n",
			ubp->u_ap,
			ubp->u_rval1,
			ubp->u_rval2);
		fprintf(fp, "\targ[0]: %8x, arg[1]: %8x, arg[2]: %8x, arg[3]: %8x\n",
			ubp->u_arg[0],
			ubp->u_arg[1],
			ubp->u_arg[2],
			ubp->u_arg[3]);
		fprintf(fp, "\targ[4]: %8x, arg[5]: %8x, arg[6]: %8x, arg[7]: %8x\n",
			ubp->u_arg[4],
			ubp->u_arg[5],
			ubp->u_arg[6],
			ubp->u_arg[7]);
#define rv ubp->u_qsave.val
		fprintf(fp, "\tqsave: pc %8x\n", rv[0]);
#if defined(mc68010) || defined(mc68020)
		fprintf(fp, "\td2 %8x d3 %8x d4 %8x d5 %8x d6 %8x d7 %8x\n", rv[1], rv[2], rv[3], rv[4], rv[5], rv[6]);
		fprintf(fp, "\ta2 %8x a3 %8x a4 %8x a5 %8x a6 %8x sp %8x\n", rv[7], rv[8], rv[9], rv[10], rv[11], rv[12]);
#endif
#ifdef sparc
	fprintf(fp, "\tsp %8x\n", rv[1]);
#endif
#undef rv
#define rv ubp->u_ssave.val
		fprintf(fp, "\tssave: pc %8x\n", rv[0]);
#if defined(mc68010) || defined(mc68020)
		fprintf(fp, "\td2 %8x d3 %8x d4 %8x d5 %8x d6 %8x d7 %8x\n", rv[1], rv[2], rv[3], rv[4], rv[5], rv[6]);
		fprintf(fp, "\ta2 %8x a3 %8x a4 %8x a5 %8x a6 %8x sp %8x\n", rv[7], rv[8], rv[9], rv[10], rv[11], rv[12]);
#endif
#ifdef sparc
	fprintf(fp, "\tsp %8x\n", rv[1]);
#endif
#undef rv
#define usage ubp->u_ru
		fprintf(fp, "\tresource usage (this process):\n");
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
#undef usage
#define usage ubp->u_cru
		fprintf(fp, "\tresource usage (children):\n");
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
#undef usage
		fprintf(fp, "\tinterval timers:\n");
#define tim ubp->u_timer[ITIMER_REAL]
		fprintf(fp, "\treal itimer    interval: %d secs %d usecs timer value: %d secs %d usecs\n",
			tim.it_interval.tv_sec, tim.it_interval.tv_usec,
			tim.it_value.tv_sec, tim.it_value.tv_usec);
#undef tim
#define tim ubp->u_timer[ITIMER_VIRTUAL]
		fprintf(fp, "\tvirtual itimer interval: %d secs %d usecs timer value: %d secs %d usecs\n",
			tim.it_interval.tv_sec, tim.it_interval.tv_usec,
			tim.it_value.tv_sec, tim.it_value.tv_usec);
#undef tim
#define tim ubp->u_timer[ITIMER_PROF]
		fprintf(fp, "\tprofile itimer interval: %d secs %d usecs timer value: %d secs %d usecs\n",
			tim.it_interval.tv_sec, tim.it_interval.tv_usec,
			tim.it_value.tv_sec, tim.it_value.tv_usec);
#undef tim
		fprintf(fp, "\tchars r/w: %x\n", ubp->u_ioch);
		fprintf(fp, "\tpr_base: %x, pr_size: %d, pr_off: %x, pr_scale: %d\n",
			ubp->u_prof.pr_base,
			ubp->u_prof.pr_size,
			ubp->u_prof.pr_off,
			ubp->u_prof.pr_scale);
		fprintf(fp, "\tEXDATA:\n");
		fprintf(fp, "\ttext: %x, data: %x, bss: %x, syms: %x, entry: %x\n",
			ubp->u_exdata.ux_tsize,
			ubp->u_exdata.ux_dsize,
			ubp->u_exdata.ux_bsize,
			ubp->u_exdata.ux_ssize,
			ubp->u_exdata.ux_entloc);
		fprintf(fp, "\tmagic#: %o\tdynamic: %x, toolvers: %x, machtype: %x\n",
			ubp->u_exdata.ux_mag,
			ubp->u_exdata.Ux_A.a_dynamic,
			ubp->u_exdata.Ux_A.a_toolversion,
			ubp->u_exdata.Ux_A.a_machtype);
	}
}

/* get arguments for pcb function */
int
getpcb()
{
	int proc = Procslot;
	int c;

	optind = 1;
	while((c = getopt(argcnt, args, "w:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		if ((proc = strcon(args[optind], 'd')) == -1)
			error("\n");
		if ((proc > nproc) || (proc < 0))
			error("%d out of range\n", proc);
		prpcb(proc);
	}
	else prpcb(proc);
}

/* print pcb */
int
prpcb(proc)
int proc;
{
	register struct pcb *pcbp;
	register i, j;

	if (getublock(proc) == -1)
		return;
	pcbp = (struct pcb *)&ubp->u_pcb;

#define rv pcbp->pcb_regs.val
	fprintf(fp, "\tregisters: pc %8x\n", rv[0]);
#if defined(mc68010) || defined(mc68020)
	fprintf(fp, "\td2 %8x d3 %8x d4 %8x d5 %8x d6 %8x d7 %8x\n", rv[1], rv[2], rv[3], rv[4], rv[5], rv[6]);
	fprintf(fp, "\ta2 %8x a3 %8x a4 %8x a5 %8x a6 %8x sp %8x\n", rv[7], rv[8], rv[9], rv[10], rv[11], rv[12]);
#endif
#ifdef sparc
	fprintf(fp, "\tsp %8x\n", rv[1]);
#endif
#undef rv
#if defined(mc68010) || defined(mc68020)
	fprintf(fp, "\tpsr: %4x\n", pcbp->pcb_sr);
#endif
#ifdef sparc
	fprintf(fp, "\tpsr: %8x\n", pcbp->pcb_psr);
#endif
	fprintf(fp, "\tflags: %8x\n", pcbp->pcb_flags);
#if defined(mc68010) || defined(mc68020)
	fprintf(fp, "\tbus err ptr: %8x user pc: %8x\n",
		pcbp->u_buserr_state.u_buserr_stack,
		pcbp->u_buserr_state.u_buserr_pc);
	if (pcbp->u_fp_status.fps_flags != FPS_UNUSED) {
#define fr pcbp->u_fp_status.fps_regs
		fprintf(fp, "\tfloating point state:\n");
		fprintf(fp, "\tregisters:\n");
		for (i = 0; i < 8; i++) {
			fprintf(fp, "freg%d: %8x%8x%8x\n", i, 
				fr[i].fp[0],
				fr[i].fp[1],
				fr[i].fp[2]);
		}
#undef fr
		fprintf(fp, "\tcontrol: %8x status: %8x iaddr: %8x\n",
			pcbp->u_fp_status.fps_control,
			pcbp->u_fp_status.fps_status,
			pcbp->u_fp_status.fps_iaddr);
		fprintf(fp, "\tstate: %s\n",
			(pcbp->u_fp_status.fps_flags == FPS_IDLE) ? "idle" : "busy");
		fprintf(fp, "\tversion: %x\n", pcbp->u_fp_istate.fpis_vers);
		fprintf(fp, "\tinternal state:");
		for (i = 0; i < pcbp->u_fp_istate.fpis_bufsiz; i++) {
			if (i % 4 == 0) printf(" ");
			if (i % 64 == 0) printf("\n");
			fprintf(fp, "%2x", pcbp->u_fp_istate.fpis_buf[i]);
		}
		fprintf(fp, "\n");
	}
	if (pcbp->u_fpa_flags) {
#define fs pcbp->u_fpa_status
		fprintf(fp, "\tstate %8x imask %8x loadptr %8x ierr %8x\n",
			fs.fpas_state,
			fs.fpas_imask,
			fs.fpas_load_ptr,
			fs.fpas_ierr);
		fprintf(fp, "\tact instr %8x next instr %8x mode %8x wstatus %8x\n",
			fs.fpas_act_instr,
			fs.fpas_nxt_instr,
			fs.fpas_mode3_0,
			fs.fpas_wstatus);
		fprintf(fp, "\tact data %8x%8x next data %8x%8x\n",
			fs.fpas_act_d1half,
			fs.fpas_act_d2half,
			fs.fpas_nxt_d1half,
			fs.fpas_nxt_d2half);
#undef fs
	}
#endif
#ifdef sparc
		fprintf(fp, "\tuwm %8x wbcnt %d wocnt %d wucnt %d\n",
			pcbp->pcb_uwm,
			pcbp->pcb_wbcnt,
			pcbp->pcb_wocnt,
			pcbp->pcb_wucnt);
		for (i = 0; i < pcbp->pcb_wbcnt; i++) {
			fprintf(fp, "\twindow %d:", i);
			fprintf(fp, "\n\tlocals:");
			for (j = 0; j < 8; j++)
				fprintf(fp, " %d: %x8", j, pcbp->pcb_wbuf[i].rw_local[j]);
			fprintf(fp, "\n\tins:");
			for (j = 0; j < 8; j++)
				fprintf(fp, " %d: %8x", j, pcbp->pcb_wbuf[i].rw_in[j]);
			fprintf(fp, "\n\tstack pointer: %x\n", pcbp->pcb_spbuf[i]);
		}
#endif
}

/* get arguments for stack function */
int
getstack()
{
	int proc = Procslot;
	int phys = 0;
	char type = 'k';
	long addr = -1;
	int c;
	struct nlist *sp;

	optind = 1;
	while((c = getopt(argcnt, args, "iukw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'i' :	type = 'i';
					break;
			case 'u' :	type = 'u';
					break;
			case 'k' :	type = 'k';
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (type == 'i') pristk();
	else {
		if (args[optind]) {
			if ((proc = strcon(args[optind], 'd')) == -1)
				error("\n");
			if ((proc > nproc) || (proc < 0))
				error("%d out of range\n", proc);
			if (type == 'u')
				prustk(proc);
			else prkstk(proc);
		}
		else if (type == 'u')
			prustk(proc);
		else prkstk(proc);
	}
}

/* print interrupt stack (if there is one) */
pristk()
{
	unsigned stkfp, intstack, eintstack;

	if (getublock(getcurproc()) == -1)
		return;
#if defined(sun2) || defined(sun3) || defined(sun3x)
	stkfp = ubp->u_pcb.pcb_regs.val[11];
#else
	stkfp = ubp->u_pcb.pcb_regs.val[1];
#endif
	intstack = symsrch("intstack")->n_value;
	eintstack = symsrch("eintstack")->n_value;

	if ((stkfp > intstack) && (stkfp < eintstack))
		pristack(stkfp, eintstack);
	else fprintf(fp, "no valid interrupt stack\n");
}

/* print kernel stack */
int
prkstk(proc)
int proc;
{
	unsigned stkfp;
	unsigned stklo, stkhi, intstack, eintstack;

	if (getublock(proc) == -1)
		return;
#if defined(sun2) || defined(sun3) || defined(sun3x)
	stkfp = ubp->u_pcb.pcb_regs.val[11];
#else
        stkfp = ubp->u_pcb.pcb_regs.val[1]; 
#endif
	stkhi = (int)procbuf.p_stack;
	stklo = stkhi - KERNSTACK;
	intstack = symsrch("intstack")->n_value;
	eintstack = symsrch("eintstack")->n_value;

	if ((stkfp > intstack) && (stkfp < eintstack))
		pristack(stkfp, eintstack);
#ifdef sparc
	stack(stkfp,stkhi);
#endif
	for (;;) {
		if ((stkfp > stklo) && (stkfp < stkhi)) {
			prkstack(stkfp, stklo, stkhi);
			return;
		}
		else if ((stkfp > intstack) && (stkfp < eintstack)) {
			kvm_read(kd, stkfp, &stkfp, sizeof(int));
		}
		else {
			fprintf(fp, "no kernel stack\n");
			return;
		}
	}
}

/* print user stack */
int
prustk(proc)
int proc;
{
	unsigned stkfp, tmp;
	unsigned stklo, stkhi, intstack, eintstack;

	if (getublock(proc) == -1)
		return;
	stkfp = ubp->u_pcb.pcb_regs.val[11];
#ifdef sparc
        stkfp = ubp->u_pcb.pcb_regs.val[1];
#endif
	stkhi = (int)procbuf.p_stack;
	stklo = stkhi - KERNSTACK;
	intstack = symsrch("intstack")->n_value;
	eintstack = symsrch("eintstack")->n_value;

	while (stkfp > USRSTACK) {
		if ((stkfp > intstack) && (stkfp < eintstack)) {
			kvm_read(kd, stkfp, &tmp, sizeof(int));
		}
                else
#if defined(sun2) || defined(sun3) || defined(sun3x)
                        tmp = *(unsigned *)&kstack[stkfp - stklo];
#else
                        stack(stkfp,stkfp+15);
                        tmp = *(unsigned*)&kstack1[14];
#endif
		stkfp = tmp;
	}
#ifdef sparc
	if ((stkfp == 0) || ((stkfp >> 24) != 0xf7)) {
		printf( " No user stack\n");
		return;
	}
#endif
	prstack(stkfp, USRSTACK, proc);
}

/* dump stack */
int
prstack(stkfp, stkhi, slot)
unsigned stkfp, stkhi;
int slot;
{
	register unsigned i;
	register *p, *q;

	fprintf(fp, "FP: %x\n", stkfp);
	fprintf(fp, "END OF STACK: %x\n", stkhi);

	stkfp = (stkfp >> 4) << 4;
#if defined(sun2) || defined(sun3) || defined(sun3x)
	q = p = (int *)malloc(stkhi - stkfp);
	if (p == NULL) error("couldn't get memory to read interrupt stack");
	readmem(stkfp, 0, slot, p, stkhi - stkfp, "user stack");
#else
        stack(stkfp,stkhi);
        p = (int *)&(kstack1[1]);
#endif 
	for (i = stkfp; i < stkhi; i += 4, p++)
	{
		if ((i % 16) == 0) {
			fprintf(fp, "\n%8x: ", i);
		}
		fprintf(fp, "  %8x", *p);
	}
	free(q);
	fprintf(fp, "\n");
	
}

/* dump kernel stack */
int
prkstack(stkfp, stklo, stkhi)
unsigned stkfp, stklo, stkhi;
{
	register unsigned i;
	register *p;

	fprintf(fp, "\nKERNEL STACK:\nFP: %8x ", stkfp);
	fprintf(fp, "END OF STACK: %8x\n", stkhi);

	/* round beginning of display to next lower 16 bytes */
	i = (stkfp >> 4) << 4;
#if defined(sun2) || defined(sun3) || defined(sun3x)
	p = (int *)&(kstack[i - stklo]);
#else
	p = (int *)&(kstack1[1]);
#endif
	
	for (; i < stkhi; i += 4, p++) {
		if ((i % 16) == 0) {
			fprintf(fp, "\n%8x: ", i);
		}
		fprintf(fp, "  %8x", *p);
	}
	fprintf(fp, "\n");
}

/* dump interrupt stack */
int
pristack(stkfp, stkhi)
unsigned stkfp, stkhi;
{
	register unsigned i;
	register *p, *q;

	fprintf(fp, "\nINTERRUPT STACK:\nFP: %8x ", stkfp);
	fprintf(fp, "END OF STACK: %8x\n", stkhi);

	stkfp = (stkfp >> 4) << 4;
	q = p = (int *)malloc(stkhi - stkfp);
	if (p == NULL) error("couldn't get memory to read interrupt stack");
	kvm_read(kd, stkfp, p, stkhi - stkfp);
	
	for (i = stkfp; i < stkhi; i += 4, p++)
	{
		if ((i % 16) == 0) {
			fprintf(fp, "\n%8x: ", i);
		}
		fprintf(fp, "  %8x", *p);
	}
	free(q);
	fprintf(fp, "\n");
}

/* get arguments for trace function */
int
gettrace()
{
	int proc = Procslot;
	int phys = 0;
	int all = 0;
	long addr = -1;
	long arg1 = -1;
	long arg2 = -1;
	int c;
	struct nlist *sp;

	optind = 1;
	while((c = getopt(argcnt, args, "epw:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 'p' :	phys = 1;
					break;
			case 'e' :	all = 1;
					break;
			default  :	longjmp(syn, 0);
		}
	}
	if (args[optind]) {
		do {
			getargs(nproc, &arg1, &arg2);
			if (arg1 == -1) 
				continue;
			if (arg2 != -1)
				for (proc = arg1; proc <= arg2; proc++)
					prktrace(proc);
			else prktrace(arg1);
			proc = arg1 = arg2 = -1;
		} while(args[++optind]);
	}
	else if (all) {
		for (proc = 0; proc < nproc; proc++) 
			prktrace(proc);
	}
	else prktrace(proc);
}

#if defined(sun2) || defined(sun3) || defined(sun3x)
/* print kernel trace */
int
prktrace(proc)
int proc;
{
	int pc, stklo, stkhi, stkfp, intstack, eintstack;
	int nargs, func;
	struct frame frame, *frp;
	char tmp[20];
	int argoff, arg;
	register i, j;
	
	if (getublock(proc) == -1)
		return;
	fprintf(fp, "      FP       PC             SYM+ OFF  ARGS\n");
#if defined(sun2) || defined(sun3) || defined(sun3x)
	stkfp = ubp->u_pcb.pcb_regs.val[11];
#else
	stkfp = ubp->u_pcb.pcb_regs.val[1];
#endif
	pc = ubp->u_pcb.pcb_regs.val[0];
	intstack = symsrch("intstack")->n_value;
	eintstack = symsrch("eintstack")->n_value;
	stkhi = (int)procbuf.p_stack;
	stklo = stkhi - KERNSTACK;
	argoff = (int)((char *)&frame.fr_arg[0] - (char *)&frame);

	for (;;) {
		if ((stkfp > intstack) && (stkfp < eintstack)) {
			kvm_read(kd, stkfp, &frame, sizeof(struct frame));
			nargs = calc_args(frame.fr_savpc);
			makesymoff(pc, tmp);
			fprintf(fp, "%8x %8x %20s ", stkfp, pc, tmp);
			for (i = 0, j = stkfp + argoff; i < nargs; i++, j += 4) {
				kvm_read(kd, j, &arg, sizeof(int));
				fprintf(fp, "%8x ", arg);
			}
			fprintf(fp, "\n");
			stkfp = (int)frame.fr_savfp;
			pc = frame.fr_savpc;
		}
		else if ((stkfp > stklo) && (stkfp < stkhi)) {
			frp = (struct frame *)&(kstack[stkfp - stklo]);
			nargs = calc_args(frp->fr_savpc);
			makesymoff(pc, tmp);
			fprintf(fp, "%8x %8x %20s ", stkfp, pc, tmp);
			for (i = 0, j = stkfp + argoff; i < nargs; i++, j += 4) {
				arg = *(int *)&(kstack[j - stklo]);
				fprintf(fp, "%8x ", arg);
			}
			fprintf(fp, "\n");
			stkfp = (int)frp->fr_savfp;
			pc = frp->fr_savpc;
		}
		else {
			makesymoff(pc, tmp);
			fprintf(fp, "%8x %8x %20s\n", stkfp, pc, tmp);
			return;
		}
	}
}

makesymoff(pc, tmp)
int pc;
char *tmp;
{
	struct nlist *symsrch(), *np;
	int offset;

	if (!(np = findsym((unsigned)pc))) {
		sprintf(tmp, "%x", pc);
	}
	else {
		offset = pc - np->n_value;
		if (offset < 0x10000)
			sprintf(tmp, "%15s+%4x", strtbl + np->n_un.n_strx, offset);
		else sprintf(tmp, "%x", pc);
	}
}

#define HIWORD	0xffff0000
#define LOWORD	0x0000ffff
#define LINKA6	0x4e560000	/* link a6,#x    */
#define ADDLSP	0xdffc0000	/* addl #x,sp    */
#define ADDWSP	0xdefc0000	/* addw #x,sp    */
#define LEASP	0x4fef0000	/* lea	sp@(x),sp*/
#define TSTBSP	0x4a2f0000	/* tstb sp@(x)   */
#define INSMSK	0xfff80000
#define MOVLSP	0x2e800000	/* movl dx,sp@   */
#define MOVLD0	0x20000000	/* movl d0,dx	 */
#define MOVLA0	0x20400000	/* movl d0,ax	 */
#define MVLMSK	0xf1ff0000
#define MOVEML	0x48d70000	/* moveml #x,sp@ */
#define JSR	0x4eb80000	/* jsr x.[WL]    */
#define JSRPC	0x4eba0000	/* jsr PC@( )    */
#define LONGBIT 0x00010000
#define BSR	0x61000000	/* bsr x	 */
#define BSRL	0x61ff0000	/* bsrl x	 (68020 only) */
#define BYTE3	0x0000ff00
#define LOBYTE	0x000000ff
#define ADQMSK	0xf1ff0000
#define ADDQSP	0x508f0000	/* addql #x,sp   */
#define ADDQWSP	0x504f0000	/* addqw #x,sp   */

/*
 * try to figure out how many args procedure was passed by looking
 * at the instruction at the return pc - ugly and not always correct
 * (see adb source)
 */
int
calc_args(addr)
{
	int instruc, val;

	kvm_read(kd, addr, &instruc, sizeof(int));
	if ((instruc&MVLMSK) == MOVLA0 || (instruc&MVLMSK) == MOVLD0) {
		kvm_read(kd, addr + 2, &instruc, sizeof(int));
	}
	if ((instruc&ADQMSK) == ADDQSP || (instruc&ADQMSK) == ADDQWSP) {
		val = (instruc >> (16+9)) & 07;
		if (val == 0)
			val = 8;
	} else if ((instruc&HIWORD) == ADDLSP) {
		kvm_read(kd, addr + 2, &val, sizeof(int));
	} else if ((instruc&HIWORD) == ADDWSP || (instruc&HIWORD) == LEASP) {
		val = instruc&LOWORD;
	} else {
		val = 0;
	}
	return(val / 4);
}
#endif
#ifdef sparc
prktrace(proc)
        int proc;
{
        printstack(proc);
}
#endif

/* get arguments for kfp function */
int
getkfp()
{
	int c;
	int proc = Procslot;

	optind = 1;
	while((c = getopt(argcnt, args, "w:s:")) !=EOF) {
		switch(c) {
			case 'w' :	redirect();
					break;
			case 's' :	proc = setproc();
					break;
			default  :	longjmp(syn, 0);
		}
	}
	prkfp(proc);
}

/* print kfp */
int
prkfp(proc)
int proc;
{
	if (getublock(proc) == -1)
		return;
	fprintf(fp, "kfp: %8x\n", ubp->u_pcb.pcb_regs.val[11]);
}
#ifdef sparc
static
is_leaf_proc ( spos, cur_o7 )
        register struct stackpos *spos;
        addr_t cur_o7;          /* caller's PC if leaf proc (rtn addr) */
                                /* invalid otherwise */
{
        addr_t  usp,            /* user's stack pointer */
                upc,            /* user's program counter */
                sv_i7,          /* if leaf, caller's PC ... */
                                /* else, caller's caller's PC */
                cto7,           /* call target of call at cur_o7 */
                                /* (if leaf, our entry point) */
                cti7,           /* call target of call at sv_i7 */
                                /* (if leaf, caller's entry point) */
                near_sym;       /* nearest symbol below PC; we hope it ... */
                                /* ... is the address of the proc we're IN */
        int offset, leaf;
        char *saveflg = errflg;

        errflg = 0;
        usp = spos->k_fp;
        upc = spos->k_pc;
        offset = findsym1( upc, ISYM );
        if( offset == MAXINT ) {
                near_sym = -1;
        } else {
                near_sym = cursym->s_value;
                if( has_save( near_sym, upc ) ) {
                        /* Not a leaf proc.  This is the most common case. */
                        return 0;
                }
        }
        cto7 = calltarget( cur_o7 );
        if( cto7 == 0 ) return 0;               /* nope */
        if( cto7 == CALL_INDIRECT ) {
                if( near_sym != -1 ) {
                        cto7 = near_sym;        /* best guess */
                } else {
                        errflg = "Cannot trace stack" ;
                        return 0;
                }
        } else {
                (void) get( cto7, ISP );        /* is the address ok? */
                if( errflg  ||  cto7 > upc ) {
                        errflg = saveflg ;
                        return 0;       /* nope */
                }
                sv_i7 = get( usp + FR_SAVPC, SSP );
                cti7 = calltarget( sv_i7 );
                if( cti7 != CALL_INDIRECT ) {
                        if( cti7 == 0 ) return 0;
                        (void) get( cti7, ISP );        /* is the address ok? */                        if( errflg  ||  cti7 > cur_o7 ) {
                                errflg = saveflg ;
                                return 0;       /* nope */
                        }
                }
                if( has_save( cto7 ) ) {
                        /* not a leaf. */
                        return 0;
                }
        }
        spos->k_caller = cur_o7;
        spos->k_entry = cto7;
        spos->k_flags |= K_LEAF;
        trampcheck( spos );
        return 1;
} /* end is_leaf_proc */
has_save ( procaddr, xlimit )
        addr_t procaddr, xlimit;
{
        char *asc_instr, *disassemble();
        unsigned int  instr;
        int save_instr = 0x9de30000;

        if( procaddr > xlimit  ||  xlimit > procaddr + SAVE_INS ) {
                xlimit = procaddr + SAVE_INS ;
        }
        while( procaddr < xlimit ) {
                instr = (get( procaddr, ISP));
                instr = instr & 0xffff0000;

                if ( instr == save_instr) {
                        return 1;       /* yep, there's a save */
                }
                procaddr += 4;  /* next instruction */
        }
        return 0;       /* nope, there's no save */
} /* end has_save */
stacktop(spos)
        register struct stackpos *spos;
{
        register int i;
        int leaf;

        char *saveflg;

        for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++)
                spos->k_regloc[ REG_RN(i) ] = REGADDR(i);

        spos->k_fp = readreg(REG_SP);
        spos->k_pc = readreg(REG_PC);
        spos->k_flags = 0;
        saveflg = errflg;
        leaf = is_leaf_proc( spos, core.c_regs.r_o7 );
        if (errflg) {
                spos->k_entry = MAXINT;
                spos->k_nargs = 0;
                errflg = saveflg; /* you didn't see this */
                return;
        }
        errflg = saveflg;
        if( leaf ) {

                if( (spos->k_flags & K_SIGTRAMP) == 0 )
                    spos->k_nargs = 0;
        } else {

                findentry(spos, 1);
        }
}
findentry (spos, fromTop )
        register struct stackpos *spos;
        int fromTop;
{
        char *saveflg = errflg;
        long offset;

        errflg = 0;
        spos->k_caller = get( spos->k_fp + FR_SAVPC, DSP );
        if( errflg  &&  fromTop  &&  regs_not_on_stack( ) ) {
                errflg = 0;
                spos->k_fp = readreg(REG_FP);
                spos->k_caller = get( spos->k_fp + FR_SAVPC, DSP );
        }
        if( errflg == 0 ) {
                spos->k_entry = calltarget( spos->k_caller );
        }
        if( errflg  ||  spos->k_entry == 0 ) {
                spos->k_entry = MAXINT;
                spos->k_nargs = 0;
                spos->k_fp = 0;   /* stopper for printstack */
                errflg = saveflg; /* you didn't see this */
                return;
        }
        errflg = saveflg;
        spos->k_nargs = NARG_REGS;
        spos->k_flags &= ~K_LEAF;
        if( spos->k_entry == CALL_INDIRECT ) {
                offset = findsym1( spos->k_pc );

                if( offset != MAXINT ) {
                        spos->k_entry = cursym->s_value ;
                } else {
                        spos->k_entry = MAXINT;
                }
                trampcheck( spos );
        }
}
static
calltarget ( calladdr )
        addr_t calladdr;
{
        char *saveflg = errflg;
        long instr, offset, symoffs, ct, jt;

        errflg = 0;
        instr = get( calladdr, ISP );
        if( errflg ) {
                errflg = saveflg;
                return 0;       /* no "there" there */
        }
        if( X_OP(instr) == SR_CALL_OP ) {
                /* Normal CALL instruction */
                offset = SR_WA2BA( instr );
                ct = offset + calladdr;
                instr = get( ct, ISP );
                if( errflg ) {
                        errflg = saveflg;
                        return 0;
                }
                if( ijmpreg( instr ) )  return CALL_INDIRECT;
                else                    return ct;
        }
        return  jmpcall( instr );
}
static struct {
        int op, rd, op3, rs1, imm, rs2, simm13;
} jmp_fields;

static
ijmpreg ( instr ) long instr ; {
        if( splitjmp( instr ) ) {
                return jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0 ;
        } else {
                return 0;
        }
}
static
jmpcall ( instr ) long instr; {
  int dest ;

        if(  splitjmp( instr ) == 0       /* Give up if it ain't a jump, */
           || jmp_fields.rd != REG_O7 ) { /* or it doesn't save pc into %o7 */

                return 0;
        }
        if( jmp_fields.imm == 0  ||  jmp_fields.rs1 != 0 )
                return CALL_INDIRECT;   /* can't find target */
        return SR_SEX13(instr);
}
static
splitjmp ( instr ) long instr; {

        jmp_fields.op     = X_OP( instr );
        jmp_fields.rd     = X_RD( instr );
        jmp_fields.op3    = X_OP3( instr );
        jmp_fields.rs1    = X_RS1( instr );
        jmp_fields.imm    = X_IMM( instr );
        jmp_fields.rs2    = X_RS2( instr );
        jmp_fields.simm13 = X_SIMM13( instr );

        if( jmp_fields.op == SR_FMT3a_OP )
                return  jmp_fields.op3 == SR_JUMP_OP ;
        else
                return 0;
}
ispltentry(addr)
        int *addr;
{
        unsigned int instr[3], dest;
        int i;
        char *saveflg;

        for(i=0;i<3;i++,addr++) {
                errflg = 0;
                instr[i] = get( addr, ISP );
                if( errflg ) {
                        errflg = saveflg;
                        return 0;
                }
        }
        if(     X_OP(instr[0]) == SR_FMT2_OP && X_OP2(instr[0]) == SR_SETHI_OP &&
                X_OP(instr[1]) == SR_FMT3a_OP  && X_IMM(instr[1]) &&
                        X_OP3(instr[1]) == SR_OR_OP &&
                X_OP(instr[2]) == SR_FMT3a_OP && X_OP3(instr[2]) == SR_JUMP_OP ) {
                         dest = SR_HI( SR_SEX22( X_DISP22(instr[0]) ) ) |
                                                  SR_SEX13( X_SIMM13(instr[1]) );
        } else {
                dest = 0;
        }
        return dest;
}
nextframe (spos)
        register struct stackpos *spos;
{
        int val, regp, i;
        register instruc;

        if (spos->k_flags & K_CALLTRAMP) {
                trampnext( spos );
                errflg = 0;
        } else
        {
                if (spos->k_entry == MAXINT) {
                        /*
                         * we don't know what registers are
                         * involved here--invalidate all
                         */
                        for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++) {
                                spos->k_regloc[ REG_RN(i) ] = -1;
                        }
                } else {
                        for (i = FIRST_STK_REG; i <= LAST_STK_REG; i++) {
                                spos->k_regloc[ REG_RN(i) ] =
                                        spos->k_fp + 4*REG_RN(i) ;
                        }
                }
                /* find caller's pc and fp */
                spos->k_pc = spos->k_caller;
                if( (spos->k_flags & (K_LEAF|K_SIGTRAMP)) == 0 ) {
                        spos->k_fp = get(spos->k_fp+FR_SAVFP, DSP);
                }
                /* else (if it is a leaf or SIGTRAMP), don't change fp. */
                if( spos->k_flags & K_SIGTRAMP ) {
                    /* Preserve K_LEAF */
                    spos->k_flags = (spos->k_flags | K_TRAMPED) & ~K_SIGTRAMP ;
                } else {
                    spos->k_flags = 0;
                }
                findentry(spos, 0);
        }
        return (spos->k_fp);
} /* end nextframe */
trampcheck (spos) register struct stackpos *spos; {

        if( trampsym == 0 ) return;

        findsym1(spos->k_caller, ISYM);
        if( cursym != trampsym ) return;

        spos->k_flags |= K_CALLTRAMP;
        spos->k_nargs = 3;

}
findsym1(val, type)
        int val, type;
{
        register struct asym *s;
        register int i, j, k;

        cursym = 0;
        if (type == NSYM)
                return (MAXINT);
        i = 0; j = nglobals - 1;
        while (i <= j) {
                k = (i + j) / 2;
                s = globals[k];
                if (s->s_value == val)
                        goto found;
                if (s->s_value > val)
                        j = k - 1;
                else
                        i = k + 1;
        }
        if (j < 0)
                return (MAXINT);
        s = globals[j];
found:
        cursym = s;
        return (val - s->s_value );
}
static struct sigcontext *
find_scp ( kfp, was_leaf ) addr_t kfp ; {
  addr_t wf, scp;
  
        wf = get( kfp + FR_IREG(REG_FP), DSP );
        scp = get( wf + FR_ARG2, DSP );
        if( scp < wf ) {
            scp = get( wf + FR_ARG1, DSP );
        }
        if( was_leaf == 0 ) {
            return find_scp( wf, 1 );   /* lie a little */
        }
        return (struct sigcontext *)scp;
}        
trampnext (spos)
        struct stackpos *spos;
{
  int was_leaf;
  struct stackpos tsp;
  addr_t maybe_o7, save_fp;
  struct sigcontext *scp; /* address in the subprocess.  Don't dereference */

        spos->k_pc = spos->k_caller;
        spos->k_entry = trampsym->s_value;
        was_leaf = ( spos->k_flags & K_LEAF );
        spos->k_flags = K_SIGTRAMP;
        spos->k_nargs = 0;
        scp = find_scp( spos->k_fp, was_leaf );
        tsp = *spos;

        maybe_o7 = get( spos->k_fp + FR_SAVPC, DSP );
        tsp.k_pc = get( &scp->sc_pc, DSP );
        spos->k_fp = get( &scp->sc_sp, DSP );
        spos->k_caller = get( &scp->sc_pc, DSP );
} /* end trampnext */

char    *regnames[]
    = {
        /* IU general regs */
        "g0", "g1", "g2", "g3", "g4", "g5", "g6", "g7",
        "o0", "o1", "o2", "o3", "o4", "o5", "sp", "o7",
        "l0", "l1", "l2", "l3", "l4", "l5", "l6", "l7",
        "i0", "i1", "i2", "i3", "i4", "i5", "fp", "i7",

        /* Miscellaneous */
        "y", "psr", "wim", "tbr", "pc", "npc", "fsr", "fq",

        /* FPU regs */
        "f0",  "f1",  "f2",  "f3",  "f4",  "f5",  "f6",  "f7",
        "f8",  "f9",  "f10", "f11", "f12", "f13", "f14", "f15",
        "f16", "f17", "f18", "f19", "f20", "f21", "f22", "f23",
        "f24", "f25", "f26", "f27", "f28", "f29", "f30", "f31",
   };
static void
regerr ( reg, wind )
        int reg, wind;
{
  static char rw_invalid[ 60 ];
  char *wp;

        wp = wind ? "window-" : "" ;
        if( reg < 0  ||  reg > NREGISTERS ) {
            sprintf( rw_invalid, "Invalid %sregister %d", wp, reg );
        } else {
            sprintf( rw_invalid, "Invalid %sregister %s (%d)", wp,
                regnames[ reg ], reg );
        }
        errflg = rw_invalid;
}
reg_windows( reg, val, wrt_flag ) {
	struct pcb *pcb_ptr;
	addr_t usp;
	int *raddr;

	if( reg < MIN_WREGISTER  ||  reg > MAX_WREGISTER ) {
		regerr( reg, 1 );
		return 0;
	}
	pcb_ptr = & u.u_pcb ;
	usp = core.c_regs.r_sp;
	if( reg >= REG_I0 ) {
		raddr = (int *)(usp + FR_IREG(reg) );
	} else {
		raddr = (int *)(usp + FR_LREG(reg) );
	}
	if( wrt_flag ) {
		*raddr = val;
	} else {
		*( adb_raddr.ra_raddr ) = *raddr;
	}
	if( wrt_flag ) {
		*( adb_raddr.ra_raddr ) = val;
	} else {
		val = *( adb_raddr.ra_raddr );
	}
	return val;
} /* end reg_windows */
void
reg_address( reg )
int reg;
{
  register struct allregs *arp = &adb_regs;
  register struct adb_raddr *ra = &adb_raddr;

        ra->ra_type = r_normal;
        switch( reg ) {
         case REG_PSR:  ra->ra_raddr = & arp->r_psr;    break;
         case REG_PC:   ra->ra_raddr = & arp->r_pc;     break;
         case REG_WIM:
         case REG_TBR:
                regerr( reg, 0 );
                ra->ra_raddr = 0;
                ra->ra_type = r_invalid;
                break;

         case REG_Y:    ra->ra_raddr = & arp->r_y;      break;

         /* Globals */
         case REG_G0:
                ra->ra_raddr = 0;
                ra->ra_type = r_gzero;
                break;
         case REG_G1:
         case REG_G2:
         case REG_G3:
         case REG_G4:
         case REG_G5:
         case REG_G6:
         case REG_G7:
                ra->ra_raddr = & arp->r_globals[ reg - REG_G1 ];
                break;
         case REG_O0:
         case REG_O1:
         case REG_O2:
         case REG_O3:
         case REG_O4:
         case REG_O5:
         case REG_SP:   /* REG_O6 is == REG_SP */
         case REG_O7:
                ra->ra_raddr = &( adb_oreg[ reg - REG_O0 ] );
		break;
         case REG_L0:
         case REG_L1:
         case REG_L2:
         case REG_L3:
         case REG_L4:
         case REG_L5:
         case REG_L6:
         case REG_L7:
                ra->ra_raddr = &( adb_lreg[ reg - REG_L0 ]);
                break;
         case REG_I0:
         case REG_I1:
         case REG_I2:
         case REG_I3:
         case REG_I4:
         case REG_I5:
         case REG_I6:
         case REG_I7:
                ra->ra_raddr = &( adb_ireg[ reg - REG_I0 ]);
                break;
         case REG_FQ:   /* Can't get the FQ */
                regerr( reg, 0 );
                ra->ra_raddr = 0;
                ra->ra_type = r_invalid;
                break;
         case REG_FSR:
                ra->ra_raddr = (int *) &core.c_fpu.fpu_fsr;
                ra->ra_type = r_normal;
                break;
         default:
                if( reg >= REG_F0  &&  reg <= REG_F31 ) {
                        ra->ra_type = r_floating;
                        ra->ra_raddr = (int *)
                            &core.c_fpu.fpu_regs[ reg - REG_F0 ];
                }
                else {
                        regerr( reg, 0 );
                        ra->ra_raddr = 0;
                        ra->ra_type = r_invalid;
                }
                break;

        }
}
stack(low,high)
        unsigned int high,low;
{
        int itype, ptype ;

        itype = DSP; ptype = DSYM;
        count = 1;
        difference = high - low;
        dot = low;
        psymoff(dot, ptype);
        scanform(count, itype, ptype);
}
psymoff(v, type)
        int v, type;
{
        int w;

        if (v)
                w = findsym1(v, type);
}
scanform(icount, itype, ptype)
        int icount;
        int itype, ptype;
{
        int fcount, savdot;
        char *fp;

        savdot = dot;
	if (difference > 0x3000)
		fcount = 0x3000;
	else
		fcount = difference;
        fp = exform(fcount, itype, ptype);
        dotinc = dot - savdot;
        dot = savdot;
} /* end scanform */
inkdot(incr)
        int incr;
{
        unsigned newdot;

        newdot = dot + incr;
        if ((dot ^ newdot) & 0x80000000)
                error("address wrap around");
        return (newdot);
}
char *
exform(fcount, itype, ptype)
        int fcount;
        int itype, ptype;
{
        char *eachform();
        char *fp;

        while (fcount > 0) {

                fp = eachform( fcount, itype, ptype);

                if (itype != NSP)
                        dot = inkdot(dotinc);
                fcount--;
        }
        return (fp);
} /* end of exform */
char *
eachform(fcount, itype, ptype)
        int fcount;
        int itype, ptype;
{
        u_short shortval;
        int val;
        unsigned char c;
        char *fp;

                /* 32- bit hacks go here */
                val = get(dot, itype);
                /* handle big-end and little-end machines */
                shortval = *(short *)&val;
                c = *(char *)&val;
        each_switch( fcount, val, itype, ptype, c, shortval );
        return (fp);
} /* end of eachform */
each_switch ( fcount, val, itype, ptype, c, shortval )
        int fcount;
        int val;
        int itype, ptype;
        unsigned char c;
        u_short shortval;
{
        static int i;

        dotinc = 4; /*printf("%16x", val); */
        kstack1[i++] = val;
        if (fcount == 1)
                i = 0;
}
extern FILE *fp;                               /* output file pointer */

printstack(proc)
	int proc;
{
        int i, val;
        char *name;
        struct stackpos pos;
        struct asym *savesym;
	unsigned stkfp;
        int def_nargs = -1;
	count = -1;

	fprintf(fp, "      FP       PC             SYM+ OFF  ARGS\n");
	if (getublock(proc) == -1)
		return;
	stkfp = ubp->u_pcb.pcb_regs.val[1];
        stacktop(&pos);
	pos.k_fp = stkfp;
        while (count) {         /* loop once per stack frame */
                count--;
#ifdef sun 
		if (pos.k_pc == MAXINT) {
                        name = "?";
                        pos.k_pc = 0;
                }
#endif
                else {
                        val =  findsym1(pos.k_pc, ISYM);
                        if (cursym && !strcmp(cursym->s_name, "start"))
                                break;
                        if (cursym)
                                name = cursym->s_name;
                        else
                                name = "?";
                }
#ifdef sun
                if (pos.k_entry != MAXINT) {
                        int val2, jmptarget;
                        struct asym *savesym = cursym;

                        val2 = findsym1(pos.k_entry, ISYM);
                        if (cursym != NULL && cursym != savesym &&
                            (savesym == NULL ||
                              cursym->s_value != savesym->s_value)) {
                                if(jmptarget = ispltentry(pos.k_entry)) {
                                                struct asym *savesym2 = cursym;

                                                findsym1(jmptarget, ISYM);
                                                if ( cursym != NULL && cursym != savesym &&
                                                                (savesym == NULL ||
                                                                cursym->s_value
!= savesym->s_value)
                                                ) {
                                                        printf("(?)\n");
                                                        if (cursym)
                                                                name = cursym->s_name;
                                                        else
                                                                name = "?";
                                                        printf("%s", name);
                                        }
                                        printf("(via ");
                                        if (savesym2)
                                                name = savesym2->s_name;
                                        else
                                                name = "?";
                                        printf("%s", name);
                                        if(val2 && val2 != MAXINT) {
                                                printf("+%x", val2);
                                        }
                                        printf(")");
                                } else {
                                        printf("(?)\n");
                                        if (cursym)
                                                name = cursym->s_name;
                                        else
                                                name = "?";
                                }
                        }

                } /* end if k_entry indicates a problem */
#endif
                printargs( def_nargs, val, &pos, name );

                if (nextframe(&pos) == 0 || errflg)
                        break;
#ifndef sun
                if (hadaddress == 0 && !INSTACK(pos.k_fp))
                        break;
#endif
        } /* end while (looping once per stack frame) */
} /* end printstack */
static
printargs ( def_nargs, val, ppos, name ) 
	struct stackpos *ppos; 
	int def_nargs;
	int val;
	char *name;
{
  addr_t regp, callers_fp, ccall_fp;
  int anr, nargs;

        regp = ppos->k_fp + FR_I0 ;
        callers_fp = get( ppos->k_fp + FR_SAVFP, DSP );
        if( callers_fp )
                ccall_fp = get( callers_fp + FR_SAVFP, DSP );
        else
                ccall_fp = 0;

        if( def_nargs < 0  ||  ppos->k_flags & K_CALLTRAMP
                || callers_fp == 0 ) {
                nargs = ppos->k_nargs;
        } else {
                nargs = def_nargs;
        }
	fprintf(fp,"%8x %8x",ppos->k_fp, ppos->k_pc);
                fprintf(fp,"%16s", name);
        if (val == MAXINT)
                fprintf(fp," at %x\n", ppos->k_pc );
        else
                fprintf(fp,"+  %x", val);
	fprintf(fp,"  ");
        if ( nargs ) {
                for( anr=0; anr < nargs; ++anr ) {
                        if( anr == NARG_REGS ) {
                                /* Reset regp for >6 (register) args */
                                regp = callers_fp + FR_XTRARGS ;
                        }
                        fprintf(fp,"%x", get(regp, DSP));
                        regp += 4;
                        if( ccall_fp  &&  regp >= ccall_fp )
                                break;
                        if( anr < nargs-1 ) printf(" ");
                }
        }
fprintf(fp,"\n");
}
#endif sparc
