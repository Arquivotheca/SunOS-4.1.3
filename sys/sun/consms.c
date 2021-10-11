#ifndef lint
static	char sccsid[] = "@(#)consms.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Console mouse driver for Sun.
 * The console "zs" port is linked under us, with the "ms" module pushed
 * on top of it.
 *
 * We don't do any real multiplexing here; this device merely provides a
 * way to have "/dev/mouse" automatically have the "ms" module present.
 * Due to problems with the way the "specfs" file system works, you can't
 * use an indirect device (a "stat" on "/dev/mouse" won't get the right
 * snode, so you won't get the right time of last access), and due to
 * problems with the kernel window system code, you can't use a "cons"-like
 * driver ("/dev/mouse" won't be a streams device, even though operations
 * on it get turned into operations on the real stream).
 */

#include <sys/param.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/ttold.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/map.h>

#include <sun/consdev.h>

static queue_t	*upperqueue;	/* regular keyboard queue above us */
static queue_t	*lowerqueue;	/* queue below us */

static int	consmsopen();
static int	consmsclose();
static int	consmsuwput();
static int	consmslrput();
static int	consmslwserv();

static struct module_info consmsm_info = {
	0,
	"consms",
	0,
	1024,
	2048,
	128
};

static struct qinit consmsurinit = {
	putq,
	NULL,
	consmsopen,
	consmsclose,
	NULL,
	&consmsm_info,
	NULL
};

static struct qinit consmsuwinit = {
	consmsuwput,
	NULL,
	consmsopen,
	consmsclose,
	NULL,
	&consmsm_info,
	NULL
};

static struct qinit consmslrinit = {
	consmslrput,
	NULL,
	NULL,
	NULL,
	NULL,
	&consmsm_info,
	NULL
};

static struct qinit consmslwinit = {
	putq,
	consmslwserv,
	NULL,
	NULL,
	NULL,
	&consmsm_info,
	NULL
};

struct streamtab consms_info = {
	&consmsurinit,
	&consmsuwinit,
	&consmslrinit,
	&consmslwinit,
	NULL
};

static void consmsioctl(/*queue_t *q, mblk_t *mp*/);

/*ARGSUSED*/
static int
consmsopen(q, dev, flag, sflag)
	queue_t *q;
	dev_t dev;
{

	if (mousedev == NODEV)
		return (OPENFAIL);	/* no mouse - you lose */
	upperqueue = q;
	return (0);
}

/*ARGSUSED*/
static int
consmsclose(q)
	queue_t *q;
{

	upperqueue = NULL;
}

/*
 * Put procedure for upper write queue.
 */
static int
consmsuwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		consmsioctl(q, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);
		if (lowerqueue != NULL)
			putq(lowerqueue, mp);		/* pass it through */
		else {
			/*
			 * No lower queue; just reflect this back upstream.
			 */
			*mp->b_rptr &= ~FLUSHW;
			if (*mp->b_rptr & FLUSHR)
				qreply(q, mp);
			else
				freemsg(mp);
		}
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
consmsioctl(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	mblk_t *replymp;
	register struct iocblk *replyiocp;
	register struct linkblk *linkp;

	if ((replymp = allocb((int)sizeof (struct iocblk), BPRI_HI)) == NULL)
		panic("consmsioctl: can't allocate reply block");

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

	case TIOCGETD: {
		/*
		 * Pretend it's the mouse line discipline; what else
		 * could it be?
		 */
		register mblk_t *datap;

		if ((datap = allocb(sizeof (int), BPRI_MED)) == NULL)
			panic("consmsioctl: can't allocate TIOCGETD response");
		*(int *)datap->b_wptr = MOUSELDISC;
		datap->b_wptr += (sizeof (int)/sizeof *datap->b_wptr);
		replymp->b_cont = datap;
		replyiocp->ioc_count = sizeof (int);
		break;
	}

	case TIOCSETD:
		/*
		 * Unless they're trying to set it to the mouse line
		 * discipline, reject this call.
		 */
		if (*(int *)mp->b_cont->b_rptr != MOUSELDISC) {
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
consmslwserv(q)
	register queue_t *q;
{
	register mblk_t *mp;

	while (canput(q->q_next) && (mp = getq(q)) != NULL)
		putnext(q, mp);
}

/*
 * Put procedure for lower read queue.
 */
static int
consmslrput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(q), FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		if (upperqueue != NULL)
			putnext(upperqueue, mp);	/* pass it through */
		else {
			/*
			 * No upper queue; just reflect this back downstream.
			 */
			*mp->b_rptr &= ~FLUSHR;
			if (*mp->b_rptr & FLUSHW)
				qreply(q, mp);
			else
				freemsg(mp);
		}
		break;

	case M_ERROR:
	case M_HANGUP:
		/* XXX - should we tell the upper queue(s) about this? */
		freemsg(mp);
		break;

	case M_DATA:
	case M_IOCACK:
	case M_IOCNAK:
		if (upperqueue != NULL)
			putnext(upperqueue, mp);
		else
			freemsg(mp);
		break;

	default:
		freemsg(mp);	/* anything useful here? */
		break;
	}
}
