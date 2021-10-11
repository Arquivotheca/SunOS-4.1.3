
#ifndef lint
static char sccsid[] = "@(#)fileio.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <pixrect/pixrect_hs.h>
#include <errno.h>
#include "video_mat.h"

extern int y_offset;

load_video_image(dst_pixrect, filename)
    struct pixrect *dst_pixrect;
    char *filename;
{
    struct pixrect *src_pixrect;
    FILE           *file;
    colormap_t     colormap;
    struct pr_pos   src, dst;
    int             width, height;

    if (!(file = fopen(filename, "r"))) {
	perror("open failed:");
	return (errno);
    }
    src_pixrect = pr_load(file, &colormap);
    fclose(file);
    if (!src_pixrect) {
	file_error("Error reading rasterfile data.");
	return;
    }
    src.x = src.y = dst.x = dst.y = 0;
    width = src_pixrect->pr_width;
    height = src_pixrect->pr_height;
    if (src_pixrect->pr_height > dst_pixrect->pr_height) {
	src.y = (src_pixrect->pr_height - dst_pixrect->pr_height) / 2;
	height = dst_pixrect->pr_height;
    } else if (src_pixrect->pr_height <= 
    		(dst_pixrect->pr_height-y_offset)) {
    	/* 
    	 * This offsets the image so that it will show all of it 
    	 */
    	dst.y = y_offset + (dst_pixrect->pr_height - 
    		(src_pixrect->pr_height+y_offset)) / 2;
    	height = dst.y + src_pixrect->pr_height;
	/* clear the top */
	pr_rop(dst_pixrect, 0, 0, dst_pixrect->pr_width, dst.y, 
    		PIX_CLR, NULL, 0, 0);
	/* now bottom */
	pr_rop(dst_pixrect, 0, height, dst_pixrect->pr_width, 
    		(dst_pixrect->pr_height - height), PIX_CLR, NULL, 0, 0);
    } else if( (src_pixrect->pr_height > dst_pixrect->pr_height) &&
    	(src_pixrect->pr_height > (dst_pixrect->pr_height-y_offset) )) {
    	dst.y = dst_pixrect->pr_height - src_pixrect->pr_height;
    	height = dst.y + src_pixrect->pr_height;
    }

    if (src_pixrect->pr_width > dst_pixrect->pr_width) {
	src.x = (src_pixrect->pr_width - dst_pixrect->pr_width) / 2;
	width = dst_pixrect->pr_width;
    } else if (src_pixrect->pr_width < dst_pixrect->pr_width) {
	dst.x = (dst_pixrect->pr_width - src_pixrect->pr_width) / 2;
	width = dst.x + src_pixrect->pr_width;
	/* clear left side  */
	pr_rop(dst_pixrect, 0, dst.y, dst.x,
    		src_pixrect->pr_height, PIX_CLR, NULL, 0, 0);
	/* and the right */
	pr_rop(dst_pixrect, width , dst.y,
	       dst_pixrect->pr_width - width,
	       src_pixrect->pr_height, PIX_CLR, NULL, 0, 0);
    }

    if (src_pixrect->pr_depth == 8) {
	/* we need to convert it to 24 */
	register int    x, y, sx, sy, i;

	if ( (colormap.type != RT_STANDARD) || (colormap.length > 256)) {
	    file_error("Sorry, this type of colormap is not supported.");
	    pr_destroy(src_pixrect);
	    return;
	}
	for (y = dst.y, sy = src.y; y < height; y++, sy++) {
	    for (x = dst.x, sx = src.x; x < width; x++, sx++) {
		i = pr_get(src_pixrect, sx, sy);
		pr_put(dst_pixrect, x, y,
		       colormap.map[0][i] |
		       colormap.map[1][i] << 8 |
		       colormap.map[2][i] << 16
		    );
	    }
	}
    } else {
	pr_rop(dst_pixrect, dst.x, dst.y, width, height, PIX_SRC,
	       src_pixrect, src.x, src.y);
    }
    pr_destroy(src_pixrect);
    return (0);

}


save_video_image(src_pixrect, filename, value)
    struct pixrect *src_pixrect;
    char *filename;
    int value;
{
    register int x, y;
    int lbytes, linewords, bpp;
    int method;
    unsigned int *im_data;
    u_char *pr_out_data, r, g, b;
    u_char greyramp[256], luma, oval, dval;
    FILE *file;
    colormap_t cmap;
    Pixrect *pr_out;
    void reduce();

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

    switch(value) {
    case 0 : 	/* 32 bits full frame buffer dump */
    	cmap.type = RMT_NONE;
    	cmap.length = 0;
    	pr_dump(src_pixrect, file, &cmap, RT_STANDARD, 0);
    	break;
    case 1 :	/* 8 bit colour image */
    	cmap.type = RMT_EQUAL_RGB;
    	cmap.map[0] = red_cmap;
    	cmap.map[1] = green_cmap;
    	cmap.map[2] = blue_cmap;

    	cmap.length = CMAP_SIZE;
    	pr_out = mem_create(src_pixrect->pr_width, 
    		(src_pixrect->pr_height-y_offset), 8);
	linewords = (mpr_d(src_pixrect)->md_linebytes)/4;
	im_data = (unsigned int *)mpr_d(src_pixrect)->md_image + 
    		(y_offset*linewords);
    	lbytes = (mpr_d(pr_out)->md_linebytes);
    	pr_out_data = (u_char *)(mpr_d(pr_out)->md_image);
	for(y=y_offset; y<src_pixrect->pr_height; y++) {
    		reduce(im_data,	pr_out_data);
    		im_data += linewords;
    		pr_out_data += lbytes;
    	}
    	pr_dump(pr_out, file, &cmap, RT_STANDARD, 0);
    	pr_destroy(pr_out);
    	break;
    case 2 :	/* 8 bit greyscale image */
    case 3 :	/* 1 bit dithered image */
    	if(value == 2)
    		bpp = 8;
    	else
    		bpp = 1;
    	cmap.type = RMT_EQUAL_RGB;
    	cmap.length = 256;
   		cmap.map[0] = cmap.map[1] = cmap.map[2] = greyramp ;
    	for(x=0; x<256; x++)
    		greyramp[x] = x;

    	pr_out = mem_create(src_pixrect->pr_width, 
    		(src_pixrect->pr_height-y_offset), bpp);
	linewords = (mpr_d(src_pixrect)->md_linebytes)/4;
	im_data = (unsigned int *)mpr_d(src_pixrect)->md_image + 
    		(y_offset*linewords);
    	lbytes = (mpr_d(pr_out)->md_linebytes);
    	pr_out_data = (u_char *)(mpr_d(pr_out)->md_image);
	for(y=y_offset; y<src_pixrect->pr_height; y++) {
		for(x=0; x<src_pixrect->pr_width; x++) {
			b = (u_char)(BLUE(im_data[x]));
			g = (u_char)(GREEN(im_data[x]));
			r = (u_char)(RED(im_data[x]));
			luma = (lumar[r] + lumag[g] + lumab[b])/256;
			if(bpp == 1) {
				dval = dither_mat[y%16][x%16];
				oval = ~(luma <= dval);
				pr_put(pr_out, x, (y-y_offset), oval);
			} else if (bpp == 8) {
				pr_out_data[x] = luma;
			}
		}
		im_data += linewords;
    		pr_out_data += lbytes;
    	}
    	pr_dump(pr_out, file, &cmap, RT_STANDARD, 0);
    	pr_destroy(pr_out);
    	break;
    default :	/* Shouldn't happen */
    	fprintf(stderr,"No such file format\n");
   	break;
    }
    fclose(file);
}

/*
 * This function reduces 24 bit data to 8 bits 
 * It uses the colormap information in the header file 
 */
void
reduce(im_data, color)
register int	*im_data;
register u_char *color;
{
	register int	error, i, low_error, high_error;
	u_char	red, green, blue;
	u_char	rval, gval, bval, rdiff, gdiff, bdiff;
	static short	red_error[MAX_SCAN_WIDTH+2],
	green_error[MAX_SCAN_WIDTH+2],
	blue_error[MAX_SCAN_WIDTH+2];
	short	red_carry = 0, green_carry = 0, blue_carry = 0;
	static int rmult = 36, gmult = 6, bmult = 1;

	/* error correcting dither (Floyd-Steinberg)*/
	for (i = 1; i <= MAX_SCAN_WIDTH; i++) {
		/* these are the "low" values */
		red = (u_char)(RED(im_data[i]));
		green = (u_char)(GREEN(im_data[i]));
		blue = (u_char)(BLUE(im_data[i]));
		rval = INDEX(RED_I, red);
		gval = INDEX(GREEN_I, green);
		bval = INDEX(BLUE_I, blue);
		low_error = (VALUE(RED_I, rval) - red) * 16; /*  <= 0 */
		high_error = ((VALUE(RED_I, rval + 1)) - red) * 16; /*  >= 0 */
		error = red_error[i];
		if (ABS(error + low_error) > ABS(error + high_error)) {
			/* choosing the next entry will result in  */
			/* less accumulated error */
			rval++;
		}
		/* error terms are 16X */
		/* note that some error will be lost due to rounding */
		/* (at most 1/32nd) which could be avoided */
		error =  VALUE(RED_I, rval) - red + ((error + 8) >> 4);
		red_error[i-1] += (3 * error); /* 3/16 down, left */
		red_error[i] = (5 * error) + red_carry; /* 5/16 down */
		red_error[i+1] += (7 * error); /* 7/16 right */
		red_carry = error;	    /* 1/16 down, right */

		low_error = (VALUE(GREEN_I, gval) - green) * 16;  /*  <= 0 */
		high_error = (VALUE(GREEN_I, gval + 1) - green) * 16; /* >= 0  */
		error = green_error[i];
		if (ABS(error + low_error) > ABS(error + high_error)) {
			gval++;
		}
		error =  VALUE(GREEN_I, gval) - green + ((error + 8) >> 4);
		green_error[i-1] += 3 * error; /* down, left */
		green_error[i] = 5 * error + green_carry; /* down */
		green_error[i+1] += 7 * error; /* right */
		green_carry = error;   /* down right */

		low_error = (VALUE(BLUE_I, bval) - blue) * 16;  /* negative */
		high_error = (VALUE(BLUE_I, bval + 1) - blue) * 16; /*  positve  */
		error = blue_error[i];
		if (ABS(error + low_error) > ABS(error + high_error)) {
			bval++;
		}
		error =  VALUE(BLUE_I, bval) - blue + ((error + 8) >> 4);
		blue_error[i-1] += 3 * error; /* down, left */
		blue_error[i] = 5 * error + blue_carry; /* down */
		blue_error[i+1] += 7 * error; /* right */
		blue_carry = error;   /* down right */

		*color = (rmult * rval) + (gmult * gval) + bval;
		color++;
	}
}
