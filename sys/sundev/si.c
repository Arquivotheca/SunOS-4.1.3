/* @(#)si.c       1.1 92/07/30 Copyr 1989 Sun Micro */
#include "si.h"
#if NSI > 0

#ifndef lint
static	char sccsid[] = "@(#)si.c       1.1 92/07/30 Copyr 1989 Sun Micro";
#endif	lint

/*#define SCSI_DEBUG			/* Turn on debugging code */

/*
 * Generic scsi routines.
 */
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
#ifdef IOC
#ifndef sun3x
#include <machine/iocache.h>
#endif sun3x
#endif IOC

#include <sun/dklabel.h>
#include <sun/dkio.h>

#include <sundev/mbvar.h>
#include <sundev/sireg.h>
#include <sundev/scsi.h>

/* Defines to simplify probe code */
#ifndef CPU_SUN3_50
#define CPU_SUN3_50	0
#endif CPU_SUN3_50

#ifndef CPU_SUN3_60
#define CPU_SUN3_60	0
#endif CPU_SUN3_60

int	siprobe(), sislave(), siattach(), sigo(), sidone(), sipoll();
int	siustart(), sistart(), si_getstatus(), si_deque();
int	siwatch(), si_off(), si_cmd(), si_cmdwait(), si_reset(), si_dmacnt();
char	*si_str_phase(), *si_str_lastphase();
static	u_char junk;
int si_phase_wait  = SI_PHASE_WAIT;

extern int scsi_ntype;
extern int scsi_disre_enable;	/* enable disconnect/reconnects */
extern int scsi_debug;		/* 0 = normal operaion
				 * 1 = enable error logging and error msgs.
				 * 2 = as above + debug msgs.
				 * 3 = enable all info msgs.
				 */
extern	u_char sc_cdb_size[];
extern	int nsi, scsi_host_id, scsi_reset_delay;
extern	struct scsi_ctlr sictlrs[];		/* per controller structs */
extern	struct scsi_unit_subr scsi_unit_subr[];
extern	struct mb_ctlr *siinfo[];
extern	struct mb_device *sdinfo[];
struct	mb_driver sidriver = {
	siprobe, sislave, siattach, sigo, sidone, sipoll,
	sizeof (struct scsi_si_reg), "sd", sdinfo, "si", siinfo, MDR_BIODMA,
};

/* routines available to devices specific portion of scsi driver */
struct scsi_ctlr_subr sisubr = {
	siustart, sistart, sidone, si_cmd, si_getstatus, si_cmdwait,
	si_off, si_reset, si_dmacnt, sigo, si_deque,
};

/* Log history of activity */
#define SI_LOG_PHASE(arg0, arg1, arg2)  {\
	c->c_last_phase = arg0; \
	c->c_phases[c->c_phase_index].phase = arg0; \
	c->c_phases[c->c_phase_index].target = arg1; \
	c->c_phases[c->c_phase_index].lun = arg2; \
	c->c_phase_index = (++c->c_phase_index) & (NPHASE -1); \
}

#ifdef SCSI_DEBUG
u_int si_nodvma  = 0;		/* # of times we had no dvma in sistart */
u_int si_noresel = 0;		/* # of times we lost reselects */
u_int si_winner  = 0;		/* # of times we had an intr at end of siintr */
u_int si_loser   = 0;		/* # of times we didn't have an intr at end */
#define SI_NODVMA	si_nodvma++
#define SI_NORESEL	si_noresel++
#define SI_WIN		si_winner++
#define SI_LOSE		si_loser++

/* Check for possible illegal SCSI-3 register access. */
#define SI_VME_OK(c, sir, str)	{\
	if ((IS_VME(c))  &&  (sir->csr & SI_CSR_DMA_EN)) \
		printf("si%d:  reg access during dma <%s>, csr 0x%x\n", \
			SINUM(c), str, sir);\
}
#define SI_DMA_OK(c, sir, str)  {\
	if (IS_VME(c)) { \
		if (sir->csr & SI_CSR_DMA_EN) \
			printf("%s: DMA DISABLED\n", str); \
		if (sir->csr & SI_CSR_DMA_CONFLICT) { \
			printf("%s: invalid reg access during dma\n", str); \
			DELAY(10000); \
		} \
		sir->csr &= ~SI_CSR_DMA_EN; \
	} \
}
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
#define SI_NODVMA
#define SI_NORESEL
#define SI_WIN
#define SI_LOSE
#define SI_VME_OK(c, sir, str)
#define SI_DMA_OK(c, sir, str)
#define DEBUG_DELAY(cnt)
#define DPRINTF(str) 
#define DPRINTF1(str, arg2) 
#define DPRINTF2(str, arg1, arg2) 
#define EPRINTF(str) 
#define EPRINTF1(str, arg2) 
#define EPRINTF2(str, arg1, arg2) 
#endif SCSI_DEBUG

/*
 * Print out the cdb.
 */
static
si_print_cdb(un)
	register struct scsi_unit *un;
{
	register u_char size, i;
	register u_char *cp = (u_char *) &un->un_cdb;

	/* If all else fails, use structure size */
	if ((size = sc_cdb_size[CDB_GROUPID(*cp)]) == 0  &&
	    (size = un->un_cmd_len) == 0)
		size = sizeof (struct scsi_cdb);

	for (i = 0; i < size; i++)
		printf("  %x", *cp++);
	printf("\n");
}


/*
 * returns string corresponding to the last phase.  Note, also encoded are
 * internal messages in addition to the last bus phase.  
 */
static char *
si_str_lastphase(phase)
	register short phase;
{
	static char *invalid_phase = "";
	static char *phase_strings[] = LAST_PHASE_STRING_DATA;

	if (phase >= PHASE_SPURIOUS) {
		if (phase > PHASE_LAST)
			return(invalid_phase);
		return(phase_strings[phase - PHASE_SPURIOUS]);
	} else {
		return(si_str_phase((u_char)phase));
	}
}


/*
 * returns string corresponding to the phase
 */
static char *
si_str_phase(phase)
	register u_char phase;
{
	register int index = (phase & CBSR_PHASE_BITS) >> 2;
	static char *phase_strings[] = PHASE_STRING_DATA;

	if (((phase & SBC_CBSR_BSY) == 0)  &&  (index == 0))
		return(phase_strings[8]);
	else
		return(phase_strings[index]);
}


/*
 * print out the current hardware state
 */
static
si_print_state(sir, c)
	register struct scsi_si_reg *sir;
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register short	x, z;
	register int bcr;
	int flag = 0;

	if (IS_VME(c)  &&  (sir->csr & SI_CSR_DMA_EN)) {
		sir->csr &= ~SI_CSR_DMA_EN;
		flag = 1;
	}
	if (IS_VME(c))
		bcr = GET_BCR(sir);
	else
		bcr = sir->bcr;

	printf("\tcsr= 0x%x  bcr= %d  tcr= 0x%x\n",
			sir->csr, bcr, SBC_RD.tcr);
	printf("\tcbsr= 0x%x (%s)  cdr= 0x%x  mr= 0x%x  bsr= 0x%x\n",
			SBC_RD.cbsr, si_str_phase(SBC_RD.cbsr), SBC_RD.cdr,
			SBC_RD.mr, SBC_RD.bsr);
#ifdef	SCSI_DEBUG
	printf("\tdriver wins= %d  loses= %d  nodvma= %d  noresel= %d\n",
		si_winner, si_loser, si_nodvma, si_noresel);
#endif	SCSI_DEBUG

	if (un != NULL) {
		register int dma_count;

		/* If in data phase, use bcr for dma count */
		if (un->un_flags & SC_UNF_DMA_ACTIVE)
			dma_count = bcr;
		else
			dma_count = un->un_dma_curcnt;

		printf("\ttarget= %d, lun= %d    ", un->un_target, un->un_lun);
		printf("DMA addr= 0x%x  count= %d (%d)\n",
			   un->un_dma_curaddr, dma_count, un->un_dma_count);
		printf("\tcdb=  ");
		si_print_cdb(un);
	}
	z = c->c_phase_index;
	for (x = 0; x < NPHASE; x++) {
		register short y;

		z = --z & (NPHASE -1);
		y = c->c_phases[z].phase;
		printf("\tlast phase= 0x%x (%s)", y, si_str_lastphase(y));
		if ( c->c_phases[z].target != -1)
			printf("   %d", c->c_phases[z].target);

		if ( c->c_phases[z].lun != -1)
			printf("   %d\n", c->c_phases[z].lun);
		else
			printf("\n");
	}
	if (flag)
		sir->csr |= SI_CSR_DMA_EN;

}

/*
 * This routine unlocks this driver when I/O has ceased, and a call
 * to sistart is * needed to start I/O up again.  Refer to  xd.c and
 * xy.c for similar routines upon which this routine is based.
 */
static
siwatch(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un;

	/*
	 * Find a valid un to call s?start with to for next command
	 * to be queued in case DVMA ran out.  Try the pending que
	 * then the disconnect que.  Otherwise, driver will hang
	 * if DVMA runs out.  The reason for this kludge is that
	 * sistart should really be passed c instead of un, but it
	 * would break lots of 3rd party S/W if this were fixed.
	 */
	un = (struct scsi_unit *)c->c_tab.b_actf;
	if (un == NULL)
		un = (struct scsi_unit *)c->c_disqtab.b_actf;

	if (un != NULL)
		sistart(un);
	timeout(siwatch, (caddr_t)c, 10*hz);
}



/*
 * Determine existence of SCSI host adapter.
 * Returns 0 for failure, size of si data structure if ok.
 */
siprobe(sir, ctlr)
	register struct scsi_si_reg *sir;
	register int ctlr;
{
	register struct scsi_ctlr *c = &sictlrs[ctlr];
	register int x;

	/* 
	 * Check for sbc - NCR 5380 Scsi Bus Ctlr chip.
	 * sbc is common to sun3/50 onboard scsi and vme
	 * scsi board.
	 */
	if (peekc((char *)&sir->sbc_rreg.cbsr) == -1)
		return (0);

	/*
	 * Determine whether the host adaptor interface is onboard or vme.
	 */
#ifdef	sun4
	if (cpu == CPU_SUN4_110)
		return (0);
#endif	sun4

	if (cpu == CPU_SUN3_50  ||  cpu == CPU_SUN3_60) {
		/* probe for sun3/50 or 3/60 dma interface */
		if (peek((short *)&sir->udc_rdata) == -1)
			return (0);

		c->c_flags = SCSI_PRESENT | SCSI_ONBOARD;
		c->c_udct = (struct udc_table *)rmalloc(iopbmap,
			     (long)(sizeof (struct udc_table) +4));
		if (c->c_udct == NULL) {
			printf("si%d:  siprobe: no space for inquiry data\n",
				SINUM(c));
			return(0);
		}
		/* align buffer */
		c->c_udct = (struct udc_table *)
			     (((u_int)c->c_udct + 0x03) & ~0x03);          

	} else {
		/*
		 * Probe for vme scsi card but make sure it is not the SC host
		 * adaptor interface. SI vme scsi host adaptor occupies
		 * 2K bytes in the vme address space. SC vme scsi host adaptor
		 * occupies 4K bytes in the vme address space. So, peek past
		 * 2K bytes to determine which host adaptor is there.
		 */
		if ((peek((short *)&sir->dma_addr) == -1)  ||
		    (peek((short *)((int)sir + 0x800)) != -1))
			return (0);

		/* Check for obsolete SCSI-3 boards */
		if (!(sir->csr & SI_CSR_ID)) {
			printf("si%d:  UNMODIFIED SCSI-3 BOARD!  PLEASE UPGRADE!\n",
				SINUM(c));
			return (0);
		}
		c->c_flags = SCSI_PRESENT;
	}
	EPRINTF2("si%d:  siprobe: sir= 0x%x,  ", ctlr, (u_int)sir);
	EPRINTF1("c= 0x%x\n", (u_int)c );

	if (scsi_disre_enable)
		c->c_flags |= SCSI_EN_DISCON;
        /*
         * Allocate a page for being able to
         * flush the last bit of a data transfer.
         */
        c->c_kment = rmalloc(kernelmap, (long)mmu_btopr(MMU_PAGESIZE));
        if (c->c_kment == 0) {
                printf("si%d: out of kernelmap for last DMA page\n", ctlr);
                return (0);
        }

	/* Initialize last phase */
	for (x = 0; x < NPHASE; x++) {
		c->c_phases[x].phase = PHASE_CMD_CPLT;
		c->c_phases[x].target = -1;
		c->c_phases[x].lun = -1;
	}
	c->c_phase_index = 0;
	SI_LOG_PHASE(PHASE_CMD_CPLT, -1, -1);
	c->c_drainun = (struct scsi_unit *) -1;	/* clear drain un */
	c->c_reg = (int)sir;
	c->c_ss = &sisubr;
	c->c_name = "si";
	c->c_tab.b_actf = NULL;
	c->c_disqtab.b_actf = NULL;
	c->c_un = NULL;
	c->c_tab.b_active = C_INACTIVE;		/* clear interlocks */
	c->c_intpri = 2;
	si_reset(c, NO_MSG);			/* quietly reset the scsi bus */
	timeout(siwatch, (caddr_t)c, 10*hz);
	return (sizeof (struct scsi_si_reg));
}


/*
 * See if a slave exists.
 * Since it may exist but be powered off, we always say yes.
 */
/*ARGSUSED*/
sislave(md, reg)
	register struct mb_device *md;
	register struct scsi_si_reg *reg;
{
	register struct scsi_unit *un;
	register int type;

#ifdef 	lint
	reg = reg; junk = junk; junk = scsi_debug;
#endif	lint

	/*
	 * This kludge allows autoconfig to print out "sd" for
	 * disks and "st" for tapes.  The problem is that there
	 * is only one md_driver for all scsi devices.
	 */
	type = TYPE(md->md_flags);
	if (type >= scsi_ntype)
		panic("sislave:  unknown type in md_flags\n");

	/* link unit to its controller */
	un = (struct scsi_unit *)(*scsi_unit_subr[type].ss_unit_ptr)(md);
	if (un == NULL)
		panic("sislave:  md_flags scsi type not configured\n");

	un->un_c = &sictlrs[md->md_ctlr];
	md->md_driver->mdr_dname = scsi_unit_subr[type].ss_devname;
	return (1);
}


/*
 * Attach device (boot time).
 */
siattach(md)
	register struct mb_device *md;
{
	register int type = TYPE(md->md_flags);
	register struct mb_ctlr *mc = md->md_mc;
	register struct scsi_ctlr *c = &sictlrs[md->md_ctlr];
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);

	/*DPRINTF("siattach:\n");*/
	if (type >= scsi_ntype)
		panic("siattach:  unknown type in md_flags\n");

	c->c_intpri = mc->mc_intpri;

	/* Call top-level driver s?attach routine */
	(*scsi_unit_subr[type].ss_attach)(md);

	if (IS_ONBOARD(c))
		return;
	/* 
	 * Initialize interrupt vector and address modifier register.
	 * Address modifier specifies standard supervisor data access
	 * with 24 bit vme addresses. May want to change this in the
	 * future to handle 32 bit vme addresses.
	 */
	sir->csr &= ~SI_CSR_DMA_EN;		/* disable DMA */
	if (mc->mc_intr) {
		/* setup for vectored interrupts - we will pass ctlr ptr */
		sir->iv_am = (mc->mc_intr->v_vec & 0xff) | 
		    VME_SUPV_DATA_24;
		(*mc->mc_intr->v_vptr) = (int)c;
	}
}


/*
 * corresponding to un is an md.  if this md has SCSI commands queued up
 * (md->md_utab.b_actf != NULL) and md is currently not active
 * (md->md_utab.b_active == 0), this routine queues the un on its queue of
 * devices (c->c_tab) for which to run commands Note that md is active if its
 * un is already queued up on c->c_tab, or its un is on the scsi_ctlr's
 * disconnect queue c->c_disqtab.
 */
siustart(un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct mb_device *md = un->un_md;
	int s;

	DPRINTF("siustart:\n");
	s = splr(pritospl(c->c_intpri));
	if (md->md_utab.b_actf != NULL  &&  md->md_utab.b_active == MD_INACTIVE) {
		if (c->c_tab.b_actf == NULL) 
			c->c_tab.b_actf = (struct buf *)un;
		else 
			((struct scsi_unit *)(c->c_tab.b_actl))->un_forw = un;

		un->un_forw = NULL;
		c->c_tab.b_actl = (struct buf *)un;
		md->md_utab.b_active |= MD_QUEUED;
	}
	(void) splx(s);
}

/*
 * this un is removed from the active
 * position of the scsi_ctlr's queue of devices (c->c_tab.b_actf) and
 * queued on scsi_ctlr's disconnect queue (c->c_disqtab).
 */
si_discon_queue (un)
	struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	/* DPRINTF("si_discon_queue:\n"); */

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
 * searches the scsi_ctlr's disconnect queue for the un which points to the
 * scsi_unit defined by (target,lun).  Then requeues this un on scsi_ctlr's
 * queue of devices.
 */
si_recon_queue (c, target, lun, flag)
	register struct scsi_ctlr *c;
	register short target, lun, flag;
{
	register struct scsi_unit *pun, *un;
	/*DPRINTF("si_recon_queue:\n");*/

	/* search the disconnect queue */
	for (un=(struct scsi_unit *)(c->c_disqtab.b_actf), pun=NULL; un;
	     pun=un, un=un->un_forw) {
		if (un->un_target == target  &&  un->un_lun == lun)
			break;
	}
	/*
	 * Could not find the device in the disconnect queue, die!
	 * Note, if flag set, just return fail as we're trying
	 * to figure out if we've started the request or not.
	 */
	if (un == NULL) {
		if (flag)
			return (FAIL);

		printf("si%d:  si_reconnect: can't find dis unit: target %d lun %d\n",
			SINUM(c), target, lun);
		/* dump out disconnnect queue */
		printf("  disconnect queue:\n");
		for (un = (struct scsi_unit *)(c->c_disqtab.b_actf), pun = NULL; un;
                     pun = un, un = un->un_forw) {
			printf("\tun = 0x%x  ---  target = %d,  lun = %d\n",
				un, un->un_target, un->un_lun);
		}
		return (FAIL);
	}
	/*
	 * If inhibit reconnect flag set, exit.  This is used by deque
	 * to determine whether to reset everything or ignore timeout.
	 */
	if (flag)
		return(OK);

	/* remove un from the disconnect queue */
	if (un == (struct scsi_unit *)(c->c_disqtab.b_actf))
		c->c_disqtab.b_actf = (struct buf *)(un->un_forw);
	else
		pun->un_forw = un->un_forw;

	if (un == (struct scsi_unit *)c->c_disqtab.b_actl)
		c->c_disqtab.b_actl = (struct buf *)pun;

	/* requeue un at the active position of scsi_ctlr's queue of devices. */
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

	/* If finished flushing disconnect que, clear interlock */
	if (c->c_disqtab.b_actf == NULL  &&  (c->c_tab.b_active & C_FLUSHING)) {
		c->c_tab.b_active &= ~C_FLUSHING;
		printf("si%d:  warning, scsi bus saturated\n", SINUM(c));
	}
	return(OK);
}


/* starts the next SCSI command */
sistart(un)
	struct scsi_unit *un;
{
	struct scsi_ctlr *c;
	struct mb_device *md;
	struct buf *bp;
	int s;
	/*DPRINTF("sistart:\n");*/

	/* return immediately if passed NULL un */
	if (un == NULL) {
		EPRINTF("sistart: un NULL\n");
		return;
	}
	/*
	 * return immediately, if the ctlr is already actively
	 * running a SCSI command
	 */
	c = un->un_c;
	s = splr(pritospl(c->c_intpri));
	if (c->c_tab.b_active != C_INACTIVE) {
		DPRINTF1("sistart: locked (0x%x)\n", c->c_tab.b_active);
		(void) splx(s);
		return;
	}
	/*
	 * if there are currently no SCSI devices queued to run
	 * a command, then simply return.  otherwise, obtain the
	 * next un for which a command should be run.
	 */
	if ((un=(struct scsi_unit *)(c->c_tab.b_actf)) == NULL) {
		/*DPRINTF("sistart: no device to run\n");*/
		(void) splx(s);
		return;
	}
	md = un->un_md;
	c->c_tab.b_active |= C_QUEUED;
	/*
	 * if an attempt was already made to run this command, but the
	 * attempt was pre-empted by a SCSI bus reselection then DVMA
	 * has already been set up, and we can call sigo directly.
	 */
	if (md->md_utab.b_active & MD_PREEMPT) {
		/*DPRINTF("sistart: starting pre-empted");*/
		c->c_un = un;
		sigo(md);
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}
	if (md->md_utab.b_active & MD_IN_PROGRESS) {
		/*DPRINTF("sistart: md in progress\n");*/
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}
	md->md_utab.b_active |= MD_IN_PROGRESS;
	bp = md->md_utab.b_actf;
	md->md_utab.b_forw = bp;

	/* only happens when called by intr */
	if (bp == NULL) {
		EPRINTF("sistart: bp is NULL\n");
		c->c_tab.b_active &= C_ACTIVE;
		(void) splx(s);
		return;
	}
	/*
	 * special commands which are initiated by the high-level driver,
	 * are run using its special buffer, un->un_sbuf.  In most cases,
	 * main bus set-up has already been done, so sigo can be called
	 * for on-line formatting, we need to call mbsetup.
	 */
	if ((*un->un_ss->ss_start)(bp, un)) {
		c->c_un = un;
		if (bp == &un->un_sbuf  &&
		    ((un->un_flags & SC_UNF_DVMA) == 0)  &&
		    ((un->un_flags & SC_UNF_SPECIAL_DVMA) == 0)) {
			sigo(md);
		} else { 
			if (mbugo(md) == 0) {
				md->md_utab.b_active |= MD_NODVMA;
				SI_NODVMA;
			}
		}
	} else {
		sidone(md);
	}
	c->c_tab.b_active &= C_ACTIVE;
	(void) splx(s);
	return;
}



/*
 * Start up a scsi operation.
 * Called via mbgo after buffer is in memory.
 */
sigo(md)
	register struct mb_device *md;
{
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register struct buf *bp;
	register int unit;
	register int err;
	int s, type;
	/*DPRINTF("sigo:\n"); */

	if (md == NULL)
		panic("sigo:  queueing error1\n");

	c = &sictlrs[md->md_mc->mc_ctlr];
	s = splr(pritospl(c->c_intpri));

	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);

	bp = md->md_utab.b_forw;
	if (bp == NULL) {
		EPRINTF("sigo: bp is NULL\n");
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

	/* Diddle stats if necessary. */
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

	if ((err=si_cmd(c, un, 1)) != OK) {
		if (err == FAIL) {
			/* DPRINTF("sigo:  si_cmd FAILED\n"); */
			(*un->un_ss->ss_intr)(c, 0, SE_RETRYABLE);
		} else if (err == HARD_FAIL) {
			DPRINTF("sigo:  si_cmd hard FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
		} else if (err == SCSI_FAIL) {
			DPRINTF("sigo:  si_cmd scsi FAIL\n");
			(*un->un_ss->ss_intr)(c, 0, SE_FATAL);
			si_off(c, un, SE_FATAL);
		}
	}
	c->c_tab.b_active &= C_ACTIVE;
	(void) splx(s);
}


/*
 * Handle a polling SCSI bus interrupt.
 */
sipoll()
{
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *sir;
	register int serviced = 0;
	/* DPRINTF("sipoll:\n"); */

	for (c = sictlrs; c < &sictlrs[nsi]; c++) {
		if ((c->c_flags & SCSI_PRESENT) == 0)
			continue;
		sir = (struct scsi_si_reg *)(c->c_reg);
		if ((sir->csr & (SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT)) == 0)
			continue;

		EPRINTF("sipoll: go\n");
		serviced = 1;
		siintr(c);
	}
	return (serviced);
}

/*
 * The SCSI command is done, so start up the next command.
 */
sidone (md)
	struct mb_device *md;
{
	struct scsi_ctlr *c;
	struct scsi_unit *un;
	struct buf *bp;
	int type, s;
	DPRINTF("sidone:\n");

	c = &sictlrs[md->md_mc->mc_ctlr];
	s = splr(pritospl(c->c_intpri));
	bp = md->md_utab.b_forw;

	/* more reliable than un = c->c_un; */
	type = TYPE(md->md_flags);
	un = (struct scsi_unit *)
	     (*scsi_unit_subr[type].ss_unit_ptr)(md);

	/* advance mb_device queue */
	md->md_utab.b_actf = bp->av_forw;

	/*
	 * we are done, so clear buf in active position of 
	 * md's queue. then call iodone to complete i/o
	 */
	md->md_utab.b_forw = NULL;

	/*
	 * just got done with i/o so mark ctlr inactive 
	 * then advance to next md on the ctlr.
	 */
	c->c_tab.b_actf = (struct buf *)(un->un_forw);

	/*
	 * advancing the ctlr's queue has removed un from
	 * the queue.  if there are any more i/o for this
	 * md, siustart will queue up md again. at tail.
	 * first, need to mark md as inactive (not on queue)
	 */
	md->md_utab.b_active = MD_INACTIVE;
	siustart(un);
	iodone(bp);

	/* Start up the next command on the scsi_ctlr */
	sistart(un);
	(void) splx(s);
}


/*
 * Bring a unit offline.  Note, if unit already offline, don't print anything.
 * If flag = SE_FATAL, take device offline.
 */
/*ARGSUSED*/
si_off(c, un, flag)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	int flag;
{
	register struct mb_device *md = un->un_md;
	char *msg = "online";

	if (un->un_present) {
		if (flag == SE_FATAL) {
			msg = "offline";
			un->un_present = 0;
		}
		printf("si%d:  %s%d, unit %s\n", SINUM(c),
		 	scsi_unit_subr[md->md_flags].ss_devname,
		      	un->un_unit, msg);
	}
}


/*
 * Pass a command to the SCSI bus.
 * Returns OK if successful, FAIL for (maybe) retryable failure,
 * HARD_FAIL for unrecoverable failure, and SCSI_FAIL if we failed due to
 * timing problem with SCSI bus. RESEL_FAIL is returned if we failed due to
 * target being in process of reselecting us.
 * (posponed til after reconnect done)
 */
si_cmd(c, un, intr)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
	register int intr;		/* if 0, use polled mode
					 * if 1, use interrupts
					 */
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct mb_device *md = un->un_md;
	register int err, i;
	register u_char size;
	u_char msg;
	int s;
	DPRINTF("si_cmd:\n");

	/* disallow disconnects if waiting for command completion */
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
	}
	/* Check for odd-byte boundary buffer */
	if ((un->un_flags & SC_UNF_DVMA)  &&  (un->un_dma_addr & 0x1)) {
		printf("si%d:  illegal odd byte DMA, address= 0x%x\n",
			SINUM(c), un->un_dma_curaddr);
		return (HARD_FAIL);
	}
	/*
	 * Determine size of the cdb.  Since we're smart, we look at group
	 * code and guess.  If we don't recognize the group id, we use the
	 * specified cdb length.  If both are zero, use max. size of
	 * data structure.
	 */
	if ((size = sc_cdb_size[CDB_GROUPID(un->un_cdb.cmd)]) == 0  &&
	    (size = un->un_cmd_len) == 0) {
		printf("si%d:  setting cdb size= %d\n",
			SINUM(c), sizeof (struct scsi_cdb));
		size = sizeof (struct scsi_cdb);
	}
	/*
	 * For vme host adaptor interface, dma enable bit may be set to allow
	 * reconnect interrupts to come in.  This must be disabled before
	 * arbitration/selection of target is done. Don't worry about
	 * re-enabling dma. If arb/sel fails, then si_idle will re-enable.
	 * If arb/sel succeeds then handling of command will re-enable.
	 *
	 * Also, disallow sbc to accept reconnect attempts.  Again, si_idle
	 * will re-enable this if arb/sel fails.  If arb/sel succeeds then
	 * we do not want to allow reconnects anyway.
	 */
	s = splr(pritospl(c->c_intpri));
	if ((intr == 1)  &&  (c->c_tab.b_active >= C_ACTIVE)) {
		md->md_utab.b_active |= MD_PREEMPT;
		(void) splx(s);
		return (OK);
	}
	c->c_tab.b_active |= C_ACTIVE;

	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;
	SI_VME_OK(c, sir, "start of si_cmd");

	/* performing target selection */
	if ((err = si_arb_sel(c, sir, un)) != OK) {
		/* 
		 * May not be able to execute this command at this time due
		 * to a target reselecting us. Indicate this in the unit
		 * structure for when we perform this command later.
		 */
		if (err == RESEL_FAIL) {
			md->md_utab.b_active |= MD_PREEMPT;
			err = OK;	/* not an error */
		} else {
			c->c_tab.b_active &= C_QUEUED;
		}
		if (IS_VME(c))
			sir->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
		return (err);
	}
	SBC_WR.ser = 0;			/* clear (re)sel int */
	SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int */
	c->c_un = un;
	(void) splx(s);
	/*
	 * Must split dma setup into 2 parts due to sun3/50 which requires
	 * bcr to be set before target changes phase on scsi bus to data
	 * phase.  Three fields in the per scsi unit structure hold
	 * information pertaining to the current dma operation:
	 * un_dma_curdir, un_dma_curaddr, and un_dma_curcnt. These fields
	 * are used to track the amount of data dma'd especially when
	 * disconnects and reconnects occur.  If the current command does
	 * not involve dma, these fields are set appropriately.
	 *
	 * Currently we don't use all 24 bits of the count register on the
	 * vme interface. To do this changes are required other places,
	 * e.g. in the scsi_unit structure the fields un_dma_curcnt and
	 * un_dma_count would need to be changed.
	 */
	un->un_flags |= SC_UNF_DMA_INITIALIZED;
	un->un_flags &= ~SC_UNF_DMA_ACTIVE;
	if (un->un_dma_count != 0) {

		/* Set DMA direction flag */
		if (un->un_flags & SC_UNF_RECV_DATA)
			un->un_dma_curdir = SI_RECV_DATA;
		else
			un->un_dma_curdir = SI_SEND_DATA;

		/* save current dma info for disconnect */
		un->un_dma_curaddr = un->un_dma_addr;
		un->un_dma_curcnt = un->un_dma_count;
		un->un_dma_curbcr = 0;

		if (IS_VME(c))
			si_vme_dma_setup(c, un);
		else
			si_ob_dma_setup(c, un);
	} else {
		un->un_dma_curdir = SI_NO_DATA;
		un->un_dma_curaddr = 0;
		un->un_dma_curcnt = 0;
	}

RETRY_CMD_PHASE:
	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_COMMAND  ||
	    si_wait_phase(sir, PHASE_COMMAND) == OK) {
		if (si_putdata(c, PHASE_COMMAND, (u_char *)&un->un_cdb,
		    size, intr) != OK) {
			printf("si%d:  si_cmd: cmd put failed\n", SINUM(c));
			si_reset(c, PRINT_MSG);
			c->c_tab.b_active &= C_QUEUED;
			return(FAIL);
		}
		SI_LOG_PHASE(PHASE_COMMAND, un->un_cdb.cmd, -1);

	/*
	 * Handle synchronous messages (6 bytes) and other unknown
	 * messages.  Note, all extended messages are rejected.
	 */
	} else {
		register u_char *icrp = &SBC_WR.icr;

		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_IN) {
			*icrp = SBC_ICR_ATN;
			msg = si_getdata(c, PHASE_MSG_IN);
			EPRINTF1("si_cmd:  rejecting msg 0x%x\n", msg);

			i = 255; /* accept 255 message bytes (overkill) */
			while((si_getdata(c, PHASE_MSG_IN) != -1)  &&  --i);

			if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_OUT) {
				msg = SC_MSG_REJECT;
				*icrp = 0;		/* turn off ATN */
				(void) si_putdata(c, PHASE_MSG_OUT, &msg, 1, 0);
			}
			/* Should never fail this check. */
			if( i > 0 )
				goto RETRY_CMD_PHASE;
		}
		/* target is skipping cmd phase, resynchronize... */
		if (SBC_RD.cbsr & SBC_CBSR_REQ) {
#ifdef			SCSI_DEBUG
			printf("si_cmd:  skipping cmd phase\n");
			si_print_state(sir, c);
#endif			SCSI_DEBUG
			goto SI_CMD_EXIT;
		}
		/* we've had a target failure, report it and quit */
		printf("si%d:  si_cmd: no command phase\n", SINUM(c));
		si_reset(c, PRINT_MSG);
		c->c_tab.b_active &= C_QUEUED;
		return (FAIL);
	}
	SBC_WR.ser = scsi_host_id;		/* enable (re)sel int */


	/* If not polled I/O mode, we're done */
SI_CMD_EXIT:
	if (intr) {
		sir->csr |= SI_CSR_INTR_EN;
		if (IS_VME(c))
			sir->csr |= SI_CSR_DMA_EN;
		return (OK);
	}

	/* 
	 * Polled SCSI data transfer mode.
	 */
	sir->csr &= ~SI_CSR_INTR_EN;
	if (un->un_dma_curdir != SI_NO_DATA) {
		register u_char phase;

		if (un->un_dma_curdir == SI_RECV_DATA)
			phase = PHASE_DATA_IN;
		else
			phase = PHASE_DATA_OUT;
		/*
		 * Check if we have a problem with the command not going into
		 * data phase.  If we do, then we'll skip down and get any
		 * status.  Of course, this means that the command failed.
		 */
		if ((si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		     SI_LONG_WAIT, 10, 1) == OK)  &&
		     ((SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)) {
			/*
			 * Wait for DMA to finish.  If it fails,
			 * attempt to get status and report failure.
			 */
			si_sbc_dma_setup(c, sir);
			if ((err=si_cmdwait(c)) != OK) {
				if (err != SCSI_FAIL) {
					EPRINTF("si_cmd:  cmdwait failed\n");
					msg = si_getstatus(un);
				}
				c->c_tab.b_active &= C_QUEUED;
				return (err);
			}
		} else {
			EPRINTF("si_cmd:  skipping data phase\n");
		}
	}
	/*
	 * Get completion status for polled command.  Note, if <0, it's
	 * retryable; if 0, it's fatal.  Someday I should give polled status
	 * results more options.  For now, everything is FATAL.
	 */
	if ((err=si_getstatus(un)) <= 0) {
		c->c_tab.b_active &= C_QUEUED;
		if (err == 0)
			return (HARD_FAIL);
		else
			return (FAIL);
	}
	/*sir->csr |= SI_CSR_INTR_EN;*/
	c->c_tab.b_active &= C_QUEUED;
	return (OK);
}


/*
 * Perform the SCSI arbitration and selection phases.
 * Returns FAIL if unsuccessful, returns RESEL_FAIL if unsuccessful due to
 * target reselecting, returns OK if all was cool.
 */
static
si_arb_sel(c, sir, un)
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *sir;
	register struct scsi_unit *un;
{
	register u_char *icrp = &SBC_WR.icr;
	register u_char *mrp = &SBC_WR.mr;
	register u_char icr;
	u_char id;
	int ret_val, s, i, j;
	int sel_retries = 0;
	/* DPRINTF("si_arb_sel:\n"); */

	/* 
	 * The following 3 lines prevent 5380 from getting onto
	 * the SCSI bus too soon.
	 * CRITICAL CODE SECTION DON'T TOUCH
	 */
	SBC_WR.tcr = 0;
	*mrp &= ~SBC_MR_ARB;	/* turn off arb */
	*icrp = 0;
	SBC_WR.odr = scsi_host_id;

	/* arbitrate for the scsi bus */
	for (i = 0; i < SI_ARB_RETRIES; i++) {

		/* wait for scsi bus to become free */
		for (j = 0; j < SI_WAIT_COUNT/4; j++) {
			if ((SBC_RD.cbsr & SBC_CBSR_BSY) == 0)
				goto SI_ARB_SEL_FREE;

			/* Check for reselect */
			if ((SBC_RD.cbsr & SBC_CBSR_RESEL) == SBC_CBSR_RESEL  &&
			    (SBC_RD.cdr & scsi_host_id)) {
				ret_val = RESEL_FAIL;
				goto SI_ARB_SEL_EXIT;
			}
			DELAY (40);
		}
		printf("si%d:  si_arb_sel: scsi bus continuously busy\n",
			SINUM(c));
		si_reset(c, PRINT_MSG);
		ret_val = FAIL;		/* may be retryable */
		goto SI_ARB_SEL_EXIT;

		/* Start arbitration and do some early setup of things.  */
SI_ARB_SEL_FREE:
		*mrp |= SBC_MR_ARB;		/* turn on arb */
		icr = SBC_ICR_SEL | SBC_ICR_BUSY | SBC_ICR_DATA;
		if (c->c_flags & SCSI_EN_DISCON)
			icr |= SBC_ICR_ATN;

		/* wait for sbc to begin arbitration */
		if (si_sbc_wait((caddr_t)icrp, SBC_ICR_AIP | SBC_ICR_LA,
		    SI_ARB_WAIT, 10, 1) != OK)
			goto SI_ARB_SEL_RETRY;

		/* check to see if we won arbitration */
		s = splr(pritospl(7));		/* time critical */
		DELAY (SI_ARBITRATION_DELAY);
		if ((*icrp & SBC_ICR_LA) == 0  && 
		    ((SBC_RD.cdr & ~scsi_host_id) < scsi_host_id)) {
			/*
			 * WON ARBITRATION!  Perform selection.
			 */
			*icrp = icr;
			*mrp &= ~SBC_MR_ARB;	/* turn off arb */
			DELAY (SI_BUS_CLEAR_DELAY + SI_BUS_SETTLE_DELAY);
			SBC_WR.odr = (1 << un->un_target) | scsi_host_id;
			*icrp &= ~SBC_ICR_BUSY;
			(void) splx(s);

			/* wait for target to acknowledge selection */
			for (j = 0; j < SI_SHORT_WAIT *10; j++) {
				if (SBC_RD.cbsr & SBC_CBSR_BSY) {
					*icrp &= ~(SBC_ICR_SEL|SBC_ICR_DATA);
					goto SI_ARB_SEL_WON;
				}
				DELAY (1);
			}
			/* Target failed selection */
			DPRINTF("si_arb_sel:  busy never set\n");
			*icrp = 0;
			if (un->un_present) {
				if (sel_retries++ < SI_SEL_RETRIES)
					goto SI_ARB_SEL_RETRY;
				printf("si%d:  si_arb_sel: failed selection\n",
					SINUM(c));
#ifdef				SCSI_DEBUG
				si_print_state(sir, c);
#endif				SCSI_DEBUG
			}
			ret_val = SCSI_FAIL;
			goto SI_ARB_SEL_EXIT;
		}
		(void) splx(s);

SI_ARB_SEL_RETRY:
		/*
		 * Arbitration may not begin due to target reselection,
		 * external SCSI bus reset, or we lost to another initiator.
		 */
		*mrp &= ~SBC_MR_ARB;	/* turn off arb */
		/*
		 * If RESET doesn't clear, we have a cable problem
		 */
		if (SBC_RD.cbsr & SBC_CBSR_RST) {
			if (si_sbc_wait((caddr_t)&SBC_RD.cbsr,
			    SBC_CBSR_RST, SI_SHORT_WAIT/4, 40, 0) != OK) {
				EPRINTF1("si%d:  si_cmd: hardware failure, check cable\n",
					SINUM(c));
				ret_val = SCSI_FAIL;
			} else {
				EPRINTF("si_arb_sel: external reset\n");
				ret_val = RESEL_FAIL;
			}
			goto SI_ARB_SEL_EXIT;
		/*
		 * Check for reselect
		 */
		} else if (((SBC_RD.cbsr & SBC_CBSR_RESEL) ==
		       SBC_CBSR_RESEL)  &&  (SBC_RD.cdr & scsi_host_id)) {
			ret_val = RESEL_FAIL;
			goto SI_ARB_SEL_EXIT;
		/*
		 * If Busy isn't set, we have a cable problem
		 */
		} else if (si_sbc_wait((caddr_t)&SBC_RD.cbsr,
		   SBC_CBSR_BSY, SI_SHORT_WAIT/4, 40, 1) != OK) {
			EPRINTF1("si%d:  si_cmd: hardware failure, check cable\n",
				SINUM(c));
			ret_val = SCSI_FAIL;
			goto SI_ARB_SEL_EXIT;
		}
		EPRINTF("si_arb_sel:  lost arbitration\n");
	}
	/* FAILED ARBITRATION even with retries.  */
	*icrp = 0;
	printf("si%d:  si_arb_sel: never won arbitration, \n", SINUM(c));
	si_reset(c, PRINT_MSG);
	ret_val = FAIL;		/* may be retryable */

SI_ARB_SEL_EXIT:
	SBC_WR.tcr = TCR_UNSPECIFIED;
	junk = SBC_RD.clr;		/* clear int */
	return (ret_val);


SI_ARB_SEL_WON:
	/* If SEL not dropped, assume bad cable */
	if (SBC_RD.cbsr & SBC_CBSR_SEL) {
		EPRINTF("si_arb_sel:  SEL still active\n");
		ret_val = SCSI_FAIL;
		goto SI_ARB_SEL_EXIT;
	}

	/* If disconnects enabled, tell target it's ok to do it. */
	SI_LOG_PHASE(PHASE_ARBITRATE, un->un_target, un->un_lun);
	if (c->c_flags & SCSI_EN_DISCON) {

		/* DPRINTF("si_arb_sel:  disconnects ENABLED\n"); */
		id = SC_DR_IDENTIFY | un->un_cdb.lun;
		SBC_WR.tcr = TCR_MSG_OUT;
		if ((SBC_RD.cbsr & (CBSR_PHASE_BITS | SBC_CBSR_REQ))
		    == (PHASE_MSG_OUT | SBC_CBSR_REQ)  ||
 		    si_wait_phase(sir, PHASE_MSG_OUT) == OK) {
			*icrp = 0;		/* turn off ATN */
			if (si_putdata(c, PHASE_MSG_OUT, &id, 1, 0) != OK) {
				EPRINTF1("si%d:  si_cmd: id msg failed\n",
					SINUM(c));
		               si_reset(c, PRINT_MSG);
				return(FAIL);
			}
		} else {
			printf("si%d:  target skipped identify MSG\n", SINUM(c));
		}
	}
	return (OK);
}


/*
 * Set up the SCSI control logic for a dma transfer for vme host adaptor.
 */
static
si_vme_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);

	/* sir->csr &= ~SI_CSR_DMA_EN; */

	/* Set dma direction */
	if (un->un_dma_curdir == SI_RECV_DATA)
		sir->csr &= ~SI_CSR_SEND;
	else
		sir->csr |= SI_CSR_SEND;

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

	/* set up byte packing control info */
	if (un->un_dma_curaddr & 0x2) {
		/*DPRINTF("si_vme_dma_setup: word xfer\n");*/
		sir->csr |= SI_CSR_BPCON;	/* 16-bit data transfers */
	} else {
		/*DPRINTF("si_vme_dma_setup: long word xfer\n");*/
		sir->csr &= ~SI_CSR_BPCON;	/* 32-bit data transfers */
	}
	/*
	 * setup starting dma address and number bytes to dma
	 * Note, the dma count is set to zero to prevent it from
	 * starting up.  It will be set later in si_sbc_dma_setup
	 */
	SET_DMA_ADDR(sir, un->un_dma_curaddr);
	SET_DMA_COUNT(sir, 0);

}


/*
 * Set up the SCSI control logic for a dma transfer for onboard host adaptor.
 */
static
si_ob_dma_setup(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct udc_table *udct = c->c_udct;
	register int addr;

	if (un->un_dma_curdir == SI_RECV_DATA)
		sir->csr &= ~SI_CSR_SEND;
	else
		sir->csr |= SI_CSR_SEND;

	/* Set bcr */
	sir->bcr = un->un_dma_curcnt;

	/* reset udc */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_COMMAND;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_CMD_RESET;;
	DELAY(SI_UDC_WAIT);

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;

	/* set up udc dma information */
	addr = un->un_dma_curaddr;
	if (addr < DVMA_OFFSET)
		addr += DVMA_OFFSET;

	udct->haddr = ((addr & 0xff0000) >> 8) | UDC_ADDR_INFO;
	udct->laddr = addr & 0xffff;
	udct->hcmr = UDC_CMR_HIGH;
	udct->count = un->un_dma_curcnt / 2; /* #bytes -> #words */

	if (un->un_dma_curdir == SI_RECV_DATA) {
		DPRINTF("si_ob_dma_setup:  RECEIVE DMA\n");
		udct->rsel = UDC_RSEL_RECV;
		udct->lcmr = UDC_CMR_LRECV;
	} else {
		DPRINTF("si_ob_dma_setup:  SEND DMA\n");
		udct->rsel = UDC_RSEL_SEND;
		udct->lcmr = UDC_CMR_LSEND;
		if (un->un_dma_curcnt & 1)
			udct->count++;
	}

	/* initialize udc chain address register */
	sir->udc_raddr = UDC_ADR_CAR_HIGH;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = ((int)udct & 0xff0000) >> 8;
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_CAR_LOW;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = (int)udct & 0xffff;

	/* initialize udc master mode register */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_MODE;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_MODE;

	/* issue channel interrupt enable command, in case of error, to udc */
	DELAY(SI_UDC_WAIT);
	sir->udc_raddr = UDC_ADR_COMMAND;
	DELAY(SI_UDC_WAIT);
	sir->udc_rdata = UDC_CMD_CIE;
	DELAY(SI_UDC_WAIT);
}


/*
 * Setup and start the sbc for a dma operation.
 */
static
si_sbc_dma_setup(c, sir)
	register struct scsi_ctlr *c;
	register struct scsi_si_reg *sir;
{
	register struct scsi_unit *un = c->c_un;
	register int s;

	SI_VME_OK(c, sir, "si_sbc_dma_setup");

	un->un_flags |= SC_UNF_DMA_ACTIVE;
	un->un_dma_curbcr = un->un_dma_curcnt;
	if (IS_VME(c)) {
		SET_DMA_COUNT(sir, un->un_dma_curcnt);
		SET_BCR(sir, un->un_dma_curcnt);
	}else {
		/* issue start chain command to udc */
		sir->udc_rdata = UDC_CMD_STRT_CHN;
		DELAY(SI_UDC_WAIT);
	}

	if (un->un_dma_curdir == SI_RECV_DATA) {
		/* DPRINTF("si_sbc_dma_setup:  RECEIVE DMA\n"); */
		SI_LOG_PHASE(PHASE_DATA_IN, un->un_dma_curcnt, -1);

		/* CRITICAL CODE SECTION DON'T TOUCH */
		s = splr(pritospl(7));	/* time critical */
		SBC_WR.tcr = TCR_DATA_IN;
		junk = SBC_RD.clr;      /* clear intr */
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.ircv = 0;
		if (IS_VME(c))
			sir->csr |= SI_CSR_DMA_EN;
		(void) splx(s);

	} else {
		/* DPRINTF("si_sbc_dma_setup:  XMIT DMA\n"); */
		SI_LOG_PHASE(PHASE_DATA_OUT | SBC_CBSR_BSY,
			     un->un_dma_curcnt, -1);

		/* CRITICAL CODE SECTION DON'T TOUCH */
		s = splr(pritospl(7));	/* time critical */
		SBC_WR.tcr = TCR_DATA_OUT;
		junk = SBC_RD.clr;      /* clear intr */
		SBC_WR.icr = SBC_ICR_DATA;
		SBC_WR.mr |= SBC_MR_DMA;
		SBC_WR.send = 0;
		if (IS_VME(c))
			sir->csr |= SI_CSR_DMA_EN;
		(void) splx(s);
	}
}


/*
 * Cleanup up the SCSI control logic after a dma transfer.
 */
static
si_dma_cleanup(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	DPRINTF("si_dma_cleanup:\n");

	/* disable dma controller */
	if (IS_VME(c)) {
		sir->csr &= ~SI_CSR_DMA_EN;
		SET_DMA_COUNT(sir, 0);
		sir->bcrh = sir->bcr = 0;
	} else {
		DELAY(SI_UDC_WAIT);
		sir->udc_raddr = UDC_ADR_COMMAND;
		DELAY(SI_UDC_WAIT);
		sir->udc_rdata = UDC_CMD_RESET;
		DELAY(SI_UDC_WAIT);
		sir->bcr = 0;
	}
	sir->csr &= ~SI_CSR_SEND;

	/* reset fifo */
	sir->csr &= ~SI_CSR_FIFO_RES;
	sir->csr |= SI_CSR_FIFO_RES;
}

/*
 * Handle special dma receive situations, e.g. an odd number of bytes in
 * a dma transfer.  The Sun3/50 onboard interface has different situations
 * which must be handled than the vme interface.  Note, this is only
 * called by si_disconnect.
 * Returns OK if sucessful; Otherwise FAIL.
 */
static
si_dma_recv(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct scsi_unit *un = c->c_un;
	register int bcr, offset;

	/*DPRINTF("si_dma_recv:\n");*/
	un->un_flags &= ~SC_UNF_DMA_ACTIVE;
	offset = un->un_dma_curaddr + (un->un_dma_curcnt - un->un_dma_curbcr);

	/* handle the onboard scsi situations */
	if (IS_ONBOARD(c)) {
		bcr = sir->bcr;
		sir->udc_raddr = UDC_ADR_COUNT;

		/* wait for the fifo to empty */
		if (si_wait((u_short *)&sir->csr, SI_CSR_FIFO_EMPTY, 1) != OK) {
			printf("si%d:  fifo never emptied\n", SINUM(c));
			return (FAIL);
		}
		/*
		 * Didn't transfer any data.  "Just say no" and leave,
		 * rather than erroneously executing left over byte code.
		 * The bcr +1 above wards against 5380 prefetch.
		 */
		if (bcr == un->un_dma_curcnt  ||  (bcr +1) == un->un_dma_curcnt)
			return (OK);

		/* handle odd byte */
		if ((un->un_dma_curcnt - bcr) & 1) {
			DVMA[offset -1] = (sir->fifo_data & 0xff00) >> 8;
			DPRINTF1("si_dma_recv:  byte left, 0x%x\n",
				 sir->fifo_data);
		/*
		 * The udc may not dma the last word from the fifo_data
		 * register into memory due to how the hardware turns
		 * off the udc at the end of the dma operation.
		 */
		} else if ((sir->udc_rdata*2) - bcr == 2) {
			DVMA[offset -2] = (sir->fifo_data & 0xff00) >> 8;
			DVMA[offset -1] = sir->fifo_data & 0x00ff;
			DPRINTF1("si_dma_recv:  word left, 0x%x\n",
				 sir->fifo_data);
		}

	/* handle the vme scsi situations */
	} else if ((sir->csr & SI_CSR_LOB) != 0) {
	    /*
	     * Grabs last few bytes which may not have been dma'd.  Worst
	     * case is when longword dma transfers are being done and there
	     * are 3 bytes leftover.  If BPCON bit is set then longword dma
	     * was being done, otherwise word dma was being done.
	     */
	     DPRINTF2("si_dma_recv:  offset= 0x%x curcnt= 0x%x",
			 offset, un->un_dma_curcnt);
	     DPRINTF1(" curbcr= 0x%x\n", un->un_dma_curbcr);

#ifdef IOC
	    /*
	     * If there is an IO Cache, these odd bytes will
	     * not be in it.  So we must flush the IOC line before
	     * we read the odd bytes into the buffer, else when the
	     * IOC line is flushed at buffer release time the odd
	     * bytes would overwritten by extra garbage in the IOC line.
	     *
	     * The 2nd flush at buffer release time will be
	     * harmless since the line will no longer be dirty.
	     */
#ifdef SUN3X_470
	    if(iocenable) {
		int linenum = ((offset - 1) >> MMU_PAGESHIFT);
		ioc_flush(linenum);
	    }
#else SUN3X_470
	    if(ioc) {
		register int linenum = (((offset - 1) >> MMU_PAGESHIFT) &
				(IOC_CACHE_LINES - 1));

		ioc_flush(linenum);
	    }
#endif SUN3X_470
#endif IOC
	    if ((sir->csr & SI_CSR_BPCON) == 0) {
		switch (sir->csr & SI_CSR_LOB) {
		case SI_CSR_LOB_THREE:
		    if (MBI_MR(offset) < dvmasize) {
			DVMA[offset -3] = (sir->bprh & 0xff00) >> 8;
			DVMA[offset -2] = sir->bprh & 0x00ff;
			DVMA[offset -1] = (sir->bprl & 0xff00) >> 8;
		    }
		    else {
			si_flush_vmebyte(c, (offset -3),
				 ((sir->bprh & 0xff00) >> 8));
			si_flush_vmebyte(c, (offset -2), (sir->bprh & 0x00ff));
			si_flush_vmebyte(c, (offset -1),
				 ((sir->bprl & 0xff00) >> 8));
		    }
	  	    DPRINTF1("si_dma_recv: 3 bytes left, 0x%x\n", sir->bprh);
		    break;
		case SI_CSR_LOB_TWO:
		    if (MBI_MR(offset) < dvmasize) {
			DVMA[offset -2] = (sir->bprh & 0xff00) >> 8;
			DVMA[offset -1] = sir->bprh & 0x00ff;
		    }
		    else {
			si_flush_vmebyte(c, (offset -2),
				 ((sir->bprh & 0xff00) >> 8));
			si_flush_vmebyte(c, (offset -1), (sir->bprh & 0x00ff));
		    }
		    DPRINTF1("si_dma_recv: 2 bytes left, 0x%x\n", sir->bprh);
		    break;

		case SI_CSR_LOB_ONE:
		    if (MBI_MR(offset) < dvmasize) {
			DVMA[offset -1] = (sir->bprh & 0xff00) >> 8;
		    }
		    else {
			si_flush_vmebyte(c, (offset -1),
				 ((sir->bprh & 0xff00) >> 8));
		    }
		    DPRINTF1("si_dma_recv: 1 bytes left, 0x%x\n", sir->bprh);
		    break;
		}
	    } else {
		if (MBI_MR(offset) < dvmasize) {
			DVMA[offset - 1] = (sir->bprl & 0xff00) >> 8;
		}
		else {
			si_flush_vmebyte(c, (offset - 1), ((sir->bprl & 0xff00) >> 8));
		}
		DPRINTF1("si_dma_recv:  1 byte left, 0x%x\n", sir->bprl);
	    }
	}
	return (OK);
}

si_flush_vmebyte(c, offset, byte)
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
        DPRINTF2("si_flush_vmebyte: mapaddr = %x, byte= %x\n", mapaddr, byte);
        segkmem_mapout(&kseg,
            (addr_t) (((int)mapaddr) & PAGEMASK), (u_int) mmu_ptob(1));
}

/*
 * Handle a scsi interrupt.
 */
siintr(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct scsi_unit *un;
	register u_char *cp;
	register int resid = 0;
	int bcr, status, i, s;
	short lun;
	u_char msg;
	/*DPRINTF("siintr:\n");*/

	/* 
	 * For vme host adaptor interface, must disable dma before
	 * accessing any registers other than the csr or the 
	 * SI_CSR_DMA_CONFLICT bit in the csr will be set.
	 * Store bcr before the 5380 can change it because the 5380
	 * likes to prefetch data whether it needs it or not.
	 */
	un = c->c_un;
	if (IS_VME(c)) {
		sir->csr &= ~SI_CSR_DMA_EN;
 		bcr = GET_BCR(sir);
	} else {
		bcr = sir->bcr;
	}
	if (un != NULL)
		un->un_dma_curbcr = bcr;

	/* Check for external reset. */
	if (SBC_RD.cbsr & SBC_CBSR_RST) {
		SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int/dma */
		SBC_WR.tcr = TCR_UNSPECIFIED;
		junk = SBC_RD.clr;		/* clear int */
		SBC_WR.ser = 0;			/* disable resel int */
		si_dma_cleanup(c);

		/* If RESET doesn't clear, we have a cable problem */
		c->c_flags |= SCSI_FLUSH_DISQ;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_RST,
		    SI_SHORT_WAIT/4, 40, 0) != OK) {
			printf("si%d:  hardware failure, check cable\n",
				SINUM(c));
			sir->csr &= ~SI_CSR_INTR_EN;
			i = SE_FATAL;
		} else {
			EPRINTF("siintr: external reset\n");
			i = SE_TIMEOUT;
			if (IS_VME(c)  &&  nsi > 1) {
				/* WATCH OUT */
				s = splx(pritospl(c->c_intpri -1));
				DELAY(scsi_reset_delay);
				(void) splx(s);
			} else {
				DELAY(scsi_reset_delay);
			}
		}
		si_idle(c, i);		/* flush disconnect queue */
		goto LEAVE;
	}

#ifdef	SCSI_DEBUG
	if (c->c_last_phase >= PHASE_DISCONNECT) {
		SI_LOG_PHASE(PHASE_INTR2, -1, -1);
	} else {
		SI_LOG_PHASE(PHASE_INTR1, -1, -1);
	}
#endif	SCSI_DEBUG

	if (sir->csr & (SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) {
		/*
		 * Service VME BUS errors.  This is usually caused by
		 * sick hardware.
		 */
		if (sir->csr & SI_CSR_DMA_BUS_ERR) {
			printf("si%d:  siintr: bus error", SINUM(c));
			if (un != NULL  &&
			    un->un_flags & SC_UNF_DMA_ACTIVE)
				printf(" during dma\n");
			else
				printf("\n");

		/*
		 * Service illegal register access during DMA errors.
		 * This is normally caused by coding bugs.
		 */
		} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
			printf("si%d:  siintr: invalid reg access\n", SINUM(c));

		/*
		 * Handle wierd DMA in progress status.  Some devices
		 * seem to trigger this error.  It seems to be caused
		 * by the target taking too long to ack the last bytes
		 * of a data transfer.  Switch on DMA and the
		 * things will take care of themselves.  Otherwise,
		 * reset the bus.  Note, dma overrrun detection doesn't.
		 */
		} else if (sir->csr & SI_CSR_DMA_IP) {
			DPRINTF1("si%d:  dma in progress\n", SINUM(c));
			if (IS_VME(c))
				sir->csr |= SI_CSR_DMA_EN;

			/* wait for indication of dma completion */
			(void) si_wait((u_short *)&sir->csr,
			     SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT, 1);
			if (IS_VME(c)) {
				sir->csr &= ~SI_CSR_DMA_EN;
		 		bcr = GET_BCR(sir);
			} else {
				bcr = sir->bcr;
			}
			if (un != NULL)
				un->un_dma_curbcr = bcr;
			goto SYNCHRONIZE_PHASE;
		}
		goto RESET_AND_LEAVE;
	}

	/*
	 * We have an SBC interrupt due to a phase change on the bus or a
	 * reconnection attempt.  First, check for reconnect attempt.
	 */
	if (((SBC_RD.cbsr & SBC_CBSR_RESEL) == SBC_CBSR_RESEL)  && 
	    (SBC_RD.cdr & scsi_host_id)) {
		register u_char j, id, cbsr;

		/*
		 * Acknowledge reselection.  Get reselecting scsi id and
		 * verify that only 2 scsi id's set.  Also, lock out
		 * ALL interrupts for the duration as we'll die horribly
		 * otherwise.
		 */
HANDLE_RECONNECT:
		s = splr(pritospl(7));		/* time critical */
		id = SBC_RD.cdr & ~scsi_host_id;
		cbsr = SBC_RD.cbsr;
		/*
		 * If reselection valid, data bus will contain a target id
		 * (excluding ours) and reselect I/O signals will be active.
		 * Then, we can assert busy.  Otherwise, we lost the reselect
		 * and will wait for the next one..
		 */
		if (id == 0  ||  (cbsr & SBC_CBSR_RESEL) != SBC_CBSR_RESEL) {
			(void) splx(s);

			SI_LOG_PHASE(PHASE_LOST_RESELECT, 1, 0);
			goto SYNCHRONIZE_RECONNECT;
		}
		for (i = 0; i < 8; i++) {
			j = 1 <<i;
			if (id & j)
				break;
		}
		if (id != j) {
			(void) splx(s);

			/* If Select doesn't clear, we have a cable problem */
			if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_SEL,
			    SI_SHORT_WAIT *10, 1, 0) != OK) {
				printf("si%d:  hardware failure, check cable\n",
					SINUM(c));
				goto LEAVE;
			}
			SI_LOG_PHASE(PHASE_LOST_RESELECT, 2, i);
			goto SYNCHRONIZE_RECONNECT;
		}
		SBC_WR.icr |= SBC_ICR_BUSY;
		SBC_WR.ser = 0;			/* clear (re)sel int */

		c->c_recon_target = i;		/* save for reconnect */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_SEL,
		    SI_WAIT_COUNT *10, 1, 0) != OK) {
			(void) splx(s);

			printf("si%d:  siintr: SEL not released\n", SINUM(c));
			goto RESET_AND_LEAVE;
		}
		/*
		 * After clearing busy, the target should assert busy.  We
		 * should then go to msg_in phase.  If not, then we missed
		 * the reselection and will need to try again.
		 */
		SBC_WR.icr &= ~SBC_ICR_BUSY;
		DELAY (SI_BUS_SETTLE_DELAY);
		if ((SBC_RD.cbsr & SBC_CBSR_BSY) != SBC_CBSR_BSY) {
			(void) splx(s);

			SI_LOG_PHASE(PHASE_LOST_RESELECT, 3, i);
			goto SYNCHRONIZE_RECONNECT;
		}
		/*
		 * Set que interlock, we're busy now with a reconnected cmd.
		 * This prevents another request from being started.
		 */
		c->c_tab.b_active |= C_ACTIVE;
		SI_LOG_PHASE(PHASE_RESELECT, i, -1);
		(void) splx(s);			/* end of critical section */

		/* Reselection ok, target should now go into MSG_IN phase. */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SI_WAIT_COUNT *10, 1, 1) != OK) {
			printf("si%d:  siintr: no MSG_IN req\n", SINUM(c));
			goto RESET_AND_LEAVE;
		}
		SBC_WR.ser = scsi_host_id;	/* enable (re)sel int */

	/* Possibly lost reselect completely.  Wait for next one */
	} else if (c->c_last_phase >= PHASE_DISCONNECT  &&
	    	   sir->csr & SI_CSR_SBC_IP) {
		SI_LOG_PHASE(PHASE_LOST_RESELECT, 4, -1);

SYNCHRONIZE_RECONNECT:
		/*
		 * Missed this reselect, wait for next one.
		 */
		SI_NORESEL;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_SEL,
		    SI_SHORT_WAIT/4, 40, 0))
			goto RESET_AND_LEAVE;

		for (i = 0; i < SI_SHORT_WAIT /4; i++) {
			if (((SBC_RD.cbsr & SBC_CBSR_RESEL) == SBC_CBSR_RESEL)  && 
			    (SBC_RD.cdr & scsi_host_id)) {
				goto HANDLE_RECONNECT;
			}
			DELAY(40);
		}

		/* There was no next reselect, we're going to die! */
		SI_LOG_PHASE(PHASE_LOST_RESELECT, 0, SBC_RD.cbsr);
		junk = SBC_RD.clr;		/* clear int */
		SBC_WR.ser = 0;			/* clear (re)sel int */
		SBC_WR.ser = scsi_host_id;	/* enable (re)sel int */
		goto LEAVE;
	}

	/*
	 * We know that we have a new phase we have to handle.
	 */
SYNCHRONIZE_PHASE:
	DPRINTF("siintr: synch\n");
	SBC_WR.tcr = TCR_UNSPECIFIED;
	SBC_WR.mr &= ~SBC_MR_DMA;
	junk = SBC_RD.clr;		/* clear int */

	/* Check for glitches */
	if ((SBC_RD.cbsr & SBC_CBSR_REQ) == 0) {
		EPRINTF1("si%d:  siintr: REQ glitch\n", SINUM(c));
		goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
	}
	/* Check other boards */
	if (nsi > 1)
		(void) sipoll();
	SI_WIN;
	un = c->c_un;
	switch (SBC_RD.cbsr & CBSR_PHASE_BITS) {

	case PHASE_DATA_IN:
	case PHASE_DATA_OUT:
		DPRINTF("  DATA\n");
		SI_VME_OK(c, sir, "siintr: data");

		if (un == NULL  ||  un->un_dma_curcnt == 0  ||
		    un->un_dma_curdir == SI_NO_DATA) {
			printf("si%d:  siintr: unexpected DATA phase,",
				SINUM(c));
			printf("un= 0x%x, curcnt= %d, curdir= %d\n",
				(u_int)un, un->un_dma_curcnt, un->un_dma_curdir);
			goto RESET_AND_LEAVE;
		}
		/* Check for odd-byte DMA starting address */
		if (un->un_dma_curcnt != 0  &&  (u_int)(un->un_dma_curaddr) & 0x1) {
			printf("si%d:  illegal odd byte DMA, address= 0x%x\n",
				SINUM(c), un->un_dma_curaddr);
			goto RESET_AND_LEAVE;
		}
		/* Data is expected, start dma data transfer and exit */
		si_sbc_dma_setup(c, sir);
		goto LEAVE;


	case PHASE_MSG_IN:
		DPRINTF("  MSG");
		SI_VME_OK(c, sir, "siintr: msg_in");
		msg = SBC_RD.cdr & 0xf0;	/* peek at message */
		if ((msg == SC_IDENTIFY)  ||  (msg == SC_DR_IDENTIFY)) {
			DPRINTF("siintr:  msg= identify\n");
			lun = SBC_RD.cdr & 0x07;

			if (si_recon_queue(c, c->c_recon_target, lun, 0) != OK)
				goto RESET_AND_LEAVE;
			un = c->c_un;

			/* Set DMA idling flag */
			un->un_flags |= SC_UNF_DMA_INITIALIZED;
			if (un->un_dma_curdir != SI_NO_DATA) {
				if (IS_VME(c))
					si_vme_dma_setup(c, un);
				else
					si_ob_dma_setup(c, un);
			}
			msg = si_getdata(c, PHASE_MSG_IN);  /* Ack msg byte */
			SI_LOG_PHASE(PHASE_IDENTIFY, un->un_target, lun);
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
		}

		/*
		 * If DMA engine running, idle it.
		 */
		if (un->un_flags & SC_UNF_DMA_ACTIVE) {
			/* Clear DMA flags */
			un->un_flags &= ~SC_UNF_DMA_ACTIVE;
			un->un_flags &= ~SC_UNF_DMA_INITIALIZED;

			if (si_disconnect(c) != OK)
				goto RESET_AND_LEAVE;
		}
		if ((un->un_flags & SC_UNF_DMA_INITIALIZED) == 0  &&
		     un->un_dma_curcnt != 0) {
			/* Set DMA idling flag */
			un->un_flags |= SC_UNF_DMA_INITIALIZED;
			if (un->un_dma_curdir != SI_NO_DATA) {
				if (IS_VME(c))
					si_vme_dma_setup(c, un);
				else
					si_ob_dma_setup(c, un);
			}
		}

		msg = SBC_RD.cdr;	/* peek at message */
		switch (msg) {
		case SC_COMMAND_COMPLETE:
			DPRINTF("siintr:  command complete\n");
			cp = (u_char *) &un->un_scb;
			if (cp[0] & SCB_STATUS_MASK)
				status = SE_RETRYABLE;
			else
				status = SE_NO_ERROR;
			SI_LOG_PHASE(PHASE_CMD_CPLT, resid, -1);
			msg = si_getdata(c, PHASE_MSG_IN);

			/* Drop priority to allow other VME boards access */
			resid = un->un_dma_curcnt;
			if (IS_VME(c)  &&  nsi > 1) {
				s = splx(pritospl(c->c_intpri -1));
				(*un->un_ss->ss_intr)(c, resid, status);
				(void) splx(s);
			} else {
				(*un->un_ss->ss_intr)(c, resid, status);
			}
			c->c_tab.b_active &= C_QUEUED;
			goto START_NEXT_COMMAND;

		case SC_DISCONNECT:
			DPRINTF("siintr:  msg= disconnect\n");
			si_discon_queue(un);
			SI_LOG_PHASE(PHASE_DISCONNECT, un->un_dma_curbcr, -1);
			msg = si_getdata(c, PHASE_MSG_IN);	/* ack msg */
			goto START_NEXT_COMMAND;

		case SC_RESTORE_PTRS:
			DPRINTF("siintr:  msg= restore pointer\n");
			SI_LOG_PHASE(PHASE_RESTORE_PTR, un->un_dma_curcnt, -1);
			break;

		case SC_SAVE_DATA_PTR:
			DPRINTF("siintr:  msg= save pointer\n");
			SI_LOG_PHASE(PHASE_SAVE_PTR, un->un_dma_curbcr,
				     GET_DMA_ADDR(sir));
			break;

		case SC_NO_OP:
			EPRINTF("siintr:  msg= no op\n");
			break;

		case SC_SYNCHRONOUS:
			EPRINTF("siintr:  msg= extended\n");
			SI_LOG_PHASE(PHASE_SYNCHRONOUS, -1, -1);
			goto MSG_IN_REJECT;

		case SC_PARITY:
			EPRINTF("siintr:  msg= parity error\n");
			SI_LOG_PHASE(PHASE_PARITY, -1, -1);
			goto MSG_IN_REJECT;

		case SC_ABORT:
			EPRINTF("siintr:  msg= abort\n");
			SI_LOG_PHASE(PHASE_ABORT, -1, -1);
			goto MSG_IN_REJECT;

		case SC_DEVICE_RESET:
			EPRINTF("siintr:  msg= reset device\n");
			/*SI_LOG_PHASE(PHASE_DEV_RESET, -1, -1);*/
			goto MSG_IN_REJECT;

		default:
			printf("si%d:  siintr: rejecting message, 0x%x\n",
				SINUM(c), msg);
MSG_IN_REJECT:
			EPRINTF1("siintr:  rejecting msg 0x%x\n", msg);
			SBC_WR.icr = SBC_ICR_ATN;
			msg = 255; /* accept 255 message bytes (overkill) */
			while((si_getdata(c, PHASE_MSG_IN) !=  -1)  &&  --msg);
			if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_MSG_OUT) {
				msg = SC_MSG_REJECT;
				SBC_WR.icr = 0;		/* turn off ATN */
				(void) si_putdata(c, PHASE_MSG_OUT, &msg, 1, 0);
			}
			goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;
		}
		/* Ack msg byte, and continue */
		SBC_WR.tcr = TCR_UNSPECIFIED;
		SBC_WR.icr = SBC_ICR_ACK;
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    SI_WAIT_COUNT *10, 1, 0) != OK) {
			printf("si%d:  siintr: REQ not INactive\n",
				SINUM(c));
		}
		SBC_WR.icr = 0;
		goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;


	case PHASE_STATUS:
		DPRINTF("  STATUS");
		SI_VME_OK(c, sir, "siintr: status");
		if (un->un_flags & SC_UNF_DMA_ACTIVE) {
			/* Clear DMA flags */
			un->un_flags &= ~SC_UNF_DMA_ACTIVE;
			un->un_flags &= ~SC_UNF_DMA_INITIALIZED;
			if (si_disconnect(c) != OK)
				goto RESET_AND_LEAVE;
		}
		cp = (u_char *) &un->un_scb;
		cp[0] = si_getdata(c, PHASE_STATUS);
		if (cp[0] == 0xff) {
			printf("si%d:  siintr: no status\n", SINUM(c));
			goto RESET_AND_LEAVE;
		}
		SI_LOG_PHASE(PHASE_STATUS, cp[0], -1);
		goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;


	default:
		printf("si%d:  siintr: spurious phase\n", SINUM(c));
		/* goto RESET_AND_LEAVE; */
}


RESET_AND_LEAVE:
	EPRINTF("reset_and_leave:\n");
	si_reset(c, PRINT_MSG);
	goto SET_UP_FOR_NEXT_INTR_AND_LEAVE;


START_NEXT_COMMAND:
	/*
	 * Check if we have a reconnect pending already.  This happens
	 * with some stupid SCSI controllers which automatically disconnect,
	 * even when they don't have to. Rather than field another
	 * interrupt, let's go handle it.
	 */
	DPRINTF("START_NEXT_COMMAND\n");
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;
	SI_VME_OK(c, sir, "siintr: start next command");

	if ((SBC_RD.cbsr & SBC_CBSR_RESEL) == SBC_CBSR_RESEL  && 
	    SBC_RD.cdr & scsi_host_id)
		goto HANDLE_RECONNECT;

	un = (struct scsi_unit *)c->c_tab.b_actf;
	if (un != NULL)
		sistart(un);
	/* goto SET_UP_FOR_NEXT_INTR_AND_LEAVE; */


SET_UP_FOR_NEXT_INTR_AND_LEAVE:
	DPRINTF("SET_UP_FOR_NEXT_INTR_AND_LEAVE\n");
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;
	SI_VME_OK(c, sir, "siintr: setup for next intr");
	/*
	 * Depending on the last phase, we need to check either for
	 * target reselection or a SCSI bus phase change.
	 */
	if (c->c_last_phase >= PHASE_DISCONNECT) {
		SBC_WR.mr &= ~SBC_MR_DMA;
		SBC_WR.tcr = TCR_UNSPECIFIED;
		junk = SBC_RD.clr;		/* clear int */

		/* Check for reselection.  */
		msg = SBC_RD.cbsr;
		if (((msg & SBC_CBSR_RESEL) == SBC_CBSR_RESEL)  && 
		    (SBC_RD.cdr & scsi_host_id)) {
			SI_WIN;
			/*DPRINTF("  win2");*/
			goto HANDLE_RECONNECT;
		} else {
			SI_LOSE;
			/*DPRINTF("  lose2");*/
		}
	} else {
		/* If busy gone, the target just died! */
		if ((SBC_RD.cbsr & SBC_CBSR_BSY) == 0) {
			printf("si%d:  lost busy\n", SINUM(c));
			goto RESET_AND_LEAVE;
		}
		SBC_WR.tcr = TCR_UNSPECIFIED;
		junk = SBC_RD.clr;	 /* clear any pending int */
		SBC_WR.mr |= SBC_MR_DMA;

		/* Check for SCSI bus phase change.  */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
		    si_phase_wait, 20, 1) == OK) {
			/*DPRINTF("  win3");*/
			goto SYNCHRONIZE_PHASE;
		} else {
			SI_LOSE;
			/*DPRINTF("  lose3");*/
		}
	}
	/* Enable interrupts and DMA. */
	if (IS_VME(c))
		sir->csr |= SI_CSR_DMA_EN;

LEAVE:
	DPRINTF("\n");
	return;
}


/*
 * Handle target disconnecting.
 * Returns true if all was OK, false otherwise.
 */
static
si_disconnect(c) 
	register struct scsi_ctlr *c;
{
	register struct scsi_unit *un = c->c_un;
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	int status = OK;
	int bcr;

	DPRINTF("si_disconnect:\n");
	/*
	 * If command doen't require dma, don't save dma info.
	 * for reconnect.  If it does, but data phase was missing,
	 * don't update dma info.
	 */
	if (un->un_dma_curdir != SI_NO_DATA) {

		if (IS_VME(c)) {
			/* 
			 * bcr does not reflect how many bytes were actually
			 * transferred for VME.
			 *
			 * SCSI-3 VME interface is a little funny on writes:
			 * if we have a disconnect, the dma has overshot by
			 * one byte and needs to be incremented.  This is
			 * true if we have not transferred either all data
			 * or no data.
			 */
			bcr = GET_BCR(sir);
			if ((un->un_dma_curdir == SI_SEND_DATA)  &&
			    (bcr != un->un_dma_curcnt)  &&  (bcr != 0)) {

				if (un->un_dma_curbcr != 0)
					bcr = un->un_dma_curbcr + 1;
				else
					bcr++;
			/* 
			 * Use the bcr value we got before we pulled in the
			 * discon message.
			 */
			} else if (un->un_dma_curdir == SI_RECV_DATA) {
				bcr = un->un_dma_curbcr;
				status = si_dma_recv(c);
			}
		} else {
			/* Handle onboard case (e.g. Sun 3/50) */
			bcr = sir->bcr;
			un->un_dma_curbcr = bcr;
			if (un->un_dma_curdir == SI_RECV_DATA)
				status = si_dma_recv(c);
		}
		/*
		 * Save dma information so dma can be restarted when
		 * a reconnect occurs.
		 */
		si_dma_cleanup(c);
		un->un_dma_curaddr += un->un_dma_curcnt - bcr;
		un->un_dma_curcnt = bcr;
	}
	return (status);
}


/*
 * Remove timed-out I/O request and report error to
 * it's interrupt handler.
 * Return OK if sucessful, FAIL if not.
 */
si_deque(c, un)
	register struct scsi_ctlr *c;
	register struct scsi_unit *un;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct mb_device *md = un->un_md;
	int s;
	/*DPRINTF("si_deque:\n");*/

#ifdef	lint
	md = md;
#endif	lint

	/* Lock out the rest of si till we've finished the dirty work. */
	s = splr(pritospl(c->c_intpri));	/* time critical */
	/*
	 * If current SCSI I/O request is the one that timed out,
	 * Reset the SCSI bus as this looks serious.
	 */
	if (un == c->c_un) {
		if (sir->csr & (SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT)) {
			/* Hardware lost int., attempt restart */
			(void) splx(s);
			printf("si%d:  lost interrupt\n", SINUM(c));
			si_print_state(sir, c);
			siintr(c);
			return (OK);
		} else {
			/* Really did timeout */
			(void) splx(s);
			return (FAIL);
		}
	}

	if (c->c_tab.b_actf == NULL) {
		/* Search for entry on disconnect queue */
		if (si_recon_queue(c, un->un_target, un->un_lun, 0)) {
			/* died in active que,  don't restart */
			(void) splx(s);
			printf("si%d:  I/O request not started, ignoring timeout\n",
				SINUM(c));
			if (scsi_debug)
				si_print_state(sir, c);
			return (OK);
		}
		/* died in disconnect que,  restart */
		(void) splx(s);
		EPRINTF("si_deque:  reactivating request\n");
		si_print_state(sir, c);
		(*un->un_ss->ss_intr)(c, un->un_dma_count, SE_TIMEOUT);
		c->c_tab.b_active &= C_QUEUED;		/* clear interlock */
		return (OK);

	} else {
                /*
		 * Another request is active. If timed out request is
		 * still waiting to run, restart timeout.  If
		 * it's already disconnected, things get interesting.
		 * In this case we may have saturated the bus so we stop
		 * sending further commands and wait for the disconnect
		 * que to flush.  If it doesn't clear, the device really
		 * timed out.
		 */
		if (si_recon_queue(c, un->un_target, un->un_lun, 1)) {
			(void) splx(s);

			/* died in active que,  restart timeout */
			printf("si%d:  I/O request not started, ignoring timeout\n",
				SINUM(c));
			if (scsi_debug)
				si_print_state(sir, c);
			return (OK);
		}
		if (! (c->c_tab.b_active & C_FLUSHING)) {
			/* flush disconnect que and see what happens... */
			c->c_drainun = un;
			c->c_tab.b_active |= C_FLUSHING;
			(void) splx(s);
			printf("si%d:  draining disconnect que\n", SINUM(c));
			return (OK);
		}
		/*
		 * If another device timed out while we're draining
		 * the disconnect que, ignore it as it's likely to
		 * be caused as a by-product of the initial timeout.
		 * If the one we're timing out on times out again, die!
		 */
		if (c->c_drainun != un  &&
		    (c->c_tab.b_active & C_FLUSHING)) {
			(void) splx(s);
			printf("si%d:  drain in progress, ignoring timeout\n",
				 SINUM(c));
			return (OK);
		}
		printf("si%d:  disconnect que drain failed\n", SINUM(c));
		c->c_drainun = (struct scsi_unit *) -1;	/* clear drain un */
		c->c_tab.b_active &= ~C_FLUSHING;

		/* died in disconnect que,  reset */
		(void) splx(s);
		return (FAIL);
	}
}



/*
 * Flush disconnected I/O requests after a SCSI bus reset since this causes
 * targets to "forget" about any disconnected activity.
 */
si_idle(c, flag)
	register struct scsi_ctlr *c;
	int flag;
{
	register struct scsi_unit *un;
	int s, t, resid;
	/*DPRINTF("si_idle:\n");*/

	/* If flushing in progress, exit */
	if (c->c_flags & SCSI_FLUSHING)
		return;

	/* flush disconnect tasks if a reconnect will never occur */
	if (c->c_flags & SCSI_FLUSH_DISQ) {

		s = splr(pritospl(c->c_intpri));	/* time critical */
		c->c_tab.b_active = C_ACTIVE;		/* engage interlock */
		/*
		 * Force current I/O request to be preempted and put it
		 * on disconnect que so we can flush it.
		 */
		un = (struct scsi_unit *)(c->c_tab.b_actf);
		if (un != NULL  &&  un->un_md->md_utab.b_active & MD_IN_PROGRESS) {
			/*un->un_md->md_utab.b_active |= MD_PREEMPT;*/
			si_discon_queue(un);
			/*EPRINTF("si_idle: dequeueing active request\n");*/
		}
		/* now in process of flushing tasks */
		c->c_flags &= ~SCSI_FLUSH_DISQ;
		c->c_flags |= SCSI_FLUSHING;
		c->c_flush = c->c_disqtab.b_actl;

		for (un = (struct scsi_unit *)c->c_disqtab.b_actf; 
                     un  &&  c->c_flush; 
		     un = (struct scsi_unit *)c->c_disqtab.b_actf) {

			/* If last disconnected request flushed, quit */
			if (c->c_flush == (struct buf *)un) 
				c->c_flush = NULL;

			/* requeue on controller active queue */
			if (si_recon_queue(c, un->un_target, un->un_lun, 0))
				continue;

			resid = un->un_dma_curcnt;

			/* Drop priority to allow other VME boards access */
			if (IS_VME(c)  &&  nsi > 1) {
				t = splx(pritospl(c->c_intpri -1));
				(*un->un_ss->ss_intr)(c, resid, flag);
				(void) splx(t);
			} else {
				(*un->un_ss->ss_intr)(c, resid, flag);
			}
			si_off(c, un, flag);
		}
		c->c_flags &= ~SCSI_FLUSHING;
		c->c_tab.b_active &= C_QUEUED;	/* Clear interlock */
		(void) splx(s);
		return;
	}
}


/*
 * Get status bytes from scsi bus.
 * Returns number of status bytes read if no error.
 * If error, returns -1.  If scsi bus error, returns 0.
 */
si_getstatus(un)
	register struct scsi_unit *un;
{
	register struct scsi_ctlr *c = un->un_c;
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register u_char *cp;
	register u_char msg = 0;
	/* DPRINTF("si_getstatus:\n"); */

	if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ,
	    SI_LONG_WAIT, 10, 1) != OK) {
		printf("si%d:  si_getstatus: REQ inactive\n", SINUM(c));
		si_print_state(sir, c);
		return (0);
	}
	cp = (u_char *) &un->un_scb;
	cp[0] = 0;
	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) == PHASE_STATUS) {
		cp[0] = si_getdata(c, PHASE_STATUS);
		SI_LOG_PHASE(PHASE_STATUS, cp[0], -1);
	}

	if (si_wait_phase(sir, PHASE_MSG_IN) == OK)
		msg = si_getdata(c, PHASE_MSG_IN);
	else
		printf("si%d:  si_getstatus: no MSG_IN phase\n", SINUM(c));

	SBC_WR.tcr = TCR_UNSPECIFIED;
	if (msg != SC_COMMAND_COMPLETE) {
		SI_LOG_PHASE(PHASE_MSG_IN, msg, 0);
		si_print_state(sir, c);
		return (-1);	/* retryable */
	}

	/*
	 * Check status for error condition, return -1 if error.
	 * Otherwise, return 1 for no error.
	 */
	SI_LOG_PHASE(PHASE_CMD_CPLT, 0, -1);
	if (cp[0] & SCB_STATUS_MASK)
		return (-1);	/* retryable */

	return (1);	 	/* no error */
}


/* 
 * Wait for a scsi dma request to complete.
 * Disconnects were disabled in si_cmd when polling for command completion.
 * Called by drivers in order to poll on command completion.
 */
si_cmdwait(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register struct scsi_unit *un = c->c_un;
	register int ret_val = OK;

	/*DPRINTF("si_cmdwait:\n");*/
	/* wait for dma transfer to complete */
	if (si_wait((u_short *)&sir->csr, SI_CSR_DMA_ACTIVE, 0) != OK) {
		EPRINTF1("si%d:  si_cmdwait: DMA still ACTIVE\n", SINUM(c));
		ret_val = SCSI_FAIL;
		goto SI_CMDWAIT_EXIT;
	}

	/* if command does not involve dma activity, then we are finished */
	if (un->un_dma_curdir == SI_NO_DATA)
		return (OK);

	/* wait for indication of dma completion */
	if (si_wait((u_short *)&sir->csr, 
	    SI_CSR_SBC_IP | SI_CSR_DMA_CONFLICT, 1) != OK) {
		EPRINTF1("si%d:  si_cmdwait: dma timeout\n", SINUM(c));
		ret_val = SCSI_FAIL;
		goto SI_CMDWAIT_EXIT;
	}
	if (IS_VME(c))
		goto SI_CMDWAIT_EXIT;

	/* check for DMA errors */
	if (sir->csr & (SI_CSR_DMA_IP | SI_CSR_DMA_CONFLICT)) {
#ifdef		SCSI_DEBUG
		if (sir->csr & SI_CSR_DMA_BUS_ERR) {
			EPRINTF1("si%d:  si_cmdwait: bus error during dma\n",
				SINUM(c));
		} else if (sir->csr & SI_CSR_DMA_CONFLICT) {
			EPRINTF1("si%d:  si_cmdwait: reg access during dma\n",
				SINUM(c));
		} else {
			EPRINTF1("si%d:  si_cmdwait: dma in progress\n", SINUM(c));
		}
#endif		SCSI_DEBUG
		ret_val = SCSI_FAIL;
		goto SI_CMDWAIT_EXIT;
	}

	/* handle special dma recv situations */
	if (si_disconnect(c) != OK) {
		ret_val = SCSI_FAIL;
		goto SI_CMDWAIT_EXIT;
	}

SI_CMDWAIT_EXIT:
	if (IS_VME(c))
		sir->csr &= ~SI_CSR_DMA_EN;	/* turn it off to be sure */
	junk = SBC_RD.clr;			/* clear sbc int */
	if (ret_val != OK)
		si_reset(c, PRINT_MSG);
	return (ret_val);
}


/*
 * Wait for a condition to be (de)asserted on the scsi bus.
 * Returns OK for successful.  Otherwise, returns FAIL.
 */
static
si_sbc_wait(reg, cond, wait_count, count, set)
	register caddr_t reg;
	register u_char cond;
	register int wait_count, count;
	register int set;
{
	register int i;
	register u_char regval;

	for (i = 0; i < wait_count; i++) {
		regval = *reg;
		if ((set == 1)  &&  (regval & cond))
			return (OK);
		if ((set == 0)  &&  !(regval & cond))
			return (OK);
		DELAY(count);
	}
	/*DPRINTF("si_sbc_wait:  timeout\n");*/
	return (FAIL);
}


/*
 * Wait for a condition to be (de)asserted.  Used for monitor DMA controller.
 * Returns OK for successful.  Otherwise, returns FAIL.
 */
static
si_wait(reg, cond, set)
	register u_short *reg;
	register u_short cond;
	register int set;
{
	register int i;
	register u_short regval;

	for (i = 0; i < SI_WAIT_COUNT; i++) {
		regval = *reg;
		if ((set == 1)  &&  (regval & cond))
			return (OK);
		if ((set == 0)  &&  !(regval & cond))
			return (OK);
		DELAY(10);
	}
	return (FAIL);
}


/*
 * Wait for a phase on the SCSI bus.
 * Returns OK for successful.  Otherwise, returns FAIL.
 */
static
si_wait_phase(sir, phase)
	register struct scsi_si_reg *sir;
	register u_char phase;
{
	register int i;

	/*DPRINTF2("si_wait_phase:  %s phase (0x%x)\n",si_str_phase(phase),phase);*/
	for (i = 0; i < SI_WAIT_COUNT; i++) {
		if (SBC_RD.cbsr & SBC_CBSR_REQ  &&
		   (SBC_RD.cbsr & CBSR_PHASE_BITS) == phase)
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
si_putdata(c, phase, data, num, want_intr)
	register struct scsi_ctlr *c;
	register u_short phase;
	register u_char *data;
	register u_char num;
	register int want_intr;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register int i, j, k;
	char *fail;
	/*DPRINTF2("si_putdata:  %s phase (0x%x):",
	 *	   si_str_phase(phase), phase);
	 */

	/* Set up tcr so we can transmit data.  */
	SBC_WR.tcr = phase >> 2;

	/* put all desired bytes onto scsi bus */
	for (i = 0; i < num; i++ ) {

		SBC_WR.icr = SBC_ICR_DATA;	/* clear ack, enable data bus */

		/* wait for target to request a byte */
		for (j = 0; j < SI_WAIT_COUNT *100; j++) {
			if (SBC_RD.cbsr & SBC_CBSR_REQ) {
				/* send data and complete req/ack handshake */
				/*DPRINTF1("  0x%x", *data);*/
				SBC_WR.odr = *data++;
				SBC_WR.icr |= SBC_ICR_ACK;
				for (k = 0; k < SI_WAIT_COUNT *100; k++) {
					if ((SBC_RD.cbsr & SBC_CBSR_REQ) == 0)
						goto SI_PUTDATA_NEXT;
					/* DELAY (1); */
				}
				goto SI_PUTDATA_FAILURE;
			}
			DELAY (1);
		}
		goto SI_PUTDATA_FAILURE;

SI_PUTDATA_NEXT:
	continue;
	}

	/* CRITICAL CODE SECTION DON'T TOUCH */
	/*DPRINTF("\n");*/
	SBC_WR.tcr = TCR_UNSPECIFIED;
	junk = SBC_RD.clr;			/* clear int */
	if (want_intr)
		SBC_WR.mr |= SBC_MR_DMA;	/* allow ints. */
	SBC_WR.icr = 0;				/* clear ack */
	return (OK);

SI_PUTDATA_FAILURE:
	/* We've had a req/ack handshake failure */
	if (SBC_RD.cbsr & SBC_CBSR_REQ)
		fail = "active";
	else
		fail = "INactive";

	printf("si%d:  si_putdata: REQ not %s\n", SINUM(c), fail);
	SBC_WR.tcr = TCR_UNSPECIFIED;
	junk = SBC_RD.clr;			/* clear int */
	if (want_intr)
		SBC_WR.mr |= SBC_MR_DMA;	/* allow ints. */
	SBC_WR.icr = 0;				/* clear ack */
	return (FAIL);
}


/*
 * Get data from the scsi bus.
 * Returns a single byte of data, -1 if unsuccessful.
 */
static
si_getdata(c, phase)
	register struct scsi_ctlr *c;
	register u_short phase;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register int data = -1;
	register int i;
	register u_char icr;

	/*DPRINTF2("si_getdata: %s phase (0x%x)", si_str_phase(phase), phase);*/

	/* See if we're in correct phase and wait for req */
	SBC_WR.tcr = TCR_UNSPECIFIED;
	SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int/dma */
	if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != phase  ||
	    (SBC_RD.cbsr & SBC_CBSR_REQ) == 0) {

		/* wait for req */
		if (si_sbc_wait((caddr_t)&SBC_RD.cbsr, SBC_CBSR_REQ, 
		    SI_WAIT_COUNT, 10, 1) != OK) {
			printf("si%d:  si_getdata: REQ not active\n", SINUM(c));
			si_print_state(sir, c);
			return (-1);
		}
		/* verify correct phase */
		if ((SBC_RD.cbsr & CBSR_PHASE_BITS) != phase) {
			DPRINTF1("si_getdata:  unexpected phase(0x%x)\n",
				SBC_RD.cbsr);
			return (-1);
		}
	}
	/* grab data and complete req/ack handshake */
	data = SBC_RD.cdr;
	icr = SBC_WR.icr;
	SBC_WR.icr = icr | SBC_ICR_ACK;

	for (i = 0; i < SI_WAIT_COUNT *100; i++) {
		if ((SBC_RD.cbsr & SBC_CBSR_REQ) == 0)
			goto SI_GETDATA_EXIT;
		DELAY (1);
	}
	printf("si%d:  si_getdata: REQ not INactive\n", SINUM(c));

SI_GETDATA_EXIT:
	/* DPRINTF1("  data= 0x%x\n", data); */
	SBC_WR.icr = icr;		/* clear ack */
	return (data);
}


/*
 * Reset SCSI control logic and bus.
 */
si_reset(c, msg_enable)
	register struct scsi_ctlr *c;
	register int msg_enable;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);
	register int s;

	if (msg_enable) {
		printf("si%d:  resetting scsi bus\n", SINUM(c));
		si_print_state(sir, c);
	}

	/* Disable sbc interrupts.  Reconnects will be enabled by si_cmd. */
	si_dma_cleanup(c);		/* shutdown DMA engine */
	SBC_WR.mr &= ~SBC_MR_DMA;	/* clear phase int/dma */
	SBC_WR.tcr = TCR_UNSPECIFIED;
	SBC_WR.ser = 0;			/* disable resel int */

	/* reset scsi control logic */
	sir->csr = 0;
	DELAY(10);
	sir->csr = SI_CSR_SCSI_RES | SI_CSR_FIFO_RES;

	/* issue scsi bus reset (make sure interrupts from sbc are disabled) */
	SI_LOG_PHASE(PHASE_RESET, -1, -1);
	SBC_WR.icr = SBC_ICR_RST;
	DELAY(1000);
	SBC_WR.icr = 0;			/* clear reset */
	junk = SBC_RD.clr;		/* clear ints */
	/*
	 * Allow devices recovery time after reset
	 * Note, on-board SCSI does not like splx's.
	 */
	if (IS_VME(c)  &&  nsi > 1  &&  msg_enable != NO_MSG) {
		s = splx(pritospl(c->c_intpri -1));	/* WATCH OUT */
		DELAY(scsi_reset_delay);
		(void) splx(s);
	} else {
		DELAY(scsi_reset_delay);
	}

	/* disconnect queue needs to be flushed */
	if (c->c_tab.b_actf != NULL  ||  c->c_disqtab.b_actf != NULL) {
		c->c_flags |= SCSI_FLUSH_DISQ;
		si_idle(c, SE_TIMEOUT);
	}
}


/*
 * Return residual count for a dma.
 */
si_dmacnt(c)
	register struct scsi_ctlr *c;
{
	register struct scsi_si_reg *sir = (struct scsi_si_reg *)(c->c_reg);

	if (IS_VME(c))
		return (GET_BCR(sir));
	else
		return (sir->bcr);
}
#endif NSI > 0
