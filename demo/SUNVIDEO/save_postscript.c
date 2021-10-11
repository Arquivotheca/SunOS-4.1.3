#ifndef lint
static	char sccsid[] = "@(#)save_postscript.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


/*
 * This software converts the data in the SunVideo frame buffer
 * to either a postscript or an encapsulated postscript file.
 * The postscript stuff was originally taken from a program
 * written by Paul Heckbert at PIXAR. Sun made the modifications
 * for this code and the encapsulated postscript conversion.
 */
#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>
#include "video_mat.h"

#define PS		0	/* Dump as postscript file */
#define EPSF		1	/* Dump as epsf file */

#define PAGEWID		8.5	/* Page width in inches */
#define PAGEHEI		11.0	/* Page height in inches */
#define POINT(x) 	(int)((x)*72.)		/* convert inches to points */
#define MAX(a, b) 	((a)>(b) ? (a) : (b))

/*
 * set postscript image size defaults
 */
static double cenx=4.25, ceny=3; /* Centre of the image in inches */
static double rot=0, size=6;	/* Rotation and size in inches */
static int pwidth;		/* Width of data in file */
int epsf_format=0;
extern int y_offset;

void
save_postscript(src_pixrect, filename, format)
    struct pixrect *src_pixrect;
    char *filename;
    int format;
{
    FILE *file;
    void save_ps(), save_epsf();

    if ((file = fopen(filename, "r"))) {
        /* file exists */
        if (!confirm_overwrite()) {
            fclose(file);
            return;
        }
        fclose(file);
    }
    if (!(file = fopen(filename, "w"))) {
        file_error("Cannot write file.");
    	return;
    }

    if(format == PS)
    	save_ps(src_pixrect, file);
    else
    	save_epsf(src_pixrect, file);
    return;
}

/*
 * Save as postscript file
 */
void
save_ps(src_pixrect, file)
    struct pixrect *src_pixrect;
    FILE *file;
{
    register int x, y;
    int tempx, tempy, bpp;
    int linewords;
    unsigned int *im_data;
    u_char r, g, b, luma;
    void ps_tail(), ps_head(), ps_pixel(), ps_pixelflush();
    void ps_image_head();

    pwidth = 72;
    bpp = 8;
    tempx = src_pixrect->pr_width;
    tempy = src_pixrect->pr_height-y_offset;
    ps_head(file, tempx, tempy);
    ps_image_head(file, tempx, tempy);

    linewords = (mpr_d(src_pixrect)->md_linebytes)/4;
    im_data = (unsigned int *)mpr_d(src_pixrect)->md_image 
    	+ (linewords * y_offset);

    for(y=y_offset; y<(tempy+y_offset); y++) {
    	for(x=0; x<tempx; x++) {
    		b = (u_char)(BLUE(im_data[x]));
    		g = (u_char)(GREEN(im_data[x]));
    		r = (u_char)(RED(im_data[x]));
    		luma = (u_char)((lumar[r] + lumag[g] + lumab[b])/256);
    		ps_pixel(file, luma, bpp);
    	}
    	ps_pixelflush(file, bpp);
    	im_data += linewords;
    }
    ps_tail(file);
    fclose(file);
    return;
}

void
ps_head(file, dx, dy)
FILE *file;
int dx, dy;
{
    char host[20];
    int curtime;

    curtime = time(0);
    gethostname(host, sizeof host);
    fprintf(file, "%%!PS-Adobe-1.0\n");
    fprintf(file, "%%%%Creator: %s:%s\n", host, getlogin());
    fprintf(file, "%%%%Creation Date: %s", ctime(&curtime));
    fprintf(file, "%%%%End Prolog\n");
    fprintf(file, "%%%%Page: ? 1\n\n");
    return;
}

void
ps_image_head(file, dx, dy)
FILE *file;
int dx, dy;
{
    int strlen;
    double maxd, wid, hei;

    strlen = (dx*8+7)/8;	/* this must be right or no output! */
    fprintf(file, "/picstr %d string def\n", strlen);
    maxd = MAX(dx, dy);
    wid = size*dx/maxd;
    hei = size*dy/maxd;
    fprintf(file, "%d %d translate %g rotate %d %d translate\n",
    POINT(cenx), POINT(ceny), rot, POINT(-wid/2.), POINT(-hei/2.));
    fprintf(file, "%d %d scale\n\n", POINT(wid), POINT(hei));
    fprintf(file, "%d %d 8 [%d 0 0 %d 0 %d]\n", dx, dy, dx, -dy, dy);
    fprintf(file, "{currentfile picstr readhexstring pop}\n");
    fprintf(file, "image\n");
    return;
}

void
ps_tail(file)
FILE *file;
{
    fprintf(file, "showpage\n");
    return;
}

/*
 * Save as an encapsulated postscript file
 */
void
save_epsf(src_pixrect, file)
    struct pixrect *src_pixrect;
    FILE *file;
{
    register int x, y, loop;
    int tempx, tempy, bpp;
    int linewords;
    unsigned int *im_data;
    u_char r, g, b, luma;
    u_char oval, dval;
    void epsf_head(), epsf_head_end(), epsf_image_head();
    void epsf_tail(), ps_pixel(), ps_pixelflush();

    tempx = src_pixrect->pr_width;	
    tempy = src_pixrect->pr_height-y_offset;

    /*
     * Go through the image twice and generate a 1-bit version
     * first, and then generate the real printed data
     */
    for(loop=0; loop<2; loop++) {
    	if(loop == 0) {
    		epsf_head(file, tempx, tempy);
    		pwidth = (src_pixrect->pr_width*2)/8;
    		bpp = 1;
    	} else {
    		pwidth = 72;
    		epsf_head_end(file);
    		epsf_image_head(file, tempx, tempy);
    		bpp = 8;
    	}

    	linewords = (mpr_d(src_pixrect)->md_linebytes)/4;
    	im_data = (unsigned int *)mpr_d(src_pixrect)->md_image 
    		+ (linewords * y_offset);

    	for(y=y_offset; y<(tempy+y_offset); y++) {
    		/*
    		 *  % comment at start of bitmap line
    		 */
    		if(loop == 0)
    			fprintf(file, "%% ");
    		for(x=0; x<tempx; x++) {
    			b = (u_char)(BLUE(im_data[x]));
    			g = (u_char)(GREEN(im_data[x]));
    			r = (u_char)(RED(im_data[x]));
    			luma = (u_char)((lumar[r] + lumag[g] + lumab[b])/256);
    			if(loop == 0) { 
    				/* Dither image to 1-bit */
    				dval = dither_mat[y%16][x%16] ;
    				oval = luma <= dval;
    				ps_pixel(file, oval, bpp);
    			} else 
				ps_pixel(file, luma, bpp);
    		}
    		ps_pixelflush(file, bpp);
    		im_data += linewords;
    	}
    }
    epsf_tail(file);
    fclose(file);
    return;
}

void
epsf_head(file, dx, dy)
    FILE *file;
    int dx, dy;
{
    char host[20];
    int curtime, strlen;
    double max, wid, hei;

    strlen = (dx*1+7)/8;	/* this must be right or no output! */
    max = MAX(dx, dy);
    wid = size*dx/max;
    hei = size*dy/max;
    curtime = time(0);
    gethostname(host, sizeof host);
    fprintf(file, "%%!PS-Adobe-2.0 EPSF-1.2\n");
    fprintf(file, "%%%%Creator: %s:%s\n", host, getlogin());
    fprintf(file, "%%%%Creation Date: %s", ctime(&curtime));
    fprintf(file, "%%%%BoundingBox: 0 0 %d %d\n",POINT(wid), POINT(hei));
    fprintf(file, "%%%%EndComments\n");
    if(epsf_format) {
	/*
	 * This is the old style format that Frame uses
	 */
	fprintf(file, "%%%%BeginPreview: \n");
	fprintf(file, "%%%%ImageWidth: %d\n", dx);
	fprintf(file, "%%%%ImageHeight: %d\n", dy);
	fprintf(file, "%%%%ImageDepth: 1\n");
	fprintf(file, "%%%%ImageMatrix: [ %d 0 0 %d 0 %d]\n", dx, -dy, dy);
	fprintf(file, "%%%%ImageProc: /picstr %d string def\n", strlen);
	fprintf(file, "%%%%+ .{currentfile picstr readhexstring pop}\n");
	fprintf(file, "%%%%BeginImage: %d\n", dy);
    } else 
	/*
	 * This is the new style format that SunWrite uses
	 */
	fprintf(file, "%%%%BeginPreview: %d %d 1 %d\n",dx, dy, dy);
    return;
}

void
epsf_head_end(file)
    FILE *file;
{
    if(epsf_format)
    	fprintf(file, "%%%%EndImage\n");
    fprintf(file, "%%%%EndPreview\n");
    fprintf(file, "%%%%End Prolog\n");
    fprintf(file, "%%%%Page: ? 1\n\n");
    return;
}

void
epsf_image_head(file, dx, dy)
    FILE *file;
    int dx, dy;
{
    int strlen;
    double max, wid, hei;
    void ps_quote_string(), epsf_gsave();

    strlen = (dx*8+7)/8;	/* this must be right or no output! */
    epsf_gsave(file);
    fprintf(file, "/picstr %d string def\n", strlen);
    max = MAX(dx, dy);
    wid = size*dx/max;
    hei = size*dy/max;
    fprintf(file, "%d %d scale\n\n", POINT(wid), POINT(hei));
    fprintf(file, "%d %d 8 [%d 0 0 %d 0 %d]\n", dx, dy, dx, -dy, dy);
    fprintf(file, "{currentfile picstr readhexstring pop}\n");
    fprintf(file, "%%%%BeginBinary: %d\n", ((dx*dy*8*2)/8+6));
    fprintf(file, "image\n");
    return;
}

void
epsf_tail(file)
    FILE *file;
{
    void epsf_grestore();

    fprintf(file, "%%%%EndBinary\n");
    epsf_grestore(file);
    fprintf(file, "\nshowpage\n");
    fprintf(file, "%%%%Trailer\n");
    return;
}

void
epsf_gsave(file)
    FILE *file;
{
    fprintf(file, "/BEGINEPSFILE { %%def\n");
    fprintf(file, "\t/EPSFsave save def\n");
    fprintf(file, "\t0 setgray 0 setlinecap 1 setlinewidth 0 setlinejoin ");
    fprintf(file, "10 setlimiterlimit [] 0 setdash\n");
    fprintf(file, "\tnewpath\n");
    fprintf(file, "\t/showpage {} def\n");
    fprintf(file, "} bind def\n");
    return;
}

void
epsf_grestore(file)
    FILE *file;
{
    fprintf(file, "/ENDEPSFILE { %%def\n");
    fprintf(file, "\tEPSFsave restore\n");
    fprintf(file, "} bind def\n");
    return;
}


/*
 * ps_pixel: accumulate bpp data, where bpp = 1 or 8,
 * outputting 2 hex digits for each byte
 * Will also work for 2 and 4 if required
 */
static int accum = 0, count = 0, col = 0;

void
ps_pixel(file, data, bpp)
    FILE *file;
    int data, bpp;
{
    accum = accum<<bpp | data;
    count += bpp;
    if (count>=8) {
    	fprintf(file, "%02x", accum);
    	accum = 0;
    	count = 0;
    	col += 2;
    	if (col>=pwidth) {
    		fprintf(file, "\n");
    		col = 0;
    	}
    }
    return;
}

/*
 * ps_pixelflush: pad to finish this byte
 */
void
ps_pixelflush(file, bpp)
    FILE *file;
    int bpp;
{
    while (count!=0) 
    	ps_pixel(file, 0, bpp);
    if (col!=0) {
    	fprintf(file, "\n");
    	col = 0;
    }
    return;
}
