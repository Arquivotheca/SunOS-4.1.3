#ifndef lint
static	char sccsid[] = "@(#)suncube.c 1.1 92/07/30 Copyr 1985 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <usercore.h>
#include "demolib.h"
#include <math.h>
#include <stdio.h>

#define SETTAB	15
#define NVERT	29
#define NFACE	3

typedef struct {
	int opcode;
	int instance;
	int logical;
	char *ptr1, *ptr2, *ptr3;
	int int1, int2, int3;
	float float1, float2, float3;
	} ddargtype;


static float u_up_rel_x[NVERT], u_up_rel_y[NVERT],
	     u_left_rel_x[NVERT], u_left_rel_y[NVERT],
	     u_down_rel_x[NVERT], u_down_rel_y[NVERT],
	     u_right_rel_x[NVERT], u_right_rel_y[NVERT],
	     zeroes[NVERT] = {0};

static float bbox[3][2] = {{-256.0, 256.0}, {-256.0, 256.0}, {-256.0, 256.0}};

static float modmat[NFACE][4][4] = {
				{
				 { 1.0, 0.0, 0.0, 0.0},		/* FRONT  */
				 { 0.0, 1.0, 0.0, 0.0},
				 { 0.0, 0.0, 1.0, 0.0},
				 { 0.0, 0.0, 0.0, 1.0}
				},
				{
				 { 1.0, 0.0, 0.0, 0.0},		/* TOP    */
				 { 0.0, 0.0,-1.0, 0.0},
				 { 0.0, 1.0, 0.0, 0.0},
				 { 0.0, 0.0, 0.0, 1.0}
				},
				{
				 { 0.0, 0.0,-1.0, 0.0},		/* RIGHT  */
				 { 0.0, 1.0, 0.0, 0.0},
				 { 1.0, 0.0, 0.0, 0.0},
				 { 0.0, 0.0, 0.0, 1.0}
				}
			       };

static int fillindex;
static unsigned char red[512], green[512], blue[512];
static unsigned char rmap[256], gmap[256], bmap[256];
static ddargtype ddstruct;

static int cg1dd();
static int cg2dd();
static int cg4dd();
static int gp1dd();
static int cgpixwindd();
static int gp1pixwindd();


main(argc, argv)
	int argc;
	char *argv[];
{
	int i, counter, quick_flag;

	/* Check to see if we are running a quick demo */
	quick_flag = quick_test(argc,argv);

	/* Make the object */
	mkobjdat();

	/* Initialization */
	get_view_surface(our_surface, argv);
	start_up_core();
	makemap();
	loadmap(0);
	setvwpc(768.0, 768.0, 768.0);

	/* draw it */
	create_retained_segment(100);
	suncube();
	close_retained_segment();

	/* If not a color surface don't waste cycles rotating colormap */
	if(!((our_surface->dd == cg1dd) || (our_surface->dd == cg2dd) ||
	    (our_surface->dd == cgpixwindd) || (our_surface->dd == gp1dd) ||
	    (our_surface->dd == gp1pixwindd) || (our_surface->dd == cg4dd))) {
		if(quick_flag)
			sleep(20); 	/* For short demo */
		else
			sleep(1000000); /* For long demo */
		}

	/* If color surface rotate the colormap */
	i = 1;
	if (quick_flag)
		counter=5;		/* For short demo */
	else
		counter=1000000; 	/* For long demo */
	while (counter) {
		loadmap(i);
		if (++i == 510) {
			i = 0;
			counter--;
			}
		}

	/* Exit nicely */
	shut_down_core();
	return 0;
} /* end of main() */


static makemap()
{
	int i, j;

	j = 0;
	for (i = 0; i < 85; i++)
		{
		red[j] = 255;
		green[j] = 255 * i / 85;
		blue[j++] = 0;
		}
	for (i = 0; i < 85; i++)
		{
		red[j] = 255 - 255 * i / 85;
		green[j] = 255;
		blue[j++] = 0;
		}
	for (i = 0; i < 85; i++)
		{
		red[j] = 0;
		green[j] = 255;
		blue[j++] = 255 * i / 85;
		}
	for (i = 0; i < 85; i++)
		{
		red[j] = 0;
		green[j] = 255 - 255 * i / 85;
		blue[j++] = 255;
		}
	for (i = 0; i < 85; i++)
		{
		red[j] = 255 * i / 85;
		green[j] = 0;
		blue[j++] = 255;
		}
	for (i = 0; i < 85; i++)
		{
		red[j] = 255;
		green[j] = 0;
		blue[j++] = 255 - 255 * i / 85;
		}
	} /* end of makemap() */


static loadmap(n)
	int n;
{
	int i;

	for (i = 0; i < 24; i++) {
		rmap[i] = red[n];
		gmap[i] = green[n];
		bmap[i] = blue[n];
		n += 21;
		if (n >= 510)
			n = n % 510;
	}
	(*our_surface->dd)( &ddstruct);
} /* end of loadmap() */


start_up_core() 
{
	initialize_core(BASIC, SYNCHRONOUS, THREED);
	initialize_device(KEYBOARD, 1);
	our_surface->cmapsize = 32;
	our_surface->cmapname[0] = '\0';
	if (initialize_view_surface( our_surface, FALSE)) exit(1);
	select_view_surface( our_surface);
	fillindex = 1;
	ddstruct.opcode = SETTAB;
	ddstruct.int1 = 1;
	ddstruct.int2 = 24;
	ddstruct.instance = our_surface->instance;
	ddstruct.ptr1 = (char *)rmap;
	ddstruct.ptr2 = (char *)gmap;
	ddstruct.ptr3 = (char *)bmap;

} /* end of start_up_core() */

shut_down_core()
{
	deselect_view_surface( our_surface);
	terminate_core();

} /* end of shut_down_core() */


mkobjdat()
{
	makeu(u_up_rel_x, u_up_rel_y);
	rot90(u_up_rel_x, u_up_rel_y, u_left_rel_x, u_left_rel_y, 29);
	rot90(u_left_rel_x, u_left_rel_y, u_down_rel_x, u_down_rel_y, 29);
	rot90(u_down_rel_x, u_down_rel_y, u_right_rel_x, u_right_rel_y, 29);

} /* end of mkobjdat */


static makeu(ux, uy)
	float ux[], uy[];
{
	float theta, deltheta;
	int i;

	theta = 3.0 * 3.14159 / 2.0;
	deltheta = 3.14159 / 20.0;
	ux[0] = uy[0] = 0.0;
	for (i = 1; i < 10; i++)
		{
		ux[i] = cos(theta - deltheta * (float) i) * 60.0;
		uy[i] = sin(theta - deltheta * (float) i) * 60.0 + 60.0;
		}
	ux[10] = -60.0; uy[10] = 60.0;
	ux[11] = -60.0; uy[11] = 248.0;
	ux[12] = -4.0; uy[12] = 248.0;
	ux[13] = -4.0; uy[13] = 60.0;
	ux[14] = -2.0; uy[14] = 56.0;
	ux[15] = 2.0; uy[15] = 56.0;
	ux[16] = 4.0; uy[16] = 60.0;
	ux[17] = 4.0; uy[17] = 248.0;
	ux[18] = 60.0; uy[18] = 248.0;
	ux[19] = 60.0; uy[19] = 60.0;
	theta = 2.0 * 3.14159;
	for (i = 1; i < 10; i++)
		{
		ux[i + 19] = cos(theta - deltheta * (float) i) * 60.0;
		uy[i + 19] = sin(theta - deltheta * (float) i) * 60.0 + 60.0;
		}
	for (i = 0; i < 28; i++)
		{
		ux[i] = ux[i + 1] - ux[i];
		uy[i] = uy[i + 1] - uy[i];
		}
	ux[28] = -ux[28];
	uy[28] = -uy[28];

} /* end of  makeu */


static rot90(x, y, rx, ry, n)
	float x[], y[], rx[], ry[];
	int n;
{
	int i;

	for (i = 0; i < n; i++)
		{
		*rx++ = - *y++;
		*ry++ = *x++;
		}

} /* end of rot90 */


setvwpc(vx, vy, vz)
	float vx, vy, vz;
{
	int i;
	float diag, del;

	set_view_reference_point(vx, vy, vz);
	set_view_plane_normal(-vx, -vy, -vz);
	set_view_plane_distance(256.0);
	set_projection(PERSPECTIVE, 0.0, 0.0, 0.0);
	if ((vx == 0.0) && (vz == 0.0))
		set_view_up_3(0.0, 0.0, vy);
	else
		set_view_up_3(0.0, 1.0, 0.0);
	set_window(-85.0, 85.0, -92.0, 78.0);
	diag = 0.0;
	for (i = 0; i < 3; i++)
		{
		del = bbox[i][1] - bbox[i][0];
		diag += del * del;
		}
	diag = sqrt(diag) / 2.0;
	set_view_depth(4.0, sqrt(vx * vx + vy * vy + vz * vz) + diag);
	set_window_clipping(TRUE);
	set_viewport_2(0.125, 0.875, 0.0, 0.75);

} /* end of setvwpc() */


suncube()
{
	drawface(0);
	drawface(1);
	drawface(2);

} /* end of suncube() */


drawface(i)
int i;
{

	set_world_coordinate_matrix_3(modmat[i]);

	if((our_surface->dd == cg1dd) || (our_surface->dd == cg2dd) ||
	    (our_surface->dd == cgpixwindd) || (our_surface->dd == gp1dd) ||
	    (our_surface->dd == gp1pixwindd) || (our_surface->dd == cg4dd))
		sunlogo_polygon();
	else
		sunlogo_polyline();

} /* end of drawface() */


static sunlogo_polygon()
{
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(-4.0, 64.0, 256.0);
	polygon_rel_3(u_left_rel_x, u_left_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(-252.0, 192.0, 256.0);
	polygon_rel_3(u_right_rel_x, u_right_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(64.0, 12.0, 256.0);
	polygon_rel_3(u_up_rel_x, u_up_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(192.0, 252.0, 256.0);
	polygon_rel_3(u_down_rel_x, u_down_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(4.0, -64.0, 256.0);
	polygon_rel_3(u_right_rel_x, u_right_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(252.0, -192.0, 256.0);
	polygon_rel_3(u_left_rel_x, u_left_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(-64.0, -4.0, 256.0);
	polygon_rel_3(u_down_rel_x, u_down_rel_y, zeroes, NVERT);
	set_fill_index(fillindex);
	fillindex += 1;
	move_abs_3(-192.0, -252.0, 256.0);
	polygon_rel_3(u_up_rel_x, u_up_rel_y, zeroes, NVERT);

} /* end of sunlogo_polygon() */


static sunlogo_polyline()
{
	move_abs_3(-4.0, 64.0, 256.0);
	polyline_rel_3(u_left_rel_x, u_left_rel_y, zeroes, 29);
	move_abs_3(-252.0, 192.0, 256.0);
	polyline_rel_3(u_right_rel_x, u_right_rel_y, zeroes, 29);
	move_abs_3(64.0, 4.0, 256.0);
	polyline_rel_3(u_up_rel_x, u_up_rel_y, zeroes, 29);
	move_abs_3(192.0, 252.0, 256.0);
	polyline_rel_3(u_down_rel_x, u_down_rel_y, zeroes, 29);
	move_abs_3(4.0, -64.0, 256.0);
	polyline_rel_3(u_right_rel_x, u_right_rel_y, zeroes, 29);
	move_abs_3(252.0, -192.0, 256.0);
	polyline_rel_3(u_left_rel_x, u_left_rel_y, zeroes, 29);
	move_abs_3(-64.0, -4.0, 256.0);
	polyline_rel_3(u_down_rel_x, u_down_rel_y, zeroes, 29);
	move_abs_3(-192.0, -252.0, 256.0);
	polyline_rel_3(u_up_rel_x, u_up_rel_y, zeroes, 29);

} /* end of sunlogo_polyline() */


int quick_test(argc,argv) int argc; char *argv[];
{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);

} /* end of quick_test() */
