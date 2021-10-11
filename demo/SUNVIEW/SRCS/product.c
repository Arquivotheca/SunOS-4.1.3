#ifndef lint
static	char sccsid[] = "@(#)product.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

/*
	This demo program displays a Product Overview for the SUN
	workstation.
	*/
/*---------------------------*/
#include <stdio.h>
#include <usercore.h>
#include "demolib.h"

#define PRODUCT_SEGMENT 1

	short axl[100], bxl[100], ayl[100], byl[100], na, nb;
	int x0 = 500, y0 = 600;
	float cdx[] = { 2.,3.,4.,3.,2.,1.,0. };
	float cdy[] = { 0.,-1.,-2.,-3.,-4.,-3.,-2. };

	float	red[16] = 	{1.0, 0.0, 0.5, 0.0, 0.6, 0.7, 0.0, 0.0, },
		green[16] = 	{1.0, 0.0, 0.0, 0.0, 0.5, 0.0, 0.0, 0.0, },
		blue[16] = 	{1.0, 0.0, 0.7, 1.0, 0.0, 0.7, 0.0, 0.0, };

main(argc, argv)
int	argc;
char	*argv[];
{
	int quick_flag;
	quick_flag=quick_test(argc,argv);
	get_view_surface(our_surface, argv);
	if (initialize_core( DYNAMICC, SYNCHRONOUS,THREED)) exit(0);
	our_surface->cmapsize = 16;
	our_surface->cmapname[0] = '\0';
	if (initialize_view_surface( our_surface, FALSE)) exit(1);
	select_view_surface( our_surface);
	set_viewport_2( 0., 1., 0., .75);
	set_window( 0., 1023., 200., 1000.);
	set_output_clipping( TRUE);
	define_color_indices( our_surface, 0, our_surface->cmapsize-1,
		red,green,blue);
	
	create_retained_segment(PRODUCT_SEGMENT);
	    set_charprecision( CHARACTER);
	    set_font(3);  set_charsize(30.,30.);
	    set_text_index( 2);
	    move_abs_2( 90., 950.);
	    text("Sun-2/120 Workstation");
	    draw_boxes();
	    draw_lines();
	    draw_text();
	    features();
	close_retained_segment();
	if(quick_flag)
		sleep(30);
	else
		sleep(1000000);
	deselect_view_surface(our_surface);
	terminate_core();
}
/*--------------------------------*/
draw_boxes()
{
	set_line_index( 3);
	set_linewidth( .4);
	dbox( 65,500,400,800);		/* boundary */
	dbox( 120,520,70,130);		/* SMD disk controller */
	dbox( 490,520,70,130);		/* 9-track tape controller */
	dbox( 120,715,125,320);		/* Processor */
	dbox( 490,680,70,130);		/* Display controller */
	dbox( 490,790,70,130);		/* Display 1024 by 800 17" landscape */
	dbox( 680,680,70,130);		/* Ethernet controller */
	dbox( 140,740,55,65);		/* 68010 CPU */
	dbox( 240,740,55,65);		/* virtual memory MMU */
	dbox( 340,740,55,65);		/* main memory 1-4MB */
	}
/*--------------------------------*/
draw_lines()
{
	set_line_index( 3);
	set_linewidth( .4);
	move_abs_2( 195.,605.); line_abs_2( 195.,636.);		/* SMD */
	move_abs_2( 565.,605.); line_abs_2( 565.,636.);		/* Tape */
	move_abs_2( 282.,640.); line_abs_2( 282.,700.);		/* Proc */
	move_abs_2( 565.,640.); line_abs_2( 565.,665.);		/* Disp cont */
	move_abs_2( 755.,640.); line_abs_2( 755.,665.);		/* Ether cont */
	move_abs_2( 755.,765.); line_abs_2( 755.,796.);		/* Disp */
	move_abs_2( 565.,765.); line_abs_2( 565.,775.);
	move_abs_2( 280.,560.); line_rel_2( 40.,0.);
	move_abs_2( 280.,540.); line_rel_2( 40.,0.);
	move_abs_2( 650.,560.); line_rel_2( 40.,0.);
	move_abs_2( 650.,540.); line_rel_2( 40.,0.);
	set_charprecision( CHARACTER);
	set_font( STICK); set_charsize(10.,10.);;
	set_text_index( 1);
	move_abs_2( 320.,560.); text(" 130MB Disk");
	move_abs_2( 320.,540.); text(" 42MB Disk");
	move_abs_2( 690.,580.); text(" 1600 bpi");
	move_abs_2( 690.,560.); text(" 9-track Tape");
	move_abs_2( 690.,540.); text(" Archive Tape");
	}
draw_text()
{
	set_charprecision( CHARACTER);
	set_font( ROMAN);  set_charsize( 15.,15.);
	set_text_index( 5);
	move_abs_2( 170.,830.); text("Processor");
	set_text_index( 4);
	set_line_index( 4);
	set_font( SCRIPT);
	set_linewidth( .8);
	move_abs_2( 695.,820.); text("Ethernet");
	move_abs_2( 680.,800.); line_rel_2( 160.,0.);

	move_abs_2( 120.,660.); text("796-Bus");
	move_abs_2( 330.,660.); text("(Multibus)");
	move_abs_2( 120.,640.); line_rel_2( 720.,0.);

	set_charprecision( CHARACTER);
	set_font( STICK); set_charsize(10.,10.);;
	set_text_index( 1);
	move_abs_2( 145.,775.); text("68010");
	move_abs_2( 145.,755.); text(" CPU");

	move_abs_2( 135.,570.); text("Disk");
	move_abs_2( 135.,550.); text("Controller");
	move_abs_2( 135.,520.); text("SMD/SCSI");

	move_abs_2( 505.,570.); text("Tape");
	move_abs_2( 505.,550.); text("Controller");

	move_abs_2( 500.,740.); text("Color");
	move_abs_2( 500.,720.); text("Display");
	move_abs_2( 500.,700.); text("640 x 480");

	move_abs_2( 500.,850.); text("Bitmap");
	move_abs_2( 500.,830.); text("Display");
	move_abs_2( 500.,810.); text("1152 x 900");

	move_abs_2( 690.,730.); text("Ethernet");
	move_abs_2( 690.,710.); text("Controller");

	move_abs_2( 245.,790.); text("virt");
	move_abs_2( 245.,770.); text("memory");
	move_abs_2( 245.,750.); text(" MMU");

	move_abs_2( 345.,790.); text("main");
	move_abs_2( 345.,770.); text("memory");
	move_abs_2( 345.,750.); text("1-4MB");
	}
/*--------------------------------*/
dbox( x,y,h,w) short x,y,h,w;
{
	int i;
	move_abs_2( (float)x, (float)y);
	line_rel_2( 0., (float)h);
	for (i=0; i<7; i++) line_rel_2( -cdy[i],cdx[i]);
	line_rel_2( (float)w, 0.);
	for (i=0; i<7; i++) line_rel_2( cdx[i],cdy[i]);
	line_rel_2( 0.,(float)(-h));
	for (i=0; i<7; i++) line_rel_2( cdy[i],-cdx[i]);
	line_rel_2( (float)(-w),0.);
	for (i=0; i<7; i++) line_rel_2( -cdx[i],-cdy[i]);
	}
/*-----------------------------------*/
features()
{
	set_charprecision( CHARACTER);
	set_font( ROMAN); set_charsize(12.,12.);
	set_text_index( 1);
	move_abs_2( 90.,450.);
	text("- Processor");
	move_abs_2( 90.,425.);
	text("- Virtual Memory");
	move_abs_2( 90.,400.);
	text("- Network");
	move_abs_2( 90.,375.);
	text("- Software");
	move_abs_2( 90.,350.);
	text("- Peripherals");
	
	set_font( ROMAN); set_charsize(12.,12.);
	move_abs_2( 400.,450.);
	text("68010, 10MHz");
	move_abs_2( 400.,425.);
	text("16MB per process");
	move_abs_2( 400.,400.);
	text("10 Mb/sec Ethernet");
	move_abs_2( 400.,375.);
	text("UNIX 4.2, SIGGRAPH CORE Graphics");
	move_abs_2( 400.,350.);
	text("Color display, Mouse, RS-232 Ports");

	move_abs_2(120.,275.);
	text("The Sun-2/120 is one of a family of workstations");
	}

int quick_test(argc,argv) int argc; char *argv[];
	{
	while (--argc > 0) {
		if(!strncmp(argv[argc],"-q",2))
			return(TRUE);
		}
	return(FALSE);
	}
