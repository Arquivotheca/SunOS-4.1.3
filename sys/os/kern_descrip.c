/*      @(#)kern_descrip.c 1.1 92/07/30 SMI; from UCB 7.1 6/5/86 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/kernel.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/asynch.h>

#include <rpc/types.h>
#ifdef	NFSSERVER
#include <nfs/nfs.h>
#endif

/*
 * Descriptor management.
 */

/*
 * TODO:
 *	eliminate u.u_error side effects
 */

/*
 * Private routine forward declarations.
 */
static void	dupit(/* fd, fp, flags */);
static void	alloc_fd_slots();

/*
 * System calls on descriptors.
 */


/*
 * Getdtablesize is implemented as a system call only for binary
 * compatibility.  As of SunOS 4.1 it's really just the value of the soft
 * RLIMIT_NOFILE resource limit.
 */
getdtablesize()
{

	u.u_r.r_val1 = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;
}

getdopt()
{

}

setdopt()
{

}

dup()
{
	register struct a {
		int	i;
	} *uap = (struct a *) u.u_ap;
	struct file *fp;
	int j;

#ifdef	notdef
	/*
	 * This code is obsolete; since the system now supports > 077 open
	 * descriptors, it's no longer possible to allow dup on high-valued
	 * descriptors to act like dup2.
	 */
	if (uap->i &~ 077) {
		uap->i &= 077;
		dup2();
		return;
	}
#endif	notdef

	GETF(fp, uap->i);
	j = ufalloc(0);
	if (j < 0)
		return;
	dupit(j, fp, u.u_pofile[uap->i]);
}

dup2()
{
	register struct a {
		int	i, j;
	} *uap = (struct a *) u.u_ap;
	register struct file *fp;
	register int fd_1, fd_2;

	fd_1 = uap->i;
	fd_2 = uap->j;

	GETF(fp, fd_1);

	/*
	 * Validate and prepare the target slot.
	 */
	if (fd_2 < 0 || fd_2 >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur) {
		u.u_error = EBADF;
		return;
	}

	if (fd_2 >= NOFILE_IN_U && u.u_ofile == u.u_ofile_arr)
		alloc_fd_slots();

	u.u_r.r_val1 = fd_2;
	if (fd_1 == fd_2)
		return;
	if (u.u_ofile[fd_2]) {
		/* Release all System-V style record locks, if any */
		(void) vno_lockrelease(u.u_ofile[fd_2]);	/* errors? */
		closef(u.u_ofile[fd_2]);
		/*
		 * Even if an error occurred when calling the close routine
		 * for the vnode or the device, the file table entry has
		 * had its reference count decremented anyway.  As such,
		 * the descriptor is closed, so there's not much point
		 * in worrying about errors; we might as well pretend
		 * the "close" succeeded.
		 */
		u.u_error = 0;
	}
	dupit(fd_2, fp, u.u_pofile[fd_1]);
}

static void
dupit(fd, fp, flags)
	int fd;
	register struct file *fp;
	register int flags;
{

	u.u_ofile[fd] = fp;
	u.u_pofile[fd] = flags &~ UF_EXCLOSE;
	fp->f_count++;
	if (fd > u.u_lastfile)
		u.u_lastfile = fd;
}

/*
 * The file control system call.
 */
fcntl()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		int	cmd;
		int	arg;
	} *uap;
	register i;
	register char *pop;

	/* remote locks use an extended ld structure. */
	union {
		struct flock fl;
		struct eflock efl;
	} xld;
#define	rld (xld.fl)	/* regular lock descriptor */
#define	eld (xld.efl)	/* extended lock descriptor */

	register int oldwhence;
	register int newflag;
	int fioarg, type;

#ifdef	NFSSERVER
	/* for F_CNVT */
	struct f_cnvt_arg {
		fhandle_t *fh;
		int filemode;
	};
	struct f_cnvt_arg a;
	register struct f_cnvt_arg *ap;
	fhandle_t tfh;
	register int filemode;
	int error, indx, mode;
	struct vnode *vp;
	struct vnode *myfhtovp();
	extern struct fileops vnodefops;
#endif

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	pop = &u.u_pofile[uap->fdes];
	switch (uap->cmd) {
	case F_DUPFD:
		i = uap->arg;
		if (i < 0 || i >= u.u_rlimit[RLIMIT_NOFILE].rlim_cur) {
			u.u_error = EINVAL;
			return;
		}
		if ((i = ufalloc(i)) < 0)
			return;
		dupit(i, fp, *pop);
		break;

	case F_GETFD:
		u.u_r.r_val1 = *pop & 1;
		break;

	case F_SETFD:
		*pop = (*pop &~ 1) | (uap->arg & 1);
		break;

	case F_GETFL:
		u.u_r.r_val1 = fp->f_flag+FOPEN;
		break;

	case F_SETFL:
		/*
		 * XXX Actually, there should not be any connection
		 * between the "ioctl"-settable per-object FIONBIO
		 * and FASYNC flags and any per-file-descriptor flags,
		 * so this call should simply fiddle fp->f_flag.
		 * Unfortunately, 4.2BSD has such a connection, so we
		 * must support that.  Thus, we must pass the new
		 * values of the FNDELAY and FASYNC flags down to the
		 * object by doing the appropriate "ioctl"s.
		 */
		newflag = fp->f_flag;
		newflag &= FCNTLCANT;
		newflag |= (uap->arg-FOPEN) &~ FCNTLCANT;
		fioarg = (newflag & FNDELAY) != 0;
		u.u_error = fioctl(fp, FIONBIO, (caddr_t) &fioarg);
		if (u.u_error)
			break;
		fioarg = (newflag & FASYNC) != 0;
		u.u_error = fioctl(fp, FIOASYNC, (caddr_t) &fioarg);
		if (u.u_error) {
			fioarg = (fp->f_flag & FNDELAY) != 0;
			(void) fioctl(fp, FIONBIO, (caddr_t) &fioarg);
			break;
		}
		fp->f_flag = newflag;
		break;

	case F_GETOWN:
		u.u_error = fioctl(fp, FIOGETOWN, (caddr_t) &u.u_r.r_val1);
		break;

	case F_SETOWN:
		u.u_error = fioctl(fp, FIOSETOWN, (caddr_t) &uap->arg);
		break;

		/* System-V Record-locking (lockf() maps to fcntl()) */
	case F_GETLK:
	case F_SETLK:
	case F_SETLKW:
		/* remote System-V compatible locks. */
	case F_RGETLK:
	case F_RSETLK:
	case F_RSETLKW:
		/* First off, allow only vnodes here */
		if (fp->f_type != DTYPE_VNODE) {
			u.u_error = EBADF;
			return;
		}
		/* get flock structure from user-land */
		if ((uap->cmd == F_RGETLK) ||
		    (uap->cmd == F_RSETLK) || (uap->cmd == F_RSETLKW))
			u.u_error = copyin((caddr_t)uap->arg, (caddr_t)&eld,
						sizeof (eld));
		else 
			u.u_error = copyin((caddr_t)uap->arg, (caddr_t)&rld,
						sizeof (rld));
		if (u.u_error)	return;

		/*
		 * *** NOTE ***
		 * The SVID does not say what to return on file access errors!
		 * Here, EBADF is returned, which is compatible with S5R3
		 * and is less confusing than EACCES
		 */
		type = eld.l_type;
		switch (type) {
		case F_RDLCK:
			if (!((uap->cmd == F_GETLK) ||
				(uap->cmd == F_RGETLK)) &&
				!(fp->f_flag & FREAD)) {
				u.u_error = EBADF;
				return;
			}
			break;

		case F_WRLCK:
			if (!((uap->cmd == F_GETLK) ||
				(uap->cmd == F_RGETLK)) &&
				!(fp->f_flag & FWRITE)) {
				u.u_error = EBADF;
				return;
			}
			break;

		case F_UNLCK:
		case F_UNLKSYS:
			break;

		default:
			u.u_error = EINVAL;
			return;
		}

		/* convert offset to start of file */
		oldwhence = eld.l_whence; /* save to renormalize later */
		if (u.u_error = extend_rewhence(&eld, fp, 0))
			return;

		/* convert negative lengths to positive */
		if (eld.l_len < 0) {
			eld.l_start += eld.l_len; /* adjust start point */
			eld.l_len = -(eld.l_len); /* absolute value */
		}


		/* check for validity */
		if ((eld.l_start < 0) ||
		    ((eld.l_len != 0) && 	/* check for arith. overflow */
		     (eld.l_start > (eld.l_start + eld.l_len - 1)))) {
			u.u_error = EINVAL;
			return;
		}

		if (((uap->cmd == F_SETLK) || (uap->cmd == F_SETLKW) ||
			(uap->cmd == F_RSETLK) || (uap->cmd == F_RSETLKW)) &&
			(eld.l_type != F_UNLCK)) {
			/*
			 * If any locking is attempted, mark file locked
			 * to force unlock on close.
			 * Also, since the SVID specifies that the FIRST
			 * close releases all locks, mark process to
			 * reduce the search overhead in vno_lockrelease().
			 * NB: Remote locks are not unlocked on close since
			 * the lock manager is only getting the lock for another
			 * process who has marked the file on that system. When
			 * it closes over there, the Lock manager will get the
			 * nod and unlock it over here. However, if the lock
			 * manager exits, all of the locks it was holding as
			 * proxies are removed.
			 */
			u.u_procp->p_flag |= SLKDONE;
			if (uap->cmd != F_RSETLK)
				*pop |= UF_FDLOCK;
		}

		/*
		 * Dispatch out to vnode layer to do the actual locking.
		 * Then, translate error codes for SVID compatibility
		 */
		switch (u.u_error = VOP_LOCKCTL((struct vnode *)fp->f_data,
			&eld, uap->cmd, fp->f_cred, u.u_procp->p_pid)) {
			case 0:
				break;		/* continue, if successful */
			case EWOULDBLOCK:
				u.u_error = EACCES;	/* EAGAIN ??? */
				return;
			default:
				return;		/* some other error code */
		}

		/* if F_GETLK, return flock structure to user-land */
		/* per SVID, change only 'l_type' field if unlocked */
		if (u.u_error = extend_rewhence(&eld, fp, oldwhence))
			return;
		if ((uap->cmd == F_RGETLK) ||
		    (uap->cmd == F_RSETLK) || (uap->cmd == F_RSETLKW))
			u.u_error = copyout((caddr_t)&eld,
				(caddr_t)uap->arg, sizeof (eld));
		else
			u.u_error = copyout((caddr_t)&rld,
				(caddr_t)uap->arg, sizeof (rld));

		if (u.u_error) return;

		break;

#ifdef	NFSSERVER
	/*
	 *	F_CNVT fcntl:  given a pointer to an fhandle_t and a mode, open
	 *	the file corresponding to the fhandle_t with the given mode and
	 *	return a file descriptor.  Note:  uap->fd is unused.
	 */
	case F_CNVT:
		if (!suser())
			break;

		if (error = copyin((caddr_t) uap->arg, (caddr_t) &a,
			sizeof (a))) {
			u.u_error = error;
			break;
		} else
			ap = &a;
		if (error = copyin((caddr_t) ap->fh, (caddr_t) &tfh,
			sizeof (tfh))) {
			u.u_error = error;
			break;
		}

		filemode = ap->filemode - FOPEN;
		if (filemode & FCREAT) {
			u.u_error = EINVAL;
			break;
		}
		mode = 0;
		if (filemode & FREAD)
			mode |= VREAD;
		if (filemode & (FWRITE | FTRUNC))
			mode |= VWRITE;

		/*
		 *	Adapted from copen:
		 */
		fp = falloc();
		if (fp == (struct file *) NULL)
			break;
		else
			indx = u.u_r.r_val1;

		/*
		 *	This takes the place of lookupname in copen.  Note that
		 *	we can't use the normal fhtovp function because we want
		 *	this to work on files that may not have been exported.
		 */
		if ((vp = myfhtovp(&tfh)) == (struct vnode *) NULL) {
			error = ESTALE;
			goto out;
		}

		/*
		 *	Adapted from vn_open:
		 */
		if (filemode & (FWRITE | FTRUNC)) {
			if (vp->v_type == VDIR) {
				error = EISDIR;
				goto out;
			}
			if (isrofile(vp)) {
				error = EROFS;
				goto out;
			}
		}
		error = VOP_ACCESS(vp, mode, u.u_cred);
		if (error)
			goto out;
		if (vp->v_type == VSOCK) {
			error = EOPNOTSUPP;
			goto out;
		}
		error = VOP_OPEN(&vp, filemode, u.u_cred);
		if ((error == 0) && (filemode & FTRUNC)) {
			struct vattr vattr;

			filemode &= ~FTRUNC;
			vattr_null(&vattr);
			vattr.va_size = 0;
			error = VOP_SETATTR(vp, &vattr, u.u_cred);
		}
		if (error)
			goto out;

		/*
		 *	Adapted from copen:
		 */
		fp->f_flag = filemode & FMASK;
		fp->f_type = DTYPE_VNODE;
		fp->f_data = (caddr_t) vp;
		fp->f_ops = &vnodefops;
		if (vp->v_type == VFIFO)
			fp->f_flag |= (filemode & FNDELAY);
		break;
out:
		u.u_ofile[indx] = NULL;
		crfree(fp->f_cred);
		fp->f_count = 0;
		if (vp)
			VN_RELE(vp);
		u.u_error = error;
		break;
#endif
	default:
		u.u_error = EINVAL;
	}
}

#ifdef	NFSSERVER
/*
 * We require a version of fhtovp that simply converts an fhandle_t to
 * a vnode without any ancillary checking (e.g., whether it's exported).
 */
static struct vnode *
myfhtovp(fh)
	fhandle_t *fh;
{
	int error;
	struct vnode *vp;
	register struct vfs *vfsp;

	vfsp = getvfs(&fh->fh_fsid);
	if (vfsp == (struct vfs *) NULL) {
		return ((struct vnode *) NULL);
	}
	error = VFS_VGET(vfsp, &vp, (struct fid *)&(fh->fh_len));
	if (error || vp == (struct vnode *) NULL) {
#ifdef	NFSDEBUG
		dprint(nfsdebug, 1, "myfhtovp(%x) couldn't vget\n", fh);
#endif
		return ((struct vnode *) NULL);
	}
	return (vp);
}
#endif

fioctl(fp, cmd, value)
	struct file *fp;
	int cmd;
	caddr_t value;
{

	return ((*fp->f_ops->fo_ioctl)(fp, cmd, value));
}

close()
{
	register struct a {
		int	i;
	} *uap = (struct a *)u.u_ap;
	register int i = uap->i;
	register struct file *fp;
	register u_char *pf;
	extern void astop();

	GETF(fp, i);

#ifdef	ASYNCHIO
#ifdef	LWP
	/* Stop asynchronous I/O on the file descriptor, if necessary. */
	if (u.u_procp->p_aio_forw != NULL)
		astop(i, (struct aio_result_t *)ALL_AIO);
#endif	LWP
#endif	ASYNCHIO

	/* Release all System-V style record locks, if any */
	(void) vno_lockrelease(fp);	/* WHAT IF error returned? */

	pf = (u_char *)&u.u_pofile[i];
	u.u_ofile[i] = NULL;
	while (u.u_lastfile >= 0 && u.u_ofile[u.u_lastfile] == NULL)
		u.u_lastfile--;
	*pf = 0;
	closef(fp);
	/* WHAT IF u.u_error ? */
}

fstat()
{
	register struct file *fp;
	register struct a {
		int	fdes;
		struct	stat *sb;
	} *uap;
	struct stat ub;

	uap = (struct a *)u.u_ap;
	GETF(fp, uap->fdes);
	switch (fp->f_type) {

	case DTYPE_VNODE:
		u.u_error = vno_stat((struct vnode *)fp->f_data, &ub);
		break;

	case DTYPE_SOCKET:
		u.u_error = soo_stat((struct socket *)fp->f_data, &ub);
		break;

	default:
		panic("fstat");
		/* NOTREACHED */
	}
	if (u.u_error == 0)
		u.u_error = copyout((caddr_t)&ub, (caddr_t)uap->sb,
		    sizeof (ub));
}

/*
 * Allocate descriptor slots in the range [0 .. NOFILE), assuming that
 * the range [0 .. NOFILE_IN_U) is currently allocated.
 */
static void
alloc_fd_slots()
{
	register struct file	**ofpp;
	register char		*pofpp;

	/*
	 * Allocate the expanded arrays and make them active.
	 */
	ofpp = (struct file **) new_kmem_alloc(
			NOFILE * sizeof *(u.u_ofile), KMEM_SLEEP);
	pofpp = new_kmem_alloc(NOFILE * sizeof *(u.u_pofile), KMEM_SLEEP);
	u.u_ofile = ofpp;
	u.u_pofile = pofpp;

	/*
	 * Initialize the expanded arrays.  This could be optimized
	 * a bit by not bzeroing over the part that will be bcopied.
	 */
	bzero((caddr_t)ofpp, NOFILE * sizeof *(u.u_ofile));
	bzero((caddr_t)pofpp, NOFILE * sizeof *(u.u_pofile));
	bcopy((caddr_t)u.u_ofile_arr, (caddr_t)ofpp,
		NOFILE_IN_U * sizeof *(u.u_ofile));
	bcopy((caddr_t)u.u_pofile_arr, (caddr_t)pofpp,
		NOFILE_IN_U * sizeof *(u.u_pofile));
}

/*
 * Allocate a user file descriptor.
 */
int
ufalloc(i)
	register int i;
{
	register int fdlimit = u.u_rlimit[RLIMIT_NOFILE].rlim_cur;

	for (; i < fdlimit; i++) {
		/*
		 * Make sure that slot i exists before referring to it.
		 */
		if (i >= NOFILE_IN_U && u.u_ofile == u.u_ofile_arr)
			alloc_fd_slots();

		if (u.u_ofile[i] == NULL) {
			if (i > u.u_lastfile)
				u.u_lastfile = i;
			u.u_r.r_val1 = i;
			u.u_pofile[i] = 0;
			return (i);
		}
	}
	u.u_error = EMFILE;
	return (-1);
}

/*
 * Return number of descriptor slots that are available for use.  Respects the
 * current soft RLIMIT_NOFILE value.
 */
int
ufavail()
{
	register int i;
	register int maxfd;
	register int avail;
	register int softmaxfd;

	/*
	 * All slots from u.u_lastfile + 1 to the current soft limit are
	 * available.  Slots from 0 to u.u_lastfile must be examined
	 * explicitly.
	 */
	softmaxfd = u.u_rlimit[RLIMIT_NOFILE].rlim_cur - 1;
	maxfd = u.u_lastfile;

	if (softmaxfd > maxfd)
		avail = softmaxfd - maxfd;
	else
		avail = 0;

	if (maxfd > softmaxfd)
		maxfd = softmaxfd;
	for (i = 0; i <= maxfd; i++)
		if (u.u_ofile[i] == NULL)
			avail++;
	return (avail);
}

struct	file *lastf;
/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
struct	file *
falloc()
{
	register struct file *fp;
	register i;

	i = ufalloc(0);
	if (i < 0)
		return (NULL);
	if (lastf == 0)
		lastf = file;
	for (fp = lastf; fp < fileNFILE; fp++)
		if (fp->f_count == 0)
			goto slot;
	for (fp = file; fp < lastf; fp++)
		if (fp->f_count == 0)
			goto slot;
	tablefull("file");
	u.u_error = ENFILE;
	return (NULL);
slot:
	u.u_ofile[i] = fp;
	fp->f_count = 1;
	fp->f_type = 0;
	fp->f_ops = NULL;
	fp->f_data = 0;
	fp->f_offset = 0;
	crhold(u.u_cred);
	fp->f_cred = u.u_cred;
	lastf = fp + 1;
	return (fp);
}

/*
 * Convert a user supplied file descriptor into a pointer
 * to a file structure.  Only task is to check range of the descriptor.
 * Critical paths should use the GETF macro.
 */
struct	file *
getf(f)
	register int f;
{
	register struct file *fp;
	register int lastfile = u.u_lastfile;

	if (f < 0 || f > lastfile || (fp = u.u_ofile[f]) == NULL) {
		u.u_error = EBADF;
		return (NULL);
	}
	return (fp);
}

/*
 * Internal form of close.
 * Decrement reference count on file structure.
 */
closef(fp)
	register struct file *fp;
{

	if (fp == NULL)
		return;
	if (fp->f_type == DTYPE_VNODE) {
		register struct vnode *vp;
		extern void strclean();

		vp = (struct vnode *)fp->f_data;
		/*
		 * If it's a stream, must clean up on EVERY close.
		 */
		if (vp->v_type == VCHR && vp->v_stream)
			strclean(vp);
	}
	if (fp->f_count < 1)
		panic("closef: count < 1");

	if (fp->f_ops != NULL)
		(*fp->f_ops->fo_close)(fp);
	fp->f_count--;
	if (fp->f_count == 0)
		crfree(fp->f_cred);
}

/*
 * Normalize SystemV-style record locks
 */
rewhence(ld, fp, newwhence)
	struct flock *ld;
	struct file *fp;
	int newwhence;
{
	struct vattr va;
	register int error;

	/* if reference to end-of-file, must get current attributes */
	if ((ld->l_whence == 2) || (newwhence == 2)) {
		if (error = VOP_GETATTR((struct vnode *)fp->f_data, &va,
		    u.u_cred))
			return (error);
	}

	if (ld->l_whence != newwhence) {	/* normalize to start of file */
		switch (ld->l_whence) {
		case 0:
			break;
		case 1:
			ld->l_start += fp->f_offset;
			break;
		case 2:
			ld->l_start += va.va_size;
			break;
		default:
			return (EINVAL);
		}
	} else {			/* renormalize to given start point */
		switch (ld->l_whence) {
		case 1:
			ld->l_start -= fp->f_offset;
			break;
		case 2:
			ld->l_start -= va.va_size;
			break;
		}
	}
	return (0);
}

/*
 * Normalize SystemV-style record locks
 */
extend_rewhence(ld, fp, newwhence)
	struct eflock *ld;
	struct file *fp;
	int newwhence;
{
	struct vattr va;
	register int error;

	/* if reference to end-of-file, must get current attributes */
	if ((ld->l_whence == 2) || (newwhence == 2)) {
		if (error = VOP_GETATTR((struct vnode *)fp->f_data, &va,
			u.u_cred))
			return (error);
	}

	if (ld->l_whence != newwhence) {	/* normalize to start of file */
		switch (ld->l_whence) {
		case 0:
			break;
		case 1:
			ld->l_start += fp->f_offset;
			break;
		case 2:
			ld->l_start += va.va_size;
			break;
		default:
			return (EINVAL);
		}
	} else {	/* renormalize to given start point */
		switch (ld->l_whence) {
		case 1:
			ld->l_start -= fp->f_offset;
			break;
		case 2:
			ld->l_start -= va.va_size;
			break;
		}
	}
	return (0);
}


/*
 * Apply an advisory lock on a file descriptor.
 */
flock()
{
	register struct a {
		int	fd;
		int	how;
	} *uap = (struct a *)u.u_ap;
	register struct file *fp;

	GETF(fp, uap->fd);
	if (fp->f_type != DTYPE_VNODE) {
		u.u_error = EOPNOTSUPP;
		return;
	}

	if (uap->how & LOCK_UN) {
		vno_bsd_unlock(fp, FSHLOCK|FEXLOCK);
		return;
	}
	/* check for valid lock type */
	if (uap->how & LOCK_EX)
		uap->how &= ~LOCK_SH;		/* can't have both types */
	else if (!(uap->how & LOCK_SH)) {
		u.u_error = EINVAL;		/* but must have one */
		return;
	}
	u.u_error = vno_bsd_lock(fp, uap->how);
}
