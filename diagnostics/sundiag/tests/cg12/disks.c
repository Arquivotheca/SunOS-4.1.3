
#ifndef lint
static  char sccsid[] = "@(#)disks.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/cms_colorcube.h> 
#include <pixrect/gp1cmds.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

#define GP2_XF_TRISTAR_FLT_3D_RGB (0x7d << 8)


/**********************************************************************/
char *
disks()
/**********************************************************************/
{
    extern char *disk();

    char *errmsg;
    double cx;
    double cy;
    double cz;
    double length = 1.0;

    func_name = "disks";
    TRACE_IN

    /* set context */
    errmsg = ctx_set(CTX_SET_RGB_COLOR, 0x00ffff,
		    CTX_SET_HIDDEN_SURF, GP1_ZBHIDDENSURF,
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = -0.375;
    cy = -0.375;
    cz = 0.0;
    errmsg = disk(cx, cy, cz, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_DONT_CLEAR,
		    CTX_SET_RGB_COLOR, 0xff0000,
		    CTX_SET_HIDDEN_SURF, GP1_ZBHIDDENSURF,
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.375;
    cy = -0.375;
    cz = 0.25;
    errmsg = disk(cx, cy, cz, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_DONT_CLEAR,
		    CTX_SET_RGB_COLOR, 0x00ff00,
		    CTX_SET_HIDDEN_SURF, GP1_ZBHIDDENSURF,
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.375;
    cy = 0.375;
    cz = 0.5;
    errmsg = disk(cx, cy, cz, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_DONT_CLEAR,
		    CTX_SET_RGB_COLOR, 0x0000ff,
		    CTX_SET_HIDDEN_SURF, GP1_ZBHIDDENSURF,
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = -0.375;
    cy = 0.375;
    cz = 0.75;
    errmsg = disk(cx, cy, cz, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    /* set context */
    errmsg = ctx_set(CTX_DONT_CLEAR,
		    CTX_SET_RGB_COLOR, 0xffffff,
		    CTX_SET_HIDDEN_SURF, GP1_ZBHIDDENSURF,
		    0);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }
    cx = 0.375;
    cy = 0.0;
    cz = 1.0;
    errmsg = disk(cx, cy, cz, length);
    if (errmsg) {
	TRACE_OUT
	return(errmsg);
    }

    TRACE_OUT
    return (char *) 0;
}
    
/**********************************************************************/
static char *
disk(cx, cy, cz, radius)
/**********************************************************************/
double cx;
double cy;
double cz;
double radius;
{
    short *allocbuf();

    short *p;
    float faux;
    float x;
    float y;
    int res;
    int i;
    int n = 36;
    double inc = M_PI/(double)(n/2);
    double angl = 0.0;

    func_name = "sphere";
    TRACE_IN


    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    *p++ = GP2_XF_TRISTAR_FLT_3D_RGB | GP1_SHADE_CONSTANT;
    *p++ = n;

    faux = cx;
    GP1_PUT_F(p, faux);	/* the starting point is the center of */
    faux = cy;
    GP1_PUT_F(p, faux);	/* the disc */
    faux = cz;
    GP1_PUT_F(p, faux);

    for (i = 0 ; i < n-1 ; i++) {
	tri_coord(angl, cx, cy, radius, &x, &y);
	GP1_PUT_F(p, x);
	GP1_PUT_F(p, y);
	GP1_PUT_F(p, faux);
	angl += inc;
    }


    res = postbuf(p);
    if (res == -1) {
	TRACE_OUT
	return (errmsg_list[12]);
    }

    return (char *)0;

}

/**********************************************************************/
static
tri_coord(angl, centerx, centery, radius, x, y)
/**********************************************************************/
double angl;
double centerx;
double centery;
double radius;
float *x;
float *y;
{

    register double l = radius/2.0;

    func_name = "tri_coord";
    TRACE_IN

    *x = l*cos(angl) + centerx;
    *y = l*sin(angl) + centery;

    TRACE_OUT
}
    
