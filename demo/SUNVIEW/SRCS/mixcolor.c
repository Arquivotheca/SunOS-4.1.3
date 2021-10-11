/*	mixcolor.c	1.1	92/07/30	*/

/*	This program allows you to mix red, green, and blue and create 25
	samples. The program takes input from the left button of the mouse.
	To change the amount of a color first click the name of the color,
	position the mouse over the dial accordingly and click the mouse.
	To change one of the sample squares to the current color mix 
	click the mouse over the appropriate square. To exit the program
	click the QUIT box. */
	
#include <stdio.h>
#include <math.h>
#include <usercore.h>
#include "demolib.h"

#define STOP_SIGN	200

static float box_x[4]={170.,0.,-170.,0.};
static float box_y[4]={0.,120.,0.,-120.};
static float stop_sign_x[4]={100.,0.,-100.,0.};
static float stop_sign_y[4]={0.,200.,0.,-200.};
static float triangle_2_x[3]={15.,-30.,15.};
static float triangle_2_y[3]={-30.,0.,30.};
static float triangle_1_x[3]={15.,-30.,15.};
static float triangle_1_y[3]={30.,0.,-30.};

static float redtbl[30]={0.95,0.,0.95,0.0,0.,0.2,0.4,0.6,0.8,0.95,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.2,0.4,0.6,0.8,0.95,0.1,0.1,0.1,0.1,0.1};

static float grntbl[30]={0.95,0.,0.0,0.95,0.,0.1,0.1,0.1,0.1,0.1,0.2,0.4,0.6,0.8,0.95,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.2,0.4,0.6,0.8,0.95};

static float blutbl[30]={0.95,0.,0.0,0.0,0.95,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.1,0.2,0.4,0.6,0.8,0.95,0.2,0.4,0.6,0.8,0.95,0.2,0.4,0.6,0.8,0.95};

static float current_red_value, current_grn_value, current_blu_value;

int cg1dd();
int cgpixwindd();
int gp1dd();
int gp1pixwindd();

main(argc,argv)
	int argc;
	char *argv[];
	{
	int pickid, pickseg;
	get_view_surface(our_surface,argv);

	/* if monochrome display then set colors to black */
	if((our_surface->dd != cg1dd) && (our_surface->dd != cgpixwindd) ||
		(our_surface->dd==gp1dd) || (our_surface->dd==gp1pixwindd)) {
		redtbl[2]=grntbl[2]=blutbl[2]=0.;
		redtbl[3]=grntbl[3]=blutbl[3]=0.;
		redtbl[4]=grntbl[4]=blutbl[4]=0.;
		}
	go_setup_core();
	current_red_value = 0.5;
	current_grn_value = 0.5;
	current_blu_value = 0.5;
	create_layout();
	create_dials();
	create_boxes();
	create_stop_sign();
	while (TRUE) {
		set_echo(PICK,1,1);
		set_echo(LOCATOR,1,1);
		await_pick(100000,1,&pickseg,&pickid);
		switch(pickseg) {
			case 0:		break;
			case 400:	change_dial(0);
					break;
			case 401:	change_dial(1);
					break;
			case 402:	change_dial(2);
					break;
			case STOP_SIGN:	shutdown_core();
				  	break;
			default:	change_box_color(pickseg);
					break;
			}
		}
	}

go_setup_core()
	{
	if(initialize_core(DYNAMICC,SYNCHRONOUS,TWOD))
		exit(1);
	our_surface->cmapsize = 32;
	if(initialize_view_surface(our_surface,FALSE))
		exit(2);
	if(select_view_surface(our_surface))
		exit(3);
	set_window(-50.,1050.,-50.,1050.);
	define_color_indices(our_surface,0,29,redtbl,grntbl,blutbl);

	initialize_device(PICK,1);
	initialize_device(LOCATOR,1);
	initialize_device(VALUATOR,1);
	initialize_device(BUTTON,1);
	set_echo_surface(LOCATOR,1,our_surface);
	set_echo_surface(PICK,1,our_surface);
	set_echo_surface(VALUATOR,1,our_surface);
	}

create_layout()
	{
	int i;

	set_charprecision(CHARACTER);
	set_detectability(100);
	create_retained_segment(400);
	set_text_index(2);
	set_charsize(16.,40.);
	move_abs_2(30.,1000.);
	text("RED");
	close_retained_segment(400);

	create_retained_segment(401);
	set_text_index(3);
	set_charsize(16.,40.);
	move_abs_2(30.,900.);
	text("GREEN");
	close_retained_segment(401);

	create_retained_segment(402);
	set_text_index(4);
	set_charsize(16.,40.);
	move_abs_2(30.,800.);
	text("BLUE");
	close_retained_segment(402);

	for (i=0;i<3;i++) {
		set_detectability(0);
		create_retained_segment(600+i);
		set_detectability(0);
		set_line_index(2+i);
		move_abs_2(200.,1000.-100.*i);
		line_abs_2(700.,1000.-100.*i);
		close_retained_segment(600+i);
		}
	}

create_boxes()
	{
	int i,j,segnum;
	for(i=0;i<5;i++)
		for(j=0;j<5;j++) {
			segnum=5*i+j+1;
			set_detectability(100);
			create_retained_segment(segnum);
			set_fill_index(segnum+4);
			move_abs_2(190.*j,140.*i);
			polygon_rel_2(box_x,box_y,4);
			close_retained_segment(segnum);
			}
		}
			
create_stop_sign()
	{
	create_retained_segment(STOP_SIGN);
	set_detectability(100);
	move_abs_2(800.,800.);
	set_fill_index(0);
	polygon_rel_2(stop_sign_x,stop_sign_y,4);
	move_abs_2(800.,800.);
	set_line_index(1);
	polyline_rel_2(stop_sign_x,stop_sign_y,4);
	move_abs_2(810.,890.);
	set_charprecision(CHARACTER);
	set_charsize(16.,40.);
	set_text_index(1);
	text("QUIT");
	close_retained_segment(STOP_SIGN);
	}

change_box_color(pickseg) int pickseg;
	{
	redtbl[pickseg+4]=current_red_value;
	grntbl[pickseg+4]=current_grn_value;
	blutbl[pickseg+4]=current_blu_value;
	define_color_indices(our_surface,0,29,redtbl,grntbl,blutbl);
	set_segment_visibility(pickseg,FALSE);  /* blink to set new color */
	set_segment_visibility(pickseg,TRUE);
	}

create_dials()
	{
	int i;
	for (i=0;i<3;i++) {
		set_image_transformation_type(XLATE2);
		create_retained_segment(500+i);
		set_fill_index(1);
		move_abs_2(450.,960.-100.*i);
		set_fill_index(2+i);
		polygon_rel_2(triangle_1_x,triangle_1_y,3);
		move_abs_2(450.,1040.-100.*i);
		polygon_rel_2(triangle_2_x,triangle_2_y,3);
		close_retained_segment(500+i);
		set_image_transformation_type(NONE);
		}
	}

change_dial(i) int i;
	{
	int button_number;
	float val_value_ndc, val_value_world;
	float initial_dial_x = 450.;
	float x_offset_world;
	float dummy;

	while(button_number != 1) {
		await_any_button_get_valuator(0,1,&button_number,&val_value_ndc);
		}

	map_ndc_to_world_2(val_value_ndc,0.,&val_value_world,&dummy);
	if (val_value_world > 700.) val_value_world = 700.;
	if (val_value_world < 200.) val_value_world = 200.;

	switch(i) {
		case 0: current_red_value = (val_value_world - 200.)/ 500.;
			break;
		case 1: current_grn_value = (val_value_world - 200.)/ 500.;
			break;
		case 2: current_blu_value = (val_value_world - 200.)/ 500.;
			break;
		}

	x_offset_world = val_value_world - initial_dial_x;
	map_world_to_ndc_2(x_offset_world-45.,0.,&val_value_ndc,&dummy);
	set_segment_image_translate_2(500+i,val_value_ndc,0.);
	}

shutdown_core()
	{
	delete_all_retained_segments();
	deselect_view_surface(our_surface);
	terminate_core();
	exit(0);
	}
