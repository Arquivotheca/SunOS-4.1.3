/*
 * @(#)st.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc.
 *
 * Driver for SCSI tape drives.
 */
/*#define STDEBUG 		/* Allow compiling of debug code */
#define REL4			/* Enable release 4 mods */

#ifdef	REL4
#include <stand/saio.h>
#include <stand/param.h>
#include <sys/buf.h>
#include <sun/dklabel.h>
#include <sun/dkio.h>
#include <mon/cpu.addrs.h>
#include <mon/idprom.h>
#include <stand/scsi.h>
#include <stand/streg.h>
#else REL4
#include "saio.h"
#include "param.h"
#include "../h/buf.h"
#include "../sun/dklabel.h"
#include "../sun/dkio.h"
#include "../mon/idprom.h"
#include "../mon/cpu.addrs.h"
#include "scsi.h"
#include "streg.h"
#endif REL4


#ifdef STDEBUG
int st_debug = 1;			/* 0 = normal operation
					 * 1 = extended error info only
					 * 2 = debug and error info
					 * 3 = all status info
					 */
/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (st_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (st_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (st_debug > 1) printf(str,arg1,arg2)

/* Handy extended error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (st_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (st_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (st_debug) printf(str,arg1,arg2)

static char *st_key_error_str[] = SENSE_KEY_INFO;
#define MAX_KEY_ERROR_STR \
	(sizeof(st_key_error_str)/sizeof(st_key_error_str[0]))

#else STDEBUG
#define DPRINTF(str)
#define DPRINTF1(str, arg2)
#define DPRINTF2(str, arg1, arg2)
#define EPRINTF(str)
#define EPRINTF1(str, arg2)
#define EPRINTF2(str, arg1, arg2)
#endif STDEBUG

#ifdef BOOTBLOCK
#define SC_ERROR(str)
#define SC_ERROR1(str, arg1)
#else BOOTBLOCK
#define SC_ERROR	printf
#define SC_ERROR1	printf
#endif BOOTBLOCK

extern	int sc_reset;
extern	int xxprobe(), xxboot();
int stopen(), stclose(), ststrategy();

/*
 * This table defines the auto-probe disk list.  Currently only st0 and st1
 * are probed automatically.  Others can be added by simply adding them
 * to the following table.
 */
u_char st_index[] = { 0x20, 0x28, 0x18, 0x10 };

struct stparam {
	int		st_target;
	int		st_unit;
	int		st_ctype;
	int		st_dev_bsize;
	int		st_eof;
	int		st_lastcmd;
	struct host_saioreq h_sip;	/* sip for host adapter */
};

/*
 * DMA-able buffers
 */
/* Max tape record length allowed for various devices */
#define ST_MAXXTREC	(10*1024)	/* 1/2-inch reel */

/* Define virtual addresses space.  Note, sun-2 is limited. */
#ifdef	sun2
#define ST_MAXSIZE	(30 * 1024)
/*#define ST_MAXSIZE	(20 * 1024) */
#else	sun2
#define ST_MAXSIZE	(126 * 512)	
#endif	sun2


struct stdma {
	char	databuffer[ST_MAXSIZE];
	char	sbuffer[SC_SENSE_LENGTH];
};
#define BUF		(((struct stdma *)sip->si_dmaaddr)->databuffer)
#define STSBUF		(((struct stdma *)sip->si_dmaaddr)->sbuffer)

/*
 * What resources we need to run
 */
struct devinfo stinfo = {
	0,				/* No device to map in */
	sizeof (struct stdma),
	sizeof (struct stparam),
	0,				/* Dummy devices */
	0,				/* Dummy devices */
	MAP_MAINMEM,
	ST_MAXSIZE,			/* transfer size */
};

struct boottab stdriver = {
	"st",	xxprobe, xxboot, stopen, stclose, ststrategy,
	"st: SCSI tape", &stinfo
};

#define TAPE_TARGET	4	/* default SCSI target # for tape */
#define SENSELOC	4	/* sysgen returns sense at this offset */
#define ROUNDUP(x)	((x + DEV_BSIZE - 1) & ~(DEV_BSIZE - 1))
#define min(a,b)	((a)<(b)? (a): (b))
#define ISASYSGEN(stp)	(stp->st_ctype == ST_TYPE_SYSGEN ? 1 : 0)


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


/*
 * Open the SCSI Tape controller
 */
stopen(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;
	register struct host_saioreq *h_sip;	/* Host adapter block */
	register struct st_ms_mspl *ms;
	register short r;
	register int skip;
	int i, index, ctlr;
	int do_reset = 1;

	DPRINTF("stopen:\n");
	stp = (struct stparam *) sip->si_devdata;
	bzero( (char *)stp, (sizeof (struct stparam)));
	ctlr = sip->si_ctlr;		/* Ctlr. number */
	stp->st_ctype = ST_TYPE_SYSGEN;	/* Always start with Sysgen! */

	/* Set up host adapter and target info. */
ST_OPEN_PROBE:
	if (sip->si_unit == 0) {
		index = 0;
		stp->st_unit = st_index[index] & 0x07; /* Logical unit */
		stp->st_target = st_index[index++] >> 3; /* Target number */
	} else {
		stp->st_unit = sip->si_unit & 0x07;	/* Logical unit */
		stp->st_target = sip->si_unit >> 3;  	/* Target number */
	}

ST_RETRY_OPEN:
	EPRINTF2("stopen: ctlr= 0x%x  unit= 0x%x  ", ctlr, stp->st_unit);
	EPRINTF1("target= 0x%x\n", stp->st_target);

	h_sip = &stp->h_sip;
	h_sip->ctlr = ctlr;			/* Ctlr. number */
	h_sip->unit = stp->st_target;		/* Target number */

	/* Probe for host adapter. */ 
	if (scsi_probe(h_sip) == 0) {

		/* If an error occurred previously, reset the SCSI bus. */
		if (sc_reset  &&  do_reset) {
			EPRINTF("stopen: reset\n");
			(void) (*h_sip->reset)(h_sip, 1);
		}

		/*
		 * Test for the controller being ready. First test will fail
		 * if the SCSI bus is permanently busy or if a previous op
		 * was interrupted in mid-transfer. Second one should work.
		 */
		for (i = 0; i < 4; i++) {
			if ((r=stcmd(SC_TEST_UNIT_READY, sip, 0)) > 0)
				goto ST_OPEN;
			else if (r < -1)
				break;
			/*DELAY(2200);	/* XXX: Extra timing margin for HP */
		}
		/* If current tape device not ready, try the next one. */
		if (sip->si_unit == 0  &&  index < sizeof(st_index)) {
			stp->st_unit = st_index[index] & 0x07;
			stp->st_target = st_index[index++] >> 3;
			do_reset = 0;
			goto ST_RETRY_OPEN;
		}
	}

	/* If current host adapter not ready, try the next one. */
	if (sip->si_ctlr == 0  &&  ctlr < SC_NSC) {
		EPRINTF("stopen: trying next ha\n");
		do_reset++;
		ctlr++;
		h_sip->devaddr = 0;
		goto ST_OPEN_PROBE;
	}
	EPRINTF("stopen: device offline\n");
	return (-1);

	/*
	 * Check for tape drive (that is..it's not a disk).  Note, if
	 * inquiry fails, we will presume the user knows what he's doing.
	 */
ST_OPEN:
	sc_reset = 1;
	sip->si_ctlr = ctlr;
	sip->si_unit = (stp->st_target <<3) + stp->st_unit;
	sip->si_ma = BUF;
	if (stcmd(SC_INQUIRY, sip, 0) > 0  &&
	   ((struct scsi_inquiry_data *)sip->si_ma)->dtype != 1) {
		EPRINTF("stopen: not a tape\n");
		return (-1);
	}

	/*
	 * To figure out what type of tape controller is out there we send 
	 * a REQUEST_SENSE command and see how much sense data comes back.  
	 */
	stp->st_ctype = ST_TYPE_EMULEX;	
	sip->si_cc = ST_EMULEX_SENSE_LEN;
	/*sip->si_ma = BUF;*/
	r = stcmd(SC_REQUEST_SENSE, sip, 0);
	DPRINTF1("stopen: sense bytes= %x\n", r);

	if (r == ST_SYSGEN_SENSE_BYTES) {
		EPRINTF("stopen: Sysgen found\n");
		stp->st_ctype = ST_TYPE_SYSGEN;
		stp->st_dev_bsize = DEV_BSIZE;
		sip->si_cc = ST_SYSGEN_SENSE_LEN;
		if (stcmd(SC_REQUEST_SENSE, sip, 0) <= 0) {
			SC_ERROR("st: cannot get sense\n");
			return (-1);
		}
	} else {
		/* sip->si_ma = BUF; */
		ms = (struct st_ms_mspl *) sip->si_ma;
		r = stcmd(SC_MODE_SENSE, sip, 0);
		stp->st_dev_bsize = (ms->high_bl <<16) + (ms->mid_bl <<8) +
			(ms->low_bl);
		EPRINTF1("stopen: density= 0x%x\n", ms->density);
		EPRINTF1("stopen: block size= %d\n", stp->st_dev_bsize);

	}

	/* Check for 1/2" tape */
	(void) stcmd(SC_REWIND, sip, 0);
	if (stp->st_dev_bsize == 0) {
		if (stcmd(SC_QIC24, sip, 0) <= 0) {
			EPRINTF("stopen: Fixed CCS found\n");
			stp->st_dev_bsize = DEV_BSIZE;
			goto STOPEN_TAPE;
		}
		sip->si_cc = 0;
		/* sip->si_ma = BUF; */
		if (stcmd(SC_READ, sip, 0) <= 0) {
			EPRINTF("stopen: Emulex found\n");
			stp->st_dev_bsize = DEV_BSIZE;
			(void) stcmd(SC_REWIND, sip, 0);
			goto STOPEN_TAPE;

		} else {
			EPRINTF("stopen: Variable CCS found\n");
			stinfo.d_maxiobytes =  ST_MAXXTREC;
			(void) stcmd(SC_REWIND, sip, 0);
			goto STOPEN_SKIP;
		}
	}

	/* 
	 * Default format mode for emulex is Qic24 for Sun-3
	 * or later.  For Sun-2 use Qic-11.
	 */
STOPEN_TAPE:
#ifdef	sun2
	(void) stcmd(SC_QIC11, sip, 1);
#else	sun2
	(void) stcmd(SC_QIC24, sip, 1);
#endif	sun2

	/* 
	 * If read fails, try the other format.  For Sun-3 or later
	 * this means Qic-11.  For Sun-2, try Qic-24.
	 */
	sip->si_cc = 512;
	sip->si_ma = BUF;
	if (stcmd(SC_READ, sip, 0) > 0) {
		(void) stcmd(SC_REWIND, sip, 0);
	} else {
		(void) stcmd(SC_REWIND, sip, 0);
#ifdef		sun2
		if (stcmd(SC_QIC24, sip, 1) <= 0)
			SC_ERROR("st: mode select failed\n");
#else		sun2
		if (stcmd(SC_QIC11, sip, 1) <= 0)
			SC_ERROR("st: mode select failed\n");
#endif		sun2
	}

STOPEN_SKIP:
	skip = sip->si_boff;
	while (skip--) {
		sip->si_cc = 0;
		if (stcmd(SC_SPACE_FILE, sip, 1) <= 0) {
			SC_ERROR("st: space file failed\n");
			return (-1);
		}
	}
	sc_reset = 0;
	stp->st_eof = 0;
	stp->st_lastcmd = 0;
	return (0);
}


/*
 * Close the tape drive and don't rewind it.
 */
stclose(sip)
	register struct saioreq *sip;
{
	register struct stparam *stp;

	EPRINTF("stclose:\n");
	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_lastcmd == SC_WRITE)
		(void) stcmd(SC_WRITE_FILE_MARK, sip, 0);

	sc_reset = 0;
	return (0);
}


/*
 * Perform a read or write of the SCSI tape.
 */
ststrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	register struct stparam *stp;
	DPRINTF("ststrategy:\n");

	sc_reset = 1;
	stp = (struct stparam *) sip->si_devdata;
	if (stp->st_eof) {
		stp->st_eof = 0;
		return (0);
	}
	DPRINTF2("ststrategy:  addr= 0x%x  size= 0x%x\n",
		 (int)(sip->si_ma), sip->si_cc);
	rw = stcmd((rw == WRITE ? SC_WRITE : SC_READ), sip, 1);
	return (rw);
}


/*
 * Execute a scsi tape command
 */
static int
stcmd(cmd, sip, errprint)
	short cmd;
	register struct saioreq *sip;
	int errprint;
{
	register struct stparam *stp;
	register struct host_saioreq *h_sip;	/* Sip for host adapter */
	register struct st_ms_mspl *mode;
	register struct st_sense *ems;
	register struct sysgen_sense *scs;
	struct scsi_cdb6 cdb, scdb;
	struct scsi_scb scb, sscb;
	char *buf;
	int density;
	int r, cc;

	DPRINTF("stcmd:\n");

AGAIN:
	stp = (struct stparam *)sip->si_devdata;
	h_sip = &stp->h_sip;		/* host adapter */
	h_sip->unit = stp->st_target;
	buf =  sip->si_ma;

	/* Set up cdb */
	bzero((char *) &scb, sizeof scb);
	bzero((char *) &cdb, CDB_GROUP0);
	stp->st_lastcmd = cmd;
	cdb.cmd = cmd;
	cdb.lun = stp->st_unit;
	h_sip->cc = 0; 
	h_sip->ma = 0;
	DPRINTF2("stcmd: cmd= 0x%x  lun= 0x%x\n", cdb.cmd, cdb.lun);

	/* Some fields in the cdb are command specific */
	switch (cmd) {

	case SC_READ:
		h_sip->dma_dir = SC_RECV_DATA;
		if (stp->st_dev_bsize == 0) {
			DPRINTF1("stcmd: variable read (0x%x)\n", sip->si_cc);
			cc = h_sip->cc = sip->si_cc;
		} else {
			DPRINTF1("stcmd: read (0x%x)\n", sip->si_cc);
			cc = h_sip->cc = ROUNDUP(sip->si_cc);
			cc = cc / stp->st_dev_bsize;
			cdb.t_code = 1;
		}
STCMD_RD:
		cdb.mid_count = (cc >> 8) & 0xFF;
		cdb.low_count = cc & 0xFF;
		if (buf >= DVMA_BASE) {
			h_sip->ma = buf;
			DPRINTF1("stcmd: DVMA addr= 0x%x\n", h_sip->ma);
		} else {
			h_sip->ma = BUF;
			DPRINTF("stcmd: BUF addr\n");
		}
		if (cc == 0)
			h_sip->dma_dir = SC_NO_DATA;
		break;

	case SC_WRITE:
		h_sip->dma_dir = SC_SEND_DATA;
		if (buf < DVMA_BASE) {
			DPRINTF("stcmd: write bcopy\n");
			bcopy(buf, BUF, sip->si_cc);
		}
		if (stp->st_dev_bsize == 0) {
			DPRINTF1("stcmd: variable write (0x%x)\n", sip->si_cc);
			cc = h_sip->cc = sip->si_cc;
		} else {
			DPRINTF1("stcmd: write (0x%x)\n", sip->si_cc);
			cc = h_sip->cc = ROUNDUP(sip->si_cc);
			cc = cc / stp->st_dev_bsize;
			cdb.t_code = 1;
		}
		goto STCMD_RD;
		/* break; */

	case SC_MODE_SENSE:
		DPRINTF("stcmd:  mode sense\n");
		h_sip->dma_dir = SC_RECV_DATA;
		h_sip->cc = sip->si_cc = sizeof (struct st_ms_mspl);
		cdb.low_count = h_sip->cc & 0xFF;
		h_sip->ma = BUF;
		bzero((char *)h_sip->ma, sizeof (struct st_ms_mspl));
		break;

	case SC_QIC11:
		EPRINTF("stcmd: low density (qic-11)\n");
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		if (ISASYSGEN(stp)) {
			cdb.cmd = SC_QIC02;
			cdb.high_count = ST_SYSGEN_QIC11;
		} else {
			density = ST_EMULEX_QIC11;
			goto MODE;
		}
		break;

	case SC_QIC24:
		EPRINTF("stcmd: high density (qic-24)\n");
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		if (ISASYSGEN(stp)) {
			cdb.cmd = SC_QIC02;
			cdb.high_count = ST_SYSGEN_QIC24;
		} else {
			density = ST_EMULEX_QIC24;
			goto MODE;
		}
		break;

	case SC_MODE_SELECT:
MODE:
		cdb.cmd = SC_MODE_SELECT;
 		mode = (struct st_ms_mspl *)STSBUF;
		bzero((char *)mode, sizeof(*mode));
		mode->bufm = 1;
		mode->bd_len = MS_BD_LEN;
		mode->density = density;
		mode->mid_bl =  (stp->st_dev_bsize >> 8) & 0xff;
		mode->low_bl =  stp->st_dev_bsize & 0xff;

		h_sip->ma = (char *)mode;
		h_sip->dma_dir = SC_SEND_DATA;
		sip->si_cc = h_sip->cc = cdb.low_count =
			sizeof (struct st_ms_mspl);
		break;

	case SC_SPACE_FILE:
		EPRINTF("stcmd: space file\n");
		cdb.cmd = SC_SPACE_REC;	/* Real SCSI cmd */
		cdb.t_code = 1;		/* Space file, not rec */
		cdb.low_count = 1;	/* Space 1 file */
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		break;

	case SC_WRITE_FILE_MARK:
		DPRINTF("stcmd: write file mark\n");
		cdb.low_count = 1;	
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		break;

	case SC_REQUEST_SENSE:
		DPRINTF("stcmd: request sense\n");
		cdb.low_count = h_sip->cc = sip->si_cc;
		h_sip->ma = buf;
		h_sip->dma_dir = SC_RECV_DATA;
		/* bzero((char *)buf, sizeof (struct st_ms_mspl)); */
		break;

	case SC_INQUIRY:
		EPRINTF("stcmd: inquiry\n");
		h_sip->cc = sip->si_cc = sizeof (struct scsi_inquiry_data);
		cdb.low_count = h_sip->cc;
		h_sip->dma_dir = SC_RECV_DATA;
		h_sip->ma = buf;
		bzero((char *)buf, h_sip->cc);
		break;

	case SC_TEST_UNIT_READY:
#ifdef		STDEBUG
		DPRINTF("stcmd: test unit\n");
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		break;
#endif		STDEBUG

	case SC_REWIND:
		DPRINTF("stcmd: rewind\n");
		sip->si_cc = 0;
		h_sip->dma_dir = SC_NO_DATA;
		break;

	default:
		SC_ERROR1("st: unknown command (0x%x)\n", cdb.cmd);
		return (0);
	}

	/* Execute the command */
	r = (*h_sip->doit)(&cdb, &scb, h_sip);
	DPRINTF2("stcmd: r = 0x%x (0x%x)\n", r, sip->si_cc);

	/* Host adapter error */
	if (r < 0) {
		EPRINTF("stcmd: SCSI bus error\n");
		return (r);
	}

	/* Allow for tape blocking */
	if (r > sip->si_cc) {
		EPRINTF1("stcmd: round to 0x%x\n", sip->si_cc);
		r = sip->si_cc;
	}

	/* Device busy error */
	if(scb.busy) {
		EPRINTF("stcmd: busy\n");
		DELAY(2000000);
		goto AGAIN;	
	}

	if (cmd == SC_READ  &&  buf < DVMA_BASE) {
		DPRINTF("stcmd: read bcopy\n");
		if (min(sip->si_cc, r))
			bcopy(BUF, buf, (min(sip->si_cc, r)));
	}

	/* We may need to get sense data */
	if (scb.chk) {
		EPRINTF("stcmd: check condition\n");
		bzero((char *) &sscb, sizeof(sscb));
		bzero((char *) &scdb, CDB_GROUP0);
		scdb.cmd = SC_REQUEST_SENSE;
		scdb.lun = stp->st_unit;
		if (ISASYSGEN(stp)) {
			h_sip->cc = scdb.low_count = ST_SYSGEN_SENSE_LEN;
		} else {
			h_sip->cc = scdb.low_count = ST_EMULEX_SENSE_LEN;
		}
		h_sip->ma = STSBUF;
		h_sip->dma_dir = SC_RECV_DATA;
		(void) (*h_sip->doit)(&scdb, &sscb, h_sip);

		/* Process Sysgen tape drive errors */
		if (ISASYSGEN(stp)) {
			scs = (struct sysgen_sense *)STSBUF;
			stp->st_eof = 1;

			/* No tape */
			if (scs->no_cart) {
				EPRINTF("stcmd: not ready\n");
				return (-2);
			}

			/* QIC-24 */
			if (scs->no_data != 0) {
				EPRINTF("stcmd: blank check\n");
				return (-2);
			}

			/* EOF */
#ifdef			STDEBUG
			if (scs->file_mark)
				EPRINTF("stcmd: eof\n");
#endif			STDEBUG

			/* Other errors */
			if ((scs->file_mark == 0)  &&  errprint) {
				SC_ERROR1("st: error %x\n", 
					*(u_short *)&STSBUF[SENSELOC]);
			}
			return (r);

		/* Process MT-02 and CCS tape drive errors */
		} else  {
			ems = (struct st_sense *)STSBUF;

			/* Blank Check error */
			if (ems->key == SC_BLANK_CHECK) {
				EPRINTF("stcmd: blank check\n");
				stp->st_eof = 1;
				return (-2);

			/* Soft error */
			} else if (ems->key == SC_RECOVERABLE_ERROR) {
				EPRINTF("stcmd: soft error\n");
				return (r ? r : 1);

			/* EOF error */
			}else if (ems->fil_mk) {
				EPRINTF("stcmd: eof\n");
				stp->st_eof = 1;
				return (r);

			/* Illegal length error */
			} else if (ems->ili) {
				EPRINTF("stcmd: illegal length\n");
				return (r ? r : 1);

			/* Other errors */
			} else if(errprint) {
				SC_ERROR1("st: sense key = %x", ems->key);
#ifdef	 			STDEBUG
				SC_ERROR1(" (%s)", st_key_error_str[ems->key]);
#endif	 			STDEBUG
				SC_ERROR1("  error = %x\n", ems->error);

				stp->st_eof = 1;
				return (-1);
			}
		}
	}
	if (r >= sip->si_cc) {
		/* Normal completion exit */
		return (sip->si_cc ? sip->si_cc : 1);

	} else {
		/* Error exit */
		if (errprint) {
			EPRINTF1("stcmd: short transfer (0x%x)\n", r);
		}
		return (r);
	}
}
