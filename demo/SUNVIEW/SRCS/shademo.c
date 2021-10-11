#ifndef lint
static	char sccsid[] = "@(#)shademo.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
#include <usercore.h>
#include <sun/fbio.h>
#include <math.h>
static 	int npts;
static float red[256], grn[256], blu[256];

static float up_x[40], up_y[40], up_z[40], up_dx[40], up_dy[40], up_dz[40],
	     lt_x[40], lt_y[40], lt_z[40], lt_dx[40], lt_dy[40], lt_dz[40];
static float dx[] = {-252.,192.,252.,-192.,-4.,64.,4.,-64. };
static float dy[] = {192.,252.,-192.,-252.,64.,4.,-64.,-4. };
static float spdx[] = {-222.,192.,222.,-192.,-34.,64.,34.,-64. };
static float spdy[] = {192.,222.,-192.,-222.,64.,34.,-64.,-34. };
static int angle[] = {1,2,3,0,3,0,1,2};
static float xx0 = 0., yy0 = 0.;

struct	vwsurf	*get_view_surface(/* (struct vwsurf *), argv */);
static	struct	vwsurf	Core_vwsurf;
struct	vwsurf	*our_surface = &Core_vwsurf;

#define CMAPSIZE	64
#define SHADEBITS	6

main(argc, argv)
	int argc;
	char *argv[];
{
	int i,j;

	get_view_surface(our_surface, argv);
	initialize_core( DYNAMICC, SYNCHRONOUS,THREED);
	our_surface->cmapsize = CMAPSIZE;
	strncpy( our_surface->cmapname, "SHADED", DEVNAMESIZE);
	initialize_view_surface( our_surface, FALSE);
	select_view_surface( our_surface);
	set_window(-384.0, 384.0, -384.0, 384.0);
	set_viewport_2(.125, .874, .0006, .75);
	set_view_depth( 0.0, 60.0);
	set_light_direction( -0.2,0.2,-1.);
	set_window_clipping( FALSE);

	red[0] = 0.0; grn[0] = 0.6; blu[0] = 0.0;
	for (i=1; i<CMAPSIZE; i++) {			/* load color LUT */
		red[i] = i * (1.0 / CMAPSIZE);
		grn[i] = i * (0.3 / CMAPSIZE);
		blu[i] = 0.0;
	}
	red[CMAPSIZE-1] = 1.0;
	grn[CMAPSIZE-1] = 1.0;
	blu[CMAPSIZE-1] = 1.0;
	define_color_indices(our_surface,0,our_surface->cmapsize-1,red,grn,blu);

	set_shading_parameters( .05, .90, 0., 0., 0., 1, PHONG);
	set_view_up_3( 0.,1., 0.);
	set_image_transformation_type( NONE);
	create_retained_segment(1);
	    set_text_index(1);
	    move_abs_2( -300.,-300.);
	    set_font( GREEK);
	    text("Satin Sun");
	close_retained_segment();
	set_view_up_3( 1.,1., 0.);
	makeu(up_x, up_y, up_z, up_dx, up_dy, up_dz);
	create_retained_segment( 2);
	    set_polygon_interior_style( SHADED);
	    for (i=0; i<8; i++) {
		transu( dx[i], dy[i], angle[i]);
		j = 2;
		set_vertex_normals( &lt_dx[j], &lt_dy[j], &lt_dz[j], 10);
		polygon_abs_3(&lt_x[j], &lt_y[j], &lt_z[j], 10);
		j = 12;
		set_vertex_normals( &lt_dx[j], &lt_dy[j], &lt_dz[j], 4);
		polygon_abs_3(&lt_x[j], &lt_y[j], &lt_z[j], 4);
		j = 16;
		set_vertex_normals( &lt_dx[j], &lt_dy[j], &lt_dz[j], 6);
		polygon_abs_3(&lt_x[j], &lt_y[j], &lt_z[j], 6);
		j = 24;
		set_vertex_normals( &lt_dx[j], &lt_dy[j], &lt_dz[j], 10);
		polygon_abs_3(&lt_x[j], &lt_y[j], &lt_z[j], 10);
		j = 34;
		set_vertex_normals( &lt_dx[j], &lt_dy[j], &lt_dz[j], 4);
		polygon_abs_3(&lt_x[j], &lt_y[j], &lt_z[j], 4);
		}
	close_retained_segment();
	while( 1) sleep( 20);
	terminate_core();
}
/*-------------------------------------------------------------*/
static makeu(ux, uy, uz, udx, udy, udz)
float ux[], uy[], uz[], udx[], udy[], udz[];
{
	float theta, deltheta;
	int i,j;
	float depth = -30.0;

	theta = 3.0 * 3.14159 / 2.0;
	deltheta = 3.14159 / 20.0;
	for (i = 0; i < 9; i++) {			/* LL part of U */
		ux[i] = cos(theta - deltheta * (float) i) * 60.0;
		uy[i] = sin(theta - deltheta * (float) i) * 60.0 + 60.0;
		uz[i] = depth;
		udx[i] = ux[i];
		udy[i] = uy[i] - 60.;
		udz[i] = 6.;
		}
	i = 9; ux[i] = -60.0; uy[i] = 60.0; uz[i] = depth;
	udx[i] = ux[i]; udy[i] = 0.; udz[i] = 6.;
	i = 10; ux[i] = -4.0; uy[i] = 60.0; uz[i] = depth;
	udx[i] = 60.0; udy[i] = 0.; udz[i] = 6.;
	i = 11; ux[i] = 0.0; uy[i] = 56.0; uz[i] = depth;
	udx[i] = 0.0; udy[i] = 60.; udz[i] = 6.;

	i = 12; ux[i] = -60.0; uy[i] = 60.0; uz[i] = depth;/* left bar of U */
	udx[i] = -60.0; udy[i] = 0.; udz[i] = 6.;
	i = 13; ux[i] = -4.0; uy[i] = 60.0; uz[i] = depth;
	udx[i] = 60.0; udy[i] = 0.; udz[i] = 6.;
	i = 14; ux[i] = -4.0; uy[i] = 248.0; uz[i] = depth;
	udx[i] = 60.0; udy[i] = 0.; udz[i] = 6.;
	i = 15; ux[i] = -60.0; uy[i] = 248.0; uz[i] = depth;
	udx[i] = -60.0; udy[i] = 0.; udz[i] = 6.;


	i = 16; ux[i] = 0.0; uy[i] = 0.0; uz[i] = depth;/* ctrl triangle of U */
	udx[i] = 0.0; udy[i] = -60.; udz[i] = 6.;
	i = 17; ux[i] = ux[1]; uy[i] = uy[1]; uz[i] = depth;
	udx[i] = udx[1]; udy[i] = udy[1]; udz[i] = udz[1];
	i = 18; ux[i] = ux[2]; uy[i] = uy[2]; uz[i] = depth;
	udx[i] = udx[2]; udy[i] = udy[2]; udz[i] = udz[2];
	i = 19; ux[i] = 0.0; uy[i] = 56.0; uz[i] = depth;
	udx[i] = 0.0; udy[i] = 60.; udz[i] = 6.;
	i = 20; ux[i] = -ux[2]; uy[i] = uy[2]; uz[i] = depth;
	udx[i] = -udx[2]; udy[i] = udy[2]; udz[i] = udz[2];
	i = 21; ux[i] = -ux[1]; uy[i] = uy[1]; uz[i] = depth;
	udx[i] = -udx[1]; udy[i] = udy[1]; udz[i] = udz[1];

	for (i=0; i<16; i++) {				/* bilateral symmetry */
	    j = i+22;
	    ux[j] = -ux[i]; uy[j] = uy[i]; uz[j] = uz[i];
	    udx[j] = -udx[i]; udy[j] = udy[i]; udz[j] = udz[i];
	    }
	    
}
/*--------------------------------------------------*/
static transu( dx, dy, ang)
float dx, dy; int ang;
{
	int i; float offx, offy;

	offx = xx0 + dx;   offy = yy0 + dy;
	switch (ang) {
	case 0:
	    for (i=0; i<38; i++) {
		lt_x[i] = offx + up_x[i]; lt_y[i] = offy + up_y[i];
		lt_z[i] = up_z[i];
		lt_dx[i] = up_dx[i]; lt_dy[i] = up_dy[i];
		lt_dz[i] = up_dz[i];
		}
	    break;
	case 1:
	    for (i=0; i<38; i++) {
		lt_x[i] = offx + up_y[i]; lt_y[i] = offy - up_x[i];
		lt_z[i] = up_z[i];
		lt_dx[i] = up_dy[i]; lt_dy[i] = -up_dx[i];
		lt_dz[i] = up_dz[i];
		}
	    break;
	case 2:
	    for (i=0; i<38; i++) {
		lt_x[i] = offx - up_x[i]; lt_y[i] = offy - up_y[i];
		lt_z[i] = up_z[i];
		lt_dx[i] = -up_dx[i]; lt_dy[i] = -up_dy[i];
		lt_dz[i] = up_dz[i];
		}
	    break;
	case 3:
	    for (i=0; i<38; i++) {
		lt_x[i] = offx - up_y[i]; lt_y[i] = offy + up_x[i];
		lt_z[i] = up_z[i];
		lt_dx[i] = -up_dy[i]; lt_dy[i] = up_dx[i];
		lt_dz[i] = up_dz[i];
		}
	    break;
	default: break;
	}
}
