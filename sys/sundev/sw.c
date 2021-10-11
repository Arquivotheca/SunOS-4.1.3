/* @(#)sw.c       1.1 92/07/30 Copyr 1988 Sun Micro */
#include "sw.h"
#if NSW > 0

#ifndef lint
static  char sccsid[] = "@(#)sw.c       1.1 92/07/30 Copyr 1988 Sun Micro";
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
#include "../h/mman.h"

#include "../machine/pte.h"
#include "../machine/psl.h"
#include "../machine/mmu.h"
#include "../machine/cpu.h"
#include "../machine/scb.h"
#include "../machine/enable.h"
#include "../vm/seg.h"
#include "../machine/seg_kmem.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/swreg.h"
#include "../sundev/scsi.h"

#else REL4
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/dir.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/mman.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <machine/enable.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/swreg.h>
#include <sundev/scsi.h>
#endif REL4

/* Shorthand, to make the code look a bit cleaner. */
#define SWNUM(sw)	(sw - swctlrs)
#define SWUNIT(dev)     (minor(dev) & 0x03)

int	swwatch();
int	swprobe(), swslave(), swattach(), swgo(), sw_done(), swpoll();
int	swustart(), swstart(), sw_getstatus(), sw_reconnect(), sw_deque();
int	sw_off(), sw_cmd(), sw_cmdwait(), sw_reset();
int	sw_wait(), sw_sbc_wait(), sw_dmacnt(),sw_getdata();

extern	int nsw, scsi_host_id, scsi_reset_delay;
extern	struct scsi_ctlr swctlrs[];		/* per controller structs */
extern	struct mb_ctlr *swinfo[];
extern	struct mb_device *sdinfo[];
struct	mb_driver swdriver = {
	swprobe, swslave, swattach, swgo, sw_done, swpoll,
	sizeof (struct scsi_sw_reg), "sd", sdinfo, "sw", swinfo, MDR_BIODMA,
};

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr swsubr = {
	swustart, swstart, sw_done, sw_cmd, sw_getstatus, sw_cmdwait,
	sw_off, sw_reset, sw_dmacnt, swgo, sw_deque,
};

extern struct scsi_unit_subr scsi_unit_subr[];
extern int scsi_ntype;
extern int scsi_disre_enable;		/* enable disconnect/reconnects */
extern int scsi_debug;			/* 0 = normal operaion
					 * 1 = enable error logging and error msgs.
					 * 2 = as above + debug msgs.
					 * 3 = enable all info msgs.
					 */


/*
 * Patchable delays for debugging.
 */
static u_char junk;
int sw_arbitration_delay = SI_ARBITRATION_DELAY;
int sw_bus_clear_delay = SI_BUS_CLEAR_DELAY;
int sw_bus_settle_delay = SI_BUS_SETTLE_DELAY;

/*
 * possible return values from sw_arb_sel, sw_cmd, 
 * sw_putdata, and sw_reconnect.
 */
#define OK		0	/* successful */ 
#define FAIL		1	/* failed maybe recoverable */ 
#define HARD_FAIL	2	/* failed not recoverable */ 
#define SCSI_FAIL	3	/* failed due to scsi bus fault */ 
#define RESEL_FAIL	4	/* failed due to target reselection */ 

/* possible return values from sw_process_complete_msg() */
#define CMD_CMPLT_DONE	0	/* cmd processing done */ 
#define CMD_CMPLT_WAIT	1	/* cmd processing waiting
				 * on sense cmd complete
				 */
/*
 * possible argument values for sw_reset
 */
#define NO_MSG		0	/* don't print reset warning message */ 
#define PRINT_MSG	1	/* print reset warning message */ 

/* 
 * scsi CDB op code to command length decode table.
 */
static u_char cdb_size[] = {
	CDB_GROUP0,		/* Group 0, 6 byte cdb */
	CDB_GROUP1,		/* Group 1, 10 byte cdb */
	CDB_GROUP2,		/* Group 2, ? byte cdb */
	CDB_GROUP3,		/* Group 3, ? byte cdb */
	CDB_GROUP4,		/* Group 4, ? byte cdb */
	CDB_GROUP5,		/* Group 5, ? byte cdb */
	CDB_GROUP6,		/* Group 6, ? byte cdb */
	CDB_GROUP7		/* Group 7, ? byte cdb */
};



#ifdef SCSI_DEBUG

int sw_dis_debug = 0;	/* disconnect debug info */
u_int sw_winner  = 0;	/* # of times we had an intr at end of swintr */
u_int sw_loser   = 0;	/* # of times we didn't have an intr at end */

#define SW_WIN		sw_winner++
#define SW_LOSE		sw_loser++

/* Check for possible illegal SCSI-3 register access. */
#define SW_VME_OK(c, swr, str)	{\
	if ((IS_VME(c))  &&  (swr->csr & SI_CSR_DMA_EN)) \
		printf("sw%d:  reg access during dma <%s>, csr 0x%x\n", \
			SWNUM(c), str, swr);\
}
#define SW_DMA_OK(c, swr, str)  {\
	if (IS_VME(c)) { \
		if (swr->csr & SI_CSR_DMA_EN) \
			printf("%s: DMA DISABLED\n", str); \
		if (swr->csr & SI_CSR_DMA_CONFLICT) { \
			printf("%s: invalid reg access during dma\n", str); \
			DELAY(50000000); \
		} \
		swr->csr &= ~SI_CSR_DMA_EN; \
	} \
}
#define SCSI_TRACE(where, swr, un) \
	if (scsi_debug)		scsi_trace(where, swr, un)
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
#define SW_WIN
#define SW_LOSE
#define SW_VME_OK(c, swr, str)
#define SW_DMA_OK(c, swr, str)
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 
#define DPRINTF1(str, arg2) 
#define DPRINTF2(str, arg1, arg2) 
#define EPRINTF(str) 
#define EPRINTF1(str, arg2) 
#define EPRINTF2(str, arg1, arg2) 
#define SCSI_TRACE(where, swr, un)
#define SCSI_RECON_TRACE(where, c, data0, data1, data2)
#endif SCSI_DEBUG

#define CLEAR_INTERRUPT(msg) \
	if (SBC_RD.bsr & SBC_BSR_INTR) { \
		EPRINTF1("%s: int cleared\n", msg); \
		junk = SBC_RD.clr; \
	} else { \
		EPRINTF1("%s: no int\n", msg); \
	}




/*
 * trace buffer stuff.
 */
#ifdef SCSI_DEBUG
#define TRBUF_LEN	64
struct trace_buf {
	u_char wh[2];		/* where in code */
	u_char r[6];		/* 5380 registers */
	u_long csr;
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
int sw_stbi = 0;
struct trace_buf sw_trace_buf[TRBUF_LEN];

/* disconnect/reconnect trace buffer */
int sw_strbi = 0;
struct disre_trace_buf sw_recon_trace_buf[TRBUF_LEN];

static
scsi_trace(where, swr, un)
	register char where;
	register struct scsi_sw_reg *swr;
	struct scsi_unit *un;
{
	register u_char *r = &(SBC_RD.cdr);
	register u_int i;
	register struct trace_buf *tb = &(sw_trace_buf[sw_stbi]);
#ifdef 	lint
	un= un;
#endif	lint

	tb->wh[0] = tb->wh[1] = where;
	for (i = 0; i < 6; i++)
		tb->r[i] = *r++;
	tb->csr = swr->csr;
	tb->dma_addr  = GET_DMA_ADDR(swr);
	tb->dma_count = GET_DMA_COUNT(swr);

	if (++sw_stbi >= TRBUF_LEN)
		sw_stbi = 0;

	sw_trace_buf[sw_stbi].wh[0] = '?';		/* mark end */
}


static
scsi_recon_trace(where, c, data0, data1, data2)
	char where;
	register struct scsi_ctlr *c;
	register int data0, data1, data2;
{
	register struct scsi_unit *un = c->c_un;
	register struct disre_trace_buf *tb = 
		 &(sw_recon_trace_buf[sw_strbi]);

	tb->wh[0] = tb->wh[1] = where;
	tb->cdb = un->un_cdb;
	tb->target = un->un_target;
	tb->lun = un->un_lun;
	tb->data[0] = data0;
	tb->data[1] = data1;
	tb->data[2] = data2;

	if (++sw_strbi >= TRBUF_LEN)
		sw_strbi = 0;

	sw_recon_trace_buf[sw_strbi].wh[0] = '?';	/* mark end */
}
#endif SCSI_DEBUG

/*
 * Print out the cdb.
 */
static
sw_print_cdb(un)
	register struct scsi_unit *un;
{
	register u_char size;
	register u_char *cp;
	register u_char i;

	cp = (u_char *) &un->un_cdb;
	if ((size = cdb_size[CDB_GROUPID(*cp)]) == 0  &&
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
sw_str_phase(phase)
	register u_char phase;
{
	register int index = (phase & CBSR_PHASE_BITS) >> 2;
	static char *phase_strings[] = {
		"DATA OUT",
		"DATA IN",
		"COMMAND",
		"STATUS",
		"",
		"",
		"MSG OUT",
		"MSG IN",
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
sw_str_lastphase(phase)
	register u_char phase;
{
	static char *invalid_phase = "";
	static char *phase_strings[] = {
		"Spurious",		/* 0x80 */
		"Arbitration",		/* 0x81 */
		"Identify MSG",		/* 0x82 */
		"Save ptr MSG",		/* 0x83 */
		"Restore ptr MSG",	/* 0x84 */
		"Disconnect MSG",	/* 0x85 */
		"Reconnect MSG",	/* 0x86 */
		"Cmd complete MSG",	/* 0x87 */
	};

	if (phase >= 0x80) {
		if (phase > 0x87)
			return (invalid_phase);
		return (phase_strings[phase - 0x80]);
	} else {
		return (sw_str_phase(phase));
	}
}


/*
 * print out the current hardware state
 */
static
sw_print_state(swr, c)
	register struct scsi_sw_reg *swr;
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register short flag = 0;

	if ((swr->csr & SI_CSR_DMA_EN)) {
		swr->csr &= ~SI_CSR_DMA_EN;
		flag = 1;
	}

	printf("\tlast phase= 0x%x (%s)\n",
		c->c_last_phase, sw_str_lastphase((u_char)c->c_last_phase));
	printf("\tcsr= 0x%x  bcr= %d  tcr= 0x%x\n",
			swr->csr, swr->bcr, SBC_RD.tcr);
	printf("\tcbsr= 0x%x (%s)  cdr= 0x%x  mr= 0x%x  bsr= 0x%x\n",
			SBC_RD.cbsr, sw_str_phase(SBC_RD.cbsr), SBC_RD.cdr,
			SBC_RD.mr, SBC_RD.bsr);
	if (flag) {
		swr->csr |= SI_CSR_DMA_EN;
	}
#ifdef	SCSI_DEBUG
	printf("\tdriver wins= %d  loses= %d\n", sw_winner, sw_loser);
#endif	SCSI_DEBUG

	if (un != NULL) {
		printf("\ttarget= %d, lun= %d    ", un->un_target, un->un_lun);
		printf("DMA addr= 0x%x  count= %d (%d)\n",
			   un->un_dma_curaddr, un->un_dma_curcnt,
			   un->un_dma_count);
		printf("\tcdb=  ");
		sw_print_cdb(un);
	}
}

/*
 * This routine unlocks this driver when I/O
 * has ceased, and a call to swstart is
 * needed to start I/O up again.  Refer to
 * xd.c and xy.c for similar routines upon
 * which this routine is based
 */
static
swwatch(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;

	/*
	 * Find a valid un to call s?start with to for next command
	 * to be queued in case DVMA ran out.  Try the pending que
	 * then the disconnect que.  Otherwise, driver will hang
	 * if DVMA runs out.  The reason for this kludge is that
	 * swstart should really be passed c instead of un, but it
	 * would break lots of 3rd party S/W if this were fixed.
	 */
	un = (struct scsi_unit *)c->c_tab.b_actf;
	if (un == NULL)
		un = (struct scsi_unit *)c->c_disqtab.b_actf;
	if (un != NULL)
		swstart(un);
	timeout(swwatch, (caddr_t)c, 10*hz);
}



/*
 * Determine existence of SCSI host adapter.
 * Returns 0 for failure, size of sw data structure if ok.
 */
swprobe(swr, ctlr)
	register struct scsi_sw_reg *swr;
	register int ctlr;
{
	register struct scsi_ctlr *c = &swctlrs[ctlr];
	int val;

	/* 
	 * Check for sbc - NCR 5380 Scsi Bus Ctlr chip.
	 * sbc is common to sun3/50 onboard scsi and vme
	 * scsi board.
	 */
	if (peekc((char *)&swr->sbc_rreg.cbsr) == -1) {
		return (0);
	}

	/*
	 * Determine whether the host adaptor interface is onboard or vme.
	 */
	if (cpu == CPU_SUN4_110) {
		/* probe for 4/110 dma interface */
		if (peekl((long *)&swr->dma_addr, (long *)&val) == -1) {
			return (0);
		}
		c->c_flags = SCSI_COBRA;
	} 

	/* probe for different scsi host adaptor interfaces */
	EPRINTF2("sw%d:  swprobe: swr= 0x%x,  ", ctlr, (u_int)swr);
	EPRINTF1("c= 0x%x\n", (u_int)c );

	/* init controller information */
	if (scsi_disre_enable) {
		/* DPRINTF("swprobe:  disconnects enabled\n"); */
		c->c_flags |= SCSI_EN_DISCON;
	} else {
		EPRINTF("swprobe:  all disconnects disabled\n");
	}

        /*
         * Allocate a page for being able to
         * flush the last bit of a data transfer.
         */
        c->c_kment = rmalloc(kernelmap, (long)mmu_btopr(MMU_PAGESIZE));
        if (c->c_kment == 0) {
                printf("si%d: out of kernelmap for last DMA page\n", ctlr);
                return (0);
        }

	c->c_flags |= SCSI_PRESENT;
	c->c_last_phase = PHASE_CMD_CPLT;
	c->c_reg = (int)swr;
	c->c_ss = &swsubr;
	c->c_name = "sw";
	c->c_tab.b_actf = NULL;
	c->c_tab.b_active = C_INACTIVE;
	c->c_intpri = 2;		/* initalize before sw_reset() */	
	sw_reset(c, NO_MSG);		/* quietly reset the scsi bus */
	c->c_un = NULL;
	timeout(swwatch, (caddr_t)c, 10*hz);
	return (sizeof (struct scsi_sw_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
swslave(md, swr)
	register struct mb_device *md;
	register struct scsi_sw_reg *swr;
{
	register struct scsi_unit *un;
	register int type;

#ifdef 	lint
	swr = swr;
	junk = junk;
	junk = scsi_debug;
#endif	lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("swslave:  unknown type in md_flags\n");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == NULL) 
		panic("swslave:  md_flags scsi type not configured\n");

	un->un_c = &swctlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}


/*
 * Attach device (boot time).
 */
swattach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct scsi_ctlr *c = &swctlrs[md->md_ctlr];
	register struct scsi_sw_reg *swr = (struct scsi_sw_reg *)(c->c_reg);


	/* DPRINTF("swattach:\n"); */
#ifdef	SCSI_DEBUG
	if ((scsi_disre_enable != 0)  &&  scsi_debug)
		printf("sw%d:  swattach: disconnects disabled\n", SWNUM(c));
#endif	SCSI_DEBUG

	if (type >= scsi_ntype) 
		panic("swattach:  unknown type in md_flags\n");
	 
	(*scsi_unit_subr[type].ss_attach)(md);

	/*
	 * Make sure dma enable bit is off or 
	 * SI_CSR_DMA_CONFLICT will occur when 
	 * the iv_am register is accessed.
	 */
	c->c_intpri = md->md_mc->mc_intpri;
	swr->csr &= ~SI_CSR_DMA_EN;
        return;
}


/*
 * corresponding to un is an md.  if this md has SCSI commands
 * queued up (md->md_utab.b_actf != NULL) and md is currently
 * not active (md->md_utab.b_active == 0), this routine queues the
 * un on its queue of devices (c->c_tab) for which to run commands
 * Note that md is active if its un is already queued up on c->c_tab,
 * or its un is on the scsi_ctlr's disconnect queue c->c_disqtab.
 */
swustart (un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct mb_device *md = un->un_md;
	int s;

	DPRINTF("swustart:\n");
	s = splr(pritospl(c->c_intpri));
	if (md->md_utab.b_actf != NULL && md->md_utab.b_active == MD_INACTIVE) {
		if (c->c_tab.b_actf == NULL) 
			c->c_tab.b_actf = (struct buf *)un;
		else 
			((struct scsi_unit *)(c->c_tab.b_actl))->un_forw = un;

		un->un_forw = NULL;
		c->c_tab.b_actl = (struct buf *)un;
		md->md_utab.b_active |= MD_QUEUED;
		DPRINTF1("swustart: md_utab = %x\n", md->md_utab.b_active);

	}
	(void) splx(s);
}

/*
 * this un is removed from the active
 * position of the scsi_ctlr's queue of devices (c->c_tab.b_actf) and
 * queued on scsi_ctlr's disconnect queue (c->c_disqtab).
 */
sw_discon_queue (un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;

	/* DPRINTF("sw_discon_queue:\n"); */

	/* a disconnect has occurred, so mb_ctlr is no longer active */
	c->c_tab.b_active &= C_QUEUED;
	c->c_un = NULL;

	/* remove disconnected un from scsi_ctlr's queue */
	if((c->c_tab.b_actf = (struct buf *)un->un_forw) == NULL)
		c->c_tab.b_actl = NULL;

	/* put un on scsi_ctlr's disconnect queue */
	if (c->c_disqtab.b_actf == NULL)
		c->c_disqtab.b_actf = (struct buf *)un;
	 else
		((struct scsi_unit *)(c->c_disqtab.b_actl))->un_forw = un;

	c->c_disqtab.b_actl = (struct buf *)un;
	un->un_forw = NULL;

}

/*
 * searches the scsi_ctlr's disconnect queue for the un
 * which points to the scsi_unit defined by (target,lun).
 * then requeues this un on scsi_ctlr's queue of devices.
 */
sw_recon_queue (c, target, lun, flag)
	register struct scsi_ctlr *c;
	register short target, lun, flag;
{
	register struct scsi_unit *un;
	register struct scsi_unit *pun;

	DPRINTF("sw_recon_queue:\n"); 

	/* search the disconnect queue */
	for (un=(struct scsi_unit *)(c->c_disqtab.b_actf), pun=NULL; un;
	     pun=un, un=un->un_forw) {
		if ((un->un_target == target) &&
		    (un->un_lun == lun))
			break;
	}


	/* could not find the device in the disconnect queue */
	if (un == NULL) {
		printf("sw%d:  sw_reconnect: can't find dis unit: target %d lun %d\n",
			SWNUM(c), target, lun);
		/* dump out disconnnect queue */
		printf("  disconnect queue:\n");
		for (un = (struct scsi_unit *)(c->c_disqtab.b_actf), pun = NULL; un;
                     pun = un, un = un->un_forw) {
			printf("\tun = 0x%x  ---  target = %d,  lun = %d\n",
				un, un->un_target, un->un_lun);
		}
		return (FAIL);
	}
	if (flag)
		return(OK);

	/* remove un from the disconnect queue */
	if (un == (struct scsi_unit *)(c->c_disqtab.b_actf))
		c->c_disqtab.b_actf = (struct buf *)(un->un_forw);
	else
		pun->un_forw = un->un_forw;

	if (un == (struct scsi_unit *)c->c_disqtab.b_actl)
		c->c_disqtab.b_actl = (struct buf *)pun;

	/* requeue un at the active position of scsi_ctlr's queue
	 * of devices.
	 */

	if (c->c_tab.b_actf == NULL)  {
		un->un_forw = NULL;
		c->c_tab.b_actf = c->c_tab.b_actl = (struct buf *)un;
	} else {
		un->un_forw = (struct scsi_unit *)(c->c_tab.b_actf);
		c->c_tab.b_actf = (struct buf *)un;
	}

	/* scsi_ctlr now has an actively running SCSI command */
	c->c_tab.b_active |= C_ACTIVE;
	c->c_un = un;
	return (OK);
}

/* starts the next SCSI command */
swstart (un)
	struct scsi_unit *un;
{
	struct scsi_ctlr *c;
	struct mb_device *md;
	struct buf *bp;
	int s;

	DPRINTF("swstart:\n"); 

	if (un == NULL)
		return;

	/* return immediately, if the ctlr is already actively
	 * running a SCSI command
	 */
	s = splr(pritospl(3));
	c = un->un_c;
#ifdef notdef
	c = un->un_c;
	s = splr(pritospl(c->c_intpri));
#endif notdef

	if (c->c_tab.b_active != C_INACTIVE) {
		DPRINTF1("sistart: locked (0x%x)\n", c->c_tab.b_active);
		(void) splx(s);
		return;
	}

	/* if there are currently no SCSI devices queued to run
	 * a command, then simply return.  otherwise, obtain the
	 * next un for which a command should be run.
	 */
	if ((un=(struct scsi_unit *)(c->c_tab.b_actf)) == NULL) {
		DPRINTF("swstart: no device to run\n");
		(void) splx(s);
		return;
	}
	md = un->un_md;
	c->c_tab.b_active |= C_QUEUED;

	/* if an attempt was already made to run
	 * this command, but the attempt was 
	 * pre-empted by a SCSI bus reselection
	 * then DVMA has already been set up, and
	 * we can call swgo directly
	 */
	if (md->md_utab.b_active & MD_PREEMPT) {
		DPRINTF1("swstart: md_utab = %d\n", md->md_utab.b_active);
		DPRINTF("swstart: starting pre-empted");
		un->un_c->c_un = un;
		swgo(md);
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}
	if (md->md_utab.b_active & MD_IN_PROGRESS) {
		DPRINTF1("swstart: md_utab = %d\n", md->md_utab.b_active);
		DPRINTF("swstart: md in progress\n");
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}
	md->md_utab.b_active |= MD_IN_PROGRESS;
	bp = md->md_utab.b_actf;
	md->md_utab.b_forw = bp;

	/* only happens when called by intr */
	if (bp == NULL) {
		EPRINTF("swstart: bp is NULL\n");
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}

	/* special commands which are initiated by
	 * the high-level driver, are run using
	 * its special buffer, un->un_sbuf.
	 * in most cases, main bus set-up has
	 * already been done, so swgo can be called
	 * for on-line formatting, we need to
	 * call mbsetup.
	 */
	if ((*un->un_ss->ss_start)(bp, un)) {
		un->un_c->c_un = un;

		if (bp == &un->un_sbuf &&
		    ((un->un_flags & SC_UNF_DVMA) == 0) &&
		    ((un->un_flags & SC_UNF_SPECIAL_DVMA) == 0)) {
			swgo(md);
		} else { 
			if (mbugo(md) == 0) {
				md->md_utab.b_active |= MD_NODVMA;
			}
		}
	} else {
		sw_done(md);
	}
	c->c_tab.b_active &= C_ACTIVE;
	(void) splx(s);
}

/*
 * Start up a scsi operation.
 * Called via mbgo after buffer is in memory.
 */
swgo(md)
	register struct mb_device *md;
{
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	register int unit;
	register int err;
	int type;
	int s;

	DPRINTF("swgo:\n"); 
	s = splr(pritospl(3));
	if (md == NULL)  {
		(void) splx(s);
		panic("swgo:  queueing error1\n");
	}
	c = &swctlrs[md->md_mc->mc_ctlr];

#ifdef notdef
	if (md == NULL) 
		panic("swgo:  queueing error1\n");

	c = &swctlrs[md->md_mc->mc_ctlr];
	s = splr(pritospl(c->c_intpri));
#endif notdef

	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);

	bp = md->md_utab.b_forw;

	if (bp == NULL) {
		EPRINTF("swgo: bp is NULL\n");
		(void) splx(s);
		return;
	}

	un->un_baddr = MBI_ADDR(md->md_mbinfo);

	if (md->md_utab.b_active & MD_NODVMA) {
		md->md_utab.b_active &= ~MD_NODVMA;
		md->md_utab.b_active |= MD_PREEMPT;
		(*un->un_ss->ss_mkcdb)(un);
		(void) splx(s);
		return;
	}

	c->c_tab.b_active |= C_QUEUED;

	/*
	 * Diddle stats if necessary.
	 */
	if ((unit = un->un_md->md_dk) >= 0) {
		dk_busy |= 1<<unit;
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
	if (md->md_utab.b_active & MD_PREEMPT)
		md->md_utab.b_active &= ~MD_PREEMPT;
	else
		(*un->un_ss->ss_mkcdb)(un);
	DPRINTF1("swgo: md_utab = %d\n", md->md_utab.b_active);

	if ((err=sw_cmd(c, un, 1)) != OK) {
		if (err == FAIL) {
			/* DPRINTF("swgo:  sw_cmd FAILED\n"); */
			(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
		} else if (err == HARD_FAIL) {
			EPRINTF("swgo:  sw_cmd hard FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
		} else if (err == SCSI_FAIL) {
                        DPRINTF("swgo:  sw_cmd scsi FAIL\n");
                        (*un->un_ss->ss_intr)(c, 0, SE_FATAL);
                        sw_off(c, un, SE_FATAL);
                }
	}
	c->c_tab.b_active &= C_ACTIVE;
	DPRINTF1("swgo: end  md_utab = %d\n", md->md_utab.b_active);
	(void) splx(s);
}


/*
 * Handle a polling SCSI bus interrupt.
 */
swpoll()
{
	register struct scsi_ctlr *c;
	register struct scsi_sw_reg *swr;
	register int serviced = 0;

	/* DPRINTF("swpoll:\n"); */
	for (c = swctlrs; c < &swctlrs[nsw]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		(int)swr = c->c_reg;
		if ((swr->csr & (SI_CSR_SBC_IP | SI_CSR_DMA_IP
			| SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR)) == 0) {
			continue;
		}
		serviced = 1;
		swintr(c);
	}
	return (serviced);
}

/* the SCSI command is done, so start up the next command
 */
sw_done (md)
	struct mb_device *md;
{
	struct scsi_ctlr *c;
	struct scsi_unit *un;
	struct buf *bp;
	int type;
	int s;

	EPRINTF("swdone:\n");
	
	s = splr(pritospl(3));
	c = &swctlrs[md->md_mc->mc_ctlr];
#ifdef notdef
	s = splr(pritospl(c->c_intpri));
#endif notdef

	bp = md->md_utab.b_forw;

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
	c->c_tab.b_actf = (struct buf *)(un->un_forw);

	/* advancing the ctlr's queue has removed un from
	 * the queue.  if there are any more i/o for this
	 * md, swustart will queue up un again. at tail.
	 * first, need to mark md as inactive (not on queue)
	 */
	DPRINTF1("swdone: md_utab = %d\n", md->md_utab.b_active);
	md->md_utab.b_active = MD_INACTIVE;
	swustart(un);
	iodone(bp);

	/* start up the next command on the scsi_ctlr */
	swstart(un);
	(void) splx(s);
}


/*
 * Bring a unit offline.
 */
/*ARGSUSED*/
sw_off(c, un, flag)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	int flag;
{
	register struct mb_device *md = un->un_md;
	char *msg = "online";

	/*
	 * Be warned, if you take the root device offline,
	 * the system is going to have a heartattack !
	 * Note, if unit is already offline, don't bother
	 * to print error message.
	 */
	if (un->un_present) {
		if (flag == SE_FATAL) {
                        msg = "offline";
                        un->un_present = 0;
                }
		printf("sw%d:  %s%d, unit %s\n", SWNUM(c),
			scsi_unit_subr[md->md_flags].ss_devname,
			SWUNIT(un->un_unit), msg);
	}
}


/*
 * Pass a command to the SCSI bus.
 * OK if fully successful, 
 * Return FAIL on failure (may be retryable),
 * SCSI_FAIL if we failed due to timing problem with SCSI bus. (terminal)
 * RESEL_FAIL if we failed due to target being in process of reselecting us.
 * (posponed til after reconnect done)
 */
sw_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;		/* if 0, run cmd to completion
					 *       in polled mode
					 * if 1, allow disconnects
					 *       if enabled and use
					 *       interrupts
					 */
{
	register struct scsi_sw_reg *swr;
	register struct mb_device *md = un->un_md;
	register int err;
	register int i;
	register u_char size;
	u_char msg;
	int s;

	(int)swr = c->c_reg;

	/* disallow disconnects if waiting for command completion */
	DPRINTF("sw_cmd:\n"); 
	if (intr == 0) {
		c->c_flags &= ~SCSI_EN_DISCON;
	} else {
		/*
		 * If disconnect/reconnect globally disabled or only
		 * disabled for this command set internal flag.
		 * Otherwise, we enable disconnects and reconnects.
		 */
		if ((scsi_disre_enable == 0)  ||  (un->un_flags & SC_UNF_NO_DISCON))
			c->c_flags &= ~SCSI_EN_DISCON;
		else
			c->c_flags |= SCSI_EN_DISCON;
		un->un_wantint = 1;
	}

	/* Check for odd-byte boundary buffer */
	if ((un->un_flags & SC_UNF_DVMA)  &&  (un->un_dma_addr & 0x1)) {
		printf("sw%d:  illegal odd byte DMA, address= 0x%x\n",
			SWNUM(c), un->un_dma_curaddr);
		return (HARD_FAIL);
	}

	/*
	 * For vme host adaptor interface, dma enable bit may
	 * be set to allow reconnect interrupts to come in.
	 * This must be disabled before arbitration/selection
	 * of target is done. Don't worry about re-enabling
	 * dma. If arb/sel fails, then sw_idle will re-enable.
	 * If arb/sel succeeds then handling of command will
	 * re-enable.
	 *
	 * Also, disallow sbc to accept reconnect attempts.
	 * Again, sw_idle will re-enable this if arb/sel fails.
	 * If arb/sel succeeds then we do not want to allow
	 * reconnects anyway.
	 */
        s = splr(pritospl(c->c_intpri));
	if ((intr == 1) && (c->c_tab.b_active >= C_ACTIVE)) {
		md->md_utab.b_active |= 0x2;
		DPRINTF1("sw_cmd: md_utab = %d\n", md->md_utab.b_active);
		DPRINTF("sw_cmd:  currently locked out\n");
		(void) splx(s);
		return (OK);
	}
	c->c_tab.b_active |= C_ACTIVE;
	swr->csr &= ~SI_CSR_DMA_EN;

	SW_VME_OK(c, swr, "start of sw_cmd");
	SCSI_TRACE('c', swr, un);
	un->un_flags &= ~SC_UNF_DMA_INITIALIZED;

	/* performing target selection */
	if ((err = sw_arb_sel(c, swr, un)) != OK) {
		/* 
		 * May not be able to execute this command at this time due
		 * to a target reselecting us. Indicate this in the unit
		 * structure for when we perform this command later.
		 */
		SW_VME_OK(c, swr, "sw_cmd: arb_sel_fail");

		if (err == RESEL_FAIL) {
			md->md_utab.b_active |= MD_PREEMPT;
			DPRINTF1("sw_cmd:  preempted, tgt= %x\n",
				 un->un_target);
			err = OK;		/* not an error */

			/* Check for lost reselect interrupt */
		    	if ( !(SBC_RD.bsr & SBC_BSR_INTR)) {
		printf("sw_cmd:  hardware reselect failure, software recovered.\n");
				swintr(c);
				}
		} else {
			c->c_tab.b_active &= C_QUEUED;
		}
		un->un_wantint = 0;
		swr->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
		return (err);
	}

	/*
	 * We need to send out an identify message to target.
	 */
	SBC_WR.ser = 0;			/* clear (re)sel int */
	SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int */
	(void) splx(s);
	/*
	 * Must split dma setup into 2 parts due to sun3/50
	 * which requires bcr to be set before target
	 * changes phase on scsi bus to data phase.
	 *
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
		if (un->un_flags & SC_UNF_RECV_DATA) {
			un->un_dma_curdir = SI_RECV_DATA;
			swr->csr &= ~SI_CSR_SEND;
		} else {
			un->un_dma_curdir = SI_SEND_DATA;
			swr->csr |= SI_CSR_SEND;
		}
		/* save current dma info for disconnect.  Note,
		 * address tweak for cobra.
		 */
 		un->un_dma_curaddr = un->un_dma_addr | 0xf00000;
		un->un_dma_curcnt = un->un_dma_count;
		un->un_dma_curbcr = 0;

		/* DPRINTF1("csr after resetting fifo 0x%x\n", swr->csr); */
	        /* 
	         * Currently we don't use all 24 bits of the
	         * count register on the vme interface. To do
		 * this changes are required other places, e.g.
		 * in the scsi_unit structure the fields
		 * un_dma_curcnt and un_dma_count would need to
		 * be changed.
		 */
		swr->dma_count = 0;

		/*
		 * New: we set up everything we can here, rather
		 * than wait until data phase.
		 */
		SW_VME_OK(c, swr, "sw_cmd: before cmd phase");
		sw_dma_setup(c, un);

	} else {
		un->un_dma_curdir = SI_NO_DATA;
		un->un_dma_curaddr = 0;
		un->un_dma_curcnt = 0;
	}
	SW_VME_OK(c, swr, "sw_cmd: before cmd phase");
	SCSI_TRACE('C', swr, un);

RETRY_CMD_PHASE:
	if (sw_wait_phase(swr, PHASE_COMMAND) != OK) {
		/*
		 * Handle synchronous messages (6 bytes) and other
		 * unknown messages.  Note, all messages will be
		 * rejected.  It would be nice someday to figure
		 * out what to do with them; but not today.
		 */
		register u_char *icrp = &SBC_WR.icr;

		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_IN) {
			*icrp = SBC_ICR_ATN;
			msg = sw_getdata(c, PHASE_MSG_IN);
			EPRINTF1("sw_cmd:  rejecting msg 0x%x\n", msg);

			i = 255; /* accept 255 message bytes (overkill) */
			while((sw_getdata(c, PHASE_MSG_IN) != -1)  &&  --i);

			if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_OUT) {
				msg = SC_MSG_REJECT;
				*icrp = 0;		/* turn off ATN */
				(void) sw_putdata(c, PHASE_MSG_OUT, &msg, 1, 0);
			}
			/* Should never fail this check. */
			if (i > 0)
				goto RETRY_CMD_PHASE;
		}

		/* target is skipping cmd phase, resynchronize... */
		if (SBC_RD.cbsr & SBC_CBSR_REQ) {
#ifdef			SCSI_DEBUG
			printf("sw_cmd:  skipping cmd phase\n");
			sw_print_state(swr, c);
#endif			SCSI_DEBUG
        		s = splr(pritospl(c->c_intpri));
			goto SW_CMD_EXIT;
		 }

		/* we've had a target failure, report it and quit */
		printf("sw%d:  sw_cmd: no command phase\n", SWNUM(c));
		sw_reset(c, PRINT_MSG);
		c->c_tab.b_active &= C_QUEUED;
		return (FAIL);

	}

	/*
	 * Determine size of the cdb.  Since we're smart, we
	 * look at group code and guess.  If we don't
	 * recognize the group id, we use the specified
	 * cdb length.  If both are zero, use max. size
	 * of data structure.
	 */
	if ((size = cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) == 0  &&
	    (size = un->un_cmd_len) == 0) {
		printf("sw%d:  invalid cdb size, using size= %d\n",
			SWNUM(c), sizeof (struct scsi_cdb));
		size = sizeof (struct scsi_cdb);
	}
	c->c_last_phase = PHASE_COMMAND;
	s = splr(pritospl(3));
#ifdef notdef
	s = splr(pritospl(c->c_intpri));
#endif notdef
	SBC_WR.ser = scsi_host_id;	/* enable (re)sel int */
	if (sw_putdata(c, PHASE_COMMAND, (u_char *)&un->un_cdb, size, 
	    intr) != OK) {
		printf("sw%d:  sw_cmd: put cmd failed\n", SWNUM(c));
		sw_print_state(swr, c);
		sw_reset(c, PRINT_MSG);
		c->c_tab.b_active &= C_QUEUED;
		(void) splx(s);
		return (FAIL);
	}

	/* If not polled I/O mode, we're done */
SW_CMD_EXIT:
	if (intr)  {
		/* Check for lost phase change interrupt */
		if ((SBC_RD.cbsr & SBC_CBSR_REQ)  &&
	 	    !(SBC_RD.bsr & SBC_BSR_INTR)) {
				printf("sw_cmd:  interrupt failure\n");
				swintr(c);
		}
		swr->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
		return (OK);
	}
	(void) splx(s);

	/* 
	 * Polled SCSI data transfer mode.
	 */
	if (un->un_dma_curdir != SI_NO_DATA) {
		register u_char phase;

		if (un->un_dma_curdir == SI_RECV_DATA)
			phase = PHASE_DATA_IN;
		else
			phase = PHASE_DATA_OUT;

		SW_VME_OK(c, swr, "sw_cmd: before data xfer, sync");
		/*
		 * Check if we have a problem with the command
		 * not going into data phase.  If we do,
		 * then we'll skip down and get any status.
		 * Of course, this means that the command
		 * failed.
		 */
	       if ((sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SI_LONG_WAIT, 1) == OK)  &&
		   ((SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)) {
			/*
			 * Must actually start DMA xfer here - setup
			 * has already been done.  So, put sbc in dma
			 * mode and start dma transfer.
			 */
			sw_sbc_dma_setup(c, swr, (int)un->un_dma_curdir);

			/*
			 * Wait for DMA to finish.  If it fails,
			 * attempt to get status and report failure.
			 */
			if ((err=sw_cmdwait(c)) != OK) {
				EPRINTF("sw_cmd:  cmdwait failed\n");
				if (err != SCSI_FAIL)
					msg = sw_getstatus(un);
				c->c_tab.b_active &= C_QUEUED;
				return (err);
			}
		} else {
			EPRINTF("sw_cmd:  skipping data phase\n");
		}
	}

	/*
	 * Get completion status for polled command.
	 * Note, if <0, it's retryable; if 0, it's fatal.
	 * Someday I should give polled status results
	 * more options.  For now, everything is FATAL.
	 */
	c->c_last_phase = PHASE_CMD_CPLT;
	if ((err=sw_getstatus(un)) <= 0) {
		c->c_tab.b_active &= C_QUEUED;
		if (err == 0) 
			return (HARD_FAIL);
		else
			return (FAIL);
	}
	c->c_tab.b_active &= C_QUEUED;
	return (OK);
}


/*
 * Perform the SCSI arbitration and selection phases.
 * Returns FAIL if unsuccessful, 
 * returns RESEL_FAIL if unsuccessful due to target reselecting, 
 * returns OK if all was cool.
 */
static
sw_arb_sel(c, swr, un)
	register struct scsi_ctlr *c;
	register struct scsi_sw_reg *swr;
	register struct scsi_unit *un;
{
	register u_char *icrp = &SBC_WR.icr;
	register u_char *mrp = &SBC_WR.mr;
	register u_char icr;
	u_char id;
	int ret_val;
	int s;
	int j;
	int sel_retries = 0;

	DPRINTF("sw_arb_sel:\n");
	SW_VME_OK(c, swr, "sw_arb_sel");

	/* 
	 * It seems that the tcr must be 0 for arbitration to work.
	 */
	SBC_WR.tcr = 0;
	*mrp &= ~SBC_MR_ARB;	/* turn off arb */
	*icrp = 0;
	SBC_WR.odr = scsi_host_id;

#ifdef	SCSI_DEBUG
	if (scsi_debug > 2)
		sw_print_state(swr, c);
#endif	SCSI_DEBUG

	/* arbitrate for the scsi bus */
SW_RETRY_SEL:
	for (j = 0; j < SI_ARB_RETRIES; j++) {

	/* wait for scsi bus to become free */
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SI_WAIT_COUNT, 0) != OK) {
		printf("sw%d:  sw_arb_sel: scsi bus continuously busy\n", SWNUM(c));
		sw_reset(c, PRINT_MSG);
		ret_val = FAIL;		/* may be retryable */
		goto SW_ARB_SEL_EXIT;
	}

		/* wait for sbc to begin arbitration */
		*mrp |= SBC_MR_ARB;		/* turn on arb */
		if (sw_sbc_wait((caddr_t)icrp, SBC_ICR_AIP, SI_ARB_WAIT, 1)
		    != OK) {
			/*
			 * sbc may never begin arbitration due to a
			 * target reselecting us, the initiator.
			 */
			*mrp &= ~SBC_MR_ARB;	/* turn off arb */
			if (((SBC_RD.cbsr & SBC_CBSR_RESEL) == SBC_CBSR_RESEL) && 
			    (SBC_RD.cdr & scsi_host_id)) {
				DPRINTF("sw_arb_sel:  recon1\n");
				ret_val = RESEL_FAIL;
				goto SW_ARB_SEL_EXIT;
			}
			EPRINTF1("sw_arb_sel:  AIP never set, cbsr= 0x%x\n",
				 SBC_RD.cbsr);
#ifdef			SCSI_DEBUG
			sw_print_state(swr, c);
#endif			SCSI_DEBUG
			goto SW_ARB_SEL_RETRY;
		}

		/* check to see if we won arbitration */
		s = splr(pritospl(7));  /* time critical */
		DELAY(sw_arbitration_delay);
		if ( (*icrp & SBC_ICR_LA) == 0  && 
		    	((SBC_RD.cdr & ~scsi_host_id) < scsi_host_id) ) {

			/*
			 * WON ARBITRATION!  Perform selection.
			 * If disconnect/reconnect enabled, set ATN.
			 * If not, skip ATN so target won't do disconnects.
			 */
			/* DPRINTF("sw_arb_sel:  won arb\n"); */
			icr = SBC_ICR_SEL | SBC_ICR_BUSY | SBC_ICR_DATA;
			if (c->c_flags & SCSI_EN_DISCON) {
				icr |= SBC_ICR_ATN;
			}
			SBC_WR.odr = (1 << un->un_target) | scsi_host_id;
			*icrp = icr;
			*mrp &= ~SBC_MR_ARB;	/* turn off arb */
			DELAY(sw_bus_clear_delay + sw_bus_settle_delay);
			goto SW_ARB_SEL_WON;
		}
		(void) splx(s);

SW_ARB_SEL_RETRY:
		/* Lost arb, try again */
		*mrp &= ~SBC_MR_ARB;	/* turn off arb */
		if (((SBC_RD.cbsr & SBC_CBSR_RESEL)  == SBC_CBSR_RESEL) && 
		    (SBC_RD.cdr & scsi_host_id)) {
			DPRINTF("sw_arb_sel:  recon2\n");
			ret_val = RESEL_FAIL;
			goto SW_ARB_SEL_EXIT;
		}
		EPRINTF("sw_arb_sel:  lost arbitration\n");
	}
	/*
	 * FAILED ARBITRATION even with retries.
	 * This shouldn't ever happen since
	 * we have the highest priority id on the scsi bus.
	 */
	*icrp = 0;
	printf("sw%d:  sw_arb_sel: never won arbitration\n", SWNUM(c));
	sw_print_state(swr, c);
	sw_reset(c, PRINT_MSG);
	ret_val = FAIL;         /* may be retryable */

SW_ARB_SEL_EXIT:
	return (ret_val);


SW_ARB_SEL_WON:
	/* wait for target to acknowledge selection */
	*icrp &= ~SBC_ICR_BUSY;
	(void) splx(s);
	DELAY(1);
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_BSY,
	    SI_SHORT_WAIT, 1) != OK) {
		EPRINTF("sw_arb_sel:  busy never set\n");
#ifdef		SCSI_DEBUG
		sw_print_state(swr, c);
#endif		SCSI_DEBUG

		*icrp = 0;
		/* Target failed selection */
		if (un->un_present  &&  (++sel_retries < SI_SEL_RETRIES))
			goto SW_RETRY_SEL;
		ret_val = HARD_FAIL;
		goto SW_ARB_SEL_EXIT;
	}
	/* DPRINTF("sw_arb_sel:  selected\n"); */
	*icrp &= ~(SBC_ICR_SEL | SBC_ICR_DATA);
	c->c_last_phase = PHASE_ARBITRATION;

	/* If disconnects enabled, tell target it's ok to do it. */
	if (c->c_flags & SCSI_EN_DISCON) {

		/* Tell target we do disconnects */
		/* DPRINTF("sw_arb_sel:  disconnects ENABLED\n"); */
		id = SC_DR_IDENTIFY | un->un_cdb.lun;
		SBC_WR.tcr = TCR_MSG_OUT;
 		if (sw_wait_phase(swr, PHASE_MSG_OUT) == OK) {
			*icrp = 0;		/* turn off ATN */
			if (sw_putdata(c, PHASE_MSG_OUT, &id, 1, 0) != OK) {
				EPRINTF1("sw%d:  sw_arb_sel: id msg failed\n",
					SWNUM(c));
               			sw_reset(c, PRINT_MSG);
               		        return (FAIL);
			}
		}
	}
	return (OK);
}


/*
 * Set up the SCSI control logic for a dma transfer for vme host adaptor.
 */
static
sw_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_sw_reg *swr;
	(int)swr = c->c_reg;

	SW_VME_OK(c, swr, "sw_dma_setup");
	SCSI_RECON_TRACE('v', c, un->un_dma_curaddr, (int)un->un_dma_curcnt,
				(int)un->un_dma_curbcr);

	/* DPRINTF2("sw_dma_setup:  after fifo reset, csr 0x%x, bcr 0x%x\n",
	 *	swr->csr, swr->bcr);
	 */

	/* Check for odd-byte boundary buffer */
	/* NEED BETTER STRATEGY!! */
	if ((u_int)(un->un_dma_curaddr) & 0x1) {
		printf("sw%d:  illegal odd byte DMA address= 0x%x\n",
			SWNUM(c), un->un_dma_curaddr);
		/* sw_reset(c, PRINT_MSG); */
		/* return; */
	}

	/*
	 * setup starting dma address and number bytes to dma
	 * Note, the dma count is set to zero to prevent it from
	 * starting up.  It will be set later in sw_sbc_dma_setup
	 */
	swr->dma_addr = un->un_dma_curaddr;
	swr->dma_count = 0;

        /* set up byte packing control info */
	if (swr->dma_addr & 0x2) {
		if (un->un_dma_curdir == SI_RECV_DATA) {
			EPRINTF2("sw_dma_setup:  word transfer --  %X %X\n",
				 un->un_dma_curaddr, un->un_dma_count);
			un->un_flags |= SC_UNF_WORD_XFER;
		}
	}

	/*
	 * junk = GET_DMA_ADDR(swr);
	 * DPRINTF1("sw_dma_setup:  addr= 0x%x", junk);
	 * junk = GET_DMA_COUNT(swr);
	 * DPRINTF1("  count= %d ", junk);
	 * DPRINTF2("csr= 0x%x  bcr= 0x%x\n", swr->csr, swr->bcr);
	 */
}

/*
 * Setup and start the sbc for a dma operation.
 */
static
sw_sbc_dma_setup(c, swr, dir)
	register struct scsi_ctlr *c;
	register struct scsi_sw_reg *swr;
	register int dir;
{
	register struct scsi_unit *un = c->c_un;
	register int s;


	SCSI_TRACE('G', swr, un);
	SW_VME_OK(c, swr, "sw_sbc_dma_setup");
	un->un_flags |= SC_UNF_DMA_INITIALIZED;

	if (un->un_flags & SC_UNF_WORD_XFER) {
		char *cp;

		/* 
		 * HARDWARE BUG: writing to swr->dma_addr works fine
		 * but the read done here causes the most significant
		 * byte to be lost, so it must be or'ed back in.
		 * This bug was lots of fun to trace down.
		 */
		cp = (char *)swr->dma_addr;
		cp = (char *)((int)cp | 0xff000000);
		*cp++ = sw_getdata(c, PHASE_DATA_IN);
		*cp++ = sw_getdata(c, PHASE_DATA_IN);
		swr->dma_addr = (int)cp;
	}
	un->un_flags |= SC_UNF_DMA_INITIALIZED;
	if (un->un_flags & SC_UNF_WORD_XFER) {
		swr->dma_count = un->un_dma_curcnt - 2;
		un->un_flags &= ~SC_UNF_WORD_XFER;
	} else {
		swr->dma_count = un->un_dma_curcnt;
	}

	if (dir == SI_RECV_DATA) {
		/* DPRINTF("sw_sbc_dma_setup:  RECEIVE DMA\n"); */
		c->c_last_phase = PHASE_DATA_IN;
		s = splr(pritospl(7));  /* time critical */
		SBC_WR.tcr = TCR_DATA_IN;
		junk = SBC_RD.clr;      /* clear intr */

		/* CRITICAL CODE SECTION DON'T TOUCH */
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.ircv = 0;
		swr->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
	} else {
		/* DPRINTF("sw_sbc_dma_setup:  XMIT DMA\n"); */
		c->c_last_phase = PHASE_DATA_OUT;
		s = splr(pritospl(7));  /* time critical */
		SBC_WR.tcr = TCR_DATA_OUT;
		junk = SBC_RD.clr;      /* clear intr */
		SBC_WR.icr = SBC_ICR_DATA;

		/* CRITICAL CODE SECTION DON'T TOUCH */
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.send = 0;
		swr->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
	}
}


/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */
static
sw_dma_cleanup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_sw_reg *swr;
	(int)swr = c->c_reg;

	/* DPRINTF("sw_dma_cleanup:\n"); */

	/* disable dma controller */
	swr->dma_addr = 0;
	swr->dma_count = 0;
}

/*
 * Handle special dma receive situations, e.g. an odd number of bytes 
 * in a dma transfer.
 * The Sun3/50 onboard interface has different situations which
 * must be handled than the vme interface.
 * Returns OK if sucessful; Otherwise FAIL.
 */
static
sw_dma_recv(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_sw_reg *swr;
	register struct scsi_unit *un = c->c_un;
	register int offset, addr;
	(int)swr = c->c_reg;

#ifdef	lint
	un = un;
#endif	lint

	/* DPRINTF("sw_dma_recv:\n"); */
	SCSI_RECON_TRACE('R', c, un->un_dma_curaddr, (int)un->un_dma_curcnt,
			 offset);
	SCSI_TRACE('u', swr, un);
    	    /*
	     * Grabs last few bytes which may not have been dma'd.
	     * Worst case is when longword dma transfers are being done
	     * and there are 3 bytes leftover.
	     * If BPCON bit is set then longword dmas were being done,
	     * otherwise word dmas were being done.
	     */
	addr = swr->dma_addr;
	offset = (addr - (int)DVMA) & 0xFFFFFC;
	switch (swr->dma_addr & 0x3) {
	case 3:			/* three bytes left */
	   if (MBI_MR(offset) < dvmasize) {
		DVMA[offset] = (swr->bpr & 0xff000000) >> 24;
		DVMA[offset + 1] = (swr->bpr & 0x00ff0000) >> 16;
		DVMA[offset + 2] = (swr->bpr & 0x0000ff00) >> 8;
	   }
           else {
		sw_flush_vmebyte(c, (offset), ((swr->bpr & 0xff000000) >> 24));
		sw_flush_vmebyte(c, (offset + 1), 
			((swr->bpr & 0x00ff0000) >> 16));
		sw_flush_vmebyte(c, (offset + 2), 
			((swr->bpr & 0x0000ff00) >> 8));
	   }
	   break;
	case 2:			/* two bytes left */
	   if (MBI_MR(offset) < dvmasize) {
		DVMA[offset] = (swr->bpr & 0xff000000) >> 24;
		DVMA[offset + 1] = (swr->bpr & 0x00ff0000) >> 16;
	   }
           else {
		sw_flush_vmebyte(c, (offset), ((swr->bpr & 0xff000000) >> 24));
		sw_flush_vmebyte(c, (offset + 1), 
			((swr->bpr & 0x00ff0000) >> 16));
	   }
	   break;
	case 1:			/* one byte left */
	   if (MBI_MR(offset) < dvmasize) {
		DVMA[offset] = (swr->bpr & 0xff000000) >> 24;
	   }
           else {
		sw_flush_vmebyte(c, (offset), ((swr->bpr & 0xff000000) >> 24));
	   }
	   break;
	}
	return (OK);

}

sw_flush_vmebyte(c, offset, byte)
struct scsi_ctlr *c;
int offset;
u_char byte;
{
        u_char *mapaddr;
        u_int pv;
 
#ifdef  sun3x
        pv = btop (VME24D16_BASE + (offset & VME24D16_MASK));
#else   sun3x
        pv = PGT_VME_D16 | btop(VME24_BASE | (offset & VME24_MASK));
#endif  sun3x
        mapaddr = (u_char *) ((u_long) kmxtob(c->c_kment) |
                              (u_long) MBI_OFFSET(offset));
        segkmem_mapin(&kseg, (addr_t) (((int)mapaddr) & PAGEMASK),
                (u_int) mmu_ptob(1), PROT_READ | PROT_WRITE, pv, 0);
        *mapaddr = byte;
        DPRINTF2("sw_flush_vmebyte: mapaddr = %x, byte= %x\n", mapaddr, byte);
        segkmem_mapout(&kseg,
            (addr_t) (((int)mapaddr) & PAGEMASK), (u_int) mmu_ptob(1));
}

/*
 * Handle a scsi interrupt.
 */
swintr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_sw_reg *swr;
	register struct scsi_unit *un;
	register u_char *cp;
	register int resid;
	int status;
	short lun;
	u_short bcr;	/* get it for discon stuff BEFORE we clr int */
	u_char msg;

	(int)swr = c->c_reg;

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or the 
	 * SI_CSR_DMA_CONFLICT bit in the csr will be set.
	 */
	/* DPRINTF("swintr:\n"); */ 
HEAD_SWINTR:
	un = c->c_un;

	if (swr->csr & SI_CSR_DMA_CONFLICT)
		printf("swintr: head CSR %X\n", swr->csr);
	swr->csr &= ~SI_CSR_DMA_EN;
	SW_VME_OK(c, swr, "top of swintr");

	/*
	 * We need to store the contents of the byte count register
	 * before we change the state of the 5380.  The 5380 has
	 * a habit of prefetching data before it knows whether it
	 * needs it or not, and this can throw off the bcr.
	 * Note: Cobra does not have a bcr so we must figure
	 * this out from the dma_addr register.  Also un could be zero
	 * coming into the interrupt routine so we need to check it
	 * and not use it if it is (will this cause problems later?)
	 */
	resid = 0;
	bcr = 0;
	SCSI_TRACE('i', swr, un);

	if (un != NULL) {	/* prepare for a disconnect */
		if (un->un_dma_curdir != SI_NO_DATA) {
			bcr = un->un_dma_curcnt - 
			      (swr->dma_addr - un->un_dma_curaddr);
			DPRINTF2("swintr:  bcr= %X   tcr= %X  ",
				 bcr, SBC_RD.tcr);
			DPRINTF2("dma addr= %X   cur addr= %X  ",
				 swr->dma_addr, un->un_dma_curaddr);
			DPRINTF1("count= %X\n", un->un_dma_curcnt );

			if ((swr->dma_addr - un->un_dma_curaddr) && 
			       !(SBC_RD.tcr & SBC_TCR_LAST) && 
			       (un->un_dma_curdir == SI_SEND_DATA))
				bcr++;
		}
	}
	
	/*
	 * Determine source of interrupt.
	 */
	if (swr->csr & (SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR)) {
		/*
		 * DMA related error.
		 */
		if (swr->csr & SI_CSR_DMA_BUS_ERR) {
			printf("sw%d:  swintr: bus error during dma\n",
				SWNUM(c));
			/* goto RESET_AND_LEAVE; */

		} else if (swr->csr & SI_CSR_DMA_CONFLICT) {
			printf("sw%d:  swintr: invalid reg access during dma\n",
				SWNUM(c));
		} else {
				/*
				 * I really think that this is also DMA
				 * in progress.
				 */
				printf("sw%d:  swintr: dma overrun\n", SWNUM(c));
		}
		junk = SBC_RD.clr;	/* clear int, if any */
		sw_print_state(swr, c);

		/*
		 * Either we were waiting for an interrupt on a phase change 
		 * on the scsi bus, an interrupt on a reconnect attempt,
		 * or an interrupt upon completion of a real dma operation.
		 * Each of these situations must be handled appropriately.
		 */
		if (un == NULL) {
			/* Spurious reconnect. */
			printf("sw%d:  swintr: illegal reconnection for DMA\n",
				SWNUM(c));
			goto RESET_AND_LEAVE;

		} else if (un->un_flags & SC_UNF_DMA_ACTIVE) {
			/*
			 * Unit was DMAing, must clean up.
			 * This is a bit tricky.  The bcr is set to
			 * zero for the SCSI-3 host adaptor until we
			 * actually go into data phase.  If we run
			 * a data xfer command, but never go into
			 * data phase, the bcr will be zero.  We must
			 * guard against interpeting this as meaning
			 * that we have transferred all our data -
			 * in this instance, we have really transferred none.
			 * SC_UNF_DMA_INITIALIZED tells us (for SCSI-3)
			 * whether we have loaded the bcr.
			 */
			if (un->un_flags & SC_UNF_DMA_INITIALIZED) {
				resid = un->un_dma_curcnt = bcr;
			} else {
				resid = un->un_dma_curcnt;
			}
			sw_dma_cleanup(c);
			un->un_flags &= ~SC_UNF_DMA_ACTIVE;
		}
		/*
		 * We have probably blew it on the DMA and should reset.
		 * But, let's pospone it until things really screw-up.
		 */
		goto HANDLE_SPURIOUS_AND_LEAVE;
	}

	/*
	 * We have an SBC interrupt due to a phase change on the bus
	 * or a reconnection attempt.  First, check for reconnect
	 * attempt.
	 */
	if (((SBC_RD.cbsr & SBC_CBSR_RESEL)  == SBC_CBSR_RESEL) && 
	    (SBC_RD.cdr & scsi_host_id)) {
		register u_char cdr;
		register int i;

HANDLE_RECONNECT:
		/* get reselecting target scsi id */
		/* CRITICAL CODE SECTION DON'T TOUCH */
		junk = SBC_RD.clr;		 /* clear int */
		/*SBC_WR.ser = 0;			/* clear (re)sel int */
		cdr = SBC_RD.cdr & ~scsi_host_id;

		/* make sure there are only 2 scsi id's set */
		DPRINTF1("swintr:  reconnecting, cdr 0x%x\n", cdr); 
		for (i=0; i < 8; i++) {
			if (cdr & (1<<i))
				break;
		}
		/* CRITICAL CODE SECTION DON'T TOUCH */
		SBC_WR.ser = 0;			/* clear (re)sel int */
		cdr &= ~(1<<i);
		if (cdr != 0) {
			printf("sw%d:  swintr: >2 scsi reselection ids, cdr 0x%x\n",
				SWNUM(c), SBC_RD.cdr);
			SBC_WR.ser = scsi_host_id; /* enable (re)sel int */
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
		}

		/* acknowledge reselection */
#ifdef		SCSI_DEBUG
		if (scsi_debug > 2) {
			printf("swintr:  before resel ack\n");
			sw_print_state(swr, c);
		}
#endif		SCSI_DEBUG
		SBC_WR.icr |= SBC_ICR_BUSY;
		c->c_recon_target = i;		/* save for reconnect */
#ifdef		SCSI_DEBUG
		if (scsi_debug > 2) {
			printf("swintr:  after resel ack\n");
			sw_print_state(swr, c);
		}
#endif		SCSI_DEBUG

		/*
		 * If reselection ok, target should drop select.
		 * Otherwise, we took too long.  It would be nice
		 * to wait for next resel attempt...someday.
		 */
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_SEL,
		    SI_WAIT_COUNT, 0) != OK) {
			printf("sw%d:  swintr: SEL not released\n",
				SWNUM(c));
			SBC_WR.icr &= ~SBC_ICR_BUSY;
			SBC_WR.ser = scsi_host_id; /* enable (re)sel int */
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
		}
		SBC_WR.icr &= ~SBC_ICR_BUSY;
#ifdef		SCSI_DEBUG
		if (scsi_debug > 2) {
			printf("swintr:  reconnecting\n");
			sw_print_state(swr, c);
		}
#endif		SCSI_DEBUG

		/* After reselection, target should go into MSG_IN phase. */
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SI_WAIT_COUNT, 1) != OK) {
			printf("sw%d:  swintr: no MSG_IN req\n", SWNUM(c));
			SBC_WR.ser = scsi_host_id; /* enable (re)sel int */
			/* goto RESET_AND_LEAVE; */
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
		}
		c->c_last_phase = PHASE_RECONNECT;
		SBC_WR.ser = 0;			/* clear (re)sel int */
		SBC_WR.ser = scsi_host_id;	/* enable (re)sel int */
		/* FALL THROUGH INTO SYNCHRONIZE_PHASE */
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;


SYNCHRONIZE_PHASE:
	/*
	 * We know that we have a new phase we have to handle.
	 */
	DPRINTF("swintr: synch\n");
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, SI_PHASE_WAIT, 1)
	    != OK) {
		/* DPRINTF("swintr:  phase timeout\n"); */
		goto HANDLE_SPURIOUS_AND_LEAVE;
	}
	SW_WIN;
	junk = SBC_RD.clr;		 /* clear any pending sbc interrupt */
	un = c->c_un;
	switch (SBC_RD.cbsr & CBSR_PHASE_BITS) {

	case PHASE_DATA_IN:
	case PHASE_DATA_OUT:
		DPRINTF("  DATA\n");

#ifdef		SCSI_DEBUG
		if (scsi_debug > 2)
			sw_print_state(swr, c);
#endif		SCSI_DEBUG
		SW_VME_OK(c, swr, "swintr: data_out");
		SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int */

		if (un == NULL) {
			printf("sw%d:  swintr: no unit expecting DATA phase\n",
				SWNUM(c));
			goto RESET_AND_LEAVE;
		}

		if (un->un_dma_curcnt == 0  ||  un->un_dma_curdir == SI_NO_DATA) {
			printf("sw%d:  swintr: unexpected DATA phase, curcnt %d, curdir 0x%x\n",
				SWNUM(c), un->un_dma_curcnt, un->un_dma_curdir);
			goto RESET_AND_LEAVE;
		}

		/* data is expected, start dma data transfer and exit */
		sw_sbc_dma_setup(c, swr, (int)un->un_dma_curdir);
		goto LEAVE;


	case PHASE_MSG_IN:
		DPRINTF("  MSG");	
#ifdef		SCSI_DEBUG
		if (scsi_debug > 2)
			sw_print_state(swr, c);
#endif		SCSI_DEBUG
		SW_VME_OK(c, swr, "swintr: msg_in");

		msg = SBC_RD.cdr;		/* peek at message */
		lun = msg & 0x07;
		msg &= 0xf0;			/* mask off unit number */

		/* DPRINTF2("swintr:  msg 0x%x, lun %d\n", msg, lun); */
		if ((msg == SC_IDENTIFY)  ||  (msg == SC_DR_IDENTIFY)) {
			/*
			 * If we have a reconnect, we want to do our
			 * DMA setup before we go into DATA phase.
			 * This is why we peek at the message via the cdr
			 * rather than doing an sw_getdata from the start.
			 * sw_reconnect acknowledges the reconnect message.
			 */
			/* EPRINTF("swintr:  msg= identify\n"); */
			if (sw_reconnect(c, c->c_recon_target, lun, 0) != OK){
				printf("sw$d:  swintr:  reconnection failure\n",
					SWNUM(c));
				goto RESET_AND_LEAVE;
			}
			DPRINTF("swintr: after return from sw_reconnect\n");
			un = c->c_un;
			if ( un->un_dma_curdir == SI_SEND_DATA) {
				DPRINTF2("cnt= %d, dma= %d  ",
					un->un_dma_curcnt, un->un_dma_count);
				DPRINTF2("tgt= %d, lun= %d\n",
					un->un_target, un->un_lun);
			}
			/* DMA is setup, but NOT running */
			un->un_flags &= ~SC_UNF_DMA_INITIALIZED;
			c->c_last_phase = PHASE_RECONNECT;
			goto SYNCHRONIZE_PHASE;
		}

		if (SBC_RD.cdr == SC_DISCONNECT) {

			if (un->un_dma_curdir == SI_RECV_DATA) {
				un->un_dma_curbcr = bcr;

				SCSI_RECON_TRACE('a', c, (int)un->un_dma_curaddr, 
				(int)un->un_dma_curbcr, (int)swr->bcr);
			}
		}

		c->c_last_phase = PHASE_MSG_IN;
		msg = sw_getdata(c, PHASE_MSG_IN);
		switch (msg) {

		case SC_COMMAND_COMPLETE:
			DPRINTF("swintr:  msg= command complete\n");
			c->c_last_phase = PHASE_CMD_CPLT;
			cp = (u_char *) &un->un_scb;
			if (cp[0] & SCB_STATUS_MASK)
				status = SE_RETRYABLE;
			else
				status = SE_NO_ERROR;
			goto HAND_OFF_INTR;

		case SC_DISCONNECT:
			DPRINTF("swintr:  msg= disconnect\n");
			c->c_last_phase = PHASE_DISCONNECT;
			if ((un->un_flags & SC_UNF_DMA_INITIALIZED) == 0  &&
			    (un->un_dma_curdir == SI_SEND_DATA) &&
			    (un->un_dma_curcnt != un->un_dma_count)) {
				EPRINTF("swintr:  Warning, write disconnect w/o DMA\n");
				EPRINTF2("\tcnt= %d (%d)  ",
					 un->un_dma_curcnt, un->un_dma_count);
				EPRINTF2("bcr= %d  tgt= %d  ",
					 un->un_dma_curbcr, un->un_target);
				EPRINTF1("lun= %d  cdb =", un->un_lun);
#ifdef				SCSI_DEBUG
				if (scsi_debug)
					sw_print_cdb(un);
#endif				SCSI_DEBUG
			}
			if (sw_disconnect(c) != OK) {
				printf("sw%d:  swintr: disconnect failure\n",
					SWNUM(c));
				goto RESET_AND_LEAVE;
			}
			goto START_NEXT_COMMAND_AND_LEAVE;

		case SC_RESTORE_PTRS:
			/* these messages are noise  - ignore them */
			DPRINTF("swintr:  msg= restore pointer\n");
			c->c_last_phase = PHASE_RESTORE_PTR;
			goto SYNCHRONIZE_PHASE;

		case SC_SAVE_DATA_PTR:
			/* save the bcr before the bastard pre-fetches again */
			DPRINTF("swintr:  msg= save pointer\n");
			un->un_dma_curbcr = bcr;
			c->c_last_phase = PHASE_SAVE_PTR;
			goto SYNCHRONIZE_PHASE;

		case SC_SYNCHRONOUS:
			EPRINTF("swintr:  msg= extended\n");

			if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_IN) {
				SBC_WR.icr = SBC_ICR_ATN;

				/* accept 255 message bytes (overkill) */
				msg = 255;
				while((sw_getdata(c, PHASE_MSG_IN) !=  0xff)
				      &&  --msg);
				if ((SBC_RD.cbsr & CBSR_PHASE_BITS)
				     == PHASE_MSG_OUT) {
					msg = SC_MSG_REJECT;
					SBC_WR.icr = 0;	/* turn off ATN */
					(void) sw_putdata(c, PHASE_MSG_OUT,
					       &msg, 1, 0);
				}
			}
			goto SYNCHRONIZE_PHASE;

		case SC_NO_OP:
			EPRINTF("swintr:  msg= no op\n");
			goto SYNCHRONIZE_PHASE;

		case SC_PARITY:
			EPRINTF("swintr:  msg= parity error\n");
			goto SYNCHRONIZE_PHASE;

		case SC_ABORT:
			EPRINTF("swintr:  msg= abort\n");
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;

		case SC_DEVICE_RESET:
			EPRINTF("swintr:  msg= reset device\n");
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;

		default:
#ifdef		SCSI_DEBUG
			printf("sw%d:  swintr: ignoring unknown message= 0x%x\n",
				SWNUM(c), msg);
			sw_print_state(swr, c);
#endif		SCSI_DEBUG
			goto HANDLE_SPURIOUS_AND_LEAVE;
		}


	case PHASE_STATUS:
		DPRINTF("  STATUS");

#ifdef		SCSI_DEBUG
		if (scsi_debug > 2)
			sw_print_state(swr, c);
#endif		SCSI_DEBUG
		SW_VME_OK(c, swr, "swintr: status");
		if ((un->un_dma_curdir == SI_RECV_DATA)  &&
		    (sw_dma_recv(c) != OK)) {
			/* DMA failure, time to reset SCSI bus */
			printf("sw%d:  swintr: DMA failure\n", SWNUM(c));
			goto RESET_AND_LEAVE;
		}

		/* DPRINTF("swintr:  getting status bytes\n"); */
		cp = (u_char *) &un->un_scb;
		cp[0] = sw_getdata(c, PHASE_STATUS);
		if (cp[0] == 0xff) {
			/* status failure, time to reset SCSI bus */
			printf("sw%d:  swintr: no status\n", SWNUM(c));
			goto RESET_AND_LEAVE;
		}
		SCSI_TRACE('s', swr, un);
		c->c_last_phase = PHASE_STATUS;
		goto SYNCHRONIZE_PHASE;


	default:
		printf("sw%d:  swintr: ignoring spurious phase\n", SWNUM(c));
		sw_print_state(swr, c);
		goto HANDLE_SPURIOUS_AND_LEAVE;
}


HAND_OFF_INTR:
	DPRINTF("hand_off_intr:\n"); 
	SW_VME_OK(c, swr, "swintr: hand off intr");

	c->c_tab.b_active &= 0x1;	/* Release que interlock */
	swr->csr &= ~SI_CSR_DMA_EN;

	/* pass interrupt info to unit */
	if (un  &&  un->un_wantint) {
		un->un_wantint = 0;

		SCSI_RECON_TRACE('f', c, 0, 0, 0);
		if (un->un_dma_curdir != SI_NO_DATA) {
			if (bcr == 0xffff)	/* fix pre-fetch botch */
				bcr = 0;
			/*
			 * This is a bit tricky.  The bcr is set to
			 * zero for the SCSI-3 host adaptor until we
			 * actually go into data phase.  If we run
			 * a data xfer command, but never go into
			 * data phase, the bcr will be zero.  We must
			 * guard against interpeting this as meaning
			 * that we have transferred all our data -
			 * in this instance, we didn't really transfer any.
			 * SC_UNF_DMA_INITIALIZED is used to handle
			 * the problem.
			 */
			if (un->un_flags & SC_UNF_DMA_INITIALIZED) {
				/* bcr is valid */
				resid = un->un_dma_curcnt = bcr;
			} else {
				/* bcr is invalid */
				resid = un->un_dma_curcnt;
			}
		}
#ifdef		SCSI_DEBUG
		if ((scsi_debug > 2)  &&  resid) {
			printf("swintr:  residue error\n");
			sw_print_state(swr, c);
		}
#endif		SCSI_DEBUG


		/* call high-level scsi device interrupt handler to finish */
		(*un->un_ss->ss_intr)(c, resid, status);
		un = c->c_un;
		swr->csr &= ~SI_CSR_DMA_EN;
	}

	/* fall through to start_next_command_and_leave */


START_NEXT_COMMAND_AND_LEAVE:

	/* start next I/O activity on controller */
	DPRINTF("swintr:  start_next_command\n");
	if (IS_VME(c))
		swr->csr &= ~SI_CSR_DMA_EN;
	SW_VME_OK(c, swr, "swintr: start next command");
	/*
	 * Check if we have a reconnect pending already.
	 * This happens with some stupid SCSI controllers
	 * which automatically disconnect, even when
	 * they don't have to. Rather than field another
	 * interrupt, let's go handle it.
	 */

	if (un == NULL)
		goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;

	if ((un->un_c->c_tab.b_actf) && (un->un_c->c_tab.b_active == 0)) {
		msg = SBC_RD.cbsr;
		if (((msg & SBC_CBSR_RESEL)  == SBC_CBSR_RESEL) && 
		    (SBC_RD.cdr & scsi_host_id)) {
			DPRINTF1("swintr:  recon1, cbsr= 0x%x\n", msg);
			goto HANDLE_RECONNECT;
		}
	}

	swstart(un);
	/*
	 * Either we've started a command or there weren't any to start.
	 * In any case, we're done...
	 */
	goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;


RESET_AND_LEAVE:
	EPRINTF1("swintr:  reset_and_leave: RESET to state 0x%x\n",sw_stbi);
	sw_reset(c, PRINT_MSG);
	goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;


HANDLE_SPURIOUS_AND_LEAVE:
	/* FALL THROUGH */


SET_UP_FOR_NEXT_INTR_AND_LEAVE:
	DPRINTF("swintr:  set_up_for_next_intr\n");
	swr->csr &= ~SI_CSR_DMA_EN;
	junk = SBC_RD.clr;		/* clear int */
	SW_VME_OK(c, swr, "swintr: setup for next intr");

	/*
	 * For depending on the last phase, we need to
	 * check either for target reselection or a
	 * SCSI bus phase change.
	 */
	if (c->c_last_phase == PHASE_CMD_CPLT  ||
	    c->c_last_phase == PHASE_DISCONNECT) {

		/* Check for reselection.  */
		msg = SBC_RD.cbsr;
		if (((msg & SBC_CBSR_RESEL)  == SBC_CBSR_RESEL) && 
		    (SBC_RD.cdr & scsi_host_id)) {
			SW_WIN;
			/* DPRINTF("swintr:  recon2\n"); */
			goto HEAD_SWINTR;
		} else {
			SW_LOSE;
			/* EPRINTF("  lose2"); */
		}
	} else {
		/* Check for phase change.  */
		SBC_WR.tcr = TCR_UNSPECIFIED;
		SBC_WR.mr |= SBC_MR_DMA;
		junk = SBC_RD.clr;		/* clear int */
		msg = SBC_RD.cbsr;
		if (msg & SBC_CBSR_REQ) {
			SW_WIN;
			/* EPRINTF("swintr:  recon3\n"); */
			goto HEAD_SWINTR;
		} else {
			SW_LOSE;
			/* EPRINTF("  lose3"); */
		}
	}
        /* Enable interrupts and DMA. */
	swr->csr |= SI_CSR_DMA_EN;

LEAVE:
	DPRINTF("\n");
	return;
}


/*
 * Handle target disconnecting.
 * Returns true if all was OK, false otherwise.
 */
static
sw_disconnect(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_sw_reg *swr = (struct scsi_sw_reg *)c->c_reg;
	register u_short bcr;

	bcr = un->un_dma_curbcr;

	/* DPRINTF("sw_disconnect:\n"); */
	SCSI_RECON_TRACE('d', c, un->un_dma_curaddr, (int) un->un_dma_curbcr,
				(int) bcr);

	/*
	 * If command doen't require dma, don't save dma info.
	 * for reconnect.  If it does, but data phase was missing,
	 * don't update dma info.
	 */
	if ((un->un_dma_curdir != SI_NO_DATA)  &&
	    (un->un_flags & SC_UNF_DMA_INITIALIZED)) {

		if ((un->un_dma_curdir == SI_RECV_DATA)  &&
		    (sw_dma_recv(c) != OK)) {
			printf("sw%d:  sw_disconnect: DMA bogosity\n",SWNUM(c));
			return (FAIL);
		}

		/*
		 * Save dma information so dma can be restarted when
		 * a reconnect occurs.
		 */
		un->un_dma_curaddr += un->un_dma_curcnt - bcr;
		un->un_dma_curcnt = bcr;
		SCSI_RECON_TRACE('q', c, un->un_dma_curaddr, 
					(int)un->un_dma_curcnt, (int)bcr); 
#ifdef		SCSI_DEBUG
		if (bcr == 1)
			printf("sw_disconnect:  bcr is 1\n");
		if (sw_dis_debug) {
		    printf("sw_disconnect:  addr= 0x%x  count= %d  bcr= 0x%x  ",
			   un->un_dma_curaddr, un->un_dma_curcnt,  bcr);
		    printf("sr= 0x%x  baddr= 0x%x\n", swr->csr, un->un_baddr);
		}
#endif		SCSI_DEBUG
	}

	/* 
	 * Remove this disconnected task from the ctlr ready queue and save 
	 * on disconnect queue until a reconnect is done.
	 */
	sw_discon_queue(un);

	SW_VME_OK(c, swr, "sw_disconnect");

	swr->dma_count = 0;

	/* DPRINTF("sw_disconnect:  done\n"); */
	return (OK);
}


/*
 * Complete reselection phase and reconnect to target.
 *
 * Return OK if sucessful, FAIL if not.
 *
 * NOTE: this routine cannot use sw_getdata to get identify msg
 * from reconnecting target due to sun3/50 scsi interface. The bcr
 * must be setup before the target changes scsi bus to data phase
 * if the command being reconnected involves dma (which we do not
 * know until we get the identify msg). Thus we cannot acknowledge
 * the identify msg until some setup of the host adaptor registers 
 * is done.
 */
sw_reconnect(c, target, lun, flag)
	register struct scsi_ctlr *c;
	register short target, lun;
	int	flag;
{
	register struct scsi_sw_reg *swr = (struct scsi_sw_reg *)(c->c_reg);
	register struct scsi_unit *un;

	/* search disconnect queue for reconnecting task */
	DPRINTF2("sw_reconnect:  target %d lun %d\n", target, lun); 
	if (sw_recon_queue(c, target, lun, flag) != OK)
		return (FAIL);

	un = c->c_un;

	/* restart disconnect activity */
	SCSI_RECON_TRACE('r', c, 0, 0, 0); 
	if (un->un_dma_curdir != SI_NO_DATA) {

		/* do initial dma setup */
		swr->dma_count = 0;
		if (un->un_dma_curdir == SI_RECV_DATA)
			swr->csr &= ~SI_CSR_SEND;
		else
			swr->csr |= SI_CSR_SEND;

		/*
		 * New: we set up everything we can here, rather
		 * than wait until data phase.
		 */
		swr->csr &= ~SI_CSR_DMA_EN;
		sw_dma_setup(c, un);

#ifdef		SCSI_DEBUG
		if (sw_dis_debug) {
		    printf("sw_reconnect:  addr= 0x%x  count= 0x%x  bcr= 0x%x",
			  un->un_dma_curaddr, un->un_dma_curcnt, swr->bcr); 
		    printf("  sr= 0x%x  baddr= 0x%x\n", swr->csr, un->un_baddr);
		}
#endif		SCSI_DEBUG
	}

	/* we can finally acknowledge identify message */
	SBC_WR.icr = SBC_ICR_ACK;

	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
	    SI_WAIT_COUNT, 0) != OK) {
		printf("sw%d:  sw_reconnect: REQ not INactive\n", SWNUM(c));
		return (FAIL);
	}
	SBC_WR.icr = 0;				/* clear ack */
	return (OK);
}


/*
 * Remove timed-out I/O request and report error to
 * it's interrupt handler.
 * Return OK if sucessful, FAIL if not.
 */
sw_deque(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_sw_reg *swr = (struct scsi_sw_reg *)c->c_reg;
	int s;

	EPRINTF("sw_deque:\n");

	/* Lock out the rest of sw till we've finished the dirty work. */
	s = splr(pritospl(c->c_intpri));
	/*
         * If current SCSI I/O request is the one that timed out,
         * Reset the SCSI bus as this looks serious.
         */
        if (un == c->c_un) {
	    	if (swr->csr & 
		  (SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR)) {
                        /* Hardware lost int., attempt restart */
                        (void) splx(s);
                        printf("sw%d:  sw_deque: lost interrupt\n", SWNUM(c));
                        sw_print_state(swr, c);
                        swintr(c);
                        return (OK);
                } else {
                        /* Really did timeout */
                        (void) splx(s);
                        return (FAIL);
                }
        }
	if (c->c_tab.b_actf == NULL) {
                /* Search for entry on disconnect queue */
                if (sw_recon_queue(c, un->un_target, un->un_lun, 0)) {
                        /* died in active que,  don't restart */
                        (void) splx(s);
                        return (OK);
                }
                /* died in disconnect que,  restart */
                (void) splx(s);
                EPRINTF("sw_deque: reactivating request\n");
                sw_print_state(swr, c);
                (*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
                c->c_tab.b_active &= C_QUEUED;          /* clear interlock */
                return (OK);
        } else {
                /* Search for entry on disconnect queue; but don't reconnect */
                if (sw_recon_queue(c, un->un_target, un->un_lun, 1)) {
                        /* died in active que,  don't restart */
                        (void) splx(s);
			return (OK);
                }
                /* died in disconnect que,  restart */
                (void) splx(s);
                EPRINTF("sw_deque: queue was not NULL\n");
                return (FAIL);
        }
}


/*
 * No current activity for the scsi bus. May need to flush some
 * disconnected tasks if a scsi bus reset occurred before the
 * target reconnected, since a scsi bus reset causes targets to 
 * "forget" about any disconnected activity.
 * Also, enable reconnect attempts.
 */
sw_idle(c, flag)
	register struct scsi_ctlr *c;
	int flag;
{
	register struct scsi_unit *un;
	register struct scsi_sw_reg *swr = (struct scsi_sw_reg *)c->c_reg;
	int s, resid;

	/* DPRINTF("sw_idle:\n"); */
	if (c->c_flags & SCSI_FLUSHING) {
		EPRINTF1("sw_idle:  flushing, flags 0x%x\n", c->c_flags);
		return;
	}

	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {
		EPRINTF("sw_idle:  flushing disconnect que\n"); 
		s = splr(pritospl(c->c_intpri));        /* time critical */
                c->c_tab.b_active = C_ACTIVE;           /* engage interlock */
		/*
                 * Force current I/O request to be preempted and put it
                 * on disconnect que so we can flush it.
                 */
                un = (struct scsi_unit *)(c->c_tab.b_actf);
                if (un != NULL  &&  un->un_md->md_utab.b_active & MD_IN_PROGRESS) {
                        /*un->un_md->md_utab.b_active |= MD_PREEMPT;*/
                        sw_discon_queue(un);
                        printf("si_idle: dequeueing active request\n");
                }

		/* now in process of flushing tasks */
		c->c_flags &= ~SCSI_FLUSH_DISQ;
		c->c_flags |= SCSI_FLUSHING;
		c->c_flush = c->c_disqtab.b_actl;

		for (un = (struct scsi_unit *)c->c_disqtab.b_actf; 
                     un  &&  c->c_flush; 
		     un = (struct scsi_unit *)c->c_disqtab.b_actf) {

			/* keep track of last task to flush */
			if (c->c_flush == (struct buf *) un) 
				c->c_flush = NULL;
			/* requeue on controller active queue */
                        if (sw_recon_queue(c, un->un_target, un->un_lun, 0))
                                continue;

			/* inform device routines of error */
			if (un->un_dma_curdir != SI_NO_DATA) {
				resid = un->un_dma_curcnt;
			} else {
				resid = 0;
			}
			(*un->un_ss->ss_intr)(c, resid, flag);
			sw_off(c, un, flag);		/* unit is going offline */
		}
		c->c_flags &= ~SCSI_FLUSHING;
		c->c_tab.b_active &= C_QUEUED;  /* Clear interlock */
                (void) splx(s);
	}

	/* enable reconnect attempts */
	swr->csr &= ~SI_CSR_DMA_EN;	/* turn off before SBC access */
	swr->dma_count = 0;
	swr->csr &= ~SI_CSR_SEND;
	swr->csr |= SI_CSR_DMA_EN;
}


/*
 * Get status bytes from scsi bus.
 * Returns number of status bytes read if no error.
 * If error, returns -1.
 * If scsi bus error, returns 0.
 */
sw_getstatus(un)
	register struct scsi_unit *un;
{
	register struct scsi_sw_reg *swr;
	register struct scsi_ctlr *c = un->un_c;
	register u_char *cp;
	register u_char msg;

	(int)swr = c->c_reg;

	/* DPRINTF("sw_getstatus:\n"); */
	SW_VME_OK(c, swr, "sw_getstatus:");

	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
	    SI_LONG_WAIT, 1) != OK) {
		printf("sw%d:  sw_getstatus: REQ inactive\n", SWNUM(c));
		sw_print_state(swr, c);
		return (0);
	}

	if (sw_wait_phase(swr, PHASE_STATUS) != OK) {
		printf("sw%d:  sw_getstatus: no STATUS phase\n", SWNUM(c));
		sw_print_state(swr, c);
		return (0);
	}
	cp = (u_char *) &un->un_scb;
	cp[0] = sw_getdata(c, PHASE_STATUS);
	DPRINTF1("si_getstatus:  status = 0x%x", cp[0]);

	if (sw_wait_phase(swr, PHASE_MSG_IN) != OK) {
		printf("sw%d:  sw_getstatus: no MSG_IN phase\n", SWNUM(c));
		sw_print_state(swr, c);
		return (0);
	}

	SW_VME_OK(c, swr, "sw_getstatus: msg_in");
	msg = sw_getdata(c, PHASE_MSG_IN);
	if (msg != SC_COMMAND_COMPLETE) {
		EPRINTF1("sw_getstatus:  bogus msg_in 0x%x\n", msg);
	}
	SBC_WR.tcr = TCR_UNSPECIFIED;

	/*
	 * Check status for error condition, return -1 if error.
	 * Otherwise, return 1 for no error.
	 */
	if (cp[0] & SCB_STATUS_MASK)
		return (-1);	/* retryable */

	return (1);	 	/* no error */
}


/* 
 * Wait for a scsi dma request to complete.
 * Disconnects were disabled in sw_cmd when polling for command completion.
 * Called by drivers in order to poll on command completion.
 */
sw_cmdwait(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_sw_reg *swr;
	register struct scsi_unit *un = c->c_un;
	register int ret_val;
	(int)swr = c->c_reg;

	DPRINTF("sw_cmdwait:\n");

	/* if command does not involve dma activity, then we are finished */
	if (un->un_dma_curdir == SI_NO_DATA) {
		return (OK);
	}

	/* wait for indication of dma completion */
	if (sw_wait((u_int *)&swr->csr, 
	    (SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT | SI_CSR_DMA_BUS_ERR), 1) != OK) {
		printf("sw%d:  sw_cmdwait: dma never completed\n", SWNUM(c));
		sw_reset(c, PRINT_MSG);
		ret_val = SCSI_FAIL;
		goto SW_CMDWAIT_EXIT;
	}

	/* 
	 * Early Cobra units have a faulty gate array.
	 * It can cause an illegal memory access if full
	 * page DMA is being used.  For less than a full
	 * page, no problem.  This problem typically shows
	 * up when dumping core.  The following code fixes
	 * this problem.
	 */
	swr->csr &= ~SI_CSR_DMA_EN;
	if(swr->csr & SI_CSR_DMA_BUS_ERR){
		(void) sw_dma_recv(c);
		junk = SBC_RD.clr;
		ret_val = OK;
		goto SW_CMDWAIT_EXIT;
	}

	/* handle special dma recv situations */
	if ((un->un_dma_curdir == SI_RECV_DATA)  &&  (sw_dma_recv(c) != OK)) {
		printf("sw%d:  sw_cmdwait: special DMA failure\n", SWNUM(c));
		sw_reset(c, PRINT_MSG);
		ret_val = SCSI_FAIL;
		goto SW_CMDWAIT_EXIT;
	}

	/* ack sbc interrupt and cleanup */
	junk = SBC_RD.clr;
	sw_dma_cleanup(c);
	ret_val = OK;

SW_CMDWAIT_EXIT:
	if (swr->csr & SI_CSR_SBC_IP) {
		junk = SBC_RD.clr;	/* clear sbc int */
	}
	swr->csr &= ~SI_CSR_DMA_EN;	/* turn it off to be sure */
	return (ret_val);

}


/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 * Returns OK for successful.  Otherwise, returns
 * FAIL.
 */
static
sw_sbc_wait(reg, cond, wait_count, set)
	register caddr_t reg;
	register u_char cond;
	register int wait_count;
	register int set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < wait_count; i++) {
		regval = *reg;
		if ((set == 1)  &&  (regval & cond)) {
			return (OK);
		}
		if ((set == 0)  &&  !(regval & cond)) {
			return (OK);
		} 
		DELAY(10);
	}
	DPRINTF("sw_sbc_wait:  timeout\n");
	return (FAIL);
}


/*
 * Wait for a condition to be (de)asserted.
 * Used for monitor DMA controller.
 * Returns OK for successful.  Otherwise,
 * returns FAIL.
 */
static
sw_wait(reg, cond, set)
	register u_int *reg;
	register u_int cond;
	register int set;
{
	register int i;
	register u_int regval;


	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1)  &&  (regval & cond)) {
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
sw_wait_phase(swr, phase)
	register struct scsi_sw_reg *swr;
	register u_char phase;
{
	register int i;

	DPRINTF2("sw_wait_phase:  %s phase (0x%x)\n",sw_str_phase(phase),phase);
	for (i = 0; i < SI_WAIT_COUNT; i++) {
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
sw_putdata(c, phase, data, num, want_intr)
	register struct scsi_ctlr *c;
	register u_short phase;
	register u_char *data;
	register u_char num;
	register int want_intr;
{
	register struct scsi_sw_reg *swr;
	register u_char i;
	swr = (struct scsi_sw_reg *)c->c_reg;

	DPRINTF2("sw_putdata:  %s phase (0x%x): ", sw_str_phase(phase), phase);
	SW_VME_OK(c, swr, "sw_putdata");

	/* Set up tcr so we can transmit data.  */
	SBC_WR.tcr = phase >> 2;

	/* put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		SBC_WR.icr = SBC_ICR_DATA;	/* clear ack, enable data bus */

		/* wait for target to request a byte */
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
		    SI_WAIT_COUNT, 1) != OK) {
			printf("sw%d:  putdata, REQ not active\n", SWNUM(c));
			return (FAIL);
		}

		/* load data */
		DPRINTF1("  0x%x", *data);
		SBC_WR.odr = *data++;

		/* complete req/ack handshake */
		SBC_WR.icr |= SBC_ICR_ACK;
		if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SI_WAIT_COUNT, 0) != OK) {
			printf("sw%d:  putdata, req not INactive\n", SWNUM(c));
			return (FAIL);
		}
	}
	DPRINTF("\n");
	SBC_WR.tcr = TCR_UNSPECIFIED;
	junk = SBC_RD.clr;			/* clear int */
	if (want_intr) {

		/* CRITICAL CODE SECTION DON'T TOUCH */
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.icr = 0;			/* ack last byte */
	} else {
		SBC_WR.icr = 0;			/* clear ack */
	}
	return (OK);
}


/*
 * Get data from the scsi bus.
 * Returns a single byte of data, -1 if unsuccessful.
 */
static
sw_getdata(c, phase)
	register struct scsi_ctlr *c;
	register u_short phase;
{
	register struct scsi_sw_reg *swr;
	register u_char data;
	register u_char icr;
	(int)swr = c->c_reg;

	DPRINTF2("sw_getdata: %s phase (0x%x)",sw_str_phase(phase),phase);
	SW_VME_OK(c, swr, "sw_getdata");

	/* wait for target request */
	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
	    SI_WAIT_COUNT, 1) != OK) {
		printf("sw%d:  getdata, REQ not active, cbsr 0x%x\n",
			SWNUM(c), SBC_RD.cbsr);
		sw_print_state(swr, c);
		return (-1);
	}

	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != phase) {
		/* not the phase we expected */
		DPRINTF1("sw_getdata:  unexpected phase, expecting %s\n",
			sw_str_phase(phase));
		return (-1);
	}

	/* grab data and complete req/ack handshake */
	data = SBC_RD.cdr;
	icr = SBC_WR.icr;
	DPRINTF1("  icr= %x  ", icr);
	SBC_WR.icr = icr | SBC_ICR_ACK;

	if (sw_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
	    SI_WAIT_COUNT, 0) != OK) {
		printf("sw%d:  getdata, REQ not inactive\n", SWNUM(c));
		return (-1);
	}

	DPRINTF1("  data= 0x%x\n", data);
	if ((phase == PHASE_MSG_IN)  && 
	    ((data == SC_COMMAND_COMPLETE)  ||  (data == SC_DISCONNECT))) {
		/* CRITICAL CODE SECTION DON'T TOUCH */
		SBC_WR.tcr = TCR_UNSPECIFIED;
		SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int */
		junk = SBC_RD.clr;		/* clear int */
		SBC_WR.icr = icr;		/* clear ack */
	} else {
		SBC_WR.icr = icr;		/* clear ack */
	}
	return (data);
}


/*
 * Reset SCSI control logic and bus.
 */
sw_reset(c, msg_enable)
	register struct scsi_ctlr *c;
	register int msg_enable;
{
	register struct scsi_sw_reg *swr;
	(int)swr = c->c_reg;

	if (msg_enable) {
		printf("sw%d:  resetting scsi bus\n", SWNUM(c));
		sw_print_state(swr, c);
		DEBUG_DELAY(100000);
	}

	/* reset scsi control logic */
	swr->csr = 0;
	DELAY(10);
	swr->csr = SI_CSR_SCSI_RES;
	swr->dma_addr = 0;
	swr->dma_count = 0;

	/* issue scsi bus reset (make sure interrupts from sbc are disabled) */
	SBC_WR.icr = SBC_ICR_RST;
	DELAY(1000);
	SBC_WR.icr = 0;				/* clear reset */

	/* give reset scsi devices time to recover (> 2 Sec) */
	DELAY(scsi_reset_delay);
	junk = SBC_RD.clr;

	/* Disable sbc interrupts.  Reconnects enabled by sw_cmd. */	
	SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int */
	swr->csr |= SI_CSR_INTR_EN;

	/* disconnect queue needs to be flushed */
	if ((c->c_tab.b_actf != NULL) || (c->c_disqtab.b_actf != NULL)) {
		c->c_flags |= SCSI_FLUSH_DISQ;
		sw_idle(c, SE_TIMEOUT);
	}
}


/*
 * Return residual count for a dma.
 */
/*ARGSUSED*/
sw_dmacnt(c)
	struct scsi_ctlr *c;
{

	return (0);
}
#endif NSW > 0
