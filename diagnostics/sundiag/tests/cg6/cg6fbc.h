/*
 * @(#)cg6fbc.h 1.5 89/05/09 SMI
 */

/*
 * Copyright 1988-1989, Sun Microsystems, Inc.
 */

#ifndef cg6fbc_h
#define cg6fbc_h

/*
 * CG6 register definitions, common to all environments.
 */

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
 * FBC RASTEROP register bits
 */
typedef enum {
	L_FBC_RASTEROP_PLANE_IGNORE, L_FBC_RASTEROP_PLANE_ZEROES,
	L_FBC_RASTEROP_PLANE_ONES, L_FBC_RASTEROP_PLANE_MASK,
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
union l_fbc_pattalign {
	unsigned word;
	unsigned short l_fbc_pattalign_array[2];
#define	l_fbc_pattalign_alignx	l_fbc_pattalign_array[0]
#define	l_fbc_pattalign_aligny	l_fbc_pattalign_array[1]
};

/*
 * FBC offsets &  structure definition
 */
struct fbc {

/* miscellaneous & clipcheck registers */

	char			fil0[ 0x4 ];

	struct	l_fbc_misc	l_fbc_misc;
	u_int			l_fbc_clipcheck;

#define L_FBC_MISC		( 0x004 / sizeof(int) )
#define L_FBC_CLIPCHECK		( 0x008 / sizeof(int) )

	char			fill00[ 0x10 - 0x08 - 4 ];

	u_int			l_fbc_status;
	u_int			l_fbc_drawstatus;
	u_int			l_fbc_blitstatus;
	u_int			l_fbc_font;
	
/* status and command registers */
#define L_FBC_STATUS		( 0x010 / sizeof(int) )
#define L_FBC_DRAWSTATUS	( 0x014 / sizeof(int) )
#define L_FBC_BLITSTATUS	( 0x018 / sizeof(int) )
#define L_FBC_FONT		( 0x01C / sizeof(int) )

	char			fill01[ 0x80 - 0x1C - 4 ];

	u_int			l_fbc_x0;
	u_int			l_fbc_y0;
	u_int			l_fbc_z0;
	u_int			l_fbc_color0;
	u_int			l_fbc_x1;
	u_int			l_fbc_y1;
	u_int			l_fbc_z1;
	u_int			l_fbc_color1;
	u_int			l_fbc_x2;
	u_int			l_fbc_y2;
	u_int			l_fbc_z2;
	u_int			l_fbc_color2;
	u_int			l_fbc_x3;
	u_int			l_fbc_y3;
	u_int			l_fbc_z3;
	u_int			l_fbc_color3;

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

	u_int			l_fbc_rasteroffx;
	u_int			l_fbc_rasteroffy;

#define L_FBC_RASTEROFFX	( 0x0C0 / sizeof(int) )
#define L_FBC_RASTEROFFY	( 0x0C4 / sizeof(int) )

	char			fill02[ 0xD0 - 0xC4 - 4 ];

	u_int			l_fbc_autoincx;
	u_int			l_fbc_autoincy;

/* autoincrement registers */
#define L_FBC_AUTOINCX		( 0x0D0 / sizeof(int) )
#define L_FBC_AUTOINCY		( 0x0D4 / sizeof(int) )


/* window registers */

	char			fill03[ 0xE0 - 0xD4 - 4 ];

	u_int			l_fbc_clipminx;
	u_int			l_fbc_clipminy;

#define L_FBC_CLIPMINX		( 0x0E0 / sizeof(int) )
#define L_FBC_CLIPMINY		( 0x0E4 / sizeof(int) )

	char			fill04[ 0xF0 - 0xE4 - 4 ];

	u_int			l_fbc_clipmaxx;
	u_int			l_fbc_clipmaxy;

#define L_FBC_CLIPMAXX		( 0x0F0 / sizeof(int) )
#define L_FBC_CLIPMAXY		( 0x0F4 / sizeof(int) )

	char			fill05[ 0x100 - 0x0F4 - 4 ];

	u_int			l_fbc_fcolor;
	u_int			l_fbc_bcolor;
	struct l_fbc_rasterop	l_fbc_rasterop;
	u_int			l_fbc_planemask;
	u_int			l_fbc_pixelmask;

/* attribute registers */
#define L_FBC_FCOLOR		( 0x100 / sizeof(int) )
#define L_FBC_BCOLOR		( 0x104 / sizeof(int) )
#define L_FBC_RASTEROP		( 0x108 / sizeof(int) )
#define L_FBC_PLANEMASK		( 0x10C / sizeof(int) )
#define L_FBC_PIXELMASK		( 0x110 / sizeof(int) )

	char 			fill06[ 0x11C - 0x110 - 4 ];

	union l_fbc_pattalign	l_fbc_pattalign;

#define L_FBC_PATTALIGN		( 0x11C / sizeof(int) )

	u_int			l_fbc_pattern0;
	u_int			l_fbc_pattern1;
	u_int			l_fbc_pattern2;
	u_int			l_fbc_pattern3;
	u_int			l_fbc_pattern4;
	u_int			l_fbc_pattern5;
	u_int			l_fbc_pattern6;
	u_int			l_fbc_pattern7;

#define L_FBC_PATTERN0		( 0x120 / sizeof(int) )
#define L_FBC_PATTERN1		( 0x124 / sizeof(int) )
#define L_FBC_PATTERN2		( 0x128 / sizeof(int) )
#define L_FBC_PATTERN3		( 0x12C / sizeof(int) )
#define L_FBC_PATTERN4		( 0x130 / sizeof(int) )
#define L_FBC_PATTERN5		( 0x134 / sizeof(int) )
#define L_FBC_PATTERN6		( 0x138 / sizeof(int) )
#define L_FBC_PATTERN7		( 0x13C / sizeof(int) )

/* indexed address registers */

	char			fill07[ 0x800 - 0x13C - 4 ];

	u_int 			l_fbc_ipointabsx;
	u_int 			l_fbc_ipointabsy;
	u_int 			l_fbc_ipointabsz;

#define L_FBC_IPOINTABSX	( 0x800 / sizeof(int) )
#define L_FBC_IPOINTABSY	( 0x804 / sizeof(int) )
#define L_FBC_IPOINTABSZ	( 0x808 / sizeof(int) )

	char 			fill08[ 0x810 - 0x808 - 4 ];

	u_int 			l_fbc_ipointrelx;
	u_int 			l_fbc_ipointrely;
	u_int 			l_fbc_ipointrelz;

#define L_FBC_IPOINTRELX	( 0x810 / sizeof(int) )
#define L_FBC_IPOINTRELY	( 0x814 / sizeof(int) )
#define L_FBC_IPOINTRELZ	( 0x818 / sizeof(int) )

	char			fill09[ 0x830 - 0x818 - 4 ];

	u_int			l_fbc_ipointcolr;
	u_int			l_fbc_ipointcolg;
	u_int			l_fbc_ipointcolb;
	u_int			l_fbc_ipointcola;
	u_int			l_fbc_ilineabsx;
	u_int			l_fbc_ilineabsy;
	u_int			l_fbc_ilineabsz;

#define L_FBC_IPOINTCOLR	( 0x830 / sizeof(int) )
#define L_FBC_IPOINTCOLG	( 0x834 / sizeof(int) )
#define L_FBC_IPOINTCOLB	( 0x838 / sizeof(int) )
#define L_FBC_IPOINTCOLA	( 0x83C / sizeof(int) )
#define L_FBC_ILINEABSX		( 0x840 / sizeof(int) )
#define L_FBC_ILINEABSY		( 0x844 / sizeof(int) )
#define L_FBC_ILINEABSZ		( 0x848 / sizeof(int) )

	char			fill10[ 0x850 - 0x848 - 4 ];

	u_int			l_fbc_ilinerelx;
	u_int			l_fbc_ilinerely;
	u_int			l_fbc_ilinerelz;

#define L_FBC_ILINERELX		( 0x850 / sizeof(int) )
#define L_FBC_ILINERELY		( 0x854 / sizeof(int) )
#define L_FBC_ILINERELZ		( 0x858 / sizeof(int) )

	char			fill11[ 0x870 - 0x858 - 4 ];

	u_int			l_fbc_ilinecolr;
	u_int			l_fbc_ilinecolg;
	u_int			l_fbc_ilinecolb;
	u_int			l_fbc_ilinecola;

#define L_FBC_ILINECOLR		( 0x870 / sizeof(int) )
#define L_FBC_ILINECOLG		( 0x874 / sizeof(int) )
#define L_FBC_ILINECOLB		( 0x878 / sizeof(int) )
#define L_FBC_ILINECOLA		( 0x87C / sizeof(int) )

	u_int			l_fbc_itriabsx;
	u_int			l_fbc_itriabsy;
	u_int			l_fbc_itriabsz;

#define L_FBC_ITRIABSX		( 0x880 / sizeof(int) )
#define L_FBC_ITRIABSY		( 0x884 / sizeof(int) )
#define L_FBC_ITRIABSZ		( 0x888 / sizeof(int) )

	char			fill12[ 0x890 - 0x888 - 4 ];

	u_int			l_fbc_itrirelx;
	u_int			l_fbc_itrirely;
	u_int			l_fbc_itrirelz;

#define L_FBC_ITRIRELX		( 0x890 / sizeof(int) )
#define L_FBC_ITRIRELY		( 0x894 / sizeof(int) )
#define L_FBC_ITRIRELZ		( 0x898 / sizeof(int) )

	char			fill13[ 0x8B0 - 0x898 - 4 ];

	u_int			l_fbc_itricolr;
	u_int			l_fbc_itricolg;
	u_int			l_fbc_itricolb;
	u_int			l_fbc_itricola;
	u_int			l_fbc_iquadabsx;
	u_int			l_fbc_iquadabsy;
	u_int			l_fbc_iquadabsz;

#define L_FBC_ITRICOLR		( 0x8B0 / sizeof(int) )
#define L_FBC_ITRICOLG		( 0x8B4 / sizeof(int) )
#define L_FBC_ITRICOLB		( 0x8B8 / sizeof(int) )
#define L_FBC_ITRICOLA		( 0x8BC / sizeof(int) )
#define L_FBC_IQUADABSX		( 0x8C0 / sizeof(int) )
#define L_FBC_IQUADABSY		( 0x8C4 / sizeof(int) )
#define L_FBC_IQUADABSZ		( 0x8C8 / sizeof(int) )

	char			fill14[ 0x8D0 - 0x8C8 - 4 ];

	u_int			l_fbc_iquadrelx;
	u_int			l_fbc_iquadrely;
	u_int			l_fbc_iquadrelz;

#define L_FBC_IQUADRELX		( 0x8D0 / sizeof(int) )
#define L_FBC_IQUADRELY		( 0x8D4 / sizeof(int) )
#define L_FBC_IQUADRELZ		( 0x8D8 / sizeof(int) )

	char			fill15[ 0x8F0 - 0x8D8 - 4 ];

	u_int			l_fbc_iquadcolr;
	u_int			l_fbc_iquadcolg;
	u_int			l_fbc_iquadcolb;
	u_int			l_fbc_iquadcola;
	u_int			l_fbc_irectabsx;
	u_int			l_fbc_irectabsy;
	u_int			l_fbc_irectabsz;

#define L_FBC_IQUADCOLR		( 0x8F0 / sizeof(int) )
#define L_FBC_IQUADCOLG		( 0x8F4 / sizeof(int) )
#define L_FBC_IQUADCOLB		( 0x8F8 / sizeof(int) )
#define L_FBC_IQUADCOLA		( 0x8FC / sizeof(int) )
#define L_FBC_IRECTABSX		( 0x900 / sizeof(int) )
#define L_FBC_IRECTABSY		( 0x904 / sizeof(int) )
#define L_FBC_IRECTABSZ		( 0x908 / sizeof(int) )

	char			fill17[ 0x910 - 0x908 - 4 ];

	u_int			l_fbc_irectrelx;
	u_int			l_fbc_irectrely;
	u_int			l_fbc_irectrelz;

#define L_FBC_IRECTRELX		( 0x910 / sizeof(int) )
#define L_FBC_IRECTRELY		( 0x914 / sizeof(int) )
#define L_FBC_IRECTRELZ		( 0x918 / sizeof(int) )

	char			fill18[ 0x930 - 0x918 - 4 ];

	u_int			l_fbc_irectcolr;
	u_int			l_fbc_irectcolg;
	u_int			l_fbc_irectcolb;
	u_int			l_fbc_irectcola;

#define L_FBC_IRECTCOLR		( 0x930 / sizeof(int) )
#define L_FBC_IRECTCOLG		( 0x934 / sizeof(int) )
#define L_FBC_IRECTCOLB		( 0x938 / sizeof(int) )
#define L_FBC_IRECTCOLA		( 0x93C / sizeof(int) )

};

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
 * FBC/FHC CONFIG register
 */
#define	FHC_CONFIG_FBID_SHIFT		24
#define	FHC_CONFIG_FBID_MASK		255
#define	FHC_CONFIG_REV_SHIFT		20
#define	FHC_CONFIG_REV_MASK		15
#define	FHC_CONFIG_FROP_DISABLE		(1 << 19)
#define	FHC_CONFIG_ROW_DISABLE		(1 << 18)
#define	FHC_CONFIG_SRC_DISABLE		(1 << 17)
#define	FHC_CONFIG_DST_DISABLE		(1 << 16)
#define	FHC_CONFIG_RESET		(1 << 15)
#define	FHC_CONFIG_LITTLE_ENDIAN	(1 << 13)
#define	FHC_CONFIG_RES_MASK		(3 << 11)
#define	FHC_CONFIG_1024			(0 << 11)
#define	FHC_CONFIG_1152			(1 << 11)
#define	FHC_CONFIG_1280			(2 << 11)
#define	FHC_CONFIG_1600			(3 << 11)
#define	FHC_CONFIG_CPU_MASK		(3 << 9)
#define	FHC_CONFIG_CPU_SPARC		(0 << 9)
#define	FHC_CONFIG_CPU_68020		(1 << 9)
#define	FHC_CONFIG_CPU_386		(2 << 9)
#define	FHC_CONFIG_TEST			(1 << 8)
#define	FHC_CONFIG_TESTX_SHIFT		4
#define	FHC_CONFIG_TESTX_MASK		(15 << 4)
#define	FHC_CONFIG_TESTY_SHIFT		0
#define	FHC_CONFIG_TESTY_MASK		15

#endif cg6fbc_h
