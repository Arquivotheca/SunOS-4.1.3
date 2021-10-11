/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static	char *sccsid = "@(#)pstat.c 1.1 92/07/30 SMI"; /* from UCB 5.8 5/5/86 */
#endif not lint

/*
 * Print system stuff
 */

#include <stdio.h>
#include <fcntl.h>
#define	KERNEL
#include <sys/param.h>
#include <sys/file.h>
#undef	KERNEL
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/vnode.h>
#define	KERNEL
#include <ufs/inode.h>
#undef KERNEL
#include <sys/stream.h>
#include <sys/stropts.h>
#include <nlist.h>
#include <vm/anon.h>
#include <kvm.h>

char	*fcore	= NULL;
char	*fnlist	= NULL;

struct nlist nl[] = {
#define	SIHEAD		0
	{ "_ihead" },
#define	SINONEW		1
	{ "_ino_new" },
#define SPROC		2
	{ "_proc" },
#define	SFIL		3
	{ "_file" },
#define	SNPROC		4
	{ "_nproc" },
#define	SNFILE		5
	{ "_nfile" },
#define	SALLSTREAM	6
	{ "_allstream" },
#define	SANON		7
	{ "_anoninfo" },
	{ "" }
};

int	inof;
int	prcf;
int	usrf;
int	filf;
int	strf;
int	swpf;
int	totflg;
int	allflg;
kvm_t 	*kd;
void	kread();
u_long	getword();
void	printru();

extern char *calloc();

main(argc, argv)
char **argv;
{
	register char *argp;
	char *identity = argv[0];
	int allflags = 0;
	int pid;

	argc--, argv++;
	while (argc > 0 && **argv == '-') {
		argp = *argv++;
		argp++;
		argc--;
		while (*argp++)
		switch (argp[-1]) {

		case 'T':
			totflg++;
			break;

		case 'a':
			allflg++;
			break;

		case 'i':
			inof++;
			break;

		case 'p':
			prcf++;
			break;

		case 'u':
			if (argc == 0) {
				(void) fprintf(stderr,
				    "pstat: process id required for -u\n");
				exit(1);
			}
			argc--;
			usrf++;
			if (sscanf(*argv++, "%d", &pid) != 1) {
				(void) fprintf(stderr,
				    "pstat: process id for -u is invalid\n");
				exit(1);
			}
			break;

		case 'f':
			filf++;
			break;

		case 'S':
			strf++;
			break;

		case 's':
			swpf++;
			break;
		default:
			usage();
			exit(1);
		}
	}
	allflags = filf | totflg | inof | prcf | usrf | strf | swpf;
	if (allflags == 0) {
		(void) fprintf(stderr,
		    "pstat: one or more of -[ipfsuST] is required\n");
		exit(1);
	}

	if (argc>0) {
		fnlist = argv[0];
		if (argc>1)
			fcore = argv[1];
	}
	if ((kd = kvm_open(fnlist, fcore, (char *)NULL, O_RDONLY,
	    identity)) == NULL)
		exit(1);
	if (kvm_nlist(kd, nl) < 0) {
		(void) fprintf(stderr, "pstat: bad namelist\n");
		exit(1);
	}

	if (filf||totflg)
		dofile();
	if (inof||totflg)
		doinode();
	if (prcf||totflg)
		doproc();
	if (strf)
		dostream();
	if (usrf)
		dousr(pid);
	if (swpf||totflg)
		doswap();
	exit(0);
	/* NOTREACHED */
}

usage()
{

	(void) fprintf(stderr,
	    "usage: pstat -[aipfsST] [-u pid] [system [core]]\n");
}

#define	putf(v, n)	(void) printf("%c", (v) ? (n) : ' ')

/* Inode kernel address stored here */
#define i_kaddr i_nextr

doinode()
{
	register struct inode *ip, *nip;
	register struct vnode *vp;
	union ihead *xihead, *axihead, *axitail, *ih; 	/* Hash list */
	register int nin = 0;			    	/* Active inodes */
	u_long ino_new;				/* Total curent inodes */
	struct inode *getinode();

	if (nl[SINONEW].n_type == 0) {	/* diskless client? */
		(void) printf("%3d/%3d inodes\n", 0, 0);
		return;
	}
	xihead = (union ihead *)calloc((unsigned)INOHSZ, sizeof(union ihead));
	axihead = (union  ihead *)nl[SIHEAD].n_value;
	axitail = axihead + sizeof (union  ihead) * INOHSZ;
	ino_new = getword(SINONEW);
	if (xihead == NULL) {
		(void) fprintf(stderr,
		    "pstat: can't allocate memory for inode table\n");
		return;
	}
	kread((unsigned long)axihead, (char *)xihead, INOHSZ * sizeof(*xihead));
	if (!totflg)
		(void) printf("  ILOC   IFLAG  IDEVICE   INO   MODE NLK  UID  SIZE/DEV VFLAG CNT SHC EXC TYPE\n");

	for (ih = xihead; ih < &xihead[INOHSZ]; ih++)
	    for (ip = getinode(ih->ih_chain[0]); 
		 ip &&
		  ((union ihead *)ip->i_kaddr < axihead ||
		   (union ihead *)ip->i_kaddr > axitail);
		 nip = ip->i_forw, freeinode(ip), ip = getinode(nip)) {
		if (ip->i_vnode.v_count)
			nin++;
		if (totflg)
			continue;
		vp = &ip->i_vnode;
		if (vp->v_count == 0)
			continue;
		(void) printf("%8.1x ", ip->i_kaddr);
		putf(ip->i_flag&IACC, 'A');
		putf(ip->i_flag&ICHG, 'C');
		putf(ip->i_flag&ILOCKED, 'L');
		putf(ip->i_flag&IREF, 'R');
		putf(ip->i_flag&IUPD, 'U');
		putf(ip->i_flag&IWANT, 'W');
		(void) printf(" %3d,%3d", major(ip->i_dev), minor(ip->i_dev));
		(void) printf("%6d", ip->i_number);
		(void) printf("%7o", ip->i_mode);
		(void) printf("%4d", ip->i_nlink);
		(void) printf("%5d", ip->i_uid);
		if ((ip->i_mode&IFMT)==IFBLK || (ip->i_mode&IFMT)==IFCHR)
			(void) printf("%6d,%3d", major(ip->i_rdev),
			    minor(ip->i_rdev));
		else
			(void) printf("%10ld", ip->i_size);
		(void) printf(" ");
		putf(vp->v_flag&VROOT, 'R');
		putf(vp->v_flag&VSHLOCK, 'S');
		putf(vp->v_flag&VEXLOCK, 'E');
		putf(vp->v_flag&VLWAIT, 'Z');
		(void) printf(" ");
		(void) printf("%4d", vp->v_count);
		(void) printf("%4d", vp->v_shlockc);
		(void) printf("%4d", vp->v_exlockc);
		switch (vp->v_type) {
			case VNON:
				(void) printf(" VNON");
				break;
			case VREG:
				(void) printf(" VREG");
				break;
			case VDIR:
				(void) printf(" VDIR");
				break;
			case VBLK:
				(void) printf(" VBLK");
				break;
			case VCHR:
				(void) printf(" VCHR");
				break;
			case VLNK:
				(void) printf(" VLNK");
				break; 
			case VSOCK: 
				(void) printf(" VSOCK"); 
				break;
			case VBAD:
				(void) printf(" VBAD");
				break;
			case VFIFO:
				(void) printf(" VFIFO");
				break;
			default:
				(void) printf(" ?????");
				break;
		}
		(void) printf("\n");
	}
	(void) printf("%3d/%3d inodes\n", nin, ino_new);
	free((char *) xihead);
}

struct inode *
getinode(kiaddr)
	caddr_t  kiaddr;
{
	struct inode *ip;
	
	if (kiaddr == NULL)
		return NULL;
	if ((ip = (struct inode *) calloc(1, sizeof(*ip))) == NULL) {
		fprintf(stderr, "pstat: can't allocate memory for inodes\n");
		exit(-1);
	}
	kread((unsigned)kiaddr, (char *)ip, sizeof(*ip));
	ip->i_kaddr = (daddr_t) kiaddr;
	return (ip);
}

freeinode(ip)
	struct inode *ip;
{
	free((char *) ip);
}

u_long
getword(n)
	int n;
{
	u_long word;

	if (nl[n].n_type == 0) {
		(void) fprintf(stderr, "pstat: '%s' not in namelist\n",
			nl[n].n_name);
		exit(1);
	}
	kread(nl[n].n_value, (char *)&word, sizeof (word));
	return (word);
}

doproc()
{
	struct proc *xproc, *aproc;
	int nproc;
	register struct proc *pp;
	register np;

	nproc = getword(SNPROC);
	if (nproc < 0 || nproc > 10000) {
		(void) fprintf(stderr,
		    "pstat: number of procs is preposterous (%d)\n", nproc);
		return;
	}
	xproc = (struct proc *)calloc((unsigned)nproc, sizeof (struct proc));
	if (xproc == NULL) {
		(void) fprintf(stderr,
		    "pstat: can't allocate memory for proc table\n");
		return;
	}
	aproc = (struct proc *)getword(SPROC);
	kread((unsigned long)aproc, (char *)xproc,
	    nproc * sizeof (struct proc));
	np = 0;
	for (pp=xproc; pp < &xproc[nproc]; pp++)
		if (pp->p_stat)
			np++;
	if (totflg) {
		(void) printf("%3d/%3d processes\n", np, nproc);
		return;
	}
	(void) printf("%d/%d processes\n", np, nproc);
	(void) printf("   LOC    S    F PRI      SIG  UID SLP TIM  CPU  NI   PGRP    PID   PPID  RSS SRSS  SIZE    WCHAN     LINK\n");

	for (pp = xproc ; pp < &xproc[nproc] ; pp++) {
		if (pp->p_stat==0 && allflg==0)
			continue;
		(void) printf("%8x", aproc + (pp - xproc));
		(void) printf(" %2d", pp->p_stat);
		(void) printf(" %4x", pp->p_flag & 0xffff);
		(void) printf(" %3d", pp->p_pri);
		(void) printf(" %8x", pp->p_sig);
		(void) printf(" %4d", pp->p_uid);
		(void) printf(" %3d", pp->p_slptime);
		(void) printf(" %3d", pp->p_time);
		(void) printf(" %4d", pp->p_cpu&0377);
		(void) printf(" %3d", pp->p_nice-NZERO);
		(void) printf(" %6d", pp->p_pgrp);
		(void) printf(" %6d", pp->p_pid);
		(void) printf(" %6d", pp->p_ppid);
		(void) printf(" %4x", pp->p_rssize);
		(void) printf(" %4x", pp->p_swrss);
		(void) printf(" %5x", pp->p_dsize+pp->p_ssize);
		(void) printf(" %8x", pp->p_wchan);
		(void) printf(" %8x", pp->p_link);
		(void) printf("\n");
	}
	free((char *)xproc);
}

/*
 *  Print information from the u-area for process pid.
 *  The process does not have to be in memory.
 */
dousr(pid)
	int pid;
{
	struct user *u;
	struct sess sess;
	struct ucred uc;
	struct proc *proc;
	struct file **ofp;
	register i, *ip;
	int numfiles;
	char *pfp;

	if ((proc = kvm_getproc(kd, pid)) == NULL) {
		(void) fprintf(stderr, "pstat: invalid process id: %d\n", pid);
		return;
	}
	if ((u = kvm_getu(kd, proc)) == NULL) {
		(void) fprintf(stderr,
		    "pstat: cannot access u-area for %d\n", pid);
		return;
	}
	kread((unsigned long)(proc->p_cred), (char *)&uc, sizeof(uc));
	kread((unsigned long)(proc->p_sessp), (char *)&sess, sizeof(sess));
	(void) printf("pcb");
	ip = (int *)&u->u_pcb;
	while (ip < &u->u_arg[0]) {
		if ((ip - (int *)&u->u_pcb) % 4 == 0)
			(void) printf("\t");
		(void) printf("%x ", *ip++);
		if ((ip - (int *)&u->u_pcb) % 4 == 0)
			(void) printf("\n");
	}
	if ((ip - (int *)&u->u_pcb) % 4 != 0)
		(void) printf("\n");
	(void) printf("arg\t");
	for (i=0; i<6; i++)
		(void) printf(" %.1x", u->u_arg[i]);
	(void) printf("\n");
	(void) printf("ssave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0)
			(void) printf("\t");
		(void) printf("%9.1x", u->u_ssave.val[i]);
		if (i%5==4)
			(void) printf("\n");
	}
	if (i%5)
		(void) printf("\n");
	(void) printf("error %d\n", u->u_error);
	(void) printf("uids\t%d,%d,%d,%d\n", uc.cr_uid,uc.cr_gid,uc.cr_ruid,
	    uc.cr_rgid);
	(void) printf("procp\t%.1x\n", u->u_procp);
	(void) printf("ap\t%.1x\n", u->u_ap);
	(void) printf("r_val?\t%.1x %.1x\n", u->u_r.r_val1, u->u_r.r_val2);
	(void) printf("cdir rdir %.1x %.1x\n", u->u_cdir, u->u_rdir);

	(void) printf("file");
	/* ofile array is in user struct if u_ofile points to u_ofile_arr */
	if ((caddr_t) u->u_ofile - (caddr_t) proc->p_uarea ==
			(caddr_t) u->u_ofile_arr - (caddr_t) u) {
		numfiles = NOFILE_IN_U;
		ofp = u->u_ofile_arr;
	}
	/* ofile array is in dynamically allocated space */
	else {
		numfiles = NOFILE;
		if ((ofp = (struct file **) calloc(sizeof(struct file **), 
						NOFILE)) == NULL) {
			(void) fprintf(stderr,
		    	    "pstat: can't allocate memory for file table\n");
			return;
		}
		kread((unsigned long) u->u_ofile, (char *) ofp, 
					NOFILE * sizeof(struct file **));
	}
	for (i = 0; i < numfiles; i++) {
		if (i%5==0)
			(void) printf("\t");
		(void) printf("%9.1x", ofp[i]);
		if (i%5==4)
			(void) printf("\n");
	}
	if (ofp != u->u_ofile_arr)
		free((char *) ofp);
	(void) printf("\n");

	(void) printf("pofile");
	/* pofile array is in user struct if u_pofile points to u_pofile_arr */
	if ((caddr_t) u->u_pofile - (caddr_t) proc->p_uarea ==
			(caddr_t) u->u_pofile_arr - (caddr_t) u) {
		numfiles = NOFILE_IN_U;
		pfp = u->u_pofile_arr;
	}
	/* pofile array is in dynamically allocated space */
	else {
		numfiles = NOFILE;
		if ((pfp = (char *) calloc(sizeof(char *), NOFILE)) == NULL) {
			(void) fprintf(stderr,
		    	    "pstat: can't allocate memory for pofile table\n");
			return;
		}
		kread((unsigned long) u->u_pofile, pfp, 
					NOFILE * sizeof(char *));
	}
	for (i = 0; i < numfiles; i++) {
		if (i%5==0)
			(void) printf("\t");
		(void) printf("%9.1x", pfp[i]);
		if (i%5==4)
			(void) printf("\n");
	}
	if (pfp != u->u_pofile_arr)
		free(pfp);
	(void) printf("\n");

	(void) printf("ssave");
	for (i=0; i<sizeof(label_t)/sizeof(int); i++) {
		if (i%5==0)
			(void) printf("\t");
		(void) printf("%9.1x", u->u_ssave.val[i]);
		if (i%5==4)
			(void) printf("\n");
	}
	if (i%5)
		(void) printf("\n");
	(void) printf("sigs");
	for (i=0; i<NSIG; i++) {
		if (i%5==0)
			(void) printf("\t");
		(void) printf("%9.1x ", u->u_signal[i]);
		if (i%5==4)
			(void) printf("\n");
	}
	(void) printf("\n");
	(void) printf("code\t%.1x\n", u->u_code);
	(void) printf("ar0\t%.1x\n", u->u_ar0);
	(void) printf("prof\t%lx %lx %lx %lx\n", u->u_prof.pr_base,
	    u->u_prof.pr_size, u->u_prof.pr_off, u->u_prof.pr_scale);
	(void) printf("\neosys\t%d\n", u->u_eosys);
	(void) printf("ttyp\t%.1x\n", sess.s_ttyp);
	(void) printf("ttyd\t%d,%d\n", major(sess.s_ttyd), minor(sess.s_ttyd));
#ifdef sun386
	(void) printf("textvaddr %lx datavaddr %lx bssvaddr %lx\n",
	    u->u_textvaddr, u->u_datavaddr, u->u_bssvaddr);
#endif
	(void) printf("exdata:");
	(void) printf("\tux_mach %d ux_magic %o\n",
	    u->u_exdata.ux_mach, u->u_exdata.ux_mag);
	(void) printf("\tux_tsize %ld ux_dsize %ld ux_bsize %ld\n",
	    u->u_exdata.ux_tsize, u->u_exdata.ux_dsize,
	    u->u_exdata.ux_bsize);
	(void) printf("\tux_entloc %lx\n", u->u_exdata.ux_entloc);
#ifdef sun386
	(void) printf("\tux_toffset %lx ux_doffset %lx\n",
	    u->u_exdata.ux_toffset, u->u_exdata.ux_doffset);
#else
	(void) printf("\tux_ssize %ld ux_relflg %d\n",
	    u->u_exdata.ux_ssize, u->u_exdata.ux_relflg);
#endif
	(void) printf("comm\t%.*s\n", sizeof u->u_comm, u->u_comm);
	(void) printf("start\t%s", ctime(&u->u_start.tv_sec));
	(void) printf("acflag\t%o\n", u->u_acflag);
	(void) printf("cmask\t%o\n", u->u_cmask);
	(void) printf("sizes\t%.1x %.1x %.1x\n",
	    proc->p_tsize, proc->p_dsize, proc->p_ssize);
	printru("ru", &u->u_ru);
	printru("cru", &u->u_cru);
	free((char *)u);
}

void
printru(name, rup)
	char *name;
	register struct rusage *rup;
{
	(void) printf("%s:", name);
	(void) printf("\tutime %3ld:%02ld stime %3ld:%02ld\n",
	    rup->ru_utime.tv_sec / 60, rup->ru_utime.tv_sec % 60,
	    rup->ru_stime.tv_sec / 60, rup->ru_stime.tv_sec % 60);
	(void) printf("\tmaxrss %d xrss %d drss %d srss %d\n",
	    rup->ru_ixrss, rup->ru_idrss, rup->ru_isrss);
	(void) printf("\tminflt %d majflt %d nswap %d\n",
	    rup->ru_minflt, rup->ru_majflt, rup->ru_nswap);
	(void) printf("\tinblock %d oublock %d msgsnd %d msgrcv %d\n",
	    rup->ru_inblock, rup->ru_oublock, rup->ru_msgsnd, rup->ru_msgrcv);
	(void) printf("\tnsignals %d nvcsw %d nivcsw %d\n",
	    rup->ru_nsignals, rup->ru_nvcsw, rup->ru_nivcsw);
}

dofile()
{
	int nfile;
	struct file *xfile, *afile;
	register struct file *fp;
	register nf;
	int loc;
	static char *dtypes[] = { "???", "vnode", "socket" };

	nf = 0;
	nfile = getword(SNFILE);
	xfile = (struct file *)calloc((unsigned)nfile, sizeof (struct file));
	afile = (struct file *)getword(SFIL);
	if (nfile < 0 || nfile > 10000) {
		(void) fprintf(stderr,
		    "pstat: number of files is preposterous (%d)\n", nfile);
		return;
	}
	if (xfile == NULL) {
		(void) fprintf(stderr,
		    "pstat: can't allocate memory for file table\n");
		return;
	}
	kread((unsigned long)afile, (char *)xfile,
	    nfile * sizeof (struct file));
	for (fp=xfile; fp < &xfile[nfile]; fp++)
		if (fp->f_count)
			nf++;
	if (totflg) {
		(void) printf("%3d/%3d files\n", nf, nfile);
		return;
	}
	(void) printf("%d/%d open files\n", nf, nfile);
	(void) printf("   LOC   TYPE    FLG     CNT  MSG    DATA    OFFSET\n");
	for (fp=xfile,loc=(int)afile; fp < &xfile[nfile]; fp++,loc+=sizeof(xfile[0])) {
		if (fp->f_count==0)
			continue;
		(void) printf("%8x ", loc);
		if (fp->f_type <= DTYPE_SOCKET)
			(void) printf("%-8.8s", dtypes[fp->f_type]);
		else
			(void) printf("%8d", fp->f_type);
		putf(fp->f_flag&FREAD, 'R');
		putf(fp->f_flag&FWRITE, 'W');
		putf(fp->f_flag&FAPPEND, 'A');
		putf(fp->f_flag&FSHLOCK, 'S');
		putf(fp->f_flag&FEXLOCK, 'X');
		putf(fp->f_flag&FASYNC, 'I');
		(void) printf("  %3d", fp->f_count);
		(void) printf("  %3d", fp->f_msgcount);
		(void) printf("  %8.1x", fp->f_data);
		if (fp->f_offset < 0)
			(void) printf("  %x\n", fp->f_offset);
		else
			(void) printf("  %ld\n", fp->f_offset);
	}
	free((char *)xfile);
}

dostream()
{
	unsigned long curstream;
	struct stdata stream;
	struct vnode strvnode;
	register struct queue *qloc, *prevqloc;
	struct queue queue;

	kread((unsigned long)nl[SALLSTREAM].n_value, (char *)&curstream,
	    sizeof (struct stdata *));
	while (curstream != 0) {
		kread(curstream, (char *)&stream, sizeof (struct stdata));
		(void) printf(
"   LOC     WRQ       VNODE     DEVICE   PGRP SIGIO  FLAGS\n");
		(void) printf("%8x ", curstream);
		curstream = (unsigned long)stream.sd_next;
		(void) printf("  %8.1x", stream.sd_wrq);
		(void) printf("  %8.1x", stream.sd_vnode);
		if (stream.sd_vnode != NULL) {
			kread((unsigned long)stream.sd_vnode,
			    (char *)&strvnode, sizeof (struct vnode));
			(void) printf(" %3d,%3d", major(strvnode.v_rdev),
			    minor(strvnode.v_rdev));
		} else
			(void) printf("        ");
		(void) printf("  %5d", stream.sd_pgrp);
		(void) printf(" %5d", stream.sd_sigio);
		putchar(' ');
		putf(stream.sd_flag&IOCWAIT, 'I');
		putf(stream.sd_flag&RSLEEP, 'R');
		putf(stream.sd_flag&WSLEEP, 'W');
		putf(stream.sd_flag&STRPRI, 'P');
		putf(stream.sd_flag&STRHUP, 'H');
		putf(stream.sd_flag&STWOPEN, 'O');
		putf(stream.sd_flag&STPLEX, 'M');
		putf(stream.sd_flag&RMSGDIS, 'D');
		putf(stream.sd_flag&RMSGNODIS, 'N');
		putf(stream.sd_flag&STRERR, 'E');
		putf(stream.sd_flag&STRTIME, 'T');
		putf(stream.sd_flag&STR2TIME, '2');
		putf(stream.sd_flag&STR3TIME, '3');
		putf(stream.sd_flag&NBIO, 'B');
		putf(stream.sd_flag&ASYNC, 'A');
		putf(stream.sd_flag&OLDNDELAY, 'o');
		putf(stream.sd_flag&STRTOSTOP, 'S');
		putf(stream.sd_flag&CLKRUNNING, 'C');
		putf(stream.sd_flag&TIMEDOUT, 'V');
		putf(stream.sd_flag&RCOLL, 'r');
		putf(stream.sd_flag&WCOLL, 'w');
		putf(stream.sd_flag&ECOLL, 'e');
		putchar('\n');
		if ((qloc = stream.sd_wrq) == NULL)
			continue;
		(void) printf("  Write side:\n");
		(void) printf(
"    NAME      COUNT FLG    MINPS  MAXPS  HIWAT  LOWAT\n");
		while (qloc != NULL) {
			kread((unsigned long)qloc, (char *)&queue,
			    sizeof (struct queue));
			printq((int)qloc, &queue);
			prevqloc = qloc;
			qloc = queue.q_next;
		}
		(void) printf("  Read side:\n");
		qloc = RD(prevqloc);
		while (qloc != NULL) {
			kread((unsigned long)qloc, (char *)&queue,
			    sizeof (struct queue));
			printq((int)qloc, &queue);
			qloc = queue.q_next;
		}
		putchar('\n');
	}
}

printq(loc, qp)
	int loc;
	register struct queue *qp;
{
	struct qinit qinfo;
	struct module_info modinfo;
	char mname[FMNAMESZ];

	(void) strcpy(mname, "");
	if (qp->q_qinfo != NULL) {
		kread((unsigned long)qp->q_qinfo, (char *)&qinfo,
		    sizeof (struct qinit));
		if (qinfo.qi_minfo != NULL) {
			kread((unsigned long)qinfo.qi_minfo, (char *)&modinfo,
			    sizeof (struct module_info));
			if (modinfo.mi_idname != NULL)
				kread((unsigned long)modinfo.mi_idname, mname,
				    FMNAMESZ);
		}
	}
	(void) printf("    ");
	(void) printf("%-8.8s", mname);
	(void) printf("  %5d", qp->q_count);
	putchar(' ');
	putf(qp->q_flag&QENAB, 'E');
	putf(qp->q_flag&QWANTR, 'R');
	putf(qp->q_flag&QWANTW, 'W');
	putf(qp->q_flag&QFULL, 'F');
	putf(qp->q_flag&QNOENB, 'N');
	(void) printf("  %5d", qp->q_minpsz);
	if (qp->q_maxpsz == INFPSZ)
		(void) printf("    INF");
	else
		(void) printf("  %5d", qp->q_maxpsz);
	(void) printf("  %5d", qp->q_hiwat);
	(void) printf("  %5d", qp->q_lowat);
	putchar('\n');
}

#define ctok(x) ((ctob(x))>>10)

doswap()
{
	struct anoninfo ai;

	kread(nl[SANON].n_value, (char *)&ai, sizeof (struct anoninfo));

	if (totflg)
		(void) printf("%3d/%3d swap\n", ctok(ai.ani_resv),
		    ctok(ai.ani_max));
	else {
		int nalloc;

		nalloc = ctok(ai.ani_max - ai.ani_free);
		(void) printf(
		    "%dk allocated + %dk reserved = %dk used, %dk available\n",
		    nalloc, ctok(ai.ani_resv) - nalloc, ctok(ai.ani_resv),
		    ctok(ai.ani_max - ai.ani_resv));
	}
}

/*
 * kread: invoke kvm_read & check result
 */
void
kread(addr, buf, len)
	unsigned long addr;
	char *buf;
	int len;
{
	if (kvm_read(kd, addr, buf, (unsigned)len) != len) {
		(void) fprintf(stderr, "pstat: kernel read error\n");
		exit(1);
	}
}
