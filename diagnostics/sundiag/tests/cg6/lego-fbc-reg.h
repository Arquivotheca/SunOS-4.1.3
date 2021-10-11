
/*
 * Lego register definitions, common to all environments.
 */

#ifndef lint
static char	sccsid_lego_fbc_reg_h[] = { "@(#)lego-fbc-reg.h 3.8 88/08/23" };
#endif lint

/*
 * FBC register offsets from base address. These offsets are
 * intended to be added to a pointer-to-integer whose value is the
 * base address of the Lego memory mapped register area.
 */

/* status and command registers */
#define L_FBC_STATUS		( 0x010 / sizeof(int) )
#define L_FBC_DRAWSTATUS	( 0x014 / sizeof(int) )
#define L_FBC_BLITSTATUS	( 0x018 / sizeof(int) )
#define L_FBC_FONT		( 0x01C / sizeof(int) )

/* address registers */
/* writing a z-register just sets the corresponding z clip status bits */
#define L_FBC_X0		( 0x080 / sizeof(int) )
#define L_FBC_Y0		( 0x084 / sizeof(int) )
#define L_FBC_Z0		( 0x088 / sizeof(int) )
#define L_FBC_COLOR0		( 0x08C / sizeof(int) )
#define L_FBC_X1		( 0x090 / sizeof(int) )
#define L_FBC_Y1		( 0x094 / sizeof(int) )
#define L_FBC_Z1		( 0x098 / sizeof(int) )
#define L_FBC_COLOR1		( 0x09C / sizeof(int) )
#define L_FBC_X2		( 0x0A0 / sizeof(int) )
#define L_FBC_Y2		( 0x0A4 / sizeof(int) )
#define L_FBC_Z2		( 0x0A8 / sizeof(int) )
#define L_FBC_COLOR2		( 0x0AC / sizeof(int) )
#define L_FBC_X3		( 0x0B0 / sizeof(int) )
#define L_FBC_Y3		( 0x0B4 / sizeof(int) )
#define L_FBC_Z3		( 0x0B8 / sizeof(int) )
#define L_FBC_COLOR3		( 0x0BC / sizeof(int) )

/* raster offset registers */
#define L_FBC_RASTEROFFX	( 0x0C0 / sizeof(int) )
#define L_FBC_RASTEROFFY	( 0x0C4 / sizeof(int) )

/* autoincrement registers */
#define L_FBC_AUTOINCX		( 0x0D0 / sizeof(int) )
#define L_FBC_AUTOINCY		( 0x0D4 / sizeof(int) )

/* indexed address registers */

#define L_FBC_IPOINTABSX	( 0x800 / sizeof(int) )
#define L_FBC_IPOINTABSY	( 0x804 / sizeof(int) )
#define L_FBC_IPOINTABSZ	( 0x808 / sizeof(int) )
#define L_FBC_IPOINTRELX	( 0x810 / sizeof(int) )
#define L_FBC_IPOINTRELY	( 0x814 / sizeof(int) )
#define L_FBC_IPOINTRELZ	( 0x818 / sizeof(int) )

#define L_FBC_IPOINTCOLR	( 0x830 / sizeof(int) )
#define L_FBC_IPOINTCOLG	( 0x834 / sizeof(int) )
#define L_FBC_IPOINTCOLB	( 0x838 / sizeof(int) )
#define L_FBC_IPOINTCOLA	( 0x83C / sizeof(int) )

#define L_FBC_ILINEABSX		( 0x840 / sizeof(int) )
#define L_FBC_ILINEABSY		( 0x844 / sizeof(int) )
#define L_FBC_ILINEABSZ		( 0x848 / sizeof(int) )
#define L_FBC_ILINERELX		( 0x850 / sizeof(int) )
#define L_FBC_ILINERELY		( 0x854 / sizeof(int) )
#define L_FBC_ILINERELZ		( 0x858 / sizeof(int) )

#define L_FBC_ILINECOLR		( 0x870 / sizeof(int) )
#define L_FBC_ILINECOLG		( 0x874 / sizeof(int) )
#define L_FBC_ILINECOLB		( 0x878 / sizeof(int) )
#define L_FBC_ILINECOLA		( 0x87C / sizeof(int) )

#define L_FBC_ITRIABSX		( 0x880 / sizeof(int) )
#define L_FBC_ITRIABSY		( 0x884 / sizeof(int) )
#define L_FBC_ITRIABSZ		( 0x888 / sizeof(int) )
#define L_FBC_ITRIRELX		( 0x890 / sizeof(int) )
#define L_FBC_ITRIRELY		( 0x894 / sizeof(int) )
#define L_FBC_ITRIRELZ		( 0x898 / sizeof(int) )

#define L_FBC_ITRICOLR		( 0x8B0 / sizeof(int) )
#define L_FBC_ITRICOLG		( 0x8B4 / sizeof(int) )
#define L_FBC_ITRICOLB		( 0x8B8 / sizeof(int) )
#define L_FBC_ITRICOLA		( 0x8BC / sizeof(int) )

#define L_FBC_IQUADABSX		( 0x8C0 / sizeof(int) )
#define L_FBC_IQUADABSY		( 0x8C4 / sizeof(int) )
#define L_FBC_IQUADABSZ		( 0x8C8 / sizeof(int) )
#define L_FBC_IQUADRELX		( 0x8D0 / sizeof(int) )
#define L_FBC_IQUADRELY		( 0x8D4 / sizeof(int) )
#define L_FBC_IQUADRELZ		( 0x8D8 / sizeof(int) )

#define L_FBC_IQUADCOLR		( 0x8F0 / sizeof(int) )
#define L_FBC_IQUADCOLG		( 0x8F4 / sizeof(int) )
#define L_FBC_IQUADCOLB		( 0x8F8 / sizeof(int) )
#define L_FBC_IQUADCOLA		( 0x8FC / sizeof(int) )

#define L_FBC_IRECTABSX		( 0x900 / sizeof(int) )
#define L_FBC_IRECTABSY		( 0x904 / sizeof(int) )
#define L_FBC_IRECTABSZ		( 0x908 / sizeof(int) )
#define L_FBC_IRECTRELX		( 0x910 / sizeof(int) )
#define L_FBC_IRECTRELY		( 0x914 / sizeof(int) )
#define L_FBC_IRECTRELZ		( 0x918 / sizeof(int) )

#define L_FBC_IRECTCOLR		( 0x930 / sizeof(int) )
#define L_FBC_IRECTCOLG		( 0x934 / sizeof(int) )
#define L_FBC_IRECTCOLB		( 0x938 / sizeof(int) )
#define L_FBC_IRECTCOLA		( 0x93C / sizeof(int) )

/* window registers */
#define L_FBC_CLIPMINX		( 0x0E0 / sizeof(int) )
#define L_FBC_CLIPMINY		( 0x0E4 / sizeof(int) )
#define L_FBC_CLIPMAXX		( 0x0F0 / sizeof(int) )
#define L_FBC_CLIPMAXY		( 0x0F4 / sizeof(int) )

/* clipcheck register */
#define L_FBC_CLIPCHECK		( 0x008 / sizeof(int) )

/* attribute registers */
#define L_FBC_FCOLOR		( 0x100 / sizeof(int) )
#define L_FBC_BCOLOR		( 0x104 / sizeof(int) )
#define L_FBC_RASTEROP		( 0x108 / sizeof(int) )
#define L_FBC_PLANEMASK		( 0x10C / sizeof(int) )
#define L_FBC_PIXELMASK		( 0x110 / sizeof(int) )
#define L_FBC_PATTERN0		( 0x120 / sizeof(int) )
#define L_FBC_PATTERN1		( 0x124 / sizeof(int) )
#define L_FBC_PATTERN2		( 0x128 / sizeof(int) )
#define L_FBC_PATTERN3		( 0x12C / sizeof(int) )
#define L_FBC_PATTERN4		( 0x130 / sizeof(int) )
#define L_FBC_PATTERN5		( 0x134 / sizeof(int) )
#define L_FBC_PATTERN6		( 0x138 / sizeof(int) )
#define L_FBC_PATTERN7		( 0x13C / sizeof(int) )
#define L_FBC_PATTALIGN		( 0x11C / sizeof(int) )

/* miscellaneous register */
#define L_FBC_MISC		( 0x004 / sizeof(int) )

/*
 * FBC CLIPCHECK register bits.
 */
#define CLIP_MASK	0x3
#define CLIP_IN		0x0
#define CLIP_LT		0x1
#define CLIP_GT		0x2
#define CLIP_BACK	0x3

#define CLIP_X(bits, reg_num)	((bits) << (0+(2*(reg_num))))
#define CLIP_Y(bits, reg_num)	((bits) << (8+(2*(reg_num))))
#define CLIP_Z(bits, reg_num)	((bits) << (16+(2*(reg_num))))

/*
 * FBC STATUS, DRAWSTATUS, and BLITSTATUS register bits.
 */
#define L_FBC_ACC_CLEAR		0x80000000	/* when writing STATUS */
#define L_FBC_DRAW_EXCEPTION	0x80000000	/* when reading DRAWSTATUS */
#define L_FBC_BLIT_EXCEPTION	0x80000000	/* when reading BLITSTATUS */
#define L_FBC_TEC_EXCEPTION	0x40000000
#define L_FBC_FULL		0x20000000
#define L_FBC_BUSY		0x10000000
#define L_FBC_UNSUPPORTED_ATTR	0x02000000
#define L_FBC_HRMONO		0x01000000
#define L_FBC_ACC_OVERFLOW	0x00200000
#define L_FBC_ACC_PICK		0x00100000
#define L_FBC_TEC_HIDDEN	0x00040000
#define L_FBC_TEC_INTERSECT	0x00020000
#define L_FBC_TEC_VISIBLE	0x00010000
#define L_FBC_BLIT_HARDWARE	0x00008000
#define L_FBC_BLIT_SOFTWARE	0x00004000
#define L_FBC_BLIT_SRC_HID	0x00002000
#define L_FBC_BLIT_SRC_INT	0x00001000
#define L_FBC_BLIT_SRC_VIS	0x00000800
#define L_FBC_BLIT_DST_HID	0x00000400
#define L_FBC_BLIT_DST_INT	0x00000200
#define L_FBC_BLIT_DST_VIS	0x00000100
#define L_FBC_DRAW_HARDWARE	0x00000010
#define L_FBC_DRAW_SOFTWARE	0x00000008
#define L_FBC_DRAW_HIDDEN	0x00000004
#define L_FBC_DRAW_INTERSECT	0x00000002
#define L_FBC_DRAW_VISIBLE	0x00000001

/*
 * FBC RASTEROP register bits
 */
typedef enum {
	L_FBC_RASTEROP_PLANE_IGNORE, L_FBC_RASTEROP_PLANE_ZEROES,
	L_FBC_RASTEROP_PLANE_ONES, L_FBC_RASTEROP_PLANE_MASK
	} l_fbc_rasterop_plane_t;

typedef enum {
	L_FBC_RASTEROP_PIXEL_IGNORE, L_FBC_RASTEROP_PIXEL_ZEROES,
	L_FBC_RASTEROP_PIXEL_ONES, L_FBC_RASTEROP_PIXEL_MASK
	} l_fbc_rasterop_pixel_t;

typedef enum {
	L_FBC_RASTEROP_PATTERN_IGNORE, L_FBC_RASTEROP_PATTERN_ZEROES,
	L_FBC_RASTEROP_PATTERN_ONES, L_FBC_RASTEROP_PATTERN_MASK
	} l_fbc_rasterop_patt_t;

typedef enum {
	L_FBC_RASTEROP_POLYG_IGNORE, L_FBC_RASTEROP_POLYG_OVERLAP,
	L_FBC_RASTEROP_POLYG_NONOVERLAP, L_FBC_RASTEROP_POLYG_ILLEGAL
	} l_fbc_rasterop_polyg_t;

typedef enum {
	L_FBC_RASTEROP_ATTR_IGNORE, L_FBC_RASTEROP_ATTR_UNSUPP,
	L_FBC_RASTEROP_ATTR_SUPP, L_FBC_RASTEROP_ATTR_ILLEGAL
	} l_fbc_rasterop_attr_t;

typedef enum {
	L_FBC_RASTEROP_RAST_BOOL, L_FBC_RASTEROP_RAST_LINEAR
	} l_fbc_rasterop_rast_t;

typedef enum {
	L_FBC_RASTEROP_PLOT_PLOT, L_FBC_RASTEROP_PLOT_UNPLOT
	} l_fbc_rasterop_plot_t;

struct l_fbc_rasterop {
	l_fbc_rasterop_plane_t	l_fbc_rasterop_plane : 2; /* plane mask */
	l_fbc_rasterop_pixel_t	l_fbc_rasterop_pixel : 2; /* pixel mask */
	l_fbc_rasterop_patt_t	l_fbc_rasterop_patt : 2;  /* pattern mask */
	l_fbc_rasterop_polyg_t	l_fbc_rasterop_polyg : 2; /* polygon draw */
	l_fbc_rasterop_attr_t	l_fbc_rasterop_attr : 2;  /* attribute select */
	unsigned	: 4;				  /* not used */
	l_fbc_rasterop_rast_t	l_fbc_rasterop_rast : 1;  /* rasterop mode */
	l_fbc_rasterop_plot_t	l_fbc_rasterop_plot : 1;  /* plot/unplot mode */
	unsigned	l_fbc_rasterop_rop11: 4; /* rasterop for f==1, b==1 */
	unsigned	l_fbc_rasterop_rop10: 4; /* rasterop for f==1, b==0 */
	unsigned	l_fbc_rasterop_rop01: 4; /* rasterop for f==0, b==1 */
	unsigned	l_fbc_rasterop_rop00: 4; /* rasterop for f==0, b==0 */
	};

/*
 * FBC PATTALIGN register bits
 */
struct  l_fbc_pattalign {
	unsigned	: 12;				/* not used */
	unsigned l_fbc_pattalign_alignx : 4;		/* x alignment */
	unsigned	: 12;				/* not used */
	unsigned l_fbc_pattalign_aligny : 4;		/* y alignment */
	};

/*
 * FBC MISC register bits
 */
typedef enum {
	L_FBC_MISC_BLIT_IGNORE, L_FBC_MISC_BLIT_NOSRC, L_FBC_MISC_BLIT_SRC,
	L_FBC_MISC_BLIT_ILLEGAL
	} l_fbc_misc_blit_t;

typedef enum {
	L_FBC_MISC_DATA_IGNORE, L_FBC_MISC_DATA_COLOR8, L_FBC_MISC_DATA_COLOR1,
	L_FBC_MISC_DATA_HRMONO
	} l_fbc_misc_data_t;

typedef enum {
	L_FBC_MISC_DRAW_IGNORE, L_FBC_MISC_DRAW_RENDER, L_FBC_MISC_DRAW_PICK,
	L_FBC_MISC_DRAW_ILLEGAL
	} l_fbc_misc_draw_t;

typedef enum {
	L_FBC_MISC_BWRITE0_IGNORE, L_FBC_MISC_BWRITE0_ENABLE,
	L_FBC_MISC_BWRITE0_DISABLE, L_FBC_MISC_BWRITE0_ILLEGAL
	} l_fbc_misc_bwrite0_t;

typedef enum {
	L_FBC_MISC_BWRITE1_IGNORE, L_FBC_MISC_BWRITE1_ENABLE,
	L_FBC_MISC_BWRITE1_DISABLE, L_FBC_MISC_BWRITE1_ILLEGAL
	} l_fbc_misc_bwrite1_t;

typedef enum {
	L_FBC_MISC_BREAD_IGNORE, L_FBC_MISC_BREAD_0, L_FBC_MISC_BREAD_1,
	L_FBC_MISC_BREAD_ILLEGAL
	} l_fbc_misc_bread_t;

typedef enum {
	L_FBC_MISC_BDISP_IGNORE, L_FBC_MISC_BDISP_0, L_FBC_MISC_BDISP_1,
	L_FBC_MISC_BDISP_ILLEGAL
	} l_fbc_misc_bdisp_t;

struct l_fbc_misc {
	unsigned	: 10;			/* not used */
	l_fbc_misc_blit_t	l_fbc_misc_blit : 2;	/* blit src check */
	unsigned	l_fbc_misc_vblank : 1;	/* 1 == VBLANK has occured */
	l_fbc_misc_data_t	l_fbc_misc_data : 2;	/* Color mode select  */
	l_fbc_misc_draw_t	l_fbc_misc_draw : 2;	/* Render/Pick mode  */
	l_fbc_misc_bwrite0_t	l_fbc_misc_bwrite0 : 2;	/* buffer 0 write */
	l_fbc_misc_bwrite1_t	l_fbc_misc_bwrite1 : 2;	/* buffer 1 write */
	l_fbc_misc_bread_t	l_fbc_misc_bread : 2;	/* read enable	  */
	l_fbc_misc_bdisp_t	l_fbc_misc_bdisp : 2;	/* display enable  */
	unsigned	l_fbc_misc_index_mod : 1;	/* modify index  */
	unsigned	l_fbc_misc_index : 2;		/* index	  */
	unsigned	: 4;				/* not used */
	};


/*
 * FBC registers defined as a structure.
 */
struct l_fbc {
	int	l_fbc_pad_0[1-0];
	struct l_fbc_misc	l_fbc_misc;		/* 1 */
	int	l_fbc_clipcheck;			/* 2 */
	int	l_fbc_pad_3[4-3];
	int	l_fbc_status;				/* 4 */
	int	l_fbc_drawstatus;			/* 5 */
	int	l_fbc_blitstatus;			/* 6 */
	int	l_fbc_font;				/* 7 */
	int	l_fbc_pad_8[32-8];
	int	l_fbc_x0;				/* 32 */
	int	l_fbc_y0;				/* 33 */
	int	l_fbc_z0;				/* 34 */
	int	l_fbc_color0;				/* 35 */
	int	l_fbc_x1;				/* 36 */
	int	l_fbc_y1;				/* 37 */
	int	l_fbc_z1;				/* 38 */
	int	l_fbc_color1;				/* 39 */
	int	l_fbc_x2;				/* 40 */
	int	l_fbc_y2;				/* 41 */
	int	l_fbc_z2;				/* 42 */
	int	l_fbc_color2;				/* 43 */
	int	l_fbc_x3;				/* 44 */
	int	l_fbc_y3;				/* 45 */
	int	l_fbc_z3;				/* 46 */
	int	l_fbc_color3;				/* 47 */
	int	l_fbc_rasteroffx;			/* 48 */
	int	l_fbc_rasteroffy;			/* 49 */
	int	l_fbc_pad_50[52-50];
	int	l_fbc_autoincx;				/* 52 */
	int	l_fbc_autoincy;				/* 53 */
	int	l_fbc_pad_54[56-54];
	int	l_fbc_clipminx;				/* 56 */
	int	l_fbc_clipminy;				/* 57 */
	int	l_fbc_pad_58[60-58];
	int	l_fbc_clipmaxx;				/* 60 */
	int	l_fbc_clipmaxy;				/* 61 */
	int	l_fbc_pad_62[64-62];
	int	l_fbc_fcolor;				/* 64 */
	int	l_fbc_bcolor;				/* 65 */
	struct l_fbc_rasterop	l_fbc_rasterop;		/* 66 */
	int	l_fbc_planemask;			/* 67 */
	int	l_fbc_pixelmask;			/* 68 */
	int	l_fbc_pad1_69[71-69];
	struct l_fbc_pattalign	l_fbc_pattalign;	/* 71 */
	int	l_fbc_pattern0;				/* 72 */
	int	l_fbc_pattern1;				/* 73 */
	int	l_fbc_pattern2;				/* 74 */
	int	l_fbc_pattern3;				/* 75 */
	int	l_fbc_pattern4;				/* 76 */
	int	l_fbc_pattern5;				/* 77 */
	int	l_fbc_pattern6;				/* 78 */
	int	l_fbc_pattern7;				/* 79 */
	int	l_fbc_pad_80[512-80];
	int	l_fbc_ipointabsx;			/* 512 */
	int	l_fbc_ipointabsy;			/* 513 */
	int	l_fbc_ipointabsz;			/* 514 */
	int	l_fbc_pad_515[516-515];
	int	l_fbc_ipointrelx;			/* 516 */
	int	l_fbc_ipointrely;			/* 517 */
	int	l_fbc_ipointrelz;			/* 518 */
	int	l_fbc_pad_519[524-519];
	int	l_fbc_ipointcolr;			/* 524 */
	int	l_fbc_ipointcolg;			/* 525 */
	int	l_fbc_ipointcolb;			/* 526 */
	int	l_fbc_ipointcola;			/* 527 */
	int	l_fbc_ilineabsx;			/* 528 */
	int	l_fbc_ilineabsy;			/* 529 */
	int	l_fbc_ilineabsz;			/* 530 */
	int	l_fbc_pad_521[532-531];
	int	l_fbc_ilinerelx;			/* 532 */
	int	l_fbc_ilinerely;			/* 533 */
	int	l_fbc_ilinerelz;			/* 534 */
	int	l_fbc_pad_535[540-535];
	int	l_fbc_ilinecolr;			/* 540 */
	int	l_fbc_ilinecolg;			/* 541 */
	int	l_fbc_ilinecolb;			/* 542 */
	int	l_fbc_ilinecola;			/* 543 */
	int	l_fbc_itriabsx;				/* 544 */
	int	l_fbc_itriabsy;				/* 545 */
	int	l_fbc_itriabsz;				/* 546 */
	int	l_fbc_pad_547[548-547];
	int	l_fbc_itrirelx;				/* 548 */
	int	l_fbc_itrirely;				/* 549 */
	int	l_fbc_itrirelz;				/* 550 */
	int	l_fbc_pad_551[556-551];
	int	l_fbc_itricolr;				/* 556 */
	int	l_fbc_itricolg;				/* 557 */
	int	l_fbc_itricolb;				/* 558 */
	int	l_fbc_itricola;				/* 559 */
	int	l_fbc_iquadabsx;			/* 560 */
	int	l_fbc_iquadabsy;			/* 561 */
	int	l_fbc_iquadabsz;			/* 562 */
	int	l_fbc_pad_563[564-563];
	int	l_fbc_iquadrelx;			/* 564 */
	int	l_fbc_iquadrely;			/* 565 */
	int	l_fbc_iquadrelz;			/* 566 */
	int	l_fbc_pad_567[572-567];
	int	l_fbc_iquadcolr;			/* 572 */
	int	l_fbc_iquadcolg;			/* 573 */
	int	l_fbc_iquadcolb;			/* 574 */
	int	l_fbc_iquadcola;			/* 575 */
	int	l_fbc_irectabsx;			/* 576 */
	int	l_fbc_irectabsy;			/* 577 */
	int	l_fbc_irectabsz;			/* 578 */
	int	l_fbc_pad_579[580-579];
	int	l_fbc_irectrelx;			/* 580 */
	int	l_fbc_irectrely;			/* 581 */
	int	l_fbc_irectrelz;			/* 582 */
	int	l_fbc_pad_583[588-583];
	int	l_fbc_irectcolr;			/* 588 */
	int	l_fbc_irectcolg;			/* 589 */
	int	l_fbc_irectcolb;			/* 590 */
	int	l_fbc_irectcola;			/* 591 */
	};
