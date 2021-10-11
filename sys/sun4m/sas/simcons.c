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

#include <sun/autoconf.h>

#ifdef PSMP
#include <os/mutex.h>
#endif PSMP

#include <machine/clock.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/intreg.h>
#include <sundev/mbvar.h>

#include <sys/varargs.h>

#define	ISPEED	B9600
#define	IFLAGS	(CS7|CREAD|PARENB)

int	nsimc = 0;

int	console_via_map = 0;

int	simcidentify();
int	simcattach();
int	simcintr();
int	simcintr_lowpri();

struct dev_ops simc_ops = {
	1,
	simcidentify,
	simcattach,
};

int
simcidentify(name)
	char *name;
{
	if (strcmp(name, "simc"))
		return 0;
	nsimc ++;
	return 1;
}

struct simcregs {
	unsigned char	data;
};

#define	SIMC_ISOPEN	0x00000002	/* open is complete */
#define	SIMC_STOPPED	0x00000010	/* output is stopped */

/*
 * Fast ring buffer code from zs driver, sorta.
 */
#define	RINGSIZE	256
#define	RINGFRAC	2		/* fraction of ring to force flush */

#define	RING_INIT	(sc->rput = sc->rget = 0)
#define	RING_CNT	((sc->rput - sc->rget))
#define	RING_EMPTY	(sc->rput == sc->rget)
#define	RING_PUT(c)	(sc->data[sc->rput++] = (u_char)(c))
#define	RING_GET	(sc->data[sc->rget++])
#define	RING_EAT(n)	(sc->rget += (n))

struct simcons {
	struct simcregs *regs;
	unsigned	flags;	/* random flags */
	tty_common_t	tty_common;
	unsigned char	rput;
	unsigned char	rget;
	unsigned char	data[RINGSIZE];
}      *simcunits = 0;

int
simcattach(dev)
	struct dev_info *dev;
{
	struct simcons *sc;
	static int unit = -1;
	int nreg, nint, part;

	unit ++;

	if (!simcunits) {
		simcunits = (struct simcons *)
			new_kmem_zalloc((u_int)(nsimc * sizeof (struct simcons)),
					KMEM_SLEEP);
		if (!simcunits) {
			printf ("simc: no space for structures\n");
			return -1;
		}
	}
	dev->devi_unit = unit;
	sc = simcunits + unit;
	RING_INIT;
	nreg = dev->devi_nreg;
	if (nreg != 1) {
		printf("simc: regs not specified correctly for unit %d\n",
		       unit);
		return -1;
	}
	sc->regs = (struct simcregs *)
		map_regs(dev->devi_reg->reg_addr,
			 dev->devi_reg->reg_size,
			 dev->devi_reg->reg_bustype);
	if (!sc->regs) {
		printf("simc: unable to map registers\n");
		return -1;
	}
	nint = dev->devi_nintr;
	if (nint != 2) {
		printf("simc: ints not specified correctly for unit %d\n",
		       unit);
		return -1;
	}
	addintr(dev->devi_intr[0].int_pri, simcintr, dev->devi_name, unit);
	addintr(dev->devi_intr[1].int_pri, simcintr_lowpri, dev->devi_name, unit);
	report_dev(dev);
	return 0;
}

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

static void	simcioctl( /* queue_t *q, mblk_t *mp */ );
static void	simcstart();

extern char	simxinc();

#ifndef	PSMP
#define	cpuid	0
#else	PSMP
extern int cpuid;
#endif	PSMP

/*ARGSUSED*/
static int
simcopen(q, dev, flag, sflag)
	register queue_t *q;
	dev_t dev;
{
	register int unit = minor(dev);
	register struct simcons *sc = simcunits + unit;
	if (unit >= nsimc)
		return (OPENFAIL);
	if (!(sc->flags & SIMC_ISOPEN)) {
		sc->tty_common.t_iflag = 0;
		sc->tty_common.t_cflag = (ISPEED << IBSHIFT)|ISPEED|IFLAGS;
		sc->flags = SIMC_ISOPEN;
	} else if ((sc->tty_common.t_flags & TS_XCLUDE) && (u.u_uid != 0)) {
		u.u_error = EBUSY;
		return (OPENFAIL);
	}
	sc->tty_common.t_readq = q;
	sc->tty_common.t_writeq = WR(q);
	q->q_ptr = WR(q)->q_ptr = (caddr_t)sc;
	console_via_map = 1;
	return (dev);
}

static int
simcclose(q)
	register queue_t *q;
{
	struct simcons *sc = (struct simcons *)q->q_ptr;
	sc->flags &= ~SIMC_ISOPEN;
	ttycommon_close (&sc->tty_common);
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
	register struct simcons *sc = (struct simcons *)q->q_ptr;
	register int s;

	switch (mp->b_datap->db_type) {
	case M_STOP:
		/*
		 * Since we don't do real DMA, we can just let the
		 * chip coast to a stop after applying the brakes.
		 */
		sc->flags |= SIMC_STOPPED;
		freemsg(mp);
		break;

	case M_START:
		if (sc->flags & SIMC_STOPPED) {
			sc->flags &= ~SIMC_STOPPED;
			simcstart(sc);
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
		simcstart(sc);
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
	struct simcons *sc = simcunits + unit;
	queue_t *q;
	register mblk_t *mp;

	if ((q = sc->tty_common.t_writeq) == NULL)
		return;
	if ((mp = sc->tty_common.t_iocpending) != NULL) {
		sc->tty_common.t_iocpending = NULL;
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
	struct simcons *sc = (struct simcons *)q->q_ptr;
	struct iocblk *iocp;
	register unsigned datasize;
	int error;
	int s;

	if (sc->tty_common.t_iocpending != NULL) {
		/*
		 * We were holding an "ioctl" response pending the
		 * availability of an "mblk" to hold data to be passed up;
		 * another "ioctl" came through, which means that "ioctl"
		 * must have timed out or been aborted.
		 */
		freemsg(sc->tty_common.t_iocpending);
		sc->tty_common.t_iocpending = NULL;
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
	if ((datasize = ttycommon_ioctl(&sc->tty_common, q, mp, &error)) != 0) {
		sc->tty_common.t_iocpending = mp;
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
		((struct iocblk *)mp->b_rptr)->ioc_error = error;
		mp->b_datap->db_type = M_IOCNAK;
	}
	qreply(q, mp);
}

/*
 * Start output on a line, unless it's busy, frozen, or otherwise.
 */
static void
simcstart(sc)
	register struct simcons *sc;
{
	register queue_t *q;
	register mblk_t *bp;
	register int s;
	register int unit;

	if ((q = sc->tty_common.t_writeq) == NULL)
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
					if (sc->flags & SIMC_STOPPED) {
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
	mblk_t *bp;
	queue_t *q;
	char c;
	int unit;
	struct simcons *sc;
	int rv=0;

	for (unit=0; unit<nsimc; ++unit) {
		sc = simcunits + unit;

		while (!RING_EMPTY) {
			rv = 1;
			c = RING_GET;
			/*
			 * Send data up the stream if there's somebody listening.
			 */
			if ((q = sc->tty_common.t_readq) != NULL) {
				if ((bp = allocb(1, BPRI_MED)) != NULL) {
					if (!canput(q->q_next)) {
						ttycommon_qfull(&sc->tty_common, q);
						freemsg(bp);
					} else {
						*bp->b_wptr++ = c;
						putnext(q, bp);
					}
				}
			}
		}
	}
	return rv;
}

/*
 * High priority interrupt handler.
 * Read the character and stick in a ringbuffer.
 */
int
simcintr()
{
	int c, unit, rv=0;
	struct simcons *sc;
	rv = 0;

	for (unit=0; unit<nsimc; ++unit) {
		sc = simcunits + unit;
		while ((c = sc->regs->data) != 0) { /* %%% null == nothing available */
			rv = 1;
			RING_PUT(c);
			if (RING_EMPTY)
				RING_EAT(1); /* overflow, toss ancient character */
		}
	}

	/*
	 * handle rest of interrupt processing later, at lower priority,
	 * don't fool around with streams buffers at hi priority
	 */
	if (rv)
		send_dirint(cpuid, 6); /* turn on softint level 6 */
	
	return rv;
}

cnputc(c)
	register char c;
{
	int unit = 0;		/* cnputc goes to unit zero */
	struct simcons *sc = simcunits + unit;
	if (console_via_map)
		sc->regs->data = c;
	else
		simcoutc(c);	/* use magic-traps if not mapped yet */
	if ((c&127) == '\n')
		cnputc('\r');
}

int
cngetc()
{
	int unit = 0;		/* cngetc gets from unit zero */
	struct simcons *sc = simcunits + unit;
	if (console_via_map)
		return sc->regs->data;
	return simcinc();	/* use magic-traps if not mapped yet */
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

#ifdef	SHOWSTAGE
int	simcshowstartenable = 0;
int	simcshowstart_last[4] = { 0, 0, 0, 0 };
int	simcshowstart_curr[4] = { 0, 0, 0, 0 };

simcshowexit(pp)
    register struct proc *pp;
{
    struct user *up;
    int who, pid;
    char *cp;

    if (!simcshowstartenable)
	return;

    who = 3 & getprocessorid();
    pid = (pp==0) ? -2 : pp->p_pid;
    up = (pp==0) ? 0 : pp->p_uarea;
    cp = (up==0) ? 0 : up->u_comm;
    simcprtf("cpu %d <= pid %d exit %s\n\r",
	   who, pid, cp);
}

simcshowexec(pp)
    register struct proc *pp;
{
    struct user *up;
    int who, pid;
    char *cp;

    if (!simcshowstartenable)
	return;

    who = 3 & getprocessorid();
    pid = (pp==0) ? -2 : pp->p_pid;
    up = (pp==0) ? 0 : pp->p_uarea;
    cp = (up==0) ? 0 : up->u_comm;
    simcprtf("cpu %d <= pid %d exec %s\n\r",
	   who, pid, cp);
}

simcshowstart(pp)
    register struct proc *pp;
{
    struct user *up;
    int who, pid, i;
    char *cp;

    if (!simcshowstartenable)
	return;

    who = 3 & getprocessorid();
    pid = (pp==0) ? -2 : pp->p_pid;
    up = (pp==0) ? 0 : pp->p_uarea;
    cp = (up==0) ? 0 : up->u_comm;
    simcshowstart_curr[who] = pid;
    if (simcshowstartenable < 2) {
	if (pid < 0)
	    return;		/* no report when going idle */
	if (simcshowstart_last[who] == pid)
	    return;		/* no report if pid did not change */
	simcshowstart_last[who] = pid;
	if (pid > 0)
	    for (i=0; i<4; ++i)
		if ((i != who) && (simcshowstart_last[i] == pid))
		    if (simcshowstart_curr[i] == pid)
			simcprtf("cpu %d <= pid %d, DUPLICATED\n\r", i, pid);
		    else {
			simcprtf("cpu %d <= pid %d (%d MIGRATED)\n\r",
				 i, simcshowstart_curr[i], pid);
			simcshowstart_last[i] = simcshowstart_curr[i];
		    }
    }
    simcprtf("cpu %d <= pid %d command %s\n\r",
	   who, pid, cp);
}

simcouts(s)
	char *s;
{
	int ch;
	while (ch = *s++)
		simcoutc(ch);
}

simcouti(i, b)
	unsigned int i;
	unsigned int b;
{
	char	ibuf[16], *p;
	if ((b == 10) && (i & 0x80000000)) {
		simcoutc('-');
		i = 1 + ~i;
	}
	p = ibuf+b;
	*--p = 0;
	do {
		*--p = "0123456789ABCDEF"[i%b];
		i /= b;
	} while (i>0);
	simcouts(p);
}

int
isdigit(c)
int c;
{
    return (c >= '0') && (c <= '9');
}

#ifdef	PSMP
mutex_t	simcprflock[1];
int	simcprflockinit = 0;
#endif	PSMP

simcprtf(fmt, va_alist)
    char *fmt;
    va_dcl
{
    int width, altfmt, sign, val, base, ch;
    va_list x1;

#ifdef PSMP
    if (!simcprflockinit) {
	mutex_init (simcprflock, 15, (void *)0);
	simcprflockinit = 1;
    }
    mutex_enter (simcprflock);
#endif
    va_start(x1);
    while (ch = *fmt++) {
	if (ch != '%') {
	    simcoutc(ch);
	    continue;
	}
	width = altfmt = sign = 0;
	if (*fmt == '0') {
	    fmt++;
	    altfmt = 1;
	}
	while (*fmt && isdigit (*fmt)) {
	    ch = *fmt++;
	    width = width * 10 + ch - '0';
	}
	if (!*fmt) {
	    simcoutc('%');
	    break;
	}
	switch (ch = *fmt++) {
	  case 'o':
	    base = 8;
	    val = va_arg (x1, int);
	    if (altfmt && val)
		simcoutc('0');
	    goto basecommon;
	  case 'd':
	    base = 10;
	    val = va_arg (x1, int);
	    goto basecommon;
	  case 'x':
	    base = 16;
	    val = va_arg (x1, int);
	    if (altfmt && val)
		simcouts("0x");
	  basecommon:
	    simcouti (val, base);
	    break;

	  case 'c':
	    simcoutc(va_arg (x1, int));
	    break;
	  case 's':
	    simcouts(va_arg (x1, char *));
	    break;

	  case '%':
	    simcoutc('%');
	    break;
	  default:
	    simcoutc('%');
	    simcoutc(ch);
	    break;
	}
    }
    va_end(x1);
#ifdef PSMP
    mutex_exit (simcprflock);
#endif
}
#endif STAGE
