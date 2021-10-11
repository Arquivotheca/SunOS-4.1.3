/*
 * @(#)sw.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*#define SWDEBUG 	/* Allow compiling of debug code */
#define REL4		/* Enable release 4 mods */

#ifdef	REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <stand/swreg.h>
#include <stand/scsi.h>
#include <stand/saio.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#else REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "swreg.h"
#include "scsi.h"
#include "saio.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#endif REL4

#ifdef SWDEBUG
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

#else SWDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SWDEBUG

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

/* How sw addresses look to the sun4/110 scsi dma hardware */
#define	SW_COBRA_DMA_ADDR(x)	(((int)x)&0x00FFFFFF)


/*
 * The interfaces we export
 */
extern int scsi_debug;
extern char *devalloc();
extern char *resalloc();
extern int nullsys();
extern u_char sc_cdb_size[];
int swopen(), swdoit(), sw_reset();

#define	COBRA 1

#define	SW_COBRA_BASE	0xfa000000
#define SW_SIZE		0x4000

#ifdef	sun4
/*
 * Open the SCSI host adapter.
 */
int
swopen(h_sip)
	register struct host_saioreq *h_sip;
{
	register int base;
	struct idprom id;
	enum MAPTYPES space;
	DPRINTF("swopen:\n");

	/* determine type of sw interface */
	if (idprom(IDFORM_1, &id) == IDFORM_1  &&
	    h_sip->ctlr == 0  &&
	    id.id_machine == IDM_SUN4_COBRA) {
		EPRINTF("swopen: CPU SCSI\n");
		base = SW_COBRA_BASE;
		space = MAP_OBIO;
	} else {
		DPRINTF("swopen: failed\n");
		return (-1);
	}

	/* Map in device registers */
	h_sip->devaddr = devalloc(space, (char *)base, 
	    sizeof(struct scsi_sw_reg));

	if ((int)h_sip->devaddr == 0) {
		EPRINTF("swopen: devalloc failure\n");
		return (-2);
	}

	/* Allocate dma resources */
	h_sip->dmaaddr = 0;

	/* Link top level driver to host adapter */
	h_sip->doit = swdoit;
	h_sip->reset = sw_reset;

	sw_reset(h_sip, SHORT_RESET);
	return (0);
}


/*
 * Write a command to the SCSI bus.
 *
 * The supplied sip is the one opened by swopen().
 * DMA is done based on sip->si_ma and sip->si_cc.
 *
 * Returns -1 for error, otherwise returns the residual count not DMAed
 * (zero for success).
 *
 */
static int
swdoit(cdb, scb, h_sip)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	register struct host_saioreq *h_sip;	/* sip for host adapter */
{
	register struct scsi_sw_reg *swr;
	register char junk;
	register u_char *cp;
	u_char size;
	register int i, b;

	DPRINTF("swdoit:\n");
	/* Get to scsi control logic registers */
	swr = (struct scsi_sw_reg *) h_sip->devaddr;
	DPRINTF1("swdoit: swr= 0x%x\n", (int)swr);

	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("sw: bus busy\n");
		goto FAILED;
	}

	/* Select target */
	DPRINTF1("swdoit: unit= 0x%x\n", h_sip->unit);
	SBC_WR.odr = (1 << h_sip->unit) | SI_HOST_ID;
	SBC_WR.icr = SBC_ICR_DATA;
	SBC_WR.icr |= SBC_ICR_SEL;

	/* Wait for target to acknowledge our selection */
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SEL_DELAY, 1) == 0) {
		EPRINTF1("sw: device offline, cbsr= 0x%x\n", SBC_RD.cbsr);
		SBC_WR.icr = 0;
		return (-2);
	}
	SBC_WR.icr = 0;

	/* Do initial dma setup */
	swr->bcr = 0;	/* also reset dma_count for vme */

	if (h_sip->cc > 0) {
		if (h_sip->dma_dir == SC_RECV_DATA) {
			DPRINTF("swdoit: DMA receive\n");
			swr->csr &= ~SI_CSR_SEND;
		} else {
			DPRINTF("swdoit: DMA send\n");
			swr->csr |= SI_CSR_SEND;
		}
	}

	/* Put command onto scsi bus */
	cp = (u_char *)cdb;
	size = sc_cdb_size[CDB_GROUPID(*cp)];

	if (sw_putbyte(swr, PHASE_COMMAND, cp, size) == 0) {
		SC_ERROR("sw: cmd put failed\n");
		goto FAILED;
	}
	if (h_sip->cc > 0) {
		DPRINTF("swdoit: dma enable\n");

		/* Finish dma setup and wait for dma completion */
		if (((int)h_sip->ma & 1) || ((int)h_sip->ma & 2)) {
			SC_ERROR("sw: illegal odd dma starting address\n");
			goto FAILED;
		}

		SET_DMA_ADDR(swr, SW_COBRA_DMA_ADDR(h_sip->ma));
		SET_DMA_COUNT(swr, h_sip->cc);
		sw_dma_setup(swr, h_sip);

	/* No dma, wait for target to request a byte */
	} else if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		   SCSI_LONG_DELAY, 1) == 0) {

		SC_ERROR("sw: target never set REQ\n");
		goto FAILED;
	}

	/* Get status */
	junk = SBC_RD.clr;
#ifdef	lint
	junk = junk;
#endif	lint
	cp = (u_char *)scb;
	cp[0] = sw_getbyte(swr, PHASE_STATUS);
	b = sw_getbyte(swr, PHASE_MSG_IN);

	if (b != SC_COMMAND_COMPLETE) {
		EPRINTF("swdoit: no cmd complete msg\n");
		/* If b < 0, sw_getbyte already printed msg */
		if (b >= 0)
			SC_ERROR("sw: invalid message\n");
		goto FAILED;
	}

	/* Compute number of bytes actually transferred. */
	if (h_sip->cc == 0)
		return (0);

	i = SW_COBRA_DMA_ADDR(swr->dma_addr | 0xFF000000);
	return ( (int)(i - SW_COBRA_DMA_ADDR(h_sip->ma)) );

FAILED:
	EPRINTF("swdoit: failed\n");
	sw_reset(h_sip, LONG_RESET);
	return (-1);
}

static int
sw_dma_setup(swr, h_sip)
	register struct scsi_sw_reg *swr;
	register struct host_saioreq *h_sip;
{       
	register char junk;
	register char *cp;

	DPRINTF("sw_dma_setup:\n");
	/* setup sbc and start dma */
	SBC_WR.mr |= SBC_MR_DMA;
	if (h_sip->dma_dir == SC_RECV_DATA) {
		SBC_WR.tcr = TCR_DATA_IN;
		SBC_WR.ircv = 0;
	} else {
		SBC_WR.tcr = TCR_DATA_OUT;
		SBC_WR.icr = SBC_ICR_DATA;
		SBC_WR.send = 0;
	}
	swr->csr |= SI_CSR_DMA_EN;

	/* wait for dma completion */
	if (sw_wait(&swr->csr, 
	    SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR, 1) == 0) {
		SC_ERROR("sw: dma never completed\n");
		swr->csr &= ~SI_CSR_DMA_EN;
		sw_dma_cleanup(swr);
		goto FAILED;
	}

	/* check reason for dma completion */
	if (swr->csr & SI_CSR_SBC_IP) {
			/* dma operation should end with a phase mismatch */
		swr->csr &= ~SI_CSR_DMA_EN;
		(void) sw_sbc_wait((caddr_t)&SBC_RD.bsr, SBC_BSR_PMTCH,
			SCSI_SHORT_DELAY, 0);
	} else {
		swr->csr &= ~SI_CSR_DMA_EN;
		if (swr->csr & SI_CSR_DMA_CONFLICT) {
			SC_ERROR("sw: illegal reg access during dma\n");
		} else if (swr->csr & SI_CSR_DMA_BUS_ERR) {	
			SC_ERROR("sw: bus error during dma\n");
		} else {
			SC_ERROR("sw: dma overrun\n");
		}
		sw_dma_cleanup(swr);
		goto FAILED;
	}

	/* handle special dma recv situations */
	if (h_sip->dma_dir == SC_RECV_DATA) {
		cp = (char *)((swr->dma_addr&0xFFFFFC) | 0xff000000);
		switch (swr->dma_addr & 0x3) {
		    case 3:
			    *cp = (swr->bpr & 0xff000000) >> 24;
			    *(cp + 1) = (swr->bpr & 0x00ff0000) >> 16;
			    *(cp + 2) = (swr->bpr & 0x0000ff00) >> 8;
			    break;
		    case 2:
			    *cp = (swr->bpr & 0xff000000) >> 24;
			    *(cp + 1) = (swr->bpr & 0x00ff0000) >> 16;
			    break;
		    case 1:
			    *cp = (swr->bpr & 0xff000000) >> 24;
			    break;
		}
	}

	/* clear sbc interrupt */
	junk = SBC_RD.clr;
#ifdef	lint
	junk = junk;
#endif	lint

	/* Cleanup after a dma operation */
	sw_dma_cleanup(swr);

FAILED:
	return;

}


/*
 * Reset some register information after a dma operation.
 */
static int
sw_dma_cleanup(swr)
	register struct scsi_sw_reg *swr;
{
	DPRINTF("sw_dma_cleanup:\n");

	swr->csr &= ~SI_CSR_DMA_EN;
	SBC_WR.mr &= ~SBC_MR_DMA;
	SBC_WR.icr = 0;
	SBC_WR.tcr = 0;
}


/*
 * Wait for a condition to be (de)asserted.
 */
static int
sw_wait(reg, cond, set)
	register u_int *reg;
	register u_int cond;
	register int set;
{
	register int i;
	register u_int regval;

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
sw_sbc_wait(reg, cond, delay, set)
	register caddr_t reg;
	register u_char cond;
	register int delay;
	register int set;
{
	register int i;
	register u_char regval;
	DPRINTF("sw_sbc_wait:\n");

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
sw_putbyte(swr, phase, data, num)
	register struct scsi_sw_reg *swr;
	register u_short phase;
	register u_char *data;
	register u_char num;
{
	register int i;
	u_char icr;
	DPRINTF("sw_putbyte:\n");

	/* Set up tcr so a phase match will occur */
	swr->sbc_wreg.tcr = phase >> 2;
	icr = SBC_WR.icr;

	/* Put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++) {

		SBC_WR.icr = icr | SBC_ICR_DATA; /* clear ack */

		/* Wait for target to request a byte */
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 1) == 0) {
			SC_ERROR1("sw: REQ not active, cbsr 0x%x\n",
				SBC_RD.cbsr);
			return (0);
		}

		/* Load data for transfer */
		SBC_WR.odr = *data++;

		/* Complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 0) == 0) {
			SC_ERROR("sw: target never released REQ\n");
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
sw_getbyte(swr, phase)
	register struct scsi_sw_reg *swr;
	register u_short phase;
{
	register u_char data;
	DPRINTF("sw_getbyte:\n");

	/* Wait for target request */
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 1) == 0) {
		SC_ERROR1("sw: REQ not active, cbsr 0x%x\n",
			SBC_RD.cbsr);
		SBC_WR.tcr = 0;
		return (-1);
	}

	/* Check for correct phase on scsi bus */
	if (phase != (SBC_RD.cbsr & CBSR_PHASE_BITS)) {
		SC_ERROR1("sw: wrong phase, cbsr 0x%x\n",
			SBC_RD.cbsr);
		return (-1);
	}

	/* Grab data */
	data = SBC_RD.cdr;
	SBC_WR.icr = SBC_ICR_ACK;

	/* Complete req/ack handshake */
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("sw: target never released REQ\n");
		SBC_WR.icr = 0;
		return (-1);
	}
	SBC_WR.icr = 0;		/* Clear ack */
	return (data);
}


/*
 * Reset SCSI control logic.
 */
static int
sw_reset(h_sip, flag)
	register struct host_saioreq *h_sip;
	int flag;
{
	register struct scsi_sw_reg *swr;
	register char junk;
	EPRINTF("sw_reset:\n");

	swr = (struct scsi_sw_reg *) h_sip->devaddr;

	/* Reset bcr, fifo, udc, and sbc */
	swr->csr = 0;
	DELAY(10);
	swr->csr = SI_CSR_SCSI_RES | SI_CSR_FIFO_RES;
	SET_DMA_ADDR(swr, 0);
	SET_DMA_COUNT(swr, 0);

	/* Issue scsi bus reset */
	if (flag != SHORT_RESET) {
		SBC_WR.icr = SBC_ICR_RST;
		DELAY(100);
		SBC_WR.icr = 0;
		SBC_WR.tcr = 0;
		SBC_WR.mr = 0;
		junk = SBC_RD.clr;
#ifdef		lint
		junk = junk;
#endif		lint
		DELAY(SCSI_RESET_DELAY);	/* Recovery time */
	}
}
#endif	sun4
