/*      @(#)esd.h 1.1 92/07/30 SMI      */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */


/* Definitions */

/* timer in menu */
#define		ITIMER_NULL			((struct itimerval *)0)

/* test number */
#define		NO_TEST				0
#define		INTEGRATION_TEST		1
#define		SINGLE_BLOCK_TEST		2


/* test list selection */
#define		LIST_DEFAULT			1
#define		LIST_ALL			2
#define		LIST_SELECT			3

/* Flag for test modes */
#define		VERIFY_FLAG			1	/* Real test */
#define		WRITE_FLAG			2	/* Generating
							   templates */

/* chanel in 24-bit buffer */
#define		RED_CHK				1
#define		GREEN_CHK			2
#define		BLUE_CHK			3

/* default file names */
#define		TEXTSW_TMP_FILE			"/tmp/esd_scratch_file"
#define		FRAME_BUFFER_DEVICE_NAME	"/dev/cgtwelve0"

/* GPSI test code for c30 built-in tests */
#define		GP1_DIAG			0x7f00

/* test names in chk sum data base */
#define		GPSI_3D_LINES_CHK	"cg12_3D_lines"
#define		GPSI_3D_SHAD_LINES_CHK	"cg12_3D_shaded_lines"
#define		GPSI_3D_POLIGONS_CHK	"cg12_3D_poligons"
#define		GPSI_ANIMATION_CHK	"cg12_animation"
#define		GPSI_CLIP_CHK		"cg12_clip"
#define		GPSI_HID_CHK		"cg12_hid"
#define		GPSI_DEPTH_CUEING_CHK	"cg12_depth_cueing"
#define		GPSI_SHAD_CHK		"cg12_shad"
#define		GPSI_XF_CHK		"cg12_xf"
#define		GPSI_TEXTURED_LINES_CHK "cg12_textured_lines"

/* GPSI DIAG test codes */

#define		DSP_TEST			1
#define		SRAM_TEST			2
#define		DRAM_TEST			3
#define		BPROM_TEST			4

/* GPSI Utilyties */
short *allocbuf();
char *ctx_set();
char *set_3D_matrix();
char *set_matrix_num();
char *chksum_verify();
int postbuf();

/* test error messages */
extern char *errmsg_list[];

/* structure of each test in the list */
struct test {
			char *testname;
			int dfault;
			char *(*proc)();
			struct test *nexttest;
		};

typedef enum {

		CTX_ATTR_END,
		CTX_DONT_CLEAR,
		CTX_SET_FB_NUM,
		CTX_SET_FB_PLANES,
		CTX_SET_ROP,
		CTX_SET_CLIP_LIST,
		CTX_SET_CLIP_PLANES,
		CTX_SET_LINE_WIDTH,
		CTX_SET_LINE_TEX,
		CTX_SET_VWP_3D,
		CTX_SET_MAT_NUM,
		CTX_SET_MAT_3D,
		CTX_SET_ZBUF,
		CTX_SET_DEPTH_CUE,
		CTX_SET_DEPTH_CUE_COLORS,
		CTX_SET_HIDDEN_SURF,
		CTX_SET_RGB_COLOR,
		CTX_SET_COLOR,

	      } Context_attribute;

typedef struct {
		float m[4][4];
	      } Matrix_3D;


/* transformation values */
typedef struct {
		float xt; /* translation in x */
		float xs; /* scaling in x */
		float xr; /* rotation around x axis */
		float yt; /* translation in y */
		float ys; /* scaling in y */
		float yr; /* rotation around y axis */
		float zt; /* translation in z */
		float zs; /* scaling in z */
		float zr; /* rotation around z axis */
	       } Xf;


struct vector {
    unsigned short ctrl_field;
    float x;
    float y;
    float z;
};

struct chksum {
    char *name;
    int red;
    int green;
    int blue;
};

