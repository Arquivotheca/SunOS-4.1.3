#ifndef lint
static char	sccsid[] = "@(#)input.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Input functions 
 */

/*
initialize_lid
release_input_device
track_on
track_off
set_initial_value
set_valuator_range
request_input
_cgi_err_check_inrep
_cgi_err_check_echo 
*/

#include "cgipriv.h"
#include <signal.h>

View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* current attributes */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
int             _cgi_vwsurf_count;	/* number of view surfaces */
int             _cgi_num_chars;
Outatt         *_cgi_att;	/* structure containing current attributes */
struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];
struct pickstr *_cgi_pick[_CGI_PICKS];
int             _cgi_event_occurred;	/* event flag */
int             _cgi_alarm_occurred;	/* alarm flag */

extern char    *strcpy(), *strncpy();;

/* lastdefaultcursor is the cursor in use before track_on was issued */
static short default_cursor_shape[16];
mpr_static(_cgi_last_defcursor, 16, 16, 1, default_cursor_shape);	
static struct cursor lastdefaultcursor =
{
 0, 15, PIX_SRC ^ PIX_DST, &_cgi_last_defcursor
};

static short    cursor_shape[] =
{
 0x0020, 0x0070, 0x009c, 0x0106,
 0x0083, 0x01c9, 0x03e2, 0x07fe,
 0x0ff0, 0x1ff0, 0x3ff0, 0x3fe0,
 0x77e0, 0x6140, 0xe000, 0xc000
};
mpr_static(_cgi_cursor_mpr, 16, 16, 1, cursor_shape);
static struct cursor _cgi_cursor =
{
 0, 15, PIX_SRC ^ PIX_DST, &_cgi_cursor_mpr
};
static struct cursor _cgi_cursor_null =
{
 0, 15, PIX_SRC ^ PIX_DST, NULL
};

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: initialize_lid					   	    */
/*                                                                          */
/*		initializes specific input devices.			    */
/****************************************************************************/

Cerror          initialize_lid(devclass, devnum, ival)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Cinrep         *ival;		/* initial value of device measure */

{
    int             fd, i, terr;
    struct screen   screen;
    static struct keybstr *keybptr;
    static struct strokstr *strokptr;
    static struct locatstr *locatptr;
    static struct valstr *valptr;
    static struct choicestr *butnptr;
    static struct pickstr *pickptr;
    static struct device idev =
    {
     NO_EVENTS, 1, 0, 0, ACK_OFF
    };

    /* Check that the input device exists (err 80 == EINDNOEX) */
    terr = _cgi_err_check_480(devclass, devnum);
    if (!terr && (devclass != IC_VALUATOR))
	terr = _cgi_err_check_inrep(devclass, devnum, ival);
    if (!terr)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (_cgi_keybord[devnum - 1])
	    {
		if (_cgi_keybord[devnum - 1]->subkey.enable != RELEASE)
		    terr = EINDALIN;
	    }
	    else
	    {
		keybptr = (struct keybstr *) malloc(sizeof(struct keybstr));
		keybptr->initstring =
		    (char *) malloc((MAXCHAR + 1) * sizeof(char));
		keybptr->rinitstring =
		    (char *) malloc((MAXCHAR + 1) * sizeof(char));
	    }
	    if (terr == 0)
	    {
		keybptr->subkey = idev;
		_cgi_keybord[devnum - 1] = keybptr;
		(void) strcpy(_cgi_keybord[devnum - 1]->initstring,
			      ival->string);
		(void) strcpy(_cgi_keybord[devnum - 1]->rinitstring,
			      ival->string);
	    }
	    break;
	case IC_STROKE:
	    if (_cgi_stroker[devnum - 1])
	    {
		if (_cgi_stroker[devnum - 1]->substroke.enable != RELEASE)
		    terr = 82;
	    }
	    else
	    {
		strokptr = (struct strokstr *)
		    malloc(sizeof(struct strokstr));
		strokptr->sarray = (Ccoorlist *) malloc(sizeof(Ccoorlist));
		strokptr->sarray->ptlist =
		    (Ccoor *) malloc(MAXPTS * sizeof(Ccoor));
		strokptr->rsarray = (Ccoorlist *) malloc(sizeof(Ccoorlist));
		strokptr->rsarray->ptlist =
		    (Ccoor *) malloc(MAXPTS * sizeof(Ccoor));
	    }
	    if (terr == 0)
	    {
		strokptr->substroke = idev;
		_cgi_stroker[devnum - 1] = strokptr;
		for (i = 0; i < ival->points->n; i++)
		{
		    _cgi_stroker[devnum - 1]->sarray->ptlist[i] =
			ival->points->ptlist[i];
		    _cgi_stroker[devnum - 1]->rsarray->ptlist[i] =
			ival->points->ptlist[i];
		}
		_cgi_stroker[devnum - 1]->sarray->n =
		    ival->points->n;
		_cgi_stroker[devnum - 1]->rsarray->n =
		    ival->points->n;
	    }
	    break;
	case IC_LOCATOR:
	    if (_cgi_locator[devnum - 1])
	    {
		if (_cgi_locator[devnum - 1]->subloc.enable != RELEASE)
		    terr = 82;
	    }
	    else
	    {
		locatptr = (struct locatstr *)
		    malloc(sizeof(struct locatstr));
	    }
	    if (terr == 0)
	    {
		locatptr->subloc = idev;
		_cgi_locator[devnum - 1] = locatptr;
		_cgi_locator[devnum - 1]->x = ival->xypt->x;
		_cgi_locator[devnum - 1]->y = ival->xypt->y;
		_cgi_locator[devnum - 1]->rx = ival->xypt->x;
		_cgi_locator[devnum - 1]->ry = ival->xypt->y;
	    }
	    break;
	case IC_PICK:
	    if (_cgi_pick[devnum - 1])
	    {
		if (_cgi_pick[devnum - 1]->subpick.enable != RELEASE)
		    terr = 82;
	    }
	    else
		pickptr = (struct pickstr *)
		    malloc(sizeof(struct pickstr));
	    if (terr == 0)
	    {
		pickptr->subpick = idev;
		_cgi_pick[devnum - 1] = pickptr;
		_cgi_pick[devnum - 1]->pck = ival->pick;
		_cgi_pick[devnum - 1]->rpck = ival->pick;
	    }
	    break;
	case IC_VALUATOR:
	    if (_cgi_valuatr[devnum - 1])
	    {
		if (_cgi_valuatr[devnum - 1]->subval.enable != RELEASE)
		    terr = 82;
	    }
	    else
		valptr = (struct valstr *) malloc(sizeof(struct valstr));
	    if (terr == 0)
	    {
		valptr->subval = idev;
		valptr->vlmin = 0.0;
		valptr->vlmax = 1.0;
		_cgi_valuatr[devnum - 1] = valptr;
		if ((ival->val < _cgi_valuatr[devnum - 1]->vlmin)
		    || (ival->val > _cgi_valuatr[devnum - 1]->vlmax)
		    )
		    terr = EBADDATA;
		else
		{
		    _cgi_valuatr[devnum - 1]->curval = ival->val;
		    _cgi_valuatr[devnum - 1]->rcurval = ival->val;
		}
	    }
	    break;
	case IC_CHOICE:
	    if (_cgi_button[devnum - 1])
	    {
		if (_cgi_button[devnum - 1]->subchoice.enable != RELEASE)
		    terr = 82;
	    }
	    else
		butnptr = (struct choicestr *)
		    malloc(sizeof(struct choicestr));
	    if (terr == 0)
	    {
		butnptr->subchoice = idev;
		_cgi_button[devnum - 1] = butnptr;
		_cgi_button[devnum - 1]->curval = ival->choice;
		_cgi_button[devnum - 1]->rcurval = ival->choice;
	    }
	    break;
	}

	if (!terr)
	{
	    for (i = 0; i < _cgi_vwsurf_count; i++)
	    {
		switch (_cgi_view_surfaces[i]->device)
		{
		case BW1DD:
		case BW2DD:
		case CG1DD:
		case CG2DD:
		case CG3DD:
		case CG4DD:
#ifndef	NO_CG6
		case CG6DD:
#endif	NO_CG6
		case GP1DD:
		    fd = _cgi_view_surfaces[i]->sunview.windowfd;
		    win_screenget(fd, &screen);
		    (void) strncpy(screen.scr_kbdname, "/dev/kbd",
				   SCR_NAMESIZE);
		    (void) strncpy(screen.scr_msname, "/dev/mouse",
				   SCR_NAMESIZE);
		    (void) win_setms(fd, &screen);
		    (void) win_setkbd(fd, &screen);
		    break;
		default:
		    break;
		}
	    }
	}
    }
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: release_input_device 					    */
/*                                                                          */
/*		Makes device unable to detect input until device	    */
/*              is reinitialized.                                           */
/****************************************************************************/

Cerror          release_input_device(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    int             i, terr, err;
    static struct device idev =
    {
     RELEASE, -1, 0, 0, ACK_OFF
    };
    err = _cgi_err_check_481(devclass, devnum);
    if (!err)
    {
	/* get rid of pending events */
	err = selective_flush_of_event_queue(devclass, devnum);
	/* dissociate from associated triggers */
	if (!err)
	{
	    for (i = 1; i < MAXTRIG + 1; i++)
	    {
		terr = _cgi_err_is_trig_assoc(devclass, devnum, i);
		if (terr)
		{		/* trigger is associated */
		    err = dissociate(i, devclass, devnum);
		}
	    }
	}
	if (!err)
	{
	    switch (devclass)
	    {			/* change state of device */
	    case IC_STRING:
		_cgi_keybord[devnum - 1]->subkey = idev;
		break;
	    case IC_STROKE:
		_cgi_stroker[devnum - 1]->substroke = idev;
		break;
	    case IC_LOCATOR:
		_cgi_locator[devnum - 1]->subloc = idev;
		break;
	    case IC_PICK:
		_cgi_pick[devnum - 1]->subpick = idev;
		break;
	    case IC_VALUATOR:
		_cgi_valuatr[devnum - 1]->subval = idev;
		break;
	    case IC_CHOICE:
		_cgi_button[devnum - 1]->subchoice = idev;
	    }
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  track_on			 			    */
/*                                                                          */
/*		Deterimines whether input device movement is displayed on   */
/*              the screen 				                    */
/****************************************************************************/

/* ARGSUSED WDS: As documented, the echoregion argument is ignored */
Cerror          track_on(devclass, devnum, echotype, echoregion, value)
Cdevoff         devclass;
Cint            devnum;
Cint            echotype;	/* type of echo */
Ccoorpair      *echoregion;	/* window where echo is enabled */
Cinrep         *value;		/* device value */
{
    int             terr;
    int             mousex, mousey;
    int             ivw;

    terr = _cgi_err_check_481(devclass, devnum);
    if (!terr)
	terr = _cgi_err_check_echo(devclass, devnum, echotype);
    if (!terr)
	terr = _cgi_err_check_inrep(devclass, devnum, value);
    if (!terr && value->xypt == (Ccoor *) NULL)
	terr = EBADDATA;	/* used for mouse initial position */
    if (!terr)
    {
	ivw = 0;
	(void) _cgi_bump_vws(&ivw);	/* already checked for VSAC above */
	switch (devclass)
	{
	case IC_STRING:
	    _cgi_keybord[devnum - 1]->subkey.echo = (short) echotype;
	    _cgi_keybord[devnum - 1]->echo_base =
		_cgi_keybord[devnum - 1]->echo_pt =
		_cgi_att->text.concat_pt;
/*		    win_setcursor (_cgi_vws->sunview.windowfd, &_cgi_cursor);
		    _cgi_devscale (value->xypt->x, value->xypt->y,
			mousex, mousey);
		    win_setmouseposition (_cgi_vws->sunview.windowfd, mousex, mousey); */
	    break;
	case IC_STROKE:
	    _cgi_stroker[devnum - 1]->substroke.echo =
		(short) echotype;
/*		    win_setcursor (_cgi_vws->sunview.windowfd, &_cgi_cursor);
		    _cgi_devscale (value->xypt->x, value->xypt->y,
			mousex, mousey);
		    win_setmouseposition (_cgi_vws->sunview.windowfd, mousex, mousey); */
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->subloc.echo = (short) echotype;
/*		    win_setcursor (_cgi_vws->sunview.windowfd, &_cgi_cursor);
		    _cgi_devscale (value->xypt->x, value->xypt->y,
			mousex, mousey);
		    win_setmouseposition (_cgi_vws->sunview.windowfd, mousex, mousey); */
	    break;
	case IC_PICK:
	    _cgi_pick[devnum - 1]->subpick.echo = (short) echotype;
/*		    win_setcursor (_cgi_vws->sunview.windowfd, &_cgi_cursor);
		    _cgi_devscale (value->xypt->x, value->xypt->y,
			mousex, mousey);
		    win_setmouseposition (_cgi_vws->sunview.windowfd, mousex, mousey); */
	    break;
	case IC_VALUATOR:
	    _cgi_valuatr[devnum - 1]->subval.echo = (short) echotype;
	    _cgi_valuatr[devnum - 1]->echo_flag = 0;
	    _cgi_valuatr[devnum - 1]->echo_base =
		_cgi_att->text.concat_pt;
/*		    win_setcursor (_cgi_vws->sunview.windowfd, &_cgi_cursor);
		    _cgi_devscale (value->xypt->x, value->xypt->y,
			mousex, mousey);
		    win_setmouseposition (_cgi_vws->sunview.windowfd, mousex, mousey);
	     	     	     	     	     	     	 	 */ break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->subchoice.echo =
		(short) echotype;
	    break;
	}
	win_getcursor(_cgi_vws->sunview.windowfd, &lastdefaultcursor);
	if ((devclass == IC_STRING) && (echotype == 2))/* set a NULL cursor */
		win_setcursor(_cgi_vws->sunview.windowfd, &_cgi_cursor_null);
	else
		win_setcursor(_cgi_vws->sunview.windowfd, &_cgi_cursor);
	_cgi_devscale(value->xypt->x, value->xypt->y,
		      mousex, mousey);
	win_setmouseposition(_cgi_vws->sunview.windowfd, mousex, mousey);
    }
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  track_off		 				    */
/*                                                                          */
/*		Terminates echo.					    */
/****************************************************************************/
/* ARGSUSED WDS: As documented, the tracktype & action arguments are ignored*/
Cerror          track_off(devclass, devnum, tracktype, action)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Cint            tracktype;	/* Unused arg */
Cfreeze         action;		/* Unused arg */
{
    int             terr;

    terr = _cgi_err_check_481(devclass, devnum);
    if (!terr)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (_cgi_num_chars != 0)
		_cgi_echo_dis(_cgi_keybord[devnum - 1]->subkey.echo,
			      0, 0, devclass, devnum, ERASE);
	    _cgi_keybord[devnum - 1]->subkey.echo = -1;
	    break;
	case IC_STROKE:
	    _cgi_stroker[devnum - 1]->substroke.echo = -1;
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->subloc.echo = -1;
	    break;
	case IC_PICK:
	    _cgi_pick[devnum - 1]->subpick.echo = -1;
	    break;
	case IC_VALUATOR:
	    if (_cgi_valuatr[devnum - 1]->echo_flag)
		_cgi_echo_dis(_cgi_valuatr[devnum - 1]->subval.echo,
			      0, 0, devclass, devnum, ERASE);
	    _cgi_valuatr[devnum - 1]->subval.echo = -1;
	    break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->subchoice.echo = -1;
	    break;
	}
    }
/*  set the cursor back to the default in place before the latest track_on */
    win_setcursor(_cgi_vws->sunview.windowfd, &lastdefaultcursor);
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  set_initial_value		 			    */
/*                                                                          */
/*		set measure of device.					    */
/****************************************************************************/

Cerror          set_initial_value(devclass, devnum, value)
Cdevoff         devclass;	/* device type */
Cint            devnum;		/* device number */
Cinrep         *value;		/* device value */
{
    int             terr;
    terr = _cgi_err_check_481(devclass, devnum);
    if (!terr)
	terr = _cgi_err_check_inrep(devclass, devnum, value);
    if (!terr)
    {
	switch (devclass)
	{
	case IC_STRING:
	    (void) strcpy(_cgi_keybord[devnum - 1]->initstring, value->string);
	    (void) strcpy(_cgi_keybord[devnum - 1]->rinitstring, value->string);
	    break;
	case IC_STROKE:
	    /* WCL 860911 These should copy pts, not use client's data space */
	    _cgi_stroker[devnum - 1]->sarray = value->points;
	    _cgi_stroker[devnum - 1]->rsarray = value->points;
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->x = value->xypt->x;
	    _cgi_locator[devnum - 1]->y = value->xypt->y;
	    _cgi_locator[devnum - 1]->rx = value->xypt->x;
	    _cgi_locator[devnum - 1]->ry = value->xypt->y;
	    break;
	case IC_VALUATOR:
	    _cgi_valuatr[devnum - 1]->curval = value->val;
	    _cgi_valuatr[devnum - 1]->rcurval = value->val;
	    break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->curval = value->choice;
	    _cgi_button[devnum - 1]->rcurval = value->choice;
	    break;
	}
    }
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  set_valuator_range		 			    */
/*                                                                          */
/*		Moves echo.						    */
/****************************************************************************/

Cerror          set_valuator_range(devnum, mn, mx)
Cint            devnum;		/* device number */
Cfloat          mn, mx;		/* limits of valuator */
{
    int             err;
    err = _cgi_err_check_4();
    if (!err)
    {
	if (devnum > 0 && devnum <= _CGI_VALUATRS)
	{
	    if (_cgi_valuatr[devnum - 1]->subval.enable != RELEASE)
	    {
		_cgi_valuatr[devnum - 1]->vlmin = mn;
		_cgi_valuatr[devnum - 1]->vlmax = mx;
	    }
	    else
		err = 81;
	}
	else
	    err = 80;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: request_input	 					    */
/*                                                                          */
/*		Awaits input and reports trigger number and value.	    */
/****************************************************************************/

Cerror          request_input(devclass, devnum, timeout, valid, sample, trigger)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Cint            timeout;	/* amount of time to wait for input */
Cawresult      *valid;		/* device status */
Cinrep         *sample;		/* device value */
Cint           *trigger;	/* trigger number */
{
    int             err;
    int             cnt;
    int            *trig_p;
    Clidstate      *enable_p;
/*     Ccoor pts[MAXPTS]; */

    err = _cgi_err_check_481(devclass, devnum);
    *trigger = 0;

    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (sample->string == (char *) NULL)
	    {
		err = EBADDATA;
		break;
	    }
	    enable_p = &_cgi_keybord[devnum - 1]->subkey.enable;
	    trig_p = &_cgi_keybord[devnum - 1]->trig;
	    break;
	case IC_CHOICE:
	    enable_p = &_cgi_button[devnum - 1]->subchoice.enable;
	    trig_p = &_cgi_button[devnum - 1]->trig;
	    break;
	case IC_LOCATOR:
	    if (sample->xypt == (Ccoor *) NULL)
	    {
		err = EBADDATA;
		break;
	    }
	    enable_p = &_cgi_locator[devnum - 1]->subloc.enable;
	    trig_p = &_cgi_locator[devnum - 1]->trig;
	    break;
	case IC_PICK:
	    if (sample->pick == (Cpick *) NULL)
	    {
		err = EBADDATA;
		break;
	    }
	    enable_p = &_cgi_pick[devnum - 1]->subpick.enable;
	    trig_p = &_cgi_pick[devnum - 1]->trig;
	    break;
	case IC_VALUATOR:
	    enable_p = &_cgi_valuatr[devnum - 1]->subval.enable;
	    trig_p = &_cgi_valuatr[devnum - 1]->trig;
	    break;
	case IC_STROKE:
	    if (sample->points == (Ccoorlist *) NULL)
	    {
		err = EBADDATA;
		break;
	    }
	    enable_p = &_cgi_stroker[devnum - 1]->substroke.enable;
	    trig_p = &_cgi_stroker[devnum - 1]->trig;
	    break;
	}
    }

    if (!err)
    {
	if (_cgi_event_occurred)
		(void) _cgi_ioint_handle();
	else
	{
	    /* Set an itimer for the user-specified timeout */
	    if (timeout >= 0)
		(void) _cgi_set_timeout((unsigned) timeout);

	    /* Pause loop that waits for the IO to complete. */
	    *enable_p = RESPOND_EVENT;
	    do
	    {
		if (! (_cgi_event_occurred || _cgi_alarm_occurred))
		    (void) _cgi_timeout(timeout);
		if (_cgi_event_occurred)
		    (void) _cgi_ioint_handle();
	    } while (!_cgi_alarm_occurred && *enable_p == RESPOND_EVENT);
	}

	switch (devclass)
	{
	case IC_STRING:
	    (void) strcpy(sample->string, _cgi_keybord[devnum - 1]->rinitstring);
	    break;
	case IC_CHOICE:
	    sample->choice = _cgi_button[devnum - 1]->rcurval;
	    break;
	case IC_LOCATOR:
	    sample->xypt->x = _cgi_locator[devnum - 1]->rx;
	    sample->xypt->y = _cgi_locator[devnum - 1]->ry;
	    break;
	case IC_PICK:
	    sample->pick->segid = _cgi_pick[devnum - 1]->rpck->segid;
	    sample->pick->pickid = _cgi_pick[devnum - 1]->rpck->pickid;
	    break;
	case IC_VALUATOR:
	    sample->val = _cgi_valuatr[devnum - 1]->rcurval;
	    break;
	case IC_STROKE:
	    cnt = _cgi_stroker[devnum - 1]->rsarray->n;
	    if (cnt > sample->points->n)
		cnt = sample->points->n;
	    bcopy((char *) _cgi_stroker[devnum - 1]->rsarray->ptlist,
		  (char *) sample->points->ptlist,
		  cnt * sizeof(*sample->points->ptlist));
	    break;
	}

	if (*enable_p == RESPOND_EVENT)
	    *trigger = 0;
	else
	    *trigger = *trig_p;
    }
    if (!err)
    {
	if (*trigger)
	    *valid = VALID_DATA;
	else
	    *valid = TIMED_OUT;
    }
    else
    {
	if (err == 4 || err == 81)
	    *valid = WRONG_STATE;
	if (err == 80)
	    *valid = NOT_SUPPORTED;
	if (err == 94)
	    *valid = DISABLED;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_inrep 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_inrep(devclass, devnum, ival)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Cinrep         *ival;		/* initial value of device measure */
{
    int             err;
    if (ival == (Cinrep *) NULL)
	err = EBADDATA;
    else
    {
	err = NO_ERROR;
	switch (devclass)
	{
	case IC_STRING:
	    if (ival->string == (char *) NULL)
		err = EBADDATA;
	    if (strlen(ival->string) > MAXCHAR)
		err = ESTRSIZE;
	    break;
	case IC_STROKE:
	    if (!ival->points)
		err = EBADDATA;
	    break;
	case IC_LOCATOR:
	    if (!ival->xypt)
		err = EBADDATA;
	    break;
	case IC_VALUATOR:
	    if ((ival->val < _cgi_valuatr[devnum - 1]->vlmin)
		|| (ival->val > _cgi_valuatr[devnum - 1]->vlmax))
		err = EBADDATA;
	    break;
	case IC_CHOICE:
	    /* if ((ival->choice != 0) && (ival->choice != 1)) */
	    if ((ival->choice < 2) && (ival->choice > 4))
		err = EBADDATA;
	    break;
	}
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_echo					    */
/*                                                                          */
/*                                                                          */
/*		(echo <= 0) means NO_ECHO, is okay  870319 MDH		    */
/****************************************************************************/
/* ARGSUSED The devnum argument is ignored: all devs same for one devclass. */
int             _cgi_err_check_echo(devclass, devnum, echo)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
short           echo;
{
    int             err;
    err = NO_ERROR;
    switch (devclass)
    {
    case IC_STRING:
	if (echo > 2)
	    err = EINETNSU;
	break;
    case IC_STROKE:
	if (echo > 5)
	    err = EINETNSU;
	break;
    case IC_LOCATOR:
	if (echo > 1)
	    err = EINETNSU;
	break;
    case IC_VALUATOR:
	if (echo > 2)
	    err = EINETNSU;
	break;
    case IC_CHOICE:
	if (echo > 1)
	    err = EINETNSU;
	break;
    }
    return (err);
}
