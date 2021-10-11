
#ifndef lint
static  char sccsid[] = "@(#)gpsi_3D_lines.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <math.h>
#include <pixrect/gp1cmds.h>
#include <esd.h>

#define BATCH_SIZE	1600

extern struct vector Enterprise[];

static Matrix_3D mat3d = {
			    1.0, 0.0, 0.0, 0.0,
			    0.0, 1.0, 0.0, 0.0,
			    0.4, 0.2, 1.0, 0.0,
			    0.0, 0.0, 0.0, 1.0
			};

static short textptr[3] = {10, 5, 0};

/**********************************************************************/
char *
gpsi_3D_lines()
/**********************************************************************/
 
{
    char * enterprise();
    char *vector_lissa();
    char *shaded_vector_lissa();
    char *centrics();
    char *textured_hexnut();

    char *errmsg;
    double ri;

    func_name = "gpsi_3D_lines";
    TRACE_IN

    /* clear all planes */
    clear_all();

    /* extract test images from tar file */
    (void)xtract(GPSI_3D_LINES_CHK);

    /* set context */
    errmsg = ctx_set(0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    errmsg = centrics();
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    errmsg = chksum_verify(GPSI_3D_LINES_CHK);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    (void)xtract(GPSI_3D_SHAD_LINES_CHK);
    /* clear the 24-bit plane */
    clear_24bit_plane();

    ri = 8.5;
    errmsg = shaded_vector_lissa(ri);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    errmsg = chksum_verify(GPSI_3D_SHAD_LINES_CHK);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    (void)xtract(GPSI_TEXTURED_LINES_CHK);
    /* clear the 24-bit plane */
    clear_24bit_plane();

    errmsg = ctx_set(CTX_SET_HIDDEN_SURF, GP1_ZBALL,
			CTX_SET_LINE_TEX, textptr, 0, 0,
			CTX_SET_MAT_NUM, 1,
			CTX_SET_MAT_3D, 1, &mat3d,
			CTX_SET_DEPTH_CUE, GP1_DEPTH_CUE_ON,
			CTX_SET_RGB_COLOR, 0x00ff00,
			CTX_SET_LINE_WIDTH, 5, 0,
    			0);
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }

    errmsg = textured_hexnut();
    if (errmsg) {
	TRACE_OUT
	return errmsg;
    }
    errmsg = chksum_verify(GPSI_TEXTURED_LINES_CHK);

    TRACE_OUT
    return(errmsg);

}

/**********************************************************************/
char *
enterprise()
/**********************************************************************/
/* draws the space ship Enterprise */
{
    register struct vector *vptr;
    register short *p;
    register int i;
    register short *ps;
    int res;

    func_name = "enterprise";
    TRACE_IN

    vptr = Enterprise;
    /*
    vptr = f15;
    */

    for(;;) {
	p = allocbuf();
	if (p == (short *)-1) {
	    TRACE_OUT
	    return (errmsg_list[52]);
	}
	*p++ = GP1_XF_LINE_FLT_3D;
	for (i = 0 ; i < BATCH_SIZE ; i++) {
	    ps = p;
	    *p++ = vptr->ctrl_field;
	    GP1_PUT_F(p, vptr->x);
	    GP1_PUT_F(p, vptr->y);
	    GP1_PUT_F(p, vptr->z);

	    if (vptr->ctrl_field == 0x8000)
		break;
	    else
		vptr++;
	}
	*ps = 0x8000;
	res = postbuf(p);
	if (res == -1) {
	    TRACE_OUT
	    return (errmsg_list[12]);
	}
	if (vptr->ctrl_field == 0x8000)
	    break;
    }
    TRACE_OUT
    return (char *)0;
}

/**********************************************************************/
char *
vector_lissa(inc)
/**********************************************************************/
double inc;
/*draws lissajou figure with vector command */
{

    double cos();
    double sin();

    register double rd;
    register int npts = 360;
    register short *p;
    register short *vp;
    register int k;
    float aux;
    int res;

    func_name = "vector_lissa";
    TRACE_IN

	rd = inc;

	p = allocbuf();
	if (p == (short *)-1) {
	    TRACE_OUT
	    return (errmsg_list[52]);
	}

	*p++ = GP1_XF_LINE_FLT_3D;
	for (k = 0 ; k < npts ; k++) {
	    vp = p;
	    *p++ = 0x0;
	    aux=(float)cos(rd);
	    GP1_PUT_F(p, aux);
	    aux=(float)sin(rd);
	    GP1_PUT_F(p, aux);
	    aux=(float)k/(float)npts;
	    GP1_PUT_F(p, aux);
	    rd += inc;
	}
	*vp =0x8000;
	res = postbuf(p);
	if (res == -1) {
	    TRACE_OUT
	    return (errmsg_list[12]);
	}

	TRACE_OUT
	return (char *)0;
}

/**********************************************************************/
char *
dc_vector_lissa(inc)
/**********************************************************************/
double inc;
/*draws lissajou figure with vector command */
{
    

    double cos();
    double sin();

    register double rd;
    register int npts = 360;
    register short *p;
    register short *vp;
    register int k;
    float aux;
    float ONE = 1.0;
    float ZERO = 0.0;
    int res;

    func_name = "vector_lissa";
    TRACE_IN

	rd = inc;

	p = allocbuf();
	if (p == (short *)-1) {
	    TRACE_OUT
	    return (errmsg_list[52]);
	}

	*p++ = GP1_SET_DEPTH_CUE | GP1_DEPTH_CUE_ON;

	*p++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_TRIPLE;
	GP1_PUT_F(p, ONE);
	GP1_PUT_F(p, ONE);
	GP1_PUT_F(p, ONE);

	*p++ = GP2_SET_DEPTH_CUE_PARAMETERS;
	GP1_PUT_F(p, ONE);   
	GP1_PUT_F(p, ZERO);

	GP1_PUT_F(p, ZERO);
	GP1_PUT_F(p, ONE);

	GP1_PUT_F(p, ZERO);
	GP1_PUT_F(p, ZERO);
	GP1_PUT_F(p, ZERO);

	*p++ = GP1_XF_LINE_FLT_3D;
	for (k = 0 ; k < npts ; k++) {
	    vp = p;
	    *p++ = 0x0;
	    aux=(float)cos(rd);
	    GP1_PUT_F(p, aux);
	    aux=(float)sin(rd);
	    GP1_PUT_F(p, aux);
	    aux=(float)k/(float)npts;
	    GP1_PUT_F(p, aux);
	    rd += inc;
	}
	*vp =0x8000;
	res = postbuf(p);
	if (res == -1) {
	    TRACE_OUT
	    return (errmsg_list[12]);
	}

	TRACE_OUT
	return (char *)0;
}

/**********************************************************************/
char *
shaded_vector_lissa(inc)
/**********************************************************************/
double inc;
/*draws lissajou figure with vector command */
{
    

    register double rd;
    register int npts = 360;
    register short *p;
    register short *vp;
    register int k;
    float r;
    float g;
    float b;
    int res;

    func_name = "shaded_vector_lissa";
    TRACE_IN

	rd = inc;

	p = allocbuf();
	if (p == (short *)-1) {
	    TRACE_OUT
	    return (errmsg_list[52]);
	}

	*p++ = GP2_XF_SHADE_LINE_FLT_3D;
	for (k = 0 ; k < npts ; k++) {
	    vp = p;

	    *p++ = 0x0;
	    r=(float)cos(rd);
	    GP1_PUT_F(p, r);
	    g=(float)sin(rd);
	    GP1_PUT_F(p, g);
	    b=(float)k/(float)npts;
	    GP1_PUT_F(p, b);
	    GP1_PUT_F(p, r);
	    GP1_PUT_F(p, g);
	    GP1_PUT_F(p, b);

	    rd += inc;
	}
	*vp =0x8000;
	res = postbuf(p);
	if (res == -1) {
	    TRACE_OUT
	    return (errmsg_list[12]);
	}

	TRACE_OUT
	return (char *)0;
}

/**********************************************************************/
char *
centrics()
/**********************************************************************/

{
    extern char *centric_lines();

    char *errmsg;
    double cx;
    double cy;
    double length = 1.0;

    func_name = "centrics";
    TRACE_IN

    /* set context */
    errmsg = ctx_set(0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.0;
    cy = 0.0;
    errmsg = centric_lines(cx, cy, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_SET_RGB_COLOR, 0x0000ff, 0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = -0.5;
    cy = 0.5;
    errmsg = centric_lines(cx, cy, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_SET_RGB_COLOR, 0x00ff00, 0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.5;
    cy = 0.5;
    errmsg = centric_lines(cx, cy, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_SET_RGB_COLOR, 0xff0000, 0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.5;
    cy = -0.5;
    errmsg = centric_lines(cx, cy, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_SET_RGB_COLOR, 0x00ffff, 0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = -0.5;
    cy = -0.5;
    errmsg = centric_lines(cx, cy, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return (char *) 0;
}
/**********************************************************************/
char *
centric_lines(cx, cy, length)
/**********************************************************************/
double cx;
double cy;
double length;
{
    
    register double angl;
    register double inc;
    register short *p;
    register short *vp;
    register int i;
    int n;
    int res;
    float x1;
    float x2;
    float y1;
    float y2;
    float aux;

    func_name = "centric_lines";
    TRACE_IN

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    *p++ = GP1_XF_LINE_FLT_3D;

    n = 720;
    inc = M_PI/(double)(n/2);
    angl = 0.0;

    for (i = 0 ; i < n ; i++) {
	(void)centric_line_coord(angl, length, cx, cy, &x1, &y1, &x2, &y2);
	*p++ = 0x1;
	GP1_PUT_F(p, x1);
	GP1_PUT_F(p, y1);
	aux = (float)n;
	GP1_PUT_F(p, aux);
	vp = p;
	*p++ = 0x0;
	GP1_PUT_F(p, x2);
	GP1_PUT_F(p, y2);
	GP1_PUT_F(p, aux);
	angl += inc;
    }

    *vp =0x8000;
    res = postbuf(p);
    if (res == -1) {
	TRACE_OUT
	return (errmsg_list[12]);
    }

    TRACE_OUT
    return (char *)0;
}

/**********************************************************************/
centric_line_coord(angl, length, centerx, centery, x1, y1, x2, y2)
/**********************************************************************/
double angl;
double centerx;
double centery;
double length;
register float *x1;
register float *y1;
register float *x2;
register float *y2;

{

    register double l = length/2.0;

    func_name = "centric_line_coord";
    TRACE_IN

    *x1 = l*cos(angl) + centerx;
    *y1 = l*sin(angl) + centery;

    *x2 = 2.0*centerx - *x1;
    *y2 = 2.0*centery - *y1;

    TRACE_OUT

}
