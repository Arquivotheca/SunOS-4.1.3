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
/*	@(#)coretypes.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
	Global defined constants and typedefs for CORE system.
*/
#ifndef TRUE
#define TRUE		1
#endif
#ifndef FALSE
#define FALSE		0
#endif
#define SHORT		1
#define FLOAT		2
#define QUESTION	63
#define MINASCII	32
#define MAXASCII	127
#define STRING		0
#define CHARACTER	1
#define PARALLEL	0	/* transform constants */
#define PERSPECTIVE	1
#define	TSize		16			/* transform size */
#define	TSlim		9			/* transform stack limit */
#define EMPTY		-2	/* segment types */
#define DELETED		-1
#define NORETAIN	0
#define RETAIN		1
#define XLATE2		2
#define XFORM2		3
#define XLATE3		2
#define XFORM3		3
#define SEGNUM		1024	/* max number of segments */
#define MAXVSURF	5	/* bw1dd,bw2dd,cg1dd,cg2dd, cgpixwindd ...*/
#define OFF		0	/* char justify constants */
#define LEFT		1
#define CENTER		2
#define RIGHT		3
#define NORMAL		0	/* rasterop selection */
#define XORROP		1
#define ORROP		2
#define PLAIN		0	/* polygon interior style */
#define SHADED		1
#define SOLID		0	/* line styles */
#define DOTTED		1
#define DASHED		2
#define DOTDASHED	3
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
#define PDFENDSEGMENT	-1	/* Pseudo Display file opcodes */
#define PDFMOVE		1
#define PDFLINE		2
#define PDFTEXT		3
#define PDFMARKER	4
#define PDFPOL2		5
#define PDFPOL3		6
#define PDFLCOLOR	7
#define PDFFCOLOR	8
#define PDFTCOLOR	9
#define PDFLINESTYLE	10
#define PDFPISTYLE	11
#define PDFPESTYLE	12
#define PDFLINEWIDTH	13
#define PDFPEN		14
#define PDFFONT		15
#define PDFSPACE	16
#define PDFPATH		17
#define PDFUP		18
#define PDFCHARQUALITY	19
#define PDFCHARJUST	20
#define PDFPICKID	21
#define PDFROP		22
#define PDFBITMAP	23
#define PDFVWPORT	24
#define CLEAR		0	/* virtual device interface opcodes */
#define INITIAL		1
#define TERMINATE	2
#define CLOSEG		3
#define SETNDC		4
#define GETCP		5
#define MOVE		6
#define ECHOMOVE	7
#define LINE		8
#define ECHOLINE	9
#define POLYGN2		10
#define POLYGN3		11
#define TEXT		12
#define VTEXT		13
#define MARK		14
#define SETTAB		15
#define GETTAB		16
#define SETLCOL		17
#define SETFCOL		18
#define SETTCOL		19
#define SETSTYL		20
#define SETPISTYL	21
#define SETPESTYL	22
#define SETWIDTH	23
#define SETPEN		24
#define SETFONT		25
#define SETUP		26
#define SETPATH		27 
#define SETSPACE	28
#define SETCHARJUST	29
#define SETPID		30
#define SETROP		31
#define RASPUT		32
#define RASGET		33
#define INITZB		34
#define TERMZB		35
#define SETZBCUT	36
#define LOCK		37
#define UNLOCK		38
#define CURSTRKON	39
#define CURSTRKOFF	40
#define KILLPID		41
#define MARKER		42
#define SEGDRAW		43
#define WINUPDATE	44
#define SETIMGXFORM	45
#define SETVWXFORM32K	46
#define VIEWXFORM2	47
#define VIEWXFORM3	48
#define MATMULT		49
#define CHKGPRESETCOUNT	50
#define OUTPUTCLIP	51
#define SETVWXFORM1	52
#define WLDVECSTONDC	53
#define WLDVECSTOSCREEN	54
#define WLDPLYGNTONDC	55
#define WLDPLYGNTOSCREEN 56
#define SETVIEWPORT	57
#define WINDOWCLIP	58
#define CHKHWZBUF	59
#define INQFORCEDREPAINT 60
#define PICK		0	/* input device constants */
#define KEYBOARD	1		/* device classes */
#define BUTTON		2
#define LOCATOR		3
#define VALUATOR	4
#define STROKE		5
#define PICKS		1		/* device counts */
#define LOCATORS	1
#define KEYBORDS	1
#define STROKES		1
#define VALUATRS	1
#define BUTTONS		3
#define SETKMASK	3		/* keyboard device driver opcodes */
#define CLRKMASK	4
#define KEYREAD		5
#define BUTREAD		3		/* mouse device driver opcodes */
#define XYREAD		4
#define XYWRITE		5
#define INITBUT		6
#define TERMBUT		7
#define ICURSTRKON	8
#define ICURSTRKOFF	9
#define FLUSHINPUT	10
#define ASSOCNUM	20
#define PMODE 0755	/*** RWE FOR OWNER,RE FOR GROUP, OTHERS ***/
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
#define INFINITY	1024
#define MAXPOLY		200
#define WARNOCK		0	/* Shading style */
#define GOURAUD		1
#define PHONG		2
#define MINCOLORINDEX	0
#define MAXCOLORINDEX	255
#define MAX_NDC_COORD	32767
#define MAX_CLIP_COORD	32767
#define DEVNAMESIZE	20
#define VWSURF_NEWFLG	1
#define NULL_VWSURF	{"", "", 0, 0, 0, 0, "", 0, 0}
#define BW_FB		1
#define COLOR_FB	2
#define BW_COLOR_FB	3
#define GP1_FB		4


typedef struct { int x,y,z,w;} ipt_type;
typedef struct { float x,y,z,w;} pt_type;
	
typedef struct {	/* VERTEX, NDC coords, unit normal at vertex 20b.10b */
	int x,y,z,w;
	int dx,dy,dz,dw; } vtxtype;
typedef struct {	/* RASTER, pixel coords */
	int width, height, depth;	/* width, height in pixels, bits/pixl */
	short *bits;
	} rast_type;
typedef struct {	/* colormap struct  */
	int type, nbytes;
	char *data;
	} colormap_type;
typedef struct { float width, height; }aspect;

typedef struct {		/* primitive attribute list structure */
   int lineindx;
   int fillindx;
   int textindx;
   int linestyl;
   int polyintstyl;
   int polyedgstyl;
   float linwidth;
   int pen;
   int font;
   aspect charsize;
   pt_type chrup, chrpath, chrspace;
   int chjust;
   int chqualty;
   int marker;
   int pickid;
   int rasterop;
   } primattr;

typedef struct {
	float xmin, xmax, ymin, ymax; } windtype;
typedef struct {
	int xmin,xmax,ymin,ymax,zmin,zmax; } porttype;

typedef struct {
      float vwrefpt[3];
      float vwplnorm[3];
      float viewdis;
      float frontdis;
      float backdis;
      int projtype;
      float projdir[3];
      windtype window;
      float vwupdir[3];
      porttype viewport;
      } vwprmtype;

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

struct windowstruct {
		    short winwidth, winheight;	/* window size */
		    short ndcxmax, ndcymax;	/* NDC extent of window */
		    short xscale, yscale,	/* xform window to NDC */
			xoff, yoff,		/* pixel coords for NDC 0,0 */
			xpixmax, ypixmax,	/* pixel coords for NDC */
			xpixdelta, ypixdelta;	/* maximums and deltas */
		    struct pixwin *pixwin;
		    struct {
			   short r_left, r_top;
			   short r_width, r_height;
			   } rect;
		    int winfd;
		    int needlock;
		    int rawdev;
		    };

typedef struct {		/* View surface descriptor */
   short hphardwr;   /* can support high performance         */
   short lshardwr;   /* can support line style               */
   short lwhardwr;   /* can support line width               */
   short clhardwr;   /* can support color                    */
   short txhardwr;   /* can support hardware text generation */
   short hihardwr;   /* can support hidden surfaces          */
   short erasure;    /* deletes by redrawing with 0          */
   short nwframdv;   /* new frame action required on change  */
   short nwframnd;   /* new frame action needed when end BOU */
   short hiddenon;   /* true if eliminating hidden surfaces  */
   short vinit;  
   short selected;
   struct vwsurf vsurf;
   struct windowstruct *windptr;
   } viewsurf;

typedef struct {		/* Segment attributes structure */
   int visbilty;			/* true if segment is visible */
   int detectbl;			/* true if segment is pickable */
   int highlght;			/* true to blink segment */
   float scale[3];			/* NDC coord scale params */
   float translat[3];			/* NDC dx,dy,dz to translate segment */
   float rotate[3];			/* radians to rotate segment */
   } segattr;

typedef struct {		/* Segment descriptor */
   int segname;
   short type;				/* noretain,retain,xlate,xform */
   segattr segats;
   int vsurfnum;			/* numbr of view surfs to display */
   viewsurf *vsurfptr[MAXVSURF];
   int pdfptr;				/* byte ptr into pseudo display file */
   int redraw;				/* true if segment needs redraw */
   int segsize;
   int imxform[4][4];			/* image xform matrix */
   int idenflag;			/* true if identity matrix */
   ipt_type bndbox_min;			/* bounding box minimum */
   ipt_type bndbox_max;			/* bounding box maximum */
   } segstruc;


typedef struct {	/*  structure for passing arguments to the  */
   int opcode;          /*  device drivers, which will use only the */
   int instance;        /*  parts they need.                        */
   int logical;
   char *ptr1, *ptr2, *ptr3;
   int   int1,  int2,  int3;
   float float1, float2, float3;
   } ddargtype;

typedef struct {		/* INPUT DEVICE DESCRIPTORS */
   short enable;			/* bit 0: 0 => uninit, 1 => init */
   short echo;				/* type of echo, 0=noecho,... */
   int echopos[2];			/* NDC pt for echo reference */
   viewsurf *echosurfp;			/* view surface to echo on */
   int instance;			/* instance tag of device driver
					/* (ptr into _core_surface array) */
   int (*devdrive)();			/* device driver routine */
   } device;

typedef struct {		/* locator */
   device subloc;
   int setpos[2];			/* set position of locator */
   } locatstr; 

typedef struct {		/* pick */
   device subpick;
   int aperture;			/* pick aperture */
   } pickstr; 

typedef struct {		/* keyboard */
   device subkey;
   char *initstring;			/* initial echo string */
   int initpos;				/* initial cursor position */
   int bufsize;				/* user keyboard buffer size  */
   int maxbufsz;			/* max buffer size = 80 chars */
   } keybstr; 

typedef struct {		/* stroke */
   device substroke;
   int bufsize;				/* xy buffer size */
   int distance;			/* minimum distance value */
   int time;				/* minimum time value */
   } strokstr;

typedef struct {		/* valuator */
   device subval;
   float vlinit;			/* set initial value */
   float vlmin;				/* minimum value */
   float vlmax;				/* maximum value */
   } valstr;

typedef struct {		/* buttons */
   device subbut;
   int prompt;				/* unused */
   } butnstr;

typedef struct {		/* associations (currently unused) */
   int evenclas;
   int eventnam;
   int samplcls;
   int samplnam;
   } asstype;

typedef struct {		/* REPORTS EVENTS QUEUES */
   short segname;
   int pickid;
   } reportpk;

typedef struct {
   int length;
   char *text;
   } reportky;

typedef struct {
   reportpk pickrept;
   reportky keybrept;
   } eventype;

typedef struct{
   int location[2];
   float value;
   } samptype;;

typedef struct {
   int samplcls;
   int samplnam;
   samptype onefsmpl;
   } smpldata;

typedef struct {
   int evenclas;
   int eventnam;
   eventype onefevnt;
   int samplnum;
   smpldata smplinfo;
   } inqueue; 
