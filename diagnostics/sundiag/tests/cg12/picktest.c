
#ifndef lint
static  char sccsid[] = "@(#)picktest.c 1.1 92/07/30 Copyr 1990 Sun Micro";
#endif

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <pixrect/pixrect_hs.h>
#include <pixrect/gp1cmds.h>

#define MAX_PICK	100

typedef struct {
     short pick_hit;
     unsigned int pick_count;
     int pick_id_0;
     int pick_id_1;
} Pick_info;

typedef enum {
     draw,
     move,
     lastdraw = 0x8000,
     lastmove = 0x8001
} Draw;

typedef enum {parallel, perspective, done} Projection;
typedef enum {red, white, green, black} Color;

typedef struct {/*how to pass window borders around*/
     union {
	  float f;
	  int i;
     } xmin, ymin, zmin, xmax, ymax, zmax;
} Window;

extern short *allocbuf();

double scale;
int color;
int hits;
int misses;
int poly;
int two_d;
int alloc_err;

enum {in, out, left, center, right};
static char *types[] = {"in", "outside", "left of", "center of", "right of"};
typedef enum {triangle, polygon} Mode;
static char *modes[] = {"Triangle", "Polygon"};

extern char *errmsg_list[];
static char errtext[512];

/**********************************************************************/
char *
picktest()
/**********************************************************************/

{
    int k, type;
    int mode;
    int err;
    float x, y;
    Pick_info *checkpick(), *pickptr;
    Window pickrect;

    err = alloc_err = 0;

    pickrect.ymin.i = 200;
    pickrect.ymax.i = 800;
    scale = 1.0;
    two_d = 0;

    pickrect.zmin.i = 0;
    pickrect.zmax.i = 0;

    for (mode = (int)triangle ; mode <= (int)polygon ; mode++) {
	poly  = mode;
	for (type = (int)in; type <= (int)right; type++) {
	    hits   = 0;
	    misses = 0;
	    k      = 0;
	    switch (type) {
		case in:
		    pickrect.xmin.i = 550;
		    pickrect.xmax.i = 650;
		break;
		case out:
		    pickrect.xmin.i = 550;
		    pickrect.xmax.i = 555;
		break;
		case left:
		    pickrect.xmin.i = 565;
		    pickrect.xmax.i = 570;
		break;
		case center:
		    pickrect.xmin.i = 575;
		    pickrect.xmax.i = 580;
		break;
		case right:
		    pickrect.xmin.i = 580;
		    pickrect.xmax.i = 585;
		break;
	    }

	    (void)fast_clear();
	    if (alloc_err) {
		error_report(errmsg_list[52]);
		return (char *)0;
	    }
	    (void)picksquare (&pickrect);
	    color = 1;

	    while (k < MAX_PICK) {
		(void)setpick(&pickrect);
	        if (alloc_err) {
		   error_report(errmsg_list[52]);
		   return (char *)0;
	        }
		(void)clearpick();
	        if (alloc_err) {
		   error_report(errmsg_list[52]);
		   return (char *)0;
	        }

		x = 0.;
		y = 0.;

		(void)marker((Color)color, x, y, 0.0);
	        if (alloc_err) {
		   error_report(errmsg_list[52]);
		   return (char *)0;
	        }
/*
		(void)marker((Color)color, x, y, ((double)k/(double)MAX_PICK));
*/

		pickptr = checkpick ();
	        if (alloc_err) {
		   error_report(errmsg_list[52]);
		   return (char *)0;
	        }

		color = (color == 3) ? 0 : color+1;

		if (pickptr->pick_hit) {
		    if (type == out) {
			misses++;
		    } else {
			hits++;
		    }
		} else {
		    if (type == out) {
			hits++;
		    } else {
			misses++;
		    }
		}
		k++;
	    }
	    if (misses) {
		err++;
		sprintf (errtext, "%s pick test: detected %d miss(es) %s the pick window out of total %d picking events.\n", modes[poly], misses, types[type], MAX_PICK);
		(void)error_report(errtext);
	    }
	}
    }
    if (err) {
	return ("Suspect problem with interrupt.\n");
    } else {
	return (char *)0;
    }
}

/**********************************************************************/
setpick(border)
/**********************************************************************/
Window *border;
{
     short *ptr;
     int pickid;


     ptr = allocbuf();
     if (ptr == (short *)-1) {
	alloc_err = -1;
	return(0);
     }

     *ptr++ = GP1_SET_PICK | GP1_PICK_DRAW;

     pickid = 5;
     *ptr++ = GP1_SET_PICK_ID | 0; /*set value of pick-id 0*/
     GP1_PUT_I(ptr,pickid);

     pickid = 6;
     *ptr++ = GP1_SET_PICK_ID | 1; /*set value of pick-id 1*/
     GP1_PUT_I(ptr,pickid);

     *ptr++ = GP1_SET_PICK_WINDOW; /*set dimensions of pick window*/
     *ptr++ = border->xmin.i; 
     *ptr++ = border->ymin.i;
     *ptr++ = border->xmax.i - border->xmin.i;
     *ptr++ = border->ymax.i - border->ymin.i;

     (void)postbuf(ptr);
}

/**********************************************************************/
picksquare(border)
/**********************************************************************/
Window *border;
{
    box(border->xmin.i, border->ymin.i, border->xmax.i, border->ymax.i);
}

/**********************************************************************/
clearpick()
/**********************************************************************/
{
     short *ptr;

     ptr = allocbuf();
     if (ptr == (short *)-1) {
	alloc_err = -1;
	return(0);
     }
     if (*ptr == -1) return(-1);

     *ptr++ = GP1_CLEAR_PICK; /*clear everything*/

     (void)postbuf(ptr);
     return(0);
}

/**********************************************************************/
Pick_info *
checkpick()
/**********************************************************************/
{
     short *ptr;
     short *ptrsave;
     short *result;
     static Pick_info info;

     ptr = allocbuf();
     if (ptr == (short *)-1) {
	alloc_err = -1;
	return(0);
     }
     ptrsave = ptr;

     *ptr++ = GP1_GET_PICK | 1; /*get one pick*/
     result = ptr; /*save return location*/
     *ptr++ = 1; /*not ready*/
      ptr += 7; /*save room for pick event*/

     postbuf_n_wait(ptr, result);

     info.pick_hit = *(++result); /*load info data structure*/
     result++;
     GP1_GET_I(result,info.pick_count);
     GP1_GET_I(result,info.pick_id_0);
     GP1_GET_I(result,info.pick_id_1);

     ptr = ptrsave;
     *ptr++ = GP1_CLEAR_PICK; /*clear everything*/
     (void)postbuf(ptr);

     return(&info);
}


/**********************************************************************/
marker(color,xp,yp,zp)
/**********************************************************************/
Color color;
float xp,yp,zp;
{
    float r,g,b;
    float x,y,z;
    float tempf;
    short *ptr;

    x = xp;
    y = yp;
    z = zp;

    ptr = allocbuf();
    if (ptr == (short *)-1) {
	alloc_err = -1;
	return(0);
    }

    switch ((int)color) {
	case black:
	    r = 0.0;
	    g = 0.0;
	    b = 0.0;
	    break;
	case red:
	    r = 1.0;
	    g = 0.0;
	    b = 0.0;
	    break;
	case green:
	    r = 0.0;
	    g = 1.0;
	    b = 0.0;
	    break;
	case white:
	    r = 1.0;
	    g = 1.0;
	    b = 1.0;
	    break;
    }

    *ptr++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_TRIPLE; /*set marker color*/
    GP1_PUT_F(ptr,r);
    GP1_PUT_F(ptr,g);
    GP1_PUT_F(ptr,b);

    if (two_d) {
	*ptr++ = GP1_XF_PGON_FLT_2D;
	*ptr++ = 1;
	*ptr++ = 4;

	tempf = x;     GP1_PUT_F(ptr, tempf);
	tempf = y-.15; GP1_PUT_F(ptr, tempf);

	tempf = x-.05; GP1_PUT_F(ptr, tempf);
	tempf = y-.05; GP1_PUT_F(ptr, tempf);

	tempf = x;     GP1_PUT_F(ptr, tempf);
	tempf = y+.05; GP1_PUT_F(ptr, tempf);

	tempf = x+.05; GP1_PUT_F(ptr, tempf);
	tempf = y-.05; GP1_PUT_F(ptr, tempf);
    }
    else {
	if (poly) {
	    *ptr++ = GP1_XF_PGON_FLT_3D/* _RGB */;
	    *ptr++ = 1;
	    *ptr++ = 4;

	    tempf = x;     GP1_PUT_F(ptr, tempf);
	    tempf = y-.15; GP1_PUT_F(ptr, tempf);
	    GP1_PUT_F(ptr, z);
	}
	else {
	    *ptr++ = GP2_XF_TRISTRIP_FLT_3D_RGB;
	    *ptr++ = 3;
	}

	tempf = x-.05; GP1_PUT_F(ptr, tempf);
	tempf = y-.05; GP1_PUT_F(ptr, tempf);
	GP1_PUT_F(ptr, z);

	tempf = x;     GP1_PUT_F(ptr, tempf);
	tempf = y+.05; GP1_PUT_F(ptr, tempf);
	GP1_PUT_F(ptr, z);

	tempf = x+.05; GP1_PUT_F(ptr, tempf);
	tempf = y-.05; GP1_PUT_F(ptr, tempf);
	GP1_PUT_F(ptr, z);
    }

    (void)postbuf(ptr);
    return (0);
}


/**********************************************************************/
fast_clear()
/**********************************************************************/
{
     short *ptr;
     float r,g,b;

     ptr = allocbuf();
     if (ptr == (short *)-1) {
	alloc_err = -1;
	return(0);
     }

     r = 0.0;
     g = 0.0;
     b = 0.0;

     *ptr++ = GP2_SET_RGB_COLOR | GP2_RGB_COLOR_TRIPLE; /*set marker color*/
     GP1_PUT_F(ptr,r);
     GP1_PUT_F(ptr,g);
     GP1_PUT_F(ptr,b);

     *ptr++ = GP1_SET_ZBUF | 3; /*clear everything*/
     *ptr++ = 0xffff ; 
     *ptr++ = 0 ; 
     *ptr++ = 0 ; 
     *ptr++ = 1152 ; 
     *ptr++ = 900 ; 

     (void)postbuf(ptr);
     return (0);
}
