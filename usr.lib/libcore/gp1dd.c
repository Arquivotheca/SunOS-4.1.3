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
static char sccsid[] = "@(#)gp1dd.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
   Device dependent core graphics driver for SUN GP1 Graphics Processor
   connected to a CG2 frame buffer and running as a raw device
*/

#include        <sunwindow/window_hs.h>
#include 	<pixrect/gp1cmds.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	"colorshader.h"
#include        "langnames.h"
#include	"gp1dd.h"
#include	"gp1_pwpr.h"
#include	<stdio.h>
#include	<sys/ioctl.h>
#include	<sys/file.h>
#include	<sun/gpio.h>
#include	<sun/fbio.h>
#include	<string.h>
#define NULL 0

extern struct pixrectops mem_ops;
extern PIXFONT *_core_defaultfont;

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
DDFUNC(gp1dd)(ddstruct) register ddargtype *ddstruct;
{
    register short i1,i2,i3,i4;
	int i, error = 0, nvtx, resetcnt;
	float fx, fy, aspect_ratio;
	rast_type *rptr;
	ipt_type *p1ptr, *p2ptr, ip1, ip2;
        int x,y,xs,ys,width,height;
	u_char *p1, *p2, *p3;
	porttype vwport, *vp;
	struct pixrect *subpr;		/* viewport pixrect for clip */
	register struct gp1ddvars *ip;	/* pointer to instance state vars */
	vtxtype *vptr1, *vptr2;
	struct pr_prpos prprpos;
	struct gp1_attr *_core_gp1_attr_init();
	short *_core_gp1_begin_batch();

	ip = (struct gp1ddvars *)ddstruct->instance;
	switch(ddstruct->opcode) {
	case INITZB:				/* init zbuffer */
                ddstruct->logical = TRUE;
                if (ip->openzbuffer != NOZB)
                        return(1);
                if (gp1_d(ip->screen)->gbufflag)
                        {
                        ip->openzbuffer = HWZB;
                        _core_gp1_set_hiddensurfflag(GP1_ZBHIDDENSURF,
                            ip->gp1attr);
                        _core_gp1_set_hwzbuf(TRUE, ip->gp1attr);
                        }
                else if (!_core_init_pwzbuffer( ip->screen->pr_width,
                    ip->screen->pr_height, &ip->zbuffer, &ip->cutaway))
                        {
                        ip->openzbuffer = SWZB;
                        ip->xarr = 0;
                        ip->zarr = 0;
                        }
                else
                        ddstruct->logical = FALSE;
                break;
	case TERMZB:
                if (ip->openzbuffer == SWZB) {
                        _core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
                        if (ip->xarr) free((char *)ip->xarr);
                        if (ip->zarr) free((char *)ip->zarr);
                        ip->openzbuffer = NOZB;
                } else if (ip->openzbuffer == HWZB)
                        {
                        _core_gp1_set_hiddensurfflag(GP1_NOHIDDENSURF,
                            ip->gp1attr);
                        ip->openzbuffer = NOZB;
                        }
                else
                        return( 1);
                break;
	case SETZBCUT:
                if (ip->openzbuffer == SWZB) {  /* ptr1 -> xarr; ptr2 -> zarr */                        if (ip->xarr) free((char *)ip->xarr);
                        if (ip->zarr) free((char *)ip->zarr);
                        ip->cutarraysize = ddstruct->int1;
                        ip->xarr = (float*)malloc((unsigned int)(sizeof(float) *
                                ip->cutarraysize));
                        ip->zarr = (float*)malloc((unsigned int)(sizeof(float) *
                                ip->cutarraysize));
                        for (i1=0; i1<ip->cutarraysize; i1++) {
                                *(ip->xarr + i1) = *((float*)ddstruct->ptr1+i1);                                *(ip->zarr + i1) = *((float*)ddstruct->ptr2+i1);                        }
                        _core_set_pwzbufcut( ip->cutaway, ip->xarr, ip->zarr,
                                ip->cutarraysize);
                } else return( 1);
                break;
	case INITIAL: {
		static char gponedev[] = "/dev/gpone0a";
		struct screen screen;
		int fbfd, pwprfd;
		struct  cms_map cmap;
    		viewsurf *surfp;

		surfp = (viewsurf*)ddstruct->ptr1;
		win_initscreenfromargv(&screen, (char **)0);
		if (*surfp->vsurf.screenname != '\0') {
			if ((fbfd = _core_chkdev(surfp->vsurf.screenname,
			    FBTYPE_SUN2GP, ddstruct->int1)) >= 0)
				strncpy(screen.scr_fbname,
				    surfp->vsurf.screenname, SCR_NAMESIZE);
			else {
		    		ddstruct->logical = FALSE;
		    		return( 1);
		    	}
		}
		else if ((fbfd = _core_chkdev("/dev/fb", FBTYPE_SUN2GP,
		    ddstruct->int1)) >= 0)
			strncpy(screen.scr_fbname, "/dev/fb", SCR_NAMESIZE);
		else if ((fbfd = _core_getfb(gponedev,FBTYPE_SUN2GP,
		    ddstruct->int1)) >= 0)
			strncpy(screen.scr_fbname, gponedev, SCR_NAMESIZE);
		else {
		    ddstruct->logical = FALSE;
		    return( 1);
		    }
		if (!ddstruct->int1)
			strncpy(screen.scr_kbdname, "NONE", SCR_NAMESIZE);
		if ((pwprfd = win_screennew(&screen)) < 0) {
		    ddstruct->logical = FALSE;
		    return( 1);
		    }

		win_screenget(pwprfd, &screen);
		ddstruct->logical = TRUE;
		ip = (struct gp1ddvars *) malloc(sizeof(struct gp1ddvars));
		ip->wind.pixwin = pw_open(pwprfd);
		ip->wind.winfd = pwprfd;
                if ((ip->gp1attr = _core_gp1_attr_init(ip->wind.pixwin)) == 0)
                        {
                        pw_close(ip->wind.pixwin);
			win_screendestroy( ip->wind.winfd);
			close(ip->wind.winfd);
                        free((char *)ip);             
                        ddstruct->logical = FALSE;
                        return(1);            
                        }

		ip->wind.needlock = FALSE;
		ip->wind.rawdev = TRUE;
		ip->port = _core_vwstate.viewport;
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
		ip->ddfont = -1;
		ip->openzbuffer = NOZB;
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

		strncpy(ip->cmsname,surfp->vsurf.cmapname,DEVNAMESIZE);
		surfp->vsurf.cmapsize = 256;
		cmap.cm_red = ip->red;
		cmap.cm_green = ip->green;
		cmap.cm_blue = ip->blue;
		_core_standardcmap(ip->cmsname, (struct colormapseg *)0, 
			(struct cms_map *)&cmap);
		pr_putattributes( ip->screen, &planes);
		pw_setcmsname( ip->wind.pixwin, ip->cmsname);
		strncpy(surfp->vsurf.cmapname, ip->cmsname, DEVNAMESIZE);
		pw_putcolormap(ip->wind.pixwin, 0,256,
			ip->red,ip->green,ip->blue);
                _core_gp1_set_pixplanes(255, ip->gp1attr);
                _core_gp1_set_op( ip->lineop, ip->gp1attr);
                _core_gp1_set_color( ip->lineindex, ip->gp1attr);
		ip->opcolorset = LINE_OPCOLOR;

		surfp->windptr = &ip->wind;
		surfp->vsurf.instance = (int) ip;
		strncpy(surfp->vsurf.screenname, screen.scr_fbname,DEVNAMESIZE);
		strncpy(surfp->vsurf.windowname, screen.scr_rootname,
		    DEVNAMESIZE);
		surfp->vsurf.windowfd = pwprfd;
		surfp->windptr = &ip->wind;
	        surfp->erasure = TRUE;		/* view surface capabilities */
	        surfp->txhardwr = TRUE;
		surfp->clhardwr = TRUE;
	        surfp->hihardwr = TRUE;
		surfp->lshardwr = TRUE;
		surfp->lwhardwr = TRUE;
		surfp->hphardwr = TRUE;         /* it is GP1 pixrect */
		_core_winupdate(&ip->wind);
		_core_gp1_pw_updclplst( ip->wind.pixwin, ip->gp1attr);
                _core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		if (surfp->vsurf.ptr != NULL)
			_core_setadjacent(*surfp->vsurf.ptr, &screen);
		}
		break;
	case TERMINATE:
		if (ip->openzbuffer == SWZB)
                        _core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
		_core_gp1_set_hwzbuf(FALSE, ip->gp1attr);
						 
                if (ip->xarr) free((char *)ip->xarr);
                if (ip->zarr) free((char *)ip->zarr);
                ip->openzbuffer = NOZB;
		if (ip->pf != _core_defaultfont)
			pf_close( ip->pf);
		_core_gp1_attr_close( ip->wind.pixwin, ip->gp1attr);
		pw_close( ip->wind.pixwin);
		win_screendestroy( ip->wind.winfd);
		close(ip->wind.winfd);
		free((char *)ip);
		break;
	case CHKGPRESETCOUNT:
                ioctl ( gp1_d((ip->wind.pixwin)->pw_pixrect)->ioctl_fd,
                        GP1IO_GET_RESTART_COUNT, &resetcnt);
                if (resetcnt != ip->gp1attr->resetcnt) {
                        ip->gp1attr->resetcnt = resetcnt;
                        ip->gp1attr->needreset = TRUE;
                        return(1);
                }
                else
                        return(0);
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
/*		printf("setndc fullxy %d %d offxy %d %d scale %d \n",
		ip->fullx, ip->fully, ip->xoff, ip->yoff, ip->scale); */
		_core_winupdate(&ip->wind);
		_core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		break;
	case CLEAR:
		if (ip->wind.needlock)
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		pr_rop(ip->screen,0,0,ip->screen->pr_width,
			ip->screen->pr_height, PIX_CLR,
			(struct pixrect *)0,0,0);	/*zero screen*/
		if (ip->wind.needlock)
			pw_unlock(ip->wind.pixwin);
		if (ip->openzbuffer == HWZB)
                        _core_gp1_clear_zbuffer(ip->wind.pixwin, ip->gp1attr);
		else if (ip->openzbuffer == SWZB)
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
		pw_putcolormap(ip->wind.pixwin, 0, 256,
			ip->red,ip->green,ip->blue);
		break;
	case SETLCOL:
		ip->lineop =
                     _core_gp1_ddsetcolor( ddstruct->int2, ddstruct->int3 ||
                        !ddstruct->int1, ddstruct->int3 || !ddstruct->int1);
                ip->lineindex = ddstruct->int1;
                if (ip->opcolorset == LINE_OPCOLOR)
                        {
                        _core_gp1_set_op( ip->lineop, ip->gp1attr);
                        _core_gp1_set_color( ip->lineindex, ip->gp1attr);
                        }
		break;
	case SETTCOL: 			/* select colors for text */
		ip->rtextop =
                    _core_gp1_ddsetcolor( ddstruct->int2, ddstruct->int3 ||
                        !ddstruct->int1, ddstruct->int3);
                ip->textindex = ddstruct->int1;
                ip->vtextop = ip->rtextop;
		break;
	case SETFCOL:			/* select colors for polygon fill */
		ip->fillop =
                    _core_gp1_ddsetcolor( ddstruct->int2, ddstruct->int3,
                        ddstruct->int3);
                ip->fillindex = ddstruct->int1;
                ip->fillrastop = ip->RAS8func = ip->fillop;
                if (ip->opcolorset == REGION_OPCOLOR)
                        {
                        _core_gp1_set_op( ip->fillop, ip->gp1attr);
                        _core_gp1_set_color( ip->fillindex, ip->gp1attr);
                        }
		break;
	case ECHOMOVE:
	case MOVE:
		ip->ddcp.x = ddstruct->int1;
                ip->ddcp.y = ddstruct->int2;
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
		if (ip->linewidth > 1)
			_core_fastwidth = FALSE;
		break;
	case SETSTYL:
		ip->linestyle = ddstruct->int1; break;
	case SETPISTYL:
		ip->polyintstyle = ddstruct->int1; break;
	case ECHOLINE:
		devscale(ip->ddcp.x, ip->ddcp.y, i1, i2);
                ip->ddcp.x = ddstruct->int1;
                ip->ddcp.y = ddstruct->int2;
                devscale(ddstruct->int1, ddstruct->int2, i3, i4);
		if (ip->wind.needlock)
                	pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
                pr_vector(ip->screen,i1,i2,i3,i4,PIX_SRC^PIX_DST,255);
                if (ip->wind.needlock)
                	pw_unlock(ip->wind.pixwin);
                break;
	case LINE:
		if ( ip->linestyle || (ip->linewidth > 1)) {
                    ip1 = ip->ddcp;
                    ip->ddcp.x = ddstruct->int1;
                    ip->ddcp.y = ddstruct->int2;
                    ip->ddcp.z = ddstruct->int3;
                    if( !_core_openseg->idenflag) {
                        ip2 = ip1;
                        _core_imxfrm3(_core_openseg,&ip2,&ip1);
                        _core_imxfrm3(_core_openseg,&ip->ddcp,&ip2);
                    } else
                        ip2 = ip->ddcp;
                    if(_core_outpclip &&
                        !_core_oclpvec2(&ip1,&ip2,&ip->port))
                                break;          /* if not visible */
                    devscale(ip1.x, ip1.y, i1, i2);
                    devscale(ip2.x, ip2.y, i3, i4);
                    i = ip->lineop | PIX_COLOR( ip->lineindex);
                    _core_csimline(i1,i2,i3,i4, i, &ip->wind,
                        ip->linewidth, ip->linestyle);
                } else {
		    register short *shmptr;
          
                    if (ip->opcolorset != LINE_OPCOLOR)
                        {
                        _core_gp1_set_op( ip->lineop, ip->gp1attr);
                        _core_gp1_set_color( ip->lineindex, ip->gp1attr);
                        ip->opcolorset = LINE_OPCOLOR;
                        }
		    if (ip->wind.needlock)
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
                    shmptr = _core_gp1_begin_batch(&ip->wind,
                                 ip->gp1attr, &ip->port);
                    *shmptr++ = GP1_USEFRAME | ip->gp1attr->statblkindx;
                    *shmptr++ = GP1_CORENDCVEC_3D;
                    *shmptr++ = 1;
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.x);
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.y);
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.z);
		    ip->ddcp.x = ddstruct->int1;
		    ip->ddcp.y = ddstruct->int2;
		    ip->ddcp.z = ddstruct->int3;
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.x);
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.y);
		    CORE_GP_PUT_INT(shmptr, &ip->ddcp.z);
                    ip->gp1attr->shmptr = shmptr;
                    _core_gp1_end_batch(ip->wind.pixwin, ip->gp1attr);
		    if (ip->wind.needlock)
			pw_unlock(ip->wind.pixwin);
                }
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
		/* fprintf(stderr, "upxy %D %D\n", ddup.x, ddup.y); */
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
		devscale(ip->ddcp.x, ip->ddcp.y, x, y);
		vp = (porttype*)ddstruct->ptr2;	/* ptr2 = &viewport */
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		i = ip->vtextop | PIX_COLOR(ip->textindex);	
		_core_cprvectext( ddstruct->ptr1, ip->ddfont,
			&ip->ddup,&ip->ddpath,&ip->ddspace, x,
			y,&vwport,i, &ip->wind);
		break;
	case TEXT:				/* raster text */
		_core_gp1_ddtext(RAWDEV, (caddr_t)ip, ip->ddfont, 
		    &ip->openfont, &ip->pf, ip->rtextop, ip->textindex, 
		    &ip->ddcp, &ip->wind, (porttype *)ddstruct->ptr2, 
		    ddstruct->int2, ddstruct->ptr1);
		break;
	case MARKER:
		i = ip->rtextop | PIX_COLOR(ip->textindex);	
		devscale(ip->ddcp.x, ip->ddcp.y, x, y);
		vp = (porttype*)ddstruct->ptr2;
		devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
		devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
		x = x - (_core_defaultfont->pf_defaultsize.x/2) -
			vwport.xmin;
		y = y - vwport.ymin +(_core_defaultfont->pf_defaultsize.y*3/8);
		subpr = pr_region(ip->screen,vwport.xmin, vwport.ymin,
		    vwport.xmax-vwport.xmin+1, vwport.ymax-vwport.ymin+1);
		if (ip->wind.needlock)
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		prprpos.pr = subpr;
		prprpos.pos.x = x;
		prprpos.pos.y = y;
		(void)pf_ttext( prprpos, i ,_core_defaultfont,ddstruct->ptr1);
		if (ip->wind.needlock)
			pw_unlock(ip->wind.pixwin);
		pr_destroy( subpr);
		break;
	case POLYGN2:
		if (ip->opcolorset != REGION_OPCOLOR)
                        {
                        _core_gp1_set_op( ip->fillop, ip->gp1attr);
                        _core_gp1_set_color( ip->fillindex, ip->gp1attr);
                        ip->opcolorset = REGION_OPCOLOR;
                        }
                if ((i = ip->gp1attr->hiddensurfflag) != GP1_NOHIDDENSURF)
                        _core_gp1_set_hiddensurfflag(GP1_NOHIDDENSURF,
                            ip->gp1attr);
                                                /* npts: ddstruct->int1
                                                   vtxptr: ddstruct->ptr1 */
                _core_gp1_polygon_2(&ip->wind, &ip->port, ip->gp1attr,
                    ddstruct->int1, (vtxtype *) ddstruct->ptr1);
                if (i != GP1_NOHIDDENSURF)
                        _core_gp1_set_hiddensurfflag(i, ip->gp1attr);
		break;
	case POLYGN3:
		if (ip->polyintstyle == PLAIN)
			i1 = WARNOCK;
		else
			i1 = _core_shading_parameters.shadestyle;
		if (ip->openzbuffer != SWZB)
			{
			if (ip->opcolorset != REGION_OPCOLOR)
				{
				_core_gp1_set_op( ip->fillop, ip->gp1attr);
				_core_gp1_set_color( ip->fillindex, ip->gp1attr);
				ip->opcolorset = REGION_OPCOLOR;
				}
			i2 = (ddstruct->int3) ?
			    GP1_ZBHIDDENSURF : GP1_NOHIDDENSURF;
			if (i2 != ip->gp1attr->hiddensurfflag)
				_core_gp1_set_hiddensurfflag(i2, ip->gp1attr);
			_core_gp1_polygon_3(&ip->wind, &ip->port, ip->gp1attr,
			    _core_openseg, ddstruct->int1, 
			    (vtxtype *)ddstruct->ptr1, i1);
			}
		else
			{
			nvtx = ddstruct->int1;
			vptr1 = (vtxtype *)(ddstruct->ptr1);
			vptr2 = (vtxtype *)(ddstruct->ptr2);
	     		/* image transform the polygon */
	     		if(! _core_openseg->idenflag)
				{
	         		if(_core_openseg->type == XLATE2)
					 {	/* translate only */
		    			for (i=0; i<nvtx; i++)
						{
		        			vptr1->x +=
						   _core_openseg->imxform[3][0];
		        			vptr1->y +=
						   _core_openseg->imxform[3][1];
		        			vptr1->z +=
						   _core_openseg->imxform[3][2];
						vptr1++;
		        			}
		    			}
	         		else if(_core_openseg->type == XFORM2)
					{	/* full transform */
		    			for (i=0; i<nvtx; i++)
						{
		        			_core_moveword(
						    (short *)vptr1, (short *)&ip1, 8);
		        			_core_imxfrm3(_core_openseg,
						    &ip1,(ipt_type *)&vptr1->x);
						/* imxform nrmals;
						   should convert to unit vec */
		        			_core_moveword(
						    (short *)&vptr1->dx, 
						    (short *)&ip1, 8);
		        			_core_imszpt3(_core_openseg,
						    (ipt_type *)&ip1, 
						    (ipt_type *)&vptr1->dx);
						vptr1++;
		        			}
		    			}
	         		}
			vptr1 = (vtxtype *)(ddstruct->ptr1);
	     		if (_core_outpclip)
				{
		 		_core_vtxcount = nvtx +
				    (vptr1 - _core_vtxlist) / sizeof(vtxtype);
		 		for (i=0; i < nvtx; i++)
					{	/* clip to viewport  */
		     			_core_oclpvtx2(vptr1, &ip->port);
					vptr1++;
		     			}
		 		_core_oclpend2();
				vptr1 = (vtxtype *) (ddstruct->ptr1) + nvtx;
				nvtx = (&_core_vtxlist[_core_vtxcount] - vptr1)/
				    sizeof(vtxtype);
		 		}
			for (i=0; i<nvtx; i++)
				{
		    		*vptr2 = *vptr1;
		    		devscale(vptr1->x, vptr1->y,
				    vptr2->x, vptr2->y);
				vptr1++; vptr2++;
		    		}
			i2 = ip->fillop | PIX_COLOR( ip->fillindex);
			vptr2 = (vtxtype *)(ddstruct->ptr2);
			_core_cregion3(vptr2, nvtx, i2, &ip->wind, ip->zbuffer,
			    ip->cutaway, ip->polyintstyle, ddstruct->int3);
			}
		break;
	case RASPUT:
		_core_gp1_ddrasterput(RAWDEV, (caddr_t)ip, &ip->wind, &ip->ddcp,
                    ip->fillrastop, ip->RAS8func, ip->fillindex, 
		    (rast_type *)ddstruct->ptr1, (ipt_type *)ddstruct->ptr2, 
		    (ipt_type *)ddstruct->ptr3);
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
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
		pr_rop( &mprect,x,y,width,height,PIX_SRC, ip->screen,
			xs,ys);
		if (ip->wind.needlock)
			pw_unlock(ip->wind.pixwin);
		break;
	case LOCK:
		if (++ip->lockcount > 1)
			break;
		if (ip->curstrk)
			{
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
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
			pw_unlock(ip->wind.pixwin);
			ip->wind.needlock = TRUE;
			}
		break;
	case CURSTRKON:
		if (ip->curstrk)
			break;
		ip->curstrk = TRUE;
		if (ip->lockcount)
			{
			pw_lock(ip->wind.pixwin, (Rect *)&ip->wind.rect);
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
			pw_unlock(ip->wind.pixwin);
			}
		else
			ip->wind.needlock = FALSE;
		break;
	case SETVWXFORM1:		/* set view transform in matrix 0 */
		_core_gp1_set_matrix_3d( ip->wind.pixwin, 0, 
		    (float *)ddstruct->ptr1, ip->gp1attr);
		break;
	case SETVWXFORM32K:		/* set view transform in matrix 1 */
		_core_gp1_set_matrix_3d( ip->wind.pixwin, 1, 
		    (float *)ddstruct->ptr1, ip->gp1attr);
		break;
	case VIEWXFORM2:		/* view transform 2D point */
		_core_gp1_xformpt_2d( ip->wind.pixwin, 1, 
		    (float *)ddstruct->ptr1, (float *)ddstruct->ptr2, 
		    ip->gp1attr);
		break;
	case VIEWXFORM3:		/* view transform 3D point */
		_core_gp1_xformpt_3d( ip->wind.pixwin, 1,
		    (float *)ddstruct->ptr1, (float *)ddstruct->ptr2, 
		    ip->gp1attr);
		break;
	case MATMULT:			/* 4x4 matrix multiply */
		/* output matrix m3 = m1 * m2  */
		_core_gp1_matmul_3d( ip->wind.pixwin, (float *)ddstruct->ptr1,
		    (float *)ddstruct->ptr2, (float *)ddstruct->ptr3, 
		    ip->gp1attr);
		break;
	case SETVIEWPORT:
		ip->port = *((porttype *)ddstruct->ptr1);
		_core_gp1_winupdate( &ip->wind, &ip->port, ip->gp1attr);
		break;
	case SETIMGXFORM:		/* set image transform */
		_core_set_gp_imxform(ddstruct->int1, ddstruct->ptr1, &ip->wind,
		    &ip->port, ip->gp1attr);
		break;
        case WLDVECSTOSCREEN:
                p1 = (ddstruct->logical == TWOD) ? 0 : (u_char *)ddstruct->ptr3;                if (ip->opcolorset != LINE_OPCOLOR) {
                        _core_gp1_set_op( ip->lineop, ip->gp1attr);
                        _core_gp1_set_color( ip->lineindex, ip->gp1attr);
                        ip->opcolorset = LINE_OPCOLOR;
                }
                _core_gp1_wld_vecs_to_screen(&ip->wind, &ip->port, ip->gp1attr,
                    ddstruct->int1, (pt_type *)ddstruct->int2, 
		    (float *)ddstruct->ptr1, (float *)ddstruct->ptr2, 
		    (float *)p1);
                break;
	case WLDVECSTONDC:
		p1 = (ddstruct->logical == TWOD) ? 0 : (u_char *)ddstruct->ptr3;
		p2 = *((u_char **) &ddstruct->float1);
		p3 = *((u_char **) &ddstruct->float2);
		_core_gp1_wld_vecs_to_ndc(&ip->wind, &ip->port, ip->gp1attr,
		    ddstruct->int1, (pt_type *)ddstruct->int2, 
		    (float *)ddstruct->ptr1, (float *)ddstruct->ptr2, 
		    (float *)p1, (short *)ddstruct->int3, (ipt_type *)p2, 
		    (ipt_type *)p3);
		break;
       case CHKHWZBUF:
                if (ip->openzbuffer == HWZB)
			ddstruct->logical = TRUE;
		else
			ddstruct->logical = FALSE;
		break;
       case INQFORCEDREPAINT:
                if ( (ip->openzbuffer == HWZB) && (ip->gp1attr->forcerepaint)) {
			ip->gp1attr->forcerepaint = 0;
			ddstruct->logical = TRUE;
		} else
			ddstruct->logical = FALSE;
		break;
       case WLDPLYGNTOSCREEN:
                p1 = (ddstruct->logical==TWOD) ? 0 : (u_char *)ddstruct->ptr3;
                if (ip->opcolorset != REGION_OPCOLOR) {
                        _core_gp1_set_op( ip->fillop, ip->gp1attr);
                        _core_gp1_set_color( ip->fillindex, ip->gp1attr);
                        ip->opcolorset = REGION_OPCOLOR;
                }
                _core_gp1_wld_polygon_to_screen(&ip->wind, &ip->port,
                        ip->gp1attr, ddstruct->int1, (pt_type *)ddstruct->int2,
                         (float *)ddstruct->ptr1, (float *)ddstruct->ptr2, 
			(float *)p1);
                break;
        case WLDPLYGNTONDC:
                p1 = (ddstruct->logical==TWOD) ? 0 : (u_char *)ddstruct->ptr3;
                p2 = *((u_char **) &ddstruct->float1);
 
                _core_gp1_wld_polygon_to_ndc(&ip->wind, &ip->port,
                        ip->gp1attr, ddstruct->int1, (pt_type *)ddstruct->int2,
                        &ddstruct->int3, (float *)ddstruct->ptr1, 
			(float *)ddstruct->ptr2,
                        (float *)p1, (vtxtype *)p2);
                break;
	case WINUPDATE:
		/* clipping list can't change except when need reset
		   org_x and org_y can never change
		   window size never changes
		*/
		break;
	case WINDOWCLIP:
		_core_gp1_set_wldclipplanes(ddstruct->int1, ip->gp1attr);
		break;
	case OUTPUTCLIP:
		i = ddstruct->int1 ? 0x3C : 0;
		_core_gp1_set_outclipplanes(i, ip->gp1attr);
		break;
	case SEGDRAW:
		_core_gp1_segdraw(RAWDEV, (caddr_t)ip, &ip->wind, ip->gp1attr,
		    &ip->ddfont, &ip->openfont, &ip->pf, ddstruct);
		break;
	default:
		break;
	}
	return(error);
}
