#ifndef lint
static char	sccsid[] = "@(#)devcont.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif

/*
 * Copyright (c) 1985, 1986, 1987, 1988, 1989 by Sun Microsystems, Inc. 
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

/*
 * CGI Device control functions
 */

/*
hard_reset
_cgi_reset_internals
_cgi_reset_tables
_cgi_reset_input
reset_to_defaults
clear_view_surface
clear_control
set_error_warning_mask
*/

/*
bugs:
default drawing modes
attribute structure.
*/
#include "cgipriv.h"

Gstate _cgi_state;		/* CGI global state */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
int             _cgi_vwsurf_count;	/* number of view surfaces */
Cacttype _cgi_clear_mode;
Cexttype _cgi_clear_extent;
int             _cgi_background_color;	/* background color */
Cerrtype _cgi_error_mode;	/* error trap mode */
int             _cgi_polybase;	/* number of elements in global polygon list */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  hard_reset		 				    */
/*                                                                          */
/*		Resets attributes and input devices, and clears the screen. */
/****************************************************************************/

Cerror          hard_reset()
{
    /* default VDC values */
    static Ccoor    minc =
    {
     0, 0
    };
    static Ccoor    maxc =
    {
     32767, 32767
    };
    static Cclip    cind = CLIP;
    static Cerrtype errsig = INTERRUPT;

    /* call functions which set defaults */
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	(void) vdc_extent(&minc, &maxc);	/* set VDC space */
	(void) clip_indicator(cind);	/* set clipping to ON */
	(void) clip_rectangle(0, 32767, 0, 32767);
	(void) set_error_warning_mask(errsig);	/* set error to interrupt */
	_cgi_reset_internals();	/* resets internals */
	(void) reset_to_defaults();
	_cgi_polybase = 0;	/* Clear global polygon list */
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_internals 					    */
/*                                                                          */
/*		resets internals states : 1) fonts			    */
/****************************************************************************/
_cgi_reset_internals()
{
    register int    i;

    /* ## is this really necessary?  It doesn't affect the user's model */
    /* reset font if necessary */
    if (_cgi_state.open_font.ptr != (struct pixfont *) 0)
    {
	(void) pf_close(_cgi_state.open_font.ptr);
	_cgi_state.open_font.ptr = (struct pixfont *) 0;
	_cgi_state.open_font.num = NO_FONT;
    }

    /* sets up default attribute and pattern tables */
    _cgi_reset_tables();

    /* cleans up input devices */
    _cgi_reset_input();

    /* Reset all view-surface-specific values */
    /* Maybe this iteration should be in the callers of _cgi_reset_internals */
    for (i = 0; i < _cgi_vwsurf_count; i++)
	if (_cgi_view_surfaces[i] != (View_surface *) NULL)
	    _cgi_reset_intatt(_cgi_view_surfaces[i]);
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_tables 					    */
/*                                                                          */
/*		resets attribute table and pattern table	 	    */
/****************************************************************************/
_cgi_reset_tables()
{
    static Cint     cross_hatching[3][3] =
    {
     {0, 1, 0},
     {1, 1, 1},
     {0, 1, 0}
    };
    static Cint     polka_dotted[3][3] =
    {
     {0, 0, 0},
     {0, 1, 0},
     {0, 0, 0}
    };
    /* pattern table */
    (void) pattern_table(0, 3, 3, (Cint *) cross_hatching);
    (void) pattern_table(1, 3, 3, (Cint *) polka_dotted);
    /* initialize attribute table */
/*     define_bundle_index (1, defatt); */
    /* return (0);	wds removed 850723 */
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_reset_input 					    */
/*                                                                          */
/*		turns off initialized input devices and clears input queue  */
/****************************************************************************/
_cgi_reset_input()
{
    Cint            i;
    Clogical        valid;
    Clidstate       state;

    /* turn off all initialized devices */
    for (i = 0; i < _CGI_KEYBORDS; i++)
    {
	(void) inquire_lid_state(IC_STRING, i + 1, &valid, &state);
	if (state != RELEASE)
	    (void) release_input_device(IC_STRING, i + 1);
    }
    for (i = 0; i < _CGI_CHOICES; i++)
    {
	(void) inquire_lid_state(IC_CHOICE, i + 1, &valid, &state);
	if (state != RELEASE)
	    (void) release_input_device(IC_CHOICE, i + 1);
    }
    for (i = 0; i < _CGI_STROKES; i++)
    {
	(void) inquire_lid_state(IC_STROKE, i + 1, &valid, &state);
	if (state != RELEASE)
	    (void) release_input_device(IC_STROKE, i + 1);
    }
    for (i = 0; i < _CGI_VALUATRS; i++)
    {
	(void) inquire_lid_state(IC_VALUATOR, i + 1, &valid, &state);
	if (state != RELEASE)
	    (void) release_input_device(IC_VALUATOR, i + 1);
    }
    for (i = 0; i < _CGI_LOCATORS; i++)
    {
	(void) inquire_lid_state(IC_LOCATOR, i + 1, &valid, &state);
	if (state != RELEASE)
	    (void) release_input_device(IC_LOCATOR, i + 1);
    }
/* clean up event queue */
    (void) flush_event_queue();
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  reset_to_defaults	 				    */
/*                                                                          */
/*		 Reset output attributes only.				    */
/****************************************************************************/

Cerror          reset_to_defaults()
{
    register Outatt *attP = _cgi_att;
    int             i, ivw;
    Cerror          err;
    static Ccoor    pt =
    {
     0, 0
    };
    static Cbunatt  att_defaults =
    {
    /*
     * Hatch and Pattern indices and marker color wrong: SOLID, 0.0, 1, DOT,
     * 4.0, 0, HOLLOW, 0, 1, 1, SOLID, 0.0, 1, STICK, STRING, 1.0, 0.1, 1 
     */
     SOLID,			/* 0 line_type */
     0.0,			/* 1 line_width */
     1,				/* 2 line_color */
     DOT,			/* 3 marker_type */
     4.0,			/* 4 marker_size */
     1,				/* 5 marker_color */
     HOLLOW,			/* 6 interior_style */
     0,				/* 7 hatch_index */
     1,				/* 8 pattern_index */
     1,				/* 9 fill color */
     SOLID,			/* 10 perimeter_type */
     0.0,			/* 11 perimeter_width */
     1,				/* 12 perimeter_color */
     STICK,			/* 13 text_font */
     STRING,			/* 14 text_precision */
     1.0,			/* 15 character_expansion */
     0.1,			/* 16 character_spacing */
     1				/* 17 text_color */
    };

    err = _cgi_check_state_5();
    if (!err)
    {
	for (i = 0; i < 18; i++)
	{
	    attP->asfs[i] = INDIVIDUAL;
	}
	(void) define_bundle_index(1, &att_defaults);
	/* wds 851002: the code below will now always match defaults bundle */
	attP->endstyle = BEST_FIT;
	attP->line.style = att_defaults.line_type;
	attP->line_spec_mode = SCALED;
	attP->line.width = att_defaults.line_width;
	attP->line.color = att_defaults.line_color;
	attP->line.index = 1;
	attP->marker.type = att_defaults.marker_type;
	attP->mark_spec_mode = SCALED;
	attP->marker.size = att_defaults.marker_size;
	attP->marker.color = att_defaults.marker_color;
	attP->marker.index = 1;
	attP->fill.style = att_defaults.interior_style;
	attP->fill.visible = ON;
	attP->fill.color = att_defaults.fill_color;
	attP->fill.index = 1;
	attP->fill.hatch_index = att_defaults.hatch_index;
	attP->fill.pattern_index = att_defaults.pattern_index;
	(void) pattern_index(1);/* sets current pattern attributes */
	attP->pattern.point = (Ccoor *) & pt;
	attP->pattern.dx = 300;
	attP->pattern.dy = 300;
	attP->fill.pstyle = att_defaults.perimeter_type;
	attP->perimeter_spec_mode = SCALED;
	attP->fill.pwidth = att_defaults.perimeter_width;
	attP->fill.pcolor = att_defaults.perimeter_color;
	attP->text.attr.fontset = 1;
	attP->text.attr.index = 1;
	attP->text.attr.current_font = att_defaults.text_font;
	attP->text.attr.precision = att_defaults.text_precision;
	attP->text.attr.exp_factor = att_defaults.character_expansion;
/*	attP->text.attr.space = att_defaults. character_spacing; */
	attP->text.attr.color = att_defaults.text_color;
	attP->text.attr.height = 1000.0;
	attP->text.attr.basex = 1.0;
	attP->text.attr.basey = 0.0;
	attP->text.attr.upx = 0.0;
	attP->text.attr.upy = 1.0;
	attP->text.attr.path = RIGHT;
	attP->text.attr.halign = NRMAL;
	attP->text.attr.valign = NORMAL;
	attP->text.attr.hcalind = 1.0;
	attP->text.attr.vcalind = 1.0;
	(void) character_spacing(att_defaults.character_spacing);

	/* Reset per-view-surface values in all active view surfaces */
	ivw = 0;
	while (_cgi_bump_all_vws(&ivw))
	{
	    _cgi_set_conv_line_width(_cgi_vws);
	    _cgi_set_conv_marker_size(_cgi_vws);
	    _cgi_set_conv_perimeter_width(_cgi_vws);
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: clear_view_surface 					    */
/*                                                                          */
/*		Clears view surface.					    */
/****************************************************************************/

Cerror          clear_view_surface(name, defflag, color)
Cint            name;		/* name assigned to cgi view surface */
Cflag           defflag;	/* default color flag */
Cint            color;
{
    register View_surface *vwsP;
    int             op;
    Cerror          err;

    err = _cgi_check_state_5();
    if (!err)
	err = _cgi_context(name);
    if (!err)
    {
	vwsP = _cgi_view_surfaces[name];
	if (defflag == ON)
	{
	    err = _cgi_check_color(color);
	    if (!err)
		_cgi_background_color = color;
	}
	else
	    _cgi_background_color = 0;
	if (!err)
	    switch (_cgi_clear_mode)
	    {
	    case CLEAR:
		/* op = _cgi_pix_mode | PIX_COLOR(color); */
		/* op = PIX_CLR | PIX_COLOR(color); */
		/* op = PIX_SRC | PIX_COLOR(color); */
		op = PIX_SRC | PIX_COLOR(_cgi_background_color);
		switch (_cgi_clear_extent)
		{
		case CLIP_RECT:
		    pw_writebackground(vwsP->sunview.orig_pw,
				       vwsP->conv.clip.r_left,
				       vwsP->conv.clip.r_top,
				       vwsP->conv.clip.r_width,
				       vwsP->conv.clip.r_height,
				       op);
		    break;
		case VIEWPORT:
		    pw_writebackground(vwsP->sunview.orig_pw,
				       vwsP->vport.r_left,
				       vwsP->vport.r_top,
				       vwsP->vport.r_width,
				       vwsP->vport.r_height,
				       op);
		    break;
		case VIEWSURFACE:
		    pw_writebackground(vwsP->sunview.orig_pw,
				       0, 0,
				       vwsP->real_screen.r_width,
				       vwsP->real_screen.r_height,
				       op);
		    break;
		}
		break;
	    case NO_OP:
		break;
	    case RETAIN:
		break;
	    }
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: clear_control	 					    */
/*                                                                          */
/*		Regulates action when clear view surface is called.	    */
/****************************************************************************/
 /* ARGSUSED WDS: As documented, hard and intern are ignored. */
Cerror          clear_control(soft, hard, intern, extent)
Cacttype        soft, hard;	/* soft-copy action, hard-copy action */
Cacttype        intern;		/* internal action  */
Cexttype        extent;		/* clear extent */
{
    int             err;

    if (_cgi_state.cgipw_mode)	/* opened via open_pw_cgi? */
	err = 112;		/* ENOTCCPW - Function not compatible with
				 * CGIPW mode. */
    else
	err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_clear_mode = soft;
	_cgi_clear_extent = extent;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_error_warning_mask					    */
/*                                                                          */
/*		CGI action when error occurs.				    */
/****************************************************************************/

Cerror          set_error_warning_mask(action)
Cerrtype        action;		/* No action, poll, or interrupt */
{
    int             err;

    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_error_mode = action;
    }
    return (_cgi_errhand(err));
}
