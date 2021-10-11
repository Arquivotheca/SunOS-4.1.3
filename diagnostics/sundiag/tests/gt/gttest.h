/*      @(#)gttest.h 1.1 92/07/30 SMI      */

/*
* Copyright (c) 1990 by Sun Microsystems, Inc.
*/

/* Definitions */

#define NO_ERROR		0
#define USAGE_ERR		1
#define HAWK_TEST_ERROR		2
#define HAWK_FATAL_ERROR	3

#define ERROR_LIMIT		30

#define	FRAME_BUFFER_DEVICE_NAME	"/dev/gt0"
#define	LIGHTPEN_DEVICE_NAME		"/dev/lightpen"
#define GTTEST_DATA 			"gttest.data"

/* ESC_DIAG files */
#define	FE_LOCAL_DATA_MEM		"i_wcs_mem_test"
#define RP_SHARED_RAM			"i_rp_shared_ram"
#define VIDEO_MEM_I860_IMAGE_A		"i_video_mem_i860_image_a"
#define VIDEO_MEM_I860_IMAGE_B		"i_video_mem_i860_image_b"
#define VIDEO_MEM_I860_DEPTH		"i_video_mem_i860_depth"
#define VIDEO_MEM_I860_WID		"i_video_mem_i860_wid"
#define VIDEO_MEM_I860_CURSOR		"i_video_mem_i860_cursor"
#define VIDEO_MEM_I860_FCS_A		"i_video_mem_i860_fcs_a"
#define VIDEO_MEM_I860_FCS_B		"i_video_mem_i860_fcs_b"
#define FB_WLUT				"i_wlut"
#define FB_CLUT				"i_clut"
#define FB_OUTPUT_SECTION		"i_fb_output_section"

/* *.hdl test files */
#define RENDERING_PIPELINE		"rendering_pipeline"
#define	VECTORS_CHK			"vectors"
#define	AA_VECTORS_CHK			"aa_vectors"
#define	WIDE_VECTORS_CHK		"wide_vectors"
#define	TEXTURED_VECTORS_CHK		"textured_vectors"
#define	TRIANGLES_CHK			"triangles"
#define	AA_TRIANGLES_CHK		"aa_triangles"
#define	FLAT_TRIANGLES_CHK		"flat_triangles"
#define	GOURAUD_TRIANGLES_CHK		"gouraud_triangles"
#define	POLYGONS_CHK			"polygons"
#define	SPLINE_CURVES_CHK		"spline_curves"
#define	AA_POLYGONS_CHK   		"aa_polygons"
#define	CLIPPING_CHK			"clipping"
#define	HIDDEN_SURFACE_REMOVAL_CHK	"hidden_surface_removal"
#define	SURFACE_FILL_CHK		"surface_fill"
#define	POLYGON_EDGE_HILITE_CHK		"polygon_edge_hilite"
#define	TRANSPARENCY_CHK		"transparency"
#define	DEPTH_CUEING_CHK		"depth_cueing"
#define	LIGHTING_SHADING_CHK		"lighting_shading"
#define	LIGHTED_SUN_LOGO		"lighted_sun_logo"
#define	MOLEC1				"molec1"
#define	BLENDER_C			"blender_c"
#define	TEXT_CHK			"text"
#define	MARKERS_CHK			"markers"
#define	PICK_ECHO_CHK			"picking"
#define	PICK_DETECT_CHK			"pick_detect"
#define	ARBITRATION_A_CHK		"arbitration_a"
#define	ARBITRATION_B_CHK		"arbitration_b"
#define	ARBITRATION_DB_CHK		"arbitration_db"
#define	STEREO_CHK			"stereo"
#define SAVE_FB_MODE			"save_fb_mode"
#define RESTORE_FB_MODE			"restore_fb_mode"


/* chanel in 24-bit buffer */
#define         RED_CHK                             1
#define         GREEN_CHK                           2
#define         BLUE_CHK                            3

#define		HDL_CHK				    4

struct test {
			char *testname;
			int  acc_port;
			char *(*proc)();
			struct test *nexttest;
		};

extern char *exec_dl();
extern char *chksum_verify();


/* Context Attributes */

typedef enum {
		CTX_ATTR_END,
	     } Ctx_attr;

