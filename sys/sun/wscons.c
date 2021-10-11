#ifndef lint
static	char sccsid[] = "@(#)wscons.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 * "Workstation console" multiplexor driver for Sun.
 *
 * Sends output to the primary frame buffer using the PROM monitor;
 * gets input from a stream linked below us that is the "keyboard
 * driver", below which is linked the primary keyboard.
 */

#include <sys/param.h>
#include <sys/user.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/tty.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/buf.h>
#include <sys/vmmac.h>
#include <sys/uio.h>
#include <sys/proc.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>

#include <mon/keyboard.h>

#include <sundev/mbvar.h>

#if defined(sun4) || defined(sun4c) || defined(sun4m)
#include <machine/eeprom.h>

/*
 * The MIN and MAX values come from the PROM monitor code.
 * The other values are the defaults for lo-res and hi-res screens.
 */
#define	MINLINES	10
#define	MAXLINES	48
#define	LOSCREENLINES	34
#define	HISCREENLINES	48

#define	MINCOLS		10
#define	MAXCOLS		120
#define	LOSCREENCOLS	80
#define	HISCREENCOLS	120
#endif

/*
 * Since this is used in obp and non-obp machines, we still have to isolate
 * these differences until obplib is extended to non-obp architectures.
 */

#ifdef	OPENPROMS

#define	STDOUT_IS_FB	prom_stdout_is_framebuffer()

#ifdef	MULTIPROCESSOR
/* can't risk user processes on other cpus bashing the output device
 * while the boot prom is also bashing it.
 */
#define	prom_writestr(p,c)	mpprom_write(p,c)
#define	prom_putchar(c)		mpprom_putchar(c)
#define	prom_mayput(c)		mpprom_mayput(c)
#else	MULTIPROCESSOR
extern void	prom_writestr();
extern void	prom_putchar();
extern int	prom_mayput();
#endif	MULTIPROCESSOR

#else	OPENPROMS

#define	STDOUT_IS_FB		(*romp->v_outsink == OUTSCREEN)

#define	prom_writestr(p,c)	(*romp->v_fwritestr)(p,c,romp->v_fbaddr)
#define	prom_putchar(x)		while ((*romp->v_mayput)(x) != 0)
#define	prom_mayput(x)  	((*romp->v_mayput)(x))

#endif	OPENPROMS

#define	IFLAGS	(CS7|CREAD|PARENB)

static struct {
	int	wc_flags;		/* random flags */
	dev_t	wc_dev;			/* major/minor for this device */
	tty_common_t wc_ttycommon;	/* data common to all tty drivers */
	int	wc_wbufcid;		/* id of pending write-side bufcall */
	int	wc_pendc;		/* pending output character */
	queue_t	*wc_kbdqueue;		/* "console keyboard" device queue */
					/* below us */
} wscons;

#define	WCS_ISOPEN	0x00000001	/* open is complete */
#define	WCS_STOPPED	0x00000002	/* output is stopped */
#define	WCS_DELAY	0x00000004	/* waiting for delay to finish */
#define	WCS_BUSY	0x00000008	/* waiting for transmission to finish */

static int	wcopen();
static int	wcclose();
static int	wcuwput();
static int	wclrput();

static struct module_info wcm_info = {
	0,
	"wc",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit wcurinit = {
	putq,
	NULL,
	wcopen,
	wcclose,
	NULL,
	&wcm_info,
	NULL
};

static struct qinit wcuwinit = {
	wcuwput,
	NULL,
	wcopen,
	wcclose,
	NULL,
	&wcm_info,
	NULL
};

static struct qinit wclrinit = {
	wclrput,
	NULL,
	NULL,
	NULL,
	NULL,
	&wcm_info,
	NULL
};

static struct qinit wclwinit = {
	putq,
	NULL,
	NULL,
	NULL,
	NULL,
	&wcm_info,
	NULL
};

static char *wcmodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab wcinfo = {
	&wcurinit,
	&wcuwinit,
	&wclrinit,
	&wclwinit,
	wcmodlist
};

static int	wcreioctl(/*long unit*/);
static void 	wcioctl(/*queue_t *q, mblk_t *mp*/);
static int	wcopoll(/*void*/);
static int	wcrstrt(/*void*/);
static void	wcstart(/*void*/);
static int	wconsout(/*void*/);

/*ARGSUSED*/
static int
wcopen(q, dev, flag, sflag)
	register queue_t *q;
	int dev;
{
	if (minor(dev) != 0)
		return (OPENFAIL);	/* sorry, only one per customer */

	if (!(wscons.wc_flags & WCS_ISOPEN)) {
		wscons.wc_ttycommon.t_iflag = 0;
		wscons.wc_ttycommon.t_cflag = (B9600 << IBSHIFT)|B9600|IFLAGS;
		wscons.wc_ttycommon.t_iocpending = NULL;
		wscons.wc_wbufcid = 0;
		wscons.wc_flags = WCS_ISOPEN;
		/*
		 * If we can have either a normal or high-resolution screen,
		 * we should indicate the size of the screen.  Otherwise, we
		 * just say zero and let the program get the standard 34x80
		 * value from the "termcap" or "terminfo" file.
		 */
		wscons.wc_ttycommon.t_size.ws_row = 0;
		wscons.wc_ttycommon.t_size.ws_col = 0;
		wscons.wc_ttycommon.t_size.ws_xpixel = 0;
		wscons.wc_ttycommon.t_size.ws_ypixel = 0;
#if defined(sun4) || defined(sun4c) || defined (sun4m)
#ifdef sun3
		/*
		 * There is no guarantee that Sun-3s other than 3/200s and
		 * 3/60s have the EEPROM programmed properly.
		 */
		if (cpu == CPU_SUN3_260 || cpu == CPU_SUN3_60)
#endif
#ifdef sun3x
	if (cpu == CPU_SUN3X_470)
#endif sun3x
		{
			/*
			 * Unfortunately, there's no way to ask the PROM
			 * monitor how big it thinks the screen is, so we have
			 * to duplicate what the PROM monitor does.
			 */
			u_short t_cols, t_rows;

			if (EEPROM->ee_diag.eed_scrsize == EED_SCR_1600X1280) {
				t_cols = EEPROM->ee_diag.eed_colsize;
				if (t_cols < MINCOLS)
					t_cols = LOSCREENCOLS;
				else if (t_cols > MAXCOLS)
					t_cols = HISCREENCOLS;
				wscons.wc_ttycommon.t_size.ws_col = t_cols;

				t_rows = EEPROM->ee_diag.eed_rowsize;
				if (t_rows < MINLINES)
					t_rows = LOSCREENLINES;
				else if (t_rows > MAXLINES)
					t_rows = HISCREENLINES;
				wscons.wc_ttycommon.t_size.ws_row = t_rows;
			}
		}
#endif
	}
	if (wscons.wc_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	wscons.wc_ttycommon.t_readq = q;
	wscons.wc_ttycommon.t_writeq = WR(q);
	return (dev);
}

/*ARGSUSED*/
static int
wcclose(q, flag)
	queue_t *q;
	int flag;
{
	register int	s = spl5();
	/*
	 * Check for pending output on this file before shuting down.
	 */
	while (!(STDOUT_IS_FB) &&
	    (wscons.wc_flags & WCS_BUSY) && wscons.wc_pendc != -1) {
		prom_putchar(wscons.wc_pendc);
		/* we don't need this timeout anymore */
		untimeout(wcopoll, (caddr_t)0);
		wscons.wc_pendc = -1;
		wscons.wc_flags &= ~WCS_BUSY;
		/* drain anything else in the queue */
		if (!(wscons.wc_flags&(WCS_DELAY|WCS_STOPPED))) {
			wcstart();
		}
	}

	wscons.wc_flags = 0;
	ttycommon_close(&wscons.wc_ttycommon);
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (wscons.wc_wbufcid) {
		unbufcall(wscons.wc_wbufcid);
		wscons.wc_wbufcid = 0;
	}
	(void) splx(s);
}

/*
 * Put procedure for upper write queue.
 * Respond to M_STOP, M_START, M_IOCTL, and M_FLUSH messages here;
 * queue up M_BREAK, M_DELAY, and M_DATA messages for processing by
 * the start routine, and then call the start routine; discard
 * everything else.
 */
static int
wcuwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	int s;

	switch (mp->b_datap->db_type) {

	case M_STOP:
		wscons.wc_flags |= WCS_STOPPED;
		freemsg(mp);
		break;

	case M_START:
		wscons.wc_flags &= ~WCS_STOPPED;
		wcstart();
		freemsg(mp);
		break;

	case M_IOCTL: {
		register struct iocblk *iocp;
		register struct linkblk *linkp;

		iocp = (struct iocblk *)mp->b_rptr;
		switch (iocp->ioc_cmd) {

		case I_LINK:	/* stupid, but permitted */
		case I_PLINK:
			if (wscons.wc_kbdqueue != NULL)
				goto iocnak;	/* somebody already linked */
			linkp = (struct linkblk *)mp->b_cont->b_rptr;
			wscons.wc_kbdqueue = linkp->l_qbot;
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q, mp);
			break;

		case I_UNLINK:	/* stupid, but permitted */
		case I_PUNLINK:
			linkp = (struct linkblk *)mp->b_cont->b_rptr;
			if (wscons.wc_kbdqueue != linkp->l_qbot)
				goto iocnak;	/* not us */
			wscons.wc_kbdqueue = NULL;
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q, mp);
			break;

		iocnak:
			mp->b_datap->db_type = M_IOCNAK;
			qreply(q, mp);
			break;

		case TCSETSW:
		case TCSETSF:
		case TCSETAW:
		case TCSETAF:
		case TCSBRK:
			/*
			 * The changes do not take effect until all
			 * output queued before them is drained.
			 * Put this message on the queue, so that
			 * "wcstart" will see it when it's done
			 * with the output before it.  Poke the
			 * start routine, just in case.
			 */
			putq(q, mp);
			wcstart();
			break;

		default:
			/*
			 * Do it now.
			 */
			wcioctl(q, mp);
			break;
		}
		break;
	}

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			s = spl5();
			/*
			 * Flush our write queue.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			(void) splx(s);
			*mp->b_rptr &= ~FLUSHW;	/* it has been flushed */
		}
		if (*mp->b_rptr & FLUSHR) {
			s = spl5();
			flushq(RD(q), FLUSHDATA);
			(void) splx(s);
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
			freemsg(mp);
		break;

	case M_BREAK:
		/*
		 * Ignore these, as they make no sense.
		 */
		freemsg(mp);
		break;

	case M_DELAY:
	case M_DATA:
		/*
		 * Queue the message up to be transmitted,
		 * and poke the start routine.
		 */
		putq(q, mp);
		wcstart();
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
/*ARGSUSED*/
static int
wcreioctl(unit)
	long unit;
{
	queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	wscons.wc_wbufcid = 0;
	if ((q = wscons.wc_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = wscons.wc_ttycommon.t_iocpending) != NULL) {
		/* not pending any more */
		wscons.wc_ttycommon.t_iocpending = NULL;
		wcioctl(q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
wcioctl(q, mp)
	queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	register unsigned datasize;
	int error;

	iocp = (struct iocblk *)mp->b_rptr;

	if (iocp->ioc_cmd == TIOCSWINSZ || iocp->ioc_cmd == TIOCSSIZE) {
		/*
		 * Ignore all attempts to set the screen size; the value in the
		 * EEPROM is guaranteed (modulo PROM bugs) to be the value used
		 * by the PROM monitor code, so it is by definition correct.
		 * Many programs (e.g., "login" and "tset") will attempt to
		 * reset the size to (0, 0) or (34, 80), neither of which is
		 * necessarily correct.
		 * We just ACK the message, so as not to disturb programs that
		 * set the sizes.
		 */
		iocp->ioc_count = 0;	/* no data returned */
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		return;
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
	    ttycommon_ioctl(&wscons.wc_ttycommon, q, mp, &error)) != 0) {
		if (wscons.wc_wbufcid)
			unbufcall(wscons.wc_wbufcid);
		wscons.wc_wbufcid = bufcall(datasize, BPRI_HI, wcreioctl,
		    (long)0);
		return;
	}

	if (error < 0) {
		if (iocp->ioc_cmd == TCSBRK)
			error = 0;
		else
			error = ENOTTY;
	}
	if (error != 0) {
		iocp->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

static int
wcopoll()
{
	register int s;

	if ((wscons.wc_flags & WCS_ISOPEN) == 0)
		return;
	/* See if we can continue output */
	s = spl1();
	if ((wscons.wc_flags & WCS_BUSY) && wscons.wc_pendc != -1) {
		if (prom_mayput(wscons.wc_pendc) == 0) {
			wscons.wc_pendc = -1;
			wscons.wc_flags &= ~WCS_BUSY;
			if (!(wscons.wc_flags&(WCS_DELAY|WCS_STOPPED)))
				wcstart();
		} else
			timeout(wcopoll, (caddr_t)0, 1);
	}
	(void) splx(s);
}

/*
 * Restart output on the console after a timeout.
 */
static int
wcrstrt()
{
	wscons.wc_flags &= ~WCS_DELAY;
	wcstart();
}

/*
 * Start console output
 * Runs at level 5 - not safe to drop level
 */
static void
wcstart()
{
	register int c;
	register int cc;
	register queue_t *q;
	register mblk_t *bp;
	register mblk_t *nbp;
	int s;

	s = spl5();

	/*
	 * If we're waiting for something to happen (delay timeout to
	 * expire, current transmission to finish, output to be
	 * restarted, output to finish draining), don't grab anything
	 * new.
	 */
	if (wscons.wc_flags & (WCS_DELAY|WCS_BUSY|WCS_STOPPED))
		goto out;

	if ((q = wscons.wc_ttycommon.t_writeq) == NULL)
		goto out;	/* not attached to a stream */

	for (;;) {
		if ((bp = getq(q)) == NULL)
			goto out;	/* nothing to transmit */

		/*
		 * We have a new message to work on.
		 * Check whether it's a delay or an ioctl (the latter
		 * occurs if the ioctl in question was waiting for the output
		 * to drain).  If it's one of those, process it immediately.
		 */
		switch (bp->b_datap->db_type) {

		case M_DELAY:
			/*
			 * Arrange for "wcrstrt" to be called when the
			 * delay expires; it will turn WCS_DELAY off,
			 * and call "wcstart" to grab the next message.
			 */
			timeout(wcrstrt, (caddr_t)0,
			    (int)(*(unsigned char *)bp->b_rptr + 6));
			wscons.wc_flags |= WCS_DELAY;
			freemsg(bp);
			goto out;	/* wait for this to finish */

		case M_IOCTL:
			/*
			 * This ioctl was waiting for the output ahead of
			 * it to drain; obviously, it has.  Do it, and
			 * then grab the next message after it.
			 */
			wcioctl(q, bp);
			continue;
		}

		if ((cc = bp->b_wptr - bp->b_rptr) == 0) {
			freemsg(bp);
			continue;
		}

		if (STDOUT_IS_FB) {
			/*
			 * Never do output here; we're at spl5
			 * and it takes forever.
			 */
			wscons.wc_flags |= WCS_BUSY;
			wscons.wc_pendc = -1;
			if (q->q_count > 128)	/* do it soon */
				softcall(wconsout, (caddr_t)0);
			else
				/* wait a bit */
				timeout(wconsout, (caddr_t)0, hz/30);
			putbq(q, bp);
			goto out;
		}

		/*
		 * The while is used here to remove the possiblity of
		 * dereferencing a null pointer if bp is NULL and
		 * prom_mayput() is 0.
		 */
		while (bp != NULL) {
			c = *bp->b_rptr++;
			cc--;
			while (cc <= 0) {
				nbp = bp;
				bp = bp->b_cont;
				freeb(nbp);
				if (bp == NULL)
					break;
				cc = bp->b_wptr - bp->b_rptr;
			}
			if (prom_mayput(c) != 0) {
				wscons.wc_flags |= WCS_BUSY;
				wscons.wc_pendc = c;
				timeout(wcopoll, (caddr_t)0, 1);
				if (bp != NULL)
					/* not done with this message yet */
					putbq(q, bp);
				goto out;
			}
		}
	}

out:
	(void) splx(s);
}

#define	MAXHIWAT	2000

/*
 * Output to frame buffer console
 * Called at software interrupt level
 * because it takes a long time to scroll
 */
static int
wconsout()
{
	static char obuf[MAXHIWAT];
	register u_char *cp;
	register int cc;
	register queue_t *q;
	register mblk_t *bp;
	mblk_t *nbp;
	register char *current_position;
	register int bytes_left;
	int s;

	if ((q = wscons.wc_ttycommon.t_writeq) == NULL)
		return;	/* not attached to a stream */

	s = spl1();

	/*
	 * Set up to copy up to MAXHIWAT bytes.
	 */
	current_position = &obuf[0];
	bytes_left = MAXHIWAT;
	while ((bp = getq(q)) != NULL) {
		if (bp->b_datap->db_type == M_IOCTL) {
			/*
			 * This ioctl was waiting for the output ahead of
			 * it to drain; obviously, it has.  Put it back
			 * so that "wcstart" can handle it, and transmit
			 * what we've got.
			 */
			putbq(q, bp);
			goto transmit;
		}

		do {
			cp = bp->b_rptr;
			cc = bp->b_wptr - cp;
			while (cc != 0) {
				if (bytes_left == 0) {
					/*
					 * Out of buffer space; put this
					 * buffer back on the queue, and
					 * transmit what we have.
					 */
					bp->b_rptr = cp;
					putbq(q, bp);
					goto transmit;
				}
				/*
				 * Strip the characters to 7 bits along the
				 * way; the PROM doesn't handle 8-bit
				 * characters.
				 */
				*current_position++ = (*cp++);
				cc--;
				bytes_left--;
			}
			nbp = bp;
			bp = bp->b_cont;
			freeb(nbp);
		} while (bp != NULL);
	}

transmit:
	if ((cc = MAXHIWAT - bytes_left) != 0) {
		(void) splx(s);
		prom_writestr(obuf, (unsigned)cc);
	}
	s = spl5();
	wscons.wc_flags &= ~WCS_BUSY;
	wcstart();
	(void) splx(s);
}

/*
 * Put procedure for lower read queue.
 * Pass everything up to queue above "upper half".
 */
static int
wclrput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register queue_t *upq;
	int s;

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		if (*mp->b_rptr == FLUSHW || *mp->b_rptr == FLUSHRW) {
			s = spl5();
			/*
			 * Flush our write queue.
			 * XXX doesn't flush M_DELAY.
			 */
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
		if ((upq = wscons.wc_ttycommon.t_readq) != NULL)
			putnext(q, mp);
		else
			freemsg(mp);
		break;

	case M_DATA:
		if ((upq = wscons.wc_ttycommon.t_readq) != NULL) {
			if (!canput(upq->q_next)) {
				ttycommon_qfull(&wscons.wc_ttycommon, q);
				freemsg(mp);
			} else
				putnext(upq, mp);
		} else
			freemsg(mp);
		break;

	default:
		freemsg(mp);	/* anything useful here? */
		break;
	}
}
