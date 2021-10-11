/*	@(#)kern_exec.c 1.1 92/07/30 SMI; from UCB 7.1 86/06/05	*/

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <machine/reg.h>
#include <machine/pte.h>
#include <machine/psl.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mman.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/socketvar.h>
#include <sys/vnode.h>
#include <sys/pathname.h>
#include <sys/vm.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/acct.h>
#include <sys/vfs.h>
#include <sys/asynch.h>
#include <sys/syslog.h>
#include <sys/trace.h>

#include <vm/hat.h>
#include <vm/anon.h>
#include <vm/as.h>
#include <vm/seg.h>
#include <vm/seg_vn.h>
#include <machine/seg_kmem.h>

#ifdef sparc
#include <machine/asm_linkage.h>
#endif sparc

#ifdef	sun4m
#ifndef	MULTIPROCESSOR
#define	cpuid	(0)
#else	MULTIPROCESSOR
extern int	cpuid;
#endif	MULTIPROCESSOR
#endif	sun4m

/*
 * If the PREREAD(size) macro evaulates true, then we will preread in
 * the given text or data a.out segment even though the file is ZMAGIC.
 */
#define	PREREAD(size) \
	((int)btopr(size) < (int)(freemem - minfree) && btopr(size) < pgthresh)

int pgthresh = btopr(PGTHRESH);		/* maximum preread size */

#define	ARG_HUNKSIZE	0x4000

struct arg_hunk {
	struct arg_hunk *a_next;
	char a_buf[ARG_HUNKSIZE - sizeof (struct arg_hunk *)];
};

#define	ARGS_GET_IN_PROGRESS	0x01
#define	ARGS_GET_WAITING	0x02

/*
 * Allocate new struct arg_hunk in the args virtual address area as
 * pageable data and return a pointer to the new area after linking
 * the new area on the arg_hunk struct pointer passed in.
 */
static struct arg_hunk *
args_get(a)
	struct arg_hunk *a;
{
	struct arg_hunk *na;
	addr_t base;
	u_int len;
	static char argsget_flag = 0;

	base = Syslimit;	/* start of virtual arg space area */
	len = NCARGS;
	/*
	 * There is a possible race here because we could be put to sleep
	 * between the as_hole call and the portion of the as_map call
	 * that locks down the space for us. Thus, two
	 * processes could both end up getting the same value back from
	 * as_hole. The solution is to only let one process through at
	 * a time.
	 */
	while (argsget_flag & ARGS_GET_IN_PROGRESS) {
		argsget_flag |= ARGS_GET_WAITING;
		(void) sleep((caddr_t)&argsget_flag, PZERO - 1);
	}
	argsget_flag = ARGS_GET_IN_PROGRESS;
	if (as_hole(&kas,
	    sizeof (struct arg_hunk), &base, &len, AH_LO) != A_SUCCESS) {
		u.u_error = E2BIG;
		na = (struct arg_hunk *)NULL;
		goto out;
	}

	u.u_error = as_map(&kas, base, sizeof (struct arg_hunk), segvn_create,
	    kzfod_argsp);
	if (u.u_error) {
		na = (struct arg_hunk *)NULL;
		goto out;
	}

	na = (struct arg_hunk *)base;
	if (a != NULL)
		a->a_next = na;
out:
	/*
	 * End of locked region.
	 */
	if (argsget_flag & ARGS_GET_WAITING) {
		wakeup((caddr_t)&argsget_flag);
	}
	argsget_flag = 0;
	return (na);
}

static void
args_free(a)
	register struct arg_hunk *a;
{
	register struct arg_hunk *ta;

	while ((ta = a) != NULL) {
		a = ta->a_next;
		if (as_unmap(&kas, (addr_t)ta, sizeof (*ta)) != A_SUCCESS)
			panic("args_free as_unmap");
	}
}

/*
 * exec system call, with and without environments.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

execv()
{

	((struct execa *)u.u_ap)->envp = NULL;
	execve();
}

execve()
{
	register int nc, error;
	register char *cp;
	register struct execa *uap;
	register struct arg_hunk *args;
	struct arg_hunk *args_start;
	int na, ne, ucp, ap;
	int indir, uid, gid;
	char *sharg, *execnamep;
	struct vnode *vp;
	struct vattr vattr;
	struct pathname pn;
	char cfarg[SHSIZE], exnam[SHSIZE];
	char *enp = &exnam[0];
	int resid;
	u_int len, cc;
	extern void astop();

	uap = (struct execa *)u.u_ap;
	indir = 0;
	uid = u.u_uid;
	gid = u.u_gid;
	args_start = NULL;
	vp = NULL;

	u.u_error = pn_get(uap->fname, UIO_USERSPACE, &pn);
	if (u.u_error != 0)
		return;

again:
	error = lookuppn(&pn, FOLLOW_LINK, (struct vnode **)0, &vp);
	if (error != 0)
		goto bad;

	if (vp->v_type != VREG) {
		error = EACCES;
		goto bad;
	}

	error = VOP_GETATTR(vp, &vattr, u.u_cred);
	if (error != 0)
		goto bad;

	if (indir == 0) {
		if (vp->v_vfsp->vfs_flag & VFS_NOSUID) {
			if ((vattr.va_mode & (VSUID | VSGID)) != 0) {
				log(LOG_WARNING,
		"%s, uid %d: setuid execution not allowed\n", pn.pn_buf, uid);
			}
		} else {
			if (vattr.va_mode & VSUID)
				uid = vattr.va_uid;
			if (vattr.va_mode & VSGID)
				gid = vattr.va_gid;
		}
	}

	/*
	 * XXX - should change VOP_ACCESS to not let super user
	 * always have it for exec permission on regular files.
	 */
	if (error = VOP_ACCESS(vp, VEXEC, u.u_cred))
		goto bad;
	if ((u.u_procp->p_flag & STRC) &&
	    (error = VOP_ACCESS(vp, VREAD, u.u_cred)))
		goto bad;
	if ((vattr.va_mode & (VEXEC | (VEXEC>>3) | (VEXEC>>6))) == 0) {
		error = EACCES;
		goto bad;
	}

	/*
	 * Read in first few bytes of file for segment sizes, ux_mag:
	 *	OMAGIC = plain executable
	 *	NMAGIC = RO text
	 *	ZMAGIC = demand paged RO text
	 * Also an ASCII line beginning with #! is
	 * the file name of a ``shell'' and arguments may be prepended
	 * to the argument list if given here.
	 *
	 * SHELL NAMES ARE LIMITED IN LENGTH.
	 *
	 * ONLY ONE ARGUMENT MAY BE PASSED TO THE SHELL FROM
	 * THE ASCII LINE.
	 */
	u.u_exdata.ux_shell[0] = '\0';	/* for zero length files */
	error = vn_rdwr(UIO_READ, vp, (caddr_t)&u.u_exdata,
	    sizeof (u.u_exdata), 0, UIO_SYSSPACE, IO_UNIT, &resid);
	if (error != 0)
		goto bad;

	if (resid > sizeof (u.u_exdata) - sizeof (u.u_exdata.Ux_A) &&
	    u.u_exdata.ux_shell[0] != '#') {
		error = ENOEXEC;
		goto bad;
	}

	switch (u.u_exdata.ux_mag) {
	case OMAGIC:
		u.u_exdata.ux_dsize += u.u_exdata.ux_tsize;
		u.u_exdata.ux_tsize = 0;
		break;

	case ZMAGIC:
	case NMAGIC:
		break;

	default:
		if (u.u_exdata.ux_shell[0] != '#' ||
		    u.u_exdata.ux_shell[1] != '!' ||
		    indir) {
			error = ENOEXEC;
			goto bad;
		}
		cp = &u.u_exdata.ux_shell[2];		/* skip "#!" */
		while (cp != &u.u_exdata.ux_shell[SHSIZE]) {
			if (*cp == '\t')
				*cp = ' ';
			else if (*cp == '\n') {
				*cp = '\0';
				break;
			}
			cp++;
		}
		if (*cp != '\0') {
			error = ENOEXEC;
			goto bad;
		}
		cp = &u.u_exdata.ux_shell[2];
		while (*cp == ' ')
			cp++;
		execnamep = cp;
		while (*cp && *cp != ' ')
			*enp++ = *cp++;
		*enp = '\0';

		cfarg[0] = '\0';
		if (*cp) {
			*cp++ = '\0';
			while (*cp == ' ')
				cp++;
			if (*cp)
				bcopy((caddr_t)cp, (caddr_t)cfarg, SHSIZE);
		}
		indir = 1;
		VN_RELE(vp);
		vp = NULL;
		if (error = pn_set(&pn, execnamep))
			goto bad;
		goto again;
	}

	/*
	 * Copy arguments into pageable kernel virtual memory
	 */
	na = ne = nc = 0;
	cc = 0;
	args = NULL;
	if (uap->argp) for (;;) {
		ap = NULL;
		sharg = NULL;
		if (indir && na == 0) {
			sharg = exnam;
			ap = (int)sharg;
			uap->argp++;		/* ignore argv[0] */
		} else if (indir && (na == 1 && cfarg[0])) {
			sharg = cfarg;
			ap = (int)sharg;
		} else if (indir && (na == 1 || na == 2 && cfarg[0])) {
			ap = (int)uap->fname;
		} else if (uap->argp) {
			ap = fuword((caddr_t)uap->argp);
			uap->argp++;
		}
		if (ap == NULL && uap->envp) {
			uap->argp = NULL;
			if ((ap = fuword((caddr_t)uap->envp)) != NULL)
				uap->envp++, ne++;
		}
		if (ap == NULL)
			break;
		na++;
		if (ap == -1) {
			error = EFAULT;
			goto bad;
		}
		do {
			if (cc == 0) {
				args = args_get(args);
				if (args == NULL) {
					error = u.u_error;
					goto bad;
				}
				if (args_start == NULL)
					args_start = args;
				cc = sizeof (args->a_buf);
				cp = args->a_buf;
			}
			if (sharg) {
				error = copystr(sharg, cp, cc, &len);
				sharg += len;
			} else {
				error = copyinstr((caddr_t)ap, cp, cc, &len);
				ap += len;
			}
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
		if (error != 0)
			goto bad;
	}
	nc = roundup(nc, NBPW);
#ifdef sparc
	/*
	 * Make sure user register windows are empty before attempting to
	 * make a new stack.
	 */
	flush_user_windows();
	error = getxfile(vp, SA(nc + (na + 4)*NBPW) + sizeof (struct rwindow),
			uid, gid, &pn);
#else
	error = getxfile(vp, nc + (na + 4) * NBPW, uid, gid, &pn);
#endif sparc
	if (error != 0)
		goto bad;

	VN_RELE(vp);
	vp = NULL;

	/*
	 * Copy back arglist.
	 */
	ucp = USRSTACK - nc - NBPW;
#ifdef sparc
	/*
	 * Keep stack aligned and leave room for initial reg window on sparc.
	 */
	ap = USRSTACK - SA(nc + (na + 4) * NBPW);
	u.u_ar0[SP] = ap - sizeof (struct rwindow);
#else
	ap = ucp - na * NBPW - 3 * NBPW;
	u.u_ar0[SP] = ap;
#endif sparc
	(void) suword((caddr_t)ap, na-ne);
	nc = 0;
	cc = 0;
	args = NULL;
	for (;;) {
		ap += NBPW;
		if (na == ne) {
			(void) suword((caddr_t)ap, 0);
			ap += NBPW;
		}
		if (--na < 0)
			break;
		(void) suword((caddr_t)ap, ucp);
		do {
			if (cc == 0) {
				if (args != NULL) {
					args_start = args->a_next;
					if (as_unmap(&kas, (addr_t)args,
					    sizeof (*args)) != A_SUCCESS)
						panic("execve: as_unmap");
				}
				args = args_start;
				cc = sizeof (args->a_buf);
				cp = args->a_buf;
			}
			error = copyoutstr(cp, (caddr_t)ucp, cc, &len);
			ucp += len;
			cp += len;
			nc += len;
			cc -= len;
		} while (error == ENAMETOOLONG);
		if (error)
			panic("exec: copyoutstr");
	}
	(void) suword((caddr_t)ap, 0);

#if defined(ASYNCHIO) && defined(LWP)
	if (u.u_procp->p_aio_forw != NULL)
		astop(ALL_AIO, (struct aio_result_t *)ALL_AIO);
#endif /* ASYNCHIO && LWP */

	/*
	 * Reset caught signals.  Held signals
	 * remain held through p_sigmask.
	 */
	while (u.u_procp->p_sigcatch) {
		nc = ffs((long)u.u_procp->p_sigcatch);
		u.u_procp->p_sigcatch &= ~sigmask(nc);
		u.u_signal[nc] = SIG_DFL;
	}

	/*
	 * Reset stack state to the user stack.
	 * Clear set of signals caught on the signal stack.
	 */
	u.u_onstack = 0;
	u.u_sigsp = 0;
	u.u_sigonstack = 0;

	/*
	 * Discard saved bus error state
	 */
#if defined(sun3) || defined(sun3x)
	u.u_pcb.u_berr_pc = 0;
	if (u.u_pcb.u_berr_stack != NULL) {
		kmem_free((caddr_t)u.u_pcb.u_berr_stack,
		    sizeof (struct user_buserr_stack));
		u.u_pcb.u_berr_stack = NULL;
	}
#endif sun3 || sun3x

	/*
	 * Reset list of interrupting and old-style signals; the new
	 * program's environment should not be affected by detritus from
	 * the old program.
	 */
	u.u_sigintr = 0;
	u.u_sigreset = 0;

	for (nc = u.u_lastfile; nc >= 0; --nc) {
		register struct file *f;

		/* close all Close-On-Exec files */
		if (u.u_pofile[nc] & UF_EXCLOSE) {
			f = u.u_ofile[nc];

			/* Release all System-V style record locks, if any */
			(void) vno_lockrelease(f);

			closef(f);
			u.u_ofile[nc] = NULL;
			u.u_pofile[nc] = 0;
		}
	}
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;
	setregs(u.u_exdata.ux_entloc);

	/*
	 * Remember file name for accounting.
	 */
	u.u_acflag &= ~AFORK;
	if (pn.pn_pathlen > MAXCOMLEN)
		pn.pn_pathlen = MAXCOMLEN;
	bcopy((caddr_t)pn.pn_buf, (caddr_t)u.u_comm,
	    (unsigned)(pn.pn_pathlen + 1));

	/*
	 * Remember that this process has execed (for posix)
	 */
	u.u_procp->p_flag |= SEXECED;

	trace4(TR_PROC_EXEC, trs(u.u_comm, 0), trs(u.u_comm, 1),
		trs(u.u_comm, 2), trs(u.u_comm, 3));
	trace6(TR_PROC_EXECARGS, args_start ? trs(args_start->a_buf, 0) : 0,
		args_start ? trs(args_start->a_buf, 1) : 0,
		args_start ? trs(args_start->a_buf, 2) : 0,
		args_start ? trs(args_start->a_buf, 3) : 0,
		args_start ? trs(args_start->a_buf, 4) : 0,
		args_start ? trs(args_start->a_buf, 5) : 0);
bad:
	u.u_error = error;
	pn_free(&pn);
	if (args_start)
		args_free(args_start);
	if (vp)
		VN_RELE(vp);
}

/*
 * Read in and set up memory for executed file.
 */
/*ARGSUSED*/
static int
getxfile(vp, nargc, uid, gid, pn)
	register struct vnode *vp;
	int nargc, uid, gid;
	struct pathname *pn;
{
	register int ts, ds, ss, bs;	/* size info */
	register struct proc *p = u.u_procp;
	caddr_t getdmem(), gettmem();
	addr_t tmem, dmem, bmem;
	int getts(), getds();
	u_int tfile, dfile, tsize;
	int pagi;
	register int error;

	/*
	 * Compute text and data sizes and make sure not too large.
	 */
	if (error = chkaout())
		return (error);
	ts = (int)getts();
	ds = (int)getds();
	ss = clrnd(SSIZE + btoc(nargc));
	if (chksize((unsigned)ts, (unsigned)ds, (unsigned)ss))
		return (u.u_error);

	/*
	 * Take a quick look to see if it looks like we will have
	 * enough swap space for the program to get started.  This
	 * is not a guarantee that we will succeed, but it is definitely
	 * better than finding this out after we are committed to the
	 * new memory image.  Maybe what is needed is a way to "prereserve"
	 * swap space for some segment mappings here.
	 *
	 * But with shared libraries the process can make it through
	 * the exec only to have ld.so fail to get the program going
	 * because its mmap's will not be able to success if the system
	 * is running low on swap space.  In fact this is a far more
	 * common failure mode, but we cannot do much about this here
	 * other than add some slop to our anonymous memory resources
	 * requirements estimate based on some guess since we cannot know
	 * what else the program will really need to get to a useful state.
	 */
	if (anoninfo.ani_max - anoninfo.ani_resv < ds + ss) {
		return (ENOMEM);
	}

	if ((u.u_exdata.ux_mag == ZMAGIC) &&
#if defined(sun2) || defined(sun3) || defined(sun3x)
	    (u.u_exdata.ux_mach != M_OLDSUN2) &&
#endif sun2 || sun3 || sun3x
	    ((vp->v_flag & VNOMAP) == 0))
		pagi = SPAGI;
	else
		pagi = 0;

	/*
	 * At this point, we are committed to the new image!
	 * Release virtual memory resources of old process, and
	 * initialize the virtual memory of the new process.
	 */
	relvm(p);
	p->p_flag &= ~(SPAGI|SSEQL|SUANOM);
	p->p_flag |= pagi;

	u.u_pcb.pcb_flags = AST_NONE;

	/*
	 * Get file and memory addresses for segments.  In preparation,
	 * store size parameters for lower-layer routines to reference.
	 */
	p->p_tsize = ts;
	p->p_dsize = ds;
	p->p_ssize = ss;
	tmem = gettmem();
	dmem = getdmem();
	tfile = gettfile();
	dfile = getdfile();

	/*
	 * Create an address space with stack, text,
	 * initialized data, and bss segments.
	 */
	p->p_as = as_alloc();

#ifdef	sun4m
	/*
	 * Since we are making this address space active,
	 * mark it active to keep the hat layer happy.
	 */
	p->p_as->a_hat.hat_oncpu = cpuid | 8;
#endif	sun4m

	if (ss) {
		/* stack */
		error = as_map(p->p_as, (addr_t)(USRSTACK - ctob(ss)),
		    (u_int)ctob(ss), segvn_create, zfod_argsp);
		if (error)
			goto out;
	}

#if defined(sun2) || defined(sun3) || defined(sun3x)
	if (u.u_exdata.ux_mach == M_OLDSUN2)
		tsize = u.u_exdata.ux_tsize;
	else
#endif sun2 || sun3 || sun3x
		tsize = (u_int)ctob(ts);

	if (tsize) {
		/* text */
		if (pagi) {
			error = VOP_MAP(vp, tfile, p->p_as, &tmem,
			    tsize, PROT_ALL & ~PROT_WRITE, PROT_ALL,
			    MAP_PRIVATE | MAP_FIXED, u.u_cred);
			if (error)
				goto out;

			/*
			 * If the text segment can fit, then we prefault
			 * the entire segment in.  This is based on the
			 * model that says the best working set of a
			 * small program is all of its pages.
			 */
			if (PREREAD(tsize)) {
				(void) as_fault(p->p_as, tmem, tsize,
				    F_INVAL, S_EXEC);
			}
		} else {
			/*
			 * Create UNIX text segment as
			 * private zero fill on demand.
			 *
			 * N.B.  There currently is no "shared text" for
			 * NMAGIC because we cannot get sharing on a non-page
			 * aligned vnode.  If we really wanted to do this,
			 * we could create an anon_map structure for files
			 * of type NMAGIC to have same effects of the old
			 * NMAGIC files, but it hardly seems worth it with
			 * the advent of shared libraries.
			 */
			error = as_map(p->p_as, tmem, tsize, segvn_create,
			    zfod_argsp);
			if (error)
				goto out;

			/*
			 * Read in the text in one big chunk
			 */
			error = vn_rdwr(UIO_READ, vp, (caddr_t)tmem,
			    (int)u.u_exdata.ux_tsize, (int)tfile,
			    UIO_USERSPACE, IO_UNIT, (int *)0);
			if (error)
				goto out;

			/* now make text segment read only */
			(void) as_setprot(p->p_as, tmem, tsize,
			    PROT_ALL & ~PROT_WRITE);
		}
	}

	if (u.u_exdata.ux_dsize) {
		/* data */
		if (pagi) {
			error = VOP_MAP(vp, dfile, p->p_as, &dmem,
			    (u_int)u.u_exdata.ux_dsize, PROT_ALL, PROT_ALL,
			    MAP_PRIVATE | MAP_FIXED, u.u_cred);
			if (error)
				goto out;

			/*
			 * If the initialized data segment can fit, then we
			 * prefault the entire segment in.  This is based on
			 * the model that says the best working set of a
			 * small program is all of its pages.
			 */
			if (PREREAD(u.u_exdata.ux_dsize)) {
				(void) as_fault(p->p_as, dmem,
				    (u_int)u.u_exdata.ux_dsize,
				    F_INVAL, S_READ);
			}
		} else {
			error = as_map(p->p_as, dmem,
			    (u_int)u.u_exdata.ux_dsize,
			    segvn_create, zfod_argsp);
			if (error)
				goto out;

			/* Read in the data in one big chunk */
			error = vn_rdwr(UIO_READ, vp, (caddr_t)dmem,
			    (int)u.u_exdata.ux_dsize, (int)dfile,
			    UIO_USERSPACE, IO_UNIT, (int *)0);
			if (error)
				goto out;
		}
	}

	bmem = (addr_t)roundup((u_int)dmem + u.u_exdata.ux_dsize, PAGESIZE);
	bs = btopr((dmem + u.u_exdata.ux_dsize + u.u_exdata.ux_bsize) - bmem);
	if (bs) {
		/* bss */
		error = as_map(p->p_as, bmem, (u_int)ctob(bs),
		    segvn_create, zfod_argsp);
		if (error)
			goto out;
	}

out:
	if (error) {
		u.u_error = error;
		swkill(p, "exec: I/O error mapping pages");
		/* NOTREACHED */
	}

	/*
	 * set SUID/SGID protections, if no tracing
	 */
	if ((p->p_flag & STRC) == 0) {
		if (uid != u.u_uid || gid != u.u_gid)
			u.u_cred = crcopy(u.u_cred);
		u.u_uid = uid;
		p->p_suid = uid;
		u.u_gid = gid;
		p->p_sgid = gid;
	} else
		psignal(p, SIGTRAP);
	u.u_prof.pr_scale = 0;
	return (error);
}
