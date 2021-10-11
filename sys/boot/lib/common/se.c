/*
 * @(#)se.c 1.1 92/07/30 Copyright (c) 1987 by Sun Microsystems, Inc.
 */
/*#define SEDEBUG 		/* Allow compiling of debug code */
#define REL4			/* Enable release 4 mods */

#ifdef	REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <stand/saio.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#include <stand/sereg.h>
#include <stand/scsi.h>
#else REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "saio.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#include "sereg.h"
#include "scsi.h"
#endif REL4


#ifdef SEDEBUG
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

#else SEDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SEDEBUG

#ifdef BOOTBLOCK
#define SC_ERROR(str)
#define SC_ERROR1(str, arg1)
#else BOOTBLOCK
#define SC_ERROR	printf
#define SC_ERROR1	printf
#endif BOOTBLOCK

/*
 * Low-level routines common to all devices on the SCSI bus.
 *
 * Interface to the routines in this module is via a second "h_sip"
 * structure contained in the caller's local variables.
 */

/* how si addresses look to the si vme scsi dma hardware */
#define SI_VME_DMA_ADDR(x)	(((int)x)&0x000FFFFF)

/* how si addresses look to the sun3/50 scsi dma hardware */
#define SI_OB_DMA_ADDR(x)	(((int)x)&0x00FFFFFF)

struct sidma {
	struct udc_table	udct;	/* dma information for udc */
};

/*
 * The interfaces we export
 */
extern int scsi_debug;
extern char *devalloc();
extern char *resalloc();
extern int nullsys();
extern u_char sc_cdb_size[];
int se_open(), se_doit(), se_reset();
u_char se_getbyte();
int se_putbyte();

static u_char junk;

#define SE_VME_BASE	0x300000
#define SE_BUFF_SIZE	0x00010000
#define SE_SIZE		0x4000

#define SHORT_RESET	0
#define LONG_RESET	1

/*
 * Open the SCSI host adapter.
 */
int
se_open(h_sip)
	register struct host_saioreq	*h_sip;
{
	register struct scsi_si_reg *ser;
	struct idprom id;
	register int ctlr;
	register int base;
	enum MAPTYPES space;
	DPRINTF("se_open:\n");

	/* determine type of si interface */
	/* check machine type to ensure it is a sun3-e */
	if ((idprom(IDFORM_1, &id) == IDFORM_1) &&
	    (id.id_machine == IDM_SUN3_E)) {
		DPRINTF("se_open: VME found\n");
		h_sip->ob = 0;
		base = SE_VME_BASE;
		space = MAP_VME24A16D;
	} else {
		EPRINTF("se_open: failed\n");
		return (-1);
	}

	/* Get base address of registers */
	if (h_sip->ctlr <= SC_NSC) {
		ctlr = base + ((int)h_sip->ctlr * SE_SIZE);
		DPRINTF1("se_open: ctlr= 0x%x\n", (int)ctlr);
	} else {
		if ((int)h_sip->devaddr == 0) {
			EPRINTF("se_open: devalloc failure\n");
			return (-2);
		} else {
			DPRINTF("se_open: reg. already allocated\n");
			return (0);
		}
	}

	/* Map in device registers */
	h_sip->devaddr = devalloc(space, (char *)ctlr, 
	    sizeof(struct scsi_si_reg));

	if ((int)h_sip->devaddr == 0) {
		EPRINTF("se_open: devalloc failure\n");
		return (-2);
	}
	ser = (struct scsi_si_reg *) (h_sip->devaddr);
	DPRINTF1("se_open: ser= 0x%x\n", (int)ser);

	/* Verify that 3E is there */
	if (peek((short *)&ser->csr) != -1) {
	    	ser->csr = 0;
    		if (peek((short *)&ser->csr) != 0x0402) {
			EPRINTF("se_open: peek failed\n");
    			return(-1);
    		}
	} else {
		EPRINTF("se_open: peek failed\n");
		return(-1);
	}

	/* Allocate dma resources */
	h_sip->dmaaddr = 0;

	/* Link top level driver to host adapter */
	h_sip->doit = se_doit;
	h_sip->reset = se_reset;

	se_reset(h_sip, SHORT_RESET);
	return(0);
}

/*
 * Write a command to the SCSI bus.
 *
 * The supplied sip is the one opened by se_open().
 * DMA is done based on sip->si_ma and sip->si_cc.
 *
 * Returns -1 for error, otherwise returns the residual count not DMAed
 * (zero for success).
 *
 * FIXME, this must be accessed via a boottab vector,
 * to allow host adap to switch.
 * Must pass cdb, scb in sip somewhere...
 */
static int
se_doit(cdb, scb, h_sip)
	struct scsi_cdb *cdb;
	struct scsi_scb *scb;
	register struct host_saioreq *h_sip;
{
	register struct scsi_si_reg *ser;
	register char *cp;
	u_char size;
	register int b;

	DPRINTF("se_doit:\n");
	/* Get to scsi control logic registers */
	ser = (struct scsi_si_reg *) h_sip->devaddr;
	DPRINTF1("se_doit: ser= 0x%x\n", (int)ser);

	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("se: bus busy\n");
		goto DOIT_FAILED;
	}

	/* select target */
	DPRINTF1("se_doit: unit= 0x%x\n", h_sip->unit);
	SBC_WR.odr = (1 << h_sip->unit) | SI_HOST_ID;
	SBC_WR.icr = SBC_ICR_DATA;
	SBC_WR.icr |= SBC_ICR_SEL;

	/* wait for target to acknowledge our selection */
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SCSI_SEL_DELAY, 1) == 0) {
		EPRINTF("se: device offline\n");
		SBC_WR.icr = 0;
		return(-2);
	}
	SBC_WR.icr = 0;

	/* Do initial dma setup */
	ser->dma_cntr = 0;		/* also reset dma_count for vme */
	if (h_sip->cc > 0) {
		if (h_sip->dma_dir == SC_RECV_DATA) {
			DPRINTF("se_doit: DMA receive\n");
			ser->csr &= ~SI_CSR_SEND;
		} else {
			DPRINTF("se_doit: DMA send\n");
			ser->csr |= SI_CSR_SEND;
			bcopy(h_sip->ma, (char *)&ser->dma_buf[0], h_sip->cc);
		}
	}

	/* put command onto scsi bus */
	cp = (char *)cdb;
	size = sc_cdb_size[CDB_GROUPID(*cp)];

	if (se_putbyte(ser, PHASE_COMMAND, cp, size) == 0) {
		SC_ERROR("se: put of cmd onto scsi bus failed\n");
		goto DOIT_FAILED;
	}

	if (h_sip->cc > 0) {

		/* Finish dma setup and wait for dma completion */
		if ((int)h_sip->ma & 1) {
			SC_ERROR("se: illegal odd dma starting address\n");
			goto DOIT_FAILED;
		}
		se_dma_setup(h_sip, ser);

	/* No dma, wait for target to request a byte */
	} else if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_LONG_DELAY, 1) == 0) {

		SC_ERROR("se: target never set REQ\n");
		goto DOIT_FAILED;
	}


	/* get status */
	cp = (char *)scb;

	/* change from 4.0 to get one single status byte only */
	cp[0] = se_getbyte(ser, PHASE_STATUS);
	DPRINTF1("se_doit: status= %x\n", cp[0]);	

	/* get one single message byte only */
	b = se_getbyte(ser, PHASE_MSG_IN);
	DPRINTF1("se_doit: msg= %x\n", b);	

	if (b != SC_COMMAND_COMPLETE) {
		EPRINTF("se_doit: no cmd complete msg\n");
		if (b >= 0) {
			/* if not, se_getbyte already printed msg */
			SC_ERROR("se: invalid message\n");
		}
		goto DOIT_FAILED;
	}
	return (h_sip->cc - ser->dma_cntr);

DOIT_FAILED:
	EPRINTF("se_doit: failed\n");
	se_reset(h_sip, LONG_RESET);
	return (-1);
}

static
se_dma_setup(h_sip, ser)
	register struct host_saioreq *h_sip;
	register struct scsi_si_reg *ser;
{ 
	DPRINTF("se_dma_setup:\n");

	ser->dma_addr = 0;
	ser->dma_cntr = h_sip->cc;

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

	/* wait for dma completion */
	if (se_wait(&ser->csr, SI_CSR_SBC_IP, 1) == 0) {
		SC_ERROR("se: dma never completed\n");
		se_dma_cleanup(ser);
		goto SETUP_FAILED;
	}

	/* check reason for dma completion */
	if (ser->csr & SI_CSR_SBC_IP) {
		/* dma operation should end with a phase mismatch */
		(void) se_sbc_wait((caddr_t)&SBC_RD.bsr, SBC_BSR_PMTCH,
		       SCSI_SHORT_DELAY, 0);
	} else {
		SC_ERROR("se: dma overrun\n");
		se_dma_cleanup(ser);
		goto SETUP_FAILED;
	}

	/* handle special dma recv situations */
	if (h_sip->dma_dir == SC_RECV_DATA) {
		bcopy((char *)&ser->dma_buf[0], h_sip->ma, h_sip->cc);
	}
	else {
		/* on writes counter off by one */
		ser->dma_cntr++;
	}

	/* cleanup after a dma operation */
	se_dma_cleanup(ser);

SETUP_FAILED:
	junk = SBC_RD.clr;
	return;
}      

/*
 * Reset some register information after a dma operation.
 */
static int
se_dma_cleanup(ser)
	register struct scsi_si_reg *ser;
{
	DPRINTF("se_dma_cleanup:\n");

	ser->dma_addr = 0;
	SBC_WR.mr &= ~SBC_MR_DMA;
	SBC_WR.icr = 0;
	SBC_WR.tcr = 0;
}


/*
 * Wait for a condition to be (de)asserted.
 */
static int
se_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;
	DPRINTF("se_wait:\n");

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
se_sbc_wait(reg, cond, delay, set)
	register caddr_t reg;
	register u_char cond;
	register int delay;
	register int set;
{
	register int i;
	register u_char regval;
	DPRINTF("se_sbc_wait:\n");

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
se_putbyte(ser, phase, data, num)
	register struct scsi_si_reg *ser;
	register u_short phase;
	register char *data;
	register u_char num;
{
	register int i;
	u_char icr;
	DPRINTF("se_putbyte:\n");

	/* set up tcr so phase match will occur */
	ser->sbc_wreg.tcr = phase >> 2;
	icr = SBC_WR.icr;

	/* Put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++) {

		SBC_WR.icr = icr | SBC_ICR_DATA; /* clear ack */

		/* Wait for target to request a byte */
		if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 1) == 0) {
			SC_ERROR1("se: REQ not active, cbsr 0x%x\n",
				SBC_RD.cbsr);
			return (0);
		}

		/* Load data for transfer */
		SBC_WR.odr = *data++;

		/* complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
			SCSI_SHORT_DELAY, 0) == 0) {
			SC_ERROR("se: target never released REQ\n");
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
static u_char
se_getbyte(ser, phase)
	register struct scsi_si_reg *ser;
	register u_short phase;
{
	register u_char data;
	DPRINTF("se_getbyte:\n");

	/* Wait for target request */
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 1) == 0) {
		SC_ERROR1("se: REQ not active, cbsr 0x%x\n",
			SBC_RD.cbsr);
		goto GETBYTE_FAILED;
	}

	/* Check for correct phase on scsi bus */
	if (phase != (SBC_RD.cbsr & CBSR_PHASE_BITS)) {
		SC_ERROR1("se: wrong phase, cbsr 0x%x\n",
			SBC_RD.cbsr);
		goto GETBYTE_FAILED;
	}

	/* Grab data */
	data = SBC_RD.cdr;
	SBC_WR.icr = SBC_ICR_ACK;

	/* Complete req/ack handshake */
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		SCSI_SHORT_DELAY, 0) == 0) {
		SC_ERROR("se: target never released REQ\n");
		goto GETBYTE_FAILED;
	}
	SBC_WR.tcr = 0;
	SBC_WR.icr = 0;		/* Clear ack */
	return (data);

GETBYTE_FAILED:
	SBC_WR.tcr = 0;
	SBC_WR.icr = 0;
	return (-1);
}


/*
 * Reset SCSI control logic.
 */
static int
se_reset(h_sip, flag)
	register struct host_saioreq *h_sip;
	int	flag;
{
	register struct scsi_si_reg *ser;
	
	ser = (struct scsi_si_reg *) h_sip->devaddr;
	EPRINTF("se_reset:\n");
	DPRINTF1("se_reset: flag= %x\n", flag);
	
	/* added as in "si.c" to reset "sbc" */
        ser->csr = 0;			/* negative logic for assertion */
        DELAY(10);
        ser->csr = SI_CSR_SCSI_RES;	/* de-activate */

	if (flag != SHORT_RESET) {	/* issue scsi bus reset */
		SBC_WR.icr = SBC_ICR_RST;
		DELAY(100);
		SBC_WR.icr = 0;
		junk = SBC_RD.clr;

		/* Give reset scsi devices time to recover (> 2 Sec) */
		DELAY(SCSI_RESET_DELAY);
	}
}
