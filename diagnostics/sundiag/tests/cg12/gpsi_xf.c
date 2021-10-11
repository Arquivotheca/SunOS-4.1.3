
#ifndef lint
static  char sccsid[] = "@(#)gpsi_xf.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <sys/types.h>
#include <math.h>
#include <pixrect/pixrect.h>
#include <pixrect/gp1cmds.h>
#include <esd.h>

static Matrix_3D ID = {
				    1.0, 0.0, 0.0, 0.0,
				    0.0, 1.0, 0.0, 0.0,
				    0.0, 0.0, 1.0, 0.0,
				    0.0, 0.0, 0.0, 1.0,
		       };

static Xf start_pos = {-0.5, 0.5, 0.0, 0.5, 0.5, 0.0, 0.0, 0.5, 0.0};

Matrix_3D xm;
Matrix_3D ym;
Matrix_3D zm;

#define TEST_WITH_GP2
#define XBGR(r,g,b) (((b)<<16) + ((g)<<8) + (r))
/**********************************************************************/
char *
gpsi_xf()
/**********************************************************************/
 
{
    Xf xf;
    char *build_xf_matrix();
    char *enterprise();
    char *errmsg;
    int nrot = 90;
    float ri = M_PI/(float)nrot;
    int i;
    int color;

    func_name = "gpsi_xf";
    TRACE_IN

    /* extract test images from tar file */
    (void)xtract(GPSI_XF_CHK);

    /* clear screen */
    clear_all();

    /* reset the transformation matrix */
    bcopy(&ID, &xm, sizeof(Matrix_3D));
    bcopy(&ID, &ym, sizeof(Matrix_3D));
    bcopy(&ID, &zm, sizeof(Matrix_3D));
    bcopy(&start_pos, &xf, sizeof(Xf));

    color = 0;
    for (i = 0 ; i < nrot ; i++) {

	/*
	errmsg = ctx_set(CTX_SET_MAT_NUM, 1, 0);
	if (errmsg) {
	    TRACE_OUT
	    return errmsg;
	}
	*/

	/* set matrix */
	errmsg = build_xf_matrix(&xf);
	if (errmsg) {
	    TRACE_OUT
	    return errmsg;
	}

	/*
	errmsg = enterprise();
	if (errmsg) {
	    TRACE_OUT
	    return errmsg;
	}
	*/

	errmsg = ctx_set(CTX_DONT_CLEAR,
		    CTX_SET_MAT_NUM, 1,
		    CTX_SET_RGB_COLOR, XBGR(color, i/2, 0), 0);
	if (errmsg) {
	    TRACE_OUT
	    return errmsg;
	}

	errmsg = enterprise();
	if (errmsg) {
	    TRACE_OUT
	    return errmsg;
	}

	color = i * (int)(255.0/(float)nrot);

	xf.xr += ri / 1.052;
	xf.xs += 0.00080094;
	xf.xt += 0.010234;

	xf.yr += ri / 7.8967;
	xf.ys += 0.00180094;
	xf.yt -= 0.010746;

	xf.zr -= ri / 3.50986;
	xf.zs += 0.0;
	xf.zt += 0.0;

    }

    errmsg = ctx_set(CTX_DONT_CLEAR, CTX_SET_MAT_NUM, 1, 0);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }
    errmsg = enterprise();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = chksum_verify(GPSI_XF_CHK);

    TRACE_OUT
    return errmsg;
}

/**********************************************************************/
char *
build_xf_matrix(xf)
/**********************************************************************/
Xf *xf;
/* builds tranformation matrix for transformations. Assumming the
   X transformation, followed by the Y transformation, followed
   by the Z transformation.
   The result matrix is placed in slot 1.
 */
{
    char *matrix_mult();
    char *errmsg;
    int a;
    int b;
    int c;

    func_name = "build_xf_matrix";
    TRACE_IN

    /* Transformation in X and around X axis */
    xm.m[0][0] = xf->xs;
    xm.m[3][0] = xf->xt;
    xm.m[1][1] = (float)cos((double)xf->xr);
    xm.m[1][2] = (float)sin((double)xf->xr);
    xm.m[2][1] = -(float)sin((double)xf->xr);
    xm.m[2][2] = (float)cos((double)xf->xr);

    /* Transformation in Y and around Y axis */
    ym.m[1][1] = xf->ys;
    ym.m[3][1] = xf->yt;
    ym.m[0][0] = (float)cos((double)xf->yr);
    ym.m[2][0] = (float)sin((double)xf->yr);
    ym.m[0][2] = -(float)sin((double)xf->yr);
    ym.m[2][2] = (float)cos((double)xf->yr);

    /* Transformation in Z and around Z axis */
    zm.m[2][2] = xf->zs;
    zm.m[3][2] = xf->zt;
    zm.m[0][0] = (float)cos((double)xf->zr);
    zm.m[0][1] = (float)sin((double)xf->zr);
    zm.m[1][0] = -(float)sin((double)xf->zr);
    zm.m[1][1] = (float)cos((double)xf->zr);


    /* set matrix 5 = Z-matrix */
    errmsg = set_3D_matrix(5, &zm);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set matrix 4 = Y-matrix */
    errmsg = set_3D_matrix(4, &ym);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* multiply ZY, store result in matrix 3 */
    a = 5; b = 4, c =3;
    errmsg = matrix_mult(a, b, c);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set matrix 2 = X-matrix */
    errmsg = set_3D_matrix(2, &xm);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* multiply ZYX, store result in matrix 1 */
    a = 3; b = 2, c =1;
    errmsg = matrix_mult(a, b, c);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return (char *)0;

}

/**********************************************************************/
char *
matrix_mult(ma, mb, mc)
/**********************************************************************/
int ma;
int mb;
int mc;
/* lets the GP perform the matrix multiplication mc = ma X mb.
   ma, mb, mc are the matrix indexes.
 */

{
    register short *p;
    short a;
    short b;
    short c;
    int res;

    func_name = "matrix_mult";
    TRACE_IN

    a = (short)ma; b = (short)mb; c = (short)mc;

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    *p++ = GP1_MUL_MAT_3D;
    *p++ = a;
    *p++ = b;
    *p++ = c;

    res = postbuf(p);
    if (res == -1) {
	TRACE_OUT
	return (errmsg_list[12]);
    }
    
    TRACE_OUT
    return (char *)0;
}
