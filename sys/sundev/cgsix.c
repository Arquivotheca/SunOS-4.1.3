#ifndef lint
static  char sccsid[] = "@(#)cgsix.c 1.1 92/07/30";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

/*
 * CG6 Graphics Accelerator
 */
#include "cgsix.h"
#include "win.h"
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/map.h>
#include <sys/dir.h>
#include <sys/user.h>
#include <sys/proc.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <sys/vmmac.h>
#include <sys/mman.h>
#include <sundev/mbvar.h>
#include <sun/fbio.h>
#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/enable.h>
#include <machine/eeprom.h>
#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
/* XXX hack in case cg6_putcolormap is declared */
#define	cg6_putcolormap	cg6_putcolormap_xxx
#include <pixrect/cg6var.h>
#undef cg6_putcolormap
#include <sundev/cg6reg.h>
#include <sundev/p4reg.h>
#include <machine/vm_hat.h>
#include <vm/as.h>
#include <vm/seg.h>

/* hack for 4.0.x compilation */
#ifndef KMEM_SLEEP
#define	KMEM_SLEEP	0
#define	new_kmem_zalloc(a,b)	kmem_zalloc(a)
#endif


/*
 * This flag indicates that we should use kcopy instead of copyin
 * for cursor ioctls.
 */
int fbio_kcopy;
extern int copyin(), kcopy();

/*
 * Direction flags for savefbc()
 */
#define	DIR_FROM_FBC	0
#define	DIR_TO_FBC	1

#ifdef sun3x
#define	PGT_CG6 0
#endif
#ifdef sun3
#define	PGT_CG6 PGT_OBMEM
#endif
#ifdef sun4
#define	PGT_CG6 PGT_OBIO
#endif


/*
 * Table of CG6 addresses
 */
struct mapping {
	u_int	vaddr;		/* virtual (mmap offset) address */
	u_int	paddr;		/* physical (byte) address */
	u_int	size;		/* size in bytes (page multiple) */
	u_char	prot;		/* superuser only ? */
} cg6_addrs[] = {
	{ CG6_VADDR_CMAP,	CG6_ADDR_CMAP,	 CG6_CMAP_SZ, 	0 },
	{ CG6_VADDR_FHCTHC,   CG6_ADDR_FHCTHC, CG6_FHCTHC_SZ,	0 },
	{ CG6_VADDR_FBCTEC,	CG6_ADDR_FBC,	 CG6_FBCTEC_SZ,	0 },
	{ CG6_VADDR_COLOR,    CG6_ADDR_COLOR,	 CG6_FB_SZ, 	0 },

	{ CG6_VADDR_ROM,      CG6_ADDR_ROM,	 CG6_ROM_SZ,    0 },

	/* CG4 color FB */
	{ 0,		    CG6_ADDR_COLOR,	 0, 		0 },
};

#define	CG6_ADDR(a) (PGT_CG6 | btop(softc->baseaddr + (a)))
/*
 * Indices into address table
 */
#define	MAP_CMAP	0
#define	MAP_FHCTHC	1
#define	MAP_FBCTEC	2
#define	MAP_COLOR	3
#define	MAP_ROM		4
#define	MAP_CG4COLOR	5

#define	MAP_NPROBE	4	/* actually maped in by cgsixprobe */
#define	MAP_NMAP	6	/* mmappable by user process */

/*
 * A blank page for mapping CG4 overlay/enable planes
 */
caddr_t	blankpage, getpages();

/*
 * driver per-unit data
 */
static struct cg6_softc {
	int		w, h;		/* resolution */
	struct proc	*owner;		/* owner of the frame buffer */
	struct fbgattr	gattr;		/* current attributes */
	u_long		*p4reg;		/* pointer to P4 register */
	u_long		baseaddr;	/* physical base address */
	u_int		size;		/* size of frame buffer */
	u_int		bw1size;	/* size of overlay/enable */
	caddr_t		va[MAP_NPROBE];	/* Mapped in devices */
	struct fbc	fbc;		/* saved fbc for PROM */
	struct thc 	*thc;

	struct softcur {
		short enable;		/* cursor enable */
		short pad1;
		struct fbcurpos pos;	/* cursor position */
		struct fbcurpos hot;	/* cursor hot spot */
		struct fbcurpos size;	/* cursor bitmap size */
		u_long image[32];	/* cursor image bitmap */
		u_long mask[32];	/* cursor mask bitmap */
	} cur;

	union {				/* shadow overlay color map */
		u_long	omap_long[1];	/* cheating here to save space */
		u_char	omap_char[3][2];
	} omap_image;
#define	omap_rgb	omap_image.omap_char[0]
	u_short		omap_update;	/* overlay colormap update flag */

	u_short		cmap_index;	/* colormap update index */
	u_short		cmap_count;	/* colormap update count */
	union {				/* shadow color map */
		u_long	cmap_long[CG6_CMAP_ENTRIES * 3 / sizeof (u_long)];
		u_char	cmap_char[3][CG6_CMAP_ENTRIES];
	} cmap_image;
#define	cmap_red	cmap_image.cmap_char[0]
#define	cmap_green	cmap_image.cmap_char[1]
#define	cmap_blue	cmap_image.cmap_char[2]
#define	cmap_rgb	cmap_image.cmap_char[0]

#if NWIN > 0
	Pixrect	   pr;
	struct mprp_data  prd;
#endif NWIN > 0
	struct seg      *curcntxt;      /* context switching */
} cg6_softc[NCGSIX];

/*
 * probe size -- Just P4 page
 */
#define	CG6_PROBESIZE	(NBPG)

/*
 * Mainbus device data
 */
int cgsixprobe(), cgsixattach(), cgsixpoll();
struct  mb_device *cgsixinfo[NCGSIX];
struct  mb_driver cgsixdriver = {
	cgsixprobe, 0, cgsixattach, 0, 0, cgsixpoll,
	CG6_PROBESIZE, "cgsix", cgsixinfo, 0, 0, 0, 0
};

/*
 * default structure for FBIOGTYPE ioctl
 */
static struct fbtype cg6typedefault =  {
	/* type		  h  w  depth cms size */
	FBTYPE_SUN4COLOR, 0, 0, 8,    256,   0
};

/*
 * default structure for FBIOGATTR ioctl
 */
static struct fbgattr cg6attrdefault =  {
	/* real_type	 owner */
	FBTYPE_SUNFAST_COLOR, 0,
	/* fbtype: type	h  w  depth cms  size */
	{ FBTYPE_SUNFAST_COLOR, 0, 0, 8,    256,  0 },
	/* fbsattr: flags       emu_type       dev_specific */
	{ 0,			FBTYPE_SUN4COLOR, { 0 } },
	/* emu_types */
	{ FBTYPE_SUNFAST_COLOR, FBTYPE_SUN4COLOR, -1, -1},
};


/*
 * handy macros
 */
#define getsoftc(unit)  (&cg6_softc[unit])

#define	BZERO(d,c)	bzero((caddr_t) (d), (u_int) (c))
#define	COPY(f,s,d,c)	(f((caddr_t) (s), (caddr_t) (d), (u_int) (c)))
#define	CG6BCOPY(s,d,c)	COPY(bcopy,(s),(d),(c))
#define	COPYIN(s,d,c)	COPY(copyin,(s),(d),(c))
#define	COPYOUT(s,d,c)	COPY(copyout,(s),(d),(c))

/*
 * enable/disable interrupt
 */
#define	TEC_EN_VBLANK_IRQ	0x20
#define	TEC_HCMISC_IRQBIT	0x10

/* position value to use to disable HW cursor */
#define	CG6_CURSOR_OFFPOS	(0xffe0ffe0)

#define	cg6_int_enable(softc)	\
	{  \
	softc->thc->l_thc_hcmisc =   \
	(softc->thc->l_thc_hcmisc & ~THC_HCMISC_RESET) |  TEC_EN_VBLANK_IRQ;  \
	setintrenable(1);  \
	}

#define	cg6_int_disable(softc)	\
	{  \
	int x; \
	setintrenable(0);  \
	x = spl4(); \
	softc->thc->l_thc_hcmisc =  \
	 (softc->thc->l_thc_hcmisc & ~(THC_HCMISC_RESET | TEC_EN_VBLANK_IRQ)) \
			 | TEC_HCMISC_IRQBIT;  \
	(void) splx(x); \
	}

/* check if color map update is pending */
#define	cgsix_update_pending(softc) \
	((softc)->cmap_count || (softc)->omap_update)

/* forward references */
static void cgsix_reset_cmap();
static void cgsix_update_cmap();
static void cgsix_cmap_bcopy();
static void cgsixintr();

#if NWIN > 0

/*
 * SunWindows specific stuff
 */

static cg6_mem_rop(), cg6_putcolormap();

/*
 * kernel pixrect ops vector
 */
struct pixrectops cg6_ops = {
	cg6_mem_rop,
	cg6_putcolormap,
	mem_putattributes,
};

/*
 * Mem_rop which waits for idle FBC
 * If we do not wait, it seems that
 * bus timeout errors occur.
 * The wait is limited to 50 millisecs.
 */

static cg6_rop_wait = 50;

static
cg6_mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh, op;
	Pixrect *spr;
	int sx, sy;
{
	Pixrect mpr;
	int unit;

	if (spr && spr->pr_ops == &cg6_ops) {
		unit = mpr_d(spr)->md_primary;
		mpr = *spr;
		mpr.pr_ops = &mem_ops;
		spr = &mpr;
	} else
		unit = mpr_d(dpr)->md_primary;

	if (cg6_rop_wait) {
		register u_int *statp =&((struct fbc *)
			cg6_softc[unit].va[MAP_FBCTEC])->l_fbc_status;
		CDELAY(!(*statp & L_FBC_BUSY), cg6_rop_wait << 10);
	}
	return (mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy));
}

static
cg6_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg6_softc *softc = &cg6_softc[(mpr_d(pr)->md_primary)];
	register u_int rindex = (u_int) index;
	register u_int rcount = (u_int) count;
	register u_char *map;
	register u_int entries;

	map = softc->cmap_rgb;
	entries = CG6_CMAP_ENTRIES;

	/* check arguments */
	if (rindex >= entries || rindex + rcount > entries)  {
		return (PIX_ERR);
	}

	if (rcount == 0)
		return (0);

	/* lock out updates of the hardware colormap */
	if (cgsix_update_pending(softc))
		cg6_int_disable(softc);

	map += rindex * 3;
	cgsix_cmap_bcopy(red,   map++, rcount);
	cgsix_cmap_bcopy(green, map++, rcount);
	cgsix_cmap_bcopy(blue,  map,   rcount);

	cgsix_update_cmap(softc, rindex, rcount);

	/* enable interrupt so we can load the hardware colormap */
	cg6_int_enable(softc);

	return (0);
}

#endif	NWIN > 0

/*
 * Determine if a cgsix exists at the given address.
 */
/*ARGSUSED*/
cgsixprobe(reg, unit)
	caddr_t reg;
	int unit;
{

	register struct cg6_softc *softc = &cg6_softc[unit];
	register struct mapping *mp;
	register caddr_t *va;
	register int i, s;
	int pages1, pages8, id;
	u_long kmx, rmalloc();
	long peekval;
	u_int hat_getkpfnum();

	/*
	 * Determine frame buffer resolution:
	 */
#ifdef sun3
	/* this is for SunOS 3.x which does not know hat_petkpfnum */
	/* we should simply use fbgetpage in this case */
	softc->baseaddr = 0xFF000000;
#else
	softc->baseaddr = ((u_long) hat_getkpfnum((addr_t) reg) << PAGESHIFT);
#endif
#if defined(SUN4_110)
	if (cpu == CPU_SUN4_110 && !(softc->baseaddr & 0xf0000000))
			softc->baseaddr |= 0xf0000000;
#endif
	fbmapin(reg, (int) CG6_ADDR(CG6_ADDR_P4REG), (int) NBPG);

	if (peekl((long *)reg, (long *)&id))
		return (0);
#define	CG6_ID_SHIFT 24
#define	CG6_RES_SHIFT 20
#define	CG6_RES_MASK 0x0f

	if (((id >> CG6_ID_SHIFT) & P4_ID_MASK) != P4_ID_FASTCOLOR)
		return (0);
	{
	static struct pr_size res[] = {
		{1152,  900},
		{2308, 1800},
		{1600, 1280},
		{1280, 1024},
		{1152, 900},
		{1152, 870},
		{1024, 768},
		{640,  480},
		};
	int resolution;
	resolution = (id >> CG6_ID_SHIFT) & CG6_RES_MASK;
	softc->w = res[resolution].x;
	softc->h = res[resolution].y;
	}

	pages1 = btoc(mpr_linebytes(softc->w, 1) * softc->h);
	pages8 = btoc(mpr_linebytes(softc->w, 8) * softc->h);

	/*
 	 * There is at least 1 MB VRAM on the CG4, so always map
 	 * in that much (given by cg6_addrs[MAP_COLOR].size)
 	 * If there is more then map that in as well .
 	 */
	i = ctob(pages8);
	if (i > cg6_addrs[MAP_COLOR].size) {
		cg6_addrs[MAP_COLOR].size = i;
	}

	/*
	 * Map in each section
	 */
	for (va = softc->va, mp = cg6_addrs, i = MAP_NPROBE; i; i--, mp++, va++) {
		/*
		 * allocate enough pages
		 */
		s = splhigh();
		kmx = rmalloc(kernelmap, (long) btoc(mp->size));
		(void)splx(s);
		if (kmx == 0) {
			va--, mp--, i++;
			goto finito;
		}

		/*
		 * get kernel virtual address
		 */
		*va = (caddr_t) kmxtob(kmx);

		/*
		 * Map in this section
		 */
		fbmapin(*va, (int) CG6_ADDR(mp->paddr), (int) mp->size);

		/*
		 * Check if this section is present. If
		 * not, map out all sections, and return an error.
		 */
		if (peekl((long *) *va, &peekval) == -1) {
			finito:
			do {
				fbmapout(*va, (int) mp->size);
				va--;
				mp--;

			} while (i++ < MAP_NPROBE);
			return (0);
		}
	}

	softc->size = ctob(pages1 + pages1 + pages8);
	softc->bw1size = ctob(pages1 + pages1);

	cg6_addrs[MAP_CG4COLOR].size = ctob(pages8);
	cg6_addrs[MAP_CG4COLOR].vaddr = softc->bw1size;

	/*
	 * probe succeeded
	 */
	return (CG6_PROBESIZE);
}


/*
 * Initialize Device
 *
 * XXX: values are hard coded & should
 * be generalized. screen resolution, in particular is
 * fixed for 1152 X 900
 */
cgsixattach(md)
register struct mb_device *md;
{
	register struct cg6_softc *softc = &cg6_softc[md->md_unit];

	softc->owner = NULL;

 	softc->gattr = cg6attrdefault;
	softc->gattr.fbtype.fb_height = softc->h;
	softc->gattr.fbtype.fb_width =  softc->w;
	softc->gattr.fbtype.fb_size = softc->size;

	/*
 	 * Initialize DAC
 	 */
	{
	register  struct cg6_cmap * cmap;

	cmap = (struct cg6_cmap*) (softc->va[MAP_CMAP]);
	cmap->addr = 0x06060606;
	cmap->ctrl = 0x43434343;
	cmap->cmap = 0x43434343;
	cmap->addr = 0x04040404;
	cmap->ctrl = ~0;
	cmap->addr = 0x05050505;
	cmap->ctrl = 0;
	}

 	/*
	 * Initialize THC
	 */
	{
	register struct thc *thc;

	thc = (struct thc*) (softc->va[MAP_FHCTHC] + CG6_THC_POFF);
	softc->thc = thc;

	thc->l_thc_hchs		= 0x010009;
	thc->l_thc_hchsdvs 	= 0x570000;
	thc->l_thc_hchd		= 0x15005d;
	thc->l_thc_hcvs		= 0x010005;
	thc->l_thc_hcvd 	= 0x2403a8;
	thc->l_thc_hcr		= 0x00016b;

	/* or the value of this reg with bits to set the interrupt enable */
	thc->l_thc_hcmisc	|= THC_HCMISC_CLEAR_VBLANK_IRQ;

	thc->l_thc_cursor 	= (2000 << 16) | 2000;
	}

	/*
	 * Initialize FHC for 1152 X 900 screen
	 * XXX: Check actual fb dimensions
	 */
	{
	register int i, *cfg;

	cfg = (int*) (softc->va[MAP_FHCTHC]);

	/*
	 * Check the rev number of the board.
	 * For rev 0, set TESTX/TESTY to and disable
	 * FROP
	 */
	i = *cfg;
	if ((i & 0xf00000) == 0)   /* disable frops, test window, dest cache */
		*cfg = 0x090b00;
	else
	  if ((i & 0xf00000) == 0x100000) /* disable dest cache for fbc1 */
		*cfg = 0x10bbb;
	  else
		*cfg = 0xbbb;      /* don't disable anything if fbc rev > 1 */

	}

	/*
	 * Initialize TEC
	 */
	{
	register struct tec *tec;

	tec = (struct tec*) (softc->va[MAP_FBCTEC] + CG6_TEC_POFF);
	tec->l_tec_mv	= 0;
	tec->l_tec_clip	= 0;
	tec->l_tec_vdc	= 0;
	}

	/*
	 * save fbc for PROM
	 */
	savefbc(&softc->fbc, (struct fbc*)(softc->va[MAP_FBCTEC]), DIR_FROM_FBC);

	cgsix_reset_cmap((caddr_t) softc->cmap_rgb, CG6_CMAP_ENTRIES);
	cgsix_reset_cmap((caddr_t) softc->omap_rgb, 2);
	cgsix_update_cmap(softc, (u_int) _ZERO_, CG6_CMAP_ENTRIES);
	cgsix_update_cmap(softc, (u_int) _ZERO_, (u_int) _ZERO_);
}

cgsixopen(dev, flag)
	dev_t dev;
	int flag;
{
	return (fbopen(dev, flag, NCGSIX, cgsixinfo));
}

/*ARGSUSED*/
cgsixclose(dev, flag)
	dev_t dev;
	int flag;
{
	register struct cg6_softc *softc = &cg6_softc[minor(dev)];

	softc->owner = NULL;

 	softc->gattr = cg6attrdefault;
	softc->gattr.fbtype.fb_height = softc->h;
	softc->gattr.fbtype.fb_width =  softc->w;
	softc->gattr.fbtype.fb_size = softc->size;

	softc->cur.enable = 0;
	cg6_setcurpos(softc);

	/*
	 * Restore fbc for PROM
	 */
	savefbc((struct fbc*)(softc->va[MAP_FBCTEC]), &softc->fbc, DIR_TO_FBC);
}

/*ARGSUSED*/
cgsixmmap(dev, off, prot)
	dev_t dev;
	register off_t off;
	int prot;
{
	register struct cg6_softc *softc = &cg6_softc[minor(dev)];
	register struct mapping *mp;
	register int i;

	/*
	 * See if the offset specifies a 'real' CG6 address
	 */
	for (mp = cg6_addrs, i = 0; i < MAP_NMAP; i++, mp++) {

		if (off >= (off_t) mp->vaddr &&
		    off < (off_t) (mp->vaddr + mp->size)) {

			/*
			 * See if this section may only be mapped by root.
			 */
			if (mp->prot && !suser()) {
				return (-1);
			      }

			return (CG6_ADDR(mp->paddr + (off - mp->vaddr)));
		      }
	      }

	/*
	 * Check if this is within the CG4 Overlay/Enable/Color
	 * Frame buffer regions. Map overlay & enable to a page in
	 * the kernel.
	 */
	if (off < (off_t) softc->bw1size) {
		if (blankpage == 0) {
			blankpage = getpages(1, KMEM_SLEEP);
		      }
		return (btop(blankpage));
	      }

	return (-1);

      }

/*ARGSUSED*/
cgsixioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register int unit = minor(dev);
	register struct cg6_softc *softc = &cg6_softc[unit];
	int cursor_cmap;
	int (*copyfun)();
	static union {
		u_char cmap[CG6_CMAP_ENTRIES];
		u_long cur[32];
	} iobuf;

	/* default to updating normal colormap */
	cursor_cmap = 0;

	/* figure out if kernel or user mode call */
	copyfun = fbio_kcopy ? kcopy : copyin;

	switch (cmd) {

	case FBIOPUTCMAP:
	case FBIOGETCMAP: {
		struct fbcmap *cmap;
		u_int index;
		u_int count;
		u_char *map;
		u_int entries;

cmap_ioctl:
		cmap = (struct fbcmap *) data;
		index = (u_int) cmap->index;
		count = (u_int) cmap->count;

		if (cursor_cmap == 0)
			switch (PIX_ATTRGROUP(index)) {
			case 0:
			case PIXPG_8BIT_COLOR:
				map = softc->cmap_rgb;
				entries = CG6_CMAP_ENTRIES;
				break;
			default:
				return EINVAL;
			}
		else {
			map = softc->omap_rgb;
			entries = 2;
		}

		if ((index &= PIX_ALL_PLANES) >= entries ||
			index + count > entries)
			return EINVAL;

		if (count == 0)
			return 0;

		if (cmd == FBIOPUTCMAP) {
			register int error;

			if (cgsix_update_pending(softc))
				cg6_int_disable(softc);

			map += index * 3;

			/* sorry */
			do {
				if (error = COPY(copyfun, cmap->red,
					iobuf.cmap, count))
					break;
				cgsix_cmap_bcopy(iobuf.cmap, map++, count);

				if (error = COPY(copyfun, cmap->green,
 					iobuf.cmap, count))
					break;
				cgsix_cmap_bcopy(iobuf.cmap, map++, count);

				if (error = COPY(copyfun, cmap->blue,
 					iobuf.cmap, count))
					break;
				cgsix_cmap_bcopy(iobuf.cmap, map, count);
			} while (_ZERO_);

			if (error) {
				if (cgsix_update_pending(softc))
					cg6_int_enable(softc);
				return EFAULT;
			}

			/* cursor colormap update */
			if (entries < CG6_CMAP_ENTRIES)
				count = 0;

			cgsix_update_cmap(softc, index, count);
			cg6_int_enable(softc);
		}
		else {
			/* FBIOGETCMAP */
			map += index * 3;
			cgsix_cmap_bcopy(iobuf.cmap, map++, -count);
			if (COPYOUT(iobuf.cmap, cmap->red, count))
				return EFAULT;

			cgsix_cmap_bcopy(iobuf.cmap, map++, -count);
			if (COPYOUT(iobuf.cmap, cmap->green, count))
				return EFAULT;

			cgsix_cmap_bcopy(iobuf.cmap, map, -count);
			if (COPYOUT(iobuf.cmap, cmap->blue, count))
				return EFAULT;
		}
	}
	break;

	case FBIOSATTR: {
		register struct fbsattr *sattr = (struct fbsattr *) data;

#ifdef ONLY_OWNER_CAN_SATTR
		/* this can only happen for the owner */
		if (softc->owner != u.u_procp)
			return (ENOTTY);
#endif ONLY_OWNER_CAN_SATTR

		softc->gattr.sattr.flags = sattr->flags;

		if (sattr->emu_type != -1)
			softc->gattr.sattr.emu_type = sattr->emu_type;

		if (sattr->flags & FB_ATTR_DEVSPECIFIC) {
			bcopy((char *) sattr->dev_specific,
				(char *) softc->gattr.sattr.dev_specific,
				sizeof (sattr->dev_specific));

			if (softc->gattr.sattr.dev_specific
				[FB_ATTR_CG6_SETOWNER_CMD] == 1) {
				struct proc *pfind();
				register struct proc *newowner = 0;

				if (softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_PID] > 0 &&
					(newowner = pfind(
					softc->gattr.sattr.dev_specific
						[FB_ATTR_CG6_SETOWNER_PID]))) {
					softc->owner = newowner;
					softc->gattr.owner = newowner->p_pid;
				}

				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_CMD] = 0;
				softc->gattr.sattr.dev_specific
					[FB_ATTR_CG6_SETOWNER_PID] = 0;

				if (!newowner)
					return (ESRCH);
			}
		}
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *gattr = (struct fbgattr *) data;

		/* set owner if not owned or previous owner is dead */
		if (softc->owner == NULL ||
			softc->owner->p_stat == NULL ||
			softc->owner->p_pid != softc->gattr.owner) {

			softc->owner = u.u_procp;
			softc->gattr.owner = u.u_procp->p_pid;
		}

		*gattr = softc->gattr;

		if (u.u_procp == softc->owner) {
			gattr->owner = 0;
		}
	}
	break;

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		/* set owner if not owned or previous owner is dead */
		if (softc->owner == NULL ||
		    softc->owner->p_stat == NULL ||
		    softc->owner->p_pid != softc->gattr.owner) {

			softc->owner = u.u_procp;
			softc->gattr.owner = u.u_procp->p_pid;
		}

		switch (softc->gattr.sattr.emu_type) {
		case FBTYPE_SUN4COLOR:
			*fb = cg6typedefault;
			fb->fb_height = softc->h;
			fb->fb_width  = softc->w;
			fb->fb_size = softc->size;
			break;

		case FBTYPE_SUNFAST_COLOR:
		default:
			*fb = softc->gattr.fbtype;
			break;
		}
	}
	break;

#if NWIN > 0

	case FBIOGPIXRECT: {
 		((struct fbpixrect *) data)->fbpr_pixrect = &softc->pr;

		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* initialize pixrect */
		softc->pr.pr_ops = &cg6_ops;
		softc->pr.pr_size.x = softc->w;
		softc->pr.pr_size.y = softc->h;
		softc->pr.pr_depth = CG6_DEPTH;

		/* initialize private data */

		softc->prd.mpr.md_linebytes =
			mpr_linebytes(softc->w, CG6_DEPTH);
		softc->prd.mpr.md_image = (short *) (softc->va[MAP_COLOR]);
		softc->prd.mpr.md_offset.x = 0;
		softc->prd.mpr.md_offset.y = 0;
		softc->prd.mpr.md_primary = 0;
		softc->prd.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
		softc->prd.planes = 255;

		/* enable video */
		setvideoenable(1); /* does this do anything for cg6 ? */

		cg6_videnable(softc, 1);
	}
	break;

#endif NWIN > 0

	/* set video flags */
	case FBIOSVIDEO: {
		register int *video = (int *) data;

		/* does this do anything ? */
		setvideoenable((u_int) (*video & FBVIDEO_ON));

		cg6_videnable(softc, (u_int) (*video & FBVIDEO_ON));
	}
	break;

	/* get video flags */
	case FBIOGVIDEO: {
 		register struct thc *thc;
		thc = (struct thc*) (softc->va[MAP_FHCTHC] + CG6_THC_POFF);
		* (int *) data = (thc->l_thc_hcmisc & 0x400) ?
				 FBVIDEO_ON : FBVIDEO_OFF;
	}
	break;

	case GRABPAGEALLOC: 	/* Window Grabber */
		return (segcg6_graballoc(data));
	case GRABATTACH:	/* Window Grabber */
		return (segcg6_grabattach(data));
	case GRABPAGEFREE:	/* Window Grabber */
		return (segcg6_grabfree(data));
        case GRABLOCKINFO:      /* Window Grabber */
                return (segcg6_grabinfo(data));

	/* HW cursor control */
	case FBIOSCURSOR: {
		struct fbcursor *cp = (struct fbcursor *) data;
		int set = cp->set;
		int cbytes;

		if (set & FB_CUR_SETCUR)
			softc->cur.enable = cp->enable;

		if (set & FB_CUR_SETPOS)
			softc->cur.pos = cp->pos;

		if (set & FB_CUR_SETHOT)
			softc->cur.hot = cp->hot;

		/* update hardware */
		cg6_setcurpos(softc);

		if (set & FB_CUR_SETSHAPE) {
			if ((u_int) cp->size.x > 32 ||
				(u_int) cp->size.y > 32)
				return EINVAL;
			softc->cur.size = cp->size;

			/* compute cursor bitmap bytes */
			cbytes = softc->cur.size.y *
				sizeof softc->cur.image[0];

			/* copy cursor image into softc */
			if (COPY(copyfun, cp->image, iobuf.cur, cbytes))
				return EFAULT;
			BZERO(softc->cur.image, sizeof softc->cur.image);
			CG6BCOPY(iobuf.cur, softc->cur.image, cbytes);
			if (COPY(copyfun, cp->mask, iobuf.cur, cbytes))
				return EFAULT;
			BZERO(softc->cur.mask, sizeof softc->cur.mask);
			CG6BCOPY(iobuf.cur, softc->cur.mask, cbytes);

			/* load into hardware */
			cg6_setcurshape(softc);
		}

		/* load colormap */
		if (set & FB_CUR_SETCMAP) {
			cursor_cmap = 1;
			cmd = FBIOPUTCMAP;
			data = (caddr_t) &cp->cmap;
			goto cmap_ioctl;
		}

		break;
	}

	case FBIOGCURSOR: {
		struct fbcursor *cp = (struct fbcursor *) data;
		int cbytes;

		cp->set = 0;
		cp->enable = softc->cur.enable;
		cp->pos = softc->cur.pos;
		cp->hot = softc->cur.hot;
		cp->size = softc->cur.size;

		/* compute cursor bitmap bytes */
		cbytes = softc->cur.size.y * sizeof softc->cur.image[0];

		/* if image pointer is non-null copy both bitmaps */
		if (cp->image &&
			(COPYOUT(softc->cur.image, cp->image, cbytes) ||
			COPYOUT(softc->cur.mask, cp->mask, cbytes)))
			return EFAULT;

		/* if red pointer is non-null copy colormap */
		if (cp->cmap.red) {
			cursor_cmap = 1;
			cmd = FBIOGETCMAP;
			data = (caddr_t) &cp->cmap;
			goto cmap_ioctl;
		}
		/* just trying to find out colormap size */
		else {
			cp->cmap.index = 0;
			cp->cmap.count = 2;
		}
		break;
	}

	case FBIOSCURPOS:
		softc->cur.pos = * (struct fbcurpos *) data;
		cg6_setcurpos(softc);
		break;

	case FBIOGCURPOS:
		* (struct fbcurpos *) data = softc->cur.pos;
		break;

	case FBIOGCURMAX: {
		static struct fbcurpos curmax = { 32, 32 };

		* (struct fbcurpos *) data = curmax;
		break;
	}

	default:
		return (ENOTTY);

	} /* switch (cmd) */

	return (0);
}

cgsixpoll()
{
	register int serviced = 0;
	register struct cg6_softc *softc;

	/*
	 * If we were expecting an interrupt assume it is ours.
	 */
	for (softc = cg6_softc; softc < &cg6_softc[NCGSIX]; softc++) {
		if (cgsix_update_pending(softc)) {
#ifdef LATER
			if (!softc->p4reg || *softc->p4reg & P4_REG_INT) {
				cgsixintr(softc);
				serviced++;
			}
#else
			cgsixintr(softc);
			serviced++;
#endif
		}
	}


	return (serviced);
}

static void
cgsixintr(softc)
	register struct cg6_softc *softc;
{
	struct cg6_cmap *cmap = (struct cg6_cmap*)(softc->va[MAP_CMAP]);
	LOOP_T count = softc->cmap_count;
	u_long *in;
	u_long *out;
	u_long tmp;

	/* load cursor color map */
	if (softc->omap_update) {
		in = &softc->omap_image.omap_long[0];
		out = (u_long *) &cmap->omap;

		/* background color */
		cmap->addr = 1 << 24;
		tmp = in[0];
		*out = tmp;
		*out = tmp <<= 8;
		*out = tmp <<= 8;

		/* foreground color */
		cmap->addr = 3 << 24;
		*out = tmp <<= 8;
		tmp = in[1];
		*out = tmp;
		*out = tmp <<= 8;
	}

	/* load main color map */
	if (count) {
		LOOP_T index = softc->cmap_index;

		in = &softc->cmap_image.cmap_long[0];
		out = (u_long *) &cmap->cmap;

		/* count multiples of 4 RGB entries */
		count = (count + (index & 3) + 3) >> 2;

		/* round index to 4 entry boundary */
		index &= ~3;

		cmap->addr = index << 24;
		PTR_INCR(u_long *, in, index * 3);

		/* copy 4 bytes (4/3 RGB entries) per loop iteration */
		count *= 3;
		PR_LOOPV(count,
			tmp = *in++;
			*out = tmp;
			*out = tmp <<= 8;
			*out = tmp <<= 8;
			*out = tmp <<= 8);

		softc->cmap_count = 0;
	}

	softc->omap_update = 0;
	cg6_int_disable(softc);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
cgsix_reset_cmap(cmap, entries)
register caddr_t cmap;
register u_int entries;
{
	bzero(cmap, entries * 3);
	*cmap++ = 255;
	*cmap++ = 255;
	*cmap   = 255;
}

/*
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 * Zero count argument indicates cursor color map update desired.
 */
static void
cgsix_update_cmap(softc, index, count)
	struct cg6_softc *softc;
	u_int index, count;
{
	u_int high, low;

	if (count == 0) {
		softc->omap_update = 1;
		return;
	}

	if (high = softc->cmap_count) {
		high += (low = softc->cmap_index);

		if (index < low)
			softc->cmap_index = low = index;

		if (index + count > high)
			high = index + count;

		softc->cmap_count = high - low;
	}
	else {
		softc->cmap_index = index;
		softc->cmap_count = count;
	}
}

/*
 * Copy colormap entries between red, green, or blue array and
 * interspersed rgb array.
 *
 * count > 0 : copy count bytes from buf to rgb
 * count < 0 : copy -count bytes from rgb to buf
 */
static void
cgsix_cmap_bcopy(bufp, rgb, count)
register u_char *bufp, *rgb;
u_int count;
{
	register int rcount = count;

	if (--rcount >= 0)
		do {
			*rgb = *bufp++;
			rgb += 3;
		} while (--rcount >= 0);
	else {
		rcount = -rcount - 2;
		do {
			*bufp++ = *rgb;
			rgb += 3;
		} while (--rcount >= 0);
	}
}

/*
 * enable/disable/update HW cursor
 */
static
cg6_setcurpos(softc)
	struct cg6_softc *softc;
{
	softc->thc->l_thc_cursor =
		softc->cur.enable ?
		((softc->cur.pos.x - softc->cur.hot.x) << 16) |
			((softc->cur.pos.y - softc->cur.hot.y) & 0xffff) :
		CG6_CURSOR_OFFPOS;
}

/*
 * load HW cursor bitmaps
 */
static
cg6_setcurshape(softc)
	struct cg6_softc *softc;
{
	u_long tmp, edge = 0;
	u_long *image, *mask, *hw;
	int i;

	/* compute right edge mask */
	if (softc->cur.size.x)
		edge = (u_long) ~0 << (32 - softc->cur.size.x);

	image = softc->cur.image;
	mask = softc->cur.mask;
	hw = (u_long *) &softc->thc->l_thc_cursora00;

	for (i = 0; i < 32; i++) {
		hw[i] = (tmp = mask[i] & edge);
		hw[i + 32] = tmp & image[i];
	}
}

/*
 * Save fbc pointed to by fbc into fbc0
 * Have to do individual fields because
 * some registers can not be read (eg. read activated).
 */
savefbc(fbc0, fbc, dir)
	struct fbc *fbc0, *fbc;
{
	if (fbc == 0 || fbc0 == 0) {
		return;
	}

	/*
	 * Wait until the cg6 is idle.
	 * If not, may get a bus timeout error
	 */
	if (dir == DIR_FROM_FBC) {
	       while (fbc->l_fbc_status & L_FBC_BUSY) ;
	} else {
	       while (fbc0->l_fbc_status & L_FBC_BUSY) ;
	}

	fbc0->l_fbc_misc = fbc->l_fbc_misc;
	fbc0->l_fbc_rasteroffx = fbc->l_fbc_rasteroffx;
	fbc0->l_fbc_rasteroffy = fbc->l_fbc_rasteroffy;
	fbc0->l_fbc_rasterop = fbc->l_fbc_rasterop;
	fbc0->l_fbc_pixelmask = fbc->l_fbc_pixelmask;
	fbc0->l_fbc_planemask = fbc->l_fbc_planemask;
	fbc0->l_fbc_fcolor = fbc->l_fbc_fcolor;
	fbc0->l_fbc_bcolor = fbc->l_fbc_bcolor;
	fbc0->l_fbc_clipminx = fbc->l_fbc_clipminx;
	fbc0->l_fbc_clipminy = fbc->l_fbc_clipminy;
	fbc0->l_fbc_clipmaxx = fbc->l_fbc_clipmaxx;
	fbc0->l_fbc_clipmaxy = fbc->l_fbc_clipmaxy;
	fbc0->l_fbc_autoincx = fbc->l_fbc_autoincx;
	fbc0->l_fbc_autoincy = fbc->l_fbc_autoincy;
	fbc0->l_fbc_clipcheck = fbc->l_fbc_clipcheck;
}

/*
 * Enable cg6 video if on non-zero,
 * else disable.
 */
cg6_videnable(softc, on)
	struct cg6_softc *softc;
	u_int on;
{
	register struct thc *thc;

	thc = (struct thc*) (softc->va[MAP_FHCTHC] + CG6_THC_POFF);

	if (on)
		thc->l_thc_hcmisc 	= 0x00048f;
	else
		thc->l_thc_hcmisc 	= 0x00008f;
}

/* SEGMENT DRIVER CODE starts here $: */

/*
 * from here on down, is the lego segment driver.  this virtualizes the
 * lego register file by associating a register save area with each
 * mapping of the lego device (each lego segment).  only one of these
 * mappings is valid at any time; a page fault on one of the invalid
 * mappings saves off the current lego context, invalidates the current
 * valid mapping, restores the former register contents appropriate to
 * the faulting mapping, and then validates it.
 *
 * this implements a graphical context switch that is transparent to the user.
 *
 * the TEC and FBC contain the interesting context registers.
 */

/*
 * many registers in the tec and fbc do
 * not need to be saved/restored.
 */
struct cg6_cntxt {
	struct {
		u_int	mv;
		u_int	clip;
		u_int	vdc;
		u_int	data[64][2];
	} tec;
	struct {
		u_int	status;
		u_int	clipcheck;
		struct	l_fbc_misc	misc;
		u_int	x0, y0, x1, y1, x2, y2, x3, y3;
		u_int	rasteroffx, rasteroffy;
		u_int	autoincx, autoincy;
		u_int	clipminx, clipminy, clipmaxx, clipmaxy;
		u_int	fcolor, bcolor;
		struct	l_fbc_rasterop	rasterop;
		u_int	planemask, pixelmask;
		union	l_fbc_pattalign	 pattalign;
		u_int	pattern0, pattern1, pattern2, pattern3,
			pattern4, pattern5, pattern6, pattern7;
	} fbc;
};

struct segcg6_data {
	int			flag;
	struct cg6_cntxt	*cntxtp; 	/* pointer to context */
	struct	segcg6_data	*link;		/* shared_list */
	dev_t			dev;
	u_int			offset;
};

struct  segcg6_data     *shared_list;
struct  cg6_cntxt       shared_context;

#define	segcg6_crargs segcg6_data

#define	SEG_CTXINIT	(1)	/* not initted */
#define	SEG_SHARED	(2)	/* shared context */
#define	SEG_LOCK	(4)	/* lock page */

/*
 *      Window Grabber lock management
 */

#define	NLOCKS		16
#define	LOCKTIME	5	/* seconds */
static	int	locktime = LOCKTIME;

/* flag bits */
#define LOCKMAP         0x01
#define UNLOCKMAP       0x02
#define ATTACH          0x04
#define TRASHPAGE       0x08
#define UNGRAB          0x10    /* ungrab called */

struct  segproc {
	int	     flag;
	struct  proc    *procp;
	caddr_t	 lockaddr;
	struct  seg     *locksegp;
	caddr_t	 unlockaddr;
	struct  seg     *unlocksegp;
};

struct  seglock {
	short		sleepf;	/* sleep flag */
	short		allocf;	/* allocated flag */
	off_t		offset;	/* offset into device */
	int		*page;	/* virtual address of lock page */
	struct  segproc cr;     /* creator */
	struct  segproc cl;     /* client */
	struct  segproc *owner;	/* current owner of lock */
	struct  segproc *other;	/* not owner of lock */
};

caddr_t trashpage;	      /* for trashing unwanted writes */

static  struct  seglock lock_list[NLOCKS];
struct  seglock *segcg6_findlock();

/*
 * This routine is called through the cdevsw[] table to handle
 * the creation of lego (cg6) segments.
 */
/*ARGSUSED*/
int
cgsixsegmap(dev, offset, as, addr, len, prot, maxprot, flags, cred)
	dev_t dev;
	register u_int offset;
	struct as *as;
	addr_t *addr;
	register u_int len;
	u_int prot, maxprot;
	u_int flags;
	struct ucred *cred;
{
	struct segcg6_crargs dev_a;
	int	segcg6_create();
	register	i;
	caddr_t		seglock;
	int		ctxflag = 0;

	enum	as_res	as_unmap();
	int	as_map();

	/* register struct cg6_softc *softc = cg6_softc; */

	/*
	 * we now support MAP_SHARED and MAP_PRIVATE:
	 *
	 *	MAP_SHARED means you get the shared context
	 *	which is the traditional mapping method.
	 *
	 *	MAP_PRIVATE means you get your very own
	 *	LEGO context.
	 *
	 *	Note that you can't get to here without
	 *	asking for one or the other, but not both.
	 */

	/*
	 * Check to see if this is an allocated lock page.
	 */

	if ((seglock = (caddr_t)segcg6_findlock((off_t)offset)) != (caddr_t)NULL) {
		if (len != PAGESIZE)
			return (EINVAL);
	} else {
		/*
		 * Check to insure that the entire range is
		 * legal and we are not trying to map in
		 * more than the device will let us.
		 */
		for (i = 0; i < len; i += PAGESIZE) {
			if (cgsixmmap(dev, (off_t)offset + i, (int)maxprot) == -1)
				return (ENXIO);
			if ((offset+i >= (off_t)cg6_addrs[MAP_FBCTEC].vaddr) &&
			    (offset+i < (off_t)cg6_addrs[MAP_FBCTEC].vaddr+
					cg6_addrs[MAP_FBCTEC].size))
				ctxflag++;
		}
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints.
		 */
		map_addr(addr, len, (off_t)0, 0);
		if (*addr == NULL)
			return (ENOMEM);
	} else {
		/*
		 * User specified address -
		 * Blow away any previous mappings.
		 */
		 i = (int)as_unmap((struct as *)as, *addr, len);
	}

	/*
	 * record device number; mapping function doesn't do anything smart
	 * with it, but it takes it as an argument.  the offset is needed
	 * for mapping objects beginning some ways into them.
	 */

	dev_a.dev = dev;
	dev_a.offset = offset;

	/*
	 * determine whether this is a shared mapping, or a private
 	 * one. if shared, link it onto the shared list, if private,
	 * create a private LEGO context.
	 */
	if (seglock) {
		dev_a.link = NULL;
		dev_a.flag = SEG_LOCK;
		dev_a.cntxtp = NULL;
	} else
	if (flags & MAP_SHARED) { /* shared mapping */
		struct segcg6_crargs *a;

		/* link it onto the shared list */
		a = shared_list;
		shared_list = &dev_a;
		dev_a.link = a;
		dev_a.flag = SEG_SHARED;
		dev_a.cntxtp = (ctxflag) ? &shared_context : NULL;

	} else {		/* private mapping */
		dev_a.link = NULL;
		dev_a.flag = 0;
		dev_a.cntxtp =
		    (ctxflag) ?
			(struct cg6_cntxt *)
			new_kmem_zalloc(sizeof (struct cg6_cntxt), KMEM_SLEEP) :
			NULL;
	}

    return (as_map((struct as *)as, *addr, len, segcg6_create, (caddr_t)&dev_a));
}

static	int segcg6_dup(/* seg, newsegp */);
static	int segcg6_unmap(/* seg, addr, len */);
static	int segcg6_free(/* seg */);
static	faultcode_t segcg6_fault(/* seg, addr, len, type, rw */);
static	int segcg6_checkprot(/* seg, addr, size, prot */);
static	int segcg6_badop();
static  int segcg6_incore();

struct	seg_ops segcg6_ops =  {
	segcg6_dup,	/* dup */
	segcg6_unmap,	/* unmap */
	segcg6_free,	/* free */
	segcg6_fault,	/* fault */
	segcg6_badop,	/* faulta */
	segcg6_badop,	/* unload */
	segcg6_badop,	/* setprot */
	segcg6_checkprot, /* checkprot */
	segcg6_badop,	/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segcg6_badop,	/* sync */
	segcg6_incore	/* incore */
};

/*
 * Window Grabber Page Management
 */
#define	LOCK_OFFBASE	(CG6_VADDR_COLOR+CG6_FB_SZ)

/*
 * search the lock_list list for the specified offset
 *
 */

/*ARGSUSED*/
struct seglock *
segcg6_findlock(off)
off_t   off;
{
	register int i;

	if ((unsigned long)off < LOCK_OFFBASE)
		return ((struct seglock *)NULL);
	for (i = 0; i < NLOCKS; i++)
	    if  (off == lock_list[i].offset)
		return (lock_list + i);
	return ((struct seglock *)NULL);
}

/*ARGSUSED*/
static int
segcg6_grabattach(cookiep)      /* IOCTL */
caddr_t cookiep;
{
    return (EINVAL);
}

/*ARGSUSED*/
static int
segcg6_grabinfo(pp)    /* IOCTL */
caddr_t pp;
{
        *(int *)pp = 0;
        return (0);
}

/*ARGSUSED*/
static int
segcg6_graballoc(pp)    /* IOCTL */
caddr_t pp;
{
	register	int i;
	register	struct seglock	*lp = lock_list;

	if (trashpage == NULL) {
		trashpage = getpages(1, KMEM_SLEEP);
		for (i = 0; i < NLOCKS; i++)
			lock_list[i].offset = (off_t)(LOCK_OFFBASE+PAGESIZE*i);
	}

	for (i = 0; i < NLOCKS; i++, lp++)
	    if (lp->allocf == 0) {
		lp->allocf = 1;
		if (lp->page == (int *)NULL)
		    lp->page = (int *)getpages(1, KMEM_SLEEP);
		lp->cr.flag = 0;
		lp->cl.flag = 0;
		lp->cr.procp = u.u_procp;
		lp->cl.procp = NULL;
		lp->cr.locksegp = NULL;
		lp->cl.locksegp = NULL;
		lp->cr.unlocksegp = NULL;
		lp->cl.unlocksegp = NULL;
		lp->owner = NULL;
		lp->other = NULL;
		lp->sleepf = 0;
		*lp->page = 0;
		*(int *)pp = (int)lp->offset;
		return (0);
	    }
	return (ENOMEM);
}

/*ARGSUSED*/
static int
segcg6_grabfree(pp)     /* IOCTL */
caddr_t pp;
{
    register    struct seglock  *lp;
    int        segcg6_timeout();

    if ((lp = segcg6_findlock((off_t)*(int *)pp)) == NULL)
        return(EINVAL);
    lp->cr.flag |= UNGRAB;
    lp->allocf = 0;
    if (lp->sleepf) {   /* client will acquire the lock in this case */
        untimeout(segcg6_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
    return(0);
}    

/*
 * Create a device segment.  the lego context is initialized by the create args.
 */
static
segcg6_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segcg6_data *dp;

	dp = (struct segcg6_data *)
		new_kmem_zalloc(sizeof (struct segcg6_data), KMEM_SLEEP);
	*dp = *((struct segcg6_crargs *)argsp);

	seg->s_ops = &segcg6_ops;
	seg->s_data = (char *)dp;

	return (0);
}

/*
 * Duplicate seg and return new segment in newsegp. copy original lego context.
 */
static
segcg6_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
	register struct segcg6_data *ndp;

	(void) segcg6_create(newseg, (caddr_t)sdp);
	ndp = (struct segcg6_data *)newseg->s_data;

	if (sdp->flag & SEG_SHARED) {
		struct segcg6_crargs *a;

		a = shared_list;
		shared_list = ndp;
		ndp->link = a;
	} else if ((sdp->flag & SEG_LOCK) == 0) {
		ndp->cntxtp =
			(struct cg6_cntxt *)
			new_kmem_zalloc(sizeof (struct cg6_cntxt), KMEM_SLEEP);
		*ndp->cntxtp = *sdp->cntxtp;
	}
	return (0);
}

static
segcg6_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
        register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
        register struct seg *nseg;
        addr_t nbase;
        u_int nsize;
	void	hat_unload();
	void	hat_newseg();
        static  segcg6_lockfree();

        segcg6_lockfree(seg);

	/*
 	 * Check for bad sizes
 	 */
 	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
 	    (len & PAGEOFFSET) || ((u_int)addr & PAGEOFFSET))
 		panic("segdev_unmap");

        /*
         * Unload any hardware translations in the range to be taken out.
         */

        hat_unload(seg,addr,len);

 	/*
	 * Check for entire segment
	 */

 	if (addr == seg->s_base && len == seg->s_size) {
 		seg_free(seg);
 		return (0);
	}
 	/*
 	 * Check for beginning of segment
 	 */
 
 	if (addr == seg->s_base) {
 		seg->s_base += len;
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * Check for end of segment
 	 */
 	if (addr + len == seg->s_base + seg->s_size) {
 		seg->s_size -= len;
 		return (0);
 	}
 
 	/*
 	 * The section to go is in the middle of the segment,
 	 * have to make it into two segments.  nseg is made for
 	 * the high end while seg is cut down at the low end.
 	 */
 	nbase = addr + len;				/* new seg base */
 	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
 	seg->s_size = addr - seg->s_base;		/* shrink old seg */
 	nseg = seg_alloc(seg->s_as, nbase, nsize);
 	if (nseg == NULL)
 		panic("segcg6_unmap seg_alloc");
 
 	nseg->s_ops = seg->s_ops;
 
 	/* figure out what to do about the new context */
 
 	nseg->s_data = (caddr_t)new_kmem_alloc(sizeof (struct segcg6_data),KMEM_SLEEP);
 
         if (sdp->flag & SEG_SHARED) {
                 struct segcg6_crargs *a;
  
                 a = shared_list;
                 shared_list = (struct segcg6_data *)nseg->s_data;
                 ((struct segcg6_data *)nseg->s_data)->link = a;
         } else {
                 if (sdp->cntxtp == NULL)
 			((struct segcg6_data *)nseg->s_data)->cntxtp = NULL;
                 else {
                         ((struct segcg6_data *)nseg->s_data)->cntxtp =
                                 (struct cg6_cntxt *)
                                 new_kmem_zalloc(sizeof (struct cg6_cntxt),KMEM_SLEEP);
                         *((struct segcg6_data *)nseg->s_data)->cntxtp =
 				 *sdp->cntxtp;
                 }
         }
 
 	/*
 	 * Now we do something so that all the translations which used
 	 * to be associated with seg but are now associated with nseg.
 	 */
 	hat_newseg(seg, nseg->s_base, nseg->s_size, nseg);
	return(0);
}

/*
 * free a lego segment and clean up its context.
 */
static
int
segcg6_free(seg)
	struct seg *seg;
{
	register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
        register struct cg6_softc *softc = getsoftc(minor(sdp->dev));

	if (seg == softc->curcntxt) {
		softc->curcntxt = NULL;
	}
	if (sdp->flag & SEG_LOCK)
	    segcg6_lockfree(seg);
	if ((sdp->flag & SEG_SHARED) == 0 && (sdp->cntxtp != NULL))
		kmem_free((char *)sdp->cntxtp, sizeof (struct cg6_cntxt));
	kmem_free((char *)sdp, sizeof (*sdp));
	return(0);
}


#define	CG6MAP(sp, addr, page)	    \
	hat_devload(sp,		 \
		    addr,	       \
		    fbgetpage((caddr_t)page),    \
		    PROT_READ|PROT_WRITE|PROT_USER, 0)

#define	CG6UNMAP(segp)	  \
	hat_unload(segp,		\
		   (segp)->s_base,      \
		   (segp)->s_size);

/*ARGSUSED*/
static
segcg6_lockfree(seg)
struct seg *seg;
{
    register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
    struct      seglock *lp;
 
    if ((lp = segcg6_findlock((off_t)sdp->offset))==NULL)
        return;
    lp->cr.flag |= UNGRAB;
    if (lp->sleepf) {
        untimeout(segcg6_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
}

static
int
segcg6_timeout(lp)
struct	seglock *lp;
{
    struct  segproc	*np;
    void        hat_devload();

    np = &lp->cr;
    if (np->flag & LOCKMAP) {
	CG6UNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->flag & UNLOCKMAP)
	CG6UNMAP(np->unlocksegp);
    CG6MAP(np->unlocksegp, np->unlockaddr, lp->page);
    np->flag |= UNLOCKMAP;
    np->flag &= ~TRASHPAGE;

    np = &lp->cl;
    if (np->locksegp && np->flag & LOCKMAP) {
	CG6UNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->unlocksegp) {
	if (np->flag & UNLOCKMAP)
	    CG6UNMAP(np->unlocksegp);
	CG6MAP(np->unlocksegp, np->unlockaddr, trashpage);
	np->flag |= (UNLOCKMAP | TRASHPAGE);
    }
    wakeup((caddr_t)lp->page);
    return(0);
}

/*
 * Handle lock segment faults here...
 *
 */

static
int
segcg6_lockfault(seg, addr)
register struct seg *seg;
addr_t  addr;
{
	register struct seglock *lp;
	register struct segcg6_data *current
		= (struct segcg6_data *)seg->s_data;
	struct  segproc *sp;
	void	hat_devload();
	void	hat_unload();
	extern	int	hz;
	int     s;

	/* look up the segment in the lock_list */

	lp = segcg6_findlock((off_t)current->offset);
	if (lp == NULL) 
	    return (FC_MAKE_ERR(EFAULT));

        s = splsoftclock();

        if (lp->cr.flag & UNGRAB) {
            CG6MAP(seg,addr,trashpage);
            (void)splx(s);
            return(0);
        }    

	/* initialization necessary? */

	if (lp->cr.procp == u.u_procp && lp->cr.locksegp == NULL) {
	    lp->cr.locksegp = seg;
	    lp->cr.lockaddr = addr;
	} else if (lp->cr.procp != u.u_procp && lp->cl.locksegp == NULL) {
	    lp->cl.procp = u.u_procp;
	    lp->cl.locksegp = seg;
	    lp->cl.lockaddr = addr;
	} else if (lp->cr.procp == u.u_procp && lp->cr.locksegp != seg) {
	    lp->cr.unlocksegp = seg;
	    lp->cr.unlockaddr = addr;
	} else if (lp->cl.procp == u.u_procp && lp->cl.locksegp != seg) {
	    lp->cl.unlocksegp = seg;
	    lp->cl.unlockaddr = addr;
	}

	if (*(int *)lp->page == 0) {
	    if (!lp->sleepf) {
		if (lp->owner == NULL) {
		    if (lp->cr.procp == u.u_procp) {
			lp->owner = &lp->cr;
			lp->other = &lp->cl;
		    } else {
			lp->owner = &lp->cl;
			lp->other = &lp->cr;
		    }
		}
		/* switch ownership? */
		if (lp->other->locksegp == seg) {
		    if (lp->owner->flag & LOCKMAP) {
			CG6UNMAP(lp->owner->locksegp);
			lp->owner->flag &= ~LOCKMAP;
		    }
		    if (lp->owner->unlocksegp != NULL)
			if (lp->owner->flag & UNLOCKMAP) {
			    CG6UNMAP(lp->owner->unlocksegp);
			    lp->owner->flag &= ~TRASHPAGE;
			    lp->owner->flag &= ~UNLOCKMAP;
			}
		    sp = lp->owner;
		    lp->owner = lp->other;
		    lp->other = sp;
		}
		/* map lock segment to page */
		CG6MAP(lp->owner->locksegp, lp->owner->lockaddr, lp->page);
		lp->owner->flag |= LOCKMAP;
		if (lp->owner->unlocksegp != NULL) {
		    if (lp->owner->flag & UNLOCKMAP) /***/
			CG6UNMAP(lp->owner->unlocksegp);
		    CG6MAP(lp->owner->unlocksegp,
			   lp->owner->unlockaddr, lp->page);
		    lp->owner->flag &= ~TRASHPAGE;
		    lp->owner->flag |= UNLOCKMAP;
		}
		(void)splx(s);
		return (0);
	    }
            (void)splx(s);
	    return (0);
	}

	if (*(int *)lp->page == 1) {

	    if (lp->sleepf) {
		if (lp->owner->flag & UNLOCKMAP)	/***/
		    CG6UNMAP(lp->owner->unlocksegp);
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, trashpage);
		lp->owner->flag |= TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		if (lp->owner->flag & LOCKMAP) {
		    CG6UNMAP(lp->owner->locksegp);
		    lp->owner->flag &= ~LOCKMAP;
		}
		untimeout(segcg6_timeout, (caddr_t)lp);
		wakeup((caddr_t)lp->page);       /* wake up sleeper */
		(void)splx(s);
		return (0);
	    }

	    if (lp->owner->procp==u.u_procp && lp->owner->unlocksegp==seg) {
		if (lp->owner->flag & UNLOCKMAP)	/***/
		    CG6UNMAP(lp->owner->unlocksegp);
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, lp->page);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		(void)splx(s);
		return (0);
	    }

	    if (lp->owner->flag & UNLOCKMAP) {
		CG6UNMAP(lp->owner->unlocksegp);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag &= ~UNLOCKMAP;
	    }

	    lp->sleepf = 1;

	    if (lp->cr.procp == u.u_procp)	/* creator about to sleep */
		timeout(segcg6_timeout, (caddr_t)lp, locktime*hz);

            (void)sleep((caddr_t)lp->page,PRIBIO);      /* goodnight, gracie */
                                        /* we wake up */
            lp->sleepf = 0;             /* clear sleepf */
            if (lp->cr.flag & UNGRAB) {
		CG6MAP(seg,addr,trashpage);
		(void)splx(s);
                return(0);
            }    

	    sp = lp->owner;	     /* switch owner and other */
	    lp->owner = lp->other;
	    lp->other = sp;

	    /* map new owner to page */
	    CG6MAP(lp->owner->locksegp, lp->owner->lockaddr, lp->page);
	    lp->owner->flag |= LOCKMAP;

	    if (lp->owner->flag & TRASHPAGE) {
		CG6UNMAP(lp->owner->unlocksegp);
		lp->owner->flag &= ~TRASHPAGE;
		lp->owner->flag |= UNLOCKMAP;
		CG6MAP(lp->owner->unlocksegp, lp->owner->unlockaddr, lp->page);
	    }
	}
	(void)splx(s);
	return (0);
}

/*
 * Handle a fault on a lego segment.  The only tricky part is that only one
 * valid mapping at a time is allowed.  Whenever a new mapping is called for, the
 * current values of the TEC and FBC registers in the old context are saved away,
 * and the old values in the new context are restored.
 */

/*ARGSUSED*/
static
segcg6_fault(seg, addr, len, type, rw)
	register struct seg *seg;
	addr_t addr;
	u_int len;
	enum fault_type type;
	enum seg_rw rw;
{
	register addr_t adr;
	register struct segcg6_data *current
		= (struct segcg6_data *)seg->s_data;
	register struct segcg6_data *old;
	int pf;
        register struct cg6_softc *softc = getsoftc(minor(current->dev));
	void    hat_devload();
	void    hat_unload();

	if (type != F_INVAL) {
		return (FC_MAKE_ERR(EFAULT));
	}

	if (current->flag & SEG_LOCK)
		return (segcg6_lockfault(seg, addr));

	if (current->cntxtp && softc->curcntxt != seg) {
		/*
		 * time to switch lego contexts.
		 */
		if (softc->curcntxt != NULL) {
			old = (struct segcg6_data *)softc->curcntxt->s_data;
			/*
			 * wipe out old valid mapping.
			 */

			    hat_unload(softc->curcntxt,
				       softc->curcntxt->s_base,
				       softc->curcntxt->s_size);
			    /*
			     * switch hardware contexts
			     */

			    if (cg6_cntxsave(
				    (struct fbc *)softc->va[MAP_FBCTEC],
				    (struct tec *)(softc->va[MAP_FBCTEC]+CG6_TEC_POFF),
				    old->cntxtp) == 0)
				return (FC_HWERR);
		    }

		    if (cg6_cntxrestore(
			    (struct fbc *)softc->va[MAP_FBCTEC],
			    (struct tec *)(softc->va[MAP_FBCTEC]+CG6_TEC_POFF),
			    current->cntxtp) == 0)
			return (FC_HWERR);

		/*
		 * switch software "context"
		 */
		softc->curcntxt = seg;
	}
	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		pf = cgsixmmap(current->dev, (off_t)current->offset+(adr-seg->s_base),
					PROT_READ|PROT_WRITE);
		if (pf == -1)
			return (FC_MAKE_ERR(EFAULT));

		hat_devload(seg, adr, pf, PROT_READ|PROT_WRITE|PROT_USER, 0 /* don't lock */);
	}
	return (0);
}

/*ARGSUSED*/
static
segcg6_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	return (PROT_READ|PROT_WRITE);
}


/*
 * segdev pages are always "in core".
 */
/*ARGSUSED*/
static 
segcg6_incore(seg, addr, len, vec)
        struct seg *seg;
        addr_t addr;
        register u_int len;
        register char *vec;
{
        u_int v = 0;

        for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE,
            v += PAGESIZE)
                *vec++ = 1;
        return (v);
}


static
segcg6_badop()
{
	/*
	 * silently fail.
	 */
	return (0);
}

#undef L_TEC_VDC_INTRNL0
#define	L_TEC_VDC_INTRNL0 0x8000
#undef L_TEC_VDC_INTRNL1
#define	L_TEC_VDC_INTRNL1 0xa000


cg6_cntxsave(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cg6_cntxt	*saved;
{
	int	dreg;	/* counts through the data registers */
	u_int	*dp;	/* points to a tec data register */
	register spin;

	/*
	 * wait for fbc not busy; but not forever
	 */
	for (spin = 100000; fbc->l_fbc_status & L_FBC_BUSY && --spin > 0;)
		;
	if (spin == 0) {
		return (0);
	}

	/*
	 * start dumping stuff out.
	 */
	saved->fbc.status = fbc->l_fbc_status;
	saved->fbc.clipcheck = fbc->l_fbc_clipcheck;
	saved->fbc.misc = fbc->l_fbc_misc;
	saved->fbc.x0 = fbc->l_fbc_x0;
	saved->fbc.y0 = fbc->l_fbc_y0;
	saved->fbc.x1 = fbc->l_fbc_x1;
	saved->fbc.y1 = fbc->l_fbc_y1;
	saved->fbc.x2 = fbc->l_fbc_x2;
	saved->fbc.y2 = fbc->l_fbc_y2;
	saved->fbc.x3 = fbc->l_fbc_x3;
	saved->fbc.y3 = fbc->l_fbc_y3;
	saved->fbc.rasteroffx = fbc->l_fbc_rasteroffx;
	saved->fbc.rasteroffy = fbc->l_fbc_rasteroffy;
	saved->fbc.autoincx = fbc->l_fbc_autoincx;
	saved->fbc.autoincy = fbc->l_fbc_autoincy;
	saved->fbc.clipminx = fbc->l_fbc_clipminx;
	saved->fbc.clipminy = fbc->l_fbc_clipminy;
	saved->fbc.clipmaxx = fbc->l_fbc_clipmaxx;
	saved->fbc.clipmaxy = fbc->l_fbc_clipmaxy;
	saved->fbc.fcolor = fbc->l_fbc_fcolor;
	saved->fbc.bcolor = fbc->l_fbc_bcolor;
	saved->fbc.rasterop = fbc->l_fbc_rasterop;
	saved->fbc.planemask = fbc->l_fbc_planemask;
	saved->fbc.pixelmask = fbc->l_fbc_pixelmask;
	saved->fbc.pattalign = fbc->l_fbc_pattalign;
	saved->fbc.pattern0 = fbc->l_fbc_pattern0;
	saved->fbc.pattern1 = fbc->l_fbc_pattern1;
	saved->fbc.pattern2 = fbc->l_fbc_pattern2;
	saved->fbc.pattern3 = fbc->l_fbc_pattern3;
	saved->fbc.pattern4 = fbc->l_fbc_pattern4;
	saved->fbc.pattern5 = fbc->l_fbc_pattern5;
	saved->fbc.pattern6 = fbc->l_fbc_pattern6;
	saved->fbc.pattern7 = fbc->l_fbc_pattern7;

	/*
	 * the tec matrix and clipping registers are easy.
	 */
	saved->tec.mv = tec->l_tec_mv;
	saved->tec.clip = tec->l_tec_clip;
	saved->tec.vdc = tec->l_tec_vdc;

	/*
	 * the tec data registers are a little more non-obvious.
	 * internally, they are 36 bits.  what we see in the
	 * register file is a 32-bit window onto the underlying
	 * data register.  changing the data-type in the VDC
	 * gets us either of two parts of the data register.
	 * the internal format is opaque to us.
	 */
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL0;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		saved->tec.data[dreg][0] = *dp;
	}
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL1;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		saved->tec.data[dreg][1] = *dp;
	}
	return (1);
}

cg6_cntxrestore(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cg6_cntxt	*saved;
{
	int	dreg;
	u_int	*dp;
	register spin;

	/*
	 * wait for fbc not busy; but not forever
	 */
	for (spin = 100000; fbc->l_fbc_status & L_FBC_BUSY && --spin > 0;)
		;
	if (spin == 0) {
		return (0);
	}

	/*
	 * reload the tec data registers.  see above for
	 * "how do they get 36 bits in that itty-bitty int"
	 */
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL0;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		*dp = saved->tec.data[dreg][0];
	}
	tec->l_tec_vdc = (u_int)L_TEC_VDC_INTRNL1;
	for (dreg = 0, dp = &tec->l_tec_data00; dreg < 64; dreg++, dp++) {
		*dp = saved->tec.data[dreg][1];
	}

	/*
	 * the tec matrix and clipping registers are next.
	 */
	tec->l_tec_mv = saved->tec.mv;
	tec->l_tec_clip = saved->tec.clip;
	tec->l_tec_vdc = saved->tec.vdc;

	/*
	 * now the FBC vertex and address registers
	 */
	fbc->l_fbc_x0 = saved->fbc.x0;
	fbc->l_fbc_y0 = saved->fbc.y0;
	fbc->l_fbc_x1 = saved->fbc.x1;
	fbc->l_fbc_y1 = saved->fbc.y1;
	fbc->l_fbc_x2 = saved->fbc.x2;
	fbc->l_fbc_y2 = saved->fbc.y2;
	fbc->l_fbc_x3 = saved->fbc.x3;
	fbc->l_fbc_y3 = saved->fbc.y3;
	fbc->l_fbc_rasteroffx = saved->fbc.rasteroffx;
	fbc->l_fbc_rasteroffy = saved->fbc.rasteroffy;
	fbc->l_fbc_autoincx = saved->fbc.autoincx;
	fbc->l_fbc_autoincy = saved->fbc.autoincy;
	fbc->l_fbc_clipminx = saved->fbc.clipminx;
	fbc->l_fbc_clipminy = saved->fbc.clipminy;
	fbc->l_fbc_clipmaxx = saved->fbc.clipmaxx;
	fbc->l_fbc_clipmaxy = saved->fbc.clipmaxy;

	/*
	 * restoring the attribute registers
	 */
	fbc->l_fbc_fcolor = saved->fbc.fcolor;
	fbc->l_fbc_bcolor = saved->fbc.bcolor;
	fbc->l_fbc_rasterop = saved->fbc.rasterop;
	fbc->l_fbc_planemask = saved->fbc.planemask;
	fbc->l_fbc_pixelmask = saved->fbc.pixelmask;
	fbc->l_fbc_pattalign = saved->fbc.pattalign;
	fbc->l_fbc_pattern0 = saved->fbc.pattern0;
	fbc->l_fbc_pattern1 = saved->fbc.pattern1;
	fbc->l_fbc_pattern2 = saved->fbc.pattern2;
	fbc->l_fbc_pattern3 = saved->fbc.pattern3;
	fbc->l_fbc_pattern4 = saved->fbc.pattern4;
	fbc->l_fbc_pattern5 = saved->fbc.pattern5;
	fbc->l_fbc_pattern6 = saved->fbc.pattern6;
	fbc->l_fbc_pattern7 = saved->fbc.pattern7;

	fbc->l_fbc_clipcheck = saved->fbc.clipcheck;
	fbc->l_fbc_misc = saved->fbc.misc;

	/*
	 * lastly, let's restore the status
	 */
	fbc->l_fbc_status = saved->fbc.status;
	return (1);
}
