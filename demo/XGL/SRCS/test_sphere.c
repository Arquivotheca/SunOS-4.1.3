#ifndef lint
static char sccsid[] = "@(#)test_sphere.c 1.1 92/07/30 Copyr 1989, 1990 Sun Micro";
#endif
#include <stdio.h>
#include <math.h>
#include <signal.h>
#include <sys/time.h>
#include <xgl/xgl.h>
#include "WS_macros.h"
#include "XGL.icon"

/****
 *	Sun Microsystems, Inc.
 *	Graphics Products Division
 *
 *	Date:		July 14, 1989
 *	Revised:	September 7, 1989
 *
 * 
 *	Module Name:	test_sphere.c
 *
 *	Synopsis:	Test program for Xgl triangle strips.  Some of
 *			code borrowed from program test_3d.c.  This program 
 *			uses the triangle strip data file (.tri suffix)
 *
 *			NOTE: The format of triangle strip data files is
 *			      slightly different than the format of polygonal
 *			      data files (i.e. the ".pg" format).
 *
 ****/

/*
 * routines included
 */
		/* setup routines */
static	void		xgl_init();
static	void		create_menus();
		/* data handling routines */
static	void		get_data();
static	void		render_point_type_proc();
static	void		render_facet_type_proc();
static	void		clip_calc_proc();
		/* event handlers */
static	void		can_event_proc();
		/* menu selection handlers */
static	void		clear_proc();
static	void		redraw_proc();
static	void		front_no_shading_proc();
static	void		front_facet_shading_proc();
static	void		front_gouraud_shading_proc();
static	void		back_no_shading_proc();
static	void		back_facet_shading_proc();
static	void		back_gouraud_shading_proc();
static	void		edges_proc();
static	void		face_proc();
static	void		disting_proc();
static	void		norm_flip_proc();
static	void		zbuff_proc();
static	void		num_spheres_proc();
static  Notify_value    quit_proc();
static	void		quit_on_signal();

/*
 * macros used for timing
 */
#define START_TIMER(x)	(void)gettimeofday(x, (struct timezone *) 0)

#define STOP_TIMER(x)	(void)gettimeofday(x, (struct timezone *) 0)

#define ELAPSED_SECONDS(x,y) ( ((y)->tv_sec - (x)->tv_sec) + \
			       ((y)->tv_usec - (x)->tv_usec)*1e-6)

/*
 * global variables
 */


typedef struct {
  char		*str;
  Xgl_pt_f3d	pos;
  Xgl_pt_f3d	dir[2];
} text_info;

static	Xgl_facet_type			fct_type = XGL_FACET_NORMAL;

static	Xgl_boolean			p_eye_new_view;
static	Xgl_matrix_f3d			p_eye_view_mat;
static	Xgl_pt_f3d			p_eye_perspective;
static	Xgl_pt_f3d			p_eye_position_pt;
static	Xgl_pt_f3d			p_eye_look_at_pt;
static	Xgl_pt_f3d			p_eye_up_vector;

static	Menu				menu_main;
static	Menu				menu_front_shading;
static	Menu				menu_back_shading;
static	Menu				menu_edges;
static	Menu				menu_zbuff;
static	Menu				menu_face;
static	Menu				menu_disting;
static	Menu				menu_norm_flip;
static	Menu				menu_spheres;

static	Xgl_ras				ras;
static	Xgl_3d_ctx			cctx;
static	Xgl_trans			view_trans;
static	Xgl_light			light[4];
static	Xgl_boolean			light_switch[4];
static	Xgl_pt_f3d			light_dir;	/* lighting vector */
static	Xgl_pt_list			*pl;
static	Xgl_sgn32			*p_ind, *f_ind;
static  Xgl_bbox			*l_bbox;
static 	Xgl_pt_flag_f3d			*pts;
static 	Xgl_pt_normal_flag_f3d		*pts_n;
static 	Xgl_pt_color_normal_flag_f3d	*pts_nc;
static	Xgl_color_normal_facet		*fct_nc;
static	Xgl_usgn32			num_pl;
static  Xgl_bbox			pl_bbox, *bbox_pl;
static	Xgl_boolean			data_ok;
static	Xgl_usgn32			num_txt;
static	text_info			*texts;

static	Xgl_sgn32			txt_index, box_index, pts_index,
					pl_index, fct_index;

static	Xgl_sgn32			for_i;

static	Xgl_color_facet			*fcts_color;
static	Xgl_normal_facet		*fcts_normal;
static	Xgl_facet_list			*fct_list;

static	Xgl_bounds_f3d			vdc;
static	Xgl_bounds_f3d			bound;
static	Xgl_sgn32			num_spheres;
static	float				bgnd_r, bgnd_g, bgnd_b;

#ifdef	TRI_PERF
	Xgl_sgn32			tri_row_count,
					tri_pixel_count;
static  char				tri_area[256];
#endif
static	char				timestr[256];
static  char				tris_per_sec[256];
static  char				tris_count_str[256];
static	struct timeval    		t_start, t_end;
static	double              		time_elapsed = 0.0;

static  Xgl_sys_state    		sys_state;
static	Frame				frame;

/****
 *
 * main
 *
 ****/
main(argc, argv)
    int			argc;
    char		*argv[];
{
				/* output file */
    static char		*default_file_name = "sphere_i.tri";
    static char		*file_name;
    FILE		*file_pt, *fopen();
    int			i;
    int			status;

    Canvas		canvas;

				/* defaults */
    file_name = default_file_name;	/* data file */
    vdc.xmin  = bound.xmin = -1.0;	/* vdc window bounds */
    vdc.xmax  = bound.xmax =  1.0;
    vdc.ymin  = bound.ymin = -1.0;
    vdc.ymax  = bound.ymax =  1.0;
    num_spheres = 1;			/* # of spheres to render */
    bgnd_r = 0.0;			/* background color components */
    bgnd_g = 0.0;
    bgnd_b = 0.6;
/*  bgnd_r = 0.21; */			/* patrick's favorites */
/*  bgnd_g = 0.31; */
/*  bgnd_b = 0.24; */
    light_dir.x = 1.0;
    light_dir.y = -1.0;
    light_dir.z = 1.0;
				/* initialize */
    texts	= (text_info *)0;
    pts		= (Xgl_pt_flag_f3d *)0;
    pts_n	= (Xgl_pt_normal_flag_f3d *)0;
    pl		= (Xgl_pt_list *)0;
    fct_list	= (Xgl_facet_list *)0;
    p_ind	= (int *)0;
    f_ind	= (int *)0;
    l_bbox	= (Xgl_bbox *)0;
    pts_nc	= (Xgl_pt_color_normal_flag_f3d *)0;
    fct_nc	= (Xgl_color_normal_facet *)0;

				/* read command line args */
    for (i = 1; i < argc; i++) {
	if (*argv[i] != '-') {
	    printf ("ERROR: All arguments must start with -\n");
	    printf ("Use  %s -h  for help\n", argv[0]);
	    exit (0);
	}

	switch (argv[i][1])
	{
	    case 'b':		/* clip bounds */
	    case 'B':
		bound.xmin = atof(argv[++i]);
		bound.xmax = atof(argv[++i]);
		bound.ymin = atof(argv[++i]);
		bound.ymax = atof(argv[++i]);
		break;

	    case 'c':		/* canvas background color */
	    case 'C':
		bgnd_r = atof(argv[++i]);
		bgnd_g = atof(argv[++i]);
		bgnd_b = atof(argv[++i]);
		break;

	    case 'f':		/* output filename */
	    case 'F':
		file_name = argv[++i];
		break;

	    case 'l':		/* light vector */
	    case 'L':
		light_dir.x = atof(argv[++i]);
		light_dir.y = atof(argv[++i]);
		light_dir.z = atof(argv[++i]);
		break;

	    case 'n':		/* number of spheres to render */
	    case 'N':
		num_spheres = atof(argv[++i]);
		break;

	    case 'v':		/* vdc window */
	    case 'V':
		vdc.xmin = atof(argv[++i]);
		vdc.xmax = atof(argv[++i]);
		vdc.ymin = atof(argv[++i]);
		vdc.ymax = atof(argv[++i]);
		break;

	    case 'h':		/* help */
	    case 'H':
	    case '?':
		printf ("\ntest_sphere: Xgl test program for triangle strips\n");
		printf ("Invoke with : %s [args]\n\n", argv[0]);
		printf ("\t[args] allowed:\n");
		printf ("\t   -f file              where file = input filename\n");
		printf ("\t                        default: images/sphere_i.tri\n");
		printf ("\t   -b xmin xmax ymin ymax\n");
		printf ("\t                        where xmin, etc. = clip bounds\n");
		printf ("\t                        default: -1 1 -1 1\n");
		printf ("\t   -c r g b             where r,g,b = background color\n");
		printf ("\t                        default: 0.21 0.31 0.24\n");
		printf ("\t   -l x y z             where x, y, z = light vector\n");
		printf ("\t                        default: 1 -1 1\n");
		printf ("\t   -n num               where num = number of spheres\n");
		printf ("\t                        default: 1 sphere rendered\n");
		printf ("\t   -v xmin xmax ymin ymax\n");
		printf ("\t                        where xmin, etc. = vdc bounds\n");
		printf ("\t                        default: -1 1 -1 1\n");
		printf ("\t   -h                   prints this message\n\n");
		exit(0);

	    default:		/* bad argument */
		printf ("WARNING: Unknown command line argument used.\n");
		printf ("         \"-%c\"  is unknown.  Continuing execution.\n", argv[i][1]);
		i++;
		break;
	}
    }

    (void)xv_init(XV_INIT_ARGS, argc, argv, 0);

    xgl_icon = icon_create(ICON_IMAGE, &icon_pixrect, 0);

    frame = xv_create(NULL,  FRAME, 
		WIN_DYNAMIC_VISUAL, TRUE,
		FRAME_ICON,	xgl_icon,
		FRAME_LABEL,  "Xgl Spheres",
		WIN_WIDTH, 	600, 
		WIN_HEIGHT, 	500, 
		WIN_SHOW, TRUE,
		0);

    (void) notify_interpose_destroy_func(frame, quit_proc);

    sprintf(tris_count_str, "Count: %10.1f", 0.);
    sprintf(timestr, "Time: %9.3f sec", time_elapsed);
    strcpy(tris_per_sec, "Tris/Sec: NA");
#ifdef	TRI_PERF
    strcpy(tri_area, "Av Tri Area: NA");
#endif

    canvas = xv_create(frame, CANVAS, 
		WIN_DYNAMIC_VISUAL, TRUE,
		OPENWIN_AUTO_CLEAR, FALSE, 
		CANVAS_RETAINED, FALSE, 
		CANVAS_FIXED_IMAGE, FALSE,
		0);

    create_menus();
    xgl_init(canvas);
    get_data(file_name);

    xv_main_loop(frame);
} /* end of main() */



/**********                                                **********/
/**********                  Setup Routines                **********/
/**********                                                **********/

/****
 * xgl_init(canvas)
 *
 * Sets up color maps, creates raster, context, etc.
 *
 ****/
static
void
xgl_init(canvas)
    Canvas		canvas;
{   
    Xgl_cmap		cmap;
    Xgl_color_list	clist;
    Xgl_color		colors[128];
    Xgl_color		color0, color63, color95, color96;
    Xgl_sgn32		i;
    Xgl_segment		segments[3];

    sys_state = xgl_open(XGL_SYS_ST_ERROR_DETECTION, TRUE, 0);

    (void) signal(SIGINT, quit_on_signal);

    /* create color list */
    clist.start_index = 0;
    clist.length = 128;

    colors[0].rgb.r = bgnd_r;	/* canvas background */
    colors[0].rgb.g = bgnd_g;
    colors[0].rgb.b = bgnd_b;

    for (i = 1; i < 64; i++) {	/* front-facing facet ramp */
        colors[i].rgb.r = colors[i].rgb.g = colors[i].rgb.b = ((float) i)/63.0;
    }
    for (i = 64; i < 96; i++) {	/* back-facing facet ramp */
        colors[i].rgb.r = ((float)(i - 64))/32.;
        colors[i].rgb.g = 0.0;
        colors[i].rgb.b = 0.0;
    }
    colors[96].rgb.r = 0;   colors[96].rgb.g = 1.0; colors[96].rgb.b = 0;
    colors[97].rgb.r = 0;   colors[97].rgb.g = 0;   colors[97].rgb.b = 1.0;
    colors[98].rgb.r = 1.0; colors[98].rgb.g = 1.0; colors[98].rgb.b = 0;
    colors[99].rgb.r = 1.0; colors[99].rgb.g = 0;   colors[99].rgb.b = 1.0;
    for (i = 100; i < 128; i++) {
        colors[i].rgb.r = 0.0;
        colors[i].rgb.b = 0.0;
        colors[i].rgb.g = ((float)(i - 100))/28.0;
    }
    colors[127].rgb.r = colors[127].rgb.g = colors[127].rgb.b = 0;

    clist.colors = colors;
    segments[0].offset = 1;
    segments[0].length = 63;
    segments[1].offset = 64;
    segments[1].length = 32;
    segments[2].offset = 100;
    segments[2].length = 27;


    INIT_X11_WG(frame, canvas, can_event_proc, redraw_proc, resize_proc)
    ras = xgl_window_raster_device_create(XGL_WIN_X, &xgl_x_win, 0);

    cmap = xgl_color_map_create(0);
    xgl_object_set(cmap, XGL_CMAP_COLOR_TABLE_SIZE, 128,
			XGL_CMAP_COLOR_TABLE, &clist, 
			XGL_CMAP_RAMP_NUM, 3,
			XGL_CMAP_RAMP_LIST, segments,
			0);

    xgl_object_set(ras, XGL_RAS_COLOR_MAP, cmap, 0);
    cctx = xgl_3d_context_create(XGL_CTX_DEVICE, ras, 
				XGL_CTX_VDC_MAP, XGL_VDC_MAP_ALL, 
                                XGL_CTX_MARKER_DESCRIPTION, xgl_marker_dot,
				XGL_CTX_SFONT_CHAR_HEIGHT, 0.1,
				XGL_3D_CTX_SURF_FRONT_AMBIENT, 0.1, 
				XGL_3D_CTX_SURF_BACK_AMBIENT, 0.1, 
				XGL_3D_CTX_SURF_FRONT_DIFFUSE, 0.8, 
				XGL_3D_CTX_SURF_BACK_DIFFUSE, 0.8, 
				XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASAP,
				0);
    xgl_object_get(cctx, XGL_CTX_VIEW_TRANS, &view_trans);

    color0.index = 0;
    color63.index = 63;
    color95.index = 95;
    color96.index = 96;
    xgl_object_set(cctx, XGL_CTX_LINE_COLOR, &color96,
                         XGL_CTX_EDGE_COLOR, &color96,
                         XGL_CTX_MARKER_COLOR, &color96,
                         XGL_CTX_SFONT_TEXT_COLOR, &color96,
                         XGL_CTX_LINE_ALT_COLOR, &color0,
                         XGL_CTX_EDGE_ALT_COLOR, &color0,
                         XGL_CTX_SURF_FRONT_COLOR, &color63,
                         XGL_3D_CTX_SURF_BACK_COLOR, &color95,
			 XGL_3D_CTX_SURF_FACE_DISTINGUISH, TRUE,
                         XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_ZBUFFER, 
			 XGL_CTX_NEW_FRAME_ACTION, 
				   XGL_CTX_NEW_FRAME_CLEAR | 
				   XGL_CTX_NEW_FRAME_HLHSR_ACTION, 
                         0);

    light_switch[0] = light_switch[1] = TRUE;
    xgl_object_set(cctx, XGL_3D_CTX_LIGHT_NUM, 2, 
			XGL_3D_CTX_LIGHT_SWITCHES, light_switch,
			0);
    xgl_object_get(cctx, XGL_3D_CTX_LIGHTS, light);
    xgl_object_set(light[0], XGL_LIGHT_TYPE, XGL_LIGHT_AMBIENT, 
			0);
    xgl_object_set(light[1], XGL_LIGHT_TYPE, XGL_LIGHT_DIRECTIONAL,
			XGL_LIGHT_DIRECTION, &light_dir,
			0);

    data_ok = FALSE;
    fct_type = XGL_FACET_NORMAL;
    p_eye_new_view = FALSE;

}


/****
 * create_menus()
 *
 * Creates application menu and sub-menus.
 *
 ****/
static
void
create_menus()
{					/* front_shading sub-menu */
    menu_front_shading = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"No Shading",
			MENU_NOTIFY_PROC, front_no_shading_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Facet Shading",
			MENU_NOTIFY_PROC, front_facet_shading_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Vertex (Gouraud) Shading",
			MENU_NOTIFY_PROC, front_gouraud_shading_proc,
			0, 0);

					/* bac_shading sub-menu */
    menu_back_shading = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"No Shading",
			MENU_NOTIFY_PROC, back_no_shading_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Facet Shading",
			MENU_NOTIFY_PROC, back_facet_shading_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Vertex (Gouraud) Shading",
			MENU_NOTIFY_PROC, back_gouraud_shading_proc,
			0, 0);

					/* edge-type sub-menu */
    menu_edges = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"On",
			MENU_NOTIFY_PROC, edges_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Off",
			MENU_NOTIFY_PROC, edges_proc,
			0, 0);

    menu_zbuff = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"On",
			MENU_NOTIFY_PROC, zbuff_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Off",
			MENU_NOTIFY_PROC, zbuff_proc,
			0, 0);

    menu_face = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"Off",
			MENU_NOTIFY_PROC, face_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Front",
			MENU_NOTIFY_PROC, face_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Back",
			MENU_NOTIFY_PROC, face_proc,
			0, 0);
    menu_disting = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"False",
			MENU_NOTIFY_PROC, disting_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"True",
			MENU_NOTIFY_PROC, disting_proc,
			0, 0);
    menu_norm_flip = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"False",
			MENU_NOTIFY_PROC, norm_flip_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"True",
			MENU_NOTIFY_PROC, norm_flip_proc,
			0, 0);
					/* number of spheres sub-menu */
    menu_spheres = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"0",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"1",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"2",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"3",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"4",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"5",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"6",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"7",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"8",
			MENU_NOTIFY_PROC, num_spheres_proc,
			0, 0);

					/* main application menu */
    menu_main = xv_create(XV_NULL,MENU_COMMAND_MENU, 
			MENU_ITEM,
			MENU_STRING,	"Clear",
			MENU_NOTIFY_PROC, clear_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Redraw",
			MENU_NOTIFY_PROC, redraw_proc,
			0,
			MENU_ITEM,
			MENU_STRING,	"Front Shading",
			MENU_PULLRIGHT,	menu_front_shading,
			0,
			MENU_ITEM,
			MENU_STRING,	"Back Shading",
			MENU_PULLRIGHT,	menu_back_shading,
			0,
			MENU_ITEM,
			MENU_STRING,	"Edges",
			MENU_PULLRIGHT,	menu_edges,
			0,
			MENU_ITEM,
			MENU_STRING,	"Z-Buffer",
			MENU_PULLRIGHT,	menu_zbuff,
			0,
			MENU_ITEM,
			MENU_STRING,	"Face Culling",
			MENU_PULLRIGHT,	menu_face,
			0,
			MENU_ITEM,
			MENU_STRING,	"Face Distinguish",
			MENU_PULLRIGHT,	menu_disting,
			0,
			MENU_ITEM,
			MENU_STRING,	"Normal Flip",
			MENU_PULLRIGHT,	menu_norm_flip,
			0,
			MENU_ITEM,
			MENU_STRING,	"Spheres",
			MENU_PULLRIGHT,	menu_spheres,
			0,
			MENU_ITEM,
			MENU_STRING,	tris_count_str,
			0,
			MENU_ITEM,
			MENU_STRING,	timestr,
			0,
			MENU_ITEM,
			MENU_STRING,	tris_per_sec,
			0,
#ifdef	TRI_PERF
			MENU_ITEM,
			MENU_STRING,	tri_area,
			0,
#endif
			0);
}



/**********                                                **********/
/**********             Data Handling Routines             **********/
/**********                                                **********/

/****
 * get_data(filename)
 *
 * Retrieve triangle strip data from ASCII file and reformat data into
 * Xgl data structures.
 *
 * Input data must be formatted using the .tri file format.
 *
 ****/
static
void
get_data(filename)
    char	*filename;
{

    char		ch[4];
    register int	i;
    float		f0, f1, f2, f3;
    int			color, text_len;
    int			facet;		/* boolean; working on facet info? */
    int			num_tri_strip, totxgl_num_facets, totxgl_num_pts;
    int			num_facets, num_pts;
    int			num_plin, num_lin, num_debug;
    int			ignore;

    FILE		*file_pt, *fopen();

    bbox_pl   = (Xgl_bbox *)0;
    txt_index = 0;
    box_index = 0;
    pts_index = 0;
    pl_index  = 0;
    fct_index = 0;


    if (pl) free(pl);
    if (fct_list) free(fct_list);
    if (p_ind) free(p_ind);
    if (f_ind) free(f_ind);
    if (l_bbox) free(l_bbox);
    if (pts_nc) free(pts_nc);
    if (fct_nc) free(fct_nc);
    if (texts) {
      for (i = 0; i < num_txt; i++) free(texts[i].str);
      free (texts);
    }


    texts	= (text_info *)0;
    pl		= (Xgl_pt_list *)0;
    fct_list	= (Xgl_facet_list *)0;
    p_ind	= (int *)0;
    f_ind	= (int *)0;
    l_bbox	= (Xgl_bbox *)0;
    pts_nc	= (Xgl_pt_color_normal_flag_f3d *)0;
    fct_nc	= (Xgl_color_normal_facet *)0;

    if (!(file_pt = fopen(filename,"r"))) {
	printf("ERROR: file %s not found!\n", filename);
	printf("        Try using the -f command line switch.\n\n");
	data_ok = FALSE;
	exit(-1);
    } else {	/* file found, scan it */

      while((i = fscanf(file_pt,"%s",ch)) != EOF) {
        switch (ch[0]) {
          case 'A' :	/* viewing info */
            fscanf(file_pt,"%f %f %f", &p_eye_perspective.x,
		   &p_eye_perspective.y, &p_eye_perspective.z);

            fscanf(file_pt,"%f %f %f", &p_eye_position_pt.x,
		   &p_eye_position_pt.y, &p_eye_position_pt.z);

            fscanf(file_pt,"%f %f %f", &p_eye_look_at_pt.x,
		   &p_eye_look_at_pt.y, &p_eye_look_at_pt.z);

            fscanf(file_pt,"%f %f %f", &p_eye_up_vector.x,
		   &p_eye_up_vector.y, &p_eye_up_vector.z);

	    p_eye_new_view = TRUE;
            break;

          case 'a' :	/* facet or vertex RGBa color */
            fscanf(file_pt,"%f %f %f %f",&f0,&f1,&f2,&f3);
            if (facet) {
              fct_nc[pl_index - 1].color.rgb.r = f0;
	      fct_nc[pl_index - 1].color.rgb.g = f1;
	      fct_nc[pl_index - 1].color.rgb.b = f2;
            } else {
              pts_nc[pts_index - 1].color.rgb.r = f0;
              pts_nc[pts_index - 1].color.rgb.g = f1;
              pts_nc[pts_index - 1].color.rgb.b = f2;
            }
            break;

          case 'B' :	/* bounding box  for all pts in this file */
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            pl_bbox.bbox_type  = XGL_BBOX_F3D;
            pl_bbox.box.f3d.xmin = f0;
            pl_bbox.box.f3d.ymin = f1;
            pl_bbox.box.f3d.zmin = f2;
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            pl_bbox.box.f3d.xmax = f0;
            pl_bbox.box.f3d.ymax = f1;
            pl_bbox.box.f3d.zmax = f2;
            bbox_pl = &pl_bbox;
	    if (pl)
		pl[pl_index - 1].bbox = bbox_pl;
            break;

          case 'b' :	/* facet bounding box */
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            l_bbox[box_index].bbox_type  = XGL_BBOX_F3D;
            l_bbox[box_index].box.f3d.xmin = f0;
            l_bbox[box_index].box.f3d.ymin = f1;
            l_bbox[box_index].box.f3d.zmin = f2;
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            l_bbox[box_index].box.f3d.xmax = f0;
            l_bbox[box_index].box.f3d.ymax = f1;
            l_bbox[box_index].box.f3d.zmax = f2;
            pl[pl_index - 1].bbox = (l_bbox + box_index);
            box_index += 1;
            break;

          case 'c' :	/* facet or vertex RGB color */
            fscanf(file_pt,"%f %f %f",&f0,&f1,&f2);
            if (facet) {
              fct_nc[fct_index - 1].color.rgb.r = f0;
	      fct_nc[fct_index - 1].color.rgb.g = f1;
	      fct_nc[fct_index - 1].color.rgb.b = f2;
            } else {
              pts_nc[pts_index - 1].color.rgb.r = f0;
              pts_nc[pts_index - 1].color.rgb.g = f1;
              pts_nc[pts_index - 1].color.rgb.b = f2;
            }
            break;

          case 'i' :	/* facet or vertex index color */
            fscanf(file_pt,"%d",&color);
	    if (color == 127) color--;
            if (facet) {
              fct_nc[fct_index - 1].color.index = color;
	    } else {
              pts_nc[pts_index - 1].color.index = color;
            }
            break;

          case 'n' :	/* facet or vertex normal */
            fscanf(file_pt,"%f %f %f",&f0,&f1,&f2);
            if (facet) {
              fct_nc[fct_index - 1].normal.x = f0;
              fct_nc[fct_index - 1].normal.y = f1;
              fct_nc[fct_index - 1].normal.z = f2;
            } else {
              pts_nc[pts_index - 1].normal.x = f0;
              pts_nc[pts_index - 1].normal.y = f1;
              pts_nc[pts_index - 1].normal.z = f2;
            }
            break;

          case 'P' :	/* header line for each triangle strip in file */
            fscanf(file_pt,"%d %d", &num_facets, &num_pts);

	    pl[pl_index].num_pts = num_pts;
	    pl[pl_index].pt_type = XGL_PT_COLOR_NORMAL_FLAG_F3D;
	    pl[pl_index].pts.color_normal_flag_f3d = &(pts_nc[pts_index]);
	    p_ind[pl_index] = pts_index;
	    f_ind[pl_index] = fct_index;

	    fct_list[pl_index].facet_type = fct_type;
	    fct_list[pl_index].num_facets = num_facets;
	    fct_list[pl_index].facets.color_normal_facets = &(fct_nc[fct_index]);

	    pl_index++;
	    break;

          case 'p' :	/* header line for vertex data to follow */
	    fscanf(file_pt,"%d %d", &num_debug, &ignore);
	    fct_index++;
            facet = 1;
            break;

	  case 'S' :	/* # of strips, total # of triangles, total # of vertices */
            fscanf(file_pt,"%d %d %d", &num_tri_strip, &totxgl_num_facets,
		   &totxgl_num_pts);
            num_pl = num_tri_strip;

            pl = (Xgl_pt_list *)(malloc(num_pl * sizeof(Xgl_pt_list)));
	    fct_list = (Xgl_facet_list *)(malloc(num_pl * sizeof(Xgl_facet_list)));
            p_ind = (int *)(malloc(num_pl * sizeof(int)));
	    f_ind = (int *)(malloc(num_pl * sizeof(int)));
            l_bbox = (Xgl_bbox *)(malloc(num_pl * sizeof(Xgl_bbox)));

            pts_nc = (Xgl_pt_color_normal_flag_f3d *)
              (malloc(totxgl_num_pts * sizeof(Xgl_pt_color_normal_flag_f3d)));
            fct_nc = (Xgl_color_normal_facet *)
	      (malloc(totxgl_num_facets *sizeof(Xgl_color_normal_facet)));
            break;

          case 'T' :	/* number of text strings in file */
            fscanf(file_pt,"%d",&num_txt);

            texts = (text_info *)(malloc(num_txt * sizeof(text_info)));
            break;

          case 't' :	/* text info */
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            texts[txt_index].pos.x = f0;
            texts[txt_index].pos.y = f1;
            texts[txt_index].pos.z = f2;
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            texts[txt_index].dir[0].x = f0;
            texts[txt_index].dir[0].y = f1;
            texts[txt_index].dir[0].z = f2;
            fscanf(file_pt,"%f %f %f", &f0, &f1, &f2);
            texts[txt_index].dir[1].x = f0;
            texts[txt_index].dir[1].y = f1;
            texts[txt_index].dir[1].z = f2;
            fscanf(file_pt, "%d", &text_len);
            texts[txt_index].str = (char *)(malloc(text_len * sizeof(char))); 
            fscanf(file_pt, "%c", ch);
            for (i = 0; i < text_len; i++) 
               fscanf(file_pt, "%c",&(texts[txt_index].str[i]));
            texts[txt_index].str[i] = 0;
            txt_index++;
            break;

          case 'v' :	/* vertices */
            fscanf(file_pt,"%f %f %f",
              &pts_nc[pts_index].x, &pts_nc[pts_index].y, &pts_nc[pts_index].z);
            pts_nc[pts_index].flag = 3;
            facet = 0;
            pts_index++;
            break;

          default:
            break;
        }  
      }
      data_ok = TRUE;
      fclose(file_pt);
      render_point_type_proc();
      render_facet_type_proc();
      clip_calc_proc();
    }
}


/****
 * render_point_type_proc()
 *
 * Converts input vertex data to Xgl input point-list format.
 *
 ****/
static
void
render_point_type_proc()
{
    register int	num_pts;

    num_pts = pts_index;	/* # of pts = # of pts read by get_data() */

      if (pts) free(pts);
      pts = (Xgl_pt_flag_f3d *)0;
      if (pts_n) free(pts_n);
      pts_n = (Xgl_pt_normal_flag_f3d *)0;
      pts_n = (Xgl_pt_normal_flag_f3d *)
		(malloc(num_pts * sizeof(Xgl_pt_normal_flag_f3d)));
      for (for_i = 0; for_i < num_pts; for_i++) {
        pts_n[for_i].x = pts_nc[for_i].x;
        pts_n[for_i].y = pts_nc[for_i].y;
        pts_n[for_i].z = pts_nc[for_i].z;
        pts_n[for_i].normal = pts_nc[for_i].normal;
        pts_n[for_i].flag = 3;
      }
      for (for_i = 0; for_i < num_pl; for_i++) {
        pl[for_i].pts.normal_flag_f3d = &(pts_n[p_ind[for_i]]);
        pl[for_i].pt_type = XGL_PT_NORMAL_FLAG_F3D;
      }
}


/****
 * render_facet_type_proc()
 *
 * Converts input facet data to Xgl input facet-list format.
 *
 ****/
static
void
render_facet_type_proc()
{
    register int	num_facets;

    num_facets = fct_index;

	if (fcts_normal) free(fcts_normal);
        fcts_normal = (Xgl_normal_facet *)0;
	if (fcts_color) free(fcts_color);
        fcts_color = (Xgl_color_facet *)0;
        fcts_normal = (Xgl_normal_facet *)(malloc(num_facets *
			sizeof(Xgl_normal_facet)));
        for (for_i = 0; for_i < num_facets; for_i++) {
	    fcts_normal[for_i].normal.x = fct_nc[for_i].normal.x;
 	    fcts_normal[for_i].normal.y = fct_nc[for_i].normal.y;
 	    fcts_normal[for_i].normal.z = fct_nc[for_i].normal.z;
	}
	for (for_i = 0; for_i < num_pl; for_i++) {
	    fct_list[for_i].facets.normal_facets = &(fcts_normal[f_ind[for_i]]);
	    fct_list[for_i].facet_type = fct_type;
	}
}


/****
 * clip_calc_proc()
 *
 * Computes vdc window depth and informs Xgl.
 *
 ****/
static
void
clip_calc_proc()
{
    float		dist, diag, xdiag, ydiag, zdiag;

    dist = sqrt(p_eye_position_pt.x * p_eye_position_pt.x +
                p_eye_position_pt.y * p_eye_position_pt.y +
                p_eye_position_pt.z * p_eye_position_pt.z);
    diag = 1.0;
    if (bbox_pl) {
      xdiag = bbox_pl->box.f3d.xmax - bbox_pl->box.f3d.xmin;
      ydiag = bbox_pl->box.f3d.ymax - bbox_pl->box.f3d.ymin;
      zdiag = bbox_pl->box.f3d.zmax - bbox_pl->box.f3d.zmin;
      diag = sqrt(xdiag * xdiag + ydiag * ydiag + zdiag * zdiag);
      diag *= 0.5;
    }

/*
    vdc.zmin = bound.zmin = dist - diag;
    vdc.zmax = bound.zmax = dist + diag;
*/
    vdc.zmin = bound.zmin = 0;
    vdc.zmax = bound.zmax = 600;

    xgl_object_set(cctx, XGL_CTX_VDC_WINDOW, &vdc,
                        XGL_CTX_VIEW_CLIP_BOUNDS, &bound, 0);
}



/**********                                                **********/
/**********     Routines to Handle Window/Canvas Events    **********/
/**********                                                **********/


/****
 * can_event_proc(window, event, arg)
 *
 * Handles canvas events.
 *
 ****/


static
void
can_event_proc(window, event, arg)
Xv_Window       window;
Event           *event;
Notify_arg      arg;
{
    int         width, height;


        switch(event_action(event)) {
                case ACTION_MENU:
                        if (event_is_down(event)) {
                                menu_show(menu_main, window, event, NULL);
                        }
                break;
                default: break;
        }
}

static
void
resize_proc(canvas, w, h)
Canvas	canvas;
int	w, h;
{
	xgl_window_raster_resize(ras, w, h);
}



/**********                                                **********/
/********** Routines to Handle Application Menu Selections **********/
/**********                                                **********/

/****
 * clear_proc()
 *
 * Calls Xgl to clear canvas (raster).
 *
 ****/
static
void
clear_proc()
{   
    Xgl_hlhsr_mode	hlhsr;
    Xgl_hlhsr_data	hlhsr_data;
    Xgl_pt_f3d		max_coords;

    xgl_object_get(cctx, XGL_3D_CTX_HLHSR_MODE, &hlhsr);
    if (hlhsr == XGL_HLHSR_ZBUFFER) {

	xgl_object_get(ras, XGL_DEV_MAXIMUM_COORDINATES, &max_coords);
	hlhsr_data.z_buffer.z_value = max_coords.z;
	xgl_object_set(cctx, XGL_3D_CTX_HLHSR_DATA, &hlhsr_data, 0);

    }
    xgl_context_new_frame(cctx);
}


/****
 * redraw_proc()
 *
 * Calls Xgl to redraw canvas (raster).
 *
 ****/
static
void
redraw_proc()
{   

    int			i, eye, eye_z;
    Xgl_pt_f3d		vpn;
    Xgl_geom_normal	geom_normal;
    Xgl_boolean		last = FALSE, clip_calc_done = FALSE;

    if (data_ok) {

	time_elapsed = 0.0;

#ifdef	TRI_PERF
	tri_pixel_count = 0;
#endif
	xgl_context_new_frame(cctx);

	for (eye = num_spheres; eye > 0; eye--) { /* cycle thru many spheres */
	    eye_z = 1 << (eye + 1);
	    p_eye_position_pt.z = -((float)eye_z);
/*	    if ((eye > 2) && !clip_calc_done) */ {
		clip_calc_proc();
		clip_calc_done = TRUE;
	    }
	    vpn.x = p_eye_look_at_pt.x - p_eye_position_pt.x;
	    vpn.y = p_eye_look_at_pt.y - p_eye_position_pt.y;
	    vpn.z = p_eye_look_at_pt.z - p_eye_position_pt.z;
	    xgli_set_3d_view(&p_eye_position_pt,
			    &vpn,
			    &p_eye_up_vector,
			    &p_eye_perspective,
			    p_eye_view_mat);
	    xgl_transform_write(view_trans, p_eye_view_mat);
	    p_eye_new_view = FALSE;

            START_TIMER(&t_start);

	    switch (fct_type) {
	      case XGL_FACET_NONE:
	      case XGL_FACET_COLOR:
		for (for_i = 0; for_i < num_pl; for_i++) {
			/* each octant has different geom normal */
		    if((for_i % 9) == 0) {
			if((for_i % 18) == 0) last = !last;
			if((for_i % 36) == 0) last = !last;
			if (last)
			    geom_normal = XGL_GEOM_NORMAL_LAST_POINTS;
			else
			    geom_normal = XGL_GEOM_NORMAL_FIRST_POINTS;
			xgl_object_set(cctx, XGL_3D_CTX_SURF_GEOM_NORMAL,
				      geom_normal, 0);
			last = !last;
		    }
		    xgl_triangle_strip(cctx, &fct_list[for_i],
					      &pl[for_i]);
		}
		break;
	      case XGL_FACET_NORMAL:
		for (for_i = 0; for_i < num_pl; for_i++) {
		xgl_triangle_strip(cctx, &fct_list[for_i], &pl[for_i]);
		}
		break;
	      case XGL_FACET_COLOR_NORMAL:
		for (for_i = 0; for_i < num_pl; for_i++) {
		xgl_triangle_strip(cctx, &fct_list[for_i], &pl[for_i]);
		}
		break;
	    }

	    for (i = 0; i < num_txt; i++) {
		xgl_stroke_text_3d(cctx, texts[i].str, &(texts[i].pos), 
					  texts[i].dir);
	    }

            STOP_TIMER(&t_end);
            time_elapsed += ELAPSED_SECONDS(&t_start, &t_end);
	}
        time_display_proc();
    }
}


/****
 * quit_proc()
 *
 * Catches when "quit" is selected from the menu, and closes XGL
 *
 ****/
static
Notify_value
quit_proc(frame, status)
 
Frame           frame;
Destroy_status  status;
{
    if (status == DESTROY_CHECKING)
        xgl_close(sys_state);
    return(notify_next_destroy_func(frame, status));
}


/****
 * quit_on_signal()
 *
 * Catches keyboard interrupt (^C), and closes XGL nicely before exiting.
 *
 ****/
static
void
quit_on_signal()
{
    xgl_close(sys_state);
    exit(0);
}


/****
 * front_no_shading_proc()
 *
 * Calls Xgl to change front surface shading type to none.
 *
 ****/
static
void
front_no_shading_proc()
{			/* use facet shading on front-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_FRONT_ILLUMINATION,
		  XGL_ILLUM_NONE, 0);
}


/****
 * front_facet_shading_proc()
 *
 * Calls Xgl to change front surface shading type to facet.
 *
 ****/
static
void
front_facet_shading_proc()
{			/* use facet shading on front-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_FRONT_ILLUMINATION,
		  XGL_ILLUM_PER_FACET, 0);
}


/****
 * front_gouraud_shading_proc()
 *
 * Calls Xgl to change front surface shading type to Gouraud.
 *
 ****/
static
void
front_gouraud_shading_proc()
{
			/* use vertex shading on front-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_FRONT_ILLUMINATION,
		  XGL_ILLUM_PER_VERTEX, 0);
}


/****
 * back_no_shading_proc()
 *
 * Calls Xgl to change back surface shading type to none.
 *
 ****/
static
void
back_no_shading_proc()
{			/* use facet shading on back-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_BACK_ILLUMINATION,
		  XGL_ILLUM_NONE, 0);
}


/****
 * back_facet_shading_proc()
 *
 * Calls Xgl to change back surface shading type to facet.
 *
 ****/
static
void
back_facet_shading_proc()
{			/* use facet shading on back-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_BACK_ILLUMINATION,
		  XGL_ILLUM_PER_FACET, 0);
}


/****
 * back_gouraud_shading_proc()
 *
 * Calls Xgl to change back surface shading type to Gouraud.
 *
 ****/
static
void
back_gouraud_shading_proc()
{
			/* use vertex shading on back-facing facets */
    xgl_object_set(cctx, XGL_3D_CTX_SURF_BACK_ILLUMINATION,
		  XGL_ILLUM_PER_VERTEX, 0);
}


/****
 * edges_proc()
 *
 * Calls Xgl to turn edges on or off.
 *
 ****/
static
void
edges_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char	*on_or_off;

    on_or_off = (char *)menu_get(menu_item, MENU_STRING);

    switch (on_or_off[1]) {

	case 'n':	/* edges on */
	case 'N':
	    xgl_object_set(cctx, XGL_CTX_SURF_EDGE_FLAG, TRUE, 0);
	    break;

	case 'f':	/* edges off */
	case 'F':
	default:
	    xgl_object_set(cctx, XGL_CTX_SURF_EDGE_FLAG, FALSE, 0);
	    break;
    }
}

/****
 * zbuff_proc()
 *
 * Calls Xgl to turn Z-buffering ON and OFF
 *
 ****/
static
void
zbuff_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char	*on_or_off;

    on_or_off = (char *)menu_get(menu_item, MENU_STRING);

    switch (on_or_off[1]) {

	case 'n':	/* Zbuffer on */
	case 'N':
            xgl_object_set(cctx, XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_ZBUFFER, 
				 XGL_CTX_NEW_FRAME_ACTION, 
				   XGL_CTX_NEW_FRAME_CLEAR | 
				   XGL_CTX_NEW_FRAME_HLHSR_ACTION, 
				 0);
	    break;

	case 'f':	/* Zbuffer off */
	case 'F':
	default:
            xgl_object_set(cctx, XGL_3D_CTX_HLHSR_MODE, XGL_HLHSR_NONE, 
				 XGL_CTX_NEW_FRAME_ACTION, 
				   XGL_CTX_NEW_FRAME_CLEAR, 
				 0);
	    break;
    }
}

/****
 * face_proc()
 *
 * Calls Xgl to turn face culling off, front or back
 *
 ****/
static
void
face_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char			*on_or_off;
    static	Xgl_surf_cull_mode	mode;

    on_or_off = (char *)menu_get(menu_item, MENU_STRING);

    switch (on_or_off[0]) {
      case 'F':
      case 'f':
	mode = XGL_CULL_FRONT;
	break;
      case 'B':
      case 'b':
	mode = XGL_CULL_BACK;
	break;
      case 'O':
      case 'o':
      default:
	mode = XGL_CULL_OFF;
	break;
    }
    xgl_object_set(cctx, XGL_3D_CTX_SURF_FACE_CULL, mode, 0);
}

/****
 * disting_proc()
 *
 * Calls Xgl to turn face distinguishing off or on
 *
 ****/
static
void
disting_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char		*on_or_off;
    static	Xgl_boolean	mode;

    on_or_off = (char *)menu_get(menu_item, MENU_STRING);

    switch (on_or_off[0]) {
      case 'T':
      case 't':
	mode = TRUE;
	break;
      case 'F':
      case 'f':
      default:
	mode = FALSE;
	break;
    }
    xgl_object_set(cctx, XGL_3D_CTX_SURF_FACE_DISTINGUISH, mode, 0);
}

/****
 * norm_flip_proc()
 *
 * Calls Xgl to turn normal flipping off or on
 *
 ****/
static
void
norm_flip_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char		*on_or_off;
    static	Xgl_boolean	mode;

    on_or_off = (char *)menu_get(menu_item, MENU_STRING);

    switch (on_or_off[0]) {
      case 'T':
      case 't':
	mode = TRUE;
	break;
      case 'F':
      case 'f':
      default:
	mode = FALSE;
	break;
    }
    xgl_object_set(cctx, XGL_3D_CTX_SURF_NORMAL_FLIP, mode, 0);
}

/****
 * num_spheres_proc()
 *
 * Changes number of spheres drawn.
 *
 ****/
static
void
num_spheres_proc(menu, menu_item)
    Menu		menu;
    Menu_item		menu_item;
{
    static	char	*asc_num_spheres;

	/* get number of spheres from menu */
    asc_num_spheres = (char *)menu_get(menu_item, MENU_STRING);
	/* convert to integer and store in global */
    num_spheres = atoi(asc_num_spheres);
}

time_display_proc()
{
    int   i, tri_count = 0;
    float x;

    for (i = 0; i < num_pl; i++) tri_count += pl[i].num_pts - 2;
    x = (float)num_spheres * (float)tri_count;

    sprintf(tris_count_str, "Count: %10.1f", x);
    sprintf(timestr, "Time: %9.3f sec", time_elapsed);
    sprintf(tris_per_sec, "Tris/Sec: %10.1f", (float)(x / time_elapsed));
#ifdef	TRI_PERF
    sprintf(tri_area, "Av Tri Area: %10.1f", (float)tri_pixel_count/x);
#endif
}
