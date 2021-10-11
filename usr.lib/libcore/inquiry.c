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
static char sccsid[] = "@(#)inquiry.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"

/*
 * Inquire current viewing parameters.
 */
inquire_viewing_parameters(viewparm)
    vwprmtype *viewparm;
{
    float *fp, f;

    *viewparm = _core_vwstate;

    fp = (float *) (&viewparm->viewport.xmin);
    f = (float) MAX_NDC_COORD;
    /* give user NDC 0..1 */
    *fp++ = (float) _core_vwstate.viewport.xmin / f;
    *fp++ = (float) _core_vwstate.viewport.xmax / f;
    *fp++ = (float) _core_vwstate.viewport.ymin / f;
    *fp++ = (float) _core_vwstate.viewport.ymax / f;
    *fp++ = (float) _core_vwstate.viewport.zmin / f;
    *fp++ = (float) _core_vwstate.viewport.zmax / f;

    _core_ndcset |= 1;
    return (0);
}

/*
 * Inquire current coordinate window.
 */
inquire_window(umin, umax, vmin, vmax)
    float *umin, *umax, *vmin, *vmax;
{
    *umin = _core_vwstate.window.xmin;
    *umax = _core_vwstate.window.xmax;
    *vmin = _core_vwstate.window.ymin;
    *vmax = _core_vwstate.window.ymax;
}

/*
 * Inquire current 2D view up direction.
 */
inquire_view_up_2(dx_up, dy_up)
    float *dx_up, *dy_up;
{
    *dx_up = _core_vwstate.vwupdir[0];
    *dy_up = _core_vwstate.vwupdir[1];
}

/*
 * Inquire current 2D ndc space limits.
 */
inquire_ndc_space_2(width, height)
    float *width, *height;
{
    *width = _core_ndc.width;
    *height = _core_ndc.height;
}

/*
 * Inquire current 2D viewport
 */
inquire_viewport_2(xmin, xmax, ymin, ymax)
    float *xmin, *xmax, *ymin, *ymax;
{
    float f;

    f = (float) MAX_NDC_COORD;
    *xmin = (float) _core_vwstate.viewport.xmin / f;
    *xmax = (float) _core_vwstate.viewport.xmax / f;
    *ymin = (float) _core_vwstate.viewport.ymin / f;
    *ymax = (float) _core_vwstate.viewport.ymax / f;
    _core_ndcset |= 1;
}

/*
 * Inquire current 3D view reference point.
 */
inquire_view_reference_point(x_ref, y_ref, z_ref)
    float *x_ref, *y_ref, *z_ref;
{
    *x_ref = _core_vwstate.vwrefpt[0];
    *y_ref = _core_vwstate.vwrefpt[1];
    *z_ref = _core_vwstate.vwrefpt[2];
}

/*
 * Inquire current 3D view plane normal
 */
inquire_view_plane_normal(dx_norm, dy_norm, dz_norm)
    float *dx_norm, *dy_norm, *dz_norm;
{
    *dx_norm = _core_vwstate.vwplnorm[0];
    *dy_norm = _core_vwstate.vwplnorm[1];
    *dz_norm = _core_vwstate.vwplnorm[2];
}

/*
 * Inquire current 3D view plane distance
 */
inquire_view_plane_distance(view_distance)
    float *view_distance;
{
    *view_distance = _core_vwstate.viewdis;
}

/*
 * Inquire current 3D view depth
 */
inquire_view_depth(front_distance, back_distance)
    float *front_distance, *back_distance;
{
    *front_distance = _core_vwstate.frontdis;
    *back_distance = _core_vwstate.backdis;
}

/*
 * Inquire current 3D viewing projection.
 */
inquire_projection(projection_type, dx_proj, dy_proj, dz_proj)
    int *projection_type;
    float *dx_proj, *dy_proj, *dz_proj;
{
    *projection_type = _core_vwstate.projtype;
    *dx_proj = _core_vwstate.projdir[0];
    *dy_proj = _core_vwstate.projdir[1];
    *dz_proj = _core_vwstate.projdir[2];
}

/*
 * Inquire current 3D view up direction
 */
inquire_view_up_3(dx_up, dy_up, dz_up)
    float *dx_up, *dy_up, *dz_up;
{
    *dx_up = _core_vwstate.vwupdir[0];
    *dy_up = _core_vwstate.vwupdir[1];
    *dz_up = _core_vwstate.vwupdir[2];
}

/*
 * Inquire curren 3D ndc space limits.
 */
inquire_ndc_space_3(width, height, depth)
    float *width, *height, *depth;
{
    *width = _core_ndc.width;
    *height = _core_ndc.height;
    *depth = _core_ndc.depth;
}

/*
 * Inquire 3D viewport.
 */
inquire_viewport_3(xmin, xmax, ymin, ymax, zmin, zmax)
    float *xmin, *xmax, *ymin, *ymax, *zmin, *zmax;
{
    float f;

    f = (float) MAX_NDC_COORD;
    *xmin = (float) _core_vwstate.viewport.xmin / f;
    *xmax = (float) _core_vwstate.viewport.xmax / f;
    *ymin = (float) _core_vwstate.viewport.ymin / f;
    *ymax = (float) _core_vwstate.viewport.ymax / f;
    *zmin = (float) _core_vwstate.viewport.zmin / f;
    *zmax = (float) _core_vwstate.viewport.zmax / f;
    _core_ndcset |= 1;
}

/*
 * Inquire inverse of concatenated world and view matrix
 */
inquire_inverse_composite_matrix(arrayptr)
    float *arrayptr;
{
    float *matrxptr;
    short i;

    matrxptr = &_core_invwxform[0][0];
    for (i = 0; i < 16; i++) {
	*arrayptr++ = *matrxptr++;
    }
    return (0);
}

/*
 * Inquire current world transformation.
 */
inquire_world_coordinate_matrix_2(arr)
    float *arr;
{
    *arr++ = _core_modxform[0][0];
    *arr++ = _core_modxform[0][1];
    *arr++ = _core_modxform[0][3];
    *arr++ = _core_modxform[1][0];
    *arr++ = _core_modxform[1][1];
    *arr++ = _core_modxform[1][3];
    *arr++ = _core_modxform[2][0];
    *arr++ = _core_modxform[2][1];
    *arr++ = _core_modxform[2][3];
    return (0);
}

/*
 * Inquire current world transformation.
 */
inquire_world_coordinate_matrix_3(arrayptr)
    float *arrayptr;
{
    float *matrxptr;
    short i;

    matrxptr = &_core_modxform[0][0];
    for (i = 0; i < 16; i++) {
	*arrayptr++ = *matrxptr++;
    }
    return (0);
}

/*
 * Inquire viewing control parameters.
 */
inquire_viewing_control_parameters(windowclip, frontclip, backclip, type)
    int *windowclip, *frontclip, *backclip, *type;
{
    *windowclip = _core_wndwclip;
    *frontclip = _core_frontclip;
    *backclip = _core_backclip;
    *type = _core_coordsys;
}

/*
 * Inquire which surfaces were selected when the segment was created
 */
inquire_retained_segment_surfaces(segname, arraycnt, surfaray, surfnum)
    int segname;
    int *surfnum;
    struct vwsurf surfaray[];
int arraycnt;

{
    char *funcname;
    register int index;
    short found;
    segstruc *segptr;

    funcname = "inquire_retained_segment_surfaces";
    found = FALSE;

    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if (segname == segptr->segname) {
	    found = TRUE;
	    break;
	}
    }
    if (!found) {		/*** SPECIFIED SEGMENT EXIST ?? ***/
	_core_errhand(funcname, 14);
	return (1);
    }

    /*
     * Copy number and logical names of view surfaces selected when 
     * segment was created.
     */

    *surfnum = segptr->vsurfnum;

    for (index = 0; index < segptr->vsurfnum; index++) {
	if (index >= arraycnt)
	    break;
	else
	    surfaray[index] = segptr->vsurfptr[index]->vsurf;
    }
    return (0);
}

/*
 * Inquire existing segment names
 */
inquire_retained_segment_names(listcnt, seglist, segcnt)
    int seglist[];
int listcnt;
int *segcnt;

{
    register int index;
    segstruc *segptr;

    index = 0;
    *segcnt = _core_segnum;
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	if (segptr->type > NORETAIN) {	/* SEGMENT CURRENTLY EXIST? */
	    if (index >= listcnt)
		break;
	    else
		seglist[index++] = segptr->segname;
	}
    }
    return (0);
}

/*
 * Inquire segment name of open segment
 */
inquire_open_retained_segment(segname)
    int *segname;
{
    if (_core_osexists)
	*segname = _core_openseg->segname;
    else
	*segname = 0;
    return (0);
}

/*
 * Inquire if temporary segment is open
 */
inquire_open_temporary_segment(open)
    int *open;
{
    *open = (_core_osexists && (_core_openseg->type == NORETAIN));
    return (0);
}
