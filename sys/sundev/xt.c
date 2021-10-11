#ifndef lint
static	char sccsid[] = "@(#)xt.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

#include "xt.h"
#if NXT > 0
/*
 * Driver for Xylogics 472 Tape controller
 * Controller names are xtc?
 * Device names are xt?
 * This driver lifted from the TapeMaster driver
 *
 * TODO:
 *	test driver with more than one slave
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/file.h>
#include <sys/map.h>
#include <sys/vm.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>

#include <sundev/mbvar.h>
#include <sundev/xycreg.h>
#include <sundev/xtreg.h>

#define	SPLXT()	spl3()
#define	PRIXT	(PZERO+1)		/* for interruptible sleeps */

/*
 * There is a cxtbuf per tape drive.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * When the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for cxtbuf.
 */
struct	buf	cxtbuf[NXT];

#define	b_repcnt  b_bcount
#define	b_command b_resid

/*
 * Raw tape operations use rxtbuf.  The driver
 * notices when rxtbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 */
struct	buf	rxtbuf[NXT];

/*
 * Driver Multibus interface routines and variables.
 */
int	xtprobe(), xtslave(), xtattach(), xtgo(), xtdone(), xtpoll();

struct	mb_ctlr *xtcinfo[NXTC];
struct	mb_device *xtdinfo[NXT];
struct	mb_driver xtcdriver = {
	xtprobe, xtslave, xtattach, xtgo, xtdone, xtpoll,
	sizeof (struct xydevice), "xt", xtdinfo, "xtc", xtcinfo, MDR_BIODMA,
};
struct	buf xtutab[NXT];
short	xttoxtc[NXT];

/* bits in minor device */
#define	NUNIT		4
#define	XTCTLR(dev)	(xttoxtc[MTUNIT(dev)])
#define	T_HIDENS	MT_DENSITY2	/* select high density */

#define	INF	(daddr_t)1000000L

/*
 * Max # of buffers outstanding per unit
 */
#define	MAXMTBUF	3

#define	EOM_TWO_EOFS	2
#define	EOM_ONE_EOF	1
#define	EOM_NO_EOF	0
#define	NO_EOM		-1

#define	FOREVER		0x7fffffff

#define	FEET(n)		((n) << 2) /* 3 inch per erase, 4 times per ft */
#define	ERASE_AFT_EOT	(FEET(1))

#define	MX_RETRY	3
#define	MINPHYS_BYTES	65534

#define	XTE_HOST_TMO	0x40

/* sc_eot status */
#define	XT_EOT_OFF	0	/* clear off EOT */
#define	XT_EOT_ON	1	/* hit EOT but user does not know */
#define	XT_EOT_KNOWN	2	/* hit EOT and user knows about it */

/*
 * Software state per tape transport.
 *
 * 1. A tape drive is a unique-open device; we refuse opens when it is already.
 * 2. We keep track of the current position on a block tape and seek
 *    before operations by forward/back spacing if necessary.
 * 3. We remember if the last operation was a write on a tape, so if a tape
 *    is open read write and the last thing done is a write we can
 *    write a standard end of tape mark (two eofs).
 * 4. We remember the status registers after the last command, using
 *    then internally and returning them to the SENSE ioctl.
 */
struct	xt_softc {
	char	sc_openf;	/* lock against multiple opens */
	char	sc_lastiow;	/* last op was a write */
	char	sc_stream;	/* tape is a streamer */
	char	sc_bufcnt;	/* queued system buffer count */
	u_short	sc_xtstat;	/* status bits from xt_status */
	u_short	sc_error;	/* copy of last erreg */
	u_short	sc_lastcomm;	/* last command executed */
	long	sc_resid;	/* copy of last bytecount */
	daddr_t	sc_timo;	/* time until timeout expires */
	short	sc_tact;	/* timeout is active */
	daddr_t	sc_fileno;	/* file number */
	daddr_t	sc_recno;	/* rec number */
				/* NOTE:-1 indicates we're before EOF;	*/
				/*	 0 indicates we're after EOF;	*/
				/*	 0 if at EOM.			*/
	int	sc_open_opt;	/* open flags */
	off_t	sc_lstpos;	/* last offset */
	int	sc_eot;		/* eot status */
	int	sc_firstopen;	/* set on attach, cleared on first open */
} xt_softc[NXT];

/*
 * Data per controller
 */
struct	xtctlr {
	struct xt_softc	*c_units[NUNIT];	/* units on controller */
	struct xydevice	*c_io;			/* ptr to I/O space data */
	struct xtiopb	*c_iopb;		/* ptr to IOPB */
	char		c_present;		/* controller is present */
	char		c_wantint;		/* expecting interrupt */
} xtctlrs[NXTC];

/*
 * States for mc->mc_tab.b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */
#define	SERR	5		/* doing erase gap (error on write) */
#define	SIOBSR	6		/* back record before SIO retry */
#define	SWRTEOM	8		/* EOM when WRITE */

#ifndef sun3x
int xtthrot = 4;
#else
int xtthrot = 7;
#endif

/*
 * Give a simple command to a controller
 * and spin until done.
 * Returns the error number or zero
 */
static
simple(c, unit, cmd)
	struct xtctlr *c;
{
	register struct xydevice *xyio = c->c_io;
	register struct xtiopb *xt = c->c_iopb;
	int piopb, t;

	t = xyio->xy_resupd;		/* reset */
	if (!xtwait(xyio))
		return (XTE_TIMEOUT);
	bzero((caddr_t)xt, sizeof *xt);
	xt->xt_autoup = 1;
	xt->xt_reloc = 1;
	xt->xt_cmd = cmd;
	xt->xt_throttle = xtthrot;
	xt->xt_unit = unit;
	piopb = ((char *)xt) - DVMA;
	t = XYREL(xyio, piopb);
	xyio->xy_iopbrel[0] = t >> 8;
	xyio->xy_iopbrel[1] = t;
	t = XYOFF(piopb);
	xyio->xy_iopboff[0] = t >> 8;
	xyio->xy_iopboff[1] = t;
	xyio->xy_csr = XY_GO;
	while (xyio->xy_csr & XY_BUSY)
		DELAY(10);
	return (xt->xt_errno);
}

/*
 * Determine existence of controller
 */
xtprobe(reg, ctlr)
	caddr_t reg;
{
	register struct xtctlr *c = &xtctlrs[ctlr];
	int err;

	c->c_io = (struct xydevice *)reg;
	if (peekc((char *)&c->c_io->xy_resupd) == -1)
		return (0);

	c->c_iopb = (struct xtiopb *)rmalloc(iopbmap,
		(long)sizeof (struct xtiopb));
	if (c->c_iopb == NULL) {
		printf("xtprobe: no iopb space\n");
		return (0);
	}
	c->c_present = 1;
	(void) simple(c, 0, XT_NOP);
	if (c->c_iopb->xt_ctype != XYC_472) {
		printf("xtc%d: unknown controller type\n", ctlr);
		return (0);
	}
	if (err = simple(c, 0, XT_TEST))
		printf("xtc%d: self test error %x\n", ctlr, err);
	if (err)
		return (0);
	return (sizeof (struct xydevice));
}

/*
 * Always say that a unit is there.
 * We can't tell for sure anyway, and this lets
 * a guy plug one in without taking down the system
 * (These are micros, after all!)
 */
/*ARGSUSED*/
xtslave(md, reg)
	struct mb_device *md;
	caddr_t reg;
{

	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
/*ARGSUSED*/
xtattach(md)
	struct mb_device *md;
{
	register struct xtctlr *c = &xtctlrs[md->md_ctlr];
	int unit = md->md_slave;

	/* set up for any vectored interrupts to pass ctlr pointer */
	if (md->md_mc->mc_intr) {
		if ((c->c_io->xy_csr & XY_ADDR24) == 0)
			printf("xtc%d: WARNING: 20 bit addresses\n",
			    md->md_mc->mc_ctlr);
		*(md->md_mc->mc_intr->v_vptr) = (int)c;
	}

	c->c_units[unit] = &xt_softc[md->md_unit];
	/*
	 * xttoxtc is used in XTCTLR to index the controller
	 * arrays given a xt unit number.
	 */
	xttoxtc[md->md_unit] = md->md_mc->mc_ctlr;
	/*
	 * Set first open flag.
	 */
	xt_softc[md->md_unit].sc_firstopen = 1;
}

int	xttimer();
int	xtdefdens = 0;
int	xtdefspeed = 0;
int	recurse_flag = 0;
/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
xtopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int xtunit;
	register struct mb_device *md;
	register struct xt_softc *sc;
	int t;
	struct xydevice *xyio;
	int result;

	xtunit = MTUNIT(dev);

	if (xtunit>=NXT || (md = xtdinfo[xtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);

	if ((sc = &xt_softc[xtunit])->sc_openf)
		return (EBUSY);

	sc->sc_openf = 1;
	if (result = xtcommand(dev, XT_DSTAT, 0, 1)) { 	/* timed out */
		if (result == EINTR) {
			sc->sc_openf = 0;
			return (EINTR);
		}
		goto retry;
	}
	if ((sc->sc_xtstat & (XTS_ONL|XTS_RDY)) != (XTS_ONL|XTS_RDY)) {
		uprintf("xt%d: not online\n", xtunit);
		sc->sc_openf = 0;
		return (EIO);
	}
	if ((flag&FWRITE) && (sc->sc_xtstat & XTS_FPT)) {
		uprintf("xt%d: no write ring\n", xtunit);
		sc->sc_openf = 0;
		return (EACCES);
	}
	if (sc->sc_tact == 0) {
		sc->sc_timo = INF;
		sc->sc_tact = 1;
		timeout(xttimer, (caddr_t)dev, 2*hz);
	}
	if ((sc->sc_xtstat & XTS_BOT) || sc->sc_firstopen) {
		/*
		 * First time through, ensure we're at BOT.
		 */
		if (sc->sc_firstopen) {
			if (!(sc->sc_xtstat & XTS_BOT) &&
			    xtcommand(dev, XT_SEEK, XT_REW, 1)) {
				uprintf("xt%d: could not rewind\n", xtunit);
				sc->sc_openf = 0;
				return (EIO);
			}
			sc->sc_firstopen = 0;
		}

		/*
		 * suninstall doesn't rewind or offline the tape between
		 * volumes, so reset accounting variables here.
		 */
		sc->sc_fileno = 0;
		sc->sc_recno = 0;

		switch (md->md_flags) {
		default:
		case 0:		/* unknown drive type */
			if (xtdefdens) {
				if (xtdefdens > 0) {
					if (xtcommand(dev, XT_PARAM,
					    XT_HIDENS, 1))
						goto retry;
				} else
					if (xtcommand(dev, XT_PARAM,
					    XT_LODENS, 1))
						goto retry;
			}
			if (xtdefspeed) {
				if (xtdefspeed > 0) {
					if (xtcommand(dev, XT_PARAM,
					    XT_HIGH, 1))
						goto retry;
				} else
					if (xtcommand(dev, XT_PARAM,
					    XT_LOW, 1))
						goto retry;
			}
			break;
		case 1:		/* CDC Keystone III and Telex 9250 */
			if (minor(dev) & T_HIDENS) {
				if (xtcommand(dev, XT_PARAM, XT_HIDENS, 1))
					goto retry;
			} else
				if (xtcommand(dev, XT_PARAM, XT_LODENS, 1))
					goto retry;
			break;
		case 2:		/* Kennedy triple density */
			if (minor(dev) & T_HIDENS) {
				if (xtcommand(dev, XT_PARAM, XT_HIGH, 1))
					goto retry;
			} else
				if (xtcommand(dev, XT_PARAM, XT_LOW, 1))
					goto retry;
			break;
		}
	}
	sc->sc_lastiow = 0;
	sc->sc_open_opt = flag;
	sc->sc_lstpos = 0;

	/* invalidate the tape */
	if (sc->sc_open_opt == FWRITE)
		return (xtwrt_eom(dev, EOM_TWO_EOFS));
	else
		return (0);

retry:	sc->sc_openf = 0;
	if (!recurse_flag) {
		printf("xt: attempting reset\n");
		recurse_flag = 1;
		xyio = xtctlrs[md->md_ctlr].c_io;
		t = xyio->xy_resupd;			/* reset */
		(void) xtwait(xyio);
		t = xtopen(dev, flag);
		recurse_flag = 0;
		return (t);
	} else
		return (EIO);
}

static
xtwrt_eom(dev, n_eof)
	register dev_t dev;
	int n_eof;
{
	register struct xt_softc *sc = &xt_softc[MTUNIT(dev)];
	int i, stat = 0;

	for (i = 0; i < 2; i++)
		if (xtcommand(dev, XT_FMARK, 0, 1)) {
			printf("xtwrt_eom:failed in writing EOM %d.\n", i);
			return (EIO);
		}

	if (n_eof != EOM_NO_EOF) {
		if (stat = xtcommand(dev, XT_SEEK, XT_FILE+XT_REVERSE, n_eof))
			printf("xtwrt_eom: failed in backup %d eof.\n", n_eof);
		else
			/* we're positioned at EOM so recno = 0 */
			sc->sc_recno = 0;
	}

	return (stat);
}

static
xtflush_eom(dev, n_eof)
	register dev_t dev;
	int n_eof;
{
	register struct xt_softc *sc = &xt_softc[MTUNIT(dev)];

	if ((sc->sc_open_opt & FWRITE) && sc->sc_lastiow) {
		return (xtwrt_eom(dev, n_eof));
	} else
		return (0);
}

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
xtclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct xt_softc *sc = &xt_softc[MTUNIT(dev)];

#ifdef	lint
	flag = flag;
#endif	lint

	if (xtflush_eom(dev, EOM_ONE_EOF)) {
		printf("xtclose: flush eom failed\n");
		sc->sc_openf = 0;
	}

	if ((minor(dev)&MT_NOREWIND) == 0 &&
	    sc->sc_lastcomm != ((XT_SEEK<<8)+XT_UNLOAD))
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather cxtbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a TM_SENSE from completing.
		 */
		(void) xtcommand(dev, XT_SEEK, XT_REW, 0);
	sc->sc_openf = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 */
xtcommand(dev, com, subfunc, count)
	dev_t dev;
	int com, subfunc, count;
{
	register struct xt_softc *sc = &xt_softc[MTUNIT(dev)];
	register struct buf *bp;
	register int s;
	int error;

	if ((subfunc == XT_REW) && (sc->sc_xtstat & XTS_BOT))
		/* we're already at BOT */
		return (0);

	bp = &cxtbuf[MTUNIT(dev)];
	s = SPLXT();
	while (bp->b_flags&B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
		/*
		 * Allow user to control-C out of this sleep
		 * while tape is rewinding from previous process.
		 */
		if (sleep((caddr_t)bp, PRIXT|PCATCH)) {
			(void) splx(s);
			return (EINTR);
		}
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);

	bp->b_dev = dev;
	bp->b_repcnt = count;
	bp->b_command = (com << 8) + subfunc;
	bp->b_blkno = 0;
	xtstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return (0);

	s = SPLXT();
	while ((bp->b_flags&B_DONE) == 0) {
		bp->b_flags |= B_WANTED;
		(void) sleep((caddr_t)bp, PRIBIO);
	}
	(void) splx(s);

	error = geterror(bp);
	if (com == XT_SEEK && bp->b_resid != 0 && !error) {
		switch (subfunc) {
		case XT_REW:
		case XT_UNLOAD:
			break;
		default:
			error = EIO;
			break;
		}
	}
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;		/* note: clears B_BUSY */
	return (error);
}

/*
 * Queue a tape operation.
 */
xtstrategy(bp)
	register struct buf *bp;
{
	int xtunit = MTUNIT(bp->b_dev);
	register struct mb_ctlr *mc;
	register struct buf *dp;
	register struct xt_softc *sc = &xt_softc[xtunit];
	int s;

	/*
	 * Put transfer at end of unit queue
	 */
	dp = &xtutab[xtunit];
	bp->av_forw = NULL;
	s = SPLXT();
	while (sc->sc_bufcnt >= MAXMTBUF)
		(void) sleep((caddr_t)&sc->sc_bufcnt, PRIBIO);
	sc->sc_bufcnt++;
	mc = xtdinfo[xtunit]->md_mc;
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		/*
		 * Transport not already active...
		 * put at end of controller queue.
		 */
		dp->b_forw = NULL;
		if (mc->mc_tab.b_actf == NULL)
			mc->mc_tab.b_actf = dp;
		else
			mc->mc_tab.b_actl->b_forw = dp;
		mc->mc_tab.b_actl = dp;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;
	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (mc->mc_tab.b_active == 0)
		xtstart(mc);
	(void) splx(s);
}

/*
 * Start activity on a xt controller.
 */
xtstart(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct xt_softc *sc;
	register struct mb_device *md;
	register struct xtiopb *xt;
	register struct xydevice *xyio;
	int xtunit, cmd, subfunc, count, size, fast;
	int state;

	/*
	 * Look for an idle transport on the controller.
	 */
loop:
	if ((dp = mc->mc_tab.b_actf) == NULL)
		return;
	if ((bp = dp->b_actf) == NULL) {
		mc->mc_tab.b_actf = dp->b_forw;
		goto loop;
	}
	xtunit = MTUNIT(bp->b_dev);
	md = xtdinfo[xtunit];
	sc = &xt_softc[xtunit];
	xyio = xtctlrs[md->md_ctlr].c_io;
	/*
	 * Check for command overlap.
	 */
	if (!xtcsrvalid(xyio) || (xyio->xy_csr & XY_BUSY)) {
		printf("xt%d: bad command synchronization\n", xtunit);
		sc->sc_openf = -1;
	}
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in xtintr().
	 */
	sc->sc_lastiow = 0;

	if ((int)sc->sc_openf < 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		goto next;
	}

	fast = 0;
	count = 1;
	subfunc = 0;
	if (bp == &cxtbuf[xtunit]) {
		/*
		 * Execute control operation with the specified count.
		 */
		/*
		 * Set next state; give 10 minutes to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 5 minutes) to complete other ops.
		 */
		switch (bp->b_command & ~XT_REVERSE) {

		case (XT_SEEK<<8)+XT_REW:
		case (XT_SEEK<<8)+XT_UNLOAD:
			mc->mc_tab.b_active = SREW;
			sc->sc_timo = 10 * 60;
			fast = 1;
			break;

		case (XT_SEEK<<8)+XT_FILE:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo = 10 * 60;
			fast = 1;
			break;

		case (XT_NOP<<8):
		case (XT_DSTAT<<8):
		case (XT_PARAM<<8)+XT_LODENS:
		case (XT_PARAM<<8)+XT_HIDENS:
		case (XT_PARAM<<8)+XT_LOW:
		case (XT_PARAM<<8)+XT_HIGH:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo = imin(4*(int)bp->b_repcnt, 60);
			break;
		default:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo =
			    imin(imax(20*(int)bp->b_repcnt, 30), 5*60);
			break;
		}
		count = bp->b_repcnt;
		cmd = bp->b_command >> 8;
		subfunc = bp->b_command & 0xFF;
		size = 0;
		goto dobpcmd;
	}
	/* from here on should be all I/O related stuff, i.e. no ioctl .. */
	state = mc->mc_tab.b_active;

	/* NOTE: lseek should be done at higher level. */
	switch (state) {
	case 0:
		mc->mc_tab.b_active = SIO;
		/* NOTE: fall through SIO state */

	case SIO:
		size = bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0)
			cmd = XT_WRITE;
		else
			cmd = XT_READ;
		sc->sc_timo = imin(imax(20*(size/65536)+20, 30), 5 * 60);
		break;

	case SWRTEOM:	/* hit EOM in WRITE, back up one rec, return 0 count */
	case SIOBSR:	/* READ/WRITE error retry */
		cmd = XT_SEEK, subfunc = XT_REC+XT_REVERSE;
		count = 1;
		sc->sc_timo = imin(imax(20 * count, 30), 5 * 60);
		fast = 0;
		size = 0;
		break;

	case SERR:
		cmd = XT_FMARK, subfunc = XT_ERASE;
		mc->mc_tab.b_active = SERR;
		size = 0;
		count = 1;
		sc->sc_timo = 60;
		break;

	default:
		printf("xtstart: BAD state: %d\n", state);
	}

dobpcmd:
	/*
	 * Do the command in bp.
	 */
	xt = xtctlrs[md->md_ctlr].c_iopb;
	bzero((caddr_t)xt, sizeof *xt);
	xt->xt_autoup = 1;
	xt->xt_reloc = 1;
	xt->xt_ie = 1;
	xt->xt_cmd = cmd;
	xt->xt_throttle = xtthrot;
	xt->xt_subfunc = subfunc;
	xt->xt_unit = xtdinfo[xtunit]->md_slave;
#ifdef notdef
	if (sc->sc_stream) {
		tpb->tm_ctl.tmc_speed = fast;
	} else {
		tpb->tm_ctl.tmc_speed = (minor(bp->b_dev) & T_HIDENS) ? 0 : 1;
	}
#else
#ifdef lint
	fast = fast;
#endif
#endif
	if (size) {
		xt->xt_swab = 1;
		xt->xt_retry = 1;
		xt->xt_cnt = size;
		(void) mbgo(mc);
	} else {
		xt->xt_cnt = count;
		xtgo(mc);
	}
	sc->sc_lastcomm = (cmd << 8) + subfunc;
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 */
	mc->mc_tab.b_errcnt = 0;
	mc->mc_tab.b_active = 0;
	sc->sc_timo = INF;
	dp->b_actf = bp->av_forw;
	bp->b_flags |= B_ERROR;
	bp->b_resid = bp->b_bcount;
	iodone(bp);
	if (sc->sc_bufcnt-- >= MAXMTBUF)
		wakeup((caddr_t)&sc->sc_bufcnt);
	goto loop;
}

/*
 * The Multibus resources we needed have been
 * allocated to us; start the device.
 * We assume the controller is ready because this is checked in xtstart.
 */
xtgo(mc)
	struct mb_ctlr *mc;
{
	register struct xtctlr *c = &xtctlrs[mc->mc_ctlr];
	register struct xydevice *xyio = c->c_io;
	register struct xtiopb *xt = c->c_iopb;
	register int dmaddr, piopb, t;

	dmaddr = MBI_ADDR(mc->mc_mbinfo);
	if ((dmaddr + xt->xt_cnt) > 0x100000 && (xyio->xy_csr & XY_ADDR24) == 0)
		panic("xt: exceeded 20 bit address");
	xt->xt_bufoff = XYOFF(dmaddr);
	xt->xt_bufrel = XYREL(xyio, dmaddr);
	/* stuff IOPB info into I/O registers */
	piopb = ((char *)xt) - DVMA;
	t = XYREL(xyio, piopb);
	xyio->xy_iopbrel[0] = t >> 8;
	xyio->xy_iopbrel[1] = t;
	t = XYOFF(piopb);
	xyio->xy_iopboff[0] = t >> 8;
	xyio->xy_iopboff[1] = t;
	xyio->xy_csr = XY_GO;
	mc->mc_tab.b_flags &=~ B_DONE;
	mc->mc_tab.b_flags |= B_BUSY;
}

/*
 * interrupt routine.
 */
xtintr(c)
	register struct xtctlr *c;
{
	register struct mb_ctlr *mc;
	register struct xt_softc *sc;
	register struct xtiopb *xt;
	register struct buf *dp;
	register struct buf *bp;
	register int xtunit, state;
	register struct xydevice *xyio;
	int err;

	xyio = c->c_io;
	/* wait for controller to settle down  */
	(void) xtcsrvalid(xyio);
	mc = xtcinfo[c - xtctlrs];

	xt = c->c_iopb;
	if (xt->xt_errno == XTE_HOST_TMO) {
		if ((err = simple(c, 0, XT_DRESET)))
			printf("xtintr: XTE_TIMEOUT reset failed, err= 0x%x\n",
				err);
		xt->xt_errno = XTE_HOST_TMO;
	} else {
		/* clear the interrupt */
		if (xyio->xy_csr & (XY_ERROR|XY_DBLERR)) {
			xyio->xy_csr = XY_ERROR;
			(void) xtcsrvalid(xyio);
		}
		xyio->xy_csr = XY_INTR;
	}

	mc->mc_tab.b_flags &= ~B_BUSY;
	if ((state = mc->mc_tab.b_active) == 0) {
		printf("xt%d: stray interrupt\n", mc->mc_ctlr);
		(void) xtcsrvalid(xyio); /* don't return too fast */
		return;
	}
	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	if (bp == NULL)
		panic("xtintr: queuing error");
	xtunit = MTUNIT(bp->b_dev);
	if (xtunit >= NXT)
		panic("xtintr: queueing error 2");
	sc = &xt_softc[xtunit];
	sc->sc_xtstat = xt->xt_status;

	/*
	 * EOT handling: we let user read/write pass EOT. In READ, EOT is
	 * transparent to the users. In WRITE, users will be notified at
	 * the first time WRITE past EOT by 0 byte count return.
	 */

	if (mc->mc_tab.b_active != SWRTEOM &&
		mc->mc_tab.b_active != SIOBSR) {
		switch (sc->sc_eot) {
		case XT_EOT_OFF:
			if (sc->sc_xtstat & XTS_EOT) {
				if ((bp->b_flags & B_READ) == B_WRITE) {
					sc->sc_eot = XT_EOT_KNOWN;
					mc->mc_tab.b_active = SWRTEOM;
					goto opcont;
				} else
					sc->sc_eot = XT_EOT_ON;
			}
			break;

		case XT_EOT_ON:
			if ((sc->sc_xtstat & XTS_EOT) == 0) {
				sc->sc_eot = XT_EOT_OFF;
			} else {
				if ((bp->b_flags & B_READ) == B_WRITE)  {
					sc->sc_eot = XT_EOT_KNOWN;
					mc->mc_tab.b_active = SWRTEOM;
					goto opcont;
				}
			}
			break;

		case XT_EOT_KNOWN:
			/*
			 * NOTE: since we do a SWRTEOM, so the very next
			 * write may be back off EOT completely, but we
			 * want to stay in XT_EOT_KNOWN state until other
			 * command moves the tape off EOT.
			 */
			if ((sc->sc_xtstat & XTS_EOT) == 0 &&
				(bp->b_flags & B_READ) != B_WRITE)
				sc->sc_eot = XT_EOT_OFF;
			break;

		}

		/* set residue count */
		sc->sc_resid = xt->xt_cnt - xt->xt_acnt;
	}

	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for another interrupt, triggered
	 * by xttimer
	 */
	if ((state == SREW) && (sc->sc_xtstat & XTS_ONL) &&
	    (sc->sc_xtstat & (XTS_RDY|XTS_BOT)) != (XTS_RDY|XTS_BOT)) {
		(void) xtcsrvalid(xyio); /* don't return too fast */
		return;
	}

	/*
	 * An operation completed... record status
	 */
	err = xt->xt_errno;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;

	if (err == XTE_BOT && (sc->sc_xtstat & XTS_BOT))
		err = XTE_NOERROR;

	if (err == XTE_SOFTERR) {
		printf("xt%d: soft error bn=%d\n", xtunit, bp->b_blkno);
		err = XTE_NOERROR;
	}
	sc->sc_error = err;
	/*
	 * Check for errors.
	 */
	if (err != XTE_NOERROR && err != XTE_SHORTREC && err != XTE_EOT) {
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if (err == XTE_EOF) {
			/* just read past EOF */
			sc->sc_fileno++;
			sc->sc_recno = 0;
			goto opdone;
		}
		if (err == XTE_LONGREC) {
			bp->b_flags |= B_ERROR;
			bp->b_error = EINVAL;
			sc->sc_resid = bp->b_bcount;
			sc->sc_recno++;		/* this rec has been read */
			goto opdone;
		}
		/*
		 * If error is not hard, and this was an i/o operation
		 * retry up to MX_RETRY times.
		 */
		if (state == SIO && err != XTE_HOST_TMO) {
			if (++mc->mc_tab.b_errcnt <= MX_RETRY) {
				mc->mc_tab.b_active = SIOBSR;
				goto opcont;
			}
		} else {
			/*
			 * Hard or non-i/o errors on non-raw tape
			 * cause it to close.
			 */
			if ((int)sc->sc_openf > 0 && bp != &rxtbuf[xtunit])
				sc->sc_openf = -1;
		}
		/*
		 * Couldn't recover error
		 */
		printf("xt%d: hard error bn=%d er=0x%x\n", xtunit,
		    bp->b_blkno, err);
		bp->b_flags |= B_ERROR;
		goto opdone;
	}
	/*
	 * Advance tape control FSM.
	 */
	switch (state) {

	case SIO:
		/*
		 * Read/write increments tape block number
		 */
		sc->sc_recno++;
		sc->sc_lstpos += xt->xt_acnt;
		break;

	case SREW:
	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		if (bp == &cxtbuf[xtunit])
		switch (bp->b_command) {

		/* record skipping */
		case (XT_SEEK<<8)+XT_REC:
			sc->sc_recno += xt->xt_acnt;
			if (err == XTE_SHORTREC) {
				sc->sc_fileno++;
				sc->sc_recno = 0;
			}
			break;

		case (XT_SEEK<<8)+XT_REVERSE+XT_REC:
			sc->sc_recno -= xt->xt_acnt;
			if (err == XTE_SHORTREC) {
				sc->sc_fileno--;
				sc->sc_recno = -1;
			}
			break;

		/* file skipping */
		case (XT_SEEK<<8)+XT_FILE:
			sc->sc_fileno += xt->xt_acnt;
			sc->sc_recno = 0;
			break;

		case (XT_SEEK<<8)+XT_REVERSE+XT_FILE:
			sc->sc_fileno -= xt->xt_acnt;
			if (sc->sc_fileno == 0)
				sc->sc_recno = 0;	/* at BOT */
			else
				sc->sc_recno = -1;	/* before EOF */
			break;

		case (XT_SEEK<<8)+XT_REW:
		case (XT_SEEK<<8)+XT_UNLOAD:
			sc->sc_recno = 0;
			sc->sc_fileno = 0;
			sc->sc_resid = 0;
			break;

		/* WEOF */
		case (XT_FMARK<<8):
			/* NOTE: we write one EOF at a time */
			sc->sc_fileno++;
			break;
		}
		break;

	case SIOBSR:
		/* we just back up one record */
		if (bp->b_flags & B_READ)
			mc->mc_tab.b_active = SIO;
		else
			mc->mc_tab.b_active = SERR; /* ready for short earse */
		goto opcont;

	case SWRTEOM:
		/* return zero byte count */
		sc->sc_resid = bp->b_bcount;
		goto opdone;

	case SERR:
		mc->mc_tab.b_active = SIO;	/* ready to write again */
		goto opcont;

	default:
		printf("xtintr: BAD STATE: %d\n", state);
		goto opcont;
	}
opdone:
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_flags |= B_DONE;
opcont:
	sc->sc_timo = INF;
	if (mc->mc_mbinfo != 0)
		mbdone(mc);
	else
		xtdone(mc);
}

/*
 * polling interrupt routine.
 */
xtpoll()
{
	register struct xtctlr *c;

	for (c = xtctlrs; c < &xtctlrs[NXTC]; c++) {
		if (!c->c_present || !xtcsrvalid(c->c_io) ||
		    (c->c_io->xy_csr & XY_INTR) == 0)
			continue;
		xtintr(c);
		return (1);
	}
	return (0);
}

xtdone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *dp;
	register struct buf *bp;
	register struct xt_softc *sc;
	int xtunit;

	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	xtunit = MTUNIT(bp->b_dev);
	sc = &xt_softc[xtunit];
	if (mc->mc_tab.b_flags & B_DONE) {
		/*
		 * Reset error count and remove
		 * from device queue.
		 */
		mc->mc_tab.b_errcnt = 0;
		dp->b_actf = bp->av_forw;
		bp->b_resid = sc->sc_resid;
		iodone(bp);
		if (sc->sc_bufcnt-- >= MAXMTBUF)
			wakeup((caddr_t)&sc->sc_bufcnt);
		/*
		 * Advance controller queue and put this
		 * unit back on the controller queue if
		 * the unit queue is not empty
		 */
		mc->mc_tab.b_actf = dp->b_forw;
		if (dp->b_actf) {
			dp->b_forw = NULL;
			if (mc->mc_tab.b_actf == NULL)
				mc->mc_tab.b_actf = dp;
			else
				mc->mc_tab.b_actl->b_forw = dp;
			mc->mc_tab.b_actl = dp;
		}
	} else {
		/*
		 * Circulate slave to end of controller
		 * queue to give other slaves a chance.
		 * No need to look at unit queue since operation
		 * is still in progress
		 */
		if (dp->b_forw) {
			mc->mc_tab.b_actf = dp->b_forw;
			dp->b_forw = NULL;
			mc->mc_tab.b_actl->b_forw = dp;
			mc->mc_tab.b_actl = dp;
		}
	}
	if (mc->mc_tab.b_actf)
		xtstart(mc);
}

xttimer(dev)
	int dev;
{
	register struct mb_ctlr *mc = xtcinfo[XTCTLR(dev)];
	register struct xt_softc *sc = &xt_softc[MTUNIT(dev)];
	register struct xtctlr *c = &xtctlrs[XTCTLR(dev)];
	register struct xtiopb *xt = c->c_iopb;
	register struct xydevice *xyio;
	register int s;
	int piopb, t;

	if (sc->sc_timo != INF && (sc->sc_timo -= 2) < 0) {
		printf("xt%d: timeout interrupt\n", MTUNIT(dev));
		sc->sc_timo = INF;
		s = SPLXT();
		xt->xt_errno = XTE_HOST_TMO;
		xtintr(c);
		(void) splx(s);
	}
	if (mc->mc_tab.b_active == SREW) {	/* check rewind status */
		s = SPLXT();
		if ((mc->mc_tab.b_flags & B_BUSY) == 0 &&
		    (c->c_io->xy_csr & XY_BUSY) == 0) {
			xyio = c->c_io;
			bzero((caddr_t)xt, sizeof *xt);
			xt->xt_autoup = 1;
			xt->xt_reloc = 1;
			xt->xt_ie = 1;
			xt->xt_cmd = XT_DSTAT;
			xt->xt_throttle = xtthrot;
			xt->xt_unit = xtdinfo[MTUNIT(dev)]->md_slave;
			piopb = ((char *)xt) - DVMA;
			t = XYREL(xyio, piopb);
			xyio->xy_iopbrel[0] = t >> 8;
			xyio->xy_iopbrel[1] = t;
			t = XYOFF(piopb);
			xyio->xy_iopboff[0] = t >> 8;
			xyio->xy_iopboff[1] = t;
			xyio->xy_csr = XY_GO;
			mc->mc_tab.b_flags |= B_BUSY;
		}
		(void) splx(s);
	}
	timeout(xttimer, (caddr_t)dev, 2*hz);
}

/*
 * Wait for controller csr to become valid.
 * Waits for at most 100 usec. Returns true if wait succeeded.
 */
int
xtcsrvalid(xyio)
	register struct xydevice *xyio;
{
	register int i;

	for (i = 10; i && xyio->xy_csr == (XY_BUSY|XY_DBLERR); i--)
		DELAY(10);
	return (xyio->xy_csr != (XY_BUSY|XY_DBLERR));
}

/*
 * Wait for controller become ready. Used after reset or interrupt.
 * Waits for at most 200 usec. Returns true if wait succeeded.
 */
int
xtwait(xyio)
	register struct xydevice *xyio;
{
	register int i;

	for (i = 20; i && (xyio->xy_csr & XY_BUSY); i--)
		DELAY(10);
	return ((xyio->xy_csr & XY_BUSY) == 0);
}

void
xtminphys(bp)
	struct buf *bp;
{
	if (bp->b_bcount > MINPHYS_BYTES)
		bp->b_bcount = MINPHYS_BYTES;
}

xtread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int xtunit = MTUNIT(dev);
	register struct mb_device *md;

	if (xtunit >= NXT || (md=xtdinfo[xtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);

	if (xtflush_eom(dev, EOM_TWO_EOFS))
		return (EIO);

	return (physio(xtstrategy, &rxtbuf[MTUNIT(dev)], dev, B_READ,
	    xtminphys, uio));
}

xtwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int xtunit = MTUNIT(dev);
	register struct mb_device *md;

	if (xtunit >= NXT || (md=xtdinfo[xtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);

	return (physio(xtstrategy, &rxtbuf[MTUNIT(dev)], dev, B_WRITE,
	    xtminphys, uio));
}

/*ARGSUSED*/
xtioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	int xtunit = MTUNIT(dev);
	register struct xt_softc *sc = &xt_softc[xtunit];
	register callcount;
	int fcount, op, stat;
	struct mtop *mtop;
	struct mtget *mtget;
	int flush = NO_EOM;	/* default flush value */

	/* we depend on the values and order of the MT codes here */
	static tmops[] = {
		(XT_FMARK<<8),				/* MTWEOF */
		(XT_SEEK<<8)+XT_FILE,			/* MTFSF */
		(XT_SEEK<<8)+XT_REVERSE+XT_FILE,	/* MTBSF */
		(XT_SEEK<<8)+XT_REC,			/* MTFSR */
		(XT_SEEK<<8)+XT_REVERSE+XT_REC,		/* MTBSR */
		(XT_SEEK<<8)+XT_REW,			/* MTREW */
		(XT_SEEK<<8)+XT_UNLOAD,			/* MTOFFL */
		(XT_DSTAT<<8),				/* MTNOP */
		(XT_NOP<<8),				/* MTRETEN */
		(XT_FMARK<<8)+XT_ERASE,			/* MTERASE */
		(XT_NOP<<8),				/* MTEOM */
		(XT_SEEK<<8)+XT_REVERSE+XT_FILE, 	/* MTNBSF */

	};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;

		switch (mtop->mt_op) {
		case MTFSF:
		case MTBSF:
			/*
			 * For ASF and compatibility with st driver,
			 * we allow a count of 0 which means we just
			 * want to go to beginning of current file.
			 * Equivalent to "nbsf(0)" or "bsf(1) + fsf".
			 */
			callcount = 1;
			if ((fcount = mtop->mt_count) == 0) {
				fcount++;
				mtop->mt_op = MTNBSF;
			}
			flush = EOM_TWO_EOFS;
			break;

		case MTNBSF:
			callcount = 1;
			/*
			 * NBSF(n) == BSF(n+1) + FSF
			 */
			fcount = mtop->mt_count + 1;
			flush = EOM_TWO_EOFS;
			break;

		case MTFSR:
		case MTBSR:
			callcount = 1;
			/*
			 * For compatibility with st driver,
			 * 0 count is NOP.
			 */
			if ((fcount = mtop->mt_count) == 0)
				return (0);
			flush = EOM_TWO_EOFS;
			break;

		case MTREW:
		case MTOFFL:
		case MTNOP:
			callcount = 1;
			fcount = mtop->mt_count;
			flush = EOM_NO_EOF;
			break;

		case MTERASE:
			if ((sc->sc_open_opt & FWRITE) == 0)
				return (EACCES);

			if (xtcommand(dev, XT_SEEK, XT_REW, 1))
				return (EIO);

			callcount = FOREVER;
			fcount = 1;
			break;

		case MTWEOF:
			if ((sc->sc_open_opt & FWRITE) == 0)
				return (EACCES);

			callcount = mtop->mt_count;
			fcount = 1;
			if (callcount == 1) {
				/* always write one extra for EOM */
				if (xtwrt_eom(dev, EOM_ONE_EOF)) {
					return (EIO);
				} else
					goto opdone;
			}
			break;

		case MTEOM:
			if (xtflush_eom(dev, EOM_TWO_EOFS))
				return (EIO);

			/* search until error or EOM found */
			while (1)
				if ((stat = xtchk_file(dev)) != 0) {
					if (stat == -1) {
						/* empty file (EOM) found */
						sc->sc_resid = 0;
						/* EOM => recno 0 */
						sc->sc_recno = 0;
						return (0);
					} else { /* error */
						sc->sc_resid = 1;
						return (EIO);
					}
				}

		default:
			return (ENOTTY);
		}

		if (callcount <= 0 || fcount <= 0)
			return (ENOTTY);

		if (flush != NO_EOM && xtflush_eom(dev, flush))
			return (EIO);

		while (--callcount >= 0) {
			op = tmops[mtop->mt_op];
			if (xtcommand(dev, op >> 8, op & 0xFF, fcount))
				return (EIO);
			/*
			 * stop erase, otherwise, it would rip the
			 *  tape off the wheel.
			 */
			if ((sc->sc_xtstat & XTS_EOT) &&
				(mtop->mt_op == MTERASE) &&
				callcount > ERASE_AFT_EOT) {
					callcount = ERASE_AFT_EOT;
			}
		}
		if (mtop->mt_op == MTERASE) {
			if (xtcommand(dev, XT_SEEK, XT_REW, 1))
				return (EIO);
			break;
		}
		if (mtop->mt_op == MTNBSF) {
			/* skip over EOF for NBSF */
			op = tmops[MTFSF];
			if (xtcommand(dev, op >> 8, op & 0xFF, 1))
				return (EIO);
			break;
		}

opdone:
		/* do extra accounting work */
		if (mtop->mt_op == MTWEOF)
			sc->sc_recno = 0;
		break;

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = sc->sc_xtstat;
		mtget->mt_erreg = sc->sc_error;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISXY;
		if (sc->sc_xtstat & XTS_BOT) {
			/*
			 * Foolproofing in case there's manual intervention
			 * in the middle of a program which is not well-
			 * behaved.  (e.g., suninstall doesn't always issue
			 * offline/rewind the tape when it should.)  This
			 * really belongs only in xtopen, but as long as
			 * we're foolproofing, we might as well allow manual
			 * intervention in a program which does its own ioctls.
			 */
			sc->sc_fileno = 0;
			sc->sc_recno = 0;
		}
		mtget->mt_fileno = sc->sc_fileno;
		mtget->mt_blkno = sc->sc_recno;
		/*
		 * Note that ASF has really only been implemented
		 * to the extent that it will support suninstall.
		 */
		mtget->mt_flags = MTF_REEL | MTF_ASF;
		mtget->mt_bf = 20;
		break;

	default:
		return (ENOTTY);
	}

	return (0);
}

static
xtfwd_rec(dev, count)
	dev_t dev;
	int count;
{
	struct xt_softc *sc = &xt_softc[MTUNIT(dev)];
	int sv_resid;

	/*
	 * a record length short status is posted when the
	 * 472 detects a tape mark on space record forward
	 */
	if (xtcommand(dev, XT_SEEK, XT_REC, count)) {
		if (sc->sc_error == XTE_SHORTREC || sc->sc_error == XTE_EOT) {
			sv_resid = sc->sc_resid;
			/* leave tape positioned between the two EOFs */
			(void) xtcommand(dev, XT_SEEK, XT_FILE+XT_REVERSE, 1);
			sc->sc_resid = sv_resid;
			sc->sc_error = XTE_SHORTREC;
		}
		return (EIO);
	} else
		return (0);

}

static
xtchk_file(dev)
	dev_t	dev;
{
	struct xt_softc *sc = &xt_softc[MTUNIT(dev)];
	int stat;

	/*
	 * If we're just after EOF (recno == 0) and we just
	 * encountered another EOF, then we've found the EOM.
	 */
	if (sc->sc_recno == 0 && xtfwd_rec(dev, 1)) {
		if (sc->sc_error == XTE_SHORTREC)
			return (-1);	/* empty file found!! */
		else
			return (EIO);
	}

	/* not an empty file, skip it */
	stat = xtcommand(dev, XT_SEEK, XT_FILE, 1);

	return (stat);
}

#endif
