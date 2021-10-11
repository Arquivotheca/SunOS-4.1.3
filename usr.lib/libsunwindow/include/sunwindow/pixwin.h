/*	@(#)pixwin.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#ifndef pixwin_DEFINED
#define	pixwin_DEFINED

#include <sunwindow/attr.h>

/*
 * Raster device independent access of windows can be done using the
 * following structures and functions.
 *
 * A pixwin is a data structure used for window image access.
 * It is the data structure that is used to multiplex a pixrect
 * in an overlapping window environment.
 *
 * This is how the pixwin_clipops are to be used:
 *
 * Pwo_getclipping gets pw_opsdata->pw_clipid,
 * frees all rectlists in pixwin, gets the right pw_clipping.
 * Pw_prretained is set to describe the rectangle of the window.
 * The rectlist is examined and the following actions are taken depending
 * on the case:
 *	Rectlist is composed of a single rect:
 *		A secondary pixrect is generated from pw_pixrect that describes
 *		the rect.  This new pixrect is pwcd_prclipping.
 *		pwcd_state is set to PWCD_SINGLERECT.
 *	Rectlist is composed of multiple rects:
 *		A secondary pixrect is generated from pw_pixrect that describes
 *		the rect of the window.  This new pixrect is pwcd_prclipping.
 *		pwcd_state is set to PWCD_MULTIRECTS.
 *	Rectlist is null:
 *		pwcd_state is set to PWCD_NULL.
 *
 * Pwo_lock compares an id gotten from the kernel with pw_opsdata->pw_clipid
 * and calls pwo_getclipping if the two ids are not equal.
 * This lock call adjusts pw_ops and pw_opshandle depending on pwcd_state:
 *	PWCD_SINGLERECT:
 *		pw_ops = pw_pixrect->pr_ops.
 *		pw_opshandle = cached secondary pixrect.
 *		This avoids rectlist clipping if do other operations before
 *		unlocking.
 *	PWCD_MULTIRECTS:
 *		pw_ops = standard pw_ops interpretations.
 *		pw_opshandle = pixwin.
 *		These procedures handle clipping to a rectlist.
 *	PWCD_NULL:
 *		pw_ops = standard minimum pw_ops interpretations.
 *		pw_opshandle = pixwin.
 *		No writing takes place except to pw_prretained.
 *	PWCD_USERDEFINE:
 *		Assumes that the user is handling the op and handle selection
 *		so does nothing.
 * If pw_lockcount is > 0 only increment this count.
 *
 * Pwo_unlock releases the display access lock.
 * Pw_ops and pw_opshandle are reset to PWCD_MULTIRECTS values to ensure
 * that the next imaging call involves a pwo_lock. 
 * If pw_lockcount is > 1 only decrement this count.
 *
 * Pwo_reset forces the release of the display access lock, frees
 * all rectlists in pixwin and resets all data to a base state.
 *
 * Here is how the basic utilities work:
 *
 * Pw_open creates a new pixwin.  Pw_close should be called when you are
 * done with the pixwin.
 *
 * Calling pw_exposed or pw_damaged sets pwo_getclipping to either
 * pw_exposed or pw_damaged, respectively, and gets the related clipping.
 *
 * Pw_damaged also sets pw_opsdata->pw_damagedid.  This is the value used
 * in the pw_donedamaged call to notify the kernel that a certain version
 * of display damage has been fixed up.  Note, that this may not be equal
 * to the pw_opsdata->pw_clipping being used when pw_donedamaged is called.
 * In this case some multiple repainting of the same area may occur but this
 * should happen infrequently.
 */

typedef	struct	pixwin {
	struct	pixrectops *pw_ops;	/* imaging ops appropriate to this pw */
	caddr_t	pw_opshandle;		/* handle used during pw_ops calls */
	int	pw_opsx;		/* left offset used during pw_ops call*/
	int	pw_opsy;		/* top offset used during pw_ops call*/
	struct	rectlist pw_fixup;	/* undefined window image after
					 * doing pw_op in which window
					 * was the source */
	struct	pixrect *pw_pixrect;	/* device containing this window */
	struct	pixrect *pw_prretained;	/* retained pixrect of window */
	struct	pixwin_clipops *pw_clipops;/* locking and clipping ops */
	struct	pixwin_clipdata *pw_clipdata;/* locking and clipping data */
	char	pw_cmsname[20];		/* name of colormap segment (cms.h) */
} Pixwin;

#define PIXWIN_NULL     ((Pixwin *)0)
#define pw_windowfd     pw_clipdata->pwcd_windowfd

/*
 * This structure contains clipping and locking operations.
 */
struct	pixwin_clipops {
	int	(*pwco_lock)();		/* lock against window manager */
	int	(*pwco_unlock)();	/* release */
	int	(*pwco_reset)();	/* clear lock & clipping to base state,
					 * can be used on non-local goto */
	int	(*pwco_getclipping)();	/* get clipping */
};

/* This type defines the arguments to pw_batch */
typedef	enum pw_batch_type {
	PW_NONE=0x7FFFFFFF,	/* Stop batching & refresh screen (if changed) */
	PW_ALL=0x7FFFFFFE,	/* Start batching all operations */
	PW_SHOW=0x7FFFFFFD,	/* Refresh screen (if changed) &
				 * reset op count */
#define	PW_OP_COUNT(n)	((Pw_batch_type)(n))
				/* Number of pixwin "operations" before do
				 * PW_SHOW (silently) */
} Pw_batch_type;
#define	pw_batch_on(pw)	pw_batch((pw), PW_ALL)
#define	pw_show(pw)	pw_batch((pw), PW_SHOW)
#define	pw_batch_off(pw) pw_batch((pw), PW_NONE)


/*
 * This structure contains clipping and locking data.
 */
struct	pixwin_clipdata {
	int	pwcd_windowfd;
	short	pwcd_state;
#define	PWCD_NULL	0
#define	PWCD_MULTIRECTS	1
#define	PWCD_SINGLERECT	2
#define	PWCD_USERDEFINE	3
	struct	rectlist pwcd_clipping;	/* current clipping */
	int	pwcd_clipid;
	int	pwcd_damagedid;
	int	pwcd_lockcount;
	struct	pixrect *pwcd_prmulti;	/* pixrect for multi rect clipping */
	struct	pixrect *pwcd_prsingle;	/* pixrect for single rect clipping */
	struct	pixwin_prlist *pwcd_prl; /* pixrect list for clipping vectors*/
	struct	rectlist pwcd_clippingsorted[RECTS_SORTS];
					/* cached current clipping sorted
					 * to facilitate pw_copy */
	struct	rect *pwcd_regionrect;	/* If !0 intersect with clipping */
	int	pwcd_x_offset;		/* X offset of pixwin from origin,
					 * retained image always at (0,0) */
	int	pwcd_y_offset;		/* Y offset of pixwin from origin
					 * retained image always at (0,0) */
	int	pwcd_flags;		/* various flags */
#define	PWCD_CURSOR_INVERTED	0x01	/* pixwin is inverted rel. to kernel */
#define PWCD_DBLACCESSED	0x02	/* pixwin has accessed double buffering */
#define PWCD_DBL_AVAIL		0x04	/* Is double buffering available ? */
#define PWCD_IGNORE_DBLSET	0x08
#define PWCD_COLOR24		0x10	/* Has 24 bit (in 32) true color. */
#define PWCD_VIDEO		0x20	/* Uses video */
/* force correct plane mask setting when colormap size is not a power of 2 */ 
#define PWCD_SET_CMAP_SIZE	0x40
#define PWCD_WID_DBL		0x80	/* window id double buffering */
#define PWCD_COPY_ON_DBL_RELEASE	0x100
#define PWCD_REQUEST_RENDER_CLIP	0x200
#define PWCD_HAVE_RENDER_CLIP		0x400
#define PWCD_WID_ALLOCATED		0x800

	caddr_t	pwcd_client;		/* Unused */
	enum	pw_batch_type pwcd_batch_type;
					/* If != PW_NONE then batching */
	int	pwcd_op_count;		/* Update during batching with number
					 * of operations done so far */
	int	pwcd_op_limit;		/* Number of operations to do before
					 * a pw_batch(pw, PW_SHOW) */
	struct	rect pwcd_batchrect;	/* Update during batching with changed
					 * area */
	int		pwcd_winnum;	/* window number to check clip id */
	struct win_lock_block	*pwcd_wl;	/* shared locking info */
	int	pwcd_screen_x;		/* screen relative left of window */
	int	pwcd_screen_y;		/* screen relative top of window */
	char	pwcd_plane_group;	/* cached version of pw_pixrect's
					 * plane group setting */
#define	PIX_MAX_PLANE_GROUPS	20	/* 20 is arbitrary */
	char	pwcd_plane_groups_available[PIX_MAX_PLANE_GROUPS];
					/* frame buffer capabilities */
};

/*
 * Vector clipping must be done at the lowest level in order to maintain
 * stepping integrity across abutting rectangle boundaries.
 * Thus, when a vector is to be clipped to multiple rects the clipper is
 * a pixrect.  The pixwin_prlist data structure is used to hold a
 * pixrect clipping list.
 */
struct	pixwin_prlist {
	struct	pixwin_prlist *prl_next;	/* Next record */
	struct	pixrect *prl_pixrect;		/* clipping pixrect */
	int	prl_x, prl_y;			/* offset of prl_pixrect from
						   window origin */
};
extern	struct	pixrectops *pw_opsstd_ptr;

#ifndef lint

#define pw_rop(dpw, dx, dy, w, h, op, sp, sx, sy)			\
	(*(dpw)->pw_ops->pro_rop)((dpw)->pw_opshandle,			\
	    (dx)-(dpw)->pw_opsx, (dy)-(dpw)->pw_opsy, (w), (h), (op),	\
	    (sp), (sx), (sy))
#define pw_write(dpw, dx, dy, w, h, op, spr, sx, sy)			\
	(*(dpw)->pw_ops->pro_rop)((dpw)->pw_opshandle,			\
	    (dx)-(dpw)->pw_opsx, (dy)-(dpw)->pw_opsy, (w), (h), (op),	\
	    (spr), (sx), (sy))
#define pw_writebackground(dpw, dx, dy, w, h, op)			\
	(*(dpw)->pw_ops->pro_rop)((dpw)->pw_opshandle,			\
	    (dx)-(dpw)->pw_opsx, (dy)-(dpw)->pw_opsy, (w), (h), (op),	\
	    (struct pixrect *)0, 0, 0)
#define pw_read(dpr, dx, dy, w, h, op, spw, sx, sy)			\
	(*pw_opsstd_ptr->pro_rop)((dpr), (dx), (dy), (w), (h),		\
	    (op), (spw), (sx)-(spw)->pw_clipdata->pwcd_x_offset,	\
	    (sy)-(spw)->pw_clipdata->pwcd_y_offset)
#define pw_copy(dpw, dx, dy, w, h, op, spw, sx, sy)			\
	(*pw_opsstd_ptr->pro_rop)((dpw),				\
	    (dx)-(dpw)->pw_clipdata->pwcd_x_offset,			\
	    (dy)-(dpw)->pw_clipdata->pwcd_y_offset, (w), (h), (op),	\
	    (spw), (sx)-(spw)->pw_clipdata->pwcd_x_offset,		\
	    (sy)-(spw)->pw_clipdata->pwcd_y_offset)
#define pw_batchrop(dpw, x, y, op, sbp, n)				\
	(*(dpw)->pw_ops->pro_batchrop)((dpw)->pw_opshandle,		\
	    (x)-(dpw)->pw_opsx, (y)-(dpw)->pw_opsy, (op), (sbp), (n))
#define pw_stencil(dpw, x, y, w, h, op, stpr, stx, sty, spr, sy, sx)	\
	(*(dpw)->pw_ops->pro_stencil)((dpw)->pw_opshandle,              \
	    x-(dpw)->pw_opsx, y-(dpw)->pw_opsy, (w), (h), (op),		\
	    (stpr), (stx), (sty), (spr), (sy), (sx))
#define pw_destroy(pw)							\
	(*pw_opsstd_ptr->pro_destroy)((pw))
#define	pw_get(pw, x, y)						\
	(*pw_opsstd_ptr->pro_get)((pw),					\
	    (x)-(pw)->pw_clipdata->pwcd_x_offset,			\
	    (y)-(pw)->pw_clipdata->pwcd_y_offset)
#define	pw_put(pw, x, y, val)						\
	(*(pw)->pw_ops->pro_put)((pw)->pw_opshandle,			\
	    (x)-(pw)->pw_opsx, (y)-(pw)->pw_opsy, (val))
#define pw_vector(pw, x0, y0, x1, y1, op, val)				\
	(*(pw)->pw_ops->pro_vector)((pw)->pw_opshandle,			\
	    (x0)-(pw)->pw_opsx, (y0)-(pw)->pw_opsy,			\
	    (x1)-(pw)->pw_opsx, (y1)-(pw)->pw_opsy, (op), (val))
#define	pw_region(pw, x, y, w, h)					\
	(struct pixwin *)(*pw_opsstd_ptr->pro_region)((pw),		\
	    (x), (y), (w), (h))
#define	pw_putattributes(pw, planes)					\
	(*pw_opsstd_ptr->pro_putattributes)((pw), (planes))
#define	pw_getattributes(pw, planes)					\
	(*pw_opsstd_ptr->pro_getattributes)((pw), (planes))
#define	pw_putcolormap(pw, index, count, red, green, blue)		\
	(*pw_opsstd_ptr->pro_putcolormap)((pw),				\
	    (index), (count), (red), (green), (blue))
#define	pw_getcolormap(pw, index, count, red, green, blue)		\
	(*pw_opsstd_ptr->pro_getcolormap)((pw),				\
	    (index), (count), (red), (green), (blue))

#define pw_lock(pixwin,rect)						\
	(*(pixwin)->pw_clipops->pwco_lock)((pixwin), (rect));
#define pw_unlock(pixwin)						\
	(*(pixwin)->pw_clipops->pwco_unlock)((pixwin));
#define pw_reset(pixwin)						\
	(*(pixwin)->pw_clipops->pwco_reset)((pixwin));
#define pw_getclipping(pixwin)						\
	(*(pixwin)->pw_clipops->pwco_getclipping)((pixwin));

#else lint

extern pw_rop();
extern pw_write();
extern pw_writebackground();
extern pw_read();
extern pw_copy();
extern pw_batchrop();
extern pw_stencil();
extern pw_destroy();
extern pw_get();
extern pw_put();
extern pw_vector();
extern struct pixwin * pw_region();
extern pw_putattributes();
extern pw_getattributes();
extern pw_putcolormap();
extern pw_getcolormap();
extern pw_lock();
extern pw_unlock();
extern pw_reset();
extern pw_getclipping();

#endif lint

extern	struct pixwin *pw_open();
extern	struct pixwin *pw_open_monochrome();
extern	struct	pixfont *pw_pfsysopen();
#ifndef PRE_FLAMINGO
extern	struct pixwin *pw_use_video();
#endif ndef PRE_FLAMINGO

typedef struct pw_pixel_cache {
	Rect r;
	struct pixrect * plane_group[PIX_MAX_PLANE_GROUPS];
} Pw_pixel_cache;
#define	PW_PIXEL_CACHE_NULL	((Pw_pixel_cache *)0)
extern	Pw_pixel_cache *pw_save_pixels();
extern	void pw_restore_pixels();
#define pw_primary_cached_mpr(pw, pixel_cache) \
	    ((pixel_cache)->plane_group[(pw)->pw_clipdata->pwcd_plane_group])

#ifdef	cplus
/*
 * C Library routine specifically relating to pixel device functions
 */

/*
 * Pixwin_clipops types
 */
int	pwco_lock(struct pixwin *pw, struct rect *affected);
void	pwco_unlock(struct pixwin *pw);
void	pwco_reset(struct pixwin *pw);
int	pwco_getclipping(struct pixwin *pw);

/*
 * Pixwin structure utilities
 */
void	pw_open(int windowfd);
void	pw_open_monochrome(int windowfd);
void	pw_exposed(struct pixwin *pw);
void	pw_repairretained(struct pixwin *pw);
void	pw_damaged(struct pixwin *pw);
void	pw_donedamaged(struct pixwin *pw);
void	pw_close(struct pixwin *pw);

/*
 * Pixwin color map segment utilities
 */
void	pw_setcmsname(struct pixwin *pw, char cmsname[CMS_NAMESIZE]);
void	pw_getcmsname(struct pixwin *pw, char cmsname[CMS_NAMESIZE]);
void	pw_getcmsdata(struct pixwin *pw, struct colormapseg *cms, int *planes);
void	pw_blackonwhite(struct pixwin *pw, int min, max);
void	pw_whiteonblack(struct pixwin *pw, int min, max);
void	pw_reversevideo(struct pixwin *pw, int min, max);
void	pw_cyclecolormap(struct pixwin *pw, int cycles, begin, length);
void	pw_preparesurface(struct pixwin *pw, struct rect *rect);

/*
 * Global plane group preference
 */
void	pw_set_plane_group_preference(int planes);
int	pw_get_plane_group_preference();
void	pw_use_fast_monochrome();

/*
 * Additional write functions
 */
void	pw_replrop(struct pixwin *pw, int xw, yw, width, height, int op,\
		struct pixrect *pr, int xr, yr);
void	pw_text(struct pixwin *pw, int xw, yw, int op,\
		struct pixfont *pixfont, char *s);
void	pw_ttext(struct pixwin *pw, int xw, yw, int op,\
		struct pixfont *pixfont, char *s);
void	pw_char(struct pixwin *pw, int xw, yw, int op,\
		struct pixfont *pixfont, char c);
/*
 * Font utilities for extern struct pixfont *pf_sys.
 */
struct	pixfont *pw_pfsysopen();
	pw_pfsysclose();

/*
 * Retained control.
 */
void	pw_batch(struct pixwin *pw, Pw_batch_type type);
void	pw_set_x_offset(struct pixwin *pw, int offset);
void	pw_set_y_offset(struct pixwin *pw, int offset);
void	pw_set_xy_offset(struct pixwin *pw, int x_offset, y_offset);
int	pw_get_x_offset(struct pixwin *pw);
int	pw_get_y_offset(struct pixwin *pw);

/*
 * Pixel caching (menus and prompt, using fullscreen access) that
 * accomodate mulitple plane group frame buffers.
 */
Pw_pixel_cache * pw_save_pixels(Pixwin *pw, Rect  *r);
void	pw_restore_pixels(Pixwin *pw, Pw_pixel_cache *pc);
/*
 * Double buffering support functions
 */
 void pw_dbl_access(Pixwin *pw);
 void pw_dbl_release(Pixwin *pw);
 void pw_dbl_flip(Pixwin *pw);
 int pw_dbl_get(Pixwin *pw,  Pw_dbl_attribute which_attr);
 int pw_dbl_set(Pixwin *pw, int va_alist);

/*
 * Fullscreen access operations that accomodate mulitple plane group
 * frame buffers.
 *
 * Anyone calling these fullscreen_pw_* routines is assumed to have
 * done a fullscreen_init, is using the fullscreen pixwin during
 * this call, hasn't done any prepare surface under these bits,
 * and should be using a PIX_NOT(PIX_DST) operation.  Also, pw_lock
 * should not have been called before using these operations.
 */
int	fullscreen_pw_vector(Pixwin *pw, int x0, y0, x1, y1, op, cms_index);
int	fullscreen_pw_write(Pixwin *pw, int xw, yw, width, height, op,
			    struct pixrect *pr, int xr, yr);
void	fullscreen_pw_copy(Pixwin *pw, int xw, yw, width, height, op,
			   Pixwin *pw_src, int xr, yr);
#endif

#endif pixwin_DEFINED
