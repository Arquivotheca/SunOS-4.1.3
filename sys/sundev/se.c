/* @(#)se.c 1.1 92/07/30 Copyright (c) 1988 by Sun Microsystems, Inc. */
#include "se.h"
#if NSE > 0

#ifndef	lint
static	char sccsid[] = "@(#)se.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif	lint

/*#define SCSI_DEBUG			/* Turn on debugging code */
#define REL4				/* Enable release 4.00 mods */

/*
 * Generic scsi routines.
 */
#ifndef REL4
#include "../h/param.h"
#include "../h/systm.h"
#include "../h/dk.h"
#include "../h/buf.h"
#include "../h/conf.h"
#include "../h/dir.h"
#include "../h/user.h"
#include "../h/map.h"
#include "../h/vmmac.h"
#include "../h/ioctl.h"
#include "../h/uio.h"
#include "../h/kernel.h"
#include "../h/dkbad.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/sereg.h"
#include "../sundev/scsi.h"

#else REL4
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/sereg.h>
#include <sundev/scsi.h>
#endif REL4


/* Shorthand, to make the code look a bit cleaner. */
#define SENUM(se)	(se - se_ctlrs)
#define SEUNIT(dev)     (minor(dev) & 0x03)

int	se_watch();
int	se_probe(), se_slave(), se_attach(), se_go(), se_done(), se_poll();
int	se_ustart(), se_start(), se_getstatus(), se_reconnect(), se_deque();
int	se_off(), se_cmd(), se_cmdwait(), se_reset(), se_dmacnt();
char *  se_str_phase();

extern	int nse, scsi_host_id, scsi_reset_delay;
extern	u_char sc_cdb_size[];
extern	struct scsi_ctlr se_ctlrs[];		/* per controller structs */
extern	struct mb_ctlr *se_info[];
extern	struct mb_device *sdinfo[];
struct	mb_driver sedriver = {
	se_probe, se_slave, se_attach, se_go, se_done, se_poll,
	sizeof (struct scsi_si_reg), "sd", sdinfo, "se", se_info, MDR_BIODMA,
};

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr se_subr = {
	se_ustart, se_start, se_done, se_cmd, se_getstatus, se_cmdwait,
	se_off, se_reset, se_dmacnt, se_go, se_deque,
};

extern int scsi_ntype;
static int scsi_disconnect_enable = 0;	/* DISABLE DISCONNECTS */
/* extern int scsi_disre_enable;	/* global disconnect control */
extern struct scsi_unit_subr scsi_unit_subr[];

/*
 * Software copy of ser state (debug only)
 */
static u_char ser_state = 0;
static u_char junk;

/*
 * Patchable delays for debugging.
 */
int se_arbitration_delay = SE_ARBITRATION_DELAY;
int se_bus_clear_delay = SE_BUS_CLEAR_DELAY;
#ifdef notdef
int se_bus_free_delay = SE_BUS_SETTLE_DELAY;
#endif notdef
int se_bus_settle_delay = SE_BUS_SETTLE_DELAY;

/*
 * possible return values from se_arb_sel, se_cmd, 
 * se_putdata, se_putcmd, and se_reconnect.
 */
#define OK		0	/* successful */ 
#define FAIL		1	/* failed maybe recoverable */ 
#define HARD_FAIL	2	/* failed not recoverable */ 
#define SCSI_FAIL	3	/* failed due to scsi bus fault */ 
#define RESEL_FAIL	4	/* failed due to target reselection */ 

/* possible return values from se_process_complete_msg() */
#define CMD_CMPLT_DONE	0	/* cmd processing done */ 
#define CMD_CMPLT_WAIT	1	/* cmd processing waiting
				 * on sense cmd complete
				 */
/*
 * possible argument values for se_reset
 */
#define NO_MSG		0	/* don't print reset warning message */ 
#define PRINT_MSG	1	/* print reset warning message */ 

/*
 * additional messages
 */
#define SEL_FLG			0
#define RESEL_FLG		1
#define SC_EXTNDD_MSSG		0x01	/* extended message */
#define SC_MSSG_REJ		0x07	/* message reject */
#define SC_LNKD_CMD_CMPLT	0x0A	/* linked command complete */
#define SC_FLG_LNKD_CMD_CMPLT	0x0B	/* linked command complete(with flag) */

#ifdef SCSI_DEBUG
extern int scsi_debug;		/* 0 = normal operaion
				 * 1 = enable error logging and error msgs.
				 * 2 = as above + debug msgs.
				 * 3 = enable all info msgs.
				 */

u_int seintr_winner  = 0;	/* # of times we had an intr at end of siintr */
u_int seintr_loser   = 0;	/* # of times we didn't have an intr at end */

#define SE_WIN		seintr_winner++
#define SE_LOSE		seintr_loser++

/* Check for possible illegal SCSI-3 register access. */
#define SE_VME_OK(c, ser, str)	{\
	if ((IS_VME(c))  &&  (ser->csr & SE_CSR_DMA_EN)) \
		printf("se%d:  reg access during dma <%s>, csr 0x%x\n", \
			SENUM(c), str, ser);\
}
#define SE_DMA_OK(c, ser, str)  {\
	if (IS_VME(c)) { \
		if (ser->csr & SE_CSR_DMA_EN) \
			printf("%s: DMA DISABLED\n", str); \
		if (ser->csr & SE_CSR_DMA_CONFLICT) { \
			printf("%s: invalid reg access during dma\n", str); \
			DELAY(50000000); \
		} \
		ser->csr &= ~SE_CSR_DMA_EN; \
	} \
}
#define SCSI_TRACE(where, ser, un) \
	if (scsi_debug)		scsi_trace(where, ser, un)
#define SCSI_RECON_TRACE(where, c, data0, data1, data2) \
	if (scsi_debug)		scsi_recon_trace(where, c, data0, data1, data2)
#define DEBUG_DELAY(cnt) \
	if (scsi_debug)  DELAY(cnt)

/* Handy debugging 0, 1, and 2 argument printfs */
#define DPRINTF(str) \
	if (scsi_debug > 1) printf(str)
#define DPRINTF1(str, arg1) \
	if (scsi_debug > 1) printf(str,arg1)
#define DPRINTF2(str, arg1, arg2) \
	if (scsi_debug > 1) printf(str,arg1,arg2)

/* Handy error reporting 0, 1, and 2 argument printfs */
#define EPRINTF(str) \
	if (scsi_debug) printf(str)
#define EPRINTF1(str, arg1) \
	if (scsi_debug) printf(str,arg1)
#define EPRINTF2(str, arg1, arg2) \
	if (scsi_debug) printf(str,arg1,arg2)

#else SCSI_DEBUG
#define SE_WIN
#define SE_LOSE
#define SE_VME_OK(c, ser, str)
#define SE_DMA_OK(c, ser, str)
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 
#define DPRINTF1(str, arg2) 
#define DPRINTF2(str, arg1, arg2) 
#define EPRINTF(str) 
#define EPRINTF1(str, arg2) 
#define EPRINTF2(str, arg1, arg2) 
#define SCSI_TRACE(where, ser, un)
#define SCSI_RECON_TRACE(where, c, data0, data1, data2)
#endif SCSI_DEBUG

/*
 * trace buffer stuff.
 */
#ifdef SCSI_DEBUG
#define TRBUF_LEN	64
struct trace_buf {
	u_char wh[2];		/* where in code */
	u_char r[6];		/* 5380 registers */
	u_short csr;
	u_short bcr;
	u_int dma_addr;
	u_int dma_count;
};

struct disre_trace_buf {
	u_char wh[2];
	struct scsi_cdb cdb;
	u_short target;
	u_short lun;
	u_int data[3];
	u_int dma_count;
};

/* trace buffer */
struct trace_buf se_scsi_trace_buf[TRBUF_LEN];
int se_stbi = 0;

/* disconnect/reconnect trace buffer */
struct disre_trace_buf se_scsi_recon_trace_buf[TRBUF_LEN];
int se_strbi = 0;

static
scsi_trace(where, ser, un)
	register char where;
	register struct scsi_si_reg *ser;
	struct scsi_unit *un;
{
	register u_char *r = &(SBC_RD.cdr);
	register u_int i;
	register struct trace_buf *tb = &(se_scsi_trace_buf[se_stbi]);
#ifdef 	lint
	un= un;
#endif	lint

	tb->wh[0] = tb->wh[1] = where;
	for (i = 0; i < 6; i++)
		tb->r[i] = *r++;
	tb->csr = ser->csr;
	tb->bcr = 0;
	tb->dma_addr  = ser->dma_addr;
	tb->dma_count = ser->dma_cntr;

	if (++se_stbi >= TRBUF_LEN)
		se_stbi = 0;

	se_scsi_trace_buf[se_stbi].wh[0] = '?';		/* mark end */
}


static
scsi_recon_trace(where, c, data0, data1, data2)
	char where;
	register struct scsi_ctlr *c;
	register int data0, data1, data2;
{
	register struct scsi_unit *un = c->c_un;
	register struct disre_trace_buf *tb =  &(se_scsi_recon_trace_buf[se_strbi]);

	tb->wh[0] = tb->wh[1] = where;
	tb->cdb = un->un_cdb;
	tb->target = un->un_target;
	tb->lun = un->un_lun;
	tb->data[0] = data0;
	tb->data[1] = data1;
	tb->data[2] = data2;

	if (++se_strbi >= TRBUF_LEN)
		se_strbi = 0;

	se_scsi_recon_trace_buf[se_strbi].wh[0] = '?';	/* mark end */
}
#endif SCSI_DEBUG


/*
 * This routine unlocks this driver when I/O
 * has ceased, and a call to se_start is
 * needed to start I/O up again.  Refer to
 * xd.c and xy.c for similar routines upon
 * which this routine is based
 */
static
se_watch(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;
	un = c->c_un;

	/*
	 * Find a valid un to call s?start with to for next command
	 * to be queued in case DVMA ran out.  Try the pending que
	 * then the disconnect que.  Otherwise, driver will hang
	 * if DVMA runs out.  The reason for this kludge is that
	 * sestart should really be passed c instead of un, but it 
	 * would break lots of 3rd party S/W if this were fixed.
	 */
	if (un == NULL) {
		un = (struct scsi_unit *)c->c_tab.b_actf;
		if (un == NULL)
			un = (struct scsi_unit *)c->c_disqtab.b_actf;
	}
	se_start(un);
	timeout(se_watch, (caddr_t)c, 10*hz);
}


/*
 * Determine existence of SCSI host adapter.
 */
se_probe(ser, ctlr)
	register struct scsi_si_reg *ser;
	register int ctlr;
{
	register struct scsi_ctlr *c;
	c = &se_ctlrs[ctlr];

	/*
	 * Check for sbc - NCR 5380 Scsi Bus Ctlr chip.
	 * sbc is common * to sun3/50 onboard scsi and vme
	 * scsi board.
	 */
	if (peekc((char *)&ser->sbc_rreg.cbsr) == -1) {
		return (0);
	}

	/*
	 * Determine whether the host adaptor interface is onboard or vme.
	 */
	/* probe for sun3/50 dma interface */
	if (cpu == CPU_SUN3_50) {
		return (0);
	} else {
		/*
		 * Probe for vme scsi card but make sure it is not
		 * the SC host adaptor interface. SI vme scsi host
		 * adaptor occupies 2K bytes in the vme address space. 
		 * SC vme scsi host adaptor occupies 4K bytes in the 
		 * vme address space. So, peek past 2K bytes to 
		 * determine which host adaptor is there.
		 */
		if (peek((short *)&ser->dma_addr) == -1) {
			return (0);
		}

		if (peek((short *)&ser->csr) == -1) {
			return (0);
		}
		ser->csr = 0x0000;
		if(peek((short *)&ser->csr) != 0x0402) {
			return (0);
		}
		c->c_flags = 0;
	}


	/* probe for different scsi host adaptor interfaces */
	EPRINTF2("se%d:  seprobe: ser= 0x%x,  ", ctlr, (u_int)ser);
	EPRINTF1("c= 0x%x\n", (u_int)c );

	/* init controller information */
	if (scsi_disconnect_enable) {
		/* DPRINTF("seprobe:  disconnects enabled\n"); */
		c->c_flags |= SCSI_EN_DISCON;
	} else {
		EPRINTF("seprobe:  all disconnects disabled\n");
	}

	/* init controller information */
	c->c_flags |= SCSI_PRESENT;
	c->c_reg = (int)ser;
	c->c_ss = &se_subr;
	c->c_name = "se";
	c->c_tab.b_actf = NULL;
	c->c_tab.b_active = 0;
	se_reset(c, NO_MSG);			/* quietly reset the scsi bus */
	c->c_un = NULL;
	timeout(se_watch, (caddr_t)c, 10*hz);

#ifdef	lint
	se_intr(&se_ctlrs[nse - 1]);
#endif	lint

	return (sizeof (struct scsi_si_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
se_slave(md, ser)
	register struct mb_device *md;
	register struct scsi_si_reg *ser;
{
	register struct scsi_unit *un;
	register int type;

#ifdef	lint
	ser = ser;
#endif	lint
	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("se_slave: unknown type in md_flags");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == 0) {
		panic("se_slave: md_flags scsi type not configured in\n");
	}
	un->un_c = &se_ctlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}


/*
 * Attach device (boot time).
 */
se_attach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &se_ctlrs[md->md_ctlr];
	register struct scsi_si_reg *ser;
	ser = (struct scsi_si_reg *)c->c_reg;

	/* DPRINTF("se_attach:\n");*/
#ifdef	SCSI_DEBUG
	if (! scsi_disconnect_enable  &&  scsi_debug)
		printf("se%d:  seattach: disconnects disabled\n", SENUM(c));
#endif	SCSI_DEBUG

	if (type >= scsi_ntype) {
		panic("se_attach: unknown type in md_flags");
	}

	(*scsi_unit_subr[type].ss_attach)(md);

	/* Initialize interrupt vector */
	if (mc->mc_intr) {
		/* setup for vectored interrupts - we will pass ctlr ptr */
		ser->ivec2 = (mc->mc_intr->v_vec & 0xff);
		(*mc->mc_intr->v_vptr) = (int)c;
	}
}


/*
 * corresponding to un is an md.  if this md has SCSI commands
 * queued up (md->md_utab.b_actf != NULL) and md is currently
 * not active (md->md_utab.b_active == 0), this routine queues the
 * un on its queue of devices (c->c_tab) for which to run commands
 * Note that md is active if its un is already queued up on c->c_tab,
 * or its un is on the scsi_ctlr's disconnect queue c->c_disqtab.
 */
se_ustart(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct mb_ctlr *mc = un->un_mc;
	register struct mb_device *md = un->un_md;
	int s;

	DPRINTF("se_ustart:\n");
	s = splr(pritospl(mc->mc_intpri));
	if (md->md_utab.b_actf != NULL && md->md_utab.b_active == 0) {

		if (c->c_tab.b_actf == NULL) 
			c->c_tab.b_actf = (struct buf *)un;
		else 
			((struct scsi_unit *)(c->c_tab.b_actl))->un_forw = un;

		un->un_forw = NULL;
		c->c_tab.b_actl = (struct buf *)un;
		md->md_utab.b_active = 1;

	}
	(void)splx(s);
}

/*
 * this un is removed from the active
 * position of the scsi_ctlr's queue of devices (c->c_tab.b_actf) and
 * queued on scsi_ctlr's disconnect queue (c->c_disqtab).
 */
se_discon_queue (un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;

	/* DPRINTF("se_discon_queue:\n"); */

	/* a disconnect has occurred, so mb_ctlr is no longer active */
	c->c_tab.b_active = 0;
	c->c_un = NULL;

	/* remove disconnected md from mb_ctlr's queue */
	if((c->c_tab.b_actf = (struct buf *)un->un_forw) == NULL)
		c->c_tab.b_actl = NULL;

	/* put md on scsi_ctlr's disconnect queue */
	if (c->c_disqtab.b_actf == NULL)
		c->c_disqtab.b_actf = (struct buf *)un;
	 else
		((struct scsi_unit *)(c->c_disqtab.b_actl))->un_forw = un;

	c->c_disqtab.b_actl = (struct buf *)un;
	un->un_forw = NULL;

}

/*
 * searches the scsi_ctlr's disconnect queue for an md whose
 * corresponding un is the scsi unit defined by (target,lun).
 * then requeues this md on mc_ctlr's queue of devices.
 */
se_recon_queue (c, target, lun)
	register struct scsi_ctlr *c;
	register u_int target;
	register u_int lun;
{
	register struct scsi_unit *un;
	register struct scsi_unit *pun;
	register struct scsi_si_reg *ser;
	ser = (struct scsi_si_reg *)c->c_reg;

#ifdef lint
	ser = ser;
#endif lint

	DPRINTF("se_recon_queue:\n"); 

	/* search the disconnect queue */
	for (un=(struct scsi_unit *)(c->c_disqtab.b_actf), pun=NULL; un;
	     pun=un, un=un->un_forw) {
		DPRINTF("se_recon_queue: looking at next item on discon. q\n");
		if ((un->un_target == target) &&
		    (un->un_lun == lun))
			break;
	}
	DPRINTF("se_recon_queue: done searching disconnect queue\n");

	/* could not find the device in the disconnect queue */
	if (un == NULL) {
		printf("se%d:  se_reconnect: can't find dis unit: target %d lun %d\n",
			SENUM(c), target, lun);
		/* dump out disconnnect queue */
		printf("  disconnect queue:\n");
		for (un = (struct scsi_unit *)(c->c_disqtab.b_actf), pun = NULL; un;
                     pun = un, un = un->un_forw) {
			printf("\tun = 0x%x  ---  target = %d,  lun = %d\n",
				un, un->un_target, un->un_lun);
		}

		/*
		 * If we got here, we're probably in deep yogurt.
		 * We should reset, but instead we'll pass the buck.
		 */
		return (FAIL);
	}

	/* remove md from the disconnect queue */
	if (un == (struct scsi_unit *)(c->c_disqtab.b_actf))
		c->c_disqtab.b_actf = (struct buf *)(un->un_forw);
	else
		pun->un_forw = un->un_forw;
	if (un == (struct scsi_unit *)c->c_disqtab.b_actl)
		c->c_disqtab.b_actl = (struct buf *)pun;

	DPRINTF("se_recon_queue: done removing md from discon q\n");

	/* requeue un at the active position of mb_ctlr's queue
	 * of devices.
	 */

	if (c->c_tab.b_actf == NULL)  {
		un->un_forw = NULL;
		c->c_tab.b_actf = c->c_tab.b_actl = (struct buf *)un;
	} else {
		un->un_forw = (struct scsi_unit *)(c->c_tab.b_actf);
		c->c_tab.b_actf = (struct buf *)un;
	}

	DPRINTF("se_recon_queue: done re-qing un\n");

	/* scsi_ctlr now has an actively running SCSI command */
	c->c_tab.b_active = 1;

	/* indicate which unit is now running */
	c->c_un = un;

	DPRINTF("se_recon_queue: before returning\n");
	return (OK);
}


/* starts the next SCSI command */
se_start (un)
	struct scsi_unit *un;
{
	struct scsi_ctlr *c;
	struct mb_device *md;
	struct buf *bp;
	int s;

	/* DPRINTF("se_start:\n"); */

	/* return immediately if passed NULL un */
	if (un == NULL)
		return;

	c=un->un_c;

	/* return immediately, if the ctlr is already actively
	 * running a SCSI command
	 */
	s = splr(pritospl(3));
	if (c->c_tab.b_active != 0) {
		(void)splx(s);
		return;
	}

	/* if there are currently no SCSI devices queued to run
	 * a command, then simply return.  otherwise, obtain the
	 * next un for which a command should be run.
	 */
	if ((un=(struct scsi_unit *)(c->c_tab.b_actf)) == NULL) {
		(void)splx(s);
		return;
	}
	md = un->un_md;
	c->c_tab.b_active = 1;
	(void)splx(s);

	/* at this point we have obtained the next
	 * device for which to run a SCSI command,
	 * and we have also obtained the buf 
	 * corresponding to that device's next
	 * command.  
	 */

	/* if an attempt was already made to run
	 * this command, but the attempt was 
	 * pre-empted by a SCSI bus reselection
	 * then DVMA has already been set up, and
	 * we can call se_go directly
	 */
	if (un->un_flags & SC_UNF_PREEMPT) {
		DPRINTF("se_start: starting pre-empted");
		un->un_c->c_un = un;
		se_go(md);
		return;
	}

	/* 
	 * put bp into the active position
	 */
	bp = md->md_utab.b_actf;
	md->md_utab.b_forw = bp;

	/* only happens when called by intr */
	if (bp == NULL)
		return;

	/* special commands which are initiated by
	 * the high-level driver, are run using
	 * its special buffer, un->un_sbuf.
	 * in most cases, main bus set-up has
	 * already been done, so se_go can be called
	 * for on-line formatting, we need to
	 * call mbsetup.
	 */
	if ((*un->un_ss->ss_start)(bp, un)) {
		un->un_c->c_un = un;

		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0) &&
		    ((un->un_flags & SC_UNF_SPECIAL_DVMA) == 0)) {
			se_go(md);
			return;
		} else { 
			(void)mbugo(md);
			return;
		}
	} else {
		se_done(md);
	}
}


/*
 * Start up a scsi operation.
 * Called via mbgo after buffer is in memory.
 */
se_go(md)
	register struct mb_device *md;		/* fire up this mb ctlr */
{
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &se_ctlrs[mc->mc_ctlr];
	register struct scsi_unit *un = c->c_un;
	register struct buf *bp = md->md_utab.b_forw;
	register int unit;
	register int err;

	/* DPRINTF("se_go:\n"); */
	if (md == NULL  ||  md->md_utab.b_forw == NULL) {
		panic("se_go:  queueing error1\n");
	}

	un->un_baddr = MBI_ADDR(md->md_mbinfo);

	/*
	 * Diddle stats if necessary.
	 */
	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1 << unit;
		dk_xfer[unit]++;
		if (bp->b_flags & B_READ)
			dk_read[unit]++;
		dk_wds[unit] += bp->b_bcount >> 6;
	}

	/*
	 * Make the command block and fire it up in interrupt mode.
	 * If it fails right off the bat, call the interrupt routine 
	 * to handle the failure.
	 */
	if (un->un_flags & SC_UNF_PREEMPT)
		un->un_flags &= ~SC_UNF_PREEMPT;
	else
		(*un->un_ss->ss_mkcdb)(un);

	if ((err=se_cmd(c, un, 1)) != OK) {

	/* note that in the non-polled case (here) that
	 * SCSI_FAIL is not a possible return value from
	 * in se_cmd.
	 */
		if (err == FAIL) {
			/* DPRINTF("se_go:  se_cmd FAILED\n"); */
			(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);

		} else if (err == HARD_FAIL) {
			EPRINTF("se_go:  se_cmd hard FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
			se_off(c, un);
		}
	}
}

/*
 * Handle a polling SCSI bus interrupt.  Used
 * for multiple board support.
 */
se_poll()
{
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *ser;
	register int serviced = 0;

	/* DPRINTF("se_poll:\n"); */
	for (c = se_ctlrs; c < &se_ctlrs[nse]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		ser = (struct scsi_si_reg *)c->c_reg;
		/* si.c: in addition, check for CSR_DMA_IP or 
			DMA_CONFLICT condition */
		if ((ser->csr & (SE_CSR_SBC_IP)) == 0) {
			continue;
		}
		serviced = 1;
		se_intr(c);
	}
	return(serviced);
}

/* the SCSI command is done, so start up the next command
 */
se_done(md)
	struct mb_device *md;
{
	struct mb_ctlr *mc = md->md_mc;
	struct scsi_ctlr *c = &se_ctlrs[mc->mc_ctlr];
	struct scsi_unit *un;
	struct buf *bp = md->md_utab.b_forw;
	int type;

	/* DPRINTF("se_done:\n"); */

	/* more reliable than un = c->c_un; */
	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
		(*scsi_unit_subr[type].ss_unit_ptr)(md);

	/* advance mb_device queue */
	md->md_utab.b_actf = bp->av_forw;

	/* we are done, so clear buf in active position of 
	 * md's queue. then call iodone to complete i/o
	 */
	md->md_utab.b_forw = NULL;

	/* just got done with i/o so mark ctlr inactive 
	 * then advance to next md on the ctlr.
	 */
	c->c_tab.b_active = 0;
	c->c_tab.b_actf = (struct buf *)(un->un_forw);

	/* advancing the ctlr's queue has removed md from
	 * the queue.  if there are any more i/o for this
	 * md, se_ustart will queue up md again. at tail.
	 * first, need to mark md as inactive (not on queue)
	 */
	md->md_utab.b_active = 0;
	se_ustart(un);

	iodone(bp);

	/* check if the ctlr's queue is NULL, and if so,
	 * idle the controller
	 */
	if (c->c_tab.b_actf == NULL) {
		se_idle(c);
		return;
	}
	/* old se.c: check if SC_UNF_GET_SENSE set, call se_go() instead */

	/* start up the next command on the scsi_ctlr */
	se_start(un);
}

/*
 * Bring a unit offline.
 */
/*ARGSUSED*/
se_off(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
#ifdef	notdef
	register struct mb_device *md;
	md = un->un_md;
#endif	notdef
#ifdef lint
	c = c;
#endif lint

	/*
	 * Be warned, if you take the root device offline,
	 * the system is going to have a heartattack !
	 * Note, if unit is already offline, don't bother
	 * to print error message.
	 */
	if (un->un_present) {
#ifdef SCSI_DEBUG
		if (scsi_debug) printf("se%d:  %s%d, unit offline\n", SENUM(c),
			scsi_unit_subr[md->md_flags].ss_devname,
			SEUNIT(un->un_unit));
#endif SCSI_DEBUG
		un->un_present = 0;
	}
#ifdef	notdef
	/*
	 * This really doesn't work.  It would be a nice idea to
	 * fix it sometime.
	 */
	if (md->md_dk > 0) {
		dk_mspw[md->md_dk]=0;
	}
#endif	notdef
}


/*
 * Pass a command to the SCSI bus.
 * OK if fully successful, 
 * Return FAIL on failure (may be retryable),
 * SCSI_FAIL if we failed due to timing problem with SCSI bus. (terminal)
 * RESEL_FAIL if we failed due to target being in process of reselecting us.
 * (posponed til after reconnect done)
 */
se_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;		/* if 0, run cmd to completion
					 *       in polled mode
					 * if 1, allow disconnects
					 *       if enabled and use
					 *       interrupts
					 */
{
	register struct scsi_si_reg *ser;
	register int err;
	register u_char size;
	register int i;
	u_char id;
	u_char msg;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_cmd:\n");

	/* disallow disconnects if waiting for command completion */
	if (intr == 0) {
		c->c_flags &= ~SCSI_EN_DISCON;
	} else {
		/*
		 * If disconnect/reconnect globally disabled or only
		 * disabled for this command set internal flag.
		 * Otherwise, we enable disconnects and reconnects.
		 */
		if (!scsi_disconnect_enable  ||  (un->un_flags & SC_UNF_NO_DISCON))
			c->c_flags &= ~SCSI_EN_DISCON;
		else
			c->c_flags |= SCSI_EN_DISCON;
		un->un_wantint = 1;
	}

	/* disable Reconnect */
	/* SBC_WR.ser = ser_state = 0; */

	/* si.c: allow Reconnect, check odd-byte boundary, 
		 and set time-critical */

	un->un_flags &= ~SC_UNF_DMA_INITIALIZED;
	/* performing target selection */
	if ((err = se_arb_sel(c, ser, un)) != OK) {
		/* 
		 * May not be able to execute this command at this time due
		 * to a target reselecting us. Indicate this in the unit
		 * structure for when we perform this command later.
		 */
		/* si.c: does not set SBC_WR.ser */
		SBC_WR.ser = ser_state = scsi_host_id;
		if (err == RESEL_FAIL) {
			DPRINTF1("se_cmd:  preempted, tgt= %x\n",
				 un->un_target);
			un->un_flags |= SC_UNF_PREEMPT;
			err = OK;		/* not an error */

			/* si.c: check lost reselect INT */
		}
		un->un_wantint = 0;
		return (err);
	}

	/* si.c: clear resel and phase INTs and re-enable time-INT */

	/*
	 * We need to send out an identify message to target.
	 */
	if (c->c_flags & SCSI_EN_DISCON) {

		/* Tell target we do disconnects */
		/* DPRINTF("se_cmd:  disconnects ENABLED\n"); */
		id = SC_DR_IDENTIFY | un->un_cdb.lun;
		SBC_WR.tcr = TCR_MSG_OUT;
 		if (se_wait_phase(ser, PHASE_MSG_OUT) == OK) {
			if (se_putdata(c, PHASE_MSG_OUT, &id, 1) != OK) {
				EPRINTF1("se%d:  se_cmd: id msg failed\n",
					SENUM(c));
				se_print_state(ser, c);
               			se_reset(c, PRINT_MSG);
               		        return (FAIL);
			}
		}
	} else {
		/*
		 * Play dumb!  Since we didn't raise ATN, target
		 * thinks we don't do disconnects.
		 */
		DPRINTF("se_cmd:  NO disconnect\n");
	}

	/*
	 * Three fields in the per scsi unit structure
	 * hold information pertaining to the current dma
	 * operation: un_dma_curdir, un_dma_curaddr, and
	 * un_dma_curcnt. These fields are used to track
	 * the amount of data dma'd especially when disconnects 
	 * and reconnects occur.
	 * If the current command does not involve dma,
	 * these fields are set appropriately.
	 */
	if (un->un_dma_count != 0) {
		/* si.c: reset fifo */
		if (un->un_flags & SC_UNF_RECV_DATA) {
			un->un_dma_curdir = SE_RECV_DATA;
			ser->csr &= ~SE_CSR_SEND;
		} else {
			un->un_dma_curdir = SE_SEND_DATA;
			ser->csr |= SE_CSR_SEND;
		}

		/* save current dma info for disconnect */
		un->un_dma_curaddr = un->un_dma_addr;
		un->un_dma_curcnt = un->un_dma_count;
		se_vme_dma_setup(c,un);
	} else {
		un->un_dma_curdir = SE_NO_DATA;
		un->un_dma_curaddr = 0;
		un->un_dma_curcnt = 0;
		/* si.c: clear "bcr", se.c does not use it */
	}

#ifdef	SCSI_DEBUG
	if (scsi_debug) {
		printf("se%d: se_cmd: target %d, command= ",
			SENUM(c),un->un_target);
		se_print_cdb(un);
	}
#endif	SCSI_DEBUG

RETRY_CMD_PHASE:
	if (se_wait_phase(ser, PHASE_COMMAND) != OK) {
		/*
		 * Handle synchronous messages (6 bytes) and other
		 * unknown messages.  Note, all messages will be
		 * rejected.  It would be nice someday to figure
		 * out what to do with them; but not today.
		 */
		register u_char *icrp = &SBC_WR.icr;

		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_IN) {
			*icrp = SBC_ICR_ATN;
			msg = se_getdata(c, PHASE_MSG_IN);
			EPRINTF1("se_cmd:  rejecting msg 0x%x\n", msg);

			i = 128; /* accept 128 message bytes (overkill) */
			while((se_getdata(c, PHASE_MSG_IN) != -1)  &&  --i);

			if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_OUT) {
				msg = SC_MSG_REJECT;
				*icrp = 0;		/* turn off ATN */
				(void) se_putdata(c, PHASE_MSG_OUT, &msg, 1);
			}
			/* Should never fail this check. */
			if( i > 0 )
				goto RETRY_CMD_PHASE;
		}
		/* we've had a target failure, report it and quit */
		printf("se%d:  se_cmd: no command phase\n", SENUM(c));
		se_reset(c, PRINT_MSG);
		return (FAIL);
	}

	/*
	 * Determine size of the cdb.  Since we're smart, we
	 * look at group code and guess.  If we don't
	 * recognize the group id, we use the specified
	 * cdb length.  If both are zero, use max. size
	 * of data structure.
	 */
	if ((size = sc_cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) == 0  &&
	    (size = un->un_cmd_len) == 0) {
#ifdef SCSI_DEBUG
		if (scsi_debug) printf("se%d:  invalid cdb size, using size= %d\n",
			SENUM(c), sizeof (struct scsi_cdb));
#endif SCSI_DEBUG
		size = sizeof (struct scsi_cdb);
	}
	/* si.c: set SBC_WR.ser with HOST_ID here */
	/* si.c: se_putcmd() is called rather than se_putdata() */
	if (se_putdata(c, PHASE_COMMAND, 
	    (u_char *)&un->un_cdb, (int)size) != OK) {
		printf("se%d:  se_cmd: cmd put failed\n", SENUM(c));
		se_print_state(ser, c);
		se_reset(c, PRINT_MSG);
		return (FAIL);
	}

	/* If not polled I/O mode, we're done */
	if (intr) {
		/* si.c: did not setup dma until DATA phases,
			came in from INTERRPT routine */
		/* do final dma setup and start dma operation */
		if (un->un_dma_count != 0) {
			se_sbc_dma_setup(c, ser, (int)un->un_dma_curdir);
		} else if (un->un_wantint) {
			SBC_WR.mr |= SBC_MR_DMA;
			SBC_WR.tcr = TCR_UNSPECIFIED;
			ser->csr &= ~SE_CSR_SEND;
		}
		/* si.c: check lost phase change INT */ 
		return (OK);
	}

	/* 
	 * Polled SCSI data transfer mode.
	 */
	if (un->un_dma_curdir != SE_NO_DATA) {
		register u_char phase;

		if (un->un_dma_curdir == SE_RECV_DATA)
			phase = PHASE_DATA_IN;
		else
			phase = PHASE_DATA_OUT;

		/*
		 * Check if we have a problem with the command
		 * not going into data phase.  If we do,
		 * then we'll skip down and get any status.
		 * Of course, this means that the command
		 * failed.
		 */
		if ((se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		     SE_LONG_WAIT, 1) == OK)  &&
		     ((SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)) {
			/*
			 * Must actually start DMA xfer here - setup
			 * has already been done.  So, put sbc in dma
			 * mode and start dma transfer.
			 */
			se_sbc_dma_setup(c, ser, (int)un->un_dma_curdir);

			/*
			 * Wait for DMA to finish.  If it fails,
			 * attempt to get status and report failure.
			 */
			if ((err=se_cmdwait(c)) != OK) {
				EPRINTF("se_cmd:  cmdwait failed\n");
				se_print_state(ser, c);
				if (err != SCSI_FAIL)
					msg = se_getstatus(un);
				return (err);
			}
		} else {
			EPRINTF("se_cmd:  skipping data phase\n");
		}
	}

	/*
	 * Get completion status for polled command.
	 * Note, if <0, it's retryable; if 0, it's fatal.
	 * Someday I should give polled status results
	 * more options.  For now, everything is FATAL.
	 */
	EPRINTF("se_cmd:  getting status\n");
	if ((err=se_getstatus(un)) <= 0) {
		if (err == 0) 
			return (HARD_FAIL);
		else
			return (FAIL);
	}
	return (OK);
}


/*
 * Perform the SCSI arbitration and selection phases.
 * Returns FAIL if unsuccessful, 
 * returns RESEL_FAIL if unsuccessful due to target reselecting, 
 * returns OK if all was cool.
 */
static
se_arb_sel(c, ser, un)
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *ser;
	register struct scsi_unit *un;
{
	register u_char *icrp = &SBC_WR.icr;
	register u_char *mrp = &SBC_WR.mr;
	register int j;
	register u_char icr;
	int ret_val;
	int s;
	int sel_retries = 0;

	DPRINTF("se_arb_sel:\n");

	/* 
	 * It seems that the tcr must be 0 for arbitration to work.
	 */
	SBC_WR.tcr = 0;
	/* si.c: clear mrp with SBC_MR_ARB, but not SBC_MR_DMA */
	*mrp &= ~SBC_MR_DMA;
	*icrp = 0;

#ifdef	SCSI_DEBUG
	if (scsi_debug > 2)
		se_print_state(ser, c);
#endif	SCSI_DEBUG

SE_RETRY_SEL:

	/* old se.c: wait for bus to become free here */

	for (j = 0; j < SE_ARB_RETRIES; j++) {

		/* wait for scsi bus to become free */
		if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
		    SE_WAIT_COUNT, 0) != OK) {
			printf("se%d:  se_arb_sel: scsi bus continuously busy\n", 
				SENUM(c));
			se_reset(c, PRINT_MSG);
			ret_val = SCSI_FAIL;
			goto SE_ARB_SEL_EXIT;
		}
		/* arbitrate for the scsi bus */
		SBC_WR.odr = scsi_host_id;

		*mrp |= SBC_MR_ARB;             /* turn on arb */

		/* wait for sbc to begin arbitration */
		if (se_sbc_wait((caddr_t)icrp, SBC_ICR_AIP, 
		    SE_SHORT_WAIT, 1) != OK) {
			/*
			 * sbc may never begin arbitration due to a
			 * target reselecting us, the initiator.
			 */
			*mrp &= ~SBC_MR_ARB;
			if ((SBC_RD.cbsr & SBC_CBSR_SEL) &&
			    (SBC_RD.cbsr & SBC_CBSR_IO) &&
			    (SBC_RD.cdr & scsi_host_id)) {
				DPRINTF("se_arb_sel:  reselect\n");
				ret_val = RESEL_FAIL;
				goto SE_ARB_SEL_EXIT;
			}
			EPRINTF1("se_arb_sel:  AIP never set, cbsr= 0x%x\n",
				 SBC_RD.cbsr);
			/* old se.c: exit */
			goto SE_ARB_SEL_RETRY;
		}

		/* check to see if we won arbitration */
		s = splr(pritospl(7));
		DELAY(se_arbitration_delay);
		if ((*icrp & SBC_ICR_LA) == 0  && 
		    ((SBC_RD.cdr & ~scsi_host_id) < scsi_host_id)) {

			/* 
			 * WON ARBITRATION!  Perform selection.
			 * If disconnect/reconnect enabled, set ATN.
			 * If not, skip ATN so target won't do disconnects.
			 */
			/* DPRINTF("se_arb_sel:  won arb\n"); */
			icr = SBC_ICR_SEL | SBC_ICR_BUSY | SBC_ICR_DATA;
                       if (c->c_flags & SCSI_EN_DISCON) {
				icr |= SBC_ICR_ATN;
			}
			DELAY(se_bus_clear_delay + se_bus_settle_delay);
			SBC_WR.odr = (1 << un->un_target) | scsi_host_id;
			*icrp = icr;
			*mrp &= ~SBC_MR_ARB;    /* turn off arb */
			/* si.c: delay(bus-clear_delay+bus_settle_delay) */
			goto SE_ARB_SEL_WON;
		}
		(void)splx(s);

SE_ARB_SEL_RETRY:
		*mrp &= ~SBC_MR_ARB;    /* turn off arb */
		EPRINTF("se_arb_sel:  lost arbitration\n");
#ifdef          SCSI_DEBUG
		se_print_state(ser, c);
#endif          SCSI_DEBUG
		/* si.c: check cbsr with RESEL, if failed, error */
	}
	/*
	* FAILED ARBITRATION even with retries.
	* This shouldn't ever happen since
	* we have the highest priority id on the scsi bus.
	*/
	*icrp = 0;
	printf("se%d:  se_arb_sel: never won arbitration\n", SENUM(c));
	se_print_state(ser, c);
	se_reset(c, PRINT_MSG);
	ret_val = FAIL;		/* may be retryable */


SE_ARB_SEL_EXIT:
	return (ret_val);

SE_ARB_SEL_WON:
	/* wait for target to acknowledge selection */
	*icrp &= ~SBC_ICR_BUSY;
	(void)splx(s);
	DELAY(1);
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SE_SHORT_WAIT, 1) != OK) {
		DPRINTF("se_arb_sel:  busy never set\n");
#ifdef          SCSI_DEBUG
		if (scsi_debug > 2)
			se_print_state(ser, c);
#endif          SCSI_DEBUG
		*icrp = 0;
		junk = SBC_RD.clr;      /* clear intr */
		if (un->un_present && (++sel_retries < SE_SEL_RETRIES))
			goto SE_RETRY_SEL;
		ret_val = HARD_FAIL;
		goto SE_ARB_SEL_EXIT;
	}
	/* DPRINTF("se_arb_sel:  selected\n"); */
	*icrp &= ~(SBC_ICR_SEL | SBC_ICR_DATA);
	return (OK);
}


/*
 * Set up the SCSI control logic for a dma transfer
 * for vme host adaptor. Requires 32-bit buffer alignment!!!!
 */
se_vme_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *ser;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_vme_dma_setup:\n");

	/* si:c set "dma_addr with dma_curaddr" and "count with 0" */
	/* setup starting dma address and number bytes to dma */
	ser->dma_addr = un->un_dma_count - un->un_dma_curcnt;
	ser->dma_cntr = un->un_dma_curcnt;

	if (un->un_dma_curdir == SE_SEND_DATA) {
		bcopy((caddr_t)DVMA + un->un_dma_curaddr,
		      (caddr_t)ser->dma_buf,
		      un->un_dma_curcnt);
	}
}


/*
 * Setup and start the sbc for a dma operation.
 */
se_sbc_dma_setup(c, ser, dir)
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *ser;
	register int dir;
{
	register struct scsi_unit *un = c->c_un;
	DPRINTF("se_sbc_dma_setup:\n");

	un->un_flags |= SC_UNF_DMA_INITIALIZED;
	SBC_WR.icr |= (SBC_ICR_BUSY | SBC_ICR_DATA);
	SBC_WR.mr |= SBC_MR_DMA;

	if (dir == SE_RECV_DATA) {
		SBC_WR.tcr = TCR_DATA_IN;
		SBC_WR.ircv = 0;	/* this starts DMA RECV process */
	} else {
		SBC_WR.tcr = TCR_DATA_OUT;
		SBC_WR.icr = SBC_ICR_DATA;
		SBC_WR.send = 0;	/* this starts DMA SEND process */
	}
}


/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */
se_dma_cleanup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *ser;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_dma_cleanup:\n");

	/* disable dma controller */
	ser->dma_addr = 0;
	ser->dma_cntr = 0;

	/* take sbc out of dma mode */
	SBC_WR.icr = 0;
	SBC_WR.mr &= ~SBC_MR_DMA;
	SBC_WR.tcr = TCR_DATA_OUT;
}


u_char msk_tbl[7] [4] ={
	{0x00,0xff,0x00,0xff},		/* bad entry */
	{0xf5,0x10,0xe2,0x02},		/* Selection/Reselection */
	{0xf6,0x90,0xc2,0x40},		/* End of Process */
	{0xf7,0x10,0x7f,0x00},		/* SCSI Bus Reset */
	{0xbc,0x38,0xc2,0x40},		/* Parity Error */
	{0xfd,0x10,0xe2,0x60},		/* Bus Mismatch Error */
	{0xf7,0x14,0xff,0x00}		/* Loss of BSY- */
};

#define	SELECTION	1
#define	EOP		2
#define	BUS_RESET	3
#define	PARITY		4
#define	BUS_MISMATCH	5
#define	LOSS_OF_BSY	6

/*
 * Determine the cause of the interrupt.
 * Returns interrupt cause or 0 for unsuccessful.
 */
get_int_typ(cbsr,bsr)
	register u_char cbsr;
	register u_char bsr;
{
	register int i;

	for (i = 0; i < 7; i++) {
		if (((bsr & msk_tbl[i][0]) == msk_tbl[i][1]) &&
		    ((cbsr & msk_tbl[i][2]) == msk_tbl[i][3]))
			return (i);
	}
	return (0);
}

/*
 * Handle a scsi interrupt.
 */
se_intr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *ser;
	register struct scsi_unit *un = c->c_un;
	register int status = SE_NO_ERROR;
	register int resid = 0;
	register int dma_cleanup = 0;	/* need to clean up after dma */
	register int int_typ;
	register int disconnect = 0;	/* had a disconnect */
	register int reset_occurred = 0;
	register int i;			/* for debugging */
	register u_char cbsr;
	register u_char cdr;
	register u_char bsr;
	register u_char msg;
	register u_int lun;
#ifdef notdef
	register int err;
#endif notdef
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_intr:\n");

	if (ser->csr & SE_CSR_SBC_IP) {

		/*
		 * These two registers determine what type of interrupt
		 * occurred, NCR 5380 SCSI Interface Chip Design Manual,
		 * Section 8, pg 19, Ints
		 */
		cbsr = SBC_RD.cbsr;
		bsr = SBC_RD.bsr;

		int_typ = get_int_typ(cbsr,bsr);

		/* acknowledge sbc interrupt */
		junk = SBC_RD.clr;

		switch (int_typ) {	/* service the appropriate Int */
		case 0:			/* This intr should never happen */
			se_reset(c, PRINT_MSG);
			return;

		case SELECTION:		/* (Re)Selection Interrupt tom */
			EPRINTF("se_intr:  (re)selection\n");
			if (cbsr & SBC_CBSR_IO) {
				EPRINTF("    se_intr: RESELECTION Interrupt\n");

				if (SBC_RD.cdr & scsi_host_id) {
					cdr = SBC_RD.cdr & ~scsi_host_id;
					for (i = 0; i < 8; i++) {
						if (cdr & (1 << i))
							break;
					}
					cdr &= ~(1 << i);
					if (cdr != 0) {
						se_reset(c, PRINT_MSG);
						return;
					}
					c->c_recon_target = i;
					/* disable reconnects */
					SBC_WR.ser = ser_state = 0;
					/* assert BSY within 200 usec */
					SBC_WR.icr |= SBC_ICR_BUSY;
					/* old se.c: missing "SE_WAIT_COUNT" argument */
					if(se_sbc_wait((caddr_t)&SBC_RD.cbsr,
					    SBC_CBSR_SEL,SE_WAIT_COUNT,0) != OK) {
						se_reset(c, PRINT_MSG);
						return;
					}
					SBC_WR.icr &= ~SBC_ICR_BUSY;
					/* old se.c: missing "SE_WAIT_COUNT" argument */

					if(se_sbc_wait((caddr_t)&SBC_RD.cbsr,
					   SBC_CBSR_REQ,SE_WAIT_COUNT,1) != OK) {
						se_reset(c, PRINT_MSG);
						return;
					}
					if((SBC_RD.cbsr & CBSR_PHASE_BITS) !=
					    PHASE_MSG_IN) {
						junk = SBC_RD.clr;
						se_reset(c, PRINT_MSG);
						return;
					}
					junk = SBC_RD.clr;
					msg = SBC_RD.cdr;
					lun = msg & 0x07;
					msg &= 0xF0;
					if ((msg == SC_IDENTIFY) ||
					    (msg == SC_DR_IDENTIFY) ) {
						if (se_reconnect(c,
						    (u_int)c->c_recon_target,lun) !=
						    OK) {
							se_reset(c, PRINT_MSG);
						}
						return;
					}

				}	/* end of if(scsi_host_id) */

			}	/* end of if(SBC_CBSR_IO) */
			else {
				EPRINTF("    se_intr: SELECTION Interrupt\n");
#ifdef notdef
				/* err = se_sel(c,un,SEL_FLG); */
#ifdef	lint
				/* err = err; */
#endif	lint
#endif notdef
				EPRINTF("    se_intr: break of (RE)SELECTION Int\n");
				return;
			}
			break;

		case EOP:		/* End of Process Interrupt */
			EPRINTF("se_intr:  eop\n");
			cbsr = SBC_RD.cbsr;  /* may have changed on DMA send */

			switch (cbsr & CBSR_PHASE_BITS) {
			case PHASE_STATUS:
				EPRINTF("se_intr:	status\n");
				if (SBC_RD.tcr == TCR_UNSPECIFIED) {
					SBC_WR.mr &= ~SBC_MR_DMA;
					SBC_WR.tcr = TCR_DATA_OUT;
				} else
					dma_cleanup = 1;
				break;
			default:
				printf("se_intr: EOP - default\n");
				dma_cleanup = 1;
				status = SE_FATAL;
				break;
			}
			break;

		case BUS_MISMATCH:			/* Bus Phase Mismatch Interrupt */
			EPRINTF("se_intr:  bus mismatch\n");
			cbsr = SBC_RD.cbsr;		/* may have changed on DMA send */
			switch (cbsr & CBSR_PHASE_BITS) {
			case PHASE_DATA_OUT:
				printf("    se_intr: BUS_MISMATCH phase, PHASE_DATA_OUT\n");
			case PHASE_DATA_IN:
				if ((un == NULL ) ||
				    ((int)un->un_dma_curcnt < 0) ||
				    (un->un_dma_curdir == SE_NO_DATA) ) {
					se_reset(c, PRINT_MSG);
					return;
				}

				se_sbc_dma_setup(c, ser, (int)un->un_dma_curdir);
				return;

			case PHASE_COMMAND:
				printf("    se_intr: BUS_MISMATCH phase, PHASE_COMMAND\n");
				break;

			case PHASE_STATUS:
				EPRINTF("se_intr:  status\n");
				if (SBC_RD.tcr == TCR_UNSPECIFIED) {
					SBC_WR.mr &= ~SBC_MR_DMA;
					SBC_WR.tcr = TCR_DATA_OUT;
				} else {
					dma_cleanup = 1;
					if (un->un_dma_curdir == SE_RECV_DATA) {
						/*
						 * move the received data
						 * to DVMA
						 */
						bcopy((caddr_t)ser->dma_buf,
						      (caddr_t)DVMA +
							  un->un_dma_curaddr,
						      un->un_dma_curcnt -
							  ser->dma_cntr);
					}
				}
				break;

			case PHASE_MSG_IN:
				EPRINTF("se_intr:  msg\n");
				msg = se_getdata(c,PHASE_MSG_IN);
				switch (msg) {
				case SC_COMMAND_COMPLETE:
					printf("    se_intr: MSG_IN phase, SC_COMMAND_COMPLETE\n");
					se_print_state(ser, c);
					break;

				case SC_EXTNDD_MSSG:
					printf("    se_intr: MSG_IN phase, SC_EXTNDD_MSSG\n");
					se_print_state(ser, c);
					break;

				case SC_SAVE_DATA_PTR:
					printf("    se_intr: MSG_IN phase, SC_SAVE_DATA_PTR\n");
/*
 *					un->un_dma_curcnt = ser->dma_cntr;
 *					un->un_dma_curaddr = ser->dma_addr;
 */
					return;

				case SC_RESTORE_PTRS:
					printf("    se_intr: MSG_IN phase, SC_RESTORE_PTRS\n");
					se_print_state(ser, c);
					ser->dma_cntr = un->un_dma_curcnt;
					ser->dma_addr = un->un_dma_curaddr;
					return;

				case SC_DISCONNECT:
					printf("    se_intr: MSG_IN phase, SC_DISCONNECT\n");
					disconnect = 1;
					SBC_WR.ser = ser_state = 0;
					if (se_disconnect(c) != OK) {
						se_reset(c, PRINT_MSG);
						return;
					}
					SBC_WR.ser = ser_state = scsi_host_id;
					break;

				case SC_MSSG_REJ:
					printf("    se_intr: MSG_IN phase, SC_MSSG_REJ\n");
					se_print_state(ser, c);
					break;

				case SC_LNKD_CMD_CMPLT:
					printf("    se_intr: MSG_IN phase, SC_LNKD_CMD_CMPLT\n");
					break;

				case SC_FLG_LNKD_CMD_CMPLT:
					printf("    se_intr: MSG_IN phase, SC_FLG_LNKD_CMD_CMPLT\n");
					se_print_state(ser, c);
					break;

				default:
					printf("unknown MSG_IN command\n");
					se_print_state(ser, c);
					return;

				}	/* end of switch(msg) */

				break;

			case PHASE_MSG_OUT:
				EPRINTF("    se_intr: MSG_OUT phase\n");
				break;

			default:
				se_print_state(ser, c);
				panic("se_intr: Phase Mismatch Error, Unknown Phase");
				break;
			}	/* end of switch(cbsr&CBSR_PHASE_BITS) */

			break;

		case BUS_RESET:			/* SCSI Bus Reset */
			printf("    se_intr: BUS_RESET Interrupt\n");
			ser->csr = 0;
			DELAY(100);
			ser->csr = SE_CSR_INTR_EN | SE_CSR_SCSI_RES;
			if (c->c_disqtab.b_actf != NULL) {
				c->c_flags |= SCSI_FLUSH_DISQ;
				c->c_flush = c->c_disqtab.b_actl;
			}
			status = SE_FATAL;
			reset_occurred = 1;
			if ((un == NULL) && c->c_disqtab.b_actf) {
				/* need to flush some disconnect tasks */
				se_idle(c);
				return;
			}
			break;

		case PARITY:		/* Parity Error Interrupt */
			status = SE_FATAL;
			se_print_state(ser, c);
			panic("se_intr: Parity Error Interrupt");
			break;

		case LOSS_OF_BSY:		/* Loss of BSY- signal */
			printf("    se_intr: Loss of BSY- Interrupt\n");
			status = SE_FATAL;
			se_print_state(ser, c);
			panic("se_intr: Loss of BSY-");
			break;

		default:
			break;
		}	/* end of switch(int_typ) */


		if (dma_cleanup) {
			resid = (unsigned)ser->dma_cntr;
			if (un->un_dma_curdir == SE_SEND_DATA) {
				if (resid == 0xFFFF)
					resid = 0;
				else
					resid += 1;
			}
			se_dma_cleanup(c);
		}

		/* pass interrupt info to unit */
		if (un && un->un_wantint && (disconnect == 0) ) {
			un->un_wantint = 0;
			if(status == SE_NO_ERROR) {
				if (se_getstatus(un) == 0) {
					status = SE_RETRYABLE;
				}
			} else if (reset_occurred == 0) {
				se_reset(c, PRINT_MSG);
			}

			(*un->un_ss->ss_intr)(c, resid, status);
		}
		/* si.c: check for reconnect already pending, if there is,
			 go back to beginning of INT routine */
		/* old se.c: check if next job is ready and controller not
			currentlty active, then check if the job had been
			preemptted, if so call se_cmd and send it out.
			Otherwise, call se_start() to do next job */
		/* old se.c: update mbinfo with un_baddr */
		se_start(un);
		DPRINTF("se_intr:  se_start end\n");
	} else {
		se_print_state(ser, c);
		panic("se_intr: Spurious Level 2 Interrupt");
	}
}


/*
 * Handle target disconnecting.
 */
se_disconnect(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_si_reg *ser;
	register u_short dma_counter;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_disconnect\n");

	dma_counter = ser->dma_cntr;

	/* save dma info for reconnect */
	if (un->un_dma_curdir != SE_NO_DATA) {

		if (un->un_flags & SC_UNF_DMA_INITIALIZED) {
			if ( (un->un_dma_curdir == SE_SEND_DATA)
			    && (dma_counter != un->un_dma_curcnt)
			    && (dma_counter != 0)) {
				printf("FIX THIS HERE!!!!\n");
			} else if ( un->un_dma_curdir == SE_RECV_DATA) {
				/*
				 * we have to move the read data here
				 * because of lack of semaphores on
				 * critical device. i.e. 64K buffer
				 */
				dma_counter = un->un_dma_curcnt;
				printf("    se_disconnect: put the data transfer here!!\n");
			}
		} else {
			/* we havent transfered any data yet */
			dma_counter = un->un_dma_count;
		}
/*
 *		if (un->un_dma_curdir == SE_RECV_DATA) {
 *			if (!se_dma_recv(c)) {
 *				return (FAIL);
 *			}
 *		}
 */

		/*
		 * Save dma information so dma can be
		 * restarted when a reconnect occurs.
		 */
		un->un_dma_curcnt = ser->dma_cntr;
		un->un_dma_curaddr = ser->dma_addr;
/*
 *		un->un_dma_curaddr += un->un_dma_curcnt - ser->dma_cntr;
 *		un->un_dma_curcnt = ser->dma_cntr;
 */
	}

	/*
	 * Remove this disconnected task from the ctlr ready queue and
	 * save on disconnect queue until a reconnect is done. 
	 */
	se_discon_queue(un);

	SBC_WR.mr &= ~SBC_MR_DMA;
	ser->dma_cntr = 0;
	return (OK);
}


/*
 * Complete reselection phase and reconnect to target.
 *
 * Return true if sucessful, 0 if not.
 *
 * NOTE: TODO: think about whether we really want to do the
 *	resets w/in this code, or let the return of 0 indicate
 *	to the caller that we need to reset.
 *
 * NOTE: this routine cannot use se_getdata() to get identify
 * msg from reconnecting target due to sun3/50 scsi interface.
 * The bcr must be setup before the target changes scsi bus to
 * data phase if the command being reconnected involves dma
 * (which we do not know until we get the identify msg). Thus
 * we cannot acknowledge the identify msg until some setup of
 * the host adaptor registers is done.
 * NOTE: se.c does not use bcr
 */
se_reconnect(c,target,lun)
	register struct scsi_ctlr *c;
	register u_int target;
	register u_int lun;
{
	register struct scsi_si_reg *ser;
	register struct scsi_unit *un;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_reconnect\n");

	if (se_recon_queue(c, target, lun) != OK) {
		DPRINTF("se_reconnect: failed se_recon_queue\n");
		return (FAIL);
	}

	/* disable other reconnection attempts */
	/* si.c: do not reset this */
	SBC_WR.ser = ser_state =  0;
	un = c->c_un;
	/* restart disconnect activity */
	if (un->un_dma_curdir != SE_NO_DATA) {
		/* restore mainbus resource allocation info */
		/* old se.c: update md_mbinfo from un_baddr */

		/* do initial dma setup */
		if (un->un_dma_curdir == SE_RECV_DATA)
			ser->csr &= ~SE_CSR_SEND;
		else
			ser->csr |= SE_CSR_SEND;
		se_vme_dma_setup(c,un);
	}

	/* we can finally acknowledge identify message */
	SBC_WR.icr = SBC_ICR_ACK;
	/* old se.c: missing "SE_WAIT_COUNT" argument */
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, 
	    SBC_CBSR_REQ, SE_WAIT_COUNT,0) != OK) {
		printf("se_recon: REQ not INactive, cbsr 0x%x\n", 
			SBC_RD.cbsr);
		se_reset(c, PRINT_MSG);
		return (FAIL);
	}
	SBC_WR.icr = 0;
	/* si.c: does not clear SBC_MR_DMA */
	SBC_WR.mr |= SBC_MR_DMA;
	return (OK);
}


/*
 * Remove timed-out I/O request and report error to
 * it's interrupt handler.
 * Return OK if sucessful, FAIL if not.
 */
se_deque(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *ser;
	register u_int target;
	register u_int lun;
	register struct scsi_unit *pun;
	int s;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_deque:\n");
	target = un->un_target;
	lun = un->un_lun;
	se_print_state(ser, c);
	s = splr(pritospl(un->un_mc->mc_intpri));
	un = c->c_un;			/* get current unit */

	/*
	 * If current SCSI I/O request is the one that timed out. Kill it.
	 */
	if ((int)un != NULL  &&
	    un->un_c->c_tab.b_active != 0  &&
	    un->un_target == target  &&
	    un->un_lun == lun) {
		EPRINTF("se_deque:  failed request at top of que\n");
		(*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
		(void) splx(s);
		return (OK);
	}

	for (un = (struct scsi_unit *)c->c_disqtab.b_actf, pun = NULL; 
	     un; pun = un, un = un->un_forw) {
		DPRINTF1("\tmd= 0x%x  ---  ", md);
		DPRINTF2("target= %d,  lun= %d\n", un->un_target, 
			un->un_lun);
		if ((un->un_target == target)  &&  (un->un_lun == lun))
			break;
	}

	/* Failed to find entry, dump out disconnnect queue */
	if (un == NULL) {
		printf("se_deque:  can't find: target %d, lun %d\n  que:\n",
			target, lun);
		(void) splx(s);
		return (FAIL);
	}

	/* Remove entity from disconnect queue */
	if (un == (struct scsi_unit *)c->c_disqtab.b_actf)
		c->c_disqtab.b_actf = (struct buf *)(un->un_forw);       
	else
		pun->un_forw = un->un_forw;
	if (un == (struct scsi_unit *)c->c_disqtab.b_actl)
		c->c_disqtab.b_actl = (struct buf *)pun;

	un->un_forw = NULL;

	/* Requeue at front of controller queue */
	if (un->un_c->c_tab.b_actf == NULL) {
		EPRINTF("se_deque:  reactivating request\n");
		if (un->un_c->c_tab.b_active != 0) {
		   EPRINTF("se_deque:  we have a reactivate problem\n");
		}
		un->un_c->c_tab.b_actf = (struct buf *)un;
		un->un_c->c_tab.b_actl = (struct buf *)un;
		un->un_c->c_tab.b_active = 1;
		c->c_un = un;
		(*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
	} else {
		EPRINTF("se_deque:  can't reactive request\n");
		se_reset(c, PRINT_MSG);
	}
	(void) splx(s);
	return (OK);
}


/*
 * No current activity for the scsi bus. May need to flush some
 * disconnected tasks if a scsi bus reset occurred before the
 * target reconnected, since a scsi bus reset causes targets to
 * "forget" about any disconnected activity.
 * Also, enable reconnect attempts.
 */
se_idle(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *ser;
	register struct scsi_unit *un;
	register int resid;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_idle:\n");

	if (c->c_flags & SCSI_FLUSHING) {
		EPRINTF1("se_idle:  flushing, flags 0x%x\n", c->c_flags);
		return;
	}

	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {
		EPRINTF("se_idle:  flushing disconnect que\n");
		 /* now in process of flushing tasks */
		c->c_flags &= ~SCSI_FLUSH_DISQ;
		c->c_flags |= SCSI_FLUSHING;
		for (un = (struct scsi_unit *)c->c_disqtab.b_actf;
		     un  &&  c->c_flush;
		     un = (struct scsi_unit *)c->c_disqtab.b_actf) {

		     /* keep track of last task to flush */
		     if (c->c_flush == (struct buf *) un)
				c->c_flush = NULL;

			/* remove tasks from disconnect queue */
		        c->c_disqtab.b_actf = (struct buf *)un->un_forw; 
		        un->un_forw = NULL;

			/* requeue on controller q */
			se_ustart(un);
			un->un_c->c_tab.b_active = 1;
			c->c_un = un;

			/* inform device routines of error */
			if (un->un_dma_curdir != SE_NO_DATA) {
				/* old se.c: store md_mbinfo with un_baddr */
				resid = un->un_dma_curcnt;
			} else {
				resid = 0;
			}
			(*un->un_ss->ss_intr)(c, resid, SE_RETRYABLE);
			/* si.c: call si_off(c.un) */
		}
		if (c->c_disqtab.b_actf == NULL)
			 c->c_disqtab.b_actl = NULL;
		c->c_flags &= ~SCSI_FLUSHING;
	}

	/* enable reconnect attempts */
	SBC_WR.ser = ser_state = scsi_host_id;

	if ((c->c_flags & SCSI_ONBOARD) == 0) {
		ser->dma_cntr = 0;
		ser->csr &= ~SE_CSR_SEND;
	}
}
#ifdef SCSI_DEBUG
u_char scsi_old_status[32];
#endif SCSI_DEBUG

/*
 * Get status bytes from scsi bus.
 */
se_getstatus(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct scsi_si_reg *ser;
	register u_char *cp = (u_char *)&un->un_scb;
	register int i;
	register int b;
	register u_char msg;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_getstatus:\n");
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
	    SE_LONG_WAIT, 1) != OK) {
		printf("se%d:  se_getstatus: REQ inactive\n", SENUM(c));
		se_print_state(ser, c);
		return (0);
	}
	if (se_wait_phase(ser, PHASE_STATUS) != OK) {
		printf("se%d:  se_getstatus: no STATUS phase\n", SENUM(c));
		se_print_state(ser, c);
		return (0);
	}

	/* get all the status bytes */
	for (i = 0;;) {
		b = se_getdata(c, PHASE_STATUS);
		if (b < 0) {
			break;
		}
		if (i < STATUS_LEN) {
#ifdef	SCSI_DEBUG
			scsi_old_status[i] = b;	/* debug */
#endif	SCSI_DEBUG
			cp[i++] = b;
		}
	}
#ifdef	SCSI_DEBUG
	if (scsi_debug > 2) {
		int x;
		register u_char *p = (u_char *)&un->un_scb;

		printf("se_getstatus:  %d status bytes", i);
		for (x = 0; x < i; x++)
			printf("  %x", *p++);
		printf("\n");
	}
#endif	SCSI_DEBUG
	/* si.c: wait for MSG_IN phase */

	/* get command complete message */
	msg = se_getdata(c, PHASE_MSG_IN);
	if (msg != SC_COMMAND_COMPLETE) {
		EPRINTF1("si_getstatus:  bogus msg_in 0x%x\n", msg);
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;

	/*
	 * Check status for error condition, return -1 if error.
	 * Otherwise, return positive byte count for no error.
	 */
	if (cp[0] & SCB_STATUS_MASK)
		return (-1);	/* retryable */

	return (i);	 	/* no error */
}


/* 
 * Wait for a scsi dma request to complete.
 * Disconnects were disabled in se_cmd when polling for command completion.
 * Called by drivers in order to poll on command completion.
 */
se_cmdwait(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *ser;
	register struct scsi_unit *un = c->c_un;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF("se_cmdwait:\n");
	/* si.c: wait for dma transfer completed */
	/* if command does not involve dma activity, then we are finished */
	if (un->un_dma_curdir == SE_NO_DATA) {
		DPRINTF("se_cmdwait:  no data\n");
		return (OK);
	}

	/* wait for dma completion */
	/* si.c: check SI_CSR_DMA_IP and SI_CSR_DMA_CONFLICT
		 in addition to SI_CSR_SBC_IP */
	if (se_wait((u_short *)&ser->csr, SE_CSR_SBC_IP, 1) != OK) {
		EPRINTF("se_cmdwait:  dma still in progress\n");
		se_reset(c, PRINT_MSG);
		return (SCSI_FAIL);
	}

	if ((SBC_RD.mr & SBC_MR_DMA) == 0) {
		EPRINTF("se_cmdwait:  sbc DMA disabled\n");
		se_reset(c, PRINT_MSG);
		return (SCSI_FAIL);
	}

	/* si.c: handle special dma recv a little different than here */
	if ( (un->un_dma_curdir == SE_RECV_DATA) ) {
		if (ser->dma_cntr == un->un_dma_curcnt) {
			EPRINTF("se_cmdwait:  DMA failed\n"); 
			se_reset(c, PRINT_MSG);
			return (FAIL);
		} else {		/* move the info to DVMA space */
			bcopy((caddr_t)ser->dma_buf,
			      (caddr_t)DVMA + un->un_dma_curaddr,
			      un->un_dma_curcnt - ser->dma_cntr);
		}
	} else if ((un->un_dma_curdir==SE_SEND_DATA) &&
	    (ser->dma_cntr != 0xFFFF)) {
		EPRINTF("se_cmdwait:  DMA failed\n"); 
		se_reset(c, PRINT_MSG);
		return (SCSI_FAIL);
	}
	/* ack sbc interrupt and cleanup */
	junk = SBC_RD.clr;
	se_dma_cleanup(c);
	/* si.c: check SI_CSR_SBC_IP, if set, read csr to clear INT */
	return (OK);
}


/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 * Returns OK for successful.  Otherwise, returns
 * FAIL.
 */
static
se_sbc_wait(reg, cond, wait_count, set)
	register caddr_t reg;
	register u_char cond;
	register int wait_count;
	register int set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < wait_count; i++) {
		regval = *reg;
		/* si.c: only check regval agaist cond , not equal */
		if ((set == 1)  &&  ((regval & cond) == cond)) {
			return (OK);
		}
		if ((set == 0)  &&  !(regval & cond)) {
			return (OK);
		} 
		DELAY(10);
	}
	DPRINTF("se_sbc_wait:  timeout\n");
	return (FAIL);
}


/*
 * Wait for a condition to be (de)asserted.
 * Used for monitor DMA controller.
 * Returns OK for successful.  Otherwise,
 * returns FAIL.
 */
static
se_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;

	for (i = 0; i < SE_WAIT_COUNT; i++) {
		regval = *reg;
		/* si.c: does not check (regval & cond) == cond */
		if ((set == 1)  &&  ((regval & cond) == cond)) {
			return (OK);
		}
		if ((set == 0)  &&  !(regval & cond)) {
			return (OK);
		} 
		DELAY(10);
	}
	return (FAIL);
}


/*
 * Wait for a phase on the SCSI bus.
 * Returns OK for successful.  Otherwise,
 * returns FAIL.
 */
static
se_wait_phase(ser, phase)
	register struct scsi_si_reg *ser;
	register u_char phase;
{
	register int i;

	DPRINTF2("se_wait_phase:  %s phase (0x%x)\n",se_str_phase(phase),phase);
	for (i = 0; i < SE_WAIT_COUNT; i++) {
		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)
			return (OK);
		DELAY(10);
	}
	return (FAIL);
}


/*
 * Put data onto the scsi bus.
 * Returns OK if successful, FAIL otherwise.
 */
static
se_putdata(c, phase, data, num)
	register struct scsi_ctlr *c;
	register u_short phase;
	register u_char *data;
	register int num;
{
	register struct scsi_si_reg *ser;
	register int i;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF2("se_putdata:  %s phase (0x%x) ", se_str_phase(phase), phase);
	/* DPRINTF1("num %d\n", num); */

	/* 
	 * Set up tcr so we can transmit data.
	 */
	SBC_WR.tcr = phase >> 2;

	/* put bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		/* wait for target to request a byte */
		if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
		    SE_WAIT_COUNT, 1) != OK) {
			printf("se%d:  putdata, REQ not active\n", SENUM(c));
			return (FAIL);
		}

		/* load data */
		/* DPRINTF2("putting byte # %d, = 0x%x\n", i, *data); */
		SBC_WR.odr = *data++;
		SBC_WR.icr = SBC_ICR_DATA;

		/* complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SE_WAIT_COUNT, 0) != OK) {
			printf("se%d:  putdata, req not INactive\n", SENUM(c));
			return (FAIL);
		}
		SBC_WR.icr = 0;			/* clear ack */
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;
	junk = SBC_RD.clr;			/* clear int */
	/* si.c: clear SBC_WR.icr here, rather than up in the loop */
	return (OK);
}


/*
 * Get data from the scsi bus.
 * Returns a single byte of data, -1 if unsuccessful.
 */
static
se_getdata(c, phase)
	register struct scsi_ctlr *c;
	register u_short phase;
{
	register struct scsi_si_reg *ser;
	register u_char icr;
	register u_char data;
	register int s;
	ser = (struct scsi_si_reg *)c->c_reg;

	DPRINTF2("se_getdata: %s phase (0x%x)",se_str_phase(phase),phase);

	/* wait for target request */
	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
	    SE_WAIT_COUNT, 1) != OK) {
		printf("se%d:  getdata, REQ not active, cbsr 0x%x\n",
			SENUM(c), SBC_RD.cbsr);
		se_print_state(ser, c);
		return (-1);
	}

	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != phase) {
		/* not the phase we expected */
		DPRINTF1("se_getdata:  unexpected phase, expecting %s\n",
			se_str_phase(phase));
		/* se_print_state(ser, c); */
		return (-1);
	}


	/* grab data and complete req/ack handshake */
	data = SBC_RD.cdr;
	icr = SBC_WR.icr;
	DPRINTF1("  icr= %x  ", icr);
	SBC_WR.icr = icr | SBC_ICR_ACK;

	if (se_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
	    SE_WAIT_COUNT, 0) != OK) {
		printf("se%d:  getdata, REQ not inactive\n", SENUM(c));
		return (-1);
	}


	if ((phase == PHASE_MSG_IN)  && 
	    ((data == SC_COMMAND_COMPLETE)  ||  (data == SC_DISCONNECT))) {
		/* DPRINTF("se_getdata:  done with ACK\n"); */
		s = splr(pritospl(7));
		/* si.c: set SBC_WR.tcr = TCR_UNSPECIFIED and 
			 also read SBC_RD.clr to clear INT */
		SBC_WR.icr = icr;		/* clear ack */
		SBC_WR.mr &= ~SBC_MR_DMA;
		(void) splx(s);
	} else {
		SBC_WR.icr = icr;		/* clear ack */
	}

	DPRINTF1("  data= 0x%x\n", data);
	return (data);
}


/*
 * Reset SCSI control logic and bus.
 */
se_reset(c, msg_enable)
	register struct scsi_ctlr *c;
	register int msg_enable;
{
	struct scsi_si_reg *ser;
	ser = (struct scsi_si_reg *)c->c_reg;

	if (msg_enable) {
		printf("se%d:  resetting scsi bus\n", SENUM(c));
		se_print_state(ser, c);
		DEBUG_DELAY(10000000);
	}

	/* reset scsi control logic */
	ser->csr = 0;
	DELAY(100);
	ser->csr = SE_CSR_SCSI_RES;

	ser->dma_addr = 0;
	ser->dma_cntr = 0;

	/* issue scsi bus reset (make sure INTS from sbc are disabled) */
	SBC_WR.icr = SBC_ICR_RST;
	DELAY(1000);
	SBC_WR.icr = 0;				/* clear reset */

	/* give reset scsi devices time to recover (> 2 Sec) */
	DELAY(scsi_reset_delay);
	junk = SBC_RD.clr;

	/* enable sbc interrupts */
	ser->csr |= SE_CSR_INTR_EN;

	/* disconnect queue needs flushing */
	if (c->c_disqtab.b_actf != NULL) {
		c->c_flags |= SCSI_FLUSH_DISQ;
		c->c_flush = c->c_disqtab.b_actl;
		se_idle(c);
	}
}


/*
 * Return residual count for a dma.
 */
se_dmacnt(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_si_reg *ser;
	register u_short cnt;
	ser = (struct scsi_si_reg *)c->c_reg;
	cnt = ser->dma_cntr;

	if (un->un_dma_curdir == SE_SEND_DATA) {
		if (cnt == 0xFFFF) {	/* SUCCESS on SEND of data */
			return (0);
		} else {
		return ((int)cnt + 1);
		}
	}

	return ((int)cnt);		/* residual amount after read */
}


/*
 * Print out the cdb.
 */
static
se_print_cdb(un)
	register struct scsi_unit *un;
{
	register u_char size;
	register u_char *cp;
	register u_char i;

	cp = (u_char *) &un->un_cdb;
	if ((size = sc_cdb_size[CDB_GROUPID(*cp)]) == 0  &&
	    (size = un->un_cmd_len) == 0) {

		/* If all else fails, use structure size */
		size = sizeof (struct scsi_cdb);
	}

	for (i = 0; i < size; i++)
		printf("  %x", *cp++);
	printf("\n");
}


/*
 * returns string corresponding to the phase
 */
static char *
se_str_phase(phase)
	register u_char phase;
{
	register int index = (phase & CBSR_PHASE_BITS) >> 2;
	static char *phase_strings[] = {
		"DATA_OUT",
		"DATA_IN",
		"COMMAND",
		"STATUS",
		"",
		"",
		"MSG_OUT",
		"MSG_IN",
		"BUS FREE",
	};
	if (((phase & SBC_CBSR_BSY) == 0)  &&  (index == 0))
		return (phase_strings[8]);
	else
		return (phase_strings[index]);
}


/*
 * returns string corresponding to the last phase.
 * Note, also encoded are internal messages in
 * addition to the last bus phase.  
 */
static char *
se_str_lastphase(phase)
	register u_char phase;
{
	static char *invalid_phase = "";
	static char *phase_strings[] = {
		"Identify MSG",		/* 0x80 */
		"Disconnect MSG",	/* 0x81 */
		"Cmd complete MSG",	/* 0x82 */
		"Restore ptr MSG",	/* 0x83 */
		"Save ptr MSG",		/* 0x84 */
		"Spurious"		/* 0x85 */
	};

	if (phase == 0x00)
		return (invalid_phase);

	if (phase >= 0x80) {
		if (phase > 0x85)
			return (invalid_phase);
		return (phase_strings[phase - 0x80]);

	} else {
		return (se_str_phase(phase));
	}
}


/*
 * print out the current hardware state
 */
static
se_print_state(ser, c)
	register struct scsi_si_reg *ser;
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;

	/* si.c: c_last_phase is always up-to-date */
	/* but old se.c: never update c_last_phase */
	printf("\tlast phase= 0x%x (%s)\n",
		c->c_last_phase, se_str_lastphase((u_char)c->c_last_phase));
	printf("\tcsr= 0x%x  tcr= 0x%x  ser= 0x%x\n",
			ser->csr, SBC_RD.tcr, ser_state);
	printf("\tcbsr= 0x%x (%s)  cdr= 0x%x  mr= 0x%x  bsr= 0x%x\n",
			SBC_RD.cbsr, se_str_phase(SBC_RD.cbsr), SBC_RD.cdr,
			SBC_RD.mr, SBC_RD.bsr);

	if (un != NULL) {
		printf("\ttarget= %d, lun= %d    ", un->un_target, un->un_lun);
		printf("DMA addr= 0x%x  count= %d (%d)\n",
			   un->un_dma_curaddr, un->un_dma_curcnt,
			   un->un_dma_count);
		printf("\tcdb=  ");
		se_print_cdb(un);
	}
}
#endif NSE > 0
