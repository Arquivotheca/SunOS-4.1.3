/*	shaded.c	1.1	92/07/30	*/

#include <usercore.h>
#include <sun/fbio.h>
#include <math.h>
#include <stdio.h>
#include "demolib.h"

#define MAXVERT 800
#define MAXPOLY 800
#define MAXPVERT 6400
static int nvert, npoly;
static float bbox[3][2];
static float planeq[MAXPOLY][4];
static float vertices[MAXVERT][3];
static float normal[MAXVERT][3];
static int cindex[MAXVERT];
static short normalcount[MAXVERT];
static int npvert[MAXPOLY];
static short *pvertptr[MAXPOLY];
static short pvert[MAXPVERT];
static float xlist[10], ylist[10], zlist[10];
static float dxlist[10], dylist[10], dzlist[10];
static int  indxlist[10];
static float red[256], grn[256], blu[256];
static float dred[256], dgrn[256], dblu[256];
static int renderstyle=1;
static int renderhue=0;
static float xcut[2]={0.,1.}, zcut0[2]={0.,0.}, zcut[2]={0.,1.};


main(argc, argv) 
int argc; 
char *argv[];
{
	char string[81];
	int button, length = 0, visible();
	float x, y, z;

	if (argc < 2) { printf("Usage: shadeobj objfile\n"); exit(1); }
	if (getobjdat(argv[1])) exit(2);
	initvw();
	get_view_surface(our_surface,argv);
	start_up_core();
	x = 0.0; 	y = 0.0;
	z = bbox[2][1] + (bbox[0][1] > bbox[1][1] ? bbox[0][1] : bbox[1][1]);
	for (;;) {
		new_frame();
		getxyz(&x, &y, &z);
		new_frame();
		setvwpo(x, y, z);
		create_temporary_segment();
		    set_primitive_attributes( &PRIMATTS);
		    set_polygon_interior_style( SHADED);
		    drawobj();
		close_temporary_segment();
		do {
			await_any_button(0, &button);
			if (button == 3) break;
			await_keyboard(0, 1, string, &length);
			}
		while (length == 0);
		if (length != 0) break;
		}
	shut_down_core();
	return 0;
	}

start_up_core()
{
	int i;

	initialize_core(BASIC, SYNCHRONOUS, THREED);
	our_surface->cmapsize = 256;
	our_surface->cmapname[0] = '\0';
	if(initialize_view_surface(our_surface, TRUE)) exit(1);
    	initialize_device( BUTTON, 1);		/* initialize input devices */
	initialize_device( BUTTON, 2);		/* initialize input devices */
	initialize_device( BUTTON, 3);		/* initialize input devices */
	initialize_device( PICK, 1);
	set_echo_surface( PICK, 1, our_surface);
	set_echo( PICK, 1, 0);
	select_view_surface(our_surface);
	set_light_direction( -0.4,0.4,-1.);
	inquire_color_indices(our_surface,0,255,dred,dgrn,dblu);
	for (i=1; i<=255; i++) {			/* load color LUT */
		red[i] = (float)i * 0.003921568;
		grn[i] = (float)i * 0.003921568;
		blu[i] = (float)i * 0.003921568;;
		}
	red[0] = 0.; grn[0] = .7; blu[0] = 0.;

	define_color_indices( our_surface,0,255,red,grn,blu);
			/* ambient,diffuse,specular,flood,bump,hue,style */
	set_shading_parameters( .01, .96, .0, 0., 7.,0,0);

	initialize_device(KEYBOARD, 1);
	set_echo_surface( KEYBOARD, 1, our_surface);
	set_keyboard(1, 80, "", 1);
	initialize_device(LOCATOR, 1);
	set_echo(LOCATOR, 1, 0);
	set_echo_surface( LOCATOR, 1, our_surface);
	setvwpv();  build_menu();
	set_font(1);
	}

shut_down_core()
{
	terminate_device(KEYBOARD, 1);
	deselect_view_surface(our_surface);
	terminate_view_surface(our_surface);
	terminate_core();
	}

int getobjdat(filename)
char *filename;
	{
	int i, j, k;
	short vtmp, v1, v2, v3;
	float ftmp, maxd, scale, offset[3];
	float x,y,z,x0,y0,z0,length;
	FILE *fptr;

	if ((fptr = fopen(filename, "r")) == NULL) {
		printf("Can't open file: %s\n", filename);
		return(1);
		}
	fscanf(fptr, "%d%d", &nvert, &npoly);
	if ((nvert > MAXVERT) || (npoly > MAXPOLY)) {
		printf("Too many object vertices or polygons\n");
		return(2);
		}
	fscanf(fptr, "%f%f%f%f%f%f", &bbox[0][0], &bbox[0][1], &bbox[1][0],
		&bbox[1][1], &bbox[2][0], &bbox[2][1]);
	maxd = 0.0;
	for (i = 0; i < 3; i++) {
		offset[i] = (bbox[i][0] + bbox[i][1]) / 2.0;
		bbox[i][0] -= offset[i];
		bbox[i][1] -= offset[i];
		if (bbox[i][0] > bbox[i][1]) {
			ftmp = bbox[i][0];
			bbox[i][0] = bbox[i][1];
			bbox[i][1] = ftmp;
			}
		if (maxd < bbox[i][1])
			maxd = bbox[i][1];
		}
	scale = 1000.0 / maxd;
	for (i = 0; i < 3; i++) {
		bbox[i][0] *= scale;
		bbox[i][1] *= scale;
		}
	for (i = 0; i < nvert; i++) {
		fscanf(fptr, "%f%f%f", &vertices[i][0], &vertices[i][1],
			&vertices[i][2]);
		vertices[i][0] = (vertices[i][0] - offset[0]) * scale;
		vertices[i][1] = (vertices[i][1] - offset[1]) * scale;
		vertices[i][2] = (vertices[i][2] - offset[2]) * scale;
		normal[i][0] = 0.0; normal[i][1] = 0.0; normal[i][2] = 0.0;
		normalcount[i] = 0;
		}
	k = 0;
	for (i = 0; i < npoly; i++) {
		fscanf(fptr, "%d", &npvert[i]);
		if ((k + npvert[i]) > MAXPVERT) {
			printf("Too many polygon vertices\n");
			return(3);
			}
		pvertptr[i] = &pvert[k];
		for (j = 0; j < npvert[i]; j++) {
			fscanf(fptr, "%hd", &vtmp);
			pvert[k++] = vtmp - 1;
			}
		planeq[i][0] = planeq[i][1] = planeq[i][2] = planeq[i][3] = 0.0;
		v1 = pvert[k - 1]; v2 = pvert[k - 2]; v3 = pvert[k - 3];
		for (j = 0; j < 3; j++) {
			planeq[i][0] += vertices[v1][1] *
					(vertices[v2][2] - vertices[v3][2]);
			planeq[i][1] += vertices[v1][0] *
					(vertices[v3][2] - vertices[v2][2]);
			planeq[i][2] += vertices[v1][0] *
					(vertices[v2][1] - vertices[v3][1]);
			planeq[i][3] += vertices[v1][0] *
					((vertices[v3][1] * vertices[v2][2]) -
					 (vertices[v2][1] * vertices[v3][2]));
			vtmp = v1; v1 = v2; v2 = v3; v3 = vtmp;
			}
/*		if (planeq[i][3] > 0.0)
			for (j = 0; j <= 3; j++) planeq[i][j] = -planeq[i][j];
*/
		for (j = 1; j <= npvert[i]; j++) {	/* accum normls */
			vtmp = pvert[k-j];
			x = planeq[i][0]; y = planeq[i][1]; z = planeq[i][2];
			length = sqrt( x*x + y*y + z*z);
			normal[vtmp][0] += x/length;
			normal[vtmp][1] += y/length;
			normal[vtmp][2] += z/length;
			normalcount[vtmp]++;
			}
		}
	for (i = 0; i < nvert; i++) {
		normal[i][0] /= normalcount[i];
		normal[i][1] /= normalcount[i];
		normal[i][2] /= normalcount[i];
		}
	fclose(fptr);
	return(0);
	}

setvwpo(vx, vy, vz)
float vx, vy, vz;
	{
	int i;
	float diag, del, objdist, near;

	set_view_reference_point(vx, vy, vz);
	set_view_plane_normal(-vx, -vy, -vz);
	set_projection(PERSPECTIVE, 0., 0., 0.);
	set_view_plane_distance(256.0);
	if ((vx == 0.0) && (vz == 0.0))
		set_view_up_3(0.0, 0.0, vy);
	else
		set_view_up_3(0.0, 1.0, 0.0);
	set_window(-80.0, 80.0, -80.0, 80.0);
	diag = 0.0;
	for (i = 0; i < 3; i++) {
		del = bbox[i][1] - bbox[i][0];
		diag += del * del;
		}
	diag = sqrt(diag) / 2.0;
	objdist = sqrt( vx*vx + vy*vy + vz*vz);
	near = (diag >= objdist) ? objdist/2.0 : objdist-diag;
	set_view_depth( near, objdist + diag);
	set_window_clipping(TRUE);
	set_front_plane_clipping(TRUE);
	set_back_plane_clipping(TRUE);
	set_viewport_3(.125, .874, 0., .749, 0.0, 1.0);
	}

static float invxform[4][4];

drawobj()
	{
	int i;
	float x,y,z,x0,y0,z0,length;
	char ch;

	if (renderstyle && renderstyle<3) 
		map_ndc_to_world_3( -348., 348., -870., &x,&y,&z);
		map_ndc_to_world_3( 0.0, 0.0, 0.0, &x0,&y0,&z0);
		x -= x0; y -= y0; z -= z0; 
		length = sqrt( x*x + y*y + z*z);
		if (length != 0.0) {
			x /= length; y /= length; z /= length; 
			}
	    for (i = 0; i < nvert; i++) {
		cindex[i] =
		fabs(normal[i][0]*x +normal[i][1]*y +normal[i][2]*z) * 254.;
		if (cindex[i] < 4) cindex[i] = 4;
		if (cindex[i] > 248) cindex[i] = 248;
		}
	/* inquire_inverse_composite_matrix(invxform); */
	if (renderstyle == 3) set_zbuffer_cut( our_surface, xcut, zcut, 2);
	else set_zbuffer_cut( our_surface, xcut, zcut0, 2);
	for (i = 0; i < npoly; i++) {
		/* if (visible(planeq[i])) */
		drawface(i);
		}
	}

int visible(pln)
float pln[];
	{
	int i;
	float c;

	c = 0.0;
	for (i = 0; i < 4; i++)
		c += invxform[2][i] * pln[i];
	return(c < 0.0);
	}

drawface(p) int p;
{
	int i, j, k;
	short *ptr;

	ptr = pvertptr[p];
	for (i = 0; i < npvert[p]; i++) {
		j = *ptr++;
		xlist[i] = vertices[j][0]; ylist[i] = vertices[j][1];
		zlist[i] = vertices[j][2];
		if (renderstyle < 3) {
		    indxlist[i] = cindex[j];
		}else {
		    dxlist[i] = normal[j][0]; dylist[i] = normal[j][1];
		    dzlist[i] = normal[j][2];
	    	    }
		}
	if (renderstyle < 2) {
		if (renderhue) {
		    j = indxlist[0]>>2; if (j > 62) j = 62;
		    set_fill_index( j + renderhue*64 -63);
		    }
		else set_fill_index( indxlist[0]);
		}
	else if (renderstyle == 2) {set_vertex_indices( indxlist, npvert[p]);}
	else {set_vertex_normals(dxlist,dylist,dzlist,npvert[p]);}
	if (renderstyle == 0) polyline_abs_3( xlist,ylist,zlist,npvert[p]);
	else polygon_abs_3( xlist, ylist, zlist, npvert[p]);
}

static float maxvw, vwpp, maxvdim;
static float vlx, vby, vfz, vdx, vdy, vdz;
static float minleft, maxright, minbot, maxtop, minback, maxfront;

initvw()
	{
	int i;
	float ftmp;

	ftmp = bbox[0][1];
	for (i = 1; i < 3; i++)
		if (bbox[i][1] > ftmp)
			ftmp = bbox[i][1];
	maxvw = 16.0 * ftmp;
	vwpp = 2.0 * maxvw / 480.0;
	maxvdim = maxvw - ceil(vwpp);
	vlx = (bbox[0][0] + maxvw) / vwpp;
	vby = 100.0 + (bbox[1][0] + maxvw) / vwpp;
	vfz = 580.0 - (bbox[2][1] + maxvw) / vwpp;
	vdx = (bbox[0][1] - bbox[0][0]) / vwpp;
	vdy = (bbox[1][1] - bbox[1][0]) / vwpp;
	vdz = (bbox[2][1] - bbox[2][0]) / vwpp;
	minleft = bbox[0][0] - 5.0;
	maxright = bbox[0][1] + 5.0;
	minbot = bbox[1][0] - 5.0;
	maxtop = bbox[1][1] + 5.0;
	minback = bbox[2][0] - 5.0;
	maxfront = bbox[2][1] + 5.0;
	}

getxyz(px, py, pz)
float *px, *py, *pz;
	{
	int button;
	float mx, my, fx, fy;
	int checkreg();

	setvwpv();
	create_temporary_segment();
	set_primitive_attributes( &PRIMATTS);
	set_charprecision(STRING);
	draw_labels();
	draw_cursors(*px, *py, *pz);
	set_echo(LOCATOR, 1, 1);
	for (;;) {
		do
		    await_any_button_get_locator_2(20000000,1,&button,&mx,&my);
		while(button == 0);
		if (button == 3) break;
		map_ndc_to_world_2( mx, my, &fx, &fy);
		switch (checkreg(fx, fy, button, px, py, pz)) {
		case 0:	break;
		case 1:	draw_front(*px, *py);
			break;
		case 2:	draw_top(*pz);
			break;
		case 3:	menu_select();
			break;
			}
		}
	set_echo(LOCATOR, 1, 0);
	close_temporary_segment();
	}

setvwpv()
	{
	set_view_reference_point(0.0, 0.0, 0.0);
	set_view_plane_normal(0.0, 0.0, -1.0);
	set_view_plane_distance(0.0);
	set_projection(PARALLEL, 0.0, 0.0, 1.0);
	set_view_up_3(0.0, 1.0, 0.0);
	set_window(0.0, 1023.0, 0.0, 767.0);
	set_view_depth( 0.0, 1.0);
	set_window_clipping(FALSE);
	set_viewport_3(0.0, 1., 0.0, .75, 0.0, 1.);
	}

draw_labels()
	{
	int i;

	move_abs_3(50.0, 680.0, 0.5);
	line_rel_2(0.0, -75.0);
	line_rel_2(75.0, 0.0);
	move_abs_2(45.0, 670.0);
	line_rel_2(5.0, 10.0);
	line_rel_2(5.0, -10.0);
	move_abs_2(115.0, 610.0);
	line_rel_2(10.0, -5.0);
	line_rel_2(-10.0, -5.0);
	move_abs_2(130.0, 612.0);
	text("x");
	move_abs_2(48.0, 695.0);
	text("y");
	move_abs_2(220.0, 605.0);
	text("Front View");
	move_abs_2(0.0, 100.0);
	line_rel_2(480.0, 0.0);
	line_rel_2(0.0, 480.0);
	line_rel_2(-480.0, 0.0);
	line_rel_2(0.0, -480.0);
	for (i = 0; i <= 20; i++) {
		move_abs_2((float) 24 * i, 100.0);
		line_rel_2(0.0, i % 2 ? -5.0 : -10.0);
		}
	for (i = 0; i <= 20; i++) {
		move_abs_2(480.0, ((float) 24 * i) + 100.0);
		line_rel_2(i % 2 ? 5.0 : 10.0, 0.0);
		}
	move_abs_2(593.0, 605.0);
	line_rel_2(0.0, 75.0);
	line_rel_2(75.0, 0.0);
	move_abs_2(588.0, 615.0);
	line_rel_2(5.0, -10.0);
	line_rel_2(5.0, 10.0);
	move_abs_2(658.0, 685.0);
	line_rel_2(10.0, -5.0);
	line_rel_2(-10.0, -5.0);
	move_abs_2(673.0, 687.0);
	text("x");
	move_abs_2(591.0, 600.0);
	text("z");
	move_abs_2(763.0, 605.0);
	text("Top View");
	move_abs_2(543.0, 100.0);
	line_rel_2(480.0, 0.0);
	line_rel_2(0.0, 480.0);
	line_rel_2(-480.0, 0.0);
	line_rel_2(0.0, -480.0);
	for (i = 0; i <= 20; i++) {
		move_abs_2(543.0, ((float) 24 * i) + 100.0);
		line_rel_2(i % 2 ? -5.0 : -10.0, 0.0);
		}
	}

draw_cursors(x, y, z)
float x, y, z;
	{
	draw_front(x, y);
	draw_top(z);
	}

draw_front(x, y)
float x, y;
	{
	static float vx = 240.0, vy = 340.0;

	set_line_index(0);
	move_abs_3(vx, 101.0, 0.5);
	line_rel_2(0.0, 478.0);
	move_abs_2(1.0, vy);
	line_rel_2(478.0, 0.0);
	set_line_index(1);
	move_abs_2(vlx, vby);
	line_rel_2(vdx, 0.0);
	line_rel_2(0.0, vdy);
	line_rel_2(-vdx, 0.0);
	line_rel_2(0.0, -vdy);
	vx = (x + maxvw) / vwpp;
	vy = 100.0 + (y + maxvw) / vwpp;
	move_abs_2(vx, 101.0);
	line_rel_2(0.0, 478.0);
	move_abs_2(1.0, vy);
	line_rel_2(478.0, 0.0);
	}

draw_top(z)
float z;
	{
	static float vz = 340.0;

	set_line_index(0);
	move_abs_3(544.0, vz, 0.5);
	line_rel_2(478.0, 0.0);
	set_line_index(1);
	move_abs_2(vlx + 543.0, vfz);
	line_rel_2(vdx, 0.0);
	line_rel_2(0.0, vdz);
	line_rel_2(-vdx, 0.0);
	line_rel_2(0.0, -vdz);
	vz = 580.0 - (z + maxvw) / vwpp;
	move_abs_2(544.0, vz);
	line_rel_2(478.0, 0.0);
	}

int checkreg(vx, vy, butnum, px, py, pz)
float vx, vy, *px, *py, *pz;
int butnum;
	{
	float f;
	int insideobj();

	if ((vx >= 0.0) && (vx <= 480.0) && (vy >= 100.0) && (vy <= 580.0)) {
		if (butnum == 1) {
			f = vx * vwpp - maxvw;
			f = f <= maxvdim ? f : maxvdim;
			f = f >= -maxvdim ? f : -maxvdim;
			if (insideobj(f, *py, *pz))
				return(0);
			*px = f;
			return(1);
			}
		else {
			f = (vy - 100.0) * vwpp - maxvw;
			f = f <= maxvdim ? f : maxvdim;
			f = f >= -maxvdim ? f : -maxvdim;
			if (insideobj(*px, f, *pz))
				return(0);
			*py = f;
			return(1);
			}
		}
	else if ((vx >= 543.0)&&(vx <= 1023.0)&&(vy >= 100.0)&&(vy <= 580.0)) {
		if (butnum == 2) {
			f = (580.0 - vy) * vwpp - maxvw;
			f = f <= maxvdim ? f : maxvdim;
			f = f >= -maxvdim ? f : -maxvdim;
			if (insideobj(*px, *py, f))
				return(0);
			*pz = f;
			return(2);
			}
		else
			return(0);
		}
	else
		return(3);
	}

int insideobj(x, y, z)
float x, y, z;
	{
	if ((x < minleft) || (x > maxright))
		return(0);
	if ((y < minbot) || (y > maxtop))
		return(0);
	if ((z < minback) || (z > maxfront))
		return(0);
	return(1);
	}

build_menu()
{
    int i, j;

    create_retained_segment( 2);
	 set_primitive_attributes( &PRIMATTS);
	 move_abs_3( 100.,90.,0.0);
	 set_text_index( 1);
	 set_font( 1); set_charsize( 4.,3.);
	 move_abs_2( 10., 70.); set_pick_id( 1); text("Edges");
	 move_abs_2( 210., 70.); set_pick_id( 2); text("Fast Shade");
	 move_abs_2( 410., 70.); set_pick_id( 3); text("Gouraud");
	 move_abs_2( 610., 70.); set_pick_id( 4); text("Phong diffuse");
	 move_abs_2( 810., 70.); set_pick_id( 5); text("Phong specular");
	 move_abs_2( 10., 40.); set_pick_id( 6); text("Grey");
	 move_abs_2( 210., 40.); set_pick_id( 7); text("Red");
	 move_abs_2( 410., 40.); set_pick_id( 8); text("Green");
	 move_abs_2( 610., 40.); set_pick_id( 9); text("Blue");
	 move_abs_2( 810., 40.); set_pick_id(10); text("Yellow");
    close_retained_segment();
    set_segment_detectability( 2, 1);
    set_segment_visibility( 2, FALSE);
}

menu_select()
{
    int done, segnam, pickid, butnum;
    int hue;

    set_segment_visibility( 2, TRUE);
    done = FALSE;
    while ( !done) {
	await_pick( 100000000, 1, &segnam, &pickid);	/* pick menu item */
	if (segnam == 0) { done = TRUE; }
	else if (segnam == 2) {
	    if (pickid < 6) renderstyle = pickid -1;
	    else if (pickid < 11) renderhue = pickid -6;
	    }
    }
    if (renderhue == 0) define_color_indices( our_surface,0,255,red,grn,blu);
    else define_color_indices( our_surface,0,255,dred,dgrn,dblu);
		    /* ambient,diffuse,specular,flood,bump,hue,style */
    hue = renderhue;
    switch (renderstyle) {
    case 1: set_shading_parameters( .01, .96, .0, 0., 7.,hue,0); break;
    case 2: set_shading_parameters( .01, .96, .0, 0., 7.,hue,1); break;
    case 3: set_shading_parameters( .01, .95, .0, 0., 7.,hue,2); break;
    case 4: set_shading_parameters( .05, .50, .40, 0., 7.,hue,2); break;
    default: break;
    }
    set_segment_visibility( 2, FALSE);
}


