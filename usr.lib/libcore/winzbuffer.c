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
static char sccsid[] = "@(#)winzbuffer.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/* Z-buffer routines for hidden surface removal for color windows Sun I */

#include "coretypes.h"
#include "corevars.h"
#include "colorshader.h"
#include <pixrect/pixrect_hs.h>

/*----------------------------------------------------------------------*/
_core_pwzbuffer_ptr( x, y, zbptr, zbcut, zbpr, cutpr)
	register int x,y; short **zbptr, **zbcut;
	struct pixrect *zbpr, *cutpr;
{
    *zbptr = (short*)mprd8_addr( mpr_d(zbpr), x, y, zbpr->pr_depth);
    *zbcut = (short*)mprd8_addr( mpr_d(cutpr), x, 0, cutpr->pr_depth);
    /* printf(stderr, "x %d y %d zptr %d\n", x,y,*zbptr);
    */
}
/*----------------------------------------------------------------------*/
_core_set_pwzbufcut( cutpr, xarr, zarr, n)
	float xarr[], zarr[];  int n;
	struct pixrect *cutpr;
{
    int i,j, x0, z0, x1, z1, dz;
    short *ptr;
   
    for (i=0; i<n; i++){
    	if (xarr[i] < 0. || xarr[i] > _core_ndc.width
	||  zarr[i] < 0. || zarr[i] > _core_ndc.depth ){
		_core_errhand("set_zbuffer_cut", 71);
		return(71);
		}
	}
    x0 = (cutpr->pr_width-1) * xarr[0];
    z0 = MAX_NDC_COORD * zarr[0];
    for (i=1; i<n; i++){
	x1 = (cutpr->pr_width-1) * xarr[i];
	z1 = MAX_NDC_COORD * zarr[i];
	if (x1 <= x0) continue;
	dz = (z1-z0)/(x1-x0);
     	ptr = (short *)mprd8_addr( mpr_d(cutpr), x0, 0, cutpr->pr_depth);
	for (j=x0; j<=x1; j++) {
		*ptr++ = z0;
		z0 += dz;
		}
	x0 = x1;
	z0 = z1;
	}
    return(0);
}
/*----------------------------------------------------------------------*/
_core_init_pwzbuffer( w, h, zbpr, cutpr)
	int w,h;
	struct pixrect **zbpr, **cutpr;
{
    int op;

    *zbpr = mem_create( w, h, 16);
    *cutpr = mem_create( w, 1, 16);
    if (*zbpr && *cutpr) {
        op = PIX_COLOR(MAX_NDC_COORD) | PIX_SRC;
        pr_rop( *zbpr,0,0, w,h, op, (struct pixrect *)0,0,0);
	return(0);
    }
    return(1);
}

/*----------------------------------------------------------------------*/
_core_clear_pwzbuffer( zbpr, cutpr)
	struct pixrect *zbpr, *cutpr;
{
    int op;

    op = PIX_COLOR(MAX_NDC_COORD) | PIX_SRC;
    if (zbpr) pr_rop( zbpr,0,0, zbpr->pr_size.x,zbpr->pr_size.y, op, 
	(struct pixrect *)0,0,0);
    if (cutpr) pr_rop( cutpr,0,0, cutpr->pr_size.x,1, PIX_SRC, 
	(struct pixrect *)0,0,0);
}
/*----------------------------------------------------------------------*/
_core_terminate_pwzbuffer(zbpr, cutpr)
	struct pixrect *zbpr, *cutpr;
{
    if (zbpr) mem_destroy ( zbpr);
    if (cutpr) mem_destroy ( cutpr);
}
