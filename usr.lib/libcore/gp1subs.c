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
static char sccsid[] = "@(#)gp1subs.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include        <sunwindow/window_hs.h>
#include        "coretypes.h"
#include        "corevars.h"
#include        "gp1_pwpr.h"
#include	"gp1dd.h"
#include	"gp1pixwindd.h"

#define devscale(x_ndc_coord, y_ndc_coord, x_dev_coord, y_dev_coord)	\
    if (rawdevflag == TRUE) { \
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
	x_dev_coord = (x_ndc_coord * wind -> xscale >> 15) \
	+wind -> xoff; \
	y_dev_coord = wind -> yoff - \
	(y_ndc_coord * wind -> yscale >> 15); \
}

extern PIXFONT *_core_defaultfont;

_core_set_gp_imxform( ident, imx, wind, port, gpattr)
	int ident;
	int imx[4][4];
	struct windowstruct *wind;
	porttype *port;
	struct gp1_attr *gpattr;
{
	float it[4][4], vt[4][4], gpmat[4][4];
	float *m1, *m2;
	int *ip;
	
	m2 = (float*)vt;		/* init viewport to gpclip matrix */
	while (m2 <= &vt[3][3])
		*m2++ = 0.0;
	vt[0][0] = 2.0 / (port->xmax - port->xmin);
	vt[1][1] = 2.0 / (port->ymax - port->ymin);
	vt[2][2] = 1.0 / (port->zmax - port->zmin);
	vt[3][3] = 1.0;
	vt[3][0] = (float)(port->xmin + port->xmax) /
		   (float)(port->xmin - port->xmax);
	vt[3][1] = (float)(port->ymin + port->ymax) /
		   (float)(port->ymin - port->ymax);
	vt[3][2] = - port->zmin/(float)(port->zmax - port->zmin);

	if (ident)
		_core_gp1_set_matrix_3d( wind->pixwin, 2, (float *)vt, gpattr);
	else {
		m1 = (float *)it;	/* init image transform matrix */
		ip = (int*)imx;
		while (m1 < &it[3][0]) {	/* fixed point to float */
			*m1++ = (float)(*ip >> 16) + (short)(*ip & 0xFFFF) /
			    32768.0;
			ip++;
		}		/* the offsets are not fixed point! */
		*m1++ = *ip++; *m1++ = *ip++; *m1++ = *ip++;
		it[3][3] = 1.0;
		m1 = (float*)it;
		m2 = (float*)vt;
		_core_gp1_matmul_3d( wind->pixwin, m1, m2, (float *)gpmat, 
		    gpattr);
		_core_gp1_set_matrix_3d( wind->pixwin, 2, (float *)gpmat, 
		    gpattr);
	}
}

_core_gp1_ddtext( rawdevflag, ip, ddfont, openfont, pf, rtextop, textindex, ddcp, wind, vp, scale, string)
	int rawdevflag;
	caddr_t ip;
	int ddfont, *openfont;
	short rtextop, textindex;
	ipt_type *ddcp;
	porttype *vp;
	int scale;
	char *string;
	PIXFONT **pf;
	struct windowstruct *wind;
{
	struct gp1ddvars *ip_raw;
	int op, x, y;
	short i3, i4;
	porttype vwport;

	ip_raw = (struct gp1ddvars *)ip;
	if (ddfont != *openfont) {
	    if (*pf != _core_defaultfont) pf_close( *pf);
	    *openfont = ddfont;
	    switch ( ddfont ) {
	    case 0: *pf = pf_open(
		    "/usr/lib/fonts/fixedwidthfonts/gallant.r.10");
		    break;
	    case 1: *pf = pf_open(
		    "/usr/lib/fonts/fixedwidthfonts/gacha.r.8");
		    break;
	    case 2: *pf =
		    pf_open("/usr/lib/fonts/fixedwidthfonts/sail.r.6");
		    break;
	    case 3: *pf =
		    pf_open("/usr/lib/fonts/fixedwidthfonts/gacha.b.8");
		    break;
	    case 4: *pf =
		    pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.r.8");
		    break;
	    case 5: *pf =
		    pf_open("/usr/lib/fonts/fixedwidthfonts/cmr.b.8");
		    break;
	    default: *pf = _core_defaultfont;	/* load default font */
		    break;
	    }
	    if ( !*pf) {
		    *pf = _core_defaultfont;
	    }
	}
	op = rtextop | PIX_COLOR(textindex);
	devscale(ddcp->x, ddcp->y, i3, i4);
	devscale(vp->xmin, vp->ymin, vwport.xmin, vwport.ymax);
	devscale(vp->xmax, vp->ymax, vwport.xmax, vwport.ymin);
	x = i3+(scale * (*pf)->pf_defaultsize.x);	/* vwport.xmin; */
	y = i4 + ((*pf)->pf_defaultsize.y/2);		/* vwport.ymin; */
	pw_ttext( wind->pixwin, x, y, op, *pf, string);
}

extern struct pixrectops mem_ops;

static struct mpr_data mprdata = {0, 0, 0, 0, 1, 0};
static struct pixrect mprect = {&mem_ops, 0, 0, 0, (caddr_t)&mprdata};

_core_gp1_ddrasterput( rawdevflag, ip, wind, ddcp, fillrastop, RAS8func, fillindex, rptr, p1ptr, p2ptr)
	int rawdevflag;
	caddr_t ip;
	struct windowstruct *wind;
	ipt_type *ddcp;
	short fillrastop, RAS8func, fillindex;
	rast_type *rptr;
	ipt_type *p1ptr, *p2ptr;	/* lower left, upper right viewport */
{
	struct gp1ddvars *ip_raw;
	int x, y, xs, ys, width, height;
	int i1, i2, i3, i4;

	ip_raw = (struct gp1ddvars *)ip;
	mprdata.md_linebytes =
		mpr_linebytes(rptr->width,rptr->depth);
	mprdata.md_image = (short *)rptr->bits;
	mprect.pr_size.x = rptr->width;
	mprect.pr_size.y = rptr->height;
	mprect.pr_depth  = rptr->depth;
	width = rptr->width;
	height = rptr->height;
	xs = 0; ys = 0;
	devscale(ddcp->x, ddcp->y, x, y);
	y = y-height+1;
	devscale(p1ptr->x, p1ptr->y, i1, i3);/* vwport xmin,ymin */
	devscale(p2ptr->x, p2ptr->y, i2, i4);/* vwport xmax,ymax */
	if (x<i1){ xs=i1-x; width-=xs; x=i1;}
	if (x+width-1 > i2) width=i2-x+1;
	if (width<=0) return; 
	if (y < i4) { ys = i4-y; height -= ys; y=i4;}
	if (y+height-1 > i3) height = i3-y+1;
	if (height<=0) return;
	i1 = (rptr->depth == 1) ? fillrastop : RAS8func;
	i1 |= PIX_COLOR( fillindex);
	pw_write( wind->pixwin, x, y, width, height, i1, &mprect,
	    xs, ys);
}

_core_gp1_ddsetcolor( rop, orerase, erase)
	int rop, orerase, erase;
{
	int opcode;

	if (rop == 1) {				/* XORing */
	    opcode = PIX_SRC ^ PIX_DST;
	} else if (rop == 2) {			/* ORing */
	    if (orerase) {		
		opcode = ~PIX_SRC & PIX_DST;		/* erase */
	    } else {
		opcode = PIX_SRC | PIX_DST;		/* paint */
	    }
	} else {				/* SRC */
	    opcode = (erase) ? PIX_CLR : PIX_SRC;
	}
	return( opcode);
}
