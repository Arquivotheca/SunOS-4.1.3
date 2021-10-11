/*
 * @(#)cg6tec.h 1.3 89/03/30 SMI
 */

/*
 * Copyright 1989, Sun Microsystems, Inc.
 */

#ifndef cg6tec_DEFINED
#define cg6tec_DEFINED

/*
 * TEC register offsets from base address. These offsets are
 * intended to be added to a pointer-to-integer whose value is the
 * base address of the CG6 memory mapped register area.
 */

typedef double TEC_DATA;	/* type for DATA00 - 63 */
/* base for transform data registers */

#define L_TEC_DATA_BASE		(0x100/sizeof(u_int))
#define L_TEC_DATA00		(0x100/sizeof(u_int))
#define L_TEC_DATA01		(0x104/sizeof(u_int))
#define L_TEC_DATA02		(0x108/sizeof(u_int))
#define L_TEC_DATA03		(0x10C/sizeof(u_int))
#define L_TEC_DATA04		(0x110/sizeof(u_int))
#define L_TEC_DATA05		(0x114/sizeof(u_int))
#define L_TEC_DATA06		(0x118/sizeof(u_int))
#define L_TEC_DATA07		(0x11C/sizeof(u_int))
#define L_TEC_DATA08		(0x120/sizeof(u_int))
#define L_TEC_DATA09		(0x124/sizeof(u_int))
#define L_TEC_DATA10		(0x128/sizeof(u_int))
#define L_TEC_DATA11		(0x12C/sizeof(u_int))
#define L_TEC_DATA12		(0x130/sizeof(u_int))
#define L_TEC_DATA13		(0x134/sizeof(u_int))
#define L_TEC_DATA14		(0x138/sizeof(u_int))
#define L_TEC_DATA15		(0x13C/sizeof(u_int))
#define L_TEC_DATA16		(0x140/sizeof(u_int))
#define L_TEC_DATA17		(0x144/sizeof(u_int))
#define L_TEC_DATA18		(0x148/sizeof(u_int))
#define L_TEC_DATA19		(0x14C/sizeof(u_int))
#define L_TEC_DATA20		(0x150/sizeof(u_int))
#define L_TEC_DATA21		(0x154/sizeof(u_int))
#define L_TEC_DATA22		(0x158/sizeof(u_int))
#define L_TEC_DATA23		(0x15C/sizeof(u_int))
#define L_TEC_DATA24		(0x160/sizeof(u_int))
#define L_TEC_DATA25		(0x164/sizeof(u_int))
#define L_TEC_DATA26		(0x168/sizeof(u_int))
#define L_TEC_DATA27		(0x16C/sizeof(u_int))
#define L_TEC_DATA28		(0x170/sizeof(u_int))
#define L_TEC_DATA29		(0x174/sizeof(u_int))
#define L_TEC_DATA30		(0x178/sizeof(u_int))
#define L_TEC_DATA31		(0x17C/sizeof(u_int))
#define L_TEC_DATA32		(0x180/sizeof(u_int))
#define L_TEC_DATA33		(0x184/sizeof(u_int))
#define L_TEC_DATA34		(0x188/sizeof(u_int))
#define L_TEC_DATA35		(0x18C/sizeof(u_int))
#define L_TEC_DATA36		(0x190/sizeof(u_int))
#define L_TEC_DATA37		(0x194/sizeof(u_int))
#define L_TEC_DATA38		(0x198/sizeof(u_int))
#define L_TEC_DATA39		(0x19C/sizeof(u_int))
#define L_TEC_DATA40		(0x1A0/sizeof(u_int))
#define L_TEC_DATA41		(0x1A4/sizeof(u_int))
#define L_TEC_DATA42		(0x1A8/sizeof(u_int))
#define L_TEC_DATA43		(0x1AC/sizeof(u_int))
#define L_TEC_DATA44		(0x1B0/sizeof(u_int))
#define L_TEC_DATA45		(0x1B4/sizeof(u_int))
#define L_TEC_DATA46		(0x1B8/sizeof(u_int))
#define L_TEC_DATA47		(0x1BC/sizeof(u_int))
#define L_TEC_DATA48		(0x1C0/sizeof(u_int))
#define L_TEC_DATA49		(0x1C4/sizeof(u_int))
#define L_TEC_DATA50		(0x1C8/sizeof(u_int))
#define L_TEC_DATA51		(0x1CC/sizeof(u_int))
#define L_TEC_DATA52		(0x1D0/sizeof(u_int))
#define L_TEC_DATA53		(0x1D4/sizeof(u_int))
#define L_TEC_DATA54		(0x1D8/sizeof(u_int))
#define L_TEC_DATA55		(0x1DC/sizeof(u_int))
#define L_TEC_DATA56		(0x1E0/sizeof(u_int))
#define L_TEC_DATA57		(0x1E4/sizeof(u_int))
#define L_TEC_DATA58		(0x1E8/sizeof(u_int))
#define L_TEC_DATA59		(0x1EC/sizeof(u_int))
#define L_TEC_DATA60		(0x1F0/sizeof(u_int))
#define L_TEC_DATA61		(0x1F4/sizeof(u_int))
#define L_TEC_DATA62		(0x1F8/sizeof(u_int))
#define L_TEC_DATA63		(0x1FC/sizeof(u_int))


/* matrix registers */

#define L_TEC_MV_MATRIX		(0x000/sizeof(u_int))
#define L_TEC_CLIPCHECK		(0x004/sizeof(u_int))
#define L_TEC_VDC_MATRIX	(0x008/sizeof(u_int))


/* command register */

#define L_TEC_COMMAND1		(0x0010/sizeof(u_int))
#define L_TEC_COMMAND2		(0x0014/sizeof(u_int))
#define L_TEC_COMMAND3		(0x0018/sizeof(u_int))
#define L_TEC_COMMAND4		(0x001c/sizeof(u_int))


/* u_integer indexed address registers */

#define L_TEC_IPOINTABSX	( 0x800 / sizeof(u_int) )
#define L_TEC_IPOINTABSY	( 0x804 / sizeof(u_int) )
#define L_TEC_IPOINTABSZ	( 0x808 / sizeof(u_int) )
#define L_TEC_IPOINTABSW	( 0x80C / sizeof(u_int) )
#define L_TEC_IPOINTRELX	( 0x810 / sizeof(u_int) )
#define L_TEC_IPOINTRELY	( 0x814 / sizeof(u_int) )
#define L_TEC_IPOINTRELZ	( 0x818 / sizeof(u_int) )
#define L_TEC_IPOINTRELW	( 0x81C / sizeof(u_int) )

#define L_TEC_ILINEABSX		( 0x840 / sizeof(u_int) )
#define L_TEC_ILINEABSY		( 0x844 / sizeof(u_int) )
#define L_TEC_ILINEABSZ		( 0x848 / sizeof(u_int) )
#define L_TEC_ILINEABSW		( 0x84C / sizeof(u_int) )
#define L_TEC_ILINERELX		( 0x850 / sizeof(u_int) )
#define L_TEC_ILINERELY		( 0x854 / sizeof(u_int) )
#define L_TEC_ILINERELZ		( 0x858 / sizeof(u_int) )
#define L_TEC_ILINERELW		( 0x85C / sizeof(u_int) )

#define L_TEC_ITRIABSX		( 0x880 / sizeof(u_int) )
#define L_TEC_ITRIABSY		( 0x884 / sizeof(u_int) )
#define L_TEC_ITRIABSZ		( 0x888 / sizeof(u_int) )
#define L_TEC_ITRIABSW		( 0x88C / sizeof(u_int) )
#define L_TEC_ITRIRELX		( 0x890 / sizeof(u_int) )
#define L_TEC_ITRIRELY		( 0x894 / sizeof(u_int) )
#define L_TEC_ITRIRELZ		( 0x898 / sizeof(u_int) )
#define L_TEC_ITRIRELW		( 0x89C / sizeof(u_int) )

#define L_TEC_IQUADABSX		( 0x8C0 / sizeof(u_int) )
#define L_TEC_IQUADABSY		( 0x8C4 / sizeof(u_int) )
#define L_TEC_IQUADABSZ		( 0x8C8 / sizeof(u_int) )
#define L_TEC_IQUADABSW		( 0x8CC / sizeof(u_int) )
#define L_TEC_IQUADRELX		( 0x8D0 / sizeof(u_int) )
#define L_TEC_IQUADRELY		( 0x8D4 / sizeof(u_int) )
#define L_TEC_IQUADRELZ		( 0x8D8 / sizeof(u_int) )
#define L_TEC_IQUADRELW		( 0x8DC / sizeof(u_int) )

#define L_TEC_IRECTABSX		( 0x900 / sizeof(u_int) )
#define L_TEC_IRECTABSY		( 0x904 / sizeof(u_int) )
#define L_TEC_IRECTABSZ		( 0x908 / sizeof(u_int) )
#define L_TEC_IRECTABSW		( 0x90C / sizeof(u_int) )
#define L_TEC_IRECTRELX		( 0x910 / sizeof(u_int) )
#define L_TEC_IRECTRELY		( 0x914 / sizeof(u_int) )
#define L_TEC_IRECTRELZ		( 0x918 / sizeof(u_int) )
#define L_TEC_IRECTRELW		( 0x91C / sizeof(u_int) )

/* fixed pou_int indexed address registers */

#define L_TEC_BPOINTABSX	( 0xA00 / sizeof(u_int) )
#define L_TEC_BPOINTABSY	( 0xA04 / sizeof(u_int) )
#define L_TEC_BPOINTABSZ	( 0xA08 / sizeof(u_int) )
#define L_TEC_BPOINTABSW	( 0xA0C / sizeof(u_int) )
#define L_TEC_BPOINTRELX	( 0xA10 / sizeof(u_int) )
#define L_TEC_BPOINTRELY	( 0xA14 / sizeof(u_int) )
#define L_TEC_BPOINTRELZ	( 0xA18 / sizeof(u_int) )
#define L_TEC_BPOINTRELW	( 0xA1C / sizeof(u_int) )

#define L_TEC_BLINEABSX		( 0xA40 / sizeof(u_int) )
#define L_TEC_BLINEABSY		( 0xA44 / sizeof(u_int) )
#define L_TEC_BLINEABSZ		( 0xA48 / sizeof(u_int) )
#define L_TEC_BLINEABSW		( 0xA4C / sizeof(u_int) )
#define L_TEC_BLINERELX		( 0xA50 / sizeof(u_int) )
#define L_TEC_BLINERELY		( 0xA54 / sizeof(u_int) )
#define L_TEC_BLINERELZ		( 0xA58 / sizeof(u_int) )
#define L_TEC_BLINERELW		( 0xA5C / sizeof(u_int) )

#define L_TEC_BTRIABSX		( 0xA80 / sizeof(u_int) )
#define L_TEC_BTRIABSY		( 0xA84 / sizeof(u_int) )
#define L_TEC_BTRIABSZ		( 0xA88 / sizeof(u_int) )
#define L_TEC_BTRIABSW		( 0xA8C / sizeof(u_int) )
#define L_TEC_BTRIRELX		( 0xA90 / sizeof(u_int) )
#define L_TEC_BTRIRELY		( 0xA94 / sizeof(u_int) )
#define L_TEC_BTRIRELZ		( 0xA98 / sizeof(u_int) )
#define L_TEC_BTRIRELW		( 0xA9C / sizeof(u_int) )

#define L_TEC_BQUADABSX		( 0xAC0 / sizeof(u_int) )
#define L_TEC_BQUADABSY		( 0xAC4 / sizeof(u_int) )
#define L_TEC_BQUADABSZ		( 0xAC8 / sizeof(u_int) )
#define L_TEC_BQUADABSW		( 0xACC / sizeof(u_int) )
#define L_TEC_BQUADRELX		( 0xAD0 / sizeof(u_int) )
#define L_TEC_BQUADRELY		( 0xAD4 / sizeof(u_int) )
#define L_TEC_BQUADRELZ		( 0xAD8 / sizeof(u_int) )
#define L_TEC_BQUADRELW		( 0xADC / sizeof(u_int) )

#define L_TEC_BRECTABSX		( 0xB00 / sizeof(u_int) )
#define L_TEC_BRECTABSY		( 0xB04 / sizeof(u_int) )
#define L_TEC_BRECTABSZ		( 0xB08 / sizeof(u_int) )
#define L_TEC_BRECTABSW		( 0xB0C / sizeof(u_int) )
#define L_TEC_BRECTRELX		( 0xB10 / sizeof(u_int) )
#define L_TEC_BRECTRELY		( 0xB14 / sizeof(u_int) )
#define L_TEC_BRECTRELZ		( 0xB18 / sizeof(u_int) )
#define L_TEC_BRECTRELW		( 0xB1C / sizeof(u_int) )

/* floating pou_int indexed address registers */

#define L_TEC_FPOINTABSX	( 0xC00 / sizeof(u_int) )
#define L_TEC_FPOINTABSY	( 0xC04 / sizeof(u_int) )
#define L_TEC_FPOINTABSZ	( 0xC08 / sizeof(u_int) )
#define L_TEC_FPOINTABSW	( 0xC0C / sizeof(u_int) )
#define L_TEC_FPOINTRELX	( 0xC10 / sizeof(u_int) )
#define L_TEC_FPOINTRELY	( 0xC14 / sizeof(u_int) )
#define L_TEC_FPOINTRELZ	( 0xC18 / sizeof(u_int) )
#define L_TEC_FPOINTRELW	( 0xC1C / sizeof(u_int) )

#define L_TEC_FLINEABSX		( 0xC40 / sizeof(u_int) )
#define L_TEC_FLINEABSY		( 0xC44 / sizeof(u_int) )
#define L_TEC_FLINEABSZ		( 0xC48 / sizeof(u_int) )
#define L_TEC_FLINEABSW		( 0xC4C / sizeof(u_int) )
#define L_TEC_FLINERELX		( 0xC50 / sizeof(u_int) )
#define L_TEC_FLINERELY		( 0xC54 / sizeof(u_int) )
#define L_TEC_FLINERELZ		( 0xC58 / sizeof(u_int) )
#define L_TEC_FLINERELW		( 0xC5C / sizeof(u_int) )

#define L_TEC_FTRIABSX		( 0xC80 / sizeof(u_int) )
#define L_TEC_FTRIABSY		( 0xC84 / sizeof(u_int) )
#define L_TEC_FTRIABSZ		( 0xC88 / sizeof(u_int) )
#define L_TEC_FTRIABSW		( 0xC8C / sizeof(u_int) )
#define L_TEC_FTRIRELX		( 0xC90 / sizeof(u_int) )
#define L_TEC_FTRIRELY		( 0xC94 / sizeof(u_int) )
#define L_TEC_FTRIRELZ		( 0xC98 / sizeof(u_int) )
#define L_TEC_FTRIRELW		( 0xC9C / sizeof(u_int) )

#define L_TEC_FQUADABSX		( 0xCC0 / sizeof(u_int) )
#define L_TEC_FQUADABSY		( 0xCC4 / sizeof(u_int) )
#define L_TEC_FQUADABSZ		( 0xCC8 / sizeof(u_int) )
#define L_TEC_FQUADABSW		( 0xCCC / sizeof(u_int) )
#define L_TEC_FQUADRELX		( 0xCD0 / sizeof(u_int) )
#define L_TEC_FQUADRELY		( 0xCD4 / sizeof(u_int) )
#define L_TEC_FQUADRELZ		( 0xCD8 / sizeof(u_int) )
#define L_TEC_FQUADRELW		( 0xCDC / sizeof(u_int) )

#define L_TEC_FRECTABSX		( 0xD00 / sizeof(u_int) )
#define L_TEC_FRECTABSY		( 0xD04 / sizeof(u_int) )
#define L_TEC_FRECTABSZ		( 0xD08 / sizeof(u_int) )
#define L_TEC_FRECTABSW		( 0xD0C / sizeof(u_int) )
#define L_TEC_FRECTRELX		( 0xD10 / sizeof(u_int) )
#define L_TEC_FRECTRELY		( 0xD14 / sizeof(u_int) )
#define L_TEC_FRECTRELZ		( 0xD18 / sizeof(u_int) )
#define L_TEC_FRECTRELW		( 0xD1C / sizeof(u_int) )

/* 
 * Generic unsigned types for bit fields
 */
typedef enum {
  	L_TEC_UNUSED = 0
} l_tec_unused_t;

/* 
 * typedefs for entering index registers
 */
typedef enum {		/* coordinate type of {pou_int,line,etc} */
	L_TEC_X = 0,
	L_TEC_Y = 1,
	L_TEC_Z = 2,
	L_TEC_W = 3,	 /* used as index to store index writes in tec_data array*/
  	L_TEC_X_MV = 4, 
	L_TEC_Y_MV = 5, 
	L_TEC_Z_MV = 6, 
	L_TEC_W_MV = 7, /* used as index to store xform results */
  	L_TEC_X_VDC = 8, 
	L_TEC_Y_VDC = 9, 
	L_TEC_Z_VDC = 10, /* used as index to store VDC multiply results */
  	L_TEC_SCRATCH = 11		/* scratch register */
} l_tec_coord_t;

typedef enum {			/* index coordinate type */
  	L_TEC_INT = 0, 
	L_TEC_FRAC = 1, 
	L_TEC_FLOAT = 2
} l_tec_type_t;

typedef enum {			/* index object type */
  	L_TEC_POINT = 0, 
	L_TEC_LINE = 1, 
	L_TEC_TRI = 2, 
	L_TEC_QUAD = 3, 
	L_TEC_RECT = 4
} l_tec_object_t;

typedef enum {
  	L_TEC_ABS = 0, 
	L_TEC_REL = 1
} l_tec_mode_t;


/*
 * TEC MV_MATRIX register bits.
 */

typedef enum {
  	L_TEC_MV_DIV_OFF = 0, 
	L_TEC_MV_DIV_ON = 1
} l_tec_mv_div_t;

typedef enum {
  	L_TEC_MV_AUTO_OFF = 0, 
	L_TEC_MV_AUTO_ON = 1
} l_tec_mv_auto_t;
 
typedef enum {
  	L_TEC_MV_H_FALSE = 0, 
	L_TEC_MV_H_TRUE = 1
} l_tec_mv_h_t;

typedef enum {
  	L_TEC_MV_Z_FALSE = 0, L_TEC_MV_Z_TRUE = 1
  } l_tec_mv_z_t;

typedef enum {
  L_TEC_MV_I3 = 0, L_TEC_MV_I4 = 1
  } l_tec_mv_i_t;

typedef enum {
  L_TEC_MV_J1 = 0, L_TEC_MV_J2 = 1, L_TEC_MV_J3 = 2, L_TEC_MV_J4 = 3
  } l_tec_mv_j_t;

typedef struct l_tec_mv {
  l_tec_unused_t 	l_tec_mv_unused1	:16;	/* NOT USED */
  l_tec_mv_div_t	l_tec_mv_div 		: 1;	/* divide enable */
  l_tec_mv_auto_t	l_tec_mv_autoload 	: 1;	/* autoload enable */
  l_tec_mv_h_t		l_tec_mv_h 		: 1;	/* pou_int size */
  l_tec_mv_z_t		l_tec_mv_z 		: 1;	/* pou_int size */
  l_tec_unused_t	l_tec_mv_unused2	: 1;	/* NOT USED */
  l_tec_mv_i_t		l_tec_mv_i 		: 1;	/* matrix rows */
  l_tec_mv_j_t		l_tec_mv_j 		: 2;	/* matrix columns */
  l_tec_unused_t	l_tec_mv_unused3	: 2;	/* NOT USED */
  unsigned		l_tec_mv_index 		: 6;	/* matrix start data reg. */
} l_tec_mv_t;



/*
 * TEC CLIPCHECK bits.
 */

typedef enum { 
  L_TEC_EXCEPTION_OFF = 0, L_TEC_EXCEPTION_ON= 1
  } l_tec_clip_exception_t;

typedef enum {
  L_TEC_HIDDEN_OFF = 0, L_TEC_HIDDEN_ON = 1 
  } l_tec_clip_hidden_t;

typedef enum {
  L_TEC_INTERSECT_OFF = 0, L_TEC_INTERSECT_ON = 1 
  } l_tec_clip_u_intersect_t;

typedef enum {
  L_TEC_VISIBLE_OFF = 0, L_TEC_VISIBLE_ON = 1 
  } l_tec_clip_visible_t;

typedef enum {
  L_TEC_ACC_BACKSIDE_FALSE = 0,   L_TEC_ACC_BACKSIDE_TRUE = 1
  } l_tec_clip_acc_backside_t;

typedef enum {
  L_TEC_ACC_LT_FALSE = 0, L_TEC_ACC_LT_TRUE = 1 
  } l_tec_clip_acc_lt_t;

typedef enum {
  L_TEC_ACC_INSIDE_FALSE = 0, L_TEC_ACC_INSIDE_TRUE = 1 
  } l_tec_clip_acc_inside_t;

typedef enum {
  L_TEC_ACC_GT_FALSE = 0, L_TEC_ACC_GT_TRUE = 1 
  } l_tec_clip_acc_gt_t;

typedef enum {
  L_TEC_LT_FALSE = 0, L_TEC_LT_TRUE = 1 
  } l_tec_clip_lt_t;

typedef enum {
  L_TEC_GT_FALSE = 0, L_TEC_GT_TRUE = 1 
  } l_tec_clip_gt_t;

typedef enum {
  L_TEC_CLIP_OFF = 0, L_TEC_CLIP_ON = 1 
  } l_tec_clip_enable_t;

typedef struct l_tec_clip {		/* big assumption here is compiler assigns bit fields left to right */
  	l_tec_clip_exception_t		l_tec_clip_exception		: 1;
  	l_tec_clip_hidden_t		l_tec_clip_hidden		: 1;
  	l_tec_clip_u_intersect_t		l_tec_clip_intersect		: 1;
  	l_tec_clip_visible_t		l_tec_clip_visible		: 1;
	l_tec_unused_t			l_tec_clip_unused1		: 3;		/* unused */
	l_tec_clip_enable_t		l_tec_clip_enable		: 1;
	
  	l_tec_clip_acc_backside_t	l_tec_clip_acc_z_backside	: 1;
  	l_tec_clip_acc_lt_t		l_tec_clip_acc_z_lt_front	: 1;
  	l_tec_clip_acc_inside_t		l_tec_clip_acc_z_inside		: 1;
  	l_tec_clip_acc_gt_t		l_tec_clip_acc_z_gt_back	: 1;
 	l_tec_clip_lt_t			l_tec_clip_z_lt_front		: 1;
  	l_tec_clip_gt_t			l_tec_clip_z_gt_back		: 1;
  	l_tec_clip_enable_t		l_tec_clip_front		: 1;
  	l_tec_clip_enable_t		l_tec_clip_back			: 1;

  	l_tec_clip_acc_backside_t	l_tec_clip_acc_y_backside	: 1;
  	l_tec_clip_acc_lt_t		l_tec_clip_acc_y_lt_bottom	: 1;
  	l_tec_clip_acc_inside_t		l_tec_clip_acc_y_inside		: 1;
  	l_tec_clip_acc_gt_t		l_tec_clip_acc_y_gt_top		: 1;
  	l_tec_clip_lt_t			l_tec_clip_y_lt_bottom		: 1;
  	l_tec_clip_gt_t			l_tec_clip_y_gt_top		: 1;
  	l_tec_clip_enable_t		l_tec_clip_bottom		: 1;
  	l_tec_clip_enable_t		l_tec_clip_top			: 1;

  	l_tec_clip_acc_backside_t	l_tec_clip_acc_x_backside	: 1;
  	l_tec_clip_acc_lt_t		l_tec_clip_acc_x_lt_left	: 1;
  	l_tec_clip_acc_inside_t		l_tec_clip_acc_x_inside		: 1;
  	l_tec_clip_acc_gt_t		l_tec_clip_acc_x_gt_right	: 1;
  	l_tec_clip_lt_t			l_tec_clip_x_lt_left		: 1;
  	l_tec_clip_gt_t			l_tec_clip_x_gt_right		: 1;
  	l_tec_clip_enable_t		l_tec_clip_left			: 1;
  	l_tec_clip_enable_t		l_tec_clip_right		: 1;
} l_tec_clip_t;


/*
 * TEC VDC_MATRIX register bits.
 */

typedef enum {
  L_TEC_VDC_INT = 0 , L_TEC_VDC_FIXED = 1 , L_TEC_VDC_FLOAT = 2,
  L_TEC_VDC_INTRNL0 = 4 , L_TEC_VDC_INTRNL1 = 5,
}l_tec_vdc_type_t;

typedef enum {
	L_TEC_VDC_2D = 0, L_TEC_VDC_3D = 1
	}l_tec_vdc_k_t;

typedef enum {
	L_TEC_VDC_MUL_OFF = 0 , L_TEC_VDC_MUL_ON = 1
	}l_tec_vdc_mul_t;


typedef struct l_tec_vdc {
	l_tec_unused_t		l_tec_vdc_unused1	: 16;			/* NOT USED */
	l_tec_vdc_type_t 	l_tec_vdc_type 		: 3; /* register access type */
	l_tec_vdc_mul_t		l_tec_vdc_mul 		: 1; /* enable VDC multiply */
	l_tec_unused_t		l_tec_vdc_unused2	: 3;			/* NOT USED */
	l_tec_vdc_k_t		l_tec_vdc_k 		: 1; /* 2D/3D format */
	l_tec_unused_t		l_tec_vdc_unused3	: 2;			/* NOT USED */
	unsigned		l_tec_vdc_index 	: 6; /* matrix start data reg. */
      } l_tec_vdc_t;

/*
 * TEC COMMAND register bits.
 */

typedef enum {
	L_TEC_CMD_M1 = 0, 
	L_TEC_CMD_M2 = 1, 
	L_TEC_CMD_M3 = 2, 
	L_TEC_CMD_M4 = 3
}l_tec_cmd_m_t;

typedef enum {
	L_TEC_CMD_N1 = 0, 
	L_TEC_CMD_N2 = 1, 
	L_TEC_CMD_N3 = 2, 
	L_TEC_CMD_N4 = 3
}l_tec_cmd_n_t;

typedef enum {
	L_TEC_CMD_I_POS = 0, 
	L_TEC_CMD_I_NEG = 1
}l_tec_cmd_i_t;

typedef struct l_tec_cmd {
	l_tec_cmd_m_t		l_tec_cmd_m : 2;	/* matrix A columns */
	l_tec_cmd_n_t		l_tec_cmd_n : 2;	/* matrix B columns */
	l_tec_cmd_i_t		l_tec_cmd_i11 : 1;	/* identity diagonal sign */
	l_tec_cmd_i_t		l_tec_cmd_i22 : 1;	/* identity diagonal sign */
	l_tec_cmd_i_t		l_tec_cmd_i33 : 1;	/* identity diagonal sign */
	l_tec_cmd_i_t		l_tec_cmd_i44 : 1;	/* identity diagonal sign */
	l_tec_unused_t		l_tec_cmd_unused1 : 2;			/* NOT USED */
	unsigned		l_tec_cmd_Aindex : 6;	/* A matrix start data reg. */
	l_tec_unused_t		l_tec_cmd_unused2 : 2;			/* NOT USED */
	unsigned		l_tec_cmd_Bindex : 6;	/* B matrix start data reg. */
	l_tec_unused_t		l_tec_cmd_unused3 : 2;			/* NOT USED */
	unsigned		l_tec_cmd_Cindex : 6;	/* C matrix start data reg. */
} l_tec_cmd_t;

/*
 * TEC registers defined as a structure.
 */
struct tec {
#ifdef tec_structures
	struct l_tec_mv		l_tec_mv;		/* 0 */
	struct l_tec_clip	l_tec_clip;		/* 1 */
	struct l_tec_vdc	l_tec_vdc;		/* 2 */
	u_int	l_tec_pad_3[4-3];
	struct l_tec_cmd	l_tec_command1;		/* 4 */
	struct l_tec_cmd	l_tec_command2;		/* 5 */
	struct l_tec_cmd	l_tec_command3;		/* 6 */
	struct l_tec_cmd	l_tec_command4;		/* 7 */
#else
	u_int 	l_tec_mv;				/* 0 */
	u_int 	l_tec_clip;				/* 1 */
	u_int 	l_tec_vdc;				/* 2 */
	u_int	l_tec_pad_3[4-3];
	u_int 	l_tec_command1;				/* 4 */
	u_int 	l_tec_command2;				/* 5 */
	u_int 	l_tec_command3;				/* 6 */
	u_int 	l_tec_command4;				/* 7 */
#endif
	u_int	l_tec_pad_8[64-8];
	u_int	l_tec_data00;				/* 64 */
	u_int	l_tec_data01;				/* 65 */
	u_int	l_tec_data02;				/* 66 */
	u_int	l_tec_data03;				/* 67 */
	u_int	l_tec_data04;				/* 68 */
	u_int	l_tec_data05;				/* 69 */
	u_int	l_tec_data06;				/* 70 */
	u_int	l_tec_data07;				/* 71 */
	u_int	l_tec_data08;				/* 72 */
	u_int	l_tec_data09;				/* 73 */
	u_int	l_tec_data10;				/* 74 */
	u_int	l_tec_data11;				/* 75 */
	u_int	l_tec_data12;				/* 76 */
	u_int	l_tec_data13;				/* 77 */
	u_int	l_tec_data14;				/* 78 */
	u_int	l_tec_data15;				/* 79 */
	u_int	l_tec_data16;				/* 80 */
	u_int	l_tec_data17;				/* 81 */
	u_int	l_tec_data18;				/* 82 */
	u_int	l_tec_data19;				/* 83 */
	u_int	l_tec_data20;				/* 84 */
	u_int	l_tec_data21;				/* 85 */
	u_int	l_tec_data22;				/* 86 */
	u_int	l_tec_data23;				/* 87 */
	u_int	l_tec_data24;				/* 88 */
	u_int	l_tec_data25;				/* 89 */
	u_int	l_tec_data26;				/* 90 */
	u_int	l_tec_data27;				/* 91 */
	u_int	l_tec_data28;				/* 92 */
	u_int	l_tec_data29;				/* 93 */
	u_int	l_tec_data30;				/* 94 */
	u_int	l_tec_data31;				/* 95 */
	u_int	l_tec_data32;				/* 96 */
	u_int	l_tec_data33;				/* 97 */
	u_int	l_tec_data34;				/* 98 */
	u_int	l_tec_data35;				/* 99 */
	u_int	l_tec_data36;				/* 100 */
	u_int	l_tec_data37;				/* 101 */
	u_int	l_tec_data38;				/* 102 */
	u_int	l_tec_data39;				/* 103 */
	u_int	l_tec_data40;				/* 104 */
	u_int	l_tec_data41;				/* 105 */
	u_int	l_tec_data42;				/* 106 */
	u_int	l_tec_data43;				/* 107 */
	u_int	l_tec_data44;				/* 108 */
	u_int	l_tec_data45;				/* 109 */
	u_int	l_tec_data46;				/* 110 */
	u_int	l_tec_data47;				/* 111 */
	u_int	l_tec_data48;				/* 112 */
	u_int	l_tec_data49;				/* 113 */
	u_int	l_tec_data50;				/* 114 */
	u_int	l_tec_data51;				/* 115 */
	u_int	l_tec_data52;				/* 116 */
	u_int	l_tec_data53;				/* 117 */
	u_int	l_tec_data54;				/* 118 */
	u_int	l_tec_data55;				/* 119 */
	u_int	l_tec_data56;				/* 120 */
	u_int	l_tec_data57;				/* 121 */
	u_int	l_tec_data58;				/* 122 */
	u_int	l_tec_data59;				/* 123 */
	u_int	l_tec_data60;				/* 124 */
	u_int	l_tec_data61;				/* 125 */
	u_int	l_tec_data62;				/* 126 */
	u_int	l_tec_data63;				/* 127 */
	u_int	l_tec_pad_128[512-128];
	u_int	l_tec_ipointabsx;			/* 512 */
	u_int	l_tec_ipointabsy;			/* 513 */
	u_int	l_tec_ipointabsz;			/* 514 */
	u_int	l_tec_ipointabsw;			/* 515 */
	u_int	l_tec_ipointrelx;			/* 516 */
	u_int	l_tec_ipointrely;			/* 517 */
	u_int	l_tec_ipointrelz;			/* 518 */
	u_int	l_tec_ipointrelw;			/* 519 */
	u_int	l_tec_pad_520[528-520];
	u_int	l_tec_ilineabsx;			/* 528 */
	u_int	l_tec_ilineabsy;			/* 529 */
	u_int	l_tec_ilineabsz;			/* 530 */
	u_int	l_tec_ilineabsw;			/* 531 */
	u_int	l_tec_ilinerelx;			/* 532 */
	u_int	l_tec_ilinerely;			/* 533 */
	u_int	l_tec_ilinerelz;			/* 534 */
	u_int	l_tec_ilinerelw;			/* 535 */
	u_int	l_tec_pad_536[544-536];
	u_int	l_tec_itriabsx;				/* 544 */
	u_int	l_tec_itriabsy;				/* 545 */
	u_int	l_tec_itriabsz;				/* 546 */
	u_int	l_tec_itriabsw;				/* 547 */
	u_int	l_tec_itrirelx;				/* 548 */
	u_int	l_tec_itrirely;				/* 549 */
	u_int	l_tec_itrirelz;				/* 550 */
	u_int	l_tec_itrirelw;				/* 551 */
	u_int	l_tec_pad_552[560-552];
	u_int	l_tec_iquadabsx;			/* 560 */
	u_int	l_tec_iquadabsy;			/* 561 */
	u_int	l_tec_iquadabsz;			/* 562 */
	u_int	l_tec_iquadabsw;			/* 563 */
	u_int	l_tec_iquadrelx;			/* 564 */
	u_int	l_tec_iquadrely;			/* 565 */
	u_int	l_tec_iquadrelz;			/* 566 */
	u_int	l_tec_iquadrelw;			/* 567 */
	u_int	l_tec_pad_568[576-568];
	u_int	l_tec_irectabsx;			/* 576 */
	u_int	l_tec_irectabsy;			/* 577 */
	u_int	l_tec_irectabsz;			/* 578 */
	u_int	l_tec_irectabsw;			/* 579 */
	u_int	l_tec_irectrelx;			/* 580 */
	u_int	l_tec_irectrely;			/* 581 */
	u_int	l_tec_irectrelz;			/* 582 */
	u_int	l_tec_irectrelw;			/* 583 */
	u_int	l_tec_pad_584[640-584];
	u_int	l_tec_bpointabsx;			/* 640 */
	u_int	l_tec_bpointabsy;			/* 641 */
	u_int	l_tec_bpointabsz;			/* 642 */
	u_int	l_tec_bpointabsw;			/* 643 */
	u_int	l_tec_bpointrelx;			/* 644 */
	u_int	l_tec_bpointrely;			/* 645 */
	u_int	l_tec_bpointrelz;			/* 646 */
	u_int	l_tec_bpointrelw;			/* 647 */
	u_int	l_tec_pad_648[656-648];
	u_int	l_tec_blineabsx;			/* 656 */
	u_int	l_tec_blineabsy;			/* 657 */
	u_int	l_tec_blineabsz;			/* 658 */
	u_int	l_tec_blineabsw;			/* 659 */
	u_int	l_tec_blinerelx;			/* 660 */
	u_int	l_tec_blinerely;			/* 661 */
	u_int	l_tec_blinerelz;			/* 662 */
	u_int	l_tec_blinerelw;			/* 663 */
	u_int	l_tec_pad_664[672-664];
	u_int	l_tec_btriabsx;				/* 672 */
	u_int	l_tec_btriabsy;				/* 673 */
	u_int	l_tec_btriabsz;				/* 674 */
	u_int	l_tec_btriabsw;				/* 675 */
	u_int	l_tec_btrirelx;				/* 676 */
	u_int	l_tec_btrirely;				/* 677 */
	u_int	l_tec_btrirelz;				/* 678 */
	u_int	l_tec_btrirelw;				/* 679 */
	u_int	l_tec_pad_680[688-680];
	u_int	l_tec_bquadabsx;			/* 688 */
	u_int	l_tec_bquadabsy;			/* 689 */
	u_int	l_tec_bquadabsz;			/* 690 */
	u_int	l_tec_bquadabsw;			/* 691 */
	u_int	l_tec_bquadrelx;			/* 692 */
	u_int	l_tec_bquadrely;			/* 693 */
	u_int	l_tec_bquadrelz;			/* 694 */
	u_int	l_tec_bquadrelw;			/* 695 */
	u_int	l_tec_pad_696[704-696];
	u_int	l_tec_brectabsx;			/* 704 */
	u_int	l_tec_brectabsy;			/* 705 */
	u_int	l_tec_brectabsz;			/* 706 */
	u_int	l_tec_brectabsw;			/* 707 */
	u_int	l_tec_brectrelx;			/* 708 */
	u_int	l_tec_brectrely;			/* 709 */
	u_int	l_tec_brectrelz;			/* 710 */
	u_int	l_tec_brectrelw;			/* 711 */
	u_int	l_tec_pad_712[768-712];
	u_int	l_tec_fpointabsx;			/* 768 */
	u_int	l_tec_fpointabsy;			/* 769 */
	u_int	l_tec_fpointabsz;			/* 770 */
	u_int	l_tec_fpointabsw;			/* 771 */
	u_int	l_tec_fpointrelx;			/* 772 */
	u_int	l_tec_fpointrely;			/* 773 */
	u_int	l_tec_fpointrelz;			/* 774 */
	u_int	l_tec_fpointrelw;			/* 775 */
	u_int	l_tec_pad_776[784-776];
	u_int	l_tec_flineabsx;			/* 784 */
	u_int	l_tec_flineabsy;			/* 785 */
	u_int	l_tec_flineabsz;			/* 786 */
	u_int	l_tec_flineabsw;			/* 787 */
	u_int	l_tec_flinerelx;			/* 788 */
	u_int	l_tec_flinerely;			/* 789 */
	u_int	l_tec_flinerelz;			/* 790 */
	u_int	l_tec_flinerelw;			/* 791 */
	u_int	l_tec_pad_792[800-792];
	u_int	l_tec_ftriabsx;				/* 800 */
	u_int	l_tec_ftriabsy;				/* 801 */
	u_int	l_tec_ftriabsz;				/* 802 */
	u_int	l_tec_ftriabsw;				/* 803 */
	u_int	l_tec_ftrirelx;				/* 804 */
	u_int	l_tec_ftrirely;				/* 805 */
	u_int	l_tec_ftrirelz;				/* 806 */
	u_int	l_tec_ftrirelw;				/* 807 */
	u_int	l_tec_pad_808[816-808];
	u_int	l_tec_fquadabsx;			/* 816 */
	u_int	l_tec_fquadabsy;			/* 817 */
	u_int	l_tec_fquadabsz;			/* 818 */
	u_int	l_tec_fquadabsw;			/* 819 */
	u_int	l_tec_fquadrelx;			/* 820 */
	u_int	l_tec_fquadrely;			/* 821 */
	u_int	l_tec_fquadrelz;			/* 822 */
	u_int	l_tec_fquadrelw;			/* 823 */
	u_int	l_tec_pad_824[832-824];
	u_int	l_tec_frectabsx;			/* 832 */
	u_int	l_tec_frectabsy;			/* 833 */
	u_int	l_tec_frectabsz;			/* 834 */
	u_int	l_tec_frectabsw;			/* 835 */
	u_int	l_tec_frectrelx;			/* 836 */
	u_int	l_tec_frectrely;			/* 837 */
	u_int	l_tec_frectrelz;			/* 838 */
	u_int	l_tec_frectrelw;			/* 839 */
};

#define NUM_TEC_REGS	(sizeof(struct l_tec)/sizeof(u_int))

#endif !cg6tec_DEFINED
