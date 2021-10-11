#ifndef lint
static	char sccsid[] = "@(#)mti.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987, 1989 by Sun Microsystems, Inc.
 */

/*
 * Systech MTI-800/1600 Multiple Terminal Interface driver
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/clist.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/map.h>
#include <sys/varargs.h>

#include <sun/consdev.h>
#include <sundev/mbvar.h>
#include <sundev/mtireg.h>
#include <sundev/mtivar.h>

/*
 * Driver information for auto-configuration stuff.
 * Should be "void", not "int" - they return no values!
 */
static int	mtiprobe(/*caddr_t reg*/);
static int	mtiattach(/*struct mb_device *md*/);
int		mtiintr(/*void*/);

extern	struct mb_device *mtiinfo[];

struct	mb_driver mtidriver = {
	mtiprobe, 0, mtiattach, 0, 0, mtiintr,
	sizeof (struct mtireg), "mti", mtiinfo, 0, 0, 0,
};

extern	int nmti;	/* number of MTI lines configured in */

#define	OUTLINE	0x80	/* minor device flag for dialin/out on same line */
/*#define	OUTLINE	0	/* disable dialin/out */
#define	UNIT(x)	(minor(x)&~OUTLINE)
#define	BOARD(dev)	(((dev)>>4)&0x7)	/* board number for given dev */
#define	LINE(dev)	((dev)&0xf)	/* line number for given dev */
 
extern struct mtiline mtiline[];

#define	MTIBUFSIZE	64	/* size of DMA buffers */

extern	struct mti_softc mti_softc[];

static	char mti_speeds[16] = {
	0,	/* B0 */
	0,	/* B50 */
	1,	/* B75 */
	2,	/* B110 */
	3,	/* B134 */
	4,	/* B150 */
	9,	/* B200 - MTI doesn't support, 2000 baud instead */
	5,	/* B300 */
	6,	/* B600 */
	7,	/* B1200 */
	8,	/* B1800 */
	10,	/* B2400 */
	12,	/* B4800 */
	14,	/* B9600 */
	15,	/* B19200 */
	13	/* B38400/EXTB - MTI doesn't support, 7200 baud instead */
};

static	char cmdlen[16] = {	/* number of bytes in normal commands */
	1,			/* Enable Single Character Input */
	1,			/* Read USART Status Register */
	1,			/* Read Error Code */
	0,
	2,			/* Single Character Output */
	2,			/* Write USART Command Register */
	1,			/* Disable Single Character Input */
	5,			/* Configuration Command */
	6,			/* Block Input */
	0,			
	1,			/* Abort Input */
	1,			/* Suspend Output */
	6,			/* Block Output */
	1,			/* Resume Output */
	1,			/* Abort Output */
	0
};

static	char configlen[16] = {	/* number of bytes in configure commands */
	5,			/* Configure Asynchronous */
	8,			/* Configure Synchronous */
	0,
	0,
	3,			/* Configure Input */
	5,			/* Termination Mask */
	3,			/* Configure Output */
	3,			/* Modem Status */
	5,			/* Buffer Sizes */
	2,			/* Configure Timer */
	0,
	0,
	0,
	0,
	0,
	0
};

static	char resplen[16] = {	/* number of bytes in responses */
	3,			/* Enable Single Character Input */
	2,			/* Read USART Status Register */
	2,			/* Read Error Code */
	1,
	1,			/* Single Character Output */
	1,			/* Write USART Command Register */
	1,			/* Disable Single Character Input */
	2,			/* Configuration Command */
	6,			/* Block Input */
	1,
	1,			/* Abort Input */
	1,			/* Suspend Output */
	5,			/* Block Output */
	1,			/* Resume Output */
	1,			/* Abort Output */
	1
};
 
#ifndef PORTSELECTOR
#define	ISPEED	B9600
#else
#define	ISPEED	B4800
#endif
#define	IFLAGS	(CS7|CREAD|PARENB)

int mtiticks = 1;		/* polling frequency */
int mtisoftdtr = 0;		/* if nonzero, softcarrier raises dtr */
int mtidtrlow = 3;		/* hold dtr low almost this long */
int mtib134_weird = 0;		/* if set, old B134 behavior */

/*
 * Should be "void", not "int" - they return no values!
 */
static int	mtiopen(/*queue_t *q, int dev, int flag, int sflag*/);
static int	mticlose(/*queue_t *q, int flag*/);
static int	mtiwput(/*queue_t *q, mblk_t *mp*/);

static struct module_info mtim_info = {
	0,
	"mti",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit mtirinit = {
	putq,
	NULL,
	mtiopen,
	mticlose,
	NULL,
	&mtim_info,
	NULL
};

static struct qinit mtiwinit = {
	mtiwput,
	NULL,
	NULL,
	NULL,
	NULL,
	&mtim_info,
	NULL
};

static char *mtimodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab mtistab = {
	&mtirinit,
	&mtiwinit,
	NULL,
	NULL,
	mtimodlist
};

static int	mtireioctl(/*long unit*/);
static void	mtiioctl(/*struct mtiline *mtp, queue_t *q, mblk_t *mp*/);
static int	dmtomti(/*int bits*/);
static int	mtitodm(/*int bits*/);
static void	mtiparam(/*struct mtiline *mtp*/);
static int	mtirstrt(/*struct mtiline *mtp*/);
static void	mtistart(/*struct mtiline *mtp*/);
static void	bcopy_swab(/*caddr_t pf, caddr_t pt, int n*/);
static int	mtimctl(/*struct mtiline *mtp, int bits, int how*/);
static void	mtiresponse(/*int mti, struct mti_softc *msc*/);
static int	mtidrainring(/*struct mtiline *mtp*/);
static void	mticmd(/*struct mtiline *mtp, ...*/);

extern int	setcons(/*dev_t dev, u_short uid, u_short gid*/);
extern void	resetcons();

#define	BYTESWAP(p)	((caddr_t)((int)(p)^1))

static int junk;			/* global to prevent optimization */

static int
mtiprobe(reg)
	caddr_t reg;
{

	if (peek((short *)reg) < 0)
		return (0);
	return (sizeof (struct mtireg));
}

static int
mtiattach(md)
	register struct mb_device *md;
{
	register struct mtiline *mtp = &mtiline[md->md_unit*16];
	register struct mtireg *mtiaddr = (struct mtireg *)md->md_addr;
	register int line;
	register char *p;

	/*
	 * Allocate 16*(MTIBUFSIZE+1) bytes (MTIBUFSIZE+1 bytes for each
	 * line) in DVMA space for output buffers.  The "+1" is for an
	 * extra byte to hold any flow control character being transmitted.
	 */
	p = (char *)rmalloc(iopbmap, (long)(16*(MTIBUFSIZE+1)));

	for (line = 0; line < 16; line++) {
		mtp->mt_flags = 0;
		mtp->mt_wbits = MTI_ON;
		mtp->mt_buf = p;
		mtp->mt_dtrlow = time.tv_sec - mtidtrlow;
		p += (MTIBUFSIZE+1);
		if (md->md_flags & (1<<line))
			mtp->mt_ttycommon.t_flags |= TS_SOFTCAR;
		mtp++;
	}
	mtiaddr->mtiie = MTI_RA;	/* enable interrupts on resp avail */

	/* 
	 * Scan through the lines again, and fix up the modem control
	 * signals (i.e. lower DTR, unless softcarrier set and mtisoftdtr on).
	 * we build up a fake mt_dev number, since mtimctl needs to know
	 * which board and which line to apply the changes to.
	 */
	mtp = &mtiline[md->md_unit * 16];
	for (line = 0; line < 16; ++line) {
		mtp->mt_dev = md->md_unit * 16 + line; /* temporary only */
		if (mtisoftdtr && (mtp->mt_ttycommon.t_flags & TS_SOFTCAR))
			(void) mtimctl(mtp, MTI_ON, DMSET);	/* raise dtr */
		else
			(void) mtimctl(mtp, MTI_OFF, DMSET);	/* drop dtr */
	}
}

/*ARGSUSED*/
static int
mtiopen(q, dev, flag, sflag)
	register queue_t *q;
	int dev, flag, sflag;
{
	register struct mtiline *mtp;
	register int unit, s;

	unit = UNIT(dev);
	if (unit >= nmti)
		return (OPENFAIL);

	mtp = &mtiline[unit];
	if (mtp->mt_buf == NULL)
		return (OPENFAIL);

	/*
	 * Block waiting for carrier to come up, unless this is a no-delay
	 * open.
	 */
	s = spltty();
again:
	mtp->mt_flags |= MTS_WOPEN;
	if (!(mtp->mt_flags & MTS_ISOPEN)) {
		/* clear any stale input */
		MTI_RING_INIT (mtp);
		mtp->mt_ttycommon.t_iflag = 0;
		mtp->mt_ttycommon.t_cflag = (ISPEED << IBSHIFT)|ISPEED|IFLAGS;
		mtp->mt_ttycommon.t_iocpending = NULL;
		mtp->mt_ttycommon.t_size.ws_row = 0;
		mtp->mt_ttycommon.t_size.ws_col = 0;
		mtp->mt_ttycommon.t_size.ws_xpixel = 0;
		mtp->mt_ttycommon.t_size.ws_ypixel = 0;
		mtp->mt_wbufcid = 0;
		mtp->mt_dev = UNIT(dev);	/* needed for mticmd */
		mtiparam(mtp);
	} else if (mtp->mt_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	} else if ((dev & OUTLINE) && !(mtp->mt_flags & MTS_OUT)) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	(void) mtimctl(mtp, MTI_ON, DMSET);
	if (dev & OUTLINE)
		mtp->mt_flags |= MTS_OUT;
	/*
	 * Check carrier.
	 */
	if (mtp->mt_ttycommon.t_flags & TS_SOFTCAR
	    || mtimctl(mtp, 0, DMGET) & MTI_SR_DSR)
		mtp->mt_flags |= MTS_CARR_ON;
	/*
	 * Unless DTR is held high by softcarrier, set HUPCL.
	 */
	if ((mtp->mt_ttycommon.t_flags & TS_SOFTCAR) == 0)
		mtp->mt_ttycommon.t_cflag |= HUPCL;
	/*
	 * If FNDELAY clear, block until carrier up. Quit on interrupt.
	 */
	if (!(flag & (FNDELAY|FNBIO|FNONBIO))
	    && !(mtp->mt_ttycommon.t_cflag&CLOCAL)) {
		if (!(mtp->mt_flags & (MTS_CARR_ON|MTS_OUT))
		    || (mtp->mt_flags&MTS_OUT && !(dev&OUTLINE))) {
			mtp->mt_flags |= MTS_WOPEN;
			if (sleep((caddr_t)&mtp->mt_flags, STIPRI|PCATCH)) {
				mtp->mt_flags &= ~MTS_WOPEN;
				(void) mtimctl(mtp, MTI_OFF, DMSET);
				u.u_error = EINTR;
				(void) splx(s);
				return (OPENFAIL);
			}
			goto again;
		}
	} else {
		if (mtp->mt_flags&MTS_OUT && !(dev&OUTLINE)) {
			(void) splx(s);
			u.u_error = EBUSY;
			return (OPENFAIL);
		}
	}
	(void) splx(s);

	mtp->mt_ttycommon.t_readq = q;
	mtp->mt_ttycommon.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t)mtp;
	mtp->mt_flags &= ~MTS_WOPEN;
	mtp->mt_flags |= MTS_ISOPEN;
	return (dev);
}
 
/*ARGSUSED*/
static int
mticlose(q, flag)
	register queue_t *q;
	int flag;
{
	register struct mtiline *mtp;
	register int s;
 
	if ((mtp = (struct mtiline *)q->q_ptr) == NULL)
		return;		/* already been closed once */

	if (consdev == mtp->mt_dev)
		resetcons();

	s = spltty();

	/*
	 * wait here until all data is gone or we are interrupted.
	 */
	while ((mtp->mt_flags & MTS_CARR_ON) &&
		((WR(q)->q_count != 0) || (mtp->mt_flags & MTS_BUSY)))
		if (sleep ((caddr_t)&lbolt, STOPRI|PCATCH))
			break;
	if (mtp->mt_flags & MTS_BUSY)
		mticmd(mtp, MTI_ABORTOUT);

	/*
	 * If break is in progress, stop it.
	 */
	(void) mtimctl(mtp, MTI_CR_BREAK, DMBIC);
	mtp->mt_flags &= ~MTS_BREAK;

	/*
	 * If line isn't completely opened, or has HUPCL set,
	 * or has changed the status of TS_SOFTCAR,
	 * fix up the modem lines.
	 */
	if (((mtp->mt_flags & (MTS_WOPEN|MTS_ISOPEN)) != MTS_ISOPEN) ||
	    (mtp->mt_ttycommon.t_cflag & HUPCL) ||
	    (mtp->mt_flags & MTS_SOFTC_ATTN)) {
		/*
		 * If DTR is being held high by softcarrier,
		 * set up the MTI_ON set; if not, hang up.
		 */
		if (mtp->mt_ttycommon.t_flags & TS_SOFTCAR)
			(void) mtimctl(mtp, MTI_ON, DMSET);
		else
			(void) mtimctl(mtp, MTI_OFF, DMSET);
		/*
		 * Don't let an interrupt in the middle of close
		 * bounce us back to the top; just continue
		 * closing as if nothing had happened.
		 */
		if (sleep((caddr_t)&lbolt, STOPRI|PCATCH))
			goto out;
	}

	/*
	 * If nobody's now using it, turn off receiver interrupts.
	 */
	if ((mtp->mt_flags & (MTS_ISOPEN|MTS_WOPEN)) == 0)
		mticmd(mtp, MTI_DSCI);
out:
	/*
	 * Clear out device state.
	 */
	if (mtp->mt_flags & MTS_SUSPD)
		mticmd(mtp, MTI_RESUME);
	mtp->mt_flags = 0;
	ttycommon_close(&mtp->mt_ttycommon);
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (mtp->mt_wbufcid) {
		unbufcall(mtp->mt_wbufcid);
		mtp->mt_wbufcid = 0;
	}
	q->q_ptr = WR(q)->q_ptr = NULL;
	wakeup((caddr_t)&mtp->mt_flags);
	(void) splx(s);
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
mtiwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct mtiline *mtp;
	int s, c;

	mtp = (struct mtiline *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_STOP:
		s = spltty();
		if (!(mtp->mt_flags & MTS_STOPPED)) {
			mtp->mt_flags |= MTS_STOPPED;
			/*
			 * If an output operation is in progress,
			 * suspend it.
			 */
			if (mtp->mt_flags & MTS_BUSY)
				mticmd(mtp, MTI_SUSPEND);
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_START:
		s = spltty();
		if (mtp->mt_flags & MTS_STOPPED) {
			mtp->mt_flags &= ~MTS_STOPPED;
			/*
			 * If an output operation is in progress,
			 * resume it.  Otherwise, prod the start
			 * routine.
			 */
			if (mtp->mt_flags & MTS_BUSY)
				mticmd(mtp, MTI_RESUME);
			else
				mtistart(mtp);
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_IOCTL:
		switch (((struct iocblk *)mp->b_rptr)->ioc_cmd) {

		case TCSETSW:
		case TCSETSF:
		case TCSETAW:
		case TCSETAF:
		case TCSBRK:
			/*
			 * The changes do not take effect until all
			 * output queued before them is drained.
			 * Put this message on the queue, so that
			 * "mtistart" will see it when it's done
			 * with the output before it.  Poke the
			 * start routine, just in case.
			 */
			putq(q, mp);
			mtistart(mtp);
			break;

		default:
			/*
			 * Do it now.
			 */
			mtiioctl(mtp, q, mp);
			break;
		} 
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			s = spltty();
			/*
			 * Abort any output in progress.
			 */
			if (mtp->mt_flags & MTS_BUSY) {
				mticmd(mtp, MTI_ABORTOUT);
				mtp->mt_flags |= MTS_FLUSH;
			}
			/*
			 * Flush our write queue.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			(void) splx(s);
			*mp->b_rptr &= ~FLUSHW;	/* it has been flushed */
		}
		if (*mp->b_rptr & FLUSHR) {
			s = spltty();
			flushq(RD(q), FLUSHDATA);
			(void) splx(s);
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
			freemsg(mp);
		/*
		 * We must make sure we process messages that survive the
		 * write-side flush.  Without this call, the close protocol
		 * with ldterm can hang forever.  (ldterm will have sent us a
		 * TCSBRK ioctl that it expects a response to.)
		 */
		mtistart(mtp);
		break;

	case M_STOPI:
		s = spltty();
		if (c = mtp->mt_ttycommon.t_stopc) {
			mtp->mt_flowc = c;
			/*
			 * Abort any output in progress, so that the stop character
			 * comes out NOW (even if output is frozen).
			 */
			if (mtp->mt_flags & MTS_BUSY)
				mticmd(mtp, MTI_ABORTOUT);
			mtistart(mtp); /* poke the start routine */
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_STARTI:
		s = spltty();
		if (c = mtp->mt_ttycommon.t_startc) {
			mtp->mt_flowc = c;
			/*
			 * Abort any output in progress, so that the stop character
			 * comes out NOW (even if output is frozen).
			 */
			if (mtp->mt_flags & MTS_BUSY)
				mticmd(mtp, MTI_ABORTOUT);
			mtistart(mtp); /* poke the start routine */
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_BREAK:
	case M_DELAY:
	case M_DATA:
		/*
		 * Queue the message up to be transmitted,
		 * and poke the start routine.
		 */
		putq(q, mp);
		mtistart(mtp);
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
mtireioctl(unit)
	long unit;
{
	register struct mtiline *mtp = &mtiline[unit];
	queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	mtp->mt_wbufcid = 0;
	if ((q = mtp->mt_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = mtp->mt_ttycommon.t_iocpending) != NULL) {
		mtp->mt_ttycommon.t_iocpending = NULL;	/* not pending any more */
		mtiioctl(mtp, q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
mtiioctl(mtp, q, mp)
	struct mtiline *mtp;
	queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	register unsigned datasize;
	int error;
	int s;

	iocp = (struct iocblk *)mp->b_rptr;

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
	    ttycommon_ioctl(&mtp->mt_ttycommon, q, mp, &error)) != 0) {
		if (mtp->mt_wbufcid)
			unbufcall(mtp->mt_wbufcid);
		mtp->mt_wbufcid = bufcall(datasize, BPRI_HI, mtireioctl,
		    (long)mtp->mt_dev);
		return;
	}

	if (error == 0) {
		/*
		 * "ttycommon_ioctl" did most of the work; we just use the
		 * data it set up.
		 */
		switch (iocp->ioc_cmd) {

		case TCSETS:
		case TCSETSW:
		case TCSETSF:
		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			mtiparam(mtp);
			break;
		
		case TIOCSSOFTCAR:
			mtp->mt_flags |= MTS_SOFTC_ATTN;
			break;
		}
	} else if (error < 0) {
		/*
		 * "ttycommon_ioctl" didn't do anything; we process it here.
		 */
		error = 0;
		switch (iocp->ioc_cmd) {

		case TIOCCONS:
			error = setcons (mtp->mt_dev, iocp->ioc_uid,
			    iocp->ioc_gid);
			break;

		case TCSBRK: {
			if (*(int *)mp->b_cont->b_rptr == 0) {
				s = spltty();
				/*
				 * Set the break bit, and arrange for
				 * "mtirstrt" to be called in 1/4 second; it
				 * will turn the break bit off, and call
				 * "mtistart" to grab the next message.
				 */
				(void) mtimctl(mtp, MTI_CR_BREAK, DMBIS);
				timeout(mtirstrt, (caddr_t)mtp, hz/4);
				mtp->mt_flags |= MTS_BREAK;
				(void) splx(s);
			}
			break;
		}

		case TIOCSBRK:
			(void) mtimctl(mtp, MTI_CR_BREAK, DMBIS);
			break;

		case TIOCCBRK:
			(void) mtimctl(mtp, MTI_CR_BREAK, DMBIC);
			break;

		case TIOCMSET:
			(void) mtimctl(mtp,
			    dmtomti(*(int *)mp->b_cont->b_rptr), DMSET);
			break;

		case TIOCMBIS:
			(void) mtimctl(mtp,
			    dmtomti(*(int *)mp->b_cont->b_rptr), DMBIS);
			break;

		case TIOCMBIC:
			(void) mtimctl(mtp,
			    dmtomti(*(int *)mp->b_cont->b_rptr), DMBIC);
			break;

		case TIOCMGET:
			*(int *)mp->b_cont->b_rptr =
			    mtitodm(mtimctl(mtp, 0, DMGET));
			break;

		default:
			/*
			 * We don't understand it either.
			 */
			error = ENOTTY;
			break;
		}
	}
	if (error != 0) {
		iocp->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

static int
dmtomti(bits)
	register int bits;
{
	register int b = 0;

	if (bits & TIOCM_DTR)
		b |= MTI_CR_DTR;
	if (bits & TIOCM_CAR)
		b |= MTI_SR_DSR;
	if (bits & TIOCM_RTS)
		b |= MTI_CR_RTS;
	if (bits & TIOCM_DSR)
		b |= MTI_SR_DSR;
	return (b);
}

static int
mtitodm(bits)
	register int bits;
{
	register int b;

	b = 0;
	if (bits & MTI_CR_DTR)
		b |= TIOCM_DTR;
	if (bits & MTI_SR_DSR)
		b |= TIOCM_CAR|TIOCM_DSR;
	if (bits & MTI_CR_RTS)
		b |= TIOCM_RTS;
	return (b);
}
 
/*
 * Set the parameters of the line based on the values of the "c_iflag"
 * and "c_cflag" fields supplied to us.
 */
static void
mtiparam(mtp)
	register struct mtiline *mtp;
{
	register int mr1, mr2, cr;
	register int baudrate;
	int s = spltty();
 
	if ((baudrate = mtp->mt_ttycommon.t_cflag&CBAUD) == 0) {
		(void) mtimctl(mtp, MTI_OFF, DMSET); /* hang up */
		(void) splx(s);
		return;
	}
	mr1 = MTI_MR1_X1_CLK;
	mr2 = mti_speeds[baudrate] | MTI_MR2_INIT;
	if (mtib134_weird && baudrate == B134) {
		/*
		 * XXX - should B134 set all this stuff in the compatibility
		 * module, leaving this stuff fairly clean?
		 */
		mr1 |= MTI_MR1_BITS6|MTI_MR1_PENABLE|MTI_MR1_EPAR|MTI_MR1_1_5STOP;
	} else {
		switch (mtp->mt_ttycommon.t_cflag&CSIZE) {

		case CS5:
			mr1 |= MTI_MR1_BITS5;
			break;

		case CS6:
			mr1 |= MTI_MR1_BITS6;
			break;

		case CS7:
			mr1 |= MTI_MR1_BITS7;
			break;

		case CS8:
			mr1 |= MTI_MR1_BITS8;
			break;
		}

		if (mtp->mt_ttycommon.t_cflag&PARENB) {
			mr1 |= MTI_MR1_PENABLE;
			if (!(mtp->mt_ttycommon.t_cflag&PARODD))
				mr1 |= MTI_MR1_EPAR;
		}
		mr1 |= (mtp->mt_ttycommon.t_cflag&CSTOPB) ? MTI_MR1_2STOP
							  : MTI_MR1_1STOP;
	}
	cr = MTI_CR_TXEN | mtp->mt_wbits;
	if (mtp->mt_ttycommon.t_cflag&CREAD)
		cr |= MTI_CR_RXEN;
	mticmd(mtp, MTI_CONFIG, MTIC_ASYNC, mr1, mr2, cr);
	mticmd(mtp, MTI_ESCI);
	mticmd(mtp, MTI_CONFIG, MTIC_MODEM,
	    (mtp->mt_ttycommon.t_cflag & CLOCAL) ? 0 : 1);
	mticmd(mtp, MTI_CONFIG, MTIC_OUTPUT, 1);
	mticmd(mtp, MTI_RSTAT);
	(void) splx(s);
}

/*
 * Restart output on a line after a delay or break timer expired.
 */
static int
mtirstrt(mtp)
	register struct mtiline *mtp;
{
	/*
	 * If break timer expired, turn off the break bit.
	 */
	if (mtp->mt_flags & MTS_BREAK)
		(void) mtimctl(mtp, MTI_CR_BREAK, DMBIC);
	mtp->mt_flags &= ~(MTS_DELAY|MTS_BREAK);
	mtistart(mtp);
}

/*
 * Start output on a line, unless it's busy, frozen, or otherwise.
 */
static void
mtistart(mtp)
	register struct mtiline *mtp;
{
	register int n, cc;
	register queue_t *q;
	register mblk_t *bp;
	mblk_t *nbp;
	register char *current_position;
	register int bytes_left;
	int s;
 
	s = spltty();

	/*
	 * If the board is busy (i.e., we're waiting for a break timeout
	 * to expire, or for the current transmission to finish), don't
	 * grab anything new.
	 */
	if (mtp->mt_flags & (MTS_BREAK|MTS_BUSY|MTS_FCXMIT))
		goto out;

	/*
	 * If we have a flow-control character to transmit, do it now.
	 * Do so by stuffing the character into the extra byte on the
	 * end of the DVMA-space buffer.  (We can't use MTI_OUT because
	 * that command, amazingly enough, gives no response to indicate
	 * when it's done!)
	 */
	if (mtp->mt_flowc != '\0') {
		current_position = &mtp->mt_buf[MTIBUFSIZE];
		*BYTESWAP(current_position) = mtp->mt_flowc;
		n = current_position - DVMA;
		mticmd(mtp, MTI_BLKOUT, n&0xff, (n>>8)&0xff, (n>>16)&0xff,
		    1, 0);
		mtp->mt_flags |= MTS_FCXMIT;
		goto out;	/* wait for this to finish */
	}

	/*
	 * If we're waiting for a delay timeout to expire, don't grab
	 * anything new.
	 */
	if (mtp->mt_flags & MTS_DELAY)
		goto out;

	/*
	 * If a DMA operation was terminated early, restart it unless
	 * output is stopped.
	 */
	if (mtp->mt_dmacount != 0) {
		if (!(mtp->mt_flags & MTS_STOPPED)) {
			n = mtp->mt_buf - DVMA;	/* address of buffer in DVMA space */
			n += mtp->mt_dmaoffs;	/* advance in Multibus space */
			mticmd(mtp, MTI_BLKOUT, n&0xff, (n>>8)&0xff,
			    (n>>16)&0xff, mtp->mt_dmacount, 0);
			mtp->mt_flags |= MTS_BUSY;
		}
		goto out;
	}

	if ((q = mtp->mt_ttycommon.t_writeq) == NULL)
		goto out;	/* not attached to a stream */

	/*
	 * Set up to copy up to MTIBUFSIZE bytes into our (IOPB-space)
	 * buffer.
	 */
	current_position = &mtp->mt_buf[0];
	bytes_left = MTIBUFSIZE;
	while ((bp = getq(q)) != NULL) {
		/*
		 * We have a new message to work on.
		 * Check whether it's a break, a delay, or an ioctl (the latter
		 * occurs if the ioctl in question was waiting for the output
		 * to drain).  If it's one of those, process it immediately.
		 */
		switch (bp->b_datap->db_type) {

		case M_BREAK:
			if (bytes_left != MTIBUFSIZE) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * Set the break bit, and arrange for "mtirstrt"
			 * to be called in 1/4 second; it will turn the
			 * break bit off, and call "mtistart" to grab
			 * the next message.
			 */
			(void) mtimctl(mtp, MTI_CR_BREAK, DMBIS);
			timeout(mtirstrt, (caddr_t)mtp, hz/4);	/* 0.25 seconds */
			mtp->mt_flags |= MTS_BREAK;
			freemsg(bp);
			goto out;	/* wait for this to finish */

		case M_DELAY:
			if (bytes_left != MTIBUFSIZE) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * Arrange for "mtirstrt" to be called when the
			 * delay expires; it will turn MTS_DELAY off,
			 * and call "mtistart" to grab the next message.
			 */
			timeout(mtirstrt, (caddr_t)mtp,
			    (int)(*(unsigned char *)bp->b_rptr + 6));
			mtp->mt_flags |= MTS_DELAY;
			freemsg(bp);
			goto out;	/* wait for this to finish */

		case M_IOCTL:
			if (bytes_left != MTIBUFSIZE) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * This ioctl was waiting for the output ahead of
			 * it to drain; obviously, it has.  Do it, and
			 * then grab the next message after it.
			 */
			mtiioctl(mtp, q, bp);
			continue;
		}

		/*
		 * We have data to transmit.  If output is stopped, put
		 * it back and try again later.
		 */
		if (mtp->mt_flags & MTS_STOPPED) {
			putbq(q, bp);
			goto out;
		}

		do {
			while ((cc = bp->b_wptr - bp->b_rptr) != 0) {
				if (bytes_left == 0) {
					/*
					 * Out of buffer space; put this
					 * buffer back on the queue, and
					 * transmit what we have.
					 */
					putbq(q, bp);
					goto transmit;
				}
				if (cc > bytes_left) cc = bytes_left;
				/*
				 * Swap bytes as we copy - little-endian
				 * Multibus, big-endian processor.
				 */
				bcopy_swab((caddr_t)bp->b_rptr,
				    (caddr_t)current_position, cc);
				current_position += cc;
				bytes_left -= cc;
				bp->b_rptr += cc;
			}
			nbp = bp;
			bp = bp->b_cont;
			freeb(nbp);
		} while (bp != NULL);
	}

transmit:
	if ((cc = MTIBUFSIZE - bytes_left) != 0) {
		n = mtp->mt_buf - DVMA;		/* address of buffer in DVMA space */
		mticmd(mtp, MTI_BLKOUT, n&0xff, (n>>8)&0xff, (n>>16)&0xff,
		    cc, 0);
		mtp->mt_dmaoffs = 0;	/* starting at beginning */
		mtp->mt_dmacount = cc;	/* transmitting this many characters */
		mtp->mt_flags |= MTS_BUSY;
	}

out:
	(void) splx(s);
}

/*
 * Copy bytes and swap them.
 */
static void
bcopy_swab(pf, pt, n)
	register caddr_t pf, pt;
	register int n;
{
	while (n-- > 0)
		*BYTESWAP(pt++) = *pf++;
}


/*
 * Set or get the modem control status.
 * WARNING: may sleep if trying to raise DTR.
 */
static int
mtimctl(mtp, bits, how)
	register struct mtiline *mtp;
	int bits, how;
{
	register int mbits, obits, cr, s, now, held;

 again:
	s = spltty();
	mbits = mtp->mt_wbits | mtp->mt_rbits;
	obits = mbits;
	switch (how) {

	case DMSET:
		mbits = bits;
		break;

	case DMBIS:
		mbits |= bits;
		break;

	case DMBIC:
		mbits &= ~bits;
		break;

	case DMGET:
		(void) splx(s);
		return (mbits);
	}

	now = time.tv_sec;
	held = now - mtp->mt_dtrlow;
/* if DTR is going low, stash current time away */
	if (~mbits & obits & MTI_CR_DTR)
		mtp->mt_dtrlow = now;
/* if DTR is going high, sleep until it has been low a bit */
	if ((mbits & ~obits & MTI_CR_DTR) && (held < mtidtrlow)) {
		(void) splx(s);
		(void) sleep((caddr_t)&lbolt, PZERO-1);
		goto again;
	}

	mtp->mt_wbits = mbits&(MTI_CR_DTR|MTI_CR_RTS|MTI_CR_BREAK);
	cr = MTI_CR_TXEN | mtp->mt_wbits;
	if (mtp->mt_ttycommon.t_cflag&CREAD)
		cr |= MTI_CR_RXEN;
	mticmd(mtp, MTI_WCMD, cr);
	(void) splx(s);
	return (mbits);
}

/*
 * Interrupt service routine.
 * Poll all the devices marked as "alive".
 */
int
mtiintr()
{
	register int mti;
	register struct mb_device *mtip;
	register struct mtireg *mtiaddr;
	register struct mti_softc *msc;
	int serviced = 0;
 
	for (mti = 0; mti < nmti>>4; mti++) {
		if ((mtip = mtiinfo[mti]) == NULL)
			continue;	/* it wasn't found by autoconfig */
		mtiaddr = (struct mtireg *)mtip->md_addr;
		msc = &mti_softc[mti];
		/*
		 * If command FIFO is ready, see if we have any
		 * queued commands to send out.
		 */
		if (mtiaddr->mtistat&MTI_READY) {
			register struct clist *q;
			register int n;

			q = &msc->msc_cmdq;
			if ((n = getc(q)) > 0) {
				while (n--)
					mtiaddr->mticmd = getc(q);
				mtiaddr->mtigo = 1;
				serviced++;
				if (q->c_cc == 0)
					mtiaddr->mtiie = MTI_RA;
			} else
				mtiaddr->mtiie = MTI_RA;
		}
		if (mtiaddr->mtistat&MTI_RA) {
			register int have;
			register u_char *p, *p0;

			serviced++;
			/*
			 * Determine if we have an entire response yet.
			 * If not, keep accumulating response bytes.
			 */
			have = msc->msc_have;
			p0 = msc->msc_rbuf;
			p = p0 + have;
			while (mtiaddr->mtistat & MTI_VD) {
				*p++ = mtiaddr->mtiresp;
				if (++have >= resplen[*p0>>4]) {
					mtiresponse(mti, msc);
					junk = mtiaddr->mticra;
					have = 0;
					p = p0;
				}
			}
			msc->msc_have = have;
		}
	}
	return (serviced);
}

/*
 * Process response to a command.
 */
static void
mtiresponse(mti, msc)
	int mti;
	register struct mti_softc *msc;
{
	register struct mtiline *mtp;
	register u_char *p = msc->msc_rbuf;
	register int cmd, s, c, svpri, line;
	int overrun = 0;
	int ringfull = 0;
	register queue_t *q;

	cmd = *p++;
	line = cmd & 15;
	mtp = &mtiline[(mti<<4)+line];
	cmd &= 0xf0;

	/*
	 * First, read and handle the UART status
	 * byte for commands that return it.
	 */
	switch (cmd) {

	case MTI_ESCI:
	case MTI_RSTAT:
	case MTI_CONFIG:
	case MTI_BLKOUT:
		s = *p++;
		if ((s & MTI_SR_DSR) ||
		    (mtp->mt_ttycommon.t_flags & TS_SOFTCAR)) {
			/* carrier present */
			if ((mtp->mt_flags & MTS_CARR_ON) == 0) {
				mtp->mt_flags |= MTS_CARR_ON;
				if ((q = mtp->mt_ttycommon.t_readq) != NULL)
					(void) putctl(q->q_next, M_UNHANGUP);
				wakeup((caddr_t)&mtp->mt_flags);
			}
		} else {
			if ((mtp->mt_flags&MTS_CARR_ON) &&
			    !(mtp->mt_ttycommon.t_cflag&CLOCAL)) {
				/*
				 * Carrier went away.
				 * Drop DTR, abort any output in progress,
				 * indicate that output is not stopped, and
				 * send a hangup notification upstream.
				 */
				(void) mtimctl(mtp, MTI_CR_DTR, DMBIC);
				svpri = spltty();
				if (mtp->mt_flags & MTS_BUSY) {
					mticmd(mtp, MTI_ABORTOUT);
					mtp->mt_flags |= MTS_FLUSH;
				}
				mtp->mt_flags &= ~MTS_STOPPED;
				(void) splx(svpri);
				if ((q = mtp->mt_ttycommon.t_readq) != NULL)
					(void) putctl(q->q_next, M_HANGUP);
			}
			mtp->mt_flags &= ~MTS_CARR_ON;
		}
		mtp->mt_rbits = s&(MTI_SR_DCD|MTI_SR_DSR);
		break;
	}

	/*
	 * Finish processing the command.
	 */
	switch (cmd) {

	case MTI_ESCI:
		c = *p++;
		if ((mtp->mt_flags & MTS_ISOPEN) == 0) {
#if 0
			/*
			 * What does this do?
			 */
			wakeup((caddr_t)&tp->t_rawq);
#endif
#ifdef PORTSELECTOR
			if ((mtp->mt_flags&MTS_WOPEN) == 0)
#endif
			break;
		}
		if ((q = mtp->mt_ttycommon.t_readq) == NULL)
			break;	/* nobody listening */
		if (s&MTI_SR_DO)
			++overrun;
		if (s&MTI_SR_FE) {
			if (c == 0) {
				(void) putctl(q->q_next, M_BREAK);
				break;	/* throw NUL away */
			} else
				s |= MTI_SR_PE;	/* treat it like parity error */
		}
		if (s&MTI_SR_PE && (mtp->mt_ttycommon.t_iflag & INPCK)) {
			/*
			 * IGNPAR       PARMRK  RESULT
			 *    off          off       0
			 *    off           on       3 byte sequence
			 *     on       either	     ignored
			 */
			if (!(mtp->mt_ttycommon.t_iflag & IGNPAR)) {
				/*
				 * The receive interrupt routine has already
				 * stuffed c into the ring.  Dig it out again,
				 * since the current mode settings don't allow
				 * it to appear in that position (it needs the
				 * 0377, 0 stuff first).
				 */
				if (MTI_RING_CNT(mtp) != 0)
					MTI_RING_UNPUT(mtp);
				else
					log(LOG_ERR,
					    "mti%d%x: parity error ignored\n",
					    mti, line);
				if (mtp->mt_ttycommon.t_iflag & PARMRK) {
					if (MTI_RING_POK(mtp, 3)) {
						MTI_RING_PUT(mtp, 0377);
						MTI_RING_PUT(mtp, 0);
						MTI_RING_PUT(mtp, c);
					} else
						++ringfull;
				} else {
					if (MTI_RING_POK(mtp, 1))
						MTI_RING_PUT(mtp, 0);
					else
						++ringfull;
				}
			} else {
				if (MTI_RING_CNT(mtp) != 0)
					MTI_RING_UNPUT(mtp);
				else
					log(LOG_ERR, 
					    "mti%d%x: parity error went up\n",
					    mti, line);
			}
		} else {
			if (MTI_RING_POK(mtp, 1)) {
				if (c == 0377 &&
				    ((mtp->mt_ttycommon.t_iflag &
				      (PARMRK | IGNPAR | ISTRIP)) == PARMRK)) {
					if (MTI_RING_POK(mtp, 2)) {
						MTI_RING_PUT(mtp, 0377);
						MTI_RING_PUT(mtp, c);
					} else
						++ringfull;
				} else
					MTI_RING_PUT(mtp, c);
			} else
				++ringfull;
		}

		/*
		 * If a "stop input" character arrived, or the silo is at
		 * least half full, drain it.
		 */
		if (MTI_RING_FRAC(mtp) || ((c & 0177) == mtp->mt_ttycommon.t_stopc))
			mtidrainring(mtp);
		else {
			if (!(mtp->mt_flags & MTS_DRAINPEND)) {
				mtp->mt_flags |= MTS_DRAINPEND;
				timeout(mtidrainring, (caddr_t) mtp, mtiticks);
			}
		}
		break;

	case MTI_RERR:
		c = *p++;
		log(LOG_ERR, "mti%d%x: read error code <%x>. Probable hardware fault\n",
		    mti, line, c);
		break;

	case MTI_BLKOUT:
		s = *p++;		/* get termination cause */
		c = *p++;	/* get number of characters transferred */
		c |= *p++ << 8;
		if (mtp->mt_flags & MTS_FCXMIT) {
			if (s & 4)
				log(LOG_ERR, "mti%d%x: DMA error after %d FLOW bytes (at %x)\n",
				    mti, line, c, mtp->mt_buf + MTIBUFSIZE + c);
			mtp->mt_flags &= ~MTS_FCXMIT;
			if (c > 0)
				mtp->mt_flowc = '\0';
		} else {
			if (s & 4)
				log(LOG_ERR, "mti%d%x: DMA output error after %d of %d bytes (at %x)\n",
				    mti, line, c, mtp->mt_dmacount,
				    mtp->mt_buf + mtp->mt_dmaoffs + c);
			if (mtp->mt_flags & MTS_FLUSH) {
				/*
				 * Discard untransmitted output.
				 */
				mtp->mt_dmacount = 0;
			} else {
				mtp->mt_dmacount -= c;
				mtp->mt_dmaoffs += c;
			}
			mtp->mt_flags &= ~(MTS_BUSY|MTS_FLUSH);
		}
		mtistart(mtp);
		break;

	case MTI_RSTAT:
	case MTI_CONFIG:
		break;

	case MTI_BLKIN:
	default:
		log(LOG_ERR, "mti%d%x: impossible response %x\n", mti, line,
		    *msc->msc_rbuf);
	}
	if (overrun)
		log(LOG_WARNING, "mti%d%x: silo overflow\n", mti, line);
	if (ringfull)
		log(LOG_WARNING, "mti%d%x: ring overflow\n", mti, line);
}

/*
 * Drain the ring for an MTI port.
 */
static int
mtidrainring(mtp)
	register struct mtiline *mtp;
{
	register mblk_t *bp;
	register int cc;
	register queue_t *q;
	int s;

	s = splclock();
	untimeout(mtidrainring, (caddr_t) mtp);
	mtp->mt_flags &= ~MTS_DRAINPEND;
	(void) spltty();
	if ((q = mtp->mt_ttycommon.t_readq) == NULL) {
		MTI_RING_INIT (mtp);
		(void) splx(s);
		return;
	}

	if ((cc = MTI_RING_CNT (mtp)) > 0)
		do {
			if (cc > 16)
				cc = 16;
			if (((bp = allocb(cc, BPRI_MED)) == NULL) ||
			    !canput (q->q_next)) {
				freemsg(bp);
				MTI_RING_INIT(mtp);
				ttycommon_qfull(&mtp->mt_ttycommon, q);
				break;
			}
			do {
				*(bp->b_wptr++) = MTI_RING_GET(mtp);
			} while (--cc > 0);
			putnext(q, bp);
		} while ((cc = MTI_RING_CNT (mtp)) > 15);

	if (MTI_RING_CNT(mtp) != 0) {
		if (!(mtp->mt_flags & MTS_DRAINPEND)) {
			mtp->mt_flags |= MTS_DRAINPEND;
			timeout(mtidrainring, (caddr_t) mtp, mtiticks);
		}
	}
	(void) splx(s);
}

int	mticmdwait = 30;	/* 30 determined empiricly */

/*
 * Send a command to the device.
 */
/*VARARGS1*/
/*ARGSUSED*/
static void
mticmd(mtp, va_alist)
	struct mtiline *mtp;
	va_dcl
{
	int mti = BOARD(mtp->mt_dev);
	register struct mtireg *mtiaddr =
	    (struct mtireg *)mtiinfo[mti]->md_addr;
	register int n, useq = 0;
	register struct clist *q = &mti_softc[mti].msc_cmdq;
	register int cmd, a1;
	va_list p;
	register int s;

top:
	s = spltty();
	n = 0;
	if (q->c_cc)
		useq = 1;
	else
		while ((mtiaddr->mtistat & MTI_READY) == 0)
			if (++n > mticmdwait) {
				mtiaddr->mtiie = MTI_RA|MTI_READY;
				if (mtiaddr->mtistat & MTI_READY)
					mtiaddr->mtiie = MTI_RA;
				else
					useq = 1;
				break;
			} else
				(void) splx(splx(s));

	if (!useq && mtiaddr->mtistat & MTI_ERR) {
		mtiaddr->mticmd = MTI_RERR;
		mtiaddr->mtigo = 1;
		(void) splx(s);
		goto top;
	}
	va_start(p);
	cmd = va_arg(p, int);
	a1 = va_arg(p, int);
	if (cmd == MTI_SUSPEND)
		mtp->mt_flags |= MTS_SUSPD;
	if (cmd == MTI_RESUME)
		mtp->mt_flags &= ~MTS_SUSPD;
	if (cmd == MTI_CONFIG)
		n = configlen[a1>>4];
	else
		n = cmdlen[cmd>>4];
	cmd |= LINE(mtp->mt_dev);
	if (useq) {
		(void) putc(n, q);
		(void) putc(cmd, q);
		if (--n) {
			(void) putc(a1, q);
			while (--n)
				(void) putc(va_arg (p, int), q);
		}
	} else {
		mtiaddr->mticmd = cmd;
		if (--n) {
			mtiaddr->mticmd = a1;
			while (--n)
				mtiaddr->mticmd = va_arg(p, int);
		}
		mtiaddr->mtigo = 1;
	}
	va_end(p);
	(void) splx(s);
}
 
/*
 * Reset state of driver if Multibus reset was necessary.
 * Reset parameters and restart transmission on open lines.
 */
int
mtireset()
{
	register int unit;
	register struct mtiline *mtp;
	register struct mb_device *md;

	for (unit = 0; unit < nmti; unit++) {
		md = mtiinfo[BOARD(unit)];
		if (md == 0 || md->md_alive == 0)
			continue;
		if (LINE(unit) == 0)
			printf(" mti%d", BOARD(unit));
		mtp = &mtiline[unit];
		if (mtp->mt_flags & (MTS_ISOPEN|MTS_WOPEN)) {
			mtiparam(mtp);
			(void) mtimctl(mtp, 0, DMBIS);
			mtp->mt_flags &= ~(MTS_DELAY|MTS_BREAK|MTS_BUSY|MTS_FCXMIT|MTS_DRAINING);
			mtistart(mtp);
		}
	}
}
