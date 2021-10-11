#ifndef lint
static	char sccsid[] = "@(#)tty.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/errno.h>

void
ttycommon_close(tc)
	register tty_common_t *tc;
{
	tc->t_flags &= ~TS_XCLUDE;
	tc->t_readq = NULL;
	tc->t_writeq = NULL;
	if (tc->t_iocpending != NULL) {
		/*
		 * We were holding an "ioctl" response pending the
		 * availability of an "mblk" to hold data to be passed up;
		 * another "ioctl" came through, which means that "ioctl"
		 * must have timed out or been aborted.
		 */
		freemsg(tc->t_iocpending);
		tc->t_iocpending = NULL;
	}
}

/*
 * A "line discipline" module's queue is full.
 * Check whether IMAXBEL is set; if so, output a ^G, otherwise send an M_FLUSH
 * upstream flushing all the read queues.
 */
void
ttycommon_qfull(tc, q)
	register tty_common_t *tc;
	queue_t *q;
{
	register mblk_t *mp;

	if (tc->t_iflag & IMAXBEL) {
		if (canput(WR(q)) && (mp = allocb(1, BPRI_HI)) != NULL) {
			*mp->b_wptr++ = _CTRL(g);
			(*WR(q)->q_qinfo->qi_putp)(WR(q), mp);
		}
	} else {
		flushq(q, FLUSHDATA);
		(void) putctl1(q->q_next, M_FLUSH, FLUSHR);
	}
}

/*
 * Process an "ioctl" message sent down to us, and return a reply message,
 * even if we don't understand the "ioctl".  Our client may want to use
 * that reply message for its own purposes if we don't understand it but
 * they do, and may want to modify it if we both understand it but they
 * understand it better than we do.
 * If the "ioctl" reply requires additional data to be passed up to the
 * caller, and we cannot allocate an mblk to hold the data, we return the
 * amount of data to be sent, so that our caller can do a "bufcall" and try
 * again later; otherwise, we return 0.
 */
unsigned
ttycommon_ioctl(tc, q, mp, errorp)
	register tty_common_t *tc;
	queue_t *q;
	register mblk_t *mp;
	int *errorp;
{
	register struct iocblk *iocp;
	int ioctlrespsize;
	int s;

	*errorp = 0;	/* no error detected yet */

	iocp = (struct iocblk *)mp->b_rptr;

	switch (iocp->ioc_cmd) {

	case TCSETSF:
		/*
		 * Flush the driver's queue, and send an M_FLUSH upstream
		 * to flush everybody above us.
		 */
		s = spltty();	/* XXX - splzs? */
		flushq(RD(q), FLUSHDATA);
		(void) putctl1(RD(q)->q_next, M_FLUSH, FLUSHR);
		(void) splx(s);

	case TCSETSW:
	case TCSETS: {
		register struct termios *cb =
		    (struct termios *)mp->b_cont->b_rptr;

		/*
		 * The only information we look at are the iflag word,
		 * the cflag word, and the start and stop characters.
		 */
		tc->t_iflag = cb->c_iflag;
		tc->t_cflag = cb->c_cflag;
		tc->t_stopc = cb->c_cc[VSTOP];
		tc->t_startc = cb->c_cc[VSTART];
		break;
	}

	case TCSETAF:
		s = spltty();	/* XXX - splzs? */
		/*
		 * Flush the driver's queue, and send an M_FLUSH upstream
		 * to flush everybody above us.
		 */
		flushq(RD(q), FLUSHDATA);
		(void) putctl1(RD(q)->q_next, M_FLUSH, FLUSHR);
		(void) splx(s);

	case TCSETAW:
	case TCSETA: {
		register struct termio *cb =
		    (struct termio *)mp->b_cont->b_rptr;

		/*
		 * The only information we look at are the iflag word
		 * and the cflag word.
		 * Don't clear out the unset portions, leave them as
		 * they are.
		 */
		tc->t_iflag =
		    (tc->t_iflag & 0xffff0000 | cb->c_iflag);
		tc->t_cflag =
		    (tc->t_cflag & 0xffff0000 | cb->c_cflag);
		break;
	}

	case TIOCSWINSZ: {
		register struct winsize *ws =
		    (struct winsize *)mp->b_cont->b_rptr;

		/*
		 * If the window size changed, send a SIGWINCH.
		 */
		if (bcmp((caddr_t)&tc->t_size, (caddr_t)ws,
		    sizeof (struct winsize))) {
			tc->t_size = *ws;
			(void) putctl1(RD(q)->q_next, M_PCSIG, SIGWINCH);
		}
		break;
	}

	case TIOCSSIZE:
		/*
		 * Set the window size, but don't send a SIGWINCH.
		 */
		tc->t_size.ws_row =
		    ((struct ttysize *)mp->b_cont->b_rptr)->ts_lines;
		tc->t_size.ws_col =
		    ((struct ttysize *)mp->b_cont->b_rptr)->ts_cols;
		tc->t_size.ws_xpixel = 0;
		tc->t_size.ws_ypixel = 0;
		break;

	/*
	 * Prevent more opens.
	 */
	case TIOCEXCL:
		tc->t_flags |= TS_XCLUDE;
		break;

	/*
	 * Permit more opens.
	 */
	case TIOCNXCL:
		tc->t_flags &= ~TS_XCLUDE;
		break;

	/*
	 * Set or clear the "soft carrier" flag.
	 */
	case TIOCSSOFTCAR:
		if (*(int *)mp->b_cont->b_rptr)
			tc->t_flags |= TS_SOFTCAR;
		else
			tc->t_flags &= ~TS_SOFTCAR;
		break;

	/*
	 * The permission checking has already been done at the stream
	 * head, since it has to be done in the context of the process
	 * doing the call.
	 */
	case TIOCSTI: {
		register mblk_t *bp;

		/*
		 * Simulate typing of a character at the terminal.
		 */
		if ((bp = allocb(1, BPRI_MED)) != NULL) {
			if (!canput(tc->t_readq->q_next))
				freemsg(bp);
			else {
				*bp->b_wptr++ = *mp->b_cont->b_rptr;
				putnext(tc->t_readq, bp);
			}
		}
		break;
	}
	}

	/*
	 * Turn the ioctl message into an ioctl ACK message.
	 */
	iocp->ioc_count = 0;	/* no data returned unless we say so */
	mp->b_datap->db_type = M_IOCACK;

	switch (iocp->ioc_cmd) {

	case TCSETSF:
	case TCSETSW:
	case TCSETS:
	case TCSETAF:
	case TCSETAW:
	case TCSETA:
	case TIOCSWINSZ:
	case TIOCSSIZE:
	case TIOCEXCL:
	case TIOCNXCL:
	case TIOCSSOFTCAR:
	case TIOCSTI:
		/*
		 * We've done all the important work on these already;
		 * just reply with an ACK.
		 */
		break;

	case TCGETS: {
		register struct termios *cb;
		register mblk_t *datap;

		if ((datap =
		    allocb(sizeof (struct termios), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (struct termios);
			goto allocfailure;
		}
		cb = (struct termios *)datap->b_wptr;
		/*
		 * The only information we supply is the cflag word.
		 * Our copy of the iflag word is just that, a copy.
		 */
		bzero((caddr_t) cb, sizeof (struct termios));
		cb->c_cflag = tc->t_cflag;
		datap->b_wptr += (sizeof (*cb)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (struct termios);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TCGETA: {
		register struct termio *cb;
		register mblk_t *datap;

		if ((datap = allocb(sizeof (struct termio), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (struct termio);
			goto allocfailure;
		}

		cb = (struct termio *)datap->b_wptr;
		/*
		 * The only information we supply is the cflag word.
		 * Our copy of the iflag word is just that, a copy.
		 */
		bzero((caddr_t) cb, sizeof (struct termio));
		cb->c_cflag = tc->t_cflag;
		datap->b_wptr += (sizeof (*cb)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (struct termio);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	/*
	 * Get the "soft carrier" flag.
	 */
	case TIOCGSOFTCAR: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		if (tc->t_flags & TS_SOFTCAR)
			*(int *)datap->b_wptr = 1;
		else
			*(int *)datap->b_wptr = 0;
		datap->b_wptr += (sizeof (int)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (int);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TIOCGWINSZ: {
		register mblk_t *datap;

		if ((datap =
		    allocb(sizeof (struct winsize), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (struct winsize);
			goto allocfailure;
		}
		/*
		 * Return the current size.
		 */
		*(struct winsize *)datap->b_wptr = tc->t_size;
		datap->b_wptr +=
		    (sizeof (struct winsize)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (struct winsize);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TIOCISPACE: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		/*
		 * Return the current space in the input queue.
		 */
		*(int *)datap->b_wptr = qspace(RD(q));
		datap->b_wptr += (sizeof (int)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (int);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TIOCISIZE: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		/*
		 * Return the size of the input queue.
		 */
		*(int *)datap->b_wptr = qhiwat(RD(q));
		datap->b_wptr += (sizeof (int)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (int);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TIOCMGET: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		*(int *)datap->b_wptr = 0;	/* reserve space */
		datap->b_wptr += (sizeof (int)/sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (int);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		*errorp = -1;	/* we can't do it all */
		break;
	}

	case TIOCOUTQ: {
		register mblk_t *datap;
		register mblk_t *bp;
		int count = 0;

		/*
		 * XXX - doesn't handle messages not yet being DMAed
		 * if they've been removed from the queue.  Also doesn't
		 * handle some queue above us being full, but that's
		 * life (that shouldn't happen in normal operation,
		 * since none of the standard tty modules have service
		 * procedures, and if you have a non-standard tty
		 * module you shouldn't be using this "ioctl" anyway).
		 */
		if ((datap = allocb(sizeof (int), BPRI_HI)) == NULL) {
			ioctlrespsize = sizeof (int);
			goto allocfailure;
		}
		s=spltty();
		for (bp = q->q_first; bp != NULL; bp = bp->b_next)
			count += msgdsize(bp);
		(void)splx(s);
		*(int *)datap->b_wptr = count;
		datap->b_wptr += (sizeof (int)) / (sizeof (*datap->b_wptr));
		iocp->ioc_count = sizeof (int);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	default:
		*errorp = -1;	/* we don't understand it, maybe they do */
		break;
	}
	return (0);

allocfailure:
	/*
	 * We needed to allocate something to handle this "ioctl", but
	 * couldn't; save this "ioctl" and arrange to get called back when
	 * it's more likely that we can get what we need.
	 * If there's already one being saved, throw it out, since it
	 * must have timed out.
	 */
	s = splstr();
	if (tc->t_iocpending != NULL)
		freemsg(tc->t_iocpending);
	tc->t_iocpending = mp;	/* hold this ioctl */
	(void) splx(s);
	return (ioctlrespsize);
}
