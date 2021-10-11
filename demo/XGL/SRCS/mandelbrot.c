#ifndef lint
static char sccsid[] = "@(#)mandelbrot.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif

#include <stdio.h>
#include <signal.h>
#include <xgl/xgl.h>
#include "WS_macros.h"
#include "XGL.icon"

/*
 * For sunview
 */
#define W_HEIGHT  800   
#define W_WIDTH   1000
#define C_HEIGHT  550   
#define C_WIDTH	  550
#define Init_Pt_F2D(pt, X, Y) (pt.x = X,  pt.y = Y)

/* 
 * global extern defines 
 */

extern void reset_proc();
extern void reset_vdc();
extern void redraw_proc();
extern void granular_proc();
extern void iterate_proc();
extern void go_home();
extern void create_panel();
extern void zoom_pan_proc();
static void update_panel();
static void choose_cmap_proc();
static void canvas_repaint_proc();
static void canvas_resize_proc();
static Notify_value  quit_proc();

/*
 * Global vars
 */

static Frame		frame;	
static Canvas		canvas;			/* graphics canvas */	
static Panel		panel;
static Panel_item	draw, quit;
static struct rect	*crect;			/*canvas size & location */
static Panel_item	eye_slider_itemx, 
			eye_slider_itemy, 
			eye_slider_itemz,
			fly_mode_item;
static Panel_item	center_x_item, center_y_item;
static Panel_item	delta_x_item, delta_y_item;
static Panel_item	sample_size_item, iterations_item;


static float	    zoom_coord_x, zoom_coord_y;
static int	    can_wid, can_hgt;
static Xgl_sys_state	sys_state;


/*
 * For the FRACTAL stuff only
 */
 
#define COL_WHITE	127
#define BUFFER_SIZE	1
#define MY_CMAP_SIZE	128

typedef struct 
	{ float real, 
		imag;
	} COMPLEX;

#define C_SQ(z_out, z_in)	\
	{			\
	 z_out.imag = 2*z_in.real*z_in.imag;	\
	 z_out.real = temp.real - temp.imag;	\
	}

#define C_ADD(z_out, z_op1, z_op2)	\
	{				\
	 z_out.real = z_op1.real+z_op2.real;	\
	 z_out.imag = z_op1.imag+z_op2.imag;	\
	}
	
#define C_ABS(out, z)	 		\
	{				\
	 temp.real = z.real*z.real;		\
	 temp.imag = z.imag*z.imag;		\
	 out = temp.real+temp.imag;		\
	}

	
static	Xgl_color	col,  white;
static  Xgl_color_facet	*rects_facet;
static  Xgl_rect_list	rect_list;
static  Xgl_rect_f2d	rect;
static	Xgl_2d_ctx	ctx;
static	Xgl_trans	transform;
static	Xgl_ras		ras;
static  Xgl_cmap	cmap=NULL;
static  Xgl_pt_list	zoom_pl;
static  Xgl_pt_f2d	zoom_pts[5];
static  Xgl_color	c[128];
static  Xgl_color_list	clist;

static  COMPLEX center, range, granularity, scale;
static  int  iterations;
static	int shift_factor;
static	int pix_count=0;


static
void put_rectangle(color, x, y)
Xgl_color *color;
float x, y;
{
    int pt_index;
    int i;
    float temp_x = x+granularity.real;	
    float temp_y = y+granularity.imag; 

    rect.corner_min.x = x;
    rect.corner_min.y = y;
    rect.corner_min.flag = 0;
    rect.corner_max.x = temp_x;
    rect.corner_max.y = temp_y;
    
    xgl_object_set(ctx, XGL_CTX_SURF_FRONT_COLOR, color, 0);
    xgl_multirectangle(ctx, &rect_list);
}

static
void recalc_granularity()
{				
    Xgl_pt_i2d ras_width;

    xgl_object_get(ras, XGL_RAS_WIDTH,  &ras_width.x);
    xgl_object_get(ras,  XGL_RAS_HEIGHT, &ras_width.y);
    granularity.real = scale.real*2*range.real / (float)ras_width.x;
    granularity.imag = scale.imag*2*range.imag / (float)ras_width.y; 
}

/*
 *  This is the heart of the program
 */
static
void page_proc()
{
COMPLEX		z, c;
COMPLEX		temp;	
Xgl_color	color;
int		color_count;
float		val;

update_panel();
recalc_granularity();

if ((granularity.real > 1.e-4) && (granularity.imag > 1.e-4)) {
  for (c.real=center.real - range.real; 
       c.real<=center.real + range.real; 
       c.real+=granularity.real
      )
   {
    for (c.imag=center.imag - range.imag; 
         c.imag<=center.imag + range.imag; 
         c.imag+=granularity.imag
        )
       {
	z.real = 0.0; z.imag=0.0; 
	temp.real=0.0; temp.imag=0.0;
	for (color_count=0; color_count < (iterations-1); color_count++)
	   {
	    C_SQ(z,z);
	    C_ADD(z, z, c);
	    C_ABS(val, z);
	    if (val >= 4) 
    	       break;
	   }
	color.index = color_count >> shift_factor;
	put_rectangle(&color, c.real, c.imag);
       }
   }
 }
}

main(argc, argv)
int argc;
char **argv;
{


xgl_icon = icon_create(ICON_IMAGE, &icon_pixrect, 0);

sys_state = xgl_open(0);

(void) signal(SIGINT, go_home);
create_panel();
reset_proc();

xv_main_loop(frame);
}


static
void update_panel()
{
char output_str[50];

sprintf(output_str, "%1.8f", center.real);
panel_set_value(center_x_item, output_str );
sprintf(output_str, "%1.8f", center.imag);
panel_set_value(center_y_item, output_str );

sprintf(output_str, "%1.8f", range.real);
panel_set_value(delta_x_item, output_str );
sprintf(output_str, "%1.8f", range.imag);
panel_set_value(delta_y_item, output_str );

panel_set_value(sample_size_item, (int) scale.real);
panel_set_value(iterations_item, iterations);
}

static 
void create_panel ()
{

    /* Processing begins here */
    if ((frame = xv_create (NULL, FRAME,
			WIN_DYNAMIC_VISUAL, TRUE,
        		FRAME_ICON,   xgl_icon,
			FRAME_LABEL, "Fractal Demo",
			WIN_HEIGHT, W_HEIGHT,
			WIN_WIDTH, W_WIDTH,
			0)) == NULL) {
	printf ("Cannot make frame\n");
	exit (0);
    }

    (void) notify_interpose_destroy_func(frame, quit_proc);
   
    /* Create the control panel  */
    panel = xv_create (frame, PANEL, WIN_COLUMNS, 30,
			PANEL_LAYOUT, PANEL_VERTICAL, 0);

    center_x_item = panel_create_item(panel, PANEL_TEXT,
                	PANEL_LABEL_STRING,     "Ctr x;",
                	XV_X,           xv_col(panel, 1),
                	XV_Y,           xv_row(panel, 1),
                        PANEL_VALUE_DISPLAY_LENGTH, 12,
                        0);

    center_y_item = panel_create_item(panel, PANEL_TEXT,
                	PANEL_LABEL_STRING,     "Ctr y:",
                	XV_X,           xv_col(panel, 1),
                	XV_Y,           xv_row(panel, 2),
                        PANEL_VALUE_DISPLAY_LENGTH, 12,
                        0);

    delta_x_item = panel_create_item(panel, PANEL_TEXT,
                	PANEL_LABEL_STRING,     "Rad x:",
                	XV_X,           xv_col(panel, 1),
                	XV_Y,           xv_row(panel, 3),
                        PANEL_VALUE_DISPLAY_LENGTH, 12,
                        0);
    delta_y_item = panel_create_item(panel, PANEL_TEXT,
                	PANEL_LABEL_STRING,     "Rad y:",
                	XV_X,           xv_col(panel, 1),
                	XV_Y,           xv_row(panel, 4),
                        PANEL_VALUE_DISPLAY_LENGTH, 11,
                        0);

    panel_create_item (panel, PANEL_BUTTON,
		        PANEL_LABEL_STRING, "Reset",
		        PANEL_NOTIFY_PROC, reset_proc,
		        XV_X,  xv_col(panel, 2),
		        XV_Y,  xv_row(panel, 6),
		        0);
		        
    panel_create_item (panel, PANEL_BUTTON,
		        PANEL_LABEL_STRING, "Redraw",
		        PANEL_NOTIFY_PROC, redraw_proc,
		        XV_X,  xv_col(panel, 2),
		        XV_Y,  xv_row(panel, 8),
		        0);

   panel_create_item  (panel, PANEL_BUTTON,
		        PANEL_LABEL_STRING, "Quit",
		        PANEL_NOTIFY_PROC, go_home,
		        XV_X,  xv_col(panel, 2),
		        XV_Y,  xv_row(panel, 10),
		        0);

   sample_size_item = panel_create_item (panel, PANEL_SLIDER,
			 PANEL_MIN_VALUE, 1,
			 PANEL_MAX_VALUE, 32,
			 PANEL_LABEL_STRING, "Sample Size",
			 XV_X, xv_col(panel, 1),
			 XV_Y, xv_row(panel, 13),
			 PANEL_SLIDER_WIDTH, xv_col(panel, 10), 
		 	 PANEL_SHOW_RANGE,	FALSE,
			 PANEL_SHOW_VALUE,	TRUE,
			 PANEL_PAINT, PANEL_CLEAR,
		         PANEL_VALUE, 32,
			 PANEL_NOTIFY_PROC, granular_proc, 0);

   iterations_item = panel_create_item (panel, PANEL_SLIDER,
		         PANEL_VALUE, MY_CMAP_SIZE,
			 PANEL_MIN_VALUE, 8,
			 PANEL_MAX_VALUE, 256,
			 PANEL_LABEL_STRING, "Iterations",
			 XV_X, xv_col(panel, 1),
			 XV_Y, xv_row(panel, 17),
			 PANEL_SLIDER_WIDTH, xv_col(panel, 10), 
		 	 PANEL_SHOW_RANGE,	FALSE,
			 PANEL_SHOW_VALUE,	TRUE,
			 PANEL_PAINT, PANEL_CLEAR,
			 PANEL_NOTIFY_PROC, iterate_proc, 0);

   panel_create_item(panel, PANEL_CYCLE,
			XV_Y,      xv_row(panel, 20),
			XV_X,      xv_col(panel, 1),
			PANEL_NOTIFY_PROC, choose_cmap_proc,
			PANEL_LABEL_STRING, "Cmap:", 
			PANEL_CHOICE_STRINGS, "Crayola", "Green Grade", 0,
			PANEL_VALUE,       0,
			0);

    if ((canvas = xv_create (frame, CANVAS,
			WIN_SHOW, TRUE,
			WIN_DYNAMIC_VISUAL, TRUE,
			CANVAS_RETAINED, FALSE,
			CANVAS_AUTO_CLEAR, FALSE,
			CANVAS_FIXED_IMAGE, FALSE,
			WIN_EVENT_PROC, zoom_pan_proc,
			WIN_RIGHT_OF, panel,
			 0)) == NULL) {
	printf ("Cannot make canvas\n");
	exit (0);
    }

    xv_set (canvas,
		WIN_CONSUME_PICK_EVENTS,
			WIN_MOUSE_BUTTONS, LOC_DRAG, 0,
		0);

    /* Get the size of the canvas in pixels */
    can_wid = (int) xv_get (canvas, CANVAS_WIDTH);
    can_hgt = (int) xv_get (canvas, CANVAS_HEIGHT);



    {
	Atom	catom;
	Window	canvas_window, frame_window;

	display = (Display *)xv_get(frame, XV_DISPLAY);
	pw = (Xv_Window)canvas_paint_window(canvas);
	canvas_window = (Window)xv_get(pw, XV_XID);
	frame_window = (Window)xv_get(frame, XV_XID);
	xv_set(pw,
		WIN_EVENT_PROC, zoom_pan_proc,
		WIN_CONSUME_EVENTS, LOC_DRAG, 0,
		0);
	catom = XInternAtom(display, "WM_COLORMAP_WINDOWS", False);
	XChangeProperty(display, frame_window, catom, XA_WINDOW, 32,
	PropModeAppend, &canvas_window, 1);
	xgl_x_win.X_display = (void *)XV_DISPLAY_FROM_WINDOW(pw);
	xgl_x_win.X_window = (Xgl_usgn32)canvas_window;
	xgl_x_win.X_screen = (int)DefaultScreen(display);

	xv_set(canvas, CANVAS_RESIZE_PROC, canvas_resize_proc,
			CANVAS_REPAINT_PROC, canvas_repaint_proc, 0);

    	ras = xgl_window_raster_device_create(XGL_WIN_X, &xgl_x_win, 0);
    }

    white.index = COL_WHITE;
    ctx = xgl_2d_context_create(XGL_CTX_DEVICE,  ras, 
	                XGL_CTX_VDC_MAP, XGL_VDC_MAP_ALL,
                        XGL_CTX_LINE_COLOR, &white,
			0);

    choose_cmap_proc(NULL, 0, NULL);
    xgl_object_get(ctx, XGL_CTX_VIEW_TRANS, &transform);

    rect_list.num_rects = 1;
    rect_list.rect_type = XGL_MULTIRECT_F2D;
    rect_list.bbox = NULL;
    rect_list.rects.f2d = &rect;

    /* 
     * init point list--used for drawing the zoom area. 
     */
    zoom_pl.pt_type = XGL_PT_F2D;
    zoom_pl.bbox = NULL;
    zoom_pl.num_pts = 5;
    zoom_pl.pts.f2d = zoom_pts;

}

/* Quitting routines */
static
void go_home()
{
    xgl_close(sys_state);
    exit(0);
}


static
Notify_value quit_proc(frame, status)

Frame           frame;
Destroy_status  status;
{

    if (status == DESTROY_CHECKING)
        xgl_close(sys_state);
    return(notify_next_destroy_func(frame, status));
}


static
void reset_vdc()
{
Xgl_bounds_f2d vdc;

  vdc.xmin = center.real - range.real;
  vdc.xmax = center.real + range.real;
  vdc.ymin = center.imag - range.imag;
  vdc.ymax = center.imag + range.imag;
  xgl_object_set(ctx, XGL_CTX_VDC_WINDOW, &vdc,  0);
}

static 
void reset_proc()
{
/* 
 * Init globals
 */
  center.imag=0.0; center.real=0.0;
  range.imag=2.0; range.real=2.0;
  scale.real=32.0; scale.imag=32.0;
  iterations = MY_CMAP_SIZE;
  
  xgl_context_new_frame(ctx);
  reset_vdc();
  shift_factor = (iterations-1)/ MY_CMAP_SIZE;
  update_panel();
  page_proc();
}

static
void granular_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
  scale.real = scale.imag = (float)value;
}

static
void iterate_proc(item, value, event)
Panel_item item;
int value;
Event *event;
{
  iterations = value;
}

static
void redraw_proc()
{
  xgl_context_new_frame(ctx);
  page_proc();
}

/* =============== Z O O M M I N G  IN ! ! ================== */

#undef MAX
#undef MIN

#define MAX(a,b)    (((a) > (b)) ? (a):(b))
#define MIN(a,b)    (((a) <(b)) ? (a):(b))
#define _ABS(x)	    ((x) < 0 ? -(x) : (x))

static void
pix_to_world(pix_x, pix_y, wor_x, wor_y)

    float     pix_x, pix_y;
    float     *wor_x, *wor_y;
{
    *wor_x =  2*range.real*pix_x/can_wid + center.real - range.real; 
    *wor_y =  2*range.imag*pix_y/can_hgt + center.imag - range.imag;
}


/* Conversion routine for delta calculations in world coordinates*/
static void
world_delta(p1_x, p1_y, p2_x, p2_y, wor_wid, wor_hgt,diff_x, diff_y)

    int	    p1_x, p1_y, p2_x, p2_y;
    int     *diff_x, *diff_y, wor_wid,  wor_hgt;
{
    int	    sub;

    /* x delta */
    sub = p1_x -p2_x;
    *diff_x = ((int)((wor_wid*sub)/can_wid));

    /* y delta */
    sub = p1_y - p2_y;
    *diff_y = ((int)((-wor_hgt*sub)/can_hgt));
}

/* Draw the rubber-banding box for zoom */

static
void zoom_box(pt1_x, pt1_y, pt2_x, pt2_y)
float  pt1_x, pt1_y, pt2_x, pt2_y;
{

    Init_Pt_F2D(zoom_pts[0], pt1_x, pt1_y);
    Init_Pt_F2D(zoom_pts[1], pt1_x, pt2_y);
    Init_Pt_F2D(zoom_pts[2], pt2_x, pt2_y);
    Init_Pt_F2D(zoom_pts[3], pt2_x, pt1_y);
    Init_Pt_F2D(zoom_pts[4], pt1_x, pt1_y);

    xgl_multipolyline(ctx, NULL, 1, &zoom_pl);
}


static	float	anchor_x, anchor_y;  /* where button goes down */
static	float	last_x, last_y;

/*Mouse control zoom & pan */
void
zoom_pan_proc(canvas, event, arg)
	
Canvas canvas;
Event *event;
caddr_t arg;
{
    
static	float	world_x, world_y;
static	float	current_x, current_y;/* current & last allow rbbr-band */
static  int     first_event = 1;  
int		width, height;

    /* Zoom with rubber banding */
   switch(event_action(event)) {
    case LOC_WINENTER:
	case LOC_WINEXIT:
	  xgl_window_raster_reinstall_colormap(ras);
      break;
    case WIN_RESIZE:
      width = (int)xv_get(pw, XV_WIDTH);
      height = (int)xv_get(pw, XV_HEIGHT);
      xgl_window_raster_resize(ras, width, height);
      break;
    case WIN_REPAINT:
      page_proc();
    break;
    case ACTION_SELECT:
	if (event_is_down(event)){
	        /* draw the rubberband box in white lines with XOR */
		xgl_object_set(ctx, XGL_CTX_ROP, XGL_ROP_XOR, 0);
		pix_to_world((float)event_x(event),
			     (float)event_y(event), 
			     &world_x, &world_y);
		last_x    = current_x = anchor_x  = world_x;
		last_y    = current_y = anchor_y  = world_y;
	        first_event = 0;
	} else if (event_is_up(event)){
		if (first_event) break;
		zoom_box(anchor_x, anchor_y, last_x, last_y);
		if (last_x > anchor_x) {
		    center.real   = anchor_x + (last_x - anchor_x)* 0.5;
		} else {
		    center.real   = anchor_x - (anchor_x - last_x)* 0.5;
		}
		if (last_y > anchor_y) {
		    center.imag   = anchor_y + (last_y - anchor_y)* 0.5;
		} else {
		    center.imag   = anchor_y - (anchor_y - last_y)* 0.5;
		}
		range.real = _ABS(anchor_x-last_x)/2.0;
		range.imag = _ABS(anchor_y-last_y)/2.0;
		reset_vdc();
		xgl_context_new_frame(ctx);
		xgl_object_set(ctx, XGL_CTX_ROP, XGL_ROP_COPY, 0);
    		page_proc();
	}
    break;

    /*This is to zoom out */
    case ACTION_ADJUST:

	    if (event_is_down(event)){
		   
		   range.real *= 2.0;
		   range.imag *= 2.0;
		   reset_vdc();
		   xgl_context_new_frame(ctx);
		   page_proc();
	    }
    break;

    /* Drag event !! */
    case LOC_DRAG:
        if (event->ie_shiftmask == 128)
	  {

	    if (first_event) break;     
	    /* This is for drawing the rubber-banding zoom-box */
	    pix_to_world((float)event_x(event),
	    		 (float)event_y(event), 
			  &world_x, &world_y);

	    current_x = world_x;
	    current_y = world_y;

	    /* erase the previous box*/
	    zoom_box( anchor_x, anchor_y, last_x, last_y);

	    /* draw a new box */
	    zoom_box( anchor_x, anchor_y, current_x, current_y);
	    last_x = current_x;
	    last_y = current_y;
	   }
	
    break;

    default:
    break;
    }	
    
}

/* ========================================================= */

/* COLORFUL 64 Crayola */

#define Color_RGB(c, R, G, B) (c.rgb.r = R, c.rgb.g = G, c.rgb.b = B)

static
void choose_cmap_proc(item, value,event)
Panel_item      item;
int             value;
Event           *event;
{
    int	i;
    
    switch(value) {
      case 0:  /* crayola */
	for (i = 0; i < MY_CMAP_SIZE; i++) {
	    switch(i) {
	      case  0: /* BLACK */      
		Color_RGB( c[i], 0.00, 0.00, 0.00 ); break;
	      case  1: /* APRICOT */         
		Color_RGB( c[i], 1.00, 0.36, 0.30 );break;
	      case  2: /* AQUAMARINE */      
		Color_RGB( c[i], 0.00, 0.81, 0.64 ); break;
	      case  3: /* BITTERSWEET */      
		Color_RGB( c[i], 0.81, 0.04, 0.00 ); break;
	      case  4: /* BLUE */      
		Color_RGB( c[i], 0.00, 0.00, 1.00 ); break;
	      case  5: /* BLUE_GREEN */      
		Color_RGB( c[i], 0.00, 0.49, 0.25 ); break;
	      case  6: /* BLUE_GREY */      
		Color_RGB( c[i], 0.16, 0.09, 0.25 ); break;
	      case  7: /* BLUE_VIOLET */      
		Color_RGB( c[i], 0.09, 0.00, 0.20 ); break;
	      case  8: /* BRICK_RED */      
		Color_RGB( c[i], 0.64, 0.01, 0.00 ); break;
	      case  9: /* BROWN */      
		Color_RGB( c[i], 0.20, 0.03, 0.00 ); break;
	      case 10: /* BURNT_ORANGE */ 
		Color_RGB( c[i], 0.49, 0.62, 0.00 ); break;
	      case 11: /* BURNT_SIENNA */   
		Color_RGB( c[i], 0.49, 0.62, 0.04 ); break;
	      case 12: /* CADET_BLUE */       
		Color_RGB( c[i], 0.09, 0.09, 0.36 ); break;
	      case 13: /* CARNATION_PINK */       
		Color_RGB( c[i], 1.00, 0.20, 0.25 ); break;
	      case 14: /* COPPER */       
		Color_RGB( c[i], 0.56, 0.62, 0.00 ); break;
	      case 15: /* CORNFLOWER */    
		Color_RGB( c[i], 0.25, 0.25, 1.00 ); break;
	      case 16: /* CYAN */    
		Color_RGB( c[i], 0.00, 1.00, 1.00 ); break;
	      case 17: /* FOREST_GREEN */  
		Color_RGB( c[i], 0.00, 0.36, 0.09 ); break;
	      case 18: /* GOLD */    
		Color_RGB( c[i], 1.00, 0.20, 0.00 ); break;
	      case 19: /* GOLDENROD */   
		Color_RGB( c[i], 1.00, 0.36, 0.00 ); break;
	      case 20: /* GRAY */      
		Color_RGB( c[i], 0.50, 0.50, 0.50 ); break;
	      case 21: /* GREEN */      
		Color_RGB( c[i], 0.00, 1.00, 0.00 ); break;
	      case 22: /* GREEN_BLUE */     
		Color_RGB( c[i], 0.00, 0.25, 1.00 ); break;
	      case 23: /* GREEN_YELLOW */    
		Color_RGB( c[i], 0.81, 1.00, 0.00 ); break;
	      case 24: /* INDIAN_RED */     
		Color_RGB( c[i], 0.20, 0.00, 0.00 ); break;
	      case 25: /* LAVENDAR */      
		Color_RGB( c[i], 1.00, 0.12, 1.00 ); break;
	      case 26: /* LEMON_YELLOW */      
		Color_RGB( c[i], 1.00, 0.72, 0.04 ); break;
	      case 27: /* MAGENTA */      
		Color_RGB( c[i], 1.00, 0.00, 1.00 ); break;
	      case 28: /* MAHOGANY */      
		Color_RGB( c[i], 0.25, 0.00, 0.00 ); break;
	      case 29: /* MAIZE */      
		Color_RGB( c[i], 1.00, 0.25, 0.00 ); break;
	      case 30: /* MAROON */      
		Color_RGB( c[i], 0.30, 0.00, 0.09 ); break;
	      case 31: /* MELON */      
		Color_RGB( c[i], 0.81, 0.62, 0.04 ); break;
	      case 32: /* MIDNITE_BLUE */      
		Color_RGB( c[i], 0.00, 0.00, 0.36 ); break;
	      case 33: /* MULBERRY */      
		Color_RGB( c[i], 0.20, 0.00, 0.62 ); break;
	      case 34: /* NAVY_BLUE */      
		Color_RGB( c[i], 0.00, 0.00, 0.64 ); break;
	      case 35: /* OLIVE_GREEN */      
		Color_RGB( c[i], 0.64, 0.49, 0.00 ); break;
	      case 36: /* ORANGE */      
		Color_RGB( c[i], 1.00, 0.12, 0.00 ); break;
	      case 37: /* ORANGE_RED */      
		Color_RGB( c[i], 0.56, 0.01, 0.00 ); break;
	      case 38: /* ORANGE_YELLOW */      
		Color_RGB( c[i], 1.00, 0.49, 0.00 ); break;
	      case 39: /* ORCHID */      
		Color_RGB( c[i], 0.64, 0.22, 0.81 ); break;
	      case 40: /* PEACH */      
		Color_RGB( c[i], 1.00, 0.25, 0.30 ); break;
	      case 41: /* PERIWINKLE */      
		Color_RGB( c[i], 0.36, 0.36, 1.00 ); break;
	      case 42: /* PINE_GREEN */      
		Color_RGB( c[i], 0.00, 0.12, 0.22 ); break;
	      case 43: /* PINK */      
		Color_RGB( c[i], 1.00, 0.20, 0.25 ); break;
	      case 44: /* PLUM */      
		Color_RGB( c[i], 0.12, 0.00, 0.07 ); break;
	      case 45: /* PURPLE */      
		Color_RGB( c[i], 0.09, 0.00, 0.12 ); break;
	      case 46: /* RAW_SIENNA */      
		Color_RGB( c[i], 0.25, 0.04, 0.01 ); break;
	      case 47: /* RAW_UMBER */      
		Color_RGB( c[i], 0.20, 0.62, 0.00 ); break;
	      case 48: /* RED */      
		Color_RGB( c[i], 1.00, 0.00, 0.00 ); break;
	      case 49: /* RED_ORANGE */      
		Color_RGB( c[i], 1.00, 0.04, 0.00 ); break;
	      case 50: /* RED_VIOLET */      
		Color_RGB( c[i], 0.36, 0.00, 0.25 ); break;
	      case 51: /* SALMON */      
		Color_RGB( c[i], 0.36, 0.01, 0.04 ); break;
	      case 52: /* SEA_GREEN */      
		Color_RGB( c[i], 0.16, 1.00, 0.16 ); break;
	      case 53: /* SEPIA */      
		Color_RGB( c[i], 0.16, 0.03, 0.00 ); break;
	      case 54: /* SILVER */      
		Color_RGB( c[i], 0.49, 0.49, 0.49 ); break;
	      case 55: /* SKY_BLUE */      
		Color_RGB( c[i], 0.16, 0.36, 0.81 ); break;
	      case 56: /* SPRING_GREEN */      
		Color_RGB( c[i], 0.36, 1.00, 0.36 ); break;
	      case 57: /* TAN */      
		Color_RGB( c[i], 0.56, 0.12, 0.00 ); break;
	      case 58: /* THISTLE */      
		Color_RGB( c[i], 0.81, 0.04, 1.00 ); break;
	      case 59: /* TURQUOISE_BLUE */      
		Color_RGB( c[i], 0.00, 0.56, 0.64 ); break;
	      case 60: /* VIOLET */      
		Color_RGB( c[i], 0.09, 0.00, 0.12 ); break;
	      case 61: /* VIOLET_BLUE */      
		Color_RGB( c[i], 0.04, 0.00, 0.16 ); break;
	      case 62: /* VIOLET_RED */      
		Color_RGB( c[i], 0.64, 0.00, 0.25 ); break;
	      case 63: /* WHITE */      
		Color_RGB( c[i], 1.00, 1.00, 1.00 ); break;
	      case 64: /* YELLOW */      
		Color_RGB( c[i], 1.00, 1.00, 0.00 ); break;
	      case 65: /* YELLOW_GREEN */      
		Color_RGB( c[i], 0.49, 1.00, 0.00 ); break;
	      case 66: /* YELLOW_ORANGE */      
		Color_RGB( c[i], 1.00, 0.23, 0.00 ); break;
	      default:
		Color_RGB(c[i], (float)i/128.0, (float)i/128.0, (float)i/128.0);
		break;
	    }
	}
	break;

      case 1:  /* green gradient */
	
	Color_RGB(c[0], .1, .2, .4);
	Color_RGB(c[MY_CMAP_SIZE -1], .8, .2, .9 );
	for (i=1; i < MY_CMAP_SIZE - 1; i++) {
	    Color_RGB(c[i], 0.3, (float)i/(float)MY_CMAP_SIZE, 0.3);
	}
	break;
    }



    /* create color list */
    clist.start_index = 0;
    clist.length = MY_CMAP_SIZE;
    clist.colors = c;
    if (cmap != NULL) {
	xgl_object_destroy(cmap);
    }
    cmap = xgl_color_map_create(XGL_CMAP_COLOR_TABLE_SIZE, MY_CMAP_SIZE,
			       XGL_CMAP_COLOR_TABLE, &clist, 
			       0);
    xgl_object_set(ras, XGL_RAS_COLOR_MAP, cmap, 0);
}

static void
canvas_repaint_proc(canvas, paint_window, dpy, xwin, xrects)
Canvas		canvas;
Xv_Window	paint_window;
Display		*dpy;
Window		xwin;
Xv_xrectlist	*xrects;
{
	page_proc();
}
static void
canvas_resize_proc(canvas, width, height)
Canvas	canvas;
int	width, height;
{
    /* Get the size of the canvas in pixels */
    can_wid = width;
    can_hgt = height;
    xgl_window_raster_resize(ras);
}
