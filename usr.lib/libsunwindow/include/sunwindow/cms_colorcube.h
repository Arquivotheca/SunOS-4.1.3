/*	@(#)cms_colorcube.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 */

/*
 * Definition of the colormap segment CMS_COLORCUBE.
 * Loads a color cube into locations COLOR_OFFSET through CUBE_SIZE
 * with an RGB color cube containing MAX_REDS red shades, MAX_GREEN green
 * shades, and MAX_BLUE blue shades. 
 *
 * Locations 0-14 are alterating black and white, these may be
 * changed for application specific colors.
 * Locations 15-29 contain a gray scale.  The grey scale is linear.
 * The X/NeWS version leaves gray scale gaps to be filled in by
 * the gray values in the color cube.  The linear version can be used by the
 * GP2 for depth-cueing.
 *
 * The values CUBE_OFFSET==30, MAX_REDS==MAX_BLUES==5, and MAX_GREENS==9
 * are the values expected by the GP2 for use with the GP2 RGB commands.
 *
 * The structure and position of the colormap is modeled after the one
 * planned for X/NeWS.  
 *
 * The CMS_COLORCUBE_SHIFT macros are for use in Sunview 1, when using only
 * the colorcube section of the colormap. The colorcube is installed in 
 * slots 2-227, leaving the other slots free for use by Sunview.  This 
 * minimizes the the colormap flashing as the mouse is moved out of the 
 * colorcube window.  The PWCD_SET_CMAP_SIZE flag must be set to use this
 * colormap. The GP2 expects the normal colorcube unless a 
 * GP2_SET_COLORCUBE_OFFSET command is recieved. That command will make the
 * GP2 expect the values CUBE_OFFSET==2, MAX_REDS==MAX_BLUES==5, and
 * MAX_GREENS==9 for use with GP2 RGB commands 
 */

#include <math.h>
#define	CMS_COLORCUBE		"colorcube"
#define	CMS_COLORCUBESIZE		256
#define CMS_COLORCUBE_SHIFT	"colorcubeshift"
#define	CMS_COLORCUBE_SHIFT_SIZE	227

#define MAX_REDS 5
#define MAX_GREENS 9
#define MAX_BLUES 5
#define CUBE_OFFSET 30
#define CUBESHIFT_OFFSET 2
#define CUBE_SIZE (MAX_REDS*MAX_GREENS*MAX_BLUES)
#define RSCALE 0
#define GSCALE (MAX_BLUES*MAX_REDS)
#define BSCALE MAX_REDS

#define	BLACK	(0 + CUBE_OFFSET)
#define	RED	(0*GSCALE + 0*BSCALE + 4 + CUBE_OFFSET)
#define	YELLOW	(4*GSCALE + 0*BSCALE + 2 + CUBE_OFFSET)
#define ORANGE  (2*GSCALE + 0*BSCALE + 3 + CUBE_OFFSET)
#define	GREEN	(8*GSCALE + 0*BSCALE + 0 + CUBE_OFFSET)
#define	CYAN	(4*GSCALE + 2*BSCALE + 0 + CUBE_OFFSET)
#define	BLUE	(0*GSCALE + 4*BSCALE + 0 + CUBE_OFFSET)
#define	MAGENTA	(0*GSCALE + 2*BSCALE + 2 + CUBE_OFFSET)
#define	WHITE	(8*GSCALE + 4*BSCALE + 4 + CUBE_OFFSET)

#define GRAY_OFFSET	(15)
#define GRAY_MAX	(29)
#define GRAY_SCALE	(GRAY_MAX - GRAY_OFFSET)
#define	GRAY(i)	(GRAY_SCALE*(i)/100 + GRAY_OFFSET)

#define	cms_colorcubesetup(r,g,b) \
{ int i, ri, gi, bi;\
 for (i=0; i<15; i +=2){\
  (r)[i]=(g)[i]=(b)[i]=255; (r)[i+1]=(g)[i+1]=(b)[i+1]=0;}\
 for (i=GRAY_OFFSET; i<=GRAY_MAX; i++){\
  (r)[i]=(g)[i]=(b)[i]=255*(i-GRAY_OFFSET)/GRAY_SCALE;}\
 i=CUBE_OFFSET;\
 for (gi=0; gi<MAX_GREENS; gi++)\
  for (bi=0; bi<MAX_BLUES; bi++)\
   for (ri=0; ri<MAX_REDS; ri++){\
    (r)[i] = ri*255/4; (g)[i] = gi*255/8; (b)[i] = bi*255/4; i++; }\
 (r)[255]=(g)[255]=(b)[255]=0; \
}
#define	cms_colorcubeshiftsetup(r,g,b) \
{ int i, ri, gi, bi;\
 (r)[0]=(g)[0]=(b)[0]=255; \
 (r)[1]=(g)[1]=(b)[1]=0; \
 i = CUBESHIFT_OFFSET; \
 for (gi=0; gi<MAX_GREENS; gi++)\
  for (bi=0; bi<MAX_BLUES; bi++)\
   for (ri=0; ri<MAX_REDS; ri++){\
    (r)[i] = ri*255/4; (g)[i] = gi*255/8; (b)[i] = bi*255/4; i++; }\
}

#define	cms_colorcubesetup_gamma(r,g,b,gamma) \
{ int i, ri, gi, bi; float ga = 1.0/gamma;\
 for (i=0; i<15; i +=2){\
  (r)[i]=(g)[i]=(b)[i]=255; (r)[i+1]=(g)[i+1]=(b)[i+1]=0;}\
 for (i=GRAY_OFFSET; i<=GRAY_MAX; i++){\
  (r)[i]=(g)[i]=(b)[i]=255*(i-GRAY_OFFSET)/GRAY_SCALE;}\
 i=CUBE_OFFSET;\
 for (gi=0; gi<MAX_GREENS; gi++)\
  for (bi=0; bi<MAX_BLUES; bi++)\
   for (ri=0; ri<MAX_REDS; ri++){\
    (r)[i] = pow(((float) ri / 4.0),ga) * 255;\
    (g)[i] = pow(((float) gi / 8.0),ga) * 255;\
    (b)[i] = pow(((float) bi / 4.0),ga) * 255;\
    i++; }\
 (r)[255]=(g)[255]=(b)[255]=0; \
}
#define	cms_colorcubeshiftsetup_gamma(r,g,b,gamma) \
{ int i, ri, gi, bi; float ga = 1.0/gamma;\
 (r)[0]=(g)[0]=(b)[0]=255; \
 (r)[1]=(g)[1]=(b)[1]=0; \
 i = CUBESHIFT_OFFSET; \
 for (gi=0; gi<MAX_GREENS; gi++)\
  for (bi=0; bi<MAX_BLUES; bi++)\
   for (ri=0; ri<MAX_REDS; ri++){\
    (r)[i] = pow(((float) ri / 4.0),ga) * 255;\
    (g)[i] = pow(((float) gi / 8.0),ga) * 255;\
    (b)[i] = pow(((float) bi / 4.0),ga) * 255;\
    i++; }\
 (r)[255]=(g)[255]=(b)[255]=0; \
}


