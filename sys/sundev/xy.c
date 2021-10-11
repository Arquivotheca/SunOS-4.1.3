#ifndef lint
static	char sccsid[] = "@(#)xy.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Driver for Xylogics 450/451 SMD disk controllers
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

#include <vm/hat.h>
#include <vm/seg.h>
#include <vm/as.h>

#include <machine/psl.h>

#include <sun/dkio.h>
#include <sundev/mbvar.h>
#include <sundev/xyvar.h>
#include <sundev/xyerr.h>

extern char *strncpy();
extern int dkn;
#ifdef	DKIOCWCHK
extern int dvmasize, maxphys;
#endif

static initlabel(), uselabel(), usegeom(), islabel(), ck_cksum();
static initiopb(), cleariopb(), doattach(), findslave();
static struct xyerror *finderr();
static printerr();
#ifdef	DKIOCWCHK
static void xywchk_start();
static int xywchk_done();
#endif	DKIOCWCHK

int xyprobe(), xyslave(), xyattach(), xygo(), xydone(), xypoll();

/*
 * Config dependent structures -- declared in xy_conf.c
 */
extern int nxy, nxyc;
extern struct	mb_ctlr *xycinfo[];
extern struct	mb_device *xydinfo[];
extern struct xyunit xyunits[];
extern struct xyctlr xyctlrs[];

struct mb_driver xycdriver = {
	xyprobe, xyslave, xyattach, xygo, xydone, xypoll,
	sizeof (struct xydevice), "xy", xydinfo, "xyc", xycinfo, MDR_BIODMA,
};

/*
 * Settable error level.
 */
short xyerrlvl = EL_FIXED | EL_RETRY | EL_RESTR | EL_RESET | EL_FAIL;

/*
 * Flags value that turns off command chaining
 */
#define	XY_NOCHAINING		0x1

/*
 * Macro to provide a quick test for wait avoidance.
 */
#define	XYCSRVALID(xyio) { \
	if ((xyio)->xy_csr & (XY_BUSY|XY_DBLERR)) \
		(void) xycsrvalid(xyio); \
}

/*
 * List of commands for the 450.  Used to print nice error messages.
 */
char *xycmdnames[] = {
	"nop",
	"write",
	"read",
	"write headers",
	"read headers",
	"seek",
	"drive reset",
	"format",
	"read all",
	"read status",
	"write all",
	"set drive size",
	"self test",
	"reserved",
	"buffer load",
	"buffer dump",
};

int xywstart, xywatch();	/* have started guardian */
int xyticks = 1;		/* timer for guardian */

int xythrottle = XY_THROTTLE;	/* transfer burst count */

/*
 * Determine existence of a controller.  Called at boot time only.
 * NOTE: we delay 15-20 microseconds after every register write
 * to the controller.  This is because we are paranoid about the 450
 * extending the bus cycle correctly and instead wait till it should
 * be ready for the next cycle.
 */
xyprobe(reg, ctlr)
	caddr_t reg;
	int ctlr;
{
	register struct xyctlr *c = &xyctlrs[ctlr];
	register struct xydevice *xyio;
	u_char err;

	/*
	 * See if there's hardware present by trying to reset it
	 */
	xyio = c->c_io = (struct xydevice *)reg;
	if (peekc((char *)&xyio->xy_resupd))
		return (0);
	if (!xywait(xyio)) {
		printf("xyc%d: controller reset failed\n", ctlr);
		return (0);
	}
	/*
	 * Make sure its not a 2180 controller by using unique regs
	 */
	xyio->xy_iopbrel[0] = 0x67;
	xyio->xy_iopbrel[1] = 0x89;
	DELAY(15);
	if (xyio->xy_iopbrel[0] != 0x67 || xyio->xy_iopbrel[1] != 0x89)
		return (0);
	/*
	 * Set the iopb relocation regs to 0.  This assumes that the
	 * entire IOPBMAP resides in the bottom 64K of DVMA space.
	 */
	xyio->xy_iopbrel[0] = 0;
	DELAY(15);
	xyio->xy_iopbrel[1] = 0;
	DELAY(15);
	if ((xyio->xy_iopbrel[0] != 0) || (xyio->xy_iopbrel[1] != 0))
		return (0);
	/*
	 * Allocate an iopb for controller use.
	 */
	c->c_cmd.iopb = (struct xyiopb *)rmalloc(iopbmap,
		(long)sizeof (struct xyiopb));
	if (c->c_cmd.iopb == NULL) {
		printf("xyc%d: no iopb space\n", ctlr);
		return (0);
	}
	initiopb(c->c_cmd.iopb);
	c->c_cmd.c = c;
	/*
	 * Use a nop command to get back the controller's type field.
	 * It must be a 450 or we bail out.
	 */
	err = xycmd(&c->c_cmd, XY_NOP, NOLPART, (caddr_t)0, 0, (daddr_t)0, 0,
	    XY_SYNCH, 0);
	if (err || (c->c_cmd.iopb->xy_ctype != XYC_450)) {
		printf("xyc%d: unsupported controller type\n", ctlr);
		return (0);
	}
	/*
	 * Run the controller self test
	 */
	if (err = xycmd(&c->c_cmd, XY_TEST, NOLPART, (caddr_t)0, 0, (daddr_t)0,
	    0, XY_SYNCH, 0)) {
		printf("xyc%d: self test error\n", ctlr);
		return (0);
	}
	if (xyio->xy_csr & XY_ADDR24)
		c->c_flags |= XY_C_24BIT;
	c->c_flags |= XY_C_PRESENT;
	return (sizeof (struct xydevice));
}

/*
 * See if a slave unit exists.  This routine is called only at boot time.
 * It is a wrapper for findslave() to match parameter types.
 */
xyslave(md, reg)
	register struct mb_device *md;
	caddr_t reg;
{
	register struct xyctlr *c;
	int i;

	/*
	 * Find the controller at the given io address
	 */
	for (i = 0; i < nxyc; i++)
		if (xyctlrs[i].c_io == (struct xydevice *)reg)
			break;
	if (i >= nxyc) {
		printf("xy%d: ", md->md_unit);
		panic("io address mismatch");
	}
	c = &xyctlrs[i];
	/*
	 * Use findslave() to look for the drive synchronously
	 */
	return (findslave(md, c, XY_SYNCH));
}

/*
 * Actual search for a slave unit.  Used by xyslave() and xyopen().  This
 * routine is always called at disk interrupt priority.
 */
static
findslave(md, c, mode)
	register struct mb_device *md;
	register struct xyctlr *c;
	int mode;
{
	register struct mb_ctlr *mc;
	int i, low, high;

	/*
	 * Set up the vectored interrupt to pass the ctlr pointer.
	 * Also check to make sure the address size jumper is correct.
	 * NOTE: we assume here that vectored interrupts imply
	 * a VMEbus and autovectors imply a Multibus.  This is
	 * currently guaranteed by config.  This needs to be done here so
	 * the drive reset can interrupt correctly when it completes in
	 * the asynchwait case.
	 */
	mc = xycinfo[c - xyctlrs];
	if (mc->mc_intr) {
		if (!(c->c_flags & XY_C_24BIT))
			printf("xyc%d: WARNING: 20 bit addresses\n",
			    mc->mc_ctlr);
		*(mc->mc_intr->v_vptr) = (int)c;
	} else if (c->c_flags & XY_C_24BIT)
		printf("xyc%d: WARNING: 24 bit addresses\n", mc->mc_ctlr);
	/*
	 * Set up loop to search for a wildcarded drive number
	 */
	if (md->md_slave == '?') {
		low = 0;
		high = XYUNPERC-1;
	} else
		low = high = md->md_slave;
	/*
	 * Look for a drive that's online but not already taken
	 */
	for (i = low; i <= high; i++) {
		if (i >= XYUNPERC)
			break;
		if (c->c_units[i] != NULL &&
		    (c->c_units[i]->un_flags & XY_UN_PRESENT))
			continue;
		/*
		 * If a drive reset succeeds, we assume that the drive
		 * is there.
		 */
		if (!xycmd(&c->c_cmd, XY_RESTORE, NOLPART, (caddr_t)0, i,
		    (daddr_t)0, 0, mode, XY_NOMSG)) {
			md->md_slave = i;
			return (1);
		}
	}
	return (0);
}

/*
 * This routine is used to attach a drive to the system.  It is called only
 * at boot time.  It is a wrapper for doattach() to get the parameter
 * types matched.
 */
xyattach(md)
	register struct mb_device *md;
{

	doattach(md, XY_SYNCH);
}

/*
 * This routine does the actual work involved in attaching a drive to the
 * system.  It is called from xyattach() and xyopen().  It is always called
 * at disk interrupt priority.
 */
static
doattach(md, mode)
	register struct mb_device *md;
	int mode;
{
	register struct xyctlr *c = &xyctlrs[md->md_ctlr];
	register struct xyunit *un = &xyunits[md->md_unit];
	register struct dk_label *l;
	int s, i, found = 0;

	/*
	 * If this disk has never been seen before, we need to set
	 * up all the structures for the device.
	 */
	if (!(un->un_flags & XY_UN_ATTACHED)) {
		/*
		 * Allocate an iopb for the unit.  NOTE: because other
		 * devices may be doing similar things in the asynchronous
		 * case, we lock out all mainbus activity while we are
		 * looking at the resource map.
		 */
		s = splr(pritospl(SPLMB));
		un->un_cmd.iopb = (struct xyiopb *)rmalloc(iopbmap,
		    (long)sizeof (struct xyiopb));
		(void) splx(s);
		if (un->un_cmd.iopb == NULL) {
			printf("xy%d: no space for unit iopb\n", md->md_unit);
			return;
		}
		un->un_flags |= XY_UN_ATTACHED;
		/*
		 * If no one has started the watchdog routine, do so.
		 */
		if (xywstart++ == 0)
			timeout(xywatch, (caddr_t)0, hz);
		/*
		 * Set up all the pointers and allocate space for the
		 * dynamic parts of the unit structure.  NOTE: if the
		 * alloc fails the system panics, so we don't need to
		 * test the return value.
		 */
		c->c_units[md->md_slave] = un;
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
		un->un_bad = (struct dkbad *)new_kmem_alloc(
			sizeof (struct dkbad), KMEM_SLEEP);
		initiopb(un->un_cmd.iopb);
		un->un_cmd.c = c;
		if (md->md_flags & XY_NOCHAINING)
			c->c_flags |= XY_C_NOCHN;
	}
	/*
	 * Allocate a temporary buffer in DVMA space for reading the label.
	 * Again we must lock out all main bus activity to protect the
	 * resource map.
	 */
	s = splr(pritospl(SPLMB));
	l = (struct dk_label *)rmalloc(iopbmap, (long)SECSIZE);
	(void) splx(s);
	if (l == NULL) {
		printf("xy%d: no space for disk label\n", md->md_unit);
		return;
	}
	/*
	 * Unit is now officially present.  It can now be accessed by the system
	 * even if the rest of this routine fails.
	 */
	un->un_flags |= XY_UN_PRESENT;
	/*
	 * Initialize the label structures.  This is necessary so weird
	 * entries in the bad sector map don't bite us while reading the
	 * label.
	 */
	initlabel(un);
	/*
	 * Search for a label using each possible drive type, since we
	 * don't know which type the disk is yet.
	 */
	for (un->un_drtype = 0; un->un_drtype < NXYDRIVE; un->un_drtype++) {
		l->dkl_magic = 0;	/* reset from last try */
		/*
		 * Attempt to read the label.  We use silent flag so
		 * no one will know if we fail.
		 */
		if (xycmd(&un->un_cmd, XY_READ, NOLPART,
		    (caddr_t)((char *)l - DVMA), md->md_slave, (daddr_t)0, 1,
		    mode, XY_NOMSG))
			continue;
		if (!islabel(md->md_unit, l))
			continue;
		uselabel(un, md->md_unit, l);
		if (usegeom(un, mode))
			continue;
		/*
		 * If we get here the label was digested correctly.  Mark
		 * label found and quit searching.
		 */
		found = 1;
		break;
	}
	/*
	 * If we found the label, attempt to read the bad sector map.
	 */
	if (found) {
		if (xycmd(&un->un_cmd, XY_READ, NOLPART,
		    (caddr_t)((char *)l - DVMA), md->md_slave,
		    (daddr_t)((((un->un_g->dkg_ncyl + un->un_g->dkg_acyl) *
		    un->un_g->dkg_nhead) - 1) * un->un_g->dkg_nsect), 1,
		    mode, 0)) {
			/*
			 * If we failed, print a message and invalidate
			 * the map in case it got destroyed in the read.
			 */
			printf("xy%d: unable to read bad sector info\n",
			    un - xyunits);
			for (i = 0; i < 126; i++)
				un->un_bad->bt_bad[i].bt_cyl = 0xFFFF;
		/*
		 * Use a structure assignment to fill in the real map.
		 */
		} else
			*un->un_bad = *(struct dkbad *)l;
	/*
	 * If we couldn't read the label, print a message and invalidate
	 * the label structures in case they got destroyed in the reads.
	 */
	} else {
		printf("xy%d: unable to read label\n", un - xyunits);
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
 * used by xyprobe() and doattach().  It is always called at disk
 * interrupt priority.
 */
static
initiopb(xy)
	register struct xyiopb *xy;
{

	bzero((caddr_t)xy, sizeof (struct xyiopb));
	xy->xy_recal =1;
	xy->xy_enabext = 1;
	xy->xy_eccmode = 2;
	xy->xy_autoup = 1;
	xy->xy_reloc = 1;
	xy->xy_throttle = xythrottle;
}

/*
 * This routine clears the fields of the iopb that must be zero before a
 * command is executed.  It is used by xycmd() and xyrecover(). It is always
 * called at disk interrupt priority.
 */
static
cleariopb(xy)
	register struct xyiopb *xy;
{

	xy->xy_errno = 0;
	xy->xy_iserr = 0;
	xy->xy_complete = 0;
	xy->xy_ctype = 0;
}

/*
 * This routine initializes the unit label structures.  The logical partitions
 * are set to zero so normal opens will fail.  The geometry is set to
 * nonzero small numbers as a paranoid defense against zero divides.
 * Bad sector map is filled with non-entries.
 */
static
initlabel(un)
	register struct xyunit *un;
{
	register int i;

	bzero((caddr_t)un->un_map, sizeof (struct dk_map) * XYNLPART);
	bzero((caddr_t)un->un_g, sizeof (struct dk_geom));
	un->un_g->dkg_ncyl = un->un_g->dkg_nhead = 2;
	un->un_g->dkg_nsect = 2;
	for (i = 0; i < 126; i++)
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
		printf("xy%d: corrupt label\n", unit);
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
	register struct xyunit *un;
	int unit;
	register struct dk_label *l;
{
	int i, intrlv;

	/*
	 * Print out the disk description.
	 */
	printf("xy%d: <%s>\n", unit, l->dkl_asciilabel);
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
	 * Old labels don't have pcyl in them, so we make a guess at it.
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
 * This routine is used to initialize the drive type.  The 450 requires
 * that each drive type be set up once by sending a set drive size
 * command to the controller.  After that, any number of disks of that
 * type can be used.  It is used by doattach() and xyioctl().  It is
 * always called at disk interrupt priority.
 */
static
usegeom(un, mode)
	register struct xyunit *un;
	int mode;
{
	register struct xyctlr *c = &xyctlrs[un->un_mc->mc_ctlr];
	int unit;
	daddr_t lastb;
	register struct dk_geom *ng, *g = un->un_g;
	register struct xyunit *nun;

	/*
	 * Search for other disks of the same type on this
	 * controller.  If found, they must have the same
	 * geometry or we are stuck.
	 */
	for (unit = 0; unit < XYUNPERC; unit++) {
		if ((nun = c->c_units[unit]) == NULL || un == nun)
			continue;
		if (!(nun->un_flags & XY_UN_PRESENT))
			continue;
		if (nun->un_drtype != un->un_drtype)
			continue;
		ng = nun->un_g;
		if ((g->dkg_ncyl + g->dkg_acyl != ng->dkg_ncyl +
		    ng->dkg_acyl) || g->dkg_nhead != ng->dkg_nhead ||
		    g->dkg_nsect != ng->dkg_nsect) {
			printf("xy%d and xy%d are of same type (%d)",
			    un - xyunits, nun - xyunits, un->un_drtype);
			printf(" with different geometries\n");
			return (1);
		}
		return (0);
	}
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
	 * This type is not initialized so we must execute a set drive
	 * size command.  If it fails, we return the error so the label
	 * gets invalidated.
	 */
	if (xycmd(&un->un_cmd, XY_INIT, NOLPART, (caddr_t)0,
	    un->un_md->md_slave, lastb, 0, mode, 0)) {
		printf("xy%d: initialization failed\n", un - xyunits);
		return (1);
	}
	return (0);
}

/*
 * This routine sets the fields in the iopb that are needed for an
 * asynchronous operation.  It does not start the operation, since the
 * iopb may be chained to others.  It is used by xycmd() and xyintr().
 * It is always called at disk interrupt priority.
 */
xyasynch(cmdblk)
	register struct xycmdblock *cmdblk;
{
	register struct xyiopb *xy = cmdblk->iopb;

	xy->xy_ie = 1;
	xy->xy_intrall = 0;
}

/*
 * This routine sets up and executes iopbs for a controller.  It always
 * runs the controller's private commands first.  After that, it does
 * a round-robin search of the units, looking for a ready iopb.  If chaining
 * is enabled, it will also add to that any other ready iopbs that can
 * be chained.  The chaining is done to both the iopbs themselves
 * (so the controller sees it) and also to the link fields of the cmdblocks
 * (so the interrupt routine sees it).  It is used by xycmd() and xyintr().
 * It is always called at disk interrupt priority.
 */
xychain(c)
	register struct xyctlr *c;
{
	register struct xyunit *un;
	register struct xyiopb *lptr = NULL;
	register struct xycmdblock *bptr = NULL;
	register int i, j;

	/*
	 * See if the controller's iopb is ready.  If so, execute it
	 * in isolation.  Since the only commands that use this are
	 * those searching for a disk, starvation of the unit iopbs
	 * is not an issue.
	 */
	if ((c->c_cmd.flags & XY_FBSY) &&
	    (c->c_cmd.flags & XY_FRDY) &&
	    (!(c->c_cmd.flags & XY_DONE))) {
		lptr = c->c_cmd.iopb;
		bptr = &c->c_cmd;
		lptr->xy_chain = 0;
		bptr->next = NULL;
		goto run;
	}
	/*
	 * Round-robin search of the unit iopbs for a ready one.  If
	 * we find one, and chaining is off for the controller or that
	 * command, we execute it in isolation.  Otherwise, search the
	 * rest of the unit iopbs for ones that are ready and chainable
	 * and add them in.
	 */
	for (i = 0; i < XYUNPERC; i++)
		if (((un = c->c_units[(c->c_nextunit + i) % XYUNPERC])
		    != NULL) &&
		    (un->un_cmd.flags & XY_FBSY) &&
		    (un->un_cmd.flags & XY_FRDY) &&
		    (!(un->un_cmd.flags & XY_DONE)))
			break;
	/*
	 * If we didn't find a ready one, just return.
	 */
	if (i >= XYUNPERC)
		return;
	/*
	 * Set the round-robin ptr to the unit after the ready one
	 * and set up the iopb to be run.
	 */
	c->c_nextunit = (c->c_nextunit + i + 1) % XYUNPERC;
	lptr = un->un_cmd.iopb;
	bptr = &un->un_cmd;
	lptr->xy_chain = 0;
	bptr->next = NULL;
	/*
	 * If chaining is off for any reason, just run this one.
	 */
	if ((c->c_flags & XY_C_NOCHN) || (bptr->flags & XY_NOCHN))
		goto run;
	/*
	 * Pick up any other ready iopbs and add them to the chain.
	 */
	for (j = 0; j < (XYUNPERC - i - 1); j++) {
		if (((un = c->c_units[(c->c_nextunit + j) % XYUNPERC])
		    != NULL) &&
		    (un->un_cmd.flags & XY_FBSY) &&
		    (un->un_cmd.flags & XY_FRDY) &&
		    (!(un->un_cmd.flags & XY_DONE)) &&
		    (!(un->un_cmd.flags & XY_NOCHN))) {
			un->un_cmd.iopb->xy_nxtoff = XYOFF(((char *)lptr)
			    - DVMA);
			lptr = un->un_cmd.iopb;
			lptr->xy_chain = 1;
			un->un_cmd.next = bptr;
			bptr = &un->un_cmd;
		}
	}
run:
	if (lptr != NULL) {
		c->c_chain = bptr;
		c->c_wantint = xyticks;
		xyexec(lptr, c->c_io, c - xyctlrs);
	}
}

/*
 * This routine executes a command synchronously.  This is only done at
 * boot time or when a dump is being done.  It is used by xycmd().
 * It is always called at disk interrupt priority.
 */
xysynch(cmdblk)
	register struct xycmdblock *cmdblk;
{
	register struct xyiopb *xy = cmdblk->iopb;
	register struct xydevice *xyio = cmdblk->c->c_io;
	register char junk;

	/*
	 * Clean out any old state left in the controller.
	 */
	if (xyio->xy_csr & XY_BUSY) {
		junk = xyio->xy_resupd;
		if (!xywait(xyio)) {
			printf("xyc%d: ", cmdblk->c - xyctlrs);
			panic("controller reset failed");
		}
	}
	if (xyio->xy_csr & XY_INTR) {
		xyio->xy_csr = XY_INTR;
		(void) xycsrvalid(xyio);
	}
	if (xyio->xy_csr & (XY_ERROR | XY_DBLERR)) {
		xyio->xy_csr = XY_ERROR;
		(void) xycsrvalid(xyio);
	}
#ifdef lint
	junk = junk;
#endif
	/*
	 * Set necessary iopb fields then have the command executed.
	 */
	xy->xy_ie = 0;
	xy->xy_chain = 0;
	xyexec(xy, xyio, cmdblk->c - xyctlrs);
	/*
	 * Wait for the command to complete or until a timeout occurs.
	 */
	CDELAY(((xyio->xy_csr & XY_BUSY) == 0), 1000000 * XYLOSTINTTIMO);
	/*
	 * If we timed out, use the lost interrupt error to pass
	 * back status or just reset the controller if the command
	 * had already finished.
	 */
	if (xyio->xy_csr & XY_BUSY) {
		if (xy->xy_complete) {
			junk = xyio->xy_resupd;
			if (!xywait(xyio)) {
				printf("xyc%d: ", cmdblk->c - xyctlrs);
				panic("controller reset failed");
			}
		} else {
			xy->xy_iserr = 1;
			xy->xy_errno = XYE_LINT;
		}
	/*
	 * If one of the error bits is set in the controller status
	 * register, we need to convey this to the recovery routine
	 * via the iopb.  However, if the iopb has a specific error
	 * reported, we don't want to overwrite it.  Therefore, we
	 * only fold the error bits into the iopb if the iopb thinks
	 * things were ok.
	 */
	} else if (xyio->xy_csr & XY_DBLERR) {
		xyio->xy_csr = XY_ERROR;	/* clears the error */
		(void) xycsrvalid(xyio);
		if (!(xy->xy_iserr && xy->xy_errno)) {
			xy->xy_iserr = 1;
			xy->xy_errno = XYE_DERR;
		}
	} else if (xyio->xy_csr & XY_ERROR) {
		DELAY(10);
		xyio->xy_csr = XY_ERROR;	/* clears the error */
		(void) xycsrvalid(xyio);
		if (!(xy->xy_iserr && xy->xy_errno)) {
			xy->xy_iserr = 1;
			xy->xy_errno = XYE_ERR;
		}
	}
}

/*
 * This routine is the focal point of all commands to the controller.
 * Every command passes through here, independent of its source or
 * reason.  The mode determines whether we are synchronous, asynchronous,
 * or asynchronous but waiting for completion.  The flags are used
 * to suppress error recovery and messages when we are doing special operations.
 * It is used by xyprobe(), findslave(), doattach(), usegeom(),
 * xygo(), xyioctl(), xywatch(), and xydump().
 * It is always called at disk interrupt priority.
 * NOTE: this routine assumes that all operations done before the disk's
 * geometry is defined are done on block 0.  This impacts both this routine
 * and the error recovery scheme (even the restores must use block 0).
 * Failure to abide by this restriction could result in an arithmetic trap.
 */
xycmd(cmdblk, cmd, device, bufaddr, unit, blkno, secnt, mode, flags)
	register struct xycmdblock *cmdblk;
	u_short cmd;
	int device, unit, secnt, mode, flags;
	caddr_t bufaddr;
	daddr_t blkno;
{
	register struct xyiopb *xy = cmdblk->iopb;
	register struct xyunit *un = cmdblk->c->c_units[unit];
	int stat = 0;

	/*
	 * If we are willing to wait and the cmdblock is busy, we
	 * sleep till it's free.  In the normal asynchronous case,
	 * the cmdblock was set busy up in xyustart().  In the synchronous
	 * case, the cmdblock can't be busy.
	 */
	if (mode == XY_ASYNCHWAIT) {
		while (cmdblk->flags & XY_FBSY) {
			cmdblk->flags |= XY_WANTED;
			(void) sleep((caddr_t)cmdblk, PRIBIO);
		}
	}
	/*
	 * Mark the cmdblock busy and fill in the fields.
	 */
	cmdblk->flags = XY_FBSY | XY_FRDY | flags;
	cmdblk->retries = cmdblk->restores = cmdblk->resets = 0;
	cmdblk->slave = unit;
	cmdblk->cmd = cmd;
	cmdblk->device = device;
	cmdblk->blkno = blkno;
	cmdblk->nsect = secnt;
	cmdblk->baddr = bufaddr;
	cmdblk->failed = 0;
#ifdef	DKIOCWCHK
	/*
	 * If this command is a write command, and we're supposed to be
	 * doing write checks, *AND* the transfer is to/from onboard
	 * memory (i.e., not across the VME bus), then set things up
	 * so that we do write checks later.
	 */

	if (cmd == XY_WRITE && device != NOLPART && mode == XY_ASYNCH && un &&
	    (un->un_wchkmap & (1<<LPART(device))) &&
	    MBI_MR(un->un_md->md_mbinfo) < dvmasize) {
		un->un_nverifies = 0;
		un->un_savaddr = bufaddr;
		un->un_savbn = blkno;
		un->un_savnsect = secnt;
		xywchk_start(un);
		cmdblk->flags |= XY_INWCHK;
	}
#endif	DKIOCWCHK
	/*
	 * Initialize the diagnostic info if necessary.
	 */
	if ((cmdblk->flags & XY_DIAG) && (un != NULL))
		un->un_errsevere = DK_NOERROR;
	/*
	 * Clear out the iopb fields that need it.
	 */
	cleariopb(xy);
	/*
	 * Set the iopb fields that are the same for all commands.
	 */
	xy->xy_cmd = cmd >> 8;
	xy->xy_subfunc = cmd;
	if (un != NULL)
		xy->xy_drive = un->un_drtype;
	else
		xy->xy_drive = 0;
	xy->xy_unit = unit;
	/*
	 * If the blockno is 0, we don't bother calculating the disk
	 * address.  NOTE: this is a necessary test, since some of the
	 * operations on block 0 are done while un is not yet defined.
	 * Removing the test would case bad pointer references.
	 */
	if (blkno != 0) {
		xy->xy_cylinder = blkno / (un->un_g->dkg_nhead *
		    un->un_g->dkg_nsect);
		xy->xy_head = (blkno / un->un_g->dkg_nsect) %
		    un->un_g->dkg_nhead;
		xy->xy_sector = blkno % un->un_g->dkg_nsect;
	} else
		xy->xy_cylinder = xy->xy_head = xy->xy_sector = 0;
	xy->xy_nsect = secnt;
	xy->xy_bufoff = XYOFF(bufaddr);
	xy->xy_bufrel = XYNEWREL(cmdblk->c->c_flags, bufaddr);
	/*
	 * If command is synchronous, execute it.  We continue to call
	 * error recovery (which will continue to execute commands) until
	 * it returns either success or complete failure.
	 */
	if (mode == XY_SYNCH) {
		xysynch(cmdblk);
		while ((stat = xyrecover(cmdblk, xysynch)) > 0)
			;
		cmdblk->flags &= ~XY_FBSY;
		return (stat);
	}
	/*
	 * If command is asynchronous, set up it's execution.  We only
	 * start the execution if the controller isn't busy.
	 */
	xyasynch(cmdblk);
	if (!cmdblk->c->c_wantint)
		xychain(cmdblk->c);
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
		cmdblk->flags &= ~XY_FBSY;
		if (cmdblk->flags & XY_WANTED) {
			cmdblk->flags &= ~XY_WANTED;
			wakeup((caddr_t)cmdblk);
		} else if (un != NULL && cmdblk == &un->un_cmd)
			xyustart(un);
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
 * This routine provides the error recovery for all commands to the 450.
 * It examines the results of a just-executed command, and performs the
 * appropriate action.  It will set up at most one followup command, so
 * it needs to be called repeatedly until the error is resolved.  It
 * returns three possible values to the calling routine : 0 implies that
 * the command succeeded, 1 implies that recovery was initiated but not
 * yet finished, and -1 implies that the command failed.  By passing the
 * address of the execution function in as a parameter, the routine is
 * completely isolated from the differences between synchronous and
 * asynchronous commands.  It is used by xycmd() and xyintr().  It is
 * always called at disk interrupt priority.
 */
xyrecover(cmdblk, execptr)
	register struct xycmdblock *cmdblk;
	register (*execptr)();
{
	struct xyctlr *c = cmdblk->c;
	register struct xyiopb *xy = cmdblk->iopb;
	register struct xyunit *un = c->c_units[cmdblk->slave];
	struct xyerract *actptr;
	struct xyerror *errptr;
	int bn, ns, ndone;
#ifdef	DKIOCWCHK
	static char *rewrite = "rewrite after verify failure";
#endif	DKIOCWCHK

	/*
	 * This tests whether an error occured.  NOTE: errors reported by
	 * the status register of the controller must be folded into the
	 * iopb before this routine is called or they will not be noticed.
	 */
	if (xy->xy_iserr) {
		errptr = finderr(xy->xy_errno);
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
			bn = ((xy->xy_cylinder * un->un_g->dkg_nhead) +
			    xy->xy_head) * un->un_g->dkg_nsect + xy->xy_sector;
			ndone = bn - cmdblk->blkno;
			/*
			 * Log the error for diagnostics if appropriate.
			 */
			if (cmdblk->flags & XY_DIAG) {
				un->un_errsect = bn;
				un->un_errno = errptr->errno;
				un->un_errcmd =
				    (xy->xy_cmd << 8) | xy->xy_subfunc;
			}
		} else
			bn = ndone = 0;
		if (errptr->errlevel != XY_ERCOR) {
			/*
			 * If the error wasn't corrected, see if it's a
			 * bad block.  If we are already in the middle of
			 * forwarding a bad block, we are not allowed to
			 * encounter another one.  NOTE: the check of the
			 * command is to avoid false mappings during initial
			 * stuff like trying to reset a drive
			 * (the bad map hasn't been initialized).
			 */
			if (((xy->xy_cmd == (XY_READ >> 8)) ||
			    (xy->xy_cmd == (XY_WRITE >> 8))) &&
			    (ns = isbad(un->un_bad, (int)xy->xy_cylinder,
			    (int)xy->xy_head, (int)xy->xy_sector)) >= 0) {
				if (cmdblk->flags & XY_INFRD) {
					printf("xy%d: recursive mapping --",
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
				cmdblk->flags |= XY_INFRD;
				if ((xyerrlvl & EL_FORWARD) &&
				    (!(cmdblk->flags & XY_NOMSG))) {
					printf("xy%d: forwarding %d/%d/%d",
					    un->un_md->md_unit,
					    xy->xy_cylinder, xy->xy_head,
					    xy->xy_sector);
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
			actptr = &xyerracts[errptr->errlevel];
			/*
			 * Attempt to retry the entire operation if appropriate.
			 */
			if (cmdblk->retries++ < actptr->retry) {
				if ((xyerrlvl & EL_RETRY) &&
				    (!(cmdblk->flags & XY_NOMSG)) &&
				    (errptr->errlevel != XY_ERBSY))
					printerr(un, xy->xy_cmd, cmdblk->device,
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
				if ((xyerrlvl & EL_RESTR) &&
				    (!(cmdblk->flags & XY_NOMSG)))
					printerr(un, xy->xy_cmd, cmdblk->device,
					    "restore", errptr->errmsg, bn);
				cmdblk->retries = 0;
				bn = ns = 0;
				xy->xy_cmd = XY_RESTORE >> 8;
				xy->xy_subfunc = 0;
				cmdblk->flags |= XY_INRST;
				goto exec;
			}
			/*
			 * Attempt to reset the controller if appropriate.
			 * We must busy wait for the controller, since no
			 * interrupt is generated.
			 */
			if (cmdblk->resets++ < actptr->reset) {
				if ((xyerrlvl & EL_RESET) &&
				    (!(cmdblk->flags & XY_NOMSG)))
					printerr(un, xy->xy_cmd, cmdblk->device,
					    "reset", errptr->errmsg, bn);
				cmdblk->retries = cmdblk->restores = 0;
				ns = c->c_io->xy_resupd;
				if (!xywait(c->c_io)) {
					printf("xyc%d: ", c - xyctlrs);
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
			 * If this was the read portion of a WRITE CHECK
			 * operation, drop down into the code where we
			 * try the write again. Otherwise, give up. That
			 * is, if we get a HARD ECC error on the readback
			 * drop down into the code that rewrites the
			 * block if re-verifies haven't been exhausted
			 * yet.
			 */
			if (cmdblk->flags & XY_INWCHK) {
				if (xy->xy_cmd == XY_READ>>8) {
					goto fail1;
				} else {
					cmdblk->flags &= ~XY_INWCHK;
				}
			}
#endif	DKIOCWCHK
			if ((xyerrlvl & EL_FAIL) &&
			    (!(cmdblk->flags & XY_NOMSG)))
				printerr(un, xy->xy_cmd, cmdblk->device,
				    "failed", errptr->errmsg, bn);
			/*
			 * If the failure was caused by
			 * a 'drive busy' type error, the drive has probably
			 * been taken offline, so we mark it as gone.
			 */
			if ((errptr->errlevel == XY_ERBSY) &&
			    (un != NULL) && (un->un_flags & XY_UN_PRESENT)) {
				un->un_flags &= ~XY_UN_PRESENT;
				if (un->un_md->md_dk >=0)
					dk_bps[un->un_md->md_dk] = 0;
				printf("xy%d: offline\n", un->un_md->md_unit);
			}
			/*
			 * If the failure implies the drive is faulted, do
			 * one final restore in hopes of clearing the fault.
			 */
			if ((errptr->errlevel == XY_ERFLT) &&
			    (!(cmdblk->flags & XY_FNLRST))) {
				bn = ns = 0;
				xy->xy_cmd = XY_RESTORE >> 8;
				xy->xy_subfunc = 0;
				cmdblk->flags &= ~(XY_INFRD | XY_INRST);
				cmdblk->flags |= XY_FNLRST;
				cmdblk->failed = 1;
				goto exec;
			}
			/*
			 * If the failure implies the controller is hung, do
			 * a controller reset in hopes of fixing its state.
			 */
			if (errptr->errlevel == XY_ERHNG) {
				ns = c->c_io->xy_resupd;
				if (!xywait(c->c_io)) {
					printf("xyc%d: ", c - xyctlrs);
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
		if ((xyerrlvl & EL_FIXED) && (!(cmdblk->flags & XY_NOMSG)))
			printerr(un, xy->xy_cmd, cmdblk->device,
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
		xy->xy_cmd = cmdblk->cmd >> 8;
		xy->xy_subfunc = cmdblk->cmd;
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
#ifdef	DKIOCWCHK
		cmdblk->flags &= ~XY_INWCHK;
#endif	DKIOCWCHK
		return (-1);
#ifdef	DKIOCWCHK
	/*
	 * Here seems a good place to stash write-check operations
	 */
	} else if (cmdblk->flags & XY_INWCHK) {
		/*
		 * We have to restore bn, ns, cmdblk->baddr,
		 * cmdblk->blkno && cmdblk->nsect
		 * because the cmdblk fields may have been fiddled with
		 * by bad144 forwarding....
		 */
		cmdblk->baddr = un->un_savaddr;
		bn = cmdblk->blkno = un->un_savbn;
		ns = cmdblk->nsect = un->un_savnsect;
		cmdblk->retries = cmdblk->restores = cmdblk->resets = 0;

		/*
		 * If this portion of the command was a WRITE command,
		 * then turn it into a READ command. We read back into
		 * where we wrote from (because it is mapped). If that
		 * fails to check correctly, we restore that data
		 * from the un->un_savbuf area which contains a copy
		 * of the orignal data.
		 */

		if (cmdblk->cmd == XY_WRITE) {
			cmdblk->cmd = XY_READ;
			xy->xy_cmd = cmdblk->cmd >> 8;
			xy->xy_subfunc = cmdblk->cmd;
			goto exec;
		}

		cmdblk->cmd = XY_WRITE; /* restore write command */
		xy->xy_cmd = cmdblk->cmd >> 8;
		xy->xy_subfunc = cmdblk->cmd;
		if (xywchk_done(un)) {
fail1:
			/*
			 * If we haven't exhausted our retries,
			 * attempt to rewrite the failing block
			 */
			if (un->un_nverifies++ < XY_REWRITE_MAX) {
				bcopy(un->un_savbuf,
				    (int) un->un_savaddr + DVMA,
				    un->un_savnsect*SECSIZE);
				cmdblk->failed = 0;
				goto exec;
			}
			printerr(un, (u_char) (XY_WRITE>>8),
			    cmdblk->device, "failed", rewrite, bn);
			if (cmdblk->flags & XY_DIAG)
				un->un_errsevere = DK_FATAL;
			cmdblk->flags &= ~XY_INWCHK;
			return (-1);
		}

		/*
		 * Verify done. If we had to rewrite the block, log
		 * it as a fixed error.
		 */

		if (un->un_nverifies) {
			printerr(un, (u_char) (XY_WRITE>>8),
			    cmdblk->device, "fixed", rewrite, bn);
		}
		cmdblk->flags &= ~XY_INWCHK;
		return (0);
#endif	DKIOCWCHK
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
		xy->xy_cylinder = bn / (un->un_g->dkg_nhead *
		    un->un_g->dkg_nsect);
		xy->xy_head = (bn / un->un_g->dkg_nsect) % un->un_g->dkg_nhead;
		xy->xy_sector = bn % un->un_g->dkg_nsect;
	} else
		xy->xy_cylinder = xy->xy_head = xy->xy_sector = 0;
	xy->xy_nsect = ns;
	xy->xy_unit = cmdblk->slave;
	xy->xy_bufoff = XYOFF(cmdblk->baddr);
	xy->xy_bufrel = XYNEWREL(c->c_flags, cmdblk->baddr);
	/*
	 * Clear out the iopb fields that need it.
	 */
	cleariopb(xy);
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
 * unknown error).  It is used by xyrecover().  It is always called at
 * disk interrupt priority.
 */
static struct xyerror *
finderr(errno)
	register u_char errno;
{
	register struct xyerror *errptr;

	for (errptr = xyerrors; errptr->errno != XYE_UNKN; errptr++)
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
	struct xyunit *un;
	u_char cmd;
	short device;
	char *action, *msg;
	int bn;
{

	if (device != NOLPART)
		printf("xy%d%c: ", UNIT(device), LPART(device) + 'a');
	else if (un != NULL)
		printf("xy%d: ", un->un_md->md_unit);
	else
		printf("xy: ");
	printf("%s %s (%s) -- ", xycmdnames[cmd], action, msg);
	if ((device != NOLPART) && (un != NULL))
		printf("blk #%d, ", bn - un->un_map[LPART(device)].dkl_cylno *
		    un->un_g->dkg_nsect * un->un_g->dkg_nhead);
	printf("abs blk #%d\n", bn);
}

/*
 * This routine is the actual interface to the controller registers.  It
 * starts the controller up on the iopb passed.  It is used by xychain()
 * and xysynch().  It is always called at disk interrupt priority.
 */
xyexec(xy, xyio, ctlr)
	register struct xyiopb *xy;
	register struct xydevice *xyio;
	int ctlr;
{
	register int iopbaddr;

	/*
	 * Calculate the address of the iopb in 450 terms.
	 */
	iopbaddr = XYOFF(((char *)xy) - DVMA);
	/*
	 * NOTE : We do not initialize the iopb relocation regs
	 * for every command.  They were set to zero during
	 * initialization.  This approach assumes that all iopb's
	 * are in the first 64K of DVMA space.
	 */
	/*
	 * NOTE: the delay/readback approach to the controller registers
	 * is due to a bug in the 450.  Occasionally the registers will
	 * not respond to a write, so we check every time.
	 */
	/*
	 * Check for an already busy controller.  In the asynchronous
	 * case, this implies that something is corrupted.  In the
	 * synchronous case, we just cleared the controller state so this
	 * should never happen.
	 */
	if (xyio->xy_csr & (XY_BUSY | XY_INTR)) {
		printf("xyc%d: ", ctlr);
		printf("regs accessed while busy, csr = 0x%x\n", xyio->xy_csr);

		/* call new wait routine */
		(void)xyintwait(xyio);

		if (xyio->xy_csr & (XY_BUSY | XY_INTR)) {
			printf("xyc%d: ", ctlr);
			panic("regs accessed while busy");
		}
	}
	/*
	 * Set the iopb address registers, checking for glitches.
	 */
	xyio->xy_iopboff[0] = iopbaddr >> 8;
	DELAY(15);
	if (xyio->xy_iopboff[0] != (iopbaddr >> 8)) {
		xyio->xy_iopboff[0] = iopbaddr >> 8;
		DELAY(15);
		if (xyio->xy_iopboff[0] != (iopbaddr >> 8)) {
			printf("xyc%d: ", ctlr);
			panic("iopboff[0] double miscompare");
		}
	}
	xyio->xy_iopboff[1] = iopbaddr & 0xff;
	DELAY(15);
	if (xyio->xy_iopboff[1] != (iopbaddr & 0xff)) {
		xyio->xy_iopboff[1] = iopbaddr & 0xff;
		DELAY(15);
		if (xyio->xy_iopboff[1] != (iopbaddr & 0xff)) {
			printf("xyc%d: ", ctlr);
			panic("iopboff[1] double miscompare");
		}
	}
	/*
	 * Set the go bit, checking for glitches.  Note that we must
	 * check for the case where the command finishes during the delay,
	 * so if the command is done the go bit is allowed to not be set.
	 */
	xyio->xy_csr = XY_GO;
	DELAY(15);
	if (!(xyio->xy_csr & (XY_GO | XY_INTR | XY_DBLERR))) {
		if (xy->xy_complete)
			return;
		xyio->xy_csr = XY_GO;
		DELAY(15);
		if (!(xyio->xy_csr & (XY_GO | XY_INTR | XY_DBLERR))) {
			if (xy->xy_complete)
				return;
			printf("xyc%d: ", ctlr);
			panic("csr double miscompare");
		}
	}
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
xyopen(dev, flag)
	dev_t dev;
	int flag;
{
	register struct xyunit *un;
	register struct mb_device *md;
	struct xyctlr *c;
	int i, high, low, unit, s;

	unit = UNIT(dev);
	/*
	 * Ensure that the device is configured.
	 */
	if (unit >= nxy)
		return (ENXIO);
	/*
	 * If the disk is not present, we need to look for it.
	 */
	un = &xyunits[unit];
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
			if (md->md_driver != &xycdriver || md->md_unit != unit)
				continue;
			/*
			 * Found a match.  If the controller is wild-carded,
			 * we will look at each controller in order until
			 * we find a disk.
			 */
			if (md->md_ctlr == '?') {
				low = 0;
				high = nxyc - 1;
			} else
				low = high = md->md_ctlr;
			/*
			 * Do the actual search for an available disk.
			 */
			for (i = low; i <= high; i++) {
				c = &xyctlrs[i];
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
					md->md_mc = xycinfo[i];
					md->md_hd = md->md_mc->mc_mh;
					md->md_addr = md->md_mc->mc_addr;
					xydinfo[unit] = md;
					if (md->md_dk && dkn < DK_NDRIVE)
						md->md_dk = dkn++;
					else
						md->md_dk = -1;
				}
				/*
				 * Print the found message and attach the
				 * disk to the system.
				 */
				printf("xy%d at xyc%d slave %d\n",
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
		if ((cdevsw[major(dev)].d_open == xyopen) &&
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
xysize(dev)
	dev_t dev;
{
	struct xyunit *un;
	struct dk_map *lp;

	/*
	 * Ensure that the device is configured.
	 */
	if (UNIT(dev) >= nxy)
		return (-1);
	un = &xyunits[UNIT(dev)];
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
xystrategy(bp)
	register struct buf *bp;
{
	register struct xyunit *un;
	register struct dk_map *lp;
	register struct diskhd *dp;
	register int unit, s;
	daddr_t bn;

	unit = dkunit(bp);
	if (unit >= nxy)
		goto bad;
	un = &xyunits[unit];
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
	 * Calculate which cylinder the operation starts on.
	 */
	bp->b_cylin = bn / (un->un_g->dkg_nsect * un->un_g->dkg_nhead) +
	    lp->dkl_cylno + un->un_g->dkg_bcyl;
	/*
	 * If the operation is off the end of the disk, it's an error.
	 */
	if (bp->b_cylin >= un->un_g->dkg_ncyl)
		goto bad;
	dp = &un->un_md->md_utab;
	/*
	 * Ready to queue the request - allocate DVMA resources
	 * now if possible to avoid the overhead of doing it in
	 * the interrupt routine.
	 *
	 * XXX - We let mbpresetup sleep if there's no resources
	 * right now.  Better might be to keep a queue of requests
	 * that don't yet have mb resources and as resources become
	 * available assign them to the request at the head of the
	 * queue.  It's important that the driver queue only contain
	 * requests with resources allocated, otherwise it might
	 * deadlock trying to process a request when no resources are
	 * available because they are all held by requests further
	 * back in the queue.
	 */
	mbpresetup(un->un_md->md_hd, bp, 0);

	/*
	 * We're playing with queues, so we lock out interrupts.  Sort the
	 * operation into the queue for that disk and start up an operation
	 * if the disk isn't busy.
	 */
	s = splr(pritospl(un->un_mc->mc_intpri));
	disksort(dp, bp);
	if (!(un->un_cmd.flags & XY_FBSY))
		xyustart(un);
	(void) splx(s);
	return;
	/*
	 * The operation was erroneous for some reason.  Mark the buffer
	 * with the error and call it done.
	 */
bad:
	bp->b_flags |= B_ERROR;
	iodone(bp);
	return;
}

/*
 * This routine starts an operation for a specific unit.  It is used only
 * in the normal asynchronous case, and is only called if the unit's iopb
 * is not busy.  It is ok to call this routine even if there are no
 * outstanding operations, so it is called whenever the unit becomes free.
 * It is used by xycmd(), xyintr() and xystrategy().  It is always called
 * at disk interrupt priority.
 */
xyustart(un)
	register struct xyunit *un;
{
	register struct diskhd *dp;
	register struct buf *bp;
	register short dk;

	/*
	 * Mark the unit as idle and return if there are no waiting operations.
	 */
	dk = un->un_md->md_dk;
	dk_busy &= ~(1 << dk);
	dp = &un->un_md->md_utab;
	if ((bp = dp->b_actf) == NULL)
		return;
	/*
	 * Mark the unit as busy.
	 */
	if (dk >= 0) {
		dk_busy |= 1 << dk;
		dk_seek[dk]++;
	}
	/*
	 * Put the operation in the currently active slot.
	 */
	dp->b_forw = bp;

	/*
	 * Mark the iopb busy so no one takes it while we are waiting for
	 * mainbus space.  Call the unit-oriented mb routine to give us
	 * space.
	 */
	un->un_cmd.flags = XY_FBSY;
	(void) mbugo(un->un_md);
}

/*
 * This routine translates a buf oriented command down to a level where it
 * can actually be executed.  It is called via mbugo() after the necessary
 * mainbus space has been allocated.  It is always called at disk interrupt
 * priority.
 */
xygo(md)
	register struct mb_device *md;
{
	register struct xyunit *un = &xyunits[md->md_unit];
	register struct dk_map *lp;
	register struct buf *bp;
	int bufaddr, secnt, dk;
	u_short cmd;
	daddr_t blkno;

	/*
	 * Check to be sure there's a real command waiting.
	 */
	bp = md->md_utab.b_forw;
	if (bp == NULL) {
		printf("xy%d: ", md->md_unit);
		panic("queueing error 1");
	}
	if (dkunit(bp) != md->md_unit) {
		printf("xy%d: ", md->md_unit);
		panic("queueing error 2");
	}
	/*
	 * Extract the address of the mainbus space for the operation.
	 */
	bufaddr = MBI_ADDR(md->md_mbinfo);
	/*
	 * Calculate how many sectors we really want to operate on and
	 * set resid to reflect it.
	 */
	lp = &un->un_map[LPART(bp->b_dev)];
	secnt = howmany(bp->b_bcount, SECSIZE);
	secnt = MIN(secnt, lp->dkl_nblk - dkblock(bp));
	un->un_residual = bp->b_bcount - secnt * SECSIZE;
	/*
	 * Calculate all the parameters needed to execute the command.
	 */
	if (bp->b_flags & B_READ)
		cmd = XY_READ;
	else
		cmd = XY_WRITE;
	blkno = dkblock(bp);
	blkno += lp->dkl_cylno * un->un_g->dkg_nhead * un->un_g->dkg_nsect;
	/*
	 * Make sure we didn't run over the address space limit.
	 */
	if ((bufaddr + secnt * SECSIZE) > 0x100000 &&
	    (!(un->un_cmd.c->c_flags & XY_C_24BIT))) {
		printf("xy%d: ", md->md_unit);
		panic("exceeded 20 bit address");
	}
	if ((bufaddr + secnt * SECSIZE) > 0x1000000) {
		printf("xy%d: ", md->md_unit);
		panic("exceeded 24 bit address");
	}
	/*
	 * Update stuff for iostat.
	 */
	if ((dk = md->md_dk) >= 0) {
		dk_busy |= 1 << dk;
		dk_xfer[dk]++;
		if (bp->b_flags & B_READ)
			dk_read[dk]++;
		dk_wds[dk] += bp->b_bcount >> 6;
	}
	/*
	 * Execute the command.
	 */
	(void) xycmd(&un->un_cmd, cmd, minor(bp->b_dev), (caddr_t)bufaddr,
	    md->md_slave, blkno, secnt, XY_ASYNCH, 0);
}

/*
 * This routine polls all the controllers to see if one is interrupting.
 * It is called whenever a non-vectored interrupt of the correct priority
 * is received.  It is always called at disk interrupt priority.
 */
xypoll()
{
	register struct xyctlr *c;
	register int serviced = 0;

	for (c = xyctlrs; c < &xyctlrs[nxyc]; c++) {
		if (!(c->c_flags & XY_C_PRESENT) || !xycsrvalid(c->c_io) ||
		    (c->c_io->xy_csr & XY_INTR) == 0)
			continue;
		serviced = 1;
		xyintr(c);
	}
	return (serviced);
}

/*
 * This routine handles controller interrupts.  It is called by xypoll(),
 * xywatch() or whenever a vectored interrupt for a 450 is received.
 * It is always called at disk interrupt priority.
 */
xyintr(c)
	register struct xyctlr *c;
{
	register struct xycmdblock *cmdblk, *nextcmd;
	register struct xyunit *un;
	register struct xydevice *xyio;
	int stat;
	u_char errno = 0;

	xyio = c->c_io;
	/* wait for controller to settle down  */
	XYCSRVALID(xyio);

        /*
	 * validate interrupt by checking csr for a pending interrupt.
	 * if NOT valid then just return.
         */
        if ((xyio->xy_csr & XY_INTR) == 0) {
                return;
        }

	/*
	 * Clear the interrupt.  If we weren't expecting it, just return.
	 */
	xyio->xy_csr = XY_INTR;
	DELAY(15);
	/*
	 * Use new wait routine called xyintwait which waits for INTR and
	 * BUSY to be deasserted defore proceeding.
	 */
	(void)xyintwait(xyio);
        /*
         * make sure the busy bit goes ON before we wait until it clears..
         * This is a problem with faster machines where the controller does
         * not have enough time to react to the command.
         * Changed by EK 9/10/89

        while (!(xyio->xy_csr & (XY_BUSY|XY_DBLERR|XY_ERROR)));
        DELAY(10);
/* 	(void) xycsrvalid(xyio); */
        if (!c->c_wantint) {
                DELAY(30);
                return;
        }
	/*
	 * If the controller claims an error occured, we need to assign
	 * it to an iopb so the error recovery will see it.
	 */
	if (xyio->xy_csr & XY_DBLERR)
		errno = XYE_DERR;
	else if (xyio->xy_csr & XY_ERROR)
		errno = XYE_ERR;
	if (errno != 0) {
		/*
		 * Clear the error.  If we find an iopb with the error flag
		 * set, that is all we need.
		 */
		xyio->xy_csr = XY_ERROR;
		for (cmdblk = c->c_chain; cmdblk != NULL; cmdblk = cmdblk->next)
			if (cmdblk->iopb->xy_iserr)
				break;
		/*
		 * If no one claimed the error, we blow away all the
		 * operations since we don't know which one failed.
		 */
		if (cmdblk == NULL) {
			for (cmdblk = c->c_chain; cmdblk != NULL;
			    cmdblk = cmdblk->next) {
				cmdblk->iopb->xy_iserr = 1;
				cmdblk->iopb->xy_errno = errno;
				cmdblk->iopb->xy_complete = 1;
			}
		}
		(void) xycsrvalid(xyio);
	}
	/*
	 * At this point, any register error has been correctly folded
	 * into the iopbs.  We now walk through the chain, handling each
	 * iopb individually.  The chain is dismantled as it is traversed.
	 */
	for (cmdblk = c->c_chain; cmdblk != NULL;
	    nextcmd = cmdblk->next, cmdblk->next = NULL, cmdblk = nextcmd) {

		/*
		 * If the done bit isn't set, we just ignore the iopb; it
		 * will get chained up and executed again.
		 */
		if (!cmdblk->iopb->xy_complete)
			continue;
		un = c->c_units[cmdblk->slave];
		/*
		 * Mark the unit as used and not busy.
		 */
		if (un != NULL) {
			un->un_ltick = xyticks;
			dk_busy &= ~(1 << un->un_md->md_dk);
		}
		/*
		 * Execute the error recovery on the iopb.
		 */
		stat = xyrecover(cmdblk, xyasynch);
		/*
		 * If stat came back greater than zero, the
		 * error recovery has re-executed the command.
		 * In that case, we ignore it since it will get chained
		 * up later.
		 */
		if (stat > 0)
			continue;
		/*
		 * In the ASYNCHWAIT case, we pass back
		 * status via the flags and wakeup the
		 * calling process.
		 */
		if (cmdblk->flags & XY_WAIT) {
			cmdblk->flags &= ~XY_WAIT;
			if (stat == -1)
				cmdblk->flags |= XY_FAILED;
			cmdblk->flags |= XY_DONE;
			wakeup((caddr_t)cmdblk);
		/*
		 * In the ASYNCH case, we pass back status
		 * via the buffer.  If the command used
		 * mainbus space, we release that.  If
		 * someone wants the iopb, wake
		 * them up, otherwise start up the next
		 * buf operation.
		 */
		} else {
			cmdblk->flags &= ~XY_FBSY;
			if ((cmdblk->cmd == XY_READ) ||
			    (cmdblk->cmd == XY_WRITE)) {
				if (stat == -1)
					un->un_md->md_utab.b_forw->b_flags |=
					    B_ERROR;
				mbudone(un->un_md);
			}
			if (cmdblk->flags & XY_WANTED) {
				cmdblk->flags &= ~XY_WANTED;
				wakeup((caddr_t)cmdblk);
			} else
				xyustart(un);
		}
	}
	/*
	 * Chain together the ready iopbs and execute them.
	 */
	c->c_chain = NULL;
	c->c_wantint = 0;
	xychain(c);
}

#ifdef	DKIOCWCHK


static void
xywchk_start(un)
	register struct xyunit *un;
{
	/*
	 * Somebody may have disable write checking, and un_savbuf
	 * may have gone away.
	 */
	if (un->un_savbuf) {
		bcopy((int) un->un_savaddr + DVMA, un->un_savbuf,
			MIN(un->un_savnsect*SECSIZE, maxphys));
		return;
	}
}

#ifdef	DDD
static int xy_generr = 0;
static int xy_errmod;
#endif	DDD

static int
xywchk_done(un)
	register struct xyunit *un;
{
	/*
	 * Somebody may have disable write checking, and un_savbuf
	 * may have gone away.
	 */
	if (un->un_savbuf) {
#ifdef	DDD
		if (xy_generr) {
			if (xy_errmod++ == xy_generr || xy_generr == -1) {
				caddr_t p;
				while (xy_errmod >= un->un_savnsect*SECSIZE)
					xy_errmod -= un->un_savnsect *SECSIZE;

				p = (caddr_t) (xy_errmod +
				    (int) un->un_savaddr + (int) DVMA);
				*p ^= 0xff;
				xy_errmod = 0;
			}
		}
#endif	DDD
		return (bcmp((int) un->un_savaddr + DVMA, un->un_savbuf,
				(int)MIN(un->un_savnsect*SECSIZE, maxphys)));
	}
	return (0);
}

#endif	DKIOCWCHK
/*
 * Wait for controller csr to become valid.
 * Waits for at most 200 usec. Returns true if wait succeeded.
 */
int
xycsrvalid(xyio)
	register struct xydevice *xyio;
{
	register int i;

	for (i = 20; i && xyio->xy_csr & (XY_BUSY|XY_DBLERR); i--)
		DELAY(10);
	DELAY(10);
        return ((xyio->xy_csr & (XY_BUSY|XY_DBLERR)) == 0);
}

/*
 * Wait for controller become ready. Used after reset or interrupt.
 * Waits for at most .1 sec. Returns true if wait succeeded.
 * A reset should never take more than .1 sec
 */
int
xywait(xyio)
	register struct xydevice *xyio;
{
	register int i;

	for (i = 10000; i && (xyio->xy_csr & XY_BUSY); i--)
		DELAY(10);
	DELAY(10);
	return ((xyio->xy_csr & XY_BUSY) == 0);
}

/*
 * Wait for controller become ready after an interrupt.
 */
int
xyintwait(xyio)
	register struct xydevice *xyio;
{
	register int i;

	for (i = 20; i && (xyio->xy_csr & (XY_BUSY | XY_INTR)); i--)
		DELAY(10);
	DELAY(10);
	return ((xyio->xy_csr & (XY_BUSY | XY_INTR)) == 0);
}

/*
 * This routine performs raw read operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
xyread(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (UNIT(dev) >= nxy)
		return (ENXIO);
	return (physio(xystrategy, (struct buf *)NULL,
		    dev, B_READ, minphys, uio));
}

/*
 * This routine performs raw write operations.  It is called from the
 * device switch at normal priority.  It uses a per-unit buffer for the
 * operation.
 */
xywrite(dev, uio)
	dev_t dev;
	struct uio *uio;
{

	if (UNIT(dev) >= nxy)
		return (ENXIO);
	return (physio(xystrategy, (struct buf *)NULL,
		    dev, B_WRITE, minphys, uio));
}

/*
 * This routine finishes a buf-oriented operation.  It is called from
 * mbudone() after the mainbus space has been reclaimed.  It is always
 * called at disk interrupt priority.
 */
xydone(md)
	register struct mb_device *md;
{
	/*
	 * Advance device buffer queue
	 * Set residual i/o count
	 * Release the buffer back to the world
	 * Clear active slot.
	 */

	register struct diskhd *dp = &md->md_utab;
	struct buf *bp = dp->b_forw;

	dp->b_actf = bp->av_forw;
	if (dp->b_actl == bp)
	    dp->b_actl = NULL;
	bp->b_resid = xyunits[md->md_unit].un_residual;
	iodone(bp);
	dp->b_forw = NULL;
}

/*
 * These defines are used by some of the ioctl calls.
 */
#define	ONTRACK(x)	(!((x) % un->un_g->dkg_nsect))
#define	XY_MAXBUFSIZE	(63 * 1024)

/*
 * This routine implements the ioctl calls for the 450.  It is called
 * from the device switch at normal priority.
 */
/* ARGSUSED */
xyioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd, flag;
	caddr_t data;
{
	register struct xyunit *un = &xyunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
	struct dk_info *inf;
	struct dk_conf *conf;
	struct dk_type *typ;
	struct dk_badmap *bad;
	struct dk_cmd *com;
	struct dk_diag *diag;
	int i, s, flags = 0, err, hsect, mbcookie, bfr = 0, exec = 0;
	char	cmddir;
	struct	buf *bp = un->un_rtab;
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
			if (!un->un_savbuf) {
				un->un_savbuf =
				    new_kmem_alloc((u_int) maxphys,
					KMEM_SLEEP);
			}
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
			printf("xy%d%c: write check enabled\n",
				un->un_md->md_unit, 'a'+LPART(dev));
		} else  {
			s = splr(pritospl(un->un_mc->mc_intpri));
			(*((int *) data)) =
			    ((un->un_wchkmap & (1<<LPART(dev))) != 0);
			un->un_wchkmap &= ~(1<<LPART(dev));
			if (un->un_wchkmap == 0) {
				if (un->un_savbuf) {
					kmem_free (un->un_savbuf,
					    (u_int) maxphys);
					un->un_savbuf = (caddr_t) 0;
				}
			}
			(void) splx(s);
			printf("xy%d%c: write check disabled\n",
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
			inf->dki_ctype = DKC_XY450;
			inf->dki_flags = DKI_BAD144 | DKI_FMTTRK;
			break;
		/*
		 * Return configuration info
		 */
		case DKIOCGCONF:
			conf = (struct dk_conf *)data;
			(void) strncpy(conf->dkc_cname, xycdriver.mdr_cname,
			    DK_DEVLEN);
			conf->dkc_ctype = DKC_XY450;
			conf->dkc_flags = DKI_BAD144 | DKI_FMTTRK;
			conf->dkc_cnum = un->un_mc->mc_ctlr;
			conf->dkc_addr = getdevaddr(un->un_mc->mc_addr);
			conf->dkc_space = un->un_mc->mc_space;
			conf->dkc_prio = un->un_mc->mc_intpri;
			if (un->un_mc->mc_intr)
				conf->dkc_vec = un->un_mc->mc_intr->v_vec;
			else
				conf->dkc_vec = 0;
			(void) strncpy(conf->dkc_dname, xycdriver.mdr_dname,
			    DK_DEVLEN);
			conf->dkc_unit = un->un_md->md_unit;
			conf->dkc_slave = un->un_md->md_slave;
			break;
		/*
		 * Return drive info
		 */
		case DKIOCGTYPE:
			typ = (struct dk_type *)data;
			typ->dkt_drtype = un->un_drtype;
			s = splr(pritospl(un->un_mc->mc_intpri));
			err = xycmd(&un->un_cmd, XY_STATUS, minor(dev),
			    (caddr_t)0, un->un_md->md_slave, (daddr_t)0, 0,
			    XY_ASYNCHWAIT, 0);
			typ->dkt_hsect = un->un_cmd.iopb->xy_bufrel & 0xff;
			typ->dkt_promrev = un->un_cmd.iopb->xy_nsect >> 8;
			typ->dkt_drstat = un->un_cmd.iopb->xy_status;
			(void) splx(s);
			if (err)
				return (EIO);
			break;
		/*
		 * Set drive info -- only affects drive type.  NOTE: we
		 * must raise the priority around usegeom() because we
		 * may execute an actual command.
		 */
		case DKIOCSTYPE:
			typ = (struct dk_type *)data;
			s = splr(pritospl(un->un_mc->mc_intpri));
			un->un_drtype = typ->dkt_drtype;
			err = usegeom(un, XY_ASYNCHWAIT);
			(void) splx(s);
			if (err)
				return (EINVAL);
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
			    case XY_READ:
				cmddir = XY_IN;
				/* fall through */
			    case XY_WRITE:
				if (com->dkc_buflen != (com->dkc_secnt *
				    SECSIZE)) {
					err = EINVAL;
					goto errout;
				}
				break;
			    case XY_RESTORE:
				if (com->dkc_buflen != 0) {
					err = EINVAL;
					goto errout;
				}
				break;
			    case XY_FORMAT:
				if ((com->dkc_buflen != 0) ||
				    (!ONTRACK(com->dkc_blkno)) ||
				    (!ONTRACK(com->dkc_secnt))) {
					err = EINVAL;
					goto errout;
				}
				break;
			    case XY_READHDR:
				cmddir = XY_IN;
				/* fall through */
			    case XY_WRITEHDR:
				s = splr(pritospl(un->un_mc->mc_intpri));
				err = xycmd(&un->un_cmd, XY_STATUS, minor(dev),
				    (caddr_t)0, un->un_md->md_slave, (daddr_t)0,
				    0, XY_ASYNCHWAIT, XY_NOMSG);
				hsect = un->un_cmd.iopb->xy_bufrel & 0xff;
				(void) splx(s);
				if (err) {
					err = EIO;
					goto errout;
				}
				if ((!ONTRACK(com->dkc_blkno)) ||
				    (com->dkc_buflen != hsect * XY_HDRSIZE)) {
					err = EINVAL;
					goto errout;
				}
				break;
			    case XY_READALL | XY_DEFLST:
				cmddir = XY_IN;
				if ((!ONTRACK(com->dkc_blkno)) ||
				    (com->dkc_buflen != XY_MANDEFSIZE)) {
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
			if (com->dkc_buflen > XY_MAXBUFSIZE)
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
			err = xycmd(&un->un_cmd, com->dkc_cmd, minor(dev),
			    (caddr_t)bfr, un->un_md->md_slave, com->dkc_blkno,
			    com->dkc_secnt, XY_ASYNCHWAIT, flags);
			exec = 1;
			(void) splx(s);
			if (err)
				err = EIO;
			/*
			 * Release memory and DVMA resources.
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
				un->un_errno = XYE_UNKN;
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
 * by itself and xyattach() via the timeout facility.  It is always
 * called at normal priority.
 */
xywatch()
{
	register struct xyunit *un;
	register struct xycmdblock *cmdblk;
	register struct xyctlr *c;
	int s, junk, len;

	/*
	 * Advance time.
	 */
	xyticks++;
	/*
	 * Search the controllers for one who has wanted an interrupt
	 * for too long.
	 */
	for (c = xyctlrs; c < &xyctlrs[nxyc]; c++) {
		/*
		 * Raise priority while looking at these guys....
		 *
		 * This is tricky here, as the controller may,
		 * at any point, wake up and attempt to issue
		 * an interrupt. We will try the best as we
		 * can to avoid this by continuing to look at
		 * the csr to see if an interrupt is pending..
		 *
		 */
		if (!(c->c_flags & XY_C_PRESENT))
			continue;

		s = splr(pritospl(xycinfo[c - xyctlrs]->mc_intpri));

		if (c->c_wantint) {
			/*
			 * Step through the outstanding commands.  For each
			 * one, calculate about how many tracks the command
			 * is for and sum the number of tracks.
			 */
			len = 0;
			for (cmdblk = c->c_chain; cmdblk != NULL;
			    cmdblk = cmdblk->next) {
				un = cmdblk->c->c_units[cmdblk->slave];
				if (un == NULL || un->un_g == NULL) {
					len += 1;
					continue;
				}
				len += cmdblk->nsect / un->un_g->dkg_nsect + 1;
			}
			/*
			 * If the ctlr has been working for less than
			 * XYLOSTINTTIMO times the total number of tracks
			 * outstanding, we give it more time.
			 */
			if (xyticks - c->c_wantint < XYLOSTINTTIMO * len) {
				(void) splx(s);
				continue;
			}
			/*
			 * Found a (possibly) hung one.
			 *
			 * Look for a command on the controller that isn't
			 * done.  If we find one, give it a lost interrupt
			 * error.
			 */
			junk = 0;
			for (cmdblk = c->c_chain; cmdblk != NULL;
				cmdblk = cmdblk->next) {
				if (cmdblk->iopb->xy_complete)
					continue;

				if (c->c_io->xy_csr & XY_INTR) {
					junk = 1;
					break;
				}

				/*
				 * At this point, we are now committed
				 * to giving an error to an iopb
				 * no matter whether or not the
				 * controller woke up and tried
				 * to get at us..
				 */
				cmdblk->iopb->xy_iserr = 1;
				cmdblk->iopb->xy_errno = XYE_LINT;
				cmdblk->iopb->xy_complete = 1;
				break;
			}

			if (junk) {
				/*
				 * an interrupt was pending. accept it.
				 */
				(void) splx(s);
				continue;
			} else if (cmdblk == NULL) {
				if (c->c_io->xy_csr & XY_INTR) {
					/*
					 * an interrupt is pending. accept it.
					 */
					(void) splx(s);
					continue;
				}
				/*
				 * If they were all marked done, we just reset
				 * the controller.
				 */
				/*
				 * We're now committed to a reset
				 */
				junk = c->c_io->xy_resupd;
				if (!xywait(c->c_io)) {
					printf("xyc%d: ", c - xyctlrs);
					panic("controller reset failed");
				}
			}
			/*
			 * Have the controller serviced as if it had
			 * interrupted.
			 */
			xyintr(c);
		}
		(void) splx(s);
	}
#ifdef lint
	junk = junk;
#endif
	/*
	 * Search the disks for one that's online but hasn't been used
	 * for too long.
	 */
	for (un = xyunits; un < &xyunits[nxy]; un++) {
		if (!(un->un_flags & XY_UN_PRESENT))
			continue;
		/*
		 * Raise the priority while we're looking at this unit
		 */
		s = splr(pritospl(un->un_mc->mc_intpri));

		if (xyticks - un->un_ltick < XYWATCHTIMO) {
			(void) splx(s);
			continue;
		}
		/*
		 * If the unit's iopb isn't busy, send the drive a reset
		 * command just to make sure it's still online.
		 */
		if (!(un->un_cmd.flags & XY_FBSY))
			(void) xycmd(&un->un_cmd, XY_RESTORE, NOLPART,
			    (caddr_t)0, un->un_md->md_slave, (daddr_t)0, 0,
			    XY_ASYNCH, 0);
		(void) splx(s);
	}
	/*
	 * Schedule ourself to run again in a little while.
	 */
	timeout(xywatch, (caddr_t)0, hz);
}

/*
 * This routine dumps memory to the disk.  It assumes that the memory has
 * already been mapped into mainbus space.  It is called at disk interrupt
 * priority when the system is in trouble.
 */
xydump(dev, addr, blkno, nblk)
	dev_t dev;
	caddr_t addr;
	daddr_t blkno, nblk;
{
	register struct xyunit *un = &xyunits[UNIT(dev)];
	register struct dk_map *lp = &un->un_map[LPART(dev)];
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
	err = xycmd(&un->un_cmd, XY_WRITE, minor(dev),
	    (caddr_t)((char *)addr - DVMA), unit, blkno, (int)nblk,
	    XY_SYNCH, 0);
	return (err ? EIO : 0);
}


