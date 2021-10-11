/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
#ifndef lint
static char sccsid[] = "@(#)bw2dd.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
 * Device dependent core graphics driver for SUN bw2 device 
*/

#include        <sunwindow/window_hs.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	"langnames.h"
#include	<sys/file.h>
#include	<sun/fbio.h>
#include	<strings.h>
#include	"bw2dd.h"
#define NULL 0

extern struct pixrectops mem_ops;
extern PIXFONT *_core_defaultfont;

static short msklib[] = {
	     0x0000, 0x8000, 0x8080, 0x8410, 0x8888, 0x9124, 0x9494, 0xA552,
	      0xAAAA, 0xEB6E, 0xDDDD, 0xF7F7, 0xFFFF, 0xE3E3, 0xFF00, 0x00FF
};

mpr_static(_core_bw2maskinit, 16, 16, 1, 0);
static struct mpr_data mprdata = {0, 0, 0, 0, 1, 0};
static struct pixrect mprect = {&mem_ops, 0, 0, 0, (caddr_t) & mprdata};

int _core_mousedd(), _core_keybrdd();

#define devscale(x_ndc_coord, y_ndc_coord, x_dev_coord, y_dev_coord)	\
		x_dev_coord = (x_ndc_coord<<10) / ip->scale + ip->xoff;	\
		y_dev_coord = (y_ndc_coord<<10) / ip->scale + ip->yoff;

FORTNAME(bw2dd)
PASCNAME(bw2dd)
bw2dd(ddstruct)
    register ddargtype *ddstruct;
{
    register short i1, i2, i3, i4;
    rast_type *rptr;
    ipt_type *p1ptr, *p2ptr;
    int x, y, xs, ys, width, height;
    short amask, bmask, alin, blin, asht, bsht;
    short a, b, cycl;
    int i;
    float fx, fy, aspect_ratio;
    u_char *p1, *p2, *p3;
    porttype vwport, *vp;
    struct pr_prpos prprpos;
    vtxtype *vptr1;
    struct bw2ddvars *ip;

    ip = (struct bw2ddvars *) ddstruct->instance;
    switch (ddstruct->opcode) {
	case INITIAL:
	    {
		static char bwtwodev[] = "/dev/bwtwo?";
		int fbfd, pwprfd;
		struct screen screen;
		viewsurf *surfp;

		surfp = (viewsurf *) ddstruct->ptr1;
		(void)win_initscreenfromargv(&screen, (char **) 0);
		if (*surfp->vsurf.screenname != '\0') {
		    if ((fbfd = _core_chkdev(surfp->vsurf.screenname,
				       FBTYPE_SUN2BW, ddstruct->int1)) >= 0)
			(void)strncpy(screen.scr_fbname,
				surfp->vsurf.screenname, SCR_NAMESIZE);
		    else {
			ddstruct->logical = FALSE;
			return (1);
		    }
		} else if ((fbfd = _core_chkdev("/dev/fb", FBTYPE_SUN2BW,
						ddstruct->int1)) >= 0)
		    (void)strncpy(screen.scr_fbname, "/dev/fb", SCR_NAMESIZE);
		else {
			fbfd = _core_getfb(bwtwodev, FBTYPE_SUN2BW, ddstruct->int1);
			if (fbfd >= 0)
		    		(void)strncpy(screen.scr_fbname, bwtwodev, SCR_NAMESIZE);
			else {
		    		ddstruct->logical = FALSE;
		    		return (1);
			}
		}
		if (!ddstruct->int1)
		    (void)strncpy(screen.scr_kbdname, "NONE", SCR_NAMESIZE);
		if ((pwprfd = win_screennew(&screen)) < 0) {
		    ddstruct->logical = FALSE;
		    return (1);
		}
		(void)win_screenget(pwprfd, &screen);
		ddstruct->logical = TRUE;
		ip = (struct bw2ddvars *) malloc(sizeof(struct bw2ddvars));

		ip->wind.ndcxmax = MAX_NDC_COORD;
		ip->wind.ndcymax = (MAX_NDC_COORD * 3) / 4;
		ip->wind.pixwin = pw_open(pwprfd);
		ip->wind.winfd = pwprfd;
		ip->wind.needlock = FALSE;
		ip->wind.rawdev = TRUE;
		ip->screen = ip->wind.pixwin->pw_pixrect;
		ip->prmask = _core_bw2maskinit;
		ip->prmaskdata = _core_bw2maskinit_data;
		ip->prmask.pr_data = (caddr_t) & ip->prmaskdata;
		((struct mpr_data *) ip->prmask.pr_data)->md_image = ip->msklist;
		ip->maxz = 32767;
		ip->curstrk = FALSE;
		ip->lockcount = 0;
		ip->xoff = 0;
		ip->yoff = 0;
		ip->fillop = PIX_NOT(PIX_SRC);	/* default rasterops */
		ip->fillrastop = PIX_SRC;
		ip->cursorrop = PIX_NOT(PIX_SRC);
		ip->rtextop = PIX_SRC;
		ip->vtextop = PIX_SET;
		ip->pf = _core_defaultfont;	/* load default font */
		ip->openfont = -1;
		ip->lineop = PIX_SET;
		for (i1 = 0; i1 < 256; i1++) {	/* default color table */
		    ip->red[i1] = i1;
		    ip->green[i1] = 255 - i1;
		    ip->blue[i1] = i1;
		}

		surfp->windptr = &ip->wind;
		surfp->vsurf.instance = (int) ip;
		(void)strncpy(surfp->vsurf.screenname, screen.scr_fbname, DEVNAMESIZE);
		(void)strncpy(surfp->vsurf.windowname, screen.scr_rootname,
			DEVNAMESIZE);
		surfp->vsurf.windowfd = pwprfd;
		surfp->windptr = &ip->wind;
		surfp->erasure = TRUE;	/* view surface capabilities */
		surfp->txhardwr = TRUE;
		surfp->clhardwr = TRUE;
		surfp->lshardwr = TRUE;
		surfp->lwhardwr = TRUE;
		_core_winupdate(&ip->wind);
		if (surfp->vsurf.ptr != NULL)
		    _core_setadjacent(*surfp->vsurf.ptr, &screen);
	    }
	    break;
	case TERMINATE:
	    if (ip->pf != _core_defaultfont)
		(void)pf_close(ip->pf);
	    (void)pw_close(ip->wind.pixwin);
	    (void)win_screendestroy(ip->wind.winfd);
	    (void)close(ip->wind.winfd);
	    free((char *) ip);
	    break;
	case SETNDC:
	    fx = ddstruct->float1;
	    fy = ddstruct->float2;
	    ip->maxz = (int) (ddstruct->float3 * 32767.);
	    aspect_ratio = fy / fx;
	    if (aspect_ratio <= (float) ip->screen->pr_height /
		(float) ip->screen->pr_width) {
		/* 32K NDC to full screen width */
		ip->xoff = 0;
		ip->yoff = (int) ((ip->screen->pr_height - ip->screen->pr_width * aspect_ratio) / 2.0);
		ip->scale = (int) (32768. / ip->screen->pr_width * 1024);
		ip->wind.ndcxmax = MAX_NDC_COORD;
		ip->wind.ndcymax = aspect_ratio * MAX_NDC_COORD;
	    } else if (fy < 1.0) {	/* fy*32K NDC maps to full screen
					 * height */
		ip->yoff = 0;
		ip->xoff = (int) ((ip->screen->pr_width - ip->screen->pr_height / aspect_ratio) / 2.0);
		ip->scale = (int) (fy * 32768. / ip->screen->pr_height * 1024);
		ip->wind.ndcxmax = MAX_NDC_COORD;
		ip->wind.ndcymax = aspect_ratio * MAX_NDC_COORD;
	    } else {		/* full 32K NDC maps to full screen height */
		ip->yoff = 0;
		ip->xoff = (int) ((ip->screen->pr_width - ip->screen->pr_height / aspect_ratio) / 2.0);
		ip->scale = (int) (32768. / ip->screen->pr_height * 1024);
		ip->wind.ndcymax = MAX_NDC_COORD;
		ip->wind.ndcxmax = MAX_NDC_COORD / aspect_ratio;
	    }
	    _core_winupdate(&ip->wind);
	    break;
	case CLEAR:
	    if (ip->wind.needlock)
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    (void)pr_rop(ip->screen, 0, 0, ip->screen->pr_width, ip->screen->pr_height, PIX_CLR, (struct pixrect *) 0, 0, 0);	/* white screen */
	    if (ip->wind.needlock)
		(void)pw_unlock(ip->wind.pixwin);
	    break;
	case GETTAB:
	    p1 = (u_char *) ddstruct->ptr1;
	    p2 = (u_char *) ddstruct->ptr2;
	    p3 = (u_char *) ddstruct->ptr3;
	    for (i1 = ddstruct->int1; i1 <= ddstruct->int2; i1++) {
		*p1++ = ip->red[i1];
		*p2++ = ip->green[i1];
		*p3++ = ip->blue[i1];
	    }
	    break;
	case SETTAB:
	    p1 = (u_char *) ddstruct->ptr1;
	    p2 = (u_char *) ddstruct->ptr2;
	    p3 = (u_char *) ddstruct->ptr3;
	    for (i1 = ddstruct->int1; i1 <= ddstruct->int2; i1++) {
		ip->red[i1] = *p1++;
		ip->green[i1] = *p2++;
		ip->blue[i1] = *p3++;
	    }
	    break;
	case SETLCOL:
	    if (ddstruct->int2 == 1) {	/* XORing */
		ip->lineop = PIX_NOT(PIX_DST);
	    } else if (ddstruct->int2 == 2) {	/* ORing */
		if (ddstruct->int3 || !ddstruct->int1) {
		    ip->lineop = PIX_CLR;	/* erase */
		} else {
		    ip->lineop = PIX_NOT(PIX_DST);	/* paint */
		}
	    } else {
		ip->lineop = (ddstruct->int3 || !ddstruct->int1)
		  ? PIX_CLR : PIX_SET;
	    }
	    break;
	case SETTCOL:
	    if (ddstruct->int2 == 1) {	/* XORing */
		ip->rtextop = PIX_SRC ^ PIX_DST;
		ip->vtextop = PIX_NOT(PIX_DST);
	    } else {		/* ORing */
		if (ddstruct->int3 || !ddstruct->int1) {	/* erase */
		    ip->rtextop = (PIX_NOT(PIX_SRC) & PIX_DST);
		    ip->vtextop = PIX_CLR;
		} else {	/* paint */
		    ip->rtextop = PIX_SRC | PIX_DST;
		    ip->vtextop = PIX_SET;
		}
	    }
	    break;
	case SETFCOL:
	    /* make ip->textures for polygon fill */
	    ip->texture = (int) (ip->red[ddstruct->int1]) |
	      ((int) (ip->green[ddstruct->int1]) << 8) |
	      ((int) (ip->blue[ddstruct->int1]) << 16);
	    if (ddstruct->int2 == 1) {	/* XORing */
		ip->fillop = PIX_SRC ^ PIX_DST;
		ip->cursorrop = ip->fillop;
		ip->fillrastop = PIX_SRC ^ PIX_DST;
	    } else if (ddstruct->int2 == 2) {	/* ORing */
		if (ddstruct->int3) {	/* erase */
		    ip->fillop = PIX_CLR;
		    ip->cursorrop = (PIX_SRC & ~PIX_DST);
		    ip->fillrastop = (PIX_NOT(PIX_SRC) & ~PIX_DST);
		} else {	/* paint */
		    ip->fillop = PIX_SRC | PIX_DST;
		    ip->cursorrop = PIX_NOT(PIX_SRC) & PIX_DST;
		    ip->fillrastop = PIX_SRC & PIX_DST;
		}
	    } else {
		ip->fillop = (ddstruct->int3 || !ddstruct->int1)
		  ? PIX_CLR : PIX_NOT(PIX_SRC);
		ip->cursorrop = (ddstruct->int3 || !ddstruct->int1)
		  ? PIX_CLR : PIX_NOT(PIX_SRC);
		ip->fillrastop = (ddstruct->int3 || !ddstruct->int1)
		  ? PIX_CLR : PIX_SRC;
	    }
	    amask = msklib[ip->texture & 0xF];
	    bmask = msklib[(ip->texture >> 4) & 0xF];
	    alin = (ip->texture >> 8) & 0xF;
	    blin = (ip->texture >> 12) & 0xF;
	    cycl = alin + blin;
	    asht = (ip->texture >> 16) & 0xF;
	    bsht = (ip->texture >> 20) & 0xF;
	    if (cycl == 0)
		alin = 1;
	    cycl = alin + blin;
	    if (cycl > 16) {
		blin = 16 - alin;
		cycl = alin + blin;
	    }
	    a = amask;
	    b = amask;
	    for (i = 0; i < alin; i++) {
		ip->msklist[i] = a | b;
		if (asht & 1)
		    a = ((a >> 1) & 0x7FFF) | (a << 15);
		if (asht & 2)
		    a = (a << 1) | ((a >> 15) & 0x1);
		if (asht & 4)
		    b = ((b >> 1) & 0x7FFF) | (b << 15);
		if (asht & 8)
		    b = (b << 1) | ((b >> 15) & 0x1);
	    }
	    a = bmask;
	    b = bmask;
	    for (i = alin; i < cycl; i++) {
		ip->msklist[i] = a | b;
		if (bsht & 1)
		    a = ((a >> 1) & 0x7FFF) | (a << 15);
		if (bsht & 2)
		    a = (a << 1) | ((a >> 15) & 0x1);
		if (bsht & 4)
		    b = ((b >> 1) & 0x7FFF) | (b << 15);
		if (bsht & 8)
		    b = (b << 1) | ((b >> 15) & 0x1);
	    }
	    i1 = 0;
	    for (i = cycl; i < 16; i++) {
		ip->msklist[i] = ip->msklist[i1++];
	    }
	    break;
	case ECHOMOVE:
	case MOVE:
	    devscale(ddstruct->int1, ddstruct->int2, ip->ddcp[0], ip->ddcp[1]);
	    break;
	case SETWIDTH:
	    ip->linewidth = (ddstruct->int1 << 10) / ip->scale;
	    break;
	case SETSTYL:
	    ip->linestyle = ddstruct->int1;
	    break;
	case SETPISTYL:
	    ip->polyintstyle = ddstruct->int1;
	    break;
	case ECHOLINE:
	case LINE:
	    devscale(ddstruct->int1, ddstruct->int2, i1, i2);
	    if (ip->linestyle || (ip->linewidth > 1))
		(void)_core_bw2simline(ip->ddcp[0], ip->screen->pr_height - 1 - ip->ddcp[1], i1, ip->screen->pr_height - 1 - i2,
		       &ip->wind, ip->lineop, ip->linewidth, ip->linestyle);
	    else {
		if (ip->wind.needlock)
		    (void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		(void)pr_vector(ip->screen, ip->ddcp[0], ip->screen->pr_height - 1 - ip->ddcp[1], i1,
			  ip->screen->pr_height - 1 - i2, ip->lineop, 1);
		if (ip->wind.needlock)
		    (void)pw_unlock(ip->wind.pixwin);
	    }
	    ip->ddcp[0] = i1;
	    ip->ddcp[1] = i2;
	    break;
	case SETFONT:		/* font text attribute */
	    ip->ddfont = ddstruct->int1;
	    break;
	case SETUP:		/* text up direction */
	    ip->ddup = *((ipt_type *) ddstruct->ptr1);
	    ip->ddup.x = (ip->ddup.x << 10) / ip->scale;
	    ip->ddup.y = -((ip->ddup.y << 10) / ip->scale);
	    break;
	case SETPATH:		/* text path direction */
	    ip->ddpath = *((ipt_type *) ddstruct->ptr1);
	    ip->ddpath.x = (ip->ddpath.x << 10) / ip->scale;
	    ip->ddpath.y = -((ip->ddpath.y << 10) / ip->scale);
	    break;
	case SETSPACE:		/* text leading << 4 */
	    ip->ddspace = *((ipt_type *) ddstruct->ptr1);
	    ip->ddspace.x = (ip->ddspace.x << 14) / ip->scale;
	    ip->ddspace.y = -((ip->ddspace.y << 14) / ip->scale);
	    break;
	case VTEXT:		/* vector text */
	    vp = (porttype *) ddstruct->ptr2;	/* ptr2 = &viewport */
	    devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
	    devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
	    vwport.ymax = ip->screen->pr_height - 1 - vwport.ymax;
	    vwport.ymin = ip->screen->pr_height - 1 - vwport.ymin;
	    (void)_core_bw2vectext(ddstruct->ptr1, ip->ddfont, &ip->ddup,
			     &ip->ddpath, &ip->ddspace, ip->ddcp[0], ip->screen->pr_height - 1 - ip->ddcp[1],
			     &vwport, ip->vtextop | PIX_DONTCLIP, &ip->wind);
	    break;
	case TEXT:		/* raster text */
	    if (ip->ddfont != ip->openfont) {
		if (ip->pf != _core_defaultfont)
		    (void)pf_close(ip->pf);
		ip->openfont = ip->ddfont;
		switch (ip->ddfont) {
		    case 0:
			ip->pf = pf_open(
			     "/usr/lib/fonts/fixedwidthfonts/gallant.r.10");
			break;
		    case 1:
			ip->pf = pf_open(
				"/usr/lib/fonts/fixedwidthfonts/gacha.r.8");
			break;
		    case 2:
			ip->pf =
			  pf_open("/usr/lib/fonts/fixedwidthfonts/sail.r.6");
			break;
		    case 3:
			ip->pf =
			  pf_open("/usr/lib/fonts/fixedwidthfonts/gacha.b.8");
			break;
		    case 4:
			ip->pf =
			  pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.r.8");
			break;
		    case 5:
			ip->pf =
			  pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.b.8");
			break;
		    default:
			ip->pf = _core_defaultfont;	/* load default font */
			break;
		}
		if (!ip->pf) {
		    ip->pf = _core_defaultfont;
		}
	    }
	    vp = (porttype *) ddstruct->ptr2;	/* ptr2 = &viewport */
	    devscale(vp->xmin, vp->ymin, vwport.xmin, i1);
	    devscale(vp->xmax, vp->ymax, vwport.xmax, i2);
	    vwport.ymax = ip->screen->pr_height - 1 - i1;	/* flip yhrdwr */
	    vwport.ymin = ip->screen->pr_height - 1 - i2;	/* int2 = char position */
	    x = ip->ddcp[0] + (ddstruct->int2 * ip->pf->pf_defaultsize.x) -
	      vwport.xmin;
	    y = ip->screen->pr_height - 1 + (ip->pf->pf_defaultsize.y / 2) - ip->ddcp[1] -
	      vwport.ymin;
	    prprpos.pr = pr_region(ip->screen, vwport.xmin, vwport.ymin,
	      vwport.xmax - vwport.xmin + 1, vwport.ymax - vwport.ymin + 1);
	    prprpos.pos.x = x;
	    prprpos.pos.y = y;
	    if (ip->wind.needlock)
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    (void)pf_text(prprpos, ip->rtextop, ip->pf, ddstruct->ptr1);
	    if (ip->wind.needlock)
		(void)pw_unlock(ip->wind.pixwin);
	    (void)pr_destroy(prprpos.pr);
	    break;
	case MARKER:
	    vp = (porttype *) ddstruct->ptr2;	/* ptr2 = &viewport */
	    devscale(vp->xmin, vp->ymin, vwport.xmin, i1);
	    devscale(vp->xmax, vp->ymax, vwport.xmax, i2);
	    vwport.ymax = ip->screen->pr_height - 1 - i1;	/* flip yhrdwr */
	    vwport.ymin = ip->screen->pr_height - 1 - i2;	/* int2 = char position */
	    x = ip->ddcp[0] - (_core_defaultfont->pf_defaultsize.x / 2) -
	      vwport.xmin;
	    y = ip->screen->pr_height - 1 + (_core_defaultfont->pf_defaultsize.y * 3 / 8) - ip->ddcp[1] -
	      vwport.ymin;
	    prprpos.pr = pr_region(ip->screen, vwport.xmin, vwport.ymin,
	      vwport.xmax - vwport.xmin + 1, vwport.ymax - vwport.ymin + 1);
	    prprpos.pos.x = x;
	    prprpos.pos.y = y;
	    if (ip->wind.needlock)
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    (void)pf_text(prprpos, ip->rtextop, _core_defaultfont, ddstruct->ptr1);
	    if (ip->wind.needlock)
		(void)pw_unlock(ip->wind.pixwin);
	    (void)pr_destroy(prprpos.pr);
	    break;
	case POLYGN2:
	case POLYGN3:{
		int erx0 = 32767, erx1 = 0, ery0 = 32767, ery1 = 0;
		struct pixrect *patternpr;
		struct pr_pos *prvptr, *vp;

		i1 = ddstruct->int1;	/* vertex count */
		vptr1 = (vtxtype *) (ddstruct->ptr1);
		prvptr = (struct pr_pos *) _core_prvtxlist;
		for (i = 0; i < i1; i++) {
		    devscale(vptr1[i].x, vptr1[i].y, prvptr[i].x, prvptr[i].y);
		    prvptr[i].y = ip->screen->pr_height - 1 - prvptr[i].y;
		}
		vp = prvptr;
		for (i = 0; i < i1; i++) {
		    if (erx0 > vp->x)
			erx0 = vp->x;
		    if (erx1 < vp->x)
			erx1 = vp->x;
		    if (ery0 > vp->y)
			ery0 = vp->y;
		    if (ery1 < vp->y)
			ery1 = vp->y;
		    vp++;
		}
		patternpr = (struct pixrect *) mem_create(erx1 - erx0 + 1, ery1 - ery0 + 1, 1);
		(void)pr_replrop(patternpr, 0, 0, patternpr->pr_width, patternpr->pr_height, PIX_SRC, &ip->prmask, 0, 0);
		(void)pr_polygon_2((ip->wind.pixwin)->pw_pixrect, 0, 0, 1, &(ddstruct->int1), prvptr, ip->fillop, patternpr, -erx0, -ery0);
		(void)pr_destroy(patternpr);
	    }
	    break;
	case RASPUT:
	    rptr = (rast_type *) ddstruct->ptr1;
	    mprdata.md_linebytes = mpr_linebytes(rptr->width, rptr->depth);
	    mprdata.md_image = (short *) rptr->bits;
	    mprect.pr_size.x = rptr->width;
	    mprect.pr_size.y = rptr->height;
	    mprect.pr_depth = rptr->depth;
	    p1ptr = (ipt_type *) ddstruct->ptr2;
	    p2ptr = (ipt_type *) ddstruct->ptr3;
	    width = rptr->width;
	    height = rptr->height;
	    xs = 0;
	    ys = 0;
	    x = ip->ddcp[0];
	    y = ip->screen->pr_height - 1 - ip->ddcp[1];
	    y -= height;
	    devscale(p1ptr->x, p1ptr->y, i1, i3);	/* vwport xmin,ymin */
	    devscale(p2ptr->x, p2ptr->y, i2, i4);	/* vwport xmax,ymax */
	    i3 = ip->screen->pr_height - 1 - i3;
	    i4 = ip->screen->pr_height - 1 - i4;
	    if (x < i1) {
		xs = i1 - x;
		width -= xs;
		x = i1;
	    }
	    if (x + width - 1 > i2)
		width = i2 - x + 1;
	    if (width <= 0)
		break;
	    if (y < i4) {
		ys = i4 - y;
		height -= ys;
		y = i4;
	    }
	    if (y + height - 1 > i3)
		height = i3 - y + 1;
	    if (height <= 0)
		break;
	    i1 = (ddstruct->logical) ? ip->fillrastop : ip->cursorrop;
	    if (ip->wind.needlock)
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    (void)pr_rop(ip->screen, x, y, width, height, i1 | PIX_DONTCLIP,
		   &mprect, xs, ys);
	    if (ip->wind.needlock)
		(void)pw_unlock(ip->wind.pixwin);
	    break;
	case RASGET:
	    rptr = (rast_type *) ddstruct->ptr1;
	    p1ptr = (ipt_type *) ddstruct->ptr2;
	    p2ptr = (ipt_type *) ddstruct->ptr3;
	    devscale(p1ptr->x, p1ptr->y, xs, i1);	/* raster xmin,ymin */
	    devscale(p2ptr->x, p2ptr->y, i2, ys);	/* raster xmax,ymax */
	    width = i2 - xs + 1;
	    height = ys - i1 + 1;
	    x = ddstruct->int1;
	    y = ddstruct->int2;
	    if (ddstruct->logical) {	/* if sizing raster */
		rptr->depth = 1;
		rptr->width = width;
		rptr->height = height;
		rptr->bits = 0;
		break;
	    }
	    if (!rptr->bits)
		return (1);
	    mprdata.md_linebytes = mpr_linebytes(rptr->width, rptr->depth);
	    mprdata.md_image = (short *) rptr->bits;
	    mprect.pr_size.x = rptr->width;
	    mprect.pr_size.y = rptr->height;
	    mprect.pr_depth = rptr->depth;
	    if (ip->wind.needlock)
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    (void)pr_rop(&mprect, x, y, width, height, PIX_SRC,
		   ip->screen, xs, ip->screen->pr_height - 1 - ys);
	    if (ip->wind.needlock)
		(void)pw_unlock(ip->wind.pixwin);
	    break;
	case LOCK:
	    if (++ip->lockcount > 1)
		break;
	    if (ip->curstrk) {
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		ip->wind.needlock = FALSE;
	    }
	    break;
	case UNLOCK:
	    if (ip->lockcount == 0)
		break;
	    if (--ip->lockcount > 0)
		break;
	    if (ip->curstrk) {
		(void)pw_unlock(ip->wind.pixwin);
		ip->wind.needlock = TRUE;
	    }
	    break;
	case CURSTRKON:
	    if (ip->curstrk)
		break;
	    ip->curstrk = TRUE;
	    if (ip->lockcount) {
		(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
	    } else
		ip->wind.needlock = TRUE;
	    break;
	case CURSTRKOFF:
	    if (!ip->curstrk)
		break;
	    ip->curstrk = FALSE;
	    if (ip->lockcount) {
		(void)pw_unlock(ip->wind.pixwin);
	    } else
		ip->wind.needlock = FALSE;
	    break;
	default:
	    break;
    }
    return (0);
}
