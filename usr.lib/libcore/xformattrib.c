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
static char sccsid[] = "@(#)xformattrib.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

/*
	Operation:
		These routines maintain the transformation stack,
	viewport attributes.  Included are routines for concatenating
	matrices, transforming points, taking vector crossproducts,
	computing unit vectors, obtaining the length of a vector, finding
	vector dotproducts and popping and pushing transformations on the
	transform stack.  The stack currently will hold only 10 transforms.
  
    Usage:
  
	typedef struct{ float x,y,z,w;
	    } pt_type;			' homogeneous point type 
	
		--- Transform utility operations ---
  
	_core_push( m) float *m;	' push 4x4 matrix onto stack
	_core_pop( m)  float *m;	' pop  4x4 matrix from stack
	_core_matcon();		' concatenate top matrix times next
				' matrix, pop both, push the result
				' on the stack.
	crossprod( p1,p2,p3) pt_type *p1, *p2, *p3;
				' 3-D vector cross product,
				' p3 = p1 X p2.
	float dotprod( p1,p2) pt_type *p1, *p2;
				' 3-D vector dot product
	_core_unitvec( p1) pt_type *p1;
				' Convert 3-D vector to unit vector.
	float vecleng( x,y,z) float x,y,z;
				' Return length of vector.
	_core_make_mat();		' Build transform from view attributes
				' and push it on the stack.
  
		--- Viewing attribute operations ---
  
	set_viewing_parameters(viewparm)' Set all viewing parameters
	set_view_reference_point(x,y,z)	' Set view reference in world coords.
	set_view_plane_normal( dx,dy,dz)' Set view plane normal.
	set_view_plane_distance(vwdist)	' Set view distance.
	set_view_up_3( dxup, dyup, dzup)' Set up direction for viewing window.
	set_window(umin,umax,vmin,vmax)
					' Set view window size in world coords.
	set_viewport_2(xmin,xmax,ymin,ymax)
					' Set view port in display coords.
	set_view_depth( near,far)	' Set near and far clipping planes.
	set_projection( type,dx,dy,dz);	' Specify perspective or parallel.
					' set direction of projection.
					' set center of projection.
*/

#include "coretypes.h"
#include "corevars.h"
#include <math.h>
#include <stdio.h>

static char *funcname;
static int errnum;

_core_push(m)
    float *m;			/* push 4x4 transform onto stack */
{
    int i;

    if (_core_TSp >= TSlim)
	(void) fprintf(stderr, "Transform stack overflow!\n");
    else {
	_core_TSp += 1;
	for (i = 0; i < TSize; i++)
	    _core_TStack[_core_TSp][i] = *m++;
    }
    _core_ddxformset = FALSE;
}

_core_pop(m)
    float *m;			/* pop transform, return it to caller */
{
    int i;

    if (_core_TSp < 0)
	(void) fprintf(stderr, "Transform stack empty!\n");
    else {
	for (i = 0; i < TSize; i++)
	    *m++ = _core_TStack[_core_TSp][i];
	_core_TSp -= 1;
    }
    _core_ddxformset = FALSE;
}

_core_copytop(m)
    float *m;
{
    int i;

    if (_core_TSp < 0)
	(void) fprintf(stderr, "Transform stack empty!\n");
    else
	for (i = 0; i < TSize; i++)
	    *m++ = _core_TStack[_core_TSp][i];
}

_core_matcon()
{				/* concatenate top 2 transforms on *//* stack		 */
    int j, l;			/* row col		 */
    float m3[4][4];		/* m3(j, l) = Sum(m1(j,k)*m2(k,l)  */
    float sum, *m1, *m2, *r;	/* k	 */
    float *p, *q;
    register short k;
    ddargtype ddstruct;

    if (_core_TSp < 1) {
	(void) fprintf(stderr, "Transform stack empty!\n");
	return (-1);
    }
    m1 = &_core_TStack[_core_TSp][0];
    m2 = &_core_TStack[_core_TSp - 1][0];
    if (_core_xformvs) {
	_core_critflag++;	/* critical section since slight chance of
				 * view surface termination via SIGCHILD
				 * during the call to the dd */
	ddstruct.opcode = MATMULT;	/* matrix mult */
	ddstruct.instance = _core_xformvs->instance;
	ddstruct.ptr1 = (char *) m1;
	ddstruct.ptr2 = (char *) m2;
	ddstruct.ptr3 = (char *) m3;
	(*_core_xformvs->dd) (&ddstruct);
	if (--_core_critflag == 0 && _core_updatewin && _core_sighandle)
	    (*_core_sighandle) ();
    } else {
	for (j = 0; j < 4; j++) {	/* for all output matrix rows */
	    r = m1 + j * 4;
	    for (l = 0; l < 4; l++) {	/* for all output matrix cols */
		q = m2 + l;
		p = r;
		sum = 0.0;
		for (k = 0; k < 4; k++) {	/* sum over k */
		    sum += *p++ * *q;
		    q += 4;
		}
		m3[j][l] = sum;
	    }
	}
    }
    _core_TSp -= 2;
    _core_push(&m3[0][0]);
    return (0);
}

static 
crossprod(p1, p2, p3)
    pt_type *p1, *p2, *p3;
{				/* vector cross product p3=p1xp2 */
    p3->x = (p1->y * p2->z) - (p1->z * p2->y);
    p3->y = (p1->z * p2->x) - (p1->x * p2->z);
    p3->z = (p1->x * p2->y) - (p1->y * p2->x);
}

static float 
dotprod(p1, p2)
    pt_type *p1, *p2;
{				/* vector dot product val=p1.p2 */
    return (p1->x * p2->x + p1->y * p2->y + p1->z * p2->z);
}

static float 
vecleng(x1, y1, z1)
    float x1, y1, z1;
{				/* compute length of vector */
    return (sqrt(x1 * x1 + y1 * y1 + z1 * z1));
}

_core_unitvec(p1)
    pt_type *p1;
{				/* convert p1 to unit vector */
    float len;

    len = vecleng(p1->x, p1->y, p1->z);
    if (len == 0.0)
	len = 1.0;
    p1->x /= len;
    p1->y /= len;
    p1->z /= len;
}

set_ndc_space_2(width, height)
    float width, height;
{
    char *funcname;
    viewsurf *surfp;
    ddargtype ddstruct;

    funcname = "set_ndc_space_2";
    if (_core_ndcset) {
	if (_core_ndcset & 2) {
	    _core_errhand(funcname, 97);
	    return (503);
	}
	_core_errhand(funcname, 98);
	return (504);
    }
    if (width < 0.0 || width > 1.0 || height < 0.0 || height > 1.0) {
	_core_errhand(funcname, 99);
	return (505);
    }
    if (width != 1.0 && height != 1.0) {
	_core_errhand(funcname, 100);
	return (506);
    }
    if (width == 0.0 || height == 0.0) {
	_core_errhand(funcname, 101);
	return (507);
    }
    _core_ndcset |= 2;
    _core_ndc.width = width;
    _core_ndc.height = height;
    _core_ndc.depth = 0.0;
    _core_ndcspace[0] = (int) (width * (float) MAX_NDC_COORD);
    _core_ndcspace[1] = (int) (height * (float) MAX_NDC_COORD);
    _core_ndcspace[2] = 0;
    ddstruct.opcode = SETNDC;
    ddstruct.float1 = width;
    ddstruct.float2 = height;
    ddstruct.float3 = 0.0;
    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++)
	if (surfp->vinit) {
	    ddstruct.instance = surfp->vsurf.instance;
	    (*surfp->vsurf.dd) (&ddstruct);
	}
    return (0);
}

set_ndc_space_3(width, height, depth)
    float width, height, depth;
{
    char *funcname;
    viewsurf *surfp;
    ddargtype ddstruct;

    funcname = "set_ndc_space_3";
    if (_core_ndcset) {
	if (_core_ndcset & 2) {
	    _core_errhand(funcname, 97);
	    return (503);
	}
	_core_errhand(funcname, 98);
	return (504);
    }
    if (width < 0.0 || width > 1.0 || height < 0.0 || height > 1.0 ||
	depth < 0.0 || depth > 1.0) {
	_core_errhand(funcname, 99);
	return (505);
    }
    if (width != 1.0 && height != 1.0) {
	_core_errhand(funcname, 100);
	return (506);
    }
    if (width == 0.0 || height == 0.0) {
	_core_errhand(funcname, 101);
	return (507);
    }
    _core_ndcset |= 2;
    _core_ndc.width = width;
    _core_ndc.height = height;
    _core_ndc.depth = depth;
    _core_ndcspace[0] = (int) (width * (float) MAX_NDC_COORD);
    _core_ndcspace[1] = (int) (height * (float) MAX_NDC_COORD);
    _core_ndcspace[2] = (int) (depth * (float) MAX_NDC_COORD);
    ddstruct.opcode = SETNDC;
    ddstruct.float1 = width;
    ddstruct.float2 = height;
    ddstruct.float3 = depth;
    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++)
	if (surfp->vinit) {
	    ddstruct.instance = surfp->vsurf.instance;
	    (*surfp->vsurf.dd) (&ddstruct);
	}
    return (0);
}

set_viewing_parameters(viewparm)
    struct {
	float vwrefpt[3];
	float vwplnorm[3];
	float viewdis;
	float frontdis;
	float backdis;
	int projtype;
	float projdir[3];
	windtype window;
	float vwupdir[3];
	float viewport[6];
    } *viewparm;

{
    int err;
    vwprmtype savevwp;
    int savevfinvokd, savevtchang, savendcset;

    if (_core_osexists) {
	_core_errhand("set_viewing_parameters", 8);
	return (6);
    }
    savevwp = _core_vwstate;
    savevfinvokd = _core_vfinvokd;
    savevtchang = _core_vtchang;
    savendcset = _core_ndcset;
    err = 0;
    do {
	if (set_view_reference_point(viewparm->vwrefpt[0],
			      viewparm->vwrefpt[1], viewparm->vwrefpt[2])) {
	    err = 1;
	    break;
	}
	if (set_view_plane_normal(viewparm->vwplnorm[0],
			    viewparm->vwplnorm[1], viewparm->vwplnorm[2])) {
	    err = 2;
	    break;
	}
	if (set_view_plane_distance(viewparm->viewdis)) {
	    err = 3;
	    break;
	}
	if (set_view_depth(viewparm->frontdis, viewparm->backdis)) {
	    err = 4;
	    break;
	}
	if (set_projection(viewparm->projtype, viewparm->projdir[0],
			   viewparm->projdir[1], viewparm->projdir[2])) {
	    err = 5;
	    break;
	}
	if (set_window(viewparm->window.xmin, viewparm->window.xmax,
		       viewparm->window.ymin, viewparm->window.ymax)) {
	    err = 6;
	    break;
	}
	if (set_view_up_3(viewparm->vwupdir[0], viewparm->vwupdir[1],
			  viewparm->vwupdir[2])) {
	    err = 7;
	    break;
	}
	if (set_viewport_3(viewparm->viewport[0],
			   viewparm->viewport[1], viewparm->viewport[2],
			   viewparm->viewport[3], viewparm->viewport[4],
			   viewparm->viewport[5])) {
	    err = 8;
	    break;
	}
    }
    while (FALSE);
    if (err) {
	_core_errhand("set_viewing_parameters", 88 + err);
	_core_vwstate = savevwp;
	_core_vfinvokd = savevfinvokd;
	_core_vtchang = savevtchang;
	_core_ndcset = savendcset;
	return (err);
    }
    return (0);
}

set_view_reference_point(x, y, z)
    float x, y, z;
{				/* set look at point, world coords */
    _core_vwstate.vwrefpt[0] = x;
    _core_vwstate.vwrefpt[1] = y;
    _core_vwstate.vwrefpt[2] = z;
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

set_view_plane_normal(dx, dy, dz)
    float dx, dy, dz;
{				/* set view plane normal, world   */
    funcname = "set_view_plane_normal";
    if ((dx == 0.0) && (dy == 0.0) && (dz == 0.0)) {
	errnum = 39;
	_core_errhand(funcname, errnum);
	return (2);
    }
    _core_vwstate.vwplnorm[0] = dx;
    _core_vwstate.vwplnorm[1] = dy;
    _core_vwstate.vwplnorm[2] = dz;
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

set_view_plane_distance(dist)
    float dist;
{				/* set viewing distance */
    _core_vwstate.viewdis = dist;
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

set_view_up_2(dx, dy)
    float dx, dy;
{				/* set direction of 'up'  */
    funcname = "set_view_up_2";
    if ((dx == 0.0) && (dy == 0.0)) {
	_core_errhand(funcname, 39);
	return (2);
    }
    _core_vwstate.vwupdir[0] = dx;
    _core_vwstate.vwupdir[1] = dy;
    _core_vwstate.vwupdir[2] = 0.0;
    _core_vfinvokd = TRUE;
    _core_vtchang = TRUE;
    return (0);
}

set_view_up_3(dx, dy, dz)
    float dx, dy, dz;
{				/* set direction of 'up'  */
    funcname = "set_view_up_3";
    if ((dx == 0.0) && (dy == 0.0) && (dz == 0.0)) {
	errnum = 39;
	_core_errhand(funcname, errnum);
	return (2);
    }
    _core_vwstate.vwupdir[0] = dx;
    _core_vwstate.vwupdir[1] = dy;
    _core_vwstate.vwupdir[2] = dz;
    _core_vfinvokd = TRUE;
    _core_vtchang = TRUE;
    return (0);
}

set_window(umin, umax, vmin, vmax)
    float umin, umax, vmin, vmax;
{				/* set view window in world coords */
    funcname = "set_window";
    if ((umin >= umax) || (vmin >= vmax)) {
	errnum = 40;
	_core_errhand(funcname, errnum);
	return (2);
    }
    _core_vwstate.window.xmin = umin;
    _core_vwstate.window.xmax = umax;
    _core_vwstate.window.ymin = vmin;
    _core_vwstate.window.ymax = vmax;
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

set_viewport_2(xmin, xmax, ymin, ymax)
    float xmin, xmax, ymin, ymax;
{				/* set view port in NDC coords */
    float f;

    funcname = "set_viewport_2";
    if (_core_osexists) {
	_core_errhand(funcname, 8);
	return (1);
    }
    f = (float) MAX_NDC_COORD;
    xmin *= f;
    xmax *= f;
    ymin *= f;
    ymax *= f;
    if (((int) xmax > _core_ndcspace[0]) || ((int) ymax > _core_ndcspace[1])
	|| (xmin < 0.0) || (ymin < 0.0)) {
	errnum = 46;
	_core_errhand(funcname, errnum);
	return (1);
    }
    if ((xmin >= xmax) || (ymin >= ymax)) {
	errnum = 47;
	_core_errhand(funcname, errnum);
	return (3);
    }
    _core_vwstate.viewport.xmin = (int) xmin;
    _core_vwstate.viewport.xmax = (int) xmax;
    _core_vwstate.viewport.ymin = (int) ymin;
    _core_vwstate.viewport.ymax = (int) ymax;
    /* compute scale params */
    _core_scalex = (int) ((xmax - xmin) / 2.0);
    _core_scaley = (int) ((ymax - ymin) / 2.0);;
    /* and offset params */
    _core_poffx = (int) ((xmax + xmin) / 2.0);
    _core_poffy = (int) ((ymax + ymin) / 2.0);;

    _core_vfinvokd = TRUE;
    _core_ndcset |= 1;
    _core_vtchang = TRUE;
    return (0);
}

set_viewport_3(xmin, xmax, ymin, ymax, zmin, zmax)
    float xmin, xmax, ymin, ymax, zmin, zmax;
{				/* set view port in NDC coords */
    float f;

    funcname = "set_viewport_3";
    if (_core_osexists) {
	_core_errhand(funcname, 8);
	return (1);
    }
    f = (float) MAX_NDC_COORD;
    xmin *= f;
    xmax *= f;
    ymin *= f;
    ymax *= f;
    zmin *= f;
    zmax *= f;
    if (((int) xmax > _core_ndcspace[0]) || ((int) ymax > _core_ndcspace[1])
	|| ((int) zmax > _core_ndcspace[2]) || (xmin < 0.0)
	|| (ymin < 0.0) || (zmin < 0.0)) {
	errnum = 46;
	_core_errhand(funcname, errnum);
	return (1);
    }
    if ((xmin >= xmax) || (ymin >= ymax) || (zmin >= zmax)) {
	errnum = 47;
	_core_errhand(funcname, errnum);
	return (3);
    }
    _core_vwstate.viewport.xmin = (int) xmin;
    _core_vwstate.viewport.xmax = (int) xmax;
    _core_vwstate.viewport.ymin = (int) ymin;
    _core_vwstate.viewport.ymax = (int) ymax;
    _core_vwstate.viewport.zmin = (int) zmin;
    _core_vwstate.viewport.zmax = (int) zmax;

    /* compute scale params */
    _core_scalex = (int) ((xmax - xmin) / 2.0);
    _core_scaley = (int) ((ymax - ymin) / 2.0);
    _core_scalez = (int) (zmax - zmin);

    /* and offset params */
    _core_poffx = (int) ((xmax + xmin) / 2.0);
    _core_poffy = (int) ((ymax + ymin) / 2.0);
    _core_poffz = (int) zmin;

    _core_vfinvokd = TRUE;
    _core_ndcset |= 1;
    _core_vtchang = TRUE;
    return (0);
}

set_view_depth(near, far)
    float near, far;
{				/* set clipping plane distances */

    funcname = "set_view_depth";
    if (near > far) {
	errnum = 41;
	_core_errhand(funcname, errnum);
	return (1);
    } else {
	_core_vwstate.frontdis = near;
	_core_vwstate.backdis = far;
    }
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

set_projection(projtype, dx, dy, dz)
    int projtype;
    float dx, dy, dz;
{
    funcname = "set_projection";
    if (projtype == PARALLEL) {
	if ((dx == 0.0) && (dy == 0.0) && (dz == 0.0)) {
	    errnum = 39;
	    _core_errhand(funcname, errnum);
	    return (1);
	}
	_core_vwstate.projtype = PARALLEL;	/* specify parallel
						 * projection */
	_core_vwstate.projdir[0] = dx;
	_core_vwstate.projdir[1] = dy;
	_core_vwstate.projdir[2] = dz;
    } else {
	_core_vwstate.projtype = PERSPECTIVE;
	_core_vwstate.projdir[0] = dx;
	_core_vwstate.projdir[1] = dy;
	_core_vwstate.projdir[2] = dz;
    }
    _core_vtchang = TRUE;
    _core_vfinvokd = TRUE;
    return (0);
}

int 
_core_make_mat()
{				/* build matrix from view attrib */
    int i, err;
    float cp[3];
    register float *ptr1, *ptr2;

    _core_TSp = -1;
    crossprod((pt_type *) _core_vwstate.vwplnorm, (pt_type *) _core_vwstate.vwupdir, (pt_type *) cp);
    if (vecleng(cp[0], cp[1], cp[2]) < 1.0e-15)
	err = 1;		/* vwplnorm || vwupdir */

    else if ((_core_vwstate.frontdis == _core_vwstate.backdis) &&
	     (_core_vwstate.viewport.zmin != _core_vwstate.viewport.zmax))
	err = 1;		/* Condition only legal if 3-D viewport zmin
				 * == zmax. */
    else if (_core_vwstate.projtype == PERSPECTIVE)
	err = make_perspective_mat();

    else
	err = make_parallel_mat();

    if (err) {
	_core_errhand("_core_make_mat", 82);
	_core_TSp = -1;
	_core_push(&_core_vwxform1[0][0]);
	return (1);
    } else {
	/* Always called once in initialization */
	/* with valid view parameters */
	_core_copytop(&_core_vwxform1[0][0]);
	ptr1 = &_core_vwxform1[0][0];
	ptr2 = &_core_vwxform32k[0][0];
	for (i = 0; i < 16; i++)
	    *ptr2++ = *ptr1++ * (float) MAX_CLIP_COORD;
	return (0);
    }
}

static int 
make_perspective_mat()
{				/* Terminology from Foley & van Dam Sect.
				 * 8.4.2 */
    float amat[4][4], bmat[4][4];
    float d1, d2;
    float pt1[4], pt2[4];
    float tvwplnz, tvrpz, tfplnz, tbplnz;
    float zvmin, twr;

    if (_core_coordsys == RIGHT)/* twr used to transform world */
	twr = 1.0;		/* coordinates to a RHS if necessary */
    else
	twr = -1.0;

    _core_identity(&amat[0][0]);
    d1 = vecleng(_core_vwstate.vwplnorm[0], _core_vwstate.vwplnorm[2], 0.0);
    d2 = vecleng(d1, _core_vwstate.vwplnorm[1], 0.0);
    amat[2][2] = amat[1][1] = d1 / d2;
    amat[2][1] = _core_vwstate.vwplnorm[1] / d2;
    amat[1][2] = -amat[2][1];
    _core_push(&amat[0][0]);	/* Rx */

    _core_identity(&amat[0][0]);
    if (d1 != 0.0) {
	amat[0][0] = amat[2][2] = (twr * -_core_vwstate.vwplnorm[2]) / d1;
	amat[2][0] = _core_vwstate.vwplnorm[0] / d1;
	amat[0][2] = -amat[2][0];
    }
    _core_push(&amat[0][0]);	/* Ry */
    (void) _core_matcon();	/* Ry*Rx */

    pt1[0] = _core_vwstate.vwupdir[0];
    pt1[1] = _core_vwstate.vwupdir[1];
    pt1[2] = twr * _core_vwstate.vwupdir[2];
    pt1[3] = 1.0;
    _core_tranpt(pt1, pt2);	/* pt2 used below to compute Rz */

    _core_pop(&bmat[0][0]);	/* bmat = RyRx */

    _core_identity(&amat[0][0]);
    amat[2][2] = -1.0;
    _core_push(&amat[0][0]);	/* Trl */

    _core_identity(&amat[0][0]);
    d1 = vecleng(pt2[0], pt2[1], 0.0);
    amat[0][0] = amat[1][1] = pt2[1] / d1;
    amat[0][1] = pt2[0] / d1;
    amat[1][0] = -amat[0][1];
    _core_push(&amat[0][0]);	/* Rz */
    (void) _core_matcon();	/* Rz*Trl */

    _core_push(&bmat[0][0]);
    (void) _core_matcon();	/* Ry*Rx*Rz*Trl  */

    _core_identity(&amat[0][0]);
    amat[3][0] = -_core_vwstate.vwrefpt[0] - _core_vwstate.projdir[0];
    amat[3][1] = -_core_vwstate.vwrefpt[1] - _core_vwstate.projdir[1];
    amat[3][2] = twr * (-_core_vwstate.vwrefpt[2] - _core_vwstate.projdir[2]);
    _core_push(&amat[0][0]);	/* Tper */

    _core_identity(&amat[0][0]);
    amat[2][2] = twr;
    _core_push(&amat[0][0]);	/* Twr: Transform world coords to RHS */

    (void) _core_matcon();	/* Twr * Tper */
    (void) _core_matcon();	/* Twr*Tper*Ry*Rx*Rz*Trl */

    pt1[0] = _core_vwstate.vwrefpt[0];
    pt1[1] = _core_vwstate.vwrefpt[1];
    pt1[2] = _core_vwstate.vwrefpt[2];

    /*
     * Since we now have Twr in the transform on the stack, we no longer need
     * to premultiply by twr 
     */
    pt1[3] = 1.0;
    _core_tranpt(pt1, pt2);	/* pt2 = VRP' = VRP*Twr Tper R Trl */

    _core_pop(&bmat[0][0]);

    tvrpz = pt2[2];
    tvwplnz = tvrpz + _core_vwstate.viewdis;
    tfplnz = tvrpz + _core_vwstate.frontdis;
    tbplnz = tvrpz + _core_vwstate.backdis;
    if ((tvwplnz <= 0.0) || (tfplnz <= 0.0) || (tbplnz < tfplnz))
	return (1);
    if (tbplnz == tfplnz)
	if (_core_vwstate.viewport.zmin != _core_vwstate.viewport.zmax)
	    return (1);

    _core_identity(&amat[0][0]);
    amat[0][0] = ((float) 2.0 * tvwplnz) /
      ((_core_vwstate.window.xmax - _core_vwstate.window.xmin) * tbplnz);
    amat[1][1] = ((float) 2.0 * tvwplnz) /
      ((_core_vwstate.window.ymax - _core_vwstate.window.ymin) * tbplnz);
    amat[2][2] = (float) 1.0 / tbplnz;
    _core_push(&amat[0][0]);	/* Sper */

    _core_identity(&amat[0][0]);
    amat[2][0] = -(pt2[0] + 0.5 *
	 (_core_vwstate.window.xmin + _core_vwstate.window.xmax)) / tvwplnz;
    amat[2][1] = -(pt2[1] + 0.5 *
	 (_core_vwstate.window.ymin + _core_vwstate.window.ymax)) / tvwplnz;
    _core_push(&amat[0][0]);	/* SHper */

    (void) _core_matcon();	/* SHper*Sper */
    _core_push(&bmat[0][0]);
    (void) _core_matcon();	/* Twr*Tper*Ry*Rx*Rz*Trl*SHper*Sper */
    _core_pop(&bmat[0][0]);

    _core_identity(&amat[0][0]);
    zvmin = tfplnz / tbplnz;
    amat[3][3] = 0.0;
    amat[2][3] = 1.0;
    if (zvmin != (float) 1.0) {
	amat[2][2] = (float) 1.0 / ((float) 1.0 - zvmin);
	amat[3][2] = -zvmin * amat[2][2];
    } else {			/* if F=B map z to zvmin */
	amat[2][2] = zvmin;
	amat[3][2] = 0.0;
    }
    _core_push(&amat[0][0]);	/* M */

    _core_push(&bmat[0][0]);
    (void) _core_matcon();	/* Twr*Tper*Ry*Rx*Rz*Trl*SHper*Sper*M */
    return (0);
}

static int 
make_parallel_mat()
{				/* Terminology from Foley & van Dam Sect.
				 * 8.4.1 */
    float amat[4][4], bmat[4][4];
    float d1, d2;
    float pt1[4], pt2[4];
    float twr;

    if (fabs(dotprod((pt_type *) & _core_vwstate.vwplnorm[0], (pt_type *) & _core_vwstate.projdir[0]))
	< 1.0e-15)
	return (1);		/* DOP perpendicular to VPN */

    if (_core_coordsys == RIGHT)
	twr = 1.0;
    else
	twr = -1.0;

    _core_identity(&amat[0][0]);
    d1 = vecleng(_core_vwstate.vwplnorm[0], _core_vwstate.vwplnorm[2], 0.0);
    d2 = vecleng(d1, _core_vwstate.vwplnorm[1], 0.0);
    amat[2][2] = amat[1][1] = d1 / d2;
    amat[2][1] = _core_vwstate.vwplnorm[1] / d2;
    amat[1][2] = -amat[2][1];
    _core_push(&amat[0][0]);	/* Rx */

    _core_identity(&amat[0][0]);
    if (d1 != 0.0) {
	amat[0][0] = amat[2][2] = (twr * -_core_vwstate.vwplnorm[2]) / d1;
	amat[2][0] = _core_vwstate.vwplnorm[0] / d1;
	amat[0][2] = -amat[2][0];
    }
    _core_push(&amat[0][0]);	/* Ry */
    (void) _core_matcon();	/* Ry*Rx */

    pt1[0] = _core_vwstate.vwupdir[0];
    pt1[1] = _core_vwstate.vwupdir[1];
    pt1[2] = twr * _core_vwstate.vwupdir[2];
    pt1[3] = 1.0;
    _core_tranpt(pt1, pt2);	/* pt2 used below to compute Rz */

    _core_pop(&bmat[0][0]);	/* bmat = RyRx */

    _core_identity(&amat[0][0]);
    amat[2][2] = -1.0;
    _core_push(&amat[0][0]);	/* Trl */

    _core_identity(&amat[0][0]);
    d1 = vecleng(pt2[0], pt2[1], 0.0);
    amat[0][0] = amat[1][1] = pt2[1] / d1;
    amat[0][1] = pt2[0] / d1;
    amat[1][0] = -amat[0][1];
    _core_push(&amat[0][0]);	/* Rz */
    (void) _core_matcon();	/* Rz*Trl */

    _core_push(&bmat[0][0]);
    (void) _core_matcon();	/* Ry*Rx*Rz*Trl  */

    pt1[0] = _core_vwstate.projdir[0];
    pt1[1] = _core_vwstate.projdir[1];
    pt1[2] = twr * _core_vwstate.projdir[2];
    pt1[3] = 1.0;
    _core_tranpt(pt1, pt2);	/* DOP' = DOP*R*Trl */
    /* pt2 used below to compute SHpar */

    _core_identity(&amat[0][0]);
    amat[3][0] = -_core_vwstate.vwrefpt[0];
    amat[3][1] = -_core_vwstate.vwrefpt[1];
    amat[3][2] = twr * -_core_vwstate.vwrefpt[2];
    _core_push(&amat[0][0]);	/* T */

    _core_identity(&amat[0][0]);
    amat[2][2] = twr;
    _core_push(&amat[0][0]);	/* Twr */

    (void) _core_matcon();	/* Twr*T */
    (void) _core_matcon();	/* Twr*T*Ry*Rx*Rz*Trl */
    _core_pop(&bmat[0][0]);

    _core_identity(&amat[0][0]);
    amat[0][0] = (float) 2.0 /
      (_core_vwstate.window.xmax - _core_vwstate.window.xmin);
    amat[1][1] = (float) 2.0 /
      (_core_vwstate.window.ymax - _core_vwstate.window.ymin);
    if (_core_vwstate.backdis == _core_vwstate.frontdis) {
	amat[2][2] = 0.0;
    } else {
	amat[2][2] = (float) 1.0 /
	  (_core_vwstate.backdis - _core_vwstate.frontdis);
    }
    amat[3][3] = (float) 1.0;
    _core_push(&amat[0][0]);	/* Spar */

    _core_identity(&amat[0][0]);
    amat[3][0] = -(_core_vwstate.window.xmax + _core_vwstate.window.xmin) / 2;
    amat[3][1] = -(_core_vwstate.window.ymax + _core_vwstate.window.ymin) / 2;
    amat[3][2] = -_core_vwstate.frontdis;
    _core_push(&amat[0][0]);	/* Tpar */
    (void) _core_matcon();	/* Tpar*Spar */

    _core_identity(&amat[0][0]);
    amat[2][0] = -(pt2[0]) / (pt2[2]);
    amat[2][1] = -(pt2[1]) / (pt2[2]);
    _core_push(&amat[0][0]);	/* SHpar */
    (void) _core_matcon();	/* SHpar*Tpar*Spar */
    _core_push(&bmat[0][0]);
    (void) _core_matcon();	/* Twr*T*Ry*Rx*Rz*Trl*SHpar*Tpar*Spar */
    return (0);
}

_core_moveword(a, b, c)
    short *a, *b, c;
{				/* move a word array, 16 bit words */
    int i;

    for (i = 0; i < c; i++)
	*b++ = *a++;
}
