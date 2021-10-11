/* @(#)sc.c       1.1 92/07/30 Copyr 1987 Sun Micro */
#include "sc.h"
#if NSC > 0

#ifndef lint
static char sccsid[] = "@(#)sc.c       1.1 92/07/30 Copyr 1987 Sun Micro";
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
#include "../vm/seg.h"
#include "../machine/seg_kmem.h"

#include "../sun/dklabel.h"
#include "../sun/dkio.h"

#include "../sundev/mbvar.h"
#include "../sundev/sireg.h"
#include "../sundev/screg.h"
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
#include <sys/mman.h>

#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/scb.h>
#include <vm/seg.h>
#include <machine/seg_kmem.h>

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/sireg.h>
#include <sundev/screg.h>
#include <sundev/scsi.h>
#endif REL4

#define SCNUM(sc)	(sc - scctlrs)

int     scwatch();
int	scprobe(), scslave(), scattach(), scgo(), scdone(), scpoll();
int	scustart(), scstart(), sc_cmd(), sc_getstatus(), sc_cmdwait();
int	sc_off(), sc_reset(), sc_dmacnt(), sc_deque();

extern	int nsc, scsi_host_id, scsi_reset_delay;
extern	struct scsi_ctlr scctlrs[];		/* per controller structs */
extern	struct mb_ctlr *scinfo[];
extern	struct mb_device *sdinfo[];

struct	mb_driver scdriver = {
	scprobe, scslave, scattach, scgo, scdone, scpoll,
	sizeof (struct scsi_ha_reg), "sd", sdinfo, "sc", scinfo, MDR_BIODMA,
};

/* routines available to devices for mainbus scsi host adaptor */
struct scsi_ctlr_subr scsubr = {
	scustart, scstart, scdone, sc_cmd, sc_getstatus, sc_cmdwait,
	sc_off, sc_reset, sc_dmacnt, scgo, sc_deque,
};

extern int scsi_debug;
extern int scsi_disre_enable;
extern int scsi_ntype;
extern struct scsi_unit_subr scsi_unit_subr[];

#ifdef	SCSI_DEBUG
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
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 
#define DPRINTF1(str, arg2) 
#define DPRINTF2(str, arg1, arg2) 
#define EPRINTF(str) 
#define EPRINTF1(str, arg2) 
#define EPRINTF2(str, arg1, arg2) 
#endif SCSI_DEBUG


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


/*
 * This routine unlocks this driver when I/O
 * has ceased, and a call to scstart is
 * needed to start I/O up again.  Refer to
 * xd.c and xy.c for similar routines upon
 * which this routine is based
 */
static
scwatch(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;
	un = c->c_un;

	/*
	 * Find a valid un to call s?start with to for next command
	 * to be queued in case DVMA ran out.  Try the pending que
	 * then the disconnect que.  Otherwise, driver will hang
	 * if DVMA runs out.  The reason for this kludge is that
	 * scstart should really be passed c instead of un, but it
	 * would break lots of 3rd party S/W if this were fixed.
	 */
	if (un == NULL) {
		un = (struct scsi_unit *)c->c_tab.b_actf;
		if (un == NULL)
			un = (struct scsi_unit *)c->c_disqtab.b_actf;
	}
	scstart(un);
	timeout(scwatch, (caddr_t)c, 10*hz);
}

/*
 * Determine existence of SCSI host adapter.
 * Returns 0 for failure, size of sc data structure if ok.
 */
scprobe(reg, ctlr)
	register struct scsi_ha_reg *reg;
	register int ctlr;
{
	register struct scsi_ctlr *c;

	/* probe for different scsi host adaptor interfaces */
	c = &scctlrs[ctlr];
	if (peekc((char *)&reg->dma_count) == -1) {
		return (0);
	}
	/* validate cntlr by write/read/cmp with a data pattern */
	reg->dma_count = 0x6789;
	if (reg->dma_count != 0x6789) {
		return (0);
	}
	EPRINTF2("sc%d:  scprobe: reg= 0x%x", ctlr, (u_int)reg);
	EPRINTF1(",  c= 0x%x\n", (u_int)c );
        
	/*
         * Allocate a page for being able to
         * flush the last bit of a data transfer.
         */
        c->c_kment = rmalloc(kernelmap, (long)mmu_btopr(MMU_PAGESIZE));
        if (c->c_kment == 0) {
                printf("si%d: out of kernelmap for last DMA page\n", ctlr);
                return (0);
        }

	/* init controller information */
	c->c_flags = SCSI_PRESENT;
	c->c_reg = (int)reg;
	c->c_ss = &scsubr;
	c->c_name = "sc";
	c->c_tab.b_actf = NULL;
	c->c_tab.b_active = 0;
	sc_reset(c, NO_MSG);
	c->c_un = NULL;
	timeout(scwatch, (caddr_t)c, 10*hz);
	return (sizeof (struct scsi_ha_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
scslave(md, reg)
	struct mb_device *md;
	register struct scsi_ha_reg *reg;
{
	register struct scsi_unit *un;
	register int type;
	/* DPRINTF("scslave:\n"); */

#ifdef	lint
	reg = reg;				/* use it or lose it */
	reg = (struct scsi_ha_reg *)scsi_debug;
	reg = (struct scsi_ha_reg *)scsi_disre_enable;
#endif	lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype) {
		panic("scslave:  unknown type in md_flags\n");
	}

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == 0) {
		panic("scslave:  md_flags scsi type not configured\n");
	}
	un->un_c = &scctlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}


/*
 * Attach device (boot time).
 */
scattach(md)
	register struct mb_device *md;
{
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &scctlrs[md->md_ctlr];
	register int type = TYPE(md->md_flags);
	register struct scsi_ha_reg *reg;

	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("scattach:\n"); */
	if (type >= scsi_ntype) {
		panic("scattach:  unknown type in md_flags\n");
	}
	(*scsi_unit_subr[type].ss_attach)(md);

	/*
	 * Initialize interrupt register
	 */
	if (mc->mc_intr) {
		/* set up for vectored interrupts - we will pass ctlr ptr */
		reg->intvec = mc->mc_intr->v_vec;
		*(mc->mc_intr->v_vptr) = (int)c;
	} else {
		/* use auto-vectoring */
		reg->intvec = AUTOBASE + mc->mc_intpri;
	}
}


/* 
 * corresponding to un is an md.  if this md has SCSI commands
 * queued up (md->md_utab.b_actf != NULL) and md is currently
 * not active (md->md_utab.b_active == 0), this routine queues the
 * un on its queue of devices (c->c_tab) for which to run commands
 * Note that md is active if its un is already queued up on c->c_tab,
 * or ts un is on the scsi_ctlr's disconnect queue c->c_disqtab.
 */
scustart(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct mb_ctlr *mc = un->un_mc;
	register struct mb_device *md = un->un_md;
	int	s;

	/* DPRINTF("scustart:\n"); */
	s = splr(pritospl(mc->mc_intpri));
	if ((md->md_utab.b_actf != NULL) && (md->md_utab.b_active == 0)) {

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
 * Set up a scsi operation.
 */
scstart(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c;
	register struct mb_device *md;
	register struct buf *bp;
	int s;

	/* return immediately if passed NULL un */
	if (un == NULL)
		return;

	/* DPRINTF("scstart:\n"); */

	/* return immediately, if the ctlr is already actively
         * running a SCSI command
         */
	s = splr(pritospl(3));
	c = un->un_c;
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

	/* 
         * put bp into the active position
         */
        bp = md->md_utab.b_actf;
        md->md_utab.b_forw = bp;

	/* only happens when called by intr */
	if (bp == NULL) {
		c->c_tab.b_active = 0;
		(void) splx(s);
		return;
	}

        /* special commands which are initiated by
         * the high-level driver, are run using
         * its special buffer, un->un_sbuf.
         * in most cases, main bus set-up has
         * already been done, so scgo can be called
         * for on-line formatting, we need to
         * call mbsetup.
         */
	if ((*un->un_ss->ss_start)(bp, un)) {
		un->un_c->c_un = un;
		c->c_tab.b_active = 1;

		if (bp == &un->un_sbuf &&
                    ((un->un_flags & SC_UNF_DVMA) == 0) &&
		    ((un->un_flags & SC_UNF_SPECIAL_DVMA) == 0)) {
			scgo(md);
			(void) splx(s);
			return;
		} else {
			(void)mbugo(md);
			(void) splx(s);
			return;
		}
	} else {
		scdone(md);
		(void) splx(s);
	}
}


/*
 * Start up a transfer
 * Called via mbgo after buffer is in memory
 */
scgo(md)
	register struct mb_device *md;
{
	register struct mb_ctlr *mc;
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	register int unit;
	register int err;
	int s;
	int type;


	/* DPRINTF("scgo:\n"); */
	s = splr(pritospl(3));
	if (md == NULL) {
		(void) splx(s);
                panic("scgo:  queueing error1\n");
        }

	mc = md->md_mc;
	c = &scctlrs[mc->mc_ctlr];

	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);

	bp = md->md_utab.b_forw;
	if (bp == NULL) {
		EPRINTF("scgo: bp is NULL\n");
		(void) splx(s);
		return;
	}

	un->un_baddr = MBI_ADDR(md->md_mbinfo);
	c->c_tab.b_active |= 0x1;

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
	(*un->un_ss->ss_mkcdb)(un);
	if ((err=sc_cmd(c, un, 1)) != OK) {

	/* note that in the non-polled case (here) that
	 * SCSI_FAIL is not a possible return value from
	 * sc_cmd.
	 */
		if (err == FAIL) {
			EPRINTF("scgo:  sc_cmd FAILED\n");
			(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);

		} else if (err == HARD_FAIL) {
			EPRINTF("scgo:  sc_cmd hard FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
			sc_off(c, un);
		}
	}
	(void) splx(s);
}


/*
 * Handle a polling SCSI bus interrupt.
 */
scpoll()
{
	register struct scsi_ctlr *c;
	register struct scsi_ha_reg *reg;
	register int serviced = 0;

	/* DPRINTF("scpoll:\n"); */
	for (c = scctlrs; c < &scctlrs[nsc]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		reg = (struct scsi_ha_reg *)(c->c_reg);
		if ((reg->icr & (ICR_INTERRUPT_REQUEST | ICR_BUS_ERROR)) == 0) {
			continue;
		}
		serviced = 1;
		scintr(c);
	}
	return (serviced);
}


/*
 * Clean up queues, free resources, and start next I/O
 * after I/O finishes.  Called by mbdone after moving
 * read data from Mainbus
 */
scdone(md)
	register struct mb_device *md;
{
	register struct mb_ctlr *mc = md->md_mc;
        register struct scsi_ctlr *c = &scctlrs[mc->mc_ctlr];
        register struct scsi_unit *un;
        register struct buf *bp = md->md_utab.b_forw;
	int type;

	/* DPRINTF("scdone:\n"); */

	/* more reliable than un = c->c_un; */
	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
		(*scsi_unit_subr[type].ss_unit_ptr)(md);
        /* for on-line formatting case, call mbrelse to release
         * DVMA.
         */

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
         * md, scustart will queue up md again. at tail.
         * first, need to mark md as inactive (not on queue)
         */
        md->md_utab.b_active = 0;
        scustart(un);

	iodone(bp);

        /* check if the ctlr's queue is NULL, and if so,
         * idle the controller, otherwesie,
		 * start up the next command on the scsi_ctlr.
         */
        if (c->c_tab.b_actf == NULL) {
                sc_idle(c);
		return;
	}

	scstart(un);
}


/*
 * Pass a command to the SCSI bus.
 * OK returned if successful.  Otherwise,
 * FAIL returned for recoverable error;
 * HARD_FAIL returned for non-recoverable error; and
 * SCSI_FAIL returned for SCSI bus timing failure.
 */
sc_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;
{
	register u_char *cp;
	register int i, errct = 0;
	register u_short icr_mode;
	register u_char size;
	register struct scsi_ha_reg *reg;

	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("sc_cmd:\n"); */
	do {
		/* make sure scsi bus is not continuously busy */
		for (i = SC_WAIT_COUNT; i > 0; i--) {
		        if (!(reg->icr & ICR_BUSY))
		                break;
		        DELAY(10);
		}
		if (i == 0) {
			printf("sc%d:  SCSI bus continuously busy\n",
				SCNUM(c));
			sc_reset(c, PRINT_MSG);
			sc_idle(c);
			return (FAIL);
		}

		/* select target and wait for response */
		/*+++wmb (by mjacob) for bug #1004639 */

		reg->icr = 0; /* Make sure SECONDBYTE flag is clear */

		/* ---wmb	*/
		reg->data = (1 << un->un_target) | HOST_ADDR;
		reg->icr = ICR_SELECT;

		/* target never responded to selection */
		if (sc_wait(c, SC_SHORT_WAIT, ICR_BUSY) != OK) {
			reg->data = 0;
			reg->icr = 0;
			return (HARD_FAIL);
		}
		/*
		 * may need to map between the CPU's kernel context address
		 * and the device's DVMA bus address
		 */
		SET_DMA_ADDR(reg, un->un_dma_addr);
		reg->dma_count = ~un->un_dma_count; /* hardware is funny */
		icr_mode = ICR_DMA_ENABLE;
		if (intr) {
			icr_mode |= ICR_INTERRUPT_ENABLE;
			un->un_wantint = 1;
		}

		/* Allow either byte or word transfers.
		 * Note, we trust that DMA count is even.
		 * This seems safe enough since sd & st
		 * worry about this for us.
		 */
		if  ( !(un->un_dma_addr & 0x1)) {
			icr_mode |= ICR_WORD_MODE;
		} 
#ifdef		SCSI_DEBUG
		else {
			DPRINTF("sc_cmd:  odd byte DMA address\n");
		}
#endif		SCSI_DEBUG
		reg->icr = icr_mode;

		/*
		 * Determine size of the cdb.  Since we're smart, we
		 * look at group code and guess.  If we don't
		 * recognize the group id, we use the specified
		 * cdb length.  If both are zero, use max. size
		 * of data structure.
		 */
		cp = (u_char *) &un->un_cdb;
		if ((size = cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) == 0  &&
		    (size = un->un_cmd_len) == 0) {
			size = sizeof (struct scsi_cdb);
			printf("sc%d:  invalid cdb size, using size= %d\n",
				SCNUM(c), size);
		}

#ifdef		SCSI_DEBUG
		if (scsi_debug > 2) {
			printf("sc%d:  sc_cmd: target %d command=",
			    SCNUM(c), un->un_target);
			for (i = 0; i < size; i++) {
				printf("  %x", *cp++);
			}
			printf("\n");
			cp = (u_char *) &un->un_cdb;	/* restore cp */
		}
#endif		SCSI_DEBUG

		for (i = 0; i < size; i++) {
			if (sc_putbyte(c, ICR_COMMAND, *cp++) != OK) {
				errct++;
				break;
			}
		}

		if (i == size) {
			/* If not in polled I/O mode, we're done */
			if (intr != 0)
				return (OK);

			/*
			 * If in polled I/O mode, wait for cmd completion
			 * and get the result status, too.
			 */
			if ((sc_cmdwait(c) == OK)  &&
			    (sc_getstatus(c->c_un) == OK)) {
				/* DPRINTF("sc_cmd: cmd ok\n"); */
				return (OK);
			} else {
				EPRINTF("sc_cmd: cmd error\n");
				return (FAIL);
			}
		}

	} while (errct < 5);
	printf("sc%d:  sc_cmd: unrecoverable errors\n", SCNUM(c));
	return (HARD_FAIL);
}


/*
 * Handle a scsi bus interrupt.
 */
scintr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_ha_reg *reg = (struct scsi_ha_reg *)(c->c_reg);
	register int resid;
	int offset;
	u_char *mapaddr;
        u_int pv;

	/* DPRINTF("scintr:\n"); */
	if (un->un_wantint == 0) {
		if (reg->icr & ICR_BUS_ERROR) {
                        printf("sc%d:  scsi bus error (unwanted interrupt)\n",
				SCNUM(c));
		} else {
			printf("sc%d:  spurious interrupt\n", SCNUM(c));
		}
		DEBUG_DELAY(100000000);
		sc_reset(c, PRINT_MSG);
		sc_idle(c);
		return;
	}
	un->un_wantint = 0;
	resid = ~reg->dma_count & 0xffff;
	if (reg->icr & ICR_BUS_ERROR) {
		printf("sc%d:  SCSI bus error, icr= %x  resid= %d\n",
		    	SCNUM(c), (u_short)(reg->icr), resid);
		DEBUG_DELAY(100000000);
		sc_reset(c, PRINT_MSG);
		sc_idle(c);
		return;
	}
	if (reg->icr & ICR_ODD_LENGTH) {
		if (un->un_flags & SC_UNF_RECV_DATA) {
			offset = un->un_baddr + un->un_dma_count - resid;
			if (MBI_MR(offset) < dvmasize) { 
				DVMA[offset] = reg->data;
			}
			else {
#ifdef  sun3x
			   pv = btop (VME24D16_BASE + (offset & VME24D16_MASK));
#else   sun3x
			   pv = PGT_VME_D16 | btop(VME24_BASE | (offset & VME24_MASK));
#endif  sun3x
		           mapaddr = (u_char *) ((u_long) kmxtob(c->c_kment) |
                          	    (u_long) MBI_OFFSET(offset));
		           segkmem_mapin(&kseg, 
				(addr_t) (((int)mapaddr) & PAGEMASK),
                		(u_int) mmu_ptob(1), PROT_READ | PROT_WRITE, pv, 0);
			   *mapaddr = reg->data;
        		   segkmem_mapout(&kseg, 
				(addr_t)(((int)mapaddr) & PAGEMASK), 
				(u_int) mmu_ptob(1));
			}
			resid--;
		} else if (un->un_dma_count != 0) {
			resid++;
		} else {
			printf("sc%d:  odd length without xfer\n",
				SCNUM(c));
		}
		resid &= 0xffff;	/* overflow/underflow protection */
	}
	if (sc_getstatus(c->c_un) == OK) {
		/* DPRINTF("scintr: no error\n"); */
		(*c->c_un->un_ss->ss_intr)(c, resid, SE_NO_ERROR);
	} else {
		EPRINTF("scintr: error\n");
		(*c->c_un->un_ss->ss_intr)(c, resid, SE_RETRYABLE);
	}
}


/*
 * Bring a unit offline.
 */
/*ARGSUSED*/
sc_off(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct mb_device *md = un->un_md;

	/*
	 * Be warned, if you take the root device offline,
	 * the system is going to have a heartattack !
	 * Note, if unit is already offline, don't bother
	 * to print error message.
	 */
	if (un->un_present) {
		printf("sc%d:  %s%d, unit offline\n", SCNUM(c),
			scsi_unit_subr[md->md_flags].ss_devname,
			SCUNIT(un->un_unit));
		un->un_present = 0;
	}
}


/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 * Returns OK if successful.  Otherwise, returns
 * FAIL.
 */
static
sc_wait(c, wait_count, cond)
	register struct scsi_ctlr *c;
	register int wait_count;
{
	register int i, icr;
	register struct scsi_ha_reg *reg;
	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("sc_wait:\n"); */

	for (i = 0; i < wait_count; i++) {
		icr = reg->icr;
		if ((icr & cond) == cond) {
			return (OK);
		}
		if (icr & ICR_BUS_ERROR) {
			break;
		}
		DELAY(10);
	}
	/* DPRINTF("sc_wait: failed\n"); */
	return (FAIL);
}


/*
 * Wait for a scsi dma request to complete.
 * Called by top level drivers in order to poll for
 * command completion.
 */
sc_cmdwait(c)
	register struct scsi_ctlr *c;
{
	/* DPRINTF("sc_cmdwait:\n"); */
	if (sc_wait(c, SC_WAIT_COUNT, ICR_INTERRUPT_REQUEST) != OK) {
		printf("sc%d:  sc_cmdwait: no interrupt request\n",
			SCNUM(c));
		sc_reset(c, PRINT_MSG);
		return (SCSI_FAIL);
	} else {
		return (OK);
	}
}


/*
 * Put data byte onto the scsi bus.
 * Returns OK if successful, FAIL otherwise.
 */
static
sc_putbyte(c, bits, data)
	register struct scsi_ctlr *c;
	register u_short bits;
	register u_char data;
{
	register int icr;
	register struct scsi_ha_reg *reg;
	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("sc_putbyte:\n"); */

	if (sc_wait(c, SC_WAIT_COUNT, ICR_REQUEST) != OK) {
		printf("sc%d:  sc_cmdwait: no REQ\n", SCNUM(c));
		return (FAIL);
	}
	icr = reg->icr;
	if ((icr & ICR_BITS) != bits) {
#ifdef		SCSI_DEBUG
		printf("sc_putbyte:  error\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits);
#endif		SCSI_DEBUG
		return (FAIL);
	}
#ifdef	SC_PARITY
	/* Enable parity on SCSI bus... for bug #1006663 KSAM */
	reg->icr = icr | ICR_PARITY_ENABLE;
#endif	SC_PARITY
	reg->cmd_stat = data;
	return (OK);
}


/*
 * Get data from the scsi bus.
 * Returns a single byte of data, -1 if unsuccessful.
 */
static
sc_getbyte(c, bits)
	register struct scsi_ctlr *c;
{
	register int icr;
	register struct scsi_ha_reg *reg;
	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("sc_getbyte:\n"); */

	if (sc_wait(c, SC_WAIT_COUNT, ICR_REQUEST) != OK) {
		printf("sc%d:  sc_cmdwait: no REQ\n", SCNUM(c));
		return (-1);
	}
	icr = reg->icr;
	if ((icr & ICR_BITS) != bits) {
		if (bits == ICR_STATUS) {
			return (-1);	/* no more status */
		}
#ifdef		SCSI_DEBUG
		printf("sc_getbyte:  error\n");
		sc_pr_icr("icr is     ", icr);
		sc_pr_icr("waiting for", bits);
#endif		SCSI_DEBUG
		return (-1);
	}
#ifdef  SCSI_DEBUG
	/* check parity error on SCSI bus... for bug #1006663 KSAM */
	/* for normal operation, just ignored due to some peripheral */
	/* that does NOT generate parity, resulting host parity ERRROR */
	if (icr & ICR_PARITY_ERROR) {           /* read only bit */
		EPRINTF1("sc%d:  parity error detected\n", SCNUM(c));
		/* Momentarily clear parity_enable bit, which will clear error */
		icr &= ~ICR_PARITY_ENABLE;	/* disable parity */
		reg->icr = icr;
		icr |= ICR_PARITY_ENABLE;	/* re-enable parity */
		reg->icr = icr;
	}
#endif  SCSI_DEBUG
	return (reg->cmd_stat);
}


/*
 * Routine to handle abort SCSI commands which have timed out.
 */
sc_deque(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register u_int target;
	register u_int lun;
	int s;

	/* Lock out the rest of sc till we've finished the dirty work. */
	target = un->un_target;
	lun = un->un_lun;
#ifdef 	lint
	target = target;
	lun = lun;
#endif	lint

	s = splr(pritospl(un->un_mc->mc_intpri));
	un = c->c_un;			/* get current unit */

	/*
	 * If timeout didn't occur on current I/O request,
	 * we've got a problem.
	 */
	if (un->un_c->c_tab.b_active != 1 ) {
		EPRINTF("sc_deque:  entry not found\n");
		(void) splx(s);
		return (FAIL);
	}
	EPRINTF("sc_deque:  removing failed request\n");
	(*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
	(void) splx(s);
	return (OK);
}


/*
 * Return residual count for dma.
 */
sc_dmacnt(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_ha_reg *reg;
	reg = (struct scsi_ha_reg *)(c->c_reg);

	return ((u_short)(~reg->dma_count));
}


/*
 * Flush current I/O request after SCSI bus reset.
 * If no current I/O request, forget it.
 */
static
sc_idle(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;
	un = c->c_un;
	/* DPRINTF("sc_idle:\n"); */

	if (un->un_c->c_tab.b_active) {
		/*
		 * Since we reset the bus, we should inform
		 * the top level driver that it's dead.
		 */
		EPRINTF("sc_idle:  returning fatal error\n");
		(*c->c_un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
	}
}


/*
 * Reset SCSI control logic and bus.
 */
sc_reset(c, msg_enable)
	register struct scsi_ctlr *c;
	register int msg_enable;
{
	register struct scsi_ha_reg *reg;
	reg = (struct scsi_ha_reg *)(c->c_reg);

	/* DPRINTF("sc_reset:\n"); */
	if (msg_enable) {
		printf("sc%d:  resetting scsi bus\n", SCNUM(c));
		DEBUG_DELAY(100000000);
	}

	reg->icr = ICR_RESET;
	DELAY(1000);
	reg->icr = 0;

	/* give reset scsi devices time to recover (> 2 Sec) */
	DELAY(scsi_reset_delay);
}


/*
 * Get status bytes from scsi bus.
 */
sc_getstatus(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register u_char *cp = (u_char *)&un->un_scb;
	register int i;
	register int b;
	register short retval = OK;
	/* DPRINTF("sc_getstatus:\n"); */

	/* get all the status bytes */
	for (i = 0;;) {
		b = sc_getbyte(c, ICR_STATUS);
		if (b < 0) {
			break;
		}
		if (i < STATUS_LEN) {
			cp[i++] = b;
		}
	}
	/* If status anything but no error, report it. */
	if (cp[0] & SCB_STATUS_MASK)
		retval = FAIL;		/* cmd completed with error */

	/* get command complete message */
	b = sc_getbyte(c, ICR_MESSAGE_IN);
	if (b != SC_COMMAND_COMPLETE) {
#ifdef		SCSI_DEBUG
		if (scsi_debug) {
			printf("sc_getstatus:  Invalid SCSI message: %x\n", b);
			printf("  Status bytes= ");
			for (b = 0; b < i; b++) {
				printf(" %x", cp[b]);
			}
			printf("\n");
		}
#endif		SCSI_DEBUG
		return (FAIL);
	}

#ifdef	SCSI_DEBUG
	if (scsi_debug > 1) {
		printf("sc_getstatus:  status=");
		for (b = 0; b < i; b++) {
			printf(" %x", cp[b]);
		}
		printf("\n");
	}
#endif	SCSI_DEBUG
	return (retval);
}


#ifdef	SCSI_DEBUG
/*
 * Print out the scsi host adapter interface control register.
 */
static
sc_pr_icr(cp, i)
	register char *cp;
{
	printf("\t%s: %x ", cp, i);
	if (i & ICR_PARITY_ERROR)
		printf("Parity err ");
	if (i & ICR_BUS_ERROR)
		printf("Bus err ");
	if (i & ICR_ODD_LENGTH)
		printf("Odd len ");
	if (i & ICR_INTERRUPT_REQUEST)
		printf("Int req ");
	if (i & ICR_REQUEST) {
		printf("Req ");
		switch (i & ICR_BITS) {
		case 0:
			printf("Data out ");
			break;
		case ICR_INPUT_OUTPUT:
			printf("Data in ");
			break;
		case ICR_COMMAND_DATA:
			printf("Command ");
			break;
		case ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Status ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA:
			printf("Msg out ");
			break;
		case ICR_MESSAGE | ICR_COMMAND_DATA | ICR_INPUT_OUTPUT:
			printf("Msg in ");
			break;
		default:
			printf("DCM: %x ", i & ICR_BITS);
			break;
		}
	}
	if (i & ICR_PARITY)
		printf("Parity ");
	if (i & ICR_BUSY)
		printf("Busy ");
	if (i & ICR_SELECT)
		printf("Sel ");
	if (i & ICR_RESET)
		printf("Reset ");
	if (i & ICR_PARITY_ENABLE)
		printf("Par ena ");
	if (i & ICR_WORD_MODE)
		printf("Word mode ");
	if (i & ICR_DMA_ENABLE)
		printf("Dma ena ");
	if (i & ICR_INTERRUPT_ENABLE)
		printf("Int ena ");
	printf("\n");
}


/*
 * Print out status completion block.
 */
static
sc_pr_scb(scb)
	register struct scsi_scb *scb;
{
	register u_char *cp;

	cp = (u_char *) scb;
	printf("scb: %x", cp[0]);
	if (scb->ext_st1) {
		printf(" %x", cp[1]);
		if (scb->ext_st2) {
			printf(" %x", cp[2]);
		}
	}
	if (scb->is) {
		printf(" intermediate status");
	}
	if (scb->busy) {
		printf(" busy");
	}
	if (scb->cm) {
		printf(" condition met");
	}
	if (scb->chk) {
		printf(" check");
	}
	if (scb->ext_st1 && scb->ha_er) {
		printf(" host adapter detected error");
	}
	printf("\n");
}
#endif	SCSI_DEBUG
#endif	NSC > 0
