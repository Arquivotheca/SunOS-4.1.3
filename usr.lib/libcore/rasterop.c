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
static char sccsid[] = "@(#)rasterop.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
 *  These rasterop routines support the transfer of rasters between
 *  memory and framebuffer, memory and memory, and framebuffer and
 *  framebuffer.  Also for raster text strings.
 */
#include <framebuf.h>
#define	XS	((short *)(GXBase+GXset1+GXselectX))
#define	YS	((short *)(GXBase+GXsource+GXset1+GXselecty))
#define	XD	((short *)(GXBase+GXselectX))
#define	YD	((short *)(GXBase+GXsource+GXupdate+GXselectY))
#define	XDD	((short *)(GXBase+GXsource+GXupdate+GXselectX))
#define	YDD	((short *)(GXBase+GXselectY))
typedef char unsigned uchar;
struct raster{			/* general definition of a bit raster */
	int width, height, depth;
	short *bits;
};
/*------------------------------------------------------------------*/
#define	touch(p)	((p)=0)
#define	loop(s)\
for(j=height;j>15;j-=16){\
	s;s;s;s;\
	s;s;s;s;\
	s;s;s;s;\
	s;s;s;s;\
}\
switch(j){\
case 15: s; case 14: s; case 13: s; case 12: s;\
case 11: s; case 10: s; case 9:  s; case 8:  s;\
case 7:  s; case 6:  s; case 5:  s; case 4:  s;\
case 3:  s; case 2:  s; case 1:  s;\
}
/*------------------------------------------------------------------*/
/* operate screen-to-screen up and right */
ROPssur(xs, ys, xd, yd, width, height){
	register short *yda=YD+yd, *xda=XD+xd;
	register short *ysa=YS+ys, *xsa=XS+xs;
	register short i, j;
	for(i=width;i>0;i-=16){
		if(i>=16)
			GXwidth = 16;
		else
			GXwidth = i;
		touch(*xda);
		touch(*xsa);
		j = *ysa++;	/* prime the pump */
		loop(*yda++ = *ysa++);
		yda -= height;
		xda += 16;
		ysa -= height+1;	/* 1 extra to prime the pump with */
		xsa += 16;
	}
}
/*------------------------------------------------------------------*/
/*
 * operate screen-to-screen down and right
 */
ROPssdr(xs, ys, xd, yd, width, height){
	register short *yda=YD+yd+height, *xda=XD+xd;
	register short *ysa=YS+ys+height, *xsa=XS+xs;
	register short i, j;
	for(i=width;i>0;i-=16){
		if(i>=16)
			GXwidth = 16;
		else
			GXwidth = i;
		touch(*xda);
		touch(*xsa);
		j = *--ysa;	/* prime the pump */
		loop(*--yda = *--ysa);
		yda += height;
		xda += 16;
		ysa += height+1;	/* 1 extra to prime the pump with */
		xsa += 16;
	}
}
/*------------------------------------------------------------------*/
/*
 * operate screen-to-screen up and left
 */
ROPssul(xs, ys, xd, yd, width, height){
	register short *yda=YD+yd, *xda=XD+xd+width;
	register short *ysa=YS+ys, *xsa=XS+xs+width;
	register short i, j;
	for(i=width;i>0;i-=16){
		if(i>=16){
			xda -= 16;
			xsa -= 16;
			GXwidth = 16;
		}
		else{
			xda -= i;
			xsa -= i;
			GXwidth = i;
		}
		touch(*xda);
		touch(*xsa);
		j = *ysa++;	/* prime the pump */
		loop(*yda++ = *ysa++);
		yda -= height;
		ysa -= height+1;	/* 1 extra to prime the pump with */
	}
}
/*------------------------------------------------------------------*/
/*
 * operate screen-to-screen down and left
 */
ROPssdl(xs, ys, xd, yd, width, height){
	register short *yda=YD+yd+height, *xda=XD+xd+width;
	register short *ysa=YS+ys+height, *xsa=XS+xs+width;
	register short i, j;
	for(i=width;i>0;i-=16){
		if(i>=16){
			xda -= 16;
			xsa -= 16;
			GXwidth = 16;
		}
		else{
			xda -= i;
			xsa -= i;
			GXwidth = i;
		}
		touch(*xda);
		touch(*xsa);
		j = *--ysa;	/* prime the pump */
		loop(*--yda = *--ysa);
		yda += height;
		ysa += height+1;	/* 1 extra to prime the pump with */
	}
}
/*------------------------------------------------------------------*/
/*
 * operate don't-care-to-screen
 */
ROPds(xd, yd, width, height){
	register short *yda=YD+yd, *xda=XD+xd;
	register short i, j;
	for(i=width;i>0;i-=16){
		if(i>=16)
			GXwidth = 16;
		else
			GXwidth = i;
		touch(*xda);
		loop(touch(*yda++));
		yda -= height;
		xda += 16;
	}
}
/*------------------------------------------------------------------*/
/*
 * operate don't-care-to-screen for horizontal line segments
 */
ROPdsl(xd, yd, width)
{
	register short	*yda=(short *)(YDD+yd),
			*xda=(short *)(XDD+xd);
	register short i, j;

	touch(*yda);
	GXwidth = 16;
	for(i=width>>4; i>0; i--){
	    *xda = j;
	    xda += 16;
	    }
	if(width &= 0xF){
	    GXwidth = width;		/* set a narrower width */
            *xda = j;
	    }
	return(0);
}
/*------------------------------------------------------------------*/
/*
 * operate memory-to-screen
 */
ROPms(xs, ys, rs, xd, yd, width, height)
struct raster *rs;
{
	register short *yda=YD+yd, *xda=XD+xd;
	register int wwid = ((rs->width+15)/16);
	register short *sa; short *ssa, word;
	register uchar *sba; uchar *ssba;
	register int i, j, skew;
	short k;

	if (rs->depth == 1) {		/* paint 1 bit/pixel  */
	    ssa=rs->bits + ys * wwid + xs/16;
	    sa = ssa;
	    if((skew=xs&15)!=0){
		    if (width<16-skew)  { GXwidth=width;   width=0; }
		    else   { GXwidth=16-skew; width -= 16-skew; }
		    touch(*xda);
		    loop(*yda++ = *sa<<skew; sa += wwid);
		    yda -= height;
		    xda += 16-skew;
		    sa = ++ssa;
		    }
	    for(i=width;i>0;i-=16){
		    if(i>=16)  GXwidth = 16;
		    else       GXwidth = i;
		    touch(*xda);
		    loop(*yda++ = *sa; sa += wwid);
		    yda -= height;
		    xda += 16;
		    sa = ++ssa;
		    }
	    }
	else {				/* paint 8 bits/pixel */
	    ssba = (uchar*)rs->bits + ys*rs->width + xs;
	    sba = ssba;
	    yda=YDD+yd, xda=XDD+xd;
	    for (j=height; j>0; j--) {
		touch(*yda);
		GXwidth = 16;
		for(i=width; i>15; i-=16){
		    word = 0;
		    if(*sba++) word|=0x8000; if(*sba++) word|=0x4000;
		    if(*sba++) word|=0x2000; if(*sba++) word|=0x1000;
		    if(*sba++) word|=0x800; if(*sba++) word|=0x400;
		    if(*sba++) word|=0x200; if(*sba++) word|=0x100;
		    if(*sba++) word|=0x80; if(*sba++) word|=0x40;
		    if(*sba++) word|=0x20; if(*sba++) word|=0x10;
		    if(*sba++) word|=0x8; if(*sba++) word|=0x4;
		    if(*sba++) word|=0x2; if(*sba++) word|=0x1;
		    *xda = word; xda += 16;
		    }
		GXwidth = i;
		word = 0; skew = 0x8000;
		for (k=i; k>0; k--) {
		    if(*sba++) word|=skew; skew >>= 1;
		    }
		*xda = word; xda -= width-i;
		yda++;
		sba += rs->width - width;
		}
	    }
}
/*------------------------------------------------------------------*/
/*
 * copy screen-to-memory
 */
#define	mask(nbit)	((1<<(nbit))-1)	/* the n rightmost bits of a word */
ROPcopysm(xs, ys, xd, yd, rd, width, height)
struct raster *rd;
{
	register short wwid = ((rd->width+15)/16);
	register short *da, *dda=rd->bits + yd * wwid + xd/16;
	register short *ysa=YS+ys, *xsa;
	register short j, sm, dm, skew;
	short i;
	GXwidth = 16;
	da = dda++;
	if((skew=xd&15)!=0){
		sm=mask(16-skew);
		if(width<16-skew){
			sm &= ~mask(16-skew-width);
			width=0;
		}
		else
			width-=16-skew;
		dm = ~sm;
		xsa=XS+((xs-skew)&1023);
		touch(*xsa);
		j = *ysa++;	/* prime the pump */
		loop(*da = (*ysa++&sm)|(*da&dm); da += wwid);
		da = dda++;
		ysa -= height+1;	/* 1 extra to prime the pump with */
		xsa=XS+xs+16-skew;
	}
	else
		xsa=XS+xs;
	for(i=width;i>15;i-=16){
		touch(*xsa);
		j = *ysa++;	/* prime the pump */
		loop(*da = *ysa++; da += wwid);
		da = dda++;
		ysa -= height+1;	/* 1 extra to prime the pump with */
		xsa += 16;
	}
	if(i!=0){
		dm=mask(16-i);
		sm = ~dm;
		touch(*xsa);
		j = *ysa++;	/* prime the pump */
		loop(*da = (*ysa++&sm)|(*da&dm); da += wwid);
	}
}
/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXinvert
 */
ROPdminv(xd, yd, rd, width, height)
struct raster *rd;
{
	register short wwid = ((rd->width+15)/16);
	register short *da, *dda=rd->bits + yd * wwid + xd/16;
	register short i, j, sm, skew;
	da = dda++;
	if((skew=xd&15)!=0){
		sm=mask(16-skew);
		if(width<16-skew){
			sm &= ~mask(16-skew-width);
			width=0;
		}
		else
			width-=16-skew;
		loop(*da ^= sm; da += wwid);
		da = dda++;
	}
	for(i=width;i>15;i-=16){
		loop(*da ^= -1; da += wwid);
		da = dda++;
	}
	if(i!=0){
		sm=~mask(16-i);
		loop(*da ^= sm; da += wwid);
	}
}
/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXset
 */
ROPdmset(xd, yd, rd, width, height)
struct raster *rd;
{
	register short wwid = ((rd->width+15)/16);
	register short *da, *dda=rd->bits + yd * wwid + xd/16;
	register short i, j, sm, skew;
	da = dda++;
	if((skew=xd&15)!=0){
		sm=mask(16-skew);
		if(width<16-skew){
			sm &= ~mask(16-skew-width);
			width=0;
		}
		else
			width-=16-skew;
		loop(*da |= sm; da += wwid);
		da = dda++;
	}
	for(i=width;i>15;i-=16){
		loop(*da = -1; da += wwid);
		da = dda++;
	}
	if(i!=0){
		sm=~mask(16-i);
		loop(*da |= sm; da += wwid);
	}
}
/*------------------------------------------------------------------*/
/*
 * operate don't cares to memory, operation is GXclear
 */
ROPdmclr(xd, yd, rd, width, height)
struct raster *rd;
{
	register short wwid = ((rd->width+15)/16);
	register short *da, *dda=rd->bits + yd * wwid + xd/16;
	register short i, j, sm, skew;
	if((skew=xd&15)!=0){
		sm=mask(16-skew);
		if(width<16-skew){
			sm &= ~mask(16-skew-width);
			width=0;
		}
		else
			width-=16-skew;
		sm = ~sm;
		loop(*da &= sm; da += wwid);
		da = dda++;
	}
	for(i=width;i>15;i-=16){
		loop(*da=0; da += wwid);
		da = dda++;
	}
	if(i!=0){
		sm=mask(16-i);
		loop(*da&=sm; da += wwid);
	}
}
/*------------------------------------------------------------------*/
/* minimal raster font for low precision text */

#define	CHRW	9	/* this should be per-character */
#define	CHRH	16	/* should be per-font.  Should also have independent
			 * leading */
short _core_mapchar[][128][CHRH]={
#include "cmr.c"
,
#include "gacha12.c"
,
#include "sail7.c"
};

typedef struct {
   int xmin, xmax, ymin, ymax, zmin, zmax;
   } porttype;
/*------------------------------------------------------------------*/
ROPtext( xd, yd, font, s, vwp) register char *s; register porttype *vwp;
{
	struct raster chr;
	register int xs,ys,height,width, y0;

	chr.height = CHRH; 
	chr.width = CHRW; 
	chr.depth = 1;
	y0 = yd;
	while(*s != '\0'){
		xs = 0;  ys = 0;
		height = CHRH;
		width = CHRW;
		chr.bits=_core_mapchar[font][*s++];
		if(xd < vwp->xmin) {
			xs = vwp->xmin - xd; width -= xs; xd += xs; }
		if(xd+width > vwp->xmax) width = vwp->xmax - xd;
		if(width <= 0) { xd += width; continue; }
		if(yd < vwp->ymin) {
			ys = vwp->ymin - yd; height -= ys; yd += ys;}
		if(yd+height > vwp->ymax) height = vwp->ymax - yd;
		if(height <= 0) break;
		ROPms( xs, ys, &chr, xd, yd, width, height);
		xd += width;
		yd = y0;
	}
}
