#ident	"@(#)vfs_io.c 1.1 92/07/30"

#include <sys/param.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

int vno_rw();
int vno_ioctl();
int vno_select();
int vno_close();

struct fileops vnodefops = {
	vno_rw,
	vno_ioctl,
	vno_select,
	vno_close
};

extern struct vnodeops ufs_vnodeops;

int
vno_rw(fp, rw, uiop)
	struct file *fp;
	enum uio_rw rw;
	struct uio *uiop;
{
	register struct vnode *vp;
	register int count;
	register int iomode;
	register int error;

	vp = (struct vnode *)fp->f_data;

	/*
	 * If write, make sure filesystem is writable.
	 */
	if ((rw == UIO_WRITE) && isrofile(vp))
		return (EROFS);

	count = uiop->uio_resid;
	iomode = 0;
	if (vp->v_type == VREG)
		iomode |= IO_UNIT;
	if (fp->f_flag & FAPPEND)
		iomode |= IO_APPEND;
	if (fp->f_flag & FSYNC)
		iomode |= IO_SYNC;
	if (fp->f_flag & FNBIO)
		iomode |= IO_NDELAY;
	error = VOP_RDWR(vp, uiop, rw, iomode, fp->f_cred);

	if (error)
		return (error);
	if (fp->f_flag & FAPPEND || vp->v_type == VFIFO) {
		/*
		 * The actual offset used for append is set by VOP_RDWR
		 * so compute actual starting location.
		 */
		fp->f_offset = uiop->uio_offset - (count - uiop->uio_resid);
	}
	return (0);
}

int
vno_ioctl(fp, com, data)
	struct file *fp;
	int com;
	caddr_t data;
{
	struct vattr vattr;
	register struct vnode *vp;
	int error = 0;

	vp = (struct vnode *)fp->f_data;

	/*
	 * 4.2BSD translated FIOSETOWN on any non-socket descriptor
	 * into a TIOCSPGRP with a suitably-modified argument; if the
	 * argument was positive, it was treated as a process ID, and
	 * the value used was the ID of the process group to which that
	 * process belonged, while if the argument was negative, it was
	 * treated as a process group ID, and the value used was
	 * its absolute value.
	 *
	 * We do not so map FIOSETOWN; we just map the argument and pass
	 * it down, so that pseudo-tty masters can treat TIOCSPGRP as a
	 * request to set the process group of the slave side, while
	 * treating FIOSETOWN as a request to set the owner of the master
	 * side (so that asynchronous I/O can be supported on the master side).
	 */
	if (com == FIOSETOWN) {
		register int pid;
		if ((pid = *(int *)data) > 0) {
			register struct proc *p = pfind(pid);
			if (p == 0)
				return (ESRCH);
			*(int *)data = -p->p_pgrp;
		}
	}

	switch (vp->v_type) {

	case VREG:
	case VDIR:
	case VFIFO:
		switch (com) {

		case FIONREAD:
			error = VOP_GETATTR(vp, &vattr, u.u_cred);
			if (error == 0)
				*(off_t *)data = vattr.va_size - fp->f_offset;
			break;

		case FIONBIO:
		case FIOASYNC:
			break;

		default:
			if (vp->v_op == &ufs_vnodeops) {
				error = VOP_IOCTL(vp, com, data, fp->f_flag,
					fp->f_cred);
			} else
				error = ENOTTY;
			break;
		}
		break;

	case VCHR:
		u.u_r.r_val1 = 0;
		if (setjmp(&u.u_qsave)) {
			if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
				error = EINTR;
			else
				u.u_eosys = RESTARTSYS;
		} else {
			error = VOP_IOCTL(vp, com, data, fp->f_flag,
				fp->f_cred);
		}
		break;

	default:
		error = ENOTTY;
		break;
	}

	return (error);
}

int
vno_select(fp, flag)
	struct file *fp;
	int flag;
{
	struct vnode *vp;

	vp = (struct vnode *)fp->f_data;

	switch (vp->v_type) {

	case VCHR:
	case VFIFO:
		return (VOP_SELECT(vp, flag, fp->f_cred));

	default:
		/*
		 * Always selected
		 */
		return (1);
	}
}

int
vno_stat(vp, sb)
	register struct vnode *vp;
	register struct stat *sb;
{
	register int error;
	struct vattr vattr;

	error = VOP_GETATTR(vp, &vattr, u.u_cred);
	if (error)
		return (error);
	sb->st_mode = vattr.va_mode;
	sb->st_uid = vattr.va_uid;
	sb->st_gid = vattr.va_gid;
	sb->st_dev = vattr.va_fsid;
	sb->st_ino = vattr.va_nodeid;
	sb->st_nlink = vattr.va_nlink;
	sb->st_size = vattr.va_size;
	sb->st_blksize = vattr.va_blocksize;
	sb->st_atime = vattr.va_atime.tv_sec;
	sb->st_spare1 = 0;
	sb->st_mtime = vattr.va_mtime.tv_sec;
	sb->st_spare2 = 0;
	sb->st_ctime = vattr.va_ctime.tv_sec;
	sb->st_spare3 = 0;
	sb->st_rdev = (dev_t)vattr.va_rdev;
	sb->st_blocks = vattr.va_blocks;
	sb->st_spare4[0] = sb->st_spare4[1] = 0;
	return (0);
}

int
vno_close(fp)
	register struct file *fp;
{
	register struct vnode *vp;

	vp = (struct vnode *)fp->f_data;
	if ((fp->f_count == 1) && (fp->f_flag & (FSHLOCK | FEXLOCK)))
		vno_bsd_unlock(fp, (FSHLOCK | FEXLOCK));

	u.u_error = vn_close(vp, fp->f_flag, fp->f_count);
	if (fp->f_count == 1)
		VN_RELE(vp);
	return (u.u_error);
}

/*
 * This routine is called for every file close (excluding kernel processes
 * that call closef() directly) in order to implement the brain-damaged
 * SVID 'feature' that the FIRST close of a descriptor that refers to
 * a locked object causes all the locks to be released for that object.
 * It is called, for example, by close(), exit(), exec(), & dup2().
 *
 * NOTE: If the SVID ever changes to hold locks until the LAST close,
 * 	then this routine might be moved to closef() [note that the
 * 	window system calls closef() directly for file descriptors
 * 	that is has dup'ed internally....such descriptors may or may
 * 	not count towards holding a lock]
 *
 * TODO: The record-lock flag should be in the u-area.
 */
int
vno_lockrelease(fp)
	register struct file *fp;
{

	/*
	 * Only do extra work if the process has done record-locking.
	 */
	if (u.u_procp->p_flag & SLKDONE) {
		register struct vnode *vp;
		register struct file *ufp;
		register int i;
		register int locked;
		struct flock ld;

		locked = 0;		/* innocent until proven guilty */
		u.u_procp->p_flag &= ~SLKDONE;	/* reset process flag */
		vp = (struct vnode *)fp->f_data;
		/*
		 * Check all open files to see if there's a lock
		 * possibly held for this vnode.
		 */
		for (i = u.u_lastfile; i >= 0; i--) {
			if (((ufp = u.u_ofile[i]) != NULL) &&
			    (u.u_pofile[i] & UF_FDLOCK)) {

				/* the current file has an active lock */
				if ((struct vnode *)ufp->f_data == vp) {

					/* release this lock */
					locked = 1;	/* (later) */
					u.u_pofile[i] &= ~UF_FDLOCK;
				} else {

					/* another file is locked */
					u.u_procp->p_flag |= SLKDONE;
				}
			}
		} /* for all files */

		/*
		 * If 'locked' is set, release any locks that this process
		 * is holding on this file.  If record-locking on any other
		 * files was detected, the process was marked (SLKDONE) to
		 * run thru this loop again at the next file close.
		 */
		if (locked) {
			ld.l_type = F_UNLCK;	/* set to unlock entire file */
			ld.l_whence = 0;	/* unlock from start of file */
			ld.l_start = 0;
			ld.l_len = 0;		/* do entire file */
			return (VOP_LOCKCTL(vp, &ld, F_SETLK, u.u_cred,
						u.u_procp->p_pid));
		}
	}
	return (0);
}

/*
 * Place an advisory lock on an inode.
 *
 * NOTE: This is left for compatibility with the BSD flock() system call.
 *	 It is functionally superceded by the SystemV fcntl()/lockf()
 *	 mechanism, which has slightly different semantics, but works
 *	 in the network environment.
 */
int
vno_bsd_lock(fp, cmd)
	register struct file *fp;
	int cmd;
{
	register int priority;
	register struct vnode *vp;

	/*
	 * Avoid work.
	 */
	if ((fp->f_flag & FEXLOCK) && (cmd & LOCK_EX) ||
	    (fp->f_flag & FSHLOCK) && (cmd & LOCK_SH))
		return (0);

	priority = PLOCK;
	vp = (struct vnode *)fp->f_data;

	if ((cmd & LOCK_EX) == 0)
		priority++;
	if (setjmp(&u.u_qsave)) {
		if ((u.u_sigintr & sigmask(u.u_procp->p_cursig)) != 0)
			return (EINTR);
		u.u_eosys = RESTARTSYS;
		return (0);
	}
	/*
	 * If there's a exclusive lock currently applied
	 * to the file, then we've gotta wait for the
	 * lock with everyone else.
	 */
again:
	while (vp->v_flag & VEXLOCK) {
		/*
		 * If we're holding an exclusive
		 * lock, then release it.
		 */
		if (fp->f_flag & FEXLOCK) {
			vno_bsd_unlock(fp, FEXLOCK);
			continue;
		}
		if (cmd & LOCK_NB)
			return (EWOULDBLOCK);
		vp->v_flag |= VLWAIT;
		(void) sleep((caddr_t)&vp->v_exlockc, priority);
	}
	if ((cmd & LOCK_EX) && (vp->v_flag & VSHLOCK)) {
		/*
		 * Must wait for any shared locks to finish
		 * before we try to apply a exclusive lock.
		 * If we're holding a shared
		 * lock, then release it.
		 */
		if (fp->f_flag & FSHLOCK) {
			vno_bsd_unlock(fp, FSHLOCK);
			goto again;
		}
		if (cmd & LOCK_NB)
			return (EWOULDBLOCK);
		vp->v_flag |= VLWAIT;
		(void) sleep((caddr_t)&vp->v_shlockc, PLOCK);
		goto again;
	}
	if (fp->f_flag & FEXLOCK)
		panic("vno_bsd_lock");
	if (cmd & LOCK_EX) {
		vp->v_exlockc++;
		vp->v_flag |= VEXLOCK;
		fp->f_flag |= FEXLOCK;
	}
	if ((cmd & LOCK_SH) && (fp->f_flag & FSHLOCK) == 0) {
		vp->v_shlockc++;
		vp->v_flag |= VSHLOCK;
		fp->f_flag |= FSHLOCK;
	}
	return (0);
}

/*
 * Unlock a file.
 *
 * NOTE: This is left for compatibility with the BSD flock() system call.
 *	 It is functionally superceded by the SystemV fcntl()/lockf()
 *	 mechanism, which has slightly different semantics, but works
 *	 in the network environment.
 */
int
vno_bsd_unlock(fp, kind)
	register struct file *fp;
	int kind;
{
	register struct vnode *vp;
	register int flags;

	vp = (struct vnode *)fp->f_data;
	kind &= fp->f_flag;
	if (vp == NULL || kind == 0)
		return;
	flags = vp->v_flag;
	if (kind & FSHLOCK) {
		if ((flags & VSHLOCK) == 0)
			panic("vno_bsd_unlock: SHLOCK");
		if (--vp->v_shlockc == 0) {
			vp->v_flag &= ~VSHLOCK;
			if (flags & VLWAIT)
				wakeup((caddr_t)&vp->v_shlockc);
		}
		fp->f_flag &= ~FSHLOCK;
	}
	if (kind & FEXLOCK) {
		if ((flags & VEXLOCK) == 0)
			panic("vno_bsd_unlock: EXLOCK");
		if (--vp->v_exlockc == 0) {
			vp->v_flag &= ~(VEXLOCK|VLWAIT);
			if (flags & VLWAIT)
				wakeup((caddr_t)&vp->v_exlockc);
		}
		fp->f_flag &= ~FEXLOCK;
	}
}
