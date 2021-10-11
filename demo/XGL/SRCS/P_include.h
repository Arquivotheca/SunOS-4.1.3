/* 
 * @(#)P_include.h 1.1 92/07/30 Copyr 1989, 1990 Sun Micro
 */

#define	EPSILON	.0001

typedef struct {
  float		pos[3];		/* The [x,y,z] of the 3D eye position, in WC */
  float		rop[4];		/* The quaternion associated to the eye in WC */
} P3_internxgl_quat;


typedef struct {
  float		ori[4][4];	/* The orientation matrix for visualization */
} P3_visualize_sys;


typedef struct {
  float		g;		/* The type of lens:			*/
				/*	g != 0: real perspective,	*/
				/*		x & y used for eye devi */
				/*		ation.			*/
				/*	g = 0:  linear projection,	*/
				/*		x & y define the projec */
				/*		tion vector.		*/
  float		x;		/* The x lens value */
  float		y;		/* The y lens value */
} P3_internxgl_lens;


typedef struct {
  float		coords[4];	/* The [w,x,y,z] coordsinates of a 3D point */
} P3_internxgl_point;


typedef struct {
  P3_internxgl_quat	O;		/* The eye's position & direction */
  P3_visualize_sys	S;		/* The window scale factors */
  P3_internxgl_lens	L;		/* The lens for perspective */
  float		M_quat[4][4];		/* The eye's resulting matrix */
  float		M_lens[4][4];		/* The optical resulting matrix */
  float		M_all[4][4];		/* The total transformation matrix */
  unsigned short	M_of_0;		/* The optimization flags toward 0 */
  unsigned short	M_of_1;		/* The optimization flags toward 1 */
} P3_internxgl_ctx, *P_ctx;

P_ctx		P_mk_ctx();

