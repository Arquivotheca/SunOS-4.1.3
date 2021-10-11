/* @(#)demolib.h 1.1 92/07/30 SMI */

/* for use with demolib.c and Sun demo programs */

struct	vwsurf	*get_color_surface(/* (struct vwsurf *), argv */);
struct	vwsurf	*get_view_surface(/* (struct vwsurf *), argv */);
struct	vwsurf	*get_surface(/* (struct vwsurf *), argv */);

/* storage allocation for vwsurf to pass into get_surface routines */
static	struct	vwsurf	Core_vwsurf;

/* actual vwsurf handle */
struct	vwsurf	*our_surface = &Core_vwsurf;
