#if !defined(lint) && !defined(NOID)
static char sccsid[] = "@(#)@(#)cgsix.c 1.1 92/07/30";
#endif

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

/*
 * Sbus accelerated 8 bit color frame buffer driver
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
#include <sys/mman.h>

#include <sys/kmem_alloc.h>

#include <sun/fbio.h>

#include <sundev/mbvar.h>
#include <sundev/cg6reg.h>

#include <machine/mmu.h>
#include <machine/pte.h>

#include <pixrect/pixrect.h>
#include <pixrect/pr_impl_util.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/memvar.h>
#include <pixrect/cg3var.h>	/* for CG3_MMAP_OFFSET */
#include <pixrect/cg6var.h>

#include <machine/vm_hat.h>
#include <vm/as.h>
#include <vm/seg.h>

addr_t map_regs();
void unmap_regs();
void report_dev();

/*
 * This flag indicates that we should use kcopy instead of copyin
 * for cursor ioctls.
 */
int fbio_kcopy;
extern int copyin(), kcopy();

#define	CG6_FBC_WAIT	500000		/* .5 seconds */

/*
 * A bunch of places in here need to know whether
 * we are supporting the "P4" or the "SBus" version.
 */
#if defined(sun4c) || defined(sun4m)
#define	SBUS_CGSIX
#endif

/* configuration options */
#ifndef OLDDEVSW
#define	OLDDEVSW 1
#endif  OLDDEVSW

#ifndef NOHWINIT
#define	NOHWINIT 0
#endif NOHWINIT

#ifndef CG6DEBUG
#define	CG6DEBUG 1
#endif CG6DEBUG

#if NOHWINIT
int cg6_hwinit = 1;
#endif NOHWINIT

#if CG6DEBUG
int cg6_debug = 0;
#define	DEBUGF(level, args)	_STMT(if (cg6_debug >= (level)) printf args;)
#define	DUMP_SEGS(level, c)	_STMT(if (cg6_debug >= (level)) dump_segs(c);)
#else CG6DEBUG
#define	DEBUGF(level, args)	/* nothing */
#define	DUMP_SEGS(level, c)	/* nothing */
#endif CG6DEBUG

#if OLDDEVSW
#define	STATIC /* nothing */
#define	cg6_open	cgsixopen
#define	cg6_close	cgsixclose
#define	cg6_mmap	cgsixmmap
#define	cg6_ioctl	cgsixioctl
#else OLDDEVSW
#define	STATIC static
#endif OLDDEVSW

typedef	int	(*ifunc)() ;

	/* memory-leak testing */

#if CG6DEBUG
static	int	total_memory = 0 ;

static caddr_t
my_kmem_zalloc(size,flag)
	u_int	size;
	int	flag;
{
	total_memory += size ;
	DEBUGF(5, ("zalloc %d bytes, total=%d\n",size, total_memory));
	return new_kmem_zalloc(size,flag) ;
}

static caddr_t
my_kmem_alloc(size,flag)
	u_int	size;
	int	flag;
{
	total_memory += size ;
	DEBUGF(5, ("alloc %d bytes, total=%d\n",size, total_memory));
	return new_kmem_alloc(size,flag) ;
}

static
my_kmem_free(ptr,size)
	caddr_t	ptr;
	u_int	size;
{
	total_memory -= size ;
	DEBUGF(5, ("free %d bytes, total=%d\n",size, total_memory));
	kmem_free(ptr, size) ;
}

#define	new_kmem_zalloc	my_kmem_zalloc
#define	new_kmem_alloc	my_kmem_alloc
#define	kmem_free	my_kmem_free

#endif CG6DEBUG



/* config info */
static int cg6_identify();
static int cg6_attach();
STATIC int cg6_open();
STATIC int cg6_close();
STATIC int cg6_ioctl();
STATIC int cgsixsegmap();

struct dev_ops cgsix_ops = {
	0,		/* revision */
	cg6_identify,
	cg6_attach,
	cg6_open,
	cg6_close,
	0, 0, 0, 0, 0,
	cg6_ioctl,
	0,
	cgsixsegmap,
};

/*
 * Table of CG6 addresses
 */
static	struct mapping {
	int	vaddr;		/* virtual (mmap offset) address */
	int	paddr;		/* physical (byte) address */
	int	size;		/* size in bytes (page multiple) */
} cg6_map[] = {
	{ CG6_VADDR_FBCTEC,	CG6_ADDR_FBC,	CG6_FBCTEC_SZ	},
	{ CG6_VADDR_CMAP,	CG6_ADDR_CMAP,	CG6_CMAP_SZ	},
	{ CG6_VADDR_FHCTHC,	CG6_ADDR_FHCTHC, CG6_FHCTHC_SZ	},
	{ CG6_VADDR_ROM,	CG6_ADDR_ROM,	CG6_ROM_SZ	},
	{ CG6_VADDR_DHC,	CG6_ADDR_DHC,	CG6_DHC_SZ	},
	{ CG6_VADDR_ALT,	CG6_ADDR_ALT,	CG6_ALT_SZ	},
	{ 0 }
};

#define MAP_FBCTEC      0

/* how much to map */
#define	CG6MAPSIZE	MMAPSIZE(0)

/* vertical retrace counter page */
#define	VRT_OFFSET	(CG6_VADDR_COLOR+CG6_FB_SZ)

/* per-unit data */
struct cg6_softc
{
#if NWIN > 0
	Pixrect		pr;		/* kernel pixrect */
	struct mprp_data prd;		/* pixrect private data */
#define	_w		pr.pr_size.x
#define	_h		pr.pr_size.y
#define	_fb		prd.mpr.md_image
#define	_linebytes	prd.mpr.md_linebytes
#else NWIN > 0
	int		_w, _h;		/* resolution */
	MPR_T		*_fb;		/* frame buffer address */
#endif NWIN > 0
	int		size;		/* total size of frame buffer */
	int		dummysize;	/* total size of overlay plane */
	addr_t		base;		/* mapped base address */
	int		basepage;	/* page # for base address */

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
		u_long	omap_long[2];	/* cheating here to save space */
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
#define	cmap_rgb	cmap_image.cmap_char[0]

#define	CG6VRTIOCTL	1	/* FBIOVERTICAL in effect */
#define	CG6VRTCTR	2	/* OWGX vertical retrace counter */
	unsigned long	fbmappable;	/* bytes mappable */
	int		*vrtpage;	/* pointer to VRT page */
	int		vrtmaps;	/* number of VRT page maps */
	int		vrtflag;	/* vrt interrupt flag */
	struct cg6_info	cg6info;	/* info about this cg6 */
	struct mon_info	moninfo;	/* info about this monitor */
	struct cg6_cntxt *curcntxt;	/* context switching */
	int		chiprev ;	/* fbc chip revision # */
#ifdef	MULTIPROCESSOR
	struct segcg6_data *mappings ;	/* full list of mappings */
#endif	MULTIPROCESSOR
};

/* Per-segment info:
 *	Some, but not all, segments are part of a context.
 *	Any segment that is a MAP_PRIVATE mapping to the TEC or FBC
 *	will be part of a unique context.  MAP_SHARED mappings are part
 *	of the shared context and all such programs must arbitrate among
 *	themselves to keep from stepping on each other's register settings.
 *	Mappings to the framebuffer may or may not be part of a context,
 *	depending on exact hardware type.
 */
struct segcg6_data {
	struct seg		*link;		/* next segment in ctx */
	struct seg		*seg;		/* pointer to segment */
	int			flag;		/* see below */
	struct cg6_cntxt	*cntxtp; 	/* pointer to context */
	dev_t			dev;
	u_int			offset;
	u_char			prot, maxprot ;
#ifdef	MULTIPROCESSOR
	struct segcg6_data	*next_map ;	/* link to next mapping */
#endif	MULTIPROCESSOR
};

#define	SEG_CTXINIT	(1)	/* part of a context */
#define	SEG_SHARED	(2)	/* shared context */
#define	SEG_LOCK	(4)	/* lock page */
#define	SEG_VRT		(8)	/* vrt page */
#define	SEG_FBC		(0X10)	/* segment includes fbc */
#define	SEG_TEC		(0X40)	/* segment includes tec */
#define	SEG_FB		(0X100)	/* segment includes framebuffer */
#define	SEG_CMAP	(0X400)	/* segment includes dacs */

#define	SEG_GLOBAL_MAP	(SEG_CMAP)	/* segs that need to be in map list */


static int ncg6;
static struct cg6_softc *cg6_softc = NULL ;

/* default structure for FBIOGATTR ioctl */
static struct fbgattr cg6_attr =  {
/*	real_type         owner */
	FBTYPE_SUNFAST_COLOR, 0,
/* fbtype: type             h  w  depth cms  size */
	{ FBTYPE_SUNFAST_COLOR, 0, 0, 8,    256,  0 },
/* fbsattr: flags emu_type      dev_specific */
	{ 0, FBTYPE_SUN4COLOR, { 0 } },
/*        emu_types */
	{ FBTYPE_SUNFAST_COLOR, FBTYPE_SUN3COLOR, FBTYPE_SUN4COLOR, -1}
};


/*
 * handy macros
 */

#define	getsoftc(unit)	(&cg6_softc[unit])

#define	btob(n)		ctob(btoc(n))		/* XXX */

#define	BZERO(d,c)	bzero((caddr_t) (d), (u_int) (c))
#define	COPY(f,s,d,c)	(f((caddr_t) (s), (caddr_t) (d), (u_int) (c)))
#define	CG6BCOPY(s,d,c)	COPY(bcopy,(s),(d),(c))
#define	COPYIN(s,d,c)	COPY(copyin,(s),(d),(c))
#define	COPYOUT(s,d,c)	COPY(copyout,(s),(d),(c))

/* enable/disable interrupt */
#define	TEC_EN_VBLANK_IRQ	0x20
#define	TEC_HCMISC_IRQBIT	0x10

/* position value to use to disable HW cursor */
#define	CG6_CURSOR_OFFPOS	(0xffe0ffe0)

#define	cg6_set_video(softc, on) \
	thc_set_video(CG6VA_TO_THC((softc)->base), (on))

#define	cg6_get_video(softc) \
	thc_get_video(CG6VA_TO_THC((softc)->base))

#define	cg6_int_enable(softc) \
	thc_int_enable(CG6VA_TO_THC((softc)->base))

#define	cg6_int_disable(softc) \
	thc_int_disable(CG6VA_TO_THC((softc)->base))

#define	cg6_int_pending(softc) \
	thc_int_pending(CG6VA_TO_THC((softc)->base))

/* check if color map update is pending */
#define	cg6_update_pending(softc) \
	((softc)->cmap_count || (softc)->omap_update)


/*
 * forward references
 */
static cg6_poll();
static void cg6_intr();
static void cg6_reset_cmap();
static void cg6_update_cmap();
static void cg6_cmap_bcopy();

#if NWIN > 0

/*
 * SunWindows specific stuff
 */
static cg6_rop(), cg6_putcolormap();

/* kernel pixrect ops vector */
static struct pixrectops cg6_pr_ops = {
	cg6_rop,
	cg6_putcolormap,
	mem_putattributes
};





/* XXX cursor turd avoidance -- there must be a better way to do this */

static cg6_rop_wait = 50;	/* milliseconds */

static
cg6_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy)
	Pixrect *dpr;
	int dx, dy, dw, dh, op;
	Pixrect *spr;
	int sx, sy;
{
	Pixrect mpr;
	int unit;

	if (spr && spr->pr_ops == &cg6_pr_ops) {
		unit = mpr_d(spr)->md_primary;
		mpr = *spr;
		mpr.pr_ops = &mem_ops;
		spr = &mpr;
	}
	else
		unit = mpr_d(dpr)->md_primary;

	if (cg6_rop_wait) {
		register u_int *statp =
			&CG6VA_TO_FBC((getsoftc(unit)->base))->l_fbc_status;

		CDELAY(!(*statp & L_FBC_BUSY), cg6_rop_wait << 10);
	}

	return mem_rop(dpr, dx, dy, dw, dh, op, spr, sx, sy);
}

/* XXX can I share code with ioctl? */
/* XXX vestigial overlay stuff */
static
cg6_putcolormap(pr, index, count, red, green, blue)
	Pixrect *pr;
	int index, count;
	unsigned char *red, *green, *blue;
{
	register struct cg6_softc *softc = getsoftc(mpr_d(pr)->md_primary);
	register u_int rindex = (u_int) index;
	register u_int rcount = (u_int) count;
	register u_char *map;
	register u_int entries;

	DEBUGF(5, ("cg6_putcolormap unit=%d index=%d count=%d\n",
		mpr_d(pr)->md_primary, index, count));

	map = softc->cmap_rgb;
	entries = CG6_CMAP_ENTRIES;

	/* check arguments */
	if (rindex >= entries ||
		rindex + rcount > entries)
		return PIX_ERR;

	if (rcount == 0)
		return 0;

	/* lock out updates of the hardware colormap */
	if (cg6_update_pending(softc))
		cg6_int_disable(softc);

	map += rindex * 3;
	cg6_cmap_bcopy(red,   map,     rcount);
	cg6_cmap_bcopy(green, map + 1, rcount);
	cg6_cmap_bcopy(blue,  map + 2, rcount);

	cg6_update_cmap(softc, rindex, rcount);

	/* enable interrupt so we can load the hardware colormap */
	cg6_int_enable(softc);

	return 0;
}

#endif	NWIN > 0

static
cg6_identify(name)
	char *name;
{
	DEBUGF(1, ("cg6_identify(%s)\n", name));

	if (strcmp(name, "cgsix") == 0)
		return ++ncg6;
	else
		return 0;
}

static
cg6_attach(devi)
	struct dev_info *devi;
{
	register struct cg6_softc *softc;
	register addr_t reg;
	int w, h, bytes, basepage;
	static int unit;
	extern struct dev_info *top_devinfo;	/* in autoconf.c */

	DEBUGF(1, ("cg6_attach ncg6=%d unit=%d\n", ncg6, unit));

	/* Allocate softc structs on first attach */
	if (!cg6_softc) {
	    cg6_softc = (struct cg6_softc *)
		new_kmem_zalloc((u_int) sizeof (struct cg6_softc) * ncg6,1);
		unit = 0;
	}

	softc = getsoftc(unit);

	/* Grab properties from PROM */
	/* XXX don't really want default w, h */
	softc->_w = w = getprop(devi->devi_nodeid, "width", 1152);
	softc->_h = h = getprop(devi->devi_nodeid, "height", 900);
	bytes = getprop(devi->devi_nodeid, "linebytes", mpr_linebytes(w, 8));
#if NWIN > 0
	softc->_linebytes = bytes;
#endif NWIN > 0

	/* Compute size of color frame buffer */
	bytes = btob(bytes * h);
	softc->size = CG6MAPSIZE + bytes;

	/* Compute size of dummy overlay/enable planes */
	softc->dummysize = btob(mpr_linebytes(w, 1) * h) * 2;

#if NWIN > 0
	/* only use address property if we are console fb */
	if (reg = (addr_t) getprop(devi->devi_nodeid, "address", 0)) {
		int fbid = getprop(top_devinfo->devi_nodeid, "fb", -1);
		if (fbid == devi->devi_nodeid) {
			softc->_fb = (MPR_T *) reg;
			bytes = 0;
			DEBUGF(2, ("cg6 mapped by PROM\n"));
		}
	}
#else NWIN > 0
	bytes = 0;
#endif NWIN > 0
        softc->cg6info.line_bytes = softc->_linebytes;
        softc->cg6info.vmsize = getprop(devi->devi_nodeid, "vmsize", 1);
	if (softc->cg6info.vmsize > 1) {
	    softc->size = CG6MAPSIZE+8*1024*1024;
	    softc->fbmappable = 8*1024*1024;
	} else
	    softc->fbmappable = 1024*1024;
        softc->cg6info.accessible_width =
                                getprop(devi->devi_nodeid, "awidth", 1152);
        softc->cg6info.accessible_height =
            (softc->cg6info.vmsize*1024*1024)/softc->cg6info.accessible_width;
        softc->cg6info.hdb_capable =
                                getprop(devi->devi_nodeid, "dblbuf", 0);
        softc->cg6info.boardrev = getprop(devi->devi_nodeid, "boardrev", 0);
#ifdef SBUS_SIZE
	softc->cg6info.slot =
		((int) devi->devi_reg[0].reg_addr - SBUS_BASE) / SBUS_SIZE ;
#else
	softc->cg6info.slot = sbusslot((unsigned) devi->devi_reg[0].reg_addr);
#endif
	softc->vrtpage = NULL;
	softc->vrtmaps = 0;
	softc->vrtflag = 0;
#define	DEBUG
#ifdef DEBUG
	softc->cg6info.pad1 = CG6_VADDR_COLOR+CG6_FB_SZ;
#endif
        /*
         * get monitor attributes
         */
        softc->moninfo.mon_type = getprop(devi->devi_nodeid, "montype", 0);
        softc->moninfo.pixfreq = getprop(devi->devi_nodeid, "pixfreq", 929405);
        softc->moninfo.hfreq = getprop(devi->devi_nodeid, "hfreq", 61795);
        softc->moninfo.vfreq = getprop(devi->devi_nodeid, "vfreq", 65690);
        softc->moninfo.hfporch = getprop(devi->devi_nodeid, "hfporch", 32);
        softc->moninfo.vfporch = getprop(devi->devi_nodeid, "vfporch", 2);
        softc->moninfo.hbporch = getprop(devi->devi_nodeid, "hbporch", 192);
        softc->moninfo.vbporch = getprop(devi->devi_nodeid, "vbporch", 31);
        softc->moninfo.hsync = getprop(devi->devi_nodeid, "hsync", 128);
        softc->moninfo.vsync = getprop(devi->devi_nodeid, "vsync", 4);


	/*
	 * Allocate contiguous virtual space for registers and
	 * maybe frame buffer.  This gets overlayed a few lines down
	 * by fbmapin.  Kind of a retro-kludge; fixed in SVr4.
	 */
	if (!(reg = map_regs(devi->devi_reg[0].reg_addr,
		(u_int) (CG6MAPSIZE + bytes),
		devi->devi_reg[0].reg_bustype)))
		return -1;

	softc->base = reg;
	softc->basepage = basepage = fbgetpage(reg);
	softc->curcntxt = NULL ;
#ifdef	MULTIPROCESSOR
	softc->mappings = NULL ;
#endif	MULTIPROCESSOR

	/* next, replace this register mapping new ones.  Our
	 * offsets are based on the virtual addresses in cg6reg.h
	 */

#define	FBMAPIN(v,p,s)	fbmapin((addr_t)(reg + v - CG6_VBASE), \
    (int)(basepage + btoc(p)), (int)s)

	FBMAPIN(CG6_VADDR_FBCTEC, CG6_ADDR_FBC,    CG6_FBCTEC_SZ) ;
	FBMAPIN(CG6_VADDR_CMAP,   CG6_ADDR_CMAP,   CG6_CMAP_SZ) ;
	FBMAPIN(CG6_VADDR_FHCTHC, CG6_ADDR_FHCTHC, CG6_FHCTHC_SZ) ;
	FBMAPIN(CG6_VADDR_ROM,    CG6_ADDR_ROM,    CG6_ROM_SZ) ;


#if NWIN > 0
	/* map frame buffer if necessary */
	if (bytes) {
		fbmapin((addr_t) CG6VA_TO_DFB(reg),
			(int) (basepage + btoc(CG6_ADDR_COLOR)), bytes);
		softc->_fb = CG6VA_TO_DFB(reg);
	}
#endif NWIN > 0

	softc->chiprev =
	  (*CG6VA_TO_FHC(reg) >> FHC_CONFIG_REV_SHIFT) & FHC_CONFIG_REV_MASK ;

	DEBUGF(1, ("cg6_attach reg=0x%x/0x%x (0x%x)\n",
		(u_int) reg,
		fbgetpage((addr_t) reg) << PGSHIFT,
		fbgetpage((addr_t) reg)));

#if NOHWINIT
	if (cg6_hwinit)
		cg6_init(reg);
#endif NOHWINIT
	cg6_reset(reg);

	/* save unit number */
	devi->devi_unit = unit;

	/* Notify VM system our interrupt handler calls the hat layer. */
	adddma(devi->devi_intr[0].int_pri);

	/* attach interrupt */
	addintr(devi->devi_intr[0].int_pri, cg6_poll, devi->devi_name, unit);

	/* save back pointer to softc */
	devi->devi_data = (addr_t) softc;

	/*
	 * Initialize hardware colormap and software colormap images.
	 * It might make sense to read the hardware colormap here.
	 */
	cg6_reset_cmap(softc->cmap_rgb, CG6_CMAP_ENTRIES);
	cg6_reset_cmap(softc->omap_rgb, 2);
	cg6_update_cmap(softc, (u_int) _ZERO_, CG6_CMAP_ENTRIES);
	cg6_update_cmap(softc, (u_int) _ZERO_, (u_int) _ZERO_);

	report_dev(devi);

	if ((* CG6VA_TO_FHC(reg) >> FHC_CONFIG_REV_SHIFT &
		FHC_CONFIG_REV_MASK) == 0)
		printf("Revision 0 FBC\n");

	printf("cgsix%d: screen %dx%d, %s buffered, %dM mappable, rev %d\n",
		unit, w,h, softc->cg6info.hdb_capable ? "double" : "single",
		softc->cg6info.vmsize, softc->chiprev ) ;

	/* prepare for next attach */
	unit++;

	return 0;
}

STATIC
cg6_open(dev, flag)
	dev_t dev;
	int flag;
{
	DEBUGF(2, ("cg6_open(%d), mem used=%d\n", minor(dev), total_memory));

	return fbopen(dev, flag, ncg6);
}

/*ARGSUSED*/
STATIC
cg6_close(dev, flag)
	dev_t dev;
	int flag;
{
	struct cg6_softc *softc = getsoftc(minor(dev));

	DEBUGF(2, ("cg6_close(%d), mem used=%d\n", minor(dev), total_memory));

	softc->cur.enable = 0;
	softc->curcntxt = NULL ;
	cg6_reset(softc->base);

	return 0;
}

/*ARGSUSED*/
STATIC
cg6_mmap(dev, off, prot)
	dev_t dev;
	register off_t off;
	int prot;
{
	register struct cg6_softc *softc = getsoftc(minor(dev));
	register int page = -1;

	DEBUGF(off ? 4 : 1, ("cg6_mmap(%d, 0x%x)\n", minor(dev), (u_int) off));

	if ((u_int) off < CG6_VBASE) {
		if (off >= CG3_MMAP_OFFSET)
			off += CG6_VADDR_COLOR - CG3_MMAP_OFFSET;
		else if ((off -= softc->dummysize) < 0)
			off = CG6_VADDR_ROM;
		else
			off += CG6_VADDR_COLOR;
	}

	if ((u_int) (off - CG6_VADDR_COLOR) < softc->fbmappable)
		page = CG6_ADDR_COLOR + (off - CG6_VADDR_COLOR);
	else {
		register struct mapping *mp;

		for (mp = cg6_map; mp->size; mp++)
			if ((u_int) (off - mp->vaddr) < mp->size)
				page = mp->paddr + (off - mp->vaddr);
	}

	if (page >= 0)
		page = softc->basepage + btop(page);

	DEBUGF(4, ("cg6_mmap returning 0x%x\n", page));

	return page;
}

/*ARGSUSED*/
STATIC
cg6_ioctl(dev, cmd, data, flag)
	dev_t dev;
	int cmd;
	caddr_t data;
	int flag;
{
	register struct cg6_softc *softc = getsoftc(minor(dev));
	int cursor_cmap;
	int (*copyfun)();
	static union {
		u_char cmap[CG6_CMAP_ENTRIES];
		u_long cur[32];
	} iobuf;

	DEBUGF(3, ("cg6_ioctl(%d, 0x%x)\n", minor(dev), cmd));

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

			DEBUGF(3, ("FBIOPUTCMAP\n"));
			if (cg6_update_pending(softc))
				cg6_int_disable(softc);

			map += index * 3;

			/* sorry */
			do {
				if (error = COPY(copyfun, cmap->red,
					iobuf.cmap, count))
					break;
				cg6_cmap_bcopy(iobuf.cmap, map++, count);

				if (error = COPY(copyfun, cmap->green,
 					iobuf.cmap, count))
					break;
				cg6_cmap_bcopy(iobuf.cmap, map++, count);

				if (error = COPY(copyfun, cmap->blue,
 					iobuf.cmap, count))
					break;
				cg6_cmap_bcopy(iobuf.cmap, map, count);
			} while (_ZERO_);

			if (error) {
				if (cg6_update_pending(softc))
					cg6_int_enable(softc);
				return EFAULT;
			}

			/* cursor colormap update */
			if (entries < CG6_CMAP_ENTRIES)
				count = 0;

			cg6_update_cmap(softc, index, count);
			cg6_int_enable(softc);
		}
		else {
			/* FBIOGETCMAP */
			DEBUGF(3, ("FBIOGETCMAP\n"));
			map += index * 3;
			cg6_cmap_bcopy(iobuf.cmap, map++, -count);
			if (COPYOUT(iobuf.cmap, cmap->red, count))
				return EFAULT;

			cg6_cmap_bcopy(iobuf.cmap, map++, -count);
			if (COPYOUT(iobuf.cmap, cmap->green, count))
				return EFAULT;

			cg6_cmap_bcopy(iobuf.cmap, map, -count);
			if (COPYOUT(iobuf.cmap, cmap->blue, count))
				return EFAULT;
		}
	}
	break;

	case FBIOGATTR: {
		register struct fbgattr *attr = (struct fbgattr *) data;

		DEBUGF(3, ("FBIOGATTR\n"));
		*attr = cg6_attr;
		data = (caddr_t) &attr->fbtype;
	}
	/* fall through */

	case FBIOGTYPE: {
		register struct fbtype *fb = (struct fbtype *) data;

		if (cmd == FBIOGTYPE) {
			DEBUGF(3, ("FBIOGTYPE\n"));
			*fb = cg6_attr.fbtype;
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

		DEBUGF(3, ("FBIOGPIXRECT\n"));
		/* initialize pixrect and private data */
		softc->pr.pr_ops = &cg6_pr_ops;
		/* pr_size set in attach */
		softc->pr.pr_depth = 8;
		softc->pr.pr_data = (caddr_t) &softc->prd;

		/* md_linebytes, md_image set in attach */
		/* md_offset already zero */
		softc->prd.mpr.md_primary = minor(dev);
		softc->prd.mpr.md_flags = MP_DISPLAY | MP_PLANEMASK;
		softc->prd.planes = 255;

		/* enable video */
		cg6_set_video(softc, _ONE_);

		break;

#endif NWIN > 0

	case FBIOSVIDEO:
		DEBUGF(3, ("FBIOSVIDEO\n"));
		cg6_set_video(softc, * (int *) data & FBVIDEO_ON);
		break;

	case FBIOGVIDEO:
		DEBUGF(3, ("FBIOGVIDEO\n"));
		* (int *) data =
			cg6_get_video(softc) ? FBVIDEO_ON : FBVIDEO_OFF;
		break;

        case GRABPAGEALLOC:     /* Window Grabber */
                return(segcg6_graballoc(data));
        case GRABATTACH:        /* attach to free memory */
                return(segcg6_grabattach(data));
        case GRABPAGEFREE:      /* Window Grabber */
                return(segcg6_grabfree(data));
        case GRABLOCKINFO:      /* Window Grabber */
                return(segcg6_grabinfo(data));

        /* informational ioctls */

        case    FBIOGXINFO:
                *(struct cg6_info *)data = softc->cg6info;
                return(0);

        case    FBIOMONINFO:
                *(struct mon_info *)data = softc->moninfo;
                return(0);

	/* vertical retrace interrupt */

	case 	FBIOVERTICAL:
		softc->vrtflag |= CG6VRTIOCTL;
		{   /* kludge alert. */
		    int	i;

		    i = splbio();
		    cg6_int_enable(softc);
		    /* look, ma! a critical section! right here! */
		    (void) sleep((caddr_t)softc, PRIBIO);
		    (void) splx(i);
		}   /* kludge complete */

		return(0);

	case	FBIOVRTOFFSET:
		*(int *)data = VRT_OFFSET;
		return(0);

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
		return ENOTTY;

	}

	return 0;
}

static
cg6_poll()
{
	register int i, serviced = 0;
	register struct cg6_softc *softc;

	/*
	 * Look for any frame buffers that were expecting an interrupt.
	 */

	DEBUGF(7, ("cg6_poll\n"));
	for (softc = cg6_softc, i = ncg6; --i >= 0; softc++)
		if (cg6_int_pending(softc)) {
			if (cg6_update_pending(softc) || softc->vrtflag)
				cg6_intr(softc);
			else
				/* XXX catch stray interrupts? */
				cg6_int_disable(softc);
			serviced++;
		}

	return serviced;
}

static void
cg6_intr(softc)
	register struct cg6_softc *softc;
{
	struct cg6_cmap *cmap = CG6VA_TO_CMAP(softc->base);
	LOOP_T count = softc->cmap_count;
	u_long *in;
	u_long *out;
	u_long tmp;
	void	hat_unload();

	DEBUGF(7, ("cg6_intr(%d)\n", softc - cg6_softc));

	if (softc->vrtflag & CG6VRTCTR) {
	    if (softc->vrtmaps == 0) {
		softc->vrtflag &= ~CG6VRTCTR;
	    } else
		*softc->vrtpage += 1;
	}

	if (softc->vrtflag & CG6VRTIOCTL) {
	    softc->vrtflag &= ~CG6VRTIOCTL;
	    wakeup((caddr_t)softc);
	}

	if (cg6_update_pending(softc)) {
#ifdef	MULTIPROCESSOR
	    /* blech, foo.  Need to keep user processes from touching the
	     * colormap registers while we're touching them.  Do this
	     * by invalidating all user mappings here.  Only blow away
	     * the cmap page from the mapping.
	     */

	    {
	      register struct segcg6_data *ptr ;
	      for(ptr=softc->mappings; ptr != NULL; ptr = ptr->next_map)
		if( ptr->flag & SEG_CMAP ) {
		  DEBUGF(5, ("removing mapping for seg %x at %x\n",
			ptr->seg,
			ptr->seg->s_base + CG6_VADDR_CMAP - ptr->offset));
		  hat_unload(ptr->seg,
			ptr->seg->s_base + CG6_VADDR_CMAP - ptr->offset,
			CG6_CMAP_SZ) ;
		}
	    }
#endif	MULTIPROCESSOR


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
	}

	cg6_int_disable(softc);
	if (softc->vrtflag)
	    cg6_int_enable(softc);
}

/*
 * Initialize a colormap: background = white, all others = black
 */
static void
cg6_reset_cmap(cmap, entries)
	register u_char *cmap;
	register u_int entries;
{
	bzero((char *) cmap, entries * 3);
	cmap[0] = 255;
	cmap[1] = 255;
	cmap[2] = 255;
}

/*
 * Compute color map update parameters: starting index and count.
 * If count is already nonzero, adjust values as necessary.
 * Zero count argument indicates cursor color map update desired.
 */
static void
cg6_update_cmap(softc, index, count)
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
cg6_cmap_bcopy(bufp, rgb, count)
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

/*
 * enable/disable/update HW cursor
 */
static
cg6_setcurpos(softc)
	struct cg6_softc *softc;
{
	CG6VA_TO_THC(softc->base)->l_thc_cursor =
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
	hw = (u_long *) &CG6VA_TO_THC(softc->base)->l_thc_cursora00;

	for (i = 0; i < 32; i++) {
		hw[i] = (tmp = mask[i] & edge);
		hw[i + 32] = tmp & image[i];
	}
}

static
cg6_reset(addr)
	addr_t addr;
{
	/* disable HW cursor */
	CG6VA_TO_THC(addr)->l_thc_cursor = CG6_CURSOR_OFFPOS;

	/* reinitialize TEC */
	{
		struct tec *tec = CG6VA_TO_TEC(addr);

		tec->l_tec_mv = 0;
		tec->l_tec_clip = 0;
		tec->l_tec_vdc = 0;
	}

	/* reinitialize FBC config register */
	{
		u_int *fhc = CG6VA_TO_FHC(addr), rev, conf;

		rev = *fhc >> FHC_CONFIG_REV_SHIFT & FHC_CONFIG_REV_MASK;
		if (rev <= 4) {

		    /* PROM knows how to deal with LSC and above */
		    /* rev == 0 : FBC 0 (not available to customers) */
		    /* rev == 1 : FBC 1 */
		    /* rev == 2 : FBC 2 */
		    /* rev == 3 : Toshiba (never built) */
		    /* rev == 4 : Standard Cell (not built yet) */
		    /* rev == 5 : LSC */
		    conf = *fhc & FHC_CONFIG_RES_MASK |
			    FHC_CONFIG_CPU_68020;
#if FBC_REV0
		    /* FBC0: test window = 0, disable fast rops */
		    if (rev == 0)
			    conf |= FHC_CONFIG_TEST |
				    FHC_CONFIG_FROP_DISABLE;
		    else
#endif FBC_REV0
		    /* test window = 1K x 1K */
		    conf |= FHC_CONFIG_TEST |
			    (10 + 1) << FHC_CONFIG_TESTX_SHIFT |
			    (10 + 1) << FHC_CONFIG_TESTY_SHIFT;

		    /* FBC[01]: disable destination cache */
		    if (rev <= 1)
			    conf |= FHC_CONFIG_DST_DISABLE;

		    *fhc = conf;
		}
	}

	/* reprogram DAC to enable HW cursor use */
	{
		struct cg6_cmap *cmap = CG6VA_TO_CMAP(addr);

		/* command register */
		cmap->addr = 6 << 24;

		/* turn on CR1:0, overlay enable */
		cmap->ctrl = cmap->ctrl | (0x3 << 24);
	}
}

#if NOHWINIT
cg6_init(addr)
	addr_t addr;
{
	/* Initialize DAC */
	{
		register struct cg6_cmap *cmap = CG6VA_TO_CMAP(addr);
		register char *p;

		static char dacval[] = {
			4, 0xff,
			5, 0,
			6, 0x73,
			7, 0,
			0
		};

		/* initialize DAC */
		for (p = dacval; *p; p += 2) {
			cmap->addr = p[0] << 24;
			cmap->ctrl = p[1] << 24;
		}
	}

	/* Initialize THC */
	{
		register struct thc *thc = CG6VA_TO_THC(addr);
		int vidon;

		vidon = thc_get_video(thc);
		thc->l_thc_hcmisc = THC_HCMISC_RESET | THC_HCMISC_INIT;
		thc->l_thc_hcmisc = THC_HCMISC_INIT;

		thc->l_thc_hchs = 0x010009;
		thc->l_thc_hchsdvs = 0x570000;
		thc->l_thc_hchd = 0x15005d;
		thc->l_thc_hcvs = 0x010005;
		thc->l_thc_hcvd = 0x2403a8;
		thc->l_thc_hcr = 0x00016b;

		thc->l_thc_hcmisc = THC_HCMISC_RESET | THC_HCMISC_INIT;
		thc->l_thc_hcmisc = THC_HCMISC_INIT;

		if (vidon)
			thc_set_video(thc, _ONE_);

		DEBUGF(1, ("TEC rev %d\n",
			thc->l_thc_hcmisc >> THC_HCMISC_REV_SHIFT &
			THC_HCMISC_REV_MASK));
	}

	/*
	 * Initialize FHC for 1152 X 900 screen
	 */
	{
		register u_int *fhc = CG6VA_TO_FHC(addr), rev;

		rev = *fhc >> FHC_CONFIG_REV_SHIFT & FHC_CONFIG_REV_MASK;
		DEBUGF(1, ("FBC rev %d\n", rev));

		/*
		 * FBC0: disable fast rops
		 * FBC[01]: disable destination cache
		 */
		*fhc = FHC_CONFIG_1152 |
			FHC_CONFIG_CPU_68020 |
			FHC_CONFIG_TEST |
#if FBC_REV0
			rev == 0 ? FHC_CONFIG_FROP_DISABLE : 0 |
#endif FBC_REV0
			rev <= 1 ? FHC_CONFIG_DST_DISABLE : 0;
	}
}
#endif NOHWINIT


/* external routines */
extern	caddr_t getpages();

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
 * Per-context info:
 *	many registers in the tec and fbc do
 *	not need to be saved/restored.
 */
struct cg6_cntxt
{
	struct cg6_cntxt	*link ;		/* must be first */
	struct seg		*seg_list ;	/* segments in this context */
	int			pid ;
	int			flag ;
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

static	struct cg6_cntxt       shared_context = {NULL,NULL,0,} ;






/*
 *      Window Grabber lock management
 */

#define	LOCK_PAGESIZE	(8*1024)
#define LOCK_OFFBASE    VRT_OFFSET+PAGESIZE

#define NLOCKS  	16
#define	LOCKTIME	3	/* seconds */

/* flag bits */
#define LOCKMAP         0x01
#define UNLOCKMAP       0x02
#define ATTACH          0x04
#define TRASHPAGE       0x08
#define UNGRAB          0x10    /* ungrab called */

struct  segproc {
        int             flag;
        struct  proc    *procp;
        caddr_t         lockaddr;
        struct  seg     *locksegp;
        caddr_t         unlockaddr;
        struct  seg     *unlocksegp;
};

struct  seglock {
        short           sleepf;         /* sleep flag */
	short		allocf;		/* allocated flag */
        off_t           offset;         /* offset into device */
        int    		*page;          /* virtual address of lock page */
        struct  segproc cr;             /* creator */
        struct  segproc cl;             /* client */
        struct  segproc *owner;         /* current owner of lock */
        struct  segproc *other;         /* not owner of lock */
};

static	caddr_t trashpage=NULL;		/* for trashing unwanted writes */

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
	register struct cg6_softc *softc = getsoftc(minor(dev));
	struct segcg6_data dev_a;
	int	segcg6_create();
	register	i;
	caddr_t		seglock = 0 ;
	int		segvrt = 0;

	enum	as_res	as_unmap();
	int	as_map();

	DEBUGF(3, ("segmap: d=%x, of=%x, adr=%x, l=%x, p=%x, maxp=%x, f=%x\n",
		dev, offset, addr, len, prot, maxprot, flags));

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

	dev_a.flag = 0 ;
	dev_a.link = NULL;
	dev_a.cntxtp = NULL;
	dev_a.prot = prot ;
	dev_a.maxprot = maxprot ;

	/*
	 * check to see if this is a VRT page
	 */

	if (offset == VRT_OFFSET) {

	    if (len != PAGESIZE)
		return(EINVAL);

	    segvrt++;
	    if (softc->vrtmaps++ == 0) {
		if (softc->vrtpage == NULL)
		    softc->vrtpage = (int *)getpages(1, KMEM_NOSLEEP);
		*softc->vrtpage = 0;
		softc->vrtflag |= CG6VRTCTR;
		cg6_int_enable(softc);
	    }

	} else

	/*
         * Check to see if this is an allocated lock page.
         */

        if ((seglock = (caddr_t)segcg6_findlock((off_t)offset)) !=
	    (caddr_t) NULL) {
                if (len != LOCK_PAGESIZE)
                        return(EINVAL);
        } else {
                /*
                 * Check to insure that the entire range is
                 * legal and we are not trying to map in
                 * more than the device will let us.
                 */
                for (i = 0; i < len; i += PAGESIZE)
		    if (cgsixmmap(dev, (off_t)offset + i, (int)maxprot) == -1)
			return (ENXIO);

		/* classify it */
		if( offset+len > CG6_VADDR_FBC  &&
		    offset < CG6_VADDR_FBC+CG6_TEC_POFF )
		  dev_a.flag |= SEG_FBC ;
		if( offset+len > CG6_VADDR_TEC  &&
		    offset < CG6_VADDR_FBC+CG6_FBCTEC_SZ )
		  dev_a.flag |= SEG_TEC ;
		if( offset+len > CG6_VADDR_COLOR  &&
		    offset < CG6_VADDR_COLOR+softc->fbmappable )
		  dev_a.flag |= SEG_FB ;
#ifdef	MULTIPROCESSOR
		if( offset+len > CG6_VADDR_CMAP  &&
		    offset < CG6_VADDR_CMAP+CG6_CMAP_SZ )
		  dev_a.flag |= SEG_CMAP ;
#endif	MULTIPROCESSOR
DEBUGF(3,("segment flags=%x\n", dev_a.flag));
        }

	/* LSC DFB BUG KLUDGE:  DFB must always be mapped private
	 * on the buggy (chip rev. 5) LSC chip.  This is done to ensure
	 * that nobody ever touches the framebuffer without the segment
	 * driver getting involved to make sure the chips are idle.  This
	 * involves taking a page fault, invalidating all other process's
	 * mappings to the fb, (and performing a context switch?)
	 *
	 * Under pixrects, which maps the chips and the FB all at once,
	 * the entire mapping becomes a context.  This won't hurt
	 * pixrects but entails unnecessary context switching.  Under
	 * other libraries such as XGL, which maps the chips private and
	 * the FB shared, the FB becomes part of the context.  Programs
	 * which only map the FB will also become contexts which also
	 * entails unnecessary context switching.  Such is life.
	 */
#ifdef	COMMENT
	if( softc->chiprev == 5 && (dev_a.flag & SEG_FB) )
#endif	/* COMMENT */
	  flags = (flags & ~MAP_TYPE) | MAP_PRIVATE ;

	/* decide if this segment is part of a context. */

	if( dev_a.flag & (softc->chiprev == 5 ? (SEG_FBC|SEG_TEC|SEG_FB) :
						(SEG_FBC|SEG_TEC)) ) {
	  dev_a.flag |= SEG_CTXINIT ;
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
	if (segvrt) {
                dev_a.flag = SEG_VRT;
        } else
	if (seglock) {
                dev_a.flag = SEG_LOCK;
        }
#ifdef	COMMENT
	else if ((flags & MAP_TYPE) == MAP_SHARED) { /* shared mapping */
		dev_a.flag |= SEG_SHARED;
	}
#endif	/* COMMENT */

    return(as_map((struct as *)as, *addr, len, segcg6_create, (caddr_t)&dev_a));
}

static	int segcg6_dup(/* seg, newsegp */);
static	int segcg6_unmap(/* seg, addr, len */);
static	int segcg6_free(/* seg */);
static	faultcode_t segcg6_fault(/* seg, addr, len, type, rw */);
static	int segcg6_checkprot(/* seg, addr, size, prot */);
static	int segcg6_badop();
static	int segcg6_incore();

static	struct seg_ops	segcg6_ops =  {
	segcg6_dup,	/* dup */
	segcg6_unmap,	/* unmap */
	segcg6_free,	/* free */
	segcg6_fault,	/* fault */
	segcg6_badop,	/* faulta */
	segcg6_badop,	/* unload */
	segcg6_badop,	/* setprot */
	segcg6_checkprot,/* checkprot */
	segcg6_badop,	/* kluster */
	(u_int (*)()) NULL,	/* swapout */
	segcg6_badop,	/* sync */
	segcg6_incore	/* incore */
};

/*
 * Window Grabber Page Management
 */

/*
 * search the lock_list list for the specified offset
 *
 */

/*ARGSUSED*/
static
struct seglock *
segcg6_findlock(off)
off_t   off;
{
        register int i;

	if ((unsigned long)off < LOCK_OFFBASE)
                return((struct seglock *)NULL);
        for(i=0;i<NLOCKS;i++)
            if  (off == lock_list[i].offset)
                return(lock_list+i);
        return((struct seglock *)NULL);
}

/*ARGSUSED*/
int
segcg6_grabattach(cookiep)      /* IOCTL */
caddr_t cookiep;
{

    return(EINVAL);
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
int
segcg6_graballoc(pp)    /* IOCTL */
caddr_t pp;
{
        register        int i;
	register	struct seglock	*lp = lock_list;

	if (trashpage == NULL) {
		trashpage = getpages(1,KMEM_NOSLEEP); /* for trashing unwanted writes */
		for(i=0;i<NLOCKS;i++) 	/* initialize offsets */
		    lock_list[i].offset = (off_t)(LOCK_OFFBASE+LOCK_PAGESIZE*i);
	}

        for(i=0;i<NLOCKS;i++,lp++)
            if (lp->allocf == 0) {
		lp->allocf = 1;
		if (lp->page == (int *)NULL)
		    lp->page = (int *)getpages(1,KMEM_NOSLEEP);
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
                return(0);
            }
        return(ENOMEM);
}

/*ARGSUSED*/
int
segcg6_grabfree(pp)     /* IOCTL */
caddr_t pp;
{
    register    struct seglock  *lp;
    void	segcg6_timeout();

    if ((lp = segcg6_findlock((off_t)*(int *)pp)) == NULL)
        return(EINVAL);
    lp->cr.flag |= UNGRAB;
    lp->allocf = 0;
    if (lp->sleepf) {   /* client will acquire the lock in this case */
	untimeout((ifunc)segcg6_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
    return(0);
}


#if	CG6DEBUG
static
dump_segs(cur)
	struct cg6_cntxt *cur ;
{
	register struct seg *segp ;
	register struct segcg6_data *ptr ;
	register struct cg6_cntxt *ctx = &shared_context ;

	for(; ctx; ctx=ctx->link)
	{
	  printf("  ctx %x%s%s: link=%x, seg_list=%x, pid=%d, flag=%x\n",
	  ctx, ctx==&shared_context ? " (shared)" : "",
	  ctx==cur ? "(!)" : "", ctx->link, ctx->seg_list,
	  ctx->pid, ctx->flag) ;
	  for(segp = ctx->seg_list; segp; segp=ptr->link)
	  {
	    ptr = (struct segcg6_data *)segp->s_data ;
	    printf("   seg %x: sd=%x, link=%x, flag=%x, cntxtp=%x\n",
	      segp, ptr, ptr->link, ptr->flag, ptr->cntxtp) ;
	    printf("     base=%x, size=%x, ofs=%x\n",
	      segp->s_base, segp->s_size, ptr->offset) ;
	  }
	}
}
#endif	CG6DEBUG

/*
 * Insert a segment into the segment list of a context:
 *
 * 1) find out which context this belongs in:
 *    search all contexts for a context that matches the current PID
 *    and for which no segments of this type (fbc/tec/fb) have yet
 *    been assigned.  If no such context is found, create one.
 * 2) insert into list
 */
static
ctx_seg_insert(seg)
	struct seg *seg;
{
	register struct segcg6_data *dp = (struct segcg6_data *) seg->s_data ;
	register struct cg6_cntxt *ctx ;

	if( dp->flag & SEG_SHARED )
	  ctx = &shared_context ;
	else
	{
	  for(ctx = shared_context.link; ctx != NULL; ctx=ctx->link)
	    if( ctx->pid == u.u_procp->p_pid  &&
		(ctx->flag & dp->flag & (SEG_FBC|SEG_TEC|SEG_FB)) == 0 )
	      break ;		/* use this context */

	  if( ctx == NULL )	/* no match, create a new one */
	  {
	    ctx = (struct cg6_cntxt *)
		new_kmem_zalloc( sizeof (struct cg6_cntxt),KMEM_NOSLEEP) ;
	    if( ctx == NULL )
 		panic("cgsix: out of memory in ctx_seg_insert");
	    /* TODO: MT-Safe */
	    ctx->link = shared_context.link ;
	    shared_context.link = ctx ;
	    /* END: MT-Safe */
	    ctx->pid = u.u_procp->p_pid ;
	    ctx->flag = 0 ;
	  }
	}
	dp->cntxtp = ctx ;
	DEBUGF(3, ("ctx_seg_insert: seg=%x, dp=%x, ctx=%x\n", seg,dp,ctx));


	/* now that we have the context, link this segment into it */

	/* TODO: MT-Safe */
	dp->link = ctx->seg_list ;
	ctx->seg_list = seg ;
	ctx->flag |= dp->flag ;
	/* END: MT-Safe */
}

/*
 * Create a device segment.  the lego context is initialized by the create args.
 */
static
int
segcg6_create(seg, argsp)
	struct seg *seg;
	caddr_t argsp;
{
	register struct segcg6_data *dp;

	dp = (struct segcg6_data *)
		new_kmem_zalloc(sizeof (struct segcg6_data),KMEM_NOSLEEP);
	*dp = *((struct segcg6_data *)argsp);

	dp->seg = seg ;
	seg->s_ops = &segcg6_ops;
	seg->s_data = (char *)dp;

	/* if this segment is part of a context, link it into that context */
	if( dp->flag & SEG_CTXINIT )
	  ctx_seg_insert(seg) ;

#ifdef	MULTIPROCESSOR
	if( dp->flag & SEG_GLOBAL_MAP ) {
	  register struct cg6_softc *softc = getsoftc(minor(dp->dev));
DEBUGF(3,("adding dp %x to softc list %x\n", dp, softc->mappings));
	  dp->next_map = softc->mappings ;
	  softc->mappings = dp ;
	}
#endif	MULTIPROCESSOR

	return (0);
}

/*
 * Duplicate seg and return new segment in newsegp. copy original lego context.
 */
static
int
segcg6_dup(seg, newseg)
	struct seg *seg, *newseg;
{
	register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
	struct segcg6_data dev_a;

	dev_a = *sdp ;
	dev_a.link = NULL ;
	dev_a.cntxtp = NULL ;

	return segcg6_create(newseg, (caddr_t) &dev_a);
}

static
int
segcg6_unmap(seg, addr, len)
	register struct seg *seg;
	register addr_t addr;
	u_int len;
{
        register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
        register struct segcg6_data *ndp ;
        register struct seg *nseg;
        addr_t nbase;
        u_int nsize;
	void	hat_unload();
	void	hat_newseg();
        static  segcg6_lockfree();

        segcg6_lockfree(seg);

	DEBUGF(3, ("segcg6_unmap: seg=%x, addr=%x, len=%x\n", seg, addr, len));
	DEBUGF(3, ("  seg %x: base=%x, size=%x\n",
		seg, seg->s_base, seg->s_size));
	DEBUGF(3, ("  sdp %x: link=%x, flag=%x, cntxtp=%x, dev=%x, offset=%x\n",
	  sdp, sdp->link, sdp->flag, sdp->cntxtp, sdp->dev,
	  sdp->offset)) ;

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
		DEBUGF(3, ("  entire segment\n"));
 		seg_free(seg);
 		return (0);
	}

 	/*
 	 * Check for beginning of segment
 	 */

 	if (addr == seg->s_base) {
		DEBUGF(3, ("  beginning of segment\n"));
 		sdp->offset += len;
 		seg->s_base += len;
 		seg->s_size -= len;
 		return (0);
 	}

 	/*
 	 * Check for end of segment
 	 */
 	if (addr + len == seg->s_base + seg->s_size) {
		DEBUGF(3, ("  end of segment\n"));
 		seg->s_size -= len;
 		return (0);
 	}

 	/*
 	 * The section to go is in the middle of the segment,
 	 * have to make it into two segments.  nseg is made for
 	 * the high end while seg is cut down at the low end.
 	 */
	DEBUGF(3, ("  middle of segment\n"));
 	nbase = addr + len;				/* new seg base */
 	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
 	seg->s_size = addr - seg->s_base;		/* shrink old seg */
 	nseg = seg_alloc(seg->s_as, nbase, nsize);
 	if (nseg == NULL)
 		panic("segcg6_unmap seg_alloc");

 	nseg->s_ops = seg->s_ops;

 	/* figure out what to do about the new context */

 	ndp = (struct segcg6_data *)
		new_kmem_alloc(sizeof (struct segcg6_data),KMEM_NOSLEEP);
 	nseg->s_data = (caddr_t)ndp ;

	*ndp = *sdp ;
	ndp->link = NULL ;
	ndp->dev = sdp->dev ;
	ndp->offset += addr - seg->s_base ;

	if( ndp->flag & SEG_CTXINIT )
	{
	  /* this is part of the context, link it in after seg */
	  /* TODO: MT-Safe */
	  ndp->link = sdp->link ;
	  sdp->link = nseg ;
	  /* END: MT-Safe */
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

	DEBUGF(3, ("segcg6_free: seg %x: base=%x, size=%x\n",
	      seg, seg->s_base, seg->s_size));

	if (sdp->flag & SEG_VRT) {
	    if (--softc->vrtmaps == 0)
		softc->vrtflag &= ~CG6VRTCTR;
	}
        if (sdp->flag & SEG_LOCK)
            segcg6_lockfree(seg);

	/* if this segment was part of a context, remove it from the
	 * context's segment list.  If the segment list becomes empty
	 * and the context is not the shared context, remove that
	 * context from the context list.
	 */
	if( sdp->flag & SEG_CTXINIT )
	{
	  register struct segcg6_data *ptr ;
	  for( ptr = (struct segcg6_data *) &sdp->cntxtp->seg_list ;
	       ptr != NULL ;
	       ptr = (struct segcg6_data *) ptr->link->s_data )
	    if( ptr->link == seg ) {
	      ptr->link = sdp->link ;
	      break ;
	    }
	  /* did that empty the context? */
	  if( sdp->cntxtp != &shared_context && sdp->cntxtp->seg_list == NULL )
	  {
	    register struct cg6_cntxt *ctx ;

	    if( sdp->cntxtp == softc->curcntxt )
	      softc->curcntxt = NULL ;
	    for( ctx = (struct cg6_cntxt *)&shared_context.link;
		 ctx != NULL ;
		 ctx = ctx->link )
	      if( ctx->link == sdp->cntxtp ) {
		ctx->link = sdp->cntxtp->link ;
		break ;
	      }
	    kmem_free((char *)sdp->cntxtp, sizeof(*sdp->cntxtp)) ;
	  }
	}

#ifdef	MULTIPROCESSOR
	/* remove this segment from the device's global mapping list */
	if( sdp->flag & SEG_GLOBAL_MAP ) {
DEBUGF(3,("removing sdp %x from list %x->%x\n", sdp, softc, softc->mappings));
	  if( softc->mappings == sdp )
	    softc->mappings = sdp->next_map ;
	  else {
	    register struct segcg6_data *ptr ;
	    for( ptr=softc->mappings;
		 ptr != NULL && ptr->next_map != sdp;
		 ptr = ptr->next_map ) ;
	    if( ptr != NULL )
	      ptr->next_map = sdp->next_map ;
	  }
	}
#endif	MULTIPROCESSOR

	kmem_free((char *)sdp, sizeof(*sdp)) ;
	DUMP_SEGS(3, sdp->cntxtp) ;

	return(0);
}


#define CG6MAP(sp,addr,page)            \
        hat_devload(sp,                 \
                    addr,               \
                    fbgetpage((caddr_t)page),    \
                    PROT_READ|PROT_WRITE|PROT_USER,0)

#define CG6UNMAP(segp)          \
        hat_unload(segp,                \
                   (segp)->s_base,      \
                   (segp)->s_size);

/*ARGSUSED*/
static
segcg6_lockfree(seg)
struct seg *seg;
{
    register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;
    struct	seglock	*lp;
    void	segcg6_timeout();

    if ((lp = segcg6_findlock((off_t)sdp->offset))==NULL)
        return;
    lp->cr.flag |= UNGRAB;
    if (lp->sleepf) {
	untimeout((ifunc)segcg6_timeout,(caddr_t)lp);
        wakeup((caddr_t)lp->page);
    }
}

static void
segcg6_timeout(lp)
struct	seglock *lp;
{
    struct  segproc	*np;
    void	hat_devload();

    np = &lp->cr;
    if (np->flag & LOCKMAP) {
	CG6UNMAP(np->locksegp);
	np->flag &= ~LOCKMAP;
    }
    if (np->flag & UNLOCKMAP)
	CG6UNMAP(np->unlocksegp);
    CG6MAP(np->unlocksegp,np->unlockaddr,lp->page);
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
	CG6MAP(np->unlocksegp,np->unlockaddr,trashpage);
	np->flag |= (UNLOCKMAP | TRASHPAGE);
    }
    wakeup((caddr_t)lp->page);
}

/*
 * Handle lock segment faults here...
 *
 */

static
int
segcg6_lockfault(seg,addr)
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
	int	s;

        /* look up the segment in the lock_list */

        lp = segcg6_findlock((off_t)current->offset);
        if (lp == NULL)
            return (FC_MAKE_ERR(EFAULT));

	s = splsoftclock();

        if (lp->cr.flag & UNGRAB) {
            CG6MAP(seg,addr,trashpage);
	    (void) splx(s);
            return(0);
        }

        /* initialization necessary? */

        if (lp->cr.procp == u.u_procp && lp->cr.locksegp == NULL) {
            lp->cr.locksegp = seg;
            lp->cr.lockaddr = addr;
        } else
        if (lp->cr.procp != u.u_procp && lp->cl.locksegp == NULL) {
            lp->cl.procp = u.u_procp;
            lp->cl.locksegp = seg;
            lp->cl.lockaddr = addr;
        } else
        if (lp->cr.procp == u.u_procp && lp->cr.locksegp != seg) {
            lp->cr.unlocksegp = seg;
            lp->cr.unlockaddr = addr;
        } else
        if (lp->cl.procp == u.u_procp && lp->cl.locksegp != seg) {
            lp->cl.unlocksegp = seg;
            lp->cl.unlockaddr = addr;
        }

	if( *(int *)lp->page == 0) {
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
                CG6MAP(lp->owner->locksegp,lp->owner->lockaddr,lp->page);
                lp->owner->flag |= LOCKMAP;
                if (lp->owner->unlocksegp != NULL) {
                    if (lp->owner->flag & UNLOCKMAP) /***/
                        CG6UNMAP(lp->owner->unlocksegp);
                    CG6MAP(lp->owner->unlocksegp,
                           lp->owner->unlockaddr,lp->page);
                    lp->owner->flag &= ~TRASHPAGE;
                    lp->owner->flag |= UNLOCKMAP;
                }
		(void) splx(s);
                return(0);
            }
	    (void) splx(s);
            return(0);
        }

	if (*(int *)lp->page == 1) {

            if (lp->sleepf) {
                if (lp->owner->flag & UNLOCKMAP)        /***/
                    CG6UNMAP(lp->owner->unlocksegp);
                CG6MAP(lp->owner->unlocksegp,lp->owner->unlockaddr,trashpage);
                lp->owner->flag |= TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
                if (lp->owner->flag & LOCKMAP) {
                    CG6UNMAP(lp->owner->locksegp);
                    lp->owner->flag &= ~LOCKMAP;
                }
		untimeout((ifunc)segcg6_timeout,(caddr_t)lp);
                wakeup((caddr_t)lp->page);       /* wake up sleeper */
		(void) splx(s);
                return(0);
            }

            if (lp->owner->procp==u.u_procp && lp->owner->unlocksegp==seg) {
                if (lp->owner->flag & UNLOCKMAP)        /***/
                    CG6UNMAP(lp->owner->unlocksegp);
                CG6MAP(lp->owner->unlocksegp,lp->owner->unlockaddr,lp->page);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
		(void) splx(s);
                return(0);
            }

            if (lp->owner->flag & UNLOCKMAP) {
                CG6UNMAP(lp->owner->unlocksegp);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag &= ~UNLOCKMAP;
            }

            lp->sleepf = 1;

	    if (lp->cr.procp == u.u_procp)	/* creator about to sleep */
		timeout((ifunc)segcg6_timeout,(caddr_t)lp,LOCKTIME*hz);

            (void)sleep((caddr_t)lp->page,PRIBIO);  	/* goodnight, gracie */
                                        /* we wake up */
            lp->sleepf = 0;             /* clear sleepf */

            if (lp->cr.flag & UNGRAB) {
		CG6MAP(seg,addr,trashpage);
		(void) splx(s);
                return(0);
            }

            sp = lp->owner;             /* switch owner and other */
            lp->owner = lp->other;
            lp->other = sp;

            /* map new owner to page */
            CG6MAP(lp->owner->locksegp,lp->owner->lockaddr,lp->page);
            lp->owner->flag |= LOCKMAP;

            if (lp->owner->flag & TRASHPAGE) {
                CG6UNMAP(lp->owner->unlocksegp);
                lp->owner->flag &= ~TRASHPAGE;
                lp->owner->flag |= UNLOCKMAP;
                CG6MAP(lp->owner->unlocksegp,lp->owner->unlockaddr,lp->page);
            }
        }
	(void) splx(s);
	return(0);
}



/*
 * Handle a fault on a lego segment.  The only tricky part is that only
 * one valid mapping at a time is allowed.  Whenever a new mapping is
 * called for, the current values of the TEC and FBC registers in the
 * old context are saved away, and the old values in the new context
 * are restored.
 */

/*ARGSUSED*/
static
faultcode_t
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
      	int pf, s;
        register struct cg6_softc *softc = getsoftc(minor(current->dev));
	void	hat_devload();
	void	hat_unload();
	register struct fbc *fbc = CG6VA_TO_FBC(softc->base) ;

	if (type != F_INVAL) {
	    return(FC_MAKE_ERR(EFAULT));
	}

	if (current->flag & SEG_VRT) {
	    CG6MAP(seg, addr, softc->vrtpage);
	    return(0);
	}

        if (current->flag & SEG_LOCK)
	    return(segcg6_lockfault(seg,addr));

	s = splvm();
#ifdef	MULTIPROCESSOR
	xc_attention();
#endif	MULTIPROCESSOR
	if (current->cntxtp && softc->curcntxt != current->cntxtp) {
		/*
		 * time to switch lego contexts.
		 */
		if (softc->curcntxt != NULL) {
			register struct seg *segp ;

			DEBUGF(5, ( "unloading context %x, pid=%d\n",
			    softc->curcntxt, softc->curcntxt->pid)) ;

			segp = softc->curcntxt->seg_list ;
			while( segp != NULL )
			{
			    /*
			     * wipe out old valid mapping.
			     */

			    hat_unload(segp, segp->s_base, segp->s_size);
			    segp = ((struct segcg6_data *)segp->s_data)->link ;
			}

			/*
			 * switch hardware contexts: if and only if
			 * there is a legitimate context to restore.
			 */

			if( softc->curcntxt->flag & (SEG_FBC|SEG_TEC) )
			  if( !cg6_cntxsave(fbc,
				CG6VA_TO_TEC(softc->base), softc->curcntxt)) {
				  DEBUGF(1, ("cgsix: context save failed\n"));
#ifdef	MULTIPROCESSOR
				  xc_dismissed();
#endif	MULTIPROCESSOR
				  (void)splx(s);
				  return(FC_HWERR);
			  }
		}
		DEBUGF(5, ("loading context %x, pid=%d\n",
		    current->cntxtp, current->cntxtp->pid));

		if( current->cntxtp->flag & (SEG_FBC|SEG_TEC) )
		  if (!cg6_cntxrestore( fbc,
			CG6VA_TO_TEC(softc->base), current->cntxtp)) {
			      DEBUGF(1, ("cgsix: context load failed\n"));
#ifdef	MULTIPROCESSOR
			      xc_dismissed();
#endif	MULTIPROCESSOR
			      (void)splx(s);
			      return(FC_HWERR);
		      }

		/*
		 * switch software "context"
		 */
		softc->curcntxt = current->cntxtp;
	}

	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		pf = cgsixmmap(current->dev,
				(off_t)current->offset+(adr-seg->s_base),
				PROT_READ|PROT_WRITE);
		if (pf == -1) {
#ifdef	MULTIPROCESSOR
			xc_dismissed();
#endif	MULTIPROCESSOR
			(void)splx(s);
			return (FC_MAKE_ERR(EFAULT));
		}

		hat_devload(seg, adr, pf, current->prot|PROT_USER, 0 );
	}
#ifdef	MULTIPROCESSOR
	xc_dismissed();
#endif	MULTIPROCESSOR
	(void)splx(s);
	return(0);
}

/*ARGSUSED*/
static
int
segcg6_checkprot(seg, addr, len, prot)
	struct seg *seg;
	addr_t addr;
	u_int len, prot;
{
	register struct segcg6_data *sdp = (struct segcg6_data *)seg->s_data;

	return (sdp->prot & prot) == prot ? 0 : -1 ;
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
	return(0);
}

#undef L_TEC_VDC_INTRNL0
#define L_TEC_VDC_INTRNL0 0x8000
#undef L_TEC_VDC_INTRNL1
#define L_TEC_VDC_INTRNL1 0xa000


static
cg6_cntxsave(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cg6_cntxt	*saved;
{
	int	dreg;	/* counts through the data registers */
	u_int	*dp;	/* points to a tec data register */

	/*
	 * wait for fbc not busy; but not forever
	 * XXX - what *should* we do if the FBC never goes idle?
	 */

	CDELAY(!(fbc->l_fbc_status & L_FBC_BUSY), CG6_FBC_WAIT);
	if (fbc->l_fbc_status & L_FBC_BUSY) {
		printf("cgsix: cntxsave: fbc stayed busy\n");
		return 0;
	}

	DEBUGF(5, ("saving registers for %d\n", saved->pid)) ;

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
	return(1);
}

static
cg6_cntxrestore(fbc, tec, saved)
	struct fbc	*fbc;
	struct tec	*tec;
	struct cg6_cntxt	*saved;
{
	int	dreg;
	u_int	*dp;

	/*
	 * wait for fbc not busy; but not forever
	 * XXX - what *should* we do if the FBC never goes idle?
	 */

	CDELAY(!(fbc->l_fbc_status & L_FBC_BUSY), CG6_FBC_WAIT);
	if (fbc->l_fbc_status & L_FBC_BUSY) {
		printf("cgsix: cntxrestore: fbc stayed busy\n");
		return 0;
	}

	DEBUGF(5, ("restoring registers for %d\n", saved->pid)) ;

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
	return(1);
}
