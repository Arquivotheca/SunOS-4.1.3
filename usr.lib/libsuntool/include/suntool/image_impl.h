/*	@(#)image_impl.h 1.1 92/07/30 SMI	*/

/***********************************************************************/
/*	                      image_impl.h			       */
/*          Copyright (c) 1985 by Sun Microsystems, Inc.               */
/***********************************************************************/

#ifndef image_impl_DEFINED
#define image_impl_DEFINED

#include <suntool/image.h>

#define	TRUE	1
#define FALSE	0
#define NULL	0

#define IMAGE_DEFAULT_FONT "/usr/lib/fonts/fixedwidthfonts/screen.b.12"


/***********************************************************************/
/*	        	Structures 				       */
/***********************************************************************/

struct image {

    int			*ops; /* Should be ops vector or unique id */
    struct pixfont	*font;
    char 		*string;
    struct pixrect	*pr;
/*    struct pixrect	*left_pr;	Not yet implemented */
    struct pixrect	*right_pr;
    short		 left_margin;
    short		 right_margin;
    short		 margin;

/* Auxiliary fields */
    short		 height;
    short		 width;
	
/* Flags */
    unsigned		 boxed:1;
    unsigned		 center:1;
    unsigned		 inactive:1;
    unsigned		 invert:1;
    unsigned		 free_everything:1;	/* Not used */
    unsigned		 free_image:1;
    unsigned		 free_font:1;		/* Not used */
    unsigned		 free_string:1;
    unsigned		 free_pr:1;
    unsigned		 free_right_pr:1;	/* Not used */
};


struct pixfont *pw_pfsysopen();
struct pixfont *image_get_font();


#define IMAGE_P(i) ((i) && ((struct image *)(i))->ops == 0)

#define image_attr_next(attr) \
	(Image_attribute *)attr_next((caddr_t *)attr)

#define image_vector(x1,y1,x2,y2) \
	(pr_vector(pr,x1,y1,x2,y2,PIX_SET,0))
#endif ~image_impl_DEFINED
