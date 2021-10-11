#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)cgthree.c 1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

/*
 * Sbus 8 bit color memory frame buffer driver
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
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <pixrect/cg3var.h>

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

#ifndef CG3DEBUG
#define	CG3DEBUG 0
#endif CG3DEBUG

#if NOHWINIT
int cg3_hwinit = 1;
int cg3_type = 0;	/* == 2 for small */
int cg3_on = 1;		/* must be 1 to enable cg3 */
#endif NOHWINIT

#if CG3DEBUG
int cg3_debug = 0;
#define	DEBUGF(level, args)	_STMT(if (cg3_debug >= (level)) printf args;)
#else CG3DEBUG
#define	DEBUGF(level, args)	/* nothing */
#endif CG3DEBUG

#define	CG3_CMAP_ENTRIES	MFB_CMAP_ENTRIES

#if OLDDEVSW
#define	STATIC /* nothing */
#define	cg3_open	cgthreeopen
#define	cg3_close	cgthreeclose
#define	cg3_mmap	cgthreemmap
#define	cg3_ioctl	cgthreeioctl
#else OLDDEVSW
#define	STATIC static
#endif OLDDEVSW

/* config info */
static int cg3_identify();
static int cg3_attach();
STATIC int cg3_open();
STATIC int cg3_close();
STATIC int cg3_ioctl();
STATIC int cg3_mmap();

struct dev_ops cgthree_ops = {
	0,		/* revision */
	cg3_identify,
	cg3_attach,
	cg3_open,
	cg3_close,
	0, 0, 0, 0, 0,
	cg3_ioctl,
	0,
	cg3_mmap,
};


/* per-unit data */
struct cg3_softc {
	struct mfb_reg	*reg;		/* video chip registers */
#if NWIN > 0
	Pixrect		pr;		/* kernel pixrect */
	struct mprp_data prd;		/* pixrect private data */
#define	_w		pr.pr_size.x
#define	_h		pr.pr_size.y
#define	_fb		prd.mpr.md_image
#define	_linebytes	prd.mpr.md_linebytes
#else NWIN > 0
	int		_w, _h;		/* resolution */
#endif NWIN > 0
	int		size;		/* total size of frame buffer */
	int		dummysize;	/* total size of overlay plane */
	int		basepage;	/* page # for base address */

	u_short		cmap_index;	/* colormap update index */
	u_short		cmap_count;	/* colormap update count */
	union {				/* shadow color map */
		u_long	cmap_long[CG3_CMAP_ENTRIES * 3 / sizeof (u_long)];
		u_char	cmap_char[3][CG3_CMAP_ENTRIES];
	} cmap_image;
#define	cmap_rgb	cmap_image.cmap_char[0]
};

static int ncg3;
static struct cg3_softc *cg3_softc;


/* default structure for FBIOGATTR ioctl */
static struct fbgattr cg3_attr =  {
/*	real_type		owner */
	FBTYPE_SUN3COLOR,	0,
/* fbtype: type             h  w  depth cms  size */
	{ FBTYPE_SUN3COLOR, 0, 0, 8,    256,  0 },
/* fbsattr: flags emu_type      dev_specific */
	{ 0, FBTYPE_SUN4COLOR, { 0 } },
/*        emu_types */
	{ FBTYPE_SUN3COLOR, FBTYPE_SUN4COLOR, -1, -1}
};


/*
 * handy macros
 */

#define	getsoftc(unit)	(&cg3_softc[unit])

#define	btob(n)		ctob(btoc(n))		/* XXX */

#define	BCOPY(s, d, c)	bcopy((caddr_t) (s), (caddr_t) (d), (u_int) (c))
#define	COPYIN(s, d, c)	copyin((caddr_t) (s), (caddr_t) (d), (u_int) (c))
#define	COPYOUT(s, d, c) copyout((caddr_t) (s), (caddr_t) (d), (u_int) (c))

/* enable/disable interrupt */
#define	cg3_int_enable(softc)	mfb_int_enable((softc)->reg)
#define	cg3_int_disable(softc)	mfb_int_disable((softc)->reg)

/* check if color map update is pending */
#define	cg3_update_pending(softc) ((softc)->cmap_count)

/*
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 */
/* XXX why don't I just use a tmp var, or function??? */
#define	cg3_update_cmap(softc, index, count)	\
	if ((softc)->cmap_count) { \
		if ((index) + (count) > \
			(softc)->cmap_count += (softc)->cmap_index) \
			(softc)->cmap_count = (index) + (count); \
		if ((index) < (softc)->cmap_index) \
			(softc)->cmap_index = (index); \
		(softc)->cmap_count -= (softc)->cmap_index; \
	} \
	else { \
		(softc)->cmap_index = (index); \
		(softc)->cmap_count = (count); \
	} \


/*
 * forward references
 */
static cg3_putcolormap();
static cg3_poll();
static void cg3_intr();
static void cg3_reset_cmap();
static void cg3_cmap_bcopy();

#if NWIN > 0

/*
 * SunWindows specific stuff
 */

/* kernel pixrect ops vector */
static struct pixrectops cg3_pr_ops = {
	mem_rop,
	cg3_putcolormap,
	mem_putattributes
};

/* XXX can I share code with ioctl? */
/* XXX vestigial overlay stuff */
static
cg3_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg3_softc *softc = getsoftc(mpr_d(pr)->md_primary);
	register u_int rindex = (u_int) index;
	register u_int rcount = (u_int) count;
	register u_char *map;
	register u_int entries;

	DEBUGF(5, ("cg3_putcolormap unit=%d index=%d count=%d\n",
		mpr_d(pr)->md_primary, index, count));

	map = softc->cmap_rgb;
	entries = CG3_CMAP_ENTRIES;

	/* check arguments */
	if (rindex >= entries ||
		rindex + rcount > entries)
		return (PIX_ERR);

	if (rcount == 0)
		return (0);

	/* lock out updates of the hardware colormap */
	if (cg3_update_pending(softc))
		cg3_int_disable(softc);

	map += rindex * 3;
	cg3_cmap_bcopy(red, map, rcount);
	cg3_cmap_bcopy(green, map + 1, rcount);
	cg3_cmap_bcopy(blue, map + 2, rcount);

	cg3_update_cmap(softc, rindex, rcount);

	/* enable interrupt so we can load the hardware colormap */
	cg3_int_enable(softc);

	return (0);
}

#endif	NWIN > 0

static
cg3_identify(name)
	char *name;
{
	DEBUGF(1, ("cg3_identify(%s)\n", name));

#if NOHWINIT
	if (!cg3_on)
		return (0);
#endif NOHWINIT

	if (strcmp(name, "cgthree") == 0)
		return (++ncg3);
	else
		return (0);
}

static
cg3_attach(devi)
	struct dev_info *devi;
{
	register struct cg3_softc *softc;
	register addr_t reg;
	int w, h, bytes, basepage;
	static int unit=0;
	extern struct dev_info *top_devinfo;	/* in autoconf.c */

	DEBUGF(1, ("cg3_attach ncg3=%d unit=%d\n", ncg3, unit));

	/* Allocate softc structs on first attach */
	if (!cg3_softc)
		cg3_softc = (struct cg3_softc *)
			kmem_zalloc((u_int) sizeof (struct cg3_softc) * ncg3);

	softc = getsoftc(unit);

	/* Grab properties from PROM */
	softc->_w = w = getprop(devi->devi_nodeid, "width", 1152);
	softc->_h = h = getprop(devi->devi_nodeid, "height", 900);
#if NOHWINIT
	/* if properties are missing initialize the hardware */
	if (getproplen(devi->devi_nodeid, "width") != sizeof (int)) {
		if (!cg3_probe(devi, softc))
			return (-1);
	}
#endif NOHWINIT
	bytes = getprop(devi->devi_nodeid, "linebytes", mpr_linebytes(w, 8));
#if NWIN > 0
	softc->_linebytes = bytes;
#endif  NWIN > 0

	/* Compute size of color frame buffer */
	softc->size = bytes = ptob(btopr(bytes * h));

	/* Compute size of dummy overlay/enable planes */
	softc->dummysize = ptob(btopr(mpr_linebytes(w, 1) * h)) * 2;

#if NWIN > 0
	/* only use address property if we are console fb */
	if (reg = (addr_t) getprop(devi->devi_nodeid, "address", 0)) {
		int fbid = getprop(top_devinfo->devi_nodeid, "fb", -1);
		if (fbid == devi->devi_nodeid) {
			softc->_fb = (MPR_T *) reg;
			bytes = 0;
			DEBUGF(2, ("cg3 mapped by PROM\n"));
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

	DEBUGF(1, ("cg3_attach reg=0x%x/0x%x (0x%x)\n",
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

	DEBUGF(1, ("cg3_attach fb=0x%x/0x%x (0x%x)\n",
		(u_int) softc->_fb,
		fbgetpage((addr_t) softc->_fb) << PGSHIFT,
		fbgetpage((addr_t) softc->_fb)));
#endif NWIN > 0

	/* save unit number */
	devi->devi_unit = unit;

	/* attach interrupt */
	addintr(devi->devi_intr[0].int_pri, cg3_poll, devi->devi_name, unit);

	/* save back pointer to softc */
	devi->devi_data = (addr_t) softc;

	/*
	 * Initialize hardware colormap and software colormap images.
	 * It might make sense to read the hardware colormap here.
	 */
	cg3_reset_cmap(softc->cmap_rgb, CG3_CMAP_ENTRIES);

	/* XXX wasteful macro */
	cg3_update_cmap(softc, _ZERO_, CG3_CMAP_ENTRIES);

	/* prepare for next attach */
	unit++;

	report_dev(devi);

	return (0);
}

STATIC
cg3_open(dev, flag)
	dev_t dev;
	int flag;
{
	DEBUGF(2, ("cg3_open(%d)\n", minor(dev)));

	return (fbopen(dev, flag, ncg3));
}

/*ARGSUSED*/
STATIC
cg3_close(dev, flag)
	dev_t dev;
	int flag;
{
	DEBUGF(2, ("cg3_close(%d)\n", minor(dev)));

	/* disable cursor compare */
	getsoftc(minor(dev))->reg->control &= ~MFB_CR_CURSOR;

	return (0);
}

/*ARGSUSED*/
STATIC
cg3_mmap(dev, off, prot)
	dev_t dev;
	register off_t off;
	int prot;
{
	register struct cg3_softc *softc = getsoftc(minor(dev));

	DEBUGF(off ? 9 : 1, ("cg3_mmap(%d, 0x%x)\n", minor(dev), (u_int) off));

	if (off == MFB_REG_MMAP_OFFSET && suser())
		off = 0;
	else if (off >= CG3_MMAP_OFFSET) {
		if ((off -= CG3_MMAP_OFFSET) >= softc->size)
			return (-1);
		off += MFB_OFF_FB - MFB_OFF_REG;
	} else if ((u_int) off < softc->dummysize)
			off = MFB_OFF_DUMMY - MFB_OFF_REG;
	else if ((u_int) (off -= softc->dummysize) < softc->size)
		off += MFB_OFF_FB - MFB_OFF_REG;
	else
		return (-1);

	DEBUGF(off != MFB_OFF_FB - MFB_OFF_REG ? 9 : 1,
		("cg3_mmap returning 0x%x (0x%x)\n",
		ptob(softc->basepage) + off,
		softc->basepage + btop(off)));

	return (softc->basepage + btop(off));
}

/*ARGSUSED*/
STATIC
cg3_ioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct cg3_softc *softc = getsoftc(minor(dev));

	DEBUGF(2, ("cg3_ioctl(%d, 0x%x)\n", minor(dev), cmd));

	switch (cmd) {

	case FBIOPUTCMAP:
	case FBIOGETCMAP: {
		register struct fbcmap *cmap = (struct fbcmap *) data;
		register u_int index = (u_int) cmap->index;
		register u_int count = (u_int) cmap->count;
		register u_char *map;
		register u_int entries;
		static u_char cmbuf[CG3_CMAP_ENTRIES];

		switch (PIX_ATTRGROUP(index)) {
		case 0:
		case PIXPG_8BIT_COLOR:
			map = softc->cmap_rgb;
			entries = CG3_CMAP_ENTRIES;
			break;
		default:
			return (EINVAL);
		}

		if ((index &= PIX_ALL_PLANES) >= entries ||
			index + count > entries)
			return (EINVAL);

		if (count == 0)
			return (0);

		if (cmd == FBIOPUTCMAP) {
			register int error;

			DEBUGF(2, ("FBIOPUTCMAP\n"));
			if (cg3_update_pending(softc))
				cg3_int_disable(softc);

			map += index * 3;

			if (error = COPYIN(cmap->red, cmbuf, count))
				break;
			cg3_cmap_bcopy(cmbuf, map++, count);

			if (error = COPYIN(cmap->green, cmbuf, count))
				break;
			cg3_cmap_bcopy(cmbuf, map++, count);

			if (error = COPYIN(cmap->blue, cmbuf, count))
				break;
			cg3_cmap_bcopy(cmbuf, map, count);

			if (error) {
				if (cg3_update_pending(softc))
					cg3_int_enable(softc);
				return (EFAULT);
			}

			cg3_update_cmap(softc, index, count);
			cg3_int_enable(softc);
		} else {
			/* FBIOGETCMAP */
			DEBUGF(2, ("FBIOGETCMAP\n"));
			map += index * 3;
			cg3_cmap_bcopy(cmbuf, map++, -count);
			if (COPYOUT(cmbuf, cmap->red, count))
				return (EFAULT);

			cg3_cmap_bcopy(cmbuf, map++, -count);
			if (COPYOUT(cmbuf, cmap->green, count))
				return (EFAULT);

			cg3_cmap_bcopy(cmbuf, map, -count);
			if (COPYOUT(cmbuf, cmap->blue, count))
				return (EFAULT);
		}
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *attr = (struct fbgattr *) data;

		DEBUGF(2, ("FBIOGATTR\n"));
		*attr = cg3_attr;
		data = (caddr_t) &attr->fbtype;
	}
	/* fall through */

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		if (cmd == FBIOGTYPE) {
			DEBUGF(2, ("FBIOGTYPE\n"));
			*fb = cg3_attr.fbtype;
			fb->fb_type = FBTYPE_SUN4COLOR;
		}
		fb->fb_width = softc->_w;
		fb->fb_height = softc->_h;
		/* XXX not quite like a cg4 */
		fb->fb_size = softc->size;
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT:
		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		DEBUGF(2, ("FBIOGPIXRECT\n"));
		/* initialize pixrect and private data */
		softc->pr.pr_ops = &cg3_pr_ops;
		/* pr_size set in attach */
		softc->pr.pr_depth = 8;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* md_linebytes, md_image set in attach */
		/* md_offset already zero */
		softc->prd.mpr.md_primary = minor(dev);
		softc->prd.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
		softc->prd.planes = 255;

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

	} /* switch (cmd) */

	return (0);
}

static
cg3_poll()
{
	register int i, serviced = 0;
	register struct cg3_softc *softc;

	/*
	 * Look for any frame buffers that were expecting an interrupt.
	 */

	DEBUGF(7, ("cg3_poll\n"));
	for (softc = cg3_softc, i = ncg3; --i >= 0; softc++)
		if (mfb_int_pending(softc->reg)) {
			if (cg3_update_pending(softc))
				cg3_intr(softc);
			else
				/* XXX catch stray interrupts? */
				cg3_int_disable(softc);
			serviced++;
		}

	return (serviced);
}

static void
cg3_intr(softc)
	register struct cg3_softc *softc;
{
	register struct mfb_cmap *cmap = &softc->reg->cmap;
	register LOOP_T index = softc->cmap_index;
	register LOOP_T count = softc->cmap_count;
	register u_long *in = &softc->cmap_image.cmap_long[0];
	register u_long *out = (u_long *) &cmap->cmap;

	DEBUGF(7, ("cg3_intr(%d)\n", softc - cg3_softc));

	/* count multiples of 4 RGB entries */
	count = (count + (index & 3) + 3) >> 2;

	/* round index to 4 entry boundary */
	index &= ~3;

	cmap->addr = index;
	PTR_INCR(u_long *, in, index * 3);

	/* copy 12 bytes (4 RGB entries) per loop iteration */
	PR_LOOPV(count,
		*out = in[0];
		*out = in[1];
		*out = in[2];
		in += 3);

	softc->cmap_count = 0;
	cg3_int_disable(softc);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
cg3_reset_cmap(cmap, entries)
	register u_char *cmap;
	register u_int entries;
{
	bzero((char *) cmap, entries * 3);
	cmap[0] = 255;
	cmap[1] = 255;
	cmap[2] = 255;
}

/*
 * Copy colormap entries between red, green, or blue array and
 * interspersed rgb array.
 *
 * count > 0 : copy count bytes from buf to rgb
 * count < 0 : copy -count bytes from rgb to buf
 */
static void
cg3_cmap_bcopy(bufp, rgb, count)
	register u_char *bufp, *rgb;
	u_int count;
{
	register LOOP_T rcount = count;

	if (--rcount >= 0)
		PR_LOOPVP(rcount,
			*rgb = *bufp++;
			rgb += 3);
	else {
		rcount = -rcount - 2;
		PR_LOOPVP(rcount,
			*bufp++ = *rgb;
			rgb += 3);
	}
}

#if NOHWINIT
static int
cg3_probe(devi, softc)
	struct dev_info *devi;
	struct cg3_softc *softc;
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

/* New sense codes for CG3 FB 501-1718 (prom 1.4) */
#define	MFB_SR_1152_900_76HZ_A	0x40	/* 76Hz monitor sense codes */
#define	MFB_SR_1152_900_76HZ_B	0x60

	monitor_type = stat & MFB_SR_RES_MASK;

	switch (stat & MFB_SR_ID_MASK) {
	case MFB_SR_ID_COLOR:
		if ((monitor_type == MFB_SR_1152_900_76HZ_A) ||
		    (monitor_type == MFB_SR_1152_900_76HZ_B))
			cg3_type = 1;
		break;
	default:
		DEBUGF(1, ("cg3_hwinit bad type SR=0x%x\n", (u_int) stat));
		goto bad;
	}

	if (cg3_type == 2) {
		softc->_w = 640;
		softc->_h = 480;
	}

	if (cg3_hwinit)
		cg3_init(mfb);

	DEBUGF(1, ("cg3_hwinit type=%d (%d,%d)\n",
		cg3_type, softc->_w, softc->_h));

	retval = 1;
bad:
	if (!retval) {
		DEBUGF(2, ("cg3_probe failed\n"));
	}
	unmap_regs((addr_t) mfb, (u_int) NBPG);
	return (retval);
}

/* 66 Hz */
char cg3_mfbval0[] = {
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
/* 76 Hz */
char cg3_mfbval1[] = {
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
char cg3_mfbval2[] = {
	0x14, 0x70,
	0x15, 0x20,
	0x16, 0x08,
	0x17, 0x10,
	0x18, 0x06,
	0x19, 0x02,
	0x1a, 0x31,
	0x1b, 0x51,
	0x1c, 0x06,
	0x1d, 0x0c,
	0x1e, 0xff,
	0x1f, 0x01,
	0x10, 0x22,
	0
};
static char *mfbvals[] = {
	cg3_mfbval0,
	cg3_mfbval1,
	cg3_mfbval2,
};

static
cg3_init(mfb)
	struct mfb_reg *mfb;
{
	static char dacval[] = {
		4, 0xff,
		5, 0,
		6, 0x70,
		7, 0,
		0
	};

	register char *p;

	/* initialize video chip */
	for (p = mfbvals[cg3_type]; *p; p += 2)
		((char *) mfb)[p[0]] = p[1];

	/* initialize DAC */
	for (p = dacval; *p; p += 2) {
		mfb->cmap.addr = p[0];
		mfb->cmap.ctrl = p[1];
	}

}
#endif NOHWINIT
