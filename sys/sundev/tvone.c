
#include "tvone.h"
#if NTVONE > 0

#ifndef lint
static char sccsid[] = "@(#)tvone.c	1.1 92/07/30 SMI";
#endif

/*
 * Copyright 1988 by Sun Microsystems, Inc.
 */

/*
 * Sun Video Display Board
 */

#include "win.h"

#include <sys/param.h>
#include <sys/buf.h>
#include <sys/conf.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/map.h>
#include <sys/vmmac.h>
#include <sys/user.h>

#include <sun/fbio.h>	    /* FBTYPE_SUNFB_VIDEO defined here */
#include <sun/tvio.h>

#include <machine/pte.h>
#include <machine/mmu.h>
#include <machine/cpu.h>
#include <machine/psl.h>

#include <sundev/mbvar.h>
#include <sundev/tv1reg.h>

#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>
#include <pixrect/tv1var.h>
#include <pixrect/pr_planegroups.h>


#ifndef sun2
#include <machine/eeprom.h>
#include <machine/enable.h>
#endif !sun2

#define	TV1_PROBESIZE	NBPG
#define	TV1_DEVNUM(d)   (0) /*(minor(d)) & 0x7F)*/
#define	U_CHAR(i)	(((int)(i)) < 0 ? 0 : (((int)(i)) > 255 ? 255 : (i)))
#define	TV1_BOUND_DEV(dev) (tv1_softc[TV1_DEVNUM(dev)].new_minor ? \
				 makedev(major(tv1_softc[TV1_DEVNUM(dev)].tv1_bound_dev), minor(dev)) : \
				tv1_softc[TV1_DEVNUM(dev)].tv1_bound_dev)
/*
 * Driver information for auto-configuration stuff.
 */
int             tvoneprobe(), tvoneattach(), tvoneintr();
struct mb_device *tvoneinfo[NTVONE];
struct mb_driver tvonedriver = {
    tvoneprobe, 0, tvoneattach, 0, 0, tvoneintr,
    TV1_PROBESIZE, "tvone", tvoneinfo, 0, 0, 0,
};

#if NWIN > 0
struct pixrectops tv1_ops;
#endif NWIN > 0

#ifdef sun3x
#define	TV1_PHYSICAL(off) (btop(softc->tv1_baseaddr + off))
#else
#define	TV1_PHYSICAL(off) (btop(softc->tv1_baseaddr + off) | PGT_VME_D32)
#endif
/*
 * Uses cdevsw to get at bound device
 */

extern struct cdevsw cdevsw[];


struct tv1_softc {
    u_long          tv1_baseaddr;
    struct csr	    *tv1_csr;
    struct csr	    shadow_csr;
    int		    grab_proc;
    dev_t	    tv1_bound_dev;
    int		    new_minor;
#if NWIN > 0
    Pixrect         pr;
    struct tv1_data prd;
#endif
    struct tv1_enable_plane *tv1_enable_plane;
    int			tv1_horiz, tv1_y;
    int		video_on;
    union tv1_nvram	tv1_cal;
    struct  adjustment_biases{
	int red_gain,
	red_black,
	green_gain,
        green_black,
        blue_gain,
        blue_black,
	luma_gain,
	chroma_gain;
    } adjustment_biases;
    struct tv1_rgb_gain_black *current_cal;
}               tv1_softc[NTVONE];

tvoneopen(dev, flag, newdev)
    dev_t           dev;
    int             flag;
    dev_t *newdev;
{
    dev_t new_bound_dev;
    struct tv1_softc *softc = &tv1_softc[TV1_DEVNUM(dev)];
    int r;

    if (softc->tv1_bound_dev) { /* open bound device */
	new_bound_dev = softc->tv1_bound_dev;
	/* Open bound device */
	if ((r = (cdevsw[major(softc->tv1_bound_dev)].d_open(
	    softc->tv1_bound_dev, flag, &new_bound_dev))) < 0) {
		return (r);
	    }
	if (new_bound_dev != softc->tv1_bound_dev) {
	    /* Bound device is playing games with us */
	    /* (i.e. it is a GP). Play along and switch our minor too */
	    *newdev = makedev(major(dev), minor(new_bound_dev));
	    softc->new_minor = 1; /* so we know to switch minors */
	}
    } 
    return (fbopen(dev, flag, NTVONE, tvoneinfo));
}

tvoneclose(dev, flag)
    dev_t           dev;
    int             flag;
{
    struct tv1_softc *softc = &tv1_softc[TV1_DEVNUM(dev)];
    if (softc->tv1_bound_dev) {
	/* close bound device */
	return (cdevsw[major(TV1_BOUND_DEV(dev))].d_close(
	    TV1_BOUND_DEV(dev) , flag));
    } else {
	return (0);
    }
}

tvonemmap(dev, off, prot)
    dev_t           dev;
    off_t           off;
    int             prot;
{
    register struct tv1_softc *softc = &tv1_softc[TV1_DEVNUM(dev)];
    static tvonemmap_debug = 1;

    /*
     * Since this gets called to map in both the bound device, and the 
     * video board, we cheat by setting the high bit on the offset when
     * we intend to map in the video board. */
    if ((!softc->tv1_bound_dev) || (off & 0x80000000)) {
	/* we are not bound, or high bit in offset is set, mmap video*/
	off &= 0x7FFFFFF;
	if (off > ctob(btoc((off_t) sizeof (struct tv1)))) {
	    if (tvonemmap_debug) {
		printf("tvonemmap: off(%x) exceeds tv1(%x)\n", off,
		        sizeof (struct tv1));
	    }
	    return (-1);
	}
#ifdef sun3x
	return (btop(softc->tv1_baseaddr + off));
#else
	return (PGT_VME_D32 | btop(softc->tv1_baseaddr + off));
#endif
    } else { /* Call frame buffer's mmap routine */
	return (cdevsw[major(TV1_BOUND_DEV(dev))].d_mmap(
	    TV1_BOUND_DEV(dev), off, prot));
    }
}


tvoneioctl(dev, cmd, data, flag)
    dev_t           dev;
    int             cmd;
    caddr_t         data;
    int             flag;
{
   register struct tv1_softc *softc = &tv1_softc[TV1_DEVNUM(dev)];
   struct csr     *shadow = &tv1_softc[TV1_DEVNUM(dev)].shadow_csr,
                  *tvcsr = tv1_softc[TV1_DEVNUM(dev)].tv1_csr;
    switch(cmd) {
    case TVIOSBIND:
	softc->tv1_bound_dev = *((dev_t *) data);
	softc->new_minor = 0; /* Let the next call to open handle this */
	break;
    case TVIOGBIND:
	*((dev_t *) data) = softc->tv1_bound_dev;
	break;
    case TVIOSOUT:
	switch (*(int *)data) {
	    case 0: 
	    case 1:
	    case 2:
	        shadow->general.field.show525 = ((*(int *)data) >>1) & 1;
		shadow->general.field.showup = (*(int *)data) & 01;
		tvcsr->general.packed = shadow->general.packed;
		break;
	    default:
		return (EINVAL);
	}
	break;
    case TVIOGOUT:
	*(int *)data = shadow->general.field.show525 << 1 |
	               shadow->general.field.showup;
	break;
    case TVIOSLIVE:
	switch (*(int *)data) {
	    case 0: 
	    case 1:
		shadow->general.field.live525 = (*(int *)data);
		tvcsr->general.packed = shadow->general.packed;
		break;
	    default:
		return (EINVAL);
	}
	break;
    case TVIOGLIVE:
	*(int *)data = shadow->general.field.live525;
	break;
    case TVIOSYNC:
	/* Synchronize AB state to IB state */
	shadow->control.field.abstate = tvcsr->general.field.ibstate;
	tvcsr->control.packed = shadow->control.packed;
	break;
    case TVIOGBTYPE:	/* get bound frame buffer's real type */
	{
	    struct fbtype boundtype;
	    struct fbgattr boundattr;
	    int retval = 0;

	    if (softc->tv1_bound_dev) {
		if (cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		    TV1_BOUND_DEV(dev), FBIOGATTR, &boundattr, flag)) {
			/* Get attribute failed, try get type */
			retval = cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
			    TV1_BOUND_DEV(dev), FBIOGTYPE, &boundtype, flag);
			    *(int *)data = boundtype.fb_type;
		}
		*(int *)data = boundattr.real_type;
		return (retval);
	    } else {
		return (ENODEV);
	    }

	}
	/*NOTREACHED*/
	break;    
    case FBIOGTYPE:{
	    int r = 0;

	    register struct fbtype *fbtype = (struct fbtype *) data;
	    struct fbtype boundtype;
	    if (softc->tv1_bound_dev) {
		r = cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		    TV1_BOUND_DEV(dev), cmd, &boundtype, flag);
		fbtype->fb_type = boundtype.fb_type;	/* Lie */
		fbtype->fb_height = boundtype.fb_height;
		fbtype->fb_width = boundtype.fb_width;
		fbtype->fb_depth =  boundtype.fb_depth;
		fbtype->fb_size = boundtype.fb_size;
		fbtype->fb_cmsize = boundtype.fb_cmsize;
		return (r);
	    } else {
		fbtype->fb_type = FBTYPE_SUNFB_VIDEO;
	    }
	    break;
	}
    case FBIOGATTR:{
	    register struct fbgattr *gattr = (struct fbgattr *) data;
	    struct fbtype boundtype;
	    int last, i;

	    struct fbgattr boundattr;
	    if (softc->tv1_bound_dev) {
		/* get bound device's information */
		if (cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		    TV1_BOUND_DEV(dev), cmd, &boundattr, flag) == 0) {
			gattr->fbtype.fb_type =  FBTYPE_SUNFB_VIDEO;
			gattr->fbtype.fb_height = boundattr.fbtype.fb_height;
			gattr->fbtype.fb_width = boundattr.fbtype.fb_width;
			gattr->fbtype.fb_depth = boundattr.fbtype.fb_depth;
			gattr->fbtype.fb_cmsize =  boundattr.fbtype.fb_cmsize;
			gattr->fbtype.fb_size = boundattr.fbtype.fb_size;
			gattr->sattr = boundattr.sattr;
			gattr->emu_types[0] = boundattr.emu_types[0];
			last = -1;
			for (i=0; (i < FB_ATTR_NEMUTYPES) &&
			          (boundattr.emu_types[i] != -1); i++) {
			    if ((gattr->emu_types[i] = boundattr.emu_types[i])
				!= -1) {
				last = i;
			    }
			}
			if (last == FB_ATTR_NEMUTYPES -1) {
			    /* then we are out of emulation types,
			    /* overwrite the last one. */
			    gattr->emu_types[last] = FBTYPE_SUNFB_VIDEO;
			} else {
			    /* just add ourself to the emulation types */
			    gattr->emu_types[last + 1] = FBTYPE_SUNFB_VIDEO;
			}
		    } else {
			/* bound device does not support FBIOGATTR */
			/* get type and fake it */
			cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
			    TV1_BOUND_DEV(dev), FBIOGTYPE, &boundtype,
			    flag);

		    }
		    gattr->real_type = FBTYPE_SUNFB_VIDEO;
		    gattr->owner = 0;
		    gattr->sattr.dev_specific[0] = -1;
		    gattr->emu_types[0] = boundattr.emu_types[0];
		    last = -1;
		    for (i=0; i < FB_ATTR_NEMUTYPES; i++) {
			if ((gattr->emu_types[i] = boundattr.emu_types[i])
			      != -1) {
			    last = i;
			}
		    }
		    if (last == FB_ATTR_NEMUTYPES -1) {
			/* then we are out of emulation types, overwrite the
			/* last one. */
			gattr->emu_types[last] = FBTYPE_SUNFB_VIDEO;
		    } else {
			/* just add ourself to the emulation types */
			gattr->emu_types[last + 1] = FBTYPE_SUNFB_VIDEO;
		    }
	    } else {
		    gattr->real_type = FBTYPE_SUNFB_VIDEO;
	    }
	}
	break;
    case FBIOSATTR:{
	    /* Need some work here for emulation to really work properly */
	    break;
	}
    case FBIOSVIDEO:{
	    int retval;

	    /* Turn on/off video on the bound device */
	    if (softc->tv1_bound_dev) {
		if ((retval = cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		    TV1_BOUND_DEV(dev) , FBIOSVIDEO, data, flag)) == -1) {
			return (retval);
		    }
	    }
	    /* Do something about the SunVideo video */
	    softc->video_on = *(int *)data;
	    /* To be continued... */
	    break;
	}
    case FBIOGVIDEO:
	    *(int *) data = softc->video_on;
	    break;
#if NWIN > 0
    case FBIOGPIXRECT:{
	    int r;
	    extern int tv1_putattributes();
	    extern int tv1_ioctl();

	    extern struct pixrect *sup_create();    
	    static struct pixrect enable_pixrect;
	    static struct mpr_data enable_mpr_data;

	    struct fbpixrect *fbpr = (struct fbpixrect *) data;
	    static struct fbpixrect emufb;
	    int i;

	    static struct fbdesc {
		short           group;
		short           depth;
	    }               fbdesc[TV1_NFBS] = {
		{ PIXPG_VIDEO, 1 /* LIE , really 32 */ },
		{ PIXPG_VIDEO_ENABLE, 1 }
	    };

	    if (!softc->tv1_bound_dev) {
		return (ENODEV);
	    }
	    if (r = cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		    TV1_BOUND_DEV(dev), cmd, &emufb, flag)) {
		return (r);
	    }

	    fbpr->fbpr_pixrect = &softc->pr;
	    softc->pr.pr_data = (caddr_t) & softc->prd;
	    softc->prd.emufb = emufb.fbpr_pixrect;
	    softc->prd.active = -1;
	    softc->pr.pr_ops = &tv1_ops;
	    /* set up to emulate bound device */
	    /* Changing plane groups will switch data */
	    softc->pr.pr_ops->pro_rop =  emufb.fbpr_pixrect->pr_ops->pro_rop;
	    softc->pr.pr_ops->pro_putcolormap =
		emufb.fbpr_pixrect->pr_ops->pro_putcolormap;
	    softc->pr.pr_ops->pro_putattributes = tv1_putattributes;
	    softc->pr.pr_ops->pro_nop =  tv1_ioctl;
	    softc->pr.pr_data = emufb.fbpr_pixrect->pr_data;
	    
	    softc->pr.pr_size.x = emufb.fbpr_pixrect->pr_size.x;
	    softc->pr.pr_size.y = emufb.fbpr_pixrect->pr_size.y;
	    softc->pr.pr_depth = emufb.fbpr_pixrect->pr_depth;

	    enable_pixrect.pr_size.x = CP_WIDTH;
	    enable_pixrect.pr_size.y = CP_HEIGHT;
	    enable_pixrect.pr_depth =  CP_DEPTH;
	    enable_pixrect.pr_ops = &mem_ops;
	    enable_pixrect.pr_data = (caddr_t) &enable_mpr_data;
	    enable_mpr_data.md_image = (short *)softc->tv1_enable_plane;
	    enable_mpr_data.md_linebytes = CP_LINEBYTES;
	    /* offset, and flags are left unitialized (0) */

	    softc->prd.pr_video_enable = sup_create(&enable_pixrect, 
			       &softc->tv1_enable_plane->tv1_enable_offset,
			       emufb.fbpr_pixrect->pr_size.x,
		               emufb.fbpr_pixrect->pr_size.y);
	    for (i = 0; i < TV1_NFBS; i++) {
		softc->prd.fb[i].group = fbdesc[i].group;
		softc->prd.fb[i].depth = fbdesc[i].depth;
	    }
	    break;
	}
#endif /* NWIN > 0 */
#define	SETKNOB(C,W,OFFSET) { \
	int i;	\
		\
	i = OFFSET + *(int *)data; \
	if (i > 255) { \
	    i = 255; \
        } else if (i < 0) { \
	    i = 0; \
	} \
	shadow->knob.C.field.W = i; \
	tvcsr->knob.C.packed = shadow->knob.C.packed; \
}


#define	GETKNOB(C,W) *(unsigned int *)data = shadow->knob.C.field.W
    case TVIOGFORMAT:
	*(int *) data = shadow->control.field.format;
	break;
    case TVIOSFORMAT:	/* Set input format */
	if (*(unsigned int *)data > FORMAT_RGB) {
	    return (EINVAL);
	}
	shadow->control.packed = tvcsr->control.packed;
	shadow->control.field.format = *(int *)data;
	tvcsr->control.packed = shadow->control.packed;
	if (softc->tv1_cal.field.format != 0) {
	    /* Multiple calibrations are stored. Use them */
	    switch (shadow->control.field.format) {
		case TVIO_NTSC:
		    shadow->knob.light.field.luma = U_CHAR(
			softc->tv1_cal.field.ntsc_luma_gain +
			softc->adjustment_biases.luma_gain);
		    shadow->knob.light.field.chroma = U_CHAR(
			softc->tv1_cal.field.ntsc_chroma_gain +
			softc->adjustment_biases.chroma_gain);
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    softc->current_cal = &softc->tv1_cal.field.ntsc_yc_cal;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YC:
		    shadow->knob.light.field.luma = U_CHAR(
			softc->tv1_cal.field.yc_luma_gain +
			softc->adjustment_biases.luma_gain);
		    shadow->knob.light.field.chroma = U_CHAR(
			softc->tv1_cal.field.yc_chroma_gain +
			softc->adjustment_biases.chroma_gain);
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    softc->current_cal = &softc->tv1_cal.field.ntsc_yc_cal;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YUV:
		    softc->current_cal = &softc->tv1_cal.field.yuv_cal;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.yuv_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_RGB:
		    softc->current_cal = &softc->tv1_cal.field.rgb_cal;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.rgb_cal,
			&softc->adjustment_biases);
		    break;
	    }
	}
	break;
    case TVIOSLOOPBACKCAL: /* change to "loopback" cal values */
	if (softc->tv1_cal.field.format != 0) {
	    switch (shadow->control.field.format) {
		case TVIO_NTSC:
		    shadow->knob.light.field.luma =
			softc->tv1_cal.field.loopback_ntsc_luma_gain;
		    shadow->knob.light.field.chroma =
			softc->tv1_cal.field.loopback_ntsc_chroma_gain;
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.loopback_ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YC:
		    shadow->knob.light.field.luma =
			softc->tv1_cal.field.loopback_yc_luma_gain;
		    shadow->knob.light.field.chroma =
			softc->tv1_cal.field.loopback_yc_chroma_gain;
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.loopback_ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YUV:
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.loopback_yuv_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_RGB:
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.loopback_rgb_cal,
			&softc->adjustment_biases);
		    break;
	    }
	}
	break;
    case TVIOSSYNC: { /* Sync Select */
	int sync;
	switch (*(int *)data) {
	    case TVIO_NTSC:
		sync = SYNC_NTSC;
		break;
	    case TVIO_YC:
		sync = SYNC_Y;
		break;
            case TVIO_YUV:
	    case TVIO_RGB:
		sync = SYNC_GY;
		break;
	    default:
		return (EINVAL);
	}
	shadow->control.packed = tvcsr->control.packed;
	shadow->control.field.sync = sync;
	tvcsr->control.packed = shadow->control.packed;
	break;
    }
    case TVIOSCOMPOUT:
	/* Set component out to RGB or YUV */
	switch (*(int *)data) {
	    case TVIO_YUV:
		shadow->display.field.yuv = 1; break;
	    case TVIO_RGB:
		shadow->display.field.yuv = 0; break;
	    default:
		return (EINVAL);
	}
	tvcsr->display.packed = shadow->display.packed;
	break;
    case TVIOGCONTROL:{
	    union general tmp_general;

	    tmp_general.packed = tvcsr->general.packed;
	    shadow->control.packed = tvcsr->control.packed;
	    *(int *) data = shadow->control.field.sync;
	    *(int *) data |= shadow->control.field.yc_sep << 8;
	    *(int *) data |= shadow->control.field.demod << 16;
	    *(int *) data |= tmp_general.field.burst_absent << 13;
	    *(int *) data |= tmp_general.field.sync_absent << 14;
	    break;
	}
    case TVIOSCONTROL:{
	    register int    ctl = *(int *) data;
	    register int    sync;

	    sync = (ctl & 0xf) >> 1;
	    if ((sync != shadow->control.field.format) &&
		(sync & shadow->control.field.format != 2))
		return (EINVAL);
	    shadow->control.field.sync = ctl & 0xf;
	    shadow->control.field.yc_sep = (ctl >>= 8) & 0xf;
	    shadow->control.field.demod = (ctl >>= 8) & 0xf;
	    tvcsr->control.packed = shadow->control.packed;
	    break;
	}
    case TVIOGCOMPRESS:{
	    shadow->display.packed = tvcsr->display.packed;
	    shadow->control.packed = tvcsr->control.packed;
	    *(int *) data = shadow->display.field.expand ? 0 :
		shadow->control.field.compress + 1;
	    break;
	}
    case TVIOSCOMPRESS:{
	    register int    comp = *(int *) data;

	    if (comp == 0) {	       /* 1 to 2 expand */
		shadow->control.field.compress = 0;
		shadow->display.field.expand = 1;
	    }
	    else if (comp == 1) {
		shadow->control.field.compress = 0;
		shadow->display.field.expand = 0;
	    }
	    else {
		shadow->display.field.expand = 0;
		shadow->control.field.compress = (comp - 1) & 0x7;
	    }
	    tvcsr->display.packed = shadow->display.packed;
	    tvcsr->control.packed = shadow->control.packed;
	    break;
	}
    case TVIOGCHROMAGAIN:
	*(unsigned int *)data = softc->adjustment_biases.chroma_gain;
	break;
    case TVIOSCHROMAGAIN:
	softc->adjustment_biases.chroma_gain = *(int *)data;
	switch (shadow->control.field.format) {
	    case TVIO_NTSC:
		SETKNOB(light, chroma, 
		        softc->tv1_cal.field.ntsc_chroma_gain);
		break;
	    case TVIO_YC:
		SETKNOB(light, chroma, 
		        softc->tv1_cal.field.yc_chroma_gain);
		break;
	    default:
		break;
	}
	break;
    case TVIOGLUMAGAIN:
	*(unsigned int *)data = softc->adjustment_biases.luma_gain;
	break;
    case TVIOSLUMAGAIN:
	softc->adjustment_biases.luma_gain = *(int *)data;
	switch (shadow->control.field.format) {
	    case TVIO_NTSC:
		SETKNOB(light, luma, 
		        softc->tv1_cal.field.ntsc_luma_gain);
		break;
	    case TVIO_YC:
		SETKNOB(light, luma, 
		        softc->tv1_cal.field.yc_luma_gain);
		break;
	    default:
		break;
	}
	break;
    case TVIOGREDGAIN:
	*(unsigned int *)data = softc->adjustment_biases.red_gain;
	break;
    case TVIOSREDGAIN:
	softc->adjustment_biases.red_gain = *(int *)data;
	SETKNOB(red, gain, softc->current_cal->red_gain);
	break;
    case TVIOGREDBLACK:
	*(unsigned int *)data = softc->adjustment_biases.red_black;
	GETKNOB(red, black);
	break;
    case TVIOSREDBLACK:
	softc->adjustment_biases.red_black = *(int *)data;
	SETKNOB(red, black, softc->current_cal->red_black);
	break;
    case TVIOGBLUEGAIN:
	*(unsigned int *)data = softc->adjustment_biases.blue_gain;
	break;
    case TVIOSBLUEGAIN:
	softc->adjustment_biases.blue_gain = *(int *)data;
	SETKNOB(blue, gain, softc->current_cal->blue_gain);
	break;
    case TVIOGBLUEBLACK:
	*(unsigned int *)data = softc->adjustment_biases.blue_black;
	break;
    case TVIOSBLUEBLACK:
	softc->adjustment_biases.blue_black = *(int *)data;
	SETKNOB(blue, black, softc->current_cal->blue_black);
	break;
    case TVIOGGREENGAIN:
	*(unsigned int *)data = softc->adjustment_biases.green_gain;
	break;
    case TVIOSGREENGAIN:
	softc->adjustment_biases.green_gain = *(int *)data;
	SETKNOB(green, gain, softc->current_cal->green_gain);
	break;
    case TVIOGGREENBLACK:
	*(unsigned int *)data = softc->adjustment_biases.green_black;
	break;
    case TVIOSGREENBLACK:
	softc->adjustment_biases.green_black = *(int *)data;
	SETKNOB(green, black, softc->current_cal->green_black);
	break;

    case TVIOGPOS:{
	    struct pr_pos  *pos = (struct pr_pos *) data;

	    shadow->display.packed = tvcsr->display.packed;
	    pos->x = 2047 - shadow->display.field.xpos -
		      softc->tv1_cal.field.offset.x;
	    pos->y = 4095 - shadow->display.field.ypos -
		      softc->tv1_cal.field.offset.y;
	    break;
	}
    case TVIOSPOS: {
	    int err = 0, x, y;

	    struct pr_pos  *pos = (struct pr_pos *) data;

            shadow->display.packed = tvcsr->display.packed;

	    if ((x = pos->x + softc->tv1_cal.field.offset.x) > 2047) {
		x = 2047;
		err = EINVAL;
	    }
	    if ((y = pos->y + softc->tv1_cal.field.offset.y) > 4094) {
		y = 4094;
		err = EINVAL;
	    }
	    shadow->display.field.xpos = 2047 - x;
	    shadow->display.field.ypos = 4095 - y;
	    /* update enable plane offsets: Pixrects need this */
	    softc->tv1_y = pos->y;
	    softc->tv1_enable_plane->tv1_enable_offset.x = pos->x;
	    softc->tv1_enable_plane->tv1_enable_offset.y = pos->y;
	    tvcsr->display.packed = shadow->display.packed;
	    if (err) return (err);
	}
	break;
    case TVIOGRAB:
	if (softc->grab_proc)
    	    return (EACCES);
	    /* NOT REACHED */
	softc->grab_proc = u.u_procp->p_pid;
	break;
    case TVIORELEASE:
	if (softc->grab_proc != u.u_procp->p_pid && !suser())
	    return (EACCES);
	    /* NOT REACHED */
	softc->grab_proc = 0;
	break;
    case TVIONVWRITE: /* Write NV RAM */
	{
	    int i;

	    nv_wren(shadow, tvcsr, 1);	/* set write enable latch */
	    /* store data into shadow ram */
	    for (i = 0; i < 16; i++) {
		nv_write_word(shadow, tvcsr,
		    ((union tv1_nvram *)data)->packed[i], i);
	    }
	    nv_store(shadow, tvcsr);	/* Store to non-volatile memory */
	    nv_wren(shadow, tvcsr, 0);	/* reset write enable latch */
	}
	break;
    case TVIONVREAD:  /* Read NV Ram */
	{
	    int i;
	    nv_recall(shadow, tvcsr);	/* recall data into shadow ram */
	    for (i = 0; i < 16; i++) {
		((union tv1_nvram *)data)->packed[i] = nv_read_word(shadow, tvcsr, i);
	    }
	}
	break;
    case TVIOSVIDEOCAL:  /* set calibration values */
	softc->tv1_cal = *(union tv1_nvram *)data;
	shadow->display.packed = tvcsr->display.packed;

	shadow->display.field.xpos = 2047 -
		(softc->tv1_enable_plane->tv1_enable_offset.x + 
		 softc->tv1_cal.field.offset.x);
	shadow->display.field.ypos = 4095 - 
		    (softc->tv1_enable_plane->tv1_enable_offset.y +
		     softc->tv1_cal.field.offset.y);
	tvcsr->display.packed = shadow->display.packed;
	/* Zero out user biases */
	bzero((char *)&softc->adjustment_biases,
	      sizeof (struct adjustment_biases));
	if (softc->tv1_cal.field.format != 0) {
	    /* Multiple calibrations are stored. Use them */
	    switch (shadow->control.field.format) {
		case TVIO_NTSC:
		    shadow->knob.light.field.luma =
			softc->tv1_cal.field.ntsc_luma_gain;
		    shadow->knob.light.field.chroma =
			softc->tv1_cal.field.ntsc_chroma_gain;
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YC:
		    shadow->knob.light.field.luma =
			softc->tv1_cal.field.yc_luma_gain;
		    shadow->knob.light.field.chroma =
			softc->tv1_cal.field.yc_chroma_gain;
		    tvcsr->knob.light.packed = shadow->knob.light.packed;
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.ntsc_yc_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_YUV:
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.yuv_cal,
			&softc->adjustment_biases);
		    break;
		case TVIO_RGB:
		    tv1_set_gains_and_blacks(shadow, tvcsr,
			&softc->tv1_cal.field.rgb_cal,
			&softc->adjustment_biases);
		    break;
	    }
	} else { /* Old Format */
	    return (EINVAL);
	}
	break;
    case TVIOGVIDEOCAL:  /* get calibration values */
	*(union tv1_nvram *)data = softc->tv1_cal;
	break;
    case TVIOSCHROMASEP:
	shadow->control.packed = tvcsr->control.packed;
	if (*(unsigned int *)data > 2) {
	    return (EINVAL);
	}
	shadow->control.field.yc_sep = *(unsigned int *)data;
	tvcsr->control.packed =  shadow->control.packed;
	break;
	
    case TVIOGCHROMASEP:
	shadow->control.packed = tvcsr->control.packed;
	*(unsigned int *)data = shadow->control.field.yc_sep;
	break;
    case TVIOSGENLOCK:
	shadow->display.field.genlock = *(int *)data;
	tvcsr->display.packed = shadow->display.packed;
	break;
    case TVIOVWAIT:
	{
	int s;
	/* wait for vertical interrupt */
	s = splx(pritospl(4)); 
	if (tvcsr->general.field.sync_absent) {
	    (void) splx(s);
	    return (EWOULDBLOCK);
	}
	shadow->general.field.sync_intr_en = 1;
	shadow->general.field.vtrace_intr_en = 1;
	tvcsr->general.packed = shadow->general.packed;
	(void) sleep((caddr_t)tv1_softc, PZERO -1);
	(void) splx(s);	
	if (tvcsr->general.field.sync_absent) {
	    return (EWOULDBLOCK);
	}
	}
	break;
    case TVIOSCHROMADEMOD:
	if (*(unsigned int *)data > 2) {
	    return (EINVAL);
	}
	shadow->control.field.demod = *(int *)data;    
	tvcsr->control.packed = shadow->control.packed;
	break;
    case TVIOGABSTATE:
	*(int *)data = tvcsr->control.field.abstate;
	break;
    case TVIOSABSTATE:
	if (*(unsigned int *)data > 5) {
	    return (EINVAL);
	}
	shadow->control.field.abstate = *(int *)data;
	tvcsr->control.packed = shadow->control.packed;
	break;
    case TVIOGIBSTATE:
	*(int *)data = tvcsr->general.field.ibstate;
	break;
    case TVIOSIBADVANCE:
	shadow->control.field.ib_advance = *(int *)data;
	tvcsr->control.packed = shadow->control.packed;
	break;
    case TVIOREDIRECT:/* Redirect /dev/fb to be our device */
	{
	    extern dev_t fbdev;
	    fbdev = dev;
	}
	break;
    case TVIOGSYNCABSENT:
	*(int *)data =  tvcsr->general.field.sync_absent;
	break;
    case TVIOGBURSTABSENT:
	*(int *)data =  tvcsr->general.field.burst_absent;
	break;
    default:
	if (softc->tv1_bound_dev) { /* if we are bound to a device */
	    /* pass it on */
	    return (cdevsw[major(TV1_BOUND_DEV(dev))].d_ioctl(
		TV1_BOUND_DEV(dev), cmd, data, flag));
	} else {
	    return (ENOTTY); /* no such ioctl for device */
	}
    }

    return (0);
}


#define	RANDOM_PATTERN 0xa0b0c0

/*
 * Determine if a tvone exists at the given address.
 * If it exists, determine its size.
 */
/*ARGSUSED*/
tvoneprobe(reg, unit)
    caddr_t         reg;
    int             unit;
{
    struct tv1_softc *softc = &tv1_softc[unit];
    long next_poke;
    u_long kmx;
    int s;

    extern u_long rmalloc();
    extern unsigned int hat_getkpfnum();


    /* instead of just probing to see if something exist, we also do */
    /* some sanity checking */

    /* Probe the 24 Bit Video buffer */
    softc->tv1_baseaddr = ((u_long) hat_getkpfnum(reg) << PAGESHIFT) |
	((u_long) reg & PAGEOFFSET);

    if (tv1_probe_addr((long *) reg, (long)0x00FFFFFF, 0)) {
	/* This is where we expect it to fail if no board is installed */
	return (0);
    }

    /* Probe the Control plane */
    next_poke = ctob(btoc(FB_SIZE));
    fbmapin(reg, (int)TV1_PHYSICAL(next_poke), NBPG);
    if (tv1_probe_addr((long *) reg, (long)0xFFFFFFFF, 1)) {
	printf("tv1_probe: Failed (Bus error) when accesing control plane.");
	return (0);
    }

    /* Map it in and check it again */
    s = splhigh();
    kmx = rmalloc(kernelmap, (long)btoc(CP_SIZE));
    (void)splx(s);
    if (kmx == 0) {
	printf("tvoneprobe: rmalloc failed.\n");
    }
    softc->tv1_enable_plane = (struct tv1_enable_plane *) kmxtob(kmx);
    fbmapin((caddr_t)softc->tv1_enable_plane, (int)TV1_PHYSICAL(CP_BASE),
	     CP_SIZE);
    if (tv1_probe_addr((long *) softc->tv1_enable_plane, 
	(long)0xFFFFFFFF, 1)) {
	printf("tv1_probe: Failed at control plane(2)");
	s = splhigh();
	rmfree(kernelmap, (long)CP_SIZE, kmx);
	(void)splx(s);
	return (0);
    }

    /* Probe control registers */
    next_poke = CP_BASE + CP_SIZE;
    fbmapin(reg, (int)TV1_PHYSICAL(next_poke), NBPG);
    if (tv1_probe_addr((long *) reg, (long)0xFFFFFFFF, 1)) {
	printf("tv1_probe: Failed at control registers\n");
	return (0);
    }
    next_poke = CSR_BASE;
    fbmapin(reg, (int)TV1_PHYSICAL(next_poke), NBPG);
    {
	struct csr *csr = (struct csr *) reg;
	union general *gen_csr = &softc->shadow_csr.general;
	long temp;

	if (peekl((long *) reg, &temp)) {
	    printf("tvoneprobe: csr probe failed\n");
	    s = splhigh();
	    rmfree(kernelmap, (long)NBPG, kmx);
	    (void)splx(s);
	    return (0);
	}
	/* CSR seems OK. */
	if (peekl((long *) &(csr->general.packed), (long *)gen_csr) ||
	    gen_csr->field.magic_intr != TV1_MAGIC) {
		printf("tv1probe: failed at csr");
		printf("Expected %x, got %x as magic number.\n",
		TV1_MAGIC, gen_csr->field.magic_intr);
		s = splhigh();
		rmfree(kernelmap, (long)NBPG, kmx);
		(void)splx(s);
	        return (0);
	    }
	softc->tv1_csr = csr;	/* save the address */
	softc->video_on = 1;
	gen_csr->field.magic_intr = TV1_INTR_VECTOR;
    }
    return (TV1_PROBESIZE);
}

tv1_probe_addr(addr, mask, print)
    long *addr;
    long mask;
    int print;	/* Should we print if the first peek fails? */
{
    long old, value;

#define	TESTPAT 0xA55A5AA5
    if (peekl(addr, &old)) {
	    if (print) printf("tvprobe: peek(0) failed\n");
	return (-1);
    }

    if (pokel(addr, (long)TESTPAT)) {
	printf("tvprobe: poke(1) failed\n");
	return (-2);
    }

    if (peekl(addr, &value)) {
	printf("tvprobe: peek(1) failed\n");
	return (-3);
    }

    if ((value&mask) != (TESTPAT&mask)) {
	printf("tvprobe: compare(1) failed Expected %x, got %x\n",
	        TESTPAT & mask,
		value & mask);
	return (-4);
    }

    if (pokel(addr, (long)~TESTPAT)) {
	printf("tvprobe: poke(2) failed\n");
	return (-5);
    }

    if (peekl(addr, &value)) {
	printf("tvprobe: peek(2) failed\n");
	return (-6);
    }

    if ((value & mask) != ((~TESTPAT) & mask)) {
	printf("tvprobe: compare(2) failed. Expected %x, got %x\n",
	(~TESTPAT) & mask,
	 value & mask);
	return (-7);
    }

    if (pokel(addr, old)) {
	printf("tvprobe: poke(3) failed.\n");
	return (-8);
    }
    return (0);
}

#define	X_OFFSET 244
#define	Y_OFFSET 33

tvoneattach(md)
    register struct mb_device *md;
{
    /*
     * supposedly, all registers come up zero, we reset the board just to be
     * sure.
     * The address of the general register is left over from probe and kept
     * there.
     */
    struct tv1_softc *softc = &tv1_softc[md->md_unit];
    register struct csr *tvcsr, *shadow;
    register union tv1_nvram *tv1_cal;
    int i;

    tvcsr = softc->tv1_csr;
    shadow = &softc->shadow_csr;
    tv1_cal = &softc->tv1_cal;

    shadow->general.field.magic_intr = TV1_INTR_VECTOR;
    tvcsr->general.packed = shadow->general.packed; /* reset */
    nv_recall(shadow, tvcsr);	/* Recall from NV ram */
    for (i = 0; i < 16; i++) {
	tv1_cal->packed[i] = nv_read_word(shadow, tvcsr, i);
    }
    if ((tv1_cal->field.magic != TV1_MAGIC) ||
	((unsigned int)tv1_cal->field.offset.x > 512) ||
	((unsigned int)tv1_cal->field.offset.y > 512)) {
	printf("tvoneattatch: non-volatile memory error.\n");
	/* initialize to something that looks reasonable */
	for (i=0; i < 16; i++) {
	    tv1_cal->packed[i] = 0x80808080;	/* 128 for most values */
	}
	tv1_cal->field.magic = TV1_MAGIC;
	tv1_cal->field.offset.x = X_OFFSET;
	tv1_cal->field.offset.y = Y_OFFSET;
	nv_wren(shadow, tvcsr, 1);	/* set write enable latch */
	/* store data into shadow ram */
	/* Note that we do NOT overwrite the NV part of the RAM */
	for (i = 0; i < 16; i++) {
	    nv_write_word(shadow, tvcsr, tv1_cal->packed[i], i);
	}
	nv_wren(shadow, tvcsr, 0);	/* reset write enable latch */
    }

    shadow->knob.light.field.chroma = tv1_cal->field.ntsc_chroma_gain;
    shadow->knob.light.field.luma   = tv1_cal->field.ntsc_luma_gain;
    shadow->knob.red.field.black   = tv1_cal->field.ntsc_yc_cal.red_black;
    shadow->knob.red.field.gain    = tv1_cal->field.ntsc_yc_cal.red_gain;
    shadow->knob.green.field.black = tv1_cal->field.ntsc_yc_cal.green_black;
    shadow->knob.green.field.gain  = tv1_cal->field.ntsc_yc_cal.green_gain;
    shadow->knob.blue.field.black  = tv1_cal->field.ntsc_yc_cal.blue_black;
    shadow->knob.blue.field.gain   = tv1_cal->field.ntsc_yc_cal.blue_gain;
    tvcsr->knob = shadow->knob;
    shadow->display.packed = tvcsr->display.packed;
    shadow->general.packed = tvcsr->general.packed; /* reset */
    shadow->general.field.magic_intr = TV1_INTR_VECTOR;
    softc->grab_proc = 0;   /* no one owns it */
    softc->current_cal = &tv1_cal->field.ntsc_yc_cal;

    /* control plane is already mapped in, clear it */
    {
	register int row;
	register char *p;

	p = (char *)softc->tv1_enable_plane;

	for (row=0; row < CP_HEIGHT; row++) {
	    bzero(p, mpr_linebytes(CP_WIDTH, CP_DEPTH));
	    p += CP_LINEBYTES;
	}
    }
    return (0);
}

/*ARGSUSED*/
tvoneintr(unit)
{
    wakeup((caddr_t)tv1_softc);
    return (0);
}

static
tv1_set_gains_and_blacks(shadow, tvcsr, cal, biases)
    struct csr     *shadow, *tvcsr;
    struct tv1_rgb_gain_black *cal;
    struct adjustment_biases *biases;
{
    shadow->knob.red.field.gain = U_CHAR(cal->red_gain + biases->red_gain);
    shadow->knob.red.field.black = U_CHAR(cal->red_black + biases->red_black);
    tvcsr->knob.red.packed = shadow->knob.red.packed;
    
    shadow->knob.green.field.gain = U_CHAR(cal->green_gain + biases->green_gain);
    shadow->knob.green.field.black = U_CHAR(cal->green_black + biases->green_black);
    tvcsr->knob.green.packed = shadow->knob.green.packed;
    
    shadow->knob.blue.field.gain = U_CHAR(cal->blue_gain + biases->blue_gain);
    shadow->knob.blue.field.black = U_CHAR(cal->blue_black + biases->blue_black);
    tvcsr->knob.blue.packed = shadow->knob.blue.packed;
}

/* The following Section of code deals with the NV RAM of the device */
/* There are two Xicor X2444 devices */

#define	NV_WRITE(addr)	((unsigned char)(0x83 | ((addr) << 3)))
#define	NV_READ(addr)	((unsigned char)(0x86 |  ((addr) << 3)))
#define	NV_WREN		((unsigned char)0x84)	/* set write enable */
#define	NV_RCL		((unsigned char)0x85)	/* recall nv data */
#define	NV_WRDS		((unsigned char)0x80)	/* reset write enable */
#define	NV_STO		((unsigned char)0x81)	/* Store shadow ram to nv */

nv_recall(shadow, tvcsr)
    struct csr *shadow, *tvcsr;
{
    nv_enable(shadow, tvcsr, 1);
    DELAY(1);
    nv_write_cmd(shadow, tvcsr, NV_RCL);
    DELAY(3);
    nv_enable(shadow, tvcsr, 0);
}

nv_wren(shadow, tvcsr, enable)
    struct csr *shadow, *tvcsr;
    int enable;
{
    nv_enable(shadow, tvcsr, 1);
    nv_write_cmd(shadow, tvcsr, (enable ? NV_WREN : NV_WRDS));
    DELAY(3);
    nv_enable(shadow, tvcsr, 0);
}

nv_enable(shadow, tvcsr, enable)
    struct csr *shadow, *tvcsr;
    int enable;
{
    shadow->general.field.nvram_enable = enable;
    shadow->general.field_write.nvram_alt_enable = enable;
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);
}

nv_store(shadow, tvcsr)
    struct csr *shadow, *tvcsr;
{
    nv_enable(shadow, tvcsr, 1);
    nv_write_cmd(shadow, tvcsr, NV_STO);
    DELAY(10000); /* 10 ms */
    nv_enable(shadow, tvcsr, 0);
}

nv_read_word(shadow, tvcsr, addr)
    struct csr *shadow, *tvcsr;
    int addr;
{
    int word;
    if (addr > 15) {
	return (0);	/* Should never happen */
    }
    nv_enable(shadow, tvcsr, 1);
    nv_write_cmd(shadow, tvcsr, NV_READ(addr));
    nv_read(shadow, tvcsr, (char *)&word, sizeof (word));
    nv_enable(shadow, tvcsr, 0);
    return (word);
}

nv_write_word(shadow, tvcsr, word, addr)
    struct csr *shadow, *tvcsr;
    int word, addr;
{
    if (addr > 15) {
	return;  /* Should never happen */
    }
    nv_enable(shadow, tvcsr, 1);
    nv_write_cmd(shadow, tvcsr, NV_WRITE(addr));
    nv_write(shadow, tvcsr, (char *)&word, sizeof (word));
    nv_enable(shadow, tvcsr, 0);
}
/* NV Write: writes a block of data to the nvram. */
/* Note that the two chips are interleaved, so commands */
/* must be shifted so that they are both in the even as well as the odd */
/* bits */
nv_write(shadow, tvcsr, buf, len)
    struct csr *shadow, *tvcsr;
    char *buf;
    int len;
{
    register char this_byte;
    register int i, j;
    for (i=0; i < len; i++) {
	this_byte = buf[i];
	for (j = 0; j < 4; j++) {
	    nv_write_bits(shadow, tvcsr, (this_byte >> (2*j)) & 0x3);
	}
    }
}

nv_read(shadow, tvcsr, buf, len)
    struct csr *shadow, *tvcsr;
    char *buf;
    int len;
{
    register char this_byte;
    register int i, j;
    for (i=0; i < len; i++) {
	this_byte = 0;
	for (j = 0; j < 4; j++) {
	    this_byte |= nv_read_bits(shadow, tvcsr) << (2*j);
	}
	buf[i] = this_byte;
    }
}

nv_write_cmd(shadow, tvcsr, cmd)
    struct csr *shadow, *tvcsr;
    unsigned char cmd;
{
    int             j, this_bit;

    for (j = 0; j < 8; j++) {
	this_bit = (cmd >> (7 - j)) & 01;
	nv_write_bits(shadow, tvcsr, (this_bit | (this_bit << 1)));
    }
}

/* Writes 2 bits to nvram */
nv_write_bits(shadow, tvcsr, bits)
    struct csr *shadow, *tvcsr;
    int bits;
{
    if (bits > 3) {
	printf("NVERR !\n");
	return;
    }
    shadow->general.field.nvram_data = bits;
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);	/* data set up need 0.4 */

    shadow->general.field_write.nvram_clk = 1;
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);	/* Clock width (exceeds data hold) */

    shadow->general.field_write.nvram_clk = 0;
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);
}

/* Reads 2 bits from nvram */
nv_read_bits(shadow, tvcsr)
    struct csr *shadow, *tvcsr;
{
    union general my_general;

    int bits;

    my_general.packed = tvcsr->general.packed;	   /* Read actual data */
    bits = my_general.field.nvram_data;		   /* extract our bits */

    shadow->general.field_write.nvram_clk = 1;  /* and do the clock */
    DELAY(1);
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);
    shadow->general.field_write.nvram_clk = 0;
    tvcsr->general.packed = shadow->general.packed;
    DELAY(1);
    return (bits);
}

#if NWIN > 0
/*  This is the kernel equivalent of get_tv1_data in tv1.c */

struct tv1_data*
get_tv1_data(pr)
    Pixrect* pr;
{
    int i;
    for (i = 0; i < NTVONE; i++)
	if (&(tv1_softc[i].pr) == pr)
	    return (&(tv1_softc[i].prd));
    return (NULL);
}

#endif NWIN > 0

#endif NTVONE > 0
