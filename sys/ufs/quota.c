#ifndef lint
static	char sccsid[] = "@(#)quota.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Code pertaining to management of the in-core data structures.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/vfs.h>
#include <sys/vnode.h>
#include <sys/uio.h>
#include <ufs/quota.h>
#include <ufs/inode.h>
#include <ufs/mount.h>
#include <ufs/fs.h>

/*
 * Dquot cache - hash chain headers.
 */
#define	NDQHASH		64		/* smallish power of two */
#define	DQHASH(uid, mp) \
	(((unsigned)(mp) + (unsigned)(uid)) & (NDQHASH-1))

struct	dqhead	{
	struct	dquot	*dqh_forw;	/* MUST be first */
	struct	dquot	*dqh_back;	/* MUST be second */
};

/*
 * Dquot in core hash chain headers
 */
struct	dqhead	dqhead[NDQHASH];

/*
 * Dquot free list.
 */
struct dquot dqfreelist;

#define	dqinsheadfree(DQP) { \
	(DQP)->dq_freef = dqfreelist.dq_freef; \
	(DQP)->dq_freeb = &dqfreelist; \
	dqfreelist.dq_freef->dq_freeb = (DQP); \
	dqfreelist.dq_freef = (DQP); \
}

#define	dqinstailfree(DQP) { \
	(DQP)->dq_freeb = dqfreelist.dq_freeb; \
	(DQP)->dq_freef = &dqfreelist; \
	dqfreelist.dq_freeb->dq_freef = (DQP); \
	dqfreelist.dq_freeb = (DQP); \
}

#define	dqremfree(DQP) { \
	(DQP)->dq_freeb->dq_freef = (DQP)->dq_freef; \
	(DQP)->dq_freef->dq_freeb = (DQP)->dq_freeb; \
}

typedef	struct dquot *DQptr;

/*
 * Initialize quota caches.
 */
void
qtinit()
{
	register struct dqhead *dhp;
	register struct dquot *dqp;

	/*
	 * Initialize the cache between the in-core structures
	 * and the per-file system quota files on disk.
	 */
	for (dhp = &dqhead[0]; dhp < &dqhead[NDQHASH]; dhp++) {
		dhp->dqh_forw = dhp->dqh_back = (DQptr)dhp;
	}
	dqfreelist.dq_freef = dqfreelist.dq_freeb = (DQptr)&dqfreelist;
	for (dqp = dquot; dqp < dquotNDQUOT; dqp++) {
		dqp->dq_forw = dqp->dq_back = dqp;
		dqinsheadfree(dqp);
	}
}

/*
 * Obtain the user's on-disk quota limit for file system specified.
 */
int
getdiskquota(uid, mp, force, dqpp)
	register int uid;
	register struct mount *mp;
	int force;			/* don't do enable checks */
	struct dquot **dqpp;		/* resulting dquot ptr */
{
	register struct dquot *dqp;
	register struct dqhead *dhp;
	register struct inode *qip;
	int error;

	dhp = &dqhead[DQHASH(uid, mp)];
loop:
	/*
	 * Check for quotas enabled.
	 */
	if ((mp->m_qflags & MQ_ENABLED) == 0 && !force)
		return (ESRCH);
	qip = mp->m_qinod;
	if (qip == NULL)
		panic("getdiskquota");
	/*
	 * Check the cache first.
	 */
	for (dqp = dhp->dqh_forw; dqp != (DQptr)dhp; dqp = dqp->dq_forw) {
		if (dqp->dq_uid != uid || dqp->dq_mp != mp)
			continue;
		if (dqp->dq_flags & DQ_LOCKED) {
			dqp->dq_flags |= DQ_WANT;
			(void) sleep((caddr_t)dqp, PINOD+1);
			goto loop;
		}
		dqp->dq_flags |= DQ_LOCKED;
		/*
		 * Cache hit with no references.
		 * Take the structure off the free list.
		 */
		if (dqp->dq_cnt == 0)
			dqremfree(dqp);
		dqp->dq_cnt++;
		*dqpp = dqp;
		return (0);
	}
	/*
	 * Not in cache.
	 * Get dqot at head of free list.
	 */
	if ((dqp = dqfreelist.dq_freef) == &dqfreelist) {
		tablefull("dquot");
		return (EUSERS);
	}
	if (dqp->dq_cnt != 0 || dqp->dq_flags != 0)
		panic("diskquota");
	/*
	 * Take it off the free list, and off the hash chain it was on.
	 * Then put it on the new hash chain.
	 */
	dqremfree(dqp);
	remque(dqp);
	dqp->dq_flags = DQ_LOCKED;
	dqp->dq_cnt = 1;
	dqp->dq_uid = uid;
	dqp->dq_mp = mp;
	insque(dqp, dhp);
	if (dqoff(uid) < qip->i_size) {
		/*
		 * Read quota info off disk.
		 */
		error = rdwri(UIO_READ, qip, (caddr_t)&dqp->dq_dqb,
		    sizeof (struct dqblk), dqoff(uid), UIO_SYSSPACE, (int *)0);
		if (error) {
			/*
			 * I/O error in reading quota file.
			 * Put dquot on a private, unfindable hash list,
			 * put dquot at the head of the free list and
			 * reflect the problem to caller.
			 */
			remque(dqp);
			dqp->dq_cnt = 0;
			dqp->dq_mp = NULL;
			dqp->dq_forw = dqp;
			dqp->dq_back = dqp;
			dqinsheadfree(dqp);
			DQUNLOCK(dqp);
			return (EIO);
		}
	} else {
		bzero((caddr_t)&dqp->dq_dqb, sizeof (struct dqblk));
	}
	*dqpp = dqp;
	return (0);
}

/*
 * Release dquot.
 */
void
dqput(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt == 0 || (dqp->dq_flags & DQ_LOCKED) == 0)
		panic("dqput");
	DQUNLOCK(dqp);
	if (--dqp->dq_cnt == 0) {
		dqp->dq_flags = 0;
		dqinstailfree(dqp);
	}
}

/*
 * Update on disk quota info.
 */
void
dqupdate(dqp)
	register struct dquot *dqp;
{
	register struct inode *qip;

	qip = dqp->dq_mp->m_qinod;
	if (qip == NULL ||
	    (dqp->dq_flags & (DQ_LOCKED|DQ_MOD)) != (DQ_LOCKED|DQ_MOD))
		panic("dqupdate");
	dqp->dq_flags &= ~DQ_MOD;
	(void) rdwri(UIO_WRITE, qip, (caddr_t)&dqp->dq_dqb,
	    sizeof (struct dqblk), dqoff(dqp->dq_uid), UIO_SYSSPACE, (int *)0);
}

/*
 * Invalidate a dquot.
 * Take the dquot off its hash list and put it on a private,
 * unfindable hash list. Also, put it at the head of the free list.
 */
dqinval(dqp)
	register struct dquot *dqp;
{

	if (dqp->dq_cnt || (dqp->dq_flags & (DQ_MOD|DQ_WANT)))
		panic("dqinval");
	dqp->dq_flags = 0;
	remque(dqp);
	dqremfree(dqp);
	dqp->dq_mp = NULL;
	dqp->dq_forw = dqp;
	dqp->dq_back = dqp;
	dqinsheadfree(dqp);
}
