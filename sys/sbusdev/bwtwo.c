#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)bwtwo.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

/*
 * Sbus monochrome memory frame buffer driver
 */

#include "win.h"

#include <sys/param.h>
#include <sys/time.h>
#include <sys/buf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/vmmac.h>

#include <sun/fbio.h>

#include <sundev/mbvar.h>
#include <sbusdev/memfb.h>

#include <machine/mmu.h>
#include <machine/pte.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/memvar.h>

addr_t kmem_zalloc();
addr_t map_regs();
void report_dev();

/* configuration options */

#ifndef OLDDEVSW
#define	OLDDEVSW 1
#endif  OLDDEVSW

#ifndef NOHWINIT
#define	NOHWINIT 1
#endif NOHWINIT

#ifndef BW2DEBUG
#define	BW2DEBUG 0
#endif BW2DEBUG

#if NOHWINIT
int bw2_hwinit = 1;
int bw2_type = 1;
int bw2_on = 1;		/* must be 1 to enable bw2 */
#endif NOHWINIT

#if BW2DEBUG
int bw2_debug = 0;
#define	DEBUGF(level, args)	_STMT(if (bw2_debug >= (level)) printf args; )
#else BW2DEBUG
#define	DEBUGF(level, args)	/* nothing */
#endif BW2DEBUG

#if OLDDEVSW
#define	STATIC /* nothing */
#define	bw2_open	bwtwoopen
#define	bw2_close	bwtwoclose
#define	bw2_mmap	bwtwommap
#define	bw2_ioctl	bwtwoioctl
#else OLDDEVSW
#define	STATIC static
#endif OLDDEVSW

/* config info */
static int bw2_identify();
static int bw2_attach();
STATIC int bw2_open();
STATIC int bw2_close();
STATIC int bw2_ioctl();
STATIC int bw2_mmap();

struct dev_ops bwtwo_ops = {
	0,		/* revision */
	bw2_identify,
	bw2_attach,
	bw2_open,
	bw2_close,
	0, 0, 0, 0, 0,
	bw2_ioctl,
	0,
	bw2_mmap,
};


/* per-unit data */
struct bw2_softc {
	struct mfb_reg	*reg;		/* video chip registers */
#if NWIN > 0
	Pixrect		pr;		/* kernel pixrect */
	struct mpr_data prd;		/* pixrect private data */
#define	_w		pr.pr_size.x
#define	_h		pr.pr_size.y
#define	_fb		prd.md_image
#define	_linebytes	prd.md_linebytes
#else NWIN > 0
	int		_w, _h;		/* resolution */
#endif NWIN > 0
	int		size;		/* total size of frame buffer */
	int		basepage;	/* page # for base address */
};

static int nbw2;
static struct bw2_softc *bw2_softc;

/*
 * handy macros
 */
#define	getsoftc(unit)	(&bw2_softc[unit])


static
bw2_identify(name)
	char *name;
{
	DEBUGF(1, ("bw2_identify(%s)\n", name));

#if NOHWINIT
	if (!bw2_on)
		return (0);
#endif NOHWINIT

	if (strcmp(name, "bwtwo") == 0)
		return (++nbw2);
	else
		return (0);
}

static
bw2_attach(devi)
	struct dev_info *devi;
{
	register struct bw2_softc *softc;
	register addr_t reg;
	int nodeid, bytes, basepage;
	static int unit;
	extern struct dev_info *top_devinfo;	/* in autoconf.c */

	DEBUGF(1, ("bw2_attach nbw2=%d unit=%d\n", nbw2, unit));

	/* Allocate softc structs on first attach */
	if (!bw2_softc)
		bw2_softc = (struct bw2_softc *)
			kmem_zalloc((u_int) sizeof (struct bw2_softc) * nbw2);

	softc = getsoftc(unit);

	/* Grab properties from PROM */
	nodeid = devi->devi_nodeid;
	softc->_w = getprop(nodeid, "width", 1152);
	softc->_h = getprop(nodeid, "height", 900);
#if NOHWINIT
	/* if properties are missing initialize the hardware */
	if (getproplen(nodeid, "width") != sizeof (int)) {
		if (!bw2_probe(devi, softc))
			return (-1);
	}
#endif NOHWINIT
	bytes = getprop(nodeid, "linebytes", mpr_linebytes(softc->_w, 1));
#if NWIN > 0
	softc->_linebytes = bytes;
#endif NWIN > 0

	/* Compute size of frame buffer */
	softc->size = bytes = ptob(btopr(bytes * softc->_h));

#if NWIN > 0
	/* only use address property if we are console fb */
	if (reg = (addr_t) getprop(nodeid, "address", 0)) {
		int fbid = getprop(top_devinfo->devi_nodeid, "fb", -1);
		if (fbid == nodeid) {
			softc->_fb = (MPR_T *) reg;
			bytes = 0;
			DEBUGF(2, ("bw2 mapped by PROM\n"));
		}
	}
#else NWIN > 0
	bytes = 0;
#endif NWIN > 0

	/*
	 * Allocate virtual space for registers and maybe frame buffer.
	 * Map registers.
	 */
	if (!(reg = map_regs(devi->devi_reg[0].reg_addr + MFB_OFF_REG,
		(u_int) bytes + NBPG,
		devi->devi_reg[0].reg_bustype)))
		return (-1);

	softc->reg = (struct mfb_reg *) reg;

	/* save base page # for registers */
	softc->basepage = basepage = fbgetpage(reg);

	DEBUGF(1, ("bw2_attach reg=0x%x/0x%x (0x%x)\n",
		(u_int) reg,
		basepage << PGSHIFT,
		basepage));

#if NWIN > 0
	/* map frame buffer if necessary */
	if (bytes) {
		fbmapin(reg + NBPG,
			(int) (basepage + btop(MFB_OFF_FB - MFB_OFF_REG)),
			bytes);
		softc->_fb = (MPR_T *) (reg + NBPG);
	}

	DEBUGF(1, ("bw2_attach fb=0x%x/0x%x (0x%x)\n",
		(u_int) softc->_fb,
		fbgetpage((addr_t) softc->_fb) << PGSHIFT,
		fbgetpage((addr_t) softc->_fb)));
#endif NWIN > 0


	/* save unit number */
	devi->devi_unit = unit;

	/* save back pointer to softc */
	devi->devi_data = (addr_t) softc;

	/* prepare for next attach */
	unit++;

	report_dev(devi);

	return (0);
}

STATIC
bw2_open(dev, flag)
	dev_t dev;
	int flag;
{
	DEBUGF(2, ("bw2_open(%d)\n", minor(dev)));

	return (fbopen(dev, flag, nbw2));
}

/*ARGSUSED*/
STATIC
bw2_close(dev, flag)
	dev_t dev;
	int flag;
{
	DEBUGF(2, ("bw2_close(%d)\n", minor(dev)));

	/* disable cursor compare */
	getsoftc(minor(dev))->reg->control &= ~MFB_CR_CURSOR;

	return (0);
}

/*ARGSUSED*/
STATIC
bw2_mmap(dev, off, prot)
	dev_t dev;
	register off_t off;
	int prot;
{
	register struct bw2_softc *softc = getsoftc(minor(dev));

	DEBUGF(off ? 9 : 1, ("bw2_mmap(%d, 0x%x)\n", minor(dev), (u_int) off));

	if (off == MFB_REG_MMAP_OFFSET && suser())
		off = 0;
	else if ((u_int) off < softc->size)
		off += MFB_OFF_FB - MFB_OFF_REG;
	else
		return (-1);

	DEBUGF(off != MFB_OFF_FB - MFB_OFF_REG ? 9 : 1,
		("bw2_mmap returning 0x%x (0x%x)\n",
		ptob(softc->basepage) + off,
		softc->basepage + btop(off)));

	return (softc->basepage + btop(off));
}

/*ARGSUSED*/
STATIC
bw2_ioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct bw2_softc *softc = getsoftc(minor(dev));

	DEBUGF(2, ("bw2_ioctl(%d, 0x%x)\n", minor(dev), cmd));

	switch (cmd) {

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		DEBUGF(2, ("FBIOGTYPE\n"));

		fb->fb_type = FBTYPE_SUN2BW;
		fb->fb_height = softc->_h;
		fb->fb_width = softc->_w;
		fb->fb_depth = 1;
		fb->fb_cmsize = 2;
		fb->fb_size = softc->size;
	}
	break;

#if NWIN > 0
	case FBIOGPIXRECT:
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		DEBUGF(2, ("FBIOGPIXRECT\n"));
		/* initialize pixrect and private data */
		softc->pr.pr_ops = &mem_ops;
		/* pr_size set in attach */
		softc->pr.pr_depth = 1;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* md_linebytes, md_image set in attach */
		/* md_offset already zero */
		softc->prd.md_primary = minor(dev);
		softc->prd.md_flags = MP_DISPLAY;

		/* enable video */
		mfb_set_video(softc->reg, _ONE_);

		break;
#endif NWIN > 0

	case FBIOSVIDEO:
		DEBUGF(2, ("FBIOSVIDEO\n"));
		mfb_set_video(softc->reg, * (int *) data & FBVIDEO_ON);
		break;

	case FBIOGVIDEO:
		DEBUGF(2, ("FBIOGVIDEO\n"));
		* (int *) data =
			mfb_get_video(softc->reg) ? FBVIDEO_ON : FBVIDEO_OFF;
		break;

	default:
		return (ENOTTY);

	}

	return (0);
}

#if NOHWINIT
static int
bw2_probe(devi, softc)
	struct dev_info *devi;
	struct bw2_softc *softc;
{
	struct mfb_reg *mfb;
	int stat, monitor_type, retval = 0;
	void unmap_regs();

	if (!(mfb = (struct mfb_reg *)
		map_regs(devi->devi_reg[0].reg_addr + MFB_OFF_REG,
		(u_int) NBPG,
		devi->devi_reg[0].reg_bustype)))
		goto bad;

	if ((stat = peekc((char *)&mfb->status)) == -1)
		goto bad;

/* New sense codes only for BW2 FB 501-1561 (prom 1.5) */
#define	MFB_SR_1152_900_76HZ_A	0x40	/* 76Hz monitor sense codes */
#define	MFB_SR_1152_900_76HZ_B	0x60
#define	MFB_SR_ID_MSYNC		0x04	/* SR id of 501-1561 BW2 */

	monitor_type = stat & MFB_SR_RES_MASK;

	switch (stat & MFB_SR_ID_MASK) {
	case MFB_SR_ID_MONO_ECL:
		if (monitor_type == MFB_SR_1600_1280)
			bw2_type = 0;
		else
			bw2_type = 1;
		break;
	case MFB_SR_ID_MONO:
		bw2_type = 2;
		break;
	case MFB_SR_ID_MSYNC:
		if ((monitor_type == MFB_SR_1152_900_76HZ_A) ||
		    (monitor_type == MFB_SR_1152_900_76HZ_B))
			bw2_type = 4;
		else
			bw2_type = 3;
		break;
	default:
		DEBUGF(1, ("bw2_hwinit bad type SR=0x%x\n", (u_int) stat));
		goto bad;
	}

	if (bw2_type == 0) {
		softc->_w = 1600;
		softc->_h = 1280;
	}

	if (bw2_hwinit)
		bw2_init(mfb);

	DEBUGF(1, ("bw2_hwinit type=%d (%d,%d)\n",
		bw2_type, softc->_w, softc->_h));

	retval = 1;
bad:
	if (!retval) {
		DEBUGF(2, ("bw2_probe failed\n"));
	}
	unmap_regs((addr_t) mfb, (u_int) NBPG);
	return (retval);
}

/* high-res ECL */
char bw2_mfbval0[] = {
	0x14, 0x8b,
	0x15, 0x28,
	0x16, 0x03,
	0x17, 0x13,
	0x18, 0x7b,
	0x19, 0x05,
	0x1a, 0x34,
	0x1b, 0x2e,
	0x1c, 0x00,
	0x1d, 0x0a,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x21,
	0
};
/* low-res ECL */
char bw2_mfbval1[] = {
	0x14, 0x65,
	0x15, 0x1e,
	0x16, 0x04,
	0x17, 0x0c,
	0x18, 0x5e,
	0x19, 0x03,
	0x1a, 0xa7,	/* increase for > 900 lines */
	0x1b, 0x23,
	0x1c, 0x00,
	0x1d, 0x08,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x20,
	0
};
/* low-res analog */
char bw2_mfbval2[] = {
	0x14, 0xbb,
	0x15, 0x2b,
	0x16, 0x03,
	0x17, 0x13,
	0x18, 0xb0,
	0x19, 0x03,
	0x1a, 0xa6,
	0x1b, 0x22,
	0x1c, 0x01,
	0x1d, 0x05,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x20,
	0
};
/* BW2 FB 501-1561 (prom 1.5) 66 Hz */
char bw2_mfbval3[] = {
	0x14, 0xbb,
	0x15, 0x2b,
	0x16, 0x04,
	0x17, 0x14,
	0x18, 0xae,
	0x19, 0x03,
	0x1a, 0xa8,
	0x1b, 0x24,
	0x1c, 0x01,
	0x1d, 0x05,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x20,
	0
};
/* BW2 FB 501-1561 (prom 1.5) 76 Hz */
char bw2_mfbval4[] = {
	0x14, 0xb7,
	0x15, 0x27,
	0x16, 0x03,
	0x17, 0x0f,
	0x18, 0xae,
	0x19, 0x03,
	0x1a, 0xae,
	0x1b, 0x2a,
	0x1c, 0x01,
	0x1d, 0x09,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x24,
	0
};
static char *mfbvals[] = {
	bw2_mfbval0,
	bw2_mfbval1,
	bw2_mfbval2,
	bw2_mfbval3,
	bw2_mfbval4,
};

static
bw2_init(mfb)
	struct mfb_reg *mfb;
{
	register char *p;

	/* initialize video chip */
	for (p = mfbvals[bw2_type]; *p; p += 2)
		((char *) mfb)[p[0]] = p[1];

}
#endif NOHWINIT
