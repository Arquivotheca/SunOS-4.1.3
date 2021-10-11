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
static char sccsid[] = "@(#)gp1pixwindd.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
   Device dependent SunCore graphics driver for Sun GP1 Graphics Processor
   with CG2 Frame Buffer running under SunWindows
*/

#include        <sunwindow/window_hs.h>
#include        <pixrect/gp1cmds.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	"colorshader.h"
#include	"langnames.h"
#include        "gp1pixwindd.h"
#include        "gp1_pwpr.h"
#include	<stdio.h>
#include	<string.h>
#include	<sys/ioctl.h>
#include	<sun/gpio.h>

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
DDFUNC(gp1pixwindd)(ddstruct) ddargtype *ddstruct;
{
    register short i1,i2,i3,i4;
    int i, resetcnt, nvtx;
    float fx, fy, aspect_ratio;
    rast_type *rptr;
    ipt_type *p1ptr, *p2ptr, ip1, ip2;
    int x,y,xs,ys,width,height;
    u_char *p1, *p2, *p3;
    porttype vwport, *vp;
    viewsurf *surfp;
    vtxtype *vptr1, *vptr2;
    register struct gp1pwddvars *ip;
    struct gp1_attr *_core_gp1_attr_init();
    short *_core_gp1_begin_batch();

	ip = (struct gp1pwddvars *)ddstruct->instance;
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
		    _core_get_window_vwsurf(&pwprfd, &pid, GP1_FB,
			&((viewsurf *)ddstruct->ptr1)->vsurf))
			{
			ddstruct->logical = FALSE;
			return(1);
			}

		ddstruct->logical = TRUE;
		ip = (struct gp1pwddvars *)malloc( sizeof(struct gp1pwddvars));
		ip->wind.pixwin = pw_open(pwprfd);
		if ((ip->gp1attr = _core_gp1_attr_init(ip->wind.pixwin)) == 0)
			{
			pw_close(ip->wind.pixwin);
			if (_core_gfxenvfd == pwprfd)
				{
				win_removeblanket(_core_gfxenvfd);
				close(_core_gfxenvfd);
				_core_gfxenvfd = -1;
				}
			else if (pid >= 0)
				{
				win_setowner(pwprfd, pid);
				close(pwprfd);
				kill(pid, 9);
				}
			free((char *)ip);
			ddstruct->logical = FALSE;
			return(1);
			}

		ip->toolpid = pid;
		ip->wind.ndcxmax = MAX_NDC_COORD;
		ip->wind.ndcymax = (MAX_NDC_COORD * 3) / 4;
		ip->wind.winfd = pwprfd;
		ip->wind.needlock = TRUE;
		ip->wind.rawdev = FALSE;
		ip->port = _core_vwstate.viewport;
		ip->maxz = 32767;
		ip->fillop = PIX_NOT(PIX_SRC);	/* default rasterops */
		ip->fillrastop = PIX_SRC;
		ip->cursorrop = PIX_NOT(PIX_SRC);
		ip->rtextop = PIX_SRC;
		ip->vtextop = PIX_SET;
		ip->lineop = PIX_SET;
		ip->lineindex = ip->textindex = ip->fillindex = 0;
		ip->pf = _core_defaultfont;
		ip->openfont = -1;
		ip->ddfont = -1;
		ip->openzbuffer = NOZB;

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
		ip->cmapsize = surfp->vsurf.cmapsize;
		if (ip->cmapsize == 0)  ip->cmapsize = 2;
		if (surfp->vsurf.cmapname[0] == '\0') {
			sprintf( ip->cmsname, "cmap%10D%5D", getpid(),
				ip->wind.winfd);
		} else {
			struct  colormapseg cms;
			struct  cms_map cmap;
			cmap.cm_red = ip->red;
			cmap.cm_green = ip->green;
			cmap.cm_blue = ip->blue;
			strncpy(ip->cmsname,surfp->vsurf.cmapname,DEVNAMESIZE);
			if (_core_sharedcmap( pwprfd,ip->cmsname,&cms,&cmap)) {
				ip->cmapsize = cms.cms_size;
			} else if (_core_standardcmap(ip->cmsname,&cms,&cmap)){
				ip->cmapsize = cms.cms_size;
			}
		}
		pw_setcmsname( ip->wind.pixwin, ip->cmsname);
		pw_putcolormap( ip->wind.pixwin, 0, ip->cmapsize,
			ip->red, ip->green, ip->blue);

		pw_getattributes(ip->wind.pixwin, &i);
		_core_gp1_set_pixplanes(i, ip->gp1attr);
		_core_gp1_set_op( ip->lineop, ip->gp1attr);
		_core_gp1_set_color( ip->lineindex, ip->gp1attr);
		ip->opcolorset = LINE_OPCOLOR;
		surfp->windptr = &ip->wind;
		surfp->vsurf.instance = (int) ip;
		surfp->vsurf.cmapsize = ip->cmapsize;
		strncpy(surfp->vsurf.cmapname, ip->cmsname, DEVNAMESIZE);
		win_screenget(pwprfd, &screen);
		strncpy(surfp->vsurf.screenname,screen.scr_fbname,DEVNAMESIZE);
		win_fdtoname(pwprfd, name);
		strncpy(surfp->vsurf.windowname, name, DEVNAMESIZE);
		surfp->vsurf.windowfd = pwprfd;
	        surfp->erasure = TRUE;		/* view surface capabilities */
	        surfp->txhardwr = TRUE;
		surfp->clhardwr = TRUE;
		surfp->hihardwr = TRUE;
		surfp->lshardwr = TRUE;
		surfp->lwhardwr = TRUE;
		surfp->hphardwr = TRUE;		/* it is GP1 pixrect */
		_core_winupdate(&ip->wind);
		_core_gp1_pw_updclplst( ip->wind.pixwin, ip->gp1attr);
		_core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		}
		break;
	case TERMINATE:
		if (ip->openzbuffer == SWZB)
			_core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
		_core_gp1_set_hwzbuf(TRUE, ip->gp1attr);
		if (ip->xarr) free( (char *)ip->xarr);
		if (ip->zarr) free( (char *)ip->zarr);
		ip->openzbuffer = NOZB;
		if (ip->pf != _core_defaultfont)
			pf_close( ip->pf);
		_core_gp1_attr_close( ip->wind.pixwin, ip->gp1attr);
		pw_close( ip->wind.pixwin);
		if (_core_gfxenvfd == ip->wind.winfd) {
			win_removeblanket(_core_gfxenvfd);
			close(_core_gfxenvfd);
			_core_gfxenvfd = -1;
		} else if (ip->toolpid >= 0) {
			win_setowner(ip->wind.winfd, ip->toolpid);
			close(ip->wind.winfd);
			kill(ip->toolpid, 9);
		}
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

	case INITZB:				/* init zbuffer */
		ddstruct->logical = TRUE;
		if (ip->openzbuffer != NOZB)
			return(1);
		if (gp1_d(ip->wind.pixwin->pw_pixrect)->gbufflag)
			{
			ip->openzbuffer = HWZB;
			_core_gp1_set_hiddensurfflag(GP1_ZBHIDDENSURF,
			    ip->gp1attr);
			_core_gp1_set_hwzbuf(FALSE, ip->gp1attr);
			}
		else if (!_core_init_pwzbuffer( ip->wind.winwidth,
		    ip->wind.winheight, &ip->zbuffer, &ip->cutaway))
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
			if (ip->xarr) free( (char *)ip->xarr);
			if (ip->zarr) free( (char *)ip->zarr);
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
		if (ip->openzbuffer == SWZB) {	/* ptr1 -> xarr; ptr2 -> zarr */
			if (ip->xarr) free( (char *)ip->xarr);
			if (ip->zarr) free( (char *)ip->zarr);
			ip->cutarraysize = ddstruct->int1;
			ip->xarr = (float*)malloc((unsigned int)(sizeof(float) *
				ip->cutarraysize));
			ip->zarr = (float*)malloc((unsigned int)(sizeof(float) *
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
		_core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		break;
	case CLEAR:
		pw_write(ip->wind.pixwin,0,0,ip->wind.winwidth,
			ip->wind.winheight, PIX_CLR,
			(struct pixrect *)0,0,0);
					/* Perhaps could speed this up with a
					   special microcode command */
		if (ip->openzbuffer == HWZB)
			_core_gp1_clear_zbuffer(ip->wind.pixwin, ip->gp1attr);
		else if (ip->openzbuffer == SWZB) {
			if (ip->zbuffer->pr_width != ip->wind.winwidth ||
			    ip->zbuffer->pr_height != ip->wind.winheight) {
			    _core_terminate_pwzbuffer(ip->zbuffer, ip->cutaway);
			    ip->openzbuffer = NOZB;
			    if (!_core_init_pwzbuffer( ip->wind.winwidth,
				ip->wind.winheight,&ip->zbuffer,&ip->cutaway)) {
				ip->openzbuffer = SWZB;
				if (ip->xarr && ip->zarr)
			        	_core_set_pwzbufcut( ip->cutaway,
					  ip->xarr, ip->zarr, ip->cutarraysize);
			    }
			} else {
			    _core_clear_pwzbuffer( ip->zbuffer, 
				(struct pixrect *)0);
			}
			if (ip->openzbuffer == NOZB) return( 1);
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
		pw_putcolormap(ip->wind.pixwin, 0, ip->cmapsize,
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
	case SETTCOL:
		ip->rtextop =
		    _core_gp1_ddsetcolor( ddstruct->int2, ddstruct->int3 ||
			!ddstruct->int1, ddstruct->int3);
		ip->textindex = ddstruct->int1;
		ip->vtextop = ip->rtextop;
		break;
	case SETFCOL:
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
		ip->linewidth = (short)ddstruct->int1 * ip->wind.xscale>>15;
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
		pw_vector( ip->wind.pixwin,i1,i2,i3,i4,PIX_SRC^PIX_DST,255);
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
				break;		/* if not visible */
		    devscale(ip1.x, ip1.y, i1, i2);
		    devscale(ip2.x, ip2.y, i3, i4);
		    i = ip->lineop | PIX_COLOR( ip->lineindex);
		    _core_cpwsimline(i1,i2,i3,i4, i, &ip->wind,
			ip->linewidth, ip->linestyle);
		} else {
		    register short *shmptr;

		    if (ip->opcolorset != LINE_OPCOLOR)
			{
			_core_gp1_set_op( ip->lineop, ip->gp1attr);
			_core_gp1_set_color( ip->lineindex, ip->gp1attr);
			ip->opcolorset = LINE_OPCOLOR;
			}
		    if (_core_gp1_pw_lock( ip->wind.pixwin, 
			(Rect *)&ip->wind.rect, ip->gp1attr)) {
			    _core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
			}
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
		    pw_unlock( ip->wind.pixwin);
		}
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
		_core_cpwvectext( ddstruct->ptr1, ip->ddfont,&ip->ddup,
			&ip->ddpath,&ip->ddspace, x, y,&vwport, i1, &ip->wind);
		break;
	case TEXT:				/* raster text */
		_core_gp1_ddtext(NOTRAWDEV, (caddr_t)ip,
		    ip->ddfont, &ip->openfont,
		    &ip->pf, ip->rtextop, ip->textindex, &ip->ddcp, &ip->wind,
		    (porttype *)ddstruct->ptr2, ddstruct->int2, ddstruct->ptr1);
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
		pw_ttext(ip->wind.pixwin, x, y, i1, _core_defaultfont,
		    ddstruct->ptr1);
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
		if (ip->openzbuffer != SWZB) { /* hardware buffer */
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
		else { /* software buffer */
			nvtx = ddstruct->int1;
			vptr1 = (vtxtype *)(ddstruct->ptr1);
			vptr2 = (vtxtype *)(ddstruct->ptr2);
	     		/* image transform the polygon */
	     		if(! _core_openseg->idenflag) {
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
						    (short *)vptr1, 
						    (short *)&ip1, 8);
		        			_core_imxfrm3(_core_openseg,
						    &ip1, (ipt_type *)vptr1);
						/* imxform nrmals;
						   should convert to unit vec */
		        			_core_moveword(
						    (short *)&vptr1->dx, 
						    (short *)&ip1, 8);
		        			_core_imszpt3(_core_openseg,
						    &ip1, 
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
			_core_cpwpoly3(vptr2, nvtx, i2, &ip->wind, ip->zbuffer,
			    ip->cutaway, ip->polyintstyle, ddstruct->int3);
			}
		break;
	case RASPUT:
		_core_gp1_ddrasterput(NOTRAWDEV, (caddr_t)ip, &ip->wind, 
		    &ip->ddcp, ip->fillrastop, ip->RAS8func, ip->fillindex, 
		    (rast_type *)ddstruct->ptr1, (ipt_type *)ddstruct->ptr2, 
		    (ipt_type *)ddstruct->ptr3);
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
		pw_read( &mprect,x,y,width, height,PIX_SRC,ip->wind.pixwin,
			xs,ys);
		break;
	case LOCK:
		if (_core_gp1_pw_lock( ip->wind.pixwin, 
		   (Rect *)&ip->wind.rect, ip->gp1attr)) {
			_core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		    }
		break;
	case UNLOCK:
		pw_unlock( ip->wind.pixwin);
		break;
	case KILLPID:
		if (ddstruct->int1 == ip->toolpid)
			{
			ddstruct->logical = TRUE;
			ip->toolpid = -1;
			}
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
		_core_set_gp_imxform(ddstruct->int1, ddstruct->ptr1, 
		    &ip->wind, &ip->port, ip->gp1attr);
		break;
	case WLDVECSTOSCREEN:
		p1 = (ddstruct->logical == TWOD) ? 0 : (u_char *)ddstruct->ptr3;
		if (ip->opcolorset != LINE_OPCOLOR) {
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
			(float *)ddstruct->ptr2, (float *)p1, (vtxtype *)p2);
		break;
	case WINUPDATE:
		_core_gp1_pw_updclplst( ip->wind.pixwin, ip->gp1attr);
		_core_gp1_winupdate(&ip->wind, &ip->port, ip->gp1attr);
		break;
	case WINDOWCLIP:
		_core_gp1_set_wldclipplanes(ddstruct->int1, ip->gp1attr);
		break;
	case OUTPUTCLIP:
		i = ddstruct->int1 ? 0x3C : 0;
		_core_gp1_set_outclipplanes(i, ip->gp1attr);
		break;
	case SEGDRAW:
		_core_gp1_segdraw(NOTRAWDEV, (caddr_t)ip, &ip->wind, 
		    ip->gp1attr, &ip->ddfont, &ip->openfont, &ip->pf, ddstruct);
		break;
	default:
		break;
	}
	return( 0);
}
