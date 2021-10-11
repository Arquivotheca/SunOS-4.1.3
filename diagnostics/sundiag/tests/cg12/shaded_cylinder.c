
#ifndef lint
static  char sccsid[] = "@(#)shaded_cylinder.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#define	CUE
#define LIGHT
#define MLS
#define TRIPLE
/*
#define PERSPECTIVE
*/
#define ZBUF
/*
#define NONORM
*/
#define BACK_SHADE
/*
#define POLY
*/

#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include <sunwindow/cms_colorcube.h> 
#include <pixrect/gp1cmds.h>
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

static float    id3[4][4] = {
    1.5, 0.0, 0.0, 0.0,
    0.0, 1.5, 0.0, 0.0,
#ifdef PERSPECTIVE
    0.5, 0.2, 0.9, 0.9,
#else
    0.4, 0.2, 0.9, 0.0,
#endif
    0.0, 0.0, 0.0, 1.0
};

static float ONE = 1.0, ZERO = 0.0, POINT5=0.5, MONE=-1.0;
static float POINT8=0.8;


/**********************************************************************/
char *
shaded_cylinder()
/**********************************************************************/

{
    short *allocbuf();

    short    *p;
    int       i, j, count;
    float     faux;
    int       res;

    float	fz, bz;
    float angle, l;
    float m;

    func_name = "shaded_cylinder";
    TRACE_IN

    m=0.9;

    angle = 3.1415927/6.0;
    l = 0.2;

    fz = 0.1;
    bz = 0.9;


    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

    *p++ = GP1_SET_MAT_NUM | 1;
    *p++ = GP1_SET_MAT_3D | 1;
    for (i = 0 ; i < 4 ; i++) {
	for (j = 0 ; j < 4 ; j++) {
	    GP1_PUT_F(p, id3[i][j]);
	}
    }

    *p++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_TRIPLE;
    GP1_PUT_F(p, ONE);
    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ZERO);

#ifndef CUE
    *p++ = GP1_SET_DEPTH_CUE | GP1_DEPTH_CUE_OFF;
#else
    *p++ = GP1_SET_DEPTH_CUE | GP1_DEPTH_CUE_ON;
    *p++ = GP2_SET_DEPTH_CUE_PARAMETERS;
    GP1_PUT_F(p, ONE);
    GP1_PUT_F(p, ZERO);

    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);

    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);
#endif

#ifdef LIGHT
    *p++ = GP2_SET_REFLECTANCE;
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, POINT5);
	faux = 60.0;
        GP1_PUT_F(p, faux);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
 
    *p++ = GP2_SET_LIGHT | GP2_LIGHT_AMBIENT;
        GP1_PUT_F(p, POINT8);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, POINT8);

    *p++ = GP2_SET_LIGHT | GP2_LIGHT_DIRECTIONAL;
        *p++ = 0;
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, ZERO);
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_LIGHT_ON;
    *p++ = 0;

#ifdef MLS
    *p++ = GP2_SET_LIGHT | GP2_LIGHT_DIRECTIONAL;
        *p++ = 1;
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ZERO);
 
    *p++ = GP2_SET_LIGHT | GP2_LIGHT_DIRECTIONAL;
        *p++ = 2;
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, MONE);
        GP1_PUT_F(p, ZERO);
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_LIGHT_ON;
    *p++ = 1;

    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_LIGHT_ON;
    *p++ = 2;
#endif
    *p++ = GP2_SET_EYE | GP2_EYE_DIRECTIONAL;
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, MONE);

    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_BACK_FACE_SHADE;
    *p++ = 0;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_NO_FACE_REJ;
    *p++ = GP2_NO_FACE_REJ;
#ifndef NONORM
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_GENERATE_PNORMAL;
    *p++ = 0;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_COPY_PNORM_TO_VNORM;
    *p++ = 0;
#else NONORM
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_GENERATE_PNORMAL;
    *p++ = 1;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_COPY_PNORM_TO_VNORM;
    *p++ = 1;
#endif NONORM

#ifndef BACK_SHADE
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_BACK_FACE_SHADE;
    *p++ = 0;
#else
    *p++ = GP2_SET_REFLECTANCE | GP2_BACK_PROPERTIES;
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, POINT5);
        GP1_PUT_F(p, ZERO);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
        GP1_PUT_F(p, ONE);
 
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_BACK_FACE_SHADE;
    *p++ = 1;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_FACE_REJECTION;
    *p++ = GP2_NO_FACE_REJ;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_GENERATE_PNORMAL;
    *p++ = 1;
    *p++ = GP2_SET_LIGHT_OPTIONS;
    *p++ = GP2_COPY_PNORM_TO_VNORM;
    *p++ = 1;
#endif

#endif


#ifdef ZBUF
    *p++ = GP1_SET_HIDDEN_SURF | GP1_ZBHIDDENSURF;
#else
    *p++ = GP1_SET_HIDDEN_SURF | GP1_NOHIDDENSURF;
#endif

    *p++ = GP1_SET_ZBUF;
    *p++ = 0xffff;
    *p++ = 0;
    *p++ = 0;
    *p++ = 1152;
    *p++ = 900;

#ifdef NONORM
#undef LIGHT
#endif

#ifndef POLY

#ifdef TRIPLE
#ifdef LIGHT
*p++ = GP2_XF_TRISTRIP_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE | GP2_VERTEX_NORMALS;
#else
*p++ = GP2_XF_TRISTRIP_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
#endif
#else
#ifdef LIGHT
*p++ = GP2_XF_TRISTRIP_FLT_3D_RGB | GP2_VERTEX_NORMALS;
#else
*p++ = GP2_XF_TRISTRIP_FLT_3D_RGB;
#endif
#endif

*p++ = 26;

      faux = l*cos(0.0);
      GP1_PUT_F(p, faux);
      faux = l*sin(0.0);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, bz);

#ifdef TRIPLE
      faux = fabs(m*cos(0.0));
      GP1_PUT_F(p, faux);
      faux = fabs(m*sin(0.0));
      GP1_PUT_F(p, faux);
      faux = bz * m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(0.0);
      GP1_PUT_F(p, faux);
      faux = sin(0.0);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(0.0);
      GP1_PUT_F(p, faux);
      faux = l*sin(0.0);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, fz);

#ifdef TRIPLE
      faux = fabs(m*cos(0.0));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(0.0));
      GP1_PUT_F(p, faux);
      faux = fz * m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(0.0);
      GP1_PUT_F(p, faux);
      faux = sin(0.0);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle);
      GP1_PUT_F(p, faux);
      faux = l*sin(angle);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, bz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle));
      GP1_PUT_F(p, faux);
      faux = bz*m;
      GP1_PUT_F(p, faux); 
#endif

#ifdef LIGHT
      faux = cos(angle);
      GP1_PUT_F(p, faux);
      faux = sin(angle);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle);
      GP1_PUT_F(p, faux); 
      faux = l*sin(angle);
      GP1_PUT_F(p, faux); 
      GP1_PUT_F(p, fz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle));
      GP1_PUT_F(p, faux);
      faux = fz*m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(angle);
      GP1_PUT_F(p, faux);
      faux = sin(angle);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

for(count=2; count <= 12; count++)
{
      faux = l*cos(angle*count);
      GP1_PUT_F(p, faux); 
      faux = l*sin(angle*count);
      GP1_PUT_F(p, faux); 
      GP1_PUT_F(p, bz); 

#ifdef TRIPLE
      faux = fabs(m*cos(angle*count));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle*count));
      GP1_PUT_F(p, faux); 
      faux = m*bz;
      GP1_PUT_F(p, faux);
#endif
 
#ifdef LIGHT
      faux = cos(angle*count);
      GP1_PUT_F(p, faux);
      faux = sin(angle*count);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle*count);
      GP1_PUT_F(p, faux);  
      faux = l*sin(angle*count);
      GP1_PUT_F(p, faux);  
      GP1_PUT_F(p, fz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle*count));
      GP1_PUT_F(p, faux);
      faux = fabs(m*sin(angle*count));
      GP1_PUT_F(p, faux);
      faux = m*fz;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(angle*count);
      GP1_PUT_F(p, faux);
      faux = sin(angle*count);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

}

#else

for (count = 1; count <= 12; count++)
{

#ifdef TRIPLE
#ifdef LIGHT
*p++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE | GP2_VERTEX_NORMALS;
#else
*p++ = GP2_XF_PGON_FLT_3D_RGB | GP2_RGB_COLOR_TRIPLE;
#endif
#else
#ifdef LIGHT
*p++ = GP2_XF_PGON_FLT_3D_RGB | GP2_VERTEX_NORMALS;
#else
*p++ = GP2_XF_PGON_FLT_3D_RGB;
#endif
#endif

     *p++ = 1;
     *p++ = 4;

      faux = l*cos(angle*(count-1));
      GP1_PUT_F(p, faux);
      faux = l*sin(angle*(count-1));
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, bz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle*(count-1)));
      GP1_PUT_F(p, faux);
      faux = fabs(m*sin(angle*(count-1)));
      GP1_PUT_F(p, faux);
      faux = bz * m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(angle*(count-1));
      GP1_PUT_F(p, faux);
      faux = sin(angle*(count-1));
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle*(count-1));
      GP1_PUT_F(p, faux);
      faux = l*sin(angle*(count-1));
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, fz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle*(count-1)));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle*(count-1)));
      GP1_PUT_F(p, faux);
      faux = fz * m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(angle*(count-1));
      GP1_PUT_F(p, faux);
      faux = sin(angle*(count-1));
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle*count);
      GP1_PUT_F(p, faux); 
      faux = l*sin(angle*count);
      GP1_PUT_F(p, faux); 
      GP1_PUT_F(p, fz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle*count));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle*count));
      GP1_PUT_F(p, faux);
      faux = fz*m;
      GP1_PUT_F(p, faux);
#endif

#ifdef LIGHT
      faux = cos(angle*count);
      GP1_PUT_F(p, faux);
      faux = sin(angle*count);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif

      faux = l*cos(angle*count);
      GP1_PUT_F(p, faux);
      faux = l*sin(angle*count);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, bz);

#ifdef TRIPLE
      faux = fabs(m*cos(angle*count));
      GP1_PUT_F(p, faux); 
      faux = fabs(m*sin(angle*count));
      GP1_PUT_F(p, faux);
      faux = bz*m;
      GP1_PUT_F(p, faux); 
#endif

#ifdef LIGHT
      faux = cos(angle*count);
      GP1_PUT_F(p, faux);
      faux = sin(angle*count);
      GP1_PUT_F(p, faux);
      GP1_PUT_F(p, ZERO);
#endif


}

#endif


	res = postbuf(p);
	if (res == -1) {
	    TRACE_OUT
	    return (errmsg_list[12]);
	}

	return (char *)0;

}

