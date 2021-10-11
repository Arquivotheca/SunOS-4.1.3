#ifndef lint
static	char sccsid[] = "@(#)xd.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Driver for Xylogics 7053 SMD disk controllers
 */
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/dk.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/user.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/uio.h>
#include <sys/kernel.h>
#include <sys/dkbad.h>
#include <sys/file.h>
#include <sys/kmem_alloc.h>

#include <machine/psl.h>
#include <machine/cpu.h>

#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/xdvar.h>
#include <sundev/xderr.h>

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

extern char *strncpy();
extern int dkn;

static initlabel(), uselabel(), usegeom(), islabel(), ck_cksum();
static initiopb(), cleariopb(), doattach(), findslave();
static struct xderror *finderr();
static printerr();

int xdprobe(), xdslave(), xdattach(), xdgo(), xddone();

/*
 * Config dependent structures -- declared in xd_conf.c
 */
extern int nxd, nxdc;
extern struct	mb_ctlr *xdcinfo[];
extern struct	mb_device *xddinfo[];
extern struct xdunit xdunits[];
extern struct xdctlr xdctlrs[];
extern struct xd_w_info xd_watch_info[];
extern u_short xd_ctlrpar0, xd_ctlrpar1;
extern struct xd_ctlrpar2 xd_ctlrpar2[];

struct mb_driver xdcdriver = {
	xdprobe, xdslave, xdattach, xdgo, xddone, 0,
	sizeof (struct xddevice), "xd", xddinfo, "xdc", xdcinfo, MDR_BIODMA,
};

/*
 * Settable error level.
 */
short xderrlvl = EL_FIXED | EL_RETRY | EL_RESTR | EL_RESET | EL_FAIL;

/*
 * List of commands for the 7053.  Used to print nice error messages.
 */
#define	XDCMDS	(sizeof xdcmdnames / sizeof xdcmdnames[0])
char *xdcmdnames[] = {
	"nop",
	"write",
	"read",
	"seek",
	"drive reset",
	"write parameters",
	"read parameters",
	"write extended",
	"read extended",
	"diagnostics",
};

int xdwstart, xdwatch();	/* have started guardian */
int xdticks = 1;		/* timer for guardian */

int xdthrottle = XD_THROTTLE;	/* transfer burst count */

/* need to make the following defines for parameters more readable */
/*
 * Defines for setting ctlr, drive and format parameters.
 */
#define	XD_CTLRPAR32	0x6050
#define	XD_CTLRPARAM	0x9600
#define	XD_DRPARAM7053	0x00
#define	XD_INTERLEAVE	0x00
#define	XD_FORMPAR2	((XD_FIELD1 << 8) | XD_FIELD2)
#define	XD_FORMPAR3	((((XD_FIELD3 << 8) | XD_FIELD4) * un->un_g->dkg_nhead \
			+ (XD_FIELD5 >> 8)) * un->un_g->dkg_nsect + \
			(XD_FIELD5 & 0xff))
#define	XD_FORMPAR4	((XD_FIELD6 << 24) | (XD_FIELD7 << 16) | XD_FIELD5ALT)

/*
 * Defines for specific format parameters fields.
 */
#define	XD_FIELD1	0x01		/* sector pulse to read gate */
#define	XD_FIELD2	0x0a		/* read gate to header sync */
#define	XD_FIELD3	0x1b		/* sector pulse to header sync */
#define	XD_FIELD4	0x14		/* header ecc to data sync */
#define	XD_FIELD5	0x200		/* sector size */
#define	XD_FIELD6	0x0a		/* header ecc to read gate */
#define	XD_FIELD7	0x03		/* data ecc to write gate */
#define	XD_FIELD5ALT	0x200		/* alternate sector size */

/*
 * Defines for the watchdog timer
 */
#define	SINGLE_TRACK_TO		8	/* max seconds for 1 track */
#define	XDWATCHTIMO		20	/* time to next check disk */

/*
 * List of free xdcmdblock/xdiopb (xd cbi) used for dynamic chaining.
 */
struct xdcmdblock *xdcbifree;		/* head of xd cbi free list */

int xdfreewait = 0;
/*
 * the number of cbis preallocated per drive.
 */
#define	NCBIFREE	14
int ncbifree = NCBIFREE;

/*
 * List of waiting buf requests.
 */
struct buf *waitbufq;			/* head */
struct buf *lbq;			/* tail */

/*
 * Spare cbi for dump to use (in case of disaster)
 */
int xddumpalloc;
struct xdcmdblock *xddumpcbi;

/*
 *
 */
xdmakedumpcbi()
{
	register struct xdcmdblock *xdcbi;
	int s;

	/*
	 * Allocate a cmdblock/iopb for xddump.
	 * The extra work is to insure a long word aligned iopb.
	 */
	s = splr(pritospl(SPLMB));
	xdcbi = (struct xdcmdblock *)new_kmem_alloc(
		    sizeof (struct xdcmdblock), KMEM_SLEEP);
	xdcbi->iopb = (struct xdiopb *)rmalloc(iopbmap,
		(long)(sizeof (struct xdiopb) + sizeof (u_long)));
	if (xdcbi->iopb == NULL) {
		printf("xd: no iopb space for dump\n");
		return (0);
	}
	xdcbi->iopb = (struct xdiopb *)
	    (((u_int)xdcbi->iopb + sizeof (u_long) - 1) &
	    ~(sizeof (u_long) - 1));
	initiopb(xdcbi->iopb);
	xdcbi->next = xddumpcbi;
	xddumpcbi = xdcbi;
	(void) splx(s);
	return (-1);
}

/*
 * Create xd cbi and add to the free list.  Called from xdprobe() and
 * xdattach().
 */
xdcreatecbi(c)
	register int c;
{
	register struct xdcmdblock *xdcbi;
	int s;

	/*
	 * Allocate a cmdblock/iopb put on the head of the xdcbi free
	 * list.  The extra work is to insure a long word aligned iopb.
	 */
	s = splr(pritospl(SPLMB));
	xdcbi = (struct xdcmdblock *)new_kmem_alloc(
		    sizeof (struct xdcmdblock), KMEM_SLEEP);
	xdcbi->iopb = (struct xdiopb *)rmalloc(iopbmap,
		(long)(sizeof (struct xdiopb) + sizeof (u_long)));
	if (xdcbi->iopb == NULL) {
		printf("xdc%d: no iopb space\n", c);
		return (0);
	}
	xdcbi->iopb = (struct xdiopb *)
	    (((u_int)xdcbi->iopb + sizeof (u_long) - 1) &
	    ~(sizeof (u_long) - 1));
	initiopb(xdcbi->iopb);
	/*
	 * Now add it to the free list.
	 */
	xdcbi->next = xdcbifree;
	xdcbifree = xdcbi;
	(void) splx(s);
	return (-1);
}

/*
 * Remove xd cbi from free list and put on the active list for the controller.
 * Called from xdprobe(), findslave(), doattach(), usegeom(), xdstart(),
 * xdioctl(), xddump(), wherever a controller command is initiated.
 */
struct xdcmdblock *
xdgetcbi(ctlr)
	struct xdctlr *ctlr;
{
	register struct xdcmdblock *xdcbi = NULL;
	register int s = splr(pritospl(SPLMB));

	if (xdcbifree != NULL) {
		/*
		 * Remove the cbi from the free list
		 */
		xdcbi = xdcbifree;
		xdcbifree = xdcbifree->next;
		xdcbi->c = ctlr;
		/*
		 * Add it to the active command queue for the controller.
		 */
		xdcbi->next = ctlr->actvcmdq;
		ctlr->actvcmdq = xdcbi;
	}
	(void) splx(s);
	return (xdcbi);
}

/*
 * Remove xd cbi from active list for the controller and put on the free list.
 * Called from xdcmd() for synchronous and asynchronous_wait commands and
 * from xdintr() for asynchronous commands.
 */
xdputcbi(xdcbi)
	register struct xdcmdblock *xdcbi;
{
	register struct xdcmdblock *cmdblk;
	int s;

	s = splr(pritospl(SPLMB));
	/*
	 * Find and remove from the active command queue.
	 */
	if ((cmdblk = xdcbi->c->actvcmdq) == xdcbi) {
		xdcbi->c->actvcmdq = cmdblk->next;
	} else {
		while ((cmdblk != NULL) && (cmdblk->next != xdcbi))
			cmdblk = cmdblk->next;
		if (cmdblk == NULL) {
			if (xdcbi == xddumpcbi) {
				(void) splx(s);
				return;
			} else {
				panic("cbi queuing error");
			}
		} else {
			cmdblk->next = xdcbi->next;
		}
	}
	/*
	 * Add to the free list.
	 */
	xdcbi->next = xdcbifree;
	xdcbifree = xdcbi;
	/*
	 * If anyone sleeping on cbis freeing up,
	 * wake them up now...
	 */
	if (xdfreewait) {
		xdfreewait = 0;
		wakeup((caddr_t)&xdfreewait);
	}
	(void) splx(s);
}

/*
 * Determine existence of a controller.  Called at boot time only.
 */
xdprobe(reg, ctlr)
	caddr_t reg;
	int ctlr;
{
	register struct xdctlr *c = &xdctlrs[ctlr];
	register struct xdcmdblock *xdcbi;
	u_char err;
	int	i;

	/*
	 * See if there's hardware present by trying to reset it.
	 */
	c->c_io = (struct xddevice *)reg;
	if (pokec((char *)&c->c_io->xd_csr, XD_RST))
		return (0); /* not there */
	CDELAY(((c->c_io->xd_csr & XD_RACT) == 0), 100000);
	/*
	 * A reset should never take more than .1 sec.
	 */
	if (c->c_io->xd_csr & XD_RACT) {
		printf("xdc%d: controller reset failed\n", ctlr);
		return (0);
	}
	/*
	 * Allocate NCBIFREE cbis to have on hand per controller
	 * to use for dynamic chaining.
	 */
	for (i = 0; i < ncbifree; i++) {
		if (!xdcreatecbi(ctlr))
			return (0);
	}

	/*
	 * If no one has allocated a spare cbi for xddump, do so.
	 */
	if (xddumpalloc++ == 0)
		if (!xdmakedumpcbi())
			return (0);
	/*
	 * Set the iopb address high bytes to 0.  This assumes that the
	 * entire IOPBMAP resides in the bottom 64K of DVMA space.
	 */
	c->c_io->xd_iopbaddr4 = 0;
	c->c_io->xd_iopbaddr3 = 0;
	/*
	 * Read the controller parameters to make sure it's a 7053.
	 */
	if ((xdcbi = xdgetcbi(c)) == NULL)
		panic("no cbi1");	/* this will never happen */
	err = (xdcmd(xdcbi, XD_RPAR | XD_CTLR, NOLPART, (caddr_t)0, 0,
	    (daddr_t)0, 0, XY_SYNCH, 0));
	if (err || xdcbi->iopb->xd_ctype != XDC_7053) {
		printf("xdc%d: unsupported controller type \n", ctlr);
		(void) xdputcbi(xdcbi);
		return (0);
	}
	/*
	 * Set the controller parameters.
	 */
	if (xdcmd(xdcbi, XD_WPAR | XD_CTLR, NOLPART, (caddr_t)0, 0,
	    (daddr_t)0, (int)xd_ctlrpar0, XY_SYNCH, 0)) {
		printf("xdc%d: ctlr initialization failed\n", ctlr);
		(void) xdputcbi(xdcbi);
		return (0);
	}
	(void) xdputcbi(xdcbi);
	c->c_flags |= XY_C_PRESENT;
	/*
	 * Initialize the controller queues.
	 */
	c->actvcmdq = NULL;
	c->rdyiopbq = NULL;
	return (sizeof (struct xddevice));
}

/*
 * See if a slave unit exists.  This routine is called only at boot time.
 * It is a wrapper for findslave() to match parameter types.
 */
xdslave(md, reg)
	register struct mb_device *md;
	caddr_t reg;
{
	register struct xdctlr *c;
	int i;

	/*
	 * Find the controller at the given io address
	 */
	for (i = 0; i < nxdc; i++)
		if (xdctlrs[i].c_io == (struct xddevice *)reg)
			break;
	if (i >= nxdc) {
		printf("xd%d: ", md->md_unit);
		panic("io address mismatch");
	}
	c = &xdctlrs[i];
	/*
	 * Use findslave() to look for the drive synchronously
	 */
	return (findslave(md, c, XY_SYNCH));
}

/*
 * Actual search for a slave unit.  Used by xdslave() synchronously and
 * xdopen() asynchronously.  This routine is always called at disk interrupt
 * priority.
 */
static
findslave(md, c, mode)
	register struct mb_device *md;
	register struct xdctlr *c;
	int mode;
{
	register struct mb_ctlr *mc;
	register struct xdcmdblock *xdcbi;
	int i, low, high;

	/*
	 * Set up the vectored interrupt to pass the ctlr pointer.
	 * Also set up the interrupt stuff in the ctlr structure.
	 * This needs to be done here so the drive reset can
	 * interrupt correctly when it completes in the asynchwait
	 * case.
	 */
	mc = xdcinfo[c - xdctlrs];
	if (mc->mc_intr) {
		*(mc->mc_intr->v_vptr) = (int)c;
		c->c_intvec = mc->mc_intr->v_vec;
		c->c_intpri = mc->mc_intpri;
	}
	/*
	 * Set up loop to search for a wildcarded drive number
	 */
	if (md->md_slave == '?') {
		low = 0;
		high = XDUNPERC-1;
	} else
		low = high = md->md_slave;
	/*
	 * Look for a drive that's online but not already taken
	 */
	for (i = low; i <= high; i++) {
		if (i >= XDUNPERC)
			break;
		if (c->c_units[i] != NULL &&
		    (c->c_units[i]->un_flags & XY_UN_PRESENT))
			continue;
		/*
		 * If a drive reset succeeds, we assume that the drive
		 * is there.
		 */
		while ((xdcbi = xdgetcbi(c)) == NULL){
			/* called from xdslave()?*/
			if (mode == XY_SYNCH)
				panic("xd findslave from xdslave- no iopb");
			else {
				xdfreewait++;
				(void) sleep((caddr_t)&xdfreewait, PRIBIO);
			}
		}
		if (!xdcmd(xdcbi, XD_RESTORE, NOLPART, (caddr_t)0, i,
		    (daddr_t)0, 0, mode, XY_NOMSG)) {
			md->md_slave = i;
			(void) xdputcbi(xdcbi);
			return (1);
		}
		(void) xdputcbi(xdcbi);
	}
	return (0);
}

/*
 * This routine is used to attach a drive to the system.  It is called only
 * at boot time.  It is a wrapper for doattach() to get the parameter
 * types matched.
 */
xdattach(md)
	register struct mb_device *md;
{

	doattach(md, XY_SYNCH);
}

/*
 * This routine does the actual work involved in attaching a drive to the
 * system.  It is called from xdattach() synchronously and xdopen()
 * asynchronously.  It is always called at disk interrupt priority.
 */
static
doattach(md, mode)
	register struct mb_device *md;
	int mode;
{
	register struct xdctlr *c = &xdctlrs[md->md_ctlr];
	register struct xdunit *un = &xdunits[md->md_unit];
	register struct dk_label *l;
	register struct xdcmdblock *xdcbi;
	int s, i, found = 0;

	/*
	 * If this disk has never been seen before, we need to set
	 * up all the structures for the device.
	 */
	if (!(un->un_flags & XY_UN_ATTACHED)) {
		un->un_flags |= XY_UN_ATTACHED;
		/*
		 * If no one has started the watchdog routine, do so.
		 * First initialize interval timeout info.
		 */
		for (i=0; i<nxdc; i++) {
			xd_watch_info[i].curr_timeout = 0;
			xd_watch_info[i].next_timeout = 0;
			xd_watch_info[i].got_interrupt = 0;
			xd_watch_info[i].got_rio = 0;
		}

		if (xdwstart++ == 0)
			timeout(xdwatch, (caddr_t)0, hz);
		/*
		 * Set up all the pointers and allocate space for the
		 * dynamic parts of the unit structure.  NOTE: if the
		 * alloc fails the system panics, so we don't need to
		 * test the return value.
		 */
		c->c_units[md->md_slave] = un;
		un->c = c;
		un->un_md = md;
		un->un_mc = md->md_mc;
		un->un_map = (struct dk_map *)new_kmem_alloc(
			sizeof (struct dk_map) * XYNLPART, KMEM_SLEEP);
		un->un_g = (struct dk_geom *)new_kmem_alloc(
			sizeof (struct dk_geom), KMEM_SLEEP);
		un->un_rtab = (struct buf *)new_kmem_alloc(
			sizeof (struct buf), KMEM_SLEEP);
		/*
		 * Zero the buffer. The other structures are initialized
		 * later by initlabel().
		 */
		bzero((caddr_t)un->un_rtab, sizeof (struct buf));
		un->un_bad = (struct dkbad *)
			new_kmem_alloc(sizeof (struct dkbad), KMEM_SLEEP);
	}
	/*
	 * Initialize the label structures.  This is necessary so weird
	 * entries in the bad sector map don't bite us while reading the
	 * label. Also, this MUST come before the write parameters command
	 * so the geometry is not random.
	 */
	initlabel(un);
	/*
	 * Execute a set the drive parameters command via usegeom().
	 * This is necessary to define EC32 (error correction) of the drive
	 * parameters now that the IRAM can't remember after the power has
	 * been off.  Don't worry about the bogus drive geometry info
	 * as it gets filled in once the label is read below.
	 */
	if (usegeom(un, mode)) {
		/* set driver parameters must have failed in usegeom */
		return;
	}
	/*
	 * Execute a set format parameters command.  This is necessary
	 * to define the sector size and interleave factors.  The other
	 * fields describe the format of each sector (gap sizes).
	 */
	while ((xdcbi = xdgetcbi(c)) == NULL) {
		if (mode == XY_SYNCH)
			panic("xd doattach from xdattach- no iopb");
		else {
			xdfreewait++;
			(void) sleep((caddr_t)&xdfreewait, PRIBIO);
		}
	}
	if (xdcmd(xdcbi, XD_WPAR | XD_FORMAT, NOLPART,
	    (caddr_t)XD_FORMPAR4, md->md_slave, (daddr_t)XD_FORMPAR3,
	    XD_FORMPAR2, mode, 0)) {
		printf("xd%d: Initialization failed (format parameters)\n",
			md->md_unit);
		(void) xdputcbi(xdcbi);
		return;
	}
	(void) xdputcbi(xdcbi);
	/*
	 * Allocate a temporary buffer in DVMA space for reading the label.
	 * Again we must lock out all main bus activity to protect the
	 * resource map.
	 */
	s = splr(pritospl(SPLMB));
	l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
	(void) splx(s);
	if (l == NULL) {
		printf("xd%d: no space for disk label\n", md->md_unit);
		return;
	}
	/*
	 * Unit is now officially present.  It can now be accessed by the
	 * system even if the rest of this routine fails.
	 */
	un->un_flags |= XY_UN_PRESENT;
	/*
	 * Attempt to read the label.  We use the silent flag so
	 * no one will know if we fail.
	 */
	while ((xdcbi = xdgetcbi(c)) == NULL) {
		if (mode == XY_SYNCH)
			panic("xd doattach from xdattach 2- no iopb");
		else {
			xdfreewait++;
			(void) sleep((caddr_t)&xdfreewait, PRIBIO);
		}
	}
	if (!xdcmd(xdcbi, XD_READ, NOLPART,
	    (caddr_t)((char *)l - DVMA), md->md_slave, (daddr_t)0, 1,
	    mode, XY_NOMSG)) {
		/*
		 * If we found a label, attempt to digest it.
		 */
		if (islabel(md->md_unit, l)) {
			uselabel(un, md->md_unit, l);
			if (!usegeom(un, mode))
				found = 1;
		}
	}
	(void) xdputcbi(xdcbi);
	/*
	 * If we found the label, attempt to read the bad sector map.
	 */
	if (found) {
		while ((xdcbi = xdgetcbi(c)) == NULL) {
			if (mode == XY_SYNCH)
				panic("xd doattach from xdattach 3- no iopb");
			else {
				xdfreewait++;
				(void) sleep((caddr_t)&xdfreewait, PRIBIO);
			}
		}
		if (xdcmd(xdcbi, XD_READ, NOLPART,
		    (caddr_t)((char *)l - DVMA), md->md_slave,
		    (daddr_t)((((un->un_g->dkg_ncyl + un->un_g->dkg_acyl) *
		    un->un_g->dkg_nhead) - 1) * un->un_g->dkg_nsect), 1,
		    mode, 0)) {
			/*
			 * If we failed, print a message and invalidate
			 * the map in case it got destroyed in the read.
			 */
			printf("xd%d: unable to read bad sector info\n",
			    un - xdunits);
			for (i = 0; i < NDKBAD; i++)
				un->un_bad->bt_bad[i].bt_cyl = 0xFFFF;
		/*
		 * Use a structure assignment to fill in the real map.
		 */
		} else
			*un->un_bad = *(struct dkbad *)l;
		(void) xdputcbi(xdcbi);
	/*
	 * If we couldn't read the label, print a message and invalidate
	 * the label structures in case they got destroyed in the reads.
	 */
	} else {
		printf("xd%d: unable to read label\n", un - xdunits);
		initlabel(un);
	}
	/*
	 * Free the buffer used for DVMA.
	 */
	s = splr(pritospl(SPLMB));
	rmfree(iopbmap, (long)SECSIZE, (u_long)l);
	(void) splx(s);
	return;
}

/*
 * This routine sets the fields of the iopb that never change.  It is
 * used by xdcreatecbi().  It is always called at disk interrupt priority.
 */
static
initiopb(xd)
	register struct xdiopb *xd;
{

	bzero((caddr_t)xd, sizeof (struct xdiopb));
	xd->xd_fixd = 1;
	xd->xd_nxtmod = XD_ADDRMOD24;
	xd->xd_prio = 0;
}

/*
 * This routine clears the fields of the iopb that must be zero before a
 * command is executed.  It is used by xdcmd() and xdrecover(). It is always
 * called at disk interrupt priority.
 */
static
cleariopb(xd)
	register struct xdiopb *xd;
{

	xd->xd_errno = 0;
	xd->xd_iserr = 0;
	xd->xd_complete = 0;
}

/*
 * This routine initializes the unit label structures.  The logical partitions
 * are set to zero so normal opens will fail.  The geometry is set to
 * nonzero small numbers as a paranoid defense against zero divides.
 * Bad sector map is filled with non-entries.
 */
static
initlabel(un)
	register struct xdunit *un;
{
	register int i;

	bzero((caddr_t)un->un_map, sizeof (struct dk_map) * XYNLPART);
	bzero((caddr_t)un->un_g, sizeof (struct dk_geom));
	un->un_g->dkg_ncyl = un->un_g->dkg_nhead = 8;
	un->un_g->dkg_nsect = 8;
	for (i = 0; i < NDKBAD; i++)
		un->un_bad->bt_bad[i].bt_cyl = 0xFFFF;
}

/*
 * This routine verifies that the block read is indeed a disk label.  It
 * is used by doattach().  It is always called at disk interrupt priority.
 */
static
islabel(unit, l)
	int unit;
	register struct dk_label *l;
{

	if (l->dkl_magic != DKL_MAGIC)
		return (0);
	if (!ck_cksum(l)) {
		printf("xd%d: corrupt label\n", unit);
		return (0);
	}
	return (1);
}

/*
 * This routine checks the checksum of the disk label.  It is used by
 * islabel().  It is always called at disk interrupt priority.
 */
static
ck_cksum(l)
	register struct dk_label *l;
{
	register short *sp, sum = 0;
	register short count = sizeof (struct dk_label)/sizeof (short);

	sp = (short *)l;
	while (count--)
		sum ^= *sp++;
	return (sum ? 0 : 1);
}

/*
 * This routine puts the label information into the various parts of
 * the unit structure.  It is used by doattach().  It is always called
 * at disk interrupt priority.
 */
static
uselabel(un, unit, l)
	register struct xdunit *un;
	int unit;
	register struct dk_label *l;
{
	int i, intrlv;

	/*
	 * Print out the disk description.
	 */
	printf("xd%d: <%s>\n", unit, l->dkl_asciilabel);
	/*
	 * Fill in the geometry information.
	 */
	un->un_g->dkg_ncyl = l->dkl_ncyl;
	un->un_g->dkg_acyl = l->dkl_acyl;
	un->un_g->dkg_bcyl = 0;
	un->un_g->dkg_nhead = l->dkl_nhead;
	un->un_g->dkg_nsect = l->dkl_nsect;
	un->un_g->dkg_intrlv = l->dkl_intrlv;
	un->un_g->dkg_rpm = l->dkl_rpm;
	un->un_g->dkg_pcyl = l->dkl_pcyl;
	/*
	 * Some labels might not have pcyl in them, so we make a guess at it.
	 */
	if (un->un_g->dkg_pcyl == 0)
		un->un_g->dkg_pcyl = un->un_g->dkg_ncyl + un->un_g->dkg_acyl;
	/*
	 * Fill in the logical partition map (structure assignments).
	 */
	for (i = 0; i < XYNLPART; i++)
		un->un_map[i] = l->dkl_map[i];
	/*
	 * Set up data for iostat.
	 */
	if (un->un_md->md_dk >= 0) {
		intrlv = un->un_g->dkg_intrlv;
		if (intrlv <= 0 || intrlv >= un->un_g->dkg_nsect)
			intrlv = 1;
		dk_bps[un->un_md->md_dk] = SECSIZE * 60 * un->un_g->dkg_nsect /
		    intrlv;
	}
}

/*
 * This routine is used to initialize the drive.  The 7053 requires
 * that each drive be set up once by sending a set drive parameter
 * command to the controller.  It is used by doattach() and xdioctl().
 * It is always called at disk interrupt priority.
 */
static
usegeom(un, mode)
	register struct xdunit *un;
	int mode;
{
	daddr_t lastb;
	register struct dk_geom *g = un->un_g;
	register struct xdcmdblock *xdcbi;

	/*
	 * Just to be safe, we make sure we are initializing the drive
	 * to the larger of the two sizes, logical or physical.
	 */
	if (g->dkg_pcyl < g->dkg_ncyl + g->dkg_acyl)
		lastb = (g->dkg_ncyl + g->dkg_acyl) * g->dkg_nhead *
		    g->dkg_nsect - 1;
	else
		lastb = g->dkg_pcyl * g->dkg_nhead * g->dkg_nsect - 1;
	/*
	 * If it fails, we return the error so the label gets
	 * invalidated.
	 */
	while ((xdcbi = xdgetcbi(un->c)) == NULL) {
		if (mode == XY_SYNCH)
			panic("xd usegeom- no iopb");
		else {
			xdfreewait++;
			(void) sleep((caddr_t)&xdfreewait, PRIBIO);
		}
	}
	if (xdcmd(xdcbi, XD_WPAR | XD_DRIVE, NOLPART,
	    (caddr_t)XD_DRPARAM7053, un->un_md->md_slave,
	    lastb, (int)((g->dkg_nsect - 1) << 8), mode, 0)) {
		printf("xd%d: initialization failed\n", un - xdunits);
		(void) xdputcbi(xdcbi);
		return (1);
	}
	(void) xdputcbi(xdcbi);
	return (0);
}

/*
 * This routine sets the fields in the iopb that are needed for an
 * asynchronous operation.  It does not start the operation.
 * It is used by xdcmd() and xdintr().
 * It is always called at disk interrupt priority.
 */
xdasynch(cmdblk)
	register struct xdcmdblock *cmdblk;
{
	register struct xdiopb *xd = cmdblk->iopb;

	xd->xd_intvec = cmdblk->c->c_intvec;
	xd->xd_intpri = cmdblk->c->c_intpri;
}

#define	MAX_LOOPS 10
/*
 * This routine executes a command synchronously.  This is only done at
 * boot time or when a dump is being done.  It is used by xdcmd().
 * It is always called at disk interrupt priority.
 */
xdsynch(cmdblk)
	register struct xdcmdblock *cmdblk;
{
	register struct xdiopb *xd = cmdblk->iopb;
	register struct xddevice *c_io = cmdblk->c->c_io;
	register u_long cbiaddr;
	u_char ferr;
	register struct xdctlr *c = cmdblk->c;
	register struct xdiopb *findaddr;
	register struct xdcmdblock *thiscmd;
	int repeat;
	int reset = 0;
	int max_loops = MAX_LOOPS;

	/*
	* Set necessary iopb fields then have the command executed.
	*/
	xd->xd_intpri = 0;	/* no interrupt */
	xdexec(cmdblk);

	do {
		repeat = 0;

		/*
		 * Wait for the command to complete or until a timeout occurs.
		 */
		CDELAY((c_io->xd_csr & XD_RIO), 1000000 * XYLOSTINTTIMO);

		/*
		 * If we timed out, use the lost interrupt error to pass
		 * back status or just reset the controller if the command
		 * had already finished.
		 */

		if (!(c_io->xd_csr & XD_RIO)) {
#ifdef notdef
			/* don't reset for now (i don't think it will help) */
			if (xd->xd_complete) {
				c_io->xd_csr = XD_RST;
				CDELAY(((c_io->xd_csr & XD_RACT) == 0), 100000);
				if (c_io->xd_csr & XD_RACT) {
					printf("xdc%d: ", cmdblk->c - xdctlrs);
					panic("controller reset failed");
				}
			} else {
#endif notdef
				xd->xd_iserr = 1;
				xd->xd_errno = XDE_LINT;
#ifdef notdef
			}
#endif notdef
		/*
		 * If the fatal error bit is set in the controller status
		 * register, we need to convey this to the recovery routine
		 * via the iopb.  However, if the iopb has a specific error
		 * reported, we don't want to overwrite it.  Therefore, we
		 * only fold the error bits into the iopb if the iopb thinks
		 * things were ok.
		 */
		} else if (c_io->xd_csr & XD_FERR) {
			ferr = c_io->xd_fatal;
			/*
			 * need to check the active command queue when resetting
			 * the controller, in case commands need to be re exec'd
			 */
			c_io->xd_csr = XD_RST;		/* clears the error */
			CDELAY(((c_io->xd_csr & XD_RACT) == 0), 100000);
			if (c_io->xd_csr & XD_RACT) {
				printf("xdc%d: ", cmdblk->c - xdctlrs);
				panic("controller reset failed");
			}
			if (!(xd->xd_iserr && xd->xd_errno)) {
				xd->xd_iserr = 1;
				xd->xd_errno = ferr;
			}
		}
		/*
		 * Ready the controller for a new command.
		 */
		if (c_io->xd_csr & XD_RIO) {
			cbiaddr = (c_io->xd_iopbaddr1) +
				((c_io->xd_iopbaddr2 << 8));
			/* sanity check */
			if (cmdblk->iopb != (struct xdiopb *)(cbiaddr + DVMA)) {
				thiscmd = c->actvcmdq;
				findaddr = (struct xdiopb *)(cbiaddr + DVMA);
				while (thiscmd && thiscmd->next &&
				    (thiscmd->iopb != findaddr)) {
					thiscmd = thiscmd->next;
				}
				/*
				 * If things are confused we will reset the
				 * controller and try to rerun the command once.
				 * If that doesn't work then things are bad and
				 * we panic. Otherwise, lower the interrupt
				 * priority to let the commands finish.
				 */
				if (!thiscmd || thiscmd->iopb != findaddr) {
				    if (reset) {
					panic("xdsynch iopb mismatch");
				    } else {
					reset = 1;
					repeat = 1;
					c_io->xd_csr = XD_RST;
					CDELAY(((c_io->xd_csr & XD_RACT) == 0),
					    100000);
					if (c_io->xd_csr & XD_RACT) {
					    printf("xdc%d: ",
						cmdblk->c - xdctlrs);
					    panic("controller reset failed");
					}
					c->rdyiopbq = NULL;
					c->actvcmdq = NULL;
					/* no interrupt */
					xd->xd_intpri = 0;
					xdexec(cmdblk);
				    }
				} else {
					repeat = 1;
					(void)splx(pritospl(c->c_intpri - 1));
				}
			}
			c_io->xd_csr = XD_CLRIO;
		}
	} while (max_loops-- && repeat);
}

/*
 * This routine is the focal point of all commands to the controller.
 * Every command passes through here, independent of its source or
 * reason.  The mode determines whether we are synchronous, asynchronous,
 * or asynchronous but waiting for completion.  The flags are used
 * to suppress error recovery and messages when we are doing special operations.
 * It is used by xdprobe(), findslave(), doattach(), usegeom(),
 * xdgo(), xdioctl(), xdwatch(), and xddump().
 * It is always called at disk interrupt priority.
 * NOTE: this routine assumes that all operations done before the disk's
 * geometry is defined are done on block 0.  This impacts both this routine
 * and the error recovery scheme (even the restores must use block 0).
 * Failure to abide by this restriction could result in an arithmetic trap.
 */
xdcmd(cmdblk, cmd, device, bufaddr, unit, blkno, secnt, mode, flags)
	register struct xdcmdblock *cmdblk;
	u_short cmd;
	int device, unit, secnt, mode, flags;
	caddr_t bufaddr;
	daddr_t blkno;
{
	register struct xdiopb *xd = cmdblk->iopb;
	register struct xdunit *un = cmdblk->c->c_units[unit];
	int stat = 0;
	u_int next_timeout;
	struct xd_ctlrpar2 *par2;
	u_int cnt;

	/*
	 * Fill in the cmdblock fields.
	 */
	cmdblk->flags = flags;
	cmdblk->retries = cmdblk->restores = cmdblk->resets = 0;
	cmdblk->slave = unit;
	cmdblk->cmd = cmd;
	cmdblk->device = device;
	cmdblk->blkno = blkno;
	cmdblk->nsect = secnt;
	cmdblk->baddr = bufaddr;
	cmdblk->failed = 0;

#ifdef	DKIOCWCHK
	if (cmd == XD_WRITE && device != NOLPART &&
	    un && (un->un_wchkmap & (1<<LPART(device))) &&
	    mode == XY_ASYNCH) {
		cmdblk->nverifies = 0;
		cmdblk->forwarded = 0;
		cmdblk->flags |= XY_INWCHK;
	}
#endif	DKIOCWCHK

	/*
	 * Determine how the expected maximum time to process
	 * this command will affect the next watchdog timer
	 * interval.  This expected maximum time is based upon
	 * the number of tracks for the command.
	 */
	if (un == NULL || un->un_g == NULL) {
		next_timeout = SINGLE_TRACK_TO;
	} else if (cmdblk->cmd == (XD_WEXT | XD_FORMVER)) {
		next_timeout = (cmdblk->nsect + 1) * SINGLE_TRACK_TO;
	} else {
		next_timeout = (cmdblk->nsect / un->un_g->dkg_nsect + 1) *
				SINGLE_TRACK_TO;
	}
	if (xd_watch_info[cmdblk->c - xdctlrs].next_timeout <
	    next_timeout) {
		xd_watch_info[cmdblk->c - xdctlrs].next_timeout = next_timeout;
	}

	/*
	 * Initialize the diagnostic info if necessary.
	 */
	if ((cmdblk->flags & XY_DIAG) && (un != NULL))
		un->un_errsevere = DK_NOERROR;
	/*
	 * Clear out the iopb fields that need it.
	 */
	cleariopb(xd);
	/*
	 * Set the iopb fields that are the same for all commands.
	 */
	xd->xd_cmd = cmd >> 8;
	xd->xd_subfunc = cmd;
	xd->xd_llength = 0;
	xd->xd_unit = unit;
	xd->xd_bufaddr = (u_int)bufaddr;
	if (((int)bufaddr + secnt * SECSIZE) > 0x1000000)
		xd->xd_bufmod = XD_ADDRMOD32;
	else
		xd->xd_bufmod = XD_ADDRMOD24;
	xd->xd_chain = 0;
	/*
	 * If the blockno is 0, we don't bother calculating the disk
	 * address.  NOTE: this is a necessary test, since some of the
	 * operations on block 0 are done while un is not yet defined.
	 * Removing the test would cause bad pointer references.
	 */
	if (blkno != 0) {
		xd->xd_cylinder = blkno / (un->un_g->dkg_nhead *
		    un->un_g->dkg_nsect);
		xd->xd_head = (blkno / un->un_g->dkg_nsect) %
		    un->un_g->dkg_nhead;
		xd->xd_sector = blkno % un->un_g->dkg_nsect;
	} else
		xd->xd_cylinder = xd->xd_head = xd->xd_sector = 0;
	xd->xd_nsect = secnt;
	/*
	 * optimize the path for reads and writes
	 */
	if ((cmd != XD_READ) && (cmd != XD_WRITE)) {
		switch (cmd) {
			/*
			 * If we are doing a set controller params command,
			 * we need to hack fields of the iopb.
			 */
			case (XD_WPAR | XD_CTLR) :
				xd->xd_throttle = (xd_ctlrpar1<<8) | xdthrottle;
				/*
				 * only look at a limited number of entries in
				 * case someone removed the last entry.  The
				 * system could run slow but at least it
				 * didn't bus error
				 */
				for (cnt = XD_MAXDELAYS, par2 = xd_ctlrpar2;
				    par2->cpu && cnt--; par2++) {
					if (cpu == par2->cpu) {
						break;
					}
				}
				/*
				 * If something was wrong then set the
				 * delay to the default value.
				 */
				if (cnt >= XD_MAXDELAYS)
					xd->xd_sector = 0;
				else
					xd->xd_sector = par2->delay;

				break;
			/*
			 * If we are doing a set drive parameters command,
			 * we need to hack in a field of the iopb.
			 *
			 * Note: xd_drparam also contains the interrupt
			 * level field. You *must* set the interrupt
			 * level *after* you set xd_drparam if you expect
			 * to get an interrupt back.
			 */
			case (XD_WPAR | XD_DRIVE) :
				xd->xd_drparam = (u_char)bufaddr;
				break;
			/*
			 * If we are doing a set format parameters command,
			 * we need to hack in a field of the iopb.
			 */
			case (XD_WPAR | XD_FORMAT) :
				xd->xd_interleave = XD_INTERLEAVE;
				break;
		}
	}
	/*
	 * If command is synchronous, execute it.  We continue to call
	 * error recovery (which will continue to execute commands) until
	 * it returns either success or complete failure.
	 */
	if (mode == XY_SYNCH) {
		xdsynch(cmdblk);
		while ((stat = xdrecover(cmdblk, xdsynch)) > 0)
			;
		return (stat);
	}
	/*
	 * If command is asynchronous, set up it's execution.  We only
	 * start the execution if the controller is in a state where it
	 * can accept another command via xdexec().
	 */
	xdasynch(cmdblk);
	xdexec(cmdblk);
	/*
	 * If we are waiting for the command to finish, sleep till it's
	 * done.  We must then wake up anyone waiting for the iopb.  If
	 * no one is waiting for the iopb, we simply start up the next
	 * unit operation.
	 */
	if (mode == XY_ASYNCHWAIT) {
		while (!(cmdblk->flags & XY_DONE)) {
			cmdblk->flags |= XY_WAIT;
			(void) sleep((caddr_t)cmdblk, PRIBIO);
		}
		stat = cmdblk->flags & XY_FAILED;
	}
	/*
	 * Always zero for ASYNCH case, true result for ASYNCHWAIT case.
	 */
	if (stat)
		return (-1);
	else
		return (0);
}

/*
 * This routine provides the error recovery for all commands to the 7053.
 * It examines the results of a just-executed command, and performs the
 * appropriate action.  It will set up at most one followup command, so
 * it needs to be called repeatedly until the error is resolved.  It
 * returns three possible values to the calling routine : 0 implies that
 * the command succeeded, 1 implies that recovery was initiated but not
 * yet finished, and -1 implies that the command failed.  By passing the
 * address of the execution function in as a parameter, the routine is
 * completely isolated from the differences between synchronous and
 * asynchronous commands.  It is used by xdcmd() and ndintr().  It is
 * always called at disk interrupt priority.
 */
xdrecover(cmdblk, execptr)
	register struct xdcmdblock *cmdblk;
	register (*execptr)();
{
	struct xdctlr *c = cmdblk->c;
	register struct xdiopb *xd = cmdblk->iopb;
	register struct xdunit *un = c->c_units[cmdblk->slave];
	struct xderract *actptr;
	struct xderror *errptr;
	int bn, ns, ndone;

	/*
	 * This tests whether an error occured.  NOTE: errors reported by
	 * the status register of the controller must be folded into the
	 * iopb before this routine is called or they will not be noticed.
	 */
	if (xd->xd_iserr) {
		errptr = finderr(xd->xd_errno);
		/*
		 * If drive isn't even attached, we want to use error
		 * recovery but must be careful not to reference null
		 * pointers.
		 */
		if (un != NULL) {
			/*
			 * Drive has been taken offline.  Since the operation
			 * can't succeed, there's no use trying any more.
			 */
			if (!(un->un_flags & XY_UN_PRESENT)) {
				bn = cmdblk->blkno;
				ndone = 0;
				goto fail;
			}
			/*
			 * Extract from the iopb the sector in error and
			 * how many sectors succeeded.
			 */
			bn = ((xd->xd_cylinder * un->un_g->dkg_nhead) +
			    xd->xd_head) * un->un_g->dkg_nsect + xd->xd_sector;
			ndone = bn - cmdblk->blkno;
			/*
			 * Log the error for diagnostics if appropriate.
			 */
			if (cmdblk->flags & XY_DIAG) {
				un->un_errsect = bn;
				un->un_errno = errptr->errno;
				un->un_errcmd =
				    (xd->xd_cmd << 8) | xd->xd_subfunc;
			}
		} else
			bn = ndone = 0;
		if (errptr->errlevel != XD_ERCOR) {
			/*
			 * If the error wasn't corrected, see if it's a
			 * bad block.  If we are already in the middle of
			 * forwarding a bad block, we are not allowed to
			 * encounter another one.  NOTE: the check of the
			 * command is to avoid false mappings during initial
			 * stuff like trying to reset a drive
			 * (the bad map hasn't been initialized).
			 */
			if (((xd->xd_cmd == (XD_READ >> 8)) ||
			    (xd->xd_cmd == (XD_WRITE >> 8))) &&
			    (ns = isbad(un->un_bad, (int)xd->xd_cylinder,
			    (int)xd->xd_head, (int)xd->xd_sector)) >= 0) {
				if (cmdblk->flags & XY_INFRD) {
					printf("xd%d: recursive mapping --",
					    un->un_md->md_unit);
					printf(" block #%d\n", bn);
					goto fail;
				}
				/*
				 * We have a bad block.  Advance the state
				 * info up to the block that failed.
				 */
				cmdblk->baddr += ndone * SECSIZE;
				cmdblk->blkno += ndone;
				cmdblk->nsect -= ndone;
				/*
				 * Calculate the location of the alternate
				 * block.
				 */
				cmdblk->altblkno = bn = (((un->un_g->dkg_ncyl +
				    un->un_g->dkg_acyl) *
				    un->un_g->dkg_nhead) - 1) *
				    un->un_g->dkg_nsect - ns - 1;
				/*
				 * Set state to 'forwarding' and print msg
				 */
#ifdef	DKIOCWCHK
				cmdblk->forwarded = 1;
#endif	DKIOCWCHK
				cmdblk->flags |= XY_INFRD;
				if ((xderrlvl & EL_FORWARD) &&
				    (!(cmdblk->flags & XY_NOMSG))) {
					printf("xd%d: forwarding %d/%d/%d",
					    un->un_md->md_unit,
					    xd->xd_cylinder, xd->xd_head,
					    xd->xd_sector);
					printf(" to alt blk #%d\n", ns);
				}
				ns = 1;
				/*
				 * Execute the command on the alternate block
				 */
				goto exec;
			}
			/*
			 * Error was 'real'.  Look up action to take.
			 */
			if (cmdblk->flags & XY_DIAG)
				cmdblk->failed = 1;
			actptr = &xderracts[errptr->errlevel];
			/*
			 * Attempt to retry the entire operation if appropriate.
			 */
			if (cmdblk->retries++ < actptr->retry) {
				if ((xderrlvl & EL_RETRY) &&
				    (!(cmdblk->flags & XY_NOMSG)) &&
				    (errptr->errlevel != XD_ERBSY))
					printerr(un, xd->xd_cmd, cmdblk->device,
					    "retry", errptr->errmsg, bn);
				if (cmdblk->flags & XY_INFRD) {
					bn = cmdblk->altblkno;
					ns = 1;
				} else {
					bn = cmdblk->blkno;
					ns = cmdblk->nsect;
				}
				goto exec;
			}
			/*
			 * Attempt to restore the drive if appropriate.  We
			 * set the state to 'restoring' so we know where we
			 * are.  NOTE: there is no check for a recursive
			 * restore, since that is a non-destructive condition.
			 */
			if (cmdblk->restores++ < actptr->restore) {
				if ((xderrlvl & EL_RESTR) &&
				    (!(cmdblk->flags & XY_NOMSG)))
					printerr(un, xd->xd_cmd, cmdblk->device,
					    "restore", errptr->errmsg, bn);
				cmdblk->retries = 0;
				bn = ns = 0;
				xd->xd_cmd = XD_RESTORE >> 8;
				xd->xd_subfunc = 0;
				cmdblk->flags |= XY_INRST;
				goto exec;
			}
			/*
			 * Attempt to reset the controller if appropriate.
			 * We must busy wait for the controller, since no
			 * interrupt is generated.
			 */
			if (cmdblk->resets++ < actptr->reset) {
				if ((xderrlvl & EL_RESET) &&
				    (!(cmdblk->flags & XY_NOMSG)))
					printerr(un, xd->xd_cmd, cmdblk->device,
					    "reset", errptr->errmsg, bn);
				cmdblk->retries = cmdblk->restores = 0;
/*
 * need to check for outstanding commands, since resetting the controller
 * blows them all away
 */
				c->c_io->xd_csr = XD_RST;
				CDELAY(((c->c_io->xd_csr & XD_RACT) == 0),
				    100000);
				if (c->c_io->xd_csr & XD_RACT) {
					printf("xdc%d: ", c - xdctlrs);
					panic("controller reset failed");
				}
				if (cmdblk->flags & XY_INFRD) {
					bn = cmdblk->altblkno;
					ns = 1;
				} else {
					bn = cmdblk->blkno;
					ns = cmdblk->nsect;
				}
				goto exec;
			}
			/*
			 * Command has failed.  We either fell through to
			 * here by running out of recovery or jumped here
			 * for a good reason.
			 */
fail:
#ifdef	DKIOCWCHK
			/*
			 * If the command specified error states that
			 * we should pass the error along (silently)
			 * do so.
			 */

			if (errptr->errlevel == XD_ERPAS) {
				return (-1);
			}
#endif	DKIOCWCHK
			if ((xderrlvl & EL_FAIL) &&
			    (!(cmdblk->flags & XY_NOMSG)))
				printerr(un, xd->xd_cmd, cmdblk->device,
				    "failed", errptr->errmsg, bn);
			/*
			 * If the failure was caused by
			 * a 'drive busy' type error, the drive has probably
			 * been taken offline, so we mark it as gone.
			 */
			if ((errptr->errlevel == XD_ERBSY) &&
			    (un != NULL) && (un->un_flags & XY_UN_PRESENT)) {
				un->un_flags &= ~XY_UN_PRESENT;
				if (un->un_md->md_dk >= 0)
					dk_bps[un->un_md->md_dk] = 0;
				printf("xd%d: offline\n", un->un_md->md_unit);
#ifdef	DKIOCWCHK
				cmdblk->flags &= ~XY_INWCHK;
				un->un_wchkmap = 0;
#endif	DKIOCWCHK
			}
			/*
			 * If the failure implies the drive is faulted, do
			 * one final restore in hopes of clearing the fault.
			 */
			if ((errptr->errlevel == XD_ERFLT) &&
			    (!(cmdblk->flags & XY_FNLRST))) {
				bn = ns = 0;
				xd->xd_cmd = XD_RESTORE >> 8;
				xd->xd_subfunc = 0;
				cmdblk->flags &= ~(XY_INFRD | XY_INRST);
				cmdblk->flags |= XY_FNLRST;
				cmdblk->failed = 1;
				goto exec;
			}
			/*
			 * If the failure implies the controller is hung, do
			 * a controller reset in hopes of fixing its state.
			 */
			if (errptr->errlevel == XD_ERHNG) {
/*
 * need to check for outstanding commands since resetting the controller
 * blows them all away
 */
				c->c_io->xd_csr = XD_RST;
				CDELAY(((c->c_io->xd_csr & XD_RACT) == 0),
				    100000);
				if (c->c_io->xd_csr & XD_RACT) {
					printf("xdc%d: ", c - xdctlrs);
					panic("controller reset failed");
				}
			}
			/*
			 * Note the failure for diagnostics and return it.
			 */
			if (cmdblk->flags & XY_DIAG)
				un->un_errsevere = DK_FATAL;
			return (-1);
		}
		/*
		 * A corrected error.  Simply print a message and go on.
		 */
		if (cmdblk->flags & XY_DIAG) {
			cmdblk->failed = 1;
			un->un_errsevere = DK_CORRECTED;
		}
		if ((xderrlvl & EL_FIXED) && (!(cmdblk->flags & XY_NOMSG)))
			printerr(un, xd->xd_cmd, cmdblk->device,
			    "fixed", errptr->errmsg, bn);
	}
	/*
	 * Command succeeded.  Either there was no error or it was corrected.
	 * We aren't done yet, since the command executed may have been part
	 * of the recovery procedure.  We check for 'restoring' and
	 * 'forwarding' states and restart the original operation if we
	 * find one.
	 */
	if (cmdblk->flags & XY_INRST) {
		xd->xd_cmd = cmdblk->cmd >> 8;
		xd->xd_subfunc = cmdblk->cmd;
		if (cmdblk->flags & XY_INFRD) {
			bn = cmdblk->altblkno;
			ns = 1;
		} else {
			bn = cmdblk->blkno;
			ns = cmdblk->nsect;
		}
		cmdblk->flags &= ~XY_INRST;
		goto exec;
	}
	if (cmdblk->flags & XY_INFRD) {
		cmdblk->baddr += SECSIZE;
		bn = ++cmdblk->blkno;
		ns = --cmdblk->nsect;
		cmdblk->flags &= ~XY_INFRD;
		if (ns > 0)
			goto exec;
	}

	/*
	 * Last command succeeded. However, since the overall command
	 * may have failed, we return the appropriate status.
	 */
	if (cmdblk->failed) {
		/*
		 * Figure out whether or not the work got done so
		 * diagnostics knows what happened.
		 */
		if ((cmdblk->flags & XY_DIAG) &&
		    (un->un_errsevere == DK_NOERROR)) {
			if (cmdblk->flags & XY_FNLRST)
				un->un_errsevere = DK_FATAL;
			else
				un->un_errsevere = DK_RECOVERED;
		}
		return (-1);
	} else
		return (0);
	/*
	 * Executes the command set up above.
	 * Only calculate the disk address if block isn't zero.  This is
	 * necessary since some of the operations of block 0 occur before
	 * the disk geometry is known (could result in zero divides).
	 */
exec:
	if (bn > 0) {
		xd->xd_cylinder = bn / (un->un_g->dkg_nhead *
		    un->un_g->dkg_nsect);
		xd->xd_head = (bn / un->un_g->dkg_nsect) % un->un_g->dkg_nhead;
		xd->xd_sector = bn % un->un_g->dkg_nsect;
	} else
		xd->xd_cylinder = xd->xd_head = xd->xd_sector = 0;
	xd->xd_nsect = ns;
	xd->xd_unit = cmdblk->slave;
	xd->xd_bufaddr = (u_int)cmdblk->baddr;
	/*
	 * Clear out the iopb fields that need it.
	 */
	cleariopb(xd);
	/*
	 * If we are doing a set controller params command, we need to
	 * hack in a field of the iopb.
	 */
	if (cmdblk->cmd == (XD_WPAR | XD_CTLR))
		xd->xd_throttle = (xd_ctlrpar1<<8) | xdthrottle;
	/*
	 * Execute the command and return a 'to be continued' status.
	 */
	(*execptr)(cmdblk);
	return (1);
}

/*
 * This routine searches the error structure for the specified error and
 * returns a pointer to the structure entry.  If the error is not found,
 * the last entry in the error table is returned (this MUST be left as
 * unknown error).  It is used by xdrecover().  It is always called at
 * disk interrupt priority.
 */
static struct xderror *
finderr(errno)
	register u_char errno;
{
	register struct xderror *errptr;

	for (errptr = xderrors; errptr->errno != XDE_UNKN; errptr++)
		if (errptr->errno == errno)
			break;
	return (errptr);
}

/*
 * This routine prints out an error message containing as much data
 * as is known.
 */
static
printerr(un, cmd, device, action, msg, bn)
	struct xdunit *un;
	u_char cmd;
	short device;
	char *action, *msg;
	int bn;
{

	if (device != NOLPART)
		printf("xd%d%c: ", UNIT(device), LPART(device) + 'a');
	else if (un != NULL)
		printf("xd%d: ", un->un_md->md_unit);
	else
		printf("xd: ");
	if (cmd < XDCMDS) {
		printf("%s ", xdcmdnames[cmd]);
	} else {
		printf("[cmd 0x%x] ", cmd);
	}
	printf("%s (%s) -- ", action, msg);
	if ((device != NOLPART) && (un != NULL))
		printf("blk #%d, ", bn - un->un_map[LPART(device)].dkl_cylno *
		    un->un_g->dkg_nsect * un->un_g->dkg_nhead);
	printf("abs blk #%d\n", bn);
}

/*
 * This routine is the actual interface to the controller registers.  It
 * starts the controller up on the iopb passed.  It is used by ...
 * It is always called at disk interrupt priority.
 */
/*
 * NOTE : We do not initialize the iopbaddr high bytes
 * in the controller registers for every command.  They
 * were set to zero during initialization.  This approach
 * assumes that all iopbs are in the first 64K of DVMA space.
 */
xdexec(xdcbi)
	register struct xdcmdblock *xdcbi;
{
	register struct xdctlr *c = xdcbi->c;
	register struct xddevice *xdio = c->c_io;
	register struct xdiopb *riopb;
	int s;

	s = splr(pritospl(SPLMB));
	if (c->rdyiopbq == NULL) {
		if (!(xdio->xd_csr & XD_AIOP)) {
			xdpushiopb(xdcbi->iopb, xdio);
			(void) splx(s);
			return;
		}
	}
	/*
	 * if we're here, then either there is something on the
	 * iopb ready queue or the controller isn't ready so
	 * first put this iopb on the queue.
	 */
	if (c->rdyiopbq == NULL) {
		c->rdyiopbq = xdcbi->iopb;
	} else {
		c->lrdy->xd_nxtaddr = xdcbi->iopb;
	}
	xdcbi->iopb->xd_nxtaddr = NULL;
	c->lrdy = xdcbi->iopb;
	/*
	 * now pull iopbs off the ready queue as long as the controller is
	 * ready to accept them.
	 */
	while (!(xdio->xd_csr & XD_AIOP) && c->rdyiopbq) {
		riopb = c->rdyiopbq;
		c->rdyiopbq = riopb->xd_nxtaddr;
		riopb->xd_nxtaddr = NULL;
		xdpushiopb(riopb, xdio);
	}
	(void) splx(s);
}

/*
 * this routine is called only from xdexec when the controller is ready
 * to accept another iopb.
 */
xdpushiopb(xd, xdio)
	register struct xdiopb *xd;
	register struct xddevice *xdio;
{
	register int iopbaddr;
	/*
	 * Calculate the address of the iopb.
	 */
	iopbaddr = ((char *)xd) - DVMA;
	/*
	 * Set the iopb address registers and
	 * the address modifier.
	 */
	xdio->xd_iopbaddr1 = XDOFF(iopbaddr, 0);
	xdio->xd_iopbaddr2 = XDOFF(iopbaddr, 1);
	xdio->xd_modifier = XD_ADDRMOD24;
	/*
	 * Set the go bit.
	 */
	xdio->xd_csr = XD_AIO;
}

/*
 * This routine opens the device.  It is designed so that a disk can
 * be spun up after the system is running and an open will automatically
 * attach it as if it had been there all along.  It is called from the
 * device switch at normal priority.
 */
/*
 * NOTE: there is a synchronization issue that is not resolved here. If
 * a disk is spun up and two processes attempt to open it at the same time,
 * they may both attempt to attach the disk. This is extremely unlikely, and
 * non-fatal in most cases, but should be fixed.
 */
xdopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct xdunit *un;
	register struct mb_device *md;
	struct xdctlr *c;
	int i, high, low, unit, s;

	unit = UNIT(dev);
	/*
	 * Ensure that the device is configured.
	 */
	if (unit >= nxd)
		return (ENXIO);
	/*
	 * If the disk is not present, we need to look for it.
	 */
	un = &xdunits[unit];
	if (!(un->un_flags & XY_UN_PRESENT)) {
		/*
		 * This is minor overkill, since all we really need to do
		 * is lock out disk interrupts while we are doing disk
		 * commands.  However, there is no easy way to tell what
		 * the disk priority is at this point.  Since this situation
		 * only occurs once at disk spin-up, the extra lock-out
		 * shouldn't hurt very much.
		 */
		s = splr(pritospl(SPLMB));
		/*
		 * Search through the table of devices for a match.
		 */
		for (md = mbdinit; md->md_driver; md++) {
			if (md->md_driver != &xdcdriver || md->md_unit != unit)
				continue;
			/*
			 * Found a match.  If the controller is wild-carded,
			 * we will look at each controller in order until
			 * we find a disk.
			 */
			if (md->md_ctlr == '?') {
				low = 0;
				high = nxdc - 1;
			} else
				low = high = md->md_ctlr;
			/*
			 * Do the actual search for an available disk.
			 */
			for (i = low; i <= high; i++) {
				c = &xdctlrs[i];
				if (!(c->c_flags & XY_C_PRESENT))
					continue;
				if (!findslave(md, c, XY_ASYNCHWAIT))
					continue;
				/*
				 * We found an online disk.  If it has
				 * never been attached before, we simulate
				 * the autoconf code and set up the necessary
				 * data.
				 */
				if (!md->md_alive) {
					md->md_alive = 1;
					md->md_ctlr = i;
					md->md_mc = xdcinfo[i];
					md->md_hd = md->md_mc->mc_mh;
					md->md_addr = md->md_mc->mc_addr;
					xddinfo[unit] = md;
					if (md->md_dk && dkn < DK_NDRIVE)
						md->md_dk = dkn++;
					else
						md->md_dk = -1;
				}
				/*
				 * Print the found message and attach the
				 * disk to the system.
				 */
				printf("xd%d at xdc%d slave %d\n",
				    md->md_unit, md->md_ctlr, md->md_slave);
				doattach(md, XY_ASYNCHWAIT);
				break;
			}
			break;
		}
		(void) splx(s);
	}
	/*
	 * By the time we get here the disk is marked present if it exists
	 * at all.  We simply check to be sure the partition being opened
	 * is nonzero in size.  If a raw partition is opened with the
	 * nodelay flag, we let it succeed even if the size is zero.  This
	 * allows ioctls to later set the geometry and partitions.
	 */
	if (un->un_flags & XY_UN_PRESENT) {
		if (un->un_map[LPART(dev)].dkl_nblk > 0)
			return (0);
		if ((cdevsw[major(dev)].d_open == xdopen) &&
		    (flag & (FNDELAY | FNBIO | FNONBIO)))
			return (0);
	}
	return (ENXIO);
}

/*
 * This routine returns the size of a logical partition.  It is called
 * from the device switch at normal priority.
 */
int
xdsize(dev)
	dev_t dev;
{
	struct xdunit *un;
	struct dk_map *lp;

	/*
	 * Ensure that the device is configured.
	 */
	if (UNIT(dev) >= nxd)
		return (-1);
	un = &xdunits[UNIT(dev)];
	lp = &un->un_map[LPART(dev)];
	if (!(un->un_flags & XY_UN_PRESENT))
		return (-1);
	return ((int)lp->dkl_nblk);
}

/*
 * This routine is the high level interface to the disk.  It performs
 * reads and writes on the disk using the buf as the method of communication.
 * It is called from the device switch for block operations and via physio()
 * for raw operations.  It is called at normal priority.
 */
xdstrategy(bp)
	register struct buf *bp;
{
	register struct xdunit *un;
	register struct dk_map *lp;
	register int unit, s;
	daddr_t bn;

	unit = dkunit(bp);
	if (unit >= nxd)
		goto bad;

	un = &xdunits[unit];
	lp = &un->un_map[LPART(bp->b_dev)];
	bn = dkblock(bp);
	/*
	 * Basic error checking.  If the disk isn't there or the operation
	 * is past the end of the partition, it's an error.
	 */
	if (!(un->un_flags & XY_UN_PRESENT))
		goto bad;
	if (bn > lp->dkl_nblk || lp->dkl_nblk == 0)
		goto bad;
	/*
	 * If the operation is at the end of the partition, we save time
	 * by skipping out now.
	 */
	if (bn == lp->dkl_nblk) {
		bp->b_resid = bp->b_bcount;
		iodone(bp);
		return;
	}
	/*
	 * We're playing with queues, so we lock out interrupts.
	 */
	s = splr(pritospl(un->un_mc->mc_intpri));
	/*
	 * queue the buf - fifo, the controller sorts the disk operations now.
	 */
	if (waitbufq == NULL) {
		waitbufq = bp;
	} else {
		lbq->av_forw = bp;
	}
	bp->av_forw = NULL;
	lbq = bp;

	/* run the buf queue */
	(void) xdstart((caddr_t)NULL);
	(void) splx(s);
	return;
bad:
	/*
	 * The operation was erroneous for some reason.  Mark the buffer
	 * with the error and call it done.
	 */
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * called from xdstrategy and xdintr() to run the buf queue.
 */
/* ARGSUSED */
xdstart(arg)
	caddr_t arg;
{
	register struct buf *bp;
	register struct xdcmdblock *xdcbi;
	register struct xdunit *un;
	register int bufaddr, unit, s;

	/* while there are queued bufs and available cbis ... */
	for (bp = waitbufq; bp && xdcbifree; bp = bp->av_forw) {
		unit = dkunit(bp);
		un = &xdunits[unit];
		s = splr(pritospl(un->un_mc->mc_intpri));
		if (bufaddr = mb_mapalloc(un->un_mc->mc_mh->mh_map, bp,
			    MB_CANTWAIT, xdstart, (caddr_t)NULL)) {
			if (waitbufq == NULL)
				panic("xdstart: waitbufq == NULL");
			if (un->c == NULL)
				panic("xdstart: un->c == NULL");
			xdcbi = xdgetcbi(un->c);
			xdcbi->un = un;
			xdcbi->breq = bp;
			xdcbi->mbaddr = (caddr_t)bufaddr;
			xdcbi->baddr = (caddr_t)bufaddr;
			xdgo(xdcbi);
		} else {
			waitbufq = bp;
			(void) splx(s);
			return (DVMA_RUNOUT);
		}
		(void) splx(s);
	}
	waitbufq = bp;
	return (0);
}

/*
 * This routine translates a buf oriented command down to a level where it
 * can actually be executed.  It is called after the necessary
 * mainbus space has been allocated.  It is always called at disk interrupt
 * priority.
 */
xdgo(xdcbi)
	register struct xdcmdblock *xdcbi;
{
	register struct xdunit *un = xdcbi->un;
	register struct dk_map *lp;
	register struct buf *bp;
	int bufaddr, secnt, dk;
	u_short cmd;
	daddr_t blkno;

	/*
	 * Extract the address of the mainbus space for the operation.
	 */
	bufaddr = MBI_ADDR(xdcbi->baddr);
	/*
	 * Calculate how many sectors we really want to operate on and
	 * set resid to reflect it.
	 */
	bp = xdcbi->breq;
	lp = &un->un_map[LPART(bp->b_dev)];
	secnt = howmany(bp->b_bcount, SECSIZE);
	secnt = MIN(secnt, lp->dkl_nblk - dkblock(bp));
	bp->b_resid = bp->b_bcount - secnt * SECSIZE;
	/*
	 * Calculate all the parameters needed to execute the command.
	 */
	if (bp->b_flags & B_READ)
		cmd = XD_READ;
	else
		cmd = XD_WRITE;
	blkno = dkblock(bp);
	blkno += lp->dkl_cylno * un->un_g->dkg_nhead * un->un_g->dkg_nsect;
	/*
	 * Update stuff for iostat.
	 */
	if ((dk = un->un_md->md_dk) >= 0) {
		dk_busy |= 1 << dk;
		dk_seek[dk]++;
		dk_xfer[dk]++;
		if (bp->b_flags & B_READ)
			dk_read[dk]++;
		dk_wds[dk] += bp->b_bcount >> 6;
	}
	/*
	 * Execute the command.
	 */
	(void) xdcmd(xdcbi, cmd, minor(bp->b_dev), (caddr_t)bufaddr,
	    un->un_md->md_slave, blkno, secnt, XY_ASYNCH, 0);
}

#ifdef	DKIOCWCHK

/*
 * Write Check nuts and bolts:
 *
 *	When a write command completes, it's checked to see (in xdintr())
 *	whether write verification needs to be done. If it has to be done,
 *	we go of and do it. We use the READ_EXTENDED/sub VERIFY command
 *	for this. The only thing that is tough is handling mapped blocks.
 *	The VERIFY command won't tell you where the VERIFY failed- just
 *	that it did. In order to handle writes that had forwarding in
 *	the middle of them we note that a forwarding operation took place,
 *	and break up the verify into pieces, doing the forwarding by hand
 *	ourselves at appropriate places.
 */

/*
 * xdvforw- handle verification of transfers with forwarded (mapped) sectors
 * in them. *{ secnt, blkno, mbaddr } are always the original complete values
 * for the original write request (as recalculated in xdverify). We modify
 * them here based upon how far we are through the verify.
 */

static void
xdvforw(cmdblk, secnt, blkno, mbaddr)
	register struct xdcmdblock *cmdblk;
	int *secnt;
	daddr_t *blkno;
	u_int *mbaddr;
{
	register struct xdunit *un;
	register cyl, head, sector;
	daddr_t eblk, blk, ndone;
	int bdidx;

	/*
	 * Fetch hi-water block number for the entire transfer.
	 */

	eblk = *blkno + *secnt;

	/*
	 * If the current block number is not the starting
	 * block number, figure out how far we've gone already.
	 */

	ndone = cmdblk->vblk - *blkno;

	if (ndone) {

		/*
		 * bump DMA cookie by how far we've
		 * gotten in the verify.
		 */

		*mbaddr += (ndone * SECSIZE);

		/*
		 * And repoint blkno to start here.
		 */

		*blkno = cmdblk->vblk;

		/*
		 * And adjust sector count appropriately
		 */

		*secnt -= ndone;

	}

	/*
	 * Save what is going to be our next starting block number
	 */

	cmdblk->vblk = *blkno;

	/*
	 * load pointer to unit structure (an adult
	 * compiler would make this step unnecessary).
	 */

	un = cmdblk->un;

	/*
	 * Now search for any forwarded blocks in the
	 * range from *blkno to the end of the transfer.
	 */

	for (blk = *blkno; blk < eblk; blk++) {
		cyl = blk / (un->un_g->dkg_nhead * un->un_g->dkg_nsect);
		head = (blk / un->un_g->dkg_nsect) % un->un_g->dkg_nhead;
		sector = blk % un->un_g->dkg_nsect;
		bdidx = isbad(un->un_bad, cyl, head, sector);
		if (bdidx >= 0)
			break;
	}

	/*
	 * If we run up to the end of the transfer,
	 * there aren't any more forwardings to deal with.
	 */

	if (blk == eblk) {
		cmdblk->forwarded = 0;
		return;
	}

	/*
	 * blk now is the block number of a block that has
	 * been forwarded. If there are > 0 blocks between
	 * our starting point (*blkno) and ending point (blk),
	 * we do them first. When we return from doing them we
	 * then reduce to the case of blk == *blkno.
	 *
	 * When this is the case, we set the sector
	 * count to 1 and stuff *blkno with the alternate
	 * block number (i.e., do the forwarding).
	 */

	*secnt = blk - *blkno;
	if (*secnt == 0) {
		*secnt = 1;
		*blkno = cmdblk->altblkno =
		    (((un->un_g->dkg_ncyl + un->un_g->dkg_acyl) *
			un->un_g->dkg_nhead) - 1) *
			un->un_g->dkg_nsect - bdidx - 1;
	}

	/*
	 * check to make sure that this doesn't finish the whole verification.
	 * If the last block is a forwarded block, then that block number
	 * plus one will equal eblk.
	 */

	if (blk+1 == eblk) {
		cmdblk->forwarded = 0;
	}
}


static int
xdverify(cmdblk, stat)
	register struct xdcmdblock *cmdblk;
	int stat;
{
	static char *rewrite = "rewrite after verify failure";
	register struct buf *bp;
	register struct dk_map *lp;
	register struct xdunit *un;
	auto int secnt;
	auto daddr_t blkno;
	auto u_int mbaddr;
	u_short newcmd = 0;
	int rval = 0;

	bp = cmdblk->breq;
	un = cmdblk->un;
	lp = &un->un_map[LPART(bp->b_dev)];
	secnt = howmany(bp->b_bcount, SECSIZE);
	secnt = MIN(secnt, lp->dkl_nblk - dkblock(bp));
	blkno = dkblock(bp);
	blkno += lp->dkl_cylno * un->un_g->dkg_nhead * un->un_g->dkg_nsect;
	mbaddr = MBI_ADDR(cmdblk->mbaddr);

	if (cmdblk->cmd == XD_WRITE && stat == 0) {
		newcmd = XD_VERIFY;
		if (cmdblk->forwarded) {
			/*
			 * The write command had one or more forwarded
			 * blocks in it. We can't simply use the verify
			 * command, because the verify command doesn't
			 * report where it failed, just that it failed,
			 * so we can't use the same method that the write
			 * command used to vector to a forwarded alternate
			 * block.
			 */
			cmdblk->vblk = blkno;
			xdvforw(cmdblk, &secnt, &blkno, &mbaddr);
		}
	} else if (cmdblk->cmd == XD_VERIFY) {
		if (stat < 0) {
			if (cmdblk->nverifies++ < XY_REWRITE_MAX) {
				/*
				 * zap the forwarding flag
				 */
				cmdblk->forwarded = 0;
				newcmd = XD_WRITE;
			} else {
				printerr(un, XD_WRITE>>8, cmdblk->device,
				    "failed", rewrite, (int) blkno);
				rval = -1;
			}
		} else if (cmdblk->forwarded) {
			/*
			 * adjust 'real' block number up to account
			 * for portion of VERIFY that just completed
			 */
			newcmd = XD_VERIFY;
			cmdblk->vblk += cmdblk->nsect;
			xdvforw(cmdblk, &secnt, &blkno, &mbaddr);
		} else if (cmdblk->nverifies != 0) {
			printerr(un, XD_WRITE>>8, cmdblk->device,
			    "fixed", rewrite, (int) blkno);
		}
	}

	/*
	 * If newcmd is zero, then we're done
	 */
	if (newcmd == 0) {
		cmdblk->nverifies = 0;
		cmdblk->forwarded = 0;
		cmdblk->flags &= ~XY_INWCHK;
		/*
		 * we have to restore the original write
		 * command because xdintr() uses it.
		 */
		cmdblk->cmd = XD_WRITE;
	} else {
		rval = 1;
		(void) xdcmd(cmdblk, newcmd, cmdblk->device, (caddr_t) mbaddr,
		    (int) cmdblk->slave, blkno,
		    secnt, XY_ASYNCH, cmdblk->flags);
	}
	return (rval);
}

#endif	DKIOCWCHK

/*
 * This routine handles controller interrupts.  It is called by xdwatch()
 * or whenever a vectored interrupt for a 7053 is received.
 * It is always called at disk interrupt priority.
 */
xdintr(c)
	register struct xdctlr *c;
{
	register struct xdcmdblock *thiscmd;
	register struct xdunit *un;
	register struct xdiopb *findaddr;
	register struct xdiopb *riopb;
	int stat;
	u_char ferr;

	/*
	 * note for the watchdog timer handler that we
	 * have gotten an interrupt for this controller
	 * in the current timer interval.
	 */
	xd_watch_info[c - xdctlrs].got_interrupt = 1;

	/*
	 * check for a spurious interrupt.
	 */
	if (!(c->c_io->xd_csr & XD_RIO)) {
		printf("xdc%d: spurious interrupt - no rio\n",
			c - xdctlrs);
		return;
	}

	/* reset the watch for XD_RIO */
	xd_watch_info[c - xdctlrs].got_rio = 0;

	/*
	 * Grab the address of the complete iopb and -
	 * Ready the controller for a new command.
	 */
	findaddr = (struct xdiopb *)(DVMA + (c->c_io->xd_iopbaddr1) +
		(c->c_io->xd_iopbaddr2 << 8));
	c->c_io->xd_csr = XD_CLRIO;
	/* find the cmdblk on the active queue */
	thiscmd = c->actvcmdq;
	while (thiscmd->next && (thiscmd->iopb != findaddr))
		thiscmd = thiscmd->next;
	if (thiscmd->iopb != findaddr) {
		printf("xdc%d: returned unmatched iopb addr %x\n",
			c - xdctlrs, findaddr);
		return;
	}
	/*
	 * If the controller claims an error occurred, we need to assign
	 * it to an iopb so the error recovery will see it.
	 */
	if (c->c_io->xd_csr & XD_FERR) {
		ferr = c->c_io->xd_fatal;
		/*
		 * need to check the active command queue when resetting
		 * the controller, in case commands need to be re exec'd
		 */
		c->c_io->xd_csr = XD_RST;	/* clears the error */
		CDELAY(((c->c_io->xd_csr & XD_RACT) == 0), 100000);
		if (c->c_io->xd_csr & XD_RACT) {
			printf("xdc%d: ", c - xdctlrs);
			panic("controller reset failed");
		}
		/*
		 * Fold the register error into the iopb.
		 */
		thiscmd->iopb->xd_iserr = 1;
		thiscmd->iopb->xd_errno = ferr;
		thiscmd->iopb->xd_complete = 1;
	}
	/*
	 * Now handle the iopb and error recovery.
	 */
	un = c->c_units[thiscmd->slave];
	/*
	 * Mark the unit as used and not busy.
	 */
	if (un != NULL) {
		un->un_ltick = xdticks;
		dk_busy &= ~(1 << un->un_md->md_dk);
	}
	/*
	 * Execute the error recovery on the iopb.
	 * If stat came back greater than zero, the
	 * error recovery has re-executed the command.
	 * but it needs to be continued ...
	 */
	if ((stat = xdrecover(thiscmd, xdasynch)) > 0) {
		xdexec(thiscmd);
	} else {
#ifdef	DKIOCWCHK
		if (thiscmd->flags & XY_INWCHK) {
			if ((stat = xdverify(thiscmd, stat)) > 0)
				return;
		}
#endif	DKIOCWCHK
		/*
		 * In the ASYNCHWAIT case, we pass back
		 * status via the flags and wakeup the
		 * calling process.
		 */
		if (thiscmd->flags & XY_WAIT) {
			thiscmd->flags &= ~XY_WAIT;
			if (stat < 0)
				thiscmd->flags |= XY_FAILED;
			thiscmd->flags |= XY_DONE;
			wakeup((caddr_t)thiscmd);
		/*
		 * In the ASYNCH case, we pass back status
		 * via the buffer.  If the command used
		 * mainbus space, we release that.  If
		 * someone wants the iopb, wake
		 * them up, otherwise start up the next
		 * buf operation.
		 */
		} else {
			if ((thiscmd->cmd == XD_READ) ||
			    (thiscmd->cmd == XD_WRITE)) {
				if (stat == -1)
					thiscmd->breq->b_flags |= B_ERROR;
				mb_mapfree(un->un_mc->mc_mh->mh_map,
					(int *)&thiscmd->mbaddr);
				/*
				* Some conditions, as in the case of a
				* write or read retry, may cause more than
				* one interrupt in immediate succession
				* for a given buf
				* operation even though the first
				* operation successfully completed.  To
				* avoid this extraneous call to iodone()
				* for a given bufp, we
				* make the check for B_DONE here.
				*/
				if (!(thiscmd->breq->b_flags & B_DONE))
					iodone(thiscmd->breq);
				xdputcbi(thiscmd);
			}
		}
		if (c->rdyiopbq != NULL) {
		/* need to run the ready iopb q and xdexec isn't quite right */
		}
		/* are there any ready and waiting iopbs? */
		while (!(c->c_io->xd_csr & XD_AIOP) && c->rdyiopbq) {
			/*
			 * pull an iopb off the queue and go
			 */
			riopb = c->rdyiopbq;
			c->rdyiopbq = riopb->xd_nxtaddr;
			riopb->xd_nxtaddr = NULL;
			xdpushiopb(riopb, c->c_io);
		}
		if (waitbufq != NULL) {
			(void) xdstart((caddr_t)NULL);
		}
		/* c->c_wantint = 0; */
	}
}

/*
 * This routine performs raw read operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
xdread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (UNIT(dev) >= nxd)
		return (ENXIO);
	return (physio(xdstrategy, (struct buf *)NULL,
		    dev, B_READ, minphys, uio));
}

/*
 * This routine performs raw write operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
xdwrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (UNIT(dev) >= nxd)
		return (ENXIO);
	return (physio(xdstrategy, (struct buf *)NULL,
		    dev, B_WRITE, minphys, uio));
}

/*
 * This routine finishes a buf-oriented operation.  It is called from
 * mbudone() after the mainbus space has been reclaimed.  It is always
 * called at disk interrupt priority.
 */
xddone(md)
	register struct mb_device *md;
{

	/*
	 * Release the buffer back to the world and remove it from the
	 * active slot.
	 */
	iodone(md->md_utab.b_forw);
	md->md_utab.b_forw = NULL;
}

/*
 * These defines are used by some of the ioctl calls.
 */
#define	ONTRACK(x)	(!((x) % un->un_g->dkg_nsect))
#define	XD_MAXBUFSIZE	(128 * 1024)

/*
 * This routine implements the ioctl calls for the 7053.  It is called
 * from the device switch at normal priority.
 */
/* ARGSUSED */
xdioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd, flag;
	caddr_t data;
{
	register struct xdunit *un = &xdunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	register struct xdcmdblock *xdcbi;
	struct dk_info *inf;
	struct dk_conf *conf;
	struct dk_type *typ;
	struct dk_badmap *bad;
	struct dk_cmd *com;
	struct dk_diag *diag;
	int i, s, flags = 0, err, hsect, mbcookie, bfr = 0, exec = 0;
	char	cmddir;
	struct 	buf *bp = un->un_rtab;
	faultcode_t fault_err;

	switch (cmd) {
#ifdef	DKIOCWCHK
		case DKIOCWCHK:
			if (!suser())
				return (u.u_error);
			if (!un || lp->dkl_nblk == 0)
				return (ENXIO);
			i = (*((int *) data));
			if (i) {
				s = splr(pritospl(un->un_mc->mc_intpri));
				(*((int *) data)) =
				    ((un->un_wchkmap & (1<<LPART(dev))) != 0);
				un->un_wchkmap |= (1 << LPART(dev));
				/*
				 * VM hack: turn on nopagereclaim, so that
				 * vm doesn't thwack on data while it is
				 * intransit going out (to avoid spurious
				 * verify errors).
				 */

				{
					extern int nopagereclaim;
					nopagereclaim = 1;
				}
				(void) splx(s);
				printf("xd%d%c: write check enabled\n",
					un->un_md->md_unit, 'a'+LPART(dev));
			} else  {
				s = splr(pritospl(un->un_mc->mc_intpri));
				(*((int *) data)) =
				    ((un->un_wchkmap & (1<<LPART(dev))) != 0);
				un->un_wchkmap &= ~(1<<LPART(dev));
				(void) splx(s);
				printf("xd%d%c: write check disabled\n",
					un->un_md->md_unit, 'a' + LPART(dev));
			}
			break;
#endif	DKIOCWCHK
		/*
		 * Return info concerning the controller.
		 */
		case DKIOCINFO:
			inf = (struct dk_info *)data;
			inf->dki_ctlr = getdevaddr(un->un_mc->mc_addr);
			inf->dki_unit = un->un_md->md_slave;
			inf->dki_ctype = DKC_XD7053;
			inf->dki_flags = DKI_BAD144 | DKI_FMTTRK;
			break;
		/*
		 * Return configuration info
		 */
		case DKIOCGCONF:
			conf = (struct dk_conf *)data;
			(void) strncpy(conf->dkc_cname, xdcdriver.mdr_cname,
			    DK_DEVLEN);
			conf->dkc_ctype = DKC_XD7053;
			conf->dkc_flags = DKI_BAD144 | DKI_FMTTRK;
			conf->dkc_cnum = un->un_mc->mc_ctlr;
			conf->dkc_addr = getdevaddr(un->un_mc->mc_addr);
			conf->dkc_space = un->un_mc->mc_space;
			conf->dkc_prio = un->un_mc->mc_intpri;
			if (un->un_mc->mc_intr)
				conf->dkc_vec = un->un_mc->mc_intr->v_vec;
			else
				conf->dkc_vec = 0;
			(void) strncpy(conf->dkc_dname, xdcdriver.mdr_dname,
			    DK_DEVLEN);
			conf->dkc_unit = un->un_md->md_unit;
			conf->dkc_slave = un->un_md->md_slave;
			break;
		/*
		 * Return drive info
		 */
		case DKIOCGTYPE:
			typ = (struct dk_type *)data;
			typ->dkt_drtype = 0;
			s = splr(pritospl(un->un_mc->mc_intpri));
			while ((xdcbi = xdgetcbi(un->c)) == NULL) {
				xdfreewait++;
				if (sleep((caddr_t)&xdfreewait,
				    (PZERO+1)|PCATCH)) {
					(void) splx(s);
					return (EINTR);
				}
			}
			err = xdcmd(xdcbi, XD_RPAR | XD_CTLR, minor(dev),
			    (caddr_t)0, un->un_md->md_slave, (daddr_t)0, 0,
			    XY_ASYNCHWAIT, 0);
			typ->dkt_promrev = xdcbi->iopb->xd_promrev;
			(void) xdputcbi(xdcbi);
			(void) splx(s);
			if (err)
				return (EIO);
			s = splr(pritospl(un->un_mc->mc_intpri));
			while ((xdcbi = xdgetcbi(un->c)) == NULL) {
				xdfreewait++;
				if (sleep((caddr_t)&xdfreewait,
				    (PZERO+1)|PCATCH)) {
					(void) splx(s);
					return (EINTR);
				}
			}
			err = xdcmd(xdcbi, XD_RPAR | XD_DRIVE, minor(dev),
			    (caddr_t)0, un->un_md->md_slave, (daddr_t)0, 0,
			    XY_ASYNCHWAIT, 0);
			typ->dkt_hsect = xdcbi->iopb->xd_hsect;
			typ->dkt_drstat = xdcbi->iopb->xd_dstat;
			(void) xdputcbi(xdcbi);
			(void) splx(s);
			if (err)
				return (EIO);
			break;
		/*
		 * Return the geometry of the specified unit.
		 */
		case DKIOCGGEOM:
			*(struct dk_geom *)data = *un->un_g;
			break;
		/*
		 * Set the geometry of the specified unit.  NOTE: we
		 * must raise the priority around usegeom() because we
		 * may execute an actual command.
		 */
		case DKIOCSGEOM:
			s = splr(pritospl(un->un_mc->mc_intpri));
			*un->un_g = *(struct dk_geom *)data;
			err = usegeom(un, XY_ASYNCHWAIT);
			(void) splx(s);
			if (err)
				return (EINVAL);
			break;
		/*
		 * Return the map for the specified logical partition.
		 * This has been made obsolete by the get all partitions
		 * command.
		 */
		case DKIOCGPART:
			*(struct dk_map *)data = *lp;
			break;
		/*
		 * Set the map for the specified logical partition.
		 * This has been made obsolete by the set all partitions
		 * command.  We raise the priority just to make sure
		 * an interrupt doesn't come in while the map is
		 * half updated.
		 */
		case DKIOCSPART:
			s = splr(pritospl(un->un_mc->mc_intpri));
			*lp = *(struct dk_map *)data;
			(void) splx(s);
			break;
		/*
		 * Return the map for all logical partitions.
		 */
		case DKIOCGAPART:
			for (i = 0; i < XYNLPART; i++)
				((struct dk_map *)data)[i] = un->un_map[i];
			break;
		/*
		 * Set the map for all logical partitions.  We raise
		 * the priority just to make sure an interrupt doesn't
		 * come in while the map is half updated.
		 */
		case DKIOCSAPART:
			s = splr(pritospl(un->un_mc->mc_intpri));
			for (i = 0; i < XYNLPART; i++)
				un->un_map[i] = ((struct dk_map *)data)[i];
			(void) splx(s);
			break;
		/*
		 * Get the bad sector map.
		 */
		case DKIOCGBAD:
			bad = (struct dk_badmap *)data;
			err = copyout((caddr_t)un->un_bad, bad->dkb_bufaddr,
			    (u_int)sizeof (struct dkbad));
			return (err);
		/*
		 * Set the bad sector map.  We raise the priority just
		 * to make sure an interrupt doesn't come in while the
		 * table is half updated.
		 */
		case DKIOCSBAD:
			bad = (struct dk_badmap *)data;
			s = splr(pritospl(un->un_mc->mc_intpri));
			err = copyin(bad->dkb_bufaddr, (caddr_t)un->un_bad,
			    (u_int)sizeof (struct dkbad));
			(void) splx(s);
			return (err);
		/*
		 * Generic command
		 */
		case DKIOCSCMD:
			com = (struct dk_cmd *)data;
			cmddir = XY_OUT;
			/*
			 * Check the parameters.
			 */
			switch (com->dkc_cmd) {
			    case XD_READ:
				cmddir = XY_IN;
				/* fall through */
			    case XD_WRITE:
				if (com->dkc_buflen != (com->dkc_secnt *
				    SECSIZE)) {
					err = EINVAL;
					goto errout;
				}
				break;
			    case XD_RESTORE:
				if (com->dkc_buflen != 0) {
					err = EINVAL;
					goto errout;
				}
				break;
			    /* read (extended) defect map */
			    case XD_REXT | XD_DEFECT:
			    case XD_REXT | XD_EXTDEF:
				cmddir = XY_IN;
			    /* write (extended) defect map */
			    case XD_WEXT | XD_DEFECT:
			    case XD_WEXT | XD_EXTDEF:
				if ((!ONTRACK(com->dkc_blkno)) ||
				    (com->dkc_buflen != XY_MANDEFSIZE)) {
					err = EINVAL;
					goto errout;
				}
				break;
			    /* format */
			    case XD_WEXT | XD_FORMVER:
				if ((com->dkc_buflen != 0) ||
				    (!ONTRACK(com->dkc_blkno))) {
					err = EINVAL;
					goto errout;
				}
				break;
			    /* read track headers */
			    case XD_REXT | XD_THEAD:
				cmddir = XY_IN;
				/* fall through */
			    /* write track headers */
			    case XD_WEXT | XD_THEAD:
				s = splr(pritospl(un->un_mc->mc_intpri));
				while ((xdcbi = xdgetcbi(un->c)) == NULL) {
					xdfreewait++;
					if (sleep((caddr_t)&xdfreewait,
					    (PZERO+1)|PCATCH)) {
						(void) splx(s);
						return (EINTR);
					}
				}
				err = xdcmd(xdcbi, XD_RPAR | XD_DRIVE,
				    minor(dev), (caddr_t)0, un->un_md->md_slave,
				    (daddr_t)0, 0, XY_ASYNCHWAIT, XY_NOMSG);
				hsect = xdcbi->iopb->xd_hsect;
				(void) xdputcbi(xdcbi);
				(void) splx(s);
				if (err) {
					err = EIO;
					goto errout;
				}
				if ((!ONTRACK(com->dkc_blkno)) ||
				    (com->dkc_buflen != hsect * XD_HDRSIZE)) {
					err = EINVAL;
					goto errout;
				}
				break;
			    default:
				err = EINVAL;
				goto errout;
			}
			/*
			 * Don't allow more than max at once.
			 */
			if (com->dkc_buflen > XD_MAXBUFSIZE)
				return (EINVAL);
			/*
			 * If this command moves data, we need to map
			 * the user buffer into DVMA space.
			 */
			if (com->dkc_buflen != 0) {
				/*
				 * Get the unit's raw I/O buf.
				 */
				s = spl6();
				while (bp->b_flags & B_BUSY) {
					bp->b_flags |= B_WANTED;
					(void) sleep((caddr_t)bp, PRIBIO+1);
				}
				(void) splx(s);
				/*
				 * Make the buf look real.
				 */
				bp->b_flags = B_BUSY | B_PHYS;
				bp->b_flags |= 
					(cmddir == XY_IN ? B_READ : B_WRITE);
				bp->b_un.b_addr = com->dkc_bufaddr;
				bp->b_bcount = com->dkc_buflen;
				bp->b_proc = u.u_procp;
				u.u_procp->p_flag |= SPHYSIO;
				/*
				 * Fault lock the address range of the buffer.
				 */
				fault_err = as_fault(u.u_procp->p_as,
				    bp->b_un.b_addr, (u_int)bp->b_bcount,
				    F_SOFTLOCK,
				    cmddir == XY_OUT ? S_READ : S_WRITE);
				if (fault_err != 0) {
					if (FC_CODE(fault_err) == FC_OBJERR)
						err = FC_ERRNO(fault_err);
					else
						err = EFAULT;
					goto out;
				}
				/*
				 * Make sure the address range has legal
				 * properties for the mb routines.
				 */
				if (buscheck(bp) < 0) {
					err = EFAULT;
					goto out;
				}
				/*
				 * Map the buffer into DVMA space.
				 */
				mbcookie = mbsetup(un->un_md->md_hd, bp, 0);
				bfr = MBI_ADDR(mbcookie);
			}
			/*
			 * Execute the command.
			 */
			if (com->dkc_flags & DK_SILENT)
				flags |= XY_NOMSG;
			if (com->dkc_flags & DK_DIAGNOSE)
				flags |= XY_DIAG;
			if (com->dkc_flags & DK_ISOLATE)
				flags |= XY_NOCHN;
			s = splr(pritospl(un->un_mc->mc_intpri));
			while ((xdcbi = xdgetcbi(un->c)) == NULL) {
				xdfreewait++;
				(void) sleep((caddr_t)&xdfreewait, PRIBIO);
			}
			err = xdcmd(xdcbi, com->dkc_cmd, minor(dev),
			    (caddr_t)bfr, un->un_md->md_slave, com->dkc_blkno,
			    com->dkc_secnt, XY_ASYNCHWAIT, flags);
			exec = 1;
			(void) xdputcbi(xdcbi);
			(void) splx(s);
			if (err)
				err = EIO;
			/*
			 * Release memory and DVMA space.
			 */
			if (com->dkc_buflen != 0) {
				mbrelse(un->un_md->md_hd, &mbcookie);
out:
				if (fault_err == 0)
					(void) as_fault(u.u_procp->p_as,
					    bp->b_un.b_addr,
					    (u_int)bp->b_bcount, F_SOFTUNLOCK,
					    cmddir == XY_OUT? S_READ : S_WRITE);
				(void) spl6();
				u.u_procp->p_flag &= ~SPHYSIO;
				if (bp->b_flags & B_WANTED)
					wakeup((caddr_t)bp);
				(void) splx(s);
				bp->b_flags &= ~(B_BUSY|B_WANTED|B_PHYS);
			}
errout:
			if (exec == 0 && err &&
			    (com->dkc_flags & DK_DIAGNOSE)) {
				un->un_errsect = 0;
				un->un_errno = XDE_UNKN;
				un->un_errcmd = com->dkc_cmd;
				un->un_errsevere = DK_FATAL;
			}
			return (err);
		/*
		 * Get diagnostics
		 */
		case DKIOCGDIAG:
			diag = (struct dk_diag *)data;
			diag->dkd_errsect = un->un_errsect;
			diag->dkd_errno = un->un_errno;
			diag->dkd_errcmd = un->un_errcmd;
			diag->dkd_severe = un->un_errsevere;
			break;
		/*
		 * This ain't no party, this ain't no disco.
		 */
		default:
			return (ENOTTY);
	}
	return (0);
}

/* This routine runs at regular intervals and checks to make sure a
 * controller isn't hung and a disk hasn't gone offline.  It is called
 * by itself and xdattach() via the timeout facility.  It is always
 * called at normal priority.
 *
 * This routine actually now only unconstipates the waitbufq. Later,
 * we will re-steal the watchdogging from xy.
 *
 * Take out call to xdstart(), as this should no longer be needed.
 */
xdwatch()
{
	register struct xdctlr *c;
#ifdef RECOVERY_MECHANISMS_FULLY_IMPLEMENTED
	register struct xdcmdblock *cmdblk;
#endif RECOVERY_MECHANISMS_FULLY_IMPLEMENTED
	int s;
	int i;
/*
	static char *lostint =
	"xdc%d: system lost interrupt from controller. SOFTWARE RECOVERED\n";
*/

	/*
	 * Advance time.
	 */
	xdticks++;
	/*
	 * Attempt to recover from lost interrupt.
	 * This recovery is for the case where the
	 * controller is still fine, but the host
	 * screwed up in some way.  I put this code
	 * in after tracing down a hang condition,
	 * in which the sunrise seemed to get a glitch
	 * on the VME bus, and get the wrong interrupt
	 * vector, thus causing xdintr never to be called.
	 */
	for (c = xdctlrs; c < &xdctlrs[nxdc]; c++) {
		if (!(c->c_flags & XY_C_PRESENT))
			continue;
		s = splr(pritospl(c->c_intpri));
		if (c->c_io->xd_csr & XD_RIO) {
			if (xd_watch_info[c-xdctlrs].got_rio < MAX_RIO_TICKS) {
			    (xd_watch_info[c - xdctlrs].got_rio)++;
			} else {
			    xd_watch_info[c - xdctlrs].got_rio = 0;
	/*
	 * it was requested that this message be taken out
	 * because it scares customers, and this hardware
	 * failure has never been observed to be fatal.
	 */
			    /* printf(lostint, (c - xdctlrs)); */
			    xdintr(c);
			}
		}
		(void)splx(s);
	}

	/*
	 * The following mechanism detects whether the
	 * controller has failed.  This is tricky, since we
	 * cannot be sure how the 7053 orders requests.
	 * Rather than worry about the time required for each
	 * outstanding command on a controller, the approach here
	 * is to timeout if we don't get at least one interrupt
	 * from the controller within the current time interval
	 * The size of this time interval is based upon the
	 * estimated maximum time to execute an outstanding
	 * command.
	 */
	for (i = 0; i < nxdc; i++) {
		c = &xdctlrs[i];
		if (!(c->c_flags & XY_C_PRESENT))
			continue;
		s = splr(pritospl(c->c_intpri));
		if (xd_watch_info[i].curr_timeout == 0) {
			if ((xd_watch_info[i].next_timeout != 0) &&
			    (c->actvcmdq != NULL)) {
				xd_watch_info[i].curr_timeout =
				    xd_watch_info[i].next_timeout;
				xd_watch_info[i].next_timeout = 0;
				xd_watch_info[i].got_interrupt = 0;
			}
		} else if ((--xd_watch_info[i].curr_timeout == 0) &&
		    (xd_watch_info[i].got_interrupt == 0)) {

			printf("xdc%d: controller not responding\n",
			    (c - xdctlrs));

	/*
	 * Now we need to recover from this problem just detected.
	 * This recovery involves resetting the controller, and
	 * retrying each outstanding commands on the controller.
	 * In order to do this, we need to complete development of
	 * other parts of this driver.  First, we need to break xdintr
	 * into two parts, the first which finds the cmdblk given
	 * the info supplied by the controller registers, and the
	 * second part, which performs the recovery for a
	 * particular command block.  Then, the recovery here will
	 * involve calling the second part for each outstanding
	 * cmdblk on c->actvcmdq.  There are other things to be
	 * completed in xdrecover and in other parts of this
	 * driver to fill all the holes that remain in driver
	 * functionality in regards particularly to recovery.
	 * For now, though, I was ordered not to address these
	 * problems.  The following ifdefed out code outlines
	 * the recovery here to give the person who addresses
	 * these problems a start.
	 */

#ifdef RECOVERY_MECHANISMS_FULLY_IMPLEMENTED

			/*
			 * First, try issueing a NO OP command
			 * to the controller, to see if poking it
			 * will get it going.
			 */

			/*
			 * Then, reset the controller, to save
			 * us from race conditions in the
			 * recovery.
			 */
			c->c_io->xd_csr = XD_RST;
			CDELAY(((c->c_io->xd_csr & XD_RACT) == 0), 100000);
			if (c->c_io->xd_csr & XD_RACT) {
				printf("xdc%d: ", c - xdctlrs);
				panic("controller reset failed");
			}

			/*
			 * Next, go through each outstanding
			 * cmdblk and invoke the recovery routine
			 * which will be the second part of
			 * xdintr.
			 */

			for (cmdblk = c->actvcmdq; cmdblk != NULL;
			    cmdblk = cmdblk->next) {
				if (cmdblk->iopb->xd_complete)
					continue;
				cmdblk->iopb->xd_iserr = 1;
				cmdblk->iopb->xd_errno = XDE_LINT;
				cmdblk->iopb->xd_complete = 1;

				/*
				 * start the recovery process by
				 * calling the second part of
				 * xdintr.
				 */
			}

#endif	RECOVERY_MECHANISMS_FULLY_IMPLEMENTED



		}
		(void) splx(s);
	}


	/*
	 * Schedule ourself to run again in a little while.
	 */
	timeout(xdwatch, (caddr_t)0, hz);
#ifdef	lint
	/* stupid lint */
	xdintr((struct xdctlr *)0);
#endif
}

/*
 * This routine dumps memory to the disk.  It assumes that the memory has
 * already been mapped into mainbus space.  It is called at disk interrupt
 * priority when the system is in trouble.
 */
xddump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	register struct xdunit *un = &xdunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	register struct xdcmdblock *xdcbi;
	int unit = un->un_md->md_slave;
	int err;

	/*
	 * Check to make sure the operation makes sense.
	 */
	if (!(un->un_flags & XY_UN_PRESENT))
		return (ENXIO);
	if (blkno >= lp->dkl_nblk || (blkno + nblk) > lp->dkl_nblk)
		return (EINVAL);
	/*
	 * Offset into the correct partition.
	 */
	blkno += (lp->dkl_cylno + un->un_g->dkg_bcyl) * un->un_g->dkg_nhead *
	    un->un_g->dkg_nsect;
	/*
	 * Synchronously execute the dump and return the status.
	 */
	xdcbi = xddumpcbi;
	xdcbi->c = un->c;
	err = xdcmd(xdcbi, XD_WRITE, minor(dev),
	    (caddr_t)((char *)addr - DVMA), unit, blkno, (int)nblk,
	    XY_SYNCH, 0);
	return (err ? EIO : 0);
}
