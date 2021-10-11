#ifndef lint
static	char sccsid[] = "@(#)ar.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include "ar.h"
#if NAR > 0

/*
 * Driver for Archive "Intelligent" (hah!) Streaming Tape
 *
 * Device name is ar?
 *
 * TODO:
 *	test driver with more than one slave
 *	test driver with more than one controller
 *	test reset code
 *	Make this LINT without errors.
 *	Conditionally compile debug printfs so they don't take space.
 *	Conditionally compile multi-drive-per-controller code, ditto.
 *	Put bus error catch around RD/WR loops?
 *	What if tape is yanked in mid-operation?
 *	What if cable is yanked?  Kernel loops, or Timeout & kernel crash
 *	Make random addressing of block tapes work.
 *	Test with a file system on the tape.
 *	Check that all SPL()'s are correct and that all routines are
 *		entered at the right SPL().
 *	Change to Bill Joy Normal Form.
 *	Ensure that no infinite kernel hangs remain.
 *	Try to break it with invalid protocol, see if it hangs things.
 *	Try to break it with bad tapes (eg broken tape), ditto.
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/dir.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/user.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/uio.h>

#include <machine/mmu.h>
#include <machine/cpu.h>
#include <sundev/mbvar.h>
#include <sundev/arreg.h>

/*
 * Debugging and performance monitoring variables
 */
int ardebug = 0;	/* Print huge amounts of junk */
int arprintf = 0;	/* Print messages about hardware hangs and recovery */

#define Dprintf  if (ardebug) printf
#define PRF

/* Histogram flag gives distribution of queue sizes for tape buffers */
/* #define HISTOGRAM */

#define SPL()		spl3()
#define PRI		(PZERO+5)
/* We'll go this many times looking for Ready or Exception before quitting */
/* Quitting won't fix the problem but will prevent infinite kernel hangs. */
#define BREAKOUTCOUNT	100000
/* Glitchcount is the same thing, but for loops containing a printf, which
   takes lots more time, and clutters the console if you do it 1e5 times. */
#define GLITCHCOUNT	7

/* Define assorted debugging strings to save repeating them N times */
char ar_stat_bits[] = ARCH_BITS;

/*
 * Driver Multibus interface routines and variables.
 */
int	arprobe(), arattach(), arintr();


/*
 * Device information pointers, filled in by auto-config.
 */
struct	mb_device *ardinfo[NAR];

/*
 * The mb_driver structure tells auto-config who we are and
 * what we want.
 */
struct mb_driver ardriver = {
	arprobe, 0, arattach, 0, 0, arintr,
	sizeof (struct ardevice), "ar", ardinfo, 0, 0, 0,
};

/* bits in minor device */
/* The (short) is to avoid long multiplies when using this to subscript */
#define	ARUNIT(dev)	(short)(minor(dev)&03)
#define	ARCTLR(dev)	(short)((minor(dev)>>2)&0x03)
#define ARIDENT(dev)	(minor(dev)&0x0F)
#define ARUNWEDGE(dev)	((minor(dev)&0x20) != 0)
#define	T_NOREWIND	0x10

#define	INF	(daddr_t)10000000L

#ifdef HISTOGRAM
/*
 * Histogram buckets for queue length when an operation is queued.
 * 
 * The first bucket arenqhist[0] is for overflow (queue len > 50).
 * If the queue was empty before queueing, arenqhist[1] is incremented.
 */
short arenqhist[50] = 0;
#endif HISTOGRAM

/*
 * Maximum # of buffers which writes to the tape are allowed to consume.
 * If a user attempts to write more than this number, we will sleep until
 * we free up a buffer.
 *
 * This is initialized to 1/2 the buffer pool in our probe routine.
 * It is decremented when we queue a buffer, and incremented when we 
 * release one.  We will sleep, rather than enqueue, until it is positive.
 * We will wakeup after dequeueing if it is negative.
 */
int armaxbufs;


/* 
 * States into which the tape drive can get.
 */
enum ARstates {
	FINstate = 0x00, IDLEstate, CMDstate,	/* Finished, Idle, Command  */
	WFMinit,				/* Write File Mark */
	RFMinit,				/* Read to File Mark */
	REWinit,				/* Rewind tape */
	TENSEinit,				/* Retension tape */
	ERASEinit,				/* Erase tape */
	SELinit,				/* Select a drive */
	DESELinit,				/* Deselect all drives */
	RDSTinit,				/* Read status */
	CLOSEinit, CLOSEtwo, CLOSEthree,	/* Deassert aronline */
	READinit, READcmd, READburst, READfin, READidle,	/* Read */
	WRinit, WRcmd,   WRburst,   WRfin,   WRidle,		/* Write */
	CMDOKinit,				/* OK to issue commands? */
};

#ifdef PRF
char *arstatename[] = {
	"Fin", "Idle", "Cmd", 
	"Wfm", "Rfm", "Rew", "Tense", "Erase",
	"Sel", "Desel", "RdSt", "CloseInit", "CloseTwo", "CloseThree",
	"ReadInit", "ReadCmd", "ReadBurst", "ReadFin", "ReadIdle",
	"WriteInit", "WriteCmd", "WriteBurst", "WriteFin", "WriteIdle",
	"CmdOk",
};
#endif PRF
/*
 * Interesting tape commands
 */
#define AR_CLOSE	0	/* Close tape: WTM-if-writing, rewind */
#define AR_REWIND	1	/* Rewind (overlapped) */
#define AR_STATUS	2	/* Drive Status */
#define AR_READ		3	/* Read to MB memory */
#define AR_WRITE	4	/* Write to MB memory */
#define AR_WEOF		5	/* Write file mark (EOF) */ 
#define AR_ERASE	6	/* Erase entire tape */
#define AR_SELECT	7	/* Select drive of interest */
#define AR_DESELECT	8	/* Select no interesting drive */
#define AR_TENSE	9	/* Retension tape */
#define AR_SKIPFILE	10	/* Skip one file forward */
#define	AR_CMDOK	11	/* See if ok to do cmd */

/*
 * Translation between generic tape commands and initial states of
 * the Archive state machine.
 */
enum ARstates ar_cmds[] = {
	/* CLOSE */	CLOSEinit,
	/* REWIND */	REWinit,
	/* STATUS */	RDSTinit,
	/* READ */	READinit,
	/* WRITE */	WRinit,
	/* WEOF */	WFMinit,
	/* ERASE */	ERASEinit,
	/* SELECT */	SELinit,
	/* DELSELECT */	DESELinit,
	/* TENSE */	TENSEinit,
	/* SKIPFILE */	RFMinit,
	/* CMD OK? */	CMDOKinit,
};


/*
 * Software state per tape controller.
 *
 * 1. A tape controller is a unique-open device; we refuse 2nd opens.
 * 2. We keep track of the current position on a block tape.  If a request 
 *    is made for the wrong (non-sequential) block, we give an error.
 *    This avoids problems with readaheads and such, I think. -- JCGnu
 * 3. We remember if the last operation was a write on a tape, so if a tape
 *    is open read write and the last thing done is a write we can
 *    write an end of tape mark.
 * 4. We remember the status registers after the last command, using
 *    then internally and returning them to the SENSE ioctl.
 */
struct	ar_softc {
	char	sc_initted;	/* Is controller initialized yet? */
	char	sc_openf;	/* lock against multiple opens */
	char	sc_attached;	/* Is controller attached to system? */
	char	sc_lastiow;	/* last op was a write */
	char	sc_eoflag;	/* raw eof flag */
	u_char	sc_cmdok;	/* 0 => can only issue read/RFM or write/WFM */
				/* The protocol for this is: 
					sc->sc_cmdok = 0;
					(void) arcommand(dev, AR_CMDOK, 1);
					if (sc->sc_cmdok) ...		     */
	char	sc_selecteddev;	/* currently selected drive */
	char	sc_selectok;	/* OK for aropen() to issue select command */
				/* We remember this because tape drive won't
				   tell us if it's OK.  It's OK if at BOT,
				   or if tape inserted but not yet positioned
				   to BOT.  Thanks assholes at Archive. */
	char	sc_quietly;	/* =1 if errors should not printf; they will
				   be reported by more intelligent high level
				   software.  Normally 0. */
	char	sc_writeable;	/* =1 if opened for write */
	daddr_t	sc_blkno;	/* block number, for block device tape */
	daddr_t	sc_nxrec;	/* position of end of tape, if known, or INF */
	enum ARstates sc_state;	/* Current state of hard/software */
	enum ARstates sc_oldstate;  /* Previous state of sc_state */
	enum ARstates sc_hangstate; /* Cur state when Rdy hang occurs */
	struct arstatus sc_status; /* Status at last "Read status" cmd */
	int	sc_size;	/* Size of buffer to read/write */
	char 	*sc_bufptr;	/* Pointer to buffer to read/write */
	int	sc_count;	/* # times to repeat high-level op */
	u_char	sc_drive;	/* Drive # to select/deselect */
	u_char	sc_histate;	/* Higher level state than sc_state */
	struct ardevice *sc_addr;/* Address of I/O registers */
	u_char	sc_qidle;	/* =0 if buf in progress, =1 if not. */
	short	sc_stops;	/* number of tape stops since last MTIOCGET */
	short	sc_softerrs;	/* number of soft errors since last MTIOCGET */
	int	sc_ident;	/* Identifying number of this drive/ctlr */
	struct	buf sc_pend_chain; /* head of queue of buffers awaiting I/O */
	struct	buf cbuf;	/* fake buf for ioctl's and other commands */
	struct	buf rbuf;	/* for raw tape ops; tape errs OK in an rbuf */
/* When adding new fields to ar_softc, also initialize them in arinit(). */
} ar_softc[NAR];


/*
 * Redefined fields in the buf header for cbufs (control commands)
 */
#define	b_repcnt  b_bcount
#define	b_command b_resid


/*
 * States for sc->sc_histate, the per controller state flag.
 * This is used to sequence control in the driver.
 */
#ifdef SEEK
#define	SSEEK	1		/* seeking for correct posn (unused) FIXME */
#endif SEEK
#define	SIO	2		/* doing seq i/o */
#define	SCOM	3		/* sending control command */
#define SSWAIT	4		/* Previous block completed, nothing new
				   started.  If an interrupt occurs, look
				   in sc_pend_chain for new bufs to start. */


/*
 * Determine if there exist a device at address <reg>.
 */
arprobe(reg)
	register caddr_t reg;
{
	extern int nbuf;		/* # buffers in the system */

	armaxbufs = nbuf >> 1;		/* Set max # bufs we'll allow ourself */

	if (pokec((char *)(&((struct ardevice *)reg)->arunwedge), 0)) {
		return (0);		/* Bus error trying to touch it */
	}
	return (sizeof (struct ardevice));
}

/*
 * Record attachment of the unit to the system.
 */
/*ARGSUSED*/
arattach(md)
	register struct mb_device *md;
{
	register struct ar_softc *sc;

	sc = &ar_softc[md->md_unit];
	sc->sc_addr = (struct ardevice *)md->md_addr;
	sc->sc_attached++;
}

/*
 * Initialize a controller
 */

#define SPININIT 1000000

arinit(sc, dev)
	register struct ar_softc *sc;
	dev_t dev;
{
	register int count;
	register struct ardevice *araddr = sc->sc_addr;

	sc->sc_initted = 0;			/* until later */
	sc->sc_openf = 0;
	sc->sc_lastiow = 0;
	sc->sc_selecteddev = -1;
	sc->sc_selectok = 1;
	sc->sc_stops = 0;
	sc->sc_softerrs = 0;
	sc->sc_quietly = 0;
	sc->sc_writeable = 0;
	sc->sc_blkno = 0;
	sc->sc_nxrec = INF;
	sc->sc_state = IDLEstate;
	sc->sc_oldstate = IDLEstate;
	sc->sc_hangstate = IDLEstate;
/*	sc->sc_status = 0;  Doesn't work, so leave it alone. */
	sc->sc_size = 0;
	sc->sc_bufptr = (char *) 0x1ff001;	/* very funny buf addr */
/*	sc->sc_attached had better already be 1. */
	sc->sc_drive = 0;
	sc->sc_histate = 0;
/*	sc->sc_addr had better already be initialized. */
	sc->sc_qidle = 1;
	sc->sc_stops = 0;
	sc->sc_eoflag = 0;
	sc->sc_cmdok = 0;
	sc->sc_ident = ARIDENT(dev);
/* When adding new fields to softc, be sure to initialize them here. */
/* FIXME, should initialize buffer headers here too */

	araddr->arunwedge = 1;		/* Take it out of burst mode wedge */
	araddr->arburst = 0;
	araddr->arreq = 0;
	araddr->arxfer = 0;
	araddr->arcatch = 0;
	araddr->arexcie = 0;
	araddr->arrdyie = 0;

	/*
	 * If tape is up from previous system operation,
	 * take it down gently.
	 */
	if (araddr->aronline) {
Dprintf("ar*init tape online\n");
		araddr->aronline = 0;	/* Writes TM (if writing) & rewinds */
	}

	count = SPININIT;
	while (!araddr->arrdy && !araddr->arexc && count)
		count--;
if (count == 0) Dprintf("ar: Timeout waiting for Ready at %x\n", araddr);

if (araddr->arrdy) Dprintf("ar*init arrdy on before reset\n");
if (araddr->arexc) Dprintf("ar*init arexc on before reset\n");

	/* Tape is ready or exceptional.  Reset it for good measure. */
	araddr->arreset = 1;
	DELAY(25);	/* at least 13 usec */
	araddr->arreset = 0;

	count = SPININIT;
	while (!araddr->arexc && count)
		count--;
	if (count == 0) {
		/* This happens if no tape ctlr is there, so be silent. */
Dprintf("ar: Timeout waiting for Exception after reset\n");
		return (0);
	}

	/* Now read back status from the reset. */
	sc->sc_initted = 1;	/* Must do first so interrupt OK */
	araddr->aronline = 1;	/* Must do first so RDST microcode doesn't
				   play games with arrdy line.  See comments
				   in aropen(). */
	if (arcommand(dev, AR_STATUS, 1)) {
		araddr->aronline = 0;
		sc->sc_initted = 0;	/* Try again on next open */
		return (0);
	}

	sc->sc_openf = 0;	/* In case arcommand() set it to -1 */
	return (1);
}

/*
 * Open the device.
 *
 * Archive controllers can control up to 4 drives, but only one of them
 * can be off BOT at once.  So we only allow one drive per controller to
 * be open at once, and just do a "select" command when the drive is 
 * opened, to point the controller to that drive.
 *
 * We also check that a tape is available, and
 * don't block waiting here; if you want to wait
 * for a tape you should timeout in user code.
 */
aropen(dev, flag)
	dev_t dev;
	int flag;
{
	register int unit = ARCTLR(dev);
	register struct ar_softc *sc = &ar_softc[unit];
	register struct ardevice *araddr;
	int foo;

Dprintf("ar*open(%d, %d)\n", dev, flag);

	if (unit >= NAR || !sc->sc_attached) {
		uprintf("ar%d: no such controller\n", ARIDENT(dev));
		return (ENXIO);
	}

	/*
	 * Initialize the controller, if this is the first open.
	 *
	 * If arinit() claims the device doesn't exist, believe it.
	 * (On the next open we will call init again.)
	 */
	if (!sc->sc_initted || ARUNWEDGE(dev)) {
		if (!arinit(sc, dev)) {
			uprintf("ar%d: would not initialize\n", ARIDENT(dev));
			return (ENXIO);
		}
	}

	if (sc->sc_openf) {
		uprintf("ar%d: already open\n", sc->sc_ident);
		return (EBUSY);
	}

	sc->sc_openf = 1;

	/*
	 * There is a problem in the Archive in that after each command,
	 * it goes thru a cleanup routine.  If aronline is not asserted,
	 * this cleanup routine drops arrdy while it rewinds the tape
	 * to BOT.  It deasserts arrdy for 90 us even if the tape is
	 * already at BOT.  This causes us problems because we get a arrdy
	 * interrupt and then discover that arrdy is gone.
	 *
	 * The problem has been circumvented with a two-pronged approach.
	 * The first prong is to always have ONLINE turned on.  There is
	 * one place (CLOSEinit/CLOSEtwo) where we can't do that, so 
	 * we added a kludge test to arintproc() too.
	 *
	 * This info was obtained from Lou Domshy, Mgr. of Product Mgmt and
	 * Applications Engineering(?) at the Archive factory, 714-641-0279,
	 * on 1 December 1982, by John Gilmore of Sun Microsystems.
	 */
	araddr = sc->sc_addr;
	araddr->aronline = 1;	/* Let ctrlr know we are doing a series */

	/*
	 * First select the drive we're interested in.
	 *
	 * Since the select command doesn't work when we aren't at BOT,
	 * we have to reject the open if we aren't opening the same drive as
	 * last time.  
	 */
	sc->sc_quietly = 1;		/* Let us do the error reporting */
	if (sc->sc_selecteddev != ARUNIT(dev)) {
		if (!sc->sc_selectok) {
			uprintf("ar%d: can't switch drives in mid-tape\n",
				sc->sc_ident);
			sc->sc_openf = 0;
			return (EIO);
		}
		foo = arcommand(dev, AR_SELECT, 1);
		if (foo == EINTR) {
/* FIXME: May want to expand this to a variety of errors */
Dprintf("ar*open SELECT gave EINTR\n");
err:
			foo = sc->sc_openf;
			sc->sc_openf = 1;	/* In case cmd set to -1 */
			(void) arcommand(dev, AR_DESELECT, 1); /* Unlight LED */
			sc->sc_selecteddev = -1;	/* We dunno who */
			sc->sc_selectok = 1;		/* But it's OK to chg */
			sc->sc_quietly = 0;		/* Error reports on */
			araddr->aronline = 0;
			/* After hard error, force re-init. */
			if (foo < 0 || (int)sc->sc_openf < 0)
				sc->sc_initted = 0;
			sc->sc_openf = 0;
			return (EIO);
		}
		sc->sc_selecteddev = ARUNIT(dev);
	}

	/*
	 * Now get its status and check on a few things.
	 */
	sc->sc_cmdok = 0;
	(void) arcommand(dev, AR_CMDOK, 1);	/* See if OK to issue cmds */
	if (sc->sc_cmdok) {
		if (arcommand(dev, AR_STATUS, 1)) {	/* interrupted */
			goto err;
		}
		if (sc->sc_status.NoDrive) {
			uprintf("ar%d: no such drive\n", sc->sc_ident);
			goto err;
		}	
		if (sc->sc_status.NoCart) {
			uprintf("ar%d: no cartridge in drive\n", sc->sc_ident);
			goto err;
		}
	}

	if ((flag&FWRITE) && sc->sc_status.WriteProt) {
		uprintf("ar%d: cartridge is write protected\n", sc->sc_ident);
		goto err;
	}
	sc->sc_quietly = 0;	/* Turn error reports to console back on */
	sc->sc_writeable = 0 != (flag&FWRITE);
	sc->sc_blkno = (daddr_t)0;
	sc->sc_nxrec = INF;
	sc->sc_lastiow = 0;
Dprintf("ar*open exiting\n");
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
arclose(dev, flag)
	register dev_t dev;
	register flag;
{
	register struct ar_softc *sc = &ar_softc[ARCTLR(dev)];

Dprintf("ar*close(%d, %d)\n", dev, flag);

	if ((int)sc->sc_openf <= 0)
		goto seeyalater;
	else if (minor(dev)&T_NOREWIND) {
		sc->sc_selectok = 0;	/* Probably not OK to select drive */
		if (sc->sc_lastiow) {
			/*
			 * No Rewind, and last op was write.
			 * Write a file mark, and continue.  Leave aronline on.
			 * Read status to get tape stops info.
			 */
			(void) arcommand(dev, AR_WEOF, 1);
			(void) arcommand(dev, AR_STATUS, 1);
		} else {
			/*
			 * No rewind, and last op was read/position.
			 * If in mid-file (not at tapemark) we can't do
			 * anything at all.
			 * See if tape is at BOT.  If so, de-select it.
			 * Reading status also tells us tape stops
			 * (breaks in streaming) info.
			 */
			sc->sc_cmdok = 0;
			(void) arcommand(dev, AR_CMDOK, 1);
			if (sc->sc_cmdok) {
				(void) arcommand(dev, AR_STATUS, 1);
				if (sc->sc_status.BOT) {
					(void) arcommand(dev, AR_DESELECT, 1);
					sc->sc_selecteddev = -1;
					sc->sc_selectok = 1;	/* OK to sel */
				}
			}
		}
	} else {
		/*
		 * Write file mark and rewind, by dropping aronline.
		 * FIXME.  These 3 commands should be moved into AR_CLOSE
		 * in order that the user program can continue while the
		 * tape is rewinding.
		 */
		(void) arcommand(dev, AR_CLOSE, 1);	/* Shut down things */
		(void) arcommand(dev, AR_STATUS, 1);	/* Read block counts */
		(void) arcommand(dev, AR_DESELECT, 1);	/* Turn LED off */
		sc->sc_selecteddev = -1;
		sc->sc_selectok = 1;	/* OK for aropen() to sel drive */
		sc->sc_eoflag = 0;	/* Not at eof after rewind */
	}
seeyalater:
	/* After hard error, force re-init. */
	if ((int)sc->sc_openf < 0) {
		sc->sc_initted = 0;
		sc->sc_addr->aronline = 0;	/* Terminate tape op if any */
		/* This might give an interrupt but with initted=0 we will
		   ignore and disable anyway. */
	}
	sc->sc_openf = 0;
Dprintf("ar*close exiting\n");
}

/*
 * Execute a command on the tape drive
 * a specified number of times.
 * Allow signals to occur (with arsleep) so the
 * poor user can use his terminal if something goes wrong
 *
 * FIXME: Should a low-level death wakeup bp and armaxbufs?
 */
arcommand(dev, com, count)
	dev_t dev;
	int com, count;
{
	register struct buf *bp;
	register struct ar_softc *sc = &ar_softc[ARCTLR(dev)];
	register int error, s;

Dprintf("ar*command(%x, %d, %d)\n", dev, com, count);
	if ((int)sc->sc_openf < 0)
		return (EIO);

	bp = &sc->cbuf;
	s = SPL();
	while (bp->b_flags&B_BUSY) {
		bp->b_flags |= B_WANTED;
		if (arsleep((caddr_t)bp, PRI)) {
			(void) splx(s);
			return (EINTR);
		}
	}
	bp->b_flags = B_BUSY|B_READ;
	(void) splx(s);

	bp->b_dev = dev;
	bp->b_repcnt = count;
	bp->b_command = com;
	bp->b_blkno = 0;
#ifdef PRF2
Dprintf("ar*command before strategy %x\n", bp);
#endif PRF
	arstrategy(bp);
#ifdef PRF2
Dprintf("ar*command after strategy %x\n", bp);
#endif PRF
	s = SPL();
	while ((bp->b_flags&B_DONE)==0) {
		bp->b_flags |= B_WANTED;
		if (arsleep((caddr_t)bp, PRI))
			return (1);
	}
	(void) splx(s);

#ifdef PRF2
Dprintf("ar*command before geterror\n");
#endif PRF
	error = geterror(bp);
	if (bp->b_flags&B_WANTED)
		wakeup((caddr_t)bp);
	bp->b_flags = 0;		/* note: clears B_BUSY */
	return (error);
}

/*
 * Queue a tape operation.
 */
arstrategy(bp)
	register struct buf *bp;
{
	int unit  = ARCTLR(bp->b_dev);
	struct ar_softc *sc = &ar_softc[unit];
	register struct buf *dp = &sc->sc_pend_chain;
	int s;

	bp_mapin (bp);
Dprintf ("ar*strategy bp=%x mapped in; addr=%x\n", bp, bp->b_un.b_addr);
	bp->av_forw = NULL;
	s = SPL();
	/*
	 * See if we have used up our quota of write buffers; if so, sleep.
	 */
	if ((bp->b_flags & B_READ) == 0 && bp != &sc->cbuf && 
	     bp != &sc->rbuf) {
		armaxbufs--;
		while (armaxbufs < 0) {
Dprintf("ar*strategy sleeping for maxbufs\n");
/* FIXME: This code has never been tested, since the block device
   is "unsupported".  Test it and fix it. -- JCG 2Jun83 */
			if (arsleep((caddr_t)&armaxbufs, PRI)) {
				/* Sleep interrupted, cancel out */
				armaxbufs++;
				bp->b_flags |= B_ERROR;
				bp->b_error = EINTR;
				sc->sc_histate = SSWAIT;
				sc->sc_pend_chain.b_actf = bp->av_forw;
				biodone(bp);
				goto goaway;
			}
/* FIXME:  End of untested code. */
		}
	}
	/*
	 * Put transfer at end of unit queue
	 */
	if (dp->b_actf == NULL) {
		dp->b_actf = bp;
		dp->b_forw = NULL;
	} else
		dp->b_actl->av_forw = bp;
	dp->b_actl = bp;

#ifdef HISTOGRAM
	/* Count how big the queue now is, and histogram that. */
	count = 0;
	bp = dp;	/* Start at head of queue */
	while ((bp = bp->av_forw) != NULL)
		count++;
	if (count >((sizeof arenqhist)/(sizeof arenqhist[0])))
		count = 0;
	arenqhist[count]++;	/* Histogram the queue size. */

Dprintf("ar*strategy queueing %x, qlen %d, maxbufs %d\n", bp, count, armaxbufs);
#else  HISTOGRAM

Dprintf("ar*strategy queueing %x, maxbufs %d\n", bp, armaxbufs);
#endif HISTOGRAM


	/*
	 * If the controller is not busy, get
	 * it going.
	 */
	if (sc->sc_qidle)
		arstart(unit);

goaway:
	(void) splx(s);
}

/*
 * Start activity on a device.
 *
 * Each command to be sent to a device is in a buffer. (If it's a
 * control command, the buffer is the cbuf in softc for this controller.
 * If it's a block read or write, the buffer comes out of the system buffer
 * cache.  If it's a raw read or write, the buffer is the rbuf in softc.)
 * The commands are queued in order in a singly linked list which comes off
 * sc_pend_chain. (We keep track of both the first and last things on the
 * list; it's otherwise singly linked.)  Buffers are always added on the
 * end of the chain and taken off the front.  This routine starts the I/O
 * requested by the buffer on the front of the chain.  It stays on the front
 * of the chain until it is completed, then it is dequeued and we might 
 * start the next op, if there is one.
 *
 * When entered, sc_qidle must be true (the queue must be idle).
 *
 * Must be entered at SPL() or higher.
 *
 * This is SOFTWARE oriented software.  It doesn't know or care about the
 * state of the hardware; what it knows about is the software (buffers, 
 * queued commands, etc).  It calls arstart_cmd() to kick the hardware.
 *
 * FIXME!  This routine diddles internal sc_ variables of the interrupt
 * low-level routine.  IT MUST NOT, until it knows that it's safe to
 * initiate the I/O -- that is, the low-level stuff is IDLE or FIN.  Then
 * it will be safe to do serious things in the low-level while no buffer
 * is atop the queue (eg, rewind tape after releasing buffer).
 */
arstart(unit)
	register int unit;
{
	register struct ar_softc *sc = &ar_softc[unit];
	register struct buf *bp;
	int cmd;

#ifdef PRF2
Dprintf("ar*start(%d)\n", unit);
#endif

if (!sc->sc_qidle) panic("arstart qidle not set");


loop:
	/*
	 * See if this controller has any buffers queued for it.
	 */
	if ((bp = sc->sc_pend_chain.b_actf) == NULL) {
Dprintf("ar*start unit %d found empty\n", unit);
		return;
	}

#ifdef PRF
Dprintf("ar*start unit %d sc %x bp %x\n", unit, sc, bp);
#endif
	bp_mapin (bp);
Dprintf ("ar*start bp=%x mapped in; addr=%x\n", bp, bp->b_un.b_addr);


	/*
	 * Default is that last command was NOT a write command;
	 * if we do a write command we will notice this below.
	 */
	sc->sc_lastiow = 0;

	if ((int)sc->sc_openf < 0) {
		/*
		 * Have had a hard error on a non-raw tape
		 * or the tape unit is now unavailable
		 * (e.g. taken off line, or broken).
		 */
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		goto next;
	}
	if (bp == &sc->cbuf) {
		/*
		 * Execute control operation with the specified count.
		 */
		sc->sc_histate = SCOM;	/* State for interrupt response */
		cmd = bp->b_command;
		sc->sc_size = 0;
		if (cmd == AR_SKIPFILE) {
			/*
			 * When we get the EXCEPTION/FileMark at the end of
			 * this SKIPFILE, set end-of-file pointer to blkno,
			 * which happens to be infinity; this lets us read
			 * the next file if we care to.
			 */
			sc->sc_blkno = INF;
		}
	} else {
		/*
		 * Did we hit file mark before dispatching the last buffer?
		 * If so, the next read (this one) should return 0 bytes.
		 * This flag is cleared by ioctl
		 * commands that move the tape, and by RFM command.
		 */
		if ((bp == &sc->rbuf) && sc->sc_eoflag) {
			sc->sc_eoflag = 0;
			bp->b_resid = bp->b_bcount;
			goto next;
		}
		/*
		 * Do the data transfer specified by bp (read or write).
		 * This could be a block transfer or a raw transfer, we
		 * really don't care.
		 */

		/*
		 * The following checks handle boundary cases for operation
		 * on block tapes.  On raw tapes the initialization of
		 * sc->sc_nxrec by arphys causes them to be skipped normally
		 * (except in the case of retries).
		 *
		 * These are important because block tapes do read-ahead.
		 * When we detect an EOF, we must derail the readahead requests
		 * which arrive (or are already queued) after the EOF. (Block
		 * tapes only allow you to read one file.)
		 */
		if (bp->b_blkno > sc->sc_nxrec) {
			/*
			 * Can't read past known end-of-file.
			 */
			bp->b_flags |= B_ERROR;
			bp->b_error = EIO;
			goto next;
		}
		if (bp->b_blkno == sc->sc_nxrec &&
		    bp->b_flags&B_READ) {
			/*
			 * Reading at end of file returns 0 bytes.
			 */
			clrbuf(bp);
			bp->b_resid = bp->b_bcount;
			goto next;
		}
		if ((bp->b_flags&B_READ) == 0)
			/*
			 * Writing sets EOF
			 */
			sc->sc_nxrec = bp->b_blkno+(bp->b_bcount >> AR_BSHIFT);
		/*
		 * If the data transfer command is in the correct place,
		 * set up, and start the I/O.
		 */
		if (sc->sc_blkno == bp->b_blkno) {
			sc->sc_size = bp->b_bcount;
			sc->sc_histate = SIO;
			if ((bp->b_flags&B_READ) == 0) {
				cmd = AR_WRITE;
				sc->sc_lastiow = 1;
			} else {
				cmd = AR_READ;
			}
		} else {
			/*
			 * Tape positioned incorrectly;
			 * set to seek forwards or backwards to the correct spot.
			 * This happens for raw tapes only on error retries.
			 */
			/* We eventually want to rewind and/or forwardspace. */
Dprintf("ar: block %d requested at tape posn %d, bp=%x\n",
 bp->b_blkno, sc->sc_blkno, bp);
			bp->b_flags |= B_ERROR;
			bp->b_error = ENOTBLK;	/* This ain't a block dev */ 
			goto next;
#ifdef SEEK
			/* sc->sc_histate = SSEEK; not implem */
			/* sc->sc_size = 0; we don't care since not implem. */
#endif SEEK
		}
	}

	/*
	 * Start the physical device (by invoking the state machine).
	 * If the operation is not complete, just return.  If it is,
	 * unchain the request and call biodone() to free up the user
	 * process or whoever.
	 */
	sc->sc_drive = ARUNIT(bp->b_dev);
	sc->sc_bufptr = bp->b_un.b_addr;
	/* sc->sc_size is already set. */

	sc->sc_qidle = arstart_cmd(sc, ar_cmds[cmd]);	/* Start it. */
	if (sc->sc_qidle == 2) {
		/* error in starting the command */
		sc->sc_qidle = 1;
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		goto next;
	}
	if (sc->sc_qidle)
		goto loop; /* This buf done */
	return;

next:
	/*
	 * Done with this operation due to error or
	 * the fact that it doesn't do anything.
	 */
Dprintf("ar*start op completed bp %x addr %x size %x blkno %d flags %x err %d\n"
  , bp, bp->b_un.b_addr, bp->b_bcount, bp->b_blkno, bp->b_flags, bp->b_error);
	sc->sc_histate = SSWAIT;

	sc->sc_pend_chain.b_actf = bp->av_forw;

	/*
	 * We're done with this buffer; allow another one to be queued.
	 */
	if ((bp->b_flags & B_READ) == 0 && bp != &sc->cbuf && 
	     bp != &sc->rbuf) {
		if (armaxbufs <= 0)
			wakeup((caddr_t)&armaxbufs);
		armaxbufs++;
Dprintf("ar*start buffer done, maxbufs=%d\n", armaxbufs);
	}

	biodone(bp);
	goto loop;
}


/*
 * Begin execution of a device command for the device pointed to by
 * sc.  The command begins execution in state newstate.
 *
 * This is HARDWARE oriented software.  It doesn't know or care of
 * state of buffers, etc.  Its result reflects what the hardware is
 * doing, not what the software is doing.
 *
 * The device is assumed to be in one of the FIN or IDLE states:
 *	IDLEstate, FINstate, READfin, READidle, WRfin, or WRidle.
 * This is a requirement, since various fields in sc have already
 * been set up for us, and the use of those fields would conflict
 * with their use by the interrupt routine if we weren't idle or fin.
 *
 * Our result is:
 *	0	if the operation has been started, but is not yet
 *		complete;
 *	1	if the operation is complete.
 *	2	if the operation is invalid and was not started.
 *
 * Must be entered at SPL() or higher.
 *
 * FIXME, this is too big a snake pit to leave this way.
 * (Actually much less of a pit nowadays.  Think about it.)
 */
arstart_cmd(sc, newstate)
	register struct ar_softc *sc;
	register enum ARstates newstate;
{
	register enum ARstates oldstate = sc->sc_state;

	/*
	 * Continuous Reads and Writes need not go back to READinit
	 * or WRinit; they can just tail-end into the Burst state.
	 */
	if ((oldstate == WRidle || oldstate == WRfin)) {
		if (newstate == WRinit)
			newstate = WRburst;
		else if (newstate != CLOSEinit && newstate != WFMinit)
			return (2);
	} else if ((oldstate == READidle || oldstate == READfin)) {
		if (newstate == READinit)
			newstate = READburst;
		else if (newstate == REWinit)
			newstate = CLOSEinit;
		else if (newstate != CLOSEinit && newstate != RFMinit)
			return (2);
	/*
	 * Bitch if we are not in a reasonable state now
	 */
	} else if (oldstate != IDLEstate && oldstate != FINstate) {
		printf("ar: start_cmd bad state %s=%x for newstate %s=%x\n",
			arstatename[(int)oldstate], oldstate,
			arstatename[(int)newstate], newstate);
		sc->sc_openf = -1;	/* Force close & re-initialize */
		return (2);	/* Cancel it, we know it'll just screw h/w */
	}

	sc->sc_oldstate = sc->sc_state;
	sc->sc_state = newstate;
#ifdef FIXME
	/*
	 * These assignments should happen here, once we know the 
	 * low level is in an idle state.  They currently happen in
	 * arstart() instead, which is a bug.
	 */
	sc->sc_drive = ARUNIT(bp->b_dev);
	sc->sc_bufptr = bp->b_un.b_addr;
	/* sc->sc_size is already set. */
#endif FIXME

Dprintf("ar*starting old state %s=%x, new %s=%x, addr %x, size %x\n",
	arstatename[(int)oldstate], oldstate,
	arstatename[(int)newstate], newstate, sc->sc_bufptr, sc->sc_size);

	/*
	 * If we were in an IDLE state, call arintproc() to begin
	 * the operation.  If we were in a FIN state, waiting for a
	 * completion interrupt, just keep waiting; the interrupt
	 * will cause us to kick off the machine in its new state.
	 * If we do run it now, and it returns 1, the op is done.
	 */
	if (oldstate == IDLEstate || oldstate == READidle ||
	    oldstate == WRidle) {
		return (arintproc(sc));
	}

	return (0);	/* Command not finished */
}



/*
 * Our interrupt routine.
 *
 * Must be entered at SPL() or higher.
 *
 * Result is 0 if this is not our interrupt, 1 if it is.
 */
arintr()
{
	register struct ar_softc *sc;
	register struct ardevice *araddr;
	register int unit;

	for (unit = 0, sc = ar_softc; unit < NAR; unit++, sc++) {
		if (sc->sc_attached) {
			araddr = sc->sc_addr;
#ifdef PRF2
Dprintf("ar*intr araddr %x unit %d sc %x\n", araddr, unit, sc);
#endif
			if (araddr->arintr)
				goto found;
		}
	}
	return (0);	/* Not any of our devices, boss... */

	/*
	 * We found a device that is interrupting us.
	 *
	 * unit, sc, and araddr are all set up.
	 */
found:

	if (!sc->sc_initted) {
		/*
		 * This interrupt is for a device which we have not opened
		 * yet.  Since we don't really want to deal with it yet, 
		 * just shut off the interrupt and tell Mother we're done.
		 * When somebody opens it, we'll go thru the whole arinit()
		 * rigamarole.
		 */
		printf("ar: interrupt from uninitialized controller %x\n",
			araddr);
		araddr->arrdyie = 0;
		araddr->arexcie = 0;
		return (1);
	}

	/*
	 * Process the interrupt.
	 *
	 * If this interrupt causes the current operation (chained via
	 * sc_pend_chain) to complete, arintproc() returns 1, and we
	 * see if we can start the next one.
	 *
	 * In either case, we return to ioint indicating that the interrupt
	 * has been serviced.
	 */
	sc->sc_qidle = arintproc(sc);
	if (sc->sc_qidle) {
		/* There is no buffer currently active.  Start I/O on
		   the new current buffer, if there is one. */
		if (sc->sc_pend_chain.b_actf != NULL)
			arstart(unit);
	}

	return (1);		/* Interrupt was serviced. */

}



/*
 * Process an interrupt, or an initial action on the device.
 *
 * We must be entered at interrupt level SPL().
 *
 * We negate then assert arcatch, so it will catch the next
 * transition of arrdy, which might occur while we are running.
 * If armachine() exits with arrdyie, the transition also causes
 * an interrupt. (It normally does, unless we are idle.)
 *
 * The result of arintproc() is:
 *	0 	if an operation is still in progress.
 *	1	if no operation is in progress.  Another one can be started.
 *
 * We depend on armachine() to never return a zero once it has returned 
 * a one, until somebody (arstart_cmd, or our SIO case stmt) twitches its state.
 */
arintproc(sc) 
	register struct ar_softc *sc;
{
	register struct ardevice *araddr = sc->sc_addr;
	register struct buf *bp = sc->sc_pend_chain.b_actf;
	int breakout = GLITCHCOUNT;

	/*
	 * WARNING!  bp may not reflect the buffer we are processing.
	 * If sc_histate == SSWAIT, we dispatched that buffer already
	 * (to let the user run earlier), so the buffer in bp is
	 * the next buffer on the chain of requests.  TAKE CARE to
	 * not reference bp UNTIL you have checked histate.
	 */

	/*
	 * If an exception exists, do something.
	 *
	 * Since the Archive signals some exceptions before we are
	 * ready for them (eg, FileMark before we try to read that
	 * block, unlike a 9-track tape) we must hold these in
	 * software for dealing-with next time we get a command.
	 */

again:
	if (araddr->arexc) {
		/*
		 * This interrupt is from the true level of EXC.
		 *
		 * An error has occurred.  Deal with it somehow.
		 * If we are reading status, just do it; else do our own
		 * RDST to find out the problem, and cancel the current
		 * operation.
		 */
		if (sc->sc_state == RDSTinit)
			goto doit;
Dprintf("ar*intr EXC set, old state %s=%x\n", arstatename[(int)sc->sc_state],
	sc->sc_state);
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = RDSTinit;
		/*
		 * Clear EdgeReady and enable it so the next arrdy
		 * edge will be caught (possibly inside armachine()).
		 */
		araddr->arcatch = 0;
		araddr->arcatch = 1;
		while (!armachine(sc)) {
			/* Shouldn't happen! */
			printf("ar: RDST did not complete\n");
		}

exc:

		if (sc->sc_status.FileMark) {
			/* Eof signaled now means that NEXT block is an EOF. */
			sc->sc_eoflag = 1;
			sc->sc_nxrec = sc->sc_blkno + 1;
			goto advance;
		}
	/* FIXME: GettingFlakey does NOT cause an EXC, it's just an info bit. */
		if (sc->sc_status.GettingFlakey && !sc->sc_status.HardErr) {
			uprintf(
			"ar%d: many retries, consider retiring this tape\n",
			 sc->sc_ident);
			/* FIXME, what do we do now? */
			goto opdone;		/* Keep reading, or whatever */
		}
		if (sc->sc_histate == SSWAIT) { /* FIXME, use sc_qidle? */
			/*
			 * This arexc is not for the current buffer,
			 * it's for the previous one, which we have already
			 * dispatched as "done".  Punt - ignore it for now.
			 * If this causes problems, FIXME later.
			 */
			if (!sc->sc_quietly)
				printf("ar%d: %b error at block # %d punted\n",
					sc->sc_ident,
					*(unsigned short*)&sc->sc_status,
					ar_stat_bits, sc->sc_blkno);
			goto opcont;	/* See if another buf waiting */
		}
#ifdef FIXME
/* I don't know where this line came from.  It is in error here. -- JCG */
		if (sc->sc_status.EndOfMedium || sc->sc_status.FileMark) 
#endif FIXME
		/*
		 * Errors on block tape cause it to close.
		 * This is because people doing I/O on block devices
		 * don't expect errors or know how to deal with them.
		 * This info from wnj, June '83, transcribed by gnu.
		 */
		if (bp != &sc->rbuf && bp != &sc->cbuf) {
			sc->sc_openf = -1;
		}
		/*
		 * On the Archive, all errors are hard; the controller has
		 * retried them enough times already.
		 */
		if (!sc->sc_quietly) {
			printf("ar%d: %b error at block # %d\n",
				sc->sc_ident,
				*(unsigned short*)&sc->sc_status, ar_stat_bits,
				sc->sc_blkno);
		}
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		goto opdone;
	} else {
		/*
		 * The interrupt was not from EXCEPTION.  If READY is not
		 * on, we have an Archive glitch.
		 *
		 * Problem is that it provides arrdy and then for
		 * no reason rescinds it.  It eventually comes back
		 * again on its own.  This happens after most or all
		 * commands, if ONLINE is off.  Eg, on RDST, 310 us after
		 * the 6th byte is xferred, arrdy comes on; 180 us
		 * later it drops for no reason; 90 us later it comes
		 * back on, permanently. (Tracked down with logic
		 * analyzer, triggering on (NOT arrdy) & arrdyedge).
		 *
		 * We "solve" the problem by making real damn sure that
		 * we always turn on ONLINE before we do anything.
		 * There is one place where that doesn't work -- when
		 * dropping ONLINE to rewind the tape -- and we make an
		 * explicit sc_state test below for that.
		 *
		 * Eventually it is hoped that Archive might fix this
		 * microcode bug, but I'm not holding my breath.
		 */
		if (!araddr->arrdy) {
			/*
			 * See if this is happening after a CLOSE,
			 * where we Must have ONLINE turned off.
			 * If so, just exit, another int will occur
			 * when it turns READY on again.
			 */
			if (sc->sc_state == CLOSEtwo) {
				sc->sc_oldstate = sc->sc_state;
				sc->sc_state = CLOSEthree;
				araddr->arcatch = 0;
				araddr->arcatch = 1;
Dprintf("ar*intproc badint after CLOSE\n");
				if (araddr->arrdy) goto doit;
				else return (0);
			}
			/*
			 * Otherwise, print a nasty message and go back 
			 * and look again.  Eventually break out.  Note
			 * that we're at SPL() here.
			 */
if (arprintf)
   printf("ar: interrupt without Rdy or Exc, state %s=%x, old %s=%x, bits=%b\n",
			    arstatename[(int)sc->sc_state], sc->sc_state,
			    arstatename[(int)sc->sc_oldstate], sc->sc_oldstate,
			    *(long *)(2+(char *)sc->sc_addr),
			    ARCH_LONG_CTRL_BITS);

			/* Only go around a finite number of times. */
			if (breakout--)
				goto again;
			else {
				/*
				 * OK, here's the scoop:
				 *
				 * WE know that the Archive should be ready
				 * at this point -- either it interrupted us
				 * with the edge of Ready, or it was in a 
				 * known IDLE state and we are initiating
				 * an operation.  Nevertheless, it claims
				 * it is not ready.  We have waited around
				 * awhile for it to come to grips with
				 * reality, but the flip-flop that controls
				 * the READY line to us, cannot be read back
				 * by the 8048 on the Archive controller board,
				 * so if that flop has flipped by accident,
				 * their controller will never set it right.
				 *
				 * We believe our version of the truth, and
				 * to validate our reality we give it a
				 * command.  Since probably it knows itself
				 * to be ready, it will accept the command 
				 * and we will be back in sync.  If not,
				 * our carefully-written command (RDSTinit)
				 * will time out and return us a HardErr.
				 * Then we will go thru the Exception routine
				 * and hopefully deal with this by killing
				 * the drive until re-opened and re-initted.
				 *
				 * We only do this if we are in a known good
				 * state -- if we're in the middle of something,
				 * we chicken out and give the user an error
				 * and force them to re-close and re-init.
				 *
				 * This approach was suggested by Lou Domshy
				 * at Archive, in December 1982.  Implemented
				 * by John Gilmore at Sun, June 1983.  Tech
				 * support by Kavee Phongprateep.  Wardrobe by
				 * Calvin Kline.
				 */
if (arprintf)
   printf("ar: trying to re-sync with Archive\n");
				switch (sc->sc_state) {
				case WFMinit: case RFMinit: case REWinit:
				case TENSEinit: case ERASEinit:
				case SELinit: case DESELinit: case RDSTinit:
				case CLOSEinit: case READinit: case WRinit:
				case CMDOKinit:
					sc->sc_hangstate = sc->sc_state;	
					sc->sc_oldstate = sc->sc_state;
					sc->sc_state = RDSTinit;
					/* DEPENDS ON RDST BEING ATOMIC */
					(void) armachine(sc); 
					if (sc->sc_status.AnyEx0 ||
					    sc->sc_status.InvalidCmd ||
					    sc->sc_status.NoData ||
					    sc->sc_status.GotReset) {
printf("ar: Archive hardware hang caused an error -- check your tape\n");
						goto exc;
					}
					/* DEPENDS ON RDST EXIT STATE == IDLE */
					sc->sc_oldstate = sc->sc_state;
					sc->sc_state = sc->sc_hangstate;
					/*
					 * Go on to do whatever
					 * we were trying to do before this
					 * little "problem" arose...
					 * Ready should be on but to be safe,
					 * let's check again...
					 */
					goto again;

				default:
printf("ar: Archive hardware hang -- restart entire tape job\n");
					/* Close up shop, things are hung. */
					sc->sc_openf = -1;
					bp->b_flags |= B_ERROR;
					bp->b_error = EINTR;
					goto opdone;
				}
			}
		}
doit:
		/*
		 * This interrupt is from the leading edge of arrdy.
		 *
		 * Run the low-level state machine.  Since a single op can
		 * take several interrupts, it returns us 0 if it is not
		 * finished, and 1 if it is.  If *it* is finished, then
		 * we must look to see if the completion of its low-level
		 * op means that our high-level op (buf) is done.
		 *
		 * Clear EdgeReady and enable it so the next arrdy
		 * edge will be caught (possibly inside armachine()).
		 */
		araddr->arcatch = 0;
		araddr->arcatch = 1;

		if (!armachine(sc))
			return (0);
	}

	/*
	 * An operation completed... record status
	 */

	/*
	 * Advance tape control FSM.
	 */
advance:
	switch (sc->sc_histate) {

	case SSWAIT:
		/*
		 * We have finished off our previous buffer, but are waiting
		 * for the final interrupt to come in (to tell us it's ok
		 * to do something else).  Here it is.  We should look to
		 * see if another request has come in (on sc_pend_queue)
		 * or just exit until sometime one does.
		 */
		goto opcont;

	case SIO:
		/*
		 * Read/write increments tape block number
		 * and possibly does another block if count not exhausted.
		 */
		sc->sc_blkno++;
		sc->sc_bufptr += AR_BSIZE;
		sc->sc_size -= AR_BSIZE;
		if (sc->sc_size > 0 && !sc->sc_eoflag) {
			/*
			 * If we haven't hit EOF, and need more data to
			 * satisfy the read, go back and do it again.
			 * This involves setting the state for the next
			 * interrupt so it will read/write the next block.
			 * Then return, indicating that this buffer is not
			 * yet finished.
			 */
Dprintf("ar*arintproc restarting\n");
			if (bp->b_flags & B_READ) {
				if (sc->sc_state != READidle) {
					printf("ar: OOPS READ %x\n",
						sc->sc_state);
				}
				sc->sc_oldstate = sc->sc_state;
				sc->sc_state = READburst;
				goto doit;
			} else {
				if (sc->sc_state != WRfin) {
					printf("ar: OOPS WR %x\n",
						sc->sc_state);
				}
				sc->sc_oldstate = sc->sc_state;
				sc->sc_state = WRburst;
			}
			return (0);	/* Not done this buffer yet */
		} else {
			sc->sc_histate = SSWAIT;
			goto opdone;
		}

	case SCOM:
		/*
		 * For forward/backward space record update current position.
		 */
		if (bp == &sc->cbuf) {
			switch (bp->b_command) {

			case AR_SKIPFILE:
				sc->sc_blkno++;
				break;

			case AR_CLOSE:
			case AR_REWIND:
			case AR_TENSE:
			case AR_ERASE:
				sc->sc_blkno = 0;

			}
		}
		sc->sc_histate = SSWAIT;
		goto opdone;

#ifdef SEEK
	case SSEEK:
		/* This seems to assume that we get an interrupt when
		   these ops start, then another when they end? FIXME */
		/* No, this seek came about because we were positioned
		   wrong; now we can really do the I/O op.  True? */
		sc->sc_blkno = bp->b_blkno;
		sc->sc_histate = SSWAIT;
		goto opcont;
#endif SEEK

	default:
		printf("ar: histate bad = %d\n", sc->sc_histate);
		sc->sc_histate = 0;	/* Repeat til somebody sets it */
		bp->b_flags |= B_ERROR;
		bp->b_error = EIO;
		goto opcont;
	}

opdone:
	/*
	 * Reset error count and remove
	 * from device queue.
	 */
	sc->sc_pend_chain.b_actf = bp->av_forw;
	bp->b_resid = sc->sc_size;

	/*
	 * We're done with this buffer; allow another one to be queued.
	 */
	if ((bp->b_flags & B_READ) == 0 && bp != &sc->cbuf && 
	     bp != &sc->rbuf) {
		if (armaxbufs <= 0)
			wakeup((caddr_t)&armaxbufs);
		armaxbufs++;
Dprintf("ar*arintproc buffer done, maxbufs=%d\n", armaxbufs);
	}

	biodone(bp);
	sc->sc_histate = SSWAIT;		/* Til next buf started, just wait */

	/*
	 * Begin I/O on the next buffer in the pend_chain.  This is
	 * used when the current buffer has been finished during this
	 * interrupt (fall-thru from opdone), or when the previous buffer
	 * was finished before its final interrupt (leaving us in histate
	 * SSWAIT, waiting for the final interrupt).  This is the final
	 * interrupt; we come here to see if we can start a new request.
	 */
opcont:
	return (1);		/* OK to start next request */
}

/*
 * Raw I/O routines
 */
arread(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int error;

	error = arphys(dev, uio);
	if (error)
		return (error);
	return (physio(arstrategy, &ar_softc[ARCTLR(dev)].rbuf, dev,
	    B_READ, minphys, uio));
}

arwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int error;

	error = arphys(dev, uio);
	if (error)
		return (error);
	return (physio(arstrategy, &ar_softc[ARCTLR(dev)].rbuf, dev,
	    B_WRITE, minphys, uio));
}

/*
 * Check that a raw device exists.
 * If it does, set up sc_blkno and sc_nxrec
 * so that the tape will appear positioned correctly.
 */
arphys(dev, uio)
	dev_t dev;
	struct uio *uio;
{
	register int unit = ARCTLR(dev);
	register daddr_t a;
	register struct ar_softc *sc;
	register struct mb_device *md;

	if (unit >= NAR ||(md=ardinfo[unit]) == 0 || md->md_alive == 0)
		return (ENXIO);
	sc = &ar_softc[unit];
	a = uio->uio_offset / AR_BSIZE;
	sc->sc_blkno = a;
	sc->sc_nxrec = INF;
	return (0);
}

/*
 * Process IOCTL's used on raw tape.
 */
/*ARGSUSED*/
arioctl(dev, cmd, data, flag)
	caddr_t data;
	dev_t dev;
{
	int unit = ARCTLR(dev);
	register struct ar_softc *sc = &ar_softc[unit];
	register callcount;
	int error;
	int fcount, ioctlcmd;
	struct mtop *mtop;
	struct mtget *mtget;
	/* we depend on the values and order of the MT codes here */
	static ops[] = {
		AR_WEOF,	/* Write EOF (tapemark) */
		AR_SKIPFILE,	/* Forward space file */
		0,		/* Backspace file -- can't do it */
		0,		/* Forward space record -- too dumb FIXME */
		0,		/* Backspace record -- can't do it */
		AR_REWIND,	/* Rewind tape */
		AR_REWIND,	/* Unload - we just rewind */
		AR_STATUS,	/* Read status back */
		AR_TENSE,	/* Retension tape */
		AR_ERASE,	/* Erase entire tape */
	};

#ifdef PRF2
Dprintf("ar*ioctl(%d, %d, %x, %d)\n", dev, cmd, data, flag);
#endif
	switch (cmd) {
	case MTIOCTOP:	/* tape operation */
		mtop = (struct mtop *)data;
Dprintf("ar*ioctl MTIOCTOP %d count %d\n", mtop->mt_op, mtop->mt_count);
		switch (mtop->mt_op) {

		case MTWEOF:
			if (!sc->sc_writeable) goto NoWrite;
			/* FALL THRU */
		case MTFSF:
			callcount = mtop->mt_count;
			fcount = 1;
			break;

		case MTERASE:
			if (!sc->sc_writeable) goto NoWrite;
			/* FALL THRU */
		case MTRETEN:
			error = arcommand(dev, AR_REWIND, 1);
			if (error)
				return (error);
			/* FALL THRU */
		case MTREW:
		case MTOFFL:
		case MTNOP:
			callcount = 1;
			fcount = mtop->mt_count;
			break;

		default:
		NoWrite:
			return (ENXIO);
		}
		if (callcount <= 0 || fcount <= 0)
			return (ENXIO);
		ioctlcmd = ops[mtop->mt_op];
		/*
		 * If eoflag then we already hit tape mark but
		 * didn't send notification back to the user (by sending
		 * zero length record).  Allow writes after this
		 * command by assigning to blkno.
		 */
		if (ioctlcmd == AR_SKIPFILE && sc->sc_eoflag) {
			sc->sc_eoflag = 0;
			sc->sc_blkno = INF;
			callcount--;
		}
		while (--callcount >= 0) {
			error = arcommand(dev, ioctlcmd, fcount);
			if (error)
				return (error);
		}
		if (ioctlcmd != AR_STATUS)
			sc->sc_eoflag = 0;
		return (0);

	case MTIOCGET:
		mtget = (struct mtget *)data;
		mtget->mt_type = MT_ISAR;
		error = arcommand(dev, AR_STATUS, 1);
		if (error)
			return (error);
		*(  (char *)&mtget->mt_dsreg) = *(3+(char *)sc->sc_addr);
		*(1+(char *)&mtget->mt_dsreg) = *(5+(char *)sc->sc_addr);
		mtget->mt_erreg = *(short *)&sc->sc_status;
		/* FIXME.  Move this from resid to a real place. */
		mtget->mt_resid = sc->sc_stops;	/* # tape stops since asked */
		sc->sc_stops = 0;		/* Clear it for next time */
		sc->sc_softerrs = 0;
		/* FIXME.  Add read errors info */
		return (0);

	default:
		return (ENXIO);
	}
}

/*
 * State machine for archive tape drive controller.
 * This actually accomplishes things w.r.t. the tape drive.
 * Returns 0 if operation still in progress, 1 if finished.
 * Note that we may take further interrupts after claiming that an
 * operation is "finished".  For example, we say a write is done when
 * we have transferred the last byte of the block; but there will be
 * an interrupt 5.5ms later to tell us it's ok to send the next block.
 * Eventually, we will rewind the tape asynchronously after the file is
 * closed, letting the user go free while it spins.  FIXME: THIS CANNOT
 * BE DONE until we clean up the high level code so it doesn't clobber
 * our variables as it is setting up to call arstart_cmd().
 *
 * Must be entered at SPL().
 */

armachine(sc)
	register struct ar_softc *sc;
{
	register struct ardevice *araddr = sc->sc_addr;
	register int count;
	register char *byteptr;

Dprintf("ar*machine(%x, %x) state %s=%x\n", sc, araddr, 
	arstatename[(int)sc->sc_state], sc->sc_state);

	switch (sc->sc_state) {

	case CMDOKinit:
		/*
		 * If we got here the command state is ok,
		 * i.e., not in the middle of a read or write.
		 */
		sc->sc_cmdok = 1;
		goto IdleState;

	case CLOSEinit:
		/*
		 * FIXME, this is time dependent and not documented in
		 * the Archive manual.  However, it looks like in the
		 * schematics (and in actual performance), if you drop
		 * ONLINE, hardware gates will drop READY.  This appears
		 * to depend on whether you are "at position" or not;
		 * If not, nothing happens; if so, READY drops and we move
		 * the tape.
		 */
		araddr->aronline = 0;		/* Drop online; we're done. */
		if (araddr->arrdy)
			goto IdleState;		/* No interrupt will occur. */
		else
			goto next;	/* Rewinding; wait for interrupt. */

	case CLOSEtwo:
	case CLOSEthree:
		/*
		 * Note that there is special code in arintproc() which
		 * deals with READY line glitches for state CLOSEtwo.
		 */
		araddr->aronline = 1;		/* Reassert ONLINE to avoid
						   glitch in READY line */
		goto IdleState;

	case WFMinit:
		sc->sc_status.BOT = 0;
		araddr->ardata = ARCMD_WREOF;
		araddr->arreq = 1;
		goto CmdState;

	case RFMinit:
		araddr->ardata = ARCMD_RDEOF;
		araddr->arreq = 1;
		goto CmdState;

	case REWinit:
		araddr->ardata = ARCMD_REWIND;
		araddr->arreq = 1;
		goto CmdState;

	case TENSEinit:
		araddr->ardata = ARCMD_TENSION;
		araddr->arreq = 1;
		goto CmdState;

	case ERASEinit:
		araddr->ardata = ARCMD_ERASE;
		araddr->arreq = 1;
		goto CmdState;

	case SELinit:
		araddr->ardata = ARCMD_LED | (1 << sc->sc_drive);
		araddr->arreq = 1;
		goto CmdState;

	case DESELinit:
		araddr->ardata = 1 << sc->sc_drive;
		araddr->arreq = 1;
		goto CmdState;

	RDSTagain:
		printf("ar: RDST gave Exception, retrying\n");
		/* Fall thru... */

	case RDSTinit:
		byteptr = (char *) &sc->sc_status;

		count = BREAKOUTCOUNT;
		if (araddr->arrdy) {
			araddr->ardata = ARCMD_RDSTAT;
			araddr->arreq = 1;
		} else {
			araddr->ardata = ARCMD_RDSTAT;
			araddr->arreq = 1;
			while (araddr->arexc)
				if (!count--) goto RDSTfail;
		}

		/*
		 * Now wait for arrdy indicating command accepted.
		 * Check for Exception too;
		 * (It's not legal to do RDST all the time(!).)
		 */
		while (!araddr->arrdy) {
			if (araddr->arexc) goto RDSTagain;
			if (!count--) {
RDSTfail:
				printf("ar: RDST failure\n");
				sc->sc_openf = -1;
				sc->sc_status.HardErr = 1;
				sc->sc_status.AnyEx0 = 1;
				goto DisAble;
			}
		}

		/* Negate arreq, wait for arrdy to drop. */
		araddr->arcatch = 0;	/* Clear arrdyedge */
		araddr->arcatch = 1;	/* Catch edge */
		araddr->arreq = 0;

		/* Now xfer a byte or six. */
		do {
			/* Wait for edge of arrdy */
			while (!araddr->arrdyedge) {
				if (araddr->arexc)
					goto RDSTagain;
				if (!count--) goto RDSTfail;
			}
			*byteptr++ = araddr->ardata;
			araddr->arcatch = 0;	/* Clear edge indicator */
			araddr->arcatch = 1;	/* Catch next one */
			araddr->arreq = 1;	/* Tell controller we have it */
			/* Ready will fall within 250ns of our arreq, but
			   we're supposed to keep it high for 20us */
			DELAY(30);	/* at least 20 usec */
			araddr->arreq = 0;
		} while (byteptr <
			(char *)(&sc->sc_status) + sizeof (sc->sc_status));
		/*
		 * On exit from this loop, arcatch has been negated and
		 * asserted, so it will correctly reflect the leading edge
		 * of arrdy for the next command.
		 */
		sc->sc_stops += sc->sc_status.TapeStops;  /* Accum total */
		sc->sc_softerrs += sc->sc_status.SoftErrs;

/* For debugging, dump status bytes after a command AR_STATUS */
Dprintf("ar*RDST %b %d %d\n", *(unsigned short*)&sc->sc_status,
	ar_stat_bits, sc->sc_status.SoftErrs, sc->sc_status.TapeStops);

		/*
		 * Wait for Ready to appear again, so we can exit in IDLE
		 * state and be atomic (not require any interrupts to
		 * complete).  Several places in higher-level code depend
		 * on this.  We leave the edge of Ready caught in EdgeReady,
		 * but IdleState will disable interrupts on it.
		 */
		while (!araddr->arrdy) {
			if (araddr->arexc)	/* Safety test */
				goto RDSTagain;
			if (!count--) goto RDSTfail;	/* Safety #2 */
		}
		goto IdleState;

	case READcmd:
	case WRcmd:
		araddr->arreq = 0;
		goto next;

	case READinit:
		araddr->ardata = ARCMD_RDDATA;
		araddr->arreq = 1;
		goto next;

	case READburst:
		/* Read a block of data from the tape drive. */
Dprintf("ar*READ addr %x count %x\n", sc->sc_bufptr, sc->sc_size);
		sc->sc_status.BOT = 0;
		/* FIXME, this could hang kernel, but at least it printf's. */
		while (!araddr->arrdy) {
			if (araddr->arexc)
				return (0);    /* Not yet done */
			printf("ar: Read no READY\n");
		}
		araddr->arburst = 1;			/* Begin block xfer */
		count = AR_BSIZE;
		byteptr = sc->sc_bufptr;
		while (count--)
			*byteptr++ = araddr->ardata; /* read data */
		araddr->arburst = 0;
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = READfin;		/* Like FINstate sorta */
		break;

	case WRinit:
		araddr->ardata = ARCMD_WRDATA;
		araddr->arreq = 1;
		goto next;

	case WRburst:
		/* Write a block of data to the tape drive. */
Dprintf("ar*WRITE addr %x count %x\n", sc->sc_bufptr, sc->sc_size);
		/* FIXME, this could hang kernel, but at least it printf's. */
		while (!araddr->arrdy) {
			if (araddr->arexc)
				return (0);		/* Not done yet */
			printf("ar: Write no READY\n");
		}
		araddr->arburst = 1;			/* Begin block xfer */
		count = AR_BSIZE;
		byteptr = sc->sc_bufptr;
		while (count--)
			araddr->ardata = *byteptr++;
		araddr->arburst = 0;
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = WRfin;		/* Like FINstate sorta */
Dprintf("ar*machine exiting done in state %s=%x\n", 
	arstatename[(int)sc->sc_state], sc->sc_state);
		/*
		 * This code is a copy of the code at the end of this
		 * switch statement, except that it returns 1 (operation
		 * completed) instead of 0 (more needs doing)
		 */
		araddr->arrdyie = 1;
		araddr->arexcie = 1;
		return (1);

	case CMDstate:
		/* All commands that stop interacting once you say "do it" */
		araddr->arreq = 0;	/* Done with command */
		goto FinState;		/* Final interaction for this cmd */

	IdleState:		/* Drive is idle; set IDLEstate and disable */
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = IDLEstate;
		goto DisAble;

	case WRfin:		/* Entry after writing a block */
	case READfin:		/* Entry after reading a block */
	case FINstate:		/* Entry after any other command */
		/*
		 * Go to next sequential state - WRidle, READidle, IDLEstate.
		 * Disable interrupts, and return.  arstart_cmd() will later
		 * put us into READ/WRburst or some commandinit state.
		 */
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = (enum ARstates)(1 +(int)sc->sc_state);

DisAble:
Dprintf("ar*machine idling\n");
		araddr->arrdyie = 0;		/* Negate arrdy interrupt */
		return (1);			/* Tell caller op is done */

	case WRidle:		/* Writing blocks, but none to write now */
	case READidle:		/* Reading blocks, but don't need one now */
	case IDLEstate:		/* Issuing commands, but don't have one now */
		/* This can only happen if software triggers us. */
		printf("ar: triggerred at idle %x\n", sc->sc_state);
		goto DisAble;	/* Turn off interrupt enable again */

	default:
		printf("ar: invalid state 0x%x\n", sc->sc_state);
		sc->sc_openf = -1;	/* Force closure & re-initialization */
		goto DisAble;		/* Is this reasonable? */

	next:
		/* Go to next sequential state */
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = (enum ARstates)(1 +(int)sc->sc_state);
		break;

	FinState:
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = FINstate;
		break;

	CmdState:
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = CMDstate;
		break;

	}

Dprintf("ar*machine exiting in state %s=%x\n", 
	arstatename[(int)sc->sc_state], sc->sc_state);

	/* Go to next state on the next leading edge of arrdy. */
	araddr->arrdyie = 1;	/* Interrupt on arrdy leading edge */
	araddr->arexcie = 1;	/* Interrupt on arexc too */
/* FIXME.  Figure out where to set and unset, leave alone otherwise. */

	return (0);
}


/*
 * Sort of like tsleep for the Archive driver.
 * XXX - this can be replaced by new PCATCH option to sleep.
 */
arsleep(cp, pri)
	caddr_t cp;
	int pri;
{
	label_t q;
	int rc = 0;

	q = u.u_qsave;
	if (setjmp(&u.u_qsave))
		rc = 1;
	else
		(void) sleep(cp, pri);
	u.u_qsave = q;
	return (rc);
}

#endif
