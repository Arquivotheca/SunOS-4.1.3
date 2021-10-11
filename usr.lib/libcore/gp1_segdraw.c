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
static char sccsid[] = "@(#)gp1_segdraw.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
   segdraw for GP surfaces (gp1dd and gp1pixwindd)
*/

#include        <sunwindow/window_hs.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	"colorshader.h"
#include	"langnames.h"
#include        "gp1dd.h"
#include        "gp1pixwindd.h"
#include        "gp1_pwpr.h"
#include        <pixrect/gp1cmds.h>
#include	<stdio.h>
#include	<sys/ioctl.h>
#include	<sun/gpio.h>

#define devscale(x_ndc_coord, y_ndc_coord, x_dev_coord, y_dev_coord)	\
    if (wind -> rawdev == TRUE) { \
	if (ip_raw -> screen -> pr_height != ip_raw -> screen -> pr_width) { \
	    if (ip_raw -> fullx) { \
		x_dev_coord = (x_ndc_coord + (x_ndc_coord >> 3)) >> 5; \
		y_dev_coord = ip_raw -> maxy - (((y_ndc_coord + \
			    (y_ndc_coord >> 3)) >> 5) + ip_raw -> yoff); \
	} else if (ip_raw -> fully) { \
		    x_dev_coord = ((x_ndc_coord - (x_ndc_coord >> 3)) >> 5) \
		    +ip_raw -> xoff; \
		    y_dev_coord = ip_raw -> maxy - ((y_ndc_coord - \
			    (y_ndc_coord >> 3)) >> 5); \
	    } else { \
		    x_dev_coord = (x_ndc_coord << 10) / ip_raw -> scale + ip_raw -> xoff; \
		    y_dev_coord = ip_raw -> maxy - ((y_ndc_coord << 10) / ip_raw -> scale); \
	    } \
    } else { \
	    if (ip_raw -> fullx) { \
		x_dev_coord = x_ndc_coord >> 5; \
		y_dev_coord = ip_raw -> maxy - ((y_ndc_coord >> 5) + ip_raw -> yoff); \
	} else if (ip_raw -> fully) { \
		    x_dev_coord = (x_ndc_coord >> 5) + ip_raw -> xoff; \
		    y_dev_coord = ip_raw -> maxy - (y_ndc_coord >> 5); \
	    } else { \
		    x_dev_coord = (x_ndc_coord << 10) / ip_raw -> scale + ip_raw -> xoff; \
		    y_dev_coord = ip_raw -> maxy - ((y_ndc_coord << 10) / ip_raw -> scale); \
	    } \
    } \
} else { \
	x_dev_coord = ( x_ndc_coord * wind -> xscale >> 15) \
	+wind -> xoff; \
	y_dev_coord = wind -> yoff - \
	( y_ndc_coord * wind -> yscale >> 15); \
}

extern PIXFONT *_core_defaultfont;

_core_gp1_segdraw (rawdevflag, ip, wind, attr, passed_ddfont, openfont, pf, ddstruct)
int     rawdevflag;
caddr_t ip;
struct windowstruct *wind;
int *passed_ddfont, *openfont;
PIXFONT **pf;
struct gp1_attr *attr;
ddargtype *ddstruct;

{				/* paint the segment on the viewsurface */
    struct gp1ddvars   *ip_raw;
    struct gp1pwddvars *ip_win;
    register int    i1, i2, i3, i4;
    int     x, y;
    porttype vwport, *vp;
    vtxtype * vptr1, *vptr2;
    short   sdstr[64];
    viewsurf * sptr;		/* ptr to current viewsurf */
    int     offset;		/* byte offset in PDF */
    short   sdopcode, n, i;
    char   *sdstring;
    ipt_type p0, ip1, ip2;	/* NDC point variables */
    rast_type raster;		/* bitmap structure */
    short  *pdfptr;		/* pointer into PDF */
    int     size, mk, vcount;
    primattr pdfcurrent;	/* BUG: replace this with ip->stuff */
    float   f, *fptr1, *fptr2;
    int     rastererase;
    segstruc * asegptr;
    struct gp1_attr saved_attr;
    int saved_ddfont;
    short   lineop, fillop, fillrastop, rtextop;
    short   vtextop, RAS8func, lineindex, textindex, fillindex;
    short opcolorset;
    int     ddfont, linestyle, linewidth;
    ipt_type ddcp, ndccp, ddup, ddpath, ddspace;
    porttype port;
    short *_core_gp1_begin_batch();

    if (rawdevflag)
    	ip_raw = (struct gp1ddvars *) ip;
    else
	ip_win = (struct gp1pwddvars *) ip;
    rastererase = ddstruct -> int1;
    asegptr = (segstruc *) ddstruct -> ptr1;
    sptr = asegptr -> vsurfptr[ddstruct -> int2];

    if (!(asegptr -> segats.visbilty))
	return (0);
    _core_critflag++;
    sdstring = (char *) sdstr;

    offset = asegptr -> pdfptr;
    _core_pdfseek (offset, 0, &pdfptr);
    vcount = 100;		/* lock every n opcodes */
    if (!rawdevflag || wind->needlock)
	{
    	if (_core_gp1_pw_lock (wind -> pixwin, (Rect *)&wind -> rect,
		attr))
	    _core_gp1_winupdate (wind, &port, attr);
	}

 /* save the appropriate parts of the gp attribute structure */
 /* save the old image transform first */
    fptr1 = &(attr -> mtxlist[2][0][0]);
    fptr2 = &(saved_attr.mtxlist[2][0][0]);
    for (i = 0; i < 16; i++)
	*fptr2++ = *fptr1++;
    saved_attr.xscale = attr -> xscale;
    saved_attr.xoffset = attr -> xoffset;
    saved_attr.yscale = attr -> yscale;
    saved_attr.yoffset = attr -> yoffset;
    saved_attr.zscale = attr -> zscale;
    saved_attr.zoffset = attr -> zoffset;
    saved_attr.ndcxscale = attr -> ndcxscale;
    saved_attr.ndcxoffset = attr -> ndcxoffset;
    saved_attr.ndcyscale = attr -> ndcyscale;
    saved_attr.ndcyoffset = attr -> ndcyoffset;
    saved_attr.ndczscale = attr -> ndczscale;
    saved_attr.ndczoffset = attr -> ndczoffset;
    saved_attr.op = attr -> op;
    saved_attr.color = attr -> color;
    saved_attr.wldclipplanes = attr -> wldclipplanes;
    saved_attr.outclipplanes = attr -> outclipplanes;
    saved_attr.hiddensurfflag = attr -> hiddensurfflag;
    saved_ddfont = *passed_ddfont;
    opcolorset = -1;

    while (TRUE) {		/* read PDF output to DD */
	offset = _core_pdfread (SHORT, &sdopcode);
	if ((vcount <= 0) && (!rawdevflag || wind->needlock)) {
	    vcount = 100;
	    pw_unlock (wind -> pixwin);
	    if (_core_gp1_pw_lock (wind -> pixwin, (Rect *)&wind -> rect,
			attr))
		_core_gp1_winupdate (wind, &port, attr);
	}
	switch (sdopcode) {
	    case PDFMOVE: 
		vcount--;
		_core_pdfread (FLOAT * 3, (short *)&ndccp);
		break;
	    case PDFLINE: 
		vcount--;
		if (linestyle || (linewidth > 1)) {
		    ip1 = ndccp;
		    _core_pdfread (FLOAT * 3, (short *)&ndccp);
		    ip2 = ndccp;
		    if (!asegptr -> idenflag) {
			p0 = ip1;
			_core_imxfrm3 (asegptr, &p0, &ip1);
			_core_imxfrm3 (asegptr, &ndccp, &ip2);
		    }
		    if (_core_outpclip &&
			    !_core_oclpvec2 (&ip1, &ip2, &port))
			break;	/* if not visible */
		    devscale (ip1.x, ip1.y, i1, i2);
		    devscale (ip2.x, ip2.y, i3, i4);
		    i = lineop | PIX_COLOR (lineindex);
		    if (rawdevflag == RAWDEV)
		        _core_csimline (i1, i2, i3, i4, i, wind,
				linewidth, linestyle);
		    else
		        _core_cpwsimline (i1, i2, i3, i4, i, wind,
				linewidth, linestyle);
		}
		else {
		    register short *ipa;
		    register short *shmptr;
		    int pdfx, pdfy, pdfz;
		    short *cntptr;

		    if (opcolorset != LINE_OPCOLOR)
			{
			_core_gp1_set_op (lineop, attr);
			_core_gp1_set_color (lineindex, attr);
			opcolorset = LINE_OPCOLOR;
			}
		    shmptr = _core_gp1_begin_batch(wind, attr, &port);
		    *shmptr++ = GP1_USEFRAME | attr->statblkindx;
		    *shmptr++ = GP1_CORENDCVEC_3D;
		    cntptr = shmptr;
		    shmptr++;
		    i = (512 - 23) / 12;
		    _core_pdfseek (offset, 0, &pdfptr);
		    ipa = pdfptr;
		    pdfx = ndccp.x;
		    pdfy = ndccp.y;
		    pdfz = ndccp.z;

		    /* 
		     * in case this is an openseg, have to mark end in case
                     * we run over into garbage
		     */
		    if ( (_core_openseg != 0) && (asegptr == _core_openseg) )
			_core_pdfmarkend();
		
		    do {
			if (sdopcode == PDFLINE) {
			    CORE_GP_PUT_INT(shmptr, &pdfx);
			    CORE_GP_PUT_INT(shmptr, &pdfy);
			    CORE_GP_PUT_INT(shmptr, &pdfz);
			    *((short *)(&pdfx)) = *ipa++;
			    *((short *)(&pdfx)+1) = *ipa++;
			    *((short *)(&pdfy)) = *ipa++;
			    *((short *)(&pdfy)+1) = *ipa++;
			    *((short *)(&pdfz)) = *ipa++;
			    *((short *)(&pdfz)+1) = *ipa++;
			    CORE_GP_PUT_INT(shmptr, &pdfx);
			    CORE_GP_PUT_INT(shmptr, &pdfy);
			    CORE_GP_PUT_INT(shmptr, &pdfz);
			    i--;
			    sdopcode = *ipa++;
			} else if (sdopcode == PDFMOVE) {
			    *((short *)(&pdfx)) = *ipa++;
			    *((short *)(&pdfx)+1) = *ipa++;
			    *((short *)(&pdfy)) = *ipa++;
			    *((short *)(&pdfy)+1) = *ipa++;
			    *((short *)(&pdfz)) = *ipa++;
			    *((short *)(&pdfz)+1) = *ipa++;
			    sdopcode = *ipa++;
			} else if (sdopcode == PDFPICKID) {
			    ipa++;
			    ipa++;
			    n = *ipa++;
			    ipa += (n * (FLOAT * 3));
			    sdopcode = *ipa++;
			} else
			    break;

		    } while (i);
		    attr -> shmptr = shmptr;
		    *cntptr = (512 - 23) / 12 - i;
		    _core_gp1_end_batch (wind -> pixwin, attr);
/*			    veccount += (512-23)/12-i;*/
		    _core_pdfskip (SHORT * (int) (ipa - pdfptr) - SHORT);
		    ndccp.x = pdfx;
		    ndccp.y = pdfy;
		    ndccp.z = pdfz;
		}
		break;
	    case PDFTEXT: 
		vcount -= 5;
		_core_pdfread (SHORT, &n);
		_core_pdfread ((n+1)>>1, sdstr);
		*(sdstring + n) = '\0';
		if (asegptr -> idenflag) {
		    ddcp = ndccp;
		}
		else
		    _core_imxfrm3 (asegptr, &ndccp, &ddcp);
		if ((pdfcurrent.chqualty == STRING) && (sptr -> txhardwr)) {
		    _core_gp1_ddtext (rawdevflag, ip, ddfont, openfont, pf, rtextop, textindex, &ddcp, wind, &port, 0, sdstring);
		}
		else {		/* use vector font text */
		    devscale (ddcp.x, ddcp.y, x, y);
		    vp = &port;
		    devscale (vp -> xmin, vp -> ymin, vwport.xmin, vwport.ymax);
		    devscale (vp -> xmax, vp -> ymax, vwport.xmax, vwport.ymin);
		    i1 = vtextop | PIX_COLOR (textindex);
		    if (rawdevflag == RAWDEV)
		        _core_cprvectext (sdstring, ddfont, &ddup,
			    &ddpath, &ddspace, x, y, &vwport, i1, wind);
		    else
		        _core_cpwvectext (sdstring, ddfont, &ddup,
			    &ddpath, &ddspace, x, y, &vwport, i1, wind);
		}
		break;
	    case PDFMARKER: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&mk);
		pdfcurrent.marker = mk;
		*sdstring = mk;
		*(sdstring + 1) = '\0';
		if (!asegptr -> idenflag)
		    _core_imxfrm3 (asegptr, &ndccp, &ddcp);
		else
		    ddcp = ndccp;
		if (_core_outpclip) {
		    if (!_core_oclippt2 (ddcp.x, ddcp.y, &port))
			break;
		}
		i1 = rtextop | PIX_COLOR (textindex);
		devscale (ddcp.x, ddcp.y, x, y);
		vp = &port;
		devscale (vp -> xmin, vp -> ymin, vwport.xmin, vwport.ymax);
		devscale (vp -> xmax, vp -> ymax, vwport.xmax, vwport.ymin);
		x = x - (_core_defaultfont -> pf_defaultsize.x / 2);
	    /*-vwport.xmin;*/
		y = y + (_core_defaultfont -> pf_defaultsize.y * 3 / 8);
	    /*- vwport.ymin;*/
		pw_ttext (wind -> pixwin, x, y, i1, _core_defaultfont, sdstring);
		break;
	    case PDFBITMAP: 
		vcount -= 10;
		_core_pdfread (FLOAT, (short *)&raster.width);
		_core_pdfread (FLOAT, (short *)&raster.height);
		_core_pdfread (FLOAT, (short *)&raster.depth);
		if (raster.depth == 1)
		    size = ((raster.width + 15) >> 4) * raster.height * 2;
		else
		    if (raster.depth == 8)
			size = raster.width * raster.height;
		    else
			size = raster.width * raster.height * 3;
		_core_pdfseek (0, 1, &raster.bits);
		ip2.x = port.xmin;
		ip2.y = port.ymin;
		ip1.x = port.xmax;
		ip1.y = port.ymax;
		if (!asegptr -> idenflag)
		    _core_imxfrm3 (asegptr, &ndccp, &ddcp);
		else
		    ddcp = ndccp;
		_core_gp1_ddrasterput (rawdevflag, ip, wind, &ddcp, fillrastop, RAS8func, fillindex,  &raster, &ip2, &ip1);
		_core_pdfskip (size);
		break;
	    case PDFPOL2: 	/* plot 2-D polygon */
		vcount -= 10;
		_core_pdfread (SHORT, &n);
		for (i = 0; i < n; i++) {
		    _core_pdfread (FLOAT * 4, (short *)&_core_vtxlist[i]);
		}
		if (opcolorset != REGION_OPCOLOR)
			{
			_core_gp1_set_op(fillop, attr);
			_core_gp1_set_color(fillindex, attr);
			opcolorset = REGION_OPCOLOR;
			}
		if ((i1 = attr->hiddensurfflag) != GP1_NOHIDDENSURF)
			_core_gp1_set_hiddensurfflag(GP1_NOHIDDENSURF, attr);
		_core_gp1_polygon_2(wind, &port, attr, n, _core_vtxlist);
		if (i1 != GP1_NOHIDDENSURF)
			_core_gp1_set_hiddensurfflag(i1, attr);
		break;
	    case PDFPOL3: 	/* plot 3-D polygon */
		vcount -= 10;
		_core_pdfread (SHORT, &n);
		for (i = 0; i < n; i++) {
		    _core_pdfread (FLOAT * 8, (short *)&_core_vtxlist[i]);
		}
		i1 = (rawdevflag) ? ip_raw->polyintstyle : ip_win->polyintstyle;
		if (i1 == PLAIN)
			i1 = WARNOCK;
		else
			i1 = _core_shading_parameters.shadestyle;
		i2 = (rawdevflag) ? ip_raw->openzbuffer : ip_win->openzbuffer;
		if (i2 != SWZB)
			{
			if (opcolorset != REGION_OPCOLOR)
				{
				_core_gp1_set_op(fillop, attr);
				_core_gp1_set_color(fillindex, attr);
				opcolorset = REGION_OPCOLOR;
				}
			i2 = (sptr->hiddenon) ?
			    GP1_ZBHIDDENSURF : GP1_NOHIDDENSURF;
			if (i2 != attr->hiddensurfflag)
				_core_gp1_set_hiddensurfflag(i2, attr);
			_core_gp1_polygon_3(wind, &port, attr,
			    asegptr, n, _core_vtxlist, i1);
			}
		else
			{
			/* image transform the polygon */
			if (!asegptr -> idenflag)
				{
		    		if (asegptr -> type == XLATE2)
					{		/* translate */
					for (i = 0; i < n; i++)
						{
			    			_core_vtxlist[i].x +=
						    asegptr -> imxform[3][0];
			    			_core_vtxlist[i].y +=
						    asegptr -> imxform[3][1];
			    			_core_vtxlist[i].z +=
						    asegptr -> imxform[3][2];
						}
		    			}
		    		else if (asegptr -> type == XFORM2)
					{		/* full xform */
			    		for (i = 0; i < n; i++)
						{
						ip2.x = _core_vtxlist[i].x;
						ip2.y = _core_vtxlist[i].y;
						ip2.z = _core_vtxlist[i].z;
						ip2.w = _core_vtxlist[i].w;
						_core_imxfrm3 (asegptr, &ip2,
						    (ipt_type *)&_core_vtxlist[i]);
						ip2.x = _core_vtxlist[i].dx;
						ip2.y = _core_vtxlist[i].dy;
						ip2.z = _core_vtxlist[i].dz;
						ip2.w = _core_vtxlist[i].dw;
						_core_imszpt3 (asegptr, &ip2,
						    (ipt_type *)&_core_vtxlist[i].dx);
						}
			    		}
				}
			if (_core_outpclip)
				{
		    		_core_vtxcount = n;
		    		for (i = 0; i < n; i++)
					{		/* clip to view port  */
					_core_oclpvtx2 (&_core_vtxlist[i],
					    &port);
		    			}
		    		_core_oclpend2 ();
		    		i1 = _core_vtxcount - n;    /* vertex count */
		    		vptr1 = &_core_vtxlist[n];
		    		vptr2 = &_core_ddvtxlist[n];
				}
			else
				{
		    		i1 = n;		/* vertex count */
		    		vptr1 = &_core_vtxlist[0];
		    		vptr2 = &_core_ddvtxlist[0];
				}
	    		/* output polygon */
			for (i = 0; i < i1; i++)
				{
		    		vptr2[i] = vptr1[i];
		    		devscale(vptr1[i].x, vptr1[i].y,
				    vptr2[i].x, vptr2[i].y);
				}
			i2 = fillop | PIX_COLOR (fillindex);
			if (rawdevflag)
				_core_cregion3(vptr2, i1, i2, wind,
				    ip_raw->zbuffer, ip_raw->cutaway,
				    ip_raw->polyintstyle, sptr->hiddenon);
			else
				_core_cpwpoly3 (vptr2, i1, i2, wind,
				    ip_win->zbuffer, ip_win->cutaway,
				    ip_win->polyintstyle, sptr -> hiddenon);
			}
		break;
	    case PDFLCOLOR: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.lineindx));
		lineop = _core_gp1_ddsetcolor (pdfcurrent.rasterop,
			rastererase || !pdfcurrent.lineindx,
			rastererase || !pdfcurrent.lineindx);
		lineindex = pdfcurrent.lineindx;
		if (opcolorset == LINE_OPCOLOR)
			{
			_core_gp1_set_op (lineop, attr);
			_core_gp1_set_color (lineindex, attr);
			}
		break;
	    case PDFFCOLOR: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.fillindx));
		fillop = _core_gp1_ddsetcolor (pdfcurrent.rasterop,
			rastererase, rastererase);
		fillindex = pdfcurrent.fillindx;
		fillrastop = RAS8func = fillop;
		if (opcolorset == REGION_OPCOLOR)
			{
			_core_gp1_set_op (fillop, attr);
			_core_gp1_set_color (fillindex, attr);
			}
		break;
	    case PDFTCOLOR: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.textindx));
		rtextop = _core_gp1_ddsetcolor (pdfcurrent.rasterop,
			rastererase || !pdfcurrent.textindx,
			rastererase);
		textindex = pdfcurrent.textindx;
		vtextop = rtextop;
		break;
	    case PDFLINESTYLE: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.linestyl));
		linestyle = pdfcurrent.linestyl;
		break;
	    case PDFPISTYLE: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.polyintstyl));
		if (rawdevflag) {
		    ip_raw->polyintstyle = pdfcurrent.polyintstyl;
		} else {
		    ip_win->polyintstyle = pdfcurrent.polyintstyl;
		}
		break;
	    case PDFPESTYLE: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.polyedgstyl));
		break;
	    case PDFLINEWIDTH: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.linwidth));
		f = (float) ((_core_ndcspace[0] < _core_ndcspace[1]) ?
			_core_ndcspace[0] : _core_ndcspace[1]);
		linewidth = pdfcurrent.linwidth * f / 100.;
		linewidth = (linewidth * wind -> xscale) >> 15;
		break;
	    case PDFFONT: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.font));
		ddfont = pdfcurrent.font;
		break;
	    case PDFPEN: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.pen));
		break;
	    case PDFSPACE: 
		vcount--;
		_core_pdfread (FLOAT * 3, (short *)&ddspace);
	    /* image transform spacing vector */
		if (!asegptr -> idenflag) {
		    ip1 = ddspace;
		    _core_imszpt3 (asegptr, &ip1, &ddspace);
		}
		ddspace.x = ddspace.x * wind -> xscale >> 10;
		ddspace.y = ddspace.y * -wind -> yscale >> 10;
		break;
	    case PDFPATH: 
		vcount--;
		_core_pdfread (FLOAT * 3, (short *)&ddpath);
	    /* image transform path vector */
		if (!asegptr -> idenflag) {
		    ip1 = ddpath;
		    _core_imszpt3 (asegptr, &ip1, &ddpath);
		}
		ddpath.x = ddpath.x * wind -> xscale >> 10;
		ddpath.y = ddpath.y * -wind -> yscale >> 10;
		break;
	    case PDFUP: 
		vcount--;
		_core_pdfread (FLOAT * 3, (short *)&ddup);
	    /* image transform up vector */
		if (!asegptr -> idenflag) {
		    ip1 = ddup;
		    _core_imszpt3 (asegptr, &ip1, &ddup);
		}
		ddup.x = ddup.x * wind -> xscale >> 10;
		ddup.y = ddup.y * -wind -> yscale >> 10;
		break;
	    case PDFCHARQUALITY: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.chqualty));
		break;
	    case PDFCHARJUST: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.chjust));
		break;
	    case PDFROP: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.rasterop));
		if (_core_xorflag)
		    pdfcurrent.rasterop = XORROP;
		break;
	    case PDFPICKID: 
		vcount--;
		_core_pdfread (FLOAT, (short *)&(pdfcurrent.pickid));
		_core_pdfread (SHORT, &n);
		_core_pdfskip (FLOAT * n * 3);
		break;
	    case PDFVWPORT: 	/* get viewport from PDF */
		vcount -= 10;
		_core_pdfread (FLOAT * 6, (short *)&port);
		_core_gp1_winupdate (wind, &port, attr);
		_core_set_gp_imxform (asegptr -> idenflag, asegptr -> imxform,
			wind, &port, attr);
		break;
	    case PDFENDSEGMENT: 
		goto done;
	    default: 
		break;
	}
    }

done: 

    if (!rawdevflag || wind->needlock)
    	pw_unlock (wind -> pixwin);

 /* restore gp attr stuff */
    _core_gp1_set_matrix_3d(wind->pixwin, 2,
	&saved_attr.mtxlist[2][0][0], attr);
    attr -> xscale = saved_attr.xscale;
    attr -> xoffset = saved_attr.xoffset;
    attr -> yscale = saved_attr.yscale;
    attr -> yoffset = saved_attr.yoffset;
    attr -> zscale = saved_attr.zscale;
    attr -> zoffset = saved_attr.zoffset;
    attr -> ndcxscale = saved_attr.ndcxscale;
    attr -> ndcxoffset = saved_attr.ndcxoffset;
    attr -> ndcyscale = saved_attr.ndcyscale;
    attr -> ndcyoffset = saved_attr.ndcyoffset;
    attr -> ndczscale = saved_attr.ndczscale;
    attr -> ndczoffset = saved_attr.ndczoffset;
    attr -> op = saved_attr.op;
    attr -> color = saved_attr.color;
    attr -> wldclipplanes = saved_attr.wldclipplanes;
    attr -> outclipplanes = saved_attr.outclipplanes;
    attr -> hiddensurfflag = saved_attr.hiddensurfflag;

    *passed_ddfont = saved_ddfont;

 /* make a note that things may have changed */
    attr -> attrchg = TRUE;
    attr -> xfrmtype = XFORMTYPE_CHANGE;

    if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	(*_core_sighandle) ();
    return (0);
}
