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
static char sccsid[] = "@(#)ndctowld.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"

/*
 * Convert normalized device coordinates into world coordinates
 */
map_ndc_to_world_2(ndcx, ndcy, wldx, wldy)
    float ndcx, ndcy, *wldx, *wldy;
{

    /* Compute world coordinates using inverse transform */
    static char funcname[] = "map_ndc_to_world_2";
    register int nx, ny, round;
    float f;

    if (_core_check_x_form(funcname)) {
	return(34);
    }

    f = ndcx;
    f *= (float) MAX_NDC_COORD;
    nx = _core_roundf(&f);
    f = ndcy;
    f *= (float) MAX_NDC_COORD;
    ny = _core_roundf(&f);

    if ((nx -= _core_poffx) < 0)
	round = (-_core_scalex) >> 1;
    else
	round = _core_scalex >> 1;
    nx = ((nx << 15) + round) / _core_scalex;
    if ((ny -= _core_poffy) < 0)
	round = (-_core_scaley) >> 1;
    else
	round = _core_scaley >> 1;
    ny = ((ny << 15) + round) / _core_scaley;

    *wldx = (float) nx *_core_invwxform[0][0] +
     (float) ny *_core_invwxform[1][0] +
     (float) MAX_NDC_COORD *_core_invwxform[3][0];
    *wldy = (float) nx *_core_invwxform[0][1] +
     (float) ny *_core_invwxform[1][1] +
     (float) MAX_NDC_COORD *_core_invwxform[3][1];

    return (0);
}

/*
 * Convert world coordinates to normalized device coordinates.
 */
map_world_to_ndc_2(wldx, wldy, ndcx, ndcy)
    float wldx, wldy, *ndcx, *ndcy;
{
    static char funcname[] = "map_world_to_ndc_2";
    pt_type p1, p2;
    ipt_type ip;

    /*
     * Transform world coords to current viewport NDC coords 
     */

    if (_core_check_x_form(funcname)) {
	return(34);
    }
    p1.x = wldx;
    p1.y = wldy;
    p1.z = 0.;
    p1.w = 1.0;
    _core_tranpt2(&p1.x, &p2.x);
    _core_pt3cnvrt(&p2.x, &ip.x);
    _core_vwpscale(&ip);
    *ndcx = (float) ip.x / (float) MAX_NDC_COORD;
    *ndcy = (float) ip.y / (float) MAX_NDC_COORD;

    return (0);
}

/*
 * Convert normalized device coordinates into world coordinates
 */
map_ndc_to_world_3(ndcx, ndcy, ndcz, wldx, wldy, wldz)
    float ndcx, ndcy, ndcz, *wldx, *wldy, *wldz;
{

    /*
     * Compute world coordinates using inverse view transform 
     */

    static char funcname[] = "map_ndc_to_world_3";
    register int nx, ny, nz, round;
    float f;

    if (_core_check_x_form(funcname)) {
	return(34);
    }
    f = ndcx;
    f *= (float) MAX_NDC_COORD;
    nx = _core_roundf(&f);
    f = ndcy;
    f *= (float) MAX_NDC_COORD;
    ny = _core_roundf(&f);
    f = ndcz;
    f *= (float) MAX_NDC_COORD;
    nz = _core_roundf(&f);

    if ((nx -= _core_poffx) < 0)
	round = (-_core_scalex) >> 1;
    else
	round = _core_scalex >> 1;
    nx = ((nx << 15) + round) / _core_scalex;
    if ((ny -= _core_poffy) < 0)
	round = (-_core_scaley) >> 1;
    else
	round = _core_scaley >> 1;
    ny = ((ny << 15) + round) / _core_scaley;
    if ((nz -= _core_poffz) < 0)
	round = (-_core_scalez) >> 1;
    else
	round = _core_scalez >> 1;
    nz = ((nz << 15) + round) / _core_scalez;

    *wldx = (float) nx *_core_invwxform[0][0] +
     (float) ny *_core_invwxform[1][0] +
     (float) nz *_core_invwxform[2][0] +
     (float) MAX_NDC_COORD *_core_invwxform[3][0];
    *wldy = (float) nx *_core_invwxform[0][1] +
     (float) ny *_core_invwxform[1][1] +
     (float) nz *_core_invwxform[2][1] +
     (float) MAX_NDC_COORD *_core_invwxform[3][1];
    *wldz = (float) nx *_core_invwxform[0][2] +
     (float) ny *_core_invwxform[1][2] +
     (float) nz *_core_invwxform[2][2] +
     (float) MAX_NDC_COORD *_core_invwxform[3][2];

    return (0);
}

/*
 * Convert world coordinates into normalized device coordinates.
 */
map_world_to_ndc_3(wldx, wldy, wldz, ndcx, ndcy, ndcz)
    float wldx, wldy, wldz, *ndcx, *ndcy, *ndcz;
{
    static char funcname[] = "map_world_to_ndc_3";
    pt_type p1, p2;
    ipt_type ip;

    /* View transform world coords to current viewport coords */
    if (_core_check_x_form(funcname)) {
	return(34);
    }
    p1.x = wldx;
    p1.y = wldy;
    p1.z = wldz;
    p1.w = 1.0;
    _core_tranpt(&p1.x, &p2.x);
    _core_pt3cnvrt(&p2.x, &ip.x);
    _core_vwpscale(&ip);
    *ndcx = (float) ip.x / (float) MAX_NDC_COORD;
    *ndcy = (float) ip.y / (float) MAX_NDC_COORD;
    *ndcz = (float) ip.z / (float) MAX_NDC_COORD;

    return (0);
}

_core_check_x_form(funcname)
    char *funcname;
{

    if (_core_vtchang) {
	if (_core_validvt(TRUE)) {	/*** valid viewing transformation? ***/
	    _core_errhand(funcname, 34);
	    return (34);
	}
    }
    return(0);
}

