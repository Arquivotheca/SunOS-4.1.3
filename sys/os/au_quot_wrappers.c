/*	@(#)au_quot_wrappers.c 1.1 92/07/30 SMI;	*/

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
#include <ufs/inode.h>
#include <sys/kernel.h>
#include <rpc/types.h>
#include <nfs/nfs.h>
#include <sys/mount.h>
#include <netinet/in.h>
#include <ufs/quota.h>

/*
 * Sys call to allow users to find out
 * their current position wrt quota's
 * and to allow super users to alter it.
 */
au_quotactl(uap)
	register struct a {
		int	cmd;
		caddr_t	fdev;
		int	uid;
		caddr_t	addr;
	} *uap;
{
	quotactl(uap);

	switch (uap->cmd) {
	case Q_QUOTAON:
		au_sysaudit(AUR_QUOTA_ON, AU_MINPRIV,
		    u.u_error, u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid),
		    AUP_USER, (caddr_t)uap->addr);
		break;
	case Q_QUOTAOFF:
		au_sysaudit(AUR_QUOTA_OFF, AU_MINPRIV,
		    u.u_error, u.u_rval1, 5,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid));
		break;
	case Q_GETQUOTA:
		/* Do nothing - this isn't priviledged */
		break;
	case Q_SETQUOTA:
		au_sysaudit(AUR_QUOTA_SET, AU_MINPRIV,
		    u.u_error, u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid),
		    (AUP_USER | sizeof (struct dqblk)), (caddr_t)uap->addr);
		break;
	case Q_SETQLIM:
		au_sysaudit(AUR_QUOTA_LIM, AU_MINPRIV,
		    u.u_error, u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid),
		    (AUP_USER | sizeof (struct dqblk)), (caddr_t)uap->addr);
		break;
	case Q_SYNC:
		au_sysaudit(AUR_QUOTA_SYNC, AU_MINPRIV,
		    u.u_error, u.u_rval1, 5,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid));
		break;
	default:
		au_sysaudit(AUR_QUOTA, AU_MINPRIV, u.u_error, u.u_rval1, 6,
		    0, (caddr_t)u.u_cwd->cw_root,
		    0, (caddr_t)u.u_cwd->cw_dir,
		    sizeof (uap->cmd), (caddr_t)&(uap->cmd),
		    AUP_USER, (caddr_t)uap->fdev,
		    sizeof (uap->uid), (caddr_t)&(uap->uid),
		    sizeof (uap->addr),
						(caddr_t)&(uap->addr));
		break;
	}
}
