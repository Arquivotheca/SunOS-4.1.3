#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)image.c 1.1 92/07/30 Copyright 1985 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*-
	IMAGE PACKAGE

	image.c, Fri Jul 12 12:11:18 1985

		Craig Taylor,
		Sun Microsystems
 */
		    
/* 
 * Image layout:
 * 
 *	Image width = M, Lm, LPr, Pr, Rpr, Rm, M
 *	Image height = M, max(LPr, Pr, Rpr), M
 * 
 * M = Margin, 
 * Lm = Left Margin,
 * LPr = Left Pixrect,
 * Pr = Pixrect or String,
 * Rpr = Right Pixrect,
 * Rm = Right Margin,
 * 
 */

#include <sys/types.h>
#include <stdio.h>
#include <varargs.h>

#include <pixrect/pixrect.h>
#include <pixrect/pixfont.h>

#include <sunwindow/sun.h> /* for LINT_CAST */
#include "sunwindow/sv_malloc.h"

#include <suntool/image_impl.h>

extern struct pixrect  menu_gray50_pr;
static struct pixfont *image_font;	/* Default to NULL */

#define IMAX(a, b) ((int)(b) > (int)(a) ? (int)(b) : (int)(a))
#define INHERIT_VALUE(f) im->f ? im->f : std_image ? std_image->f : 0


/* VARARGS */
Image
image_create(va_alist)
	va_dcl
{   
    caddr_t avlist[ATTR_STANDARD_SIZE];

    struct image *im = (struct image *)LINT_CAST(sv_calloc(
    	1, sizeof(struct image)));
    va_list valist;
    
    va_start(valist);
    (void) attr_make(avlist, ATTR_STANDARD_SIZE, valist);
    va_end(valist);
    (void) image_set(im, ATTR_LIST, avlist, 0);

    return (Image)im;
}


/* VARARGS1 */
int
image_set(im, va_alist)    
	register struct image *im;
	va_dcl
{   
    Image_attribute avlist[ATTR_STANDARD_SIZE];
    va_list valist;
    register Image_attribute *attr;

    if (!im) return FALSE;

    va_start(valist);
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
    va_end(valist);

    /* zero width/height if any size changes */
    for (attr = avlist; *attr; attr = image_attr_next(attr)) {
	switch (attr[0]) {

	  case IMAGE_ACTIVE:
	    im->inactive = FALSE;
	    break;
	    
	  case IMAGE_BOXED:
	    im->boxed = (int)attr[1];
	    break;
	    
	  case IMAGE_CENTER:
	    im->center = (int)attr[1];
	    break;
	    
          case IMAGE_FONT:
	    im->font = (struct pixfont *)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_INACTIVE:
	    im->inactive = TRUE;
	    break;
	    
	  case IMAGE_INVERT:
	    im->invert = (int)attr[1];
	    break;
	    
	  case IMAGE_LEFT_MARGIN:
	    im->left_margin = (int)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_MARGIN:
	    im->margin = (int)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_PIXRECT:
	    if (im->free_pr && im->pr) (void) pr_destroy(im->pr);
	    im->pr = (struct pixrect *)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_RELEASE:
	    im->free_image = TRUE;
	    break;
	    
	  case IMAGE_RELEASE_STRING:
	    im->free_string = TRUE;
	    break;
	    
	  case IMAGE_RELEASE_PR:
	    im->free_pr = TRUE;
	    break;
	    
	  case IMAGE_RIGHT_MARGIN:
	    im->right_margin = (int)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_RIGHT_PIXRECT:
	    im->right_pr = (struct pixrect *)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_STRING:
	    if (im->free_string && im->string) free(im->string);
	    im->string = (char *)attr[1];
	    im->width = im->height = 0;
	    break;
	    
	  case IMAGE_NOP:
	    break;

	  default:
	    if (ATTR_PKG_IMAGE == ATTR_PKG(attr[0]))
		(void) fprintf(stderr,
	"image_set: 0x%x not recognized as an image attribute.\n", attr[0]);
	    break;

	}	
    }

    return TRUE;
}


/* VARARGS2 */
void
image_render(im, std_image, va_alist)
	struct image *im;
	register struct image *std_image;
	va_dcl
{
    register Image_attribute *attr;
    Image_attribute avlist[ATTR_STANDARD_SIZE];
    struct pixrect *pr;
    int top, left, mid, inactive = FALSE;
    register int margin2;
    int margin, left_margin, right_margin, width, height;
    va_list valist;
    struct pr_prpos where;

    if (!im) return;

    va_start(valist);
    (void) attr_make((char **)LINT_CAST(avlist), ATTR_STANDARD_SIZE, valist);
    va_end(valist);
    for (attr = avlist; *attr; attr = image_attr_next(attr)) {
	switch (attr[0]) {

	  case IMAGE_REGION:
	    pr = (struct pixrect *)attr[1];
	    left = (int)attr[2];
	    top = (int)attr[3];
	    break;
	    
	  case IMAGE_INACTIVE:
	    inactive = TRUE;
	    break;

	  case IMAGE_NOP:
	    break;

	  default:
	    if (ATTR_PKG_IMAGE == ATTR_PKG(attr[0]))
		(void) fprintf(stderr,
	"image_render: 0x%x not recognized as a image attribute.\n", attr[0]);
	    break;
	}
    }
    margin = INHERIT_VALUE(margin);
    left_margin = INHERIT_VALUE(left_margin);
    right_margin = INHERIT_VALUE(right_margin);
    width = IMAX(im->width, std_image ? std_image->width : 0);
    height = IMAX(im->height, std_image ? std_image->height : 0);
    margin2 = margin << 1;

    mid = top + height / 2;
    left += margin, top += margin;

    if (im->string) {
	struct pixfont *font = image_get_font(im, std_image);
	
	if (*im->string == '\0') return;
	if (INHERIT_VALUE(center)) {
	    struct pr_size  temp;
	    int pad;
	    
	    temp = pf_textwidth(strlen(im->string), font, im->string);
	    pad = width - margin2 - temp.x;
	    left_margin = pad -
		IMAX(pad/2, im->right_pr ?
		       im->right_pr->pr_width + right_margin + 1 : 0);
	}
	where.pr = pr;
	where.pos.x = left+left_margin - font->pf_char[im->string[0]].pc_home.x,
	where.pos.y = mid - font->pf_defaultsize.y / 2 -
	    font->pf_char[im->string[0]].pc_home.y,
	(void) pf_text(where, PIX_SRC, font, im->string);
	if (im->inactive || inactive) {
	    where.pos.x++;
	    (void) pf_text(where, PIX_SRC, font, im->string);
	    (void) pr_replrop(pr, left, top,
		       	      width - margin2, height - margin2, 
		       	      PIX_SRC & PIX_DST, &menu_gray50_pr, 0, 0);
	}
    } else if (im->pr) {
	(void) pr_rop(pr, left, top,
	       	      width - margin2, height - margin2, 
	       	      PIX_SRC, im->pr, 0, 0);
	/* Fixme: Add inactive mask for pixrects */
    }
    
    if (im->right_pr) {
	(void) pr_rop(pr,
	       	      left + width - (im->right_pr->pr_width
		    	      + (im->string ? right_margin:0) + margin2),
	       	      mid - im->right_pr->pr_height / 2, 
	       	      im->right_pr->pr_width, im->right_pr->pr_height,
	       	      PIX_SRC, im->right_pr, 0, 0);
    }
    if (im->invert) {
	(void) pr_rop(pr,
	       	      left, top, width - margin2, height - margin2,  
	       	      PIX_NOT(PIX_DST), im->pr, 0, 0);
    }	
    if (im->boxed || (std_image ? std_image->boxed : FALSE)) {
	int x1 = left - 1;
	int x2 = left + width - margin2;
	int y1 = top - 1;
	int y2 = top + height - margin2;

	(void) pr_vector(pr, x1, y1, x2, y1, PIX_SET, 0);
	(void) pr_vector(pr, x2, y1, x2, y2, PIX_SET, 0);
	(void) pr_vector(pr, x2, y2, x1, y2, PIX_SET, 0);
	(void) pr_vector(pr, x1, y2, x1, y1, PIX_SET, 0);
    }
}


/* VARARGS2 */
caddr_t
image_get(im, std_image, attr)    
	struct image *im, *std_image;
	Image_attribute attr;
{   
    caddr_t v = NULL;

    if (!im) return NULL;
    switch (attr) {

      case IMAGE_FONT:
	v = (caddr_t)image_get_font(im, std_image);
	break;
	
      case IMAGE_HEIGHT:
	if (!im->height) compute_size(im, std_image);
	v = (caddr_t)im->height;
	break;
	
      case IMAGE_PIXRECT:
	v = (caddr_t)im->pr;
	break;
	
      case IMAGE_STRING:
	v = (caddr_t)im->string;
	break;
	
      case IMAGE_WIDTH:
	if (!im->width) compute_size(im, std_image);
	v = (caddr_t)im->width;
	break;

    }
    return v;
}


void
image_destroy(im)
	struct image *im;
{   
    if (!im || !im->free_image) return;
    if (im->free_string && im->string) free(im->string);
    if (im->free_pr && im->pr) (void) pr_destroy(im->pr);
    free((caddr_t)im);
}


static
compute_size(im, std_image)
	register struct image *im, *std_image;
{   
    struct pixfont *font;
    register int margin2;
    int margin, left_margin, right_margin;

    struct pr_size  temp;
    
    margin = INHERIT_VALUE(margin);
    left_margin = INHERIT_VALUE(left_margin);
    right_margin = INHERIT_VALUE(right_margin);
    margin2 = margin << 1;
    
    if (im->pr) {
	im->width = margin2 + im->pr->pr_width;
	im->height = margin2 + im->pr->pr_height;
	if (im->right_pr) {
	    im->width += im->right_pr->pr_width;
	    im->height = IMAX(im->right_pr->pr_height, im->height);
	}
    } else if (im->string) {
	font = image_get_font(im, std_image);

	temp = pf_textwidth(strlen(im->string), font, im->string);
	im->width = temp.x;
	im->height = font->pf_defaultsize.y;
	
	if (im->right_pr) {
	    im->width += im->right_pr->pr_width;
	    im->height = IMAX(im->right_pr->pr_height, im->height);
	}
	
	im->width += margin2 + left_margin + right_margin;
	im->height += margin2;

    } else {
	im->height = im->width = 0;
    }
    im->width = IMAX(im->width, std_image->width);
    im->height = IMAX(im->height, std_image->height);
}


static
struct pixfont *
image_get_font(im, std_image)
	struct image *im;
	register struct image *std_image;
{   
    struct pixfont *font;

    if (im->font) font = im->font;
    else if (std_image && std_image->font) font = std_image->font;
    else if (image_font) font = image_font;
    else if (image_font = pf_open(IMAGE_DEFAULT_FONT))
	font = image_font;
    else font = image_font = pw_pfsysopen();
    return font;
}
