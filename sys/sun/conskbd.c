#ifndef lint
static	char sccsid[] = "@(#)conskbd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Console kbd multiplexor driver for Sun.
 * The console "zs" port is linked under us, with the "kbd" module pushed
 * on top of it.
 * Minor device 0 is what programs normally use.
 * Minor device 1 is used to feed predigested keystrokes to the "workstation
 * console" driver, which it is linked beneath.
 */

#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/ttold.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/map.h>

#include <sundev/kbio.h>

static int	directio;

static queue_t	*regqueue;	/* regular keyboard queue above us */
static queue_t	*consqueue;	/* console queue above us */
static queue_t	*lowerqueue;	/* queue below us */

static int	conskbdopen();
static int	conskbdclose();
static int	conskbduwput();
static int	conskbdlrput();
static int	conskbdlwserv();

static struct module_info conskbdm_info = {
	0,
	"conskbd",
	0,
	1024,
	2048,
	128
};

static struct qinit conskbdurinit = {
	putq,
	NULL,
	conskbdopen,
	conskbdclose,
	NULL,
	&conskbdm_info,
	NULL
};

static struct qinit conskbduwinit = {
	conskbduwput,
	NULL,
	conskbdopen,
	conskbdclose,
	NULL,
	&conskbdm_info,
	NULL
};

static struct qinit conskbdlrinit = {
	conskbdlrput,
	NULL,
	NULL,
	NULL,
	NULL,
	&conskbdm_info,
	NULL
};

static struct qinit conskbdlwinit = {
	putq,
	conskbdlwserv,
	NULL,
	NULL,
	NULL,
	&conskbdm_info,
	NULL
};

struct streamtab conskbd_info = {
	&conskbdurinit,
	&conskbduwinit,
	&conskbdlrinit,
	&conskbdlwinit,
	NULL
};

static void conskbdioctl(/*queue_t *q, mblk_t *mp*/);

/*ARGSUSED*/
static int
conskbdopen(q, dev, flag, sflag)
	queue_t *q;
	dev_t dev;
{
	register int unit;

	unit = minor(dev);
	if (unit == 0) {
		/*
		 * Opening "/dev/kbd".
		 */
		if (lowerqueue == NULL)
			return (OPENFAIL);	/* nothing linked below us */
						/* you lose */
		regqueue = q;
	} else if (unit == 1) {
		/*
		 * Opening the device to be linked under the console.
		 */
		consqueue = q;
	} else
		return (OPENFAIL);	/* we don't do that */
					/* under Bozo's Big Tent */

	return (0);
}

static int
conskbdclose(q)
	queue_t *q;
{
	if (q == regqueue) {
		directio = 0;	/* nothing to send it to */
		regqueue = NULL;
	} else if (q == consqueue) {
		/*
		 * Well, this is probably a mistake, but we will permit you
		 * to close the path to the console if you really insist.
		 */
		consqueue = NULL;
	}
}

/*
 * Put procedure for upper write queue.
 */
static int
conskbduwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		conskbdioctl(q, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			flushq(q, FLUSHDATA);
			*mp->b_rptr &= ~FLUSHW;
		}
		if (*mp->b_rptr & FLUSHR) {
			flushq(RD(q), FLUSHDATA);
			qreply(q, mp);
		} else
			freemsg(mp);
		break;

	case M_DATA:
		if (lowerqueue == NULL)
			goto bad;
		putq(lowerqueue, mp);
		break;

	default:
	bad:
#if 0
		pass some kind of error message up (message type
		    M_ERROR, error code EINVAL);
		NOTE THAT THE MTI/ZS DRIVERS SHOULD PROBABLY DO THIS TOO.
#endif
		freemsg(mp);
		break;
	}
}

static void
conskbdioctl(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	mblk_t *replymp;
	register struct iocblk *replyiocp;
	register struct linkblk *linkp;

	if ((replymp = allocb((int)sizeof (struct iocblk), BPRI_HI)) == NULL)
		panic("conskbdioctl: can't allocate reply block");

	iocp = (struct iocblk *)mp->b_rptr;
	replyiocp = (struct iocblk *)replymp->b_wptr;
	replyiocp->ioc_cmd = iocp->ioc_cmd;
	replyiocp->ioc_id = iocp->ioc_id;
	replymp->b_wptr += sizeof (struct iocblk);

	switch (iocp->ioc_cmd) {

	case I_LINK:	/* stupid, but permitted */
	case I_PLINK:
		if (lowerqueue != NULL) {
			replyiocp->ioc_error = EINVAL;	/* XXX */
			goto iocnak;
		}
		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		lowerqueue = linkp->l_qbot;
		replyiocp->ioc_count = 0;
		break;

	case I_UNLINK:	/* stupid, but permitted */
	case I_PUNLINK:
		linkp = (struct linkblk *)mp->b_cont->b_rptr;
		if (lowerqueue != linkp->l_qbot) {
			replyiocp->ioc_error = EINVAL;	/* XXX */
			goto iocnak;	/* not us */
		}
		lowerqueue = NULL;
		replyiocp->ioc_count = 0;
		break;

	case KIOCGDIRECT: {
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_MED)) == NULL)
			panic(
			"conskbdioctl: can't allocate KIOCGDIRECT response");
		*(int *)datap->b_wptr = directio;
		datap->b_wptr += (sizeof (int)/sizeof *datap->b_wptr);
		replymp->b_cont = datap;
		replyiocp->ioc_count = sizeof (int);
		break;
	}

	case KIOCSDIRECT:
		directio = *(int *)mp->b_cont->b_rptr;

		/*
		 * Pass this through, if there's something to pass
		 * it through to, so the system keyboard can reset
		 * itself.
		 */
		if (lowerqueue != NULL) {
			putq(lowerqueue, mp);
			freemsg(replymp);	/* not needed */
			return;
		}
		replyiocp->ioc_count = 0;
		break;

	case TIOCGETD: {
		/*
		 * Pretend it's the keyboard line discipline; what else
		 * could it be?
		 */
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_MED)) == NULL)
			panic("conskbdioctl: can't allocate TIOCGETD response");
		*(int *)datap->b_wptr = KBDLDISC;
		datap->b_wptr += (sizeof (int)/sizeof *datap->b_wptr);
		replymp->b_cont = datap;
		replyiocp->ioc_count = sizeof (int);
		break;
	}

	case TIOCSETD:
		/*
		 * Unless they're trying to set it to the keyboard line
		 * discipline, reject this call.
		 */
		if (*(int *)mp->b_cont->b_rptr != KBDLDISC) {
			replyiocp->ioc_error = EINVAL;
			goto iocnak;
		}
		replyiocp->ioc_count = 0;
		break;

	default:
		/*
		 * Pass this through, if there's something to pass it
		 * through to; otherwise, reject it.
		 */
		if (lowerqueue != NULL) {
			putq(lowerqueue, mp);
			freemsg(replymp);	/* not needed */
			return;
		}
		replyiocp->ioc_error = EINVAL;
		goto iocnak;	/* nobody below us; reject it */
	}

	/*
	 * Common exit path for calls that return a positive
	 * acknowledgment with a return value of 0.
	 */
	replyiocp->ioc_rval = 0;
	replyiocp->ioc_error = 0;	/* brain rot */
	replymp->b_datap->db_type = M_IOCACK;
	qreply(q, replymp);
	freemsg(mp);
	return;

iocnak:
	replyiocp->ioc_rval = 0;
	replymp->b_datap->db_type = M_IOCNAK;
	qreply(q, replymp);
	freemsg(mp);
}

/*
 * Service procedure for lower write queue.
 * Puts things on the queue below us, if it lets us.
 */
static int
conskbdlwserv(q)
	register queue_t *q;
{
	register mblk_t *mp;

	while (canput(q->q_next) && (mp = getq(q)) != NULL)
		putnext(q, mp);
}

/*
 * Put procedure for lower read queue.
 * Pass everything up to minor device 0 if "directio" set, otherwise to minor
 * device 1.
 */
static int
conskbdlrput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	int s;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr == FLUSHW || *mp->b_rptr == FLUSHRW) {
			s = spl5();
			/*
			 * Flush our write queue.
			 */
			/* XXX doesn't flush M_DELAY */
			flushq(WR(q), FLUSHDATA);
			(void) splx(s);
			*mp->b_rptr = FLUSHR;	/* it has been flushed */
		}
		if (*mp->b_rptr == FLUSHR || *mp->b_rptr == FLUSHRW) {
			s = spl5();
			flushq(q, FLUSHDATA);
			(void) splx(s);
			*mp->b_rptr = FLUSHW;	/* it has been flushed */
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
			freemsg(mp);
		break;

	case M_ERROR:
	case M_HANGUP:
		/* XXX - should we tell the upper queue(s) about this? */
		freemsg(mp);
		break;

	case M_DATA:
		if (directio)
			putnext(regqueue, mp);
		else {
			if (consqueue != NULL)
				putnext(consqueue, mp);
		}
		break;

	case M_IOCACK:
	case M_IOCNAK:
		putnext(regqueue, mp);
		break;

	default:
		freemsg(mp);	/* anything useful here? */
		break;
	}
}
