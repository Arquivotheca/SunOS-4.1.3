#ifndef lint
static	char sccsid[] = "@(#)tm.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Driver for Computer Products Corp. TapeMaster controller
 * Controller names are tm?
 * Device names are mt?
 * This driver lifted from the VAX TM/TE driver
 */
#include "mt.h"
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
#include <sundev/tmreg.h>

#define	SPLTM()	spl3()
#define	PRITM	(PZERO+5)

/*
 * There is a ctmbuf per tape drive.
 * It is used as the token to pass to the internal routines
 * to execute tape ioctls.
 * When the tape is rewinding on close we release
 * the user process but any further attempts to use the tape drive
 * before the rewind completes will hang waiting for ctmbuf.
 */
struct	buf	ctmbuf[NMT];

/*
 * Raw tape operations use rtmbuf.  The driver
 * notices when rtmbuf is being used and allows the user
 * program to continue after errors and read records
 * not of the standard length (BSIZE).
 */
struct	buf	rtmbuf[NMT];

/*
 * Driver Multibus interface routines and variables.
 */
int	tmprobe(), tmslave(), tmattach(), tmgo(), tmdone(), tmpoll();

struct	tmscp *tmscp = NULL;

#define	NOTZERO	1	/* don't care value-but not zero (clr inst botch) */
#define	ATTN(c) 	(((struct tmdevice *)c)->tmdev_attn=NOTZERO)
#define	RESET(c) 	(((struct tmdevice *)c)->tmdev_reset=NOTZERO)
#define	clrtpb(tp)	bzero((caddr_t)(tp), sizeof *(tp));

struct	mb_ctlr *tmcinfo[NTM];
struct	mb_device *tmdinfo[NMT];
struct	mb_driver tmdriver = {
	tmprobe, tmslave, tmattach, tmgo, tmdone, tmpoll,
	sizeof (struct tmdevice), "mt", tmdinfo, "tm", tmcinfo, MDR_BIODMA,
};
struct	buf mtutab[NMT];
short	mttotm[NMT];
struct	tm_mbinfo *tm_mb[NTM];

/* bits in minor device */
#define	TMUNIT(dev)	(mttotm[MTUNIT(dev)])
#define	T_HIDENS	MT_DENSITY2	/* select high density */

#define	INF	(daddr_t)1000000L

/*
 * Max # of buffers outstanding per unit
 */
#define	MAXMTBUF	3

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
struct	mt_softc {
	char	sc_openf;	/* lock against multiple opens */
	char	sc_lastiow;	/* last op was a write */
	char	sc_stream;	/* tape is a streamer */
	char	sc_bufcnt;	/* queued system buffer count */
	daddr_t	sc_blkno;	/* block number, for block device tape */
	daddr_t	sc_nxrec;	/* position of end of tape, if known */
	u_short	sc_erreg;	/* copy of last erreg */
	struct	tmstat sc_tms;	/* last unit status */
	long	sc_resid;	/* copy of last bc */
	daddr_t	sc_timo;	/* time until timeout expires */
	short	sc_tact;	/* timeout is active */
	int	sc_firstopen;	/* set on attach, cleared on first open */
} mt_softc[NMT];

/*
 * States for mc->mc_tab.b_active, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#define	SSEEK	1		/* seeking */
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define	SREW	4		/* sending a drive rewind */
#define	SERR	5		/* doing erase gap (error on write) */

/*
 * Determine if there is a controller for
 * a tm at address reg.
 * The SCP must be allocatable to initialize the controller.
 */
tmprobe(reg, ctlr)
	register struct tmdevice *reg;
	int ctlr;
{
	register struct tmscp *scp = (struct tmscp *)&DVMA[TM_SCPADDR];
	register struct tm_mbinfo *tmb;
	register struct tpb *tpb;
	static tm_swab = 0;

	if (pokec((char *)&reg->tmdev_reset, NOTZERO))
		return (0);	/* Reset failed */

	if (tmscp == NULL &&
	    rmget(iopbmap, (long)sizeof (struct tmscp), (u_long)scp) == 0) {
		printf("tm%d: can't get iopb at %x to initialize controller\n",
			ctlr, TM_SCPADDR);
		return (0);
	}
	tmscp = scp;

	tm_mb[ctlr] = (struct tm_mbinfo *)rmalloc(iopbmap,
	    (long)sizeof (struct tm_mbinfo));
	if (tm_mb[ctlr] == 0) {
		printf("tm%d: can't get iopb space\n", ctlr);
		return (0);
	}
	bzero((caddr_t)tm_mb[ctlr], sizeof (struct tm_mbinfo));
	if (!tminit(reg, ctlr))
		return (0);
	if (tm_swab == 1)
		return (sizeof (struct tmdevice));
	/* Enable byte-swapping if we have a TapeMaster A */
	tmb = tm_mb[ctlr];
	tpb = &tmb->tmb_tpb;
	/* Issue OPTION command */
	clrtpb(tpb);
	tpb->tm_cmd = TM_OPTION;
	tpb->tm_cmd2 = 0;
	tpb->tm_ctl.tmc_width = 1;
	tpb->tm_rcount = TMOP_DOSWAB;
	/* Get the gate */
	while (tmb->tmb_ccb.tmccb_gate != TMG_OPEN)
		;
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;
	/* Start the command and wait for completion */
	ATTN(reg);
	while (tmb->tmb_ccb.tmccb_gate == TMG_CLOSED)
		;
	if (tpb->tm_stat.tms_error || (tpb->tm_rcount & TMOP_ISSWAB) == 0) {
		if (tm_swab == -1) {	/* already saw TapeMaster A */
			printf("tm%d: older TapeMaster must be first\n", ctlr);
			return (0);
		}
		tm_swab = 1;
		tmdriver.mdr_flags |= MDR_SWAB;
	} else
		tm_swab = -1;
	return (sizeof (struct tmdevice));
}

/*
 * Always say that a unit is there.
 * We can't tell for sure anyway, and this lets
 * a guy plug one in without taking down the system
 * (These are micros, after all!)
 */
/*ARGSUSED*/
tmslave(md, reg)
	struct mb_device *md;
	caddr_t reg;
{

	return (1);
}

/*
 * Record attachment of the unit to the controller.
 */
/*ARGSUSED*/
tmattach(md)
	struct mb_device *md;
{

	/*
	 * Mttotm is used in TMUNIT to index the controller
	 * arrays given a mt unit number.
	 */
	mttotm[md->md_unit] = md->md_mc->mc_ctlr;
	mt_softc[md->md_unit].sc_stream = md->md_flags ? 1 : 0;
	mt_softc[md->md_unit].sc_firstopen = 1;
}

#define	SPININIT 1000000
/*
 * Initialize a controller
 * Reset it, set up SCP, SCB, and CCB,
 * and give it an attention.
 * Make sure its there by waiting for the gate to open
 * Once initialization is done, issue CONFIG just to be safe.
 */
tminit(reg, ctlr)
	register struct tmdevice *reg;
{
	register struct tm_mbinfo *tmb = tm_mb[ctlr];
	register struct tpb *tpb;
	register int spin;

	RESET(reg);

	/* setup System Configuration Pointer */
	tmscp->tmscb_bus = tmscp->tmscb_busx = TMSCB_BUS16;
	c68t86((long)&tmb->tmb_scb, &tmscp->tmscb_ptr);

	/* setup System Configuration Block */
	tmb->tmb_scb.tmscb_03 = tmb->tmb_scb.tmscb_03x = TMSCB_CONS;
	c68t86((long)&tmb->tmb_ccb, &tmb->tmb_scb.tmccb_ptr);

	/* setup Channel Control Block */
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;

	ATTN(reg);
	for (spin = SPININIT; tmb->tmb_ccb.tmccb_gate != TMG_OPEN; )
		if (--spin <= 0)
			break;
	if (spin <= 0) {
		printf("tm%d: no response from ctlr\n", ctlr);
		return (0);
	}

	/* Finish CCB, point it at TPB */
	tmb->tmb_ccb.tmccb_ccw = TMC_NORMAL;
	tpb = &tmb->tmb_tpb;
	c68t86((long)tpb, &tmb->tmb_ccb.tmtpb_ptr);

	/* Issue CONFIG command */
	clrtpb(tpb);
	tpb->tm_cmd = TM_CONFIG;
	tpb->tm_cmd2 = 0;
	tpb->tm_ctl.tmc_width = 1;

	/* Get the gate */
	while (tmb->tmb_ccb.tmccb_gate != TMG_OPEN)
		;
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;

	/* Start the command and wait for completion */
	ATTN(reg);
	while (tmb->tmb_ccb.tmccb_gate == TMG_CLOSED)
		;

	/* Check and report errors */
	if (tpb->tm_stat.tms_error) {
		printf("tm%d: error %d during config\n", ctlr,
			tpb->tm_stat.tms_error);
		tpb->tm_stat.tms_error = 0;
		return (0);
	}
	return (1);
}

int	tmtimer();
/*
 * Open the device.  Tapes are unique open
 * devices, so we refuse if it is already open.
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
tmopen(dev, flag)
	dev_t dev;
	int flag;
{
	register int mtunit;
	register struct mb_device *md;
	register struct mt_softc *sc;

	mtunit = MTUNIT(dev);
	if ((sc = &mt_softc[mtunit])->sc_openf)
		return (EBUSY);
	if (mtunit>=NMT || (md = tmdinfo[mtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);
	sc->sc_openf = 1;

	if (tmcommand(dev, TM_STATUS, 1)) {	/* interrupted */
		sc->sc_openf = 0;
		return (EIO);
	}
	if (!sc->sc_tms.tms_online || !sc->sc_tms.tms_ready) {
		uprintf("mt%d: not online\n", mtunit);
		sc->sc_openf = 0;
		return (EIO);
	}

	if ((flag&FWRITE) && sc->sc_tms.tms_prot) {
		uprintf("mt%d: no write ring\n", mtunit);
		sc->sc_openf = 0;
		return (EIO);
	}
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
	if (sc->sc_tact == 0) {
		sc->sc_timo = INF;
		sc->sc_tact = 1;
		timeout(tmtimer, (caddr_t)dev, 5*hz);
	}

	/*
	 * Rewind only the 1st time we call this open.
	 */
	if ((sc->sc_tms.tms_load) || sc->sc_firstopen) {
		/*
		 * First time through, ensure we're at BOT.
		 */
		if (sc->sc_firstopen) {
			if (!(sc->sc_tms.tms_load) &&
			    (tmcommand(dev, TM_REWIND, 0))) {
				uprintf("tm%d: could not rewind\n", mtunit);
				sc->sc_openf = 0;
				return (EIO);
			}
			sc->sc_firstopen = 0;
		}
	}
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
tmclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct mt_softc *sc = &mt_softc[MTUNIT(dev)];

	if (flag == FWRITE || (flag&FWRITE) && sc->sc_lastiow) {
		if (tmcommand(dev, TM_WEOF, 1) ||
		    tmcommand(dev, TM_WEOF, 1) ||
		    tmcommand(dev, TM_SPACE|TM_REVERSE, 1)) { /* interrupted */
			sc->sc_openf = 0;
		}
	}
	if ((minor(dev)&MT_NOREWIND) == 0)
		/*
		 * 0 count means don't hang waiting for rewind complete
		 * rather ctmbuf stays busy until the operation completes
		 * preventing further opens from completing by
		 * preventing a TM_SENSE from completing.
		 */
		(void) tmcommand(dev, TM_REWIND, 0);
	sc->sc_openf = 0;
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 * Allow signals to occur (with tsleep) so the
 * poor user can use his terminal if something goes wrong
 */
tmcommand(dev, com, count)
	dev_t dev;
	int com, count;
{
	register struct buf *bp;
	register int s;
	int error;

	bp = &ctmbuf[MTUNIT(dev)];
	s = SPLTM();
	while (bp->b_flags&B_BUSY) {
		/*
		 * This special check is because B_BUSY never
		 * gets cleared in the non-waiting rewind case.
		 */
		if (bp->b_repcnt == 0 && (bp->b_flags&B_DONE))
			break;
		bp->b_flags |= B_WANTED;
#ifdef TSLEEP
		/* XXX - tsleep can be replaced by new PCATCH option to sleep */
		if (tsleep((caddr_t)bp, PRITM, 0) == TS_SIG)
			return (1);
#else
		(void) sleep((caddr_t)bp, PRIBIO);
#endif
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);

	bp->b_dev = dev;
	bp->b_repcnt = count;
	bp->b_command = com;
	bp->b_blkno = 0;
	tmstrategy(bp);
	/*
	 * In case of rewind from close, don't wait.
	 * This is the only case where count can be 0.
	 */
	if (count == 0)
		return (0);

	s = SPLTM();
	while ((bp->b_flags&B_DONE) == 0) {
		bp->b_flags |= B_WANTED;
#ifdef TSLEEP
		/* XXX - tsleep can be replaced by new PCATCH option to sleep */
		if (tsleep((caddr_t)bp, PRITM, 0) == TS_SIG)
			return (1);
#else
		(void) sleep((caddr_t)bp, PRIBIO);
#endif
	}
	(void) splx(s);

	error = geterror(bp);
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags &= B_ERROR;		/* note: clears B_BUSY */
	return (error);
}

/*
 * Queue a tape operation.
 */
tmstrategy(bp)
	register struct buf *bp;
{
	int mtunit = MTUNIT(bp->b_dev);
	register struct mb_ctlr *mc;
	register struct buf *dp;
	register struct mt_softc *sc = &mt_softc[mtunit];
	int s;

	/*
	 * Put transfer at end of unit queue
	 */
	dp = &mtutab[mtunit];
	bp->av_forw = NULL;
	s = SPLTM();
	while (sc->sc_bufcnt >= MAXMTBUF)
		(void) sleep((caddr_t)&sc->sc_bufcnt, PRIBIO);
	sc->sc_bufcnt++;
	mc = tmdinfo[mtunit]->md_mc;
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
		tmstart(mc);
	(void) splx(s);
}

/*
 * Start activity on a tm controller.
 */
tmstart(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *bp, *dp;
	register struct mt_softc *sc;
	register struct tpb *tpb;
	int mtunit, cmd, count, size, fast;
	daddr_t blkno;

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
	mtunit = MTUNIT(bp->b_dev);
	sc = &mt_softc[mtunit];
	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this in tmintr().
	 */
	sc->sc_lastiow = 0;

	if ((int)sc->sc_openf < 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line).
		 */
		bp->b_flags |= B_ERROR;
		goto next;
	}

	fast = 0;
	count = 1;
	if (bp == &ctmbuf[mtunit]) {
		/*
		 * Execute control operation with the specified count.
		 */
		/*
		 * Set next state; give 5 minutes to complete
		 * rewind, or 10 seconds per iteration (minimum 60
		 * seconds and max 5 minutes) to complete other ops.
		 */
		switch (bp->b_command & ~TM_DIRBIT) {
		case TM_REWIND:
		case TM_UNLOAD:
			mc->mc_tab.b_active = SREW;
			sc->sc_timo = 5 * 60;
			fast = 1;
			break;

		case TM_SEARCH:
		case TM_REWINDX:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo = 20 * 60;
			fast = 1;
			break;

		default:
			mc->mc_tab.b_active = SCOM;
			sc->sc_timo =
			    imin(imax(10*(int)bp->b_repcnt, 60), 5*60);
			break;
		}
		count = bp->b_repcnt;
		cmd = bp->b_command;
		size = 0;
		goto dobpcmd;
	}
	/*
	 * The following checks handle boundary cases for operation
	 * on non-raw tapes.  On raw tapes the initialization of
	 * sc->sc_nxrec by tmphys causes them to be skipped normally
	 * (except in the case of retries).
	 */
	if (bdbtofsb(bp->b_blkno) > sc->sc_nxrec) {
		/*
		 * Can't read past known end-of-file.
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = ENXIO;
		goto next;
	}
	if (bdbtofsb(bp->b_blkno) == sc->sc_nxrec &&
	    bp->b_flags&B_READ) {
		/*
		 * Reading at end of file returns 0 bytes.
		 */
		clrbuf(bp);
		bp->b_resid = bp->b_bcount;
		goto next;
	}
	if ((bp->b_flags&B_READ) == 0) {
		/*
		 * Writing sets EOF
		 */
		sc->sc_nxrec = bdbtofsb(bp->b_blkno) + 1;
	}
	/*
	 * If the data transfer command is in the correct place,
	 * fire the operation up.
	 */
	if ((blkno = sc->sc_blkno) == bdbtofsb(bp->b_blkno)) {
		size = bp->b_bcount;
		if ((bp->b_flags&B_READ) == 0) {
			if (mc->mc_tab.b_active!=SERR && mc->mc_tab.b_errcnt) {
				cmd = TM_ERASE;
				mc->mc_tab.b_active = SERR;
				size = 0;
				count = 1;
			} else {
				cmd = TM_WRITE;
				mc->mc_tab.b_active = SIO;
			}
		} else {
			cmd = TM_READ;
			mc->mc_tab.b_active = SIO;
		}
		sc->sc_timo = 60;	/* premature, but should serve */
	} else {
		/*
		 * Tape positioned incorrectly;
		 * set to seek forwards or backwards to the correct spot.
		 * This happens for raw tapes only on error retries.
		 */
		mc->mc_tab.b_active = SSEEK;
		if (blkno < bdbtofsb(bp->b_blkno)) {
			cmd = TM_SPACE|TM_FORWARD;
			count = bdbtofsb(bp->b_blkno) - blkno;
		} else {
			cmd = TM_SPACE|TM_REVERSE;
			count = blkno - bdbtofsb(bp->b_blkno);
		}
		sc->sc_timo = imin(imax(10 * count, 60), 5 * 60);
		fast = count > 1;
		size = 0;
	}
dobpcmd:
	/*
	 * Do the command in bp.
	 */
	tpb = &tm_mb[mc->mc_ctlr]->tmb_tpb;
	clrtpb(tpb);
	tpb->tm_cmd = cmd &~ TM_DIRBIT;
	tpb->tm_ctl.tmc_rev = cmd & TM_DIRBIT;
	tpb->tm_ctl.tmc_width = 1;
	if (sc->sc_stream) {
		tpb->tm_ctl.tmc_speed = fast;
	} else {
		tpb->tm_ctl.tmc_speed = (minor(bp->b_dev) & T_HIDENS) ? 0 : 1;
	}
	tpb->tm_ctl.tmc_intr = 1;
	tpb->tm_ctl.tmc_tape = tmdinfo[mtunit]->md_slave & 03;
	tpb->tm_rcount = count;
	tpb->tm_bsize = size;
	if (size == 0)
		tmgo(mc);
	else
		(void) mbgo(mc);
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 */
	mc->mc_tab.b_errcnt = 0;
	dp->b_actf = bp->av_forw;
	iodone(bp);
	if (sc->sc_bufcnt-- >= MAXMTBUF)
		wakeup((caddr_t)&sc->sc_bufcnt);
	goto loop;
}

/*
 * The Multibus resources we needed have been
 * allocated to us; start the device.
 */
tmgo(mc)
	register struct mb_ctlr *mc;
{
	register struct tpb *tpb;

	mc->mc_tab.b_flags &=~ B_DONE;
	tpb = &tm_mb[mc->mc_ctlr]->tmb_tpb;
	if (tpb->tm_bsize != 0)
		c68t86((long)&DVMA[MBI_ADDR(mc->mc_mbinfo)], &tpb->tm_baddr);
	if (tm_mb[mc->mc_ctlr]->tmb_ccb.tmccb_gate != TMG_OPEN)
		printf("tmgo: gate wasn't open\n");
	tm_mb[mc->mc_ctlr]->tmb_ccb.tmccb_gate = TMG_CLOSED;
	ATTN(mc->mc_addr);
	mc->mc_tab.b_flags |= B_BUSY;
}

/*
 * Tm interrupt routine.
 */
tmintr(ctlr)
	int ctlr;
{
	register struct buf *dp;
	register struct buf *bp;
	register struct mb_ctlr *mc;
	register struct mt_softc *sc;
	register struct tpb *tpb;
	register int mtunit, state;
	int spins, err;

	mc = tmcinfo[ctlr];
	/* paranoid */
	if (mc == NULL || mc->mc_tab.b_actf == NULL ||
	    (mc->mc_tab.b_flags & B_BUSY) == 0 ||
	    tm_mb[ctlr]->tmb_ccb.tmccb_gate != TMG_OPEN)
		return;

	tpb = &tm_mb[ctlr]->tmb_tpb;
	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	if (bp == NULL)
		panic("tmintr: queuing error");
	mtunit = MTUNIT(bp->b_dev);
	if (mtunit >= NMT)
		panic("tmintr: queueing error 2");
	sc = &mt_softc[mtunit];
	sc->sc_tms = tpb->tm_stat;
	if (sc->sc_tms.tms_eot && (bp->b_flags & B_READ) == B_WRITE)
		sc->sc_resid = bp->b_bcount;
	else
		sc->sc_resid = bp->b_bcount - tpb->tm_count;

	/* clear the interrupt */
	clrtpb(tpb);
	tpb->tm_cmd = TM_NOP;
	tm_mb[ctlr]->tmb_ccb.tmccb_ccw = TMC_CLRINT;
	tm_mb[ctlr]->tmb_ccb.tmccb_gate = TMG_CLOSED;
	ATTN(mc->mc_addr);
	spins = 0;
	while (tm_mb[ctlr]->tmb_ccb.tmccb_gate != TMG_OPEN) {
		if (spins++ > 1000) {
			printf("tmintr: can't clear interrupt\n");
			break;
		}
		DELAY(10);
	}

	mc->mc_tab.b_flags &= ~B_BUSY;
	if ((state = mc->mc_tab.b_active) == 0) {
		printf("tm%d: stray interrupt\n", mc->mc_ctlr);
		return;
	}
	/*
	 * If last command was a rewind, and tape is still
	 * rewinding, wait for another interrupt, triggered
	 * by tmtimer
	 */
	if (state == SREW) {
		if (sc->sc_tms.tms_online &&
		    (!sc->sc_tms.tms_load || !sc->sc_tms.tms_ready))
			return;
		state = SCOM;
	}

	/*
	 * An operation completed... record status
	 */
	sc->sc_timo = INF;
	err = sc->sc_tms.tms_error;
	if ((bp->b_flags & B_READ) == 0)
		sc->sc_lastiow = 1;
	if (err == E_EOT && sc->sc_tms.tms_load)
		err = E_NOERROR;
	/*
	 * Check for errors.
	 */
	if (err != E_NOERROR && err != E_SHORTREC) {
		/*
		 * If we hit the end of the tape file, update our position.
		 */
		if (err == E_EOF || err == E_EOT) {
			tmseteof(bp);		/* set blkno and nxrec */
			state = SCOM;		/* force completion */
			sc->sc_resid = bp->b_bcount;
			goto opdone;
		}
		/*
		 * If error is not hard, and this was an i/o operation
		 * retry up to 8 times.
		 */
		if (state == SIO) {
			if (++mc->mc_tab.b_errcnt < 7) {
				sc->sc_blkno++;	/* force repositioning */
				goto opcont;
			}
		} else {
			/*
			 * Hard or non-i/o errors on non-raw tape
			 * cause it to close.
			 */
			if ((int)sc->sc_openf > 0 && bp != &rtmbuf[mtunit])
				sc->sc_openf = -1;
		}
		/*
		 * Couldn't recover error
		 */
		printf("mt%d: hard error bn=%d er=%x\n", mtunit,
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
		sc->sc_blkno++;
		break;

	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		if (bp == &ctmbuf[mtunit])
		switch (bp->b_command) {

		case TM_SPACE|TM_FORWARD:
		case TM_SPACEF|TM_FORWARD:
			sc->sc_blkno += bp->b_repcnt;
			break;

		case TM_SPACE|TM_REVERSE:
		case TM_SPACEF|TM_REVERSE:
			sc->sc_blkno -= bp->b_repcnt;
			break;

		case TM_REWIND:
		case TM_REWINDX:
			sc->sc_blkno = 0;
			break;
		}
		break;

	case SSEEK:
		sc->sc_blkno = bdbtofsb(bp->b_blkno);
		goto opcont;

	case SERR:
		goto opcont;

	default:
		panic("tmintr");
	}
opdone:
	mc->mc_tab.b_active = 0;
	mc->mc_tab.b_flags |= B_DONE;
opcont:
	if (mc->mc_mbinfo != 0)
		mbdone(mc);
	else
		tmdone(mc);
}

/*
 * Tm polling interrupt routine.
 */
tmpoll()
{
	register struct mb_ctlr *mc;
	register int ctlr;

	for (ctlr = 0, mc = tmcinfo[0]; ctlr < NTM; ctlr++, mc++) {
		if (mc != NULL && mc->mc_tab.b_actf != NULL &&
		    (mc->mc_tab.b_flags & B_BUSY) != 0 &&
		    tm_mb[ctlr]->tmb_ccb.tmccb_gate == TMG_OPEN) {
			tmintr(ctlr);
			return (1);
		}
	}
	return (0);
}

tmdone(mc)
	register struct mb_ctlr *mc;
{
	register struct buf *dp;
	register struct buf *bp;
	register struct mt_softc *sc;

	dp = mc->mc_tab.b_actf;
	bp = dp->b_actf;
	sc = &mt_softc[MTUNIT(bp->b_dev)];
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
		tmstart(mc);
}

tmtimer(dev)
	int dev;
{
	register struct mb_ctlr *mc = tmcinfo[TMUNIT(dev)];
	register struct mt_softc *sc = &mt_softc[MTUNIT(dev)];
	register struct tm_mbinfo *tmb = tm_mb[TMUNIT(dev)];
	register struct tpb *tpb;
	register int s, slave;

	if (sc->sc_timo != INF && (sc->sc_timo -= 5) < 0) {
		printf("mt%d: lost interrupt\n", MTUNIT(dev));
		sc->sc_timo = INF;
		s = SPLTM();
		tmintr(mc->mc_ctlr);
		(void) splx(s);
	}
	if (mc->mc_tab.b_active == SREW) {	/* check rewind status */
		s = SPLTM();
		if ((mc->mc_tab.b_flags & B_BUSY) == 0 &&
		    tmb->tmb_ccb.tmccb_gate == TMG_OPEN) {
			tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;
			tmb->tmb_ccb.tmccb_ccw = TMC_NORMAL;
			tpb = &tmb->tmb_tpb;
			clrtpb(tpb);
			tpb->tm_cmd = TM_STATUS;
			tpb->tm_ctl.tmc_width = 1;
			tpb->tm_ctl.tmc_intr = 1;
			slave = tmdinfo[MTUNIT(dev)]->md_slave & 03;
			tpb->tm_ctl.tmc_tape = slave;
			ATTN(mc->mc_addr);
			mc->mc_tab.b_flags |= B_BUSY;
		}
		(void) splx(s);
	}
	timeout(tmtimer, (caddr_t)dev, 5*hz);
}

tmseteof(bp)
	register struct buf *bp;
{
	register int mtunit = MTUNIT(bp->b_dev);
	register struct mt_softc *sc = &mt_softc[mtunit];
	register struct tpb *tpb = &tm_mb[TMUNIT(bp->b_dev)]->tmb_tpb;

	if (bp == &ctmbuf[mtunit]) {
		if (sc->sc_blkno > bdbtofsb(bp->b_blkno)) {
			/* reversing */
			sc->sc_nxrec = bdbtofsb(bp->b_blkno) - tpb->tm_rcount;
			sc->sc_blkno = sc->sc_nxrec;
		} else {
			/* spacing forward */
			sc->sc_blkno = bdbtofsb(bp->b_blkno) + tpb->tm_rcount;
			sc->sc_nxrec = sc->sc_blkno - 1;
		}
		return;
	}
	/* eof on read */
	sc->sc_nxrec = bdbtofsb(bp->b_blkno);
}

tmread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err;

	if (err = tmphys(dev, uio))
		return (err);
	return (physio(tmstrategy, &rtmbuf[MTUNIT(dev)], dev, B_READ,
	    minphys, uio));
}

tmwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	int err;

	if (err = tmphys(dev, uio))
		return (err);
	return (physio(tmstrategy, &rtmbuf[MTUNIT(dev)], dev, B_WRITE,
	    minphys, uio));
}

/*
 * Check that a raw device exists.
 * If it does, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 * Also make sure buffer is word-aligned (hardware requirement)
 */
tmphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int mtunit = MTUNIT(dev);
	register daddr_t a;
	register struct mt_softc *sc;
	register struct mb_device *md;

	if (mtunit >= NMT || (md=tmdinfo[mtunit]) == 0 || md->md_alive == 0)
		return (ENXIO);
	if ((int)uio->uio_iov->iov_base & 1)
		return (EINVAL);
	sc = &mt_softc[mtunit];
	a = bdbtofsb(uio->uio_offset / DEV_BSIZE);
	sc->sc_blkno = a;
	sc->sc_nxrec = a + 1;
	return (0);
}

/*ARGSUSED*/
tmioctl(dev, cmd, data, flag)
	dev_t dev;
	caddr_t data;
{
	int mtunit = MTUNIT(dev);
	register struct mt_softc *sc = &mt_softc[mtunit];
	register callcount;
	int fcount;
	struct mtop *mtop;
	struct mtget *mtget;
	/* we depend of the values and order of the MT codes here */
	static tmops[] = {
		TM_WEOF,
		TM_SEARCH|TM_FORWARD,
		TM_SEARCH|TM_REVERSE,
		TM_SPACEF|TM_FORWARD,
		TM_SPACEF|TM_REVERSE,
		TM_REWINDX,
		TM_UNLOAD,
		TM_STATUS,
	};

	switch (cmd) {

	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
		switch (mtop->mt_op) {
		case MTWEOF:
		case MTFSF: case MTBSF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;
		case MTFSR: case MTBSR:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		case MTREW: case MTOFFL: case MTNOP:
			callcount = 1;
			fcount = mtop->mt_count;
			break;
		default:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (ENXIO);
		while (--callcount >= 0) {
			if (tmcommand(dev, tmops[mtop->mt_op], fcount))
				return (EIO);
			if ((mtop->mt_op == MTFSR || mtop->mt_op == MTBSR) &&
			    sc->sc_tms.tms_eof)
				return (EIO);
			if (sc->sc_tms.tms_eot)
				break;
		}
		break;

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_dsreg = *(short *)&sc->sc_tms;
		mtget->mt_erreg = sc->sc_tms.tms_error;
		mtget->mt_resid = sc->sc_resid;
		mtget->mt_type = MT_ISCPC;
		mtget->mt_flags = MTF_REEL;
		mtget->mt_bf = 20;
		break;

	default:
		return (ENXIO);
	}
	return (0);
}

/*ARGSUSED*/
tmdump(dev)
	dev_t dev;
{

	return (ENXIO);
}

/*
 * Convert a 68000 address into a 8086 address
 * This involves translating a virtual address into a
 * physical multibus address and converting the 20 bit result
 * into a two word base and offset.
 */
static
c68t86(a68, a86)
	long a68;
	ptr86_t *a86;
{

	a68 -= (long)DVMA;
	a86->a_offset = a68 & 0xFFFF;
	a86->a_base = (a68 & 0xF0000) >> 4;
}
