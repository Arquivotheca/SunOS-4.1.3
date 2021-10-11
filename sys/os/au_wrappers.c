/*	@(#)au_wrappers.c 1.1 92/07/30 SMI;	*/

#include <sys/param.h>
#include <sys/dir.h>
#include <sys/label.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/vfs.h>
#include <sys/pathname.h>
#include <sys/vnode.h>
#include <sys/audit.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/kmem_alloc.h>
#include <ufs/inode.h>
#include <sys/kernel.h>
#include <rpc/types.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <netinet/in.h>

void cwdup();
void au_pathfree();

/*
 *
 * system call wrappers for auditing.
 *
 */

/*
 * Open system call.
 */
au_open(uap)
	register struct a {
		char *fnamep;
		int fmode;
		int cmode;
	} *uap;
{
	register int	a_mask;

	open(uap);
	/*
	 * audit the data read/write/create
	 */
	if (uap->fmode & O_RDWR)
		a_mask = AU_DWRITE | AU_DREAD;
	else if (uap->fmode & O_WRONLY)
		a_mask = AU_DWRITE;
	else
		a_mask = AU_DREAD;
	if (uap->fmode & FCREAT)
		a_mask |= AU_DCREATE;

	au_sysaudit(AUR_OPEN, a_mask, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fnamep);
}

/*
 * Creat system call.
 */
au_creat(uap)
	register struct a {
		char *fnamep;
		int cmode;
	} *uap;
{
	creat(uap);
	/*
	 * audit the object creation
	 */
	au_sysaudit(AUR_CREAT, AU_DCREATE | AU_DWRITE, u.u_error,
	    u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fnamep);
}


/*
 * Make a directory.
 */
au_mkdir(uap)
	struct a {
		char	*dirnamep;
		int	dmode;
	} *uap;
{
	mkdir(uap);

	au_sysaudit(AUR_MKDIR, AU_DCREATE, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->dirnamep);
}

/*
 * make a hard link
 */
au_link(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	link(uap);
	au_sysaudit(AUR_LINK, AU_DCREATE, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->from,
	    AUP_USER, (caddr_t)uap->to);
}


/*
 * rename or move an existing file
 */
au_rename(uap)
	register struct a {
		char	*from;
		char	*to;
	} *uap;
{

	rename(uap);
	au_sysaudit(AUR_RENAME, AU_DCREATE, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->from,
	    AUP_USER, (caddr_t)uap->to);
}

/*
 * Create a symbolic link.
 * Similar to link or rename except target
 * name is passed as string argument, not
 * converted to vnode reference.
 */
au_symlink(uap)
	register struct a {
		char	*target;
		char	*linkname;
	} *uap;
{

	symlink(uap);
	au_sysaudit(AUR_SYMLINK, AU_DCREATE, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->target,
	    AUP_USER, (caddr_t)uap->linkname);
}

/*
 * Unlink (i.e. delete) a file.
 */
au_unlink(uap)
	struct a {
		char	*pnamep;
	} *uap;
{

	unlink(uap);
	au_sysaudit(AUR_UNLINK, AU_DCREATE, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->pnamep);
}

/*
 * Remove a directory.
 */
au_rmdir(uap)
	struct a {
		char	*dnamep;
	} *uap;
{

	rmdir(uap);
	au_sysaudit(AUR_RMDIR, AU_DCREATE, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->dnamep);
}


/*
 * Determine accessibility of file, by
 * reading its attributes and then checking
 * against our protection policy.
 */
au_access(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	access(uap);
	au_sysaudit(AUR_ACCESS, AU_DREAD, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname);
}

/*
 * Change mode of file given path name.
 */
au_chmod(uap)
	register struct a {
		char	*fname;
		int	fmode;
	} *uap;
{
	chmod(uap);
	au_sysaudit(AUR_CHMOD, AU_DACCESS, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname,
	    sizeof (uap->fmode), (caddr_t)&(uap->fmode));
}

/*
 * Change mode of file given file descriptor.
 */
au_fchmod(uap)
	register struct a {
		int	fd;
		int	fmode;
	} *uap;
{
	fchmod(uap);
	au_sysaudit(AUR_FCHMOD, AU_DACCESS, u.u_error, u.u_rval1, 2,
	    sizeof (uap->fd), (caddr_t)&(uap->fd),
	    sizeof (uap->fmode), (caddr_t)&(uap->fmode));
}

/*
 * Change ownership of file given file name.
 */
au_chown(uap)
	register struct a {
		char	*fname;
		int	uid;
		int	gid;
	} *uap;
{
	chown(uap);
	au_sysaudit(AUR_CHOWN, AU_DACCESS, u.u_error, u.u_rval1, 5,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname,
	    sizeof (uap->uid), (caddr_t)&(uap->uid),
	    sizeof (uap->gid), (caddr_t)&(uap->gid));
}

/*
 * Change ownership of file given file descriptor.
 */
au_fchown(uap)
	register struct a {
		int	fd;
		int	uid;
		int	gid;
	} *uap;
{
	fchown(uap);
	au_sysaudit(AUR_FCHOWN, AU_DACCESS, u.u_error, u.u_rval1, 3,
	    sizeof (uap->fd), (caddr_t)&(uap->fd),
	    sizeof (uap->uid), (caddr_t)&(uap->uid),
	    sizeof (uap->gid), (caddr_t)&(uap->gid));
}

/*
 * Truncate a file given its path name.
 */
au_truncate(uap)
	register struct a {
		char	*fname;
		int	length;
	} *uap;
{
	truncate(uap);
	au_sysaudit(AUR_TRUNCATE, AU_DWRITE, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname,
	    sizeof (uap->length), (caddr_t)&(uap->length));
}

/*
 * Truncate a file given a file descriptor.
 */
au_ftruncate(uap)
	register struct a {
		int	fd;
		int	length;
	} *uap;
{
	ftruncate(uap);
	au_sysaudit(AUR_FTRUNCATE, AU_DWRITE, u.u_error, u.u_rval1, 2,
	    sizeof (uap->fd), (caddr_t)&(uap->fd),
	    sizeof (uap->length), (caddr_t)&(uap->length));
}


/*
 * This wrapper for ptrace could go elsewhere.
 *
 * ptrace
 *
 */
/*
 * sys-trace system call.
 */
au_ptrace()
{
	register struct a {
		enum ptracereq req;
		int	pid;
		caddr_t	addr;
		int	data;
		caddr_t addr2;
	} *uap;

	ptrace();

	uap = (struct a *)u.u_ap;
	au_sysaudit(AUR_PTRACE, AU_DWRITE, u.u_error, u.u_rval1, 5,
	    sizeof (uap->req), (caddr_t)&(uap->req),
	    sizeof (uap->pid), (caddr_t)&(uap->pid),
	    sizeof (uap->addr), (caddr_t)&(uap->addr),
	    sizeof (uap->data), (caddr_t)&(uap->data),
	    sizeof (uap->addr2), (caddr_t)&(uap->addr2));
}


/*
 * These wrappers for kill* could go elsewhere.
 *
 * kill killpg
 *
 */
au_kill()
{
	register struct a {
		int	pid;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	kill();
	au_sysaudit(AUR_KILL, AU_DWRITE, u.u_error, u.u_rval1, 2,
	    sizeof (uap->pid), (caddr_t)&uap->pid,
	    sizeof (uap->signo), (caddr_t)&uap->signo);
}

au_killpg()
{
	register struct a {
		int	pgrp;
		int	signo;
	} *uap = (struct a *)u.u_ap;

	killpg();
	au_sysaudit(AUR_KILLPG, AU_DWRITE, u.u_error, u.u_rval1, 2,
	    sizeof (uap->pgrp), (caddr_t)&uap->pgrp,
	    sizeof (uap->signo), (caddr_t)&uap->signo);
}


au_settimeofday()
{
	register struct a {
		struct	timeval *tv;
		struct	timezone *tzp;
	} *uap = (struct a *)u.u_ap;

	settimeofday();
	au_sysaudit(AUR_SETTIMEOFDAY, AU_MINPRIV, u.u_error, u.u_rval1, 2,
	    (AUP_USER | sizeof (struct timeval)), uap->tv,
	    (AUP_USER | sizeof (struct timezone)), uap->tzp);

}


au_adjtime()
{
	register struct a {
		struct timeval *delta;
		struct timeval *olddelta;
	} *uap = (struct a *)u.u_ap;

	adjtime();
	au_sysaudit(AUR_ADJTIME, AU_MINPRIV, u.u_error, u.u_rval1, 2,
	    (AUP_USER | sizeof (struct timeval)), uap->delta,
	    (AUP_USER | sizeof (struct timeval)), uap->olddelta);
}

/*
 * System call interface to the socket abstraction.
 */

au_socket()
{
	register struct a {
		int	domain;
		int	type;
		int	protocol;
	} *uap = (struct a *)u.u_ap;

	socket();
	au_sysaudit(AUR_SOCKET, AU_DWRITE | AU_DREAD | AU_DCREATE,
	    u.u_error, u.u_rval1, 3,
	    sizeof (uap->domain), (caddr_t)&uap->domain,
	    sizeof (uap->type), (caddr_t)&uap->type,
	    sizeof (uap->protocol), (caddr_t)&uap->protocol);
}

/*
 * execve and execv. We go through the trouble of saving the path
 * here because it's gone by the time exec returns.
 */
struct execa {
	char	*fname;
	char	**argp;
	char	**envp;
};

au_execv()
{
	struct pathname pnp;
	int error;

	error = pn_get(((struct execa *)u.u_ap)->fname, UIO_USERSPACE, &pnp);
	execv();
	au_sysaudit(AUR_EXECV, AU_DREAD, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    0, (caddr_t)pnp.pn_path);
	if (!error)
		pn_free(&pnp);
}

au_execve()
{
	struct pathname pnp;
	int error;

	error = pn_get(((struct execa *)u.u_ap)->fname, UIO_USERSPACE, &pnp);
	execve();
	au_sysaudit(AUR_EXECVE, AU_DREAD, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    0, (caddr_t)pnp.pn_path);
	if (!error)
		pn_free(&pnp);
}

/*
 * from kern_xxx.c
 */

au_sethostname()
{
	register struct a {
		char	*hostname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;
	char	*old_name;
	u_int	old_len;

	old_len = hostnamelen + 1;
	old_name = new_kmem_alloc(old_len, KMEM_SLEEP);
	bcopy(hostname, old_name, old_len);

	sethostname();
	au_sysaudit(AUR_SETHOSTNAME, AU_MAJPRIV, u.u_error, u.u_rval1, 2,
	    old_len, (caddr_t)old_name,
	    (AUP_USER | (uap->len+1)), (caddr_t)uap->hostname);
	kmem_free(old_name, old_len);
}

au_setdomainname()
{
	register struct a {
		char	*domainname;
		u_int	len;
	} *uap = (struct a *)u.u_ap;
	char	*old_name;
	u_int	old_len;

	old_len = domainnamelen + 1;
	old_name = new_kmem_alloc(old_len, KMEM_SLEEP);
	bcopy(domainname, old_name, old_len);

	setdomainname();
	au_sysaudit(AUR_SETDOMAINNAME, AU_MAJPRIV, u.u_error, u.u_rval1, 2,
	    old_len, (caddr_t)old_name,
	    (AUP_USER | (uap->len+1)), (caddr_t)uap->domainname);
	kmem_free(old_name, old_len);
}

/*
 * Notice that the wrapper for reboot writes a record BEFORE the call
 * actually occurs and another if it fails. Simply put, we want to know
 * what happened without reguard to its success or failure.
 */
au_reboot()
{
	register struct a {
		int	opt;
	} *uap = (struct a *)u.u_ap;

	au_sysaudit(AUR_REBOOT, AU_MAJPRIV, AU_EITHER, 0, 1,
	    sizeof (uap->opt), (caddr_t)&uap->opt);
	reboot();

	au_sysaudit(AUR_REBOOTFAIL, AU_MAJPRIV, u.u_error, u.u_rval1, 1,
	    sizeof (uap->opt), (caddr_t)&uap->opt);
}

/*
 * Accounting
 */

au_sysacct(uap)
	register struct a {
		char	*fname;
	} *uap;
{
	sysacct(uap);
	au_sysaudit(AUR_SYSACCT, AU_MINPRIV | AU_DWRITE,
	    u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname);
}

/*
 * NFS and disk system calls
 */

/*
 * mount system call
 */
au_mount(uap)
	register struct a {
		int	type;
		char	*dir;
		int	flags;
		caddr_t	data;
	} *uap;
{
	struct ufs_args ufs_a;
	struct nfs_args nfs_a;

	mount(uap);

	switch (uap->type) {
	case MOUNT_UFS:
		if (copyin(uap->data, (caddr_t)&ufs_a, sizeof (ufs_a))) {
			/* get something deterministic */
			bzero((caddr_t)&ufs_a, sizeof (ufs_a));
		}
		au_sysaudit(AUR_MOUNT_UFS, AU_MAJPRIV, u.u_error,
		    u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->type), (caddr_t)&uap->type,
		    AUP_USER, (caddr_t)uap->dir,
		    sizeof (uap->flags), (caddr_t)&uap->flags,
		    AUP_USER, (caddr_t)ufs_a.fspec);
		break;
	case MOUNT_NFS:
		if (copyin(uap->data, (caddr_t)&nfs_a, sizeof (nfs_a))) {
			/* get something deterministic */
			bzero((caddr_t)&nfs_a, sizeof (nfs_a));
		}
		au_sysaudit(AUR_MOUNT_NFS, AU_MAJPRIV, u.u_error,
		    u.u_rval1, 13,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->type), (caddr_t)&uap->type,
		    AUP_USER, (caddr_t)uap->dir,
		    sizeof (uap->flags), (caddr_t)&uap->flags,
		    (AUP_USER | sizeof (struct sockaddr_in)),
		    (caddr_t)nfs_a.addr,
		    (AUP_USER | sizeof (fhandle_t)), (caddr_t)nfs_a.fh,
		    sizeof (nfs_a.flags), (caddr_t)&nfs_a.flags,
		    sizeof (nfs_a.wsize), (caddr_t)&nfs_a.wsize,
		    sizeof (nfs_a.rsize), (caddr_t)&nfs_a.rsize,
		    sizeof (nfs_a.timeo), (caddr_t)&nfs_a.timeo,
		    sizeof (nfs_a.retrans), (caddr_t)&nfs_a.retrans,
		    AUP_USER, (caddr_t)nfs_a.hostname);
		break;
	default:
		au_sysaudit(AUR_MOUNT, AU_MAJPRIV, u.u_error, u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->type), (caddr_t)&uap->type,
		    AUP_USER, (caddr_t)uap->dir,
		    sizeof (uap->flags), (caddr_t)&uap->flags,
		    sizeof (uap->data), (caddr_t)&uap->data);
		break;
	}
}

/*
 * Unmount system call.
 */
au_unmount(uap)
	struct a {
		char	*pathp;
	} *uap;
{

	unmount(uap);

	au_sysaudit(AUR_UNMOUNT, AU_MAJPRIV, u.u_error, u.u_rval1, 1,
	    AUP_USER, (caddr_t)uap->pathp);
}


/*
 * Set access/modify times on named file.
 */
au_utimes(uap)
	register struct a {
		char	*fname;
		struct	timeval *tptr;
	} *uap;
{
	utimes(uap);
	au_sysaudit(AUR_UTIMES, AU_DWRITE, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname,
	    (AUP_USER | (2 * sizeof (struct timeval))), (caddr_t)uap->tptr);
}

/*
 * Read contents of symbolic link.
 */
au_readlink(uap)
	register struct a {
		char	*name;
		char	*buf;
		int	count;
	} *uap;
{
	readlink(uap);
	au_sysaudit(AUR_READLINK, AU_DREAD, u.u_error, u.u_rval1, 5,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->name,
	    AUP_USER, (caddr_t)uap->buf,
	    sizeof (uap->count), (caddr_t)&(uap->count));
}


/*
 * get filesystem statistics
 */
au_statfs(uap)
	struct a {
		char *path;
		struct statfs *buf;
	} *uap;
{

	statfs(uap);
	au_sysaudit(AUR_STATFS, AU_DREAD, u.u_error, u.u_rval1, 4,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->path,
	    (AUP_USER | sizeof (struct statfs)), (caddr_t)uap->buf);
}

/*
 * Change current working directory (".").
 */
au_chdir(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct ucwd *cwd;
	au_path_t au_path;

	au_path.ap_size = 0;
	u.u_error = chdirec(uap->dirnamep, -1, &u.u_cdir, &au_path);
	if (u.u_error == 0) {
		cwdup(u.u_cwd, (caddr_t)NULL, au_path.ap_buf, &cwd);
		cwfree(u.u_cwd);
		u.u_cwd = cwd;
	}
	au_sysaudit(AUR_CHDIR, AU_DACCESS, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->dirnamep);
	au_pathfree(&au_path);
}

/*
 * This is chdir with all the problems imaginable for auditing.
 *
 * Act as follows for the various audit conditions:
 *
 * AUC_UNSET:		Auditing might get started some time. return error
 * AUC_AUDITING:	Auditing is being done. return error
 * AUC_NOAUDIT:		Auditing is not wanted. go ahead with the fchdir
 *			but set the condition to AUC_FCHDONE
 * AUC_FCHDONE:		Auditing can never be started. go ahead and fchdir
 *
 */

au_fchdir(uap)
	register struct a {
		int	fd;
	} *uap;
{

	if (au_auditon(AUC_FCHDONE) != 0)
		u.u_error = EINVAL;
	else
		fchdir(uap);
}

/*
 * Change notion of root ("/") directory.
 */
au_chroot(uap)
	register struct a {
		char *dirnamep;
	} *uap;
{
	struct ucwd *cwd;
	register int error;
	au_path_t au_path;

	au_path.ap_size = 0;
	error = chdirec(uap->dirnamep, -1, &u.u_rdir, &au_path);

	au_sysaudit(AUR_CHROOT, AU_MINPRIV, error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->dirnamep);

	if (error == 0) {
		cwdup(u.u_cwd, au_path.ap_buf, (caddr_t)NULL, &cwd);
		cwfree(u.u_cwd);
		u.u_cwd = cwd;
	}
	au_pathfree(&au_path);
	u.u_error = error;
}

/*
 * This is chroot with all the problems imaginable for auditing.
 *
 * Act as follows for the various audit conditions:
 *
 * AUC_UNSET:		Allow only to the REAL, honest to God, root.
 * AUC_AUDITING:	Allow only to the REAL, honest to God, root.
 * AUC_NOAUDIT:		Auditing is not wanted. go ahead with the fchroot
 *			but set the condition to AUC_FCHDONE
 * AUC_FCHDONE:		Auditing can never be started. go ahead
 *
 */
au_fchroot(uap)
	register struct a {
		int	fd;
	} *uap;
{
	register u_int path_size;
	register u_int root_size;
	register u_int dir_size;
	char *new_path;
	struct ucwd *cwd;
	struct ucwd *t_cwd;
	struct file *fp;

	if (au_auditon(AUC_FCHDONE) != 0) {
		if (u.u_error = getvnodefp(uap->fd, &fp)) {
			return;
		}
		if (VN_CMP((struct vnode *)(fp->f_data), rootdir) == 0) {
			u.u_error = EINVAL;
			return;
		}
	}

	/*
	 * We need to set dir=<root>"/"<dir>, root="/"
	 */
	root_size = strlen(u.u_cwd->cw_root);
	dir_size = strlen(u.u_cwd->cw_dir);
	path_size = root_size + dir_size + 2;
	new_path = new_kmem_alloc(path_size, KMEM_SLEEP);
	bcopy(u.u_cwd->cw_root, new_path, root_size);
	bcopy(u.u_cwd->cw_dir, new_path + root_size, dir_size + 1);
	cwdup(u.u_cwd, (caddr_t)NULL, new_path, &t_cwd);
	cwdup(t_cwd, "", (caddr_t)NULL, &cwd);
	cwfree(t_cwd);
	kmem_free(new_path, path_size);

	fchroot(uap);
	au_sysaudit(AUR_CHROOT, AU_MINPRIV, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    0, "/");
	if (u.u_error != 0) {
		cwfree(cwd);
		return;
	}
	cwfree(u.u_cwd);
	u.u_cwd = cwd;
}
/*
 * Get attributes from file or file descriptor.
 * Argument says whether to follow links, and is
 * passed through in flags.
 */
au_stat(uap0)
	caddr_t uap0;
{
	register struct a {
		char *fname;
		struct stat *ub;
	} *uap = (struct a *)uap0;

	stat(uap0);
	au_sysaudit(AUR_STAT, AU_DREAD, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname);
}

au_lstat(uap0)
	caddr_t uap0;
{
	register struct a {
		char *fname;
		struct stat *ub;
	} *uap = (struct a *)uap0;

	lstat(uap0);
	au_sysaudit(AUR_STAT, AU_DREAD, u.u_error, u.u_rval1, 3,
	    0, (caddr_t)u.u_cwd->cw_root,
	    0, (caddr_t)u.u_cwd->cw_dir,
	    AUP_USER, (caddr_t)uap->fname);
}
