/*
 * @(#)sd.c 1.1 92/07/30 Copyright (c) 1985 by Sun Microsystems, Inc.
 *
 * Driver for SCSI disk drives.
 */

/*#define SDDEBUG 		/* Allow compiling of debug code */
#define REL4			/* Enable release 4 mods */

#ifdef REL4
#include <stand/saio.h>
#include <stand/param.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <sys/buf.h>
#include <mon/sunromvec.h>
#include <mon/idprom.h>
#include <mon/cpu.addrs.h>
#include <stand/scsi.h>
#else REL4
#include "saio.h"
#include "param.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../h/buf.h"
#include "../mon/sunromvec.h"
#include "../mon/idprom.h"
#include "../mon/cpu.addrs.h"
#include "scsi.h"
#endif REL4


extern int sc_reset;
extern int xxprobe(), xxboot(), nullsys();
int sdopen(), sdclose(), sdstrategy();

#ifdef SDDEBUG
int sd_debug = 1;			/*
					 * 0 = normal operation
					 * 1 = extended error info only
					 * 2 = debug and error info
					 * 3 = all status info
					 */
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (sd_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (sd_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (sd_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (sd_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (sd_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (sd_debug) printf(str,arg1,arg2)

#else SDDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif SDDEBUG


#ifdef BOOTBLOCK
#define SD_MAXSIZE	MAXBSIZE
#define SC_ERROR(str)
#define SC_ERROR1(str, arg1)
#else BOOTBLOCK
#define SD_MAXSIZE	(4 * MAXBSIZE)
#define SC_ERROR	printf
#define SC_ERROR1	printf
#endif BOOTBLOCK

/*
 * This table defines the auto-probe disk list.  Currently only st0 and st1
 * are probed automatically.  Others can be added by simply adding them
 * to the following table.
 */
u_char sd_index[] = { 0x00, 0x18 };

struct sdparam {
	int	sd_target;
	int	sd_unit;
	unsigned	sd_boff;		/* real boff... */
	struct host_saioreq h_sip;	/* sip for host adapter */
};

/*
 * Our DMA space
 */
struct sddma {
	char	buffer[SD_MAXSIZE];
	char	sbuffer[SC_SENSE_LENGTH];
};
#define BUF		(((struct sddma *)sip->si_dmaaddr)->buffer)
#define STSBUF		(((struct sddma *)sip->si_dmaaddr)->sbuffer)
#define ROUNDUP(x)	((x + SECSIZE - 1) & ~(SECSIZE - 1))
#define MSGS_OFF	1
#define MSGS_ON		0

/*
 * What resources we need to run
 */
struct devinfo sdinfo = {
	0,				/* No device to map in */
	sizeof (struct sddma),
	sizeof (struct sdparam),
	0,				/* Dummy devices */
	0,				/* Dummy devices */
	MAP_MAINMEM,
	SD_MAXSIZE,			/* transfer size */
};

/*
 * The interfaces we export
 */
struct boottab sddriver = {
	"sd",	xxprobe, xxboot, sdopen, sdclose, sdstrategy,
	"sd: SCSI disk", &sdinfo
};


#ifdef	SDDEBUG
sd_print_buffer(y, count)
	register u_char *y;
	register int count;
{
	register int x;

	for (x = 0; x < count; x++)
		printf("%x  ", *y++);
	printf("\n");
}
#endif	SDDEBUG


#ifndef	BOOTBLOCK
/*
 * Test routine for isspinning() to see if SCSI disk is running.
 */
/*ARGSUSED*/
int
sdspin(sip, dummy)
	struct saioreq *sip;
	int dummy;
{
	DPRINTF("sdspin:\n");
	return (sdcmd(SC_TEST_UNIT_READY, sip, MSGS_OFF));
}
#endif	BOOTBLOCK


/*
 * Open the SCSI Disk controller
 */
sdopen(sip)
	register struct saioreq *sip;
{
	register struct sdparam *sdp;
	register struct host_saioreq *h_sip;	/* Sip for host adapter */
	register struct dk_label *label;
	int index, ctlr;
	int do_reset = 1;
	DPRINTF("sdopen:\n");

	sdp = (struct sdparam *)sip->si_devdata;
	label = (struct dk_label *)BUF;
	bzero( (char *)sdp, (sizeof (struct sdparam)));
	ctlr = sip->si_ctlr;		/* Ctlr. number */

	/* Set up host adapter and target info. */
SD_OPEN_PROBE:
	if (sip->si_unit == 0) {
		index = 0;
		sdp->sd_unit = sd_index[index] & 0x07; /* Logical unit */
		sdp->sd_target = sd_index[index++] >> 3; /* Target number */
	} else {
		sdp->sd_unit = sip->si_unit & 0x07;	/* Logical unit */
		sdp->sd_target = sip->si_unit >> 3;  	/* Target number */
	}


SD_RETRY_OPEN:
	DPRINTF2("sdopen: ctlr= 0x%x  unit= 0x%x", ctlr, sdp->sd_unit);
	DPRINTF1("  target= 0x%x\n", sdp->sd_target);

	h_sip = &sdp->h_sip;
	h_sip->ctlr = ctlr;		/* Ctlr. number */
	h_sip->unit = sdp->sd_target;		/* Host adapter LU number */

	/* Probe for host adapter */
	if (scsi_probe(h_sip) == 0) {

		/* If an error occurred previously, reset the SCSI bus. */
		if (sc_reset  &&  do_reset) {
			EPRINTF("sdopen: reset\n");
			(void) (*h_sip->reset)(h_sip, 1);
		}

		/*
		 * Test for the controller being ready. First test will fail
		 * if the SCSI bus is permanently busy or if a previous op
		 * was interrupted in mid-transfer. Second one should work.
		 */
		if (sdcmd(SC_TEST_UNIT_READY, sip, MSGS_OFF) > 0)
				goto SD_OPEN;

#ifndef	BOOTBLOCK
		/* If current disk device not ready, try the next one. */
		if (sip->si_unit == 0  &&  index < sizeof(sd_index)) {
			sdp->sd_unit = sd_index[index] & 0x07;
			sdp->sd_target = sd_index[index++] >> 3;
			do_reset = 0;
			goto SD_RETRY_OPEN;
		}

		/* If current host adapter not ready, try the next one. */
		if (sip->si_ctlr == 0  &&  ctlr < SC_NSC) {
			EPRINTF("sdopen: trying next ha\n");
			do_reset++;
			ctlr++;
			h_sip->devaddr = 0;
			goto SD_OPEN_PROBE;
		}
#endif	BOOTBLOCK
	}
	EPRINTF("sdopen: device offline\n");
	return (-1);

SD_OPEN:
	sc_reset = 1;
	sip->si_unit = (sdp->sd_target <<3) + sdp->sd_unit;
	(void) sdcmd(SC_START, sip, MSGS_OFF);	/* Spin-up disk */

#ifndef	BOOTBLOCK
	switch (isspinning(sdspin, (char *)sip, 0)) {

	default:				/* Error from sdspin */
	case 0:					/* Disk still not ready */
		EPRINTF("sdopen: device not ready\n");
		return (-1);
	case 1:					/* Spinning */
		DPRINTF("sdopen: device ready\n");
		break;
	case 2:  				/* Just started spinup */
		EPRINTF("sdopen: spinning up\n");
		DELAY(2000000);  		/* Wait 2 Sec. */
		break;
	}
#endif	BOOTBLOCK

	/*
	 * Check if it's a disk.  Note, if inquiry fails, we presume
	 * the user knows what he's doing.
	 */
	sip->si_ma = BUF;
	if (sdcmd(SC_INQUIRY, sip, MSGS_OFF) > 0  &&
	   ((struct scsi_inquiry_data *)sip->si_ma)->dtype == 1) {
		EPRINTF("sdopen: not a disk\n");
		return (-1);
	}
	/*
	 * Read the label
	 */
	label->dkl_magic = 0;
	sip->si_bn = 0;				/* Read block #0 */
	sip->si_cc = SECSIZE;
	/*sip->si_ma = BUF;*/
	if (sdcmd(SC_READ, sip, MSGS_ON) <= 0) {
		EPRINTF("sdopen: label read failed\n");
		return (-1);
	}

	EPRINTF1("sd: <%s>\n", (char *)sip->si_ma);
	if (chklabel(label)) {
		EPRINTF("sdopen: no label\n");
		return (-1);
	}

	/*
	 * Compute starting (abs.) starting block number and partition
	 * size (in bytes).  Note:
	 *	si_cyloff = starting block number of partition
	 *	si_boff = size of partition in bytes
	 * FIXME:  sd_boff really should be in sip structure.  Note
	 * being there mans that ONLY one connection to sd can be 
	 * used at a time (which is all we really do at the present).
	 */
	sip->si_cyloff = (u_short)(label->dkl_map[sip->si_boff].dkl_cylno)
	    * (u_short)(label->dkl_nhead * label->dkl_nsect);
	sdp->sd_boff =
		(u_short) label->dkl_map[sip->si_boff].dkl_nblk * SECSIZE;
	if (sdp->sd_boff == 0) {
		EPRINTF("sdopen: zero size partition\n");
		return (-1);
	}
	sc_reset = 0;
	return (0);
}


/*
 * Close the SCSI Disk controller.
 */
sdclose(sip)
	register struct saioreq *sip;
{
	EPRINTF("sdclose:\n");
#ifdef	lint
	sip = sip;
#endif	lint

	sc_reset = 0;

}


/*
 * Execute reads or writes for the outside world.
 */
sdstrategy(sip, rw)
	struct saioreq *sip;
	register int rw;
{
	register struct sdparam *sdp;
	DPRINTF2("sdstrategy:  addr= 0x%x  size= 0x%x\n",
		 (int)(sip->si_ma), sip->si_cc);

	sc_reset = 1;
	sdp = (struct sdparam *)sip->si_devdata;
	if (sdp) {
		if (sip->si_cc > sdp->sd_boff) {
			EPRINTF("sdstrategy: request too big\n");
			sip->si_cc = sdp->sd_boff;
		}
	}
	if (sip->si_cc <= 0) {
		EPRINTF("sdstrategy: request too small\n");
		return(0);
	}
	rw = sdcmd((rw == WRITE ? SC_WRITE : SC_READ), sip, MSGS_ON);
	return (rw < 0 ? 0 : rw);
}

/*
 * Internal interface to the disk command set
 *
 * Returns the character count read (or 1 if count==0) for success,
 * returns 0 for failure, or -1 for severe unretryable failure.
 */
static int
sdcmd(cmd, sip, errprint)
	short cmd;
	register struct saioreq *sip;
	int errprint;
{
	register struct sdparam *sdp;
	register struct host_saioreq *h_sip;	/* Sip for host adapter */
	register char *buf;
	struct scsi_cdb cdb, scdb;
	struct scsi_scb scb, sscb;
	int retry = 0;
	int trycount = 16;
	int r, i, cc, dir;
	int sense_key;

	DPRINTF("sdcmd:\n");
	sdp = (struct sdparam *)sip->si_devdata;
	h_sip = &sdp->h_sip;			/* Host adapter block */
	h_sip->unit = sdp->sd_target;
	cc = ROUNDUP(sip->si_cc);
	buf = BUF;

	/* Set up cdb */
	bzero((char *) &cdb, CDB_GROUP1);
	bzero((char *) &scb, sizeof scb);
	cdb.cmd = cmd;
	cdb.lun = sdp->sd_unit;
	sip->si_bn += sip->si_cyloff;

	/* Some fields in the cdb are command specific */
	switch (cmd) {

	case SC_READ:
		DPRINTF("sdcmd: read\n");
		h_sip->dma_dir = SC_RECV_DATA;
		goto SDCMD_RD;

	case SC_WRITE:
		DPRINTF("sdcmd: write\n");
		h_sip->dma_dir = SC_SEND_DATA;
		if (sip->si_ma < DVMA_BASE) {
			DPRINTF("sdcmd: write bcopy\n");
			bcopy(sip->si_ma, BUF, sip->si_cc);
		}

SDCMD_RD:
		if (sip->si_ma >= DVMA_BASE) {
			DPRINTF("sdcmd: DVMA write\n");
			buf = sip->si_ma;
		}
		FORMG0ADDR(&cdb, sip->si_bn);
		FORMG0COUNT(&cdb, (cc / SECSIZE));
		break;

	case SC_INQUIRY:
		EPRINTF("sdcmd: inquiry\n");
		h_sip->dma_dir = SC_RECV_DATA;
		sip->si_cc = cc = sizeof(struct scsi_inquiry_data);
		FORMG0COUNT(&cdb, cc);
		bzero((char *)h_sip->ma, cc);
		break;

	case SC_TEST_UNIT_READY:
		DPRINTF("sdcmd: test unit ready\n");
		trycount = 4;
		h_sip->dma_dir = SC_NO_DATA;
		cc = 0;
		break;

	case SC_START:
		DPRINTF("sdcmd: start\n");
		trycount = 2;
		h_sip->dma_dir = SC_NO_DATA;
		FORMG0COUNT(&cdb, 1);
		break;

	default:
		if (!errprint)
			SC_ERROR1("sd: unknown command (0x%x)\n", cdb.cmd);
		return (0);
	}

	/* Run command and process errors. */
	while (retry++ < trycount) {
		h_sip->cc = cc;
		h_sip->ma = buf;
		r = (*h_sip->doit)(&cdb, &scb, h_sip);
		DPRINTF2("sdcmd: r= 0x%x (0x%x)\n", cc, r);

		/* Major SCSI bus error */
		if (r < 0) {
			EPRINTF("sdcmd: SCSI bus error\n");
			return (r);
		}

		/* Disk busy - delay 2 Sec. */
		if (scb.busy) {
			EPRINTF("sdcmd: busy\n");
			DELAY(2000000);
			continue;
		}

		/* Error, get sense data. */
		if (scb.chk) {
			EPRINTF("sdcmd: check condition\n");
			bzero((char *) &scdb, CDB_GROUP0);
			bzero((char *) &sscb, sizeof scb);

			scdb.cmd = SC_REQUEST_SENSE;
			scdb.lun = sdp->sd_unit;
			h_sip->cc = sizeof(struct scsi_sense);
			FORMG0COUNT(&scdb, h_sip->cc);
			h_sip->ma = STSBUF;
			dir = h_sip->dma_dir;
			h_sip->dma_dir = SC_RECV_DATA;

			i = (*h_sip->doit)(&scdb, &sscb, h_sip);
			h_sip->dma_dir = dir;

			sense_key = sd_pr_sense((u_char *)h_sip->ma, i, errprint);
			if (sense_key == SC_RECOVERABLE_ERROR) {
				goto SDCMD_EXIT;
			}
			continue;
		}

		/* Return count should match */
		if (r < cc) {
			EPRINTF("sd: short transfer");
			if (!errprint)
				SC_ERROR1("sd: short transfer (0x%x)\n", r);
			continue;
		}

		/* Copy from buffer */
SDCMD_EXIT:
		if (cmd == SC_READ  &&  sip->si_ma < DVMA_BASE) {
			DPRINTF("sdcmd: read bcopy\n");
			bcopy(BUF, sip->si_ma, sip->si_cc);
		}
		return (r ? r : 1);	/* Returns count */
	}

	if (!errprint)
		SC_ERROR1("sd: %d retries\n", trycount); /* Unlikely */
	return (0);
}


#ifndef BOOTBLOCK
static u_char sd_adaptec_keys[] = {
	0, 4, 4, 4,  2, 4, 4, 4,	/* 0x00-0x07 */
	4, 4, 4, 4,  4, 4, 4, 4,	/* 0x08-0x0f */
	3, 3, 3, 3,  3, 3, 3, 1,	/* 0x10-0x17 */
	1, 1, 5, 5,  1, 1, 1, 1,	/* 0x18-0x1f */
	5, 5, 5, 5,  5, 5, 5, 5,	/* 0x20-0x27 */
	6, 6, 6, 6,  6, 6, 6, 6,	/* 0x28-0x30 */
};
#define MAX_ADAPTEC_KEYS \
	(sizeof(sd_adaptec_keys))
#endif		BOOTBLOCK


#ifdef	SDDEBUG
static char *sd_key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(sd_key_error_str)/sizeof(sd_key_error_str[0]))
#endif	SDDEBUG


/*
 * Print out sense info.
 */
static int
sd_pr_sense(cp, len, errprint)
	u_char *cp;
	int len;
	int errprint;
{
	register struct scsi_sense *ss;
	register struct scsi_ext_sense *ses;
	int sense_key;

	DPRINTF("sd_pr_sense:\n");
	ses = (struct scsi_ext_sense *) cp;
	ss = (struct scsi_sense *) cp;
#ifdef	lint
	len = len;
#endif	lint

#ifdef SDDEBUG
	if (sd_debug > 2) {
		printf("sd: error:");
		while (--len >= 0) {
			printf(" %x", *cp++);
		}
		printf("\n");
	}
#endif	SDDEBUG

	if (ss->code <= 6) {
		/*
		 * Synthesize sense key for extended sense. Note,
		 * Without the decode table, we don't know what's
		 * so we'll say we don't know.
		 */
#ifdef		BOOTBLOCK
		sense_key = SC_NO_SENSE;
#else		BOOTBLOCK
		if (ss->code < MAX_ADAPTEC_KEYS) {
			sense_key = sd_adaptec_keys[ss->code];
		}
#endif		BOOTBLOCK

		if (errprint)
			return (sense_key);

		SC_ERROR1("sd: sense key = %x", sense_key);
#ifdef	 	SDDEBUG
		SC_ERROR1(" (%s)", sd_key_error_str[ses->key]);
#endif	 	SDDEBUG
		SC_ERROR1("  error = %x  ", ss->code);

		if (ss->adr_val) {
			SC_ERROR1("- block %d", (ss->high_addr << 16) |
			    (ss->mid_addr << 8) | ss->low_addr);
		}


	/* Sense class 7: the standardized one */
	} else {
		/*
		 * For CCS disks, it tells us the problem.
		 */
		sense_key = ses->key;

		if (errprint)
			return (sense_key);

		SC_ERROR1("sd: sense key = %x", ses->key);
#ifdef	 	SDDEBUG
		SC_ERROR1(" (%s)", sd_key_error_str[ses->key]);
#endif		SDDEBUG
		SC_ERROR1("  error = %x  ", ses->error_code);

		if (ses->adr_val) {
			SC_ERROR1("- block %d", (ses->info_1 << 24) |
			    (ses->info_2 << 16) | (ses->info_3 << 8) |
			    ses->info_4);
		}
	}
	SC_ERROR("\n");
	return (sense_key);
}
