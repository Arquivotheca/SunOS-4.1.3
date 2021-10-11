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
static char sccsid[] = "@(#)cg4dd.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
   Device dependent core graphics driver for SUN cg4 color display
*/

#include        <sunwindow/window_hs.h>
#include	"coretypes.h"
#include	"corevars.h"
#include        "langnames.h"
#include	<sys/file.h>
#include	<sun/fbio.h>
#include	<strings.h>
#define NULL 0

extern struct pixrectops mem_ops;
extern PIXFONT *_core_defaultfont;

#include	"cgdd.h"

static struct mpr_data mprdata = {0, 0, 0, 0, 1, 0};
static struct pixrect mprect = {&mem_ops, 0, 0, 0, (caddr_t)&mprdata};

#define devscale(x_ndc_coord, y_ndc_coord, x_dev_coord, y_dev_coord)	\
	if (ip->screen->pr_height != ip->screen->pr_width) {		\
	    if (ip->fullx){						\
		    x_dev_coord = (x_ndc_coord + (x_ndc_coord>>3)) >>5;	\
		    y_dev_coord = ip->maxy - ( ((y_ndc_coord +		\
			    (y_ndc_coord >> 3)) >> 5) + ip->yoff);	\
	    } else if (ip->fully) {					\
		    x_dev_coord = (( x_ndc_coord - (x_ndc_coord >> 3)) >> 5)\
				    + ip->xoff;				\
		    y_dev_coord = ip->maxy - ((y_ndc_coord -		\
			    (y_ndc_coord >> 3)) >> 5);			\
	    } else {							\
		    x_dev_coord = (x_ndc_coord<<10)/ip->scale+ip->xoff;	\
		    y_dev_coord = ip->maxy-((y_ndc_coord<<10)/ip->scale);\
	    }								\
	} else {							\
	    if (ip->fullx){						\
		    x_dev_coord = x_ndc_coord >> 5;			\
		    y_dev_coord = ip->maxy -((y_ndc_coord>>5)+ip->yoff);\
	    } else if (ip->fully) {					\
		    x_dev_coord = ( x_ndc_coord >> 5) + ip->xoff;	\
		    y_dev_coord = ip->maxy - (y_ndc_coord >> 5);	\
	    } else {							\
		    x_dev_coord = (x_ndc_coord<<10)/ip->scale+ip->xoff;	\
		    y_dev_coord = ip->maxy-((y_ndc_coord<<10)/ip->scale);\
	    }								\
	}
#define vecscale( xin, yin, xout, yout)					\
	if (ip->screen->pr_height != ip->screen->pr_width) {		\
	    if (ip->fullx) {						\
		xout = xin + (xin >> 3);				\
		yout = yin + (yin >> 3);				\
	    } else if (ip->fully)	{				\
		xout = xin - (xin >> 3);				\
		yout = yin - (yin >> 3);				\
	    } else {							\
		xout = (xin << 15) / ip->scale;				\
		yout = (yin << 15) / ip->scale;				\
	    }								\
	} else {							\
	    if (ip->fullx || ip->fully) {				\
		xout = xin;						\
		yout = yin;						\
	    } else {							\
		xout = (xin << 15) / ip->scale;				\
		yout = (yin << 15) / ip->scale;				\
	    }								\
	}

static int planes = 255;
/*------------------------------------*/
FORTNAME(cg4dd)
PASCNAME(cg4dd)
cg4dd(ddstruct) register ddargtype *ddstruct;
{
    register short i1,i2,i3,i4;
	int i, error = 0;
	float fx, fy, aspect_ratio;
	rast_type *rptr;
	ipt_type *p1ptr, *p2ptr;
        int x,y,xs,ys,width,height;
	u_char *p1, *p2, *p3;
	porttype vwport, *vp;
	struct pixrect *subpr;		/* viewport pixrect for clip */
	struct cgddvars *ip;		/* pointer to instance state vars */
	vtxtype *vptr1, *vptr2;
	struct pr_prpos prprpos;

	ip = (struct cgddvars *)ddstruct->instance;
	switch(ddstruct->opcode) {
	case INITZB:				/* init zbuffer */
		if ( !_core_init_pwzbuffer( ip->screen->pr_width,
			ip->screen->pr_height, &ip->zbuffer, &ip->cutaway)) {
		    ddstruct->logical = TRUE;
		    ip->openzbuffer = TRUE;
		} else (void)printf("can't init zbuffer\n");
		break;
	case TERMZB:
		if (ip->openzbuffer) {
			_core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
			if (ip->xarr) free( (char *)ip->xarr);
			if (ip->zarr) free( (char *)ip->zarr);
			ip->openzbuffer = FALSE;
		} else return( 1);
		break;
	case SETZBCUT:
		if (ip->openzbuffer) {		/* ptr1 -> xarr; ptr2 -> zarr */
			if (ip->xarr) free( (char *)ip->xarr);
			if (ip->zarr) free( (char *)ip->zarr);
			ip->cutarraysize = ddstruct->int1;
			ip->xarr = (float *)malloc((unsigned)(sizeof(float) *
				ip->cutarraysize));
			ip->zarr = (float *)malloc((unsigned)(sizeof(float) *
				ip->cutarraysize));
			for (i1=0; i1<ip->cutarraysize; i1++) {
			    *(ip->xarr + i1) = *((float*)ddstruct->ptr1+i1);
			    *(ip->zarr + i1) = *((float*)ddstruct->ptr2+i1);
			}
			_core_set_pwzbufcut( ip->cutaway, ip->xarr, ip->zarr,
				ip->cutarraysize);
		} else return( 1);
		break;
	case INITIAL: {
		static char cgfourdev[] = "/dev/cgfour0";
		struct screen screen;
		int pwprfd;
		struct  cms_map cmap;
    		viewsurf *surfp;

		surfp = (viewsurf*)ddstruct->ptr1;
		(void)win_initscreenfromargv(&screen, (char **)0);
		if (*surfp->vsurf.screenname != '\0') {
			if (_core_chkdev(surfp->vsurf.screenname,
			    FBTYPE_SUN4COLOR, ddstruct->int1) >= 0)
				(void)strncpy(screen.scr_fbname,
				    surfp->vsurf.screenname, SCR_NAMESIZE);
			else {
		    		ddstruct->logical = FALSE;
		    		return( 1);
		    	}
		}
		else if (_core_chkdev("/dev/fb", FBTYPE_SUN4COLOR,
		    ddstruct->int1) >= 0)
			(void)strncpy(screen.scr_fbname, "/dev/fb", SCR_NAMESIZE);
		else if (_core_getfb(cgfourdev,FBTYPE_SUN4COLOR,
		    ddstruct->int1) >= 0)
			(void)strncpy(screen.scr_fbname, cgfourdev, SCR_NAMESIZE);
		else {
		    ddstruct->logical = FALSE;
		    return( 1);
		    }
		if (!ddstruct->int1)
			(void)strncpy(screen.scr_kbdname, "NONE", SCR_NAMESIZE);
		if ((pwprfd = win_screennew(&screen)) < 0) {
		    ddstruct->logical = FALSE;
		    return( 1);
		    }

		(void)win_screenget(pwprfd, &screen);
		ddstruct->logical = TRUE;
		ip = (struct cgddvars *) malloc(sizeof(struct cgddvars));

		ip->wind.pixwin = pw_open(pwprfd);
		ip->wind.winfd = pwprfd;
		ip->wind.needlock = FALSE;
		ip->wind.rawdev = TRUE;
		ip->screen = ip->wind.pixwin->pw_pixrect;
		ip->maxz = 32767;
		ip->maxy = ip->screen->pr_height - 1;
		ip->wind.ndcxmax = MAX_NDC_COORD;
		if (ip->screen->pr_height == ip->screen->pr_width)
			ip->wind.ndcymax = MAX_NDC_COORD;
		else
			ip->wind.ndcymax = (MAX_NDC_COORD * 3) / 4;
		ip->curstrk = FALSE;
		ip->lockcount = 0;
		ip->fullx = TRUE;
		ip->fully = FALSE;
		ip->xoff = 0;
		ip->yoff = 0;
		ip->lineop = ip->fillop = PIX_SRC;	/* default rop funcs */
		ip->fillrastop = ip->RAS8func = PIX_SRC;
		ip->cursorrop = PIX_NOT(PIX_SRC);
		ip->vtextop = ip->rtextop = PIX_SRC;
		ip->lineindex = 1; ip->textindex = 1; ip->fillindex = 1;
		ip->pf = _core_defaultfont;		/* load default font */
		ip->openfont = -1;
		ip->lineop = PIX_SET;
		for (i1=0; i1<256; i1++) {	/* load color map 0 */
			ip->red[i1] = 0; ip->green[i1] = 0; ip->blue[i1] = 0;
			}
		ip->red[0] = 150; ip->green[0] = 150;	/* grey backgrnd */
		ip->blue[0] = 150;
		for (i1=2; i1<64; i1++) ip->red[i1] = (i1<<2);
		for (i1=64; i1<128; i1++) ip->green[i1] = (i1-64)<<2;
		for (i1=128; i1<192; i1++) ip->blue[i1] = (i1-128)<<2;
		for (i1=192; i1<256; i1++) {
			ip->red[i1] = (i1-192)<<2; ip->green[i1] = (i1-192)<<2;
			}

		(void)strncpy(ip->cmsname,surfp->vsurf.cmapname,DEVNAMESIZE);
		cmap.cm_red = ip->red;
		cmap.cm_green = ip->green;
		cmap.cm_blue = ip->blue;
		_core_standardcmap(ip->cmsname, (struct colormapseg *)0, &cmap);
		(void)pr_putattributes( ip->screen, &planes);
		(void)pw_setcmsname( ip->wind.pixwin, ip->cmsname);
		(void)pw_putcolormap(ip->wind.pixwin, 0,256,
			ip->red,ip->green,ip->blue);

		surfp->windptr = &ip->wind;
		surfp->vsurf.instance = (int) ip;
		(void)strncpy(surfp->vsurf.screenname, screen.scr_fbname,DEVNAMESIZE);
		(void)strncpy(surfp->vsurf.windowname, screen.scr_rootname,
		    DEVNAMESIZE);
		surfp->vsurf.windowfd = pwprfd;
		surfp->windptr = &ip->wind;
	        surfp->erasure = TRUE;		/* view surface capabilities */
	        surfp->txhardwr = TRUE;
		surfp->clhardwr = TRUE;
	        surfp->hihardwr = TRUE;
		surfp->lshardwr = TRUE;
		surfp->lwhardwr = TRUE;
		_core_winupdate(&ip->wind);
		if (surfp->vsurf.ptr != NULL)
			_core_setadjacent(*surfp->vsurf.ptr, &screen);
		}
		break;
	case TERMINATE:
		if (ip->pf != _core_defaultfont) 
			(void)pf_close( ip->pf);
		(void)pw_close( ip->wind.pixwin);
		(void)win_screendestroy( ip->wind.winfd);
		(void)close(ip->wind.winfd);
		free( (char *)ip);
		break;
	case SETNDC:
		fx = ddstruct->float1;
		fy = ddstruct->float2;
		ip->maxz = (int) (ddstruct->float3 * 32767.);
		aspect_ratio = fy / fx;
		if (aspect_ratio <= ((float)ip->screen->pr_height /
				(float)ip->screen->pr_width) )
			{	/* full 32K NDC maps to full ip->screen width */
			ip->xoff = 0;
			ip->yoff = (int) ((ip->screen->pr_height -
				ip->screen->pr_width * aspect_ratio) / 2.0);
			ip->fullx = TRUE; ip->fully = FALSE;
			ip->wind.ndcxmax = MAX_NDC_COORD;
			ip->wind.ndcymax = aspect_ratio * MAX_NDC_COORD;
			}
		else if (fy < 1.0)
			{	/* fy*32K NDC maps to full ip->screen height */
			ip->yoff = 0;
			ip->xoff = (int) ((ip->screen->pr_width -
				ip->screen->pr_height / aspect_ratio) / 2.0);
			ip->scale = (int) (fy * 32768. /
				ip->screen->pr_height * 1024.);
			ip->fullx = FALSE; ip->fully = FALSE;
			ip->wind.ndcxmax = MAX_NDC_COORD;
			ip->wind.ndcymax = aspect_ratio * MAX_NDC_COORD;
			}
		else
			{	/* full 32K NDC maps to full ip->screen height*/
			ip->yoff = 0;
			ip->xoff = (int) ((ip->screen->pr_width -
				ip->screen->pr_height / aspect_ratio) / 2.0);
			ip->fullx = FALSE; ip->fully = TRUE;
			ip->wind.ndcymax = MAX_NDC_COORD;
			ip->wind.ndcxmax = MAX_NDC_COORD / aspect_ratio;
			}
		_core_winupdate(&ip->wind);
		break;
	case CLEAR:
		if (ip->wind.needlock)
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		(void)pr_rop(ip->screen,0,0,ip->screen->pr_width,
			ip->screen->pr_height, PIX_CLR,(struct pixrect *)0,0,0);	/*zero screen*/
		if (ip->wind.needlock)
			(void)pw_unlock(ip->wind.pixwin);
		if (ip->openzbuffer)
			    _core_clear_pwzbuffer( ip->zbuffer, 
				(struct pixrect *)0);
		break;
	case GETTAB:
		p1 = (u_char *)ddstruct->ptr1;
		p2 = (u_char *)ddstruct->ptr2;
		p3 = (u_char *)ddstruct->ptr3;
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    *p1++ = ip->red[i1];
		    *p2++ = ip->green[i1];
		    *p3++ = ip->blue[i1];
		    }
		break;
	case SETTAB:
		p1 = (u_char *)ddstruct->ptr1;
		p2 = (u_char *)ddstruct->ptr2;
		p3 = (u_char *)ddstruct->ptr3;
		for (i1=ddstruct->int1; i1<=ddstruct->int2; i1++) {
		    ip->red[i1] =   *p1++;
		    ip->green[i1] = *p2++;
		    ip->blue[i1] =  *p3++;
		    }
		(void)pw_putcolormap(ip->wind.pixwin, 0, 256,
			ip->red,ip->green,ip->blue);
		break;
	case SETLCOL:
		ip->lineindex = ddstruct->int1;
		if (ddstruct->int2 == 1) {		/* XORing */
		    ip->lineop = PIX_SRC ^ PIX_DST;
		} else if (ddstruct->int2 == 2) {	/* ORing */
		    if (ddstruct->int3) {	/* erase */
			ip->lineop = ~PIX_SRC & PIX_DST;
		    } else {			/* paint */
			ip->lineop = PIX_SRC | PIX_DST;
		    }
		} else {
		    if (ddstruct->int3) {		/* erasing */
			ip->lineop = PIX_CLR;
		    } else {				/* painting */
			ip->lineop = PIX_SRC;
		    }
		}
		break;
	case SETTCOL: 			/* select colors for text */
		ip->textindex = ddstruct->int1;
		if (ddstruct->int2 == 1) {		/* XORing */
		    ip->rtextop = ip->vtextop = PIX_SRC ^ PIX_DST;
		} else if (ddstruct->int2 == 2) {	/* ORing */
		    if (ddstruct->int3) {	/* erase */
			ip->rtextop = ip->vtextop = (~PIX_SRC & PIX_DST);
		    } else {			/* paint */
			ip->rtextop = ip->vtextop = PIX_SRC | PIX_DST;
		    }
		} else {
		    if (ddstruct->int3) {		/* erasing */
			ip->rtextop = ip->vtextop = PIX_CLR;
		    } else {				/* painting */
			ip->rtextop = ip->vtextop = PIX_SRC;
		    }
		}
		break;
	case SETFCOL:			/* select colors for polygon fill */
		ip->fillindex = ddstruct->int1;
		if (ddstruct->int2 == 1) {		/* XORing */
		    ip->fillop = ip->fillrastop = ip->RAS8func =
		    PIX_SRC ^ PIX_DST;
		} else if (ddstruct->int2 == 2) {	/* ORing */
		    if (ddstruct->int3) {	/* erase */
			ip->fillop = ip->RAS8func = ip->fillrastop =
			~PIX_SRC & PIX_DST;
		    } else {			/* paint */
			ip->fillop = ip->RAS8func = ip->fillrastop =
			PIX_SRC | PIX_DST;
		    }
		} else {
		    if (ddstruct->int3) {		/* erasing */
			ip->fillop = ip->fillrastop = ip->RAS8func = PIX_CLR;
		    } else {				/* painting */
			ip->fillop = ip->fillrastop = ip->RAS8func = PIX_SRC;
		    }
		}
		break;
	case ECHOMOVE:
	case MOVE:
		devscale(ddstruct->int1,ddstruct->int2,ip->ddcp[0],ip->ddcp[1]);
		break;
	case SETWIDTH:
		if (ip->screen->pr_height != ip->screen->pr_width) {
		    if (ip->fullx) {
			    i = ddstruct->int1;
			    ip->linewidth = (i+(i>>3)+16)>>5;
		    } else if (ip->fully) {
			    i = ddstruct->int1;
			    ip->linewidth = (i-(i>>3)+16)>>5;
		    } else {
			    i = (int) ((float) (ip->scale) / 2048.);
			    ip->linewidth = ((ddstruct->int1 + i) << 10) /
					ip->scale;
			    }
		} else {
		    if (ip->fullx || ip->fully){
			    i = ddstruct->int1;
			    ip->linewidth = (i+16)>>5;
		    } else {
			    i = (int) ((float) (ip->scale) / 2048.);
			    ip->linewidth = ((ddstruct->int1 + i) << 10) /
					ip->scale;
		    }
		}
		break;
	case SETSTYL:
		ip->linestyle = ddstruct->int1; break;
	case SETPISTYL:
		ip->polyintstyle = ddstruct->int1; break;
	case ECHOLINE:
	case LINE:
		devscale(ddstruct->int1, ddstruct->int2, i1, i2);
		i = ip->lineop | PIX_COLOR( ip->lineindex);	
		if ( ip->linestyle || (ip->linewidth > 1)) 
			_core_csimline( ip->ddcp[0],ip->ddcp[1],i1,
			i2, i, &ip->wind, ip->linewidth,ip->linestyle);
		else {
			if (ip->wind.needlock)
				(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
			(void)pr_vector(ip->screen,ip->ddcp[0],ip->ddcp[1],
			    i1,i2, i, ip->lineindex);
			if (ip->wind.needlock)
				(void)pw_unlock(ip->wind.pixwin);
		}
		ip->ddcp[0] = i1; ip->ddcp[1] = i2;
		break;
	case SETFONT:				/* font text attribute */
		ip->ddfont = ddstruct->int1;
		break;
	case SETUP:				/* text up direction */
		ip->ddup = *((ipt_type*)ddstruct->ptr1);	/* NDC coords */
		vecscale( ip->ddup.x, ip->ddup.y, x, y);
		ip->ddup.x =  x;
		ip->ddup.y =  -y;
		/* ddup is character height vector in dev coords << 5 */
		break;
	case SETPATH:				/* text path direction */
		ip->ddpath = *((ipt_type*)ddstruct->ptr1);	/* in NDC */
		vecscale( ip->ddpath.x, ip->ddpath.y, x, y);
		ip->ddpath.x =  x;
		ip->ddpath.y =  -y;
		/* ddpath is character width vector in dev coords << 5 */
		break;
	case SETSPACE:				/* text leading << 5*/
		ip->ddspace = *((ipt_type*)ddstruct->ptr1);
		vecscale( ip->ddspace.x, ip->ddspace.y, x, y);
		ip->ddspace.x =  x;
		ip->ddspace.y =  -y;
		break;
	case VTEXT:				/* vector text */
		vp = (porttype*)ddstruct->ptr2;	/* ptr2 = &viewport */
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		i = ip->vtextop | PIX_COLOR(ip->textindex);	
		_core_cprvectext( ddstruct->ptr1, ip->ddfont,
			&ip->ddup,&ip->ddpath,&ip->ddspace, ip->ddcp[0],
			ip->ddcp[1],&vwport,i, &ip->wind);
		break;
	case TEXT:				/* raster text */
 		if (ip->ddfont != ip->openfont) {
		    if (ip->pf != _core_defaultfont) 
			(void)pf_close( ip->pf);
		    ip->openfont = ip->ddfont;
		    switch ( ip->ddfont ) {
		    case 0: ip->pf = pf_open(
			    "/usr/lib/fonts/fixedwidthfonts/gallant.r.10");
			    break;
		    case 1: ip->pf = pf_open(
			    "/usr/lib/fonts/fixedwidthfonts/gacha.r.8");
			    break;
		    case 2: ip->pf =
			    pf_open("/usr/lib/fonts/fixedwidthfonts/sail.r.6");
			    break;
		    case 3: ip->pf =
			    pf_open("/usr/lib/fonts/fixedwidthfonts/gacha.b.8");
			    break;
		    case 4: ip->pf =
			    pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.r.8");
			    break;
		    case 5: ip->pf =
			    pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.b.8");
			    break;
		    default: ip->pf = _core_defaultfont;	/* load default font */
			    break;
		    }
		    if ( !ip->pf) {
			ip->pf = _core_defaultfont;
		    }
		}    
		i = ip->rtextop | PIX_COLOR(ip->textindex);	
		vp = (porttype*)ddstruct->ptr2;
		devscale(vp->xmin, vp->ymin, vwport.xmin, i1);
		devscale(vp->xmax, vp->ymax, vwport.xmax, i2);
		vwport.ymax = i1;
		vwport.ymin = i2;
		x = ip->ddcp[0]+(ddstruct->int2*ip->pf->pf_defaultsize.x) -
			vwport.xmin;
		y = ip->ddcp[1] + (ip->pf->pf_defaultsize.y/2) - vwport.ymin;
		subpr = pr_region(ip->screen,vwport.xmin, vwport.ymin,
		    vwport.xmax-vwport.xmin+1, vwport.ymax-vwport.ymin+1);
		if (ip->wind.needlock)
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		prprpos.pr = subpr;
		prprpos.pos.x = x;
		prprpos.pos.y = y;
		(void)pf_ttext(prprpos, i ,ip->pf, ddstruct->ptr1);
		if (ip->wind.needlock)
			(void)pw_unlock(ip->wind.pixwin);
		(void)pr_destroy( subpr);
		break;
	case MARKER:
		i = ip->rtextop | PIX_COLOR(ip->textindex);	
		vp = (porttype*)ddstruct->ptr2;
		devscale(vp->xmin, vp->ymin, vwport.xmin, i1);
		devscale(vp->xmax, vp->ymax, vwport.xmax, i2);
		vwport.ymax = i1;
		vwport.ymin = i2;
		x = ip->ddcp[0]-(_core_defaultfont->pf_defaultsize.x/2) -
			vwport.xmin;
		y = ip->ddcp[1]-vwport.ymin +(_core_defaultfont->pf_defaultsize.y*3/8);
		subpr = pr_region(ip->screen,vwport.xmin, vwport.ymin,
		    vwport.xmax-vwport.xmin+1, vwport.ymax-vwport.ymin+1);
		if (ip->wind.needlock)
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		prprpos.pr = subpr;
		prprpos.pos.x = x;
		prprpos.pos.y = y;
		(void)pf_ttext( prprpos, i ,_core_defaultfont, ddstruct->ptr1);
		if (ip->wind.needlock)
			(void)pw_unlock(ip->wind.pixwin);
		(void)pr_destroy( subpr);
		break;
	case POLYGN2: {
		struct pr_pos *prvptr;
                i1 =  ddstruct->int1;           /* vertex count */
                vptr1 = (vtxtype *)(ddstruct->ptr1);
                prvptr = (struct pr_pos *)_core_prvtxlist;
                for (i=0; i<i1; i++) {
                    devscale(vptr1[i].x, vptr1[i].y, prvptr[i].x, prvptr[i].y);
                }
                i2 = ip->fillop | PIX_COLOR( ip->fillindex);
                (void)pr_polygon_2( (ip->wind.pixwin)->pw_pixrect, 0, 0,
                        1, &(ddstruct->int1), prvptr, i2, 
			(struct pixrect *)0,0,0);
	    }
	    break;
	case POLYGN3:
		i1 =  ddstruct->int1;		/* vertex count */
		vptr1 = (vtxtype *)(ddstruct->ptr1);
		vptr2 = (vtxtype *)(ddstruct->ptr2);
		for (i=0; i<i1; i++) {
		    vptr2[i] = vptr1[i];
		    devscale(vptr1[i].x, vptr1[i].y, vptr2[i].x, vptr2[i].y);
		    }
		i = ip->fillop | PIX_COLOR( ip->fillindex);
		_core_cregion3( vptr2, i1, i, &ip->wind, ip->zbuffer,
			ip->cutaway, ip->polyintstyle, ddstruct->int3);
		break;
	case RASPUT:
		rptr = (rast_type*)ddstruct->ptr1;
		mprdata.md_linebytes =
			mpr_linebytes(rptr->width,rptr->depth);
		mprdata.md_image = (short *)rptr->bits;
		mprect.pr_size.x = rptr->width;
		mprect.pr_size.y = rptr->height;
		mprect.pr_depth  = rptr->depth;
		p1ptr = (ipt_type*)ddstruct->ptr2;
		p2ptr = (ipt_type*)ddstruct->ptr3;
		width = rptr->width;
		height = rptr->height;
		xs = 0; ys = 0;
		x = ip->ddcp[0]; y = ip->ddcp[1];
		y = y-height+1;
		devscale(p1ptr->x, p1ptr->y, i1, i3);	/* viewport xmin,ymin */
		devscale(p2ptr->x, p2ptr->y, i2, i4);	/* viewport xmax,ymax */
				/* y bottom = ymax in device coords */
				/* y top = ymin in device coords */
		if (x<i1){ xs=i1-x; width-=xs; x+=i1;}
		if (x+width-1>i2) width=i2-x+1;
		if (width<=0) break;
		if (y < i4) { ys = i4-y; height -= ys; y=i4;}
		if (y+height-1 > i3) height = i3-y+1;
		if (height<=0) break;
		i = (rptr->depth == 1) ? ip->fillrastop : ip->RAS8func;
		i |= PIX_COLOR( ip->fillindex);
		if (ip->wind.needlock)
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		(void)pr_rop( ip->screen, x, y, width, height, i|PIX_DONTCLIP,
			&mprect, xs, ys);
		if (ip->wind.needlock)
			(void)pw_unlock(ip->wind.pixwin);
		break;
	case RASGET:
		rptr = (rast_type*)ddstruct->ptr1;
		p1ptr = (ipt_type*)ddstruct->ptr2;
		p2ptr = (ipt_type*)ddstruct->ptr3;
		devscale(p1ptr->x, p1ptr->y, xs, i1);	/* raster xmin,ymin */
		devscale(p2ptr->x, p2ptr->y, i2, ys);	/* raster xmax,ymax */
		width = i2 - xs + 1;
		height = i1 - ys + 1;
		x = ddstruct->int1;
		y = ddstruct->int2;
		if (ddstruct->logical) {		/* if sizing raster */
		    rptr->depth = 8;
		    rptr->width = width;
		    rptr->height = height;
		    rptr->bits = 0; break;
		    }
		if (!rptr->bits) {error = 1; break;}
		mprdata.md_linebytes =
			mpr_linebytes(rptr->width,rptr->depth);
		mprdata.md_image = (short *)rptr->bits;
		mprect.pr_size.x = rptr->width;
		mprect.pr_size.y = rptr->height;
		mprect.pr_depth  = rptr->depth;
		if (ip->wind.needlock)
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		(void)pr_rop( &mprect,x,y,width,height,PIX_SRC, ip->screen,
			xs,ys);
		if (ip->wind.needlock)
			(void)pw_unlock(ip->wind.pixwin);
		break;
	case LOCK:
		if (++ip->lockcount > 1)
			break;
		if (ip->curstrk)
			{
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
			ip->wind.needlock = FALSE;
			}
		break;
	case UNLOCK:
		if (ip->lockcount == 0)
			break;
		if (--ip->lockcount > 0)
			break;
		if (ip->curstrk)
			{
			(void)pw_unlock(ip->wind.pixwin);
			ip->wind.needlock = TRUE;
			}
		break;
	case CURSTRKON:
		if (ip->curstrk)
			break;
		ip->curstrk = TRUE;
		if (ip->lockcount)
			{
			(void)pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
			}
		else
			ip->wind.needlock = TRUE;
		break;
	case CURSTRKOFF:
		if (!ip->curstrk)
			break;
		ip->curstrk = FALSE;
		if (ip->lockcount)
			{
			(void)pw_unlock(ip->wind.pixwin);
			}
		else
			ip->wind.needlock = FALSE;
		break;
	default:
		break;
	}
	return(error);
}
