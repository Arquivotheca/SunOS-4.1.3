#ident	"@(#)kern_acct.c 1.1 92/07/30 SMI"	/* from UCB 7.1 6/5/86 */

/*
 * Copyright (c) 1982, 1986 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/vnode.h>
#include <sys/vfs.h>
#include <sys/kernel.h>
#include <sys/acct.h>
#include <sys/uio.h>

/*
 * SHOULD REPLACE THIS WITH A DRIVER THAT CAN BE READ TO SIMPLIFY.
 */
struct	vnode *acctp;
struct	vnode *savacctp;
struct	ucred *acctcred;

/*
 * Perform process accounting functions.
 */
sysacct(uap)
	register struct a {
		char	*fname;
	} *uap;
{
	struct vnode *vp;
	struct vnode *vpsav;

	/*
	 * The next 4 line of codes will not be documented in man page.
	 * The feature is used in System V acct() system call wrapper.
	 * If given argument is 1, acct() system call returns the value
	 * which tells the caller if accounting is currently on or not.
	 */
	if (uap->fname == (char *)1) {
		u.u_r.r_val1 = (int)acctp;
		return;
	}

	if (suser()) {
		if (savacctp) {
			acctp = savacctp;
			savacctp = NULL;
		}
		if (uap->fname == NULL) {
			if (vp = acctp) {
				acctp = NULL;
				VN_RELE(vp);
			}
			return;
		}
		u.u_error = lookupname(uap->fname, UIO_USERSPACE, FOLLOW_LINK,
		    (struct vnode **)0, &vp);
		if (u.u_error)
			return;
		if (vp->v_type != VREG) {
			u.u_error = EACCES;
			VN_RELE(vp);
			return;
		}
		if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
			u.u_error = EROFS;
			VN_RELE(vp);
			return;
		}
		if (vpsav = acctp) {
			acctp = NULL;
			VN_RELE(vpsav);
		}
		acctp = vp;
		if (acctcred)
			crfree(acctcred);
		acctcred = crdup(u.u_cred);
	}
}

int	acctsuspend = 2;	/* stop accounting when < 2% free space left */
int	acctresume = 4;		/* resume when free space risen to > 4% */

struct	acct acctbuf;

/*
 * On exit, write a record on the accounting file.
 */
acct(st)
{
	register struct vnode *vp;
	register struct rusage *ru;
	struct timeval t;
	register struct acct *ap = &acctbuf;
	struct statfs sb;
	struct ucred *oldcred;

	if (savacctp) {
		(void) VFS_STATFS(savacctp->v_vfsp, &sb);
		if (sb.f_bavail > (acctresume * (sb.f_blocks / 100))) {
			acctp = savacctp;
			savacctp = NULL;
			printf("Accounting resumed\n");
		}
	}
	if ((vp = acctp) == NULL)
		return;
	VN_HOLD(vp);
	(void) VFS_STATFS(acctp->v_vfsp, &sb);
	if (sb.f_bavail <= (acctsuspend * (sb.f_blocks / 100))) {
		savacctp = acctp;
		acctp = NULL;
		printf("Accounting suspended\n");
		VN_RELE(vp);
		return;
	}
	ru = &u.u_ru;
	bcopy(u.u_comm, ap->ac_comm, sizeof (ap->ac_comm));
	ap->ac_btime = u.u_start.tv_sec;
	ap->ac_utime = compress((long)scale60(&ru->ru_utime));
	ap->ac_stime = compress((long)scale60(&ru->ru_stime));
	t = time;
	timevalsub(&t, &u.u_start);
	ap->ac_etime = compress((long)scale60(&t));
	ap->ac_mem = compress(ru->ru_ixrss + ru->ru_idrss + ru->ru_isrss);
	ap->ac_io = compress(u.u_ioch);
	ap->ac_rw = compress(ru->ru_inblock + ru->ru_oublock);
	ap->ac_uid = u.u_ruid;
	ap->ac_gid = u.u_rgid;
	if (u.u_procp->p_sessp->s_ttyp)
		ap->ac_tty = u.u_procp->p_sessp->s_ttyd;
	else
		ap->ac_tty = NODEV;
	ap->ac_stat = st;
	ap->ac_flag = u.u_acflag;
	oldcred = u.u_cred;
	u.u_cred = acctcred;
	u.u_error = vn_rdwr(UIO_WRITE, vp, (caddr_t)ap, sizeof (acctbuf), 0,
	    UIO_SYSSPACE, IO_UNIT|IO_APPEND, (int *)0);
	u.u_cred = oldcred;
	VN_RELE(vp);
}

/*
 * Produce a pseudo-floating point representation
 * with 3 bits base-8 exponent, 13 bits fraction.
 */
compress(t)
	register long t;
{
	register exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return ((exp<<13) + t);
}
