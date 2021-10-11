#include "sc.h"
#if	NSC > 0
#ifndef lint
static	char sccsid[] = "@(#)sc.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (C) 1989, Sun Microsystems, Inc.
 */

/*
 *
 * TO DO:	1. reimplment PARITY support
 *		2. handle linked commands
 *
 */

#include <scsi/scsi.h>
#include <scsi/adapters/screg.h>
#include <sys/mman.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <vm/seg.h>

/*
 * Definitions
 */

/*#define	SC_PARITY	/* Compile in (cough) Parity support */
/*#define	SC_LINKED	/* Compile in linked command support */
/*#define	SCDEBUG		/* Compile in debug code */

#ifdef	SC_PARITY
---- BLETCH ----- PARITY NOT SUPPORTED YET
#endif


#ifdef	SCDEBUG
#define	DEBUGGING	(scdebug || (scsi_options & SCSI_DEBUG_HA))
#define	DPRINTF(str)		if (DEBUGGING) sc_printf(sc, str)
#define	DPRINTF1(str, x)	if (DEBUGGING) sc_printf(sc, str, x)
#define	DPRINTF2(str, x, y)	if (DEBUGGING) sc_printf(sc, str, x, y)
#define	DPRINTF3(str, x, y, z)	if (DEBUGGING) sc_printf(sc, str, x, y, z)
#else
#define	DPRINTF
#define	DPRINTF1
#define	DPRINTF2
#define	DPRINTF3
#endif

#define	TRUE		1
#define	FALSE		0
#define	UNDEFINED	-1

#define	Tgt(sp)		((sp)->cmd_pkt.pkt_address.a_target)
#define	Lun(sp)		((sp)->cmd_pkt.pkt_address.a_lun)
#define	Nextcmd(sp)	((struct scsi_cmd *)((sp)->cmd_pkt.pkt_ha_private))

#define	CNAME	scdriver.mdr_cname
#define	CNUM	sc-sc_softc

#define	INTPENDING(reg) ((reg)->icr & (ICR_INTERRUPT_REQUEST | ICR_BUS_ERROR))


/*
 * SCSI Architecture Interface functions
 */

static int sc_start(), sc_abort(), sc_reset(), sc_getcap(), sc_setcap();

/*
 * Local && h/w related functions
 */


static void screset(), sc_watch(), sc_ustart(), sc_finish(), sc_abort_cmd();
static void sc_printf(), sc_flush();

/*
 * Local static data
 */

static struct scsitwo sc_softc[NSC];
static char *scbits =
	"\20\20Per\17Berr\16Odd\15IRQ\14REQ\13MSG\12CD\11IO\03Wrd\02Dma\01Ena";
static long sc_kment;
static int sc_mapins;

#ifdef	SCDEBUG
static int scdebug;
#endif	SCDEBUG

/*
 * External references
 */

extern struct seg kseg;

/*
 * Config dependencies
 */

static int scprobe(), scslave(), scattach(), scpoll();
int scintr();
struct	mb_driver scdriver = {
	scprobe, scslave, scattach, 0, 0, scpoll,
	sizeof (struct screg), "scsibus", 0, "sc", 0, MDR_BIODMA,
};

static int
scprobe(reg, ctlr)
caddr_t reg;
int ctlr;
{
	static char scwstart = 0;
	register struct scsitwo *sc;
	register struct screg *btmp = (struct screg *) reg;

	/* probe for different scsi host adaptor interfaces */
	if (peek((short *)&btmp->dma_count) == -1) {
		return (0);
	}
	/* validate ctlr by write/read/cmp with a data pattern */
	btmp->dma_count = 0x6789;
	if (btmp->dma_count != 0x6789) {
		return (0);
	}

	if (ctlr >= NSC) {
		printf("%s%d: illegal controller number", CNAME, ctlr);
		return (0);
	}

	sc_kment = rmalloc(kernelmap, mmu_btopr(MMU_PAGESIZE));
	if (sc_kment == 0)
		return (0);
	sc = &sc_softc[ctlr];
	sc->sc_reg = btmp;
	sc->sc_tran.tran_start 		= sc_start;
	sc->sc_tran.tran_abort		= sc_abort;
	sc->sc_tran.tran_reset		= sc_reset;
	sc->sc_tran.tran_getcap		= sc_getcap;
	sc->sc_tran.tran_setcap		= sc_setcap;
	sc->sc_tran.tran_pktalloc 	= scsi_std_pktalloc;
	sc->sc_tran.tran_dmaget		= scsi_std_dmaget;
	sc->sc_tran.tran_pktfree	= scsi_std_pktfree;
	sc->sc_tran.tran_dmafree	= scsi_std_dmafree;
	screset(sc, 0);
	if (!scwstart) {
		scwstart++;
		timeout (sc_watch, (caddr_t) 0, hz);
	}
	return (sizeof (struct screg));
}

/*ARGSUSED1*/
static int
scslave(md, reg)
struct mb_device *md;
caddr_t reg;
{
	struct scsitwo *sc = &sc_softc[md->md_unit];

	md->md_slave = HOST_ID << 3;
#ifndef	SC_LINKED
	/*
	 * Disable linked commands for the moment...
	 */
	if (scsi_options & SCSI_OPTIONS_LINK) {
		scsi_options ^= SCSI_OPTIONS_LINK;
		sc_printf(sc, "disabling linked command support\n");
	}
#endif
	return (1);
}

static int
scattach(md)
struct mb_device *md;
{
	int i;
	static int sc_spl = 0;
	struct scsitwo *sc = &sc_softc[md->md_unit];

	/*
	 * Calculate max spl value so far seen
	 */
	sc_spl = MAX(sc_spl, pritospl(md->md_intpri));

	/*
	 * Make sure that everyone, including us, is locking at
	 * the same priority.
	 */

	for (i = 0; i <= md->md_unit; i++) {
		sc_softc[i].sc_tran.tran_spl = sc_spl;
	}

	/*
	 * Initialize interrupt register
	 */
	if (md->md_mc->mc_intr) {
		/* set up for vectored interrupts */
		sc->sc_reg->intvec = md->md_mc->mc_intr->v_vec;
		*(md->md_mc->mc_intr->v_vptr) = (int) sc;
	} else {
		/* use auto-vectoring */
		sc->sc_reg->intvec = AUTOBASE + md->md_mc->mc_intpri;
	}
	/*
	 * Now make ourselves known to the rest of the SCSI world
	 */
	scsi_config(&sc->sc_tran, md);
}


/*
 *
 *	Begin of external interface routines
 *
 */

static int
sc_start(sp)
register struct scsi_cmd *sp;
{
	auto int s;
	register struct scsitwo *sc;
	register struct scsi_cmd *xp;

	sc = (struct scsitwo *) sp->cmd_pkt.pkt_address.a_cookie;
	s = splr(sc->sc_tran.tran_spl);
	if (xp = sc->sc_que) {
		for (;;) {
			if (Tgt(xp) == Tgt(sp) && Lun(xp) == Lun(sp)) {
				/* Queueing error */
				(void) splx(s);
				return (FALSE);
			}
			if (Nextcmd(xp) == 0)
				break;
			else
				xp = Nextcmd(xp);
		}
		Nextcmd(xp) = sp;
	} else {
		sc->sc_que = sp;
	}
	Nextcmd(sp) = 0;

	/*
	 * reinitialize some fields that need it...
	 */

	sp->cmd_pkt.pkt_resid = 0;
	sp->cmd_pkt.pkt_reason = sp->cmd_pkt.pkt_state =
		sp->cmd_pkt.pkt_statistics = 0;
	sp->cmd_cdbp = (caddr_t) sp->cmd_pkt.pkt_cdbp;
	sp->cmd_scbp = sp->cmd_pkt.pkt_scbp; *sp->cmd_scbp = 0;
	if (sp->cmd_timeout = sp->cmd_pkt.pkt_time)
		sp->cmd_flags |= CFLAG_WATCH;
	else
		sp->cmd_flags &= ~CFLAG_WATCH;
	sp->cmd_data = sp->cmd_saved_data = sp->cmd_mapping;
	sp->cmd_cursubseg = &sp->cmd_subseg;
	sp->cmd_subseg.d_base = sp->cmd_mapping;
	sp->cmd_subseg.d_count = 0;
	sp->cmd_subseg.d_next = (struct dataseg *) 0;
	sp->cmd_flags &= ~(CFLAG_NEEDSEG|CFLAG_CMDDISC);



	/*
	 * Okay- if this command is a polling command,
	 * and we're not the head of the queue, we have to
	 * wait for the commands ahead of us to finish.
	 */
	if (sp->cmd_pkt.pkt_flags & FLAG_NOINTR) {
		int id;
		while (sc->sc_que != sp) {
			DPRINTF ("polled command waiting\n");
			sc_dopoll(sc, sc->sc_cmdid);
		}
		id = sc->sc_cmdid;
		DPRINTF1 ("starting polling cmd #%d\n", id);
		sc_ustart(sc);
		DPRINTF ("waiting polled cmd completion\n");
		sc_dopoll(sc, id);
		DPRINTF1 ("polled cmd #%d complete\n", id);
		if (sc->sc_que)
			sc_ustart(sc);
	} else if (sc->sc_que == sp) {
		sc_ustart(sc);
	}
	(void) splx(s);
	return (TRUE);
}

/*ARGSUSED*/
static int
sc_abort(ap, pkt)
struct scsi_address *ap;
struct scsi_pkt *pkt;
{
	if (pkt != (struct scsi_pkt *) 0)
		return (FALSE);
	else
		return (FALSE);
}

static int
sc_reset(ap, level)
struct scsi_address *ap;
int level;
{
	struct scsitwo *sc = (struct scsitwo *) ap->a_cookie;
	struct scsi_cmd *sp = sc->sc_que;

	screset(sc, 0);

	if (sp) {
		/*
		 * If we're resetting everything, remove
		 * the current wait queue from reconsideration
		 * after we reset and blow away the current
		 * command.
		 */

		if (level == RESET_ALL) {
			sp = Nextcmd(sc->sc_que);
			Nextcmd(sc->sc_que) = 0;
		}

		sp->cmd_pkt.pkt_reason = CMD_RESET;
		sc_finish(sc);

		/*
		 * walk down the unrooted wait queue, 'finishing' off
		 * all the commands therein...
		 */
		if (level == RESET_ALL) {
			while (sp) {
				struct scsi_cmd *xp;
				xp = Nextcmd(sp);
				sp->cmd_pkt.pkt_reason = CMD_RESET;
				(*sp->cmd_pkt.pkt_comp)(sp);
				sp = xp;
			}
		}
		return (TRUE);
	} else {
		return (FALSE);
	}
}

/*ARGSUSED*/
static int
sc_getcap(ap, cap)
struct scsi_address *ap;
char *cap;
{
	if (cap == (char *) 0)
		return (UNDEFINED);
	else if (strncmp("dma_max", cap, 7) == 0)
		return (1<<16);
	else if (strncmp("msg_out", cap, 7) == 0)
		return (0);
	else if (strncmp("disconnect", cap, 10) == 0)
		return (0);
	else if (strncmp("synchronous", cap, 11) == 0)
		return (0);
	else if (strncmp("parity", cap, 6) == 0)
		return (0);
	else if (strncmp("initiator-id", cap, 12) == 0)
		return (HOST_ID);
	else
		return (UNDEFINED);
}

/*ARGSUSED*/
static int
sc_setcap(ap, cap, value)
struct scsi_address *ap;
char *cap;
int value;
{
	/*
	 * Cannot set any values yet
	 */
	return (UNDEFINED);
}



/*
 *
 * Internal start routine
 *
 */

static void
sc_ustart(sc)
register struct scsitwo *sc;
{
	register i;
	register struct scsi_cmd *sp = sc->sc_que;
	char dkn;
	int r;

	if (!sp)
		return;
	else if ((dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
		dk_busy |= (1<<dkn);
		dk_xfer[dkn]++;
		if ((sp->cmd_flags & CFLAG_DMASEND) == 0)
			dk_read[dkn]++;
		dk_wds[dkn] += sp->cmd_dmacount >> 6;
	}

	/*
	 * sc_select will wait for the scsi bus to be free, attempt
	 * to select the target, will set up the dma engine for any
	 * possible dma transfer, and will set the icr bits for having
	 * interrupts enabled if this is an interrupting command.
	 *
	 */

	sc->sc_busy = 1;
	if (sc_select(sc) == FALSE) {
		sc_finish(sc);
		return;
	}

	/*
	 * At this point, we should either ge into STATUS phase or into
	 * COMMAND phase.
	 */
#ifdef	SCDEBUG
	if (DEBUGGING) {
		sc_printf(sc, "%d.%d cmd=", Tgt(sp), Lun(sp));
		for (i = 0; i < sp->cmd_cdblen; i++) {
			printf(" 0x%x", sp->cmd_cdbp[i]);
		}
		printf("\n");
	}
#endif	SCDEBUG
	for (r = i = 0; i < sp->cmd_cdblen; i++) {
		if ((r = sc_putbyte(sc, ICR_COMMAND, sp->cmd_cdbp[i])) <= 0) {
			break;
		}
	}
	if (r < 0) {
		sc_printf(sc, "couldn't send command\n");
		sc_abort_cmd(sc);
	} else if (i > 0) {
		sp->cmd_pkt.pkt_state |= STATE_SENT_CMD;
		sp->cmd_cdbp = (caddr_t) (((u_long)sp->cmd_cdbp) + i);
	}
	/*
	 * We will either let interrupts drive us the rest of the
	 * way, or call a polling routine to do that for us
	 */
}

static int
sc_select(sc)
register struct scsitwo *sc;
{
	register struct scsi_cmd *sp = sc->sc_que;
	register int i;
	u_short icr_mode;

	DPRINTF2 ("select %d.%d\n", Tgt(sp), Lun(sp));

	/*
	 * make sure scsi bus is not continuously busy
	 */

	for (i = SC_WAIT_COUNT; i > 0; i--) {
		if ((sc->sc_reg->icr & ICR_BUSY) == 0)
			break;
		DELAY(10);
	}

	if (i == 0) {
		sc_printf(sc, "scsi bus continuously busy: ICR=0x%b\n",
			sc->sc_reg->icr, scbits);
		sp->cmd_pkt.pkt_reason = CMD_INCOMPLETE;
		screset(sc, 1); /* XXX? */
		return (FALSE);
	}

	/*
	 * Got the bus...
	 */

	sp->cmd_pkt.pkt_state |= STATE_GOT_BUS;

	/*
	 * select target and wait for response
	 */

	sc->sc_reg->icr = 0; /* Make sure SECONDBYTE flag is clear */

	/*
	 * Since we aren't an arbitrating initiator, we only
	 * put in the target address and *not* our own as well.
	 */

	sc->sc_reg->data = 1 << Tgt(sp);
	sc->sc_reg->icr = ICR_SELECT;

	/*
	 * See if target responds to selection
	 */

	if (sc_wait((u_short *)&sc->sc_reg->icr, SC_SHORT_WAIT, ICR_BUSY)) {
		DPRINTF2 ("%d.%d didn't select\n", Tgt(sp), Lun(sp));
		sc->sc_reg->data = 0;
		sc->sc_reg->icr = 0;
		sp->cmd_pkt.pkt_reason = CMD_INCOMPLETE;
		return (FALSE);
	}

	/*
	 * Got the target...
	 */

	sp->cmd_pkt.pkt_state |= STATE_GOT_TARGET;

	icr_mode = 0;
	if ((sp->cmd_pkt.pkt_flags & FLAG_NOINTR) == 0) {
		icr_mode |= ICR_INTERRUPT_ENABLE;
	}
	/*
	 * If a data transfer is expected, set it up here...
	 */

	if (sp->cmd_dmacount && (sp->cmd_flags&CFLAG_DMAVALID)) {
		SET_DMA_ADDR (sc->sc_reg, ((int)sp->cmd_data));
		sc->sc_reg->dma_count = ~sp->cmd_dmacount;
		if (((int)sp->cmd_data)&0x1) {
			DPRINTF ("odd byte DMA address\n");
		} else {
			icr_mode |= ICR_WORD_MODE;
		}
		icr_mode |= ICR_DMA_ENABLE;
	}

	/*
	 * set the interrupt register
	 */

	sc->sc_reg->icr = icr_mode;

	return (TRUE);
}

/*
 *
 * finish routine
 *
 */


static void
sc_finish(sc)
register struct scsitwo *sc;
{
	register struct scsi_cmd *sp = sc->sc_que;
	char dkn, wasintr;

	sc->sc_reg->icr = 0;	/* clear any pending interrupts */
	sc->sc_busy = 0;

	if ((dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
		dk_busy &= ~(1<<dkn);
	}
	DPRINTF1 ("finishing command %d\n", sc->sc_cmdid);
	sc->sc_cmdid++;
	sc->sc_que = Nextcmd(sp);
	if (sp->cmd_pkt.pkt_state & STATE_XFERRED_DATA) {
		/*
		 * Since we cannot disconnect, we cannot have had more
		 * than one data segment
		 */
		if (sp->cmd_subseg.d_next != (struct dataseg *) 0) {
			panic("sc_finish: more than one segment with data");
			/* NOTREACHED */
		}

		sp->cmd_pkt.pkt_resid =
			sp->cmd_dmacount - sp->cmd_subseg.d_count;

	}

	wasintr = (sp->cmd_pkt.pkt_flags & FLAG_NOINTR)? 0: 1;

	(*sp->cmd_pkt.pkt_comp)(sp);

	if (wasintr && sc->sc_que && sc->sc_busy == 0) {
		sc_ustart(sc);
	}
}


/*
 * Interrupt service section
 */

static int
sc_dopoll(sc, id)
register struct scsitwo *sc;
register id;
{
	register i;
	struct scsi_cmd *sp = sc->sc_que;

	for (i = 0; i < 300000 && id == sc->sc_cmdid; i++) {
		if (INTPENDING(sc->sc_reg)) {
			scintr(sc);
			i = 0;
		} else {
			DELAY(500);
		}
	}
	if (i >= 300000) {
		sc_printf(sc, "polled command timeout\n");
		sp->cmd_pkt.pkt_reason = CMD_TIMEOUT;
		screset(sc, 0);
		sc_finish(sc);
	}
}


/*
 * Handle a polling (autovectored) SCSI bus interrupt.
 */

static int
scpoll()
{
	register struct scsitwo *sc;
	register int serviced = 0;

	for (sc = &sc_softc[0]; sc < &sc_softc[NSC]; sc++) {
		if (sc->sc_reg && INTPENDING(sc->sc_reg)) {
			serviced += scintr(sc);
		}
	}
	return (serviced);
}


/*
 * Handle a scsi bus interrupt.
 */

int
scintr(sc)
register struct scsitwo *sc;
{
	register struct scsi_cmd *sp = sc->sc_que;

	if (sp == 0 || !sc->sc_reg || !(INTPENDING(sc->sc_reg))) {
		sc_printf(sc, "spurious interrupt\n");
		return (0);
	}

	DPRINTF2 ("scintr: ICR=0x%b\n", sc->sc_reg->icr, scbits);

	/*
	 * End of Data phase? Check for bus error first...
	 */
	if (sc->sc_reg->icr & ICR_BUS_ERROR) {
		sc_printf(sc, "Dma BUS Error on address 0x%x\n", sp->cmd_data);
		sc_abort_cmd(sc);
		return (1);
	} else if (sp->cmd_dmacount && (sp->cmd_flags & CFLAG_DMAVALID)) {
		int resid, reqamt, xfer_amt;

		resid = (u_short) (~sc->sc_reg->dma_count);
		reqamt = sp->cmd_dmacount;

		if (sc->sc_reg->icr & ICR_ODD_LENGTH) {
			if (sp->cmd_flags & CFLAG_DMASEND) {
				resid++;
			} else if (reqamt) {
				sc_flush(sc, (u_long)
				    (sp->cmd_data + reqamt - resid));
				resid--;
			}
		}
		if (xfer_amt = reqamt - resid) {
			DPRINTF1 ("xferred %d\n", xfer_amt);
			sp->cmd_data += xfer_amt;
			sp->cmd_subseg.d_count += xfer_amt;
			sp->cmd_pkt.pkt_state |= STATE_XFERRED_DATA;
		}
	}
	/*
	 * Now, get status and/or message...
	 */
	while (sc->sc_reg->icr & (ICR_BUSY|ICR_REQUEST)) {
		register u_short pbits;

		while (((pbits = sc->sc_reg->icr) & ICR_REQUEST) == 0) {
			if ((pbits & ICR_BUSY) == 0) {
				sc_printf(sc, "Target %d dropped BUSY\n",
				    Tgt(sp));
				sc_abort_cmd(sc);
				return (1);
			}
		}

		switch (pbits & ICR_BITS) {
		case ICR_STATUS:
			*sp->cmd_scbp = sc->sc_reg->cmd_stat;
			sp->cmd_pkt.pkt_state |= STATE_GOT_STATUS;
			break;
		case ICR_MESSAGE_IN:
		{
			static char *bad =
				"Bad Message '0x%x' from Target %d, Lun %d\n";

			switch (sc->sc_msgin = sc->sc_reg->cmd_stat) {
			case MSG_COMMAND_COMPLETE:
			case MSG_LINK_CMPLT:
			case MSG_LINK_CMPLT_FLAG:
				sp->cmd_pkt.pkt_reason = CMD_CMPLT;
				sc_finish(sc);
				break;
			default:
				sc_printf(sc, bad, sc->sc_msgin,
					Tgt(sp), Lun(sp));
				sc_abort_cmd(sc);
				break;
			}
			return (1);
			break;
		}
		default:
			sc_printf(sc, "Bad phase: ICR=0x%b\n", pbits, scbits);
			sc_abort_cmd(sc);
			return (1);
			break;
		}
	}
	return (1);
}

/*
 * Flush the last byte of an odd length transfer
 */

static void
sc_flush(sc, offset)
register struct scsitwo *sc;
register u_long offset;
{
	register u_int pv;
	u_char *mapaddr;

	if (MBI_MR(offset) < dvmasize) {
		DVMA[offset] = sc->sc_reg->data;
		return;
	}

#ifdef	sun3x
	pv = btop (VME24D16_BASE + (offset & VME24D16_MASK));
#else	sun3x
	pv = PGT_VME_D16 | VME24_BASE | btop(offset & VME24_MASK);
#endif	sun3x

	mapaddr = (u_char *) ((u_long) kmxtob(sc_kment) |
	    (u_long) MBI_OFFSET(offset));

	segkmem_mapin(&kseg, (addr_t) (((int)mapaddr) & PAGEMASK),
	    (u_int) mmu_ptob(1), PROT_READ | PROT_WRITE, pv, 0);

	*mapaddr = sc->sc_reg->data;

	segkmem_mapout(&kseg,
	    (addr_t) (((int)mapaddr) & PAGEMASK), (u_int) mmu_ptob(1));

	segkmem_mapout(&kseg, (addr_t) mapaddr, (u_int) mmu_btop(1));
}

/*
 * Miscellaneous functions
 */


static void
sc_abort_cmd(sc)
struct scsitwo *sc;
{
	sc->sc_que->cmd_pkt.pkt_reason = CMD_RESET;
	screset(sc, 1);
	sc_finish(sc);
}

/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 * Returns 0, if successful, else -1;
 */

static int
sc_wait(icrp, wait_count, cond)
	register u_short *icrp;
	register int wait_count;
{
	register int i;
	register u_short icr;

	for (i = 0; i < wait_count; i++) {
		icr = *icrp;
		if ((icr & cond) == cond) {
			return (0);
		} else if (icr & ICR_BUS_ERROR) {
			break;
		} else
			DELAY(10);
	}
	return (-1);
}

/*
 * Put data byte onto the scsi bus if phase match.
 * Returns:	1  if successful (phase matched)
 *		0  if incorrect phase, but REQ is present
 *		-1 if no REQ present
 */

static
sc_putbyte(sc, bits, data)
register struct scsitwo *sc;
register u_short bits;
register u_char data;
{
	register u_short icr;

	if (sc_wait((u_short *)&sc->sc_reg->icr, SC_WAIT_COUNT, ICR_REQUEST)) {
		DPRINTF ("sc_putbyte: no REQ\n");
		return (-1);
	}
	icr = sc->sc_reg->icr;
	if ((icr & ICR_BITS) != bits) {
		DPRINTF2 ("sc_putbyte: ICR=0x%b\n", icr, scbits);
		return (0);
	}
	sc->sc_reg->cmd_stat = data;
	return (1);
}

/*
 * Reset SCSI control logic and bus.
 */
static void
screset(sc, msg_enable)
struct scsitwo *sc;
int msg_enable;
{

	if (msg_enable) {
		sc_printf(sc, "Resetting SCSI bus\n");
	}

	sc->sc_reg->icr = ICR_RESET;
	DELAY(50);
	sc->sc_reg->icr = 0;

	/* give reset scsi devices time to recover (> 2 Sec) */
	DELAY(SC_RESET_DELAY);
}

/*ARGSUSED*/
static void
sc_watch(arg)
caddr_t arg;
{
	register s;
	register struct scsitwo *sc;
	register struct scsi_cmd *sp;

	for (sc = &sc_softc[0]; sc < &sc_softc[NSC]; sc++) {
		if (sc->sc_reg) {
			s = splr(sc->sc_tran.tran_spl);
			sp = sc->sc_que;
			if (sp && (sp->cmd_flags & CFLAG_WATCH)) {
				if (sp->cmd_timeout == 0) {
					/*
					 * A pending interrupt
					 * defers the sentence
					 * of death.
					 */
					if (INTPENDING(sc->sc_reg)) {
						sp->cmd_timeout++;
						(void) splx(s);
						continue;
					}
					sc_printf(sc, "command timeout\n");
					sp->cmd_pkt.pkt_reason = CMD_TIMEOUT;
					sc_reset(sc, 0);
					sc_finish(sc);
					(void) splx(s);
					continue;
				} else {
					sp->cmd_timeout -= 1;
				}
			}
			(void) splx(s);
		}
	}
	timeout(sc_watch, (caddr_t) 0, hz);
}

/*VARARGS*/
static void
sc_printf(sc, fmt, a, b, c, d, e, f, g, h)
struct scsitwo *sc;
char *fmt;
int a, b, c, d, e, f, g, h;
{
	printf("%s%d: ", CNAME, CNUM);
	printf(fmt, a, b, c, d, e, f, g, h);
}

#endif	(NSC > 0)
