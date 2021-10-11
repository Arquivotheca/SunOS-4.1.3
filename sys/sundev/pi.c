#ifndef lint
static	char sccsid[] = "@(#)pi.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Parallel Keyboard & Mouse drivers
 * Minor 0 is keyboard, 1 is mouse
 * Turns parallel data into a byte stream to be fed (usually)
 * into the keyboard or mouse streams modules
 */
#include "pi.h"
#include <sys/param.h>
#include <sys/conf.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/termios.h>
#include <sys/termio.h>
#include <sys/ttold.h>
#include <sys/stropts.h>
#include <sys/stream.h>
#include <sys/buf.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/uio.h>

#include <machine/mmu.h>
#include <machine/scb.h>
#include <sun/consdev.h>
#include <sundev/mbvar.h>
#include <sundev/kbd.h>

/*
 * Driver information for auto-configuration stuff.
 */
int	piprobe();
struct	mb_driver pidriver = {
	piprobe, 0, 0, 0, 0, 0,
	0, "pi", 0, 0, 0, 0,
};

/*
 * One structure for keyboard, one for mouse.
 */
struct pi_softc {
	int	pi_isopen;	/* 1 if open */
	struct pireg *pi_addr;	/* address of the interface */
	queue_t	*pi_readq;	/* read queue for this device */
	queue_t	*pi_writeq;	/* write queue for this device */
	mblk_t	*pi_iocpending;	/* ioctl reply pending successful allocation */
	int	pi_wbufcid;	/* id of pending write-side bufcall */
} pisoftc[2];

struct pireg {
	u_char	pi_kbd;
	char	pi_mouse;	/* signed mouse delta or buttons */
};
struct pireg *piaddr, pilast;	/* used by low level clock intr */

static int piopen();
static int piclose();
static int piwput();

static struct module_info pim_info = {
	0,
	"pi",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit pirinit = {
	NULL,
	NULL,
	piopen,
	piclose,
	NULL,
	&pim_info,
	NULL
};

static struct qinit piwinit = {
	piwput,
	NULL,
	NULL,
	NULL,
	NULL,
	&pim_info,
	NULL
};

static char *pimodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab piinfo = {
	&pirinit,
	&piwinit,
	NULL,
	NULL,
	pimodlist
};

static void piioctl();

/*
 * Probe for keyboard and mouse 
 * Diagnostics for unconnected keyboard or unpowered mouse
 */
piprobe(reg, unit)
	caddr_t reg;
	int unit;
{
	register struct pireg *pi;
	register int i;

	if (unit)
		return (0);
	if (peek((short *)reg) == -1)
		return (0);
	pi = (struct pireg *)reg;
	for (i=0; i<100000; i++)
		if (pi->pi_kbd != 0xFF)
			break;
	pisoftc[0].pi_addr = (struct pireg *)reg;
	pisoftc[1].pi_addr = (struct pireg *)reg;
	piaddr = pi;
	return (1);
}

/*
 * Open keyboard or mouse - enable call to piintr
 */
/*ARGSUSED*/
static int
piopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t dev;
	int flag;
{
	int clkpiscan();
	register struct pi_softc *sc;
	register struct pireg *pi;

	if (minor(dev) > 1)
		return (OPENFAIL);
	sc = &pisoftc[minor(dev)];
	if (sc->pi_addr == 0)
		return (OPENFAIL);
	pi = sc->pi_addr;
	if (minor(dev) == 0 && pi->pi_kbd == 0xFF)
		printf("keyboard not connected\n");
	if (minor(dev) == 1 && (pi->pi_mouse & 1) == 0)
		printf("no mouse power jumper on CPU parallel port\n");
	if (flag & FWRITE) {
		u.u_error = EINVAL;
		return (OPENFAIL);
	}
	scb.scb_autovec[5-1] = clkpiscan;
	start_piscan_clock();
	sc->pi_iocpending = NULL;
	sc->pi_wbufcid = 0;
	sc->pi_readq = q;
	sc->pi_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t)sc;
	sc->pi_isopen = 1;
	return (dev);
}

/*
 * Close keyboard or mouse 
 * We never disable clkpiscan because its too hard to synchronize
 * all the opens and closes
 */
/*ARGSUSED*/
static int
piclose(q)
	register queue_t *q;
{
	register struct pi_softc *sc;
	register mblk_t *mp;

	sc = (struct pi_softc *)q->q_ptr;
	sc->pi_isopen = 0;
	/*
	 * Release pending "ioctl".
	 */
	if ((mp = sc->pi_iocpending) != NULL) {
		sc->pi_iocpending = NULL;
		freemsg(mp);
	}
	/*
	 * Cancel outstanding "bufcall" request.
	 */
	if (sc->pi_wbufcid) {
		unbufcall(sc->pi_wbufcid);
		sc->pi_wbufcid = 0;
	}
	q->q_ptr = WR(q)->q_ptr = NULL;
}

/*
 * Put procedure for write queue.
 * Respond to M_IOCTL and M_FLUSH messages here; discard everything else.
 */
static int
piwput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct pi_softc *sc;
	register int s;

	sc = (struct pi_softc *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_IOCTL:
		piioctl(sc, q, mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			*mp->b_rptr &= ~FLUSHW;	/* nothing to flush */
		if (*mp->b_rptr & FLUSHR) {
			s = spl5();
			flushq(RD(q), FLUSHDATA);
			(void) splx(s);
			qreply(q, mp);	/* give the read queues a crack at it */
		} else
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
pireioctl(unit)
	long unit;
{
	register struct pi_softc *sc = &pisoftc[unit];
	queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	sc->pi_wbufcid = 0;
	if ((q = sc->pi_writeq) == NULL)
		return;
	if ((mp = sc->pi_iocpending) != NULL) {
		sc->pi_iocpending = NULL;	/* not pending any more */
		piioctl(sc, q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
/*ARGSUSED*/
static void
piioctl(sc, q, mp)
	register struct pi_softc *sc;
	queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	int s;
 
	if (sc->pi_iocpending != NULL) {
		/*
		 * We were holding an "ioctl" response pending the
		 * availability of an "mblk" to hold data to be passed up;
		 * another "ioctl" came through, which means that "ioctl"
		 * must have timed out or been aborted.
		 */
		freemsg(sc->pi_iocpending);
		sc->pi_iocpending = NULL;
	}

	iocp = (struct iocblk *)mp->b_rptr;

	/*
	 * Turn the ioctl message into an ioctl ACK message.
	 */
	mp->b_datap->db_type = M_IOCACK;

	switch (iocp->ioc_cmd) {

	case TCGETS: {
		register struct termios *cb;
		register mblk_t *datap;

		if ((datap = allocb(sizeof (struct termios), BPRI_HI)) == NULL)
			goto allocfailure;
		cb = (struct termios *)datap->b_wptr;
		/*
		 * The only information we supply is the cflag word.
		 * Our copy of the iflag word is just that, a copy.
		 */
		bzero((caddr_t) cb, sizeof (struct termios));
		cb->c_cflag = 0;	/* we don't have a speed */
		datap->b_wptr += (sizeof *cb)/(sizeof *datap->b_wptr);
		iocp->ioc_count = sizeof (struct termios);
		if (mp->b_cont != NULL)
			freemsg(mp->b_cont);
		mp->b_cont = datap;
		break;
	}

	case TCSETSF:
	case TCSETSW:
	case TCSETS:
		/*
		 * Do nothing, just ack, so the keyboard and mouse code
		 * above us doesn't have a canary.
		 */
		iocp->ioc_count = 0;
		break;

	default:
		/*
		 * Yeah, yeah, we know, we know, S5 returns EINVAL.
		 * So sue us.
		 */
		iocp->ioc_count = 0;
		iocp->ioc_error = ENOTTY;
		mp->b_datap->db_type = M_IOCNAK;
		break;
	}
	qreply(q, mp);
	return;

allocfailure:
	/*
	 * We needed to allocate something to handle this "ioctl", but
	 * couldn't; save this "ioctl" and arrange to get called back when
	 * it's more likely that we can get what we need.
	 * If there's already one being saved, throw it out, since it
	 * must have timed out.
	 * Call "bufcall" to request that we be called back when we
	 * stand a better chance of allocating the data.
	 */
	s = splstr();
	if (sc->pi_iocpending != NULL)
		freemsg(sc->pi_iocpending);
	sc->pi_iocpending = mp;	/* hold this ioctl */
	/*
	 * Arrange to get called back only if there isn't an outstanding
	 * "bufcall" request since the allocation size is constant.
	 */
	if (sc->pi_wbufcid == 0)
		sc->pi_wbufcid = bufcall(sizeof (struct termios), BPRI_HI,
		    pireioctl, (long)(sc - &pisoftc[0]));
	(void) splx(s);
}

/*
 * Mouse:
 * The high 4 bits have value 8 for a button sample,
 * otherwise the high 7 bits are a (signed) delta.
 * Since we can't tell the difference between a delta X
 * and a delta Y, we use the delta before the button sample as
 * the delta Y, and the delta after the button push as the delta X.
 * We re-arrange the order to button, deltaX, deltaY for the sake
 * of the common mouse code.
 */
piintr()
{
	register char m;
	register short limit = 10;
	short samp;
	register struct pireg *pi = piaddr;
	register struct pi_softc *sc;
	static char button, lastbutton, deltay = 0;
	static enum { DELY, BUTTON, DELX } state = DELY;
	register mblk_t *bp;

	/* read and wait for two reads to concur in case sample is changing */
	samp = *(short *)pi;
	while (--limit > 0) {
		if (samp == *(short *)pi)
			goto found;
		samp = *(short *)pi;
	}
	return;		/* hopeless confusion */

found:
	pi = (struct pireg *)&samp;
	if (pi->pi_kbd != pilast.pi_kbd) {
		pilast.pi_kbd = pi->pi_kbd;
		sc = &pisoftc[0];
		if (sc->pi_isopen) {
			if ((bp = allocb(1, BPRI_MED)) != NULL) {
				if (!canput(sc->pi_readq->q_next))
					freemsg(bp);
				else {
					*bp->b_wptr++ = pi->pi_kbd;
					putnext(sc->pi_readq, bp);
				}
			}
		}
	}
	if (pi->pi_mouse == pilast.pi_mouse)	/* wait for sample to change */
		return;
	pilast.pi_mouse = pi->pi_mouse;
	m = pi->pi_mouse;
	sc = &pisoftc[1];
	if (!(sc->pi_isopen))	/* mouse not active */
		return;
	if ((m & 0xf0) == 0x80) {
		state = BUTTON;
		button = 0x80 | ((m>>1)&7);
	} else
		m >>= 1;
	switch (state) {

	case DELY:		/* delta Y until we see buttons */
		deltay = m;
		break;

	case BUTTON:		/* delta X immediately after we see buttons */
		state = DELX;
		break;

	case DELX:
		state = DELY;
		if (button == lastbutton && m == 0 && deltay == 0)
			break;		/* no new info */
		lastbutton = button;
		if ((bp = allocb(5, BPRI_MED)) != NULL) {
			if (!canput(sc->pi_readq->q_next))
				freemsg(bp);
			else {
				*bp->b_wptr++ = button;
				*bp->b_wptr++ = m;
				*bp->b_wptr++ = deltay;
				/*
				 * M3 mice used on Sun-3s transmit a five-byte
				 * packet: button, dx, dy, dx, dy. Let's make
				 * our mice look as much like an M3 as
				 * possible, so the mouse input routine
				 * upstream won't choke. See sundev/ms.c for
				 * details.
				 */
				*bp->b_wptr++ = '\0';
				*bp->b_wptr++ = '\0';
				putnext(sc->pi_readq, bp);
			}
		}
		deltay = m;	/* in case next delta y equals this delta x */
		break;
	}
}
