#ifndef lint
static char sccsid[] = "@(#)gpone.c	1.1  92/07/30  SMI";
#endif

/*
 * Copyright 1986-1988 by Sun Microsystems, Inc.
 */

/*
 * Sun Graphics Processor 1 Driver
 *
 * This driver supports GPs and GP+s driving Sun-2 or Sun-3 color boards.
 * It is dependent on the color board driver, cgtwo.c or cgnine.c.
 *
 * Minor device numbers are used in an unusual way.  Minor numbers 0 to
 * (NREAL - 1) represent real GP/FB combinations with the GP unit number
 * encoded in the upper bits and the FB unit number in the lower 2 bits.
 * Higher minor numbers up to NMINOR are pseudo-devices which are mapped to
 * real minor numbers by a table.  Each time a process opens a GP, it is
 * allocated a unique minor number (there is a hack in the special file
 * open function for this).  This number is used to mark blocks
 * allocated in the GP shared memory so they can be freed when the
 *bound process closes the GP.
 */

#include "gpone.h"
#if NGPONE > 0

#include "cgtwo.h"
#include "cgnine.h"
#include "win.h"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vmmac.h>
#include <sys/conf.h>

#ifndef FNBIO
#define FNBIO   0       /* 3.x compatibility */
#endif
#ifndef FNONBIO
#define FNONBIO 0       /* 3.x compatibility */
#endif

#include <sun/consdev.h>
#include <sun/fbio.h>
#include <sun/gpio.h>
#include <sundev/mbvar.h>

#include <machine/mmu.h>
#include <machine/pte.h>
#include <machine/psl.h>
#include <machine/cpu.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/cg2var.h>
#include <pixrect/cg9var.h>
#include <pixrect/gp1reg.h>
#include <pixrect/gp1var.h>

#ifdef  GPONE_DEBUG
int gp1_debug_flag = 0;
int gp1_block_get = 0;
int gp1_block_free = 0;
#else
#define gp1_increment_sblock()
#define gp1_decrement_sblock(b)
#endif

/* handy macros */
#define	ITEMSIN(array)	(sizeof (array) / sizeof (array)[0])
#ifdef lint
#define	Min(a, b)	((a) + (b))
#define	Max(a, b)	((a) + (b))
#else lint
#define	Min(a, b)	((a) <= (b) ? (a) : (b))
#define	Max(a, b)	((a) >= (b) ? (a) : (b))
#endif lint

/* 
 * macros for maximum number of frame buffers per GP,
 * and extracting GP and FB unit from minor number 
 */

#define GPUNIT(unit)	((unit) >> 2)
/*
#define FBUNIT(unit)	((unit) > 3 ? gp1_minor[(unit)] : (unit))
*/
#define FBUNIT(unit)	((unit) & 3)

/* real, fake, total minor devices */
#define	NREAL		(NGPONE * 4)
#define	NFAKE		Min(NWIN + NREAL * 2, 256 - NREAL)
#define	NMINOR		(NREAL + NFAKE)

/* gp1_sync timeout period, ms */
int gp1_sync_timeout = 500 * 1000;

/* probe size -- enough for entire board */
#define	GP1_PROBESIZE	Max(GP1_SIZE, GP2_SIZE)

/* Mainbus device data */
int gponeprobe(), gponeattach();

struct mb_device *gponeinfo[NGPONE];
struct mb_driver gponedriver = {
	gponeprobe, 0, gponeattach, 0, 0, 0,
	GP1_PROBESIZE, "gpone", gponeinfo, 0, 0, 0, 0
};

/* externs declared in gp_conf.c */
extern int gp1_nfbmax;
extern int gp1_ncg;
extern dev_t gp1_cgfb[];


struct gp1_softc {
    struct gp1     *gp;		       /* virtual address */
    int             size;	       /* mapped size */
    dev_t           *fb;	       /* fb devices, (0, 0) if unused */
    int             gb;		       /* fb which owns gb, -1 if not avail. */
    int             restart;	       /* restart count */
    struct gp1_sblock {		       /* static block owner, minor device */
	struct proc    *owner;
	int             gpminor;
    }               sblock[GP1_STATIC_BLOCKS];
    int             gp2;	       /* GP2 flag */
}               gp1_softc[NGPONE];

extern    struct cdevsw  cdevsw[];

/* real minor device table */
u_char gp1_minor[NMINOR];

#define	MINOR_FREE	127	/* empty slot flag */
#define	MINOR_CLOSE	128	/* close pending */
#define	MINOR_MASK	127	/* mask for minor number */
#define	GPMINOR(minor)	(gp1_minor[minor] & MINOR_MASK)

/* test for valid fb unit */
#define	VALIDFB(softc, fbunit)	\
	((u_int) (fbunit) < gp1_nfbmax && (softc)->fb[(fbunit)] != makedev(0, 0))

/* 
 * pump priming microcode
 *
 *	;			; 		cjp, go .; | jump to zero
 */
static short primeucode[] = {
	0x0068, 0x0000, 0x7140, 0x0000
};

/* 
 * Microcode to probe for graphics buffer.
 * 
 * #define TestVal	0x5432
 * 
 * gbprobe: 
 *	movw 0, y;		am->stlreg;	cjp, ~zbr .;
 * 	mov2nw,s 3, acc;	0x6000->zbhiptr; ;
 * pause1:
 *	sub2nw,s 0, acc;	;		cjp, ~neg pause1;
 * 	incw 0, y;		am->zbhiptr;	cjp, ~zbr .;
 * 	movw d, r[1];		0x2345->am;	;
 * 	movw r[1], y;		am->zbloptr;	cjp, ~zbr .;
 * 	movw d, r[0];		TestVal->am;	;
 * 	movw r[0], y;		am->zbwdreg;	cjp, ~zbr .;
 * 	incw 0, y;		am->zbhiptr;	cjp, ~zbr .;
 * 	movw r[1], y;		am->zbloptr;	cjp, ~zbr .;	zbrd
 * 	subw,s d, r[0], y;	zbrdreg->am;	cjp, ~zbr .;
 * 	;			;		cjp, ~zer .;
 * 	incw 0, y;		am->stlreg;	cjp, go .;
 */
static short gbufucode[] = {
	0x0028, 0x23e0, 0xf900, 0x0000,
	0x000d, 0x6e00, 0xe786, 0x6000,
	0x0008, 0x0390, 0xe185, 0x0002,
	0x0029, 0x63e0, 0xfd00, 0x0003,
	0x002c, 0x9e00, 0xd8c1, 0x2345,
	0x0029, 0x73e0, 0xd841, 0x0005,
	0x002c, 0x9e00, 0xd8c0, 0x5432,
	0x0028, 0xb3e0, 0xd840, 0x0007,
	0x0029, 0x63e0, 0xfd00, 0x0008,
	0x0029, 0x73e5, 0xd841, 0x0009, 
	0x000a, 0x13e0, 0x9600, 0x000a, 
	0x0028, 0x0380, 0x7140, 0x000b, 
	0x00a8, 0x2300, 0xfd00, 0x000c
};

/* status register mask and test value for use after running "gbufucode" */
#define GBUF_STATUSMASK	15
#define GBUF_PRESENT	1
#define isgp2 ((softc->gp->reg.reg.gpr_ident & GP2_ID_MASK) == GP2_ID_VALUE)

/* to probe, we just check the ident register for the correct value */
/*ARGSUSED*/
gponeprobe(reg, unit)
	caddr_t reg;
	int unit;
{
    int             ident;
    int             isgp;
    int		    i;

    ident = peek(&((struct gp1 *) reg)->reg.reg.gpr_ident);

    isgp = (ident & GP1_ID_MASK) == GP1_ID_VALUE ||
	(ident & GP2_ID_MASK) == GP2_ID_VALUE;

    for (i = 0; i < NGPONE; i++) {
	gp1_softc[i].fb = gp1_cgfb;
    }
    return isgp;
}

gponeattach(md)
	struct mb_device *md;
{
	register struct gp1_softc *softc = &gp1_softc[md->md_unit];

	softc->gp = (struct gp1 *) md->md_addr;

	/* no frame buffers configured */
	bzero((caddr_t) softc->fb, sizeof(softc->fb));

	if ((softc->gp->reg.reg.gpr_ident & GP2_ID_MASK) != GP2_ID_VALUE) {
		/* GP1 -- probe for graphics buffer, microcode memory */

		register struct gp1reg *gpr = &softc->gp->reg.reg;
		register u_short umsize;

		softc->size = GP1_SIZE;
		softc->gp2 = 0;

		/* load pipe priming microcode */
		gp1_uload(gpr, primeucode, ITEMSIN(primeucode),
			GP1_CR_VP_STRT0 | GP1_CR_VP_CONT | 
			GP1_CR_PP_STRT0 | GP1_CR_PP_CONT);

		/* load graphics buffer probing microcode */
		gp1_uload(gpr, gbufucode, ITEMSIN(gbufucode),
			GP1_CR_PP_STRT0 | GP1_CR_PP_CONT);

		/* let gp run for 10 ms */
		DELAY(10000);

		softc->gb = (gpr->gpr_csr.status & GBUF_STATUSMASK) == 
			GBUF_PRESENT ? 0 : -1;

		/* size microcode memory */
		gpr->gpr_csr.control =
			GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
		gpr->gpr_csr.control = 0;

		for (umsize = 64; umsize; umsize >>= 1) {
			gpr->gpr_ucode_addr = umsize << 10;
			gpr->gpr_ucode_data = umsize;
		}

		gpr->gpr_ucode_addr = 0;
		umsize = gpr->gpr_ucode_data & 255;

		gpr->gpr_ucode_addr = umsize << 9;
		gpr->gpr_ucode_data = 0x5a;
		gpr->gpr_ucode_addr = umsize << 9;
		if ((gpr->gpr_ucode_data & 255) != 0x5a)
			umsize >>= 1;

		printf("%s%d: GP%s, %uK microcode memory%s\n",
			md->md_driver->mdr_dname, md->md_unit, 
			umsize > 8 ? "+" : "", umsize,
			softc->gb >= 0 ? ", graphics buffer installed" : "");
	}
	else {
		/* GP2 -- just reset, print message */

		softc->size = GP2_SIZE;
		softc->gp2 = 1;
		softc->gb = 0;

		softc->gp->reg.reg2.control = 0;

		printf("%s%d: GP2\n",
			md->md_driver->mdr_dname, md->md_unit);
	}

	softc->restart = 0;

	bzero((caddr_t) softc->sblock, sizeof softc->sblock);

	/* initialize minor device table too */
	{
		register u_char *mp = gp1_minor, gpminor = 0;

		PR_LOOPP(NREAL - 1, *mp++ = gpminor++);
		PR_LOOPP(NFAKE - 1, *mp++ = MINOR_FREE);
	}
}

gponeopen(dev, flag, newdev)
	dev_t dev;
	int flag;
	dev_t *newdev;
{
	register int gpminor = minor(dev);
	register struct gp1_softc *softc;

	/* check GP unit number */
	if (gpminor >= NMINOR || 
		(gpminor = GPMINOR(gpminor)) == MINOR_FREE ||
		fbopen(makedev(0, GPUNIT(gpminor)), flag, NGPONE, gponeinfo))
		return ENXIO;

	/*
	 * If the FNDELAY, FNBIO or FNONBIOflag was specified, the user doesn't
	 * care about frame buffers or the altered minor device.
	 * (This is used by gpconfig.)
	 */
	if (flag & (FNDELAY | FNBIO | FNONBIO))
		return 0;

	/* check FB unit number */
	softc = &gp1_softc[GPUNIT(gpminor)];
	if (!VALIDFB(softc, FBUNIT(gpminor)))
		return ENXIO;

	/* return now if we already have a fake minor device */
	if (minor(dev) >= NREAL)
		return 0;

	/* find a spot in the minor device table */
	do {
		register u_char *p = gp1_minor + NREAL;

		PR_LOOPP(NFAKE - 1,
			if (*p == MINOR_FREE) {
				*p = gpminor;
				*newdev = makedev(major(dev), p - gp1_minor);
				return 0;
			}
			p++);
	} while (gp1_cleanup(softc, GPUNIT(gpminor)));

	/* minor device table is full */
	return ENXIO;
}

/*ARGSUSED*/
gponeclose(dev, flag)
	dev_t dev;
	int flag;
{
	register int gpminor = minor(dev);
	register struct gp1_softc *softc;
	int unit;

	/* mark the device to be closed */
	softc = &gp1_softc[unit =
		GPUNIT((gp1_minor[gpminor] |= MINOR_CLOSE) & MINOR_MASK)];

	/* free any allocated static blocks */
#	ifdef GPONE_DEBUG
	if (gp1_debug_flag & 4)
#	endif
	{
		register struct gp1_sblock *sbp = softc->sblock;

		PR_LOOPP(GP1_STATIC_BLOCKS - 1,
			if (sbp->owner && sbp->gpminor == gpminor)
				sbp->owner = 0;
			sbp++);
	}

	/* execute pending closes if possible */
#	ifdef GPONE_DEBUG
	if (gp1_debug_flag & 1)
#	endif
	(void) gp1_cleanup(softc, unit);

	return 0;
}

gponemmap(dev, off, prot)
	dev_t dev;
	off_t off;
	int prot;
{
    register int    gpminor = GPMINOR(minor(dev));
    register struct gp1_softc *softc = &gp1_softc[GPUNIT(gpminor)];
    struct cdevsw  *devsw;
    dev_t           cgdev;

    if ((u_int) off < softc->size)
	return fbgetpage((caddr_t) softc->gp + off);
    cgdev = softc->fb[FBUNIT(gpminor)];
    if (cgdev != makedev(0, 0) &&
	(devsw = cdevsw + major(cgdev)) != NULL &&
	(devsw->d_mmap != NULL))
	return (*devsw->d_mmap)
	    (softc->fb[FBUNIT(gpminor)], off - softc->size, prot);
    return -1;
}

/*ARGSUSED*/
gponeioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int gpminor = GPMINOR(minor(dev));
	register struct gp1_softc *softc = &gp1_softc[GPUNIT(gpminor)];

	/* some ioctls are handled by the cgtwo/nine driver */
	if (VALIDFB(softc, FBUNIT(gpminor)))
	{
	    static struct use_cg {
		int             cmd;
		int             done;
	    }               use_cg[] = {
		FBIOGETCMAP, 1,
		FBIOPUTCMAP, 1,
		FBIOVERTICAL, 1,
		FBIOGTYPE, 0,
		FBIOGPIXRECT, 0,
		FBIOGINFO, 0,
		FBIOGATTR, 0,
		FBIOSATTR, 1,
		FBIOSVIDEO, 1,
		FBIOGVIDEO, 1
	    };
	    register struct cdevsw *devsw;
	    dev_t           cgdev;
	    register int    loop;
	    register struct use_cg *usep;

	    cgdev = softc->fb[FBUNIT(gpminor)];
	    /*
	     * loop thru the items in "use_cg" table to find a matched
	     * command.  Return "NOTTY" if the command matchs but the
	     * "cgioctl" function is not bound yet, or cgioctl returns
	     * none-zero. If the table indicates so, proceed to the GP part.
	     */
	    for (usep = use_cg, loop = 0;
		 loop < ITEMSIN(use_cg); loop++, usep++) {
		if (usep->cmd == cmd) {
		    if ((devsw = cdevsw + major(cgdev)) != NULL &&
			(devsw->d_ioctl != NULL) &&
			(*(devsw->d_ioctl)) (cgdev, cmd, data, flag))
			return ENOTTY;

		    if (usep->done)
			return 0;
		    break;
		}
	    }
	}

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		fb->fb_type = FBTYPE_SUN2GP;
		fb->fb_size += softc->size;
	}
	break;

	case FBIOGINFO: {
		register struct fbinfo *fbinfo = (struct fbinfo *) data;


		fbinfo->fb_physaddr =
			fbgetpage((caddr_t) softc->gp) << PGSHIFT & 0xffffff;
		fbinfo->fb_addrdelta = softc->size;
		fbinfo->fb_unit = FBUNIT(gpminor);
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;


		gattr->sattr.dev_specific[CG_TYPE] = gattr->fbtype.fb_type;
		gattr->sattr.dev_specific[CG_SIZE] = gattr->fbtype.fb_size;
		gattr->real_type = FBTYPE_SUN2GP;
		gattr->fbtype.fb_type = FBTYPE_SUN2GP;
		gattr->fbtype.fb_size += softc->size;
		gattr->sattr.dev_specific[FB_ATTR_CG2_PRFLAGS] |=
			CG2D_GP |
			(softc->gb == FBUNIT(gpminor) ? CG2D_GB : 0) |
			(softc->gp2 ? CG2D_GP2 : 0);
		gattr->sattr.dev_specific[GP_SHMEMSIZE] = 
		    isgp2 ? GP2_SHMEM_SIZE : GP1_SHMEM_SIZE;
	}
	break;

	case FBIOGPIXRECT:
	{
#	    define CG2_MAJOR 31

	    register struct fbpixrect *fbpixrect = (struct fbpixrect *) data;
	    dev_t           cgdev = softc->fb[FBUNIT(gpminor)];

	    if (major(cgdev) == CG2_MAJOR) {
		struct cg2pr *cg2pr = cg2_d(fbpixrect->fbpr_pixrect);

		cg2pr->gp_shmem = (caddr_t) & softc->gp->shmem;
		cg2pr->ioctl_fd |= gpminor << 8;
		cg2pr->flags |= CG2D_GP;
	    }
	}
	break;

	/* and some ioctl's are handled only by the gp driver	*/
	case GP1IO_PUT_INFO:
	{
	    register struct fbinfo *fbinfo = (struct fbinfo *) data;
	    dev_t           cgdev;

	   if (fbinfo->fb_unit >= gp1_ncg)
		return ENOTTY;
	    if (fbinfo->fb_unit & GP1_UNBIND_FBUNIT) {
		fbinfo->fb_unit &= ~GP1_UNBIND_FBUNIT;
		softc->fb[fbinfo->fb_unit] = makedev(0, 0);
	    } else {
		fbinfo->fb_unit &= ~GP1_UNBIND_FBUNIT;
		if ((u_int) fbinfo->fb_unit >= gp1_nfbmax ||
		    gp1_findfb((caddr_t) fbinfo->fb_ropaddr,
			       &cgdev))
		    return EINVAL;
		softc->fb[fbinfo->fb_unit] = cgdev;
	    }
	}
	break;

	case GP1IO_GET_STATIC_BLOCK: {
		register int block;

		gp1_increment_sblock();
		{
			register struct gp1_sblock *sbp = softc->sblock;

			block = 0;
			while (sbp->owner) {
				if (++block >= GP1_STATIC_BLOCKS) {
					* (int *) data = -1;
					return 0;
				}
				sbp++;
			}

			sbp->owner = u.u_procp;
			sbp->gpminor = minor(dev);
			* (int *) data = block;
		}

		{
			register u_short *p = softc->gp->shmem.stat[block];

			block = GP1_BLOCK_SHORTS - 1;
			PR_LOOPVP(block, *p++ = 0);
		}
	}
	break;

	case GP1IO_FREE_STATIC_BLOCK: {
		register int block = * (int *) data;
		register struct gp1_sblock *sbp = &softc->sblock[block];

		gp1_decrement_sblock(block);
		if ((u_int) block >= GP1_STATIC_BLOCKS ||
			sbp->owner == 0 ||
			sbp->gpminor != minor(dev))
			return EINVAL;

		sbp->owner = 0;
	}
	break;

	case GP1IO_CHK_FOR_GBUFFER:
		* (int *) data = (softc->gb >= 0);
		break;

	case GP1IO_SET_USING_GBUFFER: {
		register int gb = * (int *) data;

		if (softc->gb < 0 || !VALIDFB(softc, gb))
			return EINVAL;

		softc->gb = gb;
	}
	break;

	case GP1IO_GET_GBUFFER_STATE:
		* (int *) data = (softc->gb == FBUNIT(gpminor));
		break;

	case GP1IO_CHK_GP:
		/*
		 * Restart GP unless flag argument is given and we can
		 * free one or more posting blocks.
		 */
		if (!(* (int *) data 
			&& gp1_cleanup(softc, GPUNIT(gpminor))))
			gp1_restart(gpminor, "");
		break;

	case GP1IO_GET_RESTART_COUNT:
		* (int *) data = softc->restart;
		break;

	case GP1IO_REDIRECT_DEVFB: 
	{
	    register int    fb = *(int *) data;
	    dev_t           makedevfb_fromromp ();

	    if (fb == -1) {
		if (major (fbdev) == major (dev)) {
		    fbdev = makedevfb_fromromp ();
		    return 0;
		}
		else
		    return EINVAL;
	    }
	    if (!VALIDFB (softc, fb))
		return EINVAL;

	    fbdev = makedev (major (dev), gpminor & ~FBUNIT (~0) | fb);
	}
	break;

	case GP1IO_GET_REQDEV: 
		* (dev_t *) data = makedev(major(dev), gpminor);
		break;

	case GP1IO_GET_TRUMINORDEV:
		* (char *) data = minor(dev);
		break;

	default:
		return ENOTTY;
	}

	return 0;
}

/* execute pending closes -- free post blocks and minor devices */
gp1_cleanup(softc, gpunit)
	struct gp1_softc *softc;
	int gpunit;
{
	register struct gp1_shmem *shp = &softc->gp->shmem;
	register int allo, free, bit;
	register u_char *own, m;

	if (shp->alloc_sem || gp1_sync((caddr_t) shp, 0))
		return 0;

	allo = IF68000(* (long *) &shp->host_alloc_h ^ 
			* (long *) &shp->gp_alloc_h,
		(shp->host_alloc_h ^ shp->gp_alloc_h) << 16 | 
			(shp->host_alloc_l ^ shp->gp_alloc_l));

	free = 0;
	bit = GP1_ALLOC_BIT(23);

	own = shp->block_owners + GP1_OWNER_INDEX(23);

	do {
		if (allo & bit &&
			((m = *own) >= NMINOR ||
			(m = gp1_minor[m]) & MINOR_CLOSE &&
			GPUNIT(m &= MINOR_MASK) == (u_char) gpunit))
			free |= bit;
		own++;
	} while ((bit <<= 1) >= 0);

	IF68000(* (int *) &shp->host_alloc_h ^= free,
		shp->host_alloc_h ^= free >> 16;
		shp->host_alloc_l ^= free);

#	ifdef GPONE_DEBUG
	if (gp1_debug_flag & 2)
#	endif
	gp1_minor_cleanup(gpunit);

	return free;
}

/* clean out minor device table */
static
gp1_minor_cleanup(gpunit)
	register int gpunit;
{
	register u_char *mp = gp1_minor, m;

	PR_LOOPP(NREAL - 1,
		*mp++ &= MINOR_MASK);

	PR_LOOPP(NFAKE - 1,
		m = *mp;
		if (m & MINOR_CLOSE && 
			GPUNIT(m &= MINOR_MASK) == gpunit) {
			m = MINOR_FREE;
			*mp = m;
		}
		mp++);
}

/* halt and restart GP */
static 
gp1_restart(gpminor, msg)
	int gpminor;
	char *msg;
{
	register struct gp1_softc *softc = &gp1_softc[GPUNIT(gpminor)];
	register struct gp1 *gp = softc->gp;
	short ucode[ITEMSIN(primeucode)];

	/* halt the GP */
	if (!softc->gp2) {
		gp->reg.reg.gpr_csr.control = 
			GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
		gp->reg.reg.gpr_csr.control = 0;
	}
	else
		gp->reg.reg2.control = 0;

	/* initialize control block */
	{
		register short *cp = (short *) &gp->shmem;

		PR_LOOPP(GP1_BLOCK_SHORTS - 1, *cp++ = 0);
	}

	gp->shmem.gp_alloc_h = 0x8000;
	gp->shmem.gp_alloc_l = 0x00ff;

	if (!softc->gp2) {
		/* save the first few microcode words */
		{
			register short *up = ucode;

			gp->reg.reg.gpr_ucode_addr = 0;
			PR_LOOPP(ITEMSIN(ucode) - 1, 
				*up++ = gp->reg.reg.gpr_ucode_data);
		}

		/* prime the pipe */
		gp1_uload(&gp->reg.reg, primeucode, ITEMSIN(primeucode),
			GP1_CR_VP_STRT0 | GP1_CR_VP_CONT | 
			GP1_CR_PP_STRT0 | GP1_CR_PP_CONT);

		/* restore old microcode and restart */
		gp1_uload(&gp->reg.reg, ucode, ITEMSIN(ucode),
			GP1_CR_VP_STRT0 | GP1_CR_VP_CONT | 
			GP1_CR_PP_STRT0 | GP1_CR_PP_CONT);
	}
	else {
		/* restart GP2 */
		gp->reg.reg2.control = 
			GP2_CR_XP_HLT | GP2_CR_RP_HLT;
		gp->reg.reg2.control = 
			GP2_CR_XP_HLT | GP2_CR_RP_HLT | GP2_CR_PP_HLT |
			GP2_CR_XP_RST | GP2_CR_RP_RST | GP2_CR_PP_RST;
	}

	/* signal processes holding static blocks */
	{
		register struct gp1_sblock *sbp = softc->sblock;

		PR_LOOPP(GP1_STATIC_BLOCKS - 1,
			if (sbp->owner) 
				psignal(sbp->owner, SIGXCPU);
			sbp++);
	}

	softc->restart++;

	gp1_minor_cleanup(GPUNIT(gpminor));

	printf("\
%s%d%c: Graphics processor restarted%s after time limit exceeded.\n\
\t You may see display garbage because of this action.\n",
		gponedriver.mdr_dname, 
		GPUNIT(gpminor), 'a' + FBUNIT(gpminor), msg);
}

/* reset GP, load microcode, possibly start it */
static
gp1_uload(gpr, ucode, words, start)
	register struct gp1reg *gpr;
	register short *ucode;
	int words;
	short start;
{
	/* hardware reset */
	gpr->gpr_csr.control = 
		GP1_CR_CLRIF | GP1_CR_INT_DISABLE | GP1_CR_RESET;
	gpr->gpr_csr.control = 0;

	/* load microcode */
	gpr->gpr_ucode_addr = 0;
	PR_LOOP(words, gpr->gpr_ucode_data = *ucode++);

	/* start the processors */
	gpr->gpr_csr.control = 0;
	gpr->gpr_csr.control = start;
}

#define NOUNIT -1

static dev_t
makedevfb_fromromp()
{
	char *cp;
	register struct mb_device *md;
	register struct mb_driver *mdr;
	int bwunit = NOUNIT;
	int bwmajor;

	for (md = mbdinit; mdr = md->md_driver; md++) {
		if (!md->md_alive || md->md_slave != -1)
			continue;
		cp = mdr->mdr_dname;
		console_fb(cp, md, &bwmajor, &bwunit);
	}
	if (bwmajor != NOUNIT)
	    return makedev(bwmajor, bwunit);
	else
	    return makedev(0, 0);
}

#endif

/* 
 * gp1_sync -- wait for the GP to finish executing all pending commands 
 */
gp1_sync(shmem, fd)
	caddr_t shmem;
	int fd;
{
    register short *shp,
                    host,
                    diff;

    if (!shmem)
	return 0;
    shp = (short *) &((struct gp1_shmem *) shmem)->host_count;
    host = *shp++;		       /* point to gp_count */

    CDELAY ((diff = *shp - host, diff >= 0), gp1_sync_timeout);

    if (diff < 0)
	gp1_restart (fd >> 8, " by kernel");

    return 0;
}


gp1_sync_from_fd(fd)
    dev_t           fd;
{
    /* there is only one GP */
    caddr_t         smp = 0;

    if (gp1_softc[0].gp)
	smp = (caddr_t) & gp1_softc[0].gp->shmem;
    return gp1_sync (smp, (int) fd);
}


cg_compare (fbaddr, cg, ncg)
    caddr_t         fbaddr;
    int             ncg;
    struct mb_device *cg[];

{
    register int     cgminor = 0;

    for (cgminor = 0; cgminor < ncg; cgminor++) {
	if (cg && *cg && (*cg)->md_alive && fbaddr == (*cg)->md_addr)
	    return cgminor;
    }
    return -1;
}


#ifdef GPONE_DEBUG
gp1_increment_sblock()
{
	gp1_block_get++;
}

gp1_decrement_sblock(b)
int b;
{
	gp1_block_free++;
}
#endif
