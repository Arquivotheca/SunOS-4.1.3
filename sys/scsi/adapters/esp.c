#ident	"@(#)esp.c 1.1 92/07/30 SMI"
/*
 * Copyright (c) 1988-1991 by Sun Microsystems, Inc.
 */
#include "esp.h"
#if NESP > 0

#include <scsi/scsi.h>
#include <scsi/adapters/espvar.h>

#ifdef	OPENPROMS
#include <sun/autoconf.h>
#endif	/* OPENPROMS */

#ifdef	sun4m
#include <sun4m/iommu.h>
#endif	/* sun4m */

/*
 * Update Grok- to allow compilation on systems without newer dmaga.h header(s)
 */
#ifndef BURSTSIZE
#define	BURSTSIZE
#define	BURST1		0x01
#define	BURST2		0x02
#define	BURST4		0x04
#define	BURST8		0x08
#define	BURST16		0x10
#define	BURST32		0x20
#define	BURST64		0x40
#define	BURSTSIZE_MASK	0x7f
#define	DEFAULT_BURSTSIZE	BURST16|BURST8|BURST4|BURST2|BURST1
#endif	/* BURSTSIZE */

#ifndef DMAGA_TURBO
#define	DMAGA_TURBO	0x00400000
#endif	/* !DMAGA_TURBO */

#ifndef ESC1_REV1
#define	ESC1_REV1	0x4
#define	SET_DMAESC_COUNT(dmar, val) (dmar)->dmaga_count = val
#define	DMAESC_BSIZE	0x0800
#define	DMAESC_TCZERO	0x1000
#define	DMAESC_EN_TCI	0x2000
#define	DMAESC_INTPEND	0x4000
#define	DMAESC_PEN	0x8000
#define	DMAESC_PERR	0x00010000
#define	DMAESC_DRAIN	0x00020000
#define	DMAESC_EN_ADD	0x00040000
#define	DMAESC_SETBURST16(d)	(d)->dmaga_csr |= DMAESC_BSIZE
#define	DMAESC_SETBURST32(d)	(d)->dmaga_csr &= ~DMAESC_BSIZE
#endif	/* ESC1_REV1 */

#ifndef DMA_REV3
#define	DMA_REV3	0xA	/* DMA2 gate array */
#define	DMAGA_BURSTMASK 0x000C0000
#define	DMAGA_BURST16	0x00000000
#define	DMAGA_NOBURST	0x00080000
#define	DMAGA_BURST32	0x00040000
#define	DMA2_SETNOBURST(d)	\
	(d)->dmaga_csr &= ~DMAGA_BURSTMASK, (d)->dmaga_csr |= DMAGA_NOBURST
#define	DMA2_SETBURST16(d)	(d)->dmaga_csr &= ~DMAGA_BURSTMASK
#define	DMA2_SETBURST32(d)	\
	(d)->dmaga_csr &= ~DMAGA_BURSTMASK, (d)->dmaga_csr |= DMAGA_BURST32
#define	DMAGA_TWO_CYCLE 0x00200000
#endif	/* !DMA_REV3 */




/*
 * Local function declarations
 */

static int esp_start(), esp_abort(), esp_reset(), esp_getcap(), esp_setcap();
static int esp_poll(), esp_watch(), esp_phasemanage();
static int esp_finish_select(), esp_reconnect(), esp_finish();
static int esp_ustart(), esp_startcmd(), esp_reset_recovery(), esp_istart();
static int esp_abort_curcmd(), esp_abort_allcmds(), esp_reset_bus();
static int esp_handle_selection();
static int esp_dopoll();

#ifdef	VECTORED_INTERRUPTS
static int esp_intr();
#endif	/* VECTORED_INTERRUPTS */

static void esp_internal_reset(), esp_sync_backoff();
static void esp_hw_reset(), esp_curcmd_timeout(), esp_disccmd_timeout();
static void esplog(), eprintf(), esp_stat_int_print();
static void esp_watchsubr();
static void espsvc();
static void esp_dump_datasegs(), esp_dump_cmd();
static void esp_chip_disconnect();

static void esp_makeproxy_cmd(), esp_make_sdtr();
static void esp_init_cmd(), esp_runpoll();

/*
 * Phase handlers
 */

static int esp_handle_unknown(), esp_handle_cmd_start(), esp_handle_cmd_done();
static int esp_handle_msg_out(), esp_handle_msg_out_done();
static int esp_handle_clearing();
static int esp_handle_msg_in(), esp_handle_more_msgin();
static int esp_handle_msg_in_done();
static int esp_handle_c_cmplt(), esp_handle_data(), esp_handle_data_done();

/*
 * Debug routines- ultimately to be hidden within ESPDEBUG
 */

#ifdef	ESPDEBUG
static	void esp_dump_state();
#endif	/* ESPDEBUG */
static	void esp_printstate();
static	char *esp_state_name();

/*
 * Local static data
 */


static int espconf = 0;
static int espconf2 = 0;
static int espconf3 = 0;

#define	POLL_TIMEOUT	(3 * 60000000)	/* in usec, about 3 mins */

static struct esp *esp_softc = (struct esp *) 0;
#ifdef	OPENPROMS
static int nesp;
#else	/* OPENPROMS */
static int nesp = NESP;
#endif	/* OPENPROMS */

static char *esp_stat_bits = ESP_STAT_BITS;
static char *esp_int_bits = ESP_INT_BITS;
static char *dmaga_bits = DMAGA_BITS;
static char *msginperr = "SCSI bus MESSAGE IN phase parity error\n";
#ifdef	sun4c
static int esp_ss1_esp0sync = 0;
#endif	/* sun4c */
static int esp_nhardints;
static int esp_nmultsvc;
#ifdef	ESPDEBUG
static int espdebug = 0;
#endif	/* ESPDEBUG */

#ifdef	ESP_TEST_PARITY
static int esp_ptest_msgin;
static int esp_ptest_msg = -1;
static int esp_ptest_msgcmplt;
static int esp_ptest_status;
static int esp_ptest_data_in;
#endif	/* ESP_TEST_PARITY */

#ifdef ESP_TEST_TIMEOUT
static int esp_ttest;
static int esp_stest;
#endif /* ESP_TEST_TIMEOUT */

/*
 * autoconfiguration routines.
 */

#ifdef	OPENPROMS

static int esp_identify(), esp_attach();
struct dev_ops esp_ops = {
	1,
	esp_identify,
	esp_attach
};

#ifdef	GASP

extern int gasp_cards;
extern u_long gasp_addrs[];
#define	ESP_OFFSET	0x800000
#define	Esp_addr(card)	(gasp_addrs[(card)] + ESP_OFFSET)

static void
new_esp(prev, gaddr)
struct dev_info *prev;
u_long gaddr;
{
	struct dev_info *dev;

	dev = (struct dev_info *) kmem_zalloc (sizeof (*dev));
	dev->devi_parent =  prev->devi_parent;
	dev->devi_next = prev->devi_next;
	prev->devi_next = dev;
	dev->devi_name = prev->devi_name;
	dev->devi_nreg = 1;
	dev->devi_reg = (struct dev_reg *)
		kmem_zalloc(sizeof (struct dev_reg));
	dev->devi_reg->reg_bustype = OBIO;
	dev->devi_reg->reg_addr = (addr_t) gaddr;
	dev->devi_reg->reg_size = sizeof (struct espreg);
	dev->devi_nintr = 1;
	dev->devi_intr = prev->devi_intr;
	dev->devi_driver = &esp_ops;
	dev->devi_nodeid = prev->devi_nodeid; /* lies. all lies */
	nesp += 1;
}

#endif	/* GASP */

static int
esp_identify(name)
	char *name;
{
	/*
	 * Note the octal literal for a comma.
	 * Avoids Shannon's C-style awk script's complaints
	 */
	if (strcmp(name, "esp") == 0 || strcmp(name, "SUNW\054esp") == 0) {
		nesp++;
		return (1);
	} else {
		return (0);
	}
}


static int
esp_attach(dev)
register struct dev_info *dev;
{
#ifdef	ESP_NEW_HW_DEBUG
	static char *cdebug = "esp%d: %s %s returned as %d\n";
#endif	/* ESP_NEW_HW_DEBUG */
	static char *prop_ini_id = "initiator-id";
	static char *prop_scsi_ini_id = "scsi-initiator-id";
	static char *prop_cfreq = "clock-frequency";
	static char *prop_bur_size = "burst-sizes";
	static char *prop_diff = "differential";
	static int next_unit = 0;
	static int esp_spl = 0;
	register struct dmaga *dmar;
	register struct espreg *ep;
	register struct esp *esp, *nsp;
	register struct dev_info *ndev;
	int i, cur_unit;
	u_long ticks;
	u_char clock_conv;

#ifdef	GASP
	if (next_unit == 0) {
		register card;
		struct dev_info *tmp = dev;
		for (card = 0; card < 4; card++) {
			if (gasp_cards & (1<<card)) {
				new_esp(tmp, Esp_addr(card));
				tmp = tmp->devi_next;
			}
		}
	}
#endif	/* GASP */

	cur_unit = dev->devi_unit = next_unit++;

	if (cur_unit >= nesp || dev->devi_nreg > 1 || dev->devi_nintr == 0) {
		return (-1);
	}

#ifdef	sun4c
	/*
	 * Check to see whether this is a useful
	 * slot for an sbus scsi interface
	 */
	if ((i = slaveslot(dev->devi_reg->reg_addr)) >= 0) {
		printf("esp%d: not used - SBus slot %d is slave-only\n",
		    cur_unit, i);
		return (-1);
	}
#endif	/* sun4c */

	/*
	 * map in device registers
	 */

	ep = (struct espreg *) map_regs(dev->devi_reg->reg_addr,
	    dev->devi_reg->reg_size, dev->devi_reg->reg_bustype);

	if (ep == (struct espreg *) 0) {
		printf("esp%d: unable to map registers\n", cur_unit);
		return (-1);
	}

	dmar = dma_alloc(dev->devi_reg->reg_bustype, dev->devi_reg->reg_addr);

	if (!dmar) {
		printf("esp%d: cannot find dma controller\n", cur_unit);
		unmap_regs((addr_t) ep, dev->devi_reg->reg_size);
		return (-1);
	}
	/*
	 * Initialize state of DMA gate array.
	 * Must clear DMAGA_RESET on the ESC before accessing the esp.
	 */
	if (DMAGA_REV(dmar) == ESC1_REV1) {
		dmar->dmaga_csr &= ~DMAGA_RESET;
	}
	dmar->dmaga_csr &= ~DMAGA_WRITE;

	/*
	 * If we haven't allocated the softc structure, do so now.
	 */

	esp  = (struct esp *) kmem_zalloc((unsigned) (sizeof (struct esp)));
	if (esp != (struct esp *) 0) {
		esp->e_cmdarea = (u_char *) IOPBALLOC(FIFOSIZE);
	}

	if (esp == (struct esp *) 0 || esp->e_cmdarea == (u_char *) 0) {
		printf("esp%d: no space for %s\n", cur_unit, (esp->e_cmdarea) ?
		    "cmd areas" : "data structures");
		if (esp) {
			(void) kmem_free((caddr_t)esp, (sizeof (struct esp)));
		}
		unmap_regs((addr_t) ep, dev->devi_reg->reg_size);
		dma_free(dmar);
		return (-1);
	}

	esp->e_next = (struct esp *) 0;
	esp->e_unit = cur_unit;

	/*
	 * Establish initial softc values
	 */

	if (esp_softc == (struct esp *) 0) {
		esp_softc = esp;
		timeout (esp_watch, (caddr_t) 0, hz);
	} else {
		for (nsp = esp_softc; nsp->e_next != (struct esp *) 0;
							nsp = nsp->e_next);
		nsp->e_next = esp;
	}

	/*
	 * Determine clock frequency of attached ESP chip.
	 */

	for (ndev = dev; ndev; ndev = ndev->devi_parent) {
		i = getprop(ndev->devi_nodeid, prop_cfreq, -1);
		if ((i > 0) || ndev == top_devinfo) {
#ifdef	ESP_NEW_HW_DEBUG
			printf (cdebug, cur_unit, ndev->devi_name,
					prop_cfreq, i);
#endif	/* ESP_NEW_HW_DEBUG */
			break;
		}
	}


#ifdef	ESP_NEW_HW_DEBUG
	if (i != 40000000) {
		printf("WARNING: found clock frequency of %d\n", i);
		printf("	 Assuming clock frequency of 40 MHZ\n");
		i = 40000000;
	}
#endif	/* ESP_NEW_HW_DEBUG */

	/*
	 * Valid clock freqs. are between 10 and 40 MHz.  Otherwise
	 * presume 20 MHz. and complain.  (Notice, that we wrap to
	 * zero at 40 MHz.  Ick!)  This test should NEVER fail!
	 *
	 *	freq (MHz)	clock conversion factor
	 *	10		2
	 *	10.01-15	3
	 *	15.01-20	4
	 *	20.01-25	5
	 *	25.01-30	6
	 *	30.01-35	7
	 *	35.01-40	8 (0)
	 */
	if (i > FIVE_MEG) {
		clock_conv = (i + FIVE_MEG - 1)/ FIVE_MEG;
	} else {
		clock_conv = 0;
	}
	if (clock_conv < CLOCK_10MHZ || clock_conv > CLOCK_40MHZ) {
		esplog(esp, LOG_ERR,
		    "Bad clock frequency- setting 20mhz, asynchronous mode");
		esp->e_weak = 0xff;
		clock_conv = CLOCK_20MHZ;
		i = TWENTY_MEG;
	}
	esp->e_clock_conv = clock_conv;
	esp->e_clock_cycle = CLOCK_PERIOD(i);
	ticks = ESP_CLOCK_TICK(esp);
	esp->e_stval = ESP_CLOCK_TIMEOUT(ticks);

#ifdef	ESP_TEST_FAST_SYNC
	IPRINTF5("%d mhz, clock_conv %d, clock_cycle %d, ticks %d, stval %d\n",
		i, esp->e_clock_conv, esp->e_clock_cycle,
		ticks, esp->e_stval);
#endif	ESP_TEST_FAST_SYNC

	/*
	 * The theory is that the layout of all cpu boards post SparcStation-1
	 * is clean enough to allow SYNC mode to be on all the time.
	 */
#ifdef	sun4c
	if (cpu == CPU_SUN4C_60 && dev->devi_unit == 0 &&
	    esp_ss1_esp0sync == 0) {
		esp->e_weak = 0xff;
	}
#endif	/* sun4c */

	/*
	 * Calculate max locking spl so far seen..
	 */

	esp->e_spl = ipltospl(dev->devi_intr->int_pri);
	esp_spl = MAX(esp_spl, esp->e_spl);

	/*
	 * Make sure that everyone, including us, is locking at
	 * the same priority.
	 */

	for (nsp = esp_softc; nsp != (struct esp *) 0; nsp = nsp->e_next)
		nsp->e_tran.tran_spl = esp_spl;

#ifdef	ESPDEBUG
	/*
	 * Initialize last state log.
	 */
	for (i = 0; i < NPHASE; i++) {
		esp->e_phase[i].e_save_state = STATE_FREE;
		esp->e_phase[i].e_save_stat = -1;
		esp->e_phase[i].e_val1 = -1;
		esp->e_phase[i].e_val2 = -1;
	}
	esp->e_phase_index = 0;
	esp->e_xfer = 0;
#endif	/* ESPDEBUG */

	esp->e_tran.tran_start		= esp_start;
	esp->e_tran.tran_abort		= esp_abort;
	esp->e_tran.tran_reset		= esp_reset;
	esp->e_tran.tran_getcap		= esp_getcap;
	esp->e_tran.tran_setcap		= esp_setcap;
	esp->e_tran.tran_pktalloc	= scsi_std_pktalloc;
	esp->e_tran.tran_dmaget		= scsi_std_dmaget;
	esp->e_tran.tran_pktfree	= scsi_std_pktfree;
	esp->e_tran.tran_dmafree	= scsi_std_dmafree;

	esp->e_dev = dev;

	esp->e_espconf = DEFAULT_HOSTID;

	for (ndev = dev; ndev; ndev = ndev->devi_parent) {
		i = getprop(ndev->devi_nodeid, prop_ini_id, -1);
#ifdef	ESP_NEW_HW_DEBUG
		printf (cdebug, cur_unit, ndev->devi_name, prop_ini_id, i);
#endif	/* ESP_NEW_HW_DEBUG */
		if (i == -1) {
			i = getprop(ndev->devi_nodeid, prop_scsi_ini_id, -1);
#ifdef	ESP_NEW_HW_DEBUG
			printf (cdebug, cur_unit, ndev->devi_name,
				prop_scsi_ini_id, i);
#endif	/* ESP_NEW_HW_DEBUG */
		}
		if (i != DEFAULT_HOSTID && i >= 0 && i <= 7) {
			esplog(esp, LOG_INFO, "initiator SCSI Id now %d", i);
			esp->e_espconf = (u_char) i;
		}
		if (i > -1 || ndev == top_devinfo)
			break;
	}

	if (scsi_options & SCSI_OPTIONS_PARITY)
		esp->e_espconf |= ESP_CONF_PAREN;
	esp->e_espconf |= (espconf & ~ESP_CONF_BUSID);
	esp->e_reg = ep;
	esp->e_dma = dmar;

	/*
	 * Determine sbus burst size.
	 * top_devinfo is declared in <sun/openprom.h>.
	 */

	esp->e_burstsizes = 0xff;
	ndev = esp->e_dev;
	do {
		i = getprop(ndev->devi_nodeid, prop_bur_size, -1);
		if (i != -1) {
			/*
			 * oops- we have somebody who constrains us.
			 */
			esp->e_burstsizes = esp->e_burstsizes & i;
#ifdef	ESP_NEW_HW_DEBUG
			printf (cdebug, cur_unit, ndev->devi_name,
				prop_bur_size, i);
#endif	/* ESP_NEW_HW_DEBUG */
		}
		ndev = ndev->devi_parent;
	} while (ndev != top_devinfo);

	/*
	 * No burstsize properties found- select a reasonable default
	 */
	i = esp->e_burstsizes;
	if ((i == 0xff) || (!((i & BURST16) || (i & BURST32)))) {
#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf(esp, "Bad burst size %d- setting to %d",
			i, DEFAULT_BURSTSIZE);
		}
#endif	/* ESPDEBUG */
		esp->e_burstsizes = DEFAULT_BURSTSIZE;
	}

	/*
	 * XXX: This needs to be based upon architecture. Right now,
	 * XXX: all OPENPROM machines are sun4c and have a dma base
	 * XXX: of -1mb.
	 */
	esp->e_dma_base = (u_long) DVMA;
	esp->e_last_slot = esp->e_cur_slot = UNDEFINED;
	addintr(dev->devi_intr->int_pri, esp_poll,
	    dev->devi_name, dev->devi_unit);
	adddma(dev->devi_intr->int_pri);
	report_dev(dev);
	ep->esp_conf2 = 0;
	ep->esp_conf2 = 0xa;
	esp->e_default_period = CONVERT_PERIOD(DEFAULT_SYNC_PERIOD);
	if ((ep->esp_conf2&0xf) == 0xa) {
		esp->e_espconf2 = espconf2;
		ep->esp_conf3 = 0;
		ep->esp_conf3 = 5;
		if (ep->esp_conf3 == 0x5) {
			for (i = 0; i < NTARGETS; i++) {
				esp->e_espconf3[i] = espconf3;
			}
			if (clock_conv > CLOCK_25MHZ) {
				esp->e_espconf2 |= ESP_CONF2_FENABLE;
				ep->esp_conf2 = esp->e_espconf2;
				esp->e_default_period =
					CONVERT_PERIOD(DEFAULT_FASTSYNC_PERIOD);
				esp->e_type = FAST;
#ifdef	ESP_TEST_FAST_SYNC
				IPRINTF1("conf3=%x\n", espconf3);
				IPRINTF("found FAST\n");
#endif	ESP_TEST_FAST_SYNC
			} else {
				ep->esp_conf2 = esp->e_espconf2;
				esp->e_type = ESP236;
				IPRINTF("found ESP236\n");
			}
			ep->esp_conf3 = espconf3;
		} else {
			ep->esp_conf2 = esp->e_espconf2;
			esp->e_type = ESP100A;
		}
	} else {
		esp->e_type = ESP100;
	}

	/*
	 * check for differential scsi bus property
	 */
	if (getprop(dev->devi_nodeid, prop_diff, -1) != -1) {
		IPRINTF("differential scsi bus\n");
		esp->e_differential++;
	}

	esp_internal_reset(esp, ESP_RESET_ALL);

	/*
	 * Whew! Now make ourselves known to the rest of the SCSI world...
	 */

	scsi_config(&esp->e_tran, dev);

#ifdef	GASP
	if (dev->devi_unit == 0 && nesp > 1) {
		for (i = 1; i < nesp; i++) {
			dev = dev->devi_next;
			(void) esp_attach(dev);
		}
	}
#endif	/* GASP */
	return (0);
}

#else	/* OPENPROMS */

static int esp_probe(), esp_slave(), esp_attach();
struct mb_driver espdriver = {
	esp_probe, esp_slave, esp_attach, 0, 0, esp_poll,
	ESP_SIZE, "scsibus", 0, "esp", 0, MDR_BIODMA
};

static int
esp_probe(reg, ctlr)
caddr_t reg;
int ctlr;
{
	static char first_time = 1;
	register struct dmaga *dmar;
	register struct espreg *ep;
	register struct esp *esp, *nsp;
	u_long dma_base, ticks;
	u_char clock_conv;
	long lptr;

	/*
	 * These shorthand defines just for this routine
	 */

#define	DNAME	espdriver.mdr_cname
#define	DUNIT	ctlr


	switch (cpu) {
#ifdef	sun4
	case CPU_SUN4_330:
		dma_base = (u_long) DVMA;
		clock_conv = CLOCK_25MHZ;
		dmar = (struct dmaga *) (((long) reg) + STINGRAY_DMA_OFFSET);
		break;
#endif	/* sun4 */
#ifdef	sun3x
	case CPU_SUN3X_80:
		dma_base = (u_long) DVMA;
		clock_conv = CLOCK_20MHZ;
		dmar = (struct dmaga *) (((long) reg) + HYDRA_DMA_OFFSET);
		break;
#endif	/* sun3x */
	default:
		return (0);
	}

	if ((peekl((long *)&(dmar->dmaga_csr), (long *)&(lptr))) == -1) {
		return (0);
	}

	ep = (struct espreg *) reg;
	if ((peekc((char *)&(ep->esp_xcnt_lo))) == -1) {
		return (0);
	}

	esp  = (struct esp *) kmem_zalloc((unsigned) (sizeof (struct esp)));
	if (esp != (struct esp *) 0) {
		esp->e_cmdarea = (u_char *) IOPBALLOC(FIFOSIZE);
	}

	if (esp == (struct esp *) 0 || esp->e_cmdarea == (u_char *) 0) {
		printf("esp%d: no space for %s\n", DUNIT, (esp->e_cmdarea) ?
		    "cmd areas" : "data structures");
		if (esp) {
			(void) kmem_free((caddr_t)esp, (sizeof (struct esp)));
		}
		return (0);
	}

	esp->e_next = (struct esp *)0;
	esp->e_unit = DUNIT;

	/*
	 * Establish initial softc values
	 */

	if (esp_softc == (struct esp *) 0) {
		esp_softc = esp;
	} else {
		for (nsp = esp_softc; nsp->e_next != (struct esp *) 0;
							nsp = nsp->e_next);
		nsp->e_next = esp;
	}

	esp->e_dev = &espdriver;

	esp->e_espconf = DEFAULT_HOSTID;
	if (scsi_options & SCSI_OPTIONS_PARITY)
		esp->e_espconf |= ESP_CONF_PAREN;
	esp->e_espconf |= (espconf & ~ESP_CONF_BUSID);
	esp->e_burstsizes = 0;
	esp->e_clock_conv = clock_conv;
	esp->e_clock_cycle = FIVE_MEG * clock_conv;
	ticks = ESP_CLOCK_TICK(esp);
	esp->e_stval = ESP_CLOCK_TIMEOUT(ticks);
	esp->e_backoff = 0;
#ifdef	ESP_TEST_FAST_SYNC
	IPRINTF5("%d mhz, clock_conv %d, clock_cycle %d, ticks %d, stval %d\n",
		FIVE_MEG * clock_conv, esp->e_clock_conv, esp->e_clock_cycle,
		ticks, esp->e_stval);
#endif	ESP_TEST_FAST_SYNC

	esp->e_reg = ep;
	esp->e_dma = dmar;

	/*
	 * Initialize state of DMA gate array.
	 */
	dmar->dmaga_csr = dmar->dmaga_csr & ~DMAGA_WRITE;

	esp->e_dma_base = dma_base;
	esp->e_tran.tran_start		= esp_start;
	esp->e_tran.tran_abort		= esp_abort;
	esp->e_tran.tran_reset		= esp_reset;
	esp->e_tran.tran_getcap		= esp_getcap;
	esp->e_tran.tran_setcap		= esp_setcap;
	esp->e_tran.tran_pktalloc	= scsi_std_pktalloc;
	esp->e_tran.tran_dmaget		= scsi_std_dmaget;
	esp->e_tran.tran_pktfree	= scsi_std_pktfree;
	esp->e_tran.tran_dmafree	= scsi_std_dmafree;
	esp->e_last_slot = esp->e_cur_slot = UNDEFINED;

	if (first_time) {
		first_time = 0;
		timeout(esp_watch, (caddr_t) 0, hz);
	}
	esp_internal_reset(esp, ESP_RESET_ALL);
#undef	DNAME
#undef	DUNIT
	return (ESP_SIZE);
}

/*ARGSUSED*/
static int
esp_slave(md, reg)
struct mb_device *md;
caddr_t reg;
{
	struct esp *esp;
	register struct espreg *ep;
	register int i;

	for (esp = esp_softc, i = 0; esp->e_next != (struct esp *) 0,
		i < md->md_ctlr; esp = esp->e_next, i++);
	ep = esp->e_reg;
	ep->esp_conf2 = 0;
	ep->esp_conf2 = 0xa;
	esp->e_type = ESP100;
	if ((ep->esp_conf2&0xf) == 0xa) {
		ep->esp_conf2 = esp->e_espconf2 = espconf2;
		ep->esp_conf3 = 0;
		ep->esp_conf3 = 5;
		if (ep->esp_conf3 == 0x5) {
			ep->esp_conf3 = espconf3;
			for (i = 0; i < NTARGETS; i++) {
				esp->e_espconf3[i] = espconf3;
			}
			esp->e_type = ESP236;
		} else
			esp->e_type = ESP100A;
	}
	md->md_slave = MY_ID(esp) << 3;
	return (1);
}

static int
esp_attach(md)
struct mb_device *md;
{
	int i;
	static int esp_spl = 0;
	struct esp *esp, *nsp;


	for (esp = esp_softc, i = 0; esp->e_next != (struct esp *) 0,
		i < md->md_ctlr; esp = esp->e_next, i++);
	/*
	 * Calculate max locking spl so far seen..
	 */
	esp->e_spl = pritospl(md->md_intpri);
	esp_spl = MAX(esp_spl, esp->e_spl);

	/*
	 * Make sure that everyone, including us, is locking at
	 * the same priority.
	 */

	for (nsp = esp_softc; nsp != (struct esp *) 0; nsp = nsp->e_next) {
		nsp->e_tran.tran_spl = esp_spl;
	}

#ifdef	ESPDEBUG
	/*
	 * Initialize last state log.
	 */
	for (i = 0; i < NPHASE; i++) {
		esp->e_phase[i].e_save_state = STATE_FREE;
		esp->e_phase[i].e_save_stat = -1;
		esp->e_phase[i].e_val1 = -1;
		esp->e_phase[i].e_val2 = -1;
	}
	esp->e_phase_index = 0;
	esp->e_xfer = 0;
#endif	/* ESPDEBUG */

	/*
	 * Whew! Now make ourselves known to the rest of the SCSI world...
	 */

	scsi_config(&esp->e_tran, md);
}

#endif	/* OPENPROMS */

/*
 * Interface functions
 *
 * Visible to the external world via the transport structure.
 */

/*
 * esp_start - Accept commands for transport
 */

static int
esp_start(sp)
register struct scsi_cmd *sp;
{
	register s;
	register short slot;
	register struct esp *esp;

	esp = (struct esp *) sp->cmd_pkt.pkt_address.a_cookie;
	slot =	(sp->cmd_pkt.pkt_address.a_target * NLUNS_PER_TARGET) |
	    (sp->cmd_pkt.pkt_address.a_lun);

	if (sp->cmd_flags & CFLAG_DMAVALID) {
		u_long maxdma;
		switch (DMAGA_REV(esp->e_dma)) {
		default:
		case DMA_REV1:
		case DMA_REV2:
		case ESC1_REV1:
			maxdma = 1<<24;
			break;
		case DMA_REV3:
			maxdma = 1<<30; /* be reasonable - 2gb is enuff */
			break;
		}
		if (sp->cmd_mapseg.d_count >= maxdma) {
			return (TRAN_BADPKT);
		}
	}

	s = splr(esp->e_tran.tran_spl);
	if (esp->e_slots[slot] != (struct scsi_cmd *) 0) {
		/*
		 * This is where we would see if this is a SCSI-2
		 * multiple command target.
		 */
		/*
		 * Else queueing error...
		 */
		(void) splx(s);
		return (TRAN_BUSY);
	}

	/*
	 * Else, accept the command
	 */

	esp->e_slots[slot] = sp;
	esp->e_ncmds++;

	/*
	 * Reinitialize some fields that need it
	 */

	esp_init_cmd(sp);

	/*
	 * Run the command (maybe).
	 */

	if ((sp->cmd_pkt.pkt_flags & FLAG_NOINTR) == 0) {
		/*
		 * Okay- if this is command is *not* a polling command,
		 * and we're not pending any polling commands, *and*
		 * the state is FREE, *then* we can start this command.
		 */
		if (esp->e_npolling == 0 && esp->e_state == STATE_FREE) {
			(void) esp_ustart(esp, slot);
		}

	} else {
		esp_runpoll(esp, slot);
	}
	(void) splx(s);
	return (TRAN_ACCEPT);
}

static int
esp_abort(ap, pkt)
struct scsi_address *ap;
struct scsi_pkt *pkt;
{
	register struct esp *esp = (struct esp *) ap->a_cookie;
	register struct scsi_cmd *sp = (struct scsi_cmd *) pkt;
	register int s, rval;
	register short slot;

	/*
	 * find the slot for this specific address
	 */

	slot =	(ap->a_target * NLUNS_PER_TARGET) | ap->a_lun;

	/*
	 * Take care of the easy cases first:
	 *
	 *   A specific command was passed as an argument (to be aborted) and
	 *
	 *	The command is currently active; we cannot abort now
	 *	The command does not exist here.
	 *	The command exists here, but hasn't been started yet.
	 *
	 *   No specific command was passed as an argument (abort all
	 *   commands at this nexus), and
	 *
	 *	No commands exist for this nexus.
	 *	Commands that exist here haven't been started yet.
	 *
	 */

	s = splr(esp->e_tran.tran_spl);


	if (esp->e_state != STATE_FREE && esp->e_cur_slot == slot) {
		/*
		 * This isn't right, but it will have to do for now.
		 * If the currently active command is the one to be
		 * aborted, we don't know at this level whether it
		 * is safe, nor how to, queue up the ABORT OPERATION
		 * message. Instead, punt, and let the target driver
		 * decide whether to pull the chain on the bus.
		 */
		(void) splx(s);
		return (FALSE);
	}

	if (sp == (struct scsi_cmd *) 0) {
		sp = esp->e_slots[slot];
	}

	if (esp->e_slots[slot] == (struct scsi_cmd *) 0) {
		sp = (struct scsi_cmd *) 0;
	}

	if (sp && sp == esp->e_slots[slot] && sp->cmd_pkt.pkt_state == 0) {
		esp->e_slots[slot] = (struct scsi_cmd *) 0;
		sp->cmd_pkt.pkt_reason = CMD_ABORTED;
		esp->e_ncmds -= 1;
		(*sp->cmd_pkt.pkt_comp)(sp);
		sp = (struct scsi_cmd *) 0;
	}

	if (sp == (struct scsi_cmd *) 0) {
		(void) splx(s);
		return (TRUE);
	}

	/*
	 * Now deal with the hard cases. We have a valid sp for the
	 * single active command at this nexus (XXX later we will have
	 * a list of commands to abort XXX).
	 */
	{
		/*
		 * In this case, the command is active but disconnected
		 */
		auto struct scsi_cmd local;
		register struct scsi_cmd *savesp = sp;
		int wasdisc;

		esp->e_slots[slot] = (struct scsi_cmd *) 0;
		if (sp->cmd_flags & CFLAG_CMDDISC) {
			wasdisc = 1;
			esp->e_ndisc -= 1;
		} else {
			wasdisc = 0;
		}
		esp->e_ncmds -= 1;
		sp = &local;
		esp_makeproxy_cmd(sp, ap, MSG_ABORT);

		IPRINTF2 ("Sending proxy abort message to %d.%d\n",
		    ap->a_target, ap->a_lun);
		if (esp_start(sp) == TRAN_ACCEPT &&
		    sp->cmd_pkt.pkt_reason == CMD_CMPLT &&
		    sp->cmd_cdb[ESP_PROXY_RESULT] == TRUE) {
			IPRINTF2 ("Proxy abort succeeded for %d.%d\n",
			    ap->a_target, ap->a_lun);
			rval = TRUE;
			savesp->cmd_pkt.pkt_reason = CMD_ABORTED;
			(*savesp->cmd_pkt.pkt_comp)(savesp);
			IPRINTF2 ("Completion called for %d.%d\n",
			    ap->a_target, ap->a_lun);
		} else {
			IPRINTF2 ("Proxy abort failed for %d.%d\n",
			    ap->a_target, ap->a_lun);
			esp->e_ncmds++;
			if (wasdisc) {
				esp->e_ndisc++;
				savesp->cmd_flags |= CFLAG_CMDDISC;
			}
			esp->e_slots[slot] = savesp;
			rval = FALSE;

		}
		if (esp->e_state == STATE_FREE) {
			(void) esp_ustart(esp, NEXTSLOT(slot));
		}
	}
	(void) splx(s);
	return (rval);
}

static int
esp_reset(ap, level)
struct scsi_address *ap;
int level;
{
	register s, rval;
	struct esp *esp = (struct esp *) ap->a_cookie;

	rval = FALSE;
	s = splr(esp->e_tran.tran_spl);

	if (level == RESET_ALL) {


		/*
		 * We know that esp_reset_bus() returns ACTION_RETURN.
		 */

		(void) esp_reset_bus(esp);

		/*
		 * Now call esp_dopoll() to field the reset interrupt
		 * which will than call esp_reset_recovery which will
		 * call completion for all commands.
		 *
		 * XXX: mark that we are in 'polling' mode so that
		 * XXX: esp_reset_recovery just returns up to esp_dopoll()
		 * XXX: with state == STATE_FREE. Gross, but esp_reset_recovery
		 * XXX: will clear the e_npolling flag, but remember that
		 * XXX: we were in 'polling' mode and just return ACTION_RETURN
		 *
		 */

		esp->e_npolling++;

		if (esp_dopoll(esp, POLL_TIMEOUT)) {
			esplog(esp, LOG_ERR, "reset scsi bus failed");
		} else
			rval = TRUE;
	} else {
		short slot = (ap->a_target * NLUNS_PER_TARGET) | ap->a_lun;

		if (esp->e_state == STATE_FREE && esp->e_slots[slot] == 0 &&
		    esp->e_npolling == 0) {
			auto struct scsi_cmd local;
			register struct scsi_cmd *sp = &local;

			IPRINTF2 ("Sending proxy reset message to %d.%d\n",
			    ap->a_target, ap->a_lun);

			esp_makeproxy_cmd(sp, ap, MSG_DEVICE_RESET);
			if (esp_start(sp) == TRAN_ACCEPT &&
			    sp->cmd_pkt.pkt_reason == CMD_CMPLT &&
			    sp->cmd_cdb[ESP_PROXY_RESULT] == TRUE) {
				IPRINTF2 ("proxy reset of %d.%d ok\n",
				    ap->a_target, ap->a_lun);
				rval = TRUE;
			} else {
				/*
				 * make sure the local packet is not left in
				 * the active slot
				 */
				if (esp->e_slots[slot] == &local) {
					esp->e_slots[slot] =
					    (struct scsi_cmd *) NULL;
				}
			}
		}
	}

	if (esp->e_state == STATE_FREE) {
		(void) esp_ustart(esp, 0);
	}

	(void) splx(s);
	return (rval);
}

static int
esp_commoncap(ap, cap, val, tgtonly, doset)
struct scsi_address *ap;
char *cap;
int val, tgtonly, doset;
{
	register struct esp *esp = (struct esp *) ap->a_cookie;
	register cidx;
	register u_char tshift = (1<<ap->a_target);
	register u_char ntshift = ~tshift;
	register rval = FALSE;

	if ((tgtonly != 0 && tgtonly != 1) || cap == (char *) 0) {
		return (rval);
	}

	for (cidx = 0; scsi_capstrings[cidx]; cidx++) {
		if (strcmp(cap, scsi_capstrings[cidx]) == 0)
			break;
	}

	if (scsi_capstrings[cidx] == (char *) 0) {
		rval = UNDEFINED;
	} else if (doset && (val == 0 || val == 1)) {
		/*
		 * At present, we can only set binary (0/1) values
		 */

		switch (cidx) {
		case SCSI_CAP_DMA_MAX:
		case SCSI_CAP_MSG_OUT:
		case SCSI_CAP_PARITY:
		case SCSI_CAP_INITIATOR_ID:
			/*
			 * None of these are settable via
			 * the capability interface.
			 */
			break;

		case SCSI_CAP_DISCONNECT:

			if ((scsi_options & SCSI_OPTIONS_DR) == 0) {
				break;
			} else if (tgtonly) {
				if (val)
					esp->e_nodisc &= ntshift;
				else
					esp->e_nodisc |= tshift;
			} else {
				esp->e_nodisc = (val) ? 0 : 0xff;
			}
			rval = TRUE;
			break;

		case SCSI_CAP_SYNCHRONOUS:

			if ((scsi_options & SCSI_OPTIONS_SYNC) == 0) {
				break;
			} else if (tgtonly) {
				if (val) {
					esp->e_weak &= ntshift;
				} else {
					esp->e_weak |= tshift;
				}
				esp->e_sync_known &= ntshift;
			} else {
				if (val) {
					esp->e_weak = 0;
				} else {
					esp->e_weak = 0xff;
				}
				esp->e_sync_known = 0;
			}
			rval = TRUE;
			break;

		case SCSI_CAP_WIDE_XFER:
		case SCSI_CAP_UNTAGGED_QING:
		case SCSI_CAP_TAGGED_QING:
		default:
			rval = UNDEFINED;
			break;
		}
	} else if (doset == 0) {
		switch (cidx) {
		case SCSI_CAP_DMA_MAX:
			switch (DMAGA_REV(esp->e_dma)) {
			default:
			case DMA_REV1:
			case DMA_REV2:
			case ESC1_REV1:
				rval = 1<<24;
				break;
			case DMA_REV3:
				rval = 1<<30;
				break;
			}
			break;
		case SCSI_CAP_MSG_OUT:
			rval = TRUE;
			break;
		case SCSI_CAP_DISCONNECT:
			if ((scsi_options & SCSI_OPTIONS_DR) &&
			    (tgtonly == 0 || (esp->e_nodisc & tshift) == 0)) {
				rval = TRUE;
			}
			break;
		case SCSI_CAP_SYNCHRONOUS:
			if ((scsi_options & SCSI_OPTIONS_SYNC) &&
			    (tgtonly == 0 || esp->e_offset[ap->a_target])) {
				rval = TRUE;
			}
			break;
		case SCSI_CAP_PARITY:
			if (scsi_options & SCSI_OPTIONS_PARITY)
				rval = TRUE;
			break;
		case SCSI_CAP_INITIATOR_ID:
			rval = MY_ID(esp);
			break;
		default:
			rval = UNDEFINED;
			break;
		}
	}
	return (rval);
}

static int
esp_getcap(ap, cap, whom)
struct scsi_address *ap;
char *cap;
int whom;
{
	return (esp_commoncap(ap, cap, 0, whom, 0));
}

static int
esp_setcap(ap, cap, value, whom)
struct scsi_address *ap;
char *cap;
int value, whom;
{
	return (esp_commoncap(ap, cap, value, whom, 1));
}

/*
 * Internal Search Routine.
 *
 * Search for a command to start.
 */

static int
esp_ustart(esp, start_slot)
register struct esp *esp;
short start_slot;
{
	register struct scsi_cmd *sp;
	register short slot, found = 0;
	char dkn;

	if (start_slot == UNDEFINED)
		start_slot = 0;

	slot = start_slot;

	/*
	 * If all commands queued up here are
	 * disconnected, there is no work to do.
	 *
	 * e_ndisc should always be <= e_ncmds.
	 *
	 */

	if (esp->e_ncmds <= esp->e_ndisc) {
		return (FALSE);
	}

	do {
		sp = esp->e_slots[slot];
		if (sp && (sp->cmd_flags & CFLAG_CMDDISC) == 0) {
			found++;
		} else {
			slot = NEXTSLOT(slot);
		}
	} while (found == 0 && slot != start_slot);

	if (!found) {
		return (FALSE);
	}
	esp->e_cur_slot = slot;

	if ((dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
		dk_busy |= (1<<dkn);
		dk_xfer[dkn]++;
		if ((sp->cmd_flags & CFLAG_DMASEND) == 0)
			dk_read[dkn]++;
		dk_wds[dkn] += sp->cmd_dmacount >> 6;
	}
	return (esp_startcmd(esp));
}


/*
 * Start a command off
 */

static int
esp_startcmd(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	register struct dmaga *dmar = esp->e_dma;
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	int legal_cmd_len, i;
	u_char cmd, nstate, tshift, target;
	register caddr_t tp = (caddr_t) esp->e_cmdarea;

	/*
	 * The only reason that this should happen
	 * is if we have a re-selection attempt starting.
	 */

	if (INTPENDING(esp)) {
		ESP_PREEMPT(esp);
		LOG_STATE(ACTS_PREEMPTED, esp->e_stat, Tgt(sp), Lun(sp));
		(void) esp_poll();
		return (FALSE);
	}

	esp->e_sdtr = esp->e_omsglen = 0;
	target = Tgt(sp);
	tshift = 1<<target;
#ifdef	ESPDEBUG
	esp->e_xfer = sp->cmd_dmacount;
#endif	/* ESPDEBUG */

	/*
	 * The ESP chip will only automatically
	 * send 6, 10 or 12 byte SCSI cmds.
	 */

	switch (legal_cmd_len = sp->cmd_cdblen) {
	case CDB_GROUP0:
	case CDB_GROUP1:
	case CDB_GROUP5:
		break;
	default:
		legal_cmd_len = 0;
		break;
	}

	if (((esp->e_targets & tshift) == 0) || (esp->e_nodisc & tshift) ||
	    (sp->cmd_pkt.pkt_flags & FLAG_NODISCON) ||
	    (scsi_options & SCSI_OPTIONS_DR) == 0) {
		cmd = CMD_SEL_NOATN;
		nstate = STATE_SELECT_NOATN;
	} else {
		/*
		 * Okay, we're at least going to send an identify message.
		 */

		*tp++ = MSG_DR_IDENTIFY | Lun(sp);

		if (sp->cmd_flags & CFLAG_CMDPROXY) {
			/*
			 * This is a proxy command. It will have
			 * a message to send as part of post-selection
			 * (e.g, MSG_ABORT or MSG_DEVICE_RESET)
			 */

			/*
			 * XXX: We should check to make sure that
			 * this is a valid PROXY command, i.e,
			 * a  valid message length.
			 */

			esp->e_omsglen = sp->cmd_cdb[ESP_PROXY_DATA];
			i = 0;
			while (i < esp->e_omsglen) {
				esp->e_cur_msgout[i] =
				    sp->cmd_cdb[ESP_PROXY_DATA+1+i];
				i++;
			}
			sp->cmd_cdb[ESP_PROXY_RESULT] = FALSE;
			cmd = CMD_SEL_STOP;
			nstate = STATE_SELECT_N_SENDMSG;
			legal_cmd_len = 0;
			LOG_STATE(ACTS_PROXY, esp->e_stat,
				esp->e_cur_msgout[0], nstate);
		} else if ((esp->e_sync_known & tshift) ||
		    (scsi_options & SCSI_OPTIONS_SYNC) == 0) {
			if (legal_cmd_len) {
				cmd = CMD_SEL_ATN;
				nstate = STATE_SELECT_NORMAL;
			} else {
				cmd = CMD_SEL_STOP;
				nstate = STATE_SELECT_N_STOP;
			}
		} else {
			/*
			 * Set up to send an extended message after
			 * we select and send the identify message.
			 */

			if (esp->e_weak & tshift) {
				esp_make_sdtr(esp, 0, 0);
			} else {
				esp_make_sdtr(esp,
				    (int) esp->e_default_period,
				    (int) DEFAULT_OFFSET);
			}
			legal_cmd_len = 0;
			cmd = CMD_SEL_STOP;
			nstate = STATE_SELECT_N_SENDMSG;
			LOG_STATE(ACTS_SYNCHOUT, esp->e_stat,
				esp->e_default_period, DEFAULT_OFFSET);
			/*
			 * XXX: Set sync known here because the Sony CDrom
			 * ignores the synch negotiation msg. Net effect
			 * is we negotiate on every I/O request forever.
			 */
			esp->e_sync_known |= (1<<target);
		}
	}

	/*
	 * If we are a ESP100A or a ESP236, and in SCSI-2 mode
	 * we could also load another message here.
	 */

	/*
	 * Now load cdb (if any)
	 */

	for (i = 0; i < legal_cmd_len; i++) {
		*tp++ = sp->cmd_cdbp[i];
	}
	if (legal_cmd_len) {
		LOG_STATE(ACTS_CMD_START, esp->e_stat, sp->cmd_cdbp[0], nstate);
	}

	/*
	 * calculate total dma amount:
	 */

	esp->e_lastcount = ((u_long) tp) - ((u_long) esp->e_cmdarea);

	/*
	 * load rest of chip registers
	 */

	ep->esp_busid = target;
	ep->esp_sync_period = esp->e_period[target] & SYNC_PERIOD_MASK;
	ep->esp_sync_offset = esp->e_offset[target] | esp->e_req_ack_delay;
	if ((esp->e_type == FAS236) || (esp->e_type == FAS100A)) {
		ep->esp_conf3 = esp->e_espconf3[target];
	}


	if ((scsi_options & SCSI_OPTIONS_PARITY) &&
	    (sp->cmd_pkt.pkt_flags & FLAG_NOPARITY)) {
		ep->esp_conf = esp->e_espconf & ~ESP_CONF_PAREN;
	}

	SET_ESP_COUNT(ep, esp->e_lastcount);
	if (DMAGA_REV(dmar) == ESC1_REV1) {
		SET_DMAESC_COUNT(dmar, esp->e_lastcount);
	}
	dmar->dmaga_addr = esp->e_lastdma =
	    (((caddr_t)esp->e_cmdarea) - DVMA) | esp->e_dma_base;
	if (DMAGA_REV(dmar) == ESC1_REV1) {
		dmar->dmaga_csr |= (dmar->dmaga_csr & ~DMAGA_WRITE) |
			(DMAGA_INTEN|DMAGA_ENDVMA);
	} else {
		dmar->dmaga_csr = (dmar->dmaga_csr & ~DMAGA_WRITE) |
			(DMAGA_INTEN|DMAGA_ENDVMA);
	}
	if (dmar->dmaga_csr & DMAGA_INT_MASK) {
		ESP_PREEMPT(esp);
		LOG_STATE(ACTS_PREEMPTED, esp->e_stat, Tgt(sp), Lun(sp));
		(void) esp_poll();
		return (FALSE);
	}
	ep->esp_cmd = cmd | CMD_DMA;

	New_state(esp, nstate);
	LOG_STATE(nstate, esp->e_stat, target, Lun(sp));
#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf(esp, "sel %d.%d cmd[ ", target, Lun(sp));
		for (i = 0; i < sp->cmd_cdblen; i++)
			printf("0x%x ", sp->cmd_cdbp[i]&0xff);
		printf("]\n\tstate=%s\n", esp_state_name(esp->e_state));
	}
#endif	/* ESPDEBUG */
	return (TRUE);
}

static int
esp_link_start(esp)
register struct esp *esp;
{
	esp->e_nlinked++;
	if ((esp->e_stat & ESP_PHASE_MASK) != ESP_PHASE_COMMAND) {
		esp->e_stat = esp->e_reg->esp_stat & ~ESP_STAT_RES;
	}
	if ((esp->e_stat & ESP_PHASE_MASK) != ESP_PHASE_COMMAND) {
		/*
		 * XXX need a way to mark this particular failure
		 */

		esp_printstate(esp, "Linked command not in command phase");

		/*
		 * XXX: Needs to be 'soft' abort
		 */

		return (ACTION_ABORT_CURCMD);
	}

	/*
	 * we'll let esp_phasemanage do the dirty work from here on out...
	 */
	New_state(esp, ACTS_CMD_START);
	return (ACTION_PHASEMANAGE);
}

/*
 * Finish routines
 */

static int
esp_finish(esp)
register struct esp *esp;
{
	short last_slot;
	register int span_states = 0;
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register action;
	char dkn;


	if ((dkn = sp->cmd_pkt.pkt_pmon) >= 0) {
		dk_busy &= ~(1<<dkn);
	}
	last_slot = esp->e_last_slot = esp->e_cur_slot;
	esp->e_cur_slot = UNDEFINED;
	esp->e_ncmds -= 1;

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf(esp, "%d.%d; cmds=%d disc=%d npoll=%d; lastmsg 0x%x\n",
			Tgt(sp), Lun(sp), esp->e_ncmds, esp->e_ndisc,
			esp->e_npolling, esp->e_last_msgin);
		printf("\treason '%s'; cmd state 0x%b\n",
			scsi_rname(sp->cmd_pkt.pkt_reason),
			sp->cmd_pkt.pkt_state, state_bits);
	}
#endif	/* ESPDEBUG */

	if (esp->e_last_msgin == MSG_LINK_CMPLT ||
	    esp->e_last_msgin == MSG_LINK_CMPLT_FLAG) {
		span_states++;
	}

	/*
	 * XXX: ELSE WE NEED TO SET SPAN_STATES FOR COMMANDS COMPLETING
	 * XXX: WITH A CHECK CONDITION.
	 */

	if ((sp->cmd_pkt.pkt_state & STATE_GOT_STATUS) &&
	    ((struct scsi_status *) sp->cmd_pkt.pkt_scbp)->sts_chk) {
		/*
		 * In the case that we are getting a check condition
		 * clear our knowledge of synchronous capabilities.
		 * This will unambiguously force a renegotiation
		 * prior to any possible data transfer (we hope),
		 * including the data transfer for a UNIT ATTENTION
		 * condition generated by somebody powering on and
		 * off a target.
		 */
		if (esp->e_offset[Tgt(sp)] != 0) {
			esp->e_sync_known &= ~(1<<Tgt(sp));
		}
	}


	if (sp->cmd_pkt.pkt_state & STATE_XFERRED_DATA) {
		register struct dataseg *segtmp = sp->cmd_subseg.d_next;
		register i;

		/*
		 * XXX : FIX ME NEED TO MERGE TOGETHER OVERLAPPING AND
		 * XXX : ADJACENT SEGMENTS!!!
		 */
		if (segtmp != (struct dataseg *) 0 && segtmp->d_count) {
			panic("esp_finish: more than one segment with data");
			/* NOTREACHED */
		}

		/*
		 * Walk through all data segments and count up transfer counts
		 */

		i = 0;
		segtmp = &sp->cmd_subseg;
		while (segtmp) {
			i += segtmp->d_count;
			segtmp = segtmp->d_next;
		}

		sp->cmd_pkt.pkt_resid = sp->cmd_dmacount - i;

#ifdef	ESPDEBUG
		if (INFORMATIVE && sp->cmd_pkt.pkt_resid) {
			eprintf(esp, "%d.%d finishes with %d resid\n",
				Tgt(sp), Lun(sp), sp->cmd_pkt.pkt_resid);
		}
#endif	/* ESPDEBUG */
	}

	New_state(esp, STATE_FREE);
	esp->e_slots[last_slot] = (struct scsi_cmd *) 0;

	if (sp->cmd_pkt.pkt_flags & FLAG_NOINTR) {
		esp->e_npolling -= 1;
		(*sp->cmd_pkt.pkt_comp)(sp);
		action = ACTION_RETURN;
	} else if (span_states > 0) {
		/*
		 * If we're going to start a linked command,
		 * hold open the state of the host adapter...
		 */
		esp->e_state = ACTS_SPANNING;
		(*sp->cmd_pkt.pkt_comp)(sp);
		esp->e_state = STATE_FREE;

		/*
		 * This is we can check upon return that
		 * the target driver did the right thing...
		 *
		 * If the target driver didn't do the right
		 * thing, we have to abort the operation.
		 */

		if (esp->e_slots[last_slot] == 0) {
			/*
			 * XXX: HERE IS WHERE WE NEED A PROXY COMMAND IN
			 * XXX: IN ORDER TO SEND AN ABORT OPERATION MESSAGE
			 * XXX: TO THE TARGET!!!
			 */
			esplog(esp, LOG_ERR,
			    "linked command not started by driver");
			action = ACTION_RESET;
		} else {
			esp->e_cur_slot = last_slot;
			action = esp_link_start(esp);
		}
	} else {
		(*sp->cmd_pkt.pkt_comp)(sp);
		action = ACTION_SEARCH;
	}
	return (action);
}

/*
 * Interrupt Service Section
 */

/*
 * Poll for command completion (i.e., no interrupts)
 * time is in usec (and will not be very accurate)
 */

static int
esp_dopoll(esp, limit)
register struct esp *esp;
int limit;
{
	register int i;


	/*
	 * timeout is not very accurate since we don't know how
	 * long the poll takes
	 * also if the packet gets started fairly late, we may
	 * timeout prematurely
	 */

	if (limit == 0)
		limit = POLL_TIMEOUT;

	for (i = 0; i < limit; i += 100) {
		if (esp_poll() == 0)
			DELAY(100);
		if (esp->e_state == STATE_FREE)
			break;
	}

	if (i >= limit && esp->e_state != STATE_FREE) {
		esp_printstate(esp, "polled command timeout");
		return (-1);
	}
	return (0);
}


/*
 * Autovector Interrupt Entry Point.
 */

static int
esp_poll()
{
	register struct esp *esp;
	register int serviced = 0;
	register int search = TRUE;

	if (esp_softc == (struct esp *) 0)
		return (serviced);

	while (search == TRUE) {
		search = FALSE;
		for (esp = esp_softc; esp != (struct esp *) 0;
		    esp = esp->e_next) {
			if (esp->e_tran.tran_start) {
				register s = splr(esp->e_tran.tran_spl);
#ifdef	sun4m
				/*
				 * If s > e_tran.tran_spl, it must be either
				 * boot or dump time, or a polled command,
				 * so just do it!
				 */
				if ((s <= esp->e_tran.tran_spl) &&
				    (esp->e_npolling == 0)) {
					if (s != esp->e_spl) {
						(void) splx(s);
						continue;
					}
				}
#endif /* sun4m */
				if (INTPENDING(esp)) {
					espsvc(esp);
					serviced |= (1<<(CNUM));
					search = TRUE;
				}
				(void) splx(s);
			}
		}
	}

	if (serviced) {
		/*
		 * Did we service more than one host
		 * adapter for this hardware interrupt?
		 */
		if (serviced & (serviced-1)) {
			esp_nmultsvc++;
		}
		/*
		 * Record the fact that we fielded a hard interrupt
		 */
		esp_nhardints++;
		serviced = 1;
	}

	return (serviced);
}

#ifdef	VECTORED_INTERRUPTS

/*
 * Vectored interrupt entry point
 */

static int
esp_intr(esp)
struct esp *esp;
{
	register s = splr(esp->e_tran.tran_spl);
	if (INTPENDING(esp)) {
		espsvc(esp);
	}
	(void) splx(s);
}
#endif	/* VECTORED_INTERRUPTS */

/*
 * General interrupt service routine.
 */

static void
espsvc(esp)
register struct esp *esp;
{
	static int (*evec[])() = {
		esp_finish_select,
		esp_reconnect,
		esp_phasemanage,
		esp_finish,
		esp_reset_recovery,
		esp_istart,
		esp_abort_curcmd,
		esp_abort_allcmds,
		esp_reset_bus,
		esp_handle_selection
	};
	register int action;
	register u_char intr;
	register struct espreg *ep = esp->e_reg;
	register struct dmaga *dmar = esp->e_dma;

	/*
	 * Make sure that the DMAGA is *not* still active.
	 * If we don't disable further interrupts, we
	 * get Bus timeouts in trying to access the ESP
	 * chip for Rev-1 DMA gate arrays.
	 */

	if (DMAGA_REV(dmar) == DMA_REV1) {
		dmar->dmaga_csr &= ~DMAGA_INTEN;
	}

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf (esp, "dma csr 0x%b addr 0x%x\n",
		    esp->e_dma->dmaga_csr, dmaga_bits, esp->e_dma->dmaga_addr);
	}
#endif	/* ESPDEBUG */

	/*
	 * A read of ESP interrupt register clears interrupt,
	 * so any other volatile information needs to be latched
	 * up prior to reading the interrupt register.
	 */


	/*
	 * Even if we aren't in STATE_SELECTING, latch
	 * up the step register (for future handling
	 * of being in target mode operation)
	 */

	esp->e_step = (ep->esp_step & ESP_STEP_MASK);
	esp->e_stat = ep->esp_stat;
	esp->e_intr = intr = ep->esp_intr;

	/*
	 * unclear what could cause a gross error;
	 * most of the time we get a data overrun after this.
	 */
	if (esp->e_stat & ESP_STAT_GERR) {
		esplog(esp, LOG_ERR,
		    "gross error in esp status (%x)", esp->e_stat);
		IPRINTF5(
		    "esp_cmd=%x, stat=%x, intr=%x, step=%x, fifoflag=%x\n",
		    ep->esp_cmd, esp->e_stat, esp->e_intr, esp->e_step,
		    ep->esp_fifo_flag);
		if (esp->e_cur_slot != UNDEFINED) {
			struct scsi_cmd *sp = CURRENT_CMD(esp);
			if (sp->cmd_pkt.pkt_reason == CMD_CMPLT) {
				sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
			}
		} else {
			action = ACTION_ABORT_ALLCMDS;
			goto start_action;
		}
	}

	/*
	 * was there a dmaga error that caused espsvc() to be called?
	 */
	if (esp->e_dma->dmaga_csr & DMAGA_ERRPEND) {
		/*
		 * It would be desirable to set the ATN* line and attempt to
		 * do the whole schmear of INITIATOR DETECTED ERROR here,
		 * but that is too hard to do at present.
		 */
		esplog(esp, LOG_ERR, "Unrecoverable DMA error on dma");
		if (esp->e_cur_slot != UNDEFINED) {
			struct scsi_cmd *sp = CURRENT_CMD(esp);
			if (sp->cmd_pkt.pkt_reason == CMD_CMPLT)
				sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
			action = ACTION_RESET;
			goto start_action;
		}
	}



	/*
	 * While some documentation claims that the
	 * ESP100A's msb in the stat register is an
	 * INTERRUPT PENDING bit, an errata sheet
	 * warned that you shouldn't depend on that
	 * being so (unless you're an ESP-236)
	 */

	if (esp->e_type != ESP236) {
		esp->e_stat &= ~ESP_STAT_RES;
	}

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		esp_stat_int_print(esp);
		printf("\tState %s Laststate %s\n",
			esp_state_name(esp->e_state),
			esp_state_name(esp->e_laststate));
	}
#endif	/* ESPDEBUG */

	/*
	 * Based upon the current state of the host adapter driver
	 * we should be able to figure out what to do with an interrupt.
	 * We have several possible interrupt sources, some of them
	 * modified by various status conditions.
	 *
	 * Basically, we'll get an interrupt through the dma gate array
	 * for one or more of the following three conditions:
	 *
	 *	1. The ESP is asserting an interrupt request.
	 *
	 *	2. The dma gate array counter has reached zero
	 *	   and TERMINAL COUNT interrupts have been enabled.
	 *
	 *	3. There has been a memory exception of some kind.
	 *
	 * In the latter two cases we are either in one of the SCSI
	 * DATA phases or are using dma in sending a command to a
	 * target. We will let the various handlers for these kind
	 * of states decode any error conditions in the gate array.
	 *
	 * The ESP asserts an interrupt with one or more of 8 possible
	 * bits set in its interrupt register. These conditions are
	 * SCSI bus reset detected, an illegal command fed to the ESP,
	 * one of DISCONNECT, BUS SERVICE, FUNCTION COMPLETE conditions
	 * for the ESP, a Reselection interrupt, or one of Selection
	 * or Selection with Attention.
	 *
	 * Of these possible interrupts, we can deal with some right
	 * here and now, irrespective of the current state of the driver.
	 *
	 */

	if (intr & ESP_INT_RESET) {

		/*
		 * If we detect a SCSI reset, we blow away the current
		 * command (if there is one) and all disconnected commands
		 * because we now don't know the state of them at all.
		 */

		action = ACTION_FINRST;

	} else if (intr & ESP_INT_ILLEGAL) {

		/*
		 * This should not happen. The one situation where
		 * we can get an ILLEGAL COMMAND interrupt is due to
		 * a bug in the ESP100 during reselection which we
		 * should be handling in esp_reconnect().
		 */

		esp_printstate (esp, "ILLEGAL bit set");
		action = ACTION_ABORT_CURCMD;

	} else if (intr & (ESP_INT_SEL|ESP_INT_SELATN)) {

		action = ACTION_SELECT;

	} else if (intr & ESP_INT_RESEL) {

		if (esp->e_state & STATE_SELECTING) {
			action = ACTION_FINSEL;
		} else if (esp->e_state != STATE_FREE) {
			/*
			 * this 'cannot happen'.
			 */
			esp_printstate(esp, "illegal reselection");
			action = ACTION_RESET;
		} else {
			action = ACTION_RESEL;
		}

	} else {
		/*
		 * The rest of the reasons for an interrupt, including
		 * interrupts just from the dma gate array itself, can
		 * be handled based purely on the state that the driver
		 * is currently in now.
		 */

		if (esp->e_state & STATE_SELECTING) {
			action = ACTION_FINSEL;
		} else if (esp->e_state & STATE_ITPHASES) {
			action = ACTION_PHASEMANAGE;
		} else {
#ifdef	ESPDEBUG
			if (INFORMATIVE) {
				esp_printstate(esp, "spurious interrupt");
			}
#endif	/* ESPDEBUG */
			esplog(esp, LOG_ERR, "spurious interrupt");
			action = ACTION_RETURN;
		}
	}

start_action:
	while (action != ACTION_RETURN) {
		action = (*evec[action])(esp);
	}

	/*
	 * Reenable interrupts for the DMA gate array
	 */

	if (DMAGA_REV(dmar) == DMA_REV1) {
		dmar->dmaga_csr |= DMAGA_INTEN;
	}
}

/*
 * Manage phase transitions.
 */

static int
esp_phasemanage(esp)
struct esp *esp;
{
	register u_char state;
	register int action;
	static int (*pvecs[])() = {
		esp_handle_cmd_start,
		esp_handle_cmd_done,
		esp_handle_msg_out,
		esp_handle_msg_out_done,
		esp_handle_msg_in,
		esp_handle_more_msgin,
		esp_handle_msg_in_done,
		esp_handle_clearing,
		esp_handle_data,
		esp_handle_data_done,
		esp_handle_c_cmplt
	};

	do {
		state = esp->e_state;
		if (state == ACTS_UNKNOWN) {
			action = esp_handle_unknown(esp);
		} else if (state == STATE_FREE || state > ACTS_ENDVEC) {
			esplog(esp, LOG_ERR, "lost state in phasemanage");
			action = ACTION_ABORT_ALLCMDS;
		} else {
			action = (*pvecs[state-1]) (esp);
		}

	} while (action == ACTION_PHASEMANAGE);

	return (action);
}

static int
esp_handle_unknown(esp)
register struct esp *esp;
{
	register u_char newstate;

	if (esp->e_intr & ESP_INT_DISCON) {

		/*
		 * Okay. What to do now? Let's try (for the time being)
		 * assuming that the target went south and dropped busy,
		 * as a disconnect implies that either we received
		 * a completion or a disconnect message, or that we
		 * had sent an ABORT OPERATION or BUS DEVICE RESET
		 * message. In either case, we expected the disconnect
		 * and should have fielded it elsewhere.
		 *
		 * If we see a chip disconnect here, this is an unexpected
		 * loss of BSY*. Clean up the state of the chip and return.
		 *
		 */

#ifdef	ESPDEBUG
		if (INFORMATIVE) {
			esp_printstate(esp, "unexpected bus free");
		}
#endif	/* ESPDEBUG */

		esp_chip_disconnect(esp);

		CURRENT_CMD(esp)->cmd_pkt.pkt_reason = CMD_UNX_BUS_FREE;
		LOG_STATE(ACTS_CMD_LOST, esp->e_stat, esp->e_xfer, -1);
		return (ACTION_FINISH);
	}

	switch (esp->e_stat & ESP_PHASE_MASK) {
	case ESP_PHASE_DATA_IN:
	case ESP_PHASE_DATA_OUT:
		newstate = ACTS_DATA;
		break;
	case ESP_PHASE_MSG_OUT:
		newstate = ACTS_MSG_OUT;
		break;
	case ESP_PHASE_MSG_IN:
		newstate = ACTS_MSG_IN;
		break;
	case ESP_PHASE_STATUS:
		esp->e_reg->esp_cmd = CMD_FLUSH;
#ifdef	ESP_TEST_PARITY
		if (esp_ptest_status & (1<<Tgt(CURRENT_CMD(esp)))) {
			esp->e_reg->esp_cmd = CMD_SET_ATN;
		}
#endif	/* ESP_TEST_PARITY */
		esp->e_reg->esp_cmd = CMD_COMP_SEQ;
		esp->e_state = ACTS_C_CMPLT;
		LOG_STATE(ACTS_C_CMPLT, esp->e_stat, -1, -1);
		return (ACTION_RETURN);
	case ESP_PHASE_COMMAND:
		newstate = ACTS_CMD_START;
		break;
	default:
		esp_printstate(esp, "Unknown bus phase");
		return (ACTION_RESET);
	}
	esp->e_state = newstate;
	return (ACTION_PHASEMANAGE);
}

static int
esp_handle_cmd_start(esp)
struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	struct scsi_cmd *sp = CURRENT_CMD(esp);
	register u_long cmd_distance;

	/*
	 * Check for command overflow.
	 */

	cmd_distance = (u_long) sp->cmd_cdbp - (u_long) sp->cmd_pkt.pkt_cdbp;
	if (cmd_distance > (u_long) sp->cmd_cdblen) {
		sp->cmd_pkt.pkt_reason = CMD_CMD_OVR;
		return (ACTION_ABORT_CURCMD);
	}
	if (cmd_distance == 0) {
		LOG_STATE(ACTS_CMD_START, esp->e_stat, sp->cmd_cdbp[0], -1);
	}

	/*
	 * Stuff next command byte into fifo
	 */

	ep->esp_cmd = CMD_FLUSH;

	SET_ESP_COUNT(ep, 1);
	ep->esp_fifo_data = *(sp->cmd_cdbp++);
	ep->esp_cmd = CMD_TRAN_INFO;

	New_state(esp, ACTS_CMD_DONE);
	return (ACTION_RETURN);
}

static int
esp_handle_cmd_done(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register u_char intr = esp->e_intr;

	/*
	 * The NOP command is required following a COMMAND
	 * or MESSAGE OUT phase in order to unlatch the
	 * FIFO flags register. This is needed for all
	 * ESP chip variants.
	 */

	esp->e_reg->esp_cmd = CMD_NOP;

	/*
	 * We should have gotten a BUS SERVICE interrupt.
	 * If it isn't that, and it isn't a DISCONNECT
	 * interrupt, we have a "cannot happen" situation.
	 */

	if ((intr & ESP_INT_BUS) == 0) {
		if ((intr & ESP_INT_DISCON) == 0) {
			esp_printstate(esp, "cmd transmission error");
			return (ACTION_ABORT_CURCMD);
		}
	} else {
		sp->cmd_pkt.pkt_state |= STATE_SENT_CMD;
	}
	New_state(esp, ACTS_UNKNOWN);
	return (ACTION_PHASEMANAGE);
}

/*
 * Begin to send a message out
 */

static int
esp_handle_msg_out(esp)
register struct esp *esp;
{
	register i;
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register struct espreg *ep = esp->e_reg;

	/*
	 * Check to make *sure* that we are really
	 * in MESSAGE OUT phase. If the last state
	 * was ACTS_MSG_OUT_DONE, then we are trying
	 * to resend a message that the target stated
	 * had a parity error in it.
	 *
	 * If this is the case, and mark completion reason as CMD_NOMSGOUT.
	 * XXX: Right now, we just *drive* on. Should we abort the command?
	 */

	if ((esp->e_stat & ESP_PHASE_MASK) != ESP_PHASE_MSG_OUT &&
	    esp->e_laststate == ACTS_MSG_OUT_DONE) {
		esplog(esp, LOG_WARNING,
		    "Target %d refused message resend", Tgt(sp));
		sp->cmd_pkt.pkt_reason = CMD_NOMSGOUT;
		New_state(esp, ACTS_UNKNOWN);
		return (ACTION_PHASEMANAGE);
	}

	/*
	 * Clean the fifo.
	 */

	ep->esp_cmd = CMD_FLUSH;

	/*
	 * See if we have a valid message to send
	 */

	if (esp->e_omsglen == 0) {
		/*
		 * no valid message? Send a no-op anyway...
		 */
		esp->e_cur_msgout[0] = MSG_NOP;
		esp->e_omsglen = 1;
	}

	for (i = 0; i < esp->e_omsglen; i++) {
		ep->esp_fifo_data = esp->e_cur_msgout[i];
	}
	ep->esp_cmd = CMD_TRAN_INFO;

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf(esp, "msg out: %s", scsi_mname(esp->e_cur_msgout[0]));
		for (i = 1; i < esp->e_omsglen; i++)
			printf(" %x", esp->e_cur_msgout[i]);
		printf("\n");
	}
#endif	/* ESPDEBUG */

	New_state(esp, ACTS_MSG_OUT_DONE);
	return (ACTION_RETURN);
}

static int
esp_handle_msg_out_done(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register struct espreg *ep = esp->e_reg;
	register u_char msgout, phase, fifocnt;
	register int target = Tgt(sp);

	msgout = esp->e_cur_msgout[0];

	/*
	 * If the ESP disconnected, then the message we sent caused
	 * the target to decide to drop BSY* and clear the bus.
	 */

	if (esp->e_intr == ESP_INT_DISCON) {
		if (msgout == MSG_DEVICE_RESET || msgout == MSG_ABORT) {
			esp_chip_disconnect(esp);
			if (msgout == MSG_DEVICE_RESET) {
				esp->e_offset[target] = 0;
				esp->e_sync_known &= ~(1<<target);
			}
			EPRINTF2 ("Succesful %s message to target %d\n",
			    scsi_mname(msgout), target);
			sp->cmd_pkt.pkt_reason = CMD_CMPLT;
			if (sp->cmd_flags & CFLAG_CMDPROXY) {
				sp->cmd_cdb[ESP_PROXY_RESULT] = TRUE;
			}
			return (ACTION_FINISH);
		}
		/*
		 * If the target dropped busy on any other message, it
		 * wasn't expected. We will let the code in esp_phasemanage()
		 * handle this unexpected bus free event.
		 */

		goto out;
	}

	/*
	 * What phase have we transitioned to?
	 */

	phase = esp->e_stat & ESP_PHASE_MASK;

	/*
	 * Save current fifo count
	 */

	fifocnt = FIFO_CNT(ep);

	/*
	 * As per the ESP errata sheets, this must be done for
	 * all ESP chip variants.
	 *
	 * This releases the FIFO counter from its latched state.
	 * Note that we read the fifo counter above prior to doing
	 * this.
	 */

	ep->esp_cmd = CMD_NOP;

	/*
	 * Clean the fifo? Yes, if and only if we haven't
	 * transitioned to Synchronous DATA IN phase.
	 * The ESP chip manual notes that in the case
	 * that the target has shifted to Synchronous
	 * DATA IN phase, that while the FIFO count
	 * register stays latched up with the number
	 * of bytes not transferred out, that the fifo
	 * itself is cleared and will contain only
	 * the incoming data bytes.
	 *
	 * The manual doesn't state what happens in
	 * other receive cases (transition to STATUS,
	 * MESSAGE IN, or asynchronous DATA IN phase),
	 * but I'll assume that there is probably
	 * a single-byte pad between the fifo and
	 * the SCSI bus which the ESP uses to hold
	 * the currently asserted data on the bus
	 * (known valid by a true REQ* signal). In
	 * the case of synchronous data in, up to
	 * 15 bytes of data could arrive, so the
	 * ESP must have to make room for by clearing
	 * the fifo, but in other cases it can just
	 * hold the current byte until the next
	 * ESP chip command that would cause a
	 * data transfer.
	 */

	if (fifocnt != 0 && (phase != ESP_PHASE_DATA_IN ||
	    esp->e_offset[target] == 0)) {
		ep->esp_cmd = CMD_FLUSH;
	}

	/*
	 * If we finish sending a message out, and we are
	 * still in message out phase, then the target has
	 * detected one or more parity errors in the message
	 * we just sent and it is asking us to resend the
	 * previous message.
	 */

	if ((esp->e_intr & ESP_INT_BUS) && phase == ESP_PHASE_MSG_OUT) {
		/*
		 * As per SCSI-2 specification, if the message to
		 * be re-sent is greater than one byte, then we
		 * have to set ATN*.
		 */
		if (esp->e_omsglen > 1) {
			ep->esp_cmd = CMD_SET_ATN;
		}
		esplog(esp, LOG_ERR,
		    "SCSI bus MESSAGE OUT phase parity error");
		sp->cmd_pkt.pkt_statistics |= STAT_PERR;
		New_state(esp, ACTS_MSG_OUT);
		return (ACTION_PHASEMANAGE);
	}

	/*
	 * Count that we sent a SYNCHRONOUS DATA TRANSFER message.
	 */

	if (esp->e_omsglen == 5 && msgout == MSG_EXTENDED &&
	    esp->e_cur_msgout[2] == MSG_SYNCHRONOUS) {
		esp->e_sdtr++;
	}

out:
	esp->e_last_msgout = msgout;
	esp->e_omsglen = 0;
	New_state(esp, ACTS_UNKNOWN);
	return (ACTION_PHASEMANAGE);
}

static int
esp_handle_clearing(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register action;
	register u_char lmsg = esp->e_last_msgin;

	if (esp->e_intr != ESP_INT_DISCON) {
		if (lmsg != MSG_LINK_CMPLT && lmsg != MSG_LINK_CMPLT_FLAG) {

			/*
			 * If the chip/target didn't disconnect from the
			 * bus, that is a gross fatal error.
			 */

			esplog (esp, LOG_WARNING,
			    "Target %d didn't disconnect after sending %s",
			    Tgt(sp), scsi_mname(lmsg));
			sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
			return (ACTION_ABORT_CURCMD);
		} else {
			/*
			 * In this case, the last message in was a 'linked
			 * command complete' message and the target stays
			 * connected. We don't fiddle with any of the
			 * settings on the ESP chip. We return state
			 * such that the completing command is finished
			 * up and depend upon the finish routine to
			 * handle the case that a new command has to
			 * be available to start right away for this target.
			 */

			esp->e_last_msgout = 0xff;
			esp->e_omsglen = 0;
			return (ACTION_FINISH);
		}
	}

	/*
	 * At this point the ESP chip has disconnected. The bus should
	 * be either quiet or someone may be attempting a reselection
	 * of us (or somebody else). Call the routine the sets the
	 * chip back to a correct and known state.
	 */

	esp_chip_disconnect(esp);

	/*
	 * If the last message in was a disconnect, search
	 * for new work to do, else return to call esp_finish()
	 */

	if (lmsg == MSG_DISCONNECT) {
		sp->cmd_pkt.pkt_statistics |= STAT_DISCON;
		sp->cmd_flags |= CFLAG_CMDDISC;
		esp->e_disconnects++;
		esp->e_ndisc++;
		New_state(esp, STATE_FREE);
		esp->e_last_slot = esp->e_cur_slot;
		esp->e_cur_slot = UNDEFINED;
		action = ACTION_SEARCH;
		EPRINTF2 ("disconnecting %d.%d\n", Tgt(sp), Lun(sp));
	} else {
		action = ACTION_FINISH;
	}
	esp->e_last_msgout = 0xff;
	esp->e_omsglen = 0;
	return (action);
}

static int
esp_handle_data(esp)
struct esp *esp;
{
	register i;
	struct espreg *ep = esp->e_reg;
	struct dmaga *dmar = esp->e_dma;
	struct scsi_cmd *sp = CURRENT_CMD(esp);
	register int sending;

	if (IS_53C90(esp)) {
		ep->esp_cmd = CMD_NOP;	/* per ESP errata sheet */
	}

	if ((sp->cmd_flags & CFLAG_DMAVALID) == 0) {
		esp_printstate(esp, "unexpected data phase");
		/*
		 * XXX: This isn't the right reason
		 */
		sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
		return (ACTION_ABORT_CURCMD);
	} else {
		sending = (sp->cmd_flags & CFLAG_DMASEND)? 1 : 0;
	}

	if (sp->cmd_flags & CFLAG_NEEDSEG) {
		/*
		 * We can currently handle truncating the current
		 * subsegment (in case a restore pointers is followed
		 * by a re-entry into data phase). XXX Else we die
		 * horribly XXX
		 */
		register struct dataseg *segp = sp->cmd_cursubseg;
		IPRINTF3 ("data new seg: datap 0x%x base 0x%x count 0x%x\n",
		    sp->cmd_data, segp->d_base, segp->d_count);
		if (sp->cmd_data >= segp->d_base &&
		    (sp->cmd_data < (segp->d_base + segp->d_count))) {
			/*
			 * We are backing up within the current
			 * segment. Adjust the count field of the
			 * current segment to reflect that we
			 * only got 'this far' with it to date.
			 */
			segp->d_count = sp->cmd_data - segp->d_base;
			sp->cmd_flags ^= CFLAG_NEEDSEG;
		} else {
			/*
			 * XXX: bad bad bad...
			 */
			panic("need new seg in esp");
			/* NOTREACHED */
		}
	}

	i = scsi_chkdma(sp, ESP_MAX_DMACOUNT);

	if (i == 0) {
		esp_printstate(esp, "data transfer overrun");
		sp->cmd_pkt.pkt_reason = CMD_DATA_OVR;
		/*
		 * A fix for bug id 1048141- if we get data transfer
		 * overruns, assume we have a weak scsi bus. Note that
		 * this won't catch consistent underruns or other
		 * noise related syndromes.
		 */
		esp_sync_backoff(esp, sp);
		return (ACTION_ABORT_CURCMD);
	}
	esp->e_lastcount = i;
#ifdef	ESPDEBUG
	esp->e_xfer = i;
#endif	/* ESPDEBUG */

#ifdef	sun4m
	if (sp->cmd_flags & CFLAG_DMAKEEP) {
		dmar->dmaga_addr = esp->e_lastdma =
		    (btop(sp->cmd_data) < dvmasize) ?
		    (sp->cmd_data | esp->e_dma_base) : sp->cmd_data;
	} else {
		dmar->dmaga_addr = esp->e_lastdma =
		    (btop(sp->cmd_data) < BIGSBUSMAP_SIZE) ?
		    (sp->cmd_data | BIGSBUSDVMA_BASE) : sp->cmd_data;
	}
#else	/* sun4m */
	dmar->dmaga_addr = esp->e_lastdma =
	    (btop(sp->cmd_data) < dvmasize) ?
	    (sp->cmd_data | esp->e_dma_base) : sp->cmd_data;
#endif	/* sun4m */

	SET_ESP_COUNT(ep, i);
	if (DMAGA_REV(dmar) == ESC1_REV1) {
		SET_DMAESC_COUNT(dmar, i);
	}

	EPRINTF4 ("%d.%d cmd 0x%x to xfer %x\n", Tgt(sp), Lun(sp),
	    sp->cmd_pkt.pkt_cdbp[0], i);

	if ((esp->e_stat & ESP_PHASE_MASK) == ESP_PHASE_DATA_OUT) {
		if (!sending) {
			esplog(esp, LOG_ERR,
			    "unwanted data out for Target %d", Tgt(sp));
			sp->cmd_pkt.pkt_reason = CMD_DMA_DERR;
			return (ACTION_ABORT_CURCMD);
		}
		LOG_STATE(ACTS_DATAOUT, esp->e_stat, i, -1);
	} else {
		if (sending) {
			esplog (esp, LOG_ERR,
			    "unwanted data in for Target %d", Tgt(sp));
			sp->cmd_pkt.pkt_reason = CMD_DMA_DERR;
			return (ACTION_ABORT_CURCMD);
		}
		dmar->dmaga_csr |= DMAGA_WRITE;
		LOG_STATE(ACTS_DATAIN, esp->e_stat, i, -1);
	}

#ifdef	ESP_TEST_PARITY
	if (!sending && (esp_ptest_data_in & (1<<Tgt(sp)))) {
		ep->esp_cmd = CMD_SET_ATN;
	}
#endif	/* ESP_TEST_PARITY */
	dmar->dmaga_csr |= DMAGA_ENDVMA|DMAGA_INTEN;
	ep->esp_cmd = CMD_TRAN_INFO|CMD_DMA;
	New_state(esp, ACTS_DATA_DONE);
	return (ACTION_RETURN);
}

static int
esp_handle_data_done(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	register struct dmaga *dmar = esp->e_dma;
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register u_long xfer_amt;
	char spurious_data, do_drain_fifo, was_sending;
	register u_char stat, tgt, fifoamt;


	tgt = Tgt(sp);
	stat = esp->e_stat;
	was_sending = (sp->cmd_flags & CFLAG_DMASEND) ? 1 : 0;
	spurious_data = do_drain_fifo = 0;

	/*
	 * Check for DMAGA errors (parity or memory fault)
	 */

	if (dmar->dmaga_csr & DMAGA_ERRPEND) {
		/*
		 * It would be desirable to set the ATN* line and attempt to
		 * do the whole schmear of INITIATOR DETECTED ERROR here,
		 * but that is too hard to do at present.
		 */
		esplog(esp, LOG_ERR, "Unrecoverable DMA error on dma %s",
		    (was_sending) ? "send" : "receive");
		sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
		return (ACTION_RESET);
	}

	/*
	 * Data Receive conditions:
	 *
	 * Check for parity errors. If we have a parity error upon
	 * receive, the ESP chip has asserted ATN* for us already.
	 *
	 * For Rev-1 and Rev-2 dma gate arrayts,
	 * make sure the last bytes have flushed.
	 */

	if (!was_sending) {
#ifdef	ESP_TEST_PARITY
		if (esp_ptest_data_in & (1<<tgt)) {
			esp_ptest_data_in = 0;
			stat |= ESP_STAT_PERR;
		}
#endif	/* ESP_TEST_PARITY */
		if (stat & ESP_STAT_PERR) {
			esplog (esp, LOG_ERR,
			    "SCSI bus DATA IN phase parity error");
			esp->e_cur_msgout[0] = MSG_INITIATOR_ERROR;
			esp->e_omsglen = 1;
			sp->cmd_pkt.pkt_statistics |= STAT_PERR;
		}

		xfer_amt = 0;

		/*
		 * For DMA gate arrays, the PACKCNT field of the DMA
		 * CSR register indicates how many bytes are still
		 * latched up and need to be drained to memory.
		 *
		 * For the DMA+ CSR, the PACKCNT field will either
		 * be zero or non-zero, indicating a empty/non-empty
		 * D_CACHE. The DRAIN bit has no effect.
		 *
		 * The DMA2 CSR (as of right now- 6/1/90) actually
		 * requires a write to the CSR register to force
		 * the drain of the D_CACHE, so the practice of
		 * writing the otherwise null-effect DRAIN bit is
		 * still required.
		 */

		while (DMAGA_NPACKED(dmar) != 0 && xfer_amt < 100) {
			if (DMAGA_REV(dmar) != ESC1_REV1) {
				dmar->dmaga_csr |= DMAGA_DRAIN;
			}
			DELAY(200);
			xfer_amt++;
		}

		if (xfer_amt >= 100 && DMAGA_NPACKED(dmar)) {
			esplog (esp, LOG_ERR, "DMA gate array won't drain");
			/*
			 * This isn't quite right...
			 */
			sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
			return (ACTION_ABORT_CURCMD);
		}
	}

	/*
	 * clear state of dma gate array
	 */

	dmar->dmaga_csr |= DMAGA_FLUSH;
	dmar->dmaga_csr &= ~(DMAGA_ENDVMA|DMAGA_WRITE|DMAGA_ENATC);

	/*
	/*
	 * Check to make sure we're still connected to the target.
	 * If the target dropped the bus, that is a fatal error.
	 * We don't even attempt to count what we were transferring
	 * here. Let esp_handle_unknown clean up for us.
	 */

	if (esp->e_intr != ESP_INT_BUS) {
		New_state(esp, ACTS_UNKNOWN);
		return (ACTION_PHASEMANAGE);
	}

	/*
	 * Figure out how far we got.
	 * Latch up fifo amount first.
	 */

	fifoamt = FIFO_CNT(ep);

	if (stat & ESP_STAT_XZERO) {
		xfer_amt = esp->e_lastcount;
	} else {
		GET_ESP_COUNT(ep, xfer_amt);
		xfer_amt = esp->e_lastcount - xfer_amt;
	}
	/*
	 * Unconditionally knock off by the amount left
	 * in the fifo if we were sending out the SCSI bus.
	 *
	 * If we were receiving from the SCSI bus, believe
	 * what the chip told us (either XZERO or by the
	 * value calculated from the counter register).
	 * The reason we don't look at the fifo for
	 * incoming data is that in synchronous mode
	 * the fifo may have further data bytes, and
	 * for async mode we assume that all data in
	 * the fifo will have been transferred before
	 * the esp asserts an interrupt.
	 */

	if (was_sending) {
		xfer_amt -= fifoamt;
	}

	/*
	 * If this was a synchronous transfer, flag it.
	 * Also check for the errata condition of long
	 * last REQ/ pulse for some synchronous targets
	 */

	if (esp->e_offset[tgt]) {

		/*
		 * flag that a synchronous data xfer took place
		 */

		sp->cmd_pkt.pkt_statistics |= STAT_SYNC;
		esp->e_nsync++;

		if (IS_53C90(esp)) {
			static char *spur =
			    "Spurious %s phase from target %d\n";
			u_char phase;

			/*
			 * Okay, latch up new status register value
			 */

			/*
			 * Get a new stat from the esp chip register.
			 */

			esp->e_stat = stat = phase = ep->esp_stat;
			phase &= ESP_PHASE_MASK;

			/*
			 * Now, if we're still (maybe) in a data phase,
			 * check to be real sure that we are...
			 */

			if (phase == ESP_PHASE_DATA_IN) {
				if (FIFO_CNT(ep) == 0)
					spurious_data = 1;
			} else if (phase == ESP_PHASE_DATA_OUT) {
				if ((ep->esp_fifo_flag & ESP_FIFO_ONZ) == 0)
					spurious_data = -1;
			}

			if (spurious_data) {
				ep->esp_cmd = CMD_MSG_ACPT;
				esplog (esp, LOG_ERR,
				    spur, (spurious_data < 0) ?
				    "data out": "data in", tgt);

				/*
				 * It turns out that this can also
				 * come about if the target resets
				 * (and goes back to async SCSI mode)
				 * and we don't know about it.
				 *
				 * The degenerate case for this is
				 * turning off a lunchbox- this clears
				 * it's state. The trouble is is that
				 * we'll get a check condition (likely)
				 * on the next command after a power-cycle
				 * for this target, but we'll have to
				 * go into a DATA IN phase to pick up
				 * the sense information for the Request
				 * Sense that will likely follow that
				 * Check Condition.
				 *
				 * As a temporary fix, I'll clear
				 * the 'sync_known' flag for this
				 * target so that the next selection
				 * for this target will renegotiate
				 * the sync protocol to be followed.
				 */

				esp->e_sync_known &= ~(1<<tgt);
			}
			if (spurious_data == 0 && was_sending)
				do_drain_fifo = 1;
		} else {
			/*
			 * The need to handle for the ESP100A the case
			 * of turning off/on a target, thus destroying
			 * it's sync. setting is covered in esp_finish()
			 * where a CHECK CONDITION status causes the
			 * esp->e_sync_known flag to be cleared.
			 *
			 * If we are doing synchronous DATA OUT,
			 * we should probably drain the fifo.
			 * If we are doing synchronous DATA IN,
			 * we really don't dare do that (in case
			 * we are going from data phase to data
			 * phase).
			 */

			if (was_sending)
				do_drain_fifo = 1;
		}
	} else {
		/*
		 * If we aren't doing Synchronous Data Transfers,
		 * definitely offload the fifo.
		 */
		do_drain_fifo = 1;
	}

	/*
	 * Drain the fifo here of any left over
	 * that weren't transferred (if desirable).
	 */

	if (do_drain_fifo) {
		ep->esp_cmd = CMD_FLUSH;
	}

	/*
	 * adjust pointers...
	 */

	sp->cmd_data += xfer_amt;
	sp->cmd_cursubseg->d_count += xfer_amt;

#ifdef	ESPDEBUG
	if ((scsi_options & SCSI_DEBUG_HA) || espdebug > 1 ||
	    ((espdebug > 1) &&
		esp->e_lastcount >= 0x200 && xfer_amt & 0x1ff)) {
		eprintf (esp,
		    "DATA %s phase for %d.%d did 0x%x of 0x%x bytes\n",
		    (was_sending)? "OUT" : "IN", tgt, Lun(sp),
		    xfer_amt, esp->e_lastcount);
		esp_stat_int_print(esp);
		esp_dump_datasegs(sp);
	}
#endif	/* ESPDEBUG */

	sp->cmd_pkt.pkt_state |= STATE_XFERRED_DATA;
	New_state(esp, ACTS_UNKNOWN);
	if (spurious_data == 0) {
		stat &= ESP_PHASE_MASK;
		if (stat == ESP_PHASE_DATA_IN || stat == ESP_PHASE_DATA_OUT) {
			esp->e_state = ACTS_DATA;
		}
		return (ACTION_PHASEMANAGE);
	} else {
		return (ACTION_RETURN);
	}
}

static int
esp_handle_c_cmplt(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	struct espreg *ep = esp->e_reg;
	register u_char sts, msg, msgout, intr, perr;

	intr = esp->e_intr;

#ifdef	ESP_TEST_PARITY
	if (esp_ptest_status & (1<<Tgt(sp))) {
		esp_ptest_status = 0;
		esp->e_stat |= ESP_STAT_PERR;
	} else if ((esp_ptest_msgin & (1<<Tgt(sp))) && esp_ptest_msg == 0) {
		ep->esp_cmd = CMD_SET_ATN;
		esp_ptest_msgin = 0;
		esp_ptest_msg = -1;
		esp->e_stat |= ESP_STAT_PERR;
	}
#endif	/* ESP_TEST_PARITY */

	if (intr == ESP_INT_DISCON) {
		New_state(esp, ACTS_UNKNOWN);
		return (ACTION_PHASEMANAGE);
	}

	if (perr = (esp->e_stat & ESP_STAT_PERR)) {
		sp->cmd_pkt.pkt_statistics |= STAT_PERR;
	}

	msgout = 0;
	msg = sts = 0xff;

	/*
	 * The ESP manuals state that this sequence completes
	 * with a BUS SERVICE interrupt if just the status
	 * byte was receved, else a FUNCTION COMPLETE interrupt
	 * if both status and a message was received.
	 *
	 * The manuals also state that ATN* is asserted if
	 * bad parity is detected.
	 *
	 * The one case that we cannot handle is where we detect
	 * bad parity for the status byte, but the target refuses
	 * to go to MESSAGE OUT phase right away. This means that
	 * if that happens, we will misconstrue the parity error
	 * to be for the completion message, not the status byte.
	 */

	if (intr == ESP_INT_BUS) {
		/*
		 * We only got the status byte.
		 */
		sts = ep->esp_fifo_data;
		if (perr) {
			/*
			 * If we get a parity error on a status byte
			 * assume that it was a CHECK CONDITION
			 */
			sts = STATUS_CHECK;
			esplog (esp, LOG_ERR,
			    "SCSI bus STATUS phase parity error");
			msgout = MSG_INITIATOR_ERROR;
		}
	} else {
		sts = ep->esp_fifo_data;	/* assume good status byte */
		esp->e_last_msgin = msg = ep->esp_fifo_data;
		if (perr) {
			esplog (esp, LOG_ERR, msginperr);
			msgout = MSG_MSG_PARITY;
		}
	}

	if (sts != 0xff) {
		sp->cmd_pkt.pkt_state |= STATE_GOT_STATUS;
		*(sp->cmd_scbp++) = sts;
		EPRINTF1 ("Status=0x%x\n", sts);
	}
	LOG_STATE(ACTS_STATUS, esp->e_stat, sts, -1);

	if (msgout == 0) {
		EPRINTF1 ("Completion Message=%s\n", scsi_mname(msg));
		if (msg == MSG_COMMAND_COMPLETE ||
		    msg == MSG_LINK_CMPLT ||
		    msg == MSG_LINK_CMPLT_FLAG) {
			/*
			 * Actually, if the message was a 'linked command
			 * complete' message, the target isn't going to be
			 * clearing the bus.
			 */
			New_state(esp, ACTS_CLEARING);
		} else {
			esp->e_imsglen = 1;
			esp->e_imsgindex = 1;
			New_state(esp, ACTS_MSG_IN_DONE);
			return (esp_handle_msg_in_done(esp));
		}
	} else {
		esp->e_cur_msgout[0] = msgout;
		esp->e_omsglen = 1;
		New_state(esp, ACTS_UNKNOWN);
	}
	LOG_STATE(ACTS_C_CMPLT, esp->e_stat, esp->e_xfer, -1);

	if (intr != ESP_INT_BUS) {
		ep->esp_cmd = CMD_MSG_ACPT;
		return (ACTION_RETURN);
	} else {
		return (ACTION_PHASEMANAGE);
	}
}

/*
 *
 */

static int
esp_handle_msg_in(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;

	/*
	 * Pick up a message byte.
	 * Clear the FIFO so we
	 * don't get confused.
	 */

	ep->esp_cmd = CMD_FLUSH;
	if (IS_53C90(esp)) {
		ep->esp_cmd = CMD_NOP;
	}
	ep->esp_cmd = CMD_TRAN_INFO;
	esp->e_imsglen = 1;
	esp->e_imsgindex = 0;
	New_state(esp, ACTS_MSG_IN_DONE);
	return (ACTION_RETURN);
}

/*
 * We come here after issuing a MSG_ACCEPT
 * command and are expecting more message bytes.
 * The ESP should be asserting a BUS SERVICE
 * interrupt status, but may have asserted
 * a different interrupt in the case that
 * the target disconnected and dropped BSY*.
 *
 * In the case that we are eating up message
 * bytes (and throwing them away unread) because
 * we have ATN* asserted (we are trying to send
 * a message), we do not consider it an error
 * if the phase has changed out of MESSAGE IN.
 */

static int
esp_handle_more_msgin(esp)
register struct esp *esp;
{

	if (esp->e_intr & ESP_INT_BUS) {
		if ((esp->e_stat & ESP_PHASE_MASK) == ESP_PHASE_MSG_IN) {
			/*
			 * Fetch another byte of a message in.
			 */
			esp->e_reg->esp_cmd = CMD_TRAN_INFO;
			New_state(esp, ACTS_MSG_IN_DONE);
			return (ACTION_RETURN);
		}

		/*
		 * If we were gobbling up a message and we have
		 * changed phases, handle this silently, else
		 * complain. In either case, we return to let
		 * esp_phasemanage() handle things.
		 *
		 * If it wasn't a BUS SERVICE interrupt,
		 * let esp_phasemanage() find out if the
		 * chip disconnected.
		 */

		if (esp->e_imsglen != 0) {
			esplog (esp, LOG_WARNING,
			    "Premature end of extended message");
		}
	}
	New_state(esp, ACTS_UNKNOWN);
	return (ACTION_PHASEMANAGE);
}

static int
esp_handle_msg_in_done(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register struct espreg *ep = esp->e_reg;
	register sndmsg = 0;
	register u_char msgin;

	/*
	 * We can be called here for both the case where
	 * we had requested the ESP chip to fetch a message
	 * byte from the target (at the target's request).
	 * We can also be called in the case where we had
	 * been using the CMD_COMP_SEQ command to pick up
	 * both a status byte and a completion message from
	 * a target, but where the message wasn't one of
	 * COMMAND COMPLETE, LINKED COMMAND COMPLETE, or
	 * LINKED COMMAND COMPLETE (with flag). This is a
	 * legal (albeit extremely unusual) SCSI bus trans-
	 * -ition, so we have to handle it.
	 */

	if (esp->e_laststate != ACTS_C_CMPLT) {
#ifdef	ESP_TEST_PARITY
reloop:
#endif	/* ESP_TEST_PARITY */

		if (esp->e_intr & ESP_INT_DISCON) {
			esplog (esp, LOG_ERR,
			    "premature end of input message");
			New_state (esp, ACTS_UNKNOWN);
			return (ACTION_PHASEMANAGE);
		}

		/*
		 * Note that if e_imsglen is zero, then we are skipping
		 * input message bytes, so there is no reason to look for
		 * parity errors.
		 */

		if (esp->e_imsglen != 0 && (esp->e_stat & ESP_STAT_PERR)) {

			esplog (esp, LOG_ERR, msginperr);
			sndmsg = MSG_MSG_PARITY;
			sp->cmd_pkt.pkt_statistics |= STAT_PERR;
			ep->esp_cmd = CMD_FLUSH;

		} else if ((msgin = (FIFO_CNT(ep))) != 1) {

			/*
			 * If we have got more than one byte in the fifo,
			 * that is a gross screwup, and we should let the
			 * target know that we have completely fouled up.
			 */

			sndmsg = MSG_INITIATOR_ERROR;
			ep->esp_cmd = CMD_FLUSH;
			esplog (esp, LOG_ERR, "input message botch");

		} else if (esp->e_imsglen == 0) {


			/*
			 * If we are in the middle of gobbling up and throwing
			 * away a message (due to a previous message input
			 * error), drive on.
			 */

			msgin = ep->esp_fifo_data;
			New_state(esp, ACTS_MSG_IN_MORE);

		} else {
			esp->e_imsgarea[esp->e_imsgindex++] =
			    msgin = ep->esp_fifo_data;
		}

	} else {
		/*
		 * In this case, we have been called (from
		 * esp_handle_c_cmplt()) with the message
		 * already stored in the message array.
		 */

		msgin = esp->e_imsgarea[0];
	}


	/*
	 * Process this message byte (but not if we are
	 * going to be trying to send back some error
	 * anyway)
	 */

	if (sndmsg == 0 && esp->e_imsglen != 0) {

		if (esp->e_imsgindex < esp->e_imsglen) {

			EPRINTF2 ("message byte %d: 0x%x\n",
			    esp->e_imsgindex-1,
			    esp->e_imsgarea[esp->e_imsgindex-1]);

			New_state(esp, ACTS_MSG_IN_MORE);

		} else if (esp->e_imsglen == 1) {

#ifdef	ESP_TEST_PARITY
			if ((esp_ptest_msgin & (1<<Tgt(sp))) &&
			    esp_ptest_msg == msgin) {
				esp_ptest_msgin = 0;
				esp_ptest_msg = -1;
				ep->esp_cmd = CMD_SET_ATN;
				esp->e_stat |= ESP_STAT_PERR;
				esp->e_imsgindex -= 1;
				goto reloop;
			}
#endif	/* ESP_TEST_PARITY */

			sndmsg = esp_onebyte_msg(esp);

		} else if (esp->e_imsglen == 2) {

			if (esp->e_imsgarea[0] ==  MSG_EXTENDED) {
				static char *tool =
				    "Extended message 0x%x is too long";

				/*
				 * Is the incoming message too long
				 * to be stored in our local array?
				 */

				if ((msgin+2) > IMSGSIZE) {
					esplog(esp, LOG_ERR,
					    tool, esp->e_imsgarea[0]);
					sndmsg = MSG_REJECT;
				} else {
					esp->e_imsglen = msgin + 2;
					New_state(esp, ACTS_MSG_IN_MORE);
				}
			} else {
				sndmsg = esp_twobyte_msg(esp);
			}

		} else {
			sndmsg = esp_multibyte_msg(esp);
		}
	}

	if (sndmsg < 0) {
		/*
		 * If sndmsg is less than zero, one of the subsidiary
		 * routines needs to return some other state than
		 * ACTION_RETURN.
		 */
		return (-sndmsg);
	} else if (sndmsg > 0) {
		if (IS_1BYTE_MSG(sndmsg)) {
			esp->e_omsglen = 1;
		}
		esp->e_cur_msgout[0] = (u_char) sndmsg;
		/*
		 * The target is not guaranteed to go to message out
		 * phase, period. Moreover, until the entire incoming
		 * message is transferred, the target may (and likely
		 * will) continue to transfer message bytes (which
		 * we will have to ignore).
		 *
		 * In order to do this, we'll go to 'infinite'
		 * message in handling by setting the current input
		 * message length to a sentinel of zero.
		 *
		 * This works regardless of the message we are trying
		 * to send out. At the point in time which we want
		 * to send a message in response to an incoming message
		 * we do not care any more about the incoming message.
		 *
		 * If we are sending a message in response to detecting
		 * a parity error on input, the ESP chip has already
		 * set ATN* for us, but it doesn't hurt to set it here
		 * again anyhow.
		 */

		ep->esp_cmd = CMD_SET_ATN;
		New_state(esp, ACTS_MSG_IN_MORE);
		esp->e_imsglen = 0;
	}

	ep->esp_cmd = CMD_MSG_ACPT;
	return (ACTION_RETURN);
}

static int
esp_onebyte_msg(esp)
register struct esp *esp;
{
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register int msgout = 0;
	register u_char msgin = esp->e_imsgarea[0];
	register tgt = Tgt(sp);

	if (msgin & MSG_IDENTIFY) {
		/*
		 * How did we get here? We should only see identify
		 * messages on a reconnection, but we'll handle this
		 * fine here (just in case we get this) as long as
		 * we believe that this is a valid identify message.
		 *
		 * For this to be a valid incoming message,
		 * bits 6-4 must must be zero. Also, the
		 * bit that says that I'm an initiator and
		 * can support disconnection cannot possibly
		 * be set here.
		 */

		char garbled = ((msgin & (BAD_IDENTIFY|INI_CAN_DISCON)) != 0);

		esplog (esp, LOG_ERR, "%s message 0x%x from Target %d",
		    garbled ? "Garbled" : "Identify", msgin, tgt);

		if (garbled) {
			/*
			 * If it's a garbled message,
			 * try and tell the target...
			 */
			msgout = MSG_INITIATOR_ERROR;
		} else {
			New_state(esp, ACTS_UNKNOWN);
		}
		LOG_STATE(ACTS_MSG_IN, esp->e_stat, msgin, -1);
		return (msgout);
	} else if (IS_2BYTE_MSG(msgin) || IS_EXTENDED_MSG(msgin)) {
		esp->e_imsglen = 2;
		New_state(esp, ACTS_MSG_IN_MORE);
		return (0);
	}

	New_state(esp, ACTS_UNKNOWN);

	switch (msgin) {
	case MSG_DISCONNECT:
		/*
		 * If we 'cannot' disconnect- reject this message.
		 * Note that we only key off of the pkt_flags here-
		 * it would be inappropriate to test against scsi_options
		 * or esp->e_nodisc here (they might have been changed
		 * after this command started). I realize that this
		 * isn't complete coverage against this error, but it
		 * is the best we can do. I thought briefly about setting
		 * (in esp_init_cmd) the FLAG_NODISCON bit in a packet
		 * if either of scsi_options or esp->e_nodisc indicated
		 * that disconnect/reconnect has been turned off, but
		 * that might really bolix up the true owner of the
		 * packet (the target driver) who has really only
		 * *loaned* us this packet during transport.
		 */
		if (sp->cmd_pkt.pkt_flags & FLAG_NODISCON) {
			msgout = MSG_REJECT;
			break;
		}
		LOG_STATE(ACTS_DISCONNECT, esp->e_stat, esp->e_xfer, -1);
		/* FALL THROUGH */
	case MSG_COMMAND_COMPLETE:
	case MSG_LINK_CMPLT:
	case MSG_LINK_CMPLT_FLAG:
		esp->e_state = ACTS_CLEARING;
		LOG_STATE(ACTS_MSG_IN, esp->e_stat, msgin, -1);
		break;

	/* This has been taken care of above	*/
	/* case MSG_EXTENDED:			*/

	case MSG_NOP:
		LOG_STATE(ACTS_NOP, esp->e_stat, -1, -1);
		break;

	case MSG_REJECT:
	{
		u_char reason = 0;
		u_char lastmsg = esp->e_last_msgout;
		/*
		 * The target is rejecting the last message we sent.
		 *
		 * If the last message we attempted to send out was an
		 * extended message, we were trying to negotiate sync
		 * xfers- and we're okay.
		 *
		 * Otherwise, a target has rejected a message that
		 * it should have handled. We will abort the operation
		 * in progress and set the pkt_reason value here to
		 * show why we have completed. The process of aborting
		 * may be via a message or may be via a bus reset (as
		 * a last resort).
		 */
		msgout = MSG_ABORT;
		LOG_STATE(ACTS_REJECT, esp->e_stat, -1, -1);

		switch (lastmsg) {
		case MSG_EXTENDED:
			esp->e_sdtr = 0;
			esp->e_offset[tgt] = 0;
			esp->e_sync_known |= (1<<tgt);
			msgout = 0;
			break;

		case MSG_NOP:
			reason = CMD_NOP_FAIL;
			break;
		case MSG_INITIATOR_ERROR:
			reason = CMD_IDE_FAIL;
			break;
		case MSG_MSG_PARITY:
			reason = CMD_PER_FAIL;
			break;
		case MSG_REJECT:
			reason = CMD_REJECT_FAIL;
			break;
		case MSG_ABORT:
			/*
			 * If an ABORT OPERATION message is rejected
			 * it is time to yank the chain on the bus...
			 */
			reason = CMD_ABORT_FAIL;
			msgout = -ACTION_ABORT_CURCMD;
			break;
		default:
			if (IS_IDENTIFY_MSG(lastmsg)) {
				reason = CMD_ID_FAIL;
			} else {
				reason = CMD_TRAN_ERR;
				msgout = -ACTION_ABORT_CURCMD;
			}
			break;
		}

		if (msgout) {
			esplog (esp, LOG_WARNING,
			    "Target %d rejects our message '%s'",
			    tgt, scsi_mname(lastmsg));
			if (sp->cmd_pkt.pkt_reason == 0) {
				sp->cmd_pkt.pkt_reason = reason;
			}
		}
		break;
	}
	case MSG_RESTORE_PTRS:
		sp->cmd_cdbp = sp->cmd_pkt.pkt_cdbp;
		sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
		if (sp->cmd_data != sp->cmd_saved_data) {
			sp->cmd_data = sp->cmd_saved_data;
			sp->cmd_flags |= CFLAG_NEEDSEG;
		}
		LOG_STATE(ACTS_RESTOREDP, esp->e_stat, esp->e_xfer, -1);
		break;

	case MSG_SAVE_DATA_PTR:
		sp->cmd_saved_data = sp->cmd_data;
		LOG_STATE(ACTS_SAVEDP, esp->e_stat, esp->e_xfer, -1);
		break;


	/* These don't make sense for us, and	*/
	/* will be rejected			*/
	/*	case MSG_INITIATOR_ERROR	*/
	/*	case MSG_ABORT			*/
	/*	case MSG_MSG_PARITY		*/
	/*	case MSG_DEVICE_RESET		*/


	default:
		msgout = MSG_REJECT;
		esplog (esp, LOG_NOTICE,
		    "Rejecting message '%s' from Target %d",
		    scsi_mname(msgin), tgt);
		LOG_STATE(ACTS_MSG_IN, esp->e_stat, msgin, -1);
		break;
	}

	EPRINTF1 ("Message in: %s\n", scsi_mname(msgin));

	return (msgout);
}

static int
esp_twobyte_msg(esp)
struct esp *esp;
{
	esplog (esp, LOG_WARNING,
	    "Two byte message '%s' 0x%x rejected",
	    scsi_mname(esp->e_imsgarea[0]), esp->e_imsgarea[1]);
	return (MSG_REJECT);
}

/*
 * XXX: Should be able to use %d.03%d instead of three different messages.
 */
static int
esp_multibyte_msg(esp)
register struct esp *esp;
{
	static char *mbs =
	    "Target %d now Synchronous at %d.%d MB/s max transmit rate\n";
	static char *mbs1 =
	    "Target %d now Synchronous at %d.0%d MB/s max transmit rate\n";
	static char *mbs2 =
	    "Target %d now Synchronous at %d.00%d MB/s max transmit rate\n";
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register struct espreg *ep = esp->e_reg;
	u_char emsg = esp->e_imsgarea[2];
	int tgt = Tgt(sp);
	register msgout = 0;

	if (emsg == MSG_SYNCHRONOUS) {
		u_int period, offset, regval;
		u_int minsync, maxsync, clockval;
		u_int xfer_freq, xfer_div, xfer_mod;

		period = esp->e_imsgarea[3]&0xff;
		offset = esp->e_imsgarea[4]&0xff;
		minsync = MIN_SYNC_PERIOD(esp);
		maxsync = MAX_SYNC_PERIOD(esp);
		IPRINTF3 ("received period %d offset %d from tgt %d\n",
		    period, offset, tgt);
#ifdef	ESP_TEST_FAST_SYNC
		IPRINTF3("calculated minsync %d, maxsync %d for tgt %d\n",
		    minsync, maxsync, tgt);
#endif	ESP_TEST_FAST_SYNC

		if ((++(esp->e_sdtr)) & 1) {
			/*
			 * In cases where the target negotiates synchronousn
			 * mode before we do, and we either have sync mode
			 * disbled, or this target is known to be a weak
			 * signal target, we send back a message indicating
			 * a desire to stay in asynchronous mode (the SCSI-2
			 * spec states that if we have synchronous capability
			 * that we cannot reject a SYNCHRONOUS DATA TRANSFER
			 * REQUEST message).
			 */
			IPRINTF ("SYNC negotiation initiated by target\n");
			msgout = MSG_EXTENDED;
			if ((esp->e_weak & (1<<tgt)) ||
			    (scsi_options & SCSI_OPTIONS_SYNC) == 0) {
				/*
				 * Only zero out the offset. Don't change
				 * the period.
				 */
				if ((esp->e_type == FAS236) ||
				    (esp->e_type == FAS100A)) {
					if (period < DEFAULT_FASTSYNC_PERIOD)
					    period = DEFAULT_FASTSYNC_PERIOD;
				} else {
					if (period < DEFAULT_SYNC_PERIOD)
					    period = DEFAULT_SYNC_PERIOD;
				}
				esp_make_sdtr(esp, (int) period, 0);
				goto out;
			}
		}

		xfer_freq = regval = 0;

		/*
		 * If the target's offset is bigger than ours,
		 * that's okay. We just set to our maximum.
		 */

		if (offset > DEFAULT_OFFSET) {
			offset = DEFAULT_OFFSET;
		}

		if (offset && period > maxsync) {
			/*
			 * We cannot transmit data in synchronous
			 * mode this slow, so convert to asynchronous
			 * mode.
			 */
			msgout = MSG_EXTENDED;
			esp_make_sdtr(esp, (int) period, 0);
			goto out;

		} else if (offset && period < minsync) {
			/*
			 * If the target's period is less than ours,
			 * that's okay. We can only transmit so fast.
			 */

			period = minsync;
			regval = MIN_SYNC(esp);

		} else if (offset) {
			/*
			 * Conversion method for received PERIOD value
			 * to the number of input clock ticks to the ESP.
			 *
			 * We adjust the input period value such that
			 * we always will transmit data *not* faster
			 * than the period value received.
			 */

			clockval = esp->e_clock_cycle / 1000;
			if (esp->e_backoff[tgt]) {
				/*
				 * increase period by 20%
				 */
				period = (period * 120)/100;
			}
			regval = (((period << 2) + clockval - 1) / clockval);

			/*
			 * Strictly paranoia!
			 */
			if (regval > MAX_SYNC(esp)) {
				msgout = MSG_EXTENDED;
				esp_make_sdtr(esp, (int) period, 0);
				goto out;
			}

		}

		if (offset) {

			esp->e_period[tgt] = regval;
			ep->esp_sync_period = esp->e_period[tgt] &
				SYNC_PERIOD_MASK;
			ep->esp_sync_offset = esp->e_offset[tgt] = offset |
				esp->e_req_ack_delay;

			if ((esp->e_type == FAS236) ||
			    (esp->e_type == FAS100A)) {
				/*
				 * if transferring > 5 MB/sec then enable
				 * fastscsi in conf3
				 */
				if (period < FASTSCSI_THRESHOLD) {
					if (esp->e_type == FAS236) {
						esp->e_espconf3[tgt] |=
						    ESP_CONF3_236_FASTSCSI;
					} else {
						esp->e_espconf3[tgt] |=
						    ESP_CONF3_100A_FASTSCSI;
					}
				}
				ep->esp_conf3 = esp->e_espconf3[tgt];
			}

#ifdef	ESP_TEST_FAST_SYNC
			IPRINTF3("sending period %d, offset %d to tgt %d\n",
				esp->e_period[tgt] & SYNC_PERIOD_MASK,
				esp->e_offset[tgt] & 0xf, tgt);
			IPRINTF1 ("req/ack delay = %x\n", esp->e_req_ack_delay);
			IPRINTF1 ("conf3 = %x\n", esp->e_espconf3[tgt]);
#endif	ESP_TEST_FAST_SYNC

			/*
			 * Convert input clock cycle per
			 * byte to nanoseconds per byte.
			 * (ns/b), and convert that to
			 * k-bytes/second.
			 */

			xfer_freq = ESP_SYNC_KBPS((regval *
				esp->e_clock_cycle) / 1000);
			xfer_div = xfer_freq / 1000;
			xfer_mod = xfer_freq % 1000;

			/*
			 * This was an esplog message, but it kept on
			 * screwing up console messages at boot, so
			 * its out for the moment.
			 * They are now IPRINTF's because we do not want to see
			 * them on each renegotiation after a check condition
			 */

			if (xfer_mod > 99) {
				IPRINTF3(mbs, tgt, xfer_div, xfer_mod);
			} else if (xfer_mod > 9) {
				IPRINTF3(mbs1, tgt, xfer_div, xfer_mod);
			} else {
				IPRINTF3(mbs2, tgt, xfer_div, xfer_mod);
			}

		} else if (esp->e_offset[tgt]) {
			/*
			 * We are converting back to async mode.
			 */
			ep->esp_sync_period = esp->e_period[tgt] = 0;
			ep->esp_sync_offset = esp->e_offset[tgt] = 0;
		}

		if (msgout) {
			esp_make_sdtr(esp, (int) period, (int) offset);
		}
		esp->e_sync_known |= (1<<tgt);

	} else if (emsg == MSG_MODIFY_DATA_PTR) {
		register struct dataseg *segp;
		long mod;

		if ((sp->cmd_flags & CFLAG_DMAVALID) == 0) {
			msgout = MSG_REJECT;
			goto out;
		}

		mod =	(esp->e_imsgarea[3]<<24) |
			(esp->e_imsgarea[4]<<16) |
			(esp->e_imsgarea[5]<<8) |
			(esp->e_imsgarea[6]);

		IPRINTF2 ("mod data ptr tgt %d by %d\n", tgt, mod);
		sp->cmd_data += mod;
		if (sp->cmd_data < sp->cmd_mapping ||
		    sp->cmd_data >= sp->cmd_mapping + sp->cmd_dmacount) {
			sp->cmd_data -= mod;
			msgout = MSG_REJECT;
			goto out;
		}
		segp = sp->cmd_cursubseg;
		if (segp->d_count) {
			if (sp->cmd_data < segp->d_base ||
			    sp->cmd_data >= segp->d_base + segp->d_count) {
				sp->cmd_flags |= CFLAG_NEEDSEG;
			}
		}
	} else {
		esplog (esp, LOG_NOTICE,
		    "Rejecting message %s 0x%x from Target %d",
		    scsi_mname(MSG_EXTENDED), emsg, tgt);
		msgout = MSG_REJECT;
	}
out:
	New_state(esp, ACTS_UNKNOWN);
	return (msgout);
}


/*
 * Complete the process of selecting a target
 */

static int
esp_finish_select(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	register struct dmaga *dmar = esp->e_dma;
	register struct scsi_cmd *sp = CURRENT_CMD(esp);
	register int cmdamt, fifoamt;
	register u_char intr = esp->e_intr;
	u_char step = esp->e_step;
	u_char state = esp->e_state;
	int target = Tgt(sp);

	/*
	 * Check for DMA gate array errors
	 */

	if (dmar->dmaga_csr & DMAGA_ERRPEND) {
		/*
		 * It would be desirable to set the ATN* line and attempt to
		 * do the whole schmear of INITIATOR DETECTED ERROR here,
		 * but that is too hard to do at present.
		 */
		esplog (esp, LOG_ERR,
		    "Unrecoverable DMA error during selection");
		sp->cmd_pkt.pkt_reason = CMD_TRAN_ERR;
		return (ACTION_RESET);
	}

	/*
	 * Latch up fifo count
	 */

	fifoamt = FIFO_CNT(ep);

	/*
	 * How far did we go (by the DMA gate array's reckoning)?
	 */

	cmdamt = dmar->dmaga_addr - esp->e_lastdma;

	/*
	 * If the NEXTBYTE value is non-zero (and we have the
	 * rev 1 DMA gate array), we went one longword further
	 * less 4 minus the NEXTBYTE value....
	 */

	if (DMAGA_REV(dmar) == DMA_REV1) {
		int i;
		if (i = DMAGA_NEXTBYTE(dmar)) {
			cmdamt -= (4-i);
		}
	}

	/*
	 * Shut off DMA gate array
	 */

	dmar->dmaga_csr |= DMAGA_FLUSH;
	dmar->dmaga_csr &= ~(DMAGA_ENDVMA|DMAGA_WRITE|DMAGA_ENATC);

	/*
	 * Now adjust cmdamt by the amount of data left in the fifo
	 */

	cmdamt -= fifoamt;

	/*
	 * Be a bit defensive...
	 */

	if (cmdamt < 0 || cmdamt > FIFOSIZE)
		cmdamt = 0;

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		eprintf (esp,
		    "finsel: state %s, step %d; did %d of %d; fifo %d\n",
		    esp_state_name(state), step, cmdamt,
		    esp->e_lastcount, fifoamt);
		esp_stat_int_print(esp);
	}
#endif	/* ESPDEBUG */

	/*
	 * Did something respond to selection?
	 */

	if (intr == ESP_INT_DISCON) {
		/*
		 * This takes care of getting the bus, but no
		 * target responding to selection. Clean up the
		 * chip state.
		 */

		esp_chip_disconnect(esp);

		/*
		 * There is a definite problem where the MT02
		 * drops BSY if you use the SELECT && STOP command,
		 * which leaves ATN asserted after sending an identify
		 * message.
		 */

		if (step != 0 && (esp->e_targets & (1<<target)) &&
		    (state == STATE_SELECT_N_SENDMSG ||
		    state == STATE_SELECT_N_STOP)) {

			if (state == STATE_SELECT_N_SENDMSG &&
			    esp->e_cur_msgout[0] == MSG_EXTENDED) {
				esp->e_sync_known |= (1<<target);
			}

			if ((sp->cmd_flags & CFLAG_CMDPROXY) == 0) {
				/*
				 * Gross...
				 */
				New_state(esp, STATE_FREE);
				return (ACTION_SEARCH);
			}
		}

		sp->cmd_pkt.pkt_state |= STATE_GOT_BUS;
		sp->cmd_pkt.pkt_reason = CMD_INCOMPLETE;
		return (ACTION_FINISH);
	} else if (intr == (ESP_INT_FCMP|ESP_INT_RESEL)) {
		/*
		 * A reselection attempt glotzed our selection attempt.
		 * If we were running w/o checking parity on this
		 * command, restore parity checking.
		 */
		if ((scsi_options & SCSI_OPTIONS_PARITY) &&
		    (sp->cmd_pkt.pkt_flags & FLAG_NOPARITY)) {
			ep->esp_conf = esp->e_espconf;
		}
		ESP_PREEMPT(esp);
		LOG_STATE(ACTS_PREEMPTED, esp->e_stat, 0, -1);
		return (ACTION_RESEL);
	} else if (intr != (ESP_INT_BUS|ESP_INT_FCMP)) {
		esplog (esp, LOG_ERR, "undetermined selection failure");
		esp_stat_int_print(esp);
		return (ACTION_RESET);
	}

	/*
	 * We succesfully selected a target (we think).
	 * Now we figure out how botched things are
	 * based upon the kind of selection we were
	 * doing and the state of the step register.
	 */

	esp->e_targets |= (1<<target);

	switch (step) {
	case ESP_STEP_ARBSEL:

		/*
		 * In this case, we selected the target, but went
		 * neither into MESSAGE OUT nor COMMAND phase.
		 * However, this isn't a fatal error, so we just
		 * drive on.
		 *
		 * This might be a good point to note that we have
		 * a target that appears to not accomodate disconnecting,
		 * but it really isn't worth the effort to distinguish
		 * such targets especially from others.
		 */

		/* FALLTHROUGH */

	case ESP_STEP_SENTID:
		/*
		 * In this case, we selected the target and sent
		 * message byte and have stopped with ATN* still on.
		 * This case should only occur if we use the SELECT
		 * AND STOP command.
		 */

		/* FALLTHROUGH */

	case ESP_STEP_NOTCMD:
		/*
		 * In this case, we either didn't transition to command
		 * phase, or, if we were using the SELECT WITH ATN3 command,
		 * we possibly didn't send all message bytes.
		 */

		cmdamt = 0;
		break;

	case ESP_STEP_PCMD:
		/*
		 * In this case, not all command bytes transferred.
		 */
		/* FALLTHROUGH */

	case ESP_STEP_DONE:
		/*
		 * This is the usual 'good' completion point.
		 * If we we sent message byte(s), we subtract
		 * off the number of message bytes that were
		 * ahead of the command.
		 */

		if (state == STATE_SELECT_NORMAL)
			cmdamt -= 1;
		break;

	default:
		esplog (esp, LOG_ERR,
		    "bad sequence step (0x%x) in selection", step);
		return (ACTION_RESET);
	}

	/*
	 * If we sent any messages or sent a command, as
	 * per ESP errata sheets, we have to hit the
	 * chip with a CMD_NOP in order to unlatch the
	 * fifo counter.
	 */

	ep->esp_cmd = CMD_NOP;

	/*
	 * *Carefully* dump out any cruft left in the fifo.
	 * If this target has shifted to synchronous DATA IN
	 * phase, then the ESP has already flushed the fifo
	 * for us.
	 */

	if (fifoamt != 0 &&
	    ((esp->e_stat & ESP_PHASE_MASK) != ESP_PHASE_DATA_IN ||
	    esp->e_offset[target] == 0)) {
		ep->esp_cmd = CMD_FLUSH;
	}

	/*
	 * OR in common state...
	 */

	sp->cmd_pkt.pkt_state |= (STATE_GOT_BUS|STATE_GOT_TARGET);

	/*
	 * advance command pointer
	 */

	if (cmdamt > 0) {
		sp->cmd_pkt.pkt_state |= STATE_SENT_CMD;
		sp->cmd_cdbp = (caddr_t) (((u_long)sp->cmd_cdbp) + cmdamt);
	}

	/*
	 * initialize data information (if any)
	 */

	if (sp->cmd_data != sp->cmd_saved_data) {
		sp->cmd_data = sp->cmd_saved_data;
		sp->cmd_flags |= CFLAG_NEEDSEG;
	}

	New_state(esp, ACTS_UNKNOWN);
	return (ACTION_PHASEMANAGE);
}

/*
 * Handle the reconnection of a target
 */

static int
esp_reconnect(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	struct scsi_cmd *sp;
	register target, lun;
	register u_char tmp, myid = (1<<MY_ID(esp));
	short slot;
	static char *urecpid =
	    "unrecoverable SCSI bus parity error (IDENTIFY msg)";

	New_state(esp, ACTS_RESEL);

	/*
	 * Pick up target id from fifo
	 */

	if (FIFO_CNT(ep) != 2) {
		/*
		 * There should only be the reselecting target's id
		 * and an identify message in the fifo.
		 */
		goto bad;
	}

	tmp = ep->esp_fifo_data;

	if ((tmp & myid) == 0) {
		/*
		 * Our SCSI id is missing. This 'cannot happen'.
		 */
		goto bad;
	}

	/*
	 * Turn off our id
	 */

	tmp ^= myid;

	if (tmp == 0) {
		/*
		 * There is no other SCSI id, therefore we cannot
		 * tell who is reselecting us. This 'cannot happen'.
		 */
		goto bad;
	}

	for (target = 0; target < NTARGETS; target++) {
		if (tmp & (1<<target)) {
			tmp ^= (1<<target);
			break;
		}
	}

	if (tmp) {
		/*
		 * There is more than one reselection id on the bus.
		 * This 'cannot happen'.
		 */
		goto bad;
	}


	/*
	 * Now pick up identify message byte, and acknowledge it.
	 */

	if ((esp->e_stat & ESP_PHASE_MASK) != ESP_PHASE_MSG_IN) {
		/*
		 * If we aren't in MESSAGE IN phase,
		 * things are really screwed up.
		 */
		goto bad;
	}

	tmp = esp->e_last_msgin = ep->esp_fifo_data;

	/*
	 * XXX: Oh boy. We have problems. What happens
	 * XXX: if we have a parity error on the IDENTIFY
	 * XXX: message? We cannot know which lun is
	 * XXX: reconnecting, but we really need to know
	 * XXX: that in order to go through all the
	 * XXX: rigamarole of sending a MSG_PARITY_ERR
	 * XXX: message back to the target.
	 * XXX:
	 * XXX: In order to minimize a panic situation,
	 * XXX: we'll assume a lun of zero (i.e., synthesize
	 * XXX: the IDENTIFY message), and only panic
	 * XXX: if there is more than one active lun on
	 * XXX: this target.
	 */

	if (esp->e_stat & ESP_STAT_PERR) {
		tmp = MSG_IDENTIFY;
	}

	/*
	 * Check sanity of message.
	 */

	if (!(IS_IDENTIFY_MSG(tmp)) || (tmp & INI_CAN_DISCON)) {
		goto bad;
	}

	lun = tmp & (NLUNS_PER_TARGET-1);

	while (FIFO_CNT(ep)) {
		tmp = ep->esp_fifo_data;
	}

	/*
	 * As per the ESP100 errata sheets, if a selection attempt
	 * is preempted by a reselection coming in, we'll get a
	 * spurious ILLEGAL COMMAND error interrupt from the ESP100.
	 * Instead of trying to figure out whether we were preempted
	 * or not, just gate off of whether ot not we are a ESP100
	 * or not.
	 */

	if (IS_53C90(esp)) {
		tmp = ep->esp_intr;
	}

	/*
	 * I believe that this needs to be done to unlatch the ESP.
	 */
	ep->esp_cmd = CMD_NOP;

	/*
	 * If this target is synchronous, here is the
	 * place to set it up during a reconnect.
	 */

	ep->esp_sync_period = esp->e_period[target] & SYNC_PERIOD_MASK;
	ep->esp_sync_offset = esp->e_offset[target] | esp->e_req_ack_delay;
	if ((esp->e_type == FAS236) || (esp->e_type == FAS100A)) {
		ep->esp_conf3 = esp->e_espconf3[target];
	}
	LOG_STATE(ACTS_RESEL, esp->e_stat, target, lun);

	/*
	 * Now search for command to reconnect to.
	 */

	slot = (target * NLUNS_PER_TARGET) | lun;
	sp = esp->e_slots[slot];

	/*
	 * Handle the case of a parity error on the IDENTIFY message.
	 * Above we set things up to assume a lun of zero. If there
	 * is more than one lun active, we die. In either case, if
	 * there is a lun active, we find it and assume that it is
	 * the device we are reconnecting to.
	 */

	if (esp->e_stat & ESP_STAT_PERR) {
		tmp = 0;
		for (lun = 0; lun < NLUNS_PER_TARGET; lun++) {
			if (esp->e_slots[slot+lun]) {
				tmp++;
				if (sp == (struct scsi_cmd *) 0) {
					sp = esp->e_slots[slot+lun];
				}
			}
		}
		if (tmp == 1) {
			lun = Lun(sp);
			slot = slot + lun;
		} else if (tmp > 1) {
			esplog (esp, LOG_ERR, urecpid);
			goto bad;
		}
	}

	if ((sp == (struct scsi_cmd *) 0) ||
	    (sp->cmd_flags & (CFLAG_CMDDISC|CFLAG_CMDPROXY)) == 0) {
		/*
		 * There is either no command to reconnect to, or the command
		 * that is recorded here is not the command that the target
		 * is attempting to reconnect to. The latter case is a hard
		 * one to figure, and we only do a half-hearted job of doing
		 * so. Later, with SCSI-2 q'd commands, we will more likely
		 * depend upon queue tag messages to try and find commands
		 * to reconnect to.
		 *
		 * A reason for why this situation might occur is that a
		 * command may time out while disconnected. We knock down
		 * our record of the command and notify the target driver
		 * that the command timed out (see esp_watch()). If the
		 * target driver had been mistaken about how long a command
		 * may take to do, and the target reconnects later for
		 * the now timed-out command, we have to abort the operation.
		 *
		 * In the case that there is a command in the slot, and the
		 * command isn't marked as disconnected or proxy, then
		 * the target driver is attempting to talk to the target
		 * again. This is a very shaky edifice of race conditions
		 * here, as the slot may be taken up by yet another proxy
		 * command (generated by the target attempting to reset
		 * this target via the scsi_reset() function), in which
		 * case we'll probably die very horribly soon.
		 *
		 * Odd as it may seem, the below code seems to work pretty
		 * well. I've managed to test it on several targets, and
		 * the targets seem pretty amiable about accepting an ABORT
		 * OPERATION message during reconnection- even targets that
		 * get very bothered about ABORT OPERATION messages at other
		 * times.
		 */

		auto struct scsi_address addr;
		auto struct scsi_cmd local;
		register struct scsi_cmd *sp1;

		if (sp) {
			sp1 = sp;
		} else {
			sp1 = (struct scsi_cmd *) 0;
		}
		sp = &local;
		addr.a_cookie = scsi_cookie(&esp->e_tran);
		addr.a_target = target;
		addr.a_lun = lun;
		addr.a_sublun = 0;

		esp_makeproxy_cmd(sp, &addr, MSG_ABORT);
		esp_init_cmd(sp);
		esp->e_npolling++;
		esp->e_ncmds++;

		esplog (esp, LOG_WARNING,
		    "No command for reconnect of Target %d Lun %d",
		    target, lun);

		/*
		 * Set ATN* in order to try and send an ABORT OPERATION msg.
		 */

		esp->e_cur_msgout[0] = MSG_ABORT;
		esp->e_omsglen = 1;
		ep->esp_cmd = CMD_SET_ATN;
		ep->esp_cmd = CMD_MSG_ACPT;
		esp->e_cur_slot = slot;
		CURRENT_CMD(esp) = sp;
		New_state(esp, ACTS_UNKNOWN);
		while (esp->e_npolling) {
			if (esp_dopoll(esp, POLL_TIMEOUT)) {
				if (esp->e_slots[slot] == &local) {
					esp->e_slots[slot] =
					    (struct scsi_cmd *) NULL;
				}
				return (ACTION_RESET);
			}
		}
		esplog (esp, LOG_INFO, "Proxy abort %s for Target %d Lun %d",
		    (sp->cmd_pkt.pkt_reason == CMD_CMPLT) ? "succeeded" :
		    "failed", target, lun);
		if (sp1) {
			esp->e_slots[slot] = sp1;
		} else if (esp->e_slots[slot] == &local) {
			esp->e_slots[slot] = (struct scsi_cmd *) NULL;
		}
		if (sp->cmd_pkt.pkt_reason == CMD_CMPLT &&
		    sp->cmd_cdb[ESP_PROXY_RESULT] == TRUE) {
			return (ACTION_SEARCH);
		} else
			return (ACTION_RESET);
	} else if (sp->cmd_flags & CFLAG_CMDPROXY) {
		/*
		 * If we got here, we were already attempting to
		 * run a polled proxy command for this target.
		 * Set ATN and, copy in the message, and drive
		 * on (ignoring any parity error on the identify).
		 */
		esp->e_ndisc++; /* readjust this up */
		ep->esp_cmd = CMD_SET_ATN;
		esp->e_omsglen = sp->cmd_cdb[ESP_PROXY_DATA];
		tmp = 0;
		while (tmp < esp->e_omsglen) {
			esp->e_cur_msgout[tmp] =
			    sp->cmd_cdb[ESP_PROXY_DATA+1+tmp];
			tmp++;
		}
		sp->cmd_cdb[ESP_PROXY_RESULT] = FALSE;
		IPRINTF2 ("esp_reconnect: fielding proxy cmd for %d.%d\n",
		    target, lun);
	} else if (scsi_options & SCSI_OPTIONS_PARITY) {
		/*
		 * If we are doing PARITY checking, check for a parity
		 * error on the IDENTIFY message.
		 */
		if (sp->cmd_pkt.pkt_flags & FLAG_NOPARITY) {
			/*
			 * If we had detected a parity error
			 * on the IDENTIFY message, and this
			 * command is being run without checking,
			 * act as if we didn't get a parity
			 * error. The assumption here is that
			 * we only disable parity checking for
			 * targets that don't generate parity.
			 */
			ep->esp_conf = esp->e_espconf & ~ESP_CONF_PAREN;
		} else if (esp->e_stat & ESP_STAT_PERR) {
			esp->e_cur_msgout[0] = MSG_MSG_PARITY;
			esp->e_omsglen = 1;
		}
	}

	/*
	 * Accept the IDENTIFY message
	 */

	ep->esp_cmd = CMD_MSG_ACPT;

	/*
	 * Now- if this is a SCSI-2 type target, we have to, probably,
	 * by hand here, pick up the QUEUE TAG message that must follow
	 * in order to fully qualify which nexus is being established.
	 *
	 * Good Luck, Jim....
	 *
	 */

	esp->e_cur_slot = slot;
	if (esp->e_ndisc != 0)
		esp->e_ndisc--;
	New_state(esp, ACTS_UNKNOWN);

	sp->cmd_flags &= ~CFLAG_CMDDISC;

	/*
	 * A reconnect implies a restore pointers operation
	 */

	sp->cmd_cdbp = sp->cmd_pkt.pkt_cdbp;
	sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
	sp->cmd_data = sp->cmd_saved_data;

	/*
	 * And zero out the SYNC negotiation counter
	 */

	esp->e_sdtr = 0;

	/*
	 * Return to await the FUNCTION COMPLETE interrupt we
	 * should get out of accepting the IDENTIFY message.
	 */

	EPRINTF2 ("Reconnecting %d.%d\n", target, lun);

	return (ACTION_RETURN);

bad:
	esp_printstate(esp, "failed reselection");
	LOG_STATE(ACTS_BAD_RESEL, esp->e_stat, -1, -1);
	return (ACTION_RESET);
}

/*
 * Miscellaneous Service Subroutines
 */


static int
esp_istart(esp)
struct esp *esp;
{
	if (esp->e_state == STATE_FREE && esp->e_npolling == 0 &&
	    esp->e_ncmds > esp->e_ndisc) {
		(void) esp_ustart(esp, esp->e_last_slot);
	}
	return (ACTION_RETURN);
}

static void
esp_runpoll(esp, slot)
register struct esp *esp;
short slot;
{
	register struct scsi_cmd *sp = esp->e_slots[slot];
	int limit = sp->cmd_pkt.pkt_time * 1000000;

	/*
	 * mark that we're going to do a polling command
	 */

	esp->e_npolling++;

	while (esp->e_npolling) {
		/*
		 * drain any current active command(s)
		 */

		if (esp->e_state != STATE_FREE) {
			if (esp_dopoll(esp, POLL_TIMEOUT)) {
				printf("runpoll: timeout on draining\n");
				(void) esp_abort_curcmd(esp);
				return;
			}
		}

		/*
		 * If the draining of active commands killed the
		 * the current polled command, we're done..
		 */

		if (esp->e_npolling == 0) {
			break;
		}

		if (esp_ustart(esp, slot) != TRUE) {
			continue;
		}

		/*
		 * We're now 'running' this command.
		 *
		 * esp_dopoll will always return when
		 * esp->e_state is STATE_FREE, and
		 * we'll use the npolling token to
		 * decide whether that command got done.
		 */
		while (esp->e_npolling) {
			if (esp_dopoll(esp, limit)) {
				printf("runpoll: timeout on polling\n");
				(void) esp_abort_curcmd(esp);
				return;
			}
			/*
			 * If a preemption occurred that caused this
			 * command to actually not start, go around
			 * the loop again..
			 */
			if (sp->cmd_pkt.pkt_state == 0) {
				break;
			}
		}
	}

	/*
	 * If we stored up commands to do, start them off now.
	 */
	if (esp->e_state == STATE_FREE)
		(void) esp_ustart(esp, NEXTSLOT(slot));
}


static int
esp_reset_bus(esp)
struct esp *esp;
{
	New_state(esp, ACTS_RESET);

	esp_internal_reset(esp, ESP_RESET_HW & ~ESP_RESET_IGNORE_BRESET);

	/*
	 * Now that we've reset the SCSI bus, we'll take a SCSI RESET
	 * interrupt and use that to clean up the state of things.
	 */

	return (ACTION_RETURN);
}

static int
esp_reset_recovery(esp)
struct esp *esp;
{

	register struct scsi_cmd *sp;
	register short slot, start_slot;
	auto struct scsi_cmd *tslots[NTARGETS * NLUNS_PER_TARGET];
	u_char reason, was_polling;

	if ((start_slot = esp->e_cur_slot) == UNDEFINED)
		start_slot = 0;

	/*
	 * Actually- for right now just claim that all
	 * commands have been destroyed by a SCSI reset
	 * and let already set reason fields or callers
	 * decide otherwise for specific commands.
	 */

	reason = CMD_RESET;

	/*
	 * Preserve the state of whether
	 * we are in polling mode or not.
	 */

	was_polling = esp->e_npolling;

	/*
	 * We're blowing it all away. Remove any dead wood to the
	 * side so that completion routines don't get confused.
	 */

	bcopy((caddr_t) esp->e_slots, (caddr_t)tslots,
	    (sizeof (struct scsi_cmd *)) * NTARGETS * NLUNS_PER_TARGET);

	if (esp->e_state != ACTS_RESET && esp->e_state != ACTS_ABORTING) {
		esp_internal_reset(esp, ESP_RESET_ESP|ESP_RESET_DMA);
#ifdef	ESPDEBUG
		if (INFORMATIVE) {
			register short z = esp->e_phase_index;

			z = (z - 2) & (NPHASE - 1);
			if (esp->e_phase[z].e_save_state != ACTS_RESET)
			    esp_printstate(esp, "unexpected SCSI bus reset");
		}
#endif	/* ESPDEBUG */
		esplog (esp, LOG_NOTICE, "unexpected SCSI bus reset");
	}

	/*
	 * Call this routine to compltely reset the state of the softc data.
	 */

	esp_internal_reset(esp, ESP_RESET_SOFTC);

	/*
	 * Hold the state of the host adapter open
	 */

	New_state(esp, ACTS_FROZEN);

	slot = start_slot;
	do {
		if (sp = tslots[slot]) {
			/*
			 * If there wasn't any other reason set
			 * for this completion, note it in the
			 * command reason state.
			 */
			if (sp->cmd_pkt.pkt_reason == 0) {
				sp->cmd_pkt.pkt_reason = reason;
			}
			sp->cmd_pkt.pkt_resid = sp->cmd_dmacount;
			if (sp->cmd_pkt.pkt_comp)
				(*sp->cmd_pkt.pkt_comp)(sp);
		}
		slot = NEXTSLOT(slot);
	} while (slot != start_slot);

	/*
	 * Move the state back to free...
	 */

	New_state(esp, STATE_FREE);

	/*
	 * and if anyone requeued, let our caller know to start 'em off,
	 * unless we were running in polled mode. It is the responsiblity
	 * of the top of the thread for polled mode to restart things.
	 */

	if (esp->e_ncmds && !was_polling) {
		return (ACTION_SEARCH);
	} else {
		return (ACTION_RETURN);
	}
}

static int
esp_handle_selection(esp)
struct esp *esp;
{
	esplog (esp, LOG_NOTICE, "Unexpected Selection Attempt");
	return (ACTION_RESET);
}

static void
esp_init_cmd(sp)
struct scsi_cmd *sp;
{
	sp->cmd_pkt.pkt_reason = 0;
	sp->cmd_pkt.pkt_resid = 0;
	sp->cmd_pkt.pkt_state = sp->cmd_pkt.pkt_statistics = 0;
	sp->cmd_cdbp = (caddr_t) sp->cmd_pkt.pkt_cdbp;
	sp->cmd_scbp = sp->cmd_pkt.pkt_scbp;
	*(sp->cmd_scbp) = 0;
	sp->cmd_flags &= ~(CFLAG_NEEDSEG|CFLAG_CMDDISC|CFLAG_WATCH);
	if ((sp->cmd_timeout = sp->cmd_pkt.pkt_time) != 0)
		sp->cmd_flags |= CFLAG_WATCH;
	if (sp->cmd_flags & CFLAG_DMAVALID) {
		sp->cmd_data = sp->cmd_saved_data = sp->cmd_mapping;
		sp->cmd_cursubseg = &sp->cmd_subseg;
		sp->cmd_subseg.d_base = sp->cmd_mapping;
		sp->cmd_subseg.d_count = 0;
		sp->cmd_subseg.d_next = (struct dataseg *) 0;
	}
}

static void
esp_makeproxy_cmd(sp, ap, msg)
struct scsi_cmd *sp;
struct scsi_address *ap;
int msg;
{
	extern void scsi_pollintr();
	bzero((caddr_t) sp, sizeof (*sp));
	sp->cmd_pkt.pkt_address = *ap;
	sp->cmd_pkt.pkt_comp = scsi_pollintr;
	sp->cmd_pkt.pkt_flags = FLAG_NOINTR|FLAG_NOPARITY;
	sp->cmd_pkt.pkt_scbp = (opaque_t) &sp->cmd_scb[0];
	sp->cmd_pkt.pkt_cdbp = (opaque_t) &sp->cmd_cdb[0];
	sp->cmd_pkt.pkt_pmon = (char) -1;
	sp->cmd_flags = CFLAG_CMDPROXY;
	sp->cmd_cdb[ESP_PROXY_TYPE] = ESP_PROXY_SNDMSG;
	sp->cmd_cdb[ESP_PROXY_RESULT] = FALSE;
	sp->cmd_cdb[ESP_PROXY_DATA] = 1;
	sp->cmd_cdb[ESP_PROXY_DATA+1] = msg;
}

/*
 * Handle resetting the chip in the case
 * that a DISCONNECT interrupt is received.
 */

static void
esp_chip_disconnect(esp)
register struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	register struct scsi_cmd *sp;

	/*
	 * We used to have some code in here that cleared
	 * the sync_period && sync_offset registers and
	 * then pushed in a CMD_FLUSH command. This seemed
	 * to cause some problems for reconnecting, so
	 * I nuked those commads. Things have been a bit
	 * happier since then.
	 */

	ep->esp_cmd = CMD_EN_RESEL;
	if (esp->e_cur_slot != UNDEFINED && (sp = CURRENT_CMD(esp))) {

		/*
		 * If we were running this command without
		 * parity checking, then reload the conf
		 * register (to re-enable parity checking).
		 */

		if ((scsi_options & SCSI_OPTIONS_PARITY) &&
		    (sp->cmd_pkt.pkt_flags & FLAG_NOPARITY)) {
			ep->esp_conf = esp->e_espconf;
		}
	}
	esp->e_sdtr = 0;
}

static void
esp_make_sdtr(esp, period, offset)
struct esp *esp;
int period, offset;
{
	register u_char *p = esp->e_cur_msgout;
	*p++ = (u_char) MSG_EXTENDED;
	*p++ = (u_char) 3;
	*p++ = (u_char) MSG_SYNCHRONOUS;
	*p++ = (u_char) period;
	*p++ = (u_char) offset;
	esp->e_omsglen = 5;
}


/*
 * Command watchdog routines
 */

static int
esp_watch()
{
	struct esp *esp;
	register s;

	for (esp = esp_softc; esp != (struct esp *) 0; esp = esp->e_next) {
		if (esp == (struct esp *) 0) {
			break;
		} else if (esp->e_tran.tran_start == (func_t) 0) {
			continue;
		}
		s = splr(esp->e_tran.tran_spl);
		if (esp->e_ncmds) {
			esp_watchsubr(esp);
		}
		(void) splx(s);
	}
	timeout(esp_watch, (caddr_t) 0, hz);
}

static void
esp_watchsubr(esp)
register struct esp *esp;
{
	register short slot;
	register struct scsi_cmd *sp;

	for (slot = 0; slot < (NTARGETS*NLUNS_PER_TARGET); slot++) {
		/*
		 * No command- drive on
		 */
		if ((sp = esp->e_slots[slot]) == 0) {
			continue;
		}

#ifdef ESP_TEST_TIMEOUT
		if (esp_ttest & (1<<Tgt(sp))) {
			esp_ttest = 0;
			esp_curcmd_timeout(esp, sp);
			continue;
		}
		if ((esp_stest & (1<<Tgt(sp))) && (esp->e_cur_slot == slot) &&
			((esp->e_state	== ACTS_DATA_DONE) ||
			(esp->e_state  == ACTS_DATA)) &&
			((sp->cmd_flags & CFLAG_CMDDISC) == 0)) {
			esp_stest = 0;
			esp_curcmd_timeout(esp, sp);
			continue;
		}
#endif

		/*
		 * This command hasn't officially been started yet- drive on
		 */

		if (sp->cmd_pkt.pkt_state == 0 &&
		    esp->e_cur_slot != UNDEFINED && sp != CURRENT_CMD(esp)) {
			continue;
		}

		/*
		 * This command not to be watched- drive on
		 */

		if ((sp->cmd_flags & CFLAG_WATCH) == 0) {
			continue;
		}

		/*
		 * Else, knock a second off of the timer if any time left.
		 */

		if (sp->cmd_timeout > 0) {
			sp->cmd_timeout -= 1;
			continue;
		}

		/*
		 * No time left for this command. Last check
		 * before killing it.
		 */

		if (INTPENDING(esp)) {
			/*
			 * A pending interrupt
			 * defers the sentence
			 * of death.
			 */
			sp->cmd_timeout++;
			espsvc(esp);
			break;
		}

		if (sp->cmd_flags & CFLAG_CMDDISC) {
			esp_disccmd_timeout(esp, slot);
			break;
		} else {
			esp_curcmd_timeout(esp, sp);
			break;
		}
	}
}

static void
esp_curcmd_timeout(esp, sp)
register struct esp *esp;
struct scsi_cmd *sp;
{
	int target = Tgt(sp);
#ifdef ESPDEBUG
	register i;
	register u_char *cp = (u_char *) sp->cmd_pkt.pkt_cdbp;
	auto char buf[128];
#endif /* ESPDEBUG */

	/*
	 * Various attempts have been made to recover
	 * from this situation, all to no avail. See
	 * SCCS history for their story.
	 *
	 * If we ever figure out how to recover from this,
	 * this routine will become a function that can
	 * actually return true or false depending on whether
	 * we could get the command going again.
	 *
	 * For now, blow it all away, and rely on the target
	 * driver to restart the command (if possible).
	 */

	esplog (esp, LOG_ERR, "Current command timeout for Target %d Lun %d",
	    target, Lun(sp));

#ifdef	ESPDEBUG
	esplog (esp, LOG_ERR, "\tState=%s (0x%x), Last State=%s (0x%x)",
			esp_state_name(esp->e_state),
			esp->e_state,
			esp_state_name(esp->e_laststate),
			esp->e_laststate);

	esplog (esp, LOG_ERR, "\tCmd dump for Target %d Lun %d:",
			Tgt(sp), Lun(sp));
	buf[0] = '\0';
	for (i = 0; i < sp->cmd_cdblen; i++) {
		(void) sprintf(&buf[strlen(buf)], " 0x%x", *cp++);
		if (strlen(buf) > 124)
			break;
	}
	esplog (esp, LOG_ERR, "\tcdb=[%s ]", buf);
	if (sp->cmd_pkt.pkt_state & STATE_GOT_STATUS)
		esplog (esp, LOG_ERR,
			"\tStatus=0x%x", sp->cmd_pkt.pkt_scbp[0]);

	if (INFORMATIVE) {
		esp_printstate(esp, "Current cmd timeout");
		esp->e_stat = esp->e_reg->esp_stat & ~ESP_STAT_RES;
		esp->e_intr = esp->e_reg->esp_intr;
		esp_stat_int_print(esp);
	}
#endif	/* ESPDEBUG */

	/*
	 * Current command timeout appears to relate often to noisy SCSI
	 * in synchronous mode. Therefore, we'll disable synchronous from
	 * now on for this target. Yes, I know this is kind of bogus....
	 */

	esp_sync_backoff(esp, sp);

#ifdef	IS_IT_SOUP_YET
	if (IS_53C90(esp) && esp->e_offset[target] &&
	    esp->e_state == ACTS_DATA_DONE) {
		/*
		 * Can we do anything? We've tried
		 * 'by hand' forced initiator mode stuff..
		 */
	}
#endif	/* IS_IT_SOUP_YET */

	if (esp_abort_allcmds(esp) == ACTION_SEARCH) {
		(void) esp_ustart(esp, 0);
	}
}

static void
esp_sync_backoff(esp, sp)
register struct esp *esp;
register struct scsi_cmd *sp;
{
	char phase = esp->e_stat & ESP_PHASE_MASK;
	char state = esp->e_state;
	u_char tgt = Tgt(sp);

	IPRINTF3("esp_sync_backoff: target %d: state=%x, phase=%x\n",
	    tgt, state, phase);

#ifdef NOTNOW
	/*
	 * Only process data phase hangs.  Also, ignore any data phase
	 * hangs caused by request sense cmds as it's possible they could
	 * be caused by target reverting to asynch.
	 */
	if (state != ACTS_DATA && state != ACTS_DATA_DONE) {
		IPRINTF2("Target %d.%d hang state not in data phase\n",
			tgt, Lun(sp));
		return;
	} else if (phase != ESP_PHASE_DATA_IN && phase != ESP_PHASE_DATA_OUT) {
		IPRINTF2("Target %d.%d hang bus not in data phase\n",
			tgt, Lun(sp));
		return;
	} else if ((u_char) *(sp->cmd_pkt.pkt_cdbp) == SCMD_REQUEST_SENSE) {
		IPRINTF2("Target %d.%d ignoring request sense hang\n",
			tgt, Lun(sp));
		return;
	}
#endif

	/*
	 * First we reduce xfer rate 20% and always enable slow cable mode
	 * and if that fails we revert to async with slow cable mode
	 *
	 * XXX: Should I try delaying REQ pulse too?
	 */
	if (esp->e_offset[tgt] != 0) {
#ifdef	ESP_TEST_FAST_SYNC
		register u_int regval, maxreg;
		regval = esp->e_period[tgt];
		maxreg = MAX_SYNC(esp);
		IPRINTF4("regval %d maxreg %d backoff %d for tgt %d\n",
		    regval, maxreg, esp->e_backoff[tgt], tgt);
#endif	ESP_TEST_FAST_SYNC

		/*
		 * Compute sync transfer limits for later compensation.
		 */
		IPRINTF3("Target %d.%d back off using %s params\n", tgt,
			Lun(sp), ((esp->e_type == FAS236) ||
			(esp->e_type == FAS100A))? "FAS" : "ESP");


		/*
		 * XXX: Should I try delaying REQ pulse too?
		 * XXX: Should I backoff e_default_period too?
		 */

		if (esp->e_backoff[tgt]) {
			esp->e_period[tgt] = 0;
			esp->e_offset[tgt] = 0;
			esp->e_weak |= 1<<tgt;
			esplog(esp, LOG_ERR,
			    "Target %d.%d reverting to async. mode",
			    tgt, Lun(sp));
		} else {
			(esp->e_backoff[tgt])++;
			esplog(esp, LOG_ERR,
			    "Target %d.%d reducing sync. transfer rate",
			    tgt, Lun(sp));
		}

		/*
		 * Paranoia: Force sync. renegotiate
		 */
		esp->e_sync_known &= ~(1<<tgt);

	}
	if (((esp->e_type != FAS236) && (esp->e_type != FAS100A)) &&
	    ((esp->e_espconf & ESP_CONF_SLOWMODE) == 0)) {
		/*
		 * always enable slow cable mode
		 */
		esp->e_espconf |= ESP_CONF_SLOWMODE;
		esp->e_reg->esp_conf |= ESP_CONF_SLOWMODE;
		esplog(esp, LOG_ERR, "Reverting to slow SCSI cable mode");
	}
}

static void
esp_disccmd_timeout(esp, slot)
struct esp *esp;
short slot;
{
	struct scsi_cmd *sp = esp->e_slots[slot];
	register int target = Tgt(sp);

	esplog (esp, LOG_ERR,
	    "Disconnected command timeout for Target %d Lun %d",
	    target, Lun(sp));

#ifdef	ESPDEBUG
	if (INFORMATIVE) {
		esp_dump_cmd(sp);
	}
#endif	/* ESPDEBUG */

	sp->cmd_pkt.pkt_reason = CMD_TIMEOUT;
	esp->e_slots[slot] = 0;
	esp->e_ndisc -= 1;
	esp->e_ncmds -= 1;
	(*sp->cmd_pkt.pkt_comp) (sp);
}


/*
 * Abort routines
 */

static int
esp_abort_curcmd(esp)
register struct esp *esp;
{

	/*
	 * XXX We really need to do better than this! XXX
	 */

	if (esp->e_state != STATE_FREE) {
		return (esp_abort_allcmds(esp));
	} else {
		return (ACTION_RETURN);
	}
}

static int
esp_abort_allcmds(esp)
register struct esp *esp;
{
	/*
	 * Reset everything. XXX DO BETTER THAN THIS! XXX
	 */

	New_state(esp, ACTS_ABORTING);

	esp_internal_reset(esp, ESP_RESET_HW & ~ESP_RESET_IGNORE_BRESET);

	/*
	 * Now that we've reset the SCSI bus,  we'll take a SCSI RESET
	 * interrupt and use that to clean up the state of things.
	 */

	return (ACTION_RETURN);
}


/*
 * Hardware and Software internal reset routines
 */

static void
esp_internal_reset(esp, reset_action)
register struct esp *esp;
int reset_action;		/* how much to reset */
{

#ifdef	ESPDEBUG
	if (DEBUGGING) {
		reset_action |= ESP_RESET_MSG;
	}
#endif	/* ESPDEBUG */

	if (reset_action & ESP_RESET_MSG) {
		esp_printstate(esp, "Reset");
	}

	if (reset_action & ESP_RESET_HW) {
		esp_hw_reset(esp, reset_action);
	}

	if (reset_action & ESP_RESET_SOFTC) {
		esp->e_last_slot = esp->e_cur_slot;
		esp->e_cur_slot = UNDEFINED;
		bzero((caddr_t) esp->e_slots,
		    (sizeof (struct scsi_cmd *))*NTARGETS*NLUNS_PER_TARGET);
		bzero((caddr_t) esp->e_offset, NTARGETS * (sizeof (u_char)));
		bzero((caddr_t) esp->e_period, NTARGETS * (sizeof (u_char)));
		esp->e_sync_known = 0;
		esp->e_ncmds = esp->e_npolling = esp->e_ndisc = 0;
		esp->e_omsglen = 0;
		esp->e_cur_msgout[0] = 0xff;
		esp->e_last_msgout = 0xff;
		esp->e_last_msgin = 0xff;
		/*
		 * esp->e_weak && esp->e_nodisc are
		 * preserved across softc resets.
		 */
		New_state(esp, STATE_FREE);
	}
	LOG_STATE(ACTS_RESET, esp->e_stat, -1, reset_action);
}

static void
esp_hw_reset(esp, action)
struct esp *esp;
{
	register struct espreg *ep = esp->e_reg;
	register struct dmaga *dmar = esp->e_dma;
	u_char junk, i;

	if (action & ESP_RESET_DMA) {
		if (action & ESP_RESET_MSG) {
			eprintf (esp, "resetting DMA engine\n");
		}

		/*
		 * This approach should preserve any LANCE bits...
		 */
		dmar->dmaga_csr |= DMAGA_RESET;

		/*
		 * *DON'T* use a straight assign to zero!
		 * (preserve bits)
		 */

		dmar->dmaga_csr &= ~DMAGA_RESET; /* clear it */

#ifdef	ESP_NEW_HW_DEBUG
		printf("esp_hw_reset: dma rev=%x type=%x\n", DMAGA_REV(dmar),
		    esp->e_type);
#endif
		switch (DMAGA_REV(dmar)) {
		case ESC1_REV1:
			if (!(esp->e_burstsizes & BURST32))
				DMAESC_SETBURST16(dmar);
			dmar->dmaga_csr |= DMAESC_EN_ADD;
			break;
		case DMA_REV3:
			if ((esp->e_type == FAS100A) ||
			    (esp->e_type == FAST)) {
				dmar->dmaga_csr &= ~DMAGA_TURBO;
				dmar->dmaga_csr |= DMAGA_TWO_CYCLE;
			}
			if (esp->e_burstsizes & BURST32) {
				DMA2_SETBURST32(dmar);
			}
			break;

		case DMA_REV2:
			if (esp->e_type != ESP100)
				dmar->dmaga_csr |= DMAGA_TURBO;
			break;
		default:
			break;
		}
		dmar->dmaga_csr |= DMAGA_INTEN;
	}

	if (action & ESP_RESET_ESP) {
		if (action & ESP_RESET_MSG)
			eprintf (esp, "resetting ESP chip\n");

		ep->esp_cmd = CMD_RESET_ESP;	/* hard-reset ESP chip */
		ep->esp_cmd = CMD_NOP | CMD_DMA;
		ep->esp_cmd = CMD_NOP | CMD_DMA;/* FAS chips needs a 2nd NOP */

		/*
		 * Re-load chip configurations
		 */

		ep->esp_clock_conv = esp->e_clock_conv & CLOCK_MASK;

		ep->esp_timeout = esp->e_stval;
		ep->esp_sync_period = 0;
		ep->esp_sync_offset = 0;

		/*
		 * enable default configurations
		 */

		ep->esp_conf = esp->e_espconf;

		if (esp->e_type == FAST) {
			register u_char idcode, fcode;

			idcode = ep->esp_id_code;
			fcode = (ep->esp_id_code & ESP_FCODE_MASK) >> 3;
			if (fcode == ESP_FAS236)
				esp->e_type = FAS236;
			else
				esp->e_type = FAS100A;
#ifdef	ESP_TEST_FAST_SYNC
			IPRINTF2("Family code %d, revision %d\n",
				fcode, (idcode & ESP_REV_MASK));
#endif	ESP_TEST_FAST_SYNC
		}

		switch (esp->e_type) {
		case ESP236:
			IPRINTF("type is ESP236\n");
			ep->esp_conf2 = esp->e_espconf2;
			ep->esp_conf3 = esp->e_espconf3[0];
			break;

		case FAS236:
			IPRINTF("type is FAS236\n");
			for (i = 0; i < NTARGETS; i++) {
				esp->e_espconf3[i] |= ESP_CONF3_236_FASTCLK;
			}
			ep->esp_conf2 = esp->e_espconf2;
			if (esp->e_differential) {
				esp->e_req_ack_delay = 0;
			} else {
				esp->e_req_ack_delay = DEFAULT_REQ_ACK_DELAY_236;
			}
			break;

		case FAS100A:
			IPRINTF("type is FAS100A\n");
			for (i = 0; i < NTARGETS; i++) {
				esp->e_espconf3[i] |= ESP_CONF3_100A_FASTCLK;
			}
			ep->esp_conf2 = esp->e_espconf2;
			esp->e_req_ack_delay = DEFAULT_REQ_ACK_DELAY_101;
			break;

		case ESP100A:
			IPRINTF("type is ESP100A\n");
			ep->esp_conf2 = esp->e_espconf2;
			break;

		case FAST:
			IPRINTF("type is FAST\n");
			break;

		default:
			IPRINTF("type is ???\n");
			break;
		}
		/*
		 * Just in case...
		 */
		junk = ep->esp_intr;

#ifdef	ESP_TEST_FAST_SYNC
		IPRINTF1("clock conversion = %x\n",
		    esp->e_clock_conv & CLOCK_MASK);
		IPRINTF2("conf = %x (read back: %x)\n",
		    esp->e_espconf, ep->esp_conf);
		IPRINTF2("conf2 = %x (read back: %x)\n",
		    esp->e_espconf2, ep->esp_conf2);
		IPRINTF4("conf3 (for target 0 - 3) = %x %x %x %x\n",
			esp->e_espconf3[0], esp->e_espconf3[1],
			esp->e_espconf3[2], esp->e_espconf3[3]);
		IPRINTF4("conf3 (for target 4 - 7) = %x %x %x %x\n",
			esp->e_espconf3[4], esp->e_espconf3[5],
			esp->e_espconf3[6], esp->e_espconf3[7]);
		IPRINTF2("req_ack_delay (0x%x) = %x\n", &esp->e_req_ack_delay,
			esp->e_req_ack_delay);
#endif	ESP_TEST_FAST_SYNC
	}


	if (action & ESP_RESET_SCSIBUS) {
		if (action & ESP_RESET_MSG) {
			eprintf (esp, "resetting SCSI bus (%s)\n",
			    (action & ESP_RESET_IGNORE_BRESET) ?
			    "ignored" : "not ignored");
		}

		if (action & ESP_RESET_IGNORE_BRESET) {
			ep->esp_conf = ESP_CONF_DISRINT | esp->e_espconf;
			ep->esp_cmd = CMD_RESET_SCSI;
			ep->esp_conf = esp->e_espconf;
			DELAY(scsi_reset_delay);
			junk = ep->esp_intr;
		} else {
			ep->esp_cmd = CMD_RESET_SCSI;
			DELAY(scsi_reset_delay);
		}
	}
#ifdef	lint
	junk = junk;
#endif	/* lint */
}


/*
 * Error logging, printing, and debug print routines
 */

/*VARARGS3*/
static void
esplog(esp, level, fmt, a, b, c, d, e, f, g, h, i)
struct esp *esp;
int level;
char *fmt;
int a, b, c, d, e, f, g, h, i;
{
	int tmp;
	auto char buf[64];
	(void) sprintf(buf, "esp%d: ", CNUM);
	(void) sprintf(&buf[strlen(buf)], fmt, a, b, c, d, e, f, g, h, i);
	tmp = strlen(buf);
	buf[tmp++] = '\n';
	buf[tmp] = 0;
	log (level, buf);
}

/*VARARGS2*/
static void
eprintf(esp, fmt, a, b, c, d, e, f, g, h, i)
struct esp *esp;
char *fmt;
int a, b, c, d, e, f, g, h, i;
{
	printf("esp%d:\t", CNUM);
	printf(fmt, a, b, c, d, e, f, g, h, i);
}


static void
esp_stat_int_print(esp)
struct esp *esp;
{
	printf("\tStat=0x%b, Intr=0x%b\n", esp->e_stat,
		esp_stat_bits, esp->e_intr, esp_int_bits);
}


static void
esp_printstate(esp, msg)
register struct esp *esp;
char *msg;
{
	register struct espreg *ep = esp->e_reg;
	register struct scsi_cmd *sp;
	register struct dmaga *dmar = esp->e_dma;
	u_char fifo_flag;

	eprintf(esp, "%s\n", msg);
	printf("\tState=%s Last State=%s\n", esp_state_name(esp->e_state),
	    esp_state_name(esp->e_laststate));

	/*
	 * disable DVMA to avoid a timeout on SS1
	 */
	if (dmar->dmaga_csr & DMAGA_ENDVMA) {
		dmar->dmaga_csr &= ~DMAGA_ENDVMA;
		fifo_flag = ep->esp_fifo_flag;
		dmar->dmaga_csr |= DMAGA_ENDVMA;
	} else {
		fifo_flag = ep->esp_fifo_flag;
	}

	printf("\tLatched stat=0x%b intr=0x%b fifo 0x%x\n", esp->e_stat,
	    esp_stat_bits, esp->e_intr, esp_int_bits, fifo_flag);
	printf("\tlast msg out: %s; last msg in: %s\n",
	    scsi_mname(esp->e_last_msgout), scsi_mname(esp->e_last_msgin));
	printf("\tDMA csr=0x%b\n\taddr=%x last=%x last_count=%x\n",
	    esp->e_dma->dmaga_csr, dmaga_bits, esp->e_dma->dmaga_addr,
	    esp->e_lastdma, esp->e_lastcount);
	if (esp->e_cur_slot != UNDEFINED && (sp = CURRENT_CMD(esp))) {
		esp_dump_cmd(sp);
	}
#ifdef	ESPDEBUG
	esp_dump_state(esp);
#endif	/* ESPDEBUG */
}

#ifdef	ESPDEBUG
static void
esp_dump_state(esp)
register struct esp *esp;
{
	register short x, z;
	auto char buf[128];

	z = esp->e_phase_index;
	for (x = 1; x <= NPHASE; x++) {
		register short y;

		z = --z & (NPHASE - 1);
		y = esp->e_phase[z].e_save_state;
		if (y == STATE_FREE)
			break;

		(void) sprintf(&buf[0], "\tcurrent phase 0x%x=%s",
			y, esp_state_name((u_short)y));

		(void) sprintf(&buf[strlen(buf)], "\tstat=0x%x",
			esp->e_phase[z].e_save_stat);

		if (esp->e_phase[z].e_val1 != -1) {
			(void) sprintf(&buf[strlen(buf)], "\t0x%x",
				esp->e_phase[z].e_val1);
		}

		if (esp->e_phase[z].e_val2 != -1) {
			(void) sprintf(&buf[strlen(buf)], "\t0x%x",
				esp->e_phase[z].e_val2);
		}
		printf("%s\n", buf);
	}
}
#endif	/* ESPDEBUG */

static void
esp_dump_cmd(sp)
struct scsi_cmd *sp;
{
	register i;
	register u_char *cp = (u_char *) sp->cmd_pkt.pkt_cdbp;

	printf("\tCmd dump for Target %d Lun %d:\n", Tgt(sp), Lun(sp));
	printf("\tcdb=[");
	for (i = 0; i < sp->cmd_cdblen; i++) {
		printf(" 0x%x", *cp++);
	}
	if (sp->cmd_pkt.pkt_state & STATE_GOT_STATUS)
		printf(" ]; Status=0x%x\n", sp->cmd_pkt.pkt_scbp[0]);
	else
		printf(" ]\n");
	printf("\tpkt_state 0x%b pkt_flags 0x%x pkt_statistics 0x%x\n",
	    sp->cmd_pkt.pkt_state, state_bits, sp->cmd_pkt.pkt_flags,
	    sp->cmd_pkt.pkt_statistics);
	printf("\tcmd_flags=0x%x cmd_timeout %d\n", sp->cmd_flags,
	    sp->cmd_timeout);

	esp_dump_datasegs(sp);
}

static void
esp_dump_datasegs(sp)
struct scsi_cmd *sp;
{
	register struct dataseg *seg;

	printf("\tMapped Dma Space:\n");
	for (seg = &sp->cmd_mapseg; seg; seg = seg->d_next) {
		printf("\t\tBase = 0x%x Count = 0x%x\n",
		    seg->d_base, seg->d_count);
	}
	printf("\tTransfer History:\n");
	for (seg = &sp->cmd_subseg; seg; seg = seg->d_next) {
		printf("\t\tBase = 0x%x Count = 0x%x\n",
		    seg->d_base, seg->d_count);
	}
}

static char *
esp_state_name(state)
u_char state;
{
	if (state == STATE_FREE) {
		return ("FREE");
	} else if ((state & STATE_SELECTING) &&
		    (!(state & ACTS_LOG))) {
		if (state == STATE_SELECT_NORMAL)
			return ("SELECT");
		else if (state == STATE_SELECT_N_STOP)
			return ("SEL&STOP");
		else if (state == STATE_SELECT_N_SENDMSG)
			return ("SELECT_SNDMSG");
		else
			return ("SEL_NO_ATN");
	} else {
		static struct {
			char *sname;
			char state;
		} names[] = {
			"CMD_START",		ACTS_CMD_START,
			"CMD_DONE",		ACTS_CMD_DONE,
			"MSG_OUT",		ACTS_MSG_OUT,
			"MSG_OUT_DONE",		ACTS_MSG_OUT_DONE,
			"MSG_IN",		ACTS_MSG_IN,
			"MSG_IN_MORE",		ACTS_MSG_IN_MORE,
			"MSG_IN_DONE",		ACTS_MSG_IN_DONE,
			"CLEARING",		ACTS_CLEARING,
			"DATA",			ACTS_DATA,
			"DATA_DONE",		ACTS_DATA_DONE,
			"CMD_CMPLT",		ACTS_C_CMPLT,
			"UNKNOWN",		ACTS_UNKNOWN,
			"RESEL",		ACTS_RESEL,
			"RESET",		ACTS_RESET,
			"ABORTING",		ACTS_ABORTING,
			"SPANNING",		ACTS_SPANNING,
			"FROZEN",		ACTS_FROZEN,
			"PREEMPTED",		ACTS_PREEMPTED,
			"PROXY",		ACTS_PROXY,
			"SYNCHOUT",		ACTS_SYNCHOUT,
			"CMD_LOST",		ACTS_CMD_LOST,
			"DATAOUT",		ACTS_DATAOUT,
			"DATAIN",		ACTS_DATAIN,
			"STATUS",		ACTS_STATUS,
			"DISCONNECT",		ACTS_DISCONNECT,
			"NOP",			ACTS_NOP,
			"REJECT",		ACTS_REJECT,
			"RESTOREDP",		ACTS_RESTOREDP,
			"SAVEDP",		ACTS_SAVEDP,
			"BAD_RESEL",		ACTS_BAD_RESEL,
			0
		};
		register i;
		for (i = 0; names[i].sname; i++) {
			if (names[i].state == state)
				return (names[i].sname);
		}
	}
	return ("<BAD>");
}

#endif	/* NESP > 0 */
