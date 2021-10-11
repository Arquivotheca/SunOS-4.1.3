/*
 * @(#)si.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*#define SIDEBUG 		/* Allow compiling of debug code */
#define REL4			/* Enable release 4 mods */

#ifdef	REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <stand/sireg.h>
#include <stand/scsi.h>
#include <stand/saio.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#else REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "sireg.h"
#include "scsi.h"
#include "saio.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#endif REL4

#ifdef SIDEBUG
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (scsi_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (scsi_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (scsi_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (scsi_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (scsi_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (scsi_debug) printf(str,arg1,arg2)

#else SIDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SIDEBUG

#ifdef BOOTBLOCK
#define SC_ERROR(str)
#define SC_ERROR1(str, arg1)
#else BOOTBLOCK
#define SC_ERROR	printf
#define SC_ERROR1	printf
#endif BOOTBLOCK

#define SHORT_RESET	0
#define LONG_RESET	1

/*
 * Low-level routines common to all devices on the SCSI bus.
 *
 * Interface to the routines in this module is via a second "h_sip"
 * structure contained in the caller's local variables.
 */

/* How si addresses look to the si vme scsi dma hardware */
#define	SI_VME_DMA_ADDR(x)	(((int)x)&0x000FFFFF)

/* How si addresses look to the sun3/50 scsi dma hardware */
#define	SI_OB_DMA_ADDR(x)	(((int)x)&0x00FFFFFF)

/* How si addresses look to the sun4/110 scsi dma hardware */
#define	SI_COBRA_DMA_ADDR(x)	(((int)x)&0x00FFFFFF)


/*
 * The interfaces we export
 */
extern int scsi_debug;
extern char *devalloc();
extern char *resalloc();
extern int nullsys();
extern u_char sc_cdb_size[];
int siopen(), sidoit(), si_reset();
static u_char junk;

#define SI_VME_BASE	0x200000
#define SI_OB_BASE	0x140000
#define SI_SIZE		0x4000

struct sidma {
	struct udc_table	udct;	/* dma information for udc */
};

/*
 * Open the SCSI host adapter.
 */
int
siopen(h_sip)
	register struct host_saioreq *h_sip;
{
	register struct scsi_si_reg *sir;
	register int ctlr;
	register int base;
	register int si_nstd;
	struct idprom id;
	enum MAPTYPES space;
	DPRINTF("siopen:\n");

	/* determine type of si interface */
	if (idprom(IDFORM_1, &id) == IDFORM_1) {
		switch (id.id_machine) {

		case IDM_SUN3_F:
		case IDM_SUN3_M25:
			EPRINTF("siopen: CPU SCSI-3\n");
			h_sip->ob = 1;
			base = SI_OB_BASE;
			space = MAP_OBIO;
			si_nstd = 1;
			break;

	        default:
			EPRINTF("siopen: VME SCSI-3\n");
			h_sip->ob = 0;
			base = SI_VME_BASE;
			space = MAP_VME24A16D;
			si_nstd = SC_NSC;
			break;
		}
	} else {
		EPRINTF("siopen: failed\n");
		return (-1);
	}

	/* Get base address of registers */
	if (h_sip->ctlr <= si_nstd) {
		ctlr = base + ((int)h_sip->ctlr * SI_SIZE);
		DPRINTF1("siopen: ctlr= 0x%x\n", (int)ctlr);
	} else {
		if ((int)h_sip->devaddr == 0) {
			EPRINTF("siopen: devalloc failure\n");
			return (-2);
		} else {
			DPRINTF("siopen: reg. already allocated\n");
			return (0);
		}
	}

	/* Map in device registers */
	h_sip->devaddr = devalloc(space, (char *)ctlr, 
	    sizeof(struct scsi_si_reg));

	if ((int)h_sip->devaddr == 0) {
		EPRINTF("siopen: devalloc failure\n");
		return (-2);
	}
	sir = (struct scsi_si_reg *)(h_sip->devaddr);

	/*
	 * If not on-board, peek past 2K bytes to make sure this is
	 * a SCSI-3 host adaptor
	 */
#ifndef	sun4
	if (!h_sip->ob  &&
	    ((peek((short *)&sir->dma_addr) == -1)  ||
	     (peek((short *)((int)sir +0x800)) != -1))) {
		EPRINTF("siopen: peek failed\n");
		return (-1);
	}
#else	sun4
	if (!h_sip->ob  &&
	    ((peek((short *)&sir->dma_addrl) == -1)  ||
	     (peek((short *)((int)sir +0x800)) != -1))) {
		EPRINTF("siopen: peek failed\n");
		return (-1);
	}
#endif	sun4

	/* Allocate dma resources */
	h_sip->dmaaddr = 0;
	if (h_sip->ob == 1) {
		h_sip->dmaaddr = resalloc(RES_DMAMEM, sizeof(struct sidma));
		DPRINTF1("siopen: dmaaddr= 0x%x\n", (int)h_sip->dmaaddr);
		if (h_sip->dmaaddr == 0) {
			EPRINTF("siopen: resalloc failure\n");
			return (-2);
		}
	}
	/* set host adapter's address modifier bits */
	sir->iv_am = VME_SUPV_DATA_24;

	/* Link top level driver to host adapter */
	h_sip->doit = sidoit;
	h_sip->reset = si_reset;

	si_reset(h_sip, SHORT_RESET);
	return (0);
}


/*
 * Write a command to the SCSI bus.
 *
 * The supplied h_sip is the one opened by scsi_probe.
 * DMA is done based on h_sip->ma and h_sip->cc.
 *
 * Returns -1 for error, otherwise returns the residual count not DMAed
 * (zero for success).
 */
static int
sidoit(cdb, scb, h_sip)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	register struct host_saioreq *h_sip;	/* sip for host adapter */
{
	register struct scsi_si_reg *sir;
	register u_char *cp;
	u_char size;
	register int b;

	DPRINTF("sidoit:\n");
	/* Get to scsi control logic registers */
	sir = (struct scsi_si_reg *) h_sip->devaddr;
	DPRINTF1("sidoit: sir= 0x%x\n", (int)sir);

	/* Handle SCSI-3 case */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("si: bus busy\n");
		goto FAILED;
	}

	/* Select target */
	DPRINTF1("sidoit: unit= 0x%x\n", h_sip->unit);
	SBC_WR.odr = (1 << h_sip->unit) | SI_HOST_ID;
	SBC_WR.icr = SBC_ICR_DATA;
	SBC_WR.icr |= SBC_ICR_SEL;

	/* Wait for target to acknowledge our selection */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SEL_DELAY, 1) == 0) {
		EPRINTF1("si: device offline, cbsr= 0x%x\n", SBC_RD.cbsr);
		SBC_WR.icr = 0;
		return (-2);
	}
	SBC_WR.icr = 0;

	/* Do initial dma setup */
	sir->bcr = 0;	/* also reset dma_count for vme */

	if (h_sip->cc > 0) {
		if (h_sip->dma_dir == SC_RECV_DATA) {
			DPRINTF("sidoit: DMA receive\n");
			sir->csr &= ~SI_CSR_SEND;
		} else {
			DPRINTF("sidoit: DMA send\n");
			sir->csr |= SI_CSR_SEND;
		}
		sir->csr &= ~SI_CSR_FIFO_RES;
		sir->csr |= SI_CSR_FIFO_RES;	
		sir->bcr = h_sip->cc;
		SET_DMA_COUNT(sir, 0);

		if (h_sip->ob == 0)
			sir->bcrh = 0;
	}

	/* Put command onto scsi bus */
	cp = (u_char *)cdb;
	size = sc_cdb_size[CDB_GROUPID(*cp)];

	if (si_putbyte(sir, PHASE_COMMAND, cp, size) == 0) {
		SC_ERROR("si: cmd put failed\n");
		goto FAILED;
	}
	if (h_sip->cc > 0) {

		/* Finish dma setup and wait for dma completion */
		if ((int)h_sip->ma & 1) {
			SC_ERROR("si: illegal odd dma starting address\n");
			goto FAILED;
		}

		if (h_sip->ob) {
			si_ob_dma_setup(sir, h_sip);
		} else {
			if ((int)h_sip->ma & 0x2)
				sir->csr |= SI_CSR_BPCON;
			else
				sir->csr &= ~SI_CSR_BPCON;
			SET_DMA_ADDR(sir, SI_VME_DMA_ADDR(h_sip->ma));
			SET_DMA_COUNT(sir, h_sip->cc);
		}
		si_dma_setup(h_sip, sir);

	/* No dma, wait for target to request a byte */
	} else if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		   SCSI_LONG_DELAY, 1) == 0) {

		SC_ERROR("si: target never set REQ\n");
		goto FAILED;
	}

	/* Get status */
	junk = SBC_RD.clr;
	cp = (u_char *)scb;
	cp[0] = si_getbyte(sir, PHASE_STATUS);
	b = si_getbyte(sir, PHASE_MSG_IN);

	if (b != SC_COMMAND_COMPLETE) {
		EPRINTF("sidoit: no cmd complete msg\n");
		/* If b < 0, si_getbyte already printed msg */
		if (b >= 0)
			SC_ERROR("si: invalid message\n");
		goto FAILED;
	}
	return (h_sip->cc - sir->bcr);

FAILED:
	EPRINTF("sidoit: failed\n");
	si_reset(h_sip, LONG_RESET);
	return (-1);
}


static
si_dma_setup(h_sip, sir)
	register struct host_saioreq *h_sip;
	register struct scsi_si_reg *sir;
{       
	register char *cp;
	DPRINTF("si_dma_setup:\n");

	/* Setup sbc and start dma */
	if (h_sip->dma_dir == SC_RECV_DATA) {
		DPRINTF("si_dma_setup: RECV\n");
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.tcr = TCR_DATA_IN;
		SBC_WR.ircv = 0;
	} else {
		DPRINTF("si_dma_setup: SEND\n");
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.tcr = TCR_DATA_OUT;
		SBC_WR.icr = SBC_ICR_DATA;
		SBC_WR.send = 0;
	}
	if (h_sip->ob == 0) {
		sir->csr |= SI_CSR_DMA_EN;
	}

	/* Wait for dma completion */
	if (si_wait(&sir->csr, 
		   SI_CSR_SBC_IP | SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT, 1)
	           == 0) {
		SC_ERROR("si: dma never completed\n");
		si_dma_cleanup(sir, h_sip);
		goto FAILED;
	}
	/* Check reason for dma completion */
	if (sir->csr & SI_CSR_SBC_IP) {
		/* Dma operation should done now. */
		si_dma_cleanup(sir, h_sip);
	} else {
		DPRINTF("sidoit: ha error exit\n");
		if (sir->csr & SI_CSR_DMA_CONFLICT) {
			SC_ERROR("si: invalid reg access during dma\n");
		} else if (sir->csr & SI_CSR_DMA_BUS_ERR) {	
			SC_ERROR("si: bus error during dma\n");
		} else {
			SC_ERROR("si: dma overrun\n");
		}
		si_dma_cleanup(sir, h_sip);
		goto FAILED;
	}

	/* Handle special dma recv situations */
	if (h_sip->dma_dir == SC_RECV_DATA) {
		if (h_sip->ob) {
			sir->udc_raddr = UDC_ADR_COUNT;
			if (si_wait(&sir->csr, SI_CSR_FIFO_EMPTY, 1) == 0) {
				SC_ERROR("si: fifo never emptied\n");
				si_dma_cleanup(sir, h_sip);
				goto FAILED;
			}
			/* If odd byte recv, must grab last byte by hand */
			if ((h_sip->cc - sir->bcr) & 1) {
				DPRINTF("si_dma_setup: lob 1\n");
				cp = h_sip->ma + (h_sip->cc - sir->bcr) - 1;
				*cp = (sir->fifo_data & 0xff00) >> 8;

			/* Udc may not dma last word */
			} else if (((sir->udc_rdata *2) - sir->bcr) == 2) {
				DPRINTF("si_dma_setup: lob 2\n");
				cp = h_sip->ma + (h_sip->cc - sir->bcr);
				*(cp - 2) = (sir->fifo_data & 0xff00) >> 8;
				*(cp - 1) = sir->fifo_data & 0x00ff;
			}

		} else if ((sir->csr & SI_CSR_LOB) != 0) {
			cp = h_sip->ma + (h_sip->cc - sir->bcr);
			if ((sir->csr & SI_CSR_BPCON) == 0) {
			    switch (sir->csr & SI_CSR_LOB) {
			    case SI_CSR_LOB_THREE:
				    DPRINTF("si_dma_setup: lob 3\n");
				    *(cp - 3) = (sir->bprh & 0xff00) >> 8;
				    *(cp - 2) = (sir->bprh & 0x00ff);
				    *(cp - 1) = (sir->bprl & 0xff00) >> 8;
				    break;
			    case SI_CSR_LOB_TWO:
				    DPRINTF("si_dma_setup: lob 2\n");
				    *(cp - 2) = (sir->bprh & 0xff00) >> 8;
				    *(cp - 1) = (sir->bprh & 0x00ff);
				    break;
			    case SI_CSR_LOB_ONE:
				    DPRINTF("si_dma_setup: lob 1\n");
				    *(cp - 1) = (sir->bprh & 0xff00) >> 8;
				    break;
			    }
			} else {
				*(cp - 1) = (sir->bprl & 0xff00) >> 8;
			}
		}
	}
	/* Clear sbc interrupt */
FAILED:
	junk = SBC_RD.clr;
	return;
}


static int
si_ob_dma_setup(sir, h_sip)
	register struct scsi_si_reg *sir;
	register struct host_saioreq *h_sip;
{
	register struct udc_table *udct;
	register int dmaaddr;
	udct = (struct udc_table *)(h_sip->dmaaddr); 

	DPRINTF("si_ob_setup:\n");

	/* Setup udc dma info */
	dmaaddr = SI_OB_DMA_ADDR(h_sip->ma);
	udct->haddr = ((dmaaddr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	udct->laddr = dmaaddr & 0xffff;
	udct->hcmr = UDC_CMR_HIGH;
	udct->count = h_sip->cc / 2;

	if (h_sip->dma_dir == SC_RECV_DATA) {
		DPRINTF("si_ob_setup: DMA receive\n");
		udct->rsel = UDC_RSEL_RECV;
		udct->lcmr = UDC_CMR_LRECV;
	} else {
		DPRINTF("si_ob_setup: DMA send\n");
		udct->rsel = UDC_RSEL_SEND;
		udct->lcmr = UDC_CMR_LSEND;
		if (h_sip->cc & 1) {
			udct->count++;
		}
	}

	/* Initialize chain address register */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_CAR_HIGH;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = ((int)udct & 0xff0000) >> 8;
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_CAR_LOW;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = (int)udct & 0xffff;

	/* Initialize master mode register */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_MODE;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_MODE;

	/* Issue start chain command */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_COMMAND;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_CMD_STRT_CHN;
}


/*
 * Reset some register information after a dma operation.
 */
static int
si_dma_cleanup(sir, h_sip)
	register struct scsi_si_reg *sir;
	register struct host_saioreq *h_sip;
{
	DPRINTF("si_dma_cleanup:\n");

	if (h_sip->ob) {
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(SI_UDC_WAIT);
		sir->udc_rdata = UDC_CMD_RESET;
	} else {
		sir->csr &= ~SI_CSR_DMA_EN;
		SET_DMA_ADDR(sir, 0);
	}
	SBC_WR.mr &= ~SBC_MR_DMA;
	SBC_WR.icr = 0;
	SBC_WR.tcr = TCR_UNSPECIFIED;
}


/*
 * Wait for a condition to be (de)asserted.
 */
static int
si_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	int i;
	register u_short regval;
	/* DPRINTF("si_wait:\n"); */

	for (i = 0; i < SCSI_LONG_DELAY; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}


/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 */
static int
si_sbc_wait(reg, cond, delay, set)
	register caddr_t reg;
	register u_char cond;
	register int set;
	int delay;
{
	register u_char regval;
	int i;
	/* DPRINTF("si_sbc_wait:\n"); */

	for (i = 0; i < delay; i++) {
		regval = *reg;
		if ((set == 1) && (regval & cond)) {
			return (1);
		}
		if ((set == 0) && !(regval & cond)) {
			return (1);
		} 
		DELAY(10);
	}
	return (0);
}


/*
 * Put a byte onto the scsi bus.
 */
static int
si_putbyte(sir, phase, data, num)
	register struct scsi_si_reg *sir;
	register u_short phase;
	register u_char *data;
	register u_char num;
{
	register int i;
	register u_char icr;
	DPRINTF("si_putbyte:\n");

	/* Set up tcr so a phase match will occur */
	sir->sbc_wreg.tcr = phase >> 2;
	icr = SBC_WR.icr;

	/* Put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++) {

		SBC_WR.icr = icr | SBC_ICR_DATA; /* clear ack */

		/* Wait for target to request a byte */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 1) == 0) {
			SC_ERROR1("si: REQ not active, cbsr 0x%x\n",
				SBC_RD.cbsr);
			return (0);
		}

		/* Load data for transfer */
		SBC_WR.odr = *data++;

		/* Complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 0) == 0) {
			SC_ERROR("si: target never released REQ\n");
			return (0);
		}

	}
	SBC_WR.tcr = 0;
	SBC_WR.icr = 0;
	return (1);
}


/*
 * Get a byte from the scsi bus.
 */
static int
si_getbyte(sir, phase)
	register struct scsi_si_reg *sir;
	u_short phase;
{
	u_char data;
	DPRINTF("si_getbyte:\n");

	/* Wait for target request */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 1) == 0) {
		SC_ERROR1("si: REQ not active, cbsr 0x%x\n",
			SBC_RD.cbsr);
		goto FAILED;
	}

	/* Check for correct phase on scsi bus */
	if (phase != (SBC_RD.cbsr & CBSR_PHASE_BITS)) {
		SC_ERROR1("si: wrong phase, cbsr 0x%x\n",
			SBC_RD.cbsr);
		goto FAILED;
	}

	/* Grab data */
	data = SBC_RD.cdr;
	SBC_WR.icr = SBC_ICR_ACK;

	/* Complete req/ack handshake */
	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("si: target never released REQ\n");
		goto FAILED;
	}
	SBC_WR.tcr = 0;
	SBC_WR.icr = 0;		/* Clear ack */
	return (data);

FAILED:
	SBC_WR.tcr = 0;
	SBC_WR.icr = 0;		/* Clear ack */
	return (-1);
}


/*
 * Reset SCSI control logic.
 */
static int
si_reset(h_sip, flag)
	register struct host_saioreq *h_sip;
	int flag;
{
	register struct scsi_si_reg *sir;
	DPRINTF("si_reset:\n");

	sir = (struct scsi_si_reg *) h_sip->devaddr;

	/* Reset bcr, fifo, udc, and sbc */
	sir->csr = 0;
	DELAY(10);
	sir->bcr = 0;
	sir->csr = SI_CSR_SCSI_RES | SI_CSR_FIFO_RES;
	if(h_sip->ob == 0) {
		SET_DMA_ADDR(sir, 0);
		SET_DMA_COUNT(sir, 0);
	}

	/* Issue scsi bus reset */
	if (flag != SHORT_RESET) {
		SBC_WR.icr = SBC_ICR_RST;
		DELAY(100);
		SBC_WR.icr = 0;
		SBC_WR.tcr = 0;
		SBC_WR.mr = 0;
		junk = SBC_RD.clr;
		DELAY(SCSI_RESET_DELAY);	/* Recovery time */
	}
}
