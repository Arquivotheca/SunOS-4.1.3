/*	draw.c	1.1	92/07/30	*/
#include <usercore.h>
#include <sun/fbio.h>
#include "demolib.h"

#define MAPSIZE		256
static int colortable[] = {     1,  20,  40, 63 , 85, 105,
				127, 147, 167, 191, 254, 255 };

static int main_menu = 1,  attribute_menu = 2, menus = 2;
static int newseg = 3, segopen, opensegment;
static float xlist[100], ylist[100];
static int pointcount, colorflag;
double sin(), cos();

static int cg1dd();
static int cg2dd();
static int cg4dd();
static int cgpixwindd();
static int gp1dd();
static int gp1pixwindd();

/*-----------------Main--------------------*/

main(argc, argv) int argc; char *argv[];
{
    int done, segnam, pickid;

    set_up_core(argv);

    if((our_surface->dd == cg1dd) || (our_surface->dd == cgpixwindd) ||
	(our_surface->dd==cg2dd) || (our_surface->dd==cg4dd) ||
	(our_surface->dd==gp1dd) || (our_surface->dd==gp1pixwindd))
	colorflag=TRUE;
    else
	colorflag=FALSE;

    build_menus();
    done = FALSE;
    set_segment_visibility( main_menu, TRUE);
    while ( !done) {
        set_echo( LOCATOR,1, 1);		/* dingbat to menu */
	set_echo( PICK, 1,2);
	await_pick( 1000000, 1, &segnam, &pickid);	/* pick menu item */
	if (segnam == main_menu) {
	    set_segment_visibility( main_menu, FALSE);
	    switch (pickid) {
	    case 1: make_new_seg( XLATE2); break;	/* translatable seg */
	    case 2: make_new_seg( XFORM2); break;	/* New transform seg */
	    case 3: 					/* Delete segment */
		await_pick( 100000000,1,&segnam,&pickid);/* pick segment */
		if (segnam > menus) {
		    inquire_open_retained_segment( &opensegment);
		    delete_retained_segment( segnam);
		    if (opensegment == segnam) segopen = FALSE;
		}
		break;
	    case 4: go_draw_polyline(1); break;		/* Polyline draw */
	    case 5: go_draw_polygon();   break;		/* Polygon draw */
	    case 6: go_draw_raster();    break;		/* Raster draw */
	    case 7: go_draw_text();      break;		/* Text draw  */
	    case 8: go_draw_polyline(0); break;		/* Marker draw  */
	    case 9: go_transform_segment(0); break;	/* Position segment */
	    case 10: go_transform_segment(1); break;	/* Rotate segment  */
	    case 11: go_transform_segment(2); break;	/* Size segment  */
	    case 12: process_attribute_menu(); break;	/* attributes  */
	    case 13: go_save_segment();	break;		/* save segment  */
	    case 14: go_restore_segment(); break;	/* restore segment  */
	    case 15:
		if (segopen) close_retained_segment();	/* EXIT     */
		done = TRUE; break;
	    default:
	        set_segment_visibility( main_menu, FALSE);
		new_frame();
	    }
	    set_segment_visibility( main_menu, TRUE);
	    }
    }
    terminate_device( KEYBOARD, 1);
    terminate_device( LOCATOR, 1);
    deselect_view_surface(our_surface);
    terminate_view_surface(our_surface);
    terminate_core();
    return 0;
}
/*-------------------------------------------*/
go_draw_raster()		/* get raster prim from bitmap */
{
    int rasfid; char *rasfilename;
    int butnum;
    float xmin, ymin, x, y, xmax, ymax;
    float wx, wy;
    struct { int width, height, depth;
	short *bits; }raster;
    struct { int type, nbytes;
	char *data; }map;

    set_echo( LOCATOR,1, 6);			/* dingbat to start position */
    do {
	await_any_button_get_locator_2( 1000000,1, &butnum, &xmax, &ymax);
        if (butnum == 1) {
	    set_echo_position( LOCATOR,1, xmax, ymax);
	    xmin = xmax; ymin = ymax;
	}
    } while (butnum != 3);
    if (xmax<xmin) { x=xmax; xmax=xmin; xmin=x;}
    if (ymax<ymin) { y=ymax; ymax=ymin; ymin=y;}
    
    x = xmin;  y = ymin;
    map_ndc_to_world_2( x, y, &wx, &wy);
    set_pick_id( 1);
    move_abs_2( wx, wy);
    size_raster( our_surface, xmin, xmax, ymin, ymax, &raster);
    allocate_raster( &raster);
    get_raster( our_surface, xmin, xmax, ymin, ymax, 0, 0, &raster);
    if (raster.bits) {
	put_raster( &raster);
	rasfilename = "rasterfile";
	if( (rasfid = open( rasfilename, 1)) == -1) {	/* open the disk file */
	    rasfid = creat( rasfilename, 0755);
	    }
	if (rasfid != -1) {
	    map.type = 1;  map.nbytes = 0;  map.data = 0;
	    raster_to_file( &raster, &map, rasfid, 1);
	    close( rasfid);
	    }
	free_raster( &raster);
    }
    set_segment_detectability( opensegment, 5);
}

/*-------------------------------------------------------*/
make_new_seg( type) int type;
{
    if (segopen) close_retained_segment();
    set_image_transformation_type( type);
    opensegment = newseg++;
    create_retained_segment( opensegment);
    set_primitive_attributes( &PRIMATTS);
    segopen = TRUE;
}
/*--------------------------------------------------------------------*/
process_attribute_menu()
{
    int done, segnam, pickid, butnum;

    set_segment_visibility( attribute_menu, TRUE);
    done = FALSE;
    while ( !done) {
        set_echo( LOCATOR,1, 1);		/* dingbat to menu */
	await_pick( 1000000, 1, &segnam, &pickid);	/* pick menu item */
	if (segnam == 0) { 
	    await_any_button( 0, &butnum);
	    if (butnum == 3) done = TRUE;
	}
	else if (segnam == attribute_menu) {
	    if (pickid > 1  && pickid < 8 ) {
		set_charprecision( CHARACTER);
		set_font( pickid - 2);
	    }
	    else if (pickid > 7  && pickid < 12)
		set_linestyle( pickid - 8);
	    else if (pickid > 15 && pickid < 20)
		set_linewidth( (pickid-16)* 0.25);
	    else if (pickid > 19 && pickid < 32){
		set_line_index( colortable[pickid-20]);
		set_fill_index( colortable[pickid-20]);
		set_text_index( colortable[pickid-20]);
		}
	    else break;				/* EXIT     */
	}
    }
    set_segment_visibility( attribute_menu, FALSE);
}
/*---------------------------------------------------------------------*/
go_transform_segment( mode) int mode;	/* position 0,rotate 1,size 2 */
{
    int segnam, pickid, butnum, segtype;
    float sx0, sy0, ang0, tx0, ty0;
    float sx, sy, ang, tx, ty;
    float x, y, px, py;
    float dx, dy, nx, ny, sina, cosa;

    await_pick( 1000000000, 1, &segnam, &pickid);	/* pick segment */
    if (segnam <= 0) return (0);

    await_any_button_get_locator_2( 0,1, &butnum, &px, &py);
    inquire_segment_image_transformation_2( segnam,&sx0,&sy0,&ang0,&tx0,&ty0);
    sina = sin(ang0); cosa = cos(ang0);
    dx = px - (sx0 * cosa * px - sy0 * sina * py);
    dy = py - (sx0 * sina * px + sy0 * cosa * py);
    inquire_segment_image_transformation_type( segnam, &segtype);
    set_echo( LOCATOR,1, 0);			/* no echo, drag segment */
    do {
	await_any_button_get_locator_2( 0,1, &butnum, &x, &y);
	switch (mode) {			/* case mode of */
	case 0:					/* position segment */
	    tx = tx0 + (x-px);  ty = ty0 + (y-py);
	    if (segtype <= XLATE2)
		set_segment_image_translate_2( segnam, tx,ty);
	    else
		set_segment_image_transformation_2( segnam, sx0,sy0,ang0,tx,ty);
	    break;
	case 1:					/* rotate segment */
	    ang = ang0 + (x-px) * 6.28;
	    sina = sin(ang); cosa = cos(ang);
	    nx = px - (sx0 * cosa * px - sy0 * sina * py);
	    ny = py - (sx0 * sina * px + sy0 * cosa * py);
	    tx = tx0 + nx - dx;
	    ty = ty0 + ny - dy;
	    set_segment_image_transformation_2( segnam, sx0,sy0,ang,tx,ty);
	    if (segtype != XFORM2) return(0);
	    break;
	case 2:					/* size segment */
	    sx = sx0 + (x-px) * 15.;  sy = sy0 + (y-py) * 15.;
	    sina = sin(ang0); cosa = cos(ang0);
	    nx = px - (sx * cosa * px - sy * sina * py);
	    ny = py - (sx * sina * px + sy * cosa * py);
	    tx = tx0 + nx - dx;
	    ty = ty0 + ny - dy;
	    set_segment_image_transformation_2( segnam, sx,sy,ang0,tx,ty);
	    if (segtype != XFORM2) return(0);
	    break;
	default:;
	}
    } while (butnum != 3);
}
/*-------------------------------------------*/
go_save_segment()
{
	char string[80];  int length;
	int segnam, pickid;

	await_pick( 100000000, 1, &segnam, &pickid);
	if (segnam > menus) {
	    if (segopen && opensegment == segnam) {
		close_retained_segment();
	        segopen = FALSE;
		}
	    set_echo_position( KEYBOARD,1, 0.01, 0.02);
	    set_echo( KEYBOARD,1,1);		/* echo the text */
	    set_keyboard( 1, 80, "Filename: ", 10);	/* set prompt string */
	    await_keyboard( 1000000000,1, string, &length);
	    if (string[length-1] == '\n') string[length-1] = '\0';
	    save_segment( segnam, string);
	    }
}
/*-------------------------------------------*/
go_restore_segment()			/* restore a segment from disk */
{
	char string[80];  int length;

        if (segopen) close_retained_segment();
	opensegment = newseg++;
	set_echo_position( KEYBOARD,1, 0.01, 0.02);/* move to start positn */
	set_echo( KEYBOARD,1,1);		/* echo the text */
	set_keyboard( 1, 80, "Filename: ", 10);	/* set user char buf size */
	await_keyboard( 1000000000,1, string, &length);
	if (string[length-1] == '\n') string[length-1] = '\0';
	set_image_transformation_type( XLATE2);
	if ( !restore_segment( opensegment, string )) {
	    set_segment_detectability( opensegment, 5);
	    segopen = FALSE;
	    }
}
/*-------------------------------------------*/
go_draw_text()				/* text draw */
{
    int butnum;
    float wx, wy, x, y;
    char string[80];  int length;

    set_echo( LOCATOR,1, 1);			/* dingbat to start position */
    do {
	await_any_button_get_locator_2( 1000000,1, &butnum, &x, &y);
	if (butnum == 3) return(0);
    } while (butnum != 1);
    set_echo_position( KEYBOARD,1, x, y);	/* move to start positn */
    set_keyboard( 1, 80, "", 0);		/* set user char buf size */
    map_ndc_to_world_2( x, y, &wx, &wy);
    move_abs_2( wx, wy);
    set_echo( KEYBOARD,1,1);			/* echo the text */
    await_keyboard( 1000000000,1, string, &length);
    set_pick_id( 1);
    if (length && string[length-1] == '\n') string[length-1] = '\0';
    text( string);
    set_segment_detectability( opensegment, 10);
    return(0);
}
/*-------------------------------------------*/
go_draw_polyline( mode)	int mode;		/* polyline draw */
{
    int butnum;
    float wx, wy, x, y;

    set_echo( LOCATOR,1, 1);			/* dingbat to start position */
    do {
	await_any_button_get_locator_2( 1000000,1, &butnum, &x, &y);
	if (butnum == 3) return(0);
    } while (butnum != 1);
    set_echo_position( LOCATOR,1, x, y);	/* move to start position */
    map_ndc_to_world_2( x, y, &wx, &wy);
    move_abs_2( wx, wy);
    if (mode) set_echo( LOCATOR,1, 2);		/* rubberband to position */
    else {
	set_pick_id(1);
	marker_abs_2( wx, wy);
    }
    do {
	do {
	    await_any_button_get_locator_2( 1000000,1, &butnum, &x, &y);
	    if (butnum == 3) {
		set_segment_detectability( opensegment, 10); return(0);
	    }
	} while (butnum != 1);
        set_echo_position( LOCATOR,1, x, y);
	map_ndc_to_world_2( x, y, &wx, &wy);
	set_pick_id( 1);
	if (mode) line_abs_2( wx, wy);
	else marker_abs_2( wx, wy);
    } while (TRUE);
}
/*-------------------------------------------*/
go_draw_polygon()				/* polygon draw */
{
    int butnum;
    float wx, wy, x, y;

    set_echo( LOCATOR,1, 1);			/* dingbat to start position */
    do {
	await_any_button_get_locator_2( 1000000,1, &butnum, &x, &y);
	if (butnum == 3) return(0);
    } while (butnum != 1);
    set_echo_position( LOCATOR,1, x, y);	/* move to start position */
    map_ndc_to_world_2( x, y, &wx, &wy);
    move_abs_2( wx, wy);
    pointcount = 0;
    xlist[pointcount] = wx;
    ylist[pointcount++] = wy;
    set_echo( LOCATOR,1, 2);			/* rubberband to position */
    do {
	do {
	    await_any_button_get_locator_2( 1000000,1, &butnum, &x, &y);
	    if (butnum == 3 || pointcount > 98) {
		set_pick_id( 1);
		move_abs_2( xlist[0], ylist[0]);
		polygon_abs_2( &xlist[0], &ylist[0], pointcount);
		move_abs_2( xlist[0], ylist[0]);
		polyline_abs_2( &xlist[1], &ylist[1], pointcount-1);
    		set_segment_detectability( opensegment, 5);
		return(0);
		}
	} while (butnum != 1);
        set_echo_position( LOCATOR,1, x, y);
	map_ndc_to_world_2( x, y, &wx, &wy);
	line_abs_2( wx, wy);
	xlist[pointcount] = wx;
	ylist[pointcount++] = wy;
    } while (TRUE);
}
/*-----------------------------------------*/
set_up_core(argv) char *argv[];
{
    get_view_surface(our_surface,argv);
    our_surface->cmapsize = MAPSIZE;
    our_surface->cmapname[0] = '\0';
    if ( initialize_core(DYNAMICC, SYNCHRONOUS, THREED)) exit(0);
    if (initialize_view_surface( our_surface, FALSE)) exit(0);
    initialize_device( BUTTON, 1);		/* initialize input devices */
    initialize_device( BUTTON, 2);		/* initialize input devices */
    initialize_device( BUTTON, 3);		/* initialize input devices */
    initialize_device( LOCATOR, 1);
    initialize_device( PICK, 1);
    initialize_device( KEYBOARD, 1);
    set_keyboard( 1, 80, " ", 1);		/* set user char buf size */
    set_echo_position( LOCATOR, 1, 0., 0.);
    set_echo_surface( LOCATOR, 1, our_surface);
    set_echo_surface( PICK, 1, our_surface);
    set_echo_surface( KEYBOARD, 1, our_surface);
    set_pick(1,0.001);
    set_window(0.,511.,0.,383.);
    set_viewport_2( .01,.99,.006, .744);	/* init viewing transform */
    set_window_clipping(FALSE);
    set_output_clipping(TRUE);
    select_view_surface(our_surface);
}
/*------------------------------------------*/
static float colorbox[4] = {0., 25.,   0., -25.};
static float colorboy[4] = {25., 0., -25.,   0.};
static float attbox[5] = { 0.,  120., 0.,  -120. };
static float attboy[5] = { 190.,0.,  -190.,  0.};

static float redlist[MAPSIZE], grnlist[MAPSIZE], blulist[MAPSIZE];
static float redtex[] =
{.9961,.1765,.1334,.1334,.4667,.1334,.1334,.1334,.8001,.2667,.5334,.0};
static float grntex[] =
{.5334,.2079,.5334,.5334,.5334,.5020,.5020,.3334,.2667,.5334,.5334,.2667};
static float blutex[] = 
{   0.,   0.,.4001,   0.,.2118,.3529,.6471,.4001,.4001,.4001,.4001,.3882};

build_menus()
{
    int i, j;
    float white=1.0;

    define_color_indices( our_surface,MAPSIZE-1,MAPSIZE-1,&white,&white,&white);

    inquire_color_indices(our_surface,0,MAPSIZE-1,redlist,grnlist,blulist);
    if(!colorflag) {
        for (i=0;i<12;i++) {
            redlist[colortable[i]]=redtex[i];
            grnlist[colortable[i]]=grntex[i];
            blulist[colortable[i]]=blutex[i];
            define_color_indices(our_surface,0,MAPSIZE-1,redlist,grnlist,blulist);
            }
        }
        
    set_image_transformation_type(XLATE2);
    create_retained_segment( main_menu);			/* MAIN MENU  */
	 set_primitive_attributes( &PRIMATTS);
	 move_abs_2( 3., 116.);
   	 line_rel_2(0., 194.);
	 set_font( 1); set_charsize( 4.,3.);
	 move_abs_2( 6., 300.); set_pick_id( 1); text("New Seg xlate");
	 move_abs_2( 6., 288.); set_pick_id( 2); text("New Seg xform");
	 move_abs_2( 6., 276.); set_pick_id( 3); text("Delete Seg");
	 move_abs_2( 6., 264.); set_pick_id( 4); text("Lines");
	 move_abs_2( 6., 252.); set_pick_id( 5); text("Polygon");
	 move_abs_2( 6., 240.); set_pick_id( 6); text("Raster");
	 move_abs_2( 6., 228.); set_pick_id( 7); text("Text");
	 move_abs_2( 6., 216.); set_pick_id( 8); text("Marker");
	 move_abs_2( 6., 204.); set_pick_id( 9); text("Move");
	 move_abs_2( 6., 192.); set_pick_id(10); text("Rotate");
	 move_abs_2( 6., 180.); set_pick_id(11); text("Scale");
	 move_abs_2( 6., 168.); set_pick_id(12); text("Attributes");
	 move_abs_2( 6., 156.); set_pick_id(13); text("Save Seg");
	 move_abs_2( 6., 144.); set_pick_id(14); text("Restore Seg");
	 move_abs_2( 6., 132.); set_pick_id(15); text("Exit");
    close_retained_segment();
    set_segment_detectability( main_menu, 100);
    set_segment_visibility( main_menu, FALSE);

    create_retained_segment( attribute_menu);		/* ATTRIBUTE MENU */
	set_primitive_attributes( &PRIMATTS);
	move_abs_2( 5., 120.);
   	polyline_rel_2(attbox,attboy,4);
        set_charsize( 9.,9.);  set_charprecision(CHARACTER);
	for (i=ROMAN; i<=SYMBOLS; i++) {	/* font selections */
	    set_font( i);			/* pickids 2...7   */
	    move_abs_2( (i%3)*40.+8., 300.-(20.*(i/3)) ); set_pick_id( 2+i);
	    if (i==SYMBOLS) text("* , -");
	    else text("Abc");
	}
	set_linewidth( 0.);
	for (i=SOLID; i<=DOTDASHED; i++) {	/* linestyle selections */
	    set_linestyle( i);			/* pickids 8...11 */
	    move_abs_2( 8., 260.-i*10.); set_pick_id( 8+i);
	    line_abs_2( 75., 260.-i*10.);
	}
	set_linestyle(SOLID);
	for (i=0; i<4; i++) {			/* linewidth selections */
	    set_linewidth( i*0.25);		/* pickids 16...19      */
	    move_abs_2( 85., 260.-i*10.); set_pick_id( 16+i);
	    line_abs_2( 122., 260.-i*10.);
	}
	for (i=0; i<3; i++) {			/* color selections */
	    for (j=0; j<4; j++) {		/* pickids 20...31  */
		set_fill_index( colortable[i*4+j]);
		move_abs_2( j*30.+8., 200.-i*30.); set_pick_id( 20+i*4+j);
		polygon_rel_2( colorbox, colorboy, 4);
	    }
	}
    close_retained_segment();
    set_segment_detectability( attribute_menu, 100);
    set_segment_visibility( attribute_menu, FALSE);
    return(0);
}
