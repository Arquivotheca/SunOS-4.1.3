
/*      @(#)gp2test.h 1.2 88/04/22 SMI          */
 
/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc. 
 */

/*
 * error numbers, Sundiag gets 1 and 2
 */
#define NO_GP_DEV               3
#define NO_CG_DEV               4
#define SCREEN_NOT_OPEN		5
#define DEV_NOT_OPEN            6
#define DEV_NO_STATBLK          7
#define NO_MEMORY               8
#define POST_ERROR              9
#define SYNC_ERROR              10
#define DATA_ERROR              11
#define OPEN_ERROR              12
#define IOCTL_ERROR             13
#define MMAP_ERROR              14
#define END_ERROR               15
#define SIGSEGV_ERROR           96
#define SIGBUS_ERROR            97

/*
 * other gp2test defines
 */
/*
 * Put gp1_sync here to force us to be no more than one buffer
 * ahead of the gp
 */
#define PRE_POST_SYNC(m,f) /***** gp1_sync(m,f) */

/*
 * Put gp1_sync here to force us to never get
 * ahead of the gp
 */
#define POST_POST_SYNC(m,f)
#define GOURAUD
#define TOLERANCE	1.0e-5
#define TOLERANCE1	.01
#define MATRIX_TESTS    6
#define POINT_TESTS	4
#define LINE_TESTS	15
#define MAXPTS		10
#define MAXLINES	10
#define WHITE_COLOR		0
#define BLUE_COLOR 		1
#define NO_FB           0
#define MONO_ONLY       10
#define CG_ONLY         2
#define CG_MONO_AVAIL   12
#define GP2_BUFSIZ	256
#define GP_DEV		"/dev/gpone0a"
#define CG_DEV		"/dev/cgtwo0"
#define CR_DEV		"/dev/cgnine0"
#define FB_DEV		"/dev/fb"
#define VME_DEV		"/dev/vme24d32"

struct point {   
    float x;
    float y;
    float z;
    int i; 
};
 
struct polygon {
    int nbnds;
    int *nvptr;
    struct point *coordptr;
    double  x_max;
    double  y_max;
};

/*
 * for Sundiag purposes
 */
extern int      errors;
extern int      debug;
extern int      verbose;
extern int      looping_test;
extern int      display_read_data;
extern int      exec_by_sundiag;
char     msg[132];
extern char     *tmp_msg;
extern char     *hostname;
extern char     *errmsg1;
 
char *errmsg();

	/* gp2test.c */
setup_desktop(), unlock_devtop();
	/* gp2_probe.c */
check_gp(), probe_cg();
	/* gp2gpcitest.c */
opengp(), init_static_blk();
test_hardware(), test_mul_matrix();
test_mul_point(), test_proc_line();
test_polygons(), clrzb(), gp1poly3();
	/* gp2_hardware.c */
test_xp_local_ram(), test_xp_shared_ram();
test_xp_sequencer(), test_xp_alu();
test_xp_rp_fifo(), test_rp_local_ram();
test_rp_shared_ram(), test_rp_sequencer();
test_rp_alu(), test_rp_pp_fifo();
test_pp_ldx_ago(), test_pp_ady_ago();
test_pp_adx_ago(), test_pp_sequencer();
test_pp_alu(), test_pp_rw_zbuf();
test_pp_zbuf();
	/* gp2_matrix.c */
mat_ident(), test_translation();
test_rotate(), test_scale(), test_eye();
set_mat_in_ram(), multiply_matrices();
	/* gp2_point.c */
get_matrix(), get_pts(), multiply_pts();
float *set_pts_in_ram();
short *set_view_port(), *set_matrix();
get_line(), set_line(), multiply_line(), check_line();
	/* gp2_poygons.c */
Pixrect *create_screen();
paint_polygons();
