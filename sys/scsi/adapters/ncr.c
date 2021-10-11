/* @(#)ncr.c 1.1 92/07/30 SMI */
/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */
#include "ncr.h"
#if NNCR > 0

#ifndef lint
static  char sccsid[] = "@(#)ncr.c 1.1  92/07/30";
#endif  lint


/*
 * Features of this version:
 *
 * A.	Set up dma before any possibility of going into a DATA PHASE.
 * 	This means that the dma engine must be setup:
 *
 *	1:  prior to dropping SEL and deasserting NCR_ICR_DATA
 *	    in the selection process
 *
 *	2:  prior to acknowledging any message (including the IDENTIFY
 *	    message received during reselection).
 *
 *	Ick. This must be done because this way because the 3/50 && 3/60
 * 	DMA engines don't work on sending out the SCSI bus otherwise.
 *
 * B.	We get mostly PHASE NOT MATCHED interrupts on other than DATA PHASE.
 *
 * C.	Linked commands now work
 */


#include <scsi/scsi.h>
#include <scsi/adapters/ncrreg.h>
#include <sys/mman.h>
#include <machine/pte.h>
#include <machine/cpu.h>
#include <vm/seg.h>


/*
 * Debugging macros
 */

int	ncr_debug = 0;
#define	PRINTF1	if (ncr_debug) printf
#define	PRINTF2	if (ncr_debug > 1) printf
#define	PRINTF3	if (ncr_debug > 2) printf
/* #define	EPRINTF	if (scsi_options & SCSI_DEBUG_HA) printf */
#define	EPRINTF	printf
#define	INFORMATIVE 	(ncr_debug)

/*
 * Short hand defines
 */
#define	UNDEFINED		-1

#define	CURRENT_CMD(ncr)	((ncr)->n_slots[(ncr)->n_cur_slot])
#define	SLOT(sp)		((short)(Tgt((sp))<<3|(Lun((sp)))))
#define	NEXTSLOT(slot)		((slot)+1) & ((NTARGETS*NLUNS_PER_TARGET)-1)

#define	Tgt(sp)			((sp)->cmd_pkt.pkt_address.a_target)
#define	Lun(sp)			((sp)->cmd_pkt.pkt_address.a_lun)

#define	NON_INTR(sp)	((sp->cmd_pkt.pkt_flags & FLAG_NOINTR) != 0)

#define	CNAME	ncrdriver.mdr_cname
#define	CNUM	(ncr-ncr_softc)

/*
 * Global function declarations
 */

void ncrintr();

/*
 * Local function declarations
 */

static int ncr_start(), ncr_abort(), ncr_reset(), ncr_getcap(), ncr_setcap();

static int ncr_NACKmsg(), ncr_ACKmsg(), ncr_sbcwait(), ncr_csrwait();
static int ncr_xfrin(), ncr_xfrin_noack();


static int ncr_ustart();
static void ncr_phasemanage();
static int ncr_getphase();

static int ncr_dopoll(), ncrpoll();
static void ncrsvc();

static int ncr_select(), ncr_reselect();

static void ncr_preempt(), ncr_disconnect();

static int ncr_sendcmd(), ncr_sendmsg(), ncr_recvmsg(), ncr_senddata();
static int ncr_recvdata(), ncr_recvstatus();

static void ncr_finish();

static int ncr_watch();

static void ncr_printstate(), ncr_curcmd_timeout(), ncr_internal_abort();
static void ncr_do_abort(), ncr_internal_reset(), ncr_hw_reset();
static void ncr_dump_datasegs();

static caddr_t ncr_mapdmabuf();

/*
 * DMA function declarations
 */

static int ncr_flushbyte(), ncr_dma_wait(), ncr_dma_cleanup();
static void ncr_dma_enable(), ncr_dma_setup();


static int ncr_vme_dma_cleanup(), ncr_vme_waitdma(), ncr_vme_dma_recv();

#ifdef	sun4
static int ncr_cobra_dma_cleanup();
static int ncr_cobra_dma_chkerr(), ncr_cobra_dma_recv();
#endif	sun4
#ifdef	sun3
static int  ncr_ob_dma_cleanup();
static void ncr_ob_dma_setup();
#endif

static int ncr_3e_dma_cleanup();
static int ncr_3e_fetchdata(), ncr_3e_storedata();

/*
 * Local static data
 */

static struct ncr *ncr_softc = (struct ncr *) 0;

static long  Dmabase;	/*
			 * Base address to stuff into DMA engine
			 * and or into the assumed offset from DVMA
			 * that is in the dma cookies we get.
			 */
static char *Cname;	/*
			 * pointer to controller name- copied here so that
			 * error printouts don't generated long sequences
			 * of structure pointer de-references.
			 */

static int ncr_arbitration_delay	= NCR_ARBITRATION_DELAY;
static int ncr_bus_clear_delay		= NCR_BUS_CLEAR_DELAY;
static int ncr_bus_settle_delay		= NCR_BUS_SETTLE_DELAY;
static char *cbsr_bits = CBSR_BITS;

/*
 * Probe, Slave and Attach routines
 */

static int ncr_probe(), ncr_slave(), ncr_attach();
static int nncr = NNCR;
struct mb_ctlr *ncrctlr[NNCR];
struct mb_driver ncrdriver = {
	ncr_probe, ncr_slave, ncr_attach, 0, 0, ncrpoll,
	NCR_HWSIZE, "scsibus", 0, "ncr", ncrctlr, MDR_BIODMA
};

static int
ncr_probe(reg, ctlr)
u_short *reg;
int ctlr;
{
	register struct ncrsbc *ncrp = (struct ncrsbc *) reg;
	register struct ncr *ncr;
	long val;
	u_long dma_base, sbcaddr, ctladdr, dmaaddr;
#ifdef	sun3
	u_long udcp;
#endif
	caddr_t dmabuf;
	int host_type = -1;
	u_short *shortp;


	/*
	 * Check for ncr 5380 Scsi Bus Ctlr chip with "peekc()"; struct
	 * ncr-5380 is common to all onboard scsi and vme scsi board. if not
	 * exist, return 0.
	 */

	PRINTF3("ncr_probe: reg= %x virt, %x phys\n", reg,
		getdevaddr((caddr_t) reg));

	/*
	 * We can use the first byte address of reg, 'coz we'll guarantee
	 * (by the time we are through) that that will always be the first
	 * byte of the NCR 5380 SBC (which should be the cbsr register).
	 */

	if (peekc((caddr_t)reg) == -1) {
		return (0);
	}
	sbcaddr = (u_long) reg;
	dmaaddr = ((u_long) reg) + sizeof (struct ncrsbc);

	/*
	 * probe for different host adaptor interfaces
	 */

	switch (cpu) {
#ifdef sun4
	case CPU_SUN4_110:
		/*
		 * probe for 4/110 dma interface
		 */

		ctladdr = ((u_long) reg) + COBRA_CSR_OFF;
		if (peekl((long *) (dmaaddr), (long *) &val) == -1) {
			return (0);
		}
		host_type = IS_COBRA;
		dma_base = DMA_BASE_110;
		break;
#endif	sun4
#ifdef	sun3
	case CPU_SUN3_50:
	case CPU_SUN3_60:
		ctladdr = ((u_long) reg) + CSR_OFF;
		if (peek((short *)&
		    ((struct ncrdma *)dmaaddr)->udc_rdata) == -1)
			return (0);
		udcp = (u_long) IOPBALLOC(sizeof (struct udc_table));

		if (udcp == 0 || udcp & 0x3) {
			printf("%s%d: no udc table\n", CNAME, ctlr);
			if (udcp)
				IOPBFREE(udcp, sizeof (struct udc_table));
			return (0);
		}
		host_type = IS_3_50;
		dma_base = DMA_BASE_3_50;
		break;
#endif	sun3
	default:

		/*
		 * This is either a SCSI-3 or a 3/E VME board
		 *
		 * First see whether the dma address register is there
		 * The low 16 bits is common across all platforms.
		 *
		 */

		if (peek((short *) (dmaaddr + 2)) == -1) {
			return (0);
		}

		/*
		 * Okay, now whether it is a 3/E board
		 */

		ctladdr = ((u_long) reg) + SUN3E_CSR_OFF;
		if (peek((short *) (ctladdr + 2)) != -1) {
			if (poke ((short *) (ctladdr + 2), 0) == 0) {
				if (peek((short *) (ctladdr + 2)) == 0x402) {
					dmabuf = ncr_mapdmabuf(reg);
					if (dmabuf == (caddr_t) 0)
						return (0);
					host_type = IS_3E;
					dma_base = 0;
					break;
				}
			}
		}

		/*
		 * Okay- it wasn't a 3/E. Now see whether it's a scsi-3 board.
		 *
		 * Make sure that it isn't a SCSI-2 board (which occupies 4k
		 * of VME space instead of the 2k that the SCSI-3 occupies).
		 * (the above is a quote from si.c. The code below doesn't
		 *  seem to bear this out exactly).
		 *
		 */


		if (peek((short *) (((u_long) reg) + 0x800)) != -1) {
			return (0);
		}

		/*
		 * Make sure that we're cool (really a scsi-3 board).
		 */

		ctladdr = ((u_long) reg) + CSR_OFF;

		if (peek((short *) (ctladdr + 2)) == -1)
			return (0);

		if ((((struct ncrctl *) ctladdr)->csr.lsw & NCR_CSR_ID) == 0) {
			printf("%s%d: unmodified scsi-3 board- you lose..\n",
				CNAME, ctlr);
			return (0);
		}

		host_type = IS_SCSI3;
		dma_base = 0;
		break;
	}

	/*
	 * Establish initial softc values
	 */

	if (ncr_softc == 0) {
		ncr_softc = (struct ncr *)
			kmem_zalloc((unsigned) (NNCR * sizeof (struct ncr)));
		if (ncr_softc == 0)
			return (0);
		Dmabase = dma_base;
		timeout(ncr_watch, (caddr_t) 0, hz);
	}

	/*
	 * initialize software structure
	 */

	ncr = &ncr_softc[ctlr];
	/*
	 * Allocate a page for being able to
	 * flush the last bit of a data transfer.
	 */
	ncr->n_kment = rmalloc(kernelmap, mmu_btopr(MMU_PAGESIZE));
	if (ncr->n_kment == 0) {
		return (0);
	}
	ncr->n_dev = &ncrdriver;
	ncr->n_id = NCR_HOST_ID;
	ncr->n_sbc = (struct ncrsbc *) (sbcaddr);
	ncr->n_dma = (struct ncrdma *) (dmaaddr);
	ncr->n_ctl = (struct ncrctl *) (ctladdr);
	ncr->n_type = host_type;

	ncr->n_tran.tran_start = ncr_start;
	ncr->n_tran.tran_abort = ncr_abort;
	ncr->n_tran.tran_reset = ncr_reset;
	ncr->n_tran.tran_getcap = ncr_getcap;
	ncr->n_tran.tran_setcap = ncr_setcap;
	ncr->n_tran.tran_pktalloc = scsi_std_pktalloc;
	ncr->n_tran.tran_dmaget = scsi_std_dmaget;
	ncr->n_tran.tran_pktfree = scsi_std_pktfree;
	ncr->n_tran.tran_dmafree = scsi_std_dmafree;

	ncr->n_last_slot = ncr->n_cur_slot = UNDEFINED;
	switch (host_type) {
#ifdef	sun4
	case IS_COBRA:
		ncr->n_dma_setup = ncr_dma_setup;
		ncr->n_dma_cleanup = ncr_cobra_dma_cleanup;
		break;
#endif	sun4
#ifdef	sun3
	case IS_3_50:
		ncr->n_dma_setup = ncr_ob_dma_setup;
		ncr->n_dma_cleanup = ncr_ob_dma_cleanup;
		ncr->n_udc = (struct udc_table *) udcp;
		break;
#endif	sun3
	case IS_SCSI3:
		ncr->n_dma_setup = ncr_dma_setup;
		ncr->n_dma_cleanup = ncr_vme_dma_cleanup;
		break;
	case IS_3E:
		ncr->n_dmabuf = dmabuf;
		ncr->n_dma_setup = ncr_dma_setup;
		ncr->n_dma_cleanup = ncr_3e_dma_cleanup;
		break;
	}

	ncr_internal_reset(ncr, NCR_RESET_ALL, RESET_NOMSG);
	return (NCR_HWSIZE);
}

/*ARGSUSED*/
static int
ncr_slave(md, reg)
struct mb_device *md;
caddr_t reg;
{
	struct ncr *ncr = &ncr_softc[md->md_unit];
	md->md_slave = ncr->n_id << 3;
	return (TRUE);
}

static int
ncr_attach(md)
struct mb_device *md;
{
	int i;
	static int ncr_spl = 0;
	struct ncr *ncr = &ncr_softc[md->md_unit];

	/*
	 * calculate max spl value so far seen and the default config_value
	 */

	ncr_spl = MAX(ncr_spl, pritospl(md->md_intpri));

	/*
	 * make sure that everyone, including us, is locking at the same
	 * priority
	 */

	for (i = 0; i <= md->md_unit; i++) {
		ncr_softc[i].n_tran.tran_spl = ncr_spl;
	}

	/*
	 * Initialize interrupt vector register (if any)
	 */

	if (IS_VME(ncr->n_type) && md->md_mc->mc_intr) {
		if (ncr->n_type == IS_3E) {
			N_CTL->ivec2 = (md->md_mc->mc_intr->v_vec & 0xff);
		} else {
			N_CTL->iv_am = (md->md_mc->mc_intr->v_vec & 0xff) |
				VME_SUPV_DATA_24;
		}
		*(md->md_mc->mc_intr->v_vptr) = (int) ncr;
	}
	/*
	 * Now make ourselves known to the rest of the SCSI world
	 */
	scsi_config(&ncr->n_tran, md);
}

/*
 * map in 3/E dma buffer. This buffer resides in the
 * 64kb below the base address of the NCR 5380 registers.
 *
 *
 * XXX: We're cheating here because we know what address space
 * XXX: the 3/E board resides in, plus we know that the dma buffer
 * XXX: is on a page boundary.
 * XXX:
 * XXX: We need to do a better job than that on this!
 */

static caddr_t
ncr_mapdmabuf(sbcreg)
u_short *sbcreg;
{
	extern int getdevaddr();
	extern struct seg kseg;
	u_long addr, pageval;
	u_short *reg;
	long a;


	addr =  getdevaddr((caddr_t) sbcreg) - (64 * 1024);
	if (addr & MMU_PAGEOFFSET) {
		return ((caddr_t) 0);
	}
#ifdef	sun3x
	pageval = btop(VME24D16_BASE + (addr & VME24D16_MASK));
#else	sun3x
	pageval = PGT_VME_D16 | btop(VME24_BASE | (addr & VME24_MASK));
#endif	sun3x
	a = rmalloc(kernelmap, btoc(64 * 1024));
	if (a == 0)
		return ((caddr_t) 0);
	reg = (u_short *) ((int) kmxtob(a));

	segkmem_mapin(&kseg, (addr_t) reg, (u_int) (64 * 1024),
		PROT_READ | PROT_WRITE, pageval, 0);

	if (peek((short *)reg) < 0 || poke((short *)reg, (short) 0xa55e) ||
	    peek((short *)reg) != 0xa55e) {
		rmfree (kernelmap, btoc (64 * 1024), (u_long) a);
		return ((caddr_t) 0);
	}

	return ((caddr_t) reg);
}

/*
 * External Interface Routines
 */


static int
ncr_start(sp)
register struct scsi_cmd *sp;
{
	register struct ncr *ncr;
	register int s;
	short slot;

	/*
	 * get scsi address based on the "cookie" in cmd_packet
	 */

	ncr = (struct ncr *)sp->cmd_pkt.pkt_address.a_cookie;
	slot =	((Tgt(sp)*NLUNS_PER_TARGET) | Lun(sp));

	s = splr(ncr->n_tran.tran_spl);

	if (ncr->n_slots[slot] != (struct scsi_cmd *) 0) {
		PRINTF3("ncr_start: queuing error\n");
		(void) splx(s);
		return (FALSE);
	}

	/*
	 * reinitialize some fields in the 'cmd_pkt' that need it...
	 * if upper level requests watchdog, set timeout_cnt and flag
	 */

	sp->cmd_pkt.pkt_resid = 0;
	sp->cmd_pkt.pkt_reason = sp->cmd_pkt.pkt_state =
		sp->cmd_pkt.pkt_statistics = 0;
	sp->cmd_cdbp = (caddr_t) sp->cmd_pkt.pkt_cdbp;
	sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
	*sp->cmd_scbp = 0;		/* clear status byte array */
	if (sp->cmd_timeout = sp->cmd_pkt.pkt_time)
		sp->cmd_flags |= CFLAG_WATCH;
	else
		sp->cmd_flags &= ~CFLAG_WATCH;
	sp->cmd_data = sp->cmd_saved_data = sp->cmd_mapping;
	sp->cmd_subseg.d_base = sp->cmd_mapping;
	sp->cmd_subseg.d_count = 0;
	sp->cmd_subseg.d_next = (struct dataseg *) 0;
	sp->cmd_cursubseg = &sp->cmd_subseg;
	sp->cmd_flags &= ~(CFLAG_NEEDSEG|CFLAG_CMDDISC);

	/*
	 * If this is a non-interrupting command, don't allow disconnects
	 */
	if (NON_INTR(sp)) {
		sp->cmd_pkt.pkt_flags |= FLAG_NODISCON;
	}

	/*
	 * accept the command by setting the req job_ptr in appropriate slot
	 */

	ncr->n_ncmds++;
	ncr->n_slots[slot] = sp;

	if ((sp->cmd_pkt.pkt_flags & FLAG_NOINTR) == 0) {
		if ((ncr->n_npolling == 0) && (ncr->n_state == STATE_FREE)) {
			(void) ncr_ustart(ncr, slot);
		}
	} else  if (ncr->n_state == ACTS_ABORTING) {
		printf("%s%d: unable to start non-intr cmd\n", CNAME, CNUM);
		(void) splx(s);
		return (FALSE);
	} else {
		/*
		 * Wait till all current commands completed with STATE_FREE by
		 * "ncr_dopoll()", then fire off the job, check if preempted
		 * right away, try again; accept the command
		 */
		PRINTF3("ncr_start: poll cmd, state= %x\n", ncr->n_state);
		ncr->n_npolling++;
		while (ncr->n_npolling != 0) {
			/*
			 * drain any current active command(s)
			 */
			while (ncr->n_state != STATE_FREE) {
				if (ncr_dopoll(ncr)) {
					(void) splx(s);
					return (FALSE);
				}
			}
			/*
			 * were we preempted by a reselect coming back in?
			 * If so, 'round we go again....
			 */
			PRINTF3("ncr_start: ready to start cmd\n");
			if (ncr_ustart(ncr, slot) == FALSE)
				continue;
			/*
			 * Okay, now we're 'running' this command.
			 *
			 * ncr_dopoll will return when ncr->n_state ==
			 * STATE_FREE, but this can also mean a reselection
			 * preempt occurred.
			 */
			PRINTF3("ncr_start: poll_res\n");
			if (ncr_dopoll(ncr)) {
				(void) splx(s);
				return (FALSE);
			}
		}
		PRINTF3("ncr_start: start next slot= %x\n", NEXTSLOT(slot));
		(void) ncr_ustart(ncr, NEXTSLOT(slot));
	}
	(void) splx(s);
	PRINTF3("ncr_start: done\n");
	return (TRUE);
}

/*ARGSUSED*/
static int
ncr_abort(ap, pkt)
struct scsi_address *ap;
struct scsi_pkt *pkt;
{
	PRINTF3("ncr_abort: pkt= %x\n", pkt);
	if (pkt != (struct scsi_pkt *) 0) {
	    PRINTF3("ncr_abort: call ncr_do_abort(RESET)\n");
	    ncr_do_abort((struct ncr *)ap->a_cookie,
		NCR_RESET_ALL, RESET_NOMSG);
	    return (TRUE);
	} else {
	    return (FALSE);
	}
}

static int
ncr_reset(ap, level)		/* clear jobs & reset H/W for external-req */
struct scsi_address *ap;
int level;
{
	struct ncr *ncr = (struct ncr *) ap->a_cookie;

	PRINTF3("ncr_reset: level= %x\n", level);
	/*
	 * if RESET_ALL requested, call "ncr_do_abort()" to clear all
	 * outstanding jobs in the queue
	 */
	if (level == RESET_ALL) {
		ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
		return (TRUE);
	} else
		return (FALSE);
}

/*ARGSUSED*/
static int
ncr_getcap(ap, cap)		/* return host-adapter's capabilities */
struct scsi_address *ap;
char *cap;
{
	PRINTF3("ncr_getcap:\n");
	/*
	 * Check requested function (such as "dma_max, msg_out, sync,
	 * disconnect, parity, and Ids) support on the behave of
	 * host_adapter H/W platfrom and S/W driver
	 */
	if (cap == (char *) 0)
		return (UNDEFINED);
	else if (strncmp("dma_max", cap, 7) == 0)
		return (1<<16);
	else if (strncmp("msg_out", cap, 7) == 0)
		return (TRUE);		/* 1 */
	else if (strncmp("disconnect", cap, 10) == 0)
		return (TRUE);		/* 1 */
	else if (strncmp("synchronous", cap, 11) == 0)
		return (FALSE);		/* 0 */
	else if (strncmp("parity", cap, 6) == 0)
		return (FALSE);
	else if (strncmp("initiator-id", cap, 12) == 0) {
		struct ncr *ncr = (struct ncr *) ap->a_cookie;
		return (ncr->n_id);
	} else
		return (UNDEFINED);
}

/*ARGSUSED*/
static int
ncr_setcap(ap, cap, value)	/* set host-adapter actions per request */
struct scsi_address *ap;
char *cap;
int value;
{
	PRINTF3("ncr_setcap: val= %x\n", value);
	/* set host_adapter driver's action based on inputted request */
	return (UNDEFINED);
}

/*
 * Internal start and finish routines
 */

/*
 * Start the next command on the host adapter.
 * Search from start_slot for work to do.
 *
 *
 *	input:  (struct ncr) *ncr= pointer to a ncr software structure;
 *		(short) start_slot= requested slot (target/lun combo);
 *	return: (int) TRUE(1)= command started okay;
 *		(int) FALSE(0)= not started (due to reselection or no work)
 */

static int
ncr_ustart(ncr, start_slot)
register struct ncr *ncr;
short start_slot;
{
	register struct scsi_cmd *sp;
	register short slot = start_slot;
	int found = 0;
	int i;
	char dkn;

	PRINTF3("ncr_ustart: start_slot= %x\n", start_slot);

	/*
	 * Start off a new job in the queue ONLY number of running cmds
	 * is less than disconnected cmds, in hope of NOT floating the bus
	 * and allow the previously disconnected jobs to be finished first,
	 * if more currently disconnected jobs, return ACTION_ABORT
	 *
	 * XXX: mjacob sez that this doesn't make him happy
	 */

	if ((ncr->n_ncmds - ncr->n_ndisc) <= 0) {
		PRINTF3("ncr_ustart: NO new-job\n");
		return (FALSE);
	}

	/*
	 * search for any ready cmd available (started first with the req
	 * slot, then move to next slot until reaching req_one)
	 */

	do {
		sp = ncr->n_slots[slot];
		if (sp && ((sp->cmd_flags & CFLAG_CMDDISC) == 0)) {
			found++;
		} else {
			slot = NEXTSLOT(slot);
		}
	} while ((found == 0) && (slot != start_slot));

	if (!found) {
		return (FALSE);
	}

	UPDATE_STATE(STATE_STARTING);

	PRINTF3("ncr_ustart: starting %d.%d\n", Tgt(sp), Lun(sp));
	ncr->n_cur_slot = slot;
	ncr->n_omsgidx = ncr->n_omsglen = 0;


	/*
	 * Attempt to arbitrate for the bus and select the target.
	 *
	 * ncr_select() can return one of SEL_TRUE (target selected),
	 * SEL_ARBFAIL (unable to get the bus), SEL_FALSE (target did
	 * not respond to selection), or SEL_RESEL (a reselection attempt
	 * is in progress). As a side effect, if SEL_TRUE is the return,
	 * DMA (and interrupts) from the SBC are disabled.
	 *
	 */

	switch (ncr_select(ncr)) {
	case SEL_ARBFAIL:
		/*
		 * XXX: Should we treat arbitration failures
		 * XXX: differently than selection failures?
		 */

	case SEL_FALSE:
		(void) ncr_finish(ncr);
		return (FALSE);

	case SEL_TRUE:

		if ((sp->cmd_flags & CFLAG_DMAVALID) &&
		    (dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
			dk_busy |= (1<<dkn);
			dk_xfer[dkn]++;
			if ((sp->cmd_flags & CFLAG_DMASEND) == 0)
				dk_read[dkn]++;
			dk_wds[dkn] += sp->cmd_dmacount >> 6;
		}

		UPDATE_STATE(ACTS_UNKNOWN);
		if (NON_INTR(sp))
			ncr_phasemanage(ncr);
		return (TRUE);

	case SEL_RESEL:

		/*
		 * Couldn't select due to a reselection coming in.
		 * Push the state of this command back to what it was.
		 */
		ncr_preempt(ncr);
		return (FALSE);
	}
}

/*
 * Finish routine
 */

static void
ncr_finish(ncr)
register struct ncr *ncr;
{
	short last_slot;
	register int span_states = 0;
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	char dkn;

	PRINTF3("ncr_finish:\n");
	if ((dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
		dk_busy &= ~(1<<dkn);
	}

	if (ncr->n_last_msgin == MSG_LINK_CMPLT ||
		ncr->n_last_msgin == MSG_LINK_CMPLT_FLAG) {
		span_states++;
	}

	if (sp->cmd_pkt.pkt_state & STATE_XFERRED_DATA) {
		register struct dataseg *segtmp = sp->cmd_subseg.d_next;
		register i;

		/*
		 * XXX : FIX ME NEED TO MERGE TOGETHER OVERLAPPING AND
		 * XXX : ADJACENT SEGMENTS!!!
		 */
		if (segtmp != (struct dataseg *) 0 && segtmp->d_count) {
			panic("ncr_finish: more than one segment with data");
			/* NOTREACHED */
		}

		/*
		 * Walk through all data segments and count up transfer counts
		 */
		i = 0;
		for (segtmp = &sp->cmd_subseg; segtmp;
		    segtmp = segtmp->d_next) {
			i += segtmp->d_count;
		}
		sp->cmd_pkt.pkt_resid = sp->cmd_dmacount - i;
		if (INFORMATIVE && sp->cmd_pkt.pkt_resid) {
			PRINTF2("ncr_finish: %d.%d finishes with %d resid\n",
				Tgt(sp), Lun(sp), sp->cmd_pkt.pkt_resid);
		}
	}

	ncr->n_ncmds -= 1;
	last_slot = ncr->n_last_slot = ncr->n_cur_slot;
	ncr->n_lastcount = 0;
	ncr->n_cur_slot = UNDEFINED;
	ncr->n_slots[last_slot] = (struct scsi_cmd *) 0;
	ncr->n_omsglen = ncr->n_omsgidx = 0;
	ncr->n_last_msgin = 0xfe;
	UPDATE_STATE(STATE_FREE);

	PRINTF3("ncr_finish: span= %x, flag= %x\n",
		span_states, sp->cmd_pkt.pkt_flags);
	if (NON_INTR(sp)) {
		PRINTF3("ncr_finish: poll= calling upper target to finish\n");
		ncr->n_npolling -= 1;
		(*sp->cmd_pkt.pkt_comp)(sp);
	} else if (span_states > 0) {
		ncr->n_state = ACTS_SPANNING;
		(*sp->cmd_pkt.pkt_comp)(sp);
		/*
		 * This is we can check upon return that
		 * the target driver did the right thing...
		 *
		 * If the target driver didn't do the right
		 * thing, we have to abort the operation.
		 */
		if (ncr->n_slots[last_slot] == 0) {
			ncr_internal_abort(ncr);
		} else {
			PRINTF2("%s%d: linked command start\n", CNAME, CNUM);
			ncr->n_cur_slot = last_slot;
			ncr->n_nlinked++;
			if (ncr_ACKmsg(ncr)) {
				ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
			} else {
				sp = CURRENT_CMD(ncr);
				if ((sp->cmd_flags & CFLAG_DMAVALID) &&
				    (dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
					dk_busy |= (1<<dkn);
					dk_xfer[dkn]++;
					if (!(sp->cmd_flags & CFLAG_DMASEND))
					dk_read[dkn]++;
					dk_wds[dkn] += sp->cmd_dmacount >> 6;
				}
				UPDATE_STATE(ACTS_UNKNOWN);
				ncr_phasemanage(ncr);
			}
		}
	} else {
		(*sp->cmd_pkt.pkt_comp)(sp);
		if (ncr->n_state == STATE_FREE) {
			(void) ncr_ustart(ncr, last_slot);
		}
	}
}



/*
 * Interrupt Service Routines
 */


/*
 * polled service routine - called when interrupts are not feasible
 *
 * Note that we call ncrpoll() rather than ncrsvc directly. This is
 * so we can service other ncr controllers while we're polling this
 * one.
 */

static int
ncr_dopoll(ncr)
register struct ncr *ncr;
{
	register int i;

	PRINTF3("ncr_dopoll: state= %x\n", ncr->n_state);
	while (ncr->n_state != STATE_FREE) {
	    PRINTF3("ncr_dopoll: n_state= %x\n", ncr->n_state);
	    for (i = 0; (i < 3*120000) && (ncr->n_state != STATE_FREE); i++) {
		if (ncrpoll()) {	/* 1 is Okay */
			if (ncr->n_state == STATE_FREE)
				continue;
			else
				i = 0;
		} else {
			DELAY(100);
		}
		if (i >= 3*120000 && ncr->n_state != STATE_FREE) {
			EPRINTF("ncr_dopoll: poll_cmd timeout, state= %x\n",
				ncr->n_state);
			return (FAILURE);
		}
	    }
	}
	return (SUCCESS);
}

/*
 * polled (autovector) interrupt entry point
 */

static int
ncrpoll()
{
	register struct ncr *ncr;
	int serviced = 0;

	for (ncr = ncr_softc; ncr < &ncr_softc[NNCR]; ncr++) {
		if (ncrctlr[ncr-ncr_softc]) {
			ncr->n_ints++;
			while (INTPENDING(ncr)) {
				ncrsvc(ncr);
				serviced = 1;
			}
		}
	}
	return (serviced);
}

/*
 * vectored interrupt entry point(s)
 */

void
ncrintr(ncr)
struct ncr *ncr;
{
	if (ncrctlr[CNUM]) {
		ncr->n_ints++;
		while (INTPENDING(ncr)) {
			ncrsvc(ncr);
		}
	} else {
		printf("%s%d: spurious vectored interrupt\n", CNAME, CNUM);
	}
}


/*
 * Common interrupt service code- called asynchronously from ncrpoll() or
 * ncrintr(), or synchronously from varying places in the rest of the
 * driver to service interrupts.
 *
 * What kind of interrupts we'll get:
 *
 *	* RESELECTION interrupts
 *	* End of DATA PHASE interrupts (PHASE MISMATCH)
 *	* Some specific PHASE MISMATCH interrupts (driven by
 *	  enabling DMA mode in the sbc mr register after setting
 *	  a bogus phase into the tcr register- when REQ* is asserted
 *	  this causes a phase mismatch interrupt.
 *
 * XXX:	* Monitor LOSS OF BUSY interrupts
 * XXX:	* PARITY Errors
 */

static void
ncrsvc(ncr)
register struct ncr *ncr;
{
	register u_char binary_id = NUM_TO_BIT(ncr->n_id);
	register u_char uctmp;

	/*
	 * 'Disabling' DMA also allows access to SBC registers
	 */
	if (ncr->n_type != IS_3_50)
		DISABLE_DMA(ncr);
	uctmp = N_SBC->cbsr;

	if (ncr->n_type != IS_COBRA && ncr->n_type != IS_3E) {
		ncr->n_lastbcr = GET_BCR(ncr);
	}

	/*
	 * First check for a reselect interrupt coming in
	 */

	if (RESELECTING(uctmp, N_SBC->cdr, binary_id)) {
		if (ncr_reselect(ncr)) {
			uctmp = 0;
			ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
		} else {
			uctmp = 1;
		}
	} else if (IN_DATA_STATE(ncr)) {
		/*
		 * XXX: should be able to return and await another REQ*
		 * XXX: here? Probably wouldn't help because interrupt
		 * XXX: latency + dma cleanup generally will give the
		 * XXX: targets time to shift to status and/or msg in
		 * XXX: phase.
		 */
		if ((*ncr->n_dma_cleanup)(ncr)) {
			ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
			uctmp = 0;
		} else {
			uctmp = 1;
		}
	} else if (ncr->n_state == STATE_FREE) {
		if (ncr_debug) {
			printf("%s%d: spurious interrupt\n", CNAME, CNUM);
			ncr_printstate(ncr);
		}
		uctmp = N_SBC->clr;
		ncr->n_spurint++;
		uctmp = 0;
	} else {
		switch (ncr->n_laststate) {
		case STATE_SELECTED:
			ncr->n_pmints[PM_SEL]++;
			break;
		case ACTS_MSG_IN:
			ncr->n_pmints[PM_MSGIN]++;
			break;
		case ACTS_MSG_OUT:
			ncr->n_pmints[PM_MSGOUT]++;
			break;
		case ACTS_STATUS:
			ncr->n_pmints[PM_STATUS]++;
			break;
		case ACTS_COMMAND:
			ncr->n_pmints[PM_CMD]++;
			break;
		}
		/*
		 * dismiss cause of interrupt
		 */
		N_SBC->mr &= ~NCR_MR_DMA;
		uctmp = N_SBC->clr;
		uctmp = 1;
	}

	if (uctmp) {
		if (ncr->n_state != ACTS_UNKNOWN) {
			UPDATE_STATE(ACTS_UNKNOWN);
		}
		ncr_phasemanage(ncr);
	}

	/*
	 * Enabling dma also enables SBC interrupts
	 */
	if (ncr->n_type != IS_3_50)
		ENABLE_DMA(ncr);
}

/*
 * Complete reselection
 */

static int
ncr_reselect(ncr)
register struct ncr *ncr;
{
	struct scsi_cmd *sp;
	register s, target, lun;
	register u_char cdr;
	short slot;
	u_char msgin, binary_id = NUM_TO_BIT(ncr->n_id);

	if (ncr->n_ndisc == 0) {
		printf("%s%d: reselection with no disconnected jobs\n",
			CNAME, CNUM);
		return (FAILURE);
	} else if (ncr->n_state != STATE_FREE) {
		printf("%s%d: reselection while not in free state\n",
			CNAME, CNUM);
		return (FAILURE);
	}

	/*
	 * CRITICAL CODE SECTION DON'T TOUCH
	 */

	s = splhigh();
	cdr = N_SBC->clr;	/* clear int */

	/*
	 * get reselecting target scsi id
	 */

	cdr = N_SBC->cdr & ~binary_id;

	/*
	 * make sure there are only 2 scsi id's set
	 */

	for (target = 0; target < 8; target++) {
		if (cdr & (1 << target))
			break;
	}

	N_SBC->ser = 0; 	/* clear (re)sel int */
	cdr &= ~(1 << target);
	if (cdr != 0) {
		(void) splx(s);
		printf("%s%d: reselection w > 2 SCSI ids on the bus\n",
			CNAME, CNUM);
		return (FAILURE);
	}

	/*
	 * Respond to reselection by asserting BSY*
	 */

	N_SBC->icr |= NCR_ICR_BUSY;
	(void) splx(s);

	/*
	 * If reselection ok, target should drop select
	 */


	if (ncr_sbcwait(&CBSR, NCR_CBSR_SEL, NCR_WAIT_COUNT, 0)) {
		printf("%s%d: target didn't drop select on reselection\n",
			CNAME, CNUM);
		return (FAILURE);
	}

	/*
	 * We respond by dropping our assertion of BSY*
	 */

	N_SBC->icr &= ~NCR_ICR_BUSY;
	N_SBC->tcr = TCR_UNSPECIFIED;
	N_SBC->ser = 0;	/* clear int */
	N_SBC->ser = binary_id; /* enable (re)sel int */

	UPDATE_STATE(STATE_RESELECT);

	if (ncr_getphase(ncr) != ACTION_CONTINUE) {
		printf("%s%d: no REQ during reselect\n", CNAME, CNUM);
		return (FAILURE);
	}

	if (ncr->n_state != ACTS_MSG_IN) {
		printf("%s%d: reselect not followed by a MSG IN phase\n",
			CNAME, CNUM);
		return (FAILURE);
	}

	/*
	 * Now pick up identify message byte, and acknowledge it.
	 */

	if (ncr_xfrin_noack(ncr, PHASE_MSG_IN, &msgin)) {
		printf("%s%d: can't get IDENTIFY message on reselect\n",
			CNAME, CNUM);
		return (FAILURE);
	}

	if ((msgin & MSG_IDENTIFY) == 0 ||
	    (msgin & (INI_CAN_DISCON|BAD_IDENTIFY))) {
		printf("%s%d: mangled identify message 0x%x\n", msgin);
		return (FAILURE);
	}
	lun = msgin & (NLUNS_PER_TARGET-1);

	/*
	 * now search for lun to reconnect to
	 */

	lun &= (NLUNS_PER_TARGET-1);
	slot = (target * NLUNS_PER_TARGET) | lun;

	if (ncr->n_slots[slot] == 0) {
		printf("%s%d: Cannot Reconnect Lun %d on Target %d\n",
			CNAME, CNUM, lun, target);
		return (FAILURE);
	}
	ncr->n_cur_slot = slot;
	ncr->n_ndisc--;
	sp = CURRENT_CMD(ncr);

	/*
	 * A reconnect implies a restore pointers operation
	 */

	sp->cmd_cdbp = sp->cmd_pkt.pkt_cdbp;
	sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
	sp->cmd_data = sp->cmd_saved_data;
	sp->cmd_flags &= ~CFLAG_CMDDISC;

	/*
	 * and finally acknowledge the identify message
	 */
	ncr->n_last_msgin = msgin;
	UPDATE_STATE(ACTS_RESELECTING);
	if (ncr_ACKmsg(ncr)) {
		printf("%s%d: unable to acknowledge identify message\n",
			CNAME, CNUM);
		return (FAILURE);
	}
	return (SUCCESS);
}


/*
 * State Management Section
 */


/*
 * manage phases on the SCSI bus
 *
 * We assume (on entry) that we are connected to a
 * target and that CURRENT_CMD(ncr) is valid. We
 * also assume that the phase is unknown. We continue
 * calling one of a set of phase management routines
 * until we either are going to return (to ncrsvc
 * or ncr_ustart()), or have to abort the command,
 * or are going to call ncr_finish() (to complete
 * this command), or are going to turn aroung and
 * call ncr_ustart() again to start another command.
 *
 */

static void
ncr_phasemanage(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register int i, action;
	/*
	 * Important: The order of functions in this array *must* match
	 * the defines in ncrreg.h with respect to the linear sequence
	 * of states from ACTS_UNKNOWN thru ACTS_COMMAND.
	 */
	static int (*itvec[])() = {
		ncr_getphase,
		ncr_sendmsg,
		ncr_recvmsg,
		ncr_recvstatus,
		ncr_sendcmd,
		ncr_recvdata,
		ncr_senddata
	};
	static char *itnames[] = {
		"unknown", "msg out", "msg in", "status",
		"cmd", "data in", "data out"
	};

	action = ACTION_CONTINUE;

	do {
		i = ncr->n_state - ACTS_ITPHASE_BASE;
		PRINTF3("ncr_phasemanage: state = %s\n", itnames[i]);
		action = (*itvec[i])(ncr);
	} while (action == ACTION_CONTINUE);

	switch (action) {
	case ACTION_RETURN:
		break;
	case ACTION_ABORT:
		ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
		break;
	case ACTION_FINISH:
		ncr_finish(ncr);
		break;
	case ACTION_SEARCH:
		ncr_disconnect(ncr);
		break;
	}
}

static int
ncr_getphase(ncr)
register struct ncr *ncr;
{
	register u_char cbsr;
	register int lim, phase;
	u_long usecs = 0;
	static char phasetab[8] = {
		ACTS_DATA_OUT,
		ACTS_DATA_IN,
		ACTS_COMMAND,
		ACTS_STATUS,
		-1,	/* 4 is undefined */
		-1,	/* 5 is undefined */
		ACTS_MSG_OUT,
		ACTS_MSG_IN
	};

	usecs = 10000000;

	for (lim = 0; lim < usecs; lim++) {
		cbsr = N_SBC->cbsr;
		if ((cbsr & NCR_CBSR_BSY) == 0) {
			/*
			 * Unexpected loss of busy!
			 */
			printf("%s%d: target dropped BSY*\n", CNAME, CNUM);
			ncr_printstate(ncr);
			return (ACTION_ABORT);
		} else if ((cbsr & NCR_CBSR_REQ) == 0) {
			/*
			 * If REQ* is not asserted, than the phase bits
			 * are not valid. Delay a few u-secs before checking
			 * again..
			 */
			DELAY(2);
			continue;
		}

		phase = (cbsr >> 2) & 0x7;
		PRINTF3("ncr_getphase: phase= 0x%x\n", phase);

		if (phasetab[phase] == -1) {
			printf("%s%d: garbage phase. CBSR = 0x%b\n",
				CNAME, CNUM, cbsr, cbsr_bits);
			return (ACTION_ABORT);
		}

		/*
		 * Note that if the phase is a data phase, we don't attempt
		 * to match it. We leave that action to the discretion of the
		 * dma routines.
		 */
		if (phase > 1 && (N_SBC->bsr & NCR_BSR_PMTCH) == 0) {
			N_SBC->tcr = phase;
		}
		UPDATE_STATE(phasetab[phase]);
		return (ACTION_CONTINUE);
	}
	printf("%s%d: REQ* never set\n", CNAME, CNUM);
	return (ACTION_ABORT);
}

/*
 * Send command bytes out the SCSI bus. REQ* should be asserted for us
 * to get here, and the phase should already be matched in the tcr register
 * (i.e., ncr_getphase() has done the right thing for us already).
 *
 * If this is a non-interrupting command, we should actually be able to
 * set up for a phase-mismatch interrupt after sending the command.
 *
 */

static int
ncr_sendcmd(ncr)
register struct ncr *ncr;
{
	register u_char junk;
	register struct ncrsbc *sbc = N_SBC;
	register int s, nonintr;
	struct scsi_cmd *sp = CURRENT_CMD(ncr);

	nonintr = (NON_INTR(sp) ? 1: 0);

	/*
	 * We send a single byte of a command out here.
	 * We could probably check to see whether REQ* is
	 * asserted quickly again here, rather than awaiting
	 * it in ncr_getphase() (for non-interrupting commands)
	 * or spotting it in either ncrpoll() or ncrintr().
	 *
	 * XXX: We should check for command overflow here!
	 */

	sbc->odr = *(sp->cmd_cdbp++);	/* load data */
	sbc->icr = NCR_ICR_DATA;	/* enable sbc to send data */

	/*
	 * complete req/ack handshake
	 */

	s = splhigh();
	sbc->icr |= NCR_ICR_ACK;
	sbc->tcr = TCR_UNSPECIFIED;
	(void) splx(s);
	if (!REQ_DROPPED(ncr)) {
		EPRINTF("ncr_sendcmd: REQ not dropped, cbsr=0x%b\n",
			CBSR, cbsr_bits);
		sbc->icr = 0;
		return (ACTION_ABORT);
	}
	if (nonintr == 0) {
		sbc->mr |= NCR_MR_DMA;
		junk = sbc->clr;
	}
	sbc->icr = 0;	/* clear ack */
	sp->cmd_pkt.pkt_state |= STATE_SENT_CMD;
	UPDATE_STATE(ACTS_UNKNOWN);
	if (nonintr) {
		return (ACTION_CONTINUE);
	} else {
		if (ncr->n_type != IS_3_50)
			ENABLE_DMA(ncr);
		return (ACTION_RETURN);
	}
}

/*
 * Send out a message. We assume on entry that phase is matched in the tcr
 * register of the SBC, and that REQ* has been asserted by the target (i.e.,
 * ncr_getphase() has done the right thing).
 *
 * If this is a non-interrupting command, set up to get a phase-mismatch
 * interrupt on the next assertion of REQ* by the target.
 *
 */

static int
ncr_sendmsg(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register struct ncrsbc *sbc = N_SBC;
	u_char junk;
	register s, nonintr;

	nonintr = (NON_INTR(sp) ? 1: 0);

	if (ncr->n_omsglen == 0 || ncr->n_omsgidx >= ncr->n_omsglen) {
		/*
		 * No message to send or previous message exhausted.
		 * Send a NO-OP message instead.
		 */
		printf("%s%d: unexpected message out phase for target %d\n",
			CNAME, CNUM, Tgt(sp));
		ncr->n_omsglen = 1;
		ncr->n_omsgidx = 0;
		ncr->n_cur_msgout[0] = MSG_NOP;
	}

	if (ncr->n_omsgidx == 0) {
		ncr->n_last_msgout = ncr->n_cur_msgout[0];
	}

	/*
	 * load data
	 */

	PRINTF3("ncr_sendmsg: sending msg byte %d = 0x%x of %d len msg\n",
		ncr->n_omsgidx, ncr->n_cur_msgout[ncr->n_omsgidx],
		ncr->n_omsglen);

	sbc->odr = ncr->n_cur_msgout[ncr->n_omsgidx++];
	sbc->icr |= NCR_ICR_DATA;

	if (ncr->n_omsgidx >= ncr->n_omsglen) {
		ncr->n_omsgidx = ncr->n_omsglen = 0;
		sbc->icr &= ~NCR_ICR_ATN;
	}

	/*
	 * complete req/ack handshake
	 */
	s = splhigh();
	sbc->icr |= NCR_ICR_ACK;
	sbc->tcr = TCR_UNSPECIFIED;
	(void) splx(s);
	if (!REQ_DROPPED(ncr)) {
		sbc->icr = 0;
		return (ACTION_ABORT);
	}

	if (nonintr == 0) {
		sbc->mr |= NCR_MR_DMA;
		junk = sbc->clr;
	}

	/*
	 * Deassert ACK*. Note that this uses a mask-equal-not instead of
	 * a straight clear because we may wish to leave ATN* asserted.
	 */

	junk = sbc->icr & (NCR_ICR_ACK|NCR_ICR_DATA|NCR_ICR_ATN);
	junk &= ~(NCR_ICR_ACK|NCR_ICR_DATA);
	sbc->icr = junk;
	UPDATE_STATE(ACTS_UNKNOWN);
	if (nonintr) {
		return (ACTION_CONTINUE);
	} else {
		if (ncr->n_type != IS_3_50)
			ENABLE_DMA(ncr);
		return (ACTION_RETURN);
	}
}

static int
ncr_recvmsg(ncr)
register struct ncr *ncr;
{
	auto u_char msgin;
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	static char *messages[] = {
		"Command Complete",
		"Extended Message",
		"Save Data Pointer",
		"Restore Pointers",
		"Disconnect",
		"Initiator Detected Error",
		"Abort",
		"No-op",
		"Message Reject",
		"Message Parity Error",
		"Linked Command Complete",
		"Linked Command Complete (w/flag)",
		"Bus Device Reset"
	};

	/*
	 * Pick up a message byte from the SCSI bus. Delay giving an ack
	 * until we know if we can handle this message- that way we can
	 * assert the ATN line before the ACK so that the target knows we are
	 * gonna reject this message.
	 */

	if (ncr_xfrin_noack(ncr, PHASE_MSG_IN, &msgin)) {
		return (ACTION_ABORT);
	} else
		ncr->n_last_msgin = msgin;

	if (msgin & MSG_IDENTIFY) {
		/*
		 * We shouldn't be getting an identify message here.
		 */

		printf("%s%d: out of sequence identify message: 0x%x\n",
			CNAME, CNUM, msgin);
		if (ncr_ACKmsg(ncr)) {
			return (ACTION_ABORT);
		}
		UPDATE_STATE(ACTS_UNKNOWN);
		return (ACTION_CONTINUE);
	}

	if (ncr_debug > 2) {
		if (msgin <= MSG_DEVICE_RESET) {
			printf("%s%d: msg=%s\n", CNAME, CNUM, messages[msgin]);
		} else {
			printf("%s%d: msg=0x%x\n", CNAME, CNUM, msgin);
		}
	}

	switch (msgin) {
	case MSG_DISCONNECT:
		/*
		 * Important! Set state *before calling ncr_ACKmsg()
		 * because ncr_ACKmsg() bases some actions on the
		 * new state!
		 */

		UPDATE_STATE(ACTS_CLEARING_DISC);
		if (ncr_ACKmsg(ncr)) {
			return (ACTION_ABORT);
		}

		return (ACTION_SEARCH);

	case MSG_LINK_CMPLT:
	case MSG_LINK_CMPLT_FLAG:
	case MSG_COMMAND_COMPLETE:
	{
		/*
		 * Note well that we *do NOT* ACK the message if it
		 * is a LINKED COMMAND COMPLETE or LINKED COMMAND
		 * COMPLETE (with flag) message. We leave that for
		 * ncr_finish() to do once the target driver has
		 * given us the next command to send. This is so
		 * that the DMA engine can be set up for the new
		 * command prior to ACKing this message.
		 */

		/*
		 * Important! Set state *before calling ncr_ACKmsg()
		 * because ncr_ACKmsg() bases some actions on the
		 * new state!
		 */
		sp->cmd_pkt.pkt_reason = CMD_CMPLT;
		if (msgin == MSG_COMMAND_COMPLETE) {
			UPDATE_STATE(ACTS_CLEARING_DONE);
			if (ncr_ACKmsg(ncr)) {
				return (ACTION_ABORT);
			}
		} else {
			UPDATE_STATE(ACTS_UNKNOWN);
		}
		return (ACTION_FINISH);
	}
	case MSG_SAVE_DATA_PTR:
		sp->cmd_saved_data = (int) sp->cmd_data;
		break;
	case MSG_RESTORE_PTRS:
		sp->cmd_cdbp = sp->cmd_pkt.pkt_cdbp;
		sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
		if (sp->cmd_data != sp->cmd_saved_data) {
			sp->cmd_data = sp->cmd_saved_data;
			sp->cmd_flags |= CFLAG_NEEDSEG;
			PRINTF3("%s%d: %d.%d needs new data segment\n",
				CNAME, CNUM, Tgt(sp), Lun(sp));
		}
		break;
	case MSG_NOP:
		break;
	default:
		if (ncr_NACKmsg(ncr)) {
			return (ACTION_ABORT);
		}
		UPDATE_STATE(ACTS_UNKNOWN);
		return (ACTION_CONTINUE);
	}

	if (ncr_ACKmsg(ncr)) {
		return (ACTION_ABORT);
	}
	UPDATE_STATE(ACTS_UNKNOWN);
	return (ACTION_CONTINUE);
}


/*
 * Enable DMA. If this is a non-interrupting command, await for DMA completion.
 */

static int
ncr_senddata(ncr)
register struct ncr *ncr;
{
	struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register int reqamt;

	reqamt = ncr->n_lastcount;
	PRINTF3("ncr_senddata:\n", reqamt);


	if ((sp->cmd_flags & CFLAG_DMASEND) == 0) {
		printf("%s%d: unwanted data out phase\n", CNAME, CNUM);
		return (ACTION_ABORT);
	}

	if (reqamt == -1) {
		/*
		 * XXXX
		 */
		printf("%s%d: odd byte dma send\n", CNAME, CNUM);
		return (ACTION_ABORT);
	}
	if (reqamt == 0) {
		ncr_dump_datasegs(sp);
		panic("ncr_senddata: no data to send\n");
	}

	ncr_dma_enable(ncr, 0);	/* 0 = send */

	if (NON_INTR(sp)) {
		if (ncr_dma_wait(ncr)) {
			printf("%s%d: wait for dma timed out\n", CNAME, CNUM);
			return (ACTION_ABORT);
		}
		sp->cmd_pkt.pkt_state |= STATE_XFERRED_DATA;
		UPDATE_STATE(ACTS_UNKNOWN);
		return (ACTION_CONTINUE);
	} else {
		return (ACTION_RETURN);
	}

}

/*
 * Enable DMA. If this is a non-interrupting command, await for DMA completion.
 */

static int
ncr_recvdata(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register reqamt;

	if (sp->cmd_flags & CFLAG_DMASEND) {
		printf("%s%d: unwanted data in phase\n", CNAME, CNUM);
		return (ACTION_ABORT);
	}

	reqamt = ncr->n_lastcount;
	if (reqamt == -1) {
		/*
		 * XXXX
		 */
		printf("%s%d: odd byte dma send\n", CNAME, CNUM);
		return (ACTION_ABORT);
	}
	if (reqamt == 0) {
		panic("ncr_rcvdata: no place to put data");
		/* NOTREACHED */
	}


	ncr_dma_enable(ncr, 1);	/* 1 = recv */

	if (NON_INTR(sp)) {
		if (ncr_dma_wait(ncr)) {
			printf("%s%d: wait for dma timed out\n", CNAME, CNUM);
			return (ACTION_ABORT);
		}
		sp->cmd_pkt.pkt_state |= STATE_XFERRED_DATA;
		UPDATE_STATE(ACTS_UNKNOWN);
		return (ACTION_CONTINUE);
	} else {
		return (ACTION_RETURN);
	}
}

static int
ncr_recvstatus(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register int amt, maxsb, action;

	maxsb = (((u_int) sp->cmd_pkt.pkt_scbp) + sp->cmd_scblen) -
		((u_int) sp->cmd_scbp);
	if (maxsb <= 0) {
		printf("%s%d: status overrun\n", CNAME, CNUM);
		/*
		 * XXX: reset bus
		 */
		sp->cmd_pkt.pkt_reason = CMD_STS_OVR;
		return (ACTION_FINISH);
	}

	if (NON_INTR(sp)) {
		amt = ncr_xfrin(ncr, PHASE_STATUS, sp->cmd_scbp, maxsb);
		if (amt <= 0) {
			return (ACTION_ABORT);
		}
		PRINTF3("ncr_recvstatus: status= %x\n", *sp->cmd_scbp);
		sp->cmd_scbp += amt;
		UPDATE_STATE(ACTS_UNKNOWN);
		action = ACTION_CONTINUE;
	} else {
		if (ncr_xfrin_noack(ncr, PHASE_STATUS, sp->cmd_scbp)) {
			return (ACTION_ABORT);
		}
		PRINTF3("ncr_recvstatus: status= %x\n", *sp->cmd_scbp);
		sp->cmd_scbp += amt;
		N_SBC->tcr = TCR_UNSPECIFIED;
		amt = N_SBC->clr;
		N_SBC->icr |= NCR_ICR_ACK;
		if (!REQ_DROPPED(ncr)) {
			return (ACTION_ABORT);
		}
		N_SBC->mr |= NCR_MR_DMA;
		N_SBC->icr = 0;
		if (ncr->n_type != IS_3_50)
			ENABLE_DMA(ncr);
		action = ACTION_RETURN;
	}
	UPDATE_STATE(ACTS_UNKNOWN);
	return (action);
}


/*
 * Utility Functions
 */

/*
 * Perform Arbitration and SCSI selection
 *
 *	input:  (struct ncr) *ncr= pointer to a ncr software structure;
 *	return: (int) SEL_TRUE		= target arb/selected successful;
 *		(int) SEL_ARBFAIL	= Arbitration failed (bus busy)
 *		(int) SEL_FALSE		= selection timed out
 *		(int) SEL_RESEL		= Reselection attempt underway
 */

/* local defines */
#define	WON_ARB	(((sbc->icr & NCR_ICR_LA) == 0) && ((CDR & ~binid) < binid))

static int
ncr_select(ncr)
struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register struct ncrsbc *sbc = ncr->n_sbc;
	register int s, rval, retry;
	u_char uctmp;
	u_char binid = NUM_TO_BIT(ncr->n_id);


	PRINTF3("ncr_select: ready to do ARB/SEL for tgt %d\n", Tgt(sp));

	if (INTPENDING(ncr)) {
		if (ncr_debug > 2) {
			ncr_printstate(ncr);
			printf("ncr_select: preempting\n");
		}
		return (SEL_RESEL);
	}

	if (ncr->n_type != IS_3_50)
		DISABLE_DMA(ncr);

	if (ncr_debug > 1)
		ncr_printstate(ncr);


	/*
	 * Attempt to arbitrate for the SCSI bus.
	 *
	 * It seems that the tcr must be 0 for arbitration to work.
	 */

	sbc->tcr = 0;
	sbc->mr &= ~NCR_MR_ARB;    /* turn off arb */
	sbc->icr = 0;
	sbc->odr = binid;

	UPDATE_STATE(STATE_ARBITRATION);

	rval = SEL_ARBFAIL;

	for (retry = 0; retry < NCR_ARB_RETRIES; retry++) {
		/* wait for scsi bus to become free */
		if (ncr_sbcwait(&CBSR, NCR_CBSR_BSY, NCR_WAIT_COUNT, 0)) {
			PRINTF3("%s%d: scsi bus continuously busy, cbsr= %x\n",
			    CNAME, CNUM, sbc->cbsr);
			break;
		}
		PRINTF3("ncr_select: bus free, current cbsr= %x\n", CBSR);

		/*
		 * If the bus is now FREE, turn on ARBitration
		 */

		sbc->mr |= NCR_MR_ARB;

		/*
		 * wait for ncr to begin arbitration by calling ncr_sbcwait().
		 * If failed due to reselection, turn off ARB, preempt the job
		 * and return SEL_RESEL.
		 */
		if (ncr_sbcwait(&sbc->icr, NCR_ICR_AIP, NCR_ARB_WAIT, 1)) {
			/*
			 * sbc may never begin arbitration
			 * due to a target reselecting us.
			 * (time critical)
			 */
			s = splhigh();
			sbc->mr &= ~NCR_MR_ARB;	/* turn off arb */
			if (RESELECTING(CBSR, CDR, binid)) {
				(void) splx(s);
				rval = SEL_RESEL;
				break;
			}
			(void) splx(s);
			printf ("%s%d: AIP never set, cbsr= 0x%x\n", CNAME,
			    CNUM, CBSR);
		} else {
			/*
			 * check to see if we won arbitration
			 * (time critical)
			 */
			s = splhigh();
			DELAY(ncr_arbitration_delay);
			if (WON_ARB) {
				(void) splx(s);
				rval = SEL_FALSE;
				break;
			}
			(void) splx(s);
		}
		PRINTF3("ncr_select: lost_arb, current cbsr= %x\n", CBSR);
		/*
		 * Lost arbitration. Maybe try again in a nano or two
		 * (time critical)
		 */
		s = splhigh();
		sbc->mr &= ~NCR_MR_ARB;	/* turn off ARB */
		if (RESELECTING(CBSR, CDR, binid)) {
			(void) splx(s);
			rval = SEL_RESEL;
			break;
		}
		(void) splx(s);
	}

	if (rval == SEL_ARBFAIL) {
		/*
		 * FAILED ARBITRATION even with retries.
		 * This shouldn't happen since we (usually)
		 * have the highest priority id on the scsi bus.
		 */

		sbc->icr = 0;
		sbc->mr = 0;
		uctmp = sbc->clr;

		/*
		 * After exhausting retries, and if there
		 * are any outstanding disconnected cmds,
		 * enable reselection attempts from them
		 * and return SEL_FALSE.
		 *
		 */

		PRINTF3("ncr_select: failed arb, target= %x\n", Tgt(sp));
		if (ncr->n_ndisc > 0) {
			sbc->ser = 0;
			sbc->ser = binid;
			if (ncr->n_type != IS_3_50)
				ENABLE_DMA(ncr);
		}
		return (rval);
	} else if (rval == SEL_RESEL) {
		sbc->mr = 0;
		if (ncr->n_type != IS_3_50)
			ENABLE_DMA(ncr);
		return (rval);
	}


	/*
	 * Okay. We got the bus. Now attempt to select the target.
	 */

#ifdef	DO_NOT_DO_THIS_HERE
	/*
	 * Don't mark that we got the bus yet, because
	 * we may in fact have *not* got it (see below)
	 */
	sp->cmd_pkt.pkt_state |= STATE_GOT_BUS;
#endif

	/*
	 * XXX: ???
	 */
	sp->cmd_pkt.pkt_reason = CMD_INCOMPLETE;


	UPDATE_STATE(STATE_SELECTING);

	/*
	 * calculate binary of target_id and host_id
	 * and or them together to send out.
	 */

	uctmp = ((1 << Tgt(sp)) | binid);
	sbc->odr = uctmp;


	uctmp = (NCR_ICR_SEL | NCR_ICR_BUSY | NCR_ICR_DATA);
	if ((scsi_options & SCSI_OPTIONS_DR) &&
	    (sp->cmd_pkt.pkt_flags & FLAG_NODISCON) == 0) {
		ncr->n_cur_msgout[0] = MSG_DR_IDENTIFY | Lun (sp);
		ncr->n_omsglen = 1;
		ncr->n_omsgidx = 0;
		uctmp |= NCR_ICR_ATN;
	}

	sbc->icr = uctmp;		/* start selection */

	sbc->mr &= ~NCR_MR_ARB;		/* turn off arb */

	DELAY(ncr_bus_clear_delay + ncr_bus_settle_delay);

	/*
	 * Drop our assertion of BSY* (left on during arbitration)
	 */

	sbc->icr &= ~NCR_ICR_BUSY;

	DELAY(1);

	/*
	 * Apparently the ncr chip lies about actually
	 * getting the bus, hence we'll check for a reselection
	 * attempt here.
	 */

	for (retry = 0; retry < NCR_SHORT_WAIT; retry ++) {
		/*
		 * If BSY asserted, then the target has selected.
		 * If not, check for a reselection attempt.
		 */
		if (CBSR & NCR_CBSR_BSY) {
			PRINTF3("ncr_select: cbsr= 0x%b\n", CBSR, cbsr_bits);
			break;
		} else if (RESELECTING(CBSR, CDR, binid)) {
			sbc->mr = 0;
			sbc->icr = 0;
			if (ncr->n_type != IS_3_50)
				ENABLE_DMA(ncr);
			PRINTF3("ncr_select: arb_won, but preempt again\n");
			return (SEL_RESEL);
		}
		DELAY(10);
	}


	/*
	 * Say that we got the bus here rather than earlier.
	 */

	sp->cmd_pkt.pkt_state |= STATE_GOT_BUS;

	if (retry >= NCR_SHORT_WAIT) {
		/*
		 * Target failed selection
		 */
		sbc->icr = 0;
		sbc->mr = 0;
		uctmp = sbc->clr;

		/*
		 * if failed to select target, enable disconnect,
		 * if any discon_job pending
		 */

		if (ncr->n_ndisc > 0) {
			sbc->ser = 0;
			sbc->ser = binid;
			if (ncr->n_type != IS_3_50)
				ENABLE_DMA(ncr);
		}
		sp->cmd_pkt.pkt_reason = CMD_INCOMPLETE;
		return (SEL_FALSE);
	}

	/*
	 * Drop SEL* and DATA*
	 */

	sbc->tcr = TCR_UNSPECIFIED;
	if (!NON_INTR(sp)) {
		/*
		 * Time critical
		 */
		s = splhigh();
		if (sp->cmd_flags & CFLAG_DMAVALID) {
			(*ncr->n_dma_setup)(ncr);
		}
		sbc->mr |= NCR_MR_DMA;
		sbc->icr &= ~(NCR_ICR_SEL | NCR_ICR_DATA);
		(void) splx(s);
		if (ncr->n_type != IS_3_50)
			ENABLE_DMA(ncr);
	} else {
		if (sp->cmd_flags & CFLAG_DMAVALID) {
			(*ncr->n_dma_setup)(ncr);
		}
		sbc->icr &= ~(NCR_ICR_SEL | NCR_ICR_DATA);
	}
	/*
	 * Flag that we have selected the target...
	 */

	sp->cmd_pkt.pkt_state |= STATE_GOT_TARGET;

	UPDATE_STATE(STATE_SELECTED);
	PRINTF3("ncr_select: select ok\n");
	return (SEL_TRUE);
}


static void
ncr_preempt(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	UPDATE_STATE(STATE_FREE);
	UPDATE_SLOT(UNDEFINED);
	ncr->n_preempt++;
	if (NON_INTR(sp) && INTPENDING(ncr)) {
		ncrsvc(ncr);
	}
}

static void
ncr_disconnect(ncr)
struct ncr *ncr;
{
	struct scsi_cmd *sp = CURRENT_CMD(ncr);

	sp->cmd_pkt.pkt_statistics |= STAT_DISCON;
	sp->cmd_flags |= CFLAG_CMDDISC;
	ncr->n_ndisc++;
	ncr->n_last_slot = ncr->n_cur_slot;
	ncr->n_cur_slot = UNDEFINED;
	ncr->n_lastcount = 0;
	ncr->n_disconnects++;
	ncr->n_omsglen = ncr->n_omsgidx = 0;
	ncr->n_last_msgin = 0xff;
	UPDATE_STATE(STATE_FREE);
	if (NON_INTR(sp) == 0) {
		short nextslot;
		nextslot = Tgt(sp) + NLUNS_PER_TARGET;
		if (nextslot >= NLUNS_PER_TARGET*NTARGETS)
			nextslot = 0;
		(void) ncr_ustart(ncr, nextslot);
	}
}

/*
 * Programmed I/O Routines
 */


static int
ncr_xfrin(ncr, phase, datap, amt)	/* pick up incoming data byte(s) */
register struct ncr *ncr;
int phase;
register caddr_t datap;
register int amt;
{
	register int i, s;
	register struct ncrsbc *sbc = N_SBC;
	register u_char icr;

	PRINTF3("ncr_xfrin: amt= %x\n", amt);
	/*
	 * Get data from the scsi bus.
	 */
	if ((sbc->cbsr & NCR_CBSR_REQ) == 0) {
		sbc->tcr = TCR_UNSPECIFIED;
		EPRINTF("ncr_xfrin: bad REQ, cbsr= 0x%b\n", CBSR, cbsr_bits);
		return (FAILURE);
	} else if ((sbc->bsr & NCR_BSR_PMTCH) == 0) {
		sbc->tcr = TCR_UNSPECIFIED;
		EPRINTF("ncr_xfrin: bad bus match, bsr= %x\n", sbc->bsr);
		return (FAILURE);
	}

	for (i = 0; i < amt; i++) {
		/* wait for target request */
		if (i && !REQ_ASSERTED(ncr)) {
			sbc->tcr = TCR_UNSPECIFIED;
			return (FAILURE);
		}
		if (i && (sbc->bsr & NCR_BSR_PMTCH) == 0) {
			/*
			 * phase is not matched. Check to
			 * see whether we should match it
			 */

			if ((sbc->cbsr & (0x7<<2)) == phase)
				sbc->tcr = phase >> 2;
			else
				break;
		}
		/* grab data and complete req/ack handshake */
		*datap++ = sbc->cdr;
		s = splhigh();
		sbc->icr |= NCR_ICR_ACK;
		sbc->tcr = TCR_UNSPECIFIED;
		(void) splx(s);
		if (!REQ_DROPPED(ncr)) {
			EPRINTF("ncr_xfrin: REQ not dropped, cbsr=0x%b\n",
				CBSR, cbsr_bits);
			sbc->icr = 0;
			return (FAILURE);
		}
		/* Drop acknowledgement...  */
		sbc->icr = 0;
	}
	sbc->tcr = TCR_UNSPECIFIED;
	sbc->icr = 0;		/* duplicate */
	PRINTF3("ncr_xfrin: picked up %d\n", i);
	return (i);
}

static int
ncr_xfrin_noack(ncr, phase, datap)
register struct ncr *ncr;
int phase;
caddr_t datap;
{
	register u_char indata;

	PRINTF3("ncr_xfrin_noack: phase= %x\n", phase);
	if ((N_SBC->cbsr & NCR_CBSR_REQ) == 0) {
		N_SBC->tcr = TCR_UNSPECIFIED;
		EPRINTF("ncr_xfrin_noack: bad REQ, cbsr=0x%b\n",
			CBSR, cbsr_bits);
		return (FAILURE);
	} else if ((N_SBC->bsr & NCR_BSR_PMTCH) == 0) {
		N_SBC->tcr = TCR_UNSPECIFIED;
		return (FAILURE);
	}

	/*
	 * Get data from the scsi bus, but NO acknowlegment; if current phase
	 * is MESSAGE_IN, clear TCR
	 */
	indata = N_SBC->cdr;
	if (phase == PHASE_MSG_IN) {
		/*
		 * We've picked up the message byte, but not acknowledged it
		 * yet. Perhaps here is the best place to clear the tcr
		 * register in the case that a COMMAND COMPLETE or a
		 * DISCONNECT message was just sent and the target is gonna
		 * clear the SCSI bus just as soon as it is acknowledged. We
		 * have to have tcr 0 to recognize other targets then
		 * attempting to reselect us...
		 */
		N_SBC->tcr = 0;
	}
	*datap = indata;
	PRINTF3("ncr_xfrin_noack: indata= %x\n", indata);
	return (SUCCESS);
}

/*
 * Dma Subroutines
 */

#ifdef	sun4

static int
ncr_cobra_dma_chkerr(ncr, bcr)	/* check for any DMA error */
register struct ncr *ncr;
u_int bcr;
{
	struct scsi_cmd *sp = CURRENT_CMD(ncr);
	u_int csr;

	csr = GET_CSR(ncr);
	PRINTF3("ncr_dma_chkerr: csr= %x, bcr= %x\n", csr, bcr);

	/* check any abormal DMA conditions */
	if (csr & NCR_CSR_DMA_CONFLICT) {
		EPRINTF("ncr_dma_cleanup: dma conflict\n");
		return (FAILURE);
	} else if (csr & NCR_CSR_DMA_BUS_ERR) {
		/*
		 * Note from sw.c:
		 *
		 * Early Cobra units have a faulty gate array. It can cause an
		 * illegal memory access if full page DMA is being used.  For
		 * less than a full page, no problem.  This problem typically
		 * shows up when dumping core (in polled mode) where the last
		 * page of DVMA was being used.
		 *
		 * What this means is that if you camp on the dma gate array
		 * csr in polled mode, the ncr chip may attempt to prefetch
		 * across a page boundary into an invalid page, causing a
		 * spurious DMA error.
		 */
		if (!((ncr->n_type == IS_COBRA) && (bcr > 2) && NON_INTR(sp))) {
			EPRINTF("ncr_dma_chkerr: dma bus error\n");
			return (FAILURE);
		}
	}
	return (SUCCESS);
}

static int
ncr_cobra_dma_recv(ncr)
register struct ncr *ncr;
{
	register u_long addr, offset, bpr;

	/*
	 * if partial bytes left on the dma longword transfers, manually take
	 * care of this and return back how many bytes moved
	 */
	/*
	 * Grabs last few bytes which may not have been dma'd. Worst case is
	 * when longword dma transfers are being done and there are 3 bytes
	 * leftover.
	 *
	 * Note: limiting dma address to 20 bits (1 mb).
	 *
	 */
	addr = GET_DMA_ADDR(ncr);
	addr &= 0xfffff;
	offset = addr & ~3;
	bpr = GET_BPR(ncr);
	switch (addr & 0x3) {
	case 3:
		if (ncr_flushbyte(ncr, offset+2, (u_char) ((bpr>>8)&0xff)))
			return (FAILURE);
		/* FALLTHROUGH */
	case 2:
		if (ncr_flushbyte(ncr, offset+1, (u_char) ((bpr>>16)&0xff)))
			return (FAILURE);

		/* FALLTHROUGH */
	case 1:
		if (ncr_flushbyte(ncr, offset, (u_char) (bpr>>24)))
			return (FAILURE);
		break;
	default:
		break;
	}
	ncr->n_lastbcr += (addr & 0x3);
	return (SUCCESS);
}

static int
ncr_cobra_dma_cleanup(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register u_long amt, reqamt;
	register u_char junk;

	DISABLE_DMA(ncr);

	amt = GET_DMA_ADDR(ncr);
	reqamt = ncr->n_lastcount;

	/*
	 * Now try and figure out how much actually transferred
	 */

	ncr->n_lastbcr = reqamt - ((amt & 0xfffff) -
	    (unsigned int) sp->cmd_data & 0xfffff);
	PRINTF3("ncr_dma_cleanup: count= %x, data= %x, amt= %x, bcr= %x\n",
		reqamt, sp->cmd_data, amt, ncr->n_lastbcr);

	if ((ncr->n_lastbcr != reqamt) && (!(N_SBC->tcr & NCR_TCR_LAST)) &&
	    (sp->cmd_flags & CFLAG_DMASEND)) {
		ncr->n_lastbcr++;
	}

	/*
	 * Now check for dma related errors
	 */

	if (ncr_cobra_dma_chkerr(ncr, ncr->n_lastbcr)) {
		sp->cmd_pkt.pkt_reason = CMD_DMA_DERR;
		return (FAILURE);
	}

	/*
	 * okay, now figure out an adjustment for bytes
	 * left over in the or a byte pack register.
	 */

	if ((sp->cmd_flags & CFLAG_DMASEND) == 0) {
		if (ncr_cobra_dma_recv(ncr))
			return (FAILURE);
	}

	if (ncr->n_lastbcr < 0) {
		EPRINTF("ncr_dma_cleanup: dma overrun by %d bytes\n",
		    -ncr->n_lastbcr);
		ncr->n_lastbcr = 0;
	}

	/*
	 * Shut off dma engine
	 */

	SET_DMA_ADDR(ncr, 0);
	SET_DMA_COUNT(ncr, 0);

	/*
	 * And return result of common cleanup routine
	 */

	return (ncr_dma_cleanup(ncr));
}




#endif	sun4
#ifdef	sun3

static void
ncr_ob_dma_setup(ncr)
struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register int reqamt;
	u_int dmaaddr;

	if (sp->cmd_flags & CFLAG_NEEDSEG) {
		panic("ncr_ob_dma_setup: need seg\n");
		/*NOTREACHED*/
	}
	if ((sp->cmd_flags & CFLAG_DMAVALID) == 0) {
		panic("ncr_ob_dma_setup: need seg\n");
		/*NOTREACHED*/
	}
	if ((u_long) sp->cmd_data & 0x1) {
		panic("dma not on word boundary");
		/*NOTREACHED*/
	}

	ncr->n_lastcount = reqamt = scsi_chkdma(sp, NCR_MAX_DMACOUNT);

	if (reqamt == 0)
		return;

	PRINTF3("ncr_ob_dma_setup: addr= %x, reqamt= %x\n",
		sp->cmd_data, reqamt);

	/* reset fifo */
	SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_FIFO_RES));
	SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_FIFO_RES));

	if (sp->cmd_flags & CFLAG_DMASEND) {
		SET_CSR(ncr, (GET_CSR(ncr)) | NCR_CSR_SEND);
	} else {
		SET_CSR(ncr, (GET_CSR(ncr)) & ~NCR_CSR_SEND);
	}

	/* Set bcr */
	SET_BCR(ncr, reqamt);

	/* reset udc */
	N_DMA->udc_raddr = UDC_ADR_COMMAND;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = UDC_CMD_RESET;
	DELAY(NCR_UDC_WAIT);

	/* set up udc dma information */
	ncr->n_lastdma = dmaaddr = sp->cmd_data;
	if (dmaaddr < Dmabase)
		dmaaddr |= Dmabase;

	ncr->n_udc->haddr = ((dmaaddr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	ncr->n_udc->laddr = dmaaddr & 0xffff;
	ncr->n_udc->hcmr = UDC_CMR_HIGH;
	ncr->n_udc->count = reqamt / 2; /* #bytes -> #words */

	if (sp->cmd_flags & CFLAG_DMASEND) {
		ncr->n_udc->rsel = UDC_RSEL_SEND;
		ncr->n_udc->lcmr = UDC_CMR_LSEND;
		if (reqamt & 1)
			ncr->n_udc->count++;
	} else {
		ncr->n_udc->rsel = UDC_RSEL_RECV;
		ncr->n_udc->lcmr = UDC_CMR_LRECV;
	}

	/* initialize udc chain address register */
	N_DMA->udc_raddr = UDC_ADR_CAR_HIGH;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = (((int)ncr->n_udc) & 0xff0000) >> 8;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_raddr = UDC_ADR_CAR_LOW;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = ((int)ncr->n_udc) & 0xffff;
	DELAY(NCR_UDC_WAIT);

	/* initialize udc master mode register */
	N_DMA->udc_raddr = UDC_ADR_MODE;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = UDC_MODE;
	DELAY(NCR_UDC_WAIT);

	/* issue channel interrupt enable command, in case of error, to udc */
	N_DMA->udc_raddr = UDC_ADR_COMMAND;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = UDC_CMD_CIE;
	DELAY(NCR_UDC_WAIT);
}

/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */

static int
ncr_ob_dma_cleanup(ncr)
struct ncr *ncr;
{
	struct scsi_cmd *sp = CURRENT_CMD(ncr);
	u_char junk;
	int amt, csr;

	/*
	 * okay, now figure out an adjustment for bytes left over in a fifo
	 * or a byte pack register....
	 */
	if ((sp->cmd_flags & CFLAG_DMASEND) == 0) {
		if (ncr_ob_dma_recv(ncr))
			return (FAILURE);
	}

	/*
	 * disable dma controller
	 */

	N_DMA->udc_raddr = UDC_ADR_COMMAND;
	DELAY(NCR_UDC_WAIT);
	N_DMA->udc_rdata = UDC_CMD_RESET;
	DELAY(NCR_UDC_WAIT);
	N_DMA->bcr = 0;
	SET_CSR(ncr, (GET_CSR(ncr)) & ~NCR_CSR_SEND);

	/*
	 * reset fifo
	 */

	SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_FIFO_RES));
	SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_FIFO_RES));

	/*
	 * And return result of common cleanup routine
	 */

	return (ncr_dma_cleanup(ncr));
}

/*
 * Handle special dma receive situations, e.g. an odd number of bytes in
 * a dma transfer.
 */

static int
ncr_ob_dma_recv(ncr)
register struct ncr *ncr;
{
	register int bcr, offset;
	u_char byte;

	/*
	 * handle the onboard scsi situations
	 */

	offset = ncr->n_lastdma + (ncr->n_lastcount - ncr->n_lastbcr);
	bcr = N_DMA->bcr;

	N_DMA->udc_raddr = UDC_ADR_COUNT;
	DELAY(NCR_UDC_WAIT);

	/*
	 * wait for the fifo to empty
	 */

	if (ncr_csrwait(ncr, NCR_CSR_FIFO_EMPTY, NCR_WAIT_COUNT*3, 1)) {
		printf("%s%d: fifo never drained\n", CNAME, CNUM);
		return (FAILURE);
	}

	/*
	 * Didn't transfer any data.  "Just say no" and leave,
	 * rather than erroneously executing left over byte code.
	 * The bcr + 1 above wards against 5380 prefetch.
	 */

	if (bcr == ncr->n_lastcount) {
		return (SUCCESS);
	} else if ((bcr + 1) == ncr->n_lastcount) {
		ncr->n_lastbcr = ncr->n_lastcount;
	}

	/*
	 * For either of the following cases, it appears that the
	 * n_lastbcr count latched up in ncrsvc is, in fact, correct.
	 * Therefore, we don't need to adjust it here.
	 */

	/* handle odd byte */
	if ((ncr->n_lastcount - bcr) & 1) {
		byte = ((N_DMA->fifo_data & 0xff00) >> 8);
		if (ncr_flushbyte(ncr, offset - 1, byte)) {
			return (FAILURE);
		}
		PRINTF1("%s%d: odd byte recv: lastcount %d bcr %d\n",
			CNAME, CNUM, ncr->n_lastcount, ncr->n_lastbcr);
	} else if ((N_DMA->udc_rdata * 2) - bcr == 2) {
		/*
		 * The udc may not dma the last word from the fifo_data
		 * register into memory due to how the hardware turns
		 * off the udc at the end of the dma operation.
		 */
		byte = ((N_DMA->fifo_data & 0xff00) >> 8);
		if (ncr_flushbyte(ncr, offset - 2, byte)) {
			return (FAILURE);
		}
		byte = (N_DMA->fifo_data & 0xff);
		if (ncr_flushbyte(ncr, offset - 1, byte)) {
			return (FAILURE);
		}
		PRINTF1("%s%d: last word: lastcount %d bcr %d\n",
			CNAME, CNUM, ncr->n_lastcount, ncr->n_lastbcr);
	}
	return (SUCCESS);
}

#endif	sun3



static int
ncr_vme_dma_recv(ncr)
register struct ncr *ncr;
{
	register amt, csr;
	register u_short bprmsw, bprlsw;
	u_long offset;

	/*
	 * Grabs last few bytes which may not have been dma'd.  Worst
	 * case is when longword dma transfers are being done and there
	 * are 3 bytes leftover.  If BPCON bit is set then longword dma
	 * was being done, otherwise word dma was being done.
	 */

	csr = GET_CSR(ncr);
	if ((amt = NCR_CSR_LOB_CNT(csr)) == 0) {
		return (SUCCESS);
	}
	offset = ncr->n_lastdma + (ncr->n_lastcount - ncr->n_lastbcr);

	PRINTF3("%s%d: receive flushing %d bytes to offset %d in xfer of %d\n",
		CNAME, CNUM, amt, offset, ncr->n_lastcount);

	/*
	 * It *may* be that the order that
	 * the byte pack register is read
	 * is significant.
	 *
	 */

	bprmsw = N_CTL->bpr.msw;
	bprlsw = N_CTL->bpr.lsw;
	if (csr & NCR_CSR_BPCON) {
		return (ncr_flushbyte(ncr, offset-1, (u_char) hibyte(bprlsw)));
	}

	switch (amt) {
	case 3:
		if (ncr_flushbyte(ncr, offset-3, (u_char) hibyte(bprmsw)))
			return (FAILURE);
		if (ncr_flushbyte(ncr, offset-2, (u_char) lobyte(bprmsw)))
			return (FAILURE);
		if (ncr_flushbyte(ncr, offset-1, (u_char) hibyte(bprlsw)))
			return (FAILURE);
		break;
	case 2:
		if (ncr_flushbyte(ncr, offset-2, (u_char) hibyte(bprmsw)))
			return (FAILURE);
		if (ncr_flushbyte(ncr, offset-1, (u_char) lobyte(bprmsw)))
			return (FAILURE);
		break;
	case 1:
		if (ncr_flushbyte(ncr, offset-1, (u_char) hibyte(bprmsw)))
			return (FAILURE);
		break;
	}
	return (SUCCESS);
}

static int
ncr_vme_dma_cleanup(ncr)
register struct ncr *ncr;
{
	struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register u_int bcr, reqamt;

	DISABLE_DMA(ncr);

	/*
	 * Now try and figure out how much actually transferred
	 */

	bcr = GET_BCR(ncr);
	reqamt = ncr->n_lastcount;

	/*
	 * bcr does not reflect how many bytes were actually
	 * transferred for VME.
	 *
	 * SCSI-3 VME interface is a little funny on writes:
	 * if we have a disconnect, the dma has overshot by
	 * one byte and needs to be incremented.  This is
	 * true if we have not transferred either all data
	 * or no data.
	 */

	if ((sp->cmd_flags & CFLAG_DMASEND) && bcr != reqamt && bcr) {
		if (ncr->n_lastbcr != 0)
			bcr = ncr->n_lastbcr + 1;
		else
			bcr++;
	} else if ((sp->cmd_flags & CFLAG_DMASEND) == 0) {
		bcr = ncr->n_lastbcr;
		if (ncr_vme_dma_recv(ncr)) {
			return (FAILURE);
		}
	}

	/*
	 * rewrite last bcr count- it might have been changed by circumstances.
	 */

	ncr->n_lastbcr = bcr;

	/*
	 * Shut off dma engine
	 */

	SET_DMA_ADDR(ncr, 0);
	SET_DMA_COUNT(ncr, 0);
	SET_BCR(ncr, 0);

	SET_CSR(ncr, (GET_CSR(ncr)) & ~NCR_CSR_SEND);

	/*
	 * reset fifo
	 */

	SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_FIFO_RES));
	SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_FIFO_RES));

	/*
	 * And return result of common cleanup routine
	 */

	return (ncr_dma_cleanup(ncr));
}

static int
ncr_3e_dma_cleanup(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);

	/*
	 * Now try and figure out how much actually transferred
	 */

	ncr->n_lastbcr = (u_long) (u_short) GET_DMA_COUNT(ncr);
	if (sp->cmd_flags & CFLAG_DMASEND) {
		/*
		 * check for a pre-fetch botch
		 * XXX: I'm not sure I believe this. It was lifted from se.c.
		 */
		if (ncr->n_lastbcr == 0xffff)
			ncr->n_lastbcr = 0;
		else
			ncr->n_lastbcr++;
	}

	/*
	 * Shut off dma engine
	 */

	SET_DMA_ADDR(ncr, 0);
	SET_DMA_COUNT(ncr, 0);

	/*
	 * Copy out data from onboard dma buffer (if receive case)
	 */

	if ((sp->cmd_flags & CFLAG_DMASEND) == 0) {
		if (ncr_3e_storedata(ncr)) {
			return (FAILURE);
		}
	}

	/*
	 * And return result of common cleanup routine
	 */

	return (ncr_dma_cleanup(ncr));
}



/*
 * Cleanup operations, where the dma engine doesn't do it all.
 *
 * XXXXXXXXXXXXXXXXXXXXXXXXXXX: FIX ME :XXXXXXXXXXXXXXXXXXXXXXXXXXXX
 * XXXXXXXXX						XXXXXXXXXXXX
 * XXXXXXXXX		All These Routines Need to	XXXXXXXXXXXX
 * XXXXXXXXX		be rewritten to *not* refer	XXXXXXXXXXXX
 * XXXXXXXXX		to DVMA. This must happen!	XXXXXXXXXXXX
 * XXXXXXXXX						XXXXXXXXXXXX
 * XXXXXXXXXXXXXXXXXXXXXXXXXXX: FIX ME :XXXXXXXXXXXXXXXXXXXXXXXXXXXX
 *
 */

static int
ncr_flushbyte (ncr, offset, byte)
struct ncr *ncr;
int offset;
u_char byte;
{
	u_char *mapaddr;
	u_int pv;

	if (MBI_MR(offset) < dvmasize) {
		DVMA[offset] = byte;
		return (SUCCESS);
	}
#ifdef	sun3x
	pv = btop (VME24D16_BASE + (offset & VME24D16_MASK));
#else	sun3x
	pv = PGT_VME_D16 | VME24_BASE | btop(offset & VME24_MASK);
#endif	sun3x
	mapaddr = (u_char *) ((u_long) kmxtob(ncr->n_kment) |
				(u_long) MBI_OFFSET(offset));
	segkmem_mapin(&kseg, (addr_t) (((int)mapaddr) & PAGEMASK),
		(u_int) mmu_ptob(1), PROT_READ | PROT_WRITE, pv, 0);
	*mapaddr = byte;
	segkmem_mapout(&kseg,
	    (addr_t) (((int)mapaddr) & PAGEMASK), (u_int) mmu_ptob(1));
	return (SUCCESS);
}

static int
ncr_3e_fetchdata(ncr)
struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	if (MBI_MR(sp->cmd_data) < dvmasize) {
		bcopy(sp->cmd_data + (u_long) DVMA, ncr->n_dmabuf,
		    ncr->n_lastcount);
	} else {
		panic("3/E dma from VME board");
		/*NOTREACHED*/
	}
	return (SUCCESS);
}

static int
ncr_3e_storedata(ncr)
struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	if (MBI_MR(sp->cmd_data) < dvmasize) {
		bcopy(ncr->n_dmabuf, sp->cmd_data + (u_long) DVMA,
		    ncr->n_lastcount);
	} else {
		panic("3/E dma to VME board");
		/*NOTREACHED*/
	}
	return (SUCCESS);
}


/*
 * Common routine to enable the SBC for a dma operation.
 */

static void
ncr_dma_enable(ncr, direction)
register struct ncr *ncr;
int direction;
{
	register s;
	u_char junk;

	s = splhigh();	/* (possibly) time critical */
	if (ncr->n_type == IS_3_50) {
		/*
		 * issue start chain command to udc
		 */
		N_DMA->udc_rdata = UDC_CMD_STRT_CHN;
	} else if (ncr->n_type == IS_SCSI3) {

		/*
		 * Stuff count registers
		 */

		SET_DMA_COUNT(ncr, ncr->n_lastcount);
		SET_BCR(ncr, ncr->n_lastcount);
	}

	if (direction) {	/* 1 = recv */
		N_SBC->tcr = TCR_DATA_IN;
		junk = N_SBC->clr;
		N_SBC->mr |= NCR_MR_DMA;
		N_SBC->ircv = 0;
	} else {		/* 0 = send */
		N_SBC->tcr = TCR_DATA_OUT;
		junk = N_SBC->clr;	/* clear intr */
		N_SBC->icr = NCR_ICR_DATA;
		N_SBC->mr |= NCR_MR_DMA;
		N_SBC->send = 0;
	}
	(void) splx(s);
	if (ncr->n_type == IS_3_50) {
		DELAY(NCR_UDC_WAIT);
	} else {
		ENABLE_DMA(ncr);
	}
}

/*
 * Common dma setup routine for all except 3/50, 3/60 architectures
 */

static void
ncr_dma_setup(ncr)
struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register int reqamt, align;
	u_int dmaaddr;
	u_char type = ncr->n_type;

	if (sp->cmd_flags & CFLAG_NEEDSEG) {
		panic("ncr_dma_setup: need seg\n");
		/*NOTREACHED*/
	}
	if ((sp->cmd_flags & CFLAG_DMAVALID) == 0) {
		panic("ncr_dma_setup: need seg\n");
		/*NOTREACHED*/
	}

	/*
	 * FIX ME: have to handle misaligned transfers
	 */
	if (type == IS_COBRA) {
		align = 0x3;
	} else
		align = 0x1;

	if (sp->cmd_data & align) {
		ncr->n_lastcount = -1;
		return;
		/*NOTREACHED*/
	}

	ncr->n_lastcount = reqamt = scsi_chkdma(sp, NCR_MAX_DMACOUNT);

	if (reqamt == 0)
		return;

	if (sp->cmd_flags & CFLAG_DMASEND) {
		if (type == IS_3E) {
			if (ncr_3e_fetchdata(ncr)) {
				panic("preload of data fails!");
				/*NOTREACHED*/
			}
		}
		SET_CSR(ncr, (GET_CSR(ncr)) | NCR_CSR_SEND);
	} else {
		SET_CSR(ncr, (GET_CSR(ncr)) & ~NCR_CSR_SEND);
	}

	if (type != IS_COBRA) {
		/*
		 * reset fifo
		 */
		SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_FIFO_RES));
		SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_FIFO_RES));
	}

	if (type != IS_3E) {
		ncr->n_lastdma = dmaaddr = sp->cmd_data;
		if (Dmabase && dmaaddr < Dmabase) {
			dmaaddr = sp->cmd_data | Dmabase;
		}
	} else {
		ncr->n_lastdma = dmaaddr = 0;
	}

	if (type == IS_SCSI3) {
		if (dmaaddr & 0x2) {
			SET_CSR(ncr, ((GET_CSR(ncr)) | NCR_CSR_BPCON));
		} else {
			SET_CSR(ncr, ((GET_CSR(ncr)) & ~NCR_CSR_BPCON));
		}
	}

	SET_DMA_ADDR(ncr, dmaaddr);

	if (type == IS_SCSI3) {
		/*
		 * The SCSI-3 board dma engine should be set to zero
		 * to keep it from starting up when we don't want it to.
		 */
		SET_DMA_COUNT(ncr, 0);
	} else {
		SET_DMA_COUNT(ncr, reqamt);
	}

	PRINTF2("ncr_dma_setup: dma_addr= %x, dma_count= %x\n",
		dmaaddr, reqamt);
}


/*
 * Common DMA cleanup for all architectures
 */
static int
ncr_dma_cleanup(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	register u_long amt, reqamt;
	register u_char junk;

	reqamt = ncr->n_lastcount;

	/*
	 * Adjust dma pointer and xfr counter to reflect
	 * the amount of data actually transferred.
	 */

	amt = reqamt - ncr->n_lastbcr;
	sp->cmd_data += amt;
	sp->cmd_cursubseg->d_count += amt;

	/*
	 * Acknowledge the interrupt, unlatch the SBC.
	 */

	junk = N_SBC->clr;
	N_SBC->tcr = TCR_UNSPECIFIED;
	N_SBC->mr = 0;

	/*
	 * Call the dma setup routine right away in hopes of beating
	 * the target to the next phase (in case it is a data phase)
	 */
	(*ncr->n_dma_setup)(ncr);

	/*
	 * Clear any pending interrupt
	 */

	if (GET_CSR(ncr) & NCR_CSR_NCR_IP) {
		junk = N_SBC->clr;
	}

	if (ncr_debug > 2 ||
	    (ncr_debug && (reqamt > 0x200 && (amt & 0x1ff)))) {
		printf("%s%d: end of dma: lastcnt %d did %d\n",
			CNAME, CNUM, reqamt, amt);
	}
	sp->cmd_pkt.pkt_state |= STATE_XFERRED_DATA;
	UPDATE_STATE(ACTS_UNKNOWN);
	return (SUCCESS);
}

/*
 * Common dma wait routine for all architectures
 */

#define	WCOND (NCR_CSR_NCR_IP | NCR_CSR_DMA_CONFLICT | NCR_CSR_DMA_BUS_ERR)

static int
ncr_dma_wait(ncr)		/* wait for dma completion or error */
register struct ncr *ncr;
{

	if (ncr->n_type == IS_3_50) {
		/*
		 * wait for indication of dma inactivity
		 */
		if (ncr_csrwait(ncr, NCR_CSR_DMA_ACTIVE, NCR_WAIT_COUNT, 0)) {
			ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
			return (FAILURE);
		}
	} else {
		DELAY(10000);
	}

	/*
	 * wait for indication of dma completion
	 */
	if (ncr_csrwait(ncr, WCOND, NCR_WAIT_COUNT, 1)) {
		printf("%s%d: polled dma never completed\n", CNAME, CNUM);
		ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
		return (FAILURE);
	}
	return ((*ncr->n_dma_cleanup)(ncr));
}

/*
 * Miscellaneous subroutines
 */

static int
ncr_NACKmsg(ncr)
register struct ncr *ncr;
{
	/*
	 * Attempt to send out a reject message along with ATN
	 */
	ncr->n_cur_msgout[0] = MSG_REJECT;
	ncr->n_omsglen = 1;
	ncr->n_omsgidx = 0;
	N_SBC->icr |= NCR_ICR_ATN;
	return (ncr_ACKmsg(ncr));
}


static int
ncr_ACKmsg(ncr)
register struct ncr *ncr;
{
	register s;
	register struct ncrsbc *sbc = N_SBC;
	register u_char t = 0;
	register u_char junk;

	switch (ncr->n_state) {
	case ACTS_CLEARING_DONE:
	case ACTS_CLEARING_DISC:
		t = 1;
		break;
	case ACTS_SPANNING:
	case ACTS_RESELECTING:
		if (CURRENT_CMD(ncr)->cmd_flags & CFLAG_DMAVALID) {
			(*ncr->n_dma_setup)(ncr);
		}
		break;
	default:
		break;
	}
	/*
	 * XXXX: FIX ME else if (last message was modify data pointer...)
	 */

	s = splhigh();
	sbc->icr |= NCR_ICR_ACK;
	sbc->tcr = TCR_UNSPECIFIED;
	sbc->mr &= ~NCR_MR_DMA;
	sbc->ser = 0;
	sbc->ser = NUM_TO_BIT(ncr->n_id);
	junk = sbc->clr;	/* clear int */
	(void) splx(s);

	if (!REQ_DROPPED(ncr)) {
		sbc->icr = 0;
		return (FAILURE);
	}
	s = splhigh();		/* time critical */
	sbc->icr &= ~NCR_ICR_ACK;	/* drop ack */
	if (t) {
		/*
		 * If the state indicates that the target is
		 * clearing the bus, and we have the possibillity
		 * of an interrupt coming in, enable interrupts pronto.
		 * Also, clear the dma engine (in case it wants to
		 * take off for some reason).
		 */
#ifdef	sun3
		if (ncr->n_type == IS_3_50) {
			N_DMA->bcr =  0;
		} else {
			SET_DMA_COUNT(ncr, 0);
			SET_DMA_ADDR(ncr, 0);
			ENABLE_DMA(ncr);
		}
#else	sun3
		SET_DMA_ADDR(ncr, 0);
		SET_DMA_COUNT(ncr, 0);
		ENABLE_DMA(ncr);
#endif	sun3
		/*
		 * WE NEED TO WAIT FOR THE TARGET TO ACTUALLY DROP BSY*.
		 * OTHERWISE, IF WE ATTEMPT TO SELECT SOMEONE ELSE, WE'LL
		 * SIT IN ncr_select() AND AWAIT BUSY TO GO AWAY THERE
		 */
	}
	return (SUCCESS);
}

/*
 * wait for a bit to be set or cleared in the NCR 5380
 */

static int
ncr_sbcwait(reg, cond, wait_cnt, set)
register u_char *reg;
register cond, wait_cnt, set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < wait_cnt; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (SUCCESS);
		}
		if ((set == 0) && !(regval & cond)) {
			return (SUCCESS);
		}
		DELAY(10);
	}
	return (FAILURE);
}

static int
ncr_csrwait(ncr, cond, wait_cnt, set)
register struct ncr *ncr;
register int cond, wait_cnt, set;
{
	register int i;
	register u_int regval;

	/*
	 * Wait for a condition to be (de)asserted in the interface csr
	 */
	for (i = 0; i < wait_cnt; i++) {
		regval = GET_CSR(ncr);
		if ((set == 1) && (regval & cond)) {
			return (SUCCESS);
		}
		if ((set == 0) && !(regval & cond)) {
			return (SUCCESS);
		}
		DELAY(10);
	}
	return (FAILURE);
}



static int
ncr_watch()
{
	register struct scsi_cmd *sp;
	register struct ncr *ncr;
	register int s, slot;

	for (ncr = &ncr_softc[0]; ncr < &ncr_softc[NNCR]; ncr++) {
		if (ncrctlr[ncr-ncr_softc] == 0)
			continue;
		s = splr(ncr->n_tran.tran_spl);
		if (ncr->n_ncmds == 0) {
			(void) splx(s);
			continue;
		}
		/* if current state is not FREE */
		if (ncr->n_state != STATE_FREE) {
			sp = CURRENT_CMD(ncr);
			/*
			 * if a valid job and its "watch_flag" is
			 * set, look at timeout flag, if dec to zero,
			 * then error, unless an existing interrupt
			 * pending, if so, break out; if not yet
			 * TIMEOUT, continue checking on next tick;
			 */
			if (sp && (sp->cmd_flags & CFLAG_WATCH)) {
				if (sp->cmd_timeout == 0) {
					if (INTPENDING(ncr)) {
						sp->cmd_timeout++;
						(void) splx(s);
						break;
					}
					ncr_curcmd_timeout(ncr);
					(void) splx(s);
					continue;
				} else {
					sp->cmd_timeout -= 1;
				}
			}
		}
		if (ncr->n_ndisc == 0) {
			(void) splx(s);
			continue;
		}

		for (slot = 0; slot < NTARGETS * NLUNS_PER_TARGET; slot++) {
			if ((sp = ncr->n_slots[slot]) == 0) {
				continue;
			}
			/*
			 * if this job was not disconnected nor
			 * watching, cont; otherwise, checked timeout
			 * just exhaused, if so, ERROR, except an
			 * interrupt pending, break out; If timed
			 * out, call "upper level completion
			 * routines; if still time left, go on
			 */
			if ((sp->cmd_flags & CFLAG_CMDDISC | CFLAG_WATCH) !=
			    (CFLAG_CMDDISC | CFLAG_WATCH)) {
				continue;
			} else if (sp->cmd_timeout == 0) {
				if (INTPENDING(ncr)) {
					sp->cmd_timeout++;
					/*
					 * A pending interrupt defers
					 * the sentence of death.
					 */
					break;
				}
				sp->cmd_pkt.pkt_reason = CMD_TIMEOUT;
				ncr->n_slots[slot] = 0;
				(*sp->cmd_pkt.pkt_comp) (sp);
			} else {
				sp->cmd_timeout -= 1;
			}
		}
		(void) splx(s);
	}
	timeout(ncr_watch, (caddr_t) 0, hz);
}

static void
ncr_curcmd_timeout(ncr)
register struct ncr *ncr;
{
	register struct scsi_cmd *sp = CURRENT_CMD(ncr);
	printf("%s%d: current cmd timeout target %d\n", CNAME, CNUM, Tgt(sp));
	if (ncr_debug) {
		printf("State 0x%x Laststate 0x%x pkt_state 0x%x\n",
			ncr->n_state, ncr->n_laststate, sp->cmd_pkt.pkt_state);
		printf("Last dma 0x%x Last Count 0x%x\n",
			ncr->n_lastdma, ncr->n_lastcount);
		ncr_dump_datasegs(sp);
	}
	ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
}


/*
 * Abort routines
 */

static void
ncr_internal_abort(ncr)
register struct ncr *ncr;
{
	ncr_do_abort(ncr, NCR_RESET_ALL, RESET_NOMSG);
}

static void
ncr_do_abort(ncr, action, msg)
register struct ncr *ncr;
int action;
int msg;
{
	register struct scsi_cmd *sp;
	register short  slot, start_slot;
	auto struct scsi_cmd *tslots[NTARGETS * NLUNS_PER_TARGET];

	PRINTF3("ncr_do_abort:\n");
	if ((start_slot = ncr->n_cur_slot) == UNDEFINED)
		start_slot = 0;

	/* temporary store all the current scsi commands per all targets */
	bcopy((caddr_t) ncr->n_slots, (caddr_t) tslots,
		(sizeof (struct scsi_cmd *)) * NTARGETS * NLUNS_PER_TARGET);

	/* call "ncr_internal_reset()" to clear the host-adapter */
	ncr_internal_reset(ncr, action, msg);

	/* Set state to ABORTING */
	ncr->n_state = ACTS_ABORTING;

	/*
	 * start from current slot and around all slots, call "upper level
	 * completion routines to clear all jobs
	 */
	slot = start_slot;
	do {
		if (sp = tslots[slot]) {
			sp->cmd_pkt.pkt_reason = CMD_RESET;
			sp->cmd_pkt.pkt_resid = sp->cmd_dmacount;
			(*sp->cmd_pkt.pkt_comp) (sp);
		}
		slot = NEXTSLOT(slot);
	} while (slot != start_slot);

	/* close the state back to free */
	ncr->n_state = STATE_FREE;

	/* and if any new command queued, call "ncr_ustart()" to start it */
	if (ncr->n_ncmds) {
		(void) ncr_ustart(ncr, start_slot);
	}
}

/*
 * Hardware and Software internal reset routines
 */

static void
ncr_internal_reset(ncr, reset_action, msg_enable)
register struct ncr *ncr;
int reset_action;	/* how much to reset */
int msg_enable;		/* for printing error infos */
{
	register int i;

	if (ncr_debug || msg_enable) {
		ncr_printstate(ncr);
	}

	ncr_hw_reset(ncr, reset_action);

	ncr->n_last_slot = ncr->n_cur_slot;
	ncr->n_cur_slot = UNDEFINED;
	ncr->n_state = STATE_FREE;
	bzero((caddr_t)ncr->n_slots,
		(sizeof (struct scsi_cmd *)) * NTARGETS * NLUNS_PER_TARGET);
	ncr->n_ncmds = ncr->n_npolling = ncr->n_ndisc = 0;
	ncr->n_omsgidx = ncr->n_omsglen = 0;
}

static void
ncr_hw_reset(ncr, action)	/* reset and set up host-adapter H/W */
struct ncr *ncr;
int	action;
{
	u_char junk;

	/* reset scsi control logic */
	SET_CSR(ncr, 0);
	DELAY(10);
	SET_CSR(ncr, NCR_CSR_SCSI_RES);

	if (ncr->n_type != IS_3_50) {
		SET_DMA_ADDR(ncr, 0);
		SET_DMA_COUNT(ncr, 0);
	}

	/*
	 * issue scsi bus reset (make sure interrupts from sbc are disabled)
	 */
	N_SBC->icr = NCR_ICR_RST;
	DELAY(100);
	N_SBC->icr = 0; 	/* clear reset */
	PRINTF3("ncr_hw_reset: RESETTING scsi_bus\n");

	/* give reset scsi devices time to recover (> 2 Sec) */
	DELAY(NCR_RESET_DELAY);
	junk = N_SBC->clr;

	/* Disable sbc interrupts */
	N_SBC->mr = 0;	/* clear phase int */
	N_SBC->ser = 0;	/* disable (re)select interrupts */
	N_SBC->ser = NUM_TO_BIT(ncr->n_id);
	/*
	 * enable general interrupts
	 */
	ENABLE_INTR(ncr);
	if (ncr->n_type != IS_3_50)
		DISABLE_DMA(ncr);
}

/*
 * Debugging/printing functions
 */

static void
ncr_dump_datasegs(sp)
struct scsi_cmd *sp;
{
	struct dataseg *seg;
	if ((sp->cmd_flags & CFLAG_DMAVALID) == 0)
		return;
	printf("Data Mappings:\n");
	for (seg = &sp->cmd_mapseg; seg; seg = seg->d_next) {
		printf("\tBase = 0x%x Count = 0x%x\n",
			seg->d_base, seg->d_count);
	}
	printf("Transfer History:\n");
	for (seg = &sp->cmd_subseg; seg; seg = seg->d_next) {
		printf("\tBase = 0x%x Count = 0x%x\n",
			seg->d_base, seg->d_count);
	}
}

static void
ncr_printstate(ncr)
register struct ncr *ncr;
{
	static char *m[4] = {
		"4/110", "3/50 or 3/60", "SCSI-3", "3/E SCSI"
	};
	register int re_en = 0;
	u_char	cdr, icr, mr, tcr, cbsr, bsr;
	u_int 	csr;

	/*
	 * Print h/w information
	 */
	csr = GET_CSR(ncr);
	if (ncr->n_type != IS_3_50 && (csr & NCR_CSR_DMA_EN)) {
		re_en++;
		DISABLE_DMA(ncr);
	}
	cdr = N_SBC->cdr;
	icr = N_SBC->icr;
	mr = N_SBC->mr;
	tcr = N_SBC->tcr;
	cbsr = N_SBC->cbsr;
	bsr = N_SBC->bsr;
	printf("%s%d: host adapter for %s\n", CNAME, CNUM, m[ncr->n_type]);
	printf("csr=0x%b; cbsr=0x%b\n", csr, CSR_BITS, cbsr, cbsr_bits);
	printf("bsr=0x%b; tcr=0x%b; mr=0x%b\n", bsr, BSR_BITS, tcr, TCR_BITS,
		mr, MR_BITS);
	printf("icr=0x%b; cdr=0x%x; ", icr, ICR_BITS, cdr);
	if (ncr->n_type != IS_3_50) {
		printf(" dma_addr=0x%x; dma_cnt=0x%x",
			GET_DMA_ADDR(ncr), GET_DMA_COUNT(ncr));
		if (ncr->n_type == IS_SCSI3)
			printf(" bcr=0x%x", GET_BCR(ncr));
		printf("\n");
	} else {
		printf(" bcr=0x%x\n", GET_BCR(ncr));
	}

	/*
	 * Print s/w information
	 */

	printf("State=0x%x; Laststate=0x%x; last msgin: %x last msgout: %x\n",
	    ncr->n_state, ncr->n_laststate, ncr->n_last_msgin,
	    ncr->n_last_msgout);

	if (ncr->n_cur_slot != UNDEFINED) {
		register struct scsi_cmd *sp = CURRENT_CMD(ncr);
		printf("Currently connected to %d.%d, pkt state=0x%x\n",
			Tgt(sp), Lun(sp), sp->cmd_pkt.pkt_state);
		ncr_dump_datasegs(sp);
	}

	/*
	 * leave...
	 */
	if (re_en)
		ENABLE_DMA(ncr);
}

#endif NNCR > 0
