/*	@(#)kern_prot.c 1.1 92/07/30 SMI; from UCB 5.17 83/05/27	*/

/*
 * System calls related to processes and protection
 */

#include <machine/reg.h>

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <sys/reboot.h>
#include <sys/buf.h>
#include <sys/acct.h>
#include <sys/debug.h>

getpid()
{

	u.u_r.r_val1 = u.u_procp->p_pid;
	u.u_r.r_val2 = u.u_procp->p_ppid;
}

getpgrp()
{
	register struct a {
		int	pid;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	if (uap->pid == 0)
		uap->pid = u.u_procp->p_pid;
	p = pfind(uap->pid);
	if (p == 0) {
		u.u_error = ESRCH;
		return;
	}
	u.u_r.r_val1 = p->p_pgrp;
}

getuid()
{

	u.u_r.r_val1 = u.u_ruid;
	u.u_r.r_val2 = u.u_uid;
}

getgid()
{

	u.u_r.r_val1 = u.u_rgid;
	u.u_r.r_val2 = u.u_gid;
}

getgroups()
{
	register struct	a {
		int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;

	for (gp = &u.u_groups[NGROUPS]; gp > u.u_groups; gp--)
		if (gp[-1] != NOGROUP)
			break;
	if (uap->gidsetsize == 0) {
		u.u_r.r_val1 = gp - u.u_groups;
		return;
	}
	if (uap->gidsetsize < gp - u.u_groups) {
		u.u_error = EINVAL;
		return;
	}
	uap->gidsetsize = gp - u.u_groups;
	u.u_error = copyout((caddr_t)u.u_groups, (caddr_t)uap->gidset,
	    (u_int)(uap->gidsetsize * sizeof (u.u_groups[0])));
	if (u.u_error)
		return;
	u.u_r.r_val1 = uap->gidsetsize;
}

sys_setsid()
{
	register struct a {
		int	flag;
	} *uap = (struct a *)u.u_ap;
	int ret;

	if (ret = setsid(uap->flag)) {
		u.u_error = ret;
	} else {
		u.u_rval1 = u.u_procp->p_pgrp;
		u.u_procp->p_flag |= SSETSID;
	}
}

struct	sess sess0;		/* default session */
struct	sess* allsess = &sess0;	/* linked list; sess0 always on end */

/*
 * setsid() - implements s5 setpgrp() and posix setsid()
 *
 * Make a process the session & the process group leader.
 *	check that this process is not already a session leader.
 *	check that there is not an existing process group == u.u_pid.
 *	detach it from its current controlling terminal.
 *	orphan it & check for possible orphaned children.
 *	allocate & initialize a session.
 *
 * N.B. The flag needs to be SESS_NEW for POSIX semantics.
 *	SESS_SYS is used to return to a BSD like startup state.
 */
setsid(flag)	/* tags note: you probably want sys_setsid() */
{
	register struct proc *p;
	register struct proc *p2;
	register struct sess *sp;
	register pid;
	int s;

	p = u.u_procp;
	sp = p->p_sessp;
	pid = p->p_pid;
	switch (flag) {
	    case SESS_NEW:
		if (pid == sp->s_sid && sp->s_sid != SESS_SYS) {
			/*
			 * Allow a process to call setsid() if it hasn't called
			 * it before.
			 * XXX - compat kludge; rm for 5.0
			 */
			if (p->p_flag & SSETSID)	/* this is not the first time */
				return EPERM;
			if (sp->s_members != 1)
				return (EPERM);
			if (sp->s_vp != NULL)
				return (EPERM);
			return (0);
		}
		for (p2 = pgfind(p->p_pgrp); p2; p2 = pgnext(p2)) {
			if (p2->p_pgrp == pid && p2 != p)
				return (EPERM);
		}
		sp = (struct sess *)new_kmem_zalloc(sizeof (*sp), KMEM_SLEEP);
		break;

	    case SESS_SYS:
		if (pid == sp->s_sid && sp->s_members > 1)
			return (EINVAL);
		break;

	    default:
		return (EINVAL);
	}
	/*
	 * protected because you should always be able to dereference
	 * u.u_procp->p_sessp->*
	 */
	s = splimp();
	if (flag == SESS_SYS) {
		sp = &sess0;
		pid = 0;
	} else {
		sp->s_next = allsess;
		allsess = sp;
	}
	SESS_EXIT(p);
	pgexit(p);
	pgenter(p, pid);
	SESS_ORPH(p);
	SESS_ENTER(p, sp);
	(void) splx(s);
	sp->s_sid = p->p_pgrp = pid;
	return (0);
}

/*
 * remove session from session list & free it
 */
freesess(sp)
	register struct sess *sp;
{
	register struct sess *f;

	ASSERT(sp->s_sid != SESS_SYS);
	ASSERT(sp->s_members == 0);
	SESS_VN_RELE(sp);
	if (allsess == sp) {
		allsess = sp->s_next;
		ASSERT(allsess);
	} else {
		for (f = allsess; f; f = f->s_next) {
			if (f->s_next == sp) {
				f->s_next = sp->s_next;
				break;
			}
		}
		ASSERT(f->s_next == sp->s_next);
	}
	kmem_free((char*)sp, sizeof (struct sess));
}

/*
 * Is the tty a controlling tty?
 */
struct	sess*
isactty(vp)
	register struct vnode *vp;
{
	register struct sess *sp;

	if (vp == NULL || vp->v_stream == NULL)
		return (NULL);
	for (sp = allsess; sp; sp = sp->s_next) {
		if (vp == sp->s_vp)
			return (sp);
	}
	return (NULL);
}

/*
 * BSD setpgrp()
 *
 * setpgrp(getpid(), 0) gets mapped to setsid()
 * all others get mapped to setpgid()
 */
setpgrp()
{
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_ap;

	/*
	 * If they are trying to protect themselves from the tty,
	 * give them a new session.
	 */
	if (uap->pgrp == 0 &&
	    ((uap->pid == 0) || (uap->pid == u.u_procp->p_pid))) {
		(void) setsid(SESS_NEW);
	} else {
		csetpgid(1);
	}
}

setpgid()
{
	csetpgid(0);
}

/*
 * setpgid() - posix version of setpgrp()
 *
 * Either create a new pgrp or join an existing one within the session.
 *
 * Permission checks: (done in this order)
 *	range check pgrp (EINVAL)
 *	range check pid & pid must exist (ESRCH)
 *	pid must be calling process or child thereof (ESRCH)
 *	pid must not be a session leader (EPERM)
 *	if pid a child of the calling process, pid can't have execed (EACCES)
 *	pid must be same session as calling process (EPERM)
 *	if joining a pgrp, pgrp must be in the same session as pid (EPERM)
 */
csetpgid(setpgrp_flag)
{
	register struct a {
		int	pid;
		int	pgrp;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;	/* pid's proc struct */
	struct sess *sp;		/* pid's session */
	register struct proc *tmp;
	register pid_t pid;		/* fast version of uap->pid */
	register pid_t pgrp;		/* fast version of uap->pgrp */

	if (uap->pgrp < 0 || uap->pgrp >= MAXPID) {
		u.u_error = EINVAL;
		return;
	}
	if (uap->pid < 0 || uap->pid >= MAXPID) {
		u.u_error = ESRCH;
		return;
	}
	p = (uap->pid && uap->pid != u.u_procp->p_pid) ?
	    pfind(uap->pid) : u.u_procp;
	if (!p) {
		u.u_error = ESRCH;
		return;
	}
	sp = p->p_sessp;
	pid = p->p_pid;
	pgrp = uap->pgrp ? uap->pgrp : pid;
	if (setpgrp_flag && p->p_pgrp == pgrp)
		return;
	if (p != u.u_procp && p->p_pptr != u.u_procp) {
		u.u_error = ESRCH;
		return;
	}
	if (pid == sp->s_sid) {
		u.u_error = EPERM;
		return;
	}
	if (p->p_pptr == u.u_procp && (p->p_flag & SEXECED)) {
		u.u_error = EACCES;
		return;
	}
	if (sp != u.u_procp->p_sessp) {
		u.u_error = EPERM;
		return;
	}
	/*
	 * XXX - clean up for 5.0.
	 *
	 * POSIX does not allow setpgid(B, A) if A is not an existing
	 * pgrp in this session.  We need to allow it until the shells
	 * are rewritten for the following reason: if the first process
	 * of a pipeline exits quickly, the shell may reap this first
	 * process, A, before forking the second process, B.  Then
	 * setpgid(B, A) will fail because A, and more importantly
	 * the pgrp A, is gone.  The fix is that the shell should block
	 * SIGCHLD while forking off the pipeline but ksh does not (csh
	 * may also be a problem).
	 *
	 * Another reason: You may do a pipline A | B and the shell may
	 * try to set up B with A as a pgrp before setting up A with
	 * A as a pgrp.  Binary compat is a mondo drag.
	 *
	 * We loosen the POSIX checks to allow setpgid(B, A) when we
	 * arrived here via setpgrp(2) and the process in question is
	 * creating its own pgrp. This is a heuristic but extremely
	 * likely to succeed for the case discribed above.
	 * As a further sanity check we don't allow a process to take
	 * the pgrp that matches an existing process in another session.
	 *
	 * Tty pgrps do not come into question since they are no longer
	 * allowed to have stale values.
	 *
	 * See also TIOCSPGRP, TIOCSETPGRP in str_io.c.
	 */
	if (tmp = pgfind(pgrp)) {
		if (tmp->p_sessp != sp) {
			u.u_error = EPERM;
			return;
		}
	} else if (pid != pgrp) {
		if (setpgrp_flag) {
			/*
			 * XXX - heuristic
			 */
			if ((tmp = pfind(pgrp)) && tmp->p_sessp != sp) {
				u.u_error = EPERM;
				return;
			}
		} else {
			u.u_error = EPERM;
			return;
		}
	}
	/*
	 * if we're done, don't bother with the orphan checks
	 * N.B. This call is here, not at the top, since we want
	 * the error returns to be valid.
	 */
	if (p->p_pgrp == pgrp)
		return;

	/*
	 * orphan checks
	 *
	 * if there are other members of the old pgrp, check that pgrp
	 * if in sess0 then we are orphaned by definition
	 * else
	 *   if creating pgrp
	 *     my orphan status depends on my parent
	 *   else (joining a pgrp)
	 *     if I don't have an ok parent
	 *	 my status is the group's status
	 *     else
	 *	 I am not an orphan
	 *	 if the group is orphaned, unorphan them
	 */
	pgexit(p);
	if (pgfind(p->p_pgrp))
		(void) orphan_chk(p->p_pgrp);
	if (sp->s_sid == SESS_SYS)
		p->p_flag |= SORPHAN;
	else {
		int ok_pp;

		ok_pp = p->p_pptr->p_sessp == sp && p->p_pptr->p_stat != SZOMB;
		tmp = pgfind(pgrp);
		if (!tmp) {
			if (ok_pp)
			    p->p_flag &= ~SORPHAN;
			else
			    p->p_flag |= SORPHAN;
		} else {
			if (!ok_pp) {
				if (tmp->p_flag & SORPHAN)
					p->p_flag |= SORPHAN;
				else
					p->p_flag &= ~SORPHAN;
			} else {
				p->p_flag &= ~SORPHAN;
				if (tmp->p_flag & SORPHAN) {
					for ( ; tmp; tmp = pgnext(tmp))
						tmp->p_flag &= ~SORPHAN;
				}
			}
		}
	}
	pgenter(p, pgrp);
	p->p_pgrp = pgrp;
}

/*
 *
 * for all (active) p in process group pgrp
 * do	if (active) parent is in same session but different pgrp, return 0
 * done
 * orphan process group pgrp
 *
 * N.B. Counts on
 *	1) If there is a process leaving the pgrp that would orphan it
 *	   it has already been removed.
 *	2) If there is a parent process that is leaving the session or exiting
 *	   it has already reset its sessp or is in a zombie state.
 */
orphan_chk(pgrp)
	register pgrp;
{
	register struct proc *p;
	register int stopped;

	if ((p = pgfind(pgrp)) == NULL)
		return (0);
	if (p->p_flag & SORPHAN)	/* pgrp orphaned already */
		return (0);
	for ( ; p; p = pgnext(p)) {
		register struct proc *pp;

		if ((p->p_stat == SZOMB) || (pp = p->p_pptr)->p_stat == SZOMB)
		    continue;
		if (pp->p_pgrp != pgrp &&
		    pp->p_sessp == p->p_sessp &&
		    pp->p_stat != SZOMB)
			return (0);
	}
	stopped = 0;
	for (p = pgfind(pgrp); p; p = pgnext(p)) {
		p->p_flag |= SORPHAN;
		if (p->p_stat == SSTOP && (p->p_flag & STRC) == 0)
			stopped++;
	}
	return (stopped);
}

setreuid()
{
	struct a {
		int	ruid;
		int	euid;
	} *uap;
	register int ruid, euid;

	uap = (struct a *)u.u_ap;
	ruid = uap->ruid;
	if (ruid != -1) {
		if (!shortval(ruid))
			return;
		if (u.u_ruid != ruid && u.u_uid != ruid && !suser())
			return;
	}
	euid = uap->euid;
	if (euid != -1) {
		if (!shortval(euid))
			return;
		if (u.u_ruid != euid && u.u_uid != euid &&
		    u.u_procp->p_suid != euid && !suser())
			return;
	}
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
	if (euid != -1)
		u.u_uid = euid;
	if (ruid != -1) {
		u.u_procp->p_uid = ruid;
		u.u_ruid = ruid;
	}
	if (ruid != -1 || u.u_uid != u.u_ruid)
		u.u_procp->p_suid = u.u_uid;
}

setregid()
{
	register struct a {
		int	rgid;
		int	egid;
	} *uap;
	register int rgid, egid;

	uap = (struct a *)u.u_ap;
	rgid = uap->rgid;
	if (rgid != -1) {
		if (!shortval(rgid))
			return;
		if (u.u_rgid != rgid && u.u_gid != rgid && !suser())
			return;
	}
	egid = uap->egid;
	if (egid != -1) {
		if (!shortval(egid))
			return;
		if (u.u_rgid != egid && u.u_gid != egid &&
		    u.u_procp->p_sgid != egid && !suser())
			return;
	}
	/*
	 * Everything's okay, do it.
	 */
	u.u_cred = crcopy(u.u_cred);
	if (egid != -1)
		u.u_gid = egid;
	if (rgid != -1) {
		if (u.u_rgid != rgid) {
			u.u_rgid = rgid;
		}
	}
	if (rgid != -1 || u.u_gid != u.u_rgid)
		u.u_procp->p_sgid = u.u_gid;
}

setgroups()
{
	register struct	a {
		u_int	gidsetsize;
		int	*gidset;
	} *uap = (struct a *)u.u_ap;
	register int *gp;
	struct ucred *newcr, *tmpcr;

	if (!suser())
		return;
	if (uap->gidsetsize > sizeof (u.u_groups) / sizeof (u.u_groups[0])) {
		u.u_error = EINVAL;
		return;
	}
	newcr = crdup(u.u_cred);
	u.u_error = copyin((caddr_t)uap->gidset, (caddr_t)newcr->cr_groups,
	    uap->gidsetsize * sizeof (newcr->cr_groups[0]));
	if (u.u_error) {
		crfree(newcr);
		return;
	}
	tmpcr = u.u_cred;
	u.u_cred = newcr;
	crfree(tmpcr);
	for (gp = &u.u_groups[uap->gidsetsize]; gp < &u.u_groups[NGROUPS]; gp++)
		*gp = NOGROUP;
}

/*
 * Group utility functions.
 */

#ifdef	COMPAT
/*
 * Delete gid from the group set.
 */
leavegroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			goto found;
	return;
found:
	for (; gp < &u.u_groups[NGROUPS-1]; gp++)
		*gp = *(gp+1);
	*gp = NOGROUP;
}

/*
 * Add gid to the group set.
 */
entergroup(gid)
	int gid;
{
	register int *gp;

	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp == gid)
			return (0);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS]; gp++)
		if (*gp < 0) {
			*gp = gid;
			return (0);
		}
	return (-1);
}
#endif	COMPAT

/*
 * Check if gid is a member of the group set.
 */
groupmember(gid)
	int gid;
{
	register int *gp;

	if (u.u_gid == gid)
		return (1);
	for (gp = u.u_groups; gp < &u.u_groups[NGROUPS] && *gp != NOGROUP; gp++)
		if (*gp == gid)
			return (1);
	return (0);
}

/*
 * Test if the current user is the super user.
 */
suser()
{

	if (u.u_uid == 0) {
		u.u_acflag |= ASU;
		return (1);
	}
	u.u_error = EPERM;
	return (0);
}

/*
 * Test to see if a user-supplied uid or gid is valid.
 */
shortval(val)
	int val;
{
	if (((u_int)val & ~(u_int)((u_short)~0)) != 0) {
		u.u_error = EINVAL;
		return (0);
	}
	return (1);
}

/*
 * Routines to allocate and free credentials structures
 */
int cractive = 0;		/* number of active ucred's */
int crallocsz = 0x10;		/* number of ucred's to allocate at a time */
struct	ucred *crfreelist;	/* ucred free list */

/*
 * Allocate a zeroed cred structure and crhold it.
 */
struct	ucred *
crget()
{
	register struct ucred *cr;

	if (servicing_interrupt() && panicstr == NULL)
		panic("crget");
	cr = (struct ucred *)new_kmem_fast_alloc((caddr_t *)&crfreelist,
	    sizeof (*cr), crallocsz, KMEM_SLEEP);
	bzero((caddr_t)cr, sizeof (*cr));
	crhold(cr);
	cractive++;
	return (cr);
}

/*
 * Free a cred structure.
 * Throws away space when ref count gets to 0.
 */
void
crfree(cr)
	struct ucred *cr;
{

	if ((servicing_interrupt() && panicstr == NULL) ||
	    cr == NULL || cr->cr_ref == 0)
		panic("crfree");
	if (--cr->cr_ref != 0)
		return;
	kmem_fast_free((caddr_t *)&crfreelist, (caddr_t)cr);
	cractive--;
}

/*
 * Copy cred structure to a new one and free the old one.
 */
struct	ucred *
crcopy(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;			/* structure copy */
	crfree(cr);
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Duplicate cred structure to a new held one.
 */
struct	ucred *
crdup(cr)
	struct ucred *cr;
{
	struct ucred *newcr;

	newcr = crget();
	*newcr = *cr;			/* structure copy */
	newcr->cr_ref = 1;
	return (newcr);
}

/*
 * Return the (held) credentials for the current running process.
 */
struct	ucred *
crgetcred()
{

	crhold(u.u_cred);
	return (u.u_cred);
}
