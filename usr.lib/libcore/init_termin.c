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
#ifndef lint
static char sccsid[] = "@(#)init_termin.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include	<sunwindow/window_hs.h>
#include	"coretypes.h"
#include	"corevars.h"
#include	<stdio.h>
#include	<signal.h>

int _core_client = 123;		/* dummmy handle for notifier, must be non-0 */

Notify_value _core_winsig();
Notify_value _core_cleanup();
Notify_value _core_sigchild();
Notify_value _core_sigxcpu();

Notify_func oldsighuphdlr;
Notify_func oldsiginthdlr;
Notify_func oldsigwinchhdlr;
Notify_func oldsigxcpuhdlr;
Notify_func oldsigchildhdlr;

int _core_winsigresp();

int _core_mousedd(), _core_keybrdd();

struct vwsurf _core_nullvs = NULL_VWSURF;

PIXFONT *_core_defaultfont;

static short outlevel, inlevel, dimenslevel;

/*
 * Initialize suncore
 */
initialize_core(outlev, inlev, dim)
    int outlev, inlev, dim;
{
    char *funcname;
    int i;
    viewsurf *surfp;
    segstruc *segptr;
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    char *windowname, *getenv();

    funcname = "initialize_core";
    outlevel = DYNAMICC;
    inlevel = SYNCHRONOUS;
    dimenslevel = THREED;
    if (_core_sysinit) {	/* ROUTINE PREVIOUSLY CALLED? */
	_core_errhand(funcname, 0);
	return (1);
    }
    if (outlev > outlevel) {	/* OUTPUT LEVEL SUPPORTED? */
	_core_errhand(funcname, 1);
	return (2);
    }
    if (inlev > inlevel) {	/* INPUT LEVEL SUPPORTED? */
	_core_errhand(funcname, 1);
	return (2);
    }
    if (dim > dimenslevel) {	/* DIMENSION SUPPORTED? */
	_core_errhand(funcname, 1);
	return (2);
    }
/*
 * Initialize control related state variables
 */
    _core_sysinit = TRUE;
    _core_batchupd = FALSE;
    _core_xorflag = FALSE;
    _core_critflag = FALSE;
    _core_updatewin = FALSE;

    _core_errhand("", -1);	/* initialize error handler to "NO ERRORS" */

/*
 * Initialize segment related variables
 */
    for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM]; segptr++) {
	segptr->segname = 0;
	segptr->type = EMPTY;
	segptr->segats.visbilty = TRUE;
	segptr->segats.detectbl = FALSE;
	segptr->segats.highlght = FALSE;
	segptr->segats.scale[0] = 1.0;
	segptr->segats.scale[1] = 1.0;
	segptr->segats.scale[2] = 1.0;
	segptr->segats.translat[0] = 0.0;
	segptr->segats.translat[1] = 0.0;
	segptr->segats.translat[2] = 0.0;
	segptr->segats.rotate[0] = 0.0;
	segptr->segats.rotate[1] = 0.0;
	segptr->segats.rotate[2] = 0.0;
	segptr->vsurfnum = 0;
	for (i = 0; i < MAXVSURF; i++) {
	    segptr->vsurfptr[i] = NULL;
	}
	segptr->pdfptr = NULL;
	segptr->redraw = FALSE;
	segptr->segsize = 0;
    }
    _core_segnum = 0;

/*
 * Initilaize view surface related variables
 */
    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++) {
	surfp->hphardwr = FALSE;
	surfp->lshardwr = FALSE;
	surfp->lwhardwr = FALSE;
	surfp->clhardwr = FALSE;
	surfp->txhardwr = FALSE;
	surfp->hihardwr = FALSE;
	surfp->erasure = FALSE;
	surfp->nwframdv = FALSE;
	surfp->nwframnd = FALSE;
	surfp->vinit = FALSE;
	surfp->hiddenon = FALSE;
	surfp->selected = 0;
	surfp->vsurf = _core_nullvs;
    }


/*
 * Initilaize input device related variables
 */
    for (pickptr = &_core_pick[0]; pickptr < &_core_pick[PICKS]; pickptr++)
	pickptr->subpick.enable = FALSE;
    for (locatptr = &_core_locator[0]; locatptr < &_core_locator[LOCATORS]; locatptr++)
	locatptr->subloc.enable = FALSE;
    for (keybptr = &_core_keybord[0]; keybptr < &_core_keybord[KEYBORDS]; keybptr++)
	keybptr->subkey.enable = FALSE;
    for (valptr = &_core_valuatr[0]; valptr < &_core_valuatr[VALUATRS]; valptr++)
	valptr->subval.enable = FALSE;
    for (butnptr = &_core_button[0]; butnptr < &_core_button[BUTTONS]; butnptr++)
	butnptr->subbut.enable = FALSE;
    for (strokptr = &_core_stroker[0]; strokptr < &_core_stroker[STROKES]; strokptr++)
	strokptr->substroke.enable = FALSE;

/*
 * Initilaize primitive related variables
 */
    _core_linecflag = FALSE;
    _core_fillcflag = FALSE;
    _core_textcflag = FALSE;
    _core_shadeflag = FALSE;
    _core_lsflag = FALSE;
    _core_pisflag = FALSE;
    _core_pesflag = FALSE;
    _core_lwflag = FALSE;
    _core_fntflag = FALSE;
    _core_penflag = FALSE;
    _core_justflag = FALSE;
    _core_upflag = FALSE;
    _core_pathflag = FALSE;
    _core_spaceflag = FALSE;
    _core_qualflag = FALSE;
    _core_markflag = FALSE;
    _core_ropflag = FALSE;
    _core_cpchang = TRUE;

    _core_cp.x = 0;
    _core_cp.y = 0;
    _core_cp.z = 0;
    _core_cp.w = 1.0;

    _core_current.lineindx = 1;
    _core_current.fillindx = 1;
    _core_current.textindx = 1;
    _core_current.linestyl = SOLID;
    _core_current.polyintstyl = PLAIN;
    _core_current.polyedgstyl = SOLID;
    _core_current.linwidth = 0.0;
    _core_current.pen = 0;
    _core_current.font = STICK;
    _core_current.charsize.width = 11.0;
    _core_current.charsize.height = 11.0;
    _core_current.chrpath.x = 1.0;
    _core_current.chrpath.y = 0.0;
    _core_current.chrpath.z = 0.0;
    _core_current.chrpath.w = 1.0;
    _core_current.chrspace = _core_current.chrpath;
    _core_current.chrspace.x = 0.0;
    _core_current.chrup.x = 0.0;
    _core_current.chrup.y = 1.0;
    _core_current.chrup.z = 0.0;
    _core_current.chrup.w = 1.0;
    _core_current.chjust = OFF;
    _core_current.chqualty = STRING;
    _core_current.marker = 42;
    _core_current.pickid = 0;
    _core_current.rasterop = NORMAL;

/*
 * Initilaize segment state variables
 */
    _core_defsegat.visbilty = TRUE;
    _core_defsegat.detectbl = FALSE;
    _core_defsegat.highlght = FALSE;
    _core_defsegat.scale[0] = 1.0;
    _core_defsegat.scale[1] = 1.0;
    _core_defsegat.scale[2] = 1.0;
    _core_defsegat.translat[0] = 0.0;
    _core_defsegat.translat[1] = 0.0;
    _core_defsegat.translat[2] = 0.0;
    _core_defsegat.rotate[0] = 0.0;
    _core_defsegat.rotate[1] = 0.0;
    _core_defsegat.rotate[2] = 0.0;

    _core_openseg = NULL;
    _core_osexists = FALSE;
    _core_prevseg = FALSE;
    _core_csegtype = RETAIN;

/*
 * Initilaize viewing state variables
 */
    _core_ndc.width = 1.0;
    _core_ndc.height = 0.75;
    _core_ndc.depth = 1.0;
    _core_ndcspace[0] = MAX_NDC_COORD;
    _core_ndcspace[1] = (int) (0.75 * (float) MAX_NDC_COORD);
    _core_ndcspace[2] = MAX_NDC_COORD;

    set_viewport_3(0., 1.0, 0.,.75, 0., 1.0);

    _core_vwstate.window.xmin = 0.0;
    _core_vwstate.window.xmax = 1.0;
    _core_vwstate.window.ymin = 0.0;
    _core_vwstate.window.ymax = 0.75;

    _core_vwstate.vwrefpt[0] = 0.;
    _core_vwstate.vwrefpt[1] = 0.;
    _core_vwstate.vwrefpt[2] = 0.;

    _core_vwstate.vwplnorm[0] = 0.;
    _core_vwstate.vwplnorm[1] = 0.;
    _core_vwstate.vwplnorm[2] = -1.;

    _core_vwstate.vwupdir[0] = 0.;
    _core_vwstate.vwupdir[1] = 1.;
    _core_vwstate.vwupdir[2] = 0.;

    _core_vwstate.projtype = PARALLEL;
    _core_vwstate.projdir[0] = 0.;
    _core_vwstate.projdir[1] = 0.;
    _core_vwstate.projdir[2] = 1.;

    _core_vwstate.frontdis = _core_vwstate.viewdis = 0.;
    _core_vwstate.backdis = 1.0;

    _core_wndwclip = TRUE;
    _core_frontclip = FALSE;
    _core_backclip = FALSE;
    _core_wclipplanes = 0xF;

    _core_outpclip = FALSE;

    _core_ndcset = FALSE;	/* Note: These must be set FALSE */
    _core_vfinvokd = FALSE;	/* AFTER call to set_viewport_? above */

    _core_coordsys = RIGHT;
    _core_corsyset = FALSE;

    _core_identity(&_core_modxform[0][0]);
    _core_validvt(TRUE);	/* set up transform stack and xforms */
    _core_vtchang = TRUE;


/*
 * Initilaize performance related variables
 */
    _core_xformvs = NULL;
    _core_fastflag = FALSE;
    _core_fastwidth = FALSE;
    _core_fastpoly = FALSE;


    _core_defaultfont = pf_default();	/* open up the defaultfont */

    if (!_core_PDFinit()) {	/* INIT PSEUDO DISPLAY FILE SYSTEM */
	_core_errhand(funcname, 87);
	exit(1);
    }
    if (windowname = getenv("WINDOW_ME")) {	/* WINDOW FIX */
	_core_winsys = TRUE;
	_core_shellwinnum = win_nametonumber(windowname);
    } else
	_core_winsys = FALSE;

    oldsighuphdlr = notify_set_signal_func((Notify_client)&_core_client,
				       _core_cleanup, SIGHUP, NOTIFY_ASYNC);
    oldsiginthdlr = notify_set_signal_func((Notify_client)&_core_client,
				       _core_cleanup, SIGINT, NOTIFY_ASYNC);
    oldsigwinchhdlr = notify_set_signal_func((Notify_client)&_core_client,
				      _core_winsig, SIGWINCH, NOTIFY_ASYNC);
    oldsigxcpuhdlr = notify_set_signal_func((Notify_client)&_core_client,
				      _core_sigxcpu, SIGXCPU, NOTIFY_ASYNC);
    oldsigchildhdlr = notify_set_signal_func((Notify_client)&_core_client,
				     _core_sigchild, SIGCHLD, NOTIFY_ASYNC);
    _core_sighandle = _core_winsigresp;
    return (0);
}


/*
 * Bring suncore down
 */
terminate_core()
{
    viewsurf *surfp;
    int i;

    for (i = 0; i < PICKS; i++)
	if (_core_pick[i].subpick.enable & 1)
	    terminate_device(PICK, i + 1);
    for (i = 0; i < KEYBORDS; i++)
	if (_core_keybord[i].subkey.enable & 1)
	    terminate_device(KEYBOARD, i + 1);
    for (i = 0; i < STROKES; i++)
	if (_core_stroker[i].substroke.enable & 1)
	    terminate_device(STROKE, i + 1);
    for (i = 0; i < LOCATORS; i++)
	if (_core_locator[i].subloc.enable & 1)
	    terminate_device(LOCATOR, i + 1);
    for (i = 0; i < VALUATRS; i++)
	if (_core_valuatr[i].subval.enable & 1)
	    terminate_device(VALUATOR, i + 1);
    for (i = 0; i < BUTTONS; i++)
	if (_core_button[i].subbut.enable & 1)
	    terminate_device(BUTTON, i + 1);
    delete_all_retained_segments();

    (void)pf_close(_core_defaultfont);

    for (surfp = &_core_surface[0]; surfp < &_core_surface[MAXVSURF]; surfp++) {
	if (surfp->vinit) {
	    if (surfp->selected) {
		deselect_view_surface(&surfp->vsurf);
	    }
	    terminate_view_surface(&surfp->vsurf);
	}
    }


/* Files are closed upon completion of program */

    _core_PDFclose();
    _core_sysinit = FALSE;

    (void)notify_set_signal_func((Notify_client)&_core_client,
			   oldsighuphdlr, SIGHUP, NOTIFY_ASYNC);
    (void)notify_set_signal_func((Notify_client)&_core_client,
			   oldsiginthdlr, SIGINT, NOTIFY_ASYNC);
    (void)notify_set_signal_func((Notify_client)&_core_client,
			   oldsigwinchhdlr, SIGWINCH, NOTIFY_ASYNC);
    (void)notify_set_signal_func((Notify_client)&_core_client,
			   oldsigxcpuhdlr, SIGXCPU, NOTIFY_ASYNC);
    (void)notify_set_signal_func((Notify_client)&_core_client,
			   oldsigchildhdlr, SIGCHLD, NOTIFY_ASYNC);

    _core_sighandle = (int (*) ()) 0;
    return (0);
}
