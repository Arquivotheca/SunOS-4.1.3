#ifndef lint
static char sccsid[] = "@(#)view_3d_set.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */

/*
 *	view_3d_set.c:
 *		Collection of primitives to build the 3D view matrix.
 *		The 3D view matrix is defined as TAP, T is the eye position
 *		matrix (the origin's position in respect to the eye), A is
 *		the viewing direction matrix, and P is the optical matrix,
 *		setting any kind of centered/parallel perpective.
 */
#include <math.h>

#include <xgl/xgl.h>
#define EPSI	0.0001


Xgl_pt_f3d	vrp;		/* view reference point	*/
Xgl_pt_f3d	per;		/* perspective vector	*/


/*
 *	xgli_set_perspective:
 *		Set the user's perspective to the values in <ref_per>.
 *		ref_per.z is the inverse of the focal distance ratio.
 *			A value of 0.0 gives an infinite (parallel) perspective.
 *			A value of 0.4 to 0.6 is correct for almost of the
 *			cases.
 *			A value of more than 1.0 gives wide angles effects.
 *			A negative value shows what's behind you!
 *		ref_per.x/y are used as the parallel projection vector when
 *			ref_per.z == 0.0, and to produce non centered
 *			perspective otherwise.
 */
void
xgli_set_perspective(ref_per)
Xgl_pt_f3d	*ref_per;
{
  per = *ref_per;
}

/*
 *	xgli_set_eye_position:
 *		Set the user's eye position at point <ref_point>.
 *		The reference position is kept as a global variable.
 */
void
xgli_set_eye_position(ref_point)
Xgl_pt_f3d	*ref_point;
{
  vrp = *ref_point;
}

/*
 *	xgli_look_at_point:
 *		Compute the view matrix using <vrp>, <per> and the point given
 *		in parameter. In this case, we keep the up vector as <0,1,0>.
 */
int
xgli_look_at_point(point, mout)
Xgl_pt_f3d	*point;		/* point to look at	*/
float		mout[4][4];	/* OUT view orientation matrix	*/
{
Xgl_pt_f3d	vpn, vup;
double		s;

  vpn.x = point->x - vrp.x;
  vpn.y = point->y - vrp.y;
  vpn.z = point->z - vrp.z;

  vup.x = 0.0;
  vup.y = 1.0;
  vup.z = 0.0;

  return (xgli_set_3d_view(&vrp, &vpn, &vup, &per, mout));
}


/*
 *	xgli_set_3d_view:
 *		Compute the view matrix in the general case.
 */
int
xgli_set_3d_view(vrp, vpn, vup, per, mout)
Xgl_pt_f3d	*vrp;		/* view reference point	*/
Xgl_pt_f3d	*vpn;		/* view plane normal	*/
Xgl_pt_f3d	*vup;		/* view up vector	*/
Xgl_pt_f3d	*per;		/* perspective vector	*/
float		mout[4][4];	/* OUT view orientation matrix	*/
{

/*  Translate to VRP then change the basis.
 *  The old basis is: e1 = < 1, 0, 0>,  e2 = < 0, 1, 0>, e3 = < 0, 0, 1>.
 * The new basis is: ("x" means cross product)
 *	e3' = VPN / |VPN|
 *	e1' = VUP x VPN / |VUP x VPN|
 *	e2' = e3' x e1'
 * Therefore the transform from old to new is x' = TAPx, where:
 *
 *	     | e1'x e2'x e3'x 0 |         |   1      0      0      0 |
 *	 A = | e1'y e2'y e3'y 0 |,    T = |   0      1      0      0 |
 *	     | e1'z e2'z e3'z 0 |         |   0      0      1      0 |
 *	     |  0    0    0   1 |         | -vrp.x -vrp.y -vrp.z   1 |
 *
 *           |  1    0    0    0  |
 *       P = |  0    1    0    0  |
 *           |  a    b    1   1/f |
 *           |  0    0    0    1  |
 */

/*
 * These ei's are really ei primes.
 */
register float	(*m)[4][4];
Xgl_pt_f3d	e1, e2, e3, e4;
double		s, v;

/*
 * e1' = VUP x VPN / |VUP x VPN|, but do the division later.
 */
  e1.x = vup->y * vpn->z - vup->z * vpn->y;
  e1.y = vup->z * vpn->x - vup->x * vpn->z;
  e1.z = vup->x * vpn->y - vup->y * vpn->x;
  s = sqrt( e1.x * e1.x + e1.y * e1.y + e1.z * e1.z);
  e3.x = vpn->x;
  e3.y = vpn->y;
  e3.z = vpn->z;
  v = sqrt( e3.x * e3.x + e3.y * e3.y + e3.z * e3.z);
/*
 * Check for vup and vpn colinear (zero dot product).
 */
  if ((s > -EPSI) && (s < EPSI))
    return (2);
/* 
 * Check for a normal vector not null.
 */
  if ((v > -EPSI) && (v < EPSI))
    return (3);
  else {
/*
 * Normalize e1
 */
    e1.x /= s;
    e1.y /= s;
    e1.z /= s;
/*
 * e3 = VPN / |VPN|
 */
    e3.x /= v;
    e3.y /= v;
    e3.z /= v;
/*
 * e2 = e3 x e1
 */
    e2.x = e3.y * e1.z - e3.z * e1.y;
    e2.y = e3.z * e1.x - e3.x * e1.z;
    e2.z = e3.x * e1.y - e3.y * e1.x;
/*
 * Add the translation
 */
    e4.x = -( e1.x * vrp->x + e1.y * vrp->y + e1.z * vrp->z);
    e4.y = -( e2.x * vrp->x + e2.y * vrp->y + e2.z * vrp->z);
    e4.z = -( e3.x * vrp->x + e3.y * vrp->y + e3.z * vrp->z);
/*
 * Homogeneous entries
 *
 *  | e1.x  e2.x  e3.x  0.0 |   | 1  0  0  0 |
 *  | e1.y  e2.y  e3.y  0.0 | * | 0  1  0  0 |
 *  | e1.z  e2.z  e3.z  0.0 |   | a  b  1  c |
 *  | e4.x  e4.y  e4.z  1.0 |   | 0  0  0  1 |
 */

    m = (float (*)[4][4])mout;

    (*m)[0][0] = e1.x + e3.x * per->x;
    (*m)[0][1] = e2.x + e3.x * per->y;
    (*m)[0][2] = e3.x;
    (*m)[0][3] = e3.x * per->z;

    (*m)[1][0] = e1.y + e3.y * per->x;
    (*m)[1][1] = e2.y + e3.y * per->y;
    (*m)[1][2] = e3.y;
    (*m)[1][3] = e3.y * per->z;

    (*m)[2][0] = e1.z + e3.z * per->x;
    (*m)[2][1] = e2.z + e3.z * per->y;
    (*m)[2][2] = e3.z;
    (*m)[2][3] = e3.z * per->z;

    (*m)[3][0] = e4.x + e4.z * per->x;
    (*m)[3][1] = e4.y + e4.z * per->y;
    (*m)[3][2] = e4.z;
    (*m)[3][3] = 1.0 + e4.z * per->z;

  }
  return (0);
}
