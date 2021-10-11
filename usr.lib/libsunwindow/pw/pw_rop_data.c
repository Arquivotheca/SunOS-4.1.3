#ifndef lint
#ifdef sccs
static	char sccsid[] = "%Z%%M% %I% %E% Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * pw_rop_data.c: fix for shared libraries in SunOS4.0.  Code was isolated
 *		  from pw_rop.c 
 */

#include <sys/types.h>
#include <pixrect/pixrect.h>

extern	pwo_get(), pwo_put(), pwo_rop(), pwo_vector(), pwo_putcolormap(),
	    pwo_getcolormap(), pwo_putattributes(), pwo_getattributes(),
	    pwo_stencil(), pwo_batchrop();
extern	int pw_close();
extern	int pwo_nop();
extern	struct pixrect *pwo_region();

int	pw_dontclipflag = PIX_DONTCLIP;

/*
 * following is copied from pw_access.c for shared libs.
 *
 */
 
struct	pixrectops pw_opsstd = {
	pwo_rop,
	pwo_stencil,
	pwo_batchrop,
	0,
	pw_close,
	pwo_get,
	pwo_put,
	pwo_vector,
	pwo_region,
	pwo_putcolormap,
	pwo_getcolormap,
	pwo_putattributes,
	pwo_getattributes,
};
struct	pixrectops *pw_opsstd_ptr = &pw_opsstd;

struct	pixrectops pw_opsstd_null_lock = {
	pwo_nop,
	pwo_nop,
	pwo_nop,
	0,
	pw_close,
	pwo_nop,
	pwo_nop,
	pwo_nop,
	pwo_region,
	pwo_putcolormap,
	pwo_getcolormap,
	pwo_putattributes,
	pwo_getattributes,
};
