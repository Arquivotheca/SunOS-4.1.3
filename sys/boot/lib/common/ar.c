#ifndef lint
static	char sccsid[] = "@(#)ar.c	1.1 92/07/30	Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Standalone Driver for Archive Intelligent Streaming Tape
 */
#include <stand/saio.h>
#include <stand/param.h>
#include <sundev/arreg.h>
#include <stand/ardef.h>

#define	min(a,b)	((a) < (b) ? (a) : (b))

int ardebug = 0;
#define Dprintf  if (ardebug) printf

/* Trace all writes to control reg */
#define DebugTrace Dprintf(ar_ctrl_hdr, *(long*)(2+(char*)araddr))

/* Define assorted debugging strings to save repeating them N times */
#ifdef DEBUG
char ar_bits[] = ARCH_LONG_CTRL_BITS;
char ar_ctrl_hdr[] = "ar* Bits: %x\n";
char ar_set_hdr[] = "ar* CMD: %x\n";
#endif DEBUG
char ar_stat_bits[] = ARCH_BITS;

/*
 * Standard I/O addresses.
 */
#define NARADDR	2
u_long araddrs[] = { 0x200, 0x208};

/*
 * What resources we need to run
 */
struct devinfo arinfo = {
	sizeof (struct ardevice), 	/* I/O regs */
	0,				/* no DMA */
	sizeof (struct ar_softc),	/* work area */
	NARADDR,
	araddrs,
	MAP_MBIO,
	DEV_BSIZE,			/* transfer size */
};

/*
 * What facilities we export to the world
 */
int	aropen(), arclose(), arstrategy();
extern int	xxboot(), xxprobe();

struct boottab ardriver = {
	"ar",	xxprobe, xxboot, aropen, arclose, arstrategy,
	"ar: Multibus Archive tape controller", &arinfo,
}; 

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


#define SPININIT 1000000
/*
 * Initialize a controller
 */
aropen(sip)
	register struct saioreq *sip;
{
	register struct ar_softc *sc;
	register int count;
	register struct ardevice *araddr;
	int skip;
	
	sc = (struct ar_softc *)sip->si_devdata;
	araddr = sc->sc_addr = (struct ardevice *)sip->si_devaddr;

	sc->sc_lastiow = 0;
	sc->sc_state = IDLEstate;
/*	sc->sc_status = 0;  Doesn't work, so leave it alone. */
	sc->sc_size = 0;
	sc->sc_bufptr = (char *) 0x1ff001;	/* very funny buf addr */
/*	sc->sc_attached had better already be 1. */
	sc->sc_initted = 0;			/* until later */
	sc->sc_opened = 0;			/* until later */
	sc->sc_drive = 0;
	sc->sc_histate = 0;
/*	sc->sc_addr had better already be initialized. */
	sc->sc_qidle = 1;
	sc->sc_eoflag = 0;
	sc->sc_cmdok = 0;
	sc->sc_selecteddev = -1;
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
#ifdef PRF
Dprintf("ar*init about to arreset\n");
#endif PRF
	araddr->arreset = 1;
#ifdef PRF
Dprintf("ar*init asserted arreset\n");
#endif PRF
	DELAY(25);	/* at least 13 usec */
	araddr->arreset = 0;
#ifdef PRF
Dprintf("ar*init Reset complete\n");
#endif PRF

	count = SPININIT;
	while (!araddr->arexc && count)
		count--;
	if (count == 0) {
Dprintf("ar: Timeout waiting for Exception after reset\n");
		return (-1);
	}

	/* Now read back status from the reset. */
	sc->sc_initted = 1;	/* Must do first so interrupt OK */
	sc->sc_opened = 1;	/* Must do first so interrupt OK */
	araddr->aronline = 1;	/* Must do first so RDST microcode doesn't
				   play games with arrdy line.  See comments
				   in open(). */
	if (arcmd(sc, AR_STATUS)) {
Dprintf("ar*init Error from command STATUS\n");
		araddr->aronline = 0;
		sc->sc_initted = 0;	/* Try again on next open */
		sc->sc_opened = 0;	/* Try again on next open */
		return (-1);
	}

	/*
	 * FIXME, this is a kludge.  open() won't select the drive unless
	 * the in-core status claims we are at BOT, since the tape drive
	 * will reject the command if indeed a tape was in use and is not
	 * at BOT.  However, tapes at BOT after a Reset do not necessarily
	 * indicate BOT in their status.  We should probably do a rewind
	 * here instead, if the tape exists.
	 */
	sc->sc_status.BOT = 1;	/* Pretend we're at BOT for open() */

	/*
	 * There is a problem in the Archive in that after each command,
	 * it goes thru a cleanup routine.  If aronline is not asserted,
	 * this cleanup routine drops arrdy while it rewinds the tape
	 * to BOT.  It deasserts arrdy for 90 us even if the tape is
	 * already at BOT.  This causes us problems because we get a arrdy
	 * interrupt and then discover that arrdy is gone.
	 *
	 * The problem has been circumvented at the low level by checking
	 * for arrdy in the interrupt routine, and looping until it (or
	 * arexc) comes on.  We attempt to fix the problem here, to avoid
	 * looping at SPL(), by having aronline always on when we are doing
	 * anything.
	 *
	 * The problem seems especially a problem for us on Select and Read
	 * Status commands. (That's because those are the only commands we
	 * do with aronline deasserted.)
	 *
	 * This info was obtained from Lou Domshy, Mgr. of Product Mgmt and
	 * Applications Engineering(?) at the Archive factory, 714-641-0279,
	 * on 1 December 1982, by John Gilmore of Sun Microsystems.
	 */
	araddr->aronline = 1;		/* Let ctrlr know we are doing a series */

	/*
	 * First select the drive we're interested in.
	 *
	 * Since the select command doesn't work when we aren't at BOT,
	 * we just have to hope the same drive is still selected as last
	 * time.  FIXME.  We should record this info in softc and keep it
	 * up to date.  FIXME: also, I'm not happy about using status.BOT
	 * here, even tho it should always be up to date. -- JCGnu 22Nov82
	 */
	sc->sc_cmdok = 0;
	(void) arcmd(sc, AR_CMDOK);	/* See if OK to issue cmds */
	/*
	 * Now get its status and check on a few things.
	 */
	if (sc->sc_cmdok) {
		if (arcmd(sc, AR_STATUS)) {	/* interrupted */
Dprintf("ar*open command STATUS error\n");
			goto err;
		}
		if (sc->sc_status.NoDrive) {
			printf("ar: no drive\n");
			goto err;
		}	
		if (sc->sc_status.NoCart) {
			printf("ar: no cartridge in drive\n");
			goto err;
		}
	}

	if ((sip->si_flgs&F_WRITE) && sc->sc_status.WriteProt) {
		printf("ar: cartridge is write protected\n");
		goto err;
	}
	sc->sc_lastiow = 0;

	skip = sip->si_boff;
	while (skip--) {
		arcmd(sc, AR_SKIPFILE);
		arcmd(sc, AR_STATUS);
	}
	sc->sc_eoflag = 0;
Dprintf("ar*open exiting\n");
	return (0);

err:
	arcmd(sc, AR_DESELECT);
	araddr->aronline = 0;
	sc->sc_opened = 0;
	return (-1);
}

/*
 * Close tape device.
 *
 * If tape was open for writing or last operation was
 * a write, then write two EOF's and backspace over the last one.
 * Unless this is a non-rewinding special file, rewind the tape.
 * Make the tape available to others.
 */
arclose(sip)
	struct saioreq *sip;
{
	register struct ar_softc *sc = (struct ar_softc *)sip->si_devdata;
	register struct ardevice *araddr = sc->sc_addr;

	/*
	 * Write file mark and rewind, by dropping aronline.
	 * FIXME.  These 3 commands should be moved into AR_CLOSE
	 * in order that the user program can continue while the
	 * tape is rewinding.
	 */
	arcmd(sc, AR_CLOSE);	/* Shut down things */
	araddr->aronline = 1;		/* After rewind, set aronline */
	/* See comments in open() about aronline and read status cmds */
	/* FIXME, this might screw low level code if it affects arrdy */
	arcmd(sc, AR_STATUS);	/* Read block counts */
	arcmd(sc, AR_DESELECT);	/* Turn LED off */
	sc->sc_selecteddev = -1;
	sc->sc_eoflag = 0;	/* Not at eof after rewind */
	sc->sc_opened = 0;	/* Available to be opened again */
Dprintf("ar*close exiting\n");
}

arstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	register struct ar_softc *sc = (struct ar_softc *)sip->si_devdata;
	int func = (rw == WRITE) ? AR_WRITE : AR_READ;

	if (sc->sc_eoflag) {
		sc->sc_eoflag = 0;
		return (0);
	}
	sc->sc_size = sip->si_cc;
	sc->sc_bufptr = sip->si_ma;
	if (arcmd(sc, func))
		return (-1);
	return (sip->si_cc);
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
 *	0	if the operation completed normally
 *	1	if the operation completed abnormally
 *
 */
arcmd(sc, cmd)
	register struct ar_softc *sc;
{
	struct ardevice *araddr	= sc->sc_addr;

	if (!sc->sc_opened)
		return (-1);

	sc->sc_state = ar_cmds[cmd];

	for (;;) {
		if (araddr->arexc) {
			/*
			 * This interrupt is from the true level of arexc.
			 *
			 * An error has occurred.  Deal with it somehow.
			 * If we are reading status, just do it; else do our own
			 * RDST to find out the problem, and cancel the current
			 * operation.
			 */
			if (sc->sc_state == RDSTinit)
				goto doit;
	Dprintf("ar*intr arexc set, old state %x\n", sc->sc_state);
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
				printf("ar*intr RDST did not return 1\n");
			}
			if (!sc->sc_status.FileMark) {
				printf("ar: error %x\n",
					*(u_short *)&sc->sc_status);
				return (1);
			}
			/* Eof signaled now means that NEXT block is an EOF. */
			sc->sc_eoflag = 1;
			return (0);
		}
		if (araddr->arrdy) {

	doit:
			araddr->arcatch = 0;
			araddr->arcatch = 1;
			if (armachine(sc))
				return (0);
		}
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
 */
armachine(sc)
	register struct ar_softc *sc;
{
	register struct ardevice *araddr = sc->sc_addr;
	register int count, i, x;
	register char *byteptr;

Dprintf("ar*machine(%x, %x) state %x\n", sc, araddr, sc->sc_state);

	switch (sc->sc_state) {

	case CMDOKinit:
		/*
		 * If we got here the command state is ok,
		 * i.e., not in the middle of a read or write.
		 */
		sc->sc_state = IDLEstate;
		sc->sc_cmdok = 1;
		return (1);

	case CLOSEinit:
		/* FIXME, this is time dependent and not documented in
		   the Archive manual */
		araddr->aronline = 0;		/* Drop online; we're done. */
		if (araddr->arrdy)
			goto IdleState;	/* No interrupt will occur. */
		else
			goto FinState;	/* Rewinding; wait for interrupt. */

#ifdef FIXME
	case CLOSEend:
		/* This is entered from READidle or WRidle. */
		araddr->aronline = 0;		/* Drop it, causing rewind. */
		goto FinState;		/* Interrupt will signal end of rew. */
#endif

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

		/* We could have either arrdy or arexc; remember which */
		count = 0;
		if (araddr->arrdy)
			count = 1;
		araddr->ardata = ARCMD_RDSTAT;
		araddr->arreq = 1;

		/*
		 * Now wait for arrdy indicating command accepted.
		 * Check for Exception, if we started with arrdy.  
		 * (It's not legal to do RDST all the time(!).)
		 */
		while (!araddr->arrdy)
			if (count && araddr->arexc)
				goto RDSTagain;

		/* Negate arreq, wait for arrdy to drop. */
		araddr->arcatch = 0;	/* Clear arrdyedge */
		araddr->arcatch = 1;	/* Catch edge */
		araddr->arreq = 0;

		/* Now xfer a byte or six. */
		do {
			/* Wait for edge of arrdy */
			while (!araddr->arrdyedge)
				if (araddr->arexc)
					goto RDSTagain;
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
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = FINstate;	/* Awaiting final interrupt */

/* Dump status bytes after a command AR_STATUS */

Dprintf("ar*RDST %x %d %d\n", *(unsigned short*)&sc->sc_status,
	sc->sc_status.SoftErrs, sc->sc_status.TapeStops);

/* This code is obsolete since sc_qidle, and should be replaceable
   by a simple branch to RdWrFin.  However, that doesn't work, so
   try this.  JCGnu, 23Nov82 */
		while (!araddr->arrdy)
			if (araddr->arexc)
				goto RDSTagain;
/* We leave the edge of Ready caught in EdgeReady, but IdleState will
   disable interrupts on it. */
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
		while (!araddr->arrdy) {
			if (araddr->arexc)
				return (0);    /* Not yet done */
			Dprintf("ar*Read no READY\n");
		}
		araddr->arburst = 1;			/* Begin block xfer */
		count = min(sc->sc_size, AR_BSIZE);
		byteptr = sc->sc_bufptr;
		for (i=0; i<count; i++)
			*byteptr++ = araddr->ardata; /* read data */
		for (; i<AR_BSIZE; i++)
			x = araddr->ardata;	/* read junk */
#ifdef lint
		x = x;			/* use it */
#endif
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
		while (!araddr->arrdy) {
			if (araddr->arexc)
				return (0);		/* Not done yet */
			Dprintf("ar*Write no READY\n");
		}
		araddr->arburst = 1;			/* Begin block xfer */
		count = AR_BSIZE;
		byteptr = sc->sc_bufptr;
		while (count--)
			araddr->ardata = *byteptr++;
		araddr->arburst = 0;
		sc->sc_oldstate = sc->sc_state;
		sc->sc_state = WRfin;		/* Like FINstate sorta */
Dprintf("ar*machine exiting done in state %x\n", sc->sc_state);
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
		printf("ar: invalid state %d\n", sc->sc_state);
		goto FinState;		/* Is this reasonable? */

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

Dprintf("ar*machine exiting in state %x\n", sc->sc_state);

	/* Go to next state on the next leading edge of arrdy. */
	araddr->arrdyie = 1;	/* Interrupt on arrdy leading edge */
	araddr->arexcie = 1;	/* Interrupt on arexc too */
/* FIXME.  Figure out where to set and unset, leave alone otherwise. */

	return (0);
}
