#ifndef lint
static char	sccsid[] = "@(#)cgisigi.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Input signal functions 
 */

/*
_cgi_ioint
_cgi_ioint_handle
_cgi_new_queue
_cgi_temp_assoc_stroke
_cgi_temp_dissoc_stroke
_cgi_echo_dis
*/

#include "cgipriv.h"
#include <signal.h>
#include <stdio.h>
#include <sunwindow/window_hs.h>
#include <sunwindow/notify.h>

#define	ITIMER_NULL	((struct itimerval *) NULL)

int            *(*_cgi_pick_function) ();	/* pick handling function */
View_surface   *_cgi_vws;	/* current view surface */
Outatt         *_cgi_att;	/* structure containing current attributes */
Gstate _cgi_state;		/* CGI global state */
int             _cgi_pix_mode;	/* pixrect equivalent of drawing mode */
struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];
struct pickstr *_cgi_pick[_CGI_PICKS];
struct evqueue  _cgi_event_queue[MAXEVENTS];	/* event queue */
int             _cgi_num_events;/* event queue pointer */
int             _cgi_num_stroke_points;	/* number of stroke points */
int             _cgi_num_chars;	/* number of stroke points */
Cqtype _cgi_que_status;		/* event queue status */
Ceqflow _cgi_que_over_status;	/* event queue status */
int             _cgi_trigger[MAXTRIG][MAXASSOC + 1];	/* association list */
int             _cgi_ttext;
int             _cgi_event_occurred;	/* event flag */
static Ccoorlist *tstroke;	/* wds added static 850712 */
static Ccoor   *tarray;

extern char    *sprintf();

int             _cgi_alarm_occurred;
static struct timeval  _cgi_newtime;
#define	bits_in(n)	( 8 * sizeof(n))

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_timeout 					            */
/*                                                                          */
/*		Set a flag after a given number of seconds, using select(2).*/
/*		Guaranteed not to set the flag until the requested time	    */
/*		has elapsed, even if early return due to other signal	    */
/*		Returns to caller upon receipt of any other signal.	    */
/*		If timeflag is non-negative, _cgi_set_timeout MUST have     */
/*		been called before any call to _cgi_timeout.		    */
/****************************************************************************/

Notify_value _cgi_timeout(timeflag)
int	timeflag;	/* flag used to differentiate FOREVER blocking */ 
{
    struct timeval  curtime, delay;
    struct timeval  *pdelay = &delay;
    struct timezone zone;
    int	wahapn = 0;

    if (timeflag == 0)	/* don't block, set flag and return */
	_cgi_alarm_occurred = 1;
    else
    if (timeflag < 0)	/* block "forever" */
	(void) select(bits_in(int), NULL, NULL, NULL, NULL);
    else		/* timeout block */
    {
	(void) gettimeofday(&curtime, &zone);

	if ( curtime.tv_sec < _cgi_newtime.tv_sec
	    || (curtime.tv_sec == _cgi_newtime.tv_sec
	    &&  curtime.tv_usec < _cgi_newtime.tv_usec) )
	{
	    delay.tv_sec = _cgi_newtime.tv_sec - curtime.tv_sec;
	    delay.tv_usec = _cgi_newtime.tv_usec - curtime.tv_usec;
	    if (delay.tv_usec < 0)
	    {
		delay.tv_usec += 1000000;
		delay.tv_sec--;
	    }
	    wahapn = select(bits_in(int), NULL, NULL, NULL, pdelay);
	}
	if (wahapn == 0) _cgi_alarm_occurred = 1;
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_set_timeout					    */
/*                                                                          */
/*		Sets up for calls to _cgi_timeout().  			    */
/*		_cgi_set_timeout will set the flag and return immediately   */
/*		if the timeout is zero.  That condition may not truly be a  */
/*		timed-out situation.	    				    */
/****************************************************************************/

_cgi_set_timeout(delay)
unsigned int	delay;
{
    struct timeval  curtime;
    struct timezone zone;

    if (delay > 0)
    {
	_cgi_newtime.tv_sec = 0;
	(void) gettimeofday(&curtime, &zone);

	_cgi_newtime.tv_usec = curtime.tv_usec + (delay % 1000000);
	if (_cgi_newtime.tv_usec >= 1000000)
	{
	    _cgi_newtime.tv_sec++;
	    _cgi_newtime.tv_usec -= 1000000;
	}
	_cgi_newtime.tv_sec += curtime.tv_sec + (delay / 1000000);
	_cgi_alarm_occurred = 0;
    }
    else
	_cgi_alarm_occurred = 1;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ioint 					            */
/*                                                                          */
/*		Handles input interrupts asynchronously			    */
/****************************************************************************/

Notify_value _cgi_ioint()
{
    int             oldsigmask;

    oldsigmask = sigblock((1 << (SIGIO - 1)));
    _cgi_event_occurred++;
    (void) sigsetmask(oldsigmask);
    return (NOTIFY_DONE);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_ioint_handle				            */
/*                                                                          */
/****************************************************************************/

int             _cgi_ioint_handle()
{
    int             rfd /* , readfds */ ;
    struct inputevent event;
    struct inputmask imask;
    int             designee;
    int             trig, x, y;
    int             x1, y1;
    int             oldsigmask;
    char            data1;
    int             code;
    int             tmp, j, tst;
    Cdevoff         devclass;
    Cint            devnum;	/* device type, device number */
    static Cinrep   sample;
    int             vdc_width;
    struct locatstr *locptr;
    struct keybstr *keyptr;
    struct strokstr *strptr;
    struct valstr  *valptr;
    struct choicestr *butptr;
    struct pickstr *picptr;

    oldsigmask = sigblock((1 << (SIGIO - 1)));
    /* process i/o interrupt */
    if (_cgi_event_occurred)
    {
      _cgi_event_occurred = 0;/* prevents count from being set to -1 */
      if (_cgi_att == (Outatt *) NULL)
	  _cgi_att = _cgi_state.common_att;
      if (_cgi_vws != (View_surface *) NULL)
      {				/* else skip: don't bother if no windowfd */
	rfd = _cgi_vws->sunview.windowfd;	/* WDS: Not necessarily the
						 * correct windowfd */
      /* Should ioctl on every windowfd, without disturbing CGI context. */
	win_getinputmask(rfd, &imask, &designee);
	while (!input_readevent(rfd, &event))
	{			/* process all input events */
	    code = event.ie_code;
	    switch (code)
	    {
	    case LOC_MOVE:
		trig = 5;
		x = event.ie_locx;
		y = event.ie_locy;
		break;
	    case LOC_STILL:
		trig = 6;
		x = event.ie_locx;
		y = event.ie_locy;
		break;
	    case MS_LEFT:
		trig = 2;
		x = event.ie_locx;
		y = event.ie_locy;
		break;
	    case MS_MIDDLE:
		trig = 3;
		x = event.ie_locx;
		y = event.ie_locy;
		break;
	    case MS_RIGHT:
		trig = 4;
		x = event.ie_locx;
		y = event.ie_locy;
		break;
	    default:
		trig = 1;
		if (event.ie_code <= ASCII_LAST)
		{
		    if (!win_inputnegevent(&event))
		    {
			data1 = (char) event.ie_code;
		    }
		}
		break;
	    }
	    /* get associated devices */
	    tmp = _cgi_trigger[trig - 1][0];
	    tmp++;
	    for (j = 1; j < tmp; j++)
	    {			/* process all associated triggers */
		tst = _cgi_trigger[trig - 1][j];
		_cgi_int_rev_dev_conv(tst, &devclass, &devnum);
		switch (devclass)
		{
		case IC_STRING:
		    keyptr = _cgi_keybord[devnum - 1];
		    switch (keyptr->subkey.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case REQUEST_EVENT:
		    case RESPOND_EVENT:
			if (data1 != '\' && _cgi_num_chars < MAXCHAR)
			{
			    keyptr->rinitstring[_cgi_num_chars++] = data1;
			    keyptr->rinitstring[_cgi_num_chars] = '\0';
			    if (keyptr->subkey.enable == RESPOND_EVENT)
			    {
				keyptr->initstring[_cgi_num_chars - 1] = data1;
				keyptr->initstring[_cgi_num_chars] = '\0';
			    }
			    _cgi_echo_dis(keyptr->subkey.echo, x, y,
					  devclass, devnum, DRAW);
			}
			else
			{	/* done */
			    _cgi_echo_dis(keyptr->subkey.echo, x, y,
					  devclass, devnum, ERASE);
			    keyptr->trig = trig;
			    keyptr->subkey.enable = NO_EVENTS;
			    _cgi_num_chars = 0;
			}
			break;
		    case QUEUE_EVENT:
			if (_cgi_num_chars == 0)
			{
			    sample.string = (char *) malloc(
							    (MAXCHAR + 1) *
							    sizeof(char)
				);
			}
			if (data1 != '\' && _cgi_num_chars < MAXCHAR)
			{
			    sample.string[_cgi_num_chars] = data1;
			    keyptr->rinitstring[_cgi_num_chars] = data1;
			    _cgi_num_chars++;
			    sample.string[_cgi_num_chars] = '\0';
			    keyptr->rinitstring[_cgi_num_chars] = '\0';
			    _cgi_echo_dis(keyptr->subkey.echo, x, y,
					  devclass, devnum, DRAW);
			}
			else
			{	/* add to ev queue */
			    _cgi_echo_dis(keyptr->subkey.echo, x, y,
					  devclass, devnum, ERASE);

			    /*
			     * This copies sample into the queue, including its
			     * allocated string space which will later be freed
			     * by _cgi_free_queue_element. 
			     */
			    _cgi_new_queue(devclass, devnum, &sample);
			    sample.string = (char *) NULL;
			    _cgi_num_chars = 0;
			}
			break;
		    }
		    break;
		case IC_CHOICE:
		    butptr = _cgi_button[devnum - 1];
		    switch (butptr->subchoice.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case RESPOND_EVENT:
		    case REQUEST_EVENT:
			butptr->rcurval =
			    butptr->trig = trig;
			if (butptr->subchoice.enable == RESPOND_EVENT)
			{
			    butptr->curval = trig;
			}
			_cgi_echo_dis(butptr->subchoice.echo, x, y,
				      devclass, devnum, DRAW);
			butptr->subchoice.enable = NO_EVENTS;
			break;
		    case QUEUE_EVENT:
			_cgi_echo_dis(butptr->subchoice.echo, x, y,
				      devclass, devnum, DRAW);
			sample.choice = trig;
			_cgi_new_queue(devclass, devnum, &sample);
			/* add to ev queue */
			break;
		    }
		    break;
		case IC_PICK:
		    picptr = _cgi_pick[devnum - 1];
		    switch (picptr->subpick.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case REQUEST_EVENT:
		    case RESPOND_EVENT:
			_cgi_echo_dis(picptr->subpick.echo, x, y,
				      devclass, devnum, DRAW);
			_cgi_rev_devscale(x, y, x, y);
			picptr->rpck = (Cpick *)
			    (*_cgi_pick_function) (x, y);
			if (picptr->subpick.enable == RESPOND_EVENT)
			{
			    picptr->pck = picptr->rpck;
			}
			picptr->subpick.enable = NO_EVENTS;
			break;
		    case QUEUE_EVENT:
			_cgi_echo_dis(picptr->subpick.echo, x, y,
				      devclass, devnum, DRAW);
			_cgi_rev_devscale(x, y, x, y);
			sample.pick = (Cpick *) (*_cgi_pick_function) (x, y);
			_cgi_new_queue(devclass, devnum, &sample);
			/* add to ev queue */
			break;
		    }
		    break;
		case IC_LOCATOR:
		    locptr = _cgi_locator[devnum - 1];
		    switch (locptr->subloc.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case REQUEST_EVENT:
		    case RESPOND_EVENT:
			_cgi_rev_devscale(locptr->rx, locptr->ry, x, y);
			/*
			 * This uses "current" transform, even though input is
			 * relative to the window (master pw) and not the
			 * pw_region. 
			 */
			locptr->trig = trig;
			if (locptr->subloc.enable == RESPOND_EVENT)
			{
			    locptr->x = locptr->rx;
			    locptr->y = locptr->ry;
			}
			_cgi_echo_dis(locptr->subloc.echo, x, y,
				      devclass, devnum, DRAW);
			locptr->subloc.enable = NO_EVENTS;
			break;
		    case QUEUE_EVENT:
			_cgi_echo_dis(locptr->subloc.echo, x, y,
				      devclass, devnum, DRAW);
			sample.xypt = (Ccoor *) malloc(sizeof(Ccoor));
			_cgi_rev_devscale(sample.xypt->x, sample.xypt->y,
					  x, y);
			/* add to ev queue */
			_cgi_new_queue(devclass, devnum, &sample);
			break;
		    }
		    break;
		case IC_VALUATOR:
		    /*
		     * wds 850617: viewport coords are "VDC" for pixwins
		     * surfaces 
		     */
		    if (_cgi_att->vdc.use_pw_size)
		    {
			if (_cgi_vws != (View_surface *) NULL)
			    vdc_width = _cgi_vws->vport.r_width;
			else
			    vdc_width = 32768;
		    }
		    else
		    {
			vdc_width = _cgi_att->vdc.rect.r_width;
		    }
		    /*
		     * Now, merely use vdc_... where we used to use
		     * _cgi_vdc_... 
		     */
		    valptr = _cgi_valuatr[devnum - 1];
		    switch (valptr->subval.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case REQUEST_EVENT:
		    case RESPOND_EVENT:
			/* change measure of the device */
			_cgi_rev_devscale(x1, y1, x, y);
			sample.val = ((float) x1 / (float) vdc_width)
			    * (valptr->vlmax - valptr->vlmin);
			sample.val += valptr->vlmin;
			valptr->rcurval = sample.val;
			valptr->trig = trig;
			if (valptr->subval.enable == RESPOND_EVENT)
			{
			    valptr->curval = sample.val;
			}
			_cgi_echo_dis(valptr->subval.echo, x, y,
				      devclass, devnum, DRAW);
			valptr->subval.enable = NO_EVENTS;
			break;
		    case QUEUE_EVENT:
			_cgi_rev_devscale(x1, y1, x, y);
			sample.val = ((float) x1 / (float) vdc_width)
			    * (valptr->vlmax - valptr->vlmin);
			sample.val += valptr->vlmin;
			_cgi_echo_dis(valptr->subval.echo, x, y,
				      devclass, devnum, DRAW);
			/* add to ev queue */
			_cgi_new_queue(devclass, devnum, &sample);
			break;
		    }
		    break;
		case IC_STROKE:/* stroke device starts recording  points when
				 * a mouse button is depressed; stops recording
				 * when button is released */
		    strptr = _cgi_stroker[devnum - 1];
		    switch (strptr->substroke.enable)
		    {
		    case RELEASE:
		    case NO_EVENTS:
			break;
		    case REQUEST_EVENT:
		    case RESPOND_EVENT:
			if (_cgi_num_stroke_points <= 0)
			{
			    /* start stroke array */
			    strptr->rsarray->n = 0;
			    if (strptr->substroke.enable == RESPOND_EVENT)
			    {
				/*
				 * in here because already called by
				 * initiate_request 
				 */
				_cgi_temp_assoc_stroke(devnum);
				strptr->sarray->n =
				    _cgi_num_stroke_points = 0;
			    }
			    strptr->trig = trig;
			}
			_cgi_rev_devscale(x1, y1, x, y);
			strptr->rsarray->ptlist
			    [_cgi_num_stroke_points].x = x1;
			strptr->rsarray->ptlist
			    [_cgi_num_stroke_points].y = y1;
			strptr->rsarray->n =
			    _cgi_num_stroke_points;
			if (strptr->substroke.enable == RESPOND_EVENT)
			{
			    strptr->sarray->ptlist
				[_cgi_num_stroke_points].x = x1;
			    strptr->sarray->ptlist
				[_cgi_num_stroke_points].y = y1;
			    strptr->sarray->n =
				_cgi_num_stroke_points;
			}
			_cgi_echo_dis(strptr->substroke.echo,
				      x1, y1, devclass, devnum, DRAW);
			_cgi_num_stroke_points++;
			if (win_inputnegevent(&event)
			    || (_cgi_num_stroke_points >= MAXPTS)
			    )
			{
			    _cgi_temp_dissoc_stroke(devnum);
			    _cgi_num_stroke_points = 0;
			    strptr->substroke.enable = NO_EVENTS;
			}
			break;
		    case QUEUE_EVENT:
			if (_cgi_num_stroke_points <= 0)
			{
			    /* malloc stroke array */
			    _cgi_num_stroke_points = 0;
			    tstroke = (Ccoorlist *) malloc(sizeof(Ccoorlist));
			    tarray = (Ccoor *) malloc(MAXPTS * sizeof(Ccoor));
			    _cgi_temp_assoc_stroke(devnum);
			    sample.points = tstroke;
			    tstroke->ptlist = tarray;
			    tstroke->n = _cgi_num_stroke_points;
			}
			_cgi_rev_devscale(x1, y1, x, y);
			tarray[_cgi_num_stroke_points].x = x1;
			tarray[_cgi_num_stroke_points].y = y1;
			_cgi_echo_dis(strptr->substroke.echo,
				      x1, y1, devclass, devnum, DRAW);
			_cgi_num_stroke_points++;
			tstroke->n = _cgi_num_stroke_points;
			if (win_inputnegevent(&event)
			    || (_cgi_num_stroke_points >= MAXPTS)
			    )
			{
			    tstroke->ptlist = tarray;
			    tstroke->n = _cgi_num_stroke_points;
			    _cgi_temp_dissoc_stroke(devnum);
			    _cgi_num_stroke_points = 0;
			    sample.points = tstroke;
			    /* add to ev queue */
			    _cgi_new_queue(devclass, devnum, &sample);
			}
			break;
		    }
		    break;
		}
	    }
	}
      }
    }
    (void) sigsetmask(oldsigmask);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_new_queue 					    */
/*                                                                          */
/*                                                                          */
/* 		 add to ev queue		 			    */
/****************************************************************************/
int             _cgi_new_queue(devclass, devnum, sample)
Cdevoff         devclass;
Cint            devnum;
Cinrep         *sample;
{
    Cinrep         *newsample;
    _cgi_que_over_status = NO_OFLO;	/* assume state is OK */
    switch (_cgi_que_status)
    {
    case NOT_VALID:
	break;
    case EMPTY:
	_cgi_que_status = NON_EMPTY;
	_cgi_event_queue[0].n = _cgi_int_dev_conv(devclass, devnum);
	newsample = (Cinrep *) malloc(sizeof(Cinrep));
	*newsample = *sample;
	_cgi_event_queue[0].contents = newsample;
	_cgi_num_events = 1;
	break;
    case NON_EMPTY:
	_cgi_event_queue[_cgi_num_events].n = _cgi_int_dev_conv(devclass, devnum);
	newsample = (Cinrep *) malloc(sizeof(Cinrep));
	*newsample = *sample;
	_cgi_event_queue[_cgi_num_events].contents = newsample;
	if (MAXEVENTS - ++_cgi_num_events <= 1)
	    _cgi_que_status = ALMOST_FULL;
	break;
    case ALMOST_FULL:
	_cgi_que_status = FULL;
	_cgi_event_queue[_cgi_num_events].n = _cgi_int_dev_conv(devclass, devnum);
	newsample = (Cinrep *) malloc(sizeof(Cinrep));
	*newsample = *sample;
	_cgi_event_queue[_cgi_num_events].contents = newsample;
	_cgi_num_events++;
	break;
    case FULL:
	_cgi_que_over_status = OFLO;
	break;
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  _cgi_temp_assoc_stroke	 	  		    */
/*                                                                          */
/****************************************************************************/

int             _cgi_temp_assoc_stroke(devnum)
Cint            devnum;
{
    int             tpt;
    struct inputmask imask;
    int             designee;

    tpt = ++_cgi_trigger[4][0];
    _cgi_trigger[4][tpt] = _cgi_int_dev_conv(IC_STROKE, devnum);
    _cgi_trigger[4][0] = tpt;
    win_getinputmask(_cgi_vws->sunview.windowfd, &imask, &designee);
    imask.im_flags |= IM_NEGEVENT;
    win_setinputcodebit(&imask, LOC_MOVE);
    win_setinputmask(_cgi_vws->sunview.windowfd, &imask,
		     (struct inputmask *) NULL, designee);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  _cgi_temp_dissoc_stroke	 	  		    */
/*                                                                          */
/****************************************************************************/

int             _cgi_temp_dissoc_stroke(devnum)
Cint            devnum;
{
    int             tpt;
    struct inputmask imask;
    int             designee;
    int             x1, y1;
    int             x2, y2, ech;

    ech = _cgi_stroker[devnum - 1]->substroke.echo;
    tpt = --_cgi_trigger[4][0];
/*   _cgi_trigger[4][0] = tpt; */
    win_getinputmask(_cgi_vws->sunview.windowfd, &imask, &designee);
    if (!tpt)
	win_unsetinputcodebit(&imask, LOC_MOVE);
    win_setinputmask(_cgi_vws->sunview.windowfd, &imask,
		     (struct inputmask *) NULL, designee);
    if (ech > 1 && _cgi_num_stroke_points)
    {
	if (_cgi_stroker[devnum - 1]->substroke.enable == RESPOND_EVENT)
	{
	    x1 = _cgi_stroker[devnum - 1]->sarray->ptlist[0].x;
	    y1 = _cgi_stroker[devnum - 1]->sarray->ptlist[0].y;
	}
	else if (_cgi_stroker[devnum - 1]->substroke.enable == REQUEST_EVENT)
	{
	    x1 = _cgi_stroker[devnum - 1]->rsarray->ptlist[0].x;
	    y1 = _cgi_stroker[devnum - 1]->rsarray->ptlist[0].y;
	}
	else
	{
	    x1 = tarray[0].x;
	    y1 = tarray[0].y;
	}
	_cgi_devscale(x1, y1, x1, y1);
	if (_cgi_num_stroke_points >= 2)
	{
	    if (_cgi_stroker[devnum - 1]->substroke.enable == RESPOND_EVENT)
	    {
		x2 = _cgi_stroker[devnum - 1]->sarray->
		    ptlist[_cgi_num_stroke_points - 1].x;
		y2 = _cgi_stroker[devnum - 1]->sarray->
		    ptlist[_cgi_num_stroke_points - 1].y;
	    }
	    else
	    {
		if (_cgi_stroker[devnum - 1]->substroke.enable == REQUEST_EVENT)
		{
		    x2 = _cgi_stroker[devnum - 1]->rsarray->
			ptlist[_cgi_num_stroke_points - 1].x;
		    y2 = _cgi_stroker[devnum - 1]->rsarray->
			ptlist[_cgi_num_stroke_points - 1].y;
		}
		else
		{
		    x2 = tarray[_cgi_num_stroke_points - 1].x;
		    y2 = tarray[_cgi_num_stroke_points - 1].y;
		}
	    }
	    _cgi_devscale(x2, y2, x2, y2);
	}
	else
	{
	    x2 = x1;
	    y2 = y1;
	}
	switch (ech)
	{
	case (1):
	    break;
	case (2):
	    pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x2, y2, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_unlock(_cgi_vws->sunview.pw);
	    break;
	case (3):
	    pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x1, y2, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_unlock(_cgi_vws->sunview.pw);
	    break;
	case (4):
	    pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x2, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x1,
		      y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_unlock(_cgi_vws->sunview.pw);
	    break;
	case (5):
	    pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
	    pw_vector(_cgi_vws->sunview.pw, x1, y1, x1, y2, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x1, y2, x2, y2, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x2, y2, x2, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_vector(_cgi_vws->sunview.pw, x2, y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
	    pw_unlock(_cgi_vws->sunview.pw);
	    break;
	}
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_echo_dis                                              */
/*									    */
/*	                                                                    */
/****************************************************************************/
int             _cgi_echo_dis(ech, x, y, devclass, devnum, erase)
int             ech, x, y;
Cdevoff         devclass;
int             devnum;
Ceraseflg       erase;
{
    register struct keybstr *keyptr;
    int             x1, y1;
    int             x2, y2;
    int             tmode;
    int             saved_ttext;
    Ccoor           saved_concat_pt;
    Ctextatt        temptext;
    char            sval[10];
    static Ctextatt newtext =
    {
     1, 1, 1, STRING, 1.0, 0.1, 1, 1000, 1.0, 0.0, 0.0, 1.0,
     RIGHT, LFT, TOP, 0.0, 0.0
    };

    if (ech <= 0)		/* no CGI echoing to do */
	return;

    saved_concat_pt = _cgi_att->text.concat_pt;
    switch (devclass)
    {
    case IC_LOCATOR:
    case IC_PICK:
    case IC_CHOICE:
	break;
    case IC_VALUATOR:
	if (ech == 2)
	{
	    register struct valstr *valptr;

	    saved_ttext = _cgi_ttext;
	    temptext = _cgi_att->text.attr;
	    tmode = _cgi_pix_mode;
	    _cgi_ttext = 1;
	    _cgi_att->text.attr = newtext;
	    _cgi_pix_mode = PIX_NOT(PIX_DST);
	    valptr = _cgi_valuatr[devnum - 1];

	    if (erase == DRAW)
	    {
		if (valptr->echo_flag)
		{
		    (void) sprintf(sval, "%f", valptr->echo_val);
		    (void) text(&valptr->echo_base, sval);
		}
		valptr->echo_val = valptr->curval;
		valptr->echo_flag = 1;
	    }
	    (void) sprintf(sval, "%f", valptr->echo_val);
	    (void) text(&valptr->echo_base, sval);
	    _cgi_att->text.attr = temptext;
	    _cgi_ttext = saved_ttext;
	    _cgi_pix_mode = tmode;
	}
	break;
    case IC_STROKE:
	if (ech > 1 && _cgi_num_stroke_points)
	{
	    if (_cgi_stroker[devnum - 1]->substroke.enable == RESPOND_EVENT)
	    {
		x1 = _cgi_stroker[devnum - 1]->sarray->ptlist[0].x;
		y1 = _cgi_stroker[devnum - 1]->sarray->ptlist[0].y;
	    }
	    else if (_cgi_stroker[devnum - 1]->substroke.enable == REQUEST_EVENT)
	    {
		x1 = _cgi_stroker[devnum - 1]->rsarray->ptlist[0].x;
		y1 = _cgi_stroker[devnum - 1]->rsarray->ptlist[0].y;
	    }
	    else
	    {
		x1 = tarray[0].x;
		y1 = tarray[0].y;
	    }
	    _cgi_devscale(x1, y1, x1, y1);
	    if (_cgi_num_stroke_points >= 2)
	    {
		if (_cgi_stroker[devnum - 1]->substroke.enable == RESPOND_EVENT)
		{
		    x2 = _cgi_stroker[devnum - 1]->sarray->
			ptlist[_cgi_num_stroke_points - 1].x;
		    y2 = _cgi_stroker[devnum - 1]->sarray->
			ptlist[_cgi_num_stroke_points - 1].y;
		}
		else if (_cgi_stroker[devnum - 1]->substroke.enable == REQUEST_EVENT)
		{
		    x2 = _cgi_stroker[devnum - 1]->rsarray->
			ptlist[_cgi_num_stroke_points - 1].x;
		    y2 = _cgi_stroker[devnum - 1]->rsarray->
			ptlist[_cgi_num_stroke_points - 1].y;
		}
		else
		{
		    {
			x2 = tarray[_cgi_num_stroke_points - 1].x;
			y2 = tarray[_cgi_num_stroke_points - 1].y;
		    }
		}
		_cgi_devscale(x2, y2, x2, y2);
	    }
	    else
	    {
		x2 = x1;
		y2 = y1;
	    }
	    _cgi_devscale(x, y, x, y);
	    switch (ech)
	    {
	    case (1):
		break;
	    case (2):
		pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x2, y2, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x, y, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_unlock(_cgi_vws->sunview.pw);
		break;
	    case (3):
		pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x1, y2, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x1, y, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_unlock(_cgi_vws->sunview.pw);
		break;
	    case (4):
		pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x2, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1,
			  y1, x, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_unlock(_cgi_vws->sunview.pw);
		break;
	    case (5):
		pw_lock(_cgi_vws->sunview.pw, &_cgi_vws->sunview.lock_rect);
		pw_vector(_cgi_vws->sunview.pw, x1, y1, x1, y2, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1, y1, x1, y, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1, y2, x2, y2, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x1, y, x, y, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x2, y2, x2, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x, y, x, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x2, y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_vector(_cgi_vws->sunview.pw, x, y1, x1, y1, (PIX_DST ^ PIX_SRC), 0xffff);
		pw_unlock(_cgi_vws->sunview.pw);
		break;
	    }
	}
	break;
    case IC_STRING:
	switch (ech)
	{
	case 1:
	    break;
	case 2:
	    saved_ttext = _cgi_ttext;
	    temptext = _cgi_att->text.attr;
	    _cgi_ttext = 1;
	    _cgi_att->text.attr = newtext;
	    keyptr = _cgi_keybord[devnum - 1];
	    tmode = _cgi_pix_mode;
	    _cgi_pix_mode = PIX_NOT(PIX_DST);
	    if (erase == DRAW)
	    {
		(void) text(&keyptr->echo_pt,
			    &keyptr->rinitstring[_cgi_num_chars - 1]);
		keyptr->echo_pt = _cgi_att->text.concat_pt;
	    }
	    else
	    {
		(void) text(&keyptr->echo_base, keyptr->rinitstring);
		keyptr->echo_pt = keyptr->echo_base;
	    }
	    _cgi_pix_mode = tmode;
	    _cgi_att->text.attr = temptext;
	    _cgi_ttext = saved_ttext;
	    break;
	}
	break;
    }
    _cgi_att->text.concat_pt = saved_concat_pt;
}
