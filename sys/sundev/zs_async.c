#ifndef lint
static	char sccsid[] = "@(#)zs_async.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

/*
 *	Asynchronous protocol handler for Z8530 chips
 * 	Handles normal UNIX support for terminals & modems
 */
#include "zs.h"
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
#include <sys/reboot.h>

#include <machine/clock.h>
#include <machine/cpu.h>
#include <sun/consdev.h>
#include <sundev/mbvar.h>
#include <sundev/zsreg.h>
#include <sundev/zscom.h>
#include <debug/debug.h>

#ifndef	OPENPROMS
#define	prom_enter_mon()	montrap(*romp->v_abortent);
#else	OPENPROMS
#ifndef	MULTIPROCESSOR
extern void			prom_enter_mon();
#else	MULTIPROCESSOR
#define	prom_enter_mon()	mpprom_enter()
#endif	MULTIPROCESSOR
#endif	OPENPROMS

/*
 * Undefine DPRINTF to remove compiled debug code.
 * #define DPRINTF	if (debug_zsa); else prom_printf
 */
#ifdef	DPRINTF
static int debug_zsa = 1;
#endif	DPRINTF

#define	ZSWR1_INIT	(ZSWR1_SIE|ZSWR1_TIE|ZSWR1_RIE)

#define	ZS_ON		(ZSWR5_DTR|ZSWR5_RTS)
#define	ZS_OFF		0

#define	OUTLINE 0x80	/* minor device flag for dialin/out on same line */
#define	UNIT(x)	(minor(x)&~OUTLINE)

#define	ISPEED	B9600
#define	IFLAGS	(CS7|CREAD|PARENB)

#define	PCLK	(19660800/4)	/* basic clock rate for UARTs */
#define	ZSPEED(n)	ZSTimeConst(PCLK, n)
#define	NSPEED	16	/* max # of speeds */
u_short	zs_speeds[NSPEED] = {
	0,
	ZSPEED(50),
	ZSPEED(75),
	ZSPEED(110),
#ifdef lint
	ZSPEED(134),
#else
	ZSPEED(269/2),			/* XXX - This is sleazy */
#endif
	ZSPEED(150),
	ZSPEED(200),
	ZSPEED(300),
	ZSPEED(600),
	ZSPEED(1200),
	ZSPEED(1800),
	ZSPEED(2400),
	ZSPEED(4800),
	ZSPEED(9600),
	ZSPEED(19200),
	ZSPEED(38400),
};

/*
 * Communication between H/W level (6/12) interrupts
 * and software interrupts is done via the zsaline
 * structure. See <sundev/zsreg.h>.
 */

#ifdef	OPENPROMS
extern char *zssoftCAR;
extern struct zsaline *zsaline;
#else	OPENPROMS
extern char zssoftCAR[];
extern struct zsaline zsaline[];
#endif	OPENPROMS
extern int nzs;

int zsticks = 3;	/* polling frequency */
int zsasoftdtr = 0;	/* if nonzero, softcarrier raises dtr at attach */
int zsadtrlow = 3;	/* hold dtr low nearly this long on close */
int zsb134_weird = 0;	/* if set, old weird B134 behavior */

/*
 * Should be "void", not "int" - they return no values!
 */
static int	zsopen(/*queue_t *q, int dev, int flag, int sflag*/);
static int	zsclose(/*queue_t *q, int flag*/);
static int	zswput(/*queue_t *q, mblk_t *mp*/);

static struct module_info zsm_info = {
	0,
	"zs",
	0,
	INFPSZ,
	2048,
	128
};

static struct qinit zsrinit = {
	putq,
	NULL,
	zsopen,
	zsclose,
	NULL,
	&zsm_info,
	NULL
};

static struct qinit zswinit = {
	zswput,
	NULL,
	NULL,
	NULL,
	NULL,
	&zsm_info,
	NULL
};

static char *zsmodlist[] = {
	"ldterm",
	"ttcompat",
	NULL
};

struct streamtab zsstab = {
	&zsrinit,
	&zswinit,
	NULL,
	NULL,
	zsmodlist
};

/*
 * The async zs protocol
 */
static int	zsa_attach(/*struct zscom *zs*/);
static int	zsa_txint(/*struct zscom *zs*/);
static int	zsa_xsint(/*struct zscom *zs*/);
static int	zsa_rxint(/*struct zscom *zs*/);
static int	zsa_srint(/*struct zscom *zs*/);
static int	zsa_softint(/*struct zscom *zs*/);

struct zsops zsops_async = {
	zsa_attach,
	zsa_txint,
	zsa_xsint,
	zsa_rxint,
	zsa_srint,
	zsa_softint,
};

static int	zsreioctl(/*long unit*/);
static void	zsioctl(/*struct zsaline *za, queue_t *q, mblk_t *mp*/);
static int	dmtozs(/*int bits*/);
static int	zstodm(/*int bits*/);
static void	zsparam(/*struct zsaline *za*/);
static int	zsrstrt(/*struct zsaline *za*/);
static void	zsstart(/*struct zsaline *za*/);
static void	zsresume(/*struct zsaline *za*/);
static int	zsmctl(/*struct zsaline *za, int bits, int how*/);
static int	zspoll(/*int direct*/);
static int	zsa_process(/*struct zsaline *za*/);

extern int	setcons(/*dev_t dev, u_short uid, u_short gid*/);
extern void	resetcons();

#ifdef	OPENPROMS

/*
 * Determine if zs->zs_unit is being used by the PROM.
 * To do this we need to map stdin/stdout devices as known
 * to the PROM to zs internal minor device (zs->zs_unit) numbers:
 *
 * PROM (real device)		zs minor	device
 *
 * "zs", 0, "a"			0		ttya
 * "zs", 0, "b"			1		ttyb
 * "zs", 1, "a"			2		keyboard
 * "zs", 1, "b"			3		mouse
 * "zs", 2, "a"			4		ttyc
 * "zs", 2, "b"			5		ttyd
 *
 * Thus, there's a straightforward mapping between stdin/stdout
 * zs unit/subdev <--> zs minors.
 */

static int
zs_is_stdin(zsminor)
	int	zsminor;
{
	char	path[OBP_MAXDEVNAME];
	char	*subunitname;
	int	subunit = 0;
	int	unit;
	extern	char *prom_get_stdin_subunit();

	if (prom_get_stdin_dev_name(path, OBP_MAXDEVNAME) == -1)
		return (0);

	if (strcmp("zs", path) != 0)
		return (0);

	if ((unit = prom_get_stdin_unit()) == -1)
		return (0);		/* XXX */

	if ((subunitname = prom_get_stdin_subunit()) != (char *)0)
		if (*subunitname != (char)0)
			subunit = *subunitname - 'a';

#ifdef	DPRINTF
	DPRINTF("zs_is_stdin: dev <%s> unit <%d> subunit <%d> zsminor <%d>\n",
		path, unit, subunit, zsminor);
#endif	DPRINTF

	return (zsminor == (unit << 1) + subunit);	/* as per above */
}

static int
zs_is_stdout(zsminor)
	int	zsminor;
{
	char	path[OBP_MAXDEVNAME];
	char	*subunitname;
	int	subunit = 0;
	int	unit;
	extern	char *prom_get_stdout_subunit();

	if (prom_get_stdout_dev_name(path, OBP_MAXDEVNAME) == -1)
		return (0);

	if (strcmp("zs", path) != 0)
		return (0);

	if ((unit = prom_get_stdout_unit()) == -1)
		return (0);		/* XXX */

	if ((subunitname = prom_get_stdout_subunit()) != (char *)0)
		if (*subunitname != (char)0)
			subunit = *subunitname - 'a';

#ifdef	DPRINTF
	DPRINTF("zs_is_stdout: dev <%s> unit <%d> subunit <%d> zsminor <%d>\n",
		path, unit, subunit, zsminor);
#endif	DPRINTF

	return (zsminor == (unit << 1) + subunit);	/* as per above */
}
#else	OPENPROMS

static int
zs_is_stdin(zsminor)
	int	zsminor;
{
	int	unit;

	/*
	 * from the original code.  They're mapping zs minor numbers to
	 * v_insource/v_outsink numbers:
	 * zs minor:	a, b, kbd, mouse, c, d
	 * PROM:	screen, a, b, c, d
	 */
	unit = (zsminor < 2) ? zsminor +1: (zsminor < 4) ? 0 : zsminor -1;
	return (unit == (*romp->v_insource));
}

static int
zs_is_stdout(zsminor)
	int zsminor;
{
	int unit;

	unit = (zsminor < 2) ? zsminor +1: (zsminor < 4) ? 0 : zsminor -1;
	return (unit == (*romp->v_outsink));
}

#endif	OPENPROMS


static int
zsa_attach(zs)
	register struct zscom *zs;
{
	register struct zsaline *za;
	register int un;

#ifdef	OPENPROMS
	if (zsaline == NULL) {
		zsaline = (struct zsaline *)new_kmem_zalloc(
			(u_int) (nzs * sizeof (struct zsaline)), KMEM_SLEEP);
	}
#endif
	za = &zsaline[zs->zs_unit];
	za->za_common = zs;
	za->za_dtrlow = time.tv_sec - zsadtrlow;	/* DTR has been low */
	if (zssoftCAR[zs->zs_unit])
		za->za_ttycommon.t_flags |= TS_SOFTCAR;

	/*
	 * Raise modem control lines on serial ports associated
	 * with the console and (optionally) softcarrier lines.
	 * Drop modem control lines on all others so that modems
	 * will not answer and portselectors will skip these
	 * lines until they are opened by a getty.
	 */

	un = zs->zs_unit;
	if (zs_is_stdin(un) || zs_is_stdout(un))  {	/* active in rom? */
#ifdef	DPRINTF
		DPRINTF("zs_unit <%d> active in PROM\n", un);
#endif	DPRINTF
		(void) zsmctl(za, ZS_ON, DMSET);	/* raise dtr */
	}  else if (zsasoftdtr && (za->za_ttycommon.t_flags & TS_SOFTCAR))  {
#ifdef	DPRINTF
		DPRINTF("zs_unit <%d> NOT active in PROM\n", un);
#endif	DPRINTF
		(void) zsmctl(za, ZS_ON, DMSET);	/* raise dtr */
	}  else  {
#ifdef	DPRINTF
		DPRINTF("zs_unit <%d> NOT active in PROM\n", un);
#endif	DPRINTF
		(void) zsmctl(za, ZS_OFF, DMSET);	/* drop dtr */
	}

#ifdef lint
	if (nzs == 0)
		nzs = 1;
#endif lint
}

/*
 * Get the current speed of the console and turn it into something
 * UNIX knows about - used to preserve console speed when UNIX comes up.
 */
int
zsgetspeed(dev)
	dev_t dev;
{
	register struct zsaline *za;
	struct zscom *zs;
	int uspeed, zspeed;

	za = &zsaline[UNIT(dev)];
	if (za->za_common == 0)
		return (ENXIO);
	zs = za->za_common;
	zspeed = ZREAD(12);
	zspeed |= ZREAD(13) << 8;
	for (uspeed = 0; uspeed < NSPEED; uspeed++)
		if (zs_speeds[uspeed] == zspeed)
			return (uspeed);
	/* 9600 baud if we can't figure it out */
	return (ISPEED);
}

/*ARGSUSED*/
static int
zsopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t dev;
{
	register int unit;
	register struct zsaline *za;
	struct zscom *zs;
	int s;
	static int first = 1;
	int speed;

	unit = UNIT(dev);
	if (unit >= nzs)
		return (OPENFAIL);	/* device not configured in */

	za = &zsaline[unit];
	if ((zs = za->za_common) == NULL)
		return (OPENFAIL);	/* device not found by autoconfig */

	s = splzs();
	zs->zs_priv = (caddr_t)za;
	zsopinit(zs, &zsops_async);
	(void) splx(s);
	if (first) {
		first = 0;
		timeout(zspoll, (caddr_t)0, zsticks);
	}

	/*
	 * Block waiting for carrier to come up, unless this is a no-delay
	 * open.
	 */
	s = spl5();
again:
	if (!(za->za_flags & ZAS_ISOPEN)) {
		/* clear any stale input */
		(void) splzs();
		ZS_RING_INIT(za);
		za->za_overrun = 0;
		(void) spl5();
		if (dev == rconsdev || dev == kbddev)
			speed = zsgetspeed(dev);
		else
			speed = ISPEED;
		za->za_ttycommon.t_iflag = 0;
		za->za_ttycommon.t_cflag = (speed << IBSHIFT)|speed|IFLAGS;
		za->za_ttycommon.t_iocpending = NULL;
		za->za_ttycommon.t_size.ws_row = 0;
		za->za_ttycommon.t_size.ws_col = 0;
		za->za_ttycommon.t_size.ws_xpixel = 0;
		za->za_ttycommon.t_size.ws_ypixel = 0;
		za->za_dev = dev;
		za->za_wbufcid = 0;
		zsparam(za);
	} else if (za->za_ttycommon.t_flags & TS_XCLUDE && u.u_uid != 0) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	} else if ((dev & OUTLINE) && !(za->za_flags & ZAS_OUT)) {
		(void) splx(s);
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	(void) zsmctl(za, ZS_ON, DMSET);
	if (dev & OUTLINE)
		za->za_flags |= ZAS_OUT;
	/*
	 * Check carrier.
	 */
	if ((za->za_ttycommon.t_flags & TS_SOFTCAR) ||
	    (zsmctl(za, 0, DMGET) & ZSRR0_CD))
		za->za_flags |= ZAS_CARR_ON;
	/*
	 * Unless DTR is held high by softcarrier, set HUPCL.
	 */
	if ((za->za_ttycommon.t_flags & TS_SOFTCAR) == 0)
		za->za_ttycommon.t_cflag |= HUPCL;
	/*
	 * If FNDELAY clear, block until carrier up. Quit on interrupt.
	 */
	if (!(flag & (FNBIO|FNONBIO|FNDELAY)) &&
	    !(za->za_ttycommon.t_cflag & CLOCAL)) {
		if (!(za->za_flags & (ZAS_CARR_ON|ZAS_OUT)) ||
		    ((za->za_flags & ZAS_OUT) && !(dev & OUTLINE))) {
			za->za_flags |= ZAS_WOPEN;
			if (sleep((caddr_t)&za->za_flags, STIPRI|PCATCH)) {
				za->za_flags &= ~ZAS_WOPEN;
				(void) zsmctl(za, ZS_OFF, DMSET);
				u.u_error = EINTR;
				(void) splx(s);
				return (OPENFAIL);
			}
			goto again;
		}
	} else {
		if (za->za_flags&ZAS_OUT && !(dev&OUTLINE)) {
			u.u_error = EBUSY;
			(void) splx(s);
			return (OPENFAIL);
		}
	}
	(void) splx(s);

	za->za_ttycommon.t_readq = q;
	za->za_ttycommon.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t)za;
	za->za_flags &= ~ZAS_WOPEN;
	za->za_flags |= ZAS_ISOPEN;
	return (dev);
}

/*ARGSUSED*/
static int
zsclose(q, flag)
	register queue_t *q;
	int flag;
{
	register struct zsaline *za;
	register struct zscom *zs;
	int	s;

	if ((za = (struct zsaline *)q->q_ptr) == NULL)
		return;		/* already been closed once */

	/*
	 * If we redirected the console here, put it back.
	 */
	if ((consdev == za->za_dev) && (consdev != rconsdev))
		resetcons();

	s = splzs();
	zs = za->za_common;

	/*
	 * If we still have carrier, wait here until all the data is gone;
	 * if interrupted in close, ditch the data and continue onward.
	 */
	while ((za->za_flags & ZAS_CARR_ON) && ((za->za_ocnt > 0) ||
		(za->za_flags & (ZAS_BUSY|ZAS_DELAY|ZAS_BREAK))))
		if (sleep((caddr_t)&lbolt, STOPRI|PCATCH))
			break;

	/*
	 * If break is in progress, stop it.
	 */
	if (zs->zs_wreg[5] & ZSWR5_BREAK) {
		za->za_flags &= ~ZAS_BREAK;
		ZBIC(5, ZSWR5_BREAK);
	}

	za->za_ocnt = 0;

	/*
	 * If line has HUPCL set, has changed the status of TS_SOFTCAR,
	 *     or is incompletely opened,
	 * *and* it is not the console or the keyboard,
	 * fix up the modem lines.
	 */
	if ((za->za_dev != rconsdev) && (za->za_dev != kbddev) &&
	    (((za->za_flags & (ZAS_WOPEN|ZAS_ISOPEN)) != ZAS_ISOPEN) ||
		(za->za_ttycommon.t_cflag & HUPCL) ||
		(za->za_flags & ZAS_SOFTC_ATTN))) {
		/*
		 * If DTR is being held high by softcarrier,
		 * set up the ZS_ON set; if not, hang up.
		 */
		if (za->za_ttycommon.t_flags & TS_SOFTCAR)
			(void) zsmctl(za, ZS_ON, DMSET);
		else
			(void) zsmctl(za, ZS_OFF, DMSET);
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
	if ((za->za_flags & (ZAS_ISOPEN|ZAS_WOPEN)) == 0) {
		(void) splzs();
		ZBIC(1, ZSWR1_RIE);
		(void) spl5();
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
	q->q_ptr = WR(q)->q_ptr = NULL;
	wakeup((caddr_t)&za->za_flags);
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
zswput(q, mp)
	register queue_t *q;
	register mblk_t *mp;
{
	register struct zsaline *za;
	register int s;

	za = (struct zsaline *)q->q_ptr;

	switch (mp->b_datap->db_type) {

	case M_STOP:
		/*
		 * Since we don't do real DMA, we can just let the
		 * chip coast to a stop after applying the brakes.
		 */
		za->za_flags |= ZAS_STOPPED;
		freemsg(mp);
		break;

	case M_START:
		s = splzs();
		if (za->za_flags & ZAS_STOPPED) {
			za->za_flags &= ~ZAS_STOPPED;
			/*
			 * If an output operation is in progress,
			 * resume it.  Otherwise, prod the start
			 * routine.
			 */
			if (za->za_ocnt > 0)
				zsresume(za);
			else
				zsstart(za);
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
			 * "zsstart" will see it when it's done
			 * with the output before it.  Poke the
			 * start routine, just in case.
			 */
			putq(q, mp);
			zsstart(za);
			break;

		default:
			/*
			 * Do it now.
			 */
			zsioctl(za, q, mp);
			break;
		}
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW) {
			s = splzs();
			/*
			 * Abort any output in progress.
			 */
			if (za->za_flags & ZAS_BUSY)
				za->za_ocnt = 0;
			(void) spl5();
			/*
			 * Flush our write queue.
			 */
			flushq(q, FLUSHDATA);	/* XXX doesn't flush M_DELAY */
			(void) splx(s);
			*mp->b_rptr &= ~FLUSHW;	/* it has been flushed */
		}
		if (*mp->b_rptr & FLUSHR) {
			s = spl5();	/* XXX - splzs? */
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
		zsstart(za);
		break;

	case M_BREAK:
	case M_DELAY:
	case M_DATA:
		/*
		 * Queue the message up to be transmitted,
		 * and poke the start routine.
		 */
		putq(q, mp);
		zsstart(za);
		break;

	case M_STOPI:
		s = splzs();
		za->za_flowc = za->za_ttycommon.t_stopc;
		zsstart(za);	/* poke the start routine */
		(void) splx(s);
		freemsg(mp);
		break;

	case M_STARTI:
		s = splzs();
		za->za_flowc = za->za_ttycommon.t_startc;
		zsstart(za);	/* poke the start routine */
		(void) splx(s);
		freemsg(mp);
		break;

	case M_CTL:
		switch (*mp->b_rptr) {

		case MC_SERVICEIMM:
			za->za_flags |= ZAS_SERVICEIMM;
			break;

		case MC_SERVICEDEF:
			za->za_flags &= ~ZAS_SERVICEIMM;
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
zsreioctl(unit)
	long unit;
{
	register struct zsaline *za = &zsaline[unit];
	queue_t *q;
	register mblk_t *mp;

	/*
	 * The bufcall is no longer pending.
	 */
	za->za_wbufcid = 0;
	if ((q = za->za_ttycommon.t_writeq) == NULL)
		return;
	if ((mp = za->za_ttycommon.t_iocpending) != NULL) {
		/* not pending any more */
		za->za_ttycommon.t_iocpending = NULL;
		zsioctl(za, q, mp);
	}
}

/*
 * Process an "ioctl" message sent down to us.
 */
static void
zsioctl(za, q, mp)
	struct zsaline *za;
	queue_t *q;
	register mblk_t *mp;
{
	register struct iocblk *iocp;
	register unsigned datasize;
	int error;
	int s;

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
	if ((datasize=ttycommon_ioctl(&za->za_ttycommon, q, mp, &error)) != 0) {
		if (za->za_wbufcid)
			unbufcall(za->za_wbufcid);
		za->za_wbufcid = bufcall(datasize, BPRI_HI, zsreioctl,
		    (long)za->za_common->zs_unit);
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
			zsparam(za);
			break;
		/*
		 * Someone may have changed the status of the soft carrier.
		 * Check the modem lines when closing.
		 */
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
			if (consdev == za->za_dev)
				;
			else if (rconsdev == za->za_dev)
				resetcons ();
			else if ((UNIT (za->za_dev) != 2) &&
			    (UNIT (za->za_dev) != 3)) {
				error = setcons (za->za_dev,
				    iocp->ioc_uid, iocp->ioc_gid);
			} else
				error = ENOTTY;
			break;

		case TCSBRK: {
			register struct zscom *zs;

			if (*(int *)mp->b_cont->b_rptr == 0) {
				zs = za->za_common;
				s = splzs();
				/*
				 * Set the break bit, and arrange for "zsrstrt"
				 * to be called in 1/4 second; it will turn the
				 * break bit off, and call "zsstart" to grab
				 * the next message.
				 */
				ZBIS(5, ZSWR5_BREAK);
				timeout(zsrstrt, (caddr_t)za, hz/4);
				za->za_flags |= ZAS_BREAK;
				(void) splx(s);
			}
			break;
		}

		case TIOCSBRK: {
			register struct zscom *zs = za->za_common;

			s = splzs();
			ZBIS(5, ZSWR5_BREAK);
			(void) splx(s);
			break;
		}

		case TIOCCBRK: {
			register struct zscom *zs = za->za_common;

			s = splzs();
			ZBIC(5, ZSWR5_BREAK);
			(void) splx(s);
			break;
		}

		case TIOCMSET:
			(void) zsmctl(za, dmtozs(*(int *)mp->b_cont->b_rptr),
			    DMSET);
			break;

		case TIOCMBIS:
			(void) zsmctl(za, dmtozs(*(int *)mp->b_cont->b_rptr),
			    DMBIS);
			break;

		case TIOCMBIC:
			(void) zsmctl(za, dmtozs(*(int *)mp->b_cont->b_rptr),
			    DMBIC);
			break;

		case TIOCMGET:
			*(int *)mp->b_cont->b_rptr =
			    zstodm(zsmctl(za, 0, DMGET));
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
dmtozs(bits)
	register int bits;
{
	register int b = 0;

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
zstodm(bits)
	register int bits;
{
	register int b;

	b = 0;
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

/*
 * Set the parameters of the line based on the values of the "c_iflag"
 * and "c_cflag" fields supplied to us.
 */
static void
zsparam(za)
	register struct zsaline *za;
{
	int s = splzs();
	register struct zscom *zs = za->za_common;
	register int wr1, wr3, wr4, wr5, speed;
	int baudrate;
	int loops;
	char c;

#ifdef lint
	c = 0;
	c = c;
#endif

	if ((baudrate = za->za_ttycommon.t_cflag&CBAUD) == 0) {
		/* Hang up line. */
		(void) zsmctl(za, ZS_OFF, DMSET);
		(void) splx(s);
		return;
	}

	wr1 = ZSWR1_INIT;
	/*
	 * Do not allow the console/keyboard device to have its receiver
	 * disabled; doing that would mean you couldn't type an abort
	 * sequence.
	 */
	if (za->za_dev == rconsdev || za->za_dev == kbddev ||
	    za->za_ttycommon.t_cflag & CREAD)
		wr3 = ZSWR3_RX_ENABLE;
	else
		wr3 = 0;
	wr4 = ZSWR4_X16_CLK;
	wr5 = (zs->zs_wreg[5] & (ZSWR5_RTS|ZSWR5_DTR)) | ZSWR5_TX_ENABLE;
	if (zsb134_weird && baudrate == B134) {	/* what a joke! */
		/*
		 * XXX - should B134 set all this crap in the compatibility
		 * module, leaving this stuff fairly clean?
		 */
		wr1 |= ZSWR1_PARITY_SPECIAL;
		wr3 |= ZSWR3_RX_6;
		wr4 |= ZSWR4_PARITY_ENABLE | ZSWR4_PARITY_EVEN;
		wr4 |= ZSWR4_1_5_STOP;
		wr5 |= ZSWR5_TX_6;
	} else {
		switch (za->za_ttycommon.t_cflag&CSIZE) {

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

		if (za->za_ttycommon.t_cflag&PARENB) {
			/*
			 * The PARITY_SPECIAL bit causes a special rx
			 * interrupt on parity errors.  Turn it on iff
			 * we're checking the parity of characters.
			 */
			if (za->za_ttycommon.t_iflag&INPCK)
				wr1 |= ZSWR1_PARITY_SPECIAL;
			wr4 |= ZSWR4_PARITY_ENABLE;
			if (!(za->za_ttycommon.t_cflag&PARODD))
				wr4 |= ZSWR4_PARITY_EVEN;
		}
		wr4 |= (za->za_ttycommon.t_cflag&CSTOPB) ?
				ZSWR4_2_STOP : ZSWR4_1_STOP;
	}
	if (za->za_ttycommon.t_cflag & CRTSCTS)
		wr3 |= ZSWR3_AUTO_CD_CTS; /* auto RTS/CTS flow control */
	speed = zs->zs_wreg[12] + (zs->zs_wreg[13] << 8);
	if (wr1 != zs->zs_wreg[1] || wr3 != zs->zs_wreg[3] ||
	    wr4 != zs->zs_wreg[4] || wr5 != zs->zs_wreg[5] ||
	    speed != zs_speeds[baudrate]) {
		/*
		 * Wait for that last damn character to get out the
		 * door.  At most 1000 loops of 100 usec each is worst
		 * case of 110 baud.  Turn on the ZAS_DRAINING bit so
		 * that when that character is done the next character
		 * won't follow it.
		 * XXX - doing it here will hang the system while it's
		 * happening (since it's inside the streams code), but
		 * 1) it's only 1/10 second and 2) it doesn't happen
		 * that often.
		 */
		za->za_flags |= ZAS_DRAINING;
		loops = 1000;
		while ((ZREAD(1) & ZSRR1_ALL_SENT) == 0 && --loops > 0) {
			(void) splx(s);
			DELAY(100);
			s = splzs();
		}
		ZWRITE(3, 0); /* disable receiver while setting parameters */
		zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
		ZSDELAY(2);
		zs->zs_addr->zscc_control = ZSWR0_RESET_ERRORS;
		ZSDELAY(2);
		c = zs->zs_addr->zscc_data; /* swallow junk */
		ZSDELAY(2);
		c = zs->zs_addr->zscc_data; /* swallow junk */
		ZSDELAY(2);
		c = zs->zs_addr->zscc_data; /* swallow junk */
		ZSDELAY(2);
		ZWRITE(1, wr1);
		ZWRITE(4, wr4);
		ZWRITE(3, wr3);
		ZWRITE(5, wr5);
		speed = zs_speeds[baudrate];
		ZWRITE(11, ZSWR11_TXCLK_BAUD + ZSWR11_RXCLK_BAUD);
		ZWRITE(14, ZSWR14_BAUD_FROM_PCLK);
		ZWRITE(12, speed);
		ZWRITE(13, speed >> 8);
		ZWRITE(14, ZSWR14_BAUD_ENA + ZSWR14_BAUD_FROM_PCLK);
		za->za_flags &= ~ZAS_DRAINING;
		if (za->za_ocnt > 0)
			zsresume(za);
	}
	(void) splx(s);
}

/*
 * Restart output on a line after a delay or break timer expired.
 */
static int
zsrstrt(za)
	register struct zsaline *za;
{
	register struct zscom *zs = za->za_common;
	int s;

	/*
	 * If break timer expired, turn off the break bit.
	 */
	if (za->za_flags & ZAS_BREAK) {
		s = splzs();
		ZBIC(5, ZSWR5_BREAK);
		(void) splx(s);
	}
	za->za_flags &= ~(ZAS_DELAY|ZAS_BREAK);
	zsstart(za);
}

/*
 * Start output on a line, unless it's busy, frozen, or otherwise.
 */
static void
zsstart(za)
	register struct zsaline *za;
{
	register struct zscom *zs = za->za_common;
	register int cc;
	register queue_t *q;
	register mblk_t *bp;
	u_char *xmit_addr;
	register int s;

	s = spl5();

	/*
	 * If the chip is busy (i.e., we're waiting for a break timeout
	 * to expire, or for the current transmission to finish, or for
	 * output to finish draining from chip), don't grab anything new.
	 */
	if (za->za_flags & (ZAS_BREAK|ZAS_BUSY|ZAS_DRAINING))
		goto out;

	/*
	 * If we have a flow-control character to transmit, do it now.
	 */
	if (za->za_flowc != '\0') {
		(void) splzs();
		if (zs->zs_addr->zscc_control & ZSRR0_TX_READY) {
			ZSDELAY(2);
			zs->zs_addr->zscc_data = za->za_flowc;
			za->za_flowc = '\0';
		}
		goto out;
	}

	/*
	 * If we're waiting for a delay timeout to expire, don't grab
	 * anything new.
	 */
	if (za->za_flags & ZAS_DELAY)
		goto out;

	if ((q = za->za_ttycommon.t_writeq) == NULL)
		goto out;	/* not attached to a stream */

	for (;;) {
		if ((bp = getq(q)) == NULL)
			goto out;	/* no data to transmit */

		/*
		 * We have a message block to work on.
		 * Check whether it's a break, a delay, or an ioctl (the latter
		 * occurs if the ioctl in question was waiting for the output
		 * to drain).  If it's one of those, process it immediately.
		 */
		switch (bp->b_datap->db_type) {

		case M_BREAK:
			/*
			 * Set the break bit, and arrange for "zsrstrt"
			 * to be called in 1/4 second; it will turn the
			 * break bit off, and call "zsstart" to grab
			 * the next message.
			 */
			ZBIS(5, ZSWR5_BREAK);
			timeout(zsrstrt, (caddr_t)za, hz/4); /* 0.25 seconds */
			za->za_flags |= ZAS_BREAK;
			freemsg(bp);
			goto out;	/* wait for this to finish */

		case M_DELAY:
			/*
			 * Arrange for "zsrstrt" to be called when the
			 * delay expires; it will turn MTS_DELAY off,
			 * and call "zsstart" to grab the next message.
			 */
			timeout(zsrstrt, (caddr_t)za,
			    (int)(*(unsigned char *)bp->b_rptr + 6));
			za->za_flags |= ZAS_DELAY;
			freemsg(bp);
			goto out;	/* wait for this to finish */

		case M_IOCTL:
			/*
			 * This ioctl was waiting for the output ahead of
			 * it to drain; obviously, it has.  Do it, and
			 * then grab the next message after it.
			 */
			zsioctl(za, q, bp);
			continue;
		}

		if ((cc = bp->b_wptr - bp->b_rptr) > 0)
			break;

		freemsg(bp);
	}

	/*
	 * We have data to transmit.  If output is stopped, put
	 * it back and try again later.
	 */
	if (za->za_flags & ZAS_STOPPED) {
		putbq(q, bp);
		goto out;
	}

	za->za_xmitblk = bp;
	xmit_addr = bp->b_rptr;
	bp = bp->b_cont;
	if (bp != NULL)
		putbq(q, bp);	/* not done with this message yet */

	/*
	 * Set up this block for pseudo-DMA.
	 */
	(void) splzs();
	za->za_optr = xmit_addr;
	za->za_ocnt = cc;
	if ((za->za_ttycommon.t_cflag & CSIZE) == CS5) 
		*za->za_optr &= 037;
	/*
	 * If the transmitter is ready, shove the first
	 * character out.
	 */
	if (zs->zs_addr->zscc_control & ZSRR0_TX_READY) {
		ZSDELAY(2);

		zs->zs_addr->zscc_data = *za->za_optr++;
		/* XXX we assume there is enough time here for recovery */
		za->za_ocnt--;
	}
	za->za_flags |= ZAS_BUSY;

out:
	(void) splx(s);
}

/*
 * Resume output by poking the transmitter.
 */
static void
zsresume(za)
	register struct zsaline *za;
{
	register struct zscc_device *zsaddr = za->za_common->zs_addr;

	if (zsaddr->zscc_control & ZSRR0_TX_READY) {
		ZSDELAY(2);
		if (za->za_flowc != '\0') {
			zsaddr->zscc_data = za->za_flowc;
			za->za_flowc = '\0';
		} else if (za->za_ocnt > 0) {
			zsaddr->zscc_data = *za->za_optr++;
			za->za_ocnt--;
		}
	}
}

/*
 * Set or get the modem control status.
 */
static int
zsmctl(za, bits, how)
	struct zsaline *za;
	int bits, how;
{
	register struct zscom *zs = za->za_common;
	register int mbits, obits, s, now, held;

again:
	s = splzs();
	mbits = zs->zs_wreg[5] & (ZSWR5_RTS|ZSWR5_DTR);
	zs->zs_addr->zscc_control = ZSWR0_RESET_STATUS;
	ZSDELAY(2);
	mbits |= zs->zs_addr->zscc_control & (ZSRR0_CD|ZSRR0_CTS);
	ZSDELAY(2);
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
	if ((mbits & ~obits & ZSWR5_DTR) && (held < zsadtrlow)) {
		(void) splx(s);
		(void) sleep((caddr_t)&lbolt, PZERO-1);
		goto again;
	}

	zs->zs_wreg[5] &= ~(ZSWR5_RTS|ZSWR5_DTR);
	ZBIS(5, mbits & (ZSWR5_RTS|ZSWR5_DTR));
	(void) splx(s);
	return (mbits);
}

/*
 * Transmitter interrupt service routine.
 * If there's more data to transmit in the current pseudo-DMA block,
 * and the transmitter is ready, send the next character if output
 * is not stopped or draining.
 * Otherwise, queue up a soft interrupt.
 */
static int
zsa_txint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;

	if (zsaddr->zscc_control & ZSRR0_TX_READY) {
		if (za->za_flowc != '\0') {
			ZSDELAY(2);
			if (!(za->za_flags & ZAS_DRAINING)) {
				zsaddr->zscc_data = za->za_flowc;
				za->za_flowc = '\0';
			} else
				zsaddr->zscc_control = ZSWR0_RESET_TXINT;
			return;
		} else if (za->za_ocnt > 0) {
			ZSDELAY(2);
			if (!(za->za_flags & (ZAS_STOPPED|ZAS_DRAINING))) {
				zsaddr->zscc_data = *za->za_optr++;
				za->za_ocnt--;
			} else
				zsaddr->zscc_control = ZSWR0_RESET_TXINT;
			return;
		}
	}
	ZSDELAY(2);
	za->za_work++;
	zsaddr->zscc_control = ZSWR0_RESET_TXINT;
	ZSSETSOFT(zs);
}

/*
 * External/Status interrupt.
 */
static int
zsa_xsint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register u_char s0, x0, c;

	s0 = zsaddr->zscc_control;
	ZSDELAY(2);
	x0 = s0 ^ za->za_rr0;
	za->za_rr0 = s0;
	zsaddr->zscc_control = ZSWR0_RESET_STATUS;
	if ((x0 & ZSRR0_BREAK) && (s0 & ZSRR0_BREAK) == 0) {
		/*
		 * ZSRR0_BREAK turned off.  This means that the break sequence
		 * has completed (i.e., the stop bit finally arrived).  The NUL
		 * character in the receiver is part of the break sequence;
		 * it is discarded.
		 */
		za->za_break++;
		ZSDELAY(2);
		c = zsaddr->zscc_data; /* swallow null */
		ZSDELAY(2);
#ifdef lint
		c = c;
#endif
		zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
		ZSDELAY(2);
		/*
		 * Note: this will cause an abort if a break occurs on
		 * the "keyboard device", regardless of whether the
		 * "keyboard device" is a real keyboard or just a
		 * terminal on a serial line.  This permits you to
		 * abort a workstation by unplugging the keyboard,
		 * even if the normal abort key sequence isn't working.
		 */
		if (za->za_dev == kbddev) {
			if (boothowto & RB_DEBUG) {
#ifdef sparc
				extern int	kadb_defer, kadb_want;

				if (kadb_defer)
					kadb_want = 1;
				else
#endif
				{
					/* We're at splzs already */
					/* save pc and sp for kadb */
					(void) setjmp(&u.u_pcb.pcb_regs);
					CALL_DEBUG();
				}
			} else {
				prom_enter_mon();
			}
		}
	}
	za->za_work++;
	za->za_ext++;
	ZSSETSOFT(zs);
}

/*
 * Receiver interrupt.  Try to put the character into the circular buffer
 * for this line; if it overflows, indicate a circular buffer overrun.
 * If this port is always to be serviced immediately, or the character
 * is a STOP character, or more than 15 characters have arrived, queue up
 * a soft interrupt to drain the circular buffer.
 */
static int
zsa_rxint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register u_char c;

	c = zsaddr->zscc_data;
	if (c == 0 && (za->za_rr0 & ZSRR0_BREAK)) {
		/*
		 * A break sequence was under way, and a NUL character was
		 * received.  Discard the NUL character, as it is part of the
		 * break sequence; if ZSRR0_BREAK turned off, indicating that
		 * the break sequence has completed, call "zsa_xsint" to
		 * properly handle the error - it would appear that
		 * External/Status interrupts get lost occasionally, so this
		 * ensures that one is delivered.
		 */
		if (!(ZREAD(0) & ZSRR0_BREAK))
			zsa_xsint(zs);
		return;
	}
	/* XXX - handle framing errors?  How to tell them from BREAKs? */
	if ((za->za_ttycommon.t_iflag & PARMRK) &&
	    !(za->za_ttycommon.t_iflag & (IGNPAR|ISTRIP)) && c == 0377) {
		if (ZS_RING_POK(za, 2)) {
			ZS_RING_PUT(za, 0377);
			ZS_RING_PUT(za, c);
		} else
			za->za_sw_overrun = 1;
	} else {
		if (ZS_RING_POK(za, 1))
			ZS_RING_PUT(za, c);
		else
			za->za_sw_overrun = 1;
	}
	za->za_work++;
	if ((za->za_flags & ZAS_SERVICEIMM) ||
	    ((za->za_ttycommon.t_iflag & IXON) &&
		((c & 0177) == za->za_ttycommon.t_stopc)) ||
	    (ZS_RING_FRAC(za)))
		ZSSETSOFT(zs);
}

/*
 * Special receive condition interrupt handler.
 */
static int
zsa_srint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;
	register struct zscc_device *zsaddr = zs->zs_addr;
	register short s1;
	register u_char c;

	s1 = ZREAD(1);
	ZSDELAY(2);
	c = zsaddr->zscc_data;	/* swallow bad char */
	ZSDELAY(2);
	if (s1 & ZSRR1_PE) {
		/*
		 * IGNPAR	PARMRK		RESULT
		 * off		off		0
		 * off		on		3 byte sequence
		 * on		either		ignored
		 */
		if (!(za->za_ttycommon.t_iflag & IGNPAR)) {
			/*
			 * The receive interrupt routine has already stuffed c
			 * into the ring.  Dig it out again, since the current
			 * mode settings don't allow it to appear in that
			 * position.
			 */
			if (ZS_RING_CNT(za) != 0)
				ZS_RING_UNPUT(za);
			if (za->za_ttycommon.t_iflag & PARMRK) {
				if (ZS_RING_POK(za, 3)) {
					ZS_RING_PUT(za, 0377);
					ZS_RING_PUT(za, 0);
					ZS_RING_PUT(za, c);
				} else
					za->za_sw_overrun = 1;
			} else {
				if (ZS_RING_POK(za, 1))
					ZS_RING_PUT(za, 0);
				else
					za->za_sw_overrun = 1;
			}
			za->za_work++;
			za->za_needsoft = 0;
			ZSSETSOFT(zs);
		} else {
			if (ZS_RING_CNT(za) != 0)
				ZS_RING_UNPUT(za);
		}
	}
	zsaddr->zscc_control = ZSWR0_RESET_ERRORS;
	ZSDELAY(2);
	if (s1 & ZSRR1_DO) {
		za->za_hw_overrun = 1;
		za->za_work++;
		ZSSETSOFT(zs);
	}
}

/*
 * Handle a software interrupt.
 */
static int
zsa_softint(zs)
	register struct zscom *zs;
{
	register struct zsaline *za = (struct zsaline *)zs->zs_priv;

	if (zsa_process(za))	/* true if too much work at once */
		zspoll(1);
	return (0);
}

/*
 * Poll for events in the zscom structures
 * This routine is called at level 1, we jack up to 3 to lock
 * out zsa_softint.
 */
static int
zspoll(direct)
	int direct;
{
	register struct zsaline *za;
	register short more;
	register int s;

	do {
		more = 0;
		for (za = &zsaline[0]; za < &zsaline[nzs]; za++) {
			if (za->za_work) {
				za->za_work = 0;
				s = spl3();
				if (zsa_process(za)) {
					za->za_work++;
					more++;
				}
				(void) splx(s);
			}
		}
	} while (more);
	if (!direct)
		timeout(zspoll, (caddr_t)0, zsticks);
}

/*
 * Process software interrupts (or poll)
 * Crucial points:
 * 1.	Check whether any output has been done immediately after handing
 *	input upstream, so that if IXOFF mode is set and this message
 *	pushed the queue above the high-water mark, the stop character has
 *	a chance of being sent before enough input arrives to exceed TTYHOG.
 *	This has happened in very busy systems.
 * 2.	At most 16 characters are passed upstream before the next line
 *	is serviced -- this "schedules" more fairly among lines.  (16
 *	characters fit into one 16-byte stream buffer.)
 * 3.	BUG - breaks are handled "out-of-band" - their relative position
 *	among input events is lost, as well as multiple breaks together.
 *	This is probably not a problem in practice.
 */
static int
zsa_process(za)
	register struct zsaline *za;
{
	register struct zscc_device *zsaddr = za->za_common->zs_addr;
	register short cc;
	register mblk_t *bp;
	register queue_t *q;
	int s;

	if (za->za_ext) {
		za->za_ext = 0;
		/* carrier up? */
		if ((zsaddr->zscc_control & ZSRR0_CD) ||
		    (za->za_ttycommon.t_flags & TS_SOFTCAR)) {
			/* carrier present */
			if ((za->za_flags&ZAS_CARR_ON) == 0) {
				za->za_flags |= ZAS_CARR_ON;
				if ((q = za->za_ttycommon.t_readq) != NULL)
					(void) putctl(q->q_next, M_UNHANGUP);
				wakeup((caddr_t)&za->za_flags);
			}
		} else {
			if ((za->za_flags&ZAS_CARR_ON) &&
			    !(za->za_ttycommon.t_cflag&CLOCAL)) {
				/*
				 * Carrier went away.
				 * Drop DTR, abort any output in progress,
				 * indicate that output is not stopped, and
				 * send a hangup notification upstream.
				 */
				(void) zsmctl(za, ZSWR5_DTR, DMBIC);
				s = splzs();
				if (za->za_flags & ZAS_BUSY)
					za->za_ocnt = 0;
				za->za_flags &= ~ZAS_STOPPED;
				(void) splx(s);
				if ((q = za->za_ttycommon.t_readq) != NULL)
					(void) putctl(q->q_next, M_HANGUP);
			}
			za->za_flags &= ~ZAS_CARR_ON;
		}
	}
	if (za->za_break &&
	    (zsaddr->zscc_control & ZSRR0_BREAK) == 0) {
		za->za_break = 0;
		if ((q = za->za_ttycommon.t_readq) != NULL)
			(void) putctl(q->q_next, M_BREAK);
	}
	/*
	 * If data has been added to the circular buffer, remove
	 * it from the buffer, and send it up the stream if there's
	 * somebody listening. Try to do it 16 bytes at a time. If we
	 * have more than 16 bytes to move, move 16 byte chunks and
	 * leave the runt for next time around (maybe it will grow).
	 */
	if (q = za->za_ttycommon.t_readq) {
		if ((cc = ZS_RING_CNT(za)) > 0) {
			if (cc > 16)
				cc = 16;
			if (bp = allocb(cc, BPRI_MED)) {
				if (canput(q->q_next)) {
					do {
						*bp->b_wptr++ = ZS_RING_GET(za);
					} while (--cc);
					putnext(q, bp);
				} else {
					ttycommon_qfull(&za->za_ttycommon, q);
					freemsg(bp);
				}
			}
			ZS_RING_EAT(za, cc);
		}
	} else
		ZS_RING_INIT(za);

	/*
	 * If a transmission has finished, indicate that it's
	 * finished, and start that line up again.
	 */
	if (za->za_ocnt <= 0 && (za->za_flags & ZAS_BUSY)) {
		freeb(za->za_xmitblk);
		za->za_flags &= ~ZAS_BUSY;
		zsstart(za);
	}

	/*
	 * A note about these overrun bits: all they do is *tell* someone
	 * about an error- They do not track multiple errors. In fact,
	 * you could consider them latched register bits if you like.
	 *
	 * They do not have to be protected by an spl, as we are only
	 * interested in printing the error message once for any cluster
	 * of overrun errrors.
	 */
	if (za->za_hw_overrun) {
		if (za->za_flags & ZAS_ISOPEN)
			log(LOG_ERR, "zs%d: silo overflow\n",
			    UNIT(za->za_dev));
		za->za_hw_overrun = 0;
	}
	if (za->za_sw_overrun) {
		if (za->za_flags & ZAS_ISOPEN)
			log(LOG_ERR, "zs%d: ring buffer overflow\n",
			    UNIT(za->za_dev));
		za->za_sw_overrun = 0;
	}
	return (ZS_RING_CNT(za));
}
