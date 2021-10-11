#ifndef lint
static	char sccsid[] = "@(#)tm.c	1.1 92/07/30	Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Standalone driver for Ciprico TapeMaster Multibus tape controller
 */

#include <stand/saio.h>
#include <sundev/tmreg.h>
#include <mon/cpu.map.h>
#include <mon/cpu.addrs.h>

/*
 * Standard addresses for this board
 */
#define NTMADDR	2
unsigned long tmstd[] = { 0xA0, 0xA2 };

/*
 * Structure of our dma space
 */
#define MAXTMREC	(20*1024)	/* max size tape rec allowed */
struct tmdma {
	struct tm_mbinfo tm_mbinfo;	/* Multibus IOPB crap */
	char		tmbuf[MAXTMREC];/* Block of data */
};

#define	MB_DMA_ADDR_MASK	0x000FFFFF	/* Low meg */

/*
 * Driver definition
 */
struct devinfo tminfo = {
	sizeof(struct tmdevice),	/* Size of device registers */
	sizeof(struct tmdma),		/* DMA space required */
	0, 				/* Local variable space */
	NTMADDR,			/* # of standard addresses */
	tmstd,				/* Standard addr vector */
	MAP_MBIO,			/* Map type of device regs */
	MAXTMREC,			/* transfer size */
};

/*
 * What facilities we export to the world
 */
int	tmstrategy(), tmopen(), tmclose();
extern int ttboot();
extern int nullsys();

struct boottab mtdriver = {
	"mt", nullsys, ttboot, tmopen, tmclose, tmstrategy,
	"mt: TapeMaster 9-track tape", &tminfo,
};


/*
 * Open a tape drive
 */
tmopen(sip)
	register struct saioreq *sip;
{
	register skip;

	if (tminit(sip) == -1)
		return (-1);
	tmcmd(sip, TM_REWINDX);
	skip = sip->si_boff;
	while (skip--) {
		sip->si_cc = 0;
		tmcmd(sip, TM_SEARCH);
	}
	return (0);
}

/*ARGSUSED*/
tmclose(sip)
	struct saioreq *sip;
{

	/*tmcmd(sip, TM_REWINDX);*/
}

tmstrategy(sip, rw)
	register struct saioreq *sip;
	int rw;
{
	int func = (rw == WRITE) ? TM_WRITE : TM_READ;

	return tmcmd(sip, func);
}

#define NOTZERO	1	/* don't care value-but not zero (clr inst botch) */
#define ATTN(c) 	(((struct tmdevice *)c)->tmdev_attn=NOTZERO)
#define RESET(c) 	(((struct tmdevice *)c)->tmdev_reset=NOTZERO)
#define clrtpb(tp)	bzero((caddr_t)(tp), sizeof *(tp));

tmcmd(sip, func)
	register struct saioreq *sip;
{
#	define	tmdmap	((struct tmdma *)sip->si_dmaaddr)
#	define	tmb	(&tmdmap->tm_mbinfo)
	register struct tmdevice *tmaddr =
		(struct tmdevice *)sip->si_devaddr;
	register struct tpb *tpb = &tmb->tmb_tpb;
	int count = 1;
	int size = 0;
	struct tmstat tms;
	int err;

	switch (func) {

	case TM_READ:
		size = sip->si_cc;
		break;

	case TM_WRITE:
		size = sip->si_cc;
		swab((char *)sip->si_ma, tmdmap->tmbuf, size);
		break;
	}

	clrtpb(tpb);
	tpb->tm_cmd = func &~ TM_DIRBIT;
	tpb->tm_ctl.tmc_rev = func & TM_DIRBIT;
	tpb->tm_ctl.tmc_width = 1;
	tpb->tm_ctl.tmc_speed = sip->si_unit & 010;
	tpb->tm_ctl.tmc_tape = sip->si_unit & 03;
	tpb->tm_rcount = count;
	tpb->tm_bsize = size;
	c68t86((long)tmdmap->tmbuf, &tpb->tm_baddr);
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;

	/* Start the command and wait for completion */
	ATTN(tmaddr);
	while (tmb->tmb_ccb.tmccb_gate == TMG_CLOSED)
		;

	tms = tpb->tm_stat;
	err = tms.tms_error;

	/*
	 * An operation completed... record status
	 */
	if (err == E_EOT && tms.tms_load)
		err = E_NOERROR;
	/*
	 * Check for errors.
	 */
	if (err != E_NOERROR && err != E_SHORTREC) {
		if (err == E_EOF || err == E_EOT)
			return (0);
		/* Note: TapeMaster does retries for us */
		printf("tm hard err %x\n", err);
		return (-1);
	}
	if (func == TM_READ)
		swab(tmdmap->tmbuf, sip->si_ma, tpb->tm_count);
	return (tpb->tm_count);
#	undef	tmdma
#	undef	tmb
}

#define SPININIT 1000000
/*
 * Initialize a controller
 * Reset it, set up SCP, SCB, and CCB,
 * and give it an attention.
 * Make sure its there by waiting for the gate to open
 * Once initialization is done, issue CONFIG just to be safe.
 */
tminit(sip)
	register struct saioreq *sip;
{
	register struct tm_mbinfo *tmb = 
		&((struct tmdma *)(sip->si_dmaaddr))->tm_mbinfo;
	register struct tmdevice *tmaddr =
		(struct tmdevice *)sip->si_devaddr;
	register struct tpb *tpb = &tmb->tmb_tpb;

	bzero((caddr_t)tmb, sizeof (struct tm_mbinfo));
	RESET(tmaddr);
	
	/* setup System Configuration Block */
	tmb->tmb_scb.tmscb_03 = tmb->tmb_scb.tmscb_03x = TMSCB_CONS;
	c68t86((long)&tmb->tmb_ccb, &tmb->tmb_scb.tmccb_ptr);

	/* setup Channel Control Block */
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;

	{
		register struct tmscp *tmscp;
		char *temp;
		register int spin;
		int v1, v2;

		/* Snatch the dma space that this screwy Intel device needs */
		/* get the virtual address */
		tmscp = (struct tmscp *)(DVMA_BASE + TM_SCPADDR);
		v1 = getpgmap((char *)tmscp);
		v2 = getpgmap((char *)tmb);
		if (v1 != v2) {
			/* get random page */
			temp = resalloc(RES_MAINMEM, sizeof (struct tmscp));
			/* point virt at new page */
			setpgmap((char *)tmscp, getpgmap(temp));
		}
		/* setup System Configuration Pointer */
		tmscp->tmscb_bus = tmscp->tmscb_busx = TMSCB_BUS16;
		c68t86((long)&tmb->tmb_scb, &tmscp->tmscb_ptr);

		/* Start the TapeMaster and wait til it says "done" */
		ATTN(tmaddr); 
		for (spin = SPININIT; tmb->tmb_ccb.tmccb_gate != TMG_OPEN; )
			if (--spin <= 0)
				break;
		if (v1 != v2) {
			/*
			 * Give back the dma space this screwy Intel
			 * device needs
			 */
			setpgmap((char *)tmscp, v1);
		}
		if (spin <= 0) {
			printf("tm: no response from ctlr %x\n", sip->si_ctlr);
			return (-1);
		}
	}

	/* Finish CCB, point it at TPB */
	tmb->tmb_ccb.tmccb_ccw = TMC_NORMAL;
	c68t86((long)tpb, &tmb->tmb_ccb.tmtpb_ptr);

	/* Issue CONFIG command */
	clrtpb(tpb);
	tpb->tm_cmd = TM_CONFIG;
	tpb->tm_cmd2 = 0;
	tpb->tm_ctl.tmc_width = 1;

	/* Get the gate */
	while (tmb->tmb_ccb.tmccb_gate != TMG_OPEN)
		;
	tmb->tmb_ccb.tmccb_gate = TMG_CLOSED;

	/* Start the command and wait for completion */
	ATTN(tmaddr);
	while (tmb->tmb_ccb.tmccb_gate == TMG_CLOSED)
		;

	/* Check and report errors */
	if (tpb->tm_stat.tms_error) {
		printf("tm: error %d during config of ctlr %x\n", 
			tpb->tm_stat.tms_error, sip->si_ctlr);
		tpb->tm_stat.tms_error = 0;
		return (-1);
	}
	return (0);
}

/*
 * Convert a 68000 address into a 8086 address
 * This involves translating a virtual address into a
 * physical multibus address and converting the 20 bit result
 * into a two word base and offset.
 */
static c68t86(a68, a86)
	long a68;
	ptr86_t *a86;
{

	a68 &= MB_DMA_ADDR_MASK;
	a86->a_offset = a68 & 0xFFFF;
	a86->a_base = (a68 & 0xF0000) >> 4;
}
