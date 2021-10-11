#ifndef	lint
static char	sccsid[] = "@(#)metafile.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
#endif	lint

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
 * CGI initialization & termination functions
 */

/*
open_cgi
_cgi_open_cgi
_cgi_initialize_internals
close_cgi
_cgi_alloc_output_att
_cgi_free_output_att
_cgi_hangup
_cgi_interrupt
set_up_sigwinch
_cgi_sigxcpu
*/

#include "cgipriv.h"
#include <sys/ioctl.h>
#include <sun/gpio.h>
#include <stdio.h>
#include <signal.h>
#include <sunwindow/notify.h>

Notify_value    _cgi_winsig();
Notify_value    _cgi_sigchild();
Notify_value    _cgi_hangup();
Notify_value    _cgi_interrupt();
Notify_value    _cgi_ioint();
Notify_value    _cgi_sigxcpu();

/* int   malloc_debug (); */

View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
View_surface   *_cgi_vws;	/* current view surface */
int             _cgi_vwsurf_count;	/* number of view surfaces */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
Ccombtype       _cgi_drawing_mode;
int             _cgi_ttext;	/* text transparency flag */
int             _cgi_pix_drawing_mode;
u_char         *_cgi_disjoint_mvlist;
Ccombtype       _cgi_bit_drawing_mode;
Cbmode          _cgi_bit_vis_mode;
Cbitmaptype     _cgi_bit_source;
Cbitmaptype     _cgi_bit_dest;
Cacttype        _cgi_clear_mode;
Cexttype        _cgi_clear_extent;
int             _cgi_input_devices;	/* are input devices used */
int             _cgi_num_events;/* event queue pointer */
Cqtype          _cgi_que_status;/* event queue status */
Ceqflow         _cgi_que_over_status;	/* event queue status */
int             _cgi_trigger[MAXTRIG][MAXASSOC + 1];	/* association list */
Outatt         *_cgi_att;	/* structure containing current attributes */
int             _cgi_event_occurred;	/* event flag */
struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];
struct pickstr *_cgi_pick[_CGI_PICKS];
Cint           *_cgi_pattern_table[MAXNUMPATS];	/* pattern table */
caddr_t       (*_cgi_win_get)();        /* -> "window_get" if canvas call */

Gstate          _cgi_state =
{				/* CGI global state */
    CGCL,			/* state */
    0,				/* cgipw_mode */
    (Notify_client) NULL,	/* notifier_client_handle */
    {
	NO_FONT, (struct pixfont *) NULL
    },				/* open_font */
    0,				/* gp1resetcnt */
    1, 1,			/* vdc_change_cnt, text_change_cnt */
    (Outatt *) NULL,		/* common_att */
};

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: open_cgi	 					    */
/*                                                                          */
/*		initializes cgi and selects viewsurface			    */
/****************************************************************************/

Cerror          open_cgi()
{
    int             err;

    err = _cgi_open_cgi(0, 0);	/* cgipw_mode==0, use_pw_size==0 */
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_open_cgi	 					    */
/*                                                                          */
/*		initializes cgi and selects viewsurface			    */
/****************************************************************************/

Cerror          _cgi_open_cgi(cgipw_mode, use_pw_size)
int             cgipw_mode;	/* CGIPW mode: block certain accesses */
int             use_pw_size;	/* flag in Outatt structure for pixwin
				 * coordinates */
{
    register u_char *mvP;
    register Outatt *attP;
    register int    cnt;
    Cerror          err;
    static VdcRect  vdc_norm_rect = {0, 0, 32767, 32767};
    static VdcRect  vdc_pw_rect = {0, 0, 0, 0};

    if (_cgi_state.state == CGCL)
	err = NO_ERROR;
    else
	err = ENOTCGCL;

    /*
     * First, try allocating everything we'll be using.  If any fail, free
     * those already allocated, and return without changing anything more.
     */
    _cgi_disjoint_mvlist = (u_char *) malloc(MAXPTS * sizeof(u_char));
    if (_cgi_disjoint_mvlist == (u_char *) NULL)
	err = EMEMSPAC;
    else
    {
	_cgi_state.common_att = (Outatt *) calloc(1, sizeof(Outatt));
	if (_cgi_state.common_att == (Outatt *) NULL)
	{
	    err = EMEMSPAC;
	    free((char *) _cgi_disjoint_mvlist);
	    _cgi_disjoint_mvlist = NULL;
	}
    }

    if (!err)
    {
	/*
	 * Construct a move list for a disjoint polyline.  We can delete this
	 * code when pixrects supports POLY_DISJOINT.
	 */
	mvP = _cgi_disjoint_mvlist;
	for (cnt = MAXPTS; cnt >= 2; cnt -= 2)
	{
	    *mvP++ = 1;
	    *mvP++ = 0;
	}

	/*
	 * The first flag has a special meaning: close all sub-polygons. We
	 * need to clear it explicitly to prevent this.
	 */
	*_cgi_disjoint_mvlist = 0;

	/* Initialize all data that is once-per-CGI */
	_cgi_state.state = CGOP;
	_cgi_state.cgipw_mode = cgipw_mode;
	_cgi_state.notifier_client_handle = (Notify_client) & _cgi_state;
	_cgi_state.open_font.num = NO_FONT;
	_cgi_state.open_font.ptr = (struct pixfont *) NULL;
	_cgi_state.gp1resetcnt = 0;
	_cgi_state.vdc_change_cnt = 1;
	_cgi_state.text_change_cnt = 1;

	_cgi_att = attP = _cgi_state.common_att;
	_cgi_vws = (View_surface *) NULL;
	_cgi_vwsurf_count = 0;
	attP->vdc.clip.indicator = CLIP;
	attP->vdc.use_pw_size = use_pw_size;
	if (use_pw_size)
	{
	    attP->vdc.rect = vdc_pw_rect;
	    attP->vdc.clip.rect = vdc_pw_rect;
	}
	else
	{
	    attP->vdc.rect = vdc_norm_rect;
	    attP->vdc.clip.rect = vdc_norm_rect;
	}
	_cgi_ttext = 1;		/* text transparency flag = TRANSPARENT */
	_cgi_pix_drawing_mode = _cgi_pix_mode = PIX_SRC;
	_cgi_bit_drawing_mode = _cgi_drawing_mode = REPLACE;
	_cgi_bit_vis_mode = OPAQUE;
	_cgi_bit_source = BITTRUE;
	_cgi_bit_dest = BITTRUE;
	/* malloc_debug(2);  */
	_cgi_event_occurred = 0;/* event flag */
	_cgi_win_get = (caddr_t (*)()) NULL;   /* loaded by 'open_cgi_canvas' */

	/* Install signal catchers */
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       _cgi_hangup, SIGHUP, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       _cgi_interrupt, SIGINT, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       _cgi_winsig, SIGWINCH, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       _cgi_ioint, SIGIO, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       _cgi_sigxcpu, SIGXCPU, NOTIFY_ASYNC);
	_cgi_initialize_internals();
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_initialize_internals	 			    */
/*                                                                          */
/*		_cgi_initialize_internals startup variables.		    */
/****************************************************************************/
_cgi_initialize_internals()
{
    int             i;

    _cgi_clear_mode = CLEAR;
    _cgi_clear_extent = VIEWSURFACE;
    _cgi_input_devices = -1;	/* don't input devices unless they're used */
    for (i = 0; i < _CGI_KEYBORDS; i++)
	_cgi_keybord[i] = (struct keybstr *) NULL;
/*
	for(i = 0; i < _CGI_LOCATORS; i++)
		_cgi_locator[i].subloc.enable = L_FALSE;
	for(i = 0; i < _CGI_STROKES; i++)
		_cgi_stroker[i].substroke.enable = L_FALSE;
	for(i = 0; i < _CGI_VALUATRS; i++)
		_cgi_valuatr[i].subval.enable = L_FALSE;
	for(i = 0; i < _CGI_CHOICES; i++)
		_cgi_button[i].subchoice.enable = L_FALSE;
 */
    for (i = 0; i < MAXTRIG; i++)
	_cgi_trigger[i][0] = 0;
    _cgi_num_events = 0;	/* no pending events */
    _cgi_que_status = EMPTY;
    _cgi_que_over_status = NO_OFLO;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: close_cgi	 					    */
/*                                                                          */
/*		terminates cgi and deselects viewsurface		    */
/****************************************************************************/
Cerror          close_cgi()
{
    Cerror          err;

    if (_cgi_state.cgipw_mode)
	err = ENOTCCPW;
    else
	err = _cgi_close_cgi();
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_close_cgi	 					    */
/*                                                                          */
/*		terminates cgi and deselects viewsurface		    */
/****************************************************************************/
Cerror          _cgi_close_cgi()
{
    int             i, err;

    if (_cgi_state.state != CGCL)
	err = NO_ERROR;
    else
	err = ENOTOPOP;
    if (!err)
    {
	(void) flush_event_queue();

	/* close all view surfaces */
	for (i = 0; i < _cgi_vwsurf_count; i++)
	{
	    if (_cgi_view_surfaces[i] != (View_surface *) NULL)
		(void) close_vws(i);
	}

	/* free malloc'd globals */
	if (_cgi_state.open_font.ptr != (struct pixfont *) NULL)
	{
	    (void) pf_close(_cgi_state.open_font.ptr);
	    _cgi_state.open_font.ptr = (struct pixfont *) NULL;
	    _cgi_state.open_font.num = NO_FONT;
	}
	_cgi_free_output_att(_cgi_state.common_att);
	_cgi_state.common_att = (Outatt *) NULL;
	free((char *) _cgi_disjoint_mvlist);
	_cgi_disjoint_mvlist = (u_char *) NULL;

	/* free input devices */
	for (i = 0; i < _CGI_LOCATORS; i++)
	{
	    if (_cgi_locator[i])
	    {
		free((char *) _cgi_locator[i]);
		_cgi_locator[i] = (struct locatstr *) NULL;
	    }
	}
	for (i = 0; i < _CGI_KEYBORDS; i++)
	{
	    if (_cgi_keybord[i])
	    {
		free((char *) _cgi_keybord[i]->initstring);
		free((char *) _cgi_keybord[i]->rinitstring);
		free((char *) _cgi_keybord[i]);
		_cgi_keybord[i] = (struct keybstr *) NULL;
	    }
	}
	for (i = 0; i < _CGI_STROKES; i++)
	{
	    if (_cgi_stroker[i])
	    {
		free((char *) _cgi_stroker[i]->sarray->ptlist);
		free((char *) _cgi_stroker[i]->sarray);
		free((char *) _cgi_stroker[i]->rsarray->ptlist);
		free((char *) _cgi_stroker[i]->rsarray);
		free((char *) _cgi_stroker[i]);
		_cgi_stroker[i] = (struct strokstr *) NULL;
	    }
	}
	for (i = 0; i < _CGI_VALUATRS; i++)
	{
	    if (_cgi_valuatr[i])
	    {
		free((char *) _cgi_valuatr[i]);
		_cgi_valuatr[i] = (struct valstr *) NULL;
	    }
	}
	for (i = 0; i < _CGI_CHOICES; i++)
	{
	    if (_cgi_button[i])
	    {
		free((char *) _cgi_button[i]);
		_cgi_button[i] = (struct choicestr *) NULL;
	    }
	}
	for (i = 0; i < _CGI_PICKS; i++)
	{
	    if (_cgi_pick[i])
	    {
		free((char *) _cgi_pick[i]);
		_cgi_pick[i] = (struct pickstr *) NULL;
	    }
	}

	/* free other malloc'd private data */
	for (i = 0; i < MAXNUMPATS; i++)
	{
	    if (_cgi_pattern_table[i])
	    {
		free((char *) _cgi_pattern_table[i]);
		_cgi_pattern_table[i] = (Cint *) NULL;
	    }
	}
	_cgi_free_text_cache();

	_cgi_state.state = CGCL;

	/* Clear notifier actions */
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       (Notify_func) 0, SIGHUP, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       (Notify_func) 0, SIGINT, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       (Notify_func) 0, SIGWINCH, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       (Notify_func) 0, SIGIO, NOTIFY_ASYNC);
	notify_set_signal_func(_cgi_state.notifier_client_handle,
			       (Notify_func) 0, SIGXCPU, NOTIFY_ASYNC);
    }

    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_alloc_output_att					    */
/*                                                                          */
/*          Allocate a new Outatt structure, possibly initialize it,	    */
/*          and return a pointer to it.					    */
/****************************************************************************/
Outatt         *_cgi_alloc_output_att(initial_value)
Outatt         *initial_value;
{
    int             cnt;
    Outatt         *attP;

    attP = (Outatt *) calloc(1, sizeof(Outatt));
    if (attP != (Outatt *) NULL)
    {
	if (initial_value != (Outatt *) NULL)
	{
	    *attP = *initial_value;
	    attP->text.astring = (char *) NULL;
	    for (cnt = 0; cnt < MAXAESSIZE; cnt++)
		attP->aes_table[cnt] = (Cbunatt *) NULL;
	}
    }
    return (attP);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_free_output_att                                       */
/*                                                                          */
/*          Frees a cgi output att structure.                               */
/*          Caller should set his output att structure pointer to NULL.     */
/****************************************************************************/
_cgi_free_output_att(att)
Outatt         *att;
{
    int             i;

    if (att != NULL)
    {
	for (i = 0; (i < MAXAESSIZE) && (att->aes_table[i]); i++)
	{
	    free((char *) att->aes_table[i]);
	    att->aes_table[i] = (Cbunatt *) NULL;	/* Don't free again */
	}
	if (att->text.astring != (char *) NULL)
	{
	    free(att->text.astring);
	    att->text.astring = (char *) NULL;
	}
	/* free (att->pattern.point); Never, ever malloc'd! */
	cfree(att);
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_hangup 						    */
/*                                                                          */
/*		terminates cgi in response to SIGHUP			    */
/****************************************************************************/
Notify_value    _cgi_hangup()
{
    if (_cgi_state.cgipw_mode)
	(void) close_pw_cgi();
    else
	(void) close_cgi();
    exit(0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_interrupt						    */
/*                                                                          */
/*		terminates cgi in response to SIGINT			    */
/****************************************************************************/
Notify_value    _cgi_interrupt()
{
    if (_cgi_state.cgipw_mode)
	(void) close_pw_cgi();
    else
	(void) close_cgi();
    exit(0);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_up_sigwinch	 				    */
/*                                                                          */
/****************************************************************************/
Cerror          set_up_sigwinch(name, sig_function)
int             name;
int             (*sig_function) ();	/* signal handling function */
{
    int             err = _cgi_check_state_5();

    if (!err  &&  !(err = _cgi_legal_winname(name)) )
	_cgi_view_surfaces[name]->sunview.sig_function = sig_function;
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_sigxcpu			 			    */
/*                                                                          */
/*		handle signal SIGXCPU (possibly from gp1) for CGI.	    */
/****************************************************************************/
Notify_value    _cgi_sigxcpu()
{
    register View_surface *vwsP;
    static int      ocnt, gpsentsignal;
    int             i;

    gpsentsignal = 0;
    /* Receive this signal whenever kernel resets GP */
    /* Must assume that all GP state has been lost for this process */
    for (i = 0; i <= MAXVWS; i++)
    {
	vwsP = _cgi_view_surfaces[i];
	if (vwsP == NULL)
	    continue;		/* NULL view surface pointer, try next one */
	if (vwsP->sunview.gp_att != (Gp1_attr *) NULL)
	{
	    ocnt = vwsP->sunview.gp_att->resetcnt;
	    (void) ioctl(gp1_d(vwsP->sunview.pw->pw_pixrect)->ioctl_fd,
		   GP1IO_GET_RESTART_COUNT, (char *) &_cgi_state.gp1resetcnt);
	    if (_cgi_state.gp1resetcnt != ocnt)
	    {
		vwsP->sunview.gp_att->resetcnt = _cgi_state.gp1resetcnt;
		gpsentsignal = 1;
	    }
	}
    }
    /* if SIGXCPU was not sent by the GP pass it on */
    if (!gpsentsignal)
    {
	(void) fprintf(stderr, "SIGXCPU signal received, not as a result of the Graphics Processor restarting ... exiting.\n");
	exit(1);
    }

    return (NOTIFY_DONE);
}
