#ifndef lint
static char	sccsid[] = "@(#)cgeight.c	1.1 92/07/30, SMI";
#endif

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

/*
 * Color frame buffer with overlay plane (cg8) driver
 */

#include "cgeight.h"
#include "win.h"

#include <sys/param.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/user.h>
#include <sys/vmmac.h>
#include <sun/fbio.h>
#include <sys/buf.h>
#include <sundev/mbvar.h>
#include <machine/enable.h>
#include <machine/pte.h>
#include <machine/psl.h>


#include <pixrect/pixrect.h>
#include <pixrect/pr_planegroups.h>
#include <pixrect/cg8var.h>

/*#define	CG8_DEBUGGING*/
#ifdef	CG8_DEBUGGING
#define DEBUG_PRT(x)	printf x
#define BAD_CG8() goto bad_cg8; trial_count++
#else
#define DEBUG_PRT(x)
#define BAD_CG8() goto bad_cg8
#endif

/* CG8_PHYS_ADDR is the start of P4 bus, consider zero in mmap */
#ifdef sun4
/* Nasty cobra */
#define SYSTEM_DROPPED		0xf0000000
#else
#define SYSTEM_DROPPED		0x0

#endif

#define CG8_PHYS_ADDR		(softc->cg8_physical - P4IDREG)

/* convert the offset to a pte */
#ifdef sun4
#define CG8_PHYSICAL(p)    (PGT_OBIO | btop(CG8_PHYS_ADDR + (p)))
#endif
#ifdef sun3
#define CG8_PHYSICAL(p)    (PGT_OBMEM | btop(CG8_PHYS_ADDR + (p)))
#endif
#ifdef sun3x
#define CG8_PHYSICAL(p)    (btop(CG8_PHYS_ADDR + (p)))
#endif

#define	UPDATE_PENDING(s) ((s)->cg8d.flags & CG8_UPDATE_PENDING)
#define DISABLE_INTR(s)  {                                      \
                int x;                                          \
                x = spl4();                                     \
                *(s)->p4reg &= ~P4_REG_INTEN & ~P4_REG_RESET;   \
                setintrenable(0);                               \
                (void) splx(x);                                 \
	      }

#define ENABLE_INTR(s)                                          \
    (setintrenable(1),                                          \
     (*(s)->p4reg = (*(s)->p4reg & ~P4_REG_RESET) | 		\
      P4_REG_INTCLR | P4_REG_INTEN),				\
     (*(s)->p4reg = (*(s)->p4reg & ~(P4_REG_RESET|P4_REG_INTCLR)))) \

#define	CG8_COPYIN_MAP(w, W) {                                  \
	register i; union fbunit *save_map = map;               \
	if (softc->cg8d.flags & CG8_KERNEL_UPDATE) {            \
	    bcopy((caddr_t) cmap->w, (caddr_t) cmapbuf, count); \
            error = 0;						\
	}							\
	else if (error = copyin((caddr_t) cmap->w,              \
	    (caddr_t) cmapbuf, count)) break;                   \
	map += index;                                           \
	for (i = 0; i < count; i++, map++)                      \
	    map->channel.W = cmapbuf[i]; map = save_map; }

#define	CG8_COPYOUT_MAP(w, W) {                                 \
	union fbunit *save_map = map;                           \
	map += index;                                           \
	for (i = 0; i < count; i++, map++)                      \
	    cmapbuf[i] = map->channel.W;                        \
	if (copyout((caddr_t) cmapbuf, (caddr_t) cmap->w, count)) \
	    return EFAULT; map = save_map; }

/* system enable register, used in setvideoenable (locore.s) */

#ifdef sun3x
extern u_short enablereg;
#else
extern u_char enablereg;
#endif sun3x


static struct addr_table {
    u_int           vaddr;
    u_int           paddr;
    int             size;
}               addr_table[] = {

    CG8_VADDR_DAC, DACBASE, ctob (1),
    CG8_VADDR_P4REG, P4IDREG, ctob (1),
    CG8_VADDR_PROM, PROMBASE, PROMSIZE,
    CG8_VADDR_FB, OVLBASE, 0,
    0, ENABASE, 0,
    0, FBMEMBASE, 0,
};

#define ADDR_TABLE_SIZE	6

/* driver per-unit data */
static struct cg8_softc {
    int             w,
                    h;		       /* resolution */
    int             size;	       /* total size of frame buffer */
    int             bw2size;	       /* total size of overlay plane */
    struct proc    *owner;	       /* owner of the frame buffer */
    struct fbgattr  gattr;	       /* current attributes */
    u_long          cg8_physical;
    short          *ovlptr,
                   *enptr;
#   define CG8_CMAP_SIZE 256
#   define CG8_OMAP_SIZE 4
    union fbunit    cmap_rgb[CG8_CMAP_SIZE];
    int			cmap_begin;
    int			cmap_end;
    union fbunit	omap_rgb[CG8_OMAP_SIZE];
    int			omap_begin;
    int			omap_end;
    u_char		em_red[CG8_CMAP_SIZE], em_green[CG8_CMAP_SIZE],
			    em_blue[CG8_CMAP_SIZE]; /* indexed cmap emulation */
    u_int	    *p4reg;
    struct ramdac   *lut;
#if NWIN > 0
    Pixrect         pr;
#endif NWIN > 0
    struct cg8_data cg8d;
    u_long          kmx;	       /* kernel map entry */
}               cg8_softc[NCGEIGHT];


#define	CG8_PROBESIZE	(NBPG+NBPG)

/* Mainbus device data */
int             cgeightprobe (), cgeightattach (), cgeightpoll ();
struct mb_device *cgeightinfo[NCGEIGHT];
struct mb_driver cgeightdriver = {
    cgeightprobe, 0, cgeightattach, 0, 0, cgeightpoll,
    CG8_PROBESIZE, "cgeight", cgeightinfo, 0, 0, 0, 0
};

/* default structure for FBIOGTYPE ioctl */
static struct fbtype cg8typedefault = {
/*	type           h  w  depth cms size */
    FBTYPE_SUN2BW, 0, 0, 1, 0, 0
};

/* default structure for FBIOGATTR ioctl */
static struct fbgattr cg8attrdefault = {
/*	real_type         owner */
    FBTYPE_MEMCOLOR, 0,
/* fbtype: type             h  w  depth cms  size */
    {FBTYPE_MEMCOLOR, 0, 0, 32, 256, 0},
/* fbsattr: flags           emu_type       dev_specific */
    {FB_ATTR_AUTOINIT, FBTYPE_SUN2BW, {0}},
/*        emu_types */
    {FBTYPE_MEMCOLOR, FBTYPE_SUN2BW, -1, -1}
};

/* frame buffer description table */
static struct cg8_fbdesc {
     short           depth;	       /* depth, bits */
    short           group;	       /* plane group */
    int             allplanes;	       /* initial plane mask */
}               cg8_fbdesc[CG8_NFBS + 1] = {
    { 1, PIXPG_OVERLAY, 0 },
    { 1, PIXPG_OVERLAY_ENABLE, 0 },
    { 32, PIXPG_24BIT_COLOR, 0xffffff } };

#define	CG8_FBINDEX_OVERLAY	0
#define	CG8_FBINDEX_ENABLE	1
#define	CG8_FBINDEX_COLOR	2

/* initial active frame buffer */
#ifndef	CG8_INITFB
#define	CG8_INITFB	CG8_FBINDEX_OVERLAY
#endif

#if NWIN > 0

/* SunWindows specific stuff */

/* kernel pixrect ops vector */
struct pixrectops cg8_ops = {
    mem_rop,
    cg8_putcolormap,
    cg8_putattributes,
#   ifdef _PR_IOCTL_KERNEL_DEFINED
    cg8_ioctl
#   endif
};
#endif NWIN > 0

/*
 * Determine if a cg8 exists at the given address.
 */
/*ARGSUSED*/
cgeightprobe (reg, unit)
    caddr_t         reg;	       /* address of P4REG */
    int             unit;
{
    register struct cg8_softc *softc = &cg8_softc[unit];
    u_int           hat_getkpfnum ();
    u_long          rmalloc ();
    int             pages1;
    int             id;
    int s;
#   ifdef CG8_DEBUGGING
    int             trial_count = 0;
#   endif

    /*
     * Determine frame buffer resolution:
     */
    DEBUG_PRT (("cgeightprobe: debug turned on\n"));
    id = p4probe (reg, &softc->w, &softc->h);
    DEBUG_PRT (("cgeightprobe: p4id = 0x%x\n", id));
    if (id < 0)
	return 0;

    if (id != P4_ID_COLOR24)
	return 0;

    /*
     * reset it, and assign the old value back.  probe may be called after
     * the boot prom has probed it.  To avoid losint the video halfway
     * booting, we set it to the old value.
     */
    softc->p4reg = (u_int *) reg;
    
    *softc->p4reg = (*softc->p4reg & ~P4_REG_RESET) | P4_REG_SYNC;
    *softc->p4reg &= ~P4_REG_SYNC & ~P4_REG_RESET; /* then write zero to sync it */
    /* physical address is the page number and the offset */
    softc->cg8_physical = ((u_long) hat_getkpfnum (reg) << PAGESHIFT) |
	((u_long) reg & PAGEOFFSET);
#ifndef sun3x
    softc->cg8_physical |= SYSTEM_DROPPED;	/* cobra drops it */
#endif

    (void) printf ("cgeightprobe:  %d x %d\n", softc->w, softc->h);
    reg += NBPG;
    fbmapin (reg, (int) CG8_PHYSICAL (DACBASE), NBPG);
    softc->lut = (struct ramdac *) reg;

    /* enough to map in a one-bit plane (btoc round up to pagesize) */
    pages1 = btoc (mpr_linebytes (softc->w, 1) * softc->h);
    softc->bw2size = ctob (pages1);    /* get back # of bytes */
    /* map frame buffer sections */
    {
	register caddr_t fbaddr;
	long             poke_value;

	/*
	 * cg8 is too big for kernel to handle, we'll mapin one by one and
	 * poke the selected few addresses.
	 */
	s = splhigh();
	softc->kmx = rmalloc(kernelmap, (long) (pages1 + pages1));
	(void)splx(s);
	if (softc->kmx == 0) {
	    (void) printf ("cgeightprobe: out of kernelmap\n");
	    return 0;		       /* must free from now on */
	}

	/* compute virtual addr. of first section */
	fbaddr = (caddr_t) kmxtob (softc->kmx);

	fbmapin (fbaddr, (int) CG8_PHYSICAL (FBMEMBASE), ctob (pages1));
	if (peekl ((long *) fbaddr, &poke_value) ||
	    pokel ((long *) fbaddr, ~poke_value) ||
	    pokel ((long *) fbaddr, poke_value))/* restore old value */
	    BAD_CG8();
	fbmapin (fbaddr, (int) CG8_PHYSICAL (FBMEMBASE + FBMEM_SIZE - pages1),
		 ctob (pages1));
	if (peekl ((long *) fbaddr, &poke_value) ||
	    pokel ((long *) fbaddr, ~poke_value) ||
	    pokel ((long *) fbaddr, poke_value))
	    BAD_CG8();

	/* map each frame buffer section */
	fbmapin (fbaddr, (int) CG8_PHYSICAL (OVLBASE), ctob (pages1));
	if (peekl ((long *) fbaddr, &poke_value) ||
	    pokel ((long *) fbaddr, ~poke_value) ||
	    pokel ((long *) fbaddr, poke_value))
	    BAD_CG8();

	softc->ovlptr = (short *) fbaddr;
	fbaddr += softc->bw2size;

	fbmapin (fbaddr, (int) CG8_PHYSICAL (ENABASE), ctob (pages1));
	if (peekl ((long *) fbaddr, &poke_value) ||
	    pokel ((long *) fbaddr, ~poke_value) ||
	    pokel ((long *) fbaddr, poke_value))
	    BAD_CG8();
	softc->enptr = (short *) fbaddr;
    }

    /* probe succeeded */
    return CG8_PROBESIZE;

bad_cg8:
    /* this section missing, map out everything */
    DEBUG_PRT (("cgeightprobe: fail at %d\n", trial_count));
    s = splhigh();
    rmfree(kernelmap, (long) pages1, softc->kmx);
    (void)splx(s);
    return (0);
}


cgeightattach (md)
    register struct mb_device *md;
{
    register struct cg8_softc *softc = &cg8_softc[md->md_unit];

    DEBUG_PRT(("cgeightattach.\n"));
    softc->cg8d.flags = 0;
    softc->owner = NULL;

    softc->gattr = cg8attrdefault;
    softc->gattr.fbtype.fb_height = softc->h;
    softc->gattr.fbtype.fb_width = softc->w;
    softc->gattr.fbtype.fb_size = softc->bw2size;


    /*
     * initialize the Brooktree ramdac. The addr_reg selects which register
     * inside the ramdac will be affected.
     */

    INIT_BT458(softc->lut);
    INIT_OCMAP(softc->omap_rgb);
    INIT_CMAP(softc->cmap_rgb, CG8_CMAP_SIZE);

    softc->omap_begin = 0;
    softc->omap_end = 4;
    softc->cmap_begin = 0;
    softc->cmap_end = CG8_CMAP_SIZE;
    softc->cg8d.flags |=
	CG8_OVERLAY_CMAP | CG8_UPDATE_PENDING | CG8_24BIT_CMAP;
    (void) cgeightpoll();	       /* intr disabled */

    addr_table[3].size = softc->bw2size;
    addr_table[4].vaddr = addr_table[3].vaddr + addr_table[3].size;
    addr_table[4].size = softc->bw2size;
    addr_table[5].vaddr = addr_table[4].vaddr + addr_table[4].size;
    addr_table[5].size = ctob(btoc(mpr_linebytes(softc->w, 32) * softc->h));
}


cgeightopen (dev, flag)
    dev_t           dev;
    int             flag;
{
    DEBUG_PRT (("cgeightopen: called minor = %d, flag = %d\n",
		minor (dev), flag));
    return fbopen (dev, flag, NCGEIGHT, cgeightinfo);
}

/*ARGSUSED*/
cgeightclose (dev, flag)
    dev_t           dev;
    int             flag;
{
    DEBUG_PRT (("cgeightclose: called minor = %d, flag = %d\n",
		minor (dev), flag));
    return 0;
}

/*ARGSUSED*/
cgeightmmap (dev, off, prot)
    dev_t           dev;
    off_t           off;
    int             prot;
{
    register struct cg8_softc *softc = &cg8_softc[minor (dev)];
    register int    i;
    register struct addr_table *tbl;
    if (off >= 0 && off < softc->bw2size)	    /* bw2 emulation */
	return (CG8_PHYSICAL(off + OVLBASE));

    for (i = 0, tbl = addr_table; i < ADDR_TABLE_SIZE; i++, tbl++) 
	if (off >= tbl->vaddr && off < (tbl->vaddr + tbl->size)) {
	    off -= tbl->vaddr;
	    break;
	}

    return i < ADDR_TABLE_SIZE ? CG8_PHYSICAL ((u_int) off + tbl->paddr) : -1;

}


/*ARGSUSED*/
cgeightioctl (dev, cmd, data, flag)
    dev_t           dev;
    int             cmd;
    caddr_t         data;
    int             flag;
{
    register struct cg8_softc *softc = &cg8_softc[minor (dev)];

#   ifdef CG8_DEBUGGING
    DEBUG_PRT ( ("cgeightioctl: cmd = "));
    switch (cmd) {
    case FBIOSATTR: DEBUG_PRT ( ("FBIOSATTR\n")); break;
    case FBIOGATTR: DEBUG_PRT ( ("FBIOGATTR\n")); break;
    case FBIOGETCMAP: DEBUG_PRT ( ("FBIOGETCMAP\n")); break;
    case FBIOPUTCMAP: DEBUG_PRT ( ("FBIOPUTCMAP\n")); break;
    case FBIOGTYPE: DEBUG_PRT ( ("FBIOGTYPE\n")); break;
    case FBIOGPIXRECT: DEBUG_PRT ( ("FBIOGPIXRECT\n")); break;
    case FBIOSVIDEO: DEBUG_PRT ( ("FBIOSVIDEO\n")); break;
    case FBIOGVIDEO: DEBUG_PRT ( ("FBIOGVIDEO\n")); break;
    default: DEBUG_PRT ( ("not supported (0x%x)\n", cmd)); break;
    }
#   endif CG8_DEBUGGING
    switch (cmd) {
    case FBIOGETCMAP:
    case FBIOPUTCMAP:
	    {
		struct	fbcmap	*cmap = (struct fbcmap *) data;
		u_int		index = (u_int) cmap->index;
		u_int		count = (u_int) cmap->count;
		int		pgroup = PIX_ATTRGROUP (index);
		union	fbunit	*map;
		union	fbunit	*lutptr;
		int		*begin;
		int		*end;
		u_int		entries;
		int		i,j;
		u_int		intr_flag;
		u_char          tmp[3][CG8_CMAP_SIZE];


		if (count == 0 || !cmap->red || !cmap->green || !cmap->blue)
		    return 0;

		/* The most significant 7 bits is the group number. */
		switch (pgroup) {
		case 0:
		case PIXPG_24BIT_COLOR:
		    lutptr	= &softc->lut->lut_data;
		    begin	= &softc->cmap_begin;
		    end		= &softc->cmap_end;
		    map		= softc->cmap_rgb;
		    intr_flag	= CG8_24BIT_CMAP;
		    entries	= CG8_CMAP_SIZE;
		    break;

		case PIXPG_OVERLAY:
		    lutptr	= &softc->lut->overlay;
		    begin	= &softc->omap_begin;
		    end		= &softc->omap_end;
		    map		= softc->omap_rgb;
		    intr_flag	= CG8_OVERLAY_CMAP;
		    entries	= CG8_OMAP_SIZE;
		    break;

		default:
		    return EINVAL;
		}

		if ((index &= PIX_ALL_PLANES) >= entries ||
		    index + count > entries)
		    return EINVAL;

		if (cmd == FBIOPUTCMAP) {

		    if (softc->cg8d.flags & CG8_KERNEL_UPDATE)
		    {
			DEBUG_PRT(("kernel putcmap\n",0));
			softc->cg8d.flags &= ~CG8_KERNEL_UPDATE;
			bcopy((caddr_t)cmap->red,(caddr_t)tmp[0],count);
			bcopy((caddr_t)cmap->green,(caddr_t)tmp[1],count);
			bcopy((caddr_t)cmap->blue,(caddr_t)tmp[2],count);
		    }
		    else
		    {
			DEBUG_PRT(("user putcmap\n",0));
			if (copyin((caddr_t)cmap->red,(caddr_t)tmp[0],count) ||
			    copyin((caddr_t)cmap->green,(caddr_t)tmp[1],count)||
			    copyin((caddr_t)cmap->blue,(caddr_t)tmp[2],count))
			    return EFAULT;
		    }
 
		    if (!(cmap->index & PR_FORCE_UPDATE) &&
			intr_flag == CG8_24BIT_CMAP)        /* emulated index */
		    {
			for (i=0; i<count; i++,index++)
			{
			    softc->em_red[index] = tmp[0][i];
			    softc->em_green[index] = tmp[1][i];
			    softc->em_blue[index] = tmp[2][i];
			}
			return 0;
		    }
		    
		    softc->cg8d.flags |= intr_flag;
		    *begin = MIN(*begin,index);
		    *end = MAX(*end,index+count);
		    DEBUG_PRT(("pcmap: f=%d ",softc->cg8d.flags));
		    DEBUG_PRT(("b=%d ",*begin));
		    DEBUG_PRT(("e=%d ",*end));
		    DEBUG_PRT(("i=%d ",index));
		    DEBUG_PRT(("c=%d\n",count));
		     
		    for (i=0,map+=index; count; i++,count--,map++)
			map->packed =
			    tmp[0][i]|(tmp[1][i]<<8)|(tmp[2][i]<<16);
		     
		    ENABLE_INTR(softc);
		    break;
		}
		else {		       /* FBIOGETCMAP */
		    if (!(cmap->index & PR_FORCE_UPDATE) &&
			intr_flag == CG8_24BIT_CMAP)        /* emulated index */
		    {         
			for (i=0; i<count; i++,index++)
			{     
			    tmp[0][i] = softc->em_red[index];
			    tmp[1][i] = softc->em_green[index];
			    tmp[2][i] = softc->em_blue[index];
			}     
		    }         
		    else      
		    {         
			if (softc->cg8d.flags & intr_flag)
			    cgeight_wait(minor(dev));
			ASSIGN_LUT(softc->lut->addr_reg, index);
			for (i=0,j=count; j; i++,j--)
			{     
			    tmp[0][i] = (lutptr->packed & 0x0000ff);
			    tmp[1][i] = (lutptr->packed & 0x00ff00)>>8;
			    tmp[2][i] = (lutptr->packed & 0xff0000)>>16;
			}     
		    }         
		    if (copyout((caddr_t)tmp[0],(caddr_t)cmap->red,count) ||
			copyout((caddr_t)tmp[1],(caddr_t)cmap->green,count) ||
			copyout((caddr_t)tmp[2],(caddr_t)cmap->blue,count))
			return EFAULT;
		    break;
		}
	    }
    case FBIOSATTR:{
	    register struct fbsattr *sattr = (struct fbsattr *) data;

#ifdef ONLY_OWNER_CAN_SATTR
	    /* this can only happen for the owner */
	    if (softc->owner != u.u_procp)
		return ENOTTY;
#endif ONLY_OWNER_CAN_SATTR

	    softc->gattr.sattr.flags = sattr->flags;

	    if (sattr->emu_type != -1)
		softc->gattr.sattr.emu_type = sattr->emu_type;

	    if (sattr->flags & FB_ATTR_DEVSPECIFIC) {
		bcopy ((char *) sattr->dev_specific,
		       (char *) softc->gattr.sattr.dev_specific,
		       sizeof sattr->dev_specific);

		if (softc->gattr.sattr.dev_specific
		    [FB_ATTR_CG8_SETOWNER_CMD] == 1) {
		    struct proc    *pfind ();
		    register struct proc *newowner = 0;

		    if (softc->gattr.sattr.dev_specific
			[FB_ATTR_CG8_SETOWNER_PID] > 0 &&
			(newowner = pfind (
					   softc->gattr.sattr.dev_specific
					   [FB_ATTR_CG8_SETOWNER_PID]))) {
			softc->owner = newowner;
			softc->gattr.owner = newowner->p_pid;
		    }
		    softc->gattr.sattr.dev_specific
			[FB_ATTR_CG8_SETOWNER_CMD] = 0;
		    softc->gattr.sattr.dev_specific
			[FB_ATTR_CG8_SETOWNER_PID] = 0;

		    if (!newowner)
			return ESRCH;
		}
	    }
	}
	break;

    case FBIOGATTR:{
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

    case FBIOGTYPE:{
	    register struct fbtype *fb = (struct fbtype *) data;

	    /* set owner if not owned or previous owner is dead */
	    if (softc->owner == NULL ||
		softc->owner->p_stat == NULL ||
		softc->owner->p_pid != softc->gattr.owner) {

		softc->owner = u.u_procp;
		softc->gattr.owner = u.u_procp->p_pid;
	    }

	    switch (softc->gattr.sattr.emu_type) {
	    case FBTYPE_SUN2BW:
		*fb = cg8typedefault;
		fb->fb_height = softc->h;
		fb->fb_width = softc->w;
		fb->fb_size = softc->bw2size;
		break;

	    default:
		*fb = softc->gattr.fbtype;
		break;
	    }
	}
	break;

#if NWIN > 0

    case FBIOGPIXRECT:{
	    struct fbpixrect *fbpr = (struct fbpixrect *) data;
	    register struct cg4fb *fbp;
	    register struct cg8_fbdesc *descp;
	    register int    i;

	    /* "Allocate" pixrect and private data */
	    fbpr->fbpr_pixrect = &softc->pr;
	    softc->pr.pr_data = (caddr_t) & softc->cg8d;

	    /* initialize pixrect */
	    softc->pr.pr_ops = &cg8_ops;
	    softc->pr.pr_size.x = softc->w;
	    softc->pr.pr_size.y = softc->h;

	    /* initialize private data */
	    softc->cg8d.flags = 0;
	    softc->cg8d.planes = 0;
	    softc->cg8d.fd = minor (dev);
	    softc->cg8d.num_fbs = CG8_NFBS;

	    /* set up everything except for the image pointer */
	    for (fbp = softc->cg8d.fb, descp = cg8_fbdesc, i = 0;
		 i < CG8_NFBS; fbp++, descp++, i++) {
		fbp->group = descp->group;
		fbp->depth = descp->depth;
		fbp->mprp.mpr.md_linebytes =
		    mpr_linebytes (softc->w, descp->depth);
		fbp->mprp.mpr.md_offset.x = 0;
		fbp->mprp.mpr.md_offset.y = 0;
		fbp->mprp.mpr.md_primary = 0;
		fbp->mprp.mpr.md_flags = descp->allplanes != 0
		    ? MP_DISPLAY | MP_PLANEMASK
		    : MP_DISPLAY;
		fbp->mprp.planes = descp->allplanes;
		DEBUG_PRT(("ioctl GPIXRECT group %d depth %d\n",
		    fbp->group, fbp->depth));
	    }

	    /*
	     * switch to each plane group, set up image pointers and
	     * initialzed the images
	     */
	    {
		u_int             initplanes;
		Pixrect        *tmppr = &softc->pr;

		initplanes = PIX_GROUP (cg8_fbdesc[CG8_FBINDEX_COLOR].group) |
		    cg8_fbdesc[CG8_FBINDEX_COLOR].allplanes;

		(void) cg8_putattributes (tmppr, &initplanes);
		cg8_d (tmppr)->fb[CG8_FBINDEX_COLOR].mprp.mpr.md_image =
		    cg8_d (tmppr)->mprp.mpr.md_image = (short *) -1;
		cg8_d (tmppr)->mprp.mpr.md_linebytes = 0;


		initplanes = PIX_GROUP (cg8_fbdesc[CG8_FBINDEX_ENABLE].group) |
		    cg8_fbdesc[CG8_FBINDEX_ENABLE].allplanes;
		(void) cg8_putattributes (tmppr, &initplanes);
		DEBUG_PRT(("ioctl GPIXRECT pr_depth = %d", tmppr->pr_depth));

		cg8_d (tmppr)->fb[CG8_FBINDEX_ENABLE].mprp.mpr.md_image =
		    cg8_d (tmppr)->mprp.mpr.md_image = softc->enptr;
		pr_rop (&softc->pr, 0, 0,
			softc->pr.pr_size.x, softc->pr.pr_size.y,
			PIX_SRC | PIX_COLOR (1), (Pixrect *) 0, 0, 0);

		initplanes = PIX_GROUP (cg8_fbdesc[CG8_INITFB].group) |
		    cg8_fbdesc[CG8_INITFB].allplanes;
		(void) cg8_putattributes (&softc->pr, &initplanes);
		DEBUG_PRT(("ioctl GPIXRECT pr_depth = %d\n", tmppr->pr_depth));

		cg8_d (tmppr)->fb[CG8_INITFB].mprp.mpr.md_image =
		    cg8_d (tmppr)->mprp.mpr.md_image = softc->ovlptr;
		pr_rop (tmppr, 0, 0,
			softc->pr.pr_size.x, softc->pr.pr_size.y,
			PIX_SRC, (Pixrect *) 0, 0, 0);

	    }

	    /* enable video */
	    if (softc->p4reg)
		*softc->p4reg = (*softc->p4reg & ~P4_REG_RESET) | 
		    P4_REG_VIDEO;
	    else
		setvideoenable (1);
	}
	break;

#endif NWIN > 0

    case FBIOSVIDEO:{
	    register int    on = *(int *) data & FBVIDEO_ON;

	    if (softc->p4reg)
		*softc->p4reg = *softc->p4reg & ~P4_REG_RESET &
		    ~(P4_REG_INTCLR | P4_REG_VIDEO) |
		    (on ? P4_REG_VIDEO : 0);
	    else
		setvideoenable ((u_int) on);
	}
	break;

	/* get video flags */
    case FBIOGVIDEO:
	*(int *) data =
	    (softc->p4reg ? *softc->p4reg & P4_REG_VIDEO :
	     enablereg & ENA_VIDEO) ? FBVIDEO_ON : FBVIDEO_OFF;
	break;

    default:
	return ENOTTY;

    }				       /* switch(cmd) */
    return 0;
}

cgeight_wait(unit)
int     unit;
{
    register    struct  mb_device       *md = cgeightinfo[unit & 255];
    register    struct  cg8_softc       *softc = &cg8_softc[unit & 255];
    int         s;               
            
    DEBUG_PRT(("cgeight wait, entered\n", 0));
         
    s = splx(pritospl(md->md_intpri));
    softc->cg8d.flags |= CG8_SLEEPING;
    ENABLE_INTR(softc);
    (void) sleep((caddr_t) softc, PZERO - 1);
    (void) splx(s);
    DEBUG_PRT(("cgeight wait, waken up\n", 0));
}

#define	ENDOF(array)	((array) + (sizeof (array) / sizeof (array)[0]))
#define WAIT_TOO_LONG 70

int
cgeightpoll ()
{
    register int    i;
    register union fbunit *lutptr;
    register union fbunit *map;
    register struct cg8_softc *softc;
    register int    serviced = 0;

    for (softc = cg8_softc; softc < ENDOF (cg8_softc); softc++)
    {
	DISABLE_INTR (softc);

	if (softc->cg8d.flags & CG8_OVERLAY_CMAP)
	{
	    DEBUG_PRT(("overlay cmap s=%d",softc->omap_begin));
	    DEBUG_PRT((" e=%d\n",softc->omap_end));
	    softc->cg8d.flags &= ~CG8_OVERLAY_CMAP;
	    ASSIGN_LUT(softc->lut->addr_reg, softc->omap_begin);
	    lutptr = &softc->lut->overlay;
	    map = softc->omap_rgb+softc->omap_begin;
	    for (i=softc->omap_begin; i<softc->omap_end; i++,map++)
	    {
		lutptr->packed = map->packed;
		lutptr->packed = map->packed;
		lutptr->packed = map->packed;
	    }
	    softc->omap_begin = CG8_OMAP_SIZE;
	    softc->omap_end = 0;
	    serviced++;
	}
	if (softc->cg8d.flags & CG8_24BIT_CMAP)
	{
	    DEBUG_PRT(("cmap s=%d",softc->cmap_begin));
	    DEBUG_PRT((" e=%d\n",softc->cmap_end));
	    softc->cg8d.flags &= ~CG8_24BIT_CMAP;
	    ASSIGN_LUT(softc->lut->addr_reg, softc->cmap_begin);
	    lutptr = &softc->lut->lut_data;
	    map = softc->cmap_rgb+softc->cmap_begin;
	    for (i=softc->cmap_begin; i<softc->cmap_end; i++, map++)
	    {
		lutptr->packed = map->packed;
		lutptr->packed = map->packed;
		lutptr->packed = map->packed;
	    }
	    softc->cmap_begin = CG8_CMAP_SIZE;
	    softc->cmap_end = 0;
	    serviced++;
	}

	if (softc->cg8d.flags & CG8_SLEEPING)
	{   
	    softc->cg8d.flags &= ~CG8_SLEEPING;
	    wakeup((caddr_t) softc);
	}   
    }

    return serviced;
}
