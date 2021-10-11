/*
 * Copyright (c) 1986, 1987, 1988, 1989 by Sun Microsystems, Inc.
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both that copyright
 * notice and this permission notice are retained, and that the name
 * of Sun Microsystems, Inc., not be used in advertising or publicity
 * pertaining to this software without specific, written prior permission.
 * Sun Microsystems, Inc., makes no representations about the suitability
 * of this software or the interface defined in this software for any
 * purpose. It is provided "as is" without express or implied warranty.
 */
/*	@(#)usercoredd.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
#define TRUE		1
#define FALSE		0
#define STRING		0
#define CHARACTER	1
#define MAXVSURF	5	/* view surfaces; maximum number of */
#define PARALLEL	0	/* transform constants */
#define PERSPECTIVE	1
#define NONE		1	/* segment types */
#define XLATE2		2
#define XFORM2		3
#define XLATE3		2
#define XFORM3		3
#define SOLID		0	/* line styles */
#define DOTTED		1
#define DASHED		2
#define DOTDASHED	3
#define CONSTANT	0	/* polygon shading modes */
#define GOURAUD		1
#define PHONG		2
#define PICK		0	/* input device constants */
#define KEYBOARD	1
#define BUTTON		2
#define LOCATOR		3
#define VALUATOR	4
#define STROKE		5
#define ROMAN		0	/* vector font select constants */
#define GREEK		1
#define SCRIPT		2
#define OLDENGLISH	3
#define STICK		4
#define SYMBOLS		5
#define GALLANT		0	/* raster font constants */
#define GACHA		1
#define SAIL		2
#define GACHABOLD	3
#define CMR		4
#define CMRBOLD		5
#define OFF		0	/* char justify constants */
#define LEFT		1
#define CENTER		2
#define RIGHT		3
#define NORMAL		0	/* rasterop selection */
#define XORROP		1
#define ORROP		2
#define PLAIN		0	/* polygon interior style */
#define SHADED		1
#define BASIC		0	/* Core output levels */
#define BUFFERED	1
#define DYNAMICA	2
#define DYNAMICB	3
#define DYNAMICC	4
#define NOINPUT		0	/* Core input levels */
#define SYNCHRONOUS	1
#define COMPLETE	2
#define TWOD		0	/* Core dimensions */
#define THREED		1
static struct {			/* default primitive attributes */
	int lineindx;
	int fillindx;
	int textindx;
	int linestyl;
	int polyintstyl;
	int polyedgstyl;
	float linwidth;
	int pen;
	int font;
	float chwidth,chheight;
	float chup[4], chpath[4], chspace[4];
	int chjust;
	int chqualty;
	int marker;
	int pickid;
	int rasterop;
	} PRIMATTS = {1,1,1,SOLID,PLAIN,SOLID,0.0,0,STICK,11.,11.,
		{0.,1.,0.,1.},{1.,0.,0.,1.}, {0.,0.,0.,1.},
		OFF,STRING,42,0,NORMAL};

#define DEVNAMESIZE 20

#define CLEAR		0	/* virtual device interface opcodes */
#define INITIAL		1
#define SETVSBL		2
#define SETHILIT	3
#define SETDTCT		4
#define DELETE		5
#define ROTATE2		6
#define SCALE2		7
#define TRANS2LATE	8
#define OPENSEG		9
#define TERMINATE	10
#define CLOSEG		11
#define SETNDC		12
#define GETCP		13
#define MOVE		14
#define LINE		15
#define POLYGN2		16
#define POLYGN3		17
#define TEXT		18
#define VTEXT		19
#define MARK		20
#define SETTAB		21
#define GETTAB		22
#define SETLCOL		23
#define SETFCOL		24
#define SETTCOL		25
#define SETSTYL		26
#define SETPISTYL	27
#define SETPESTYL	28
#define SETWIDTH	29
#define SETPEN		30
#define SETFONT		31
#define SETUP		32
#define SETPATH		33 
#define SETSPACE	34
#define SETCHARJUST	35
#define SETPID		36
#define SETROP		37
#define RASPUT		38
#define RASGET		39
#define INITZB		40
#define TERMZB		41
#define LOCK		42
#define UNLOCK		43
struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		};
typedef struct {	/*  structure for passing arguments to the  */
		int opcode;          /*  device drivers, which will use only */
		int instance;        /*  the parts they need. */
		int logical;
		char *ptr1, *ptr2, *ptr3;
		int   int1,  int2,  int3;
		float float1, float2, float3;
		} ddargtype;

#define NULL_VWSURF {"", "", 0, 0, 0, 0, 0}
#define DEFAULT_VWSURF(ddname) {"", "", 0, ddname, 0, 0, 0}
