
#ifndef lint
static  char sccsid[] = "@(#)textured_hexnut.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/gp1cmds.h>
#include <sunwindow/cms_colorcube.h> 
#include <sdrtns.h>     /* sdrtns.h should always be included */
#include <esd.h>

static float ONE = 1.0, ZERO = 0.0;

#define gP1_PUT_F(a,b) tempf=b; GP1_PUT_F(a, tempf);

float tempf;
/**********************************************************************/
char *
textured_hexnut()
/**********************************************************************/

{
    extern short *allocbuf();

    short    *p;
    int       count;
    int res;

    float	fz, bz;
    float angle, l, l1;

    angle = 3.141592654/6.0;
    l = 0.2;
    l1 = 0.5;

    fz = 0.3;
    bz = 0.7;



    func_name = "textured_hexnut";
    TRACE_IN

    p = allocbuf();
    if (p == (short *)-1) {
	TRACE_OUT
	return (errmsg_list[52]);
    }

/*
    *p++ = GP1_SET_HIDDEN_SURF | GP1_ZBALL;

    *p++ = GP1_SET_LINE_TEX;
    *p++ = 10;
    *p++ = 5;
    *p++ = 0; *p++ = 0; *p++ = 0;

    *p++ = GP1_SET_MAT_NUM | 1;
    *p++ = GP1_SET_MAT_3D | 1;
    bcopy(id3, p, sizeof(id3));
    p += sizeof(id3)/sizeof(short);

    *p++ = GP1_SET_DEPTH_CUE | GP1_DEPTH_CUE_ON;

    *p++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_TRIPLE;
    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);
    GP1_PUT_F(p, ZERO);

    *p++ = GP1_SET_VWP_3D;
    GP1_PUT_F(p, xscale);
    GP1_PUT_F(p, xfs);
    GP1_PUT_F(p, yscale);
    GP1_PUT_F(p, yfs);
    GP1_PUT_F(p, zscale);
    GP1_PUT_F(p, zfs);

    *p++ = GP2_SET_DEPTH_CUE_PARAMETERS;
    GP1_PUT_F(p, ONE);   
    GP1_PUT_F(p, ZERO);

    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);

    GP1_PUT_F(p, ONE);
    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);
*/

    *p++ = GP2_SET_DEPTH_CUE_PARAMETERS;
    GP1_PUT_F(p, ONE);   
    GP1_PUT_F(p, ZERO);

    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);

    GP1_PUT_F(p, ONE);
    GP1_PUT_F(p, ZERO);
    GP1_PUT_F(p, ONE);

    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
for(count=0; count < 12; count++)
{
    *p++ = 0x0000;
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);
}
    *p++ = 0x8000;
    gP1_PUT_F(p, l*cos(0.0)); gP1_PUT_F(p, l*sin(0.0)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);

    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
for(count=0; count < 6; count++)
{
    *p++ = 0x0000;
    gP1_PUT_F(p, l1*cos(angle*count*2)); gP1_PUT_F(p, l1*sin(angle*count*2)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);
}
    *p++ = 0x8000;
    gP1_PUT_F(p, l1*cos(0.0)); gP1_PUT_F(p, l1*sin(0.0)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);

    *p++ = GP2_XF_SHADE_LINE_FLT_3D; 
for(count=0; count < 6; count++) 
{ 
    *p++ = 0x0000;
    gP1_PUT_F(p, l1*cos(angle*count*2)); gP1_PUT_F(p, l1*sin(angle*count*2)); gP1_PUT_F(p, bz); 
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);
} 
    *p++ = 0x8000;
    gP1_PUT_F(p, l1*cos(0.0)); gP1_PUT_F(p, l1*sin(0.0)); gP1_PUT_F(p, bz); 
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);

for(count=0; count < 6; count++)
{
    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
    *p++ = 0x0001;
    gP1_PUT_F(p, l1*cos(angle*count*2)); gP1_PUT_F(p, l1*sin(angle*count*2)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);
    *p++ = 0x8000;
    gP1_PUT_F(p, l1*cos(angle*count*2)); gP1_PUT_F(p, l1*sin(angle*count*2)); gP1_PUT_F(p, bz);
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);
}
/*
    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
for(count=0; count < 12; count++)
{
    *p++ = 0x0000;
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, m1z);
}
    *p++ = 0x8000;
    gP1_PUT_F(p, l*cos(0.0)); gP1_PUT_F(p, l*sin(0.0)); gP1_PUT_F(p, m1z);

    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
for(count=0; count < 12; count++)
{
    *p++ = 0x0000;
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, m2z);
}
    *p++ = 0x8000;
    gP1_PUT_F(p, l*cos(0.0)); gP1_PUT_F(p, l*sin(0.0)); gP1_PUT_F(p, m2z);
*/
 
    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
for(count=0; count < 12; count++)
{
    *p++ = 0x0000;
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, bz);
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);
}
    *p++ = 0x8000;
    gP1_PUT_F(p, l*cos(0.0)); gP1_PUT_F(p, l*sin(0.0)); gP1_PUT_F(p, bz);
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);
 
for(count=0; count < 12; count++)
{
    *p++ = GP2_XF_SHADE_LINE_FLT_3D;
    *p++ = 0x0001;
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, fz);
    gP1_PUT_F(p, ONE); gP1_PUT_F(p, ONE); gP1_PUT_F(p, ZERO);
    *p++ = 0x8000; 
    gP1_PUT_F(p, l*cos(angle*count)); gP1_PUT_F(p, l*sin(angle*count)); gP1_PUT_F(p, bz);
    gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ZERO); gP1_PUT_F(p, ONE);
}

    res = postbuf(p);
    if (res == -1) {
	TRACE_OUT
	return (errmsg_list[12]);
    }

    return (char *)0;

}

