static char version[] = "Version 1.1";
static char     sccsid[] = "@(#)cg6_polygons.c 1.1 7/30/92 Copyright 1988 Sun Micro";

#include "cg6test.h"

#define  BOUNDS		1
#define  VERTNUM	4

struct vertex {
	int x, y, z;
};

struct vertex vlist[][VERTNUM] = {
	{ {-32, -32, -32},  { 32, -32, -32},  { 32, 32, -32},  { -32,  32, -32} },
	{ { 32, -32, -32},  { 32,  32, -32},  { 32, 32,  32},  {  32, -32,  32} },
	{ {-32,  32, -32},  { 32,  32, -32},  { 32, 32,  32},  { -32,  32,  32} },
	{ {-32, -32,  32},  { 32, -32,  32},  { 32, 32,  32},  { -32,  32,  32} },
	{ {-32, -32, -32},  {-32, -32,  32},  {-32, 32,  32},  { -32,  32, -32} },
	{ {-32, -32, -32},  { 32, -32, -32},  { 32,-32,  32},  { -32, -32,  32} }
};

static struct pr_pos list2d[VERTNUM];
static struct vertex tmpvlist[VERTNUM];

/*
struct vertex vlist[][ VERTNUM ] = {
	 { -32, 32}, {32, 32}, { 32, -32}, {-32, -32}
};
*/

#define YSIZE 4
#define XSIZE 4

static float rxmat[YSIZE][XSIZE],
		rymat[YSIZE][XSIZE],
		rzmat[YSIZE][XSIZE];

set_rotxmat ( rad )
	float rad;
{
	int i;
	float *tmp;

	tmp = (float *) rxmat;

	for (i = 0; i < 16; i++) 
		*tmp = 0.0;

	rxmat[0][0] = 1.0;
	rxmat[1][1] = cos(rad);
	rxmat[2][1] = -sin(rad);
	rxmat[1][2] = sin(rad);
	rxmat[2][2] = cos(rad);
	rxmat[3][3] = 1.0;

	return(0);
}

rotaboutx ( object, newobject, rad )
	struct vertex object[];
	struct vertex newobject[];
	float rad;
{
	float tx, ty, tz;
	int i;

	set_rotzmat ( rad );

	for ( i = 0; i < VERTNUM; i++ ) {
		tx = object[i].x;
		ty = object[i].y*rxmat[1][1] + object[i].z*rxmat[2][1];
		tz = object[i].y*rxmat[1][2] + object[i].z*rxmat[3][3];
		newobject[i].x = (int) tx;
		newobject[i].y = (int) ty;
	}
}

set_rotzmat ( rad )
	float rad;
{
	int i;
	float *tmp;

	tmp = (float *) rzmat;

	for (i=0; i<16;i++) {
		*tmp = 0.0;
	}

	rzmat[0][0] = cos(rad);
	rzmat[1][0] = -sin(rad);

	rzmat[0][1] = sin(rad);
	rzmat[1][1] = cos(rad);

	rzmat[2][2] = 1.0;
	rzmat[3][3] = 1.0;

	return(0);
}

rotaboutz ( object, newobject, rad )
	struct vertex object[];
	struct vertex newobject[];
	float rad;
{
	float tx, ty, tz;
	int i;

	set_rotzmat ( rad );

	for (i = 0; i < VERTNUM; i++) {
		tx = object[i].x*rzmat[0][0] + object[i].y*rzmat[1][0];
		ty = object[i].x*rzmat[0][1] + object[i].y*rzmat[1][1];
		tz = object[i].z;
		newobject[i].x = (int) tx;
		newobject[i].y = (int) ty;
	}
}

test_polygons()
{

	int npts[BOUNDS],	/* number of vertices in the polygon */
	    dx,			/* x middle of polygon */
	    dy;			/* y middle of polygon */

	int across, down, i;

	clear_screen();
	for (i = 0; i < VERTNUM; i++) {
		list2d[i].x =  vlist[0][i].x;
		list2d[i].y =  vlist[0][i].y;
	}

	dx = width/2;
	dy = 50;
	npts[0] = VERTNUM;

	/* draw an hour glass */
	list2d[0].x =  0;	/* vertex 0 */
	list2d[0].y =  0;

	list2d[1].x =  100;	/* vertex 1 */
	list2d[1].y =  0;

	list2d[2].x =  0;	/* vertex 2 */
	list2d[2].y =  100;

	list2d[3].x =  100;	/* vertex 3 */
	list2d[3].y =  100;

	for (dy = 10, down=0; down < height/125; down++, dy += 125) {
	    for (dx = 10, across=0; across < width/125; across++, dx += 125) {
	        pr_polygon_2 ( prfd, dx, dy, 1, npts, list2d,
			   PIX_SRC|PIX_COLOR(randvals[across+down]), 0, 0, 0 );
		pr_polygon_2 ( mexp, dx, dy, 1, npts, list2d,
			   PIX_SRC|PIX_COLOR(randvals[across+down]), 0, 0, 0 );
	    }
	}
	pr_rop(mobs,0, 0, width, height, PIX_SRC,prfd,0,0);
	ropcheck(0, 0, width, height, "test hour glass");

	return(0);

}
