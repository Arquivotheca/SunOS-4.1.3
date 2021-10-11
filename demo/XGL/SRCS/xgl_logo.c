#ifndef lint
static char sccsid[] = "@(#)xgl_logo.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif

#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <xgl/xgl.h>
#include "P_include.h"
#include "WS_macros.h"
#include "XGL.icon"

/*
 *
 *      Sun Microsystems, inc.
 *      Graphics Product Division
 *
 *      Date:   September 13, 1989
 *      Author: Patrick-Gilles Maillot
 *
 *      hello_X_world.c:
 *		hello_X_world is a simple example of animation using Xgl
 *		3D pipeline. It includes a color map double buffering 
 *		technique to avoid flickering on the screen due to 
 *		successive erase/redraw functions.
 */


static void 	animate_it();
static void	read_file();
static void	switch_buffer();
static void     context_init();
static void	compute_next_view_matrix();
static void	toggle_proc();
static void	step_proc();
static void	quit_on_signal();
static Notify_value     the_quit_proc();


static char xgl_logo[] = {
"L 12 60 B -3. -8. -8. 3. 11.666 7. A 0 0 .3 100 5 5 0 5 5 0 0 -1 \
l 5 v 3. -8. -8. v 3. -7. -8. v 3. -2.5 -1.25 v 3. -3. -0.5 v 3. -8. -8. \
l 5 v 3. -2.5 -1.25 v 3. -1. 1. v 3. -5. 7. v 3. -8. 7. v 3. -2.5 -1.25 \
l 5 v 3. -2. -2. v 3. 2. -8. v 3. 5. -8. v 3. -0.5 0.25 v 3. -2. -2. \
l 5 v 3. -0.5 0.25 v 3. 0. -0.5 v 3. 5. 7. v 3. 4. 7. v 3. -0.5 0.25 \
l 5 v 0. -2. -2. v 0. 2. -8. v 0. 5. -8. v 0. -0.5 0.25 v 0. -2. -2. \
l 5 v 0. -0.5 0.25 v 0. 0. -0.5 v 0. 5. 7. v 0. 4. 7. v 0. -0.5 0.25 \
l 5 v 0. 5. -8. v 0. 7. -8. v 0. 8. -6.5 v 0. 4. -6.5 v 0. 5. -8. \
l 5 v 0. 8. -6.5 v 0. 11. -2. v 0. 10. -2. v 0. 7. -6.5 v 0. 8. -6.5 \
l 5 v 0. 11. -2. v 0. 11.666 -1. v 0. 8.666 -1. v 0. 8. -2. v 0. 11. -2. \
l 5 v -3. -2. -2. v -3. 2. -8. v -3. 5. -8. v -3. -0.5 0.25 v -3. -2. -2. \
l 5 v -3. -0.5 0.25 v -3. 0. -0.5 v -3. 5. 7. v -3. 4. 7. v -3. -0.5 0.25 \
l 5 v -3. 5. -8. v -3. 8. -8. v -3. 9. -6.5 v -3. 4. -6.5 v -3. 5. -8."};

int		toggle = 0;
int		no_quit = 1;

static Frame		frame;
static Canvas		canvas;


Xgl_sys_st	sys_st;
Xgl_3d_ctx	ctx3d;
Xgl_win_ras	ras;
Xgl_trans	view_trans;
P_ctx		V;

float		A = 0.0;
float		step, scale;

int		data_ok, pl_index, pt_index, bx_index, quit_var = 1;
Xgl_pt_list	*pl = (Xgl_pt_list *)0;
Xgl_pt_f3d	*pt = (Xgl_pt_f3d *)0;
Xgl_bbox	*l_bbox = (Xgl_bbox *)0;
Xgl_bbox	pl_bbox, *bbox_pl = (Xgl_bbox *)0;

#define BUF_1   1
#define BUF_2   2

static Xgl_color_rgb BLACK	= {0.0, 0.0, 0.0 };
static Xgl_color_rgb CYAN 	= {0.0, 1.0, 1.0 };

static Xgl_boolean	current_buffer_is_buffer_1 = TRUE;

static Xgl_color	color_table1 [4];
static Xgl_color	color_table2 [4];
static Xgl_cmap		cmap1, cmap2;
static Xgl_color_list	cmap_info1;
static Xgl_color_list	cmap_info2;


static
void
read_file()
{
char	*file_pt, ccc[4];
int	i, j, iii;
int	num_pl, num_lin, num_pt;
float	f0, f1, f2, f3;

  file_pt = xgl_logo;

  bbox_pl = (Xgl_bbox *)0;
  if (pl) free(pl);
  pl= (Xgl_pt_list *)0;
  if (pt) free(pt);
  pt = (Xgl_pt_f3d *)0;
  if (l_bbox) free(l_bbox);
  l_bbox = (Xgl_bbox *)0;
  toggle = pl_index = pt_index = bx_index = 0;
  scale = 100.0;
  step  = 0.04;

  j = 0; i = 0;
  while(sscanf(&file_pt[j],"%s%n",ccc,&i) != EOF) {
    j += i;
    switch (ccc[0]) {
      case 'A' :
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
        P_set_lens(V, f2, f0, f1);
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
        P_set_pos(V, f0, f1, f2);
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
        P_look_to(V, f0, f1, f2);
        sscanf(&file_pt[j],"%f %f %f%n",&f0, &f1, &f2,&i);
        j += i;
        xgl_transform_write(view_trans, V->M_all);
        break;
      case 'a' :
        sscanf(&file_pt[j],"%f %f %f %f%n",&f0,&f1,&f2,&f3,&i);
        j += i;
        break;
      case 'c' :
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
      case 'd' :
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
        break;
      case 'i' :
        sscanf(&file_pt[j],"%d%n",&iii,&i);
        j += i;
        break;
      case 'm' :
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
        break;
      case 'n' :
        sscanf(&file_pt[j],"%f %f %f%n",&f0,&f1,&f2,&i);
        j += i;
      case 'T' :
        sscanf(&file_pt[j],"%d%n",&iii,&i);
        j += i;
        break;
      case 't' :
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        sscanf(&file_pt[j], "%d%n", &iii,&i);
        j += i;
        sscanf(&file_pt[j], "%c%n",ccc,&i);
        j += i;
        for (i = 0; i < iii; i++) {
           sscanf(&file_pt[j], "%c%n",ccc,&i);
           j += i;
        }
        break;
      case 'P' :
        sscanf(&file_pt[j],"%d %d%n",&iii, &iii,&i);
        j += i;
        break;
      case 'L' :
        sscanf(&file_pt[j],"%d %d%n",&num_pl, &num_pt,&i);
        j += i;
        l_bbox = (Xgl_bbox *)(malloc(num_pl * sizeof(Xgl_bbox)));
        pl  = (Xgl_pt_list *)(malloc(num_pl * sizeof(Xgl_pt_list)));
        pt = (Xgl_pt_f3d *)(malloc(num_pt * sizeof(Xgl_pt_f3d)));
        break;
      case 'B' :
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        pl_bbox.bbox_type  = XGL_BBOX_F3D;
        pl_bbox.box.f3d.xmin = f0;
        pl_bbox.box.f3d.ymin = f1;
        pl_bbox.box.f3d.zmin = f2;
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        pl_bbox.box.f3d.xmax = f0;
        pl_bbox.box.f3d.ymax = f1;
        pl_bbox.box.f3d.zmax = f2;
        bbox_pl = &pl_bbox;
        f0 -= pl_bbox.box.f3d.xmin;
        f1 -= pl_bbox.box.f3d.ymin;
        f2 -= pl_bbox.box.f3d.zmin;
        scale = sqrt(f0 * f0 + f1 * f1 + f2 * f2) * 4;
        break;
      case 'p' :
        sscanf(&file_pt[j],"%d %d%n",&iii,&iii,&i);
        j += i;
        break;
      case 'l' :
        sscanf(&file_pt[j],"%d%n",&num_lin,&i);
        j += i;
        pl[pl_index].bbox = (Xgl_bbox *)0;
        pl[pl_index].pts.f3d = &(pt[pt_index]);
        pl[pl_index].num_pts = num_lin;
        pl[pl_index].pt_type = XGL_PT_F3D;
        pl_index += 1;
        break;
      case 'b' :
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        l_bbox[bx_index].bbox_type  = XGL_BBOX_F3D;
        l_bbox[bx_index].box.f3d.xmin = f0;
        l_bbox[bx_index].box.f3d.ymin = f1;
        l_bbox[bx_index].box.f3d.zmin = f2;
        sscanf(&file_pt[j],"%f %f %f%n", &f0, &f1, &f2,&i);
        j += i;
        l_bbox[bx_index].box.f3d.xmax = f0;
        l_bbox[bx_index].box.f3d.ymax = f1;
        l_bbox[bx_index].box.f3d.zmax = f2;
        pl[pl_index - 1].bbox = (l_bbox + bx_index);
        bx_index += 1;
        break;
      case 'v' :
        sscanf(&file_pt[j],"%f %f %f%n",
          &pt[pt_index].x, &pt[pt_index].y, &pt[pt_index].z,&i);
        j += i;
        pt_index += 1;
        break;
      default:
        break;
    }
  }
  data_ok = 1;
}



static
void
animate_it()
{    
  if (data_ok) {
    switch_buffer();
/* wait for V-blank */
    xgl_context_new_frame(ctx3d);
    compute_next_view_matrix();
    xgl_multipolyline(ctx3d, bbox_pl, pl_index, pl);
  }
}

static
void
compute_next_view_matrix()
{
float S, C, S2, C2;
float ex, ey, ez;

  S = sin(A);
  C = cos(A);
  S2 = sin(A/2.);
  C2 = cos(A/2.);
  ex = scale * C * C * S2;
  ey = scale * C2 * C2 * S;
  ez = C * S * ey * 2.;
  if (ex > 0.001 || ex < -0.001 || ey > 0.001 || ey < -0.001) {
    P_set_pos(V, ex, ey, ez);
    P_look_to(V, 0., 0., 0.);
    xgl_transform_write(view_trans, V->M_all);
  }
  A += step;
  if (A > (M_PI * 2)) A -= (M_PI * 2.0);
}



static
void
switch_buffer()
{
  if (current_buffer_is_buffer_1) {
    xgl_object_set(ras, XGL_RAS_COLOR_MAP, cmap1, 0);
    xgl_object_set(ctx3d, XGL_CTX_PLANE_MASK, BUF_2, 0);
  } else {
    xgl_object_set(ras, XGL_RAS_COLOR_MAP, cmap2, 0);
    xgl_object_set(ctx3d, XGL_CTX_PLANE_MASK, BUF_1, 0);
  }
  current_buffer_is_buffer_1 ^= TRUE;
}



static
void
color_init()
{
int	i, j;

  for (i = 0; i < 4; i++) {
    j = i & BUF_1;
    switch (j) {
    case 0: color_table1[i].rgb = BLACK;	break;
    case 1: color_table1[i].rgb = CYAN;		break;
    }
  }

  cmap_info1.start_index = 0;
  cmap_info1.length = 4;
  cmap_info1.colors = color_table1;
  cmap1 = xgl_color_map_create (XGL_CMAP_COLOR_TABLE_SIZE, 4,
		XGL_CMAP_COLOR_TABLE, &cmap_info1, 0);


  for (i = 0; i < 4; i++) {
    switch (i >> 1) {
    case 0: color_table2[i].rgb = BLACK;	break;
    case 1: color_table2[i].rgb = CYAN;		break;
    }
  }

  cmap_info2.start_index = 0;
  cmap_info2.length = 4;
  cmap_info2.colors = color_table2;
  cmap2 = xgl_color_map_create (XGL_CMAP_COLOR_TABLE_SIZE, 4,
		XGL_CMAP_COLOR_TABLE, &cmap_info2, 0);
}



static
void
context_init()
{
Xgl_color	l_color;
Xgl_color	bl_color;

  l_color.index  = 3;

  ctx3d = xgl_3d_context_create(
        XGL_CTX_DEVICE, ras, 
        XGL_CTX_VDC_MAP, XGL_VDC_MAP_ALL,
        XGL_CTX_LINE_COLOR, &l_color,
        XGL_CTX_NEW_FRAME_ACTION, XGL_CTX_NEW_FRAME_VRETRACE |
            XGL_CTX_NEW_FRAME_CLEAR,
	XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASTI,
        0);
  xgl_object_get(ctx3d, XGL_CTX_VIEW_TRANS, &view_trans);
}

static
void
toggle_proc()
{
  if (toggle ^= 1) {
    xgl_object_set(ctx3d, XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASTI, 0);
  } else {
    xgl_object_set(ctx3d, XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASAP, 0);
  }
}

static
void
step_proc()
{
  if(toggle) {
    toggle = 0;
    xgl_object_set(ctx3d, XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASAP, 0);
  }
  animate_it();
}

main(argc, argv)
int	argc;
char	*argv[];
{
Panel		panel;


  xgl_icon = icon_create(ICON_IMAGE, &icon_pixrect, 0);

  frame = xv_create(0, FRAME,
		WIN_DYNAMIC_VISUAL, TRUE,
    		FRAME_NO_CONFIRM, TRUE,
        	FRAME_ICON,   xgl_icon,
		FRAME_LABEL, "XGL Logo",
		WIN_X, 0,
		WIN_Y, 0,
    		WIN_WIDTH, 480,
		WIN_HEIGHT, 480,
		0);

  panel = xv_create(frame, PANEL, 0);

  (void)panel_create_item(panel, PANEL_BUTTON,
	        XV_Y,      xv_row(panel, 0),
		XV_X,      xv_col(panel, 0),
		PANEL_NOTIFY_PROC, toggle_proc,
		PANEL_LABEL_STRING, "Start/Stop",
		0);

  (void)panel_create_item(panel, PANEL_BUTTON,
	        XV_Y,      xv_row(panel, 0),
		XV_X,      xv_col(panel, 15),
		PANEL_NOTIFY_PROC, step_proc,
		PANEL_LABEL_STRING, "Step",
		0);

  window_fit_height(panel);

  canvas = xv_create(frame, CANVAS,
		WIN_DYNAMIC_VISUAL, TRUE,
		OPENWIN_AUTO_CLEAR, FALSE,
		CANVAS_RETAINED, FALSE,
		CANVAS_FIXED_IMAGE, FALSE,
		0);


  V = P_mk_ctx();

  sys_st = xgl_open(0);

  color_init();


  INIT_X11_WG(frame, canvas, event_proc, repaint_proc, resize_proc);
  ras = xgl_window_raster_device_create(XGL_WIN_X, &xgl_x_win,
		XGL_RAS_COLOR_MAP, cmap1,
		0);

  if (!ras) {
    printf("ras memory\n");
    exit(1);
  }
  context_init();
  (void) notify_interpose_destroy_func(frame, the_quit_proc);
  (void) signal(SIGINT, quit_on_signal);
  read_file();

  xv_set(frame, WIN_SHOW, TRUE, 0);
  XFlush(display);
  while(no_quit) {
    /* Bug work around:
     *     This portion of the code is suggested to
     *     fix Xview bug 1038730 (cat: xview, subcat: library) 
     *     Will not be needed with OpenWindows 3.0.
     */
    Display *dpy = (Display *)XV_DISPLAY_FROM_WINDOW(frame);

    if (QLength(dpy))
      xv_input_pending(dpy, XConnectionNumber(dpy));
    /*
     * end of work around
     */
    notify_dispatch();
    if (toggle) animate_it();
    XFlush(display);
  }
}

static Notify_value
the_quit_proc(fr, status)
Frame		fr;
Destroy_status	status;
{
  if (status == DESTROY_CHECKING) {
    no_quit = 0;
    toggle = 0;
    xgl_close(sys_st);
  }
  return(notify_next_destroy_func(fr, status));
}


static void
quit_on_signal()
{
    xgl_close(sys_st);
    exit(0);
}


void nf_proc() 
{
    xgl_context_new_frame(ctx3d);
}

EVENT_X11_WG(ras,canvas,event_proc);
REPAINT_X11_WG(ras,canvas,repaint_proc,nf_proc);
RESIZE_X11_WG(ras,canvas,resize_proc);

