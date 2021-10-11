/*	@(#)queue.c 1.1 92/07/30 SMI 	*/

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* #ident	"@(#)kern-port:nudnix/queue.c	10.18" */
/*
 *	q u e u e . c
 *
 *  A msg is the frozen instance of a system call
 *  that was begun on the local machine and, because it
 *  referenced a remote file, needs to be completed
 *  on a remote machine.
 *
 *  Msgs are received by queues.  queues either belong to the
 *  general pool of queues, which is handled as a unit,
 *  or they are handled individually.
 *
 *  A process can either wait for a msg on a specific queue
 *  or for any msg from the general pool of queues.
 */
#include <sys/types.h>
#include <rfs/sema.h>
#include <sys/sysmacros.h>
#include <rfs/rfs_misc.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/stream.h>
#include <rfs/comm.h>
#include <sys/user.h>
#include <sys/vnode.h>
#include <rfs/message.h>
#include <rfs/nserve.h>
#include <rfs/rfs_mnt.h>
#include <rfs/recover.h>
#include <sys/proc.h>
#include <sys/debug.h>
#include <rfs/rdebug.h>
#include <rfs/rfs_serve.h>

extern void	add_proc();
extern void	del_proc();

struct rcvd *msgs = NULL;	/* messages waiting for servers */
extern int msglistcnt;		/* number of rcvd's on message list */


/*
 * de_queue:  dequeue a message from the specified queue.  handle remote 
 * signals, wait for a msg to arrive, dequeue the msg.
 */
int
de_queue(rqueue, bufp, new_gift, size, srsig, sdp)
rcvd_t	rqueue;
mblk_t	**bufp;
sndd_t	new_gift;
int	*size;
int 	*srsig;		/* Flag for sent remote signal, passed in and out */
sndd_t	sdp;		/* sd to send remote signal on. If NULL, don't send */
{
	int sig = 0;
	char cursig = 0;
	int	s;
	extern rcvd_t sigrd;
	int error = 0;
	int caught_signal;

	/* This is where we have to worry about remote signals.
	 * If we were awakened and return with a 1, we've got
	 * a signal and a signal message should be sent.  
	 * Otherwise the wakeup was (presumably) because of an incoming
	 * message.
	 */

	if (*srsig)
		SAVE_SIG(u.u_procp, sig, cursig);

	s = splrf();
	while (1) {
		if (rqueue->rd_stat & RDLINKDOWN) {
			(void) splx(s);
			RESTORE_SIG(u.u_procp, sig, cursig);
			DUPRINT2(DB_GDPERR,"de_queue: lnk down rd %x\n", rqueue);
			return(ENOLINK);
		}
		if (rqueue->rd_qcnt)
			break;
		caught_signal = psema((caddr_t) &rqueue->rd_qslp, PREMOTE|PCATCH);
		if (caught_signal && !*srsig && sdp) {
			(void) splx(s);
			DUPRINT2(DB_SIGNAL,"de_queue:sig=%x\n",u.u_procp->p_sig);
			if (error = du_rsignal(sdp))
				return(error);
			(*srsig)++;
			s = splrf();
		}
		ACCUM_SIG(u.u_procp, sig, cursig);
	}	
	(void) splx(s);
	RESTORE_SIG(u.u_procp, sig, cursig);
	(void) dequeue(rqueue,bufp,new_gift,size);
	DUPRINT2(DB_COMM, "de_queue: dequeue on rd %x", rqueue);
	return(error);
}


/*
 * serve_dq:  
 *	Sleep waiting for a message on to arrive on an rd.
 *	When awoken:
 *		If caught signal return (and this server dies).
 *		Else, take message from signal rd, if there is one.
 *		Else, take message from first available rd.
 *		Else, if too many idle servers return and die.
 *		Else, do it again.
 */
rcvd_t
serve_dq(bufp, size, sp)
mblk_t	**bufp;
int	*size;
register struct serv_proc *sp;
{
	int s;
	int caught_signal;
	extern rcvd_t sigrd;
        extern  struct timeval  time;
	extern long rfs_serve_time;
	register struct proc *p = sp->s_proc;
	extern struct serv_list s_active;

	del_proc(sp, &s_active);		/* No longer at work */
	p->p_sig &= ~(sigmask(SIGTERM) | sigmask(SIGUSR1));
	if (p->p_cursig == SIGUSR1 || p->p_cursig == SIGTERM)
		p->p_cursig = 0;

	while (1) {
		s = splrf();  /* Lock out arriving messages' calls to arrmsg() */
		/* Discard old rd */
		if (sp->s_rdp != (rcvd_t) NULL)  {
			sp->s_rdp->rd_act_cnt--;
			sp->s_rdp = (rcvd_t) NULL;
		}
		/* If there's a signal message, handle it */
		if (sigrd->rd_rcvdq.qc_head != NULL) {
			sp->s_rdp = sigrd;
			sp->s_rdp->rd_act_cnt++;
			(void) splx(s);
			if (dequeue(sigrd,bufp,sp->s_sdp,size) == RFS_SUCCESS) {
				break;
			} else  continue;
		}
		/* Otherwise, if there's a regular message handle that */
		if (msgs != NULL)  {
			sp->s_rdp = msgs;
			sp->s_rdp->rd_act_cnt++;
			(void) splx(s);
			if (dequeue(msgs,bufp,sp->s_sdp,size) == RFS_SUCCESS) {
				break;
			} else  continue;
		}
		/* No message and more servers than needed so return and die */
		if ((s_idle.sl_count + 1) > minserve && 
			(time.tv_sec - sp->s_tlast) > rfs_serve_time) {
			(void) splx(s);
			break;
		}
		sp->s_tlast = time.tv_sec;
		/* Sleep waiting for a message.  If caught signal, return and
		die. This is the only place servers can be killed. The epid is 
		reset so that we don't accidentally try to signal this server. */
		add_proc(sp, &s_idle);
		caught_signal = psema((caddr_t) &sp->s_sleep, PREMOTE|PCATCH);
		del_proc(sp, &s_idle);
		(void) splx(s);
		if (caught_signal)
			break;
	}

	/* This server is going to work on s_rdp */
	if (sp->s_rdp) {
		add_proc(sp, &s_active);
		p->p_cursig = 0;
		p->p_sig  = 0;
		if (s_active.sl_count >= maxserve) /* Last srvr can't sleep */
			psignal(p, SIGUSR1);
        	else if (s_idle.sl_count == 0)   /* Adjust # servers */
			add_server();
	}
	return(sp->s_rdp);
}

/*
 * add receive queues to the END of the msgs list.
 * duplicates are ignored because interrupts on
 * different receive descriptors can try to put
 * the same receive queue on the msgs list.
 */
add_to_msgs_list(rcvdp)
rcvd_t rcvdp;
{
	rcvd_t current;

	if (msgs == NULL)  {
		msgs = rcvdp;
		rcvdp->rd_next = NULL;
		msglistcnt = 1;
	}
	else	{
		current = msgs;
		while ((current != rcvdp)
			&& (current->rd_next != NULL))
			current = current->rd_next;
		if (current != rcvdp) { 
			current->rd_next = rcvdp;
			rcvdp->rd_next = NULL;
			msglistcnt++;
		}
	}
	return;
}

/*
 * remove the queue from the msgs list.
 */
rm_msgs_list(rcvdp)
rcvd_t rcvdp;
{
	rcvd_t prev, current;

	if (!(rcvdemp(rcvdp)))
		return;
	if (msgs != NULL) {
		prev = current = msgs;
		if (msgs == rcvdp) {
			msgs = msgs->rd_next;
			msglistcnt--;
		} else  {
			while (((current = prev->rd_next) != NULL) 
				&& (current != rcvdp))
					prev = current;
			if (current != NULL) {
				prev->rd_next = current->rd_next;
				msglistcnt--;
			}
		}
	}
	return;
}
