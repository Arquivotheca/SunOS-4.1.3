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
/*	@(#)corevars.h 1.1 92/07/30 SMI	*/

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*----------------------------------------------------------------*/
/*----------------- Global variable declarations -----------------*/


		/*** PRIMITIVE STATE VARIABLES ***/

	short	_core_linecflag, _core_fillcflag, _core_textcflag,
		_core_shadeflag, _core_lsflag, _core_pisflag, _core_pesflag,
		_core_lwflag, _core_fntflag, _core_penflag, _core_justflag,
		_core_upflag, _core_pathflag, _core_spaceflag, _core_qualflag,
		_core_markflag, _core_ropflag, _core_cpchang;
	short	_core_linatt, _core_polatt, _core_texatt, _core_rasatt;
	primattr _core_current;


		/*** WINDOW STATE VARIABLES ***/

	short	_core_critflag, _core_updatewin, _core_winsys, _core_shellwinnum;
	int	(*_core_sighandle)();		/* routine to handle window signal */


		/*** VIEW SURFACES ***/

	viewsurf _core_surface[MAXVSURF];


		/*** SEGMENT STATE VARIABLES ***/

	segstruc _core_segment[SEGNUM];	/* Segment list */
	segstruc *_core_openseg;
	segattr _core_defsegat;
	short _core_segnum;
	short _core_prevseg;
	short _core_csegtype;
	short _core_osexists;			/* open segment exists */


		/*** VIEWING TRANSFORM VARIABLES ***/

	pt_type _core_cp;		/* current point world coords */
	ipt_type _core_ndccp;		/* current point NDC coords */
	vwprmtype _core_vwstate;	/* user-specified viewing parameters */
	float _core_vwxform1[4][4];	/* view transform to +/-1 xy 0/1 z */
	float _core_vwxform32k[4][4];	/* view transform to +/-32K xy 0/32K z */
	float _core_invwxform[4][4];	/* inverse of _core_compxfrm */
	float _core_TStack[10][16];	/* view transform stack */
	int _core_TSp;			/* transform stack pointer */
	float _core_modxform[4][4];	/* modelling or world xform */
	float _core_compxfrm1[4][4];	/* composite transform: mod * view1 */
	float _core_compxfrm32k[4][4];	/* composite transform: mod * view32K */
	short _core_compxfrm_invert;	/* is compxfrm32k invertible? */
	short _core_wndwclip;		/* true if clipping to window */
	short _core_outpclip;		/* true if clipping after imxform */
	int _core_frontclip;		/* onoff for hither and yon clip */
	int _core_backclip;
	int _core_wclipplanes;
	short _core_coordsys;		/* right or left handed world coords */
	int _core_ndcspace[3];
	struct	{
		float width, height, depth;
		} _core_ndc;
					/* output scaling parameters  */
	int _core_scalex, _core_scaley, _core_scalez;
	int _core_poffx, _core_poffy, _core_poffz;


		/*** POLYGON DATA STRUCTURES ***/

	vtxtype _core_vtxlist[3*MAXPOLY];   /* CLIPPER OUTPUT VTX LIST */
	vtxtype _core_ddvtxlist[3*MAXPOLY]; /* DEVICE OUTPUT VTX LIST */
	struct {int x,y;} _core_prvtxlist[3*MAXPOLY]; /* PIXRECT VTX LIST */
	short _core_vtxcount;


		/*** CONTROL RELATED STATE VARIABLES ***/

	short _core_xorflag;
	short _core_batchupd;
	short _core_sysinit;
	short _core_vfinvokd;
	short _core_corsyset;
	short _core_ndcset;
	short _core_vtchang;


		/*** PERFORMANCE HACK VARIABLES ***/

	struct vwsurf *_core_xformvs;		/* ptr to fast xform vs */
	short _core_ddxformset;			/* true if top of matrix stack
						    is loaded in xformvs dd*/
	short _core_fastflag;			/* indicates speedup possible
						   for temporary segments when
						   all selected view surfaces
						   have hphardwr */
	short _core_fastwidth;			/* if fastflag is TRUE,
						   fastwidth indicates that all
						   selected view surfaces have
						   a linewidth they can handle
						   as a fast primitive */
	short _core_fastpoly;			/* if fastflag is TRUE,
						   fastpoly indicates that all 
						   selected view surfaces have
						   a polygon they can handle
						   as a fast primitive */
						
	short _core_vtxnmlcnt;		/* remembers the # of normals
						   passed to set_vertex_normals;
						   used to speed up 3-D polygons
						   when possible */


		/*** INPUT STATE VARIABLES ***/

	pickstr _core_pick[PICKS];
	locatstr _core_locator[LOCATORS];
	keybstr _core_keybord[KEYBORDS];
	strokstr _core_stroker[STROKES];
	valstr _core_valuatr[VALUATRS];
	butnstr  _core_button[BUTTONS];
	/* asstype assocs[ASSOCNUM]; */


		/*** INPUT QUEUE VARIABLES ***/

	/* int qsize;
	inqueue cereport, inputq;
	short cervalid;
	eventype events;
	samptype samples; */

		/*** EXTERNAL DECLARATIONS ***/
	extern unsigned _core_sqrttable[];
	extern char *malloc();
