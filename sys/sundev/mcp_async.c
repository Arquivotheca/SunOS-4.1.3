#ifndef lint
static char     sccsid[] = "@(#)mcp_async.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

/*
 * Copyright (c) 1988, 1989 by Sun Microsystems, Inc.
 */

/*
 * Sun MCP Multiprotocol Communication Processor
 * Sun ALM-2 Asynchronous Line Multiplexer
 *
 * Asynchronous protocol handler for Z8530 SCCs
 * Handles normal UNIX support for terminals & modems
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
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/buf.h>
#include <sys/kernel.h>
#include <sys/syslog.h>

#include <sun/consdev.h>
#include <sundev/mbvar.h>
#include <sundev/zsreg.h>
#include <sundev/dmctl.h>
#include <sundev/mcpreg.h>
#include <sundev/mcpcom.h>
#include <sundev/mcpcmd.h>

/*
 * Share the same definitions as used in zs_async.c
 *
 * set SCC transmit and receive interrupt mode
 */
#define	ZSWR1_INIT	(ZSWR1_SIE | ZSWR1_REQ_ENABLE | ZSWR1_REQ_NOT_WAIT |\
				 ZSWR1_REQ_IS_RX | ZSWR1_RIE_SPECIAL_ONLY)

#define	ZS_ON		(ZSWR5_DTR | ZSWR5_RTS)
#define	ZS_OFF		0

#define	OUTLINE		0x80	/* minor device flag for dialin/out on same
				/* line */
#define UNIT(x)		(minor(x) & ~(OUTLINE))

extern struct mcpcom mcpcom[];
extern struct mcpaline mcpaline[];
extern u_short  mcp_speeds[];
extern int      mcpsoftCAR[];
extern int      nmcp, nmcpline;

#define I_IFLAGS	0
#define I_CFLAGS	((B9600 << IBSHIFT) | B9600 | CS8 | CREAD | HUPCL)

#define PCLK		(19660800/4)	/* basic clock rate for UARTs */
#define	ZSPEED(n)	ZSTimeConst(PCLK, n)
#define NSPEED		16		/* max # of speeds */
u_short		mcp_speeds[NSPEED] = {
	0,		ZSPEED(50),	ZSPEED(75),	ZSPEED(110),
	ZSPEED(134),	ZSPEED(150),	ZSPEED(200),	ZSPEED(300),
	ZSPEED(600),	ZSPEED(1200),	ZSPEED(1800),	ZSPEED(2400),
	ZSPEED(4800),	ZSPEED(9600),	ZSPEED(19200),	ZSPEED(38400),
};

extern struct dma_chan *mcp_dmagetchan();
extern void	mcp_dmastart();
extern u_short	mcp_getwc(), mcp_dmahalt();

static int	mcpa_attach( /* struct mcpcom *zs */ );
static int	mcpopen( /* queue_t *q, dev_t dev, int flag, int sflag */ );
static int	mcppopen( /* queue_t *q, dev_t dev, int flag, int sflag */ );
static int	mcpclose( /* queue_t *q, int flag */ );
static int	mcpwput( /* queue_t *q, mblk_t *mp */ );

static void	mcpioctl( /* struct mcpaline *za, queue_t *q, mblk_t *mp */ );
static int	mcpreioctl( /* long unit */ );
static void	mcpparam( /* struct mcpaline *za */ );
static int	dmtomcp( /* int bits */ );
static int	mcptodm( /* int bits */ );
static int	mcpmctl( /* struct mcpaline *za, int bits, int how */ );

static void	mcpstart( /* struct mcpaline *za */ );
static int	mcprstrt( /* struct mcpaline *za */ );

static int	mcpa_txint( /* struct mcpcom *zs */ );
static int	mcpa_xsint( /* struct mcpcom *zs */ );
static int	mcpa_rxint( /* struct mcpcom *zs */ );
static int	mcpa_srint( /* struct mcpcom *zs */ );
static int	mcpa_txend( /* struct mcpcom *zs */ );
static int	mcpa_rxchar( /* struct mcpcom *zs, int c */ );
static int	mcpa_drain( /* struct mcpaline *za */ );
static int	mcpa_rxend( /* struct mcpcom *zs */ );

int		mcpr_attach( /* struct mb_device *md */ );
static int	mcpropen( /* queue_t *q, dev_t dev, int flag, int sflag */ );
static void	mcprcntl( /* struct mcpaline *za, int cmd, int *datap */ );
static int	mcpr_pe( /* struct mcpcom *zs */ );
static int	mcpr_slct( /* struct mcpcom *zs */ );

extern int	setcons(/*dev_t dev, u_short uid, u_short gid*/);
extern void	resetcons();

int	mcpb134_weird = 0;	/* if set, use old B134 behavior */
int	mcpr_major = -1;
#define	isprinter(dev)	(major(dev) == mcpr_major)

static struct module_info mcpm_info = {
	0,
	"mcp",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit mcprinit = {
	putq,
	NULL,
	mcpopen,
	mcpclose,
	NULL,
	&mcpm_info,
	NULL
};

static struct qinit mcpprinit = {
	putq,
	NULL,
	mcppopen,
	mcpclose,
	NULL,
	&mcpm_info,
	NULL
};

static struct qinit mcpwinit = {
	mcpwput,
	NULL,
	NULL,
	NULL,
	NULL,
	&mcpm_info,
	NULL
};

static char    *mcpmodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab mcpstab = {
	&mcprinit,
	&mcpwinit,
	NULL,
	NULL,
	mcpmodlist
};

struct streamtab mcppstab = {
	&mcpprinit,
	&mcpwinit,
	NULL,
	NULL,
	mcpmodlist
};

struct mcpops   mcpops_async = {
	mcpa_attach,
	mcpa_txint,
	mcpa_xsint,
	mcpa_rxint,
	mcpa_srint,
	mcpa_txend,
	mcpa_rxend,
	mcpa_rxchar,
};

struct mcpops   mcpops_print = {
	mcpr_attach,
	0,
	0,
	0,
	0,
	mcpa_txend,
	0,
	0,
	0,
	mcpr_pe,
	mcpr_slct,
};

int mcpaticks = 1;		/* Polling frequency on input silos */
int mcpasoftdtr = 0;		/* softcarrier raises dtr at attach */
int mcpadtrlow = 3;		/* hold dtr low this long */

static int
mcpa_attach(zs)
	register struct mcpcom *zs;
{
	register int    ct = zs->mc_unit * 16 + zs->zs_unit;
	register struct mcpaline *za = &mcpaline[ct];
	register struct mcp_device *mc = zs->mcp_addr;

	za->za_common = zs;
	za->za_dmabuf = mc->ram.asyncbuf[zs->zs_unit];
	za->za_xoff = &(mc->xbuf[zs->zs_unit].xoff);
	za->za_devctl = &(mc->devctl[zs->zs_unit].ctr);
	MCP_RING_INIT(za);
	za->za_dtrlow = time.tv_sec - mcpadtrlow;
	if (mcpsoftCAR[zs->mc_unit] & (1 << zs->zs_unit))
		za->za_ttycommon.t_flags |= TS_SOFTCAR;
	if (mcpasoftdtr && (za->za_ttycommon.t_flags & TS_SOFTCAR))
		(void) mcpmctl(za, ZS_ON, DMSET);	/* raise dtr */
	else
		(void) mcpmctl(za, ZS_OFF, DMSET);	/* drop dtr */
}

/*
 * Printer entry point for open, set mcpr_major so
 * isprinter macro will work and just call regular open
 * routine.
 */

static int
mcppopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t		dev;
	int		flag, sflag;
{
	mcpr_major = major(dev);
	return (mcpopen(q, dev, flag, sflag));
}

/* ARGSUSED */
static int
mcpopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t		dev;
	int		flag, sflag;
{
	register struct mcpcom *zs;
	register struct mcpaline *za;
	register int    unit;
	int		s;

	if (isprinter(dev))
		return (mcpropen(q, dev, flag, sflag));
	unit = UNIT(dev);
	if (unit >= nmcpline)
		return (OPENFAIL);

	za = &mcpaline[unit];
	zs = &mcpcom[unit];
	/*
	 * Sanity check: za_common and zs should agree. If they don't either
	 * something is broken (sinister theory), or we're trying to open a
	 * port that never got attached (innocent theory).
	 */
	if (za->za_common != zs)
		return (OPENFAIL);

	s = spltty();
	if (zs->mcp_ops == NULL)
		zs->mcp_ops = &mcpops_async;
	if (zs->mcp_ops != &mcpops_async) {
		(void) splx(s);
		u.u_error = EPROTO;
		return (OPENFAIL);
	}
	zs->zs_priv = (caddr_t) za;
	za->za_dev = dev;

again:
	if ((za->za_flags & ZAS_ISOPEN) == 0) {
		za->za_ttycommon.t_iflag = I_IFLAGS;
		za->za_ttycommon.t_cflag = I_CFLAGS;
		za->za_ttycommon.t_iocpending = NULL;
		za->za_ttycommon.t_size.ws_row = 0;
		za->za_ttycommon.t_size.ws_col = 0;
		za->za_ttycommon.t_size.ws_xpixel = 0;
		za->za_ttycommon.t_size.ws_ypixel = 0;
		za->za_wbufcid = 0;
		mcpparam(za);
	} else if (za->za_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	} else if ((dev & OUTLINE) && !(za->za_flags & ZAS_OUT)) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	(void) mcpmctl(za, ZS_ON, DMSET);
	if (dev & OUTLINE)
		za->za_flags |= ZAS_OUT;
	/*
	 * Check carrier.
	 */
	if ((za->za_ttycommon.t_flags & TS_SOFTCAR)
	    || (mcpmctl(za, 0, DMGET) & ZSRR0_CD))
		za->za_flags |= ZAS_CARR_ON;
	/*
	 * Unless DTR is held high by softcarrier, set HUPCL.
	 */
	if ((za->za_ttycommon.t_flags & TS_SOFTCAR) == 0)
		za->za_ttycommon.t_cflag |= HUPCL;
	/*
	 * If FNDELAY clear, block until carrier up. Quit on interrupt.
	 */
	if (!(flag & (FNBIO|FNONBIO|FNDELAY))
	    && !(za->za_ttycommon.t_cflag & CLOCAL)) {
		if (!(za->za_flags & (ZAS_CARR_ON|ZAS_OUT))
		    || (za->za_flags & ZAS_OUT) && !(dev & OUTLINE)) {
			za->za_flags |= ZAS_WOPEN;
			if (sleep((caddr_t) &za->za_flags, STIPRI|PCATCH)) {
				za->za_flags &= ~ZAS_WOPEN;
				(void) mcpmctl(za, ZS_OFF, DMSET);
				u.u_error = EINTR;
				(void) splx(s);
				return (OPENFAIL);
			}
			goto again;
		}
	} else {
		if (za->za_flags & ZAS_OUT && !(dev & OUTLINE)) {
			u.u_error = EBUSY;
			(void) splx(s);
			return (OPENFAIL);
		}
	}

	za->za_ttycommon.t_readq = q;
	za->za_ttycommon.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t) za;
	za->za_flags &= ~ZAS_WOPEN;
	za->za_flags |= ZAS_ISOPEN;
	(void) splx(s);
	return (dev);
}

/*ARGSUSED*/
static int
mcpclose(q, flag)
	register queue_t *q;
	int flag;
{
	register struct mcpcom *zs;
	register struct mcpaline *za;
	int s;

	/*
	 * Did we already close this once?
	 */
	s = spltty();
	if ((za = (struct mcpaline *) q->q_ptr) == NULL) {
		(void) splx(s);
		return;
	}

	if (consdev == za->za_dev)
		resetcons();
	zs = za->za_common;

	/*
	 * if we still have carrier,
	 * wait here until all data is gone or we are interrupted
	 */
	while ((za->za_flags & ZAS_CARR_ON) &&
	    ((WR(q)->q_count != 0) || (za->za_flags & ZAS_BUSY)))
		if (sleep ((caddr_t)&lbolt, STOPRI|PCATCH))
			break;
	if (za->za_flags & ZAS_BUSY)
		(void) mcp_dmahalt(zs->mcp_txdma);

	if (!isprinter(za->za_dev)) {
		/*
		 * If break is in progress, stop it.
		 */
		if (zs->zs_wreg[5] & ZSWR5_BREAK)
			SCCBIC(zs->zs_addr, 5, ZSWR5_BREAK);
		za->za_flags &= ~ZAS_BREAK;

		/*
		 * If line isn't completely opened, or has HUPCL set, 
		 * or has changed the status of TS_SOFTCAR,
		 * fix up the modem lines.
		 */
		if (((za->za_flags & (ZAS_WOPEN|ZAS_ISOPEN)) != ZAS_ISOPEN) ||
		    (za->za_ttycommon.t_cflag & HUPCL) ||
		    (za->za_flags & ZAS_SOFTC_ATTN)) {
			/*
			 * If DTR is being held high by softcarrier, 
			 * set up the ZS_ON set; if not, hang up.
			 */
			if (za->za_ttycommon.t_flags & TS_SOFTCAR)
				(void) mcpmctl(za, ZS_ON, DMSET);
			else
				(void) mcpmctl(za, ZS_OFF, DMSET);
			/*
			 * Don't let an interrupt in the middle of close
			 * bounce us back to the top; just continue closing
			 * as if nothing had happened.
			 */
			if (sleep((caddr_t) &lbolt, STOPRI | PCATCH))
				goto out;
		}
	}
out:
	/*
	 * Clear out device state.
	 */
	za->za_flags = 0;
	ttycommon_close(&za->za_ttycommon);
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (za->za_wbufcid) {
		unbufcall(za->za_wbufcid);
		za->za_wbufcid = 0;
	}
	MCP_RING_INIT(za);
	wakeup((caddr_t) &za->za_flags);
	q->q_ptr = WR(q)->q_ptr = NULL;
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
mcpwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct mcpaline *za = (struct mcpaline *) q->q_ptr;
	register struct mcpcom *zs = za->za_common;
	int		s;

	switch (mp->b_datap->db_type) {

	case M_STOP:
		s = spltty();
		if (!(za->za_flags & ZAS_STOPPED)) {
			za->za_flags |= ZAS_STOPPED;
			if (za->za_flags & ZAS_BUSY) {
				/*
				 * If serial output is in progress, stop it.
				 * There's not much we can do about ongoing
				 * printer DMA.
				 */
				if (!isprinter(za->za_dev)) {
					*za->za_devctl &= ~TXENABLE;
					za->za_flags |= ZAS_PAUSED;
				}
			}
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_START:
		s = spltty();
		/*
		 * If stopped, reset flags and prod the start routine.
		 * Simplicity itself.
		 */
		if (za->za_flags & ZAS_STOPPED) {
			za->za_flags &= ~ZAS_STOPPED;
			mcpstart(za);
		}
		(void) splx(s);
		freemsg(mp);
		break;

	case M_IOCTL:
		switch (((struct iocblk *) mp->b_rptr)->ioc_cmd) {

		case TCSETSW:
		case TCSETSF:
		case TCSETAW:
		case TCSETAF:
		case TCSBRK:
			/*
			 * The changes do not take effect until all output
			 * queued before them is drained. Put this message on
			 * the queue, so that "mcpstart" will see it when
			 * it's done with the output before it. Poke the
			 * start routine, just in case.
			 */
			putq(q, mp);
			mcpstart(za);
			break;

		default:
			/*
			 * Do it now.
			 */
			mcpioctl(za, q, mp);
			break;
		}
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			s = spltty();
			/*
			 * Abort any output in progress.
			 */
			if (za->za_flags & ZAS_BUSY) {
				(void) mcp_dmahalt(zs->mcp_txdma);
				za->za_flags &= ~ZAS_BUSY;
			}
			/*
			 * Flush our write queue.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			(void) splx(s);
			*mp->b_rptr &= ~FLUSHW;	/* It has been flushed. */
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
		mcpstart(za);
		break;

	case M_BREAK:
	case M_DELAY:
	case M_DATA:
		/*
		 * Queue the message up to be transmitted, and poke the start
		 * routine.
		 */
		putq(q, mp);
		mcpstart(za);
		break;

	case M_STOPI:
		s = spltty();
		za->za_flowc = za->za_ttycommon.t_stopc;
		mcpstart(za);
		(void) splx(s);
		freemsg(mp);
		break;

	case M_STARTI:
		s = spltty();
		za->za_flowc = za->za_ttycommon.t_startc;
		mcpstart(za);
		(void) splx(s);
		freemsg(mp);
		break;

	default:
		/*
		 * *boo wee weeep* "We're sorry, you have reached a number
		 * that has been disconnected or is no longer in service. If
		 * you feel you have reached this recording in error, please
		 * check the number and try your call again."
		 */
		freemsg(mp);
		break;
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
mcpioctl(za, q, mp)
	struct mcpaline *za;
	register queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	register unsigned datasize;
	register struct mcpcom *zs = za->za_common;
	register struct termios *cb;
	int		error;
	int		s;

	if (za->za_ttycommon.t_iocpending != NULL) {
		/*
		 * We were holding an "ioctl" response pending the
		 * availability of an "mblk" to hold data to be passed up;
		 * another "ioctl" came through, which means that "ioctl"
		 * must have timed out or been aborted.
		 */
		freemsg(za->za_ttycommon.t_iocpending);
		za->za_ttycommon.t_iocpending = NULL;
	}

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
	    ttycommon_ioctl(&za->za_ttycommon, q, mp, &error)) != 0) {
		if (za->za_wbufcid)
			unbufcall(za->za_wbufcid);
		za->za_wbufcid = bufcall(datasize, BPRI_HI, mcpreioctl,
		    (long)zs->zs_unit);
		return;
	}
	if (error == 0) {
		/*
		 * "ttycommon_ioctl" did most of the work; we just use the
		 * data it set up. Two exceptions:
		 * (1) if it's any kind of SET ioctl, we also grab the
		 * VLNEXT char for use when we stash the input
		 * characters. See mcpa_rxchar() below.
		 * (2) if it affects SOFTCAR, set the ATTN flag to force
		 * a reset of the modem line status on close.
		 */
		switch (iocp->ioc_cmd) {

		/*
		 * Only the termios-related ioctls define the VLNEXT
		 * character, so we can only reset that character's value from
		 * those TC* ioctls.
		 */
		case TCSETS:
		case TCSETSW:
		case TCSETSF:
			cb = (struct termios *) mp->b_cont->b_rptr;
			za->za_lnext = cb->c_cc[VLNEXT];
			/* FALL THROUGH */

		case TCSETA:
		case TCSETAW:
		case TCSETAF:
			mcpparam(za);
			break;
		
		case TIOCSSOFTCAR:
			za->za_flags |= ZAS_SOFTC_ATTN;
			break;

		}
	} else if (error < 0) {
		/*
		 * "ttycommon_ioctl" didn't do anything; we process it here.
		 */
		error = 0;
		switch (iocp->ioc_cmd) {

		case TIOCCONS:
			error = setcons (za->za_dev, iocp->ioc_uid,
			    iocp->ioc_gid);
			break;

		case TCSBRK:
			if (*(int *)mp->b_cont->b_rptr == 0) {
				s = spltty();
				/*
				 * Set the break bit, and arrange for
				 * "mcprstrt" to be called in 1/4 second; it
				 * will turn the break bit off, and call
				 * "mcpstart" to grab the next message.
				 */
				SCCBIS(zs->zs_addr, 5, ZSWR5_BREAK);
				timeout(mcprstrt, (caddr_t) za, hz / 4);
				za->za_flags |= ZAS_BREAK;
				(void) splx(s);
			}
			break;

		case TIOCSBRK:
			s = spltty();
			SCCBIS(zs->zs_addr, 5, ZSWR5_BREAK);
			(void) splx(s);
			break;

		case TIOCCBRK:
			s = spltty();
			SCCBIC(zs->zs_addr, 5, ZSWR5_BREAK);
			(void) splx(s);
			break;

		case TIOCMSET:
			(void) mcpmctl(za,
				dmtomcp(*(int *) mp->b_cont->b_rptr), DMSET);
			break;

		case TIOCMBIS:
			(void) mcpmctl(za,
				dmtomcp(*(int *) mp->b_cont->b_rptr), DMBIS);
			break;

		case TIOCMBIC:
			(void) mcpmctl(za,
				dmtomcp(*(int *) mp->b_cont->b_rptr), DMBIC);
			break;

		case TIOCMGET:
			*(int *) mp->b_cont->b_rptr =
				mcptodm(mcpmctl(za, 0, DMGET));
			break;

		case MCPIOGPR:
			if (mp->b_cont != NULL)
				freemsg(mp->b_cont);
			if ((mp->b_cont = allocb(1, BPRI_HI)) == NULL) {
/* This is so rare and so exceptional, don't bother with the callback. */
				log (LOG_ERR, "mcpioctl: MCPIOGPR (compat) allocb failed\n");
				error = ENOTTY;
				break;
			}
			mp->b_cont->b_wptr ++;
			/* fall thru */
		case MCPIOSPR:
			if (isprinter(za->za_dev)) {
				mcprcntl(za, mp->b_cont->b_rptr, iocp->ioc_cmd);
				mp->b_cont->b_wptr[0] = mp->b_cont->b_rptr[0];
				iocp->ioc_count = 1;
			} else
				error = ENOTTY;
			break;

		default:
			/*
			 * We don't understand it either.
			 */
			error = ENOTTY;
			break;
		}
	}
	if (error) {
		iocp->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

/*
 * Retry an "ioctl", now that "bufcall" claims we may be able to allocate the
 * buffer we need.
 */
static int
mcpreioctl(unit)
	long		unit;
{
	register struct mcpaline *za = &mcpaline[unit];
	queue_t		*q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	za->za_wbufcid = 0;
	if ((q = za->za_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = za->za_ttycommon.t_iocpending) != NULL) {
		za->za_ttycommon.t_iocpending = NULL;	/* not any more */
		mcpioctl(za, q, mp);
	}
}

static void
mcpparam(za)
	register struct mcpaline *za;
{
	register struct mcpcom *zs = (struct mcpcom *) za->za_common;
	register int    wr1, wr3, wr4, wr5, speed;
	int		baudrate;
	int		loops;
	int		s;
	char		c;
	u_char		v;

	if (isprinter(za->za_dev)) return;

#ifdef lint
	c = 0;
	c = c;
#endif
	/*
	 * Hang up if null baudrate.
	 */
	if ((baudrate = za->za_ttycommon.t_cflag & CBAUD) == 0) {
		(void) mcpmctl(za, ZS_OFF, DMSET);
		return;
	}
	wr1 = ZSWR1_INIT;
	wr3 = (za->za_ttycommon.t_cflag & CREAD) ? ZSWR3_RX_ENABLE : 0;
	wr4 = ZSWR4_X16_CLK;
	wr5 = (zs->zs_wreg[5] & (ZSWR5_RTS | ZSWR5_DTR)) | ZSWR5_TX_ENABLE;

	if (mcpb134_weird && baudrate == B134) {
		/*
		 * XXX - should B134 set all this stuff in the compatibility
		 * module, leaving this stuff fairly clean?
		 */
		wr1 |= ZSWR1_PARITY_SPECIAL;
		wr3 |= ZSWR3_RX_6;
		wr4 |= ZSWR4_PARITY_ENABLE | ZSWR4_PARITY_EVEN | ZSWR4_1_5_STOP;
		wr5 |= ZSWR5_TX_6;
	} else {
		switch (za->za_ttycommon.t_cflag & CSIZE) {

		case CS5:
			wr3 |= ZSWR3_RX_5;
			wr5 |= ZSWR5_TX_5;
			break;

		case CS6:
			wr3 |= ZSWR3_RX_6;
			wr5 |= ZSWR5_TX_6;
			break;

		case CS7:
			wr3 |= ZSWR3_RX_7;
			wr5 |= ZSWR5_TX_7;
			break;

		case CS8:
			wr3 |= ZSWR3_RX_8;
			wr5 |= ZSWR5_TX_8;
			break;
		}

		wr4 |= (za->za_ttycommon.t_cflag & CSTOPB)
			? ZSWR4_2_STOP : ZSWR4_1_STOP;

		if (za->za_ttycommon.t_cflag & PARENB) {
			/*
			 * The PARITY_SPECIAL bit causes a special rx
			 * interrupt on parity errors. Turn it on iff we're
			 * checking the parity of characters.
			 */
			if (za->za_ttycommon.t_iflag & INPCK)
				wr1 |= ZSWR1_PARITY_SPECIAL;
			wr4 |= ZSWR4_PARITY_ENABLE;
			if (!(za->za_ttycommon.t_cflag & PARODD))
				wr4 |= ZSWR4_PARITY_EVEN;
		}
	}

	if ((za->za_ttycommon.t_cflag & CRTSCTS) &&
	    ((za->za_dev & 0xf) < 4))		/* only on the first four ports! */
		wr3 |= ZSWR3_AUTO_CD_CTS;       /* auto RTS/CTS flow control */

	s = spltty();
	if (za->za_ttycommon.t_iflag & IXON)
		*za->za_xoff = za->za_ttycommon.t_stopc;
	else
		*za->za_xoff = DISABLE_XOFF;
	(void) splx(s);

	speed = zs->zs_wreg[12] + (zs->zs_wreg[13] << 8);
	if (wr1 != zs->zs_wreg[1] || wr3 != zs->zs_wreg[3] ||
	    wr4 != zs->zs_wreg[4] || wr5 != zs->zs_wreg[5] ||
	    speed != mcp_speeds[baudrate]) {
		s = spltty();
		if (*za->za_devctl & TXENABLE) {
			*za->za_devctl &= ~TXENABLE;
			za->za_flags |= ZAS_PAUSED;
		}
		(void) splx(s);
		/*
		 * Wait for that last damn character to get out the door.  At
		 * most 1000 loops of 100 usec each is worst case of 110
		 * baud.  If something appears on the output queue then
		 * somebody higher up isn't synchronized and we give up.
		 */
		for (loops = 0; loops < 1000; ++loops) {
			s = spltty();
			SCC_READ(zs->zs_addr, 1, v);
			(void) splx(s);
			if (v & ZSRR1_ALL_SENT)
				break;
			DELAY(100);
		}

		/*
		 * disable receiver while setting parameters
		 */
		s = spltty();
		SCC_WRITE(zs->zs_addr, 3, 0);
		/*
		 * disable fifo transmission while reset errors
		 */
		*za->za_devctl &= ~EN_FIFO_RX;
		zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
		DELAY(2);
		zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS;
		DELAY(2);
		c = zs->zs_addr->zscc_data;	/* swallow junk */
		DELAY(2);
		c = zs->zs_addr->zscc_data;	/* swallow junk */
		DELAY(2);
		c = zs->zs_addr->zscc_data;	/* swallow junk */
		DELAY(2);
		SCC_WRITE(zs->zs_addr, 1, wr1);
		SCC_WRITE(zs->zs_addr, 4, wr4);
		SCC_WRITE(zs->zs_addr, 3, wr3);
		SCC_WRITE(zs->zs_addr, 5, wr5);
		speed = mcp_speeds[baudrate];
		SCC_WRITE(zs->zs_addr, 11, ZSWR11_TXCLK_BAUD + ZSWR11_RXCLK_BAUD);
		SCC_WRITE(zs->zs_addr, 14, ZSWR14_BAUD_FROM_PCLK);
		SCC_WRITE(zs->zs_addr, 12, speed);
		SCC_WRITE(zs->zs_addr, 13, speed >> 8);
		SCC_WRITE(zs->zs_addr, 14,
		    ZSWR14_BAUD_ENA + ZSWR14_BAUD_FROM_PCLK + ZSWR14_DTR_IS_REQUEST);
		SCC_WRITE(zs->zs_addr, 15, ZSR15_CD | ZSR15_CTS | ZSR15_BREAK);
		*za->za_devctl |= EN_FIFO_RX;
		(void) splx(s);
		mcpstart(za);
	}
}

static int
dmtomcp(bits)
	register int    bits;
{
	register int    b = 0;

	if (bits & TIOCM_CAR)
		b |= ZSRR0_CD;
	if (bits & TIOCM_CTS)
		b |= ZSRR0_CTS;
	if (bits & TIOCM_RTS)
		b |= ZSWR5_RTS;
	if (bits & TIOCM_DTR)
		b |= ZSWR5_DTR;
	return (b);
}

static int
mcptodm(bits)
	register int    bits;
{
	register int    b = 0;

	if (bits & ZSRR0_CD)
		b |= TIOCM_CAR;
	if (bits & ZSRR0_CTS)
		b |= TIOCM_CTS;
	if (bits & ZSWR5_RTS)
		b |= TIOCM_RTS;
	if (bits & ZSWR5_DTR)
		b |= TIOCM_DTR;
	return (b);
}

static int
mcpmctl(za, bits, how)
	register struct mcpaline *za;
	register int    bits;
	int		how;
{
	register struct mcpcom *zs = za->za_common;
	register int    mbits, obits;
	int		s, now, held;

	if (isprinter(za->za_dev)) return 0;

again:
	s = spltty();
	mbits = zs->zs_wreg[5] & (ZSWR5_RTS | ZSWR5_DTR);
	zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
	DELAY(2);
	mbits |= zs->zs_addr->zscc_control & (ZSRR0_CD | ZSRR0_CTS);
	DELAY(2);
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
	held = now - za->za_dtrlow;
/* if DTR is going low, stash current time away */
	if (~mbits & obits & ZSWR5_DTR)
		za->za_dtrlow = now;
/* if DTR is going high, sleep until it has been low a bit */
	if ((mbits & ~obits & ZSWR5_DTR) && (held < mcpadtrlow)) {
		(void) splx (s);
		(void) sleep ((caddr_t)&lbolt, PZERO-1);
		goto again;
	}

	zs->zs_wreg[5] &= ~(ZSWR5_RTS | ZSWR5_DTR);
	SCCBIS(zs->zs_addr, 5, mbits & (ZSWR5_RTS | ZSWR5_DTR));
	if (mbits & ZSWR5_DTR)
		*za->za_devctl |= MCP_DTR_ON;
	else
		*za->za_devctl &= ~MCP_DTR_ON;

	(void) splx(s);
	return (mbits);
}

static void
mcpstart(za)
	register struct mcpaline *za;
{
	register struct mcpcom *zs = za->za_common;
	register int    cc, avail_bytes, loops;
	register unsigned char v, *current_position;
	register queue_t *q;
	register mblk_t *bp, *nbp;
	int		s, max_buf_size;

	s = spltty();

	/*
	 * If we're waiting for a break timeout to expire, don't grab
	 * anything new.
	 */
	if (za->za_flags & ZAS_BREAK)
		goto out;

	/*
	 * If we have a flow-control character to transmit, do it now.
	 * Suspend the current transmission, reach around the DMA chip, and
	 * stuff the flow-control character into the SCC. Then tidy up and go
	 * on our way. By the way, if we're a printer, forget it.
	 */
	if ((za->za_flowc != '\0') && !isprinter(za->za_dev)) {
		if (za->za_flags & ZAS_BUSY) {
			*za->za_devctl &= ~TXENABLE;
			za->za_flags |= ZAS_PAUSED;
		}
		/*
		 * Wait for end of transmission. See comment in mcpparam().
		 */
		loops = 1000;
		do {
			SCC_READ(zs->zs_addr, 1, v);
			if (v & ZSRR1_ALL_SENT)
				break;
			(void) splx(s);
			DELAY(100);
			s = spl4();

		} while (--loops > 0);
		zs->zs_addr->zscc_data = za->za_flowc;
		DELAY(2);
		za->za_flowc = '\0';
	}
	/*
	 * If a DMA operation was terminated early, restart it unless output
	 * is stopped.
	 */
         
         if ((za->za_flags & ZAS_PAUSED) && !(za->za_flags & ZAS_STOPPED)) {
		za->za_flags &= ~ZAS_PAUSED;
		/*
		 * If there's data in the transmit DMA channel,
		 * we have to finish that first; mcpa_txend() will 
		 * kick us again.
		 */
		if (mcp_getwc(zs->mcp_txdma)) {
			*za->za_devctl |= TXENABLE;
			goto out;
		}
	}

	/*
	 * If we're waiting for a delay timeout to expire or for the current
	 * transmission to complete, don't grab anything new.
	 */
	if (za->za_flags & (ZAS_BUSY | ZAS_DELAY))
		goto out;

	/*
	 * Hopefully we're attached to a stream.
	 */
	if ((q = za->za_ttycommon.t_writeq) == NULL)
		goto out;	/* Apparently not. */

	/*
	 * Set up to copy up to a bufferful of bytes into the MCP buffer.
	 */
	max_buf_size = isprinter(za->za_dev) ? PR_BSZ : ASYNC_BSZ;
	current_position = za->za_optr = za->za_dmabuf;
	avail_bytes = max_buf_size;

	while ((bp = getq(q)) != NULL) {
		/*
		 * We have a message block to work on. Check whether it's a
		 * break, a delay, or an ioctl (the latter occurs if the
		 * ioctl in question was waiting for the output to drain). If
		 * it's one of those, process it immediately.
		 */
		switch (bp->b_datap->db_type) {

		case M_BREAK:
			if (avail_bytes != max_buf_size) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * Set the break bit, and arrange for "mcprstrt" to
			 * be called in 1/4 second; it will turn the break
			 * bit off, and call "mcpstart" to grab the next
			 * message.
			 */
			SCCBIS(zs->zs_addr, 5, ZSWR5_BREAK);
			timeout(mcprstrt, (caddr_t) za, hz / 4);
			za->za_flags |= ZAS_BREAK;
			freemsg(bp);
			goto out;

		case M_DELAY:
			if (avail_bytes != max_buf_size) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * Arrange for "mcprstrt" to be called when the delay
			 * expires; it will turn ZAS_DELAY off, then call
			 * "mcpstart" to grab the next message.
			 */
			timeout(mcprstrt, (caddr_t)za,
			    (int)(*(unsigned char *)bp->b_rptr + 6));
			za->za_flags |= ZAS_DELAY;
			freemsg(bp);
			goto out;

		case M_IOCTL:
			if (avail_bytes != max_buf_size) {
				putbq(q, bp);
				goto transmit;
			}
			/*
			 * This ioctl was waiting for the output ahead of it
			 * to drain; obviously, it has. Do it, and then grab
			 * the next message after it.
			 */
			mcpioctl(za, q, bp);
			continue;
		}

		/*
		 * We have data to transmit. If output is stopped, put it
		 * back and try again later.
		 */
		if (za->za_flags & ZAS_STOPPED) {
			putbq(q, bp);
			goto out;
		}
		do {
			while ((cc = bp->b_wptr - bp->b_rptr) != 0) {
				if (avail_bytes == 0) {
					/*
					 * Out of buffer space; put this
					 * buffer back on the queue, and
					 * transmit what we have.
					 */
					putbq(q, bp);
					goto transmit;
				}
				cc = MIN(cc, avail_bytes);
				bcopy((char *)bp->b_rptr,
				    (char *)current_position, (u_int)cc);
				current_position += cc;
				avail_bytes -= cc;
				bp->b_rptr += cc;
			}
			nbp = bp;
			bp = bp->b_cont;
			freeb(nbp);
		} while (bp != NULL);

	}			/* while getq != NULL */

transmit:

	if (za->za_ocnt = max_buf_size - avail_bytes) {
		za->za_done = 0;
		za->za_xactive = 1;
		if (!isprinter(za->za_dev) && !(*za->za_devctl & TXENABLE))
			*za->za_devctl |= TXENABLE;
		mcp_dmastart(zs->mcp_txdma, (char *)za->za_dmabuf,
		    za->za_ocnt);
		za->za_flags |= ZAS_BUSY;
		zs->zs_flags |= MCP_WAIT_DMA;	/* Set this for mcpintr(). */
	}
out:
	(void) splx(s);
}

static void
mcparingov (za)
	struct mcpaline *za;
{
	log (LOG_ERR, "mcpa%d: input ring overflow\n",
	    UNIT(za->za_dev));
}

static int
mcprstrt(za)
	register struct mcpaline *za;
{
	register struct mcpcom *zs = za->za_common;

	/*
	 * If break timer expired, turn off the break bit.
	 */
	if (za->za_flags & ZAS_BREAK)
		SCCBIC(zs->zs_addr, 5, ZSWR5_BREAK);
	za->za_flags &= ~(ZAS_DELAY | ZAS_BREAK);
	mcpstart(za);
}

/* ARGSUSED */
static int
mcpa_txint(zs)
	struct mcpcom  *zs;
{
	return;
}

static int
mcpa_xsint(zs)
	register struct mcpcom *zs;
{
	register struct mcpaline *za = (struct mcpaline *) zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register u_char s0, x0;
	register queue_t *q;
	int s;

	/*
	 * Found a break?
	 */
	s0 = zsaddr->zscc_control;
	DELAY(2);
	x0 = s0 ^ za->za_rr0;
	za->za_rr0 = s0;
	zsaddr->zscc_control = ZSWR0_RESET_STATUS;
	DELAY(2);
	if ((x0 & ZSRR0_BREAK) && (s0 & ZSRR0_BREAK) == 0) {
		/*
		 * ZSRR0_BREAK turned off.  This means that the break sequence
		 * has completed (i.e., the stop bit finally arrived).
		 * Send M_BREAK upstream, and flag the interrupt routine to
		 * throw away the NUL character in the receiver. (The
		 * ldterm module upstream doesn't need it; it will use
		 * the M_BREAK to do the right thing.)
		 */
		zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
		DELAY(2);
		zs->zs_rerror += 1;
		if ((q = za->za_ttycommon.t_readq) != NULL)
			(void) putctl(q->q_next, M_BREAK);
	}
	/*
	 * Carrier up?
	 */
	if ((za->za_ttycommon.t_flags & TS_SOFTCAR) ||
	    (mcpmctl(za, 0, DMGET) & ZSRR0_CD)) {
		/* carrier present */
		if ((za->za_flags & ZAS_CARR_ON) == 0) {
			za->za_flags |= ZAS_CARR_ON;
			if ((q = za->za_ttycommon.t_readq) != NULL)
				(void) putctl(q->q_next, M_UNHANGUP);
			wakeup((caddr_t) &za->za_flags);
		}
	} else {
		if ((za->za_flags & ZAS_CARR_ON) &&
		    !(za->za_ttycommon.t_cflag & CLOCAL)) {
			/*
			 * Carrier went away.
			 * Drop DTR, abort any output in progress,
			 * indicate that output is not stopped, and
			 * send a hangup notification upstream.
			 */
			(void) mcpmctl(za, ZSWR5_DTR, DMBIC);
			s = spltty();
			if (za->za_flags & ZAS_BUSY) {
				(void) mcp_dmahalt(zs->mcp_txdma);
				za->za_flags &= ~ZAS_BUSY;
			}
			za->za_flags &= ~ZAS_STOPPED;
			(void) splx(s);
			if ((q = za->za_ttycommon.t_readq) != NULL)
				(void) putctl(q->q_next, M_HANGUP);
		}
		za->za_flags &= ~ZAS_CARR_ON;
	}
}

/* ARGSUSED */
static int
mcpa_rxint(zs)
	struct mcpcom *zs;
{
	return;
}

static int
mcpa_srint(zs)
	register struct mcpcom *zs;
{
	register struct mcpaline *za = (struct mcpaline *) zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register short  s1;
	register u_char c;

	SCC_READ(zs->zs_addr, 1, s1);
	c = zsaddr->zscc_data;
	DELAY(2);
	if (s1 & ZSRR1_PE) {
		/*
		 * IGNPAR       PARMRK  RESULT
		 *    off          off       0
		 *    off           on       3 byte sequence
		 *     on       either	     ignored
		 */
		if (!(za->za_ttycommon.t_iflag & IGNPAR)) {
			/*
			 * The receive interrupt routine has already stuffed c
			 * into the ring.  Dig it out again, since the current
			 * mode settings don't allow it to appear in that
			 * position.
			 */
			if (MCP_RING_CNT(za) != 0)
				MCP_RING_UNPUT(za);
			else
				log(LOG_ERR, "mcpa%d: parity error ignored\n",
				    minor(za->za_dev));
			if (za->za_ttycommon.t_iflag & PARMRK) {
				/*
				 * Must stuff the 377 directly into the silo.
				 * If ISTRIP is set, rxchar() will detect the
				 * \377 and double it. This should always be
				 * safe, since we really should never have more
				 * than 16 or so characters in the input silo.
				 * But just in case...
				 */
				if (!MCP_RING_POK(za,3))
					mcpa_drain(za);
				if (MCP_RING_POK(za,3)) {
					MCP_RING_PUT(za,0377);
				mcpa_rxchar(zs, '\0');
				mcpa_rxchar(zs, c);
				} else mcparingov(za);
			} else {
				mcpa_rxchar(zs, '\0');
			}
		} else {
			if (MCP_RING_CNT(za) != 0)
				MCP_RING_UNPUT(za);
			else
				log(LOG_ERR, "mcpa%d: parity error went up\n",
				    minor(za->za_dev));
		}
	}
	if ((s1 & ZSRR1_DO) && (za->za_flags & ZAS_ISOPEN))
		log(LOG_ERR, "mcpa%d: SCC silo overflow\n",
		    minor(za->za_dev));
	zs->zs_rerror += 1;
	zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS;
	DELAY(2);
}

static int
mcpa_txend(zs)
	register struct mcpcom *zs;
{
	register struct mcpaline *za = (struct mcpaline *) zs->zs_priv;

	za->za_xactive = 0;
	if (za->za_flags & ZAS_BUSY) {
		/*
		 * If a transmission has finished, indicate that it's
		 * finished, and start that line up again.
		 */
		za->za_flags &= ~ZAS_BUSY;
		mcpstart(za);
	}
}

static int
mcpa_rxchar(zs, c)
	register struct mcpcom *zs;
	register u_char c;
{
	register struct mcpaline *za = (struct mcpaline *) zs->zs_priv;

	if ((za->za_flags & ZAS_ISOPEN) == 0)
		return;
	if (za->za_ttycommon.t_readq == NULL)
		return;				/* I ain't got nobody */

	/*
	 * NULs arriving while a break sequence is in progress are part of
	 * that sequence.  Discard them.
	 */
	if (c == '\0' && (za->za_rr0 & ZSRR0_BREAK))
		return;

	/*
	 * Stash the incoming character. Maybe.
	 */
	if (MCP_RING_POK(za,1)) {
		/*
		 * XXX - Double echo the '\377' character here if need be.
		 * This positively should not be down here in the driver.
		 * Thank Ritchie for short-circuit evaluation.
		 */
		if ((za->za_ttycommon.t_iflag & PARMRK) &&
		    !(za->za_ttycommon.t_iflag & (IGNPAR|ISTRIP)) &&
		    (c == 0377)) {
			if (MCP_RING_POK(za,2)) {
				MCP_RING_PUT(za,c);
				MCP_RING_PUT(za,c);
			} else mcparingov (za);
		} else MCP_RING_PUT(za,c);
	} else mcparingov (za);

	/*
	 * XXX - The board has half a brain: it understands stop characters
	 * but doesn't grok literal-next (^V, usually). Thus, ^V^S sequences
	 * do not have the desired effect. The board stops dead, but the rest
	 * of the system continues on blithely because "that wasn't *really*
	 * a control-S." Handle the special case here.
	 */
	if (za->za_flags & ZAS_LNEXT) {
		if (c==za->za_ttycommon.t_stopc && !(za->za_flags&ZAS_STOPPED)){
			*za->za_devctl |= TXENABLE;
		}
		za->za_flags &= ~ZAS_LNEXT;
	} else if (c == za->za_lnext) {
		za->za_flags |= ZAS_LNEXT;
	}

	/*
	 * While we are here, if c might be the stop character, or the silo
	 * is at least half full, drain it. Otherwise, schedule a drain
	 * mcpaticks in the future...unless one is already scheduled.
	 */
	if (c == za->za_ttycommon.t_stopc || MCP_RING_FRAC(za)) {
		mcpa_drain(za);
	} else if (!(za->za_flags & ZAS_DRAINING)) {
		timeout(mcpa_drain, (caddr_t) za, mcpaticks);
		za->za_flags |= ZAS_DRAINING;
	}
}

static int
mcpa_drain(za)
	register struct mcpaline *za;
{
	mblk_t		*bp;
	register int    cc;
	register queue_t *q;
	int		s, t;

	s = spltty();
	t = splclock();
	if (za->za_flags & ZAS_DRAINING) {
		za->za_flags &= ~ZAS_DRAINING;
		untimeout(mcpa_drain, (caddr_t) za);
	}
	(void) splx(t);

	if ((q = za->za_ttycommon.t_readq) == NULL) {
		MCP_RING_INIT (za);
		(void) splx(s);	/* gotta do this too! */
		return;		/* And nobody cares 'bout me */
	}

	if ((cc = MCP_RING_CNT(za)) > 0)
		do {
			if (cc > 16)
				cc = 16;
			if ((bp = allocb(cc, BPRI_MED)) != (mblk_t *)0) {
				if (canput(q->q_next)) {
					do {
						*(bp->b_wptr++) = MCP_RING_GET(za);
					} while (--cc > 0);
					putnext(q, bp);
				} else {
					ttycommon_qfull(&za->za_ttycommon, q);
					freemsg(bp);
				}
			}
			MCP_RING_EAT (za, cc);
		} while ((cc = MCP_RING_CNT(za)) > 15);

	(void) splclock();
	if ((MCP_RING_CNT(za) != 0) && !(za->za_flags & ZAS_DRAINING)) {
		timeout(mcpa_drain, (caddr_t) za, mcpaticks);
		za->za_flags |= ZAS_DRAINING;
	}
	(void) splx(s);
}

static int
mcpa_rxend(zs)
	register struct mcpcom *zs;
{
	log(LOG_ERR, "MCP port %d error: impossible input EOF interrupt\n",
	       zs->mc_unit * 16 + zs->zs_unit);
}


/*
 * Parallel printer port support routines
 */

int
mcpr_attach(md)
	register struct mb_device *md;
{
	register int    unit = md->md_unit;
	register struct mcpcom *zs = &mcpcom[unit + nmcpline];
	register struct mcpaline *za = &mcpaline[unit + nmcpline];

	zs->mcp_addr = (struct mcp_device *) md->md_addr;
	zs->mcp_txdma = mcp_dmagetchan(0, TX_DIR, unit, PR_DMA);
	zs->mcp_rxdma = mcp_dmagetchan(0, RX_DIR, unit, PR_DMA);
	if (md->md_flags & 0x10000)	/* ignore SLCT line? */
		zs->zs_flags = MCPRIGNSLCT;
	else
		zs->zs_flags = 0;
	mcpaline[unit + nmcpline].za_common = zs;
	za->za_dmabuf = zs->mcp_addr->printer_buf;
	za->za_common = zs;
}

/* ARGSUSED */
static int
mcpropen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t		dev;
	int		flag, sflag;
{
	register struct mcpcom *zs;
	register struct mcpaline *za;
	register int    unit;
	int		s;
	unsigned char   status;

	unit = UNIT(dev);
	if ((unit >= nmcp) || (dev & OUTLINE))
		return (OPENFAIL);
	za = &mcpaline[unit + nmcpline];
	zs = &mcpcom[unit + nmcpline];
	if (za->za_common != zs)
		return (OPENFAIL);
	s = spltty();
	if (zs->mcp_ops == NULL)
		zs->mcp_ops = &mcpops_print;
	if (zs->mcp_ops != &mcpops_print) {
		(void) splx(s);
		u.u_error = EPROTO;
		return (OPENFAIL);
	}
	zs->zs_priv = (caddr_t) za;
	za->za_dev = dev;

	za->za_flags |= ZAS_WOPEN;
	if ((za->za_flags & ZAS_ISOPEN) == 0)
		za->za_ttycommon.t_cflag = I_CFLAGS;
	else if (za->za_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	/* offline? */
	mcprcntl(za, &status, MCPIOGPR);
	if ((zs->zs_flags & MCPRIGNSLCT) == 0 &&	/* do we care? */
	    (status & (MCPRPE | MCPRSLCT)) != (MCPRPE | MCPRSLCT)) {
		(void) splx(s);
		u.u_error = EIO;
		return (OPENFAIL);
	}	za->za_ttycommon.t_readq = q;
	za->za_ttycommon.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t) za;
	za->za_flags &= ~ZAS_WOPEN;
	za->za_flags |= ZAS_ISOPEN | ZAS_CARR_ON;
	(void) splx(s);
	return (dev);
}

static void
mcprcntl(za, datap, cmd)
	register struct mcpaline *za;
	register u_char *datap;
	int		cmd;
{
	register struct mcpcom *zs = (struct mcpcom *) za->za_common;
	register struct ciochip *cp = &zs->mcp_addr->cio;
	register u_char uc;
	int		s;

	switch (cmd) {

	case MCPIOSPR:
		if ((*datap ^ zs->zs_flags) & (MCPRINTSLCT | MCPRINTPE)) {
			zs->zs_flags = (zs->zs_flags & ~(MCPRINTSLCT | MCPRINTPE))
				| (*datap & (MCPRINTSLCT | MCPRINTPE));
			s = spltty();
			CIO_READ(cp, CIO_PB_PM, uc);
			uc &= ~(MCPRINTSLCT | MCPRINTPE) << 2;
			uc |= (*datap & (MCPRINTSLCT | MCPRINTPE)) << 2;
			CIO_WRITE(cp, CIO_PB_PM, uc);
			(void) splx(s);
		}
		if ((*datap ^ zs->zs_flags) & MCPRDIAG) {
			zs->zs_flags = (zs->zs_flags & ~MCPRDIAG)
				| (*datap & MCPRDIAG);
			s = spltty();
			cp->portc_data = (cp->portc_data & ~MCPRDIAG & 0x0f)
				| (*datap & MCPRDIAG);
			(void) splx(s);
		}
		/* fall through */

	case MCPIOGPR:
		*datap = zs->zs_flags & (MCPRINTSLCT | MCPRINTPE | MCPRIGNSLCT);
		*datap |= cp->portb_data & (MCPRPE | MCPRSLCT);
		*datap |= cp->portc_data & (MCPRDIAG | MCPRVMEINT);
		break;
	}
}

static int
mcpr_pe(zs)
	register struct mcpcom *zs;
{
	register struct ciochip *cp = &zs->mcp_addr->cio;
	register u_char uc;

	if (zs->zs_flags & MCPRINTPE && !(cp->portb_data & MCPRPE)) {
		log(LOG_NOTICE, "Printer on mcpp%x is out of paper\n",
		    zs->mc_unit);
		CIO_READ(cp, CIO_PB_PP, uc);
		uc |= MCPRPE;
		CIO_WRITE(cp, CIO_PB_PP, uc);
	} else {
		log(LOG_NOTICE, "Printer on mcpp%x: paper ok\n", zs->mc_unit);
		CIO_READ(cp, CIO_PB_PP, uc);
		uc &= ~MCPRPE;
		CIO_WRITE(cp, CIO_PB_PP, uc);
	}
}

static int
mcpr_slct(zs)
	register struct mcpcom *zs;
{
	register struct ciochip *cp = &zs->mcp_addr->cio;
	register u_char uc;

	if (zs->zs_flags & MCPRINTSLCT && !(cp->portb_data & MCPRSLCT)) {
		log(LOG_NOTICE, "Printer on mcpp%x offline\n", zs->mc_unit);
		CIO_READ(cp, CIO_PB_PP, uc);
		uc |= MCPRSLCT;
		CIO_WRITE(cp, CIO_PB_PP, uc);
	} else {
		log(LOG_NOTICE, "Printer on mcpp%x online\n", zs->mc_unit);
		CIO_READ(cp, CIO_PB_PP, uc);
		uc &= ~MCPRSLCT;
		CIO_WRITE(cp, CIO_PB_PP, uc);
	}
}
