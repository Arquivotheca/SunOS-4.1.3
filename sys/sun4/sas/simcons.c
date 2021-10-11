#ifndef lint
static	char sccsid[] = "@(#)simcons.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Console driver for the kernel under the Sparc Architecture Simulator.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/kernel.h>

#include <machine/clock.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/intreg.h>
#include <sundev/mbvar.h>

#define	ISPEED	B9600
#define	IFLAGS	(CS7|CREAD|PARENB)

int	simc_flags;		/* random flags */
tty_common_t simc_ttycommon;	/* data common to all tty drivers */

#define	SIMC_ISOPEN	0x00000002	/* open is complete */
#define	SIMC_STOPPED	0x00000010	/* output is stopped */

static int simcopen();
static int simcclose();
static int simcwput();

static struct module_info simcm_info = {
	0,
	"simc",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit simcrinit = {
	putq,
	NULL,
	simcopen,
	simcclose,
	NULL,
	&simcm_info,
	NULL
};

static struct qinit simcwinit = {
	simcwput,
	NULL,
	simcopen,
	simcclose,
	NULL,
	&simcm_info,
	NULL
};

static char *simcmodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab simcstab = {
	&simcrinit,
	&simcwinit,
	NULL,
	NULL,
	simcmodlist
};

int simcprobe(), simcattach(), simcintr_lowpri();

struct mb_driver simcdriver = {
	simcprobe, 0, simcattach, 0, 0, simcintr_lowpri,
	0, "simc", 0, 0, 0, 0
};

static void	simcioctl( /* queue_t *q, mblk_t *mp */ );
static void	simcstart();

extern char	simxinc();

/*ARGSUSED*/
static int
simcopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t dev;
{

	if (minor(dev) != 0)
		return (OPENFAIL);
	if (!(simc_flags & SIMC_ISOPEN)) {
		simc_ttycommon.t_iflag = 0;
		simc_ttycommon.t_cflag = (ISPEED << IBSHIFT)|ISPEED|IFLAGS;
		simc_flags = SIMC_ISOPEN;
	} else if (simc_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	simc_ttycommon.t_readq = q;
	simc_ttycommon.t_writeq = WR(q);
	return (dev);
}

static int
simcclose(q)
	register queue_t *q;
{

	simc_flags &= ~SIMC_ISOPEN;
	simc_ttycommon.t_flags &= ~TS_XCLUDE;
}

/*
 * Put procedure for write queue.
 * Respond to M_STOP, M_START, M_IOCTL, and M_FLUSH messages here;
 * set the flow control character for M_STOPI and M_STARTI messages;
 * queue up M_BREAK, M_DELAY, and M_DATA messages for processing
 * by the start routine, and then call the start routine; discard
 * everything else.
 */
static int
simcwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register int s;

	switch (mp->b_datap->db_type) {
	case M_STOP:
		/*
		 * Since we don't do real DMA, we can just let the
		 * chip coast to a stop after applying the brakes.
		 */
		simc_flags |= SIMC_STOPPED;
		freemsg(mp);
		break;

	case M_START:
		if (simc_flags & SIMC_STOPPED) {
			simc_flags &= ~SIMC_STOPPED;
			simcstart();
		}
		freemsg(mp);
		break;

	case M_IOCTL:
		simcioctl(q, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			s = splzs();
			/*
			 * Flush our write queue.
			 * Probably unnecessary here.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			(void) splx(s);
			*mp->b_rptr &= ~FLUSHW;	/* it has been flushed */
		}
		if (*mp->b_rptr & FLUSHR) {
			s = splzs();
			flushq(RD(q), FLUSHDATA);
			(void) splx(s);
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
			freemsg(mp);
		break;

	case M_DATA:
		/*
		 * Queue the message up to be transmitted,
		 * and poke the start routine.
		 */
		putq(q, mp);
		simcstart();
		break;

	case M_BREAK:
	case M_DELAY:
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
/*ARGSUSED*/
static int
simcreioctl(unit)
	long unit;
{
	queue_t *q;
	register mblk_t *mp;

	if ((q = simc_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = simc_ttycommon.t_iocpending) != NULL) {
		simc_ttycommon.t_iocpending = NULL;
		simcioctl(q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
simcioctl(q, mp)
	queue_t *q;
	register mblk_t *mp;
{
	struct iocblk *iocp;
	register unsigned datasize;
	int error;
	int s;

	if (simc_ttycommon.t_iocpending != NULL) {
		/*
		 * We were holding an "ioctl" response pending the
		 * availability of an "mblk" to hold data to be passed up;
		 * another "ioctl" came through, which means that "ioctl"
		 * must have timed out or been aborted.
		 */
		freemsg(simc_ttycommon.t_iocpending);
		simc_ttycommon.t_iocpending = NULL;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	/*
	 * The only way in which "ttycommon_ioctl" can fail is if the "ioctl"
	 * requires a response containing data to be returned to the user,
	 * and no mblk could be allocated for the data.
	 * No such "ioctl" alters our state. Thus, we always go ahead and
	 * do any state-changes the "ioctl" calls for, and just stash the
	 * "ioctl" response structure away for a later retry if we couldn't
	 * allocate the data.
	 */
	if ((datasize = ttycommon_ioctl(&simc_ttycommon, q, mp, &error)) != 0) {
		simc_ttycommon.t_iocpending = mp;
		(void) bufcall(datasize, BPRI_HI, simcreioctl, 0L);
		return;
	}
	if (error < 0) {
		/*
		 * "ttyioctl" didn't do anything; we process it here.
		 */
		error = 0;
		switch (iocp->ioc_cmd) {

		case TIOCSBRK:
		case TIOCCBRK:
		default:
			/*
			 * We don't understand it either.
			 */
			error = ENOTTY;
			break;
		}
	}
	if (error != 0) {
		((struct iocblk *)mp->b_rptr)->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

/*
 * Start output on a line, unless it's busy, frozen, or otherwise.
 */
static void
simcstart()
{
	register queue_t *q;
	register mblk_t *bp;
	register int s;

	if ((q = simc_ttycommon.t_writeq) == NULL)
		return;		/* not attached to a stream */

	s = splzs();
	while ((bp = getq(q)) != NULL) {
		/*
		 * We have a message block to work on.
		 */
		switch (bp->b_datap->db_type) {

		case M_BREAK:
		case M_DELAY:
		case M_IOCTL:
			freemsg(bp);
			continue;

		default:
			do {
				register int cc;
				register mblk_t *nbp;

				cc = bp->b_wptr - bp->b_rptr;
				while (cc--) {
					/* let input in every 8 chars */
					if ((cc % 8) == 7) {
						(void) splx(s);
						(void) splzs();
					}
					/*
					 * If output is stopped, put it back
					 * and try again later.
					 */
					if (simc_flags & SIMC_STOPPED) {
						putbq(q, bp);
						(void) splx(s);
						return;
					}
					simcoutc(*bp->b_rptr++);
				}
				nbp = bp->b_cont;
				freeb(bp);
				bp = nbp;
			} while (bp != NULL);
		}
	}
	(void) splx(s);
}

/*
 * Receiver interrupt.
 * Model the standard serial ports on sun machines
 * with a two level interrupt handler.
 */
int
simcintr_lowpri()
{
	register mblk_t *bp;
	register queue_t *q;
	register char c;
	char get_ringbuf();

	set_intreg(IR_SOFT_INT6, 0);	/* turn off softint level 6 */
	while ( c = get_ringbuf() ) {
		/*
		 * Send data up the stream if there's somebody listening.
		 */
		if ((q = simc_ttycommon.t_readq) != NULL) {
			if ((bp = allocb(1, BPRI_MED)) != NULL) {
				if (!canput(q->q_next))
				{
					ttycommon_qfull(&simc_ttycommon, q);
					freemsg(bp);
				}
				else {
					*bp->b_wptr++ = c;
					putnext(q, bp);
				}
			}
		}
	}
	return (1);
}

/*
 * High priority interrupt handler.
 * Read the character and stick in a ringbuffer.
 */
int
simcintr()
{
	put_ringbuf(simcinc());		/* get character from sas */

	/*
	 * handle rest of interrupt processing later, at lower priority,
	 * don't fool around with streams buffers at hi priority
	 */
	set_intreg(IR_SOFT_INT6, 1);	/* turn on softint level 6 */
	return (1);
}

cnputc(c)
	register char c;
{
	if (c == '\n')
		simcoutc('\r');
	simcoutc(c);
}

int
cngetc()
{

	return ((int) simcinc());
}

getchar()
{
	register char c;

	c = cngetc();
	if (c=='\r')
		c = '\n';
	cnputc(c);
	return (c);
}

gets(cp)
	char *cp;
{
	register char *lp;
	register char c;

	lp = cp;
	while (c = getchar() & 0177) {
		switch (c) {
		case '\n':
		case '\r':
			*lp++ = '\0';
			return;
		case 0177:
			cnputc('\b');
		case '\b':
		case '#':
			lp--;
			if (lp < cp)
				lp = cp;
			continue;
		case '@':
		case 'u'&037:
			lp = cp;
			cnputc('\n');
			continue;
		default:
			*lp++ = c;
		}
	}
	*lp++ = '\0';
}

simcprobe()
{
	return (1);
}

#define SIMC_RBSIZE 64
char simc_ringbuf[SIMC_RBSIZE];
char *simc_put, *simc_get;

simcattach()
{
	simc_put = simc_ringbuf;	/* initialize ringbuffer pointers */
	simc_get = simc_ringbuf;
}

char
get_ringbuf()
{
	register char c;
	register int s;

	if (simc_put == simc_get) {
		return (0);		/* emptied ring buffer */
	} else {
		/* protect ring buffer pointer update from level 12 int */
		s = splzs();
		c = *simc_get++;
		(void) splx(s);

		if (simc_get == &simc_ringbuf[SIMC_RBSIZE])
			simc_get = simc_ringbuf;
		return (c);
	}
};

/* assumed to be called at high priority from level 12 interrupt */
put_ringbuf(c)
	register char c;
{
	register int s;

	s = splzs();		/* protection from hi level interrupt */
	*simc_put++ = c;	/* insert input character */
	(void) splx(s);
	if (simc_put == &simc_ringbuf[SIMC_RBSIZE])
		simc_put = simc_ringbuf;
	if (simc_put == simc_get)
		printf("simc input ringbuf wraparound, input may be lost\n");
};
