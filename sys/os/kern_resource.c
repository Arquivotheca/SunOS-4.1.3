/*	@(#)kern_resource.c 1.1 92/07/30 SMI; from UCB 4.22 83/05/27	*/

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/vm.h>
#include <sys/kernel.h>

/*
 * Resource controls and accounting.
 */

getpriority()
{
	register struct a {
		int	which;
		int	who;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_r.r_val1 = NZERO+20;
	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u.u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			return;
		u.u_r.r_val1 = p->p_nice;
		u.u_error = 0;
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_pgrp == uap->who &&
			    p->p_nice < u.u_r.r_val1) {
				u.u_r.r_val1 = p->p_nice;
				u.u_error = 0;
			}
		}
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt) {
			if (p->p_suid == uap->who &&
			    p->p_nice < u.u_r.r_val1) {
				u.u_r.r_val1 = p->p_nice;
				u.u_error = 0;
			}
		}
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
	u.u_r.r_val1 -= NZERO;
}

setpriority()
{
	register struct a {
		int	which;
		int	who;
		int	prio;
	} *uap = (struct a *)u.u_ap;
	register struct proc *p;

	u.u_error = ESRCH;
	switch (uap->which) {

	case PRIO_PROCESS:
		if (uap->who == 0)
			p = u.u_procp;
		else
			p = pfind(uap->who);
		if (p == 0)
			return;
		donice(p, uap->prio);
		break;

	case PRIO_PGRP:
		if (uap->who == 0)
			uap->who = u.u_procp->p_pgrp;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_pgrp == uap->who) {
				donice(p, uap->prio);
			}
		break;

	case PRIO_USER:
		if (uap->who == 0)
			uap->who = u.u_uid;
		for (p = allproc; p != NULL; p = p->p_nxt)
			if (p->p_suid == uap->who) {
				donice(p, uap->prio);
			}
		break;

	default:
		u.u_error = EINVAL;
		break;
	}
}

donice(p, n)
	register struct proc *p;
	register int n;
{

	if (u.u_uid && u.u_ruid &&
	    u.u_uid != p->p_suid && u.u_ruid != p->p_suid) {
		u.u_error = EPERM;
		return;
	}
	n += NZERO;
	if (n >= 2*NZERO)
		n = 2*NZERO - 1;
	if (n < 0)
		n = 0;
	if (n < p->p_nice && !suser()) {
		u.u_error = EACCES;
		return;
	}
	p->p_nice = n;
	(void) setpri(p);
	if (u.u_error == ESRCH)
		u.u_error = 0;
}

setrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *lim;
	} *uap = (struct a *)u.u_ap;
	struct rlimit alim;
	register struct rlimit *alimp;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	alimp = &u.u_rlimit[uap->which];
	u.u_error = copyin((caddr_t)uap->lim, (caddr_t)&alim,
		sizeof (struct rlimit));
	if (u.u_error)
		return;
	if (alim.rlim_cur > alim.rlim_max || alim.rlim_cur < 0) {
		u.u_error = EINVAL;
		return;
	}
	if (alim.rlim_cur > alimp->rlim_max || alim.rlim_max > alimp->rlim_max)
		if (!suser())
			return;
	switch (uap->which) {

	case RLIMIT_DATA:
		if (alim.rlim_cur > MAXDSIZ)
			alim.rlim_cur = MAXDSIZ;
		if (alim.rlim_max > MAXDSIZ)
			alim.rlim_max = MAXDSIZ;
		break;

	case RLIMIT_STACK:
		if (alim.rlim_cur > MAXSSIZ)
			alim.rlim_cur = MAXSSIZ;
		if (alim.rlim_max > MAXSSIZ)
			alim.rlim_max = MAXSSIZ;
		break;

	case RLIMIT_NOFILE:
		if (alim.rlim_cur > NOFILE)
			alim.rlim_cur = NOFILE;
		if (alim.rlim_max > NOFILE)
			alim.rlim_max = NOFILE;
		break;
	}
	*alimp = alim;
	if (uap->which == RLIMIT_RSS)
		u.u_procp->p_maxrss = alim.rlim_cur/NBPG;
}

getrlimit()
{
	register struct a {
		u_int	which;
		struct	rlimit *rlp;
	} *uap = (struct a *)u.u_ap;

	if (uap->which >= RLIM_NLIMITS) {
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)&u.u_rlimit[uap->which], (caddr_t)uap->rlp,
	    sizeof (struct rlimit));
}

getrusage()
{
	register struct a {
		int	who;
		struct	rusage *rusage;
	} *uap = (struct a *)u.u_ap;
	register struct rusage *rup;

	switch (uap->who) {

	case RUSAGE_SELF:
		rup = &u.u_ru;
		break;

	case RUSAGE_CHILDREN:
		rup = &u.u_cru;
		break;

	default:
		u.u_error = EINVAL;
		return;
	}
	u.u_error = copyout((caddr_t)rup, (caddr_t)uap->rusage,
	    sizeof (struct rusage));
}

ruadd(ru, ru2)
	register struct rusage *ru, *ru2;
{
	register long *ip, *ip2;
	register int i;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i > 0; i--)
		*ip++ += *ip2++;
}
