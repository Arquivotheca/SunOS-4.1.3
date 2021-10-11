/*	@(#)rfadmin.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/rfadmin.c	1.22" */
/*
 *	Kernel daemon for remote-file administration.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/signal.h>
#include <sys/file.h>
#include <sys/stream.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/proc.h>
#include <sys/session.h>
#include <sys/wait.h>
#include <sys/kernel.h>
#include <sys/signal.h>
#include <sys/debug.h>
#include <sys/mount.h>
#include <sys/vfs.h>
#include <nettli/tihdr.h>
#include <rfs/sema.h>
#include <rfs/rfs_misc.h>
#include <rfs/nserve.h>
#include <rfs/comm.h>
#include <rfs/message.h>
#include <rfs/adv.h>
#include <rfs/cirmgr.h>
#include <rfs/recover.h>
#include <rfs/rdebug.h>
#include <rfs/rfsys.h>
#include <rfs/rfs_serve.h>
#include <rfs/rfs_mnt.h>

extern	int	msglistcnt;		/* number of msgs in msglist */
extern	int	bootstate;		/* state of RFS -- up, down, or inter */
extern	int	rec_flag;	/* Controls activity of recovery daemon */
struct serv_list s_idle;		/* server processes waiting for work   */
struct serv_list s_active;		/* server processes doing work   */

extern struct proc *rec_proc;           /* pointer to recovery process */

rcvd_t	rd_recover;			/* rd for daemon */
struct proc *rfsdp;			/* pointer to daemon process */
struct vnode *rfs_cdir;			/* server current directory */
int 	msgflag;			/* create server if set */
sema_t rfadmin_sema;
struct bqctrl_st rfmsgq;		/* queue for user-level daemon */
int	rfmsgcnt;			/* how many msgs in user-level q */

void reply(), user_msg(), que_umsg();
extern char *strcpy(), *strncpy();


/*
 *	File sharing daemon, started when file sharing starts.
 */

void
rfdaemon()
{
	int	i,s,size;
	struct sndd reply_port;
	mblk_t	*bp, *deque();
	queue_t	*qp;
	struct request *request;
	struct response *response;
	mblk_t	*resp_bp;
	char usr_msg [ULINESIZ]; /* tmp space for user daemon message */
	struct proc *p = u.u_procp;
	int killmask = sigmask(SIGKILL) | sigmask(SIGTERM);
	struct	advertise *ap;
	struct gdp *tmp;
	struct rfsmnt *rmp;
	
	extern int  nservers, msglistcnt;
	extern void cl_fumount();
	extern struct ucred *crcopy();
	extern mblk_t *rfs_getbuf();
	extern unadv();

	rfsdp = u.u_procp;

	/* 
	 * If got unexpected signal, try to shutdown RFS and exit gracefully. 
	 * This is the case when shutdown or kill -TERM 1 is executed.
	 * Try to terminate all the rfs kernel processes and cleanup all
	 * the state, then try to do rfstop() 
	 */
	if (setjmp(&u.u_qsave)) {
		u.u_signal[SIGKILL] = SIG_IGN;
		u.u_signal[SIGTERM] = SIG_IGN;
		p->p_sig = 0;
		p->p_sigmask = ~0;
		p->p_sigignore = ~0;
		p->p_sigcatch = 0;
		DUPRINT1(DB_RECOVER, "rfadmin: unexpected signal, exiting\n");
		msgflag = NULL;
		gsignal(rfsdp->p_pgrp,SIGTERM);
		rfsdp = NULL;
		/*
	         * Simulate a disconect on all existing circuits. This
		 * will do all necessary state and resource cleanup. 
		 */
		for (tmp = gdp; tmp < &gdp[maxgdp]; tmp++) 
			if (tmp->flag !=  GDPFREE) {
				tmp->flag |= GDPDISCONN;
				rec_cleanup(tmp->queue);
				tmp->flag = GDPRECOVER;
			}
		/* 
		 * Wakeup the recovery process and wait for it to do all 
 		 * the cleanup required by the disconnects and then kill
		 * itself.
		 */
		rec_flag |= RFSKILL;
		wakeup ((caddr_t) &rec_proc);
		while (!(bootstate & DU_RECOVER_DOWN))
			(void) sleep((caddr_t) &bootstate, PZERO);
		/* 
		 * Now all the advertised resources should be purged of
	 	 * clients. Unadvertise them.
		 */
		for (ap = advertise; ap < &advertise[nadvertise]; ap++) {
			if (ap->a_flags & A_INUSE)
				(void) unadv(ap);
		}
		/* 
  		 * Now try to purge the rfs mounts from the rest of the
 		 * system by doing unmounts. We hope that all processes
	 	 * using them have already been killed off (e.g., by
		 * shutdown) otherwise an unmount will fail. Note in any 
		 * case that this doesn't fix /etc/mtab.
		 */
		for (rmp = rfsmount; rmp < &rfsmount[nrfsmount]; rmp++) {
			if (rmp->rm_flags & MINUSE)
				dounmount(rmp->rm_vfsp);
		}
		bootstate |= DU_DAEMON_DOWN;
		rfstop();
		exit(0);
	}

	netmemfree();
	rfs_cdir = u.u_cdir;
	bcopy ("rfs:rfdaemon", u.u_comm, 14);

	/* dissociate this process from terminal */
	(void) setsid(SESS_NEW);

	/* ignore all signals, except SIGKILL and SIGTERM */ 
	for (i=0; i<NSIG; i++) {
		u.u_signal[i] = SIG_IGN;
		u.u_sigmask[i] = ~killmask; 
	}

	u.u_signal[SIGKILL] = SIG_DFL;
	u.u_signal[SIGTERM] = SIG_DFL;
	p->p_sig = 0;
	p->p_sigmask = ~killmask;
	p->p_sigignore = ~killmask;
	p->p_sigcatch = 0;

	/* Don't share credentials with parent */
	u.u_cred = crcopy(u.u_cred);

	nzombies = 0;
	nservers = 0;
	s_idle.sl_count = 0;
	s_idle.sl_head = (struct serv_proc *) NULL;

	msgflag = NULL;

	/* XXX -- should cleanup and exit if failure */
	(void) creat_server(minserve);

	bzero((caddr_t) &reply_port, sizeof(reply_port));
	reply_port.sd_refcnt = 1;
	reply_port.sd_stat = SDUSED;
		
	/* Make user-daemon queue null. */
	while ((bp = deque (&rfmsgq)) != NULL) {
		DUPRINT1 (DB_RFSYS, "discarding old user-level message\n");
		freemsg (bp);
	}
	rfmsgq.qc_head = (struct msgb *) NULL;
	rfmsgq.qc_tail = (struct msgb *) NULL;
	rfmsgcnt = 0;
	reply_port.sd_stat = SDUSED;
	reply_port.sd_refcnt = 1;
	reply_port.sd_copycnt = 0;
	bootstate = DU_UP;
	wakeup((caddr_t) &bootstate);	/* end of critical section begun in rfstart */

	for (;;) {
		/* raise priority because vsema is called from gdpserv */
		s = splrf();
		u.u_error = 0;
		(void) sleep((caddr_t) &rd_recover->rd_qslp, PZERO + 1);
	loop:
		bp = NULL;
		(void) dequeue(rd_recover, &bp, &reply_port, &size);
		DUPRINT4 (DB_SERVE,"rfdaemon: awoke, bp %x, msgflag %x, zcnt %d\n",
			bp, msgflag, nzombies);
		if (nzombies > 0)
			clean_proc_table();
		if (bp) {
			(void) splx(s);
			qp = (queue_t *)((struct message *)bp->b_rptr)->m_queue;
			request = (struct request *) 
				  (bp->b_rptr + sizeof (struct message));
			DUPRINT4(DB_RECOVER, "rfadmin: bp %x qp %x op %x\n",
					bp, qp, request->rq_opcode);
			switch (request->rq_opcode ) {
			case REC_FUMOUNT:
				{
				rmp = &rfsmount[request->rq_mntindx];
				cl_fumount((index_t) request->rq_mntindx,
					(index_t) request->rq_srmntindx);
				freemsg(bp);
				reply(&reply_port, REC_FUMOUNT);
				user_msg((long) RF_FUMOUNT, rmp->rm_name, NMSZ);
				break;
				}
			case REC_MSG:
				{
				/* Got a message for user-level daemon.
				 * Enque message and wake up daemon.
 				 */
				DUPRINT1 (DB_RFSYS,
				"rfadmin: got a message for user daemon \n");
				size = request->rq_count;
				/* save message so we can free stream buf */
				(void) strncpy(usr_msg, request->rq_data, size);
				freemsg (bp);
				reply (&reply_port, REC_MSG);
				user_msg((long) RF_GETUMSG, usr_msg, size);
				break;
				}
			case DUSYNCTIME:
				DUPRINT1(DB_RFSYS, "rfadmin: date sync\n");
				GDP(qp)->time = request->rq_synctime - time.tv_sec;
				/*alocbuf and send reply */
				resp_bp = rfs_getbuf(sizeof (struct response) -
							 DATASIZE, BPRI_MED);
				response = (struct response *)
						PTOMSG(resp_bp->b_rptr);
				response->rp_type = RESP_MSG;
				size = sizeof (struct response) - DATASIZE;
		 		response->rp_opcode = DUSYNCTIME;
				response->rp_synctime = time.tv_sec;
				response->rp_errno = 0;
				response->rp_sig = 0;
				freemsg (bp);
				(void) sndmsg (&reply_port, resp_bp, size, 
					(rcvd_t)NULL);
				break;
			case DUCACHEDIS:
				{
				DUPRINT4(DB_CACHE, "rfadmin: got disable cache message, queue %x, mntindx %x, fhandle %x\n",
				qp, request->rq_mntindx, request->rq_fhandle);
/* Waiting for cacheing 
invalidate_cache(qp, request->rq_mntindx, 
request->rq_fhandle); */
				freemsg (bp);
				/* alocbuf and send reply */
				resp_bp = rfs_getbuf(sizeof (struct response) -
							 DATASIZE, BPRI_MED);
				response = (struct response *)
						PTOMSG(resp_bp->b_rptr);
				response->rp_type = RESP_MSG;
				size = sizeof (struct response) - DATASIZE;
		 		response->rp_opcode = DUCACHEDIS;
				response->rp_errno = 0;
				response->rp_sig = 0;
				(void) sndmsg (&reply_port, resp_bp, 
						size, (rcvd_t)NULL);
				/* If link is down server will recover */
				break;
				}
			default:
				u.u_error = EINVAL;
				DUPRINT2 (DB_RFSYS, "rfadmin: unknown op %d\n",
					request->rq_opcode);
				freemsg (bp);
			}
		} else {
			if (!msgflag) {
				(void) splx(s);
				continue;
			}
			if (msgflag & DISCONN) {
				msgflag &= ~DISCONN;
				(void) splx(s);
				for(tmp = gdp; tmp < &gdp[maxgdp]; tmp++) 
					if(tmp->flag & GDPDISCONN) {
						rec_cleanup(tmp->queue);
						tmp->flag = GDPRECOVER;
					}
			}
			else if (msgflag & RFSKILL) {
				DUPRINT1(DB_RECOVER, "rfadmin: KILL case\n");
				msgflag = NULL;
				(void) splx(s);
				gsignal(rfsdp->p_pgrp,SIGKILL);
				rfsdp = NULL;
				bootstate |= DU_DAEMON_DOWN;
                                wakeup((caddr_t) &bootstate);
				exit(0);
			}
			else if (msgflag & MORE_SERVE) {
				msgflag &= ~MORE_SERVE;
				(void) splx(s);
				if (s_idle.sl_count <= 1 && nservers < maxserve)
					if (creat_server(1) == RFS_FAILURE &&
						s_idle.sl_count == 0)
							sendback(&reply_port,
							    ENOMEM,RESP_MSG);
			} 
			else if (msgflag & DEADLOCK) {
				msgflag &= ~DEADLOCK;
				(void) splx(s);
				if (s_idle.sl_count == 0 && 
				    nservers == maxserve)
					sendback(&reply_port,ENOMEM,NACK_MSG);
			}
		}
		s = splrf();
		goto loop;
	}
}


/*
 *	Create num servers.
 */
creat_server(num)
int num;
{
	int i;
	extern struct serv_proc *sp_alloc();
	void do_server();

	DUPRINT2 (DB_SERVE,"create %d server(s) \n", num);
	for (i = 0; i < num; i++) {	/* make minimum server */
		/* Don't create more than maximum allowable number */
                if (nservers >= maxserve)
			goto err;
		kern_proc(do_server, 0);
		nservers++;
	}
	return(RFS_SUCCESS);
err:
	DUPRINT1(DB_SERVE,"cannot create server\n");
	/* post signal to last server so it won't go to sleep */
	if (s_active.sl_head)
		psignal(s_active.sl_head->s_proc, SIGUSR1);
	return(RFS_FAILURE);
}

/*
 * Server process routine started from rfadmin by doing a kernel fork 
 */
void
do_server()
{
	struct serv_proc *sp;

	sp = sp_alloc();
	serve(sp);
	serve_exit(sp);
}

/* 
 * Request that the rfdaemon add servers. 
 */
add_server()
{
	int s;

	s = splrf();
	msgflag |= MORE_SERVE;
	(void) splx(s);
	wakeup((caddr_t) &rd_recover->rd_qslp);
}


/*
 *	All dying servers come through here. A server dies when it is 
 *	awakened from de_queue with nothing on its queue. If the server
 *      process structure is NULL, it means that RFS is coming down, so
 *      some cleanup
 */
serve_exit(sp)
struct serv_proc *sp;
{
	extern int nservers;
	extern rcvd_t rd_recover;

	DUPRINT3(DB_SERVE, "server_exit: nservers %d, zcnt %d\n", 
			nservers - 1, nzombies + 1);
	u.u_rdir = NULL;
	u.u_cdir = rfs_cdir;
	if (sp->s_sdp)
		free_sndd(sp->s_sdp);
	--nservers;
	nzombies++;
	sp_free(sp);
	wakeup((caddr_t) &rd_recover->rd_qslp);
	/* RFS is on the way down */
	if (nservers <= 0) {
		bootstate |= DU_SERVE_DOWN;
		wakeup((caddr_t) &bootstate);
	}
	exit(0);
}


/*
 *	Free zombie servers.
 * 	Loop on wait4, cleaning up zombie server processes.
 */
clean_proc_table()
{
	struct wait4_args args;

	DUPRINT2 (DB_SERVE, "clean_proc_table: zombie count %d\n", nzombies);
	ASSERT(nzombies > 0);
	args.pid = 0;
	args.status = (union wait *) NULL;
	args.options = 0;
	args.rusage = (struct rusage *) NULL;

	while (nzombies) {
		wait4(&args);
		nzombies--;
	}
}

/* 
 * Routines to allocate and free the server process structure *
 */

struct serv_proc *
sp_alloc()
{
	register struct serv_proc *sp;

	for (sp = &serv_proc[0]; sp < &serv_proc[maxserve]; sp++) {
		if (!sp->s_proc)
			break;
	}
	ASSERT((sp < &serv_proc[maxserve]) && !sp->s_proc);
	sp->s_proc = u.u_procp;
	sp->s_pid = u.u_procp->p_pid;
	return(sp);
}

void
sp_free(sp)
struct serv_proc *sp;
{
	bzero((caddr_t) sp, (u_int) sizeof(struct serv_proc));
}



/*
 * Add server proc to the beginning of a list. 
 */
void
add_proc(sp, list)
struct serv_proc *sp;
struct serv_list *list;
{
	DUPRINT4(DB_RFSYS, "add_proc: proc %x, list %x, count %d\n", 
		sp, list, list->sl_count);
	sp->s_rlink = list->sl_head;
	list->sl_head = sp;
	list->sl_count++;
	return;
}


/*
 * Remove server process from list. If it's not on the list, do nothing.
 */
void
del_proc(dsp, list)
register struct serv_proc *dsp;
struct serv_list *list;
{
	register struct serv_proc *sp, *psp;

	DUPRINT4(DB_RFSYS, "del_proc: proc %x, list %x, count %d, ",
		dsp, list, list->sl_count);
        if (list->sl_head == (struct serv_proc *) NULL) {
		DUPRINT1(DB_RFSYS, "empty list\n");
                return;
	}

	if (list->sl_head == dsp) {	/* the one we want is first */
		DUPRINT1(DB_RFSYS, "deleted first element\n");
		list->sl_head = list->sl_head->s_rlink;
		list->sl_count--;
		dsp->s_rlink = (struct serv_proc *) NULL;
		return;
	}

	sp = list->sl_head;
	do {				/* not first - traverse list */
		psp = sp;
		sp = sp->s_rlink;
	} while ((sp != (struct serv_proc *) NULL) && (sp != dsp));

	/* It's sane not to find the server to be removed, since it may be
	   newborn and not on any list, or it may already have been removed 
	   to work on a new request in arrmsg() */
	if (sp == (struct serv_proc *) NULL) {
		DUPRINT1(DB_RFSYS, "not in list\n");
		return;
	}

	DUPRINT1(DB_RFSYS, "deleted in middle\n");
	psp->s_rlink = sp->s_rlink;
	dsp->s_rlink = (struct serv_proc *) NULL;
	list->sl_count--;
	return;
}


/*
 * Get first server process from a list.
 */
struct serv_proc *
get_proc(list)
struct serv_list *list;
{
	struct serv_proc *found;

	found = list->sl_head;
	if (found != (struct serv_proc *) NULL) {
		list->sl_head = found->s_rlink;
		found->s_rlink = (struct serv_proc *) NULL;
		list->sl_count--;
	}
	return(found);	
}


/*
 *	Send reply with opcode over destination SD.
 */

void
reply(dest, opcode)
sndd_t	dest;		/* reply path */
int	opcode;		/* what we did */
{
	mblk_t	*resp_bp;
	struct response *response;
	int	size;

	/*alocbuf and send reply */
	size = sizeof (struct response) - DATASIZE;
	resp_bp = rfs_getbuf(size, BPRI_MED);
	response = (struct response *)PTOMSG(resp_bp->b_rptr);
	response->rp_type = RESP_MSG;
	response->rp_opcode = opcode;
	response->rp_errno = 0;
	response->rp_sig = 0;
	(void) sndmsg (dest, resp_bp, size, (rcvd_t)NULL);
}


/*
 *	Create message for local user-level daemon, and enque it.
 */
 
void
user_msg(opcode, name, size)
long opcode;
char *name;
int  size;
{
	mblk_t	*bp;
	struct u_d_msg *request;

	if ((bp = allocb(sizeof(struct u_d_msg),BPRI_MED)) == NULL) {
		printf("user_msg allocb fails: ");
		printf("resource %s has been disconnected \n", name);
		return;
	}
	request = (struct u_d_msg *) bp->b_wptr;
	request->opcode = opcode;
	(void) strcpy(request->res_name, name);
	request->count = size;
	que_umsg(bp);
}

/*
 *	If there's room on user_daemon queue, enque message and wake daemon.
 */

static void
que_umsg(bp)
mblk_t	*bp;
{
	register int s;
	struct u_d_msg *request;

	s = splrf();
	request = (struct u_d_msg *) bp->b_wptr;
	if (rfmsgcnt > maxgdp) {
		printf("rfs user-daemon queue overflow: ");
		printf("make sure rfudaemon is running\n ");
		DUPRINT3(DB_RFSYS, "opcode %d, name %s\n",
			request->opcode, request->res_name);
		freemsg(bp);
		(void) splx(s);
		return;
	}
	++rfmsgcnt;
	DUPRINT2(DB_RFSYS, "que_umsg: enque msg for user-daemon, count %d\n", 
		rfmsgcnt);
	enque(&rfmsgq, bp);
	(void) splx(s);
	cvsema((caddr_t) &rfadmin_sema);
}

/*
 *This routine is called when flow control is required.
 *It is called with the buffer to be retransmitted along
 *with the send descriptor which is already setup to retransmit
 *to determine the destination.  The buffer type is a 2k type
 *which is a critical resource for the driver. Hence, this buffer
 *is freed and a lesser critical buffer (smaller) can be allocated.
 *One, however, never knows which application may be hogging this 
 *smaller buffer, so if one cannot be allocated, then use the one 
 *you have.
 */

sendback(sd,errno,msgtype)
sndd_t sd;
int errno;
int msgtype;
{
	mblk_t *nbp, *bp;
	mblk_t *sigmsg();
	register struct rcvd *rd;
	register struct response *msg_out;
	register struct message *msg, *nmsg;
	int size, opcode;
	int s;
	/* 
	 * It is wise at this point to try and use a
	 * buffer of different size.  If it is available
	 * then free up the buffer that just ran out of.
	 * If you can't get anther size, reuse the one that 
	 * came in.
	 */
	for (rd = rcvd; (rd < &rcvd[nrcvd]); rd++) {
		if (!(rd->rd_stat & RDUSED) ||
		    !(rd->rd_qtype & GENERAL) || rcvdemp(rd))
			continue;
		if (dequeue(rd, &bp, sd, &size) == RFS_SUCCESS) {
			msg = (struct message *)bp->b_rptr;	
			opcode = ((struct request *)PTOMSG(msg))->rq_opcode;
			if ((opcode == DUCLOSE || opcode == DUIPUT) &&
			   !(msg->m_stat & VER1) ) {
				if ((nbp = allocb (sizeof(struct message)+
				     sizeof (struct response)-DATASIZE, BPRI_MED)) != NULL) {
					nbp->b_wptr += sizeof(struct message);
					nmsg = (struct message *)nbp->b_rptr;	
					bcopy ((caddr_t)msg, (caddr_t)nmsg,
					       sizeof(struct message)+
					       sizeof(struct request)-DATASIZE);
					freemsg(bp);
					DUPRINT2(DB_SYSCALL,"Sendback:3.0 DUCLOSE or DUIPUT opcode = %d\n",opcode);
					s=splrf();
					enque(&rd->rd_rcvdq,nbp);
					rd->rd_qcnt++;
					add_to_msgs_list(rd);
					(void) splx(s);
					continue;
				}else {	/* no small buffer enque and continue*/	
					s=splrf();
					enque(&rd->rd_rcvdq,bp);
					rd->rd_qcnt++;
					add_to_msgs_list(rd);
					(void) splx(s);
					continue;
				}
			} 
			if (!(msg->m_stat & GIFT)) {
				if (opcode != DURSIGNAL) {
					freemsg(bp);
					continue;
				}
				if ((nbp = sigmsg(sd,bp)) == NULL)
					continue;
				bp = nbp;   /* message to be signalled */
				msg = (struct message *)bp->b_rptr;	
				opcode = ((struct request *)
						PTOMSG(msg))->rq_opcode;
				msgtype = RESP_MSG;
				errno = EINTR;
			}
			if ((nbp = allocb (sizeof(struct message)+
			     sizeof(struct response)-DATASIZE, BPRI_MED)) 
			     != NULL) {
				freemsg(bp);	 /* free scare buffer */
				bp = nbp;
			}
			bp->b_rptr = bp->b_datap->db_base;
			bp->b_wptr = bp->b_datap->db_base + 
					sizeof(struct message);
			bzero((char *)bp->b_rptr, sizeof(struct message)+
			       sizeof(struct response)-DATASIZE);
			msg_out = (struct response *)PTOMSG(bp->b_rptr);
			msg_out->rp_opcode = opcode;
			msg_out->rp_type = msgtype;
			msg_out->rp_errno = errno;
			msg_out->rp_bp = (long)bp;
			msg_out->rp_sig = 0;
			msg_out->rp_rval = 0;
			msg_out->rp_subyte = 1;	 /* set to indicate no data */
			msg_out->rp_cache = 0;
			size = sizeof (struct response) - DATASIZE;
			DUPRINT2(DB_SYSCALL,"SENDBACK: syscall=%d\n",opcode);

			/* The only time sndmsg fails is if link is down at 
			   which point you really don't care. */
			(void)sndmsg (sd, bp, size, (rcvd_t)NULL);
		}
	}
}

/* Note, Even though the signal queue "sigrd" is a high priority queue
 * it must be serviced since large numbers of signal messges could
 * accumulate on the overflow queue and hence cause deadlock.
 * This routine tries to identify the process to be signalled.  If the
 * request is still on the queue, it is taken off and returned.
 * if there is one then send the signal back.
 */
mblk_t *
sigmsg(sd,bp)
struct sndd *sd;
mblk_t *bp;
{
	register struct	request *msg_in;
	int cl_pid;	/* Process id of signalling client */
	int cl_sysid;	/* Machine id of signalling client */
	int cls_sysid;	/* Server's version of machine id of signalling client*/
	rcvd_t rdp;	/* rd of signalling client's original request */
	int s;
	struct serv_proc *tp;
	mblk_t *sbp = NULL;
	struct message *msig;
	extern mblk_t *chkrdq();

        msig = (struct message *)bp->b_rptr;
	msg_in = (struct request *) PTOMSG(msig);
	/* Get pid, sysid and rd of signalling client process from message */
	cl_pid = msg_in->rq_pid;
	cl_sysid = msg_in->rq_sysid;
	cls_sysid = GDP((queue_t *)msig->m_queue)->sysid;
	rdp = inxtord(msig->m_dest);
	freemsg(bp);

	/* Look for currently active server serving signalling client */
	for (tp = s_active.sl_head; 
 	     tp != (struct serv_proc *) NULL; tp = tp->s_rlink) {
		if (tp->s_epid == cl_pid && tp->s_sysid == cls_sysid)
			break;
	}

	/* If found, signal server; else set signal bit in matching client
	request awaiting service (if any) */
	if (tp)
		psignal(tp->s_proc, SIGTERM);
	else {
		s = splrf();
		if ((sbp = chkrdq(rdp, (long)cl_pid, (long)cl_sysid)) != NULL) {
                        msig = (struct message *)sbp->b_rptr;
                        msig->m_stat |= SIGNAL;
			set_sndd(sd,(queue_t *)msig->m_queue, 
			    (index_t) msig->m_gindex, (int) msig->m_gconnid);
		}
		(void) splx(s);
	}
	return(sbp);
}
