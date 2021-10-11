/*	@(#)recover.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/recover.c	10.31" */

#include <sys/types.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/param.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/session.h>
#include <sys/errno.h>
#include <sys/conf.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/mount.h>
#include <rfs/rfs_misc.h>
#include <rfs/sema.h>
#include <rfs/comm.h>
#include <rfs/message.h>
#include <rfs/nserve.h>
#include <rfs/cirmgr.h>
#include <rfs/recover.h>
#include <sys/debug.h>
#include <rfs/rfsys.h>
#include <rfs/rdebug.h>
#include <rfs/rfs_serve.h>
#include <rfs/rfs_node.h>
#include <rfs/rfs_mnt.h>

#define	RDUMATCH(P, SP)  \
  (((P) != (struct rd_user *) NULL) && ((P)->ru_srmntindx == (SP)->s_mntindx))

rcvd_t	rd_recover;		/* rd for recovery */
struct	rd_user *rdu_frlist;	/* free rd_user structures */
int	rdu_flck;		/* rdu-freelist lock */
struct	proc *rec_proc;		/* sleep address for recovery */
int	rec_flag;		/* set to KILL when it's time to exit */

extern	int nsrmount;
extern	int	bootstate;
extern	struct	proc *rfsdp;

void check_mount();
void clean_SRDs();
void check_srmount();
void wake_serve();
void clean_sndd();
void dec_srmcnt();
void checkq();

/*
 *	Initialize recovery (called at startup time).
 */

void
recover_init()
{
	register int	i;

	/* Initialize list of free rd_user structures. */
	for (i = 0; i < nrduser - 1; i++) {
		rd_user[i].ru_stat = RU_FREE;
		rd_user[i].ru_next = &rd_user[i+1];
	}
	if (nrduser >= 1) {
		rd_user[nrduser - 1].ru_next = (struct rd_user *) NULL;
		rdu_frlist = rd_user;
	} else {
		rdu_frlist = NULL;
	}
	initlock(&rdu_flck,1);
}

/*
 *	Non-sleeping part of recovery:  mark the resources
 *	that need to be cleaned up, and awaken the recover
 *	daemon to clean them.
 *
 *	This routine is called by the rfdaemon when a circuit
 *	gets disconnected and when a resource is fumounted
 *	(server side of fumount). THIS ROUTINE MUST NOT SLEEP.
 *	It must always be runnable to wake up procs (e.g., servers)
 *	sleeping in resources that have been disconnected.  Otherwise
 *	these procs and the recover daemon can deadlock.
 */
rec_cleanup(bad_q)
queue_t	*bad_q;		/* stream that has gone away */
{
	register struct srmnt *sp;
	register int i;
	sysid_t bad_sysid;

	DUPRINT2(DB_RECOVER, "cleanup initiated for queue %x\n",bad_q);

	clean_sndd(bad_q);
	clean_SRDs(bad_q);

	/* Wakeup procs sleeping on stream head - e.g., sndmsg canput failed. */
	wakeup((caddr_t) bad_q->q_ptr);

	bad_sysid = ((struct gdp *) bad_q->q_ptr)->sysid;
	DUPRINT1(DB_RECOVER, "recovery: check srmount table\n");

	/* Mark bad srmount entries. */
	for (sp = srmount, i = 0; sp < &srmount[nsrmount]; sp++, i++) {
		if ((sp->sr_flags & MINUSE) && (sp->sr_sysid == bad_sysid)) {
			DUPRINT2 (DB_RECOVER,
				"\t link down for srmount entry %d \n", i);
			if (!(sp->sr_flags & MINTER)) { /* fumount started */
				wake_serve((index_t) i);
			}
			sp->sr_flags |= (MINTER | MLINKDOWN);
		}
	}
	rec_flag |= DISCONN;
	wakeup ((caddr_t) &rec_proc);
	return;
}

/*
 *	Recovery daemon, awakened by cleanup to clean up after
 *	fumount or disconnect.  This part of recovery calls
 *	routines that can sleep.
 */

void
recovery()
{
	int i;
	struct gdp *gdpp;
	struct proc *p = u.u_procp;
	extern struct ucred *crcopy();

	rec_proc = u.u_procp;

	/* If got unexpected signal, try to exit gracefully */
	if (setjmp(&u.u_qsave)) {
		DUPRINT1(DB_RECOVER,"recovery: unexpected signal, exiting\n");
		rec_proc = NULL;
		bootstate |= DU_RECOVER_DOWN;
		wakeup((caddr_t) &bootstate);
		exit(0);
	}
	netmemfree();
	bcopy ("rfs:recovery", (caddr_t) u.u_comm, (u_int) 15);
	rec_flag = 0;

	/*
	 * Disassociate this process from terminal.
	 * XXX - I really want to do a setsid() 
	 * but there are permission checks that could cause it to fail.
	 */
	pgexit(p);
	pgenter(p, p->p_pgrp = 0);
	i = splimp();
	SESS_EXIT(p);
	SESS_ORPH(p);
	SESS_ENTER(p, &sess0);
	(void) splx(i);

	/* ignore all signals */
	for (i=0; i<NSIG; i++) {
		u.u_signal[i] = SIG_IGN;
		u.u_sigmask[i] = ~0;
	}
	p->p_sig = 0;
	p->p_sigmask = ~0;
	p->p_sigignore = ~0;
	p->p_sigcatch = 0;

	/* Don't share credentials with parent */
	u.u_cred = crcopy(u.u_cred);

	for (;;) {
	   while (rec_flag) {
		DUPRINT1(DB_RECOVER, "recovery daemon awakened \n");
		if (rec_flag & DISCONN) {
			rec_flag &= ~DISCONN;
			for (gdpp = gdp; gdpp < &gdp[maxgdp]; gdpp++)
				if(gdpp->flag & GDPRECOVER)  {
					check_mount(gdpp->queue);
					check_srmount();
					put_circuit(gdpp->queue);
				  }
		} else if (rec_flag & FUMOUNT) {
			rec_flag &= ~FUMOUNT;
			check_srmount();
		} else if (rec_flag & RFSKILL) { /* RFS stop */
			DUPRINT1(DB_RECOVER,"recovery daemon exits \n");
			rec_proc = NULL;
			bootstate |= DU_RECOVER_DOWN;
			wakeup((caddr_t) &bootstate);
			exit(0);
		}
	   }
	   (void) sleep ((caddr_t) &rec_proc, PREMOTE);
	}
}

/*
 *	Go through mount table looking for remote mounts over bad stream.
 *	Send message to user-level daemon for every mount with bad link.
 *	(Kernel recovery works without this routine.)
 */

static void
check_mount(bad_q)
queue_t	*bad_q;
{
	extern void user_msg();
	register struct rfsmnt *rmp = (struct rfsmnt *)NULL;

	DUPRINT1(DB_RECOVER, "recovery: check mount table\n");

	for (rmp = rfsmount; rmp < &rfsmount[nrfsmount]; rmp++) {
		if (rmp->rm_flags == MFREE)
			continue;
		/* remote mounts */
		if ((rmp->rm_flags & MDOTDOT) &&
		    (vtorfs(rmp->rm_rootvp)->rfs_sdp->sd_queue == bad_q)) {
			DUPRINT2(DB_RECOVER," link down to %s\n", rmp->rm_name);
			/* invalidate cache for this mount device */
			if (rmp->rm_flags & MCACHE)
				;
	/* 	rmntinval((struct sndd *)mp->m_mount->i_fsptr); */
			user_msg((long) RF_DISCONN, rmp->rm_name, NMSZ);
		}
		/* (Check for lbin mounts if lbin comes back.) */
	}
}

/*
 *	Cleanup SPECIFIC RDs:
 *	Wakeup procs waiting for reply over stream that went bad.
 */

void
clean_SRDs(bad_q)
queue_t	*bad_q;		/* stream that has gone away */
{
	register rcvd_t rd;
	index_t srm_index;

	for (rd = rcvd; rd < &rcvd[nrcvd]; rd++)  {
	    if (ACTIVE_SRD(rd) && (bad_q==(struct queue *)rd->rd_user_list)) {
		DUPRINT1(DB_RECOVER, "recovery: waking SPECIFIC RD \n");
		srm_index = rd->rd_sd->sd_mntindx;
		checkq(rd, srm_index);
		rd->rd_stat |= RDLINKDOWN;
		wakeup((caddr_t) &rd->rd_qslp);
	    }
	}
}

/*
 *	Check server mount table.  Wake server procs sleeping
 *	in resource that went bad.  Pretend client gave up
 *	references to resource.
 */

static void
check_srmount()
{
	register struct srmnt *sp;
	register int i;
	register rcvd_t rd;
	queue_t *bad_q, *sysid_to_queue();
	struct vnode *vp;

	DUPRINT1(DB_RECOVER, "recovery: check srmount table\n");
	for (sp = srmount, i = 0; sp < &srmount[nsrmount]; sp++, i++) {
		if (sp->sr_flags & (MLINKDOWN | MFUMOUNT)) {
			DUPRINT2 (DB_RECOVER,
			"\t link down for srmount entry %d \n", i);
			/* Wait for servers to wake, leave resource.*/
			while (sp->sr_slpcnt) {
				(void) sleep ((caddr_t) &srmount[i], PZERO);
			}
			bad_q = sysid_to_queue(sp->sr_sysid);
			ASSERT(bad_q);
			/* Bump count on vnode so it won't go away -
			 * VN_RELE in dec_srmcnt when doing unmount */
			vp = sp->sr_rootvnode;
			VN_HOLD(vp);
			/* Now clean up GENERAL RDs */
			for (rd = rcvd; rd < &rcvd[nrcvd]; rd++)  {
				if (ACTIVE_GRD (rd))
					clean_GEN_rd (rd, bad_q, i);
			}
		}
	}

}

/*
 *	On the server side, signal any server process sleeping
 *	in the resource with this srmount index. Count the servers
 *	we signal - we must wait for them to finish.
 */

void
wake_serve(srmindex)
index_t srmindex;
{
	register sndd_t sd;

	for (sd = sndd; sd < &sndd[nsndd]; sd++)
		if ((sd->sd_stat & SDSERVE) && (sd->sd_stat & SDUSED) &&
		    (sd->sd_mntindx == srmindex) &&
		    (sd->sd_srvproc != (struct proc *)NULL)) {
			DUPRINT1(DB_RECOVER, "wake_serve: waking server \n");
			sd->sd_stat |= SDLINKDOWN;
			psignal (sd->sd_srvproc, SIGTERM);
			++srmount[srmindex].sr_slpcnt;
		}
}

/*
 *	Link is down.  Mark send-descriptors that use it.
 */

static void
clean_sndd (bad_q)
queue_t	*bad_q;
{
	register sndd_t sd;

	DUPRINT1(DB_RECOVER, "recovery: marking send-descriptors\n");
	for (sd = sndd; sd < &sndd[nsndd]; sd++)
		if ((sd->sd_stat & SDUSED) && (sd->sd_queue == bad_q)) {
			DUPRINT2(DB_RECOVER, "   sd %x link is down\n",sd);
			sd->sd_stat |= SDLINKDOWN;
		}
}

/*
 *	Clean up GENERAL RD.
 *
 *	Traverse rd_user list of this RD.  For each rd_user from
 *	this srmount index, pretend that client gave up all refs
 *	to this RD.
 *
 *	(Need bad_q to get sysid for cleanlocks.)
 */

void
clean_GEN_rd (rd, bad_q, srm_index)
rcvd_t	rd;
queue_t	*bad_q;
index_t srm_index;
{
	register int i;
	struct	vnode *vp;
	struct rd_user *rduptr, *rduptr1;
	struct srmnt *smp;
	struct ucred cred;
	struct serv_proc sp;

	DUPRINT4 (DB_RECOVER, "clean_GEN RD: rd %x, refcnt %d, vnode %x\n",
				rd, rd->rd_refcnt, rd->rd_vnode);

	bzero((caddr_t)&cred, (u_int) sizeof(cred));
	crhold(&cred);
	rduptr = rd->rd_user_list;
	ASSERT (rduptr != NULL);

	checkq (rd, srm_index); /* get rid of old messages */
	vp = rd->rd_vnode;

	/* Traverse rd_user list. */
	while (rduptr != (struct rd_user *)NULL) {
		rduptr1 = rduptr->ru_next;

		if ((rduptr->ru_srmntindx != srm_index)) {
			/* don't clean this user - get next one */
			rduptr = rduptr1;
			continue;
		}
		/* index into srmount table - for del_rduser */
		smp = &srmount[srm_index];

		/* Mimic what a server would do to get rid of reference. */
		/* Clean up FILE uses of this resource. */
		if ((long) rduptr->ru_fcount > 0) {
			/* Release any locks held by downed client machine */
			rsl_lockrelease(rd, 0, (long) srm_index);

			DUPRINT1 (DB_RECOVER, "  FS_CLOSEI \n");
			if((rduptr->ru_frcnt == 0) && (rduptr->ru_fwcnt == 0)) {
				VOP_CLOSE(vp, FREAD|FWRITE, 1, &cred);
			}
			else {
				for (i = rduptr->ru_frcnt; i > 0; --i) {
					VOP_CLOSE(vp, FREAD, 1, &cred);
					rduptr->ru_frcnt--;
				}
				for (i = rduptr->ru_fwcnt; i > 0; --i) {
					VOP_CLOSE(vp, FWRITE, 1, &cred);
					rduptr->ru_fwcnt--;
				}
			}
			rduptr->ru_fcount = 0;
		}
		/* Clean up VNODE uses of this resource. */
		for (i = rduptr->ru_icount; i > 0; --i) {
			DUPRINT1(DB_RECOVER, "  clean vnode\n");
			dec_srmcnt(smp, bad_q);
			/* Yuk!! -- Cons up what associated server proc
 			  would look like */
			bzero((caddr_t) &sp, sizeof(sp));
			sp.s_mntindx = srm_index;
			sp.s_op = DUIPUT;
			sp.s_fmode = NULL;
			del_rcvd(rd, &sp);
			VN_RELE(vp);
		}
		rduptr = rduptr1;
	}	 /* end while */
}


/*
 *	Decrement the reference count in the srmount table.
 *	If it goes to zero, do the unmount.
 */

static void
dec_srmcnt(smp, bad_q)
struct	srmnt *smp;
queue_t *bad_q;
{
	struct vnode *vp;
	struct gdp *gdpp;

	if (smp->sr_refcnt > 1) { /* srumount wants refcnt of 1 */
		--smp->sr_refcnt;
		return;
	}
	/*
	 * Giving up last ref for this srmount entry - free it.
	 * The vnode we're working on in clean_GEN is NOT necessarily
	 * the root of the resource we're unmounting.
	 */
	vp = smp->sr_rootvnode;
	DUPRINT2 (DB_RECOVER, "recovery freed srmount entry %x \n",smp);
	smp->sr_flags &= ~MINTER;
	(void) srumount(smp, vp);
	/* One extra VN_RELE for bump in check_srmount and fumount. */
	VN_RELE(vp);

	/* Client usually cleans up circuit, but client is gone. */
	gdpp = GDP (bad_q);
	--gdpp->mntcnt;
}

/*
 *	Giving away this rd as a gift.  Return an rd_user structure
 *	to keep track of it.  If we've already given this rd to the
 *	same client (for the same srmount index), use the same structure.
 *	Otherwise point rd at new structure and return it.
 */

struct	rd_user *
alloc_rduser(rd, sp)
register struct rcvd *rd;
struct	serv_proc *sp;
{
	register struct rd_user *rduptr;

	rduptr = rd->rd_user_list;
	for ( ; rduptr != NULL; rduptr = rduptr->ru_next)
		if (RDUMATCH(rduptr, sp)) /* already exists - return old one */
			return (rduptr);

	/* need a new one */
	if ((rduptr = rdu_frlist) == NULL) {
		printf ("alloc_rduser: out of rd_user space\n");
		return (NULL);
	}
 	/* s_mntindx tells which srmount entry request is for */
	rduptr->ru_srmntindx = sp->s_mntindx;
	rduptr->ru_icount = 0;
	rduptr->ru_fcount = 0;
	rduptr->ru_frcnt = 0;
	rduptr->ru_fwcnt = 0;
	rdu_frlist = rdu_frlist->ru_next;
	rduptr->ru_stat = RU_USED;
	rduptr->ru_lk = NULL;
	rduptr->ru_lkflags = 0;

	/* insert at head of list */
	rduptr->ru_next = rd->rd_user_list;
	rd->rd_user_list = rduptr;
	return (rduptr);
}

/*
 *	Free an rd_user structure.
 */

static void
free_rduser(rduptr)
struct	rd_user *rduptr;
{
	DUPRINT1 (DB_RDUSER, "  free_rduser \n");
	ASSERT(rduptr->ru_lk == NULL);
	rduptr->ru_stat = RU_FREE;
	rduptr->ru_next = rdu_frlist;
	rdu_frlist = rduptr;
	return;
}

/*
 *	Giving away an RD.  Keep track of who's getting it and why.
 *	This routine must be called every time a remote machine
 *	gets a reference to a resource.
 */

struct	rd_user *
cr_rduser(rd, sp)
rcvd_t	rd;
struct	serv_proc *sp;
{
	register struct rd_user *rduptr;
	struct rd_user *alloc_rduser();
	register struct vnode *vp = rd->rd_vnode;

	if ((rduptr = alloc_rduser(rd, sp)) == NULL)
		return (NULL);

	rduptr->ru_queue = sp->s_gdpp->queue;
	DUPRINT5(DB_RDUSER, " cr_rduser: rd %x, qp %x, srmntindx %d, rduser %x\n",
			rd, rduptr->ru_queue, sp->s_mntindx, rduptr);
	if (sp->s_op == DUOPEN) {
		rduptr->ru_fcount++;
		/* for fifo case, bump reader/writer counts */
		if (vp->v_type == VFIFO) {
			if (sp->s_fmode & FREAD)
				rduptr->ru_frcnt++;
			if (sp->s_fmode & FWRITE)
				rduptr->ru_fwcnt++;
		}
	} else if (sp->s_op == DUCREAT) {
		rduptr->ru_fcount++;
		if (vp->v_type == VFIFO)
			rduptr->ru_fwcnt++;
	}
	rduptr->ru_icount++;
	return (rduptr);

}

/*
 *	A client is giving up a reference to this RD; decrement
 *	count in rd_user struct we made in cr_rduser.  If count
 *	goes to zero, free the struct.
 */

void
del_rduser(rd, sp)
rcvd_t	rd;
struct	serv_proc *sp;
{
	register struct rd_user *rduptr;
	register struct rd_user *rdut;
	void free_rduser();

	DUPRINT4 (DB_RDUSER, " del_rduser: rd %x, srmntindx %d, mode %x, ",
			rd, sp->s_mntindx, sp->s_fmode);


	rduptr = NULL;
	ASSERT (rd->rd_user_list != NULL);  /* no users to delete */

	if (RDUMATCH(rd->rd_user_list, sp)) {	/* matched first */
		rduptr = rd->rd_user_list;
		goto done;
	}
	for (rdut = rd->rd_user_list;	/* not first - traverse list */
		 ((rdut != NULL) && !RDUMATCH(rdut->ru_next, sp));
			rdut = rdut->ru_next);

	ASSERT (rdut != NULL); /* try to delete user that's not there */
	rduptr = rdut->ru_next;

done:
	DUPRINT2(DB_RDUSER, " rduser %x\n", rduptr);
	if (sp->s_op == DUCLOSE) {
		rduptr->ru_fcount--;
		if (rd->rd_vnode->v_type == VFIFO) {
			if (sp->s_fmode & FREAD)
				rduptr->ru_frcnt--;
			if (sp->s_fmode & FWRITE)
				rduptr->ru_fwcnt--;
		}
		return;
	}

	if (--rduptr->ru_icount)
		return;

	/* last reference - get rid of rd_user struct */
	if (rd->rd_user_list == rduptr)
		rd->rd_user_list = rduptr->ru_next;
	else
		rdut->ru_next = rduptr->ru_next;
	free_rduser(rduptr);
	return;
}

/*
 *  An srmount entry went bad (disconnect or fumount),
 *  and recovery is cleaning it up.  Throw away any old
 *  messages that are on this rd for the bad entry.
 */

void
checkq(rd, srmindex)
rcvd_t	rd;
index_t srmindex;
{
	register int i, qcnt, s;
	mblk_t  *bp;
	mblk_t  *deque();
	index_t msg_srmindex;
	struct message *message;
	struct request *request;

	DUPRINT1 (DB_RECOVER, "recovery: checkq \n");
	s = splrf(); /* arrmsg() calls enque() at stream priority */

	qcnt = rd->rd_qcnt;
	for (i = 0; i < qcnt; i++) {
		bp = deque (&rd->rd_rcvdq);
		ASSERT (bp != NULL);
		message = (struct message *) bp->b_rptr;
		request = (struct request *) PTOMSG (message);
		msg_srmindex = request->rq_mntindx;
		if (msg_srmindex == srmindex) {
			DUPRINT2 (DB_RECOVER, "   toss bp %x\n", bp);
			/* don't service this message - toss it */
			freemsg (bp);
			rd->rd_qcnt--;
			if (rcvdemp(rd)) {
				rm_msgs_list (rd);
				break;
			}
		} else {
			/* message OK - put it back */
			enque (&rd->rd_rcvdq, bp);
		}
	}
	(void) splx(s);
	return;
}


