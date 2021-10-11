/* ******* @(#)cg6var.h 1.1 7/30/92 Copyright 1988 Sun Micro ******/
/* @(#)cg6var.h 1.12 89/03/30 SMI */

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#ifndef	cg6var_DEFINED
#define	cg6var_DEFINED

#ifndef	FBC_REV0
#define	FBC_REV0	1	/* include FBC0 support if set */
#endif

#include "cg6fbc.h"
#include "cg6tec.h"
#include "cg6thc.h"

#define CG6_DEPTH	8

#ifndef KERNEL

#include <pixrect/memvar.h>
#include <pixrect/pr_impl_util.h>

/* pixrect private data */
struct cg6pr {
	struct mprp_data mprp;		/* memory pixrect simulator */
	int		fd;		/* device file descriptor */
	struct pr_size	cg6_size;	/* screen size */
	struct fbc	*cg6_fbc;	/* FBC base */
	struct tec 	*cg6_tec;	/* TEC base */
};

/* pixrect ops vector */
extern struct pixrectops cg6_ops;

Pixrect *cg6_make();

int cg6_rop();
int cg6_stencil();
int cg6_batchrop();
int cg6_destroy();
int cg6_get();
int cg6_put();
int cg6_vector();
Pixrect *cg6_region();
int cg6_putcolormap();
int cg6_getcolormap();
int cg6_putattributes();
int cg6_getattributes();

/* macros */

#define cg6_d(pr) 	((struct cg6pr *)(pr)->pr_data)

#define	fbc_exception(s)	((int) (s) < 0)

#define	cg6_tec_from_fbc(fbc)	((struct tec *) PTR_ADD(fbc, 4096))

/* clear TEC exception bit */
#define	cg6_fix_tec(tec)	_STMT( \
	if (* (int *) &tec->l_tec_clip < 0) \
		* (int *) &tec->l_tec_clip &= 0x7fffffff; \
)

#define cg6_waitidle(fbc) \
	do {;} while ((fbc)->l_fbc_status & L_FBC_BUSY)

#define cg6_setfontxy(fbc, x0, x1, y) \
	((fbc)->l_fbc_x0 = (x0), \
	(fbc)->l_fbc_x1 = (x1), \
	(fbc)->l_fbc_y0 = (y))

#define cg6_setinx(fbc, x, y) \
	((fbc)->l_fbc_autoincx = (x), \
	(fbc)->l_fbc_autoincy = (y))

/*
 * return draw status, if full loop until registers available before returning.
 */
#define cg6_draw_done(fbc, r) \
	do \
		(r) = (int) (fbc)->l_fbc_drawstatus; \
	while ((int) (r) < 0 && (r & L_FBC_FULL))

/*
 * set clip area.
 */
#define cg6_clip(fbc, x_min, y_min, x_max, y_max) \
	((fbc)->l_fbc_clipminx = (x_min), \
	(fbc)->l_fbc_clipminy = (y_min), \
	(fbc)->l_fbc_clipmaxx = (x_max), \
	(fbc)->l_fbc_clipmaxy = (y_max))

#define cg6_color_mode(fbc, mode) \
	(* ((u_int *) &(fbc)->l_fbc_misc) = \
		(int) L_FBC_MISC_BLIT_NOSRC << 20 | \
		((int) (mode) & 3) << 17 |	/* data */ \
		(int) L_FBC_MISC_DRAW_RENDER << 15)

#define cg6_setregs(fbc, x, y, rop, planemask, fcolor, patt, polyg) _STMT( \
	(fbc)->l_fbc_rasteroffx = (x); \
	(fbc)->l_fbc_rasteroffy = (y); \
	(fbc)->l_fbc_fcolor = (fcolor); \
	cg6_waitidle(fbc); \
	(fbc)->l_fbc_bcolor = 0; \
	(fbc)->l_fbc_status = 0; \
	* ((u_int *) &(fbc)->l_fbc_rasterop) = \
		(int) L_FBC_RASTEROP_PLANE_MASK << 30 | \
		(int) L_FBC_RASTEROP_PIXEL_ONES << 28 | \
		((int) (patt) & 3) << 26 | \
		((int) (polyg) & 3) << 24 | \
		(int) L_FBC_RASTEROP_ATTR_SUPP << 22 | \
		(int) L_FBC_RASTEROP_RAST_BOOL << 17 | \
		((int) (rop) & 15) << 8 |	/* rop10 */ \
		((int) (rop) & 3) << 2 |	/* rop00 */ \
		((int) (rop) & 3);		/* rop00 */ \
	(fbc)->l_fbc_planemask = (planemask); \
	cg6_fix_tec(cg6_tec_from_fbc(fbc)); \
	/* can set this before idle in FBC1 */ \
	(fbc)->l_fbc_clipcheck = 0; \
)

/*
 * FBC0 workarounds
 */
#if FBC_REV0
int cg6_vector0();

#define	fbc_rev0(fbc)	((((char *)(fbc))[1] & 0xf0) == 0)

#define cg6_draw_done0(fbc, x0, x1, r) \
	if (((fbc)->l_fbc_status & L_FBC_DRAW_INTERSECT) && \
		((x0) < 0 || (x1) < 0)) \
		(r) = L_FBC_DRAW_EXCEPTION; \
	else cg6_draw_done((fbc), (r))
#else FBC_REV0
#define	fbc_rev0(fbc)	(0)
#define cg6_draw_done0(fbc, x0, x1, r) \
	cg6_draw_done((fbc), (r))
#endif FBC_REV0

#endif !KERNEL

#endif !cg6var_DEFINED
