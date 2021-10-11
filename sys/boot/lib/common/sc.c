/*
 * @(#)sc.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 */
/*#define SCDEBUG  			/* Allow compiling of debug code */
#define REL4				/* Enable release 4.00 mods */

#ifdef	REL4
#include <sys/types.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#include <stand/saio.h>
#include <stand/screg.h>
#include <stand/scsi.h>

#else REL4
#include "../h/types.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#include "saio.h"
#include "screg.h"
#include "scsi.h"
#endif REL4

#ifdef SCDEBUG
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

#else SCDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SCDEBUG

#ifdef BOOTBLOCK
#define SC_ERROR(str)
#define SC_ERROR1(str, arg1)
#else BOOTBLOCK
#define SC_ERROR	printf
#define SC_ERROR1	printf
#endif BOOTBLOCK

/*
 * Low-level routines common to all devices on the SCSI bus
 *
 * Interface to the routines in this module is via a second "h_sip"
 * structure contained in the caller's local variables.
 */

/* How our addrs look to the SCSI DMA hardware */
#define SC_DMA_ADDR(x)	(((int)x)&0x000FFFFF)

#define SC_MULTI_BASE	0x80000
#define SC_VME_BASE	0x200000
#define SC_SIZE		0x4000		/* incr to next board - OK for VME ? */

/*
 * The interfaces we export
 */
extern int scsi_debug;
extern int nullsys();
extern char * devalloc();
extern u_char sc_cdb_size[];
int scopen(), scdoit(), sc_reset();

/*
 * Open the SCSI host adapter.
 * Note that the Multibus address and the VME address
 * are totally different, although both are controller '0'.
 */
int
scopen(h_sip)
	register struct host_saioreq *h_sip;
{
	register struct scsi_ha_reg *har;
	register int ctlr;
	register int base;
	struct idprom id;
	enum MAPTYPES space;
	DPRINTF("scopen:\n");

	/*
	 * Check if SCSI-2 host adapter board.  Note,
	 * this should be the last one tested.
	 */
	if (idprom(IDFORM_1, &id) == IDFORM_1  &&
	    id.id_machine == IDM_SUN2_MULTI) {
		EPRINTF("scopen: Multibus SCSI-2\n");
		base = SC_MULTI_BASE;
		space = MAP_MBMEM;
	} else {
		EPRINTF("scopen: VME SCSI-2\n");
		base = SC_VME_BASE;
		space = MAP_VME24A16D;
	}

	/* Get base address of registers */
	if (h_sip->ctlr <= SC_NSC) {
		ctlr = base + ((int)h_sip->ctlr * SC_SIZE);
		DPRINTF1("scopen: ctlr= 0x%x\n", (int)ctlr);
	} else {
		if ((int)h_sip->devaddr == 0) {
			EPRINTF("scopen: devalloc failure\n");
			return (-2);
		} else {
			DPRINTF("scopen: reg. already allocated\n");
			return (0);
		}
	}

	/* Now map it in */
	h_sip->devaddr = devalloc(space, (char *)ctlr,
	    sizeof (struct scsi_ha_reg));
	if ((int)h_sip->devaddr == 0) {
		EPRINTF("scopen: devaddr failed\n");
		return (-2);
	}

	/* Check if host adapter present. */
	har = (struct scsi_ha_reg *) h_sip->devaddr;
	if (peek((short *)&har->dma_count) == -1) {
		EPRINTF("scopen: peek failed\n");
		return (-1);
	}
	/* Link top level driver to host adapter */
	h_sip->doit = scdoit;
	h_sip->reset = sc_reset;

	/* Initialize host adapter */
	har->icr = 0;
	return (0);
}


/*
 * Write a command to the SCSI bus.
 *
 * The supplied sip is the one opened by scopen().
 * DMA is done based on sip->si_ma and sip->si_cc.
 *
 * Returns -1 for error, otherwise returns the residual count not DMAed
 * (zero for success).
 */
static int
scdoit(cdb, scb, h_sip)
	register struct scsi_cdb *cdb;
	register struct scsi_scb *scb;
	register struct host_saioreq *h_sip;
{
	register struct scsi_ha_reg *har;
	register u_char *cp;
	register u_char size;
	register int i, b;

	DPRINTF("scdoit:\n");
	har = (struct scsi_ha_reg *) h_sip->devaddr;

	/* Wait for bus */
	if (sc_wait(&har->icr, ICR_BUSY, SCSI_SHORT_DELAY, 0) < 0) {
		SC_ERROR("sc: bus busy\n");
		goto FAILED_RESET;
	}

	/* Select controller */
	har->icr = 0;
	har->data = (1 << h_sip->unit) | HOST_ADDR;
	har->icr = ICR_SELECT;

	/* Wait for target reply */
	if (sc_wait(&har->icr, ICR_BUSY, SCSI_SEL_DELAY, 1) < 0) {
		EPRINTF("sc: select failed\n");
		har->icr = 0;
		return (-2);
	}

	/* Pass command */
	SET_DMA_ADDR(har, SC_DMA_ADDR(h_sip->ma));
	har->dma_count = ~h_sip->cc;
	har->icr = ICR_WORD_MODE | ICR_DMA_ENABLE;

	cp = (u_char *) cdb;
	size = sc_cdb_size[CDB_GROUPID(*cp)];
	for (i = 0; i < size; i++) {
		if (sc_putbyte(har, ICR_COMMAND, *cp++) == 0) {
			EPRINTF("scdoit: cmd put failed\n");
			return (-1);
		}
	}

	/* Wait for command completion */
	if (sc_wait(&har->icr, ICR_INTERRUPT_REQUEST, SCSI_LONG_DELAY, 1) < 0) {
		EPRINTF("scdoit: timeout\n");
		return (-1);
	}

	/* Handle odd length dma transfer, adjust dma count */
	if (har->icr & ICR_ODD_LENGTH) {
		if (h_sip->dma_dir == SC_RECV_DATA) {
		        cp = (u_char *)h_sip->ma + h_sip->cc - 1;
		        *cp = (u_char)har->data;
			har->dma_count = ~(~har->dma_count - 1);
		} else {
			har->dma_count = ~(~har->dma_count + 1);
		}
	}

	/* Get status */
	cp = (u_char *) scb;
	for (i = 0;;) {
		b = sc_getbyte(har, ICR_STATUS);
		if (b == -1)
			break;
		if (i < STATUS_LEN) 
			cp[i++] = b;
	}

	/* Check status message */
	b = sc_getbyte(har, ICR_MESSAGE_IN);
	if (b != SC_COMMAND_COMPLETE) {
		SC_ERROR("sc: invalid status msg\n");
		return (-1);
	}
	return (h_sip->cc - (u_short)~har->dma_count);

FAILED_RESET:
	sc_reset(har, 1);
	return (-1);
}


/*
 * Wait for a condition on the scsi bus.
 */
static int
sc_wait(reg, cond, delay, set)
	register u_short *reg, cond;
	register int delay;
	register int set;
{
	int i;
	register u_short regval;

	DPRINTF("sc_wait:\n");

	for (i = 0; i < delay; i++) {
		regval = *reg;
						/* Assert */
		if ((set == 1) && (regval & cond))
			return (1);
						/* Deassert */
		if ((set == 0) && !(regval & cond))
			return (1);

		if (regval & ICR_BUS_ERROR)	/* Check for bus error */
			return (-2);

		DELAY(10);
	}
	return (-1);				/* Timeout */
}


/*
 * Put a byte into the scsi command register.
 */
static int
sc_putbyte(har, bits, data)
	register struct scsi_ha_reg *har;
	register u_short bits;
	register u_char data;
{						/* Wait for target request */
	DPRINTF("sc_putbyte:\n");

	if (sc_wait(&har->icr, ICR_REQUEST, SCSI_SHORT_DELAY, 1) < 0)
		return (0);
						/* Check I/F control reg */
						/* Is it ready to command? */
	if ((har->icr & ICR_BITS) != bits) {
		SC_ERROR("sc: sequence error\n");
		return (0);
	}
						/* Set cmd/status reg */
	har->icr |= ICR_PARITY_ENABLE;		/* Enable parity on scsi bus */
	har->cmd_stat = data;
	return (1);
}


/*
 * Get a byte from the scsi command/status register.
 * <bits> defines the bits we want to see on in the ICR.
 * If <bits> is wrong, we print a message -- unless <bits> is ICR_STATUS.
 * This hack is because scdoit keeps calling getbyte until it sees a non-
 * status byte; this is not an error in sequence as long as the next byte
 * has MESSAGE_IN tags.  FIXME.
 */
static int
sc_getbyte(har, bits)
	register struct scsi_ha_reg *har;
{
	DPRINTF("sc_getbyte:\n");

	/* Wait for target request */
	if (sc_wait(&har->icr, ICR_REQUEST, SCSI_LONG_DELAY, 1) < 0)
		return (-1);
						/* Check I/F control reg */
						/* Is it ready to command? */
	if ((har->icr & ICR_BITS) != bits) {
		if (bits != ICR_STATUS)
			SC_ERROR("sc: sequence error\n");
		return (-1);
	}
	return (har->cmd_stat);			/* Return byte */
}

/*
 * Reset SCSI control logic.
 */
static
sc_reset(har, flag)
	register struct scsi_ha_reg *har;
	int flag;
{
	EPRINTF("sc_reset:\n");
#ifdef	lint
	flag = flag;
#endif	lint

	har->icr = ICR_RESET;
	DELAY(100);
	har->icr = 0;

	/* Give reset scsi devices time to recover */
	DELAY(SCSI_RESET_DELAY);
}
