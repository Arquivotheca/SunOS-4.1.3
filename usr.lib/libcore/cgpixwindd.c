/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appears in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)cgpixwindd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
   Device dependent core graphics driver for SUN color bitmap windows 
*/

#include        <sunwindow/window_hs.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	"langnames.h"
#include        "cgpixwindd.h"
#include        <strings.h>
#include        <stdio.h>

char *sprintf();

extern struct pixrectops mem_ops;
extern PIXFONT *_core_defaultfont;

static struct mpr_data mprdata = {0, 0, 0, 0, 1, 0};
static struct pixrect mprect = {&mem_ops, 0, 0, 0, (caddr_t)&mprdata};

extern int _core_gfxenvfd;

#define devscale(x_ndc_coord, y_ndc_coord, x_dev_coord, y_dev_coord)	\
		x_dev_coord = (x_ndc_coord*ip->wind.xscale >> 15)\
			+ ip->wind.xoff;				\
		y_dev_coord = ip->wind.yoff -                           \
		(y_ndc_coord*ip->wind.yscale >> 15)

/*------------------------------------*/
DDFUNC(cgpixwindd)(ddstruct) register ddargtype *ddstruct;
{
    register short i1,i2,i3,i4;
    int i;
    float fx, fy, aspect_ratio;
    rast_type *rptr;
    ipt_type *p1ptr, *p2ptr;
    int x,y,xs,ys,width,height;
    u_char *p1, *p2, *p3;
    porttype vwport, *vp;
    viewsurf *surfp;
    vtxtype *vptr1, *vptr2;
    struct pr_pos *pwvptr;
    register struct cgpwddvars *ip;

	ip = (struct cgpwddvars *)ddstruct->instance;
	if ((ip != 0) && ip->toolpid < 0)
			/* child process has been externally terminated */
		if (ddstruct->opcode != TERMINATE && ddstruct->opcode != TERMZB)
			return(1);
	switch(ddstruct->opcode) {
	case INITIAL:
		{
		char name[DEVNAMESIZE];
		int pwprfd;
		int pid;
		struct screen screen;

		if (!ddstruct->int1 ||
		    _core_get_window_vwsurf(&pwprfd, &pid, COLOR_FB,
			&((viewsurf *)ddstruct->ptr1)->vsurf))
			{
			ddstruct->logical = FALSE;
			return(1);
			}

		ddstruct->logical = TRUE;
		ip = (struct cgpwddvars *)malloc( sizeof(struct cgpwddvars));

		ip->toolpid = pid;
		ip->wind.ndcxmax = MAX_NDC_COORD;
		ip->wind.ndcymax = (MAX_NDC_COORD * 3) / 4;
		ip->wind.pixwin = pw_open(pwprfd);
		ip->wind.winfd = pwprfd;
		ip->wind.rawdev = FALSE;
		ip->maxz = 32767;
		ip->fillop = PIX_NOT(PIX_SRC);	/* default rasterops */
		ip->fillrastop = PIX_SRC;
		ip->cursorrop = PIX_NOT(PIX_SRC);
		ip->rtextop = PIX_SRC;
		ip->vtextop = PIX_SET;
		ip->lineop = PIX_SET;
		ip->pf = _core_defaultfont;		/* load default font */
		ip->openfont = -1;
		ip->openzbuffer=FALSE;

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

		surfp = (viewsurf*)ddstruct->ptr1;
		p1 = 0;
		ip->cmapsize = surfp->vsurf.cmapsize;
		if (ip->cmapsize == 0)  ip->cmapsize = 2;
		if (surfp->vsurf.cmapname[0] == '\0') {
			(void)sprintf( ip->cmsname, "cmap%10D%5D", getpid(),
				ip->wind.winfd);
		} else {
			struct  colormapseg cms;
			struct  cms_map cmap;
			cmap.cm_red = ip->red;
			cmap.cm_green = ip->green;
			cmap.cm_blue = ip->blue;
			(void)strncpy(ip->cmsname,surfp->vsurf.cmapname,DEVNAMESIZE);
			if (_core_sharedcmap( pwprfd,ip->cmsname,&cms,&cmap)) {
				ip->cmapsize = cms.cms_size;
			} else if (_core_standardcmap(ip->cmsname,&cms,&cmap)){
				ip->cmapsize = cms.cms_size;
			}
		}
		(void)pw_setcmsname( ip->wind.pixwin, ip->cmsname);
		(void)pw_putcolormap( ip->wind.pixwin, 0, ip->cmapsize,
			ip->red, ip->green, ip->blue);

		surfp->windptr = &ip->wind;
		surfp->vsurf.instance = (int) ip;
		surfp->vsurf.cmapsize = ip->cmapsize;
		(void)strncpy(surfp->vsurf.cmapname, ip->cmsname, DEVNAMESIZE);
		(void)win_screenget(pwprfd, &screen);
		(void)strncpy(surfp->vsurf.screenname,screen.scr_fbname,DEVNAMESIZE);
		(void)win_fdtoname(pwprfd, name);
		(void)strncpy(surfp->vsurf.windowname, name, DEVNAMESIZE);
		surfp->vsurf.windowfd = pwprfd;
	        surfp->erasure = TRUE;		/* view surface capabilities */
	        surfp->txhardwr = TRUE;
		surfp->clhardwr = TRUE;
		surfp->hihardwr = TRUE;
		surfp->lshardwr = TRUE;
		surfp->lwhardwr = TRUE;
		_core_winupdate(&ip->wind);
		}
		break;
	case TERMINATE:
		if (ip->openzbuffer) {
			_core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
			ip->openzbuffer = FALSE;
			}
		if (ip->pf != _core_defaultfont) 
			(void)pf_close( ip->pf);
		(void)pw_close( ip->wind.pixwin);
		if (_core_gfxenvfd == ip->wind.winfd) {
			(void)win_removeblanket(_core_gfxenvfd);
			(void)close(_core_gfxenvfd);
			_core_gfxenvfd = -1;
		}
		else if (ip->toolpid >= 0) {
			(void)win_setowner(ip->wind.winfd, ip->toolpid);
			(void)close(ip->wind.winfd);
			(void)kill(ip->toolpid, 9);
		}
		free((char *)ip);
		break;
	case INITZB:				/* init zbuffer */
		if (!_core_init_pwzbuffer( ip->wind.winwidth, ip->wind.winheight,
		    &ip->zbuffer, &ip->cutaway)) {
		    ddstruct->logical = TRUE;
		    ip->openzbuffer = TRUE;
		    }
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
			ip->xarr = (float*)malloc((unsigned)(sizeof(float) *
				ip->cutarraysize));
			ip->zarr = (float*)malloc((unsigned)(sizeof(float) *
				ip->cutarraysize));
			for (i1=0; i1<ip->cutarraysize; i1++) {
				*(ip->xarr + i1) = *((float*)ddstruct->ptr1+i1);
				*(ip->zarr + i1) = *((float*)ddstruct->ptr2+i1);
			}
			_core_set_pwzbufcut( ip->cutaway, ip->xarr, ip->zarr,
				ip->cutarraysize);
		} else return( 1);
		break;
	case SETNDC:
		fx = ddstruct->float1;
		fy = ddstruct->float2;
		ip->maxz = (int) (ddstruct->float3 * 32767.);
		aspect_ratio = fy / fx;
		if (aspect_ratio <= 1.) { /* x is longest NDC dimension */
			ip->wind.ndcxmax = MAX_NDC_COORD;
			ip->wind.ndcymax = aspect_ratio * MAX_NDC_COORD;
			}
		else {			/* y is longest NDC dimension */
			ip->wind.ndcymax = MAX_NDC_COORD;
			ip->wind.ndcxmax = MAX_NDC_COORD / aspect_ratio;
			}
		_core_winupdate(&ip->wind);
		break;
	case CLEAR:
		(void)pw_write(ip->wind.pixwin, 0, 0, ip->wind.winwidth,
			ip->wind.winheight, PIX_CLR, (struct pixrect *)0, 0, 0);
		if (ip->openzbuffer) {
			if (ip->zbuffer->pr_width != ip->wind.winwidth ||
			    ip->zbuffer->pr_height != ip->wind.winheight) {
			    _core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
			    ip->openzbuffer = FALSE;
			    if (!_core_init_pwzbuffer( ip->wind.winwidth,
				ip->wind.winheight,&ip->zbuffer,&ip->cutaway)) {
				ip->openzbuffer = TRUE;
				if (ip->cutarraysize){
				    _core_set_pwzbufcut( ip->cutaway, ip->xarr,
				      ip->zarr, ip->cutarraysize);
				}

			    }
			} else {
			    _core_clear_pwzbuffer( ip->zbuffer, 
				(struct pixrect *)0);
			}
			if (!ip->openzbuffer) return( 1);
		}
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
		(void)pw_putcolormap(ip->wind.pixwin, 0, ip->cmapsize,
			ip->red,ip->green,ip->blue);
		break;
	case SETLCOL:
		ip->lineindex = ddstruct->int1;
		if (ddstruct->int2 == 1) {		/* XORing */
		    ip->lineop = PIX_SRC ^ PIX_DST;
		} else if (ddstruct->int2 == 2) {	/* ORing */
		    if (ddstruct->int3 || !ddstruct->int1) {		
			ip->lineop = ~PIX_SRC & PIX_DST;/* erase */
		    } else {
			ip->lineop = PIX_SRC | PIX_DST;	/* paint */
		    }
		} else {
		    ip->lineop = (ddstruct->int3 || !ddstruct->int1)
		    ? PIX_CLR : PIX_SRC;
		    }
		break;
	case SETTCOL:
		ip->textindex = ddstruct->int1;
		if (ddstruct->int2 == 1) {		/* XORing */
		    ip->rtextop = ip->vtextop = PIX_SRC ^ PIX_DST;
		} else if (ddstruct->int2 == 2) {	/* ORing */
		    if (ddstruct->int3 || !ddstruct->int1) {	/* erase */
			ip->rtextop = ip->vtextop = (~PIX_SRC & PIX_DST);
		    } else {			/* paint */
			ip->rtextop = ip->vtextop = PIX_SRC | PIX_DST;
		    }
		} else {
		    if (ddstruct->int3) {               /* erasing */
		    	ip->rtextop = ip->vtextop = PIX_CLR;
		    } else {                            /* painting */
			ip->rtextop = ip->vtextop = PIX_SRC;
		    }
		}
		break;
	case SETFCOL:
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
		ip->ddcp.x = ddstruct->int1;
		ip->ddcp.y = ddstruct->int2;
		break;
	case SETWIDTH:
		    ip->linewidth = (short)ddstruct->int1 * ip->wind.xscale>>15;
		break;
	case SETSTYL:
		ip->linestyle = ddstruct->int1; break;
	case SETPISTYL:
		ip->polyintstyle = ddstruct->int1; break;
	case ECHOLINE:
	case LINE:
		devscale(ddstruct->int1, ddstruct->int2, i1, i2);
		devscale(ip->ddcp.x, ip->ddcp.y, i3, i4);
		i = ip->lineop | PIX_COLOR( ip->lineindex);
		if ( ip->linestyle || (ip->linewidth > 1)) 
		    _core_cpwsimline(i3,i4,i1,i2, i, &ip->wind, ip->linewidth,
			ip->linestyle);
		else
		    (void)pw_vector(ip->wind.pixwin,i3,i4,i1,i2,i,ip->lineindex);
		ip->ddcp.x = ddstruct->int1;
		ip->ddcp.y = ddstruct->int2;
		break;
	case SETFONT:				/* font text attribute */
		ip->ddfont = ddstruct->int1;
		break;
	case SETUP:				/* char up vector << 5 */
		ip->ddup = *((ipt_type*)ddstruct->ptr1);
		ip->ddup.x = ip->ddup.x * ip->wind.xscale >> 10;
		ip->ddup.y = ip->ddup.y * -ip->wind.yscale >> 10;
		break;
	case SETPATH:				/* char path vector << 5 */
		ip->ddpath = *((ipt_type*)ddstruct->ptr1);
		ip->ddpath.x = ip->ddpath.x * ip->wind.xscale >> 10;
		ip->ddpath.y = ip->ddpath.y * -ip->wind.yscale >> 10;
		break;
	case SETSPACE:				/* text leading << 5 */
		ip->ddspace = *((ipt_type*)ddstruct->ptr1);
		ip->ddspace.x = ip->ddspace.x * ip->wind.xscale >> 10;
		ip->ddspace.y = ip->ddspace.y * -ip->wind.yscale >> 10;
		break;
	case VTEXT:				/* vector text */
		devscale(ip->ddcp.x, ip->ddcp.y, x, y);
		vp = (porttype*)ddstruct->ptr2;	/* ptr2 = &viewport */
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		i1 = ip->vtextop | PIX_COLOR(ip->textindex);
		_core_cpwvectext( ddstruct->ptr1, ip->ddfont, &ip->ddup,
			&ip->ddpath, &ip->ddspace, x, y, &vwport, i1, 
			&ip->wind);
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
		    default: ip->pf = _core_defaultfont;
			    break;
		    }
		    if ( !ip->pf) {
			    ip->pf = _core_defaultfont;
		    }
		}
		i1 = ip->rtextop | PIX_COLOR(ip->textindex);
		devscale(ip->ddcp.x, ip->ddcp.y, i3, i4);
		vp = (porttype*)ddstruct->ptr2;	/* ptr2 = &viewport */
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		x = i3+(ddstruct->int2 * ip->pf->pf_defaultsize.x);
						/*vwport.xmin;*/
		y = i4 + (ip->pf->pf_defaultsize.y/2);
						/* vwport.ymin;*/
						/* ptr1 = &string */
		(void)pw_ttext(ip->wind.pixwin,x,y,i1,ip->pf,ddstruct->ptr1);
		break;
	case MARKER:
		i1 = ip->rtextop | PIX_COLOR(ip->textindex);
		devscale(ip->ddcp.x, ip->ddcp.y, i3, i4);
		vp = (porttype*)ddstruct->ptr2;	/* ptr2 = &viewport */
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		x = i3 - (_core_defaultfont->pf_defaultsize.x/2);
						/*-vwport.xmin;*/
		y = i4 + (_core_defaultfont->pf_defaultsize.y*3/8);
						/*- vwport.ymin;*/
		(void)pw_ttext( ip->wind.pixwin,x,y,i1,_core_defaultfont,ddstruct->ptr1);
		break;
	case POLYGN2:
		i1 =  ddstruct->int1;		/* vertex count */
		vptr1 = (vtxtype *)(ddstruct->ptr1);
		pwvptr = (struct pr_pos *)_core_prvtxlist;
		for (i=0; i<i1; i++) {
		    devscale(vptr1[i].x, vptr1[i].y, pwvptr[i].x, pwvptr[i].y);
		    }
		i2 = ip->fillop | PIX_COLOR( ip->fillindex);
                (void)pw_polygon_2( ip->wind.pixwin, 0,0, 1, &(ddstruct->int1),
                    pwvptr, i2, (struct pixrect *)0,0,0);
		break;
	case POLYGN3:
		i1 =  ddstruct->int1;		/* vertex count */
		vptr1 = (vtxtype *)(ddstruct->ptr1);
		vptr2 = (vtxtype *)(ddstruct->ptr2);
		for (i=0; i<i1; i++) {
		    vptr2[i] = vptr1[i];
		    devscale(vptr1[i].x, vptr1[i].y, vptr2[i].x, vptr2[i].y);
		    }
		i2 = ip->fillop | PIX_COLOR( ip->fillindex);
		_core_cpwpoly3( vptr2, i1, i2, &ip->wind, ip->zbuffer, ip->cutaway, ip->polyintstyle, ddstruct->int3);
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
		devscale(ip->ddcp.x, ip->ddcp.y, x, y);
		y = y-height+1;
		devscale(p1ptr->x, p1ptr->y, i1, i3);/* vwport xmin,ymin */
		devscale(p2ptr->x, p2ptr->y, i2, i4);/* vwport xmax,ymax */
		if (x<i1){ xs=i1-x; width-=xs; x=i1;}
		if (x+width-1 > i2) width=i2-x+1;
		if (width<=0) break; 
		if (y < i4) { ys = i4-y; height -= ys; y=i4;}
		if (y+height-1 > i3) height = i3-y+1;
		if (height<=0) break;
		i1 = (rptr->depth == 1) ? ip->fillrastop : ip->RAS8func;
		i1 |= PIX_COLOR( ip->fillindex);
		(void)pw_write( ip->wind.pixwin, x, y, width, height, i1, &mprect,
		    xs, ys);
		break;
	case RASGET:
		rptr = (rast_type*)ddstruct->ptr1;
		p1ptr = (ipt_type*)ddstruct->ptr2;
		p2ptr = (ipt_type*)ddstruct->ptr3;
		devscale(p1ptr->x, p1ptr->y, xs, i1);/* raster xmin,ymin */
		devscale(p2ptr->x, p2ptr->y, i2, ys);/* raster xmax,ymax */
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
		if (!rptr->bits)
			return(1);
		mprdata.md_linebytes =
			mpr_linebytes(rptr->width,rptr->depth);
		mprdata.md_image = (short *)rptr->bits;
		mprect.pr_size.x = rptr->width;
		mprect.pr_size.y = rptr->height;
		mprect.pr_depth  = rptr->depth;
		(void)pw_read( &mprect,x,y,width, height,PIX_SRC,ip->wind.pixwin,
			xs,ys);
		break;
	case LOCK:
		(void)pw_lock( ip->wind.pixwin, (Rect *)&ip->wind.rect);
		break;
	case UNLOCK:
		(void)pw_unlock( ip->wind.pixwin);
		break;
	case KILLPID:
		if (ddstruct->int1 == ip->toolpid)
			{
			ddstruct->logical = TRUE;
			ip->toolpid = -1;
			}
		break;
	default:
		break;
	}
	return( 0);
}
