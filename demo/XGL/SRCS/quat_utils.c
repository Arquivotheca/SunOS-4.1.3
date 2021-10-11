#ifndef lint
static char sccsid[] = "@(#)quat_utils.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif

#include <stdio.h>
#include <math.h>
#include "P_include.h"

/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_mk_ctx:
 *		This primitive creates a Patk3d context in memory and sets
 *		some default values for the user's parameters.
 *
 *		The user position is set to [1,0,0], looking in the
 *		direction of the decreasing x axis.
 *
 *		The lens is set to [0,0,0] which is a direct projection, without
 *		any perspective effect.
 *
 *		The axis orientations matrix is set to
 *			(0  0 -1  0)
 *			(1  0  0  0) (Sun system).
 *			(0 -1  0  0)
 *			(0  0  0  1)
 *
 */

P_ctx
  P_mk_ctx()

{
P_ctx		C;
int		i;

/*
 * Allocate the memory space for the P3_internxgl_ctx storage
 */
  if (C = (P_ctx)malloc(sizeof(P3_internxgl_ctx))) {
/*
 * Initialize the context with default values
 */
    for (i = 0; i < 3; i++) {
      C->O.pos[i] = 0.;
      C->O.rop[i] = 0.;
    }
    C->O.rop[3] = 1.;
    C->O.pos[0] = -1.;
    C->L.g = 0.;
    C->L.x = 0.;
    C->L.y = 0.;
    C->S.ori[0][0] = 0.0;
    C->S.ori[0][1] = 0.0;
    C->S.ori[0][2] = -1.0;
    C->S.ori[0][3] = 0.0;
    C->S.ori[1][0] = 1.0;
    C->S.ori[1][1] = 0.0;
    C->S.ori[1][2] = 0.0;
    C->S.ori[1][3] = 0.0;
    C->S.ori[2][0] = 0.0;
    C->S.ori[2][1] = -1.0;
    C->S.ori[2][2] = 0.0;
    C->S.ori[2][3] = 0.0;
    C->S.ori[3][0] = 0.0;
    C->S.ori[3][1] = 0.0;
    C->S.ori[3][2] = 0.0;
    C->S.ori[3][3] = 1.0;
/*
 * Compute internal matrices
 */
    P3_quat_mat(C);
    P3_lens_mat(C);
    P3_matrix(C);
  } else {
    P3_set_error("P_mk_ctx: Unable to create context");
  }
  return (C);
}

P3_set_error(s)
char	*s;
{
  printf("%s\n",s);
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_set_pos:
 *		This primitive updates a Patk3d context in memory by setting
 *		new values for the observer eye's position in the space.
 *		The transformation matrices are also updated.
 *
 */


P_set_pos(C,x,y,z)
P_ctx	C;
float	x, y, z;

{
int	i;

/*
 * Set the values of the eye's position for context C
 */
  C->O.pos[0] = -x;
  C->O.pos[1] = -y;
  C->O.pos[2] = -z;
/*
 * Force the visualisation to be in the decreasing x axis
 */
  C->O.rop[3] = 1.;
  for (i = 0; i < 3; i++) C->O.rop[i] = 0.;
/*
 * Update the internal matrices
 */
  P3_quat_mat(C);
  P3_matrix(C);
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_set_lens:
 *		This primitive updates a Patk3d context in memory by setting
 *		the new values for the optical parameters. 
 *		The transformation matrices are also updated.
 *
 */


P_set_lens(C,g,x,y)
P_ctx	C;
float	g, x, y;

{

/*
 * Set the values of the optical lens for context C
 */
  C->L.g = g;
  C->L.x = x;
  C->L.y = y;
/*
 * Update the internal matrices
 */
  P3_lens_mat(C);
  P3_matrix(C);
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_set_orient:
 *		This primitive updates a Patk3d context in memory by setting
 *		new values for the DC axes orientation parameters.
 *
 *		A 3x3 matrix describes how each unit vector of the user
 *		coordinate system is transformed to match the Sun default
 *		orientation.
 *
 *		By default, (1, 0, 0) is transformed to be (0, 0, -1)
 *		            (0, 1, 0) .....................(1, 0, 0)
 *		            (0, 0, 1) .....................(0, -1, 0)
 *
 *		The transformation matrices are also updated.
 *
 */


P_set_orient(C,user_ori)
P_ctx	C;
float	user_ori[3][3];

{
int	i, j;
/*
 * Set the values of the orientation of each axis for context C
 */
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      C->S.ori[i][j] = user_ori[i][j];
/*
 * Update the internal matrices
 */
  P3_lens_mat(C);
  P3_matrix(C);
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_look_to:
 *		This primitive updates a Patk3d context in memory by setting
 *		new values for the observer vision direction in the space. This
 *		is done by giving a vision point in the space. The vision 
 *		direction goes from the observer eye's position to this point.
 *		The transformation matrices are also updated.
 *
 */


P_look_to(C,x,y,z)
P_ctx	C;
float	x, y, z;

{
float			P[4], Q[4];
float			R, R0, C0, X, Y;
int			i, j;

/*
 * Compute the visualisation direction according to C->O.pos and x, y, z.
 *
 * We need to transform the vision point.
 */
  Q[0] = x;
  Q[1] = y;
  Q[2] = z;
  Q[3] = 1.;
  for (i = 0; i < 4; i++) {
    P[i] = 0.;
    for (j = 0; j < 4; j++)
      P[i] -= Q[j] * C->M_quat[j][i];
  }
/*
 * compute the new quaternion
 */
  R  = P[0] * P[0] + P[1] * P[1];
  R0 = sqrt(R + P[2] * P[2]);
/*
 * look for special or error cases
 *
 * R and R0 are positive!
 */
  if (R > EPSILON) {
/*
 * This is the *normal* case, we need two rotations...
 */
    R  = sqrt(R);
    C0 = P[0] / R;
    X = sqrt((1. + C0) / 2.);
    Y = sqrt((1. - C0) / 2.);
    if (P[1] < 0.) Y = -Y;
    P3_rot_mov(C,X,Y,2,-1);
    C0 = R / R0;
    X  = sqrt((1. + C0) / 2.);
    Y  = sqrt((1. - C0) / 2.);
    if (P[2] > 0.) Y = -Y;
    P3_rot_mov(C,X,Y,1,-1);
  } else {
/*
 * If R0 is null... That means the two points are the same!
 */
    if (R0 < EPSILON) {
      P3_set_error("Error: Vision and eye's positions are the same");
      return;
    }
/*
 * Here, the vision position is vertical to the eye's position.
 * We decide to move the vision direction up or down of 90 degrees,
 * upon the sign of the z component.
 */
    X = Y = .7071068;		/* sin(45) */
    if (P[2] > 0.) Y = -Y;
    P3_rot_mov(C,X,Y,1,-1);
  }
/*
 * Update the internal matrices
 */
  P3_quat_mat(C);
  P3_matrix(C);
}



/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_tra_mov:
 *		This function computes translation movments for a given
 *		quaternion.
 *
 *		"i" is the axis of the translation, while x is the parameter
 *		that caracterize the translation for the quaternion of the
 *		context C.
 *
 *		"w" is set to -1 if the observer moves in respect
 *		to the scene, 1 if the scene moves in respect to
 *		the observer.
 *
 */


P3_tra_mov(C,x,i,w)

P_ctx	C;
float	x;
int	i, w;

{
int	j, k;
float	A, B, D, E, F;


  if (w < 0) {
/*
 * The observer moves in respect to the scene.
 */
    C->O.pos[i] -= x;
  } else {
/* 
 * The scene moves in respect to the observer.
 * Compute the successor axis of i [0,1,2];
 * and then the successor axis of j [0,1,2];
 */
    if ((j = i + 1) > 2) j = 0;
    if ((k = j + 1) > 2) k = 0;

    A = C->O.rop[j];
    B = C->O.rop[k];
    F = C->O.rop[3];
    E = C->O.rop[i];
    C->O.pos[i] += x * (E * E + F * F - A * A - B * B);
    D = x + x;
    C->O.pos[j] += D * (E * A + F * B);
    C->O.pos[k] += D * (E * B + F * A);
  }
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_rot_mov:
 *		This function computes rotation based movments for a given
 *		quaternion.
 *
 *		"i" is the axis of the rotation, while x and y are the
 *		parameters that caracterize the rotation for the quaternion
 *		of the context C.
 *
 *		"w" is set to -1 if the observer moves in respect
 *		to the scene, 1 if the scene moves in respect to
 *		the observer.
 *
 *		"x" and "y" are typically the cosinus and sinus of the half
 *		rotation angle.
 *
 */


P3_rot_mov(C,x,y,i,w)

P_ctx	C;
float	x, y;
int	i, w;

{
int	j, k;
float	E, F, R1;

/*
 * Compute the successor axis of i [0,1,2];
 * and then the successor axis of j [0,1,2];
 */
  if ((j = i + 1) > 2) j = 0;
  if ((k = j + 1) > 2) k = 0;

  E = C->O.rop[i];
  C->O.rop[i] = E * x + w * y * C->O.rop[3];
  C->O.rop[3] = C->O.rop[3] * x - w * y * E;

  E = C->O.rop[j];
  C->O.rop[j] = E * x + y * C->O.rop[k];
  C->O.rop[k] = C->O.rop[k] * x - y * E;

  if (w < 0) {
/*
 * As the observer moves in respect to the scene, we
 * need to compute the new scene's position in respect to
 * the observer.
 */
    R1 = x * x - y * y;
    F = 2. * x * y;
    E = C->O.pos[j];
    C->O.pos[j] = E * R1 + F * C->O.pos[k];
    C->O.pos[k] *= R1;
    C->O.pos[k] -= F * E;
  }
}



/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P_3D_move:
 *		This primitive updates a Patk3d context in memory by setting
 *		new values for the observer eye's position in the space.
 *
 *		It gives to the user the capability to move in the 6 
 *		liberty axes. The following  movement codes are recognized:
 *
 *			rr a:	roll right of angle a in degrees.
 *			rl a:	roll left of angle a in degrees.
 *			pf a:	pitch forward of angle a in degrees.
 *			pb a:	pitch backward of angle a in degrees.
 *			yr a:	yaw right of angle a in degrees.
 *			yl a:	yaw left of angle a in degrees.
 *
 *			fw x:	forward movement of x units.
 *			bw x:	backward movement of x units.
 *			rt x:	right movement of x units.
 *			lt x:	left movement of x units.
 *			up x:	up movement of x units.
 *			dn x:	down movement of x units.
 *
 *		The anglular values should be [-90..+90].
 *
 *		The transformation matrices are also updated.
 *
 */


P_3D_move(C,desc)
P_ctx	C;
char	*desc;

{
int	i, j, moved, called, axe;
char	ch[4];
float	val, x, y;

  moved = 0;
  i = 0;
  j = 0;
/*
 * Read the descriptor, and for each recognized code, select the
 * correct call to either P3_tra_mov or P3_rot_mov to update the
 * quaternion.
 */
  while (sscanf(&desc[j],"%s%n",ch,&i) != EOF) {
    switch (ch[0]) {
      case 'r' :
/*
 *	rr a:	roll right of angle a in degrees.
 *	rl a:	roll left of angle a in degrees.
 * 	rt x:	right movement of x units.
 */
        switch (ch[1]) {
          case 'r' : called = 1; axe = -1; break;
          case 'l' : called = 1; axe = 1; break;
          case 't' : called = 2; axe = 2; break;
          default :  called = 0; break;
        }
        break;
/*
 *	pf a:	pitch forward of angle a in degrees.
 *	pb a:	pitch backward of angle a in degrees.
 */
      case 'p' :
        switch (ch[1]) {
          case 'f' : called = 1; axe = -2; break;
          case 'b' : called = 1; axe = 2; break;
          default :  called = 0; break;
        }
        break;
/*
 *	yr a:	yaw right of angle a in degrees.
 * 	yl a:	yaw left of angle a in degrees.
 */
      case 'y' :
        switch (ch[1]) {
          case 'r' : called = 1; axe = -3; break;
          case 'l' : called = 1; axe = 3; break;
          default :  called = 0; break;
        }
        break;
/*
 * 	fw x:	forward movement of x units.
 * 	bw x:	backward movement of x units.
 * 	lt x:	left movement of x units.
 * 	up x:	up movement of x units.
 * 	dn x:	down movement of x units.
 */
      case 'f' : called = 2; axe = -1; break;
      case 'b' : called = 2; axe = 1; break;
      case 'l' : called = 2; axe = -2; break;
      case 'u' : called = 2; axe = 3; break;
      case 'd' : called = 2; axe = -3; break;
      default:
        called = 0;
      break;
    }
    j += i;
    if ((called) && (sscanf(&desc[j],"%f%n",&val,&i) != EOF )) {
      if (axe < 0) {
        val = -val;
        axe = -axe - 1;
      } else {
        axe -= 1;
      }
      if (called == 1) {
/*
 * Convert val as the half angle, in radians.
 * radiand = degree * PI / 180, and /2 for half angle.
 */
        val /= 114.591559026;
        x = cos(val);
        y = sin(val);
        P3_rot_mov(C,x,y,axe,-1);
      } else {
        P3_tra_mov(C,val,axe,-1);
      }
      moved = 1;
      j += i;
    } else {
      P3_set_error("Error: In P_3D_move descriptor, near: rr");
    }
  }
/*
 * Update the internal matrices
 */
  if (moved) {
    P3_quat_mat(C);
    P3_matrix(C);
  }
}



/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_quat_mat:
 *		This primitive (re)computes the matrix that corresponds
 *		to the observer's eye position and direction.
 *
 *		This is done by using the quaternion definition of the 
 *		context visualisation direction and the position of the
 *		eye in the WC space.
 *
 */

P3_quat_mat(C)
P3_internxgl_ctx		*C;

{

float	e, f, r[4];
int	i, j, k;


/*
 * We will need some square values!
 */
  for (i = 0; i < 4; i++) r[i] = C->O.rop[i] * C->O.rop[i];
/*
 * Compute each element of the matrix.
 * j is the successor of i (in 0, 1, 2), while k is the successor of j.
 */
  for (i = 0; i < 3; i++) {
    if ((j = i + 1) > 2) j = 0;
    if ((k = j + 1) > 2) k = 0;
    e = 2. * C->O.rop[i] * C->O.rop[j];
    f = 2. * C->O.rop[k] * C->O.rop[3];
    C->M_quat[j][i] = e - f;
    C->M_quat[i][j] = e + f;
    C->M_quat[i][i] = r[i] + r[3] - r[j] - r[k];
    C->M_quat[3][i] = C->O.pos[i];
    C->M_quat[i][3] = 0.;
  }
  C->M_quat[3][3] = 1.;
}



/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_lens_mat:
 *		This primitive (re)computes the matrix that corresponds
 *		to the observer's lens system -also called optical trans
 *		formation- for the perspective projection.
 *		It combines the lens itself, but also the orientation to apply
 *		to the components of the viewing system.
 *
 *
 *
 */


P3_lens_mat(C)
P3_internxgl_ctx		*C;

{
float	mat[4][4];
int	i;

/*
 * Compute each element of the matrix.
 */
  for (i = 0; i < 4; i++) {
    mat[i][0] = 0.0;
    mat[i][1] = 0.0;
    mat[i][2] = 0.0;
    mat[i][3] = 0.0;
    mat[i][i] = 1.0;
  }
  mat[2][0] = C->L.x;
  mat[2][1] = C->L.y;
  mat[2][3] = C->L.g;
  P3_u_4x4(C->M_lens,C->S.ori,mat);
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_u_4x4:
 *		Utility function: 4x4 matrix product.
 *
 */

P3_u_4x4(m1,m2,m3)
float	m1[4][4], m2[4][4], m3[4][4];

{
int	i, j, k;
/*
 * Compute the 4x4 matrix product m1 = m2 * m3
 */
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      m1[i][j] = 0.;
      for (k = 0; k < 4; k++)
        m1[i][j] += m2[i][k] * m3[k][j];
    }
  }
}


/*
 *	Sun Microsystems, inc.
 *	Graphics Product Division
 *
 *	Date:	January 11th, 1989
 *	Author:	Patrick-Gilles Maillot
 * 
 *	P3_matrix:
 *		This primitive (re)computes the matrix that corresponds
 *		to ALL the transformations to be executed from the WC,
 *		to the DC space, including the optical projection.
 *
 *	M_all = M_quat * M_lens
 *
 */

P3_matrix(C)
P3_internxgl_ctx		*C;

{
int	i, j, k;
float	r;

  P3_u_4x4(C->M_all,C->M_quat,C->M_lens);
/*
 * Compute the M_all optimization flag matrices:
 *	M_of_0[i,j] = 0 if M_all[i,j] = 0.
 *		    = 1 else.
 *	M_of_1[i,j] = 0 if M_all[i,j] != 1.
 *		    = 1 else.
 *
 *	Note that M_of_x[i][j] is evaluated as the bit ((1 << j) << i)
 *	of M_of_x.
 */
  k = 1;
  C->M_of_1 = 0;
  C->M_of_0 = 0;
  for (i = 0; i < 4; i++) {
    for (j = 0; j < 4; j++) {
      r = C->M_all[i][j];
      if ((r < (1. + EPSILON)) && (r > (1. - EPSILON))) {
        C->M_of_1 |= k;
      } else {
        if ((r > EPSILON) || (r < -EPSILON)) C->M_of_0 |= k;
      }
      k <<= 1;
    }
  }
}
