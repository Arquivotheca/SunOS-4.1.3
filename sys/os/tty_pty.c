#ifndef	lint
static  char sccsid[] = "@(#)tty_pty.c 1.1 92/07/30 SMI";
#endif

/*
 * PTY - Stream "pseudo-tty" device.  For each "controller" side
 * it connects to a "slave" side.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filio.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/vnode.h>	/* 1/0 on the vomit meter */
#include <sys/proc.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <sys/ptyvar.h>
#ifdef	sun
#include <sun/consdev.h>
#endif

extern	int npty;	/* number of pseudo-ttys configured in */
extern struct pty pty_softc[];

#define	IFLAGS	(CS7|CREAD|PARENB)

/*
 * Most of these should be "void", but the people who defined the "streams"
 * data structure for S5 didn't understand data types.
 */

/*
 * Slave side.  This is a streams device.
 */
static int	ptsopen(/*queue_t *q, int dev, int flag, int sflag*/);
static int	ptsclose(/*queue_t *q*/);
static int	ptswput(/*queue_t *q, mblk_t *mp*/);
static int	ptsrserv(/*queue_t *q*/);

static struct module_info ptsm_info = {
	0,
	"ptys",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit ptsrinit = {
	putq,
	ptsrserv,
	ptsopen,
	ptsclose,
	NULL,
	&ptsm_info,
	NULL
};

static struct qinit ptswinit = {
	ptswput,
	NULL,
	NULL,
	NULL,
	NULL,
	&ptsm_info,
	NULL
};

static char *ptsmodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct	streamtab ptsinfo = {
	&ptsrinit,
	&ptswinit,
	NULL,
	NULL,
	ptsmodlist
};

static int	ptsreioctl(/*long unit*/);
static void	ptsioctl(/*struct pty *pty, queue_t *q, mblk_t *bp*/);
static void	pt_sendstop(/*struct pty *pty*/);
static void	ptcwakeup(/*struct pty *pty, int flag*/);

#ifdef	sun
extern int	setcons(/*dev_t dev, u_short uid, u_short gid*/);
extern void	resetcons(/*void*/);

static int	pt_smajor;		/* for console hook */
#endif

/*
 * Open the slave side of a pty.
 */
/*ARGSUSED*/
static int
ptsopen(q, dev, flag, sflag)
	register queue_t *q;
	int dev;
{
	register int unit;
	register struct pty *pty;
	register struct vnode *vp;
	extern struct vnode *slookup();

	unit = minor(dev);
	if (unit >= npty)
		return (OPENFAIL);

	pty = &pty_softc[unit];

	/*
	 * Block waiting for controller to open, unless this is a no-delay
	 * open.
	 */
again:
	if (pty->pt_ttycommon.t_writeq == NULL) {
		pty->pt_ttycommon.t_iflag = 0;
		pty->pt_ttycommon.t_cflag = (B38400 << IBSHIFT)|B38400|IFLAGS;
		pty->pt_ttycommon.t_iocpending = NULL;
		pty->pt_wbufcid = 0;
		pty->pt_ttycommon.t_size.ws_row = 0;
		pty->pt_ttycommon.t_size.ws_col = 0;
		pty->pt_ttycommon.t_size.ws_xpixel = 0;
		pty->pt_ttycommon.t_size.ws_ypixel = 0;
	} else if (pty->pt_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	if (!(flag & (FNBIO|FNONBIO|FNDELAY)) &&
	    !(pty->pt_ttycommon.t_cflag & CLOCAL)) {
		if (!(pty->pt_flags & PF_CARR_ON)) {
			pty->pt_flags |= PF_WOPEN;
			if (sleep((caddr_t)&pty->pt_flags, STIPRI|PCATCH)) {
				pty->pt_flags &= ~PF_WOPEN;
				u.u_error = EINTR;
				return (OPENFAIL);
			}
			goto again;
		}
	}
	pty->pt_ttycommon.t_readq = q;
	pty->pt_ttycommon.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t)pty;
	pty->pt_flags &= ~(PF_WOPEN|PF_SLAVEGONE);

	/*
	 * XXX Find the stream that we are at the end of.
	 * This is a gross breach of all principles of good,
	 * modular code.  However, the same can be said about
	 * the pseudo-tty features that require us to obtain
	 * this information, and unfortunately we're stuck with them.
	 */
	if ((vp = slookup(VCHR, dev)) != NULL) {
		pty->pt_stream = vp->v_stream;
		VN_RELE(vp);	/* slookup() VN_HELD it */
	} else
		pty->pt_stream = NULL;

	return (unit);
}

static int
ptsclose(q)
	register queue_t *q;
{
	register struct pty *pty;

	if ((pty = (struct pty *)q->q_ptr) == NULL)
		return;		/* already been closed once */

#ifdef	sun
	/*
	 * If console output has been routed to this device, we revert to
	 * the "real" console, since the stream for this device will be
	 * dismantled.
	 */
	if (major(consdev) == pt_smajor && minor(consdev) == pty - pty_softc)
		resetcons();
#endif

	ttycommon_close(&pty->pt_ttycommon);

	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (pty->pt_wbufcid) {
		unbufcall(pty->pt_wbufcid);
		pty->pt_wbufcid = 0;
	}

	/*
	 * Clear out all the slave-side state.
	 */
	pty->pt_flags &= ~(PF_WOPEN|PF_STOPPED|PF_NOSTOP);
	if (pty->pt_flags & PF_CARR_ON) {
		pty->pt_flags |= PF_SLAVEGONE;	/* let the controller know */
		ptcwakeup(pty, 0);	/* wake up readers/selectors */
		ptcwakeup(pty, FWRITE);	/* wake up writers/selectors */
	}
	pty->pt_stream = NULL;
	wakeup((caddr_t)&pty->pt_flags);
	q->q_ptr = WR(q)->q_ptr = NULL;
}

/*
 * Put procedure for write queue.
 * Respond to M_STOP, M_START, M_IOCTL, and M_FLUSH messages here;
 * queue up M_DATA messages for processing by the controller "read"
 * routine; discard everything else.
 */
static int
ptswput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct pty *pty;
	register mblk_t *bp;

	pty = (struct pty *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_STOP:
		if (!(pty->pt_flags & PF_STOPPED)) {
			pty->pt_flags |= PF_STOPPED;
			pty->pt_send |= TIOCPKT_STOP;
			ptcwakeup(pty, 0);
		}
		freemsg(mp);
		break;

	case M_START:
		if (pty->pt_flags & PF_STOPPED) {
			pty->pt_flags &= ~PF_STOPPED;
			pty->pt_send = TIOCPKT_START;
			ptcwakeup(pty, 0);
		}
		ptcwakeup(pty, FREAD);	/* permit controller to read */
		freemsg(mp);
		break;

	case M_IOCTL:
		ptsioctl(pty, q, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			/*
			 * Set the "flush write" flag, so that we
			 * notify the controller if they're in packet
			 * or user control mode.
			 */
			if (!(pty->pt_send & TIOCPKT_FLUSHWRITE)) {
				pty->pt_send |= TIOCPKT_FLUSHWRITE;
				ptcwakeup(pty, 0);
			}
			/*
			 * Flush our write queue.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			*mp->b_rptr &= ~FLUSHW;	/* it has been flushed */
		}
		if (*mp->b_rptr & FLUSHR) {
			/*
			 * Set the "flush read" flag, so that we
			 * notify the controller if they're in packet
			 * mode.
			 */
			if (!(pty->pt_send & TIOCPKT_FLUSHREAD)) {
				pty->pt_send |= TIOCPKT_FLUSHREAD;
				ptcwakeup(pty, 0);
			}
			flushq(RD(q), FLUSHDATA);
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
			freemsg(mp);
		break;

	case M_DATA:
		/*
		 * Throw away any leading zero-length blocks, and queue it up
		 * for the controller to read.
		 */
		if (pty->pt_flags & PF_CARR_ON) {
			bp = mp;
			while ((bp->b_wptr - bp->b_rptr) == 0) {
				mp = bp->b_cont;
				freeb(bp);
				if (mp == NULL)
					return;	/* damp squib of a message */
			}
			putq(q, mp);
			ptcwakeup(pty, FREAD);	/* soup's on! */
		} else
			freemsg(mp);	/* nobody listening */
		break;

	case M_CTL:
		switch (*mp->b_rptr) {

		case MC_CANONQUERY:
			/*
			 * We're being asked whether we do canonicalization
			 * or not.  Send a reply back up indicating whether
			 * we do or not.
			 */
			(void) putctl1(RD(q)->q_next, M_CTL,
			    (pty->pt_flags & PF_REMOTE) ?
				MC_NOCANON : MC_DOCANON);
			break;
		}
		freemsg(mp);
		break;

	default:
		/*
		 * "No, I don't want a subscription to Chain Store Age,
		 * thank you anyway."
		 */
		freemsg(mp);
		break;
	}
}

/*
 * Retry an "ioctl", now that "bufcall" claims we may be able to allocate
 * the buffer we need.
 */
static int
ptsreioctl(unit)
	long unit;
{
	register struct pty *pty = &pty_softc[unit];
	queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	pty->pt_wbufcid = 0;
	if ((q = pty->pt_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = pty->pt_ttycommon.t_iocpending) != NULL) {
		/* It's not pending any more. */
		pty->pt_ttycommon.t_iocpending = NULL;
		ptsioctl(pty, q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
ptsioctl(pty, q, mp)
	register struct pty *pty;
	queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	register int cmd;
	register unsigned datasize;
	int error = 0;

	iocp = (struct iocblk *)mp->b_rptr;
	cmd = iocp->ioc_cmd;

	switch (cmd) {

	case TIOCSTI: {
		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call.
		 */
		register mblk_t *bp;

		/*
		 * Simulate typing of a character at the terminal.
		 */
		if ((bp = allocb(1, BPRI_MED)) != NULL) {
			*bp->b_wptr++ = *mp->b_cont->b_rptr;
			if (!(pty->pt_flags & PF_REMOTE)) {
				if (!canput(pty->pt_ttycommon.t_readq->q_next)) {
					ttycommon_qfull(&pty->pt_ttycommon, q);
					freemsg(bp);
				} else
					putnext(pty->pt_ttycommon.t_readq, bp);
			} else {
				if (pty->pt_flags & PF_UCNTL) {
					/*
					 * XXX - flow control; don't overflow
					 * this "queue".
					 */
					if (pty->pt_stuffqfirst != NULL) {
						pty->pt_stuffqlast->b_next = bp;
						bp->b_prev = pty->pt_stuffqlast;
					} else {
						pty->pt_stuffqfirst = bp;
						bp->b_prev = NULL;
					}
					bp->b_next = NULL;
					pty->pt_stuffqlast = bp;
					pty->pt_stuffqlen++;
					ptcwakeup(pty, 0);
				}
			}
		}

		/*
		 * Turn the ioctl message into an ioctl ACK message.
		 */
		iocp->ioc_count = 0;	/* no data returned */
		mp->b_datap->db_type = M_IOCACK;
		goto out;
	}

#ifdef	sun
	case TIOCCONS:
	case _O_TIOCCONS:	/* XXX */
		if (!(pty->pt_flags & PF_CARR_ON)) {
			((struct iocblk *)mp->b_rptr)->ioc_error = EINVAL;
				/* nobody's listening */
			mp->b_datap->db_type = M_IOCNAK;
			goto out;
		}
		error = setcons(makedev(pt_smajor, pty - pty_softc),
		    iocp->ioc_uid, iocp->ioc_gid);
		if (error != 0)
			goto out;

		/*
		 * Turn the ioctl message into an ioctl ACK message.
		 */
		iocp->ioc_count = 0;	/* no data returned */
		mp->b_datap->db_type = M_IOCACK;
		goto out;
#endif
	/*
	 * If they were just trying to drain output, that's OK.
	 * If they are actually trying to send a break it's an error.
	 */
	case TCSBRK:
		if (*(int *)mp->b_cont->b_rptr != 0) {
			/*
			 * Turn the ioctl message into an ioctl ACK message.
			 */
			iocp->ioc_count = 0;	/* no data returned */
			mp->b_datap->db_type = M_IOCACK;
		} else {
			error = ENOTTY;
		}
		goto out;
	}

	/*
	 * The only way in which "ttycommon_ioctl" can fail is if the "ioctl"
	 * requires a response containing data to be returned to the user,
	 * and no mblk could be allocated for the data.
	 * No such "ioctl" alters our state.  Thus, we always go ahead and
	 * do any state-changes the "ioctl" calls for.  If we couldn't allocate
	 * the data, "ttycommon_ioctl" has stashed the "ioctl" away safely, so
	 * we just call "bufcall" to request that we be called back when we
	 * stand a better chance of allocating the data.
	 */
	if ((datasize =
	    ttycommon_ioctl(&pty->pt_ttycommon, q, mp, &error)) != 0) {
		if (pty->pt_wbufcid)
			unbufcall(pty->pt_wbufcid);
		pty->pt_wbufcid = bufcall(datasize, BPRI_HI, ptsreioctl,
		    (long)(pty - pty_softc));
		return;
	}

	if (error == 0) {
		/*
		 * "ttycommon_ioctl" did most of the work; we just use the
		 * data it set up.
		 */
		switch (cmd) {

		case TCSETSF:
		case TCSETAF:
			/*
			 * Set the "flush read" flag, so that we
			 * notify the controller if they're in packet
			 * mode.
			 */
			if (!(pty->pt_send & TIOCPKT_FLUSHREAD)) {
				pty->pt_send |= TIOCPKT_FLUSHREAD;
				ptcwakeup(pty, 0);
			}

		case TCSETSW:
		case TCSETAW:
			cmd = TIOCSETP;	/* map backwards to old codes */
			pt_sendstop(pty);
			break;

		case TCSETS:
		case TCSETA:
			cmd = TIOCSETN;	/* map backwards to old codes */
			pt_sendstop(pty);
			break;
		}
	}

	if (pty->pt_flags & PF_43UCNTL) {
		if (error < 0) {
			if ((cmd & ~0xff) == _IO(u, 0)) {
				if (cmd & 0xff) {
					pty->pt_ucntl = (u_char)cmd & 0xff;
					ptcwakeup(pty, FREAD);
				}
				error = 0; /* XXX */
				goto out;
			}
			error = ENOTTY;
		}
	} else {
		if ((pty->pt_flags & PF_UCNTL) &&
		    (cmd & (_IOC_INOUT | 0xff00)) == (_IOC_IN|('t'<<8)) &&
		    (cmd & 0xff)) {
			pty->pt_ucntl = (u_char)cmd & 0xff;
			ptcwakeup(pty, FREAD);
			goto out;
		}
		if (error < 0)
			error = ENOTTY;
	}

out:
	if (error != 0) {
		((struct iocblk *)mp->b_rptr)->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

static void
pt_sendstop(pty)
	register struct pty *pty;
{
	int stop;

	if ((pty->pt_ttycommon.t_cflag&CBAUD) == 0) {
		if (pty->pt_flags & PF_CARR_ON) {
			/*
			 * Let the controller know, then wake up
			 * readers/selectors and writers/selectors.
			 */
			pty->pt_flags |= PF_SLAVEGONE;
			ptcwakeup(pty, 0);
			ptcwakeup(pty, FWRITE);
		}
	}

	stop = (pty->pt_ttycommon.t_iflag & IXON) &&
	    pty->pt_ttycommon.t_stopc == _CTRL(s) &&
	    pty->pt_ttycommon.t_startc == _CTRL(q);

	if (pty->pt_flags & PF_NOSTOP) {
		if (stop) {
			pty->pt_send &= ~TIOCPKT_NOSTOP;
			pty->pt_send |= TIOCPKT_DOSTOP;
			pty->pt_flags &= ~PF_NOSTOP;
			ptcwakeup(pty, 0);
		}
	} else {
		if (!stop) {
			pty->pt_send &= ~TIOCPKT_DOSTOP;
			pty->pt_send |= TIOCPKT_NOSTOP;
			pty->pt_flags |= PF_NOSTOP;
			ptcwakeup(pty, 0);
		}
	}
}

/*
 * Service routine for read queue.
 * Just wakes the controller side up so it can write some more data
 * to that queue.
 */
static int
ptsrserv(q)
	queue_t *q;
{

	ptcwakeup((struct pty *)q->q_ptr, FWRITE);
}

/*
 * Wake up controller side.  "flag" is 0 if a special packet or
 * user control mode message has been queued up (this data is readable,
 * so we also treat it as a regular data event; should we send SIGIO,
 * though?), FREAD if regular data has been queued up, or FWRITE if
 * the slave's read queue has drained sufficiently to allow writing.
 */
static void
ptcwakeup(pty, flag)
	register struct pty *pty;
	int flag;
{
	if (flag == 0) {
		/*
		 * "Exceptional condition" occurred.  This means that
		 * a "read" is now possible, so do a "read" wakeup.
		 */
		flag = FREAD;
		if (pty->pt_sele) {
			selwakeup(pty->pt_sele,
			    (pty->pt_flags & PF_ECOLL) != 0);
			pty->pt_sele = 0;
			pty->pt_flags &= ~PF_ECOLL;
		}
		if (pty->pt_flags & PF_ASYNC)
			gsignal(pty->pt_pgrp, SIGURG);
	}
	if (flag & FREAD) {
		if (pty->pt_selr) {
			selwakeup(pty->pt_selr,
			    (pty->pt_flags & PF_RCOLL) != 0);
			pty->pt_selr = 0;
			pty->pt_flags &= ~PF_RCOLL;
		}
		wakeup((caddr_t)&pty->pt_ttycommon.t_writeq);
		if (pty->pt_flags & PF_ASYNC)
			gsignal(pty->pt_pgrp, SIGIO);
	}
	if (flag & FWRITE) {
		if (pty->pt_selw) {
			selwakeup(pty->pt_selw,
			    (pty->pt_flags & PF_WCOLL) != 0);
			pty->pt_selw = 0;
			pty->pt_flags &= ~PF_WCOLL;
		}
		wakeup((caddr_t)&pty->pt_ttycommon.t_readq);
		if (pty->pt_flags & PF_ASYNC)
			gsignal(pty->pt_pgrp, SIGIO);
	}
}

/*
 * Controller side.  This is not, alas, a streams device; there are too
 * many old features that we must support and that don't work well
 * with streams.
 */

/*ARGSUSED*/
int
ptcopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct pty *pty;
	register queue_t *q;

	if (minor(dev) >= npty)
		return (ENXIO);
	pty = &pty_softc[minor(dev)];
	if (pty->pt_flags & PF_CARR_ON)
		return (EIO);	/* controller is exclusive use */
				/* XXX - should be EBUSY! */
	if (pty->pt_stream) return(EBUSY); /* at this point, slave side
					      data structure shouldn't have
					      been set up yet, if it is,
					      then that means someone else
					      is still using it */
	if (pty->pt_flags & PF_WOPEN)
		wakeup((caddr_t)&pty->pt_flags);
	if ((q = pty->pt_ttycommon.t_readq) != NULL &&
	    (q = q->q_next) != NULL) {
		/*
		 * Send an un-hangup to the slave, since "carrier" is
		 * coming back up.  Make sure we're doing canonicalization.
		 */
		(void) putctl(q, M_UNHANGUP);
		(void) putctl1(q, M_CTL, MC_DOCANON);
	}
	pty->pt_flags |= PF_CARR_ON;
	pty->pt_send = 0;
	pty->pt_ucntl = 0;

#ifdef	sun
	/*
	 * Kludge to find the major device number of the pseudo-tty
	 * slave.  We need this to construct the major/minor device
	 * number of a pty slave when a TIOCCONS is done (remember,
	 * it can be done on the controller side as well as the slave
	 * side).
	 */
	if (pt_smajor == 0) {
		register struct cdevsw *c;

		for (c = &cdevsw[nchrdev]; --c >= cdevsw; )
			if (c->d_str == &ptsinfo) {
				pt_smajor = c - cdevsw;
				return (0);
			}
		panic("ptcopen: Can't find pty slave major device number");
	}
#endif
	return (0);
}

int
ptcclose(dev)
	dev_t dev;
{
	register struct pty *pty;
	register mblk_t *bp;
	register queue_t *q;

	pty = &pty_softc[minor(dev)];
	if ((q = pty->pt_ttycommon.t_readq) != NULL) {
		/*
		 * Send a hangup to the slave, since "carrier" is dropping.
		 */
		(void) putctl(q->q_next, M_HANGUP);
	}

	/*
	 * Clear out all the controller-side state.  This also
	 * clears PF_CARR_ON, which is correct because the
	 * "carrier" is dropping since the controller process
	 * is going away.
	 */
	pty->pt_flags &= (PF_WOPEN|PF_STOPPED|PF_NOSTOP);
#ifdef	sun
	/*
	 * If console output has been routed to this device, we revert to
	 * the "real" console, since there won't be anybody to receive the
	 * output.
	 */
	if (major(consdev) == pt_smajor && minor(consdev) == minor(dev))
		resetcons();
#endif
	while ((bp = pty->pt_stuffqfirst) != NULL) {
		if ((pty->pt_stuffqfirst = bp->b_next) == NULL)
			pty->pt_stuffqlast = NULL;
		else
			pty->pt_stuffqfirst->b_prev = NULL;
		pty->pt_stuffqlen--;
		freemsg(bp);
	}
}

int
ptcread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register struct pty *pty = &pty_softc[minor(dev)];
	register mblk_t *bp, *nbp;
	register queue_t *q;
	register int error, cc;

	for (;;) {
		/*
		 * If there's a TIOCPKT packet waiting, pass it back.
		 */
		if (pty->pt_flags&(PF_PKT|PF_UCNTL) && pty->pt_send) {
			error = ureadc((int)pty->pt_send, uio);
			if (error)
				return (error);
			pty->pt_send = 0;
			return (0);
		}

		/*
		 * If there's a user-control packet waiting, pass the
		 * "ioctl" code back.
		 */
		if ((pty->pt_flags & (PF_UCNTL|PF_43UCNTL)) && pty->pt_ucntl) {
			error = ureadc((int)pty->pt_ucntl, uio);
			if (error)
				return (error);
			pty->pt_ucntl = 0;
			return (0);
		}

		/*
		 * If there's any data waiting, pass it back.
		 */
		if ((q = pty->pt_ttycommon.t_writeq) != NULL &&
		    q->q_first != NULL &&
		    !(pty->pt_flags & PF_STOPPED)) {
			if (pty->pt_flags & (PF_PKT|PF_UCNTL|PF_43UCNTL)) {
				/*
				 * We're about to begin a move in packet or
				 * user-control mode; precede the data with a
				 * data header.
				 */
				error = ureadc(TIOCPKT_DATA, uio);
				if (error != 0)
					return (error);
			}
			bp = getq(q);
			while (uio->uio_resid > 0) {
				while ((cc = bp->b_wptr - bp->b_rptr) == 0) {
					nbp = bp->b_cont;
					freeb(bp);
					if ((bp = nbp) == NULL) {
						if ((bp = getq(q)) == NULL)
							return (0);
					}
				}
				cc = MIN(cc, uio->uio_resid);
				error = uiomove((caddr_t)bp->b_rptr,
				    cc, UIO_READ, uio);
				if (error != 0) {
					freemsg(bp);
					return (error);
				}
				bp->b_rptr += cc;
			}
			/*
			 * Strip off zero-length blocks from the front of
			 * what we're putting back on the queue.
			 */
			while ((bp->b_wptr - bp->b_rptr) == 0) {
				nbp = bp->b_cont;
				freeb(bp);
				if ((bp = nbp) == NULL)
					return (0);	/* nothing left */
			}
			putbq(q, bp);
			return (0);
		}

		/*
		 * If there's any TIOCSTI-stuffed characters, pass
		 * them back.  (They currently arrive after all output;
		 * is this correct?)
		 */
		if (pty->pt_flags&PF_UCNTL && pty->pt_stuffqfirst != NULL) {
			error = ureadc(TIOCSTI&0xff, uio);
			while (error == 0 &&
			    (bp = pty->pt_stuffqfirst) != NULL &&
			    uio->uio_resid > 0) {
				pty->pt_stuffqlen--;
				error = ureadc((int)*bp->b_rptr, uio);
				if ((pty->pt_stuffqfirst = bp->b_next) == NULL)
					pty->pt_stuffqlast = NULL;
				else
					pty->pt_stuffqfirst->b_prev = NULL;
				freemsg(bp);
			}
#if	0
			wakeup((caddr_t)&pty->pt_stuffq.c_cf);
#endif
			return (error);
		}

		/*
		 * There's no data available.
		 * We want to block until the slave is open, and there's
		 * something to read; but if we lost the slave or we're NBIO,
		 * then return the appropriate error instead.  POSIX-style
		 * non-block has top billing and gives -1 with errno = EAGAIN,
		 * BSD-style comes next and gives -1 with errno = EWOULDBLOCK,
		 * SVID-style comes last and gives 0.
		 */
		if (pty->pt_flags & PF_SLAVEGONE)
			return (EIO);
		if (uio->uio_fmode & FNONBIO)
			return (EAGAIN);
		if (pty->pt_flags & PF_NBIO)
			return (EWOULDBLOCK);
		if (uio->uio_fmode & FNBIO)
			return (0);
		(void) sleep((caddr_t)&pty->pt_ttycommon.t_writeq, STIPRI);
	}
}

int
ptcwrite(dev, uio)
	dev_t dev;
	register struct uio *uio;
{
	register struct pty *pty = &pty_softc[minor(dev)];
	register queue_t *q;
	register int written;
	register int maxmsgsz;
	int maxpsz;
	extern mblk_t *strmakemsg();
	register mblk_t *mp;
	int nonblock;

	nonblock = (pty->pt_flags & PF_NBIO)
		| (uio->uio_fmode & (FNBIO | FNONBIO));

	while ((q = pty->pt_ttycommon.t_readq) == NULL) {
		/*
		 * Wait for slave to open.
		 */
		if (pty->pt_flags & PF_SLAVEGONE)
			return (EIO);
		if (uio->uio_fmode & FNONBIO)
			return (EAGAIN);
		if (pty->pt_flags & PF_NBIO)
			return (EWOULDBLOCK);
		if (uio->uio_fmode & FNBIO)
			return (0);
		(void) sleep((caddr_t)&pty->pt_ttycommon.t_writeq, STOPRI);
	}

	/*
	 * Never put any messages bigger than 1) the maximum stream message
	 * size or 2) the maximum message size for the module above us.
	 */
	maxmsgsz = strmsgsz;
	if ((maxpsz = q->q_next->q_maxpsz) != INFPSZ && maxpsz < maxmsgsz)
		maxmsgsz = maxpsz;

	/*
	 * If in remote mode, even zero-length writes generate messages.
	 */
	written = 0;
	if ((pty->pt_flags & PF_REMOTE) || uio->uio_resid > 0) {
		do {
			while (!canput(q->q_next)) {
				/*
				 * Wait for slave's read queue to unclog.
				 */
				if (pty->pt_flags & PF_SLAVEGONE)
					return (EIO);
				if (uio->uio_fmode & FNONBIO) {
					if (!written)
						return (EAGAIN);
					return (0);
				}
				if (pty->pt_flags & PF_NBIO) {
					if (!written)
						return (EWOULDBLOCK);
					return (0);
				}
				if (uio->uio_fmode & FNBIO)
					return (0);
				(void) sleep((caddr_t)&pty->pt_ttycommon.t_readq,
				    STOPRI);
			}

			if ((mp = strmakemsg((struct strbuf *)NULL,
			    MIN(uio->uio_resid, maxmsgsz), uio,
			    0, (long)0, nonblock)) == NULL) {
				if (u.u_error != EWOULDBLOCK)
					return (u.u_error);
				if (uio->uio_fmode & FNONBIO) {
					if (!written)
						return (EAGAIN);
					return (0);
				}
				if (pty->pt_flags & PF_NBIO) {
					if (!written)
						return (EWOULDBLOCK);
					return (0);
				}
				if (uio->uio_fmode & FNBIO) {
					return (0);
				}
				panic("ptcwrite: null return from strmakemsg");
			}

			/*
			 * Check again for safety; since "uiomove" can take a
			 * page fault, there's no guarantee that "pt_flags"
			 * didn't change while it was happening.
			 */
			if ((q = pty->pt_ttycommon.t_readq) == NULL) {
				freemsg(mp);
				return (EIO);
			}
			putnext(q, mp);
			written = 1;
			if (qready())
				runqueues();
		} while (uio->uio_resid > 0);
	}
	return (0);
}

/*ARGSUSED*/
int
ptcioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	register struct pty *pty = &pty_softc[minor(dev)];
	register queue_t *q;

	switch (cmd) {

	case TIOCPKT:
		if (*(int *)data) {
			if (pty->pt_flags & (PF_UCNTL|PF_43UCNTL))
				return (EINVAL);
			pty->pt_flags |= PF_PKT;
		} else
			pty->pt_flags &= ~PF_PKT;
		break;

	case TIOCUCNTL:
		if (*(int *)data) {
			if (pty->pt_flags & (PF_PKT|PF_UCNTL))
				return (EINVAL);
			pty->pt_flags |= PF_43UCNTL;
		} else
			pty->pt_flags &= ~PF_43UCNTL;
		break;

	case TIOCTCNTL:
		if (*(int *)data) {
			if (pty->pt_flags & PF_PKT)
				return (EINVAL);
			pty->pt_flags |= PF_UCNTL;
		} else
			pty->pt_flags &= ~PF_UCNTL;
		break;

	case TIOCREMOTE:
		if (*(int *)data) {
			if ((q = pty->pt_ttycommon.t_readq) != NULL &&
			    (q = q->q_next) != NULL)
				(void) putctl1(q, M_CTL, MC_NOCANON);
			pty->pt_flags |= PF_REMOTE;
		} else {
			if ((q = pty->pt_ttycommon.t_readq) != NULL &&
			    (q = q->q_next) != NULL)
				(void) putctl1(q, M_CTL, MC_DOCANON);
			pty->pt_flags &= ~PF_REMOTE;
		}
		break;

	case TIOCSIGNAL:
		/*
		 * Blast a M_PCSIG message up the slave stream; the
		 * signal number is the argument to the "ioctl".
		 */
		if ((q = pty->pt_ttycommon.t_readq) != NULL &&
		    (q = q->q_next) != NULL)
			(void) putctl1(q, M_PCSIG, *(int *)data);
		break;

	/*
	 * XXX These should not be here.  The only reason why an
	 * "ioctl" on the controller side should get the
	 * slave side's process group is so that the process on
	 * the controller side can send a signal to the slave
	 * side's process group; however, this is better done
	 * with TIOCSIGNAL, both because it doesn't require us
	 * to know about the slave side's process group and because
	 * the controller side process may not have permission to
	 * send that signal to the entire process group.
	 *
	 * However, since vanilla 4BSD doesn't provide TIOCSIGNAL,
	 * we can't just get rid of them.
	 */
	case TIOCGPGRP:
		if (pty->pt_stream == NULL)
			return (EIO);
		*(int *)data = pty->pt_stream->sd_pgrp;
		break;

	case TIOCSPGRP:
		if (pty->pt_stream == NULL)
			return (EIO);
		pty->pt_stream->sd_pgrp = *(int *)data;
		break;

	case FIONBIO:
		if (*(int *)data)
			pty->pt_flags |= PF_NBIO;
		else
			pty->pt_flags &= ~PF_NBIO;
		break;

	case FIOASYNC:
		if (*(int *)data)
			pty->pt_flags |= PF_ASYNC;
		else
			pty->pt_flags &= ~PF_ASYNC;
		break;

	/*
	 * These, at least, can work on the controller-side process
	 * group.
	 */
	case FIOGETOWN:
		*(int *)data = -pty->pt_pgrp;
		break;

	case FIOSETOWN:
		pty->pt_pgrp = -(*(int *)data);
		break;

	case FIONREAD: {
		/*
		 * Return the total number of bytes of data in all messages
		 * in slave write queue, which is master read queue, unless a
		 * special message would be read.
		 */
		register mblk_t *mp;
		int count = 0;

		if (pty->pt_flags&(PF_PKT|PF_UCNTL) && pty->pt_send)
			count = 1;	/* will return 1 byte */
		else if ((pty->pt_flags & (PF_UCNTL|PF_43UCNTL)) &&
		    pty->pt_ucntl)
			count = 1;	/* will return 1 byte */
		else if ((q = pty->pt_ttycommon.t_writeq) != NULL &&
		    q->q_first != NULL && !(pty->pt_flags & PF_STOPPED)) {
			/*
			 * Will return whatever data is queued up.
			 */
			for (mp = q->q_first; mp != NULL; mp = mp->b_next)
				count += msgdsize(mp);
		} else if ((pty->pt_flags & PF_UCNTL) &&
		    pty->pt_stuffqfirst != NULL) {
			/*
			 * Will return STI'ed data.
			 */
			count = pty->pt_stuffqlen + 1;
		}
		*(int *)data = count;
		break;
	}

	case TIOCSWINSZ:
		/*
		 * Unfortunately, TIOCSWINSZ and the old TIOCSSIZE "ioctl"s
		 * share the same code.  If the upper 16 bits of the number
		 * of lines is non-zero, it was probably a TIOCSWINSZ,
		 * with both "ws_row" and "ws_col" non-zero.
		 */
		if ((((struct ttysize *)data)->ts_lines&0xffff0000) != 0) {
			/*
			 * It's a TIOCSWINSZ.
			 */
			register struct winsize *ws = (struct winsize *)data;

			/*
			 * If the window size changed, send a SIGWINCH.
			 */
			if (bcmp((caddr_t)&pty->pt_ttycommon.t_size,
			    (caddr_t)ws, sizeof (struct winsize))) {
				pty->pt_ttycommon.t_size = *ws;
				if ((q = pty->pt_ttycommon.t_readq) != NULL &&
				    (q = q->q_next) != NULL)
					(void) putctl1(q, M_PCSIG, SIGWINCH);
			}
			break;
		}
		/* FALL THROUGH */

	case TIOCSSIZE:
		pty->pt_ttycommon.t_size.ws_row =
		    ((struct ttysize *)data)->ts_lines;
		pty->pt_ttycommon.t_size.ws_col =
		    ((struct ttysize *)data)->ts_cols;
		pty->pt_ttycommon.t_size.ws_xpixel = 0;
		pty->pt_ttycommon.t_size.ws_ypixel = 0;
		break;

	case TIOCGWINSZ:
		*(struct winsize *)data = pty->pt_ttycommon.t_size;
		break;

	case TIOCGSIZE:
	case _O_TIOCGSIZE:	/* XXX */
		((struct ttysize *)data)->ts_lines =
		    pty->pt_ttycommon.t_size.ws_row;
		((struct ttysize *)data)->ts_cols =
		    pty->pt_ttycommon.t_size.ws_col;
		break;

#ifdef	sun
	case TIOCCONS:
	case _O_TIOCCONS: {	/* XXX */
		register int error;

		/*
		 * If the slave side isn't open, there's no stream, so
		 * we can't make it the console.
		 */
		if (pty->pt_ttycommon.t_writeq == NULL)
			return (EINVAL);
		error = setcons(makedev(pt_smajor, minor(dev)),
		    u.u_uid, u.u_gid);
		if (error != 0)
			return (error);
		break;
	    }
#endif

	/*
	 * This is amazingly disgusting, but the stupid semantics of
	 * 4BSD pseudo-ttys makes us do it.  If we do one of these guys
	 * on the controller side, it really applies to the slave-side
	 * stream.  It should NEVER have been possible to do ANY sort
	 * of tty operations on the controller side, but it's too late
	 * to fix that now.  However, we won't waste our time implementing
	 * anything that the original pseudo-tty driver didn't handle.
	 */
	case TIOCGETP:
	case TIOCSETP:
	case TIOCSETN:
	case TIOCGETC:
	case TIOCSETC:
	case TIOCGLTC:
	case TIOCSLTC:
	case TIOCLGET:
	case TIOCLSET:
	case TIOCLBIS:
	case TIOCLBIC:
		if (pty->pt_stream == NULL ||
		    pty->pt_stream->sd_vnode == NULL ||
		    pty->pt_stream->sd_vnode->v_stream == NULL)
			return (EIO);
		strioctl(pty->pt_stream->sd_vnode, cmd, data, flag);
		return (u.u_error);

	default:
		return (ENOTTY);
	}

	return (0);
}

int
ptcselect(dev, rw)
	dev_t dev;
	int rw;
{
	register struct pty *pty = &pty_softc[minor(dev)];
	register queue_t *q;
	register struct proc *p;
	int s;

	if (pty->pt_flags & PF_SLAVEGONE)
		return (1);
	s = spltty();
	switch (rw) {

	case FREAD:
		if ((q = pty->pt_ttycommon.t_writeq) != NULL &&
		    q->q_first != NULL && !(pty->pt_flags & PF_STOPPED)) {
			/*
			 * Regular data is available.
			 */
			(void) splx(s);
			return (1);
		}
		if (pty->pt_flags & (PF_PKT|PF_UCNTL) && pty->pt_send) {
			/*
			 * A control packet is available.
			 */
			(void) splx(s);
			return (1);
		}
		if ((pty->pt_flags & PF_UCNTL) &&
		    (pty->pt_ucntl || pty->pt_stuffqfirst != NULL)) {
			/*
			 * "ioctl" or TIOCSTI data is available.
			 */
			(void) splx(s);
			return (1);
		}
		if ((pty->pt_flags & PF_43UCNTL) && pty->pt_ucntl) {
			(void) splx(s);
			return (1);
		}
		if ((p = pty->pt_selr) && p->p_wchan == (caddr_t)&selwait)
			pty->pt_flags |= PF_RCOLL;
		else
			pty->pt_selr = u.u_procp;
		break;

	case FWRITE:
		if ((q = pty->pt_ttycommon.t_readq) != NULL &&
		    canput(q->q_next)) {
			(void) splx(s);
			return (1);
		}
		if ((p = pty->pt_selw) && p->p_wchan == (caddr_t)&selwait)
			pty->pt_flags |= PF_WCOLL;
		else
			pty->pt_selw = u.u_procp;
		break;

	case 0:			/* "exceptional conditions" */
		if (((pty->pt_flags & (PF_PKT|PF_UCNTL)) && pty->pt_send) ||
		    ((pty->pt_flags & PF_UCNTL) &&
		    (pty->pt_ucntl || pty->pt_stuffqfirst != NULL))) {
			(void) splx(s);
			return (1);
		}
		if ((pty->pt_flags & PF_43UCNTL) && pty->pt_ucntl) {
			(void) splx(s);
			return (1);
		}
		if ((p = pty->pt_sele) && p->p_wchan == (caddr_t)&selwait)
			pty->pt_flags |= PF_ECOLL;
		else
			pty->pt_sele = u.u_procp;
		break;
	}
	(void) splx(s);
	return (0);
}
