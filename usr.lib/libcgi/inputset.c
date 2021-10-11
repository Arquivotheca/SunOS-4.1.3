#ifndef lint
static char	sccsid[] = "@(#)inputset.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
flush_event_queue
selective_flush_of_event_queue
sample_input
initiate_request
get_last_requested_input 
enable_events
disable_events
await_event
_cgi_int_rev_dev_conv
_cgi_err_is_dev_assoc
_cgi_err_is_trig_assoc
_cgi_err_check_94
*/

#include "cgipriv.h"
#include <signal.h>

struct locatstr *_cgi_locator[_CGI_LOCATORS];
struct keybstr *_cgi_keybord[_CGI_KEYBORDS];
struct strokstr *_cgi_stroker[_CGI_STROKES];
struct valstr  *_cgi_valuatr[_CGI_VALUATRS];
struct choicestr *_cgi_button[_CGI_CHOICES];
struct pickstr *_cgi_pick[_CGI_PICKS];
struct evqueue  _cgi_event_queue[MAXEVENTS];	/* event queue */
int             _cgi_num_events;/* event queue pointer */
Cqtype _cgi_que_status;		/* event queue status */
Ceqflow _cgi_que_over_status;	/* event queue status */
int             _cgi_trigger[MAXTRIG][MAXASSOC + 1];	/* association list */
int             _cgi_event_occurred;    /* event flag */
int             _cgi_alarm_occurred;    /* alarm flag */


extern char    *strcpy();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  flush_event_queue	  				    */
/*                                                                          */
/*		Discards all events in the event queue			    */
/****************************************************************************/

Cerror          flush_event_queue()
{
    int             i, err;
    err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_ioint_handle();
	for (i = 0; i < _cgi_num_events; i++)
	{
	    /* clean-up malloc'ed space */
	    _cgi_free_queue_element(&_cgi_event_queue[i]);
	}
	_cgi_que_status = EMPTY;
	_cgi_num_events = 0;
	_cgi_que_over_status = NO_OFLO;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: selective_flush_of_event_queue	  		            */
/*                                                                          */
/*		Discards all events in the event queue corresponding to     */
/*		the specified device					    */
/****************************************************************************/

Cerror          selective_flush_of_event_queue(devclass, devnum)
Cdevoff         devclass;	/* device type */
Cint            devnum;		/* device number */
{
    int             err, i, j, num, tnum;

    err = _cgi_err_check_481(devclass, devnum);
    if (err == 4)
	err = _cgi_check_state_5();
    if (!err)
    {
	_cgi_ioint_handle();
	num = _cgi_int_dev_conv(devclass, devnum);	/* get code number */
	tnum = _cgi_num_events;
	for (i = 0; i < _cgi_num_events; i++)
	{			/* discard events */
	    if (_cgi_event_queue[i].n == num)
	    {
		_cgi_free_queue_element(&_cgi_event_queue[i]);
		for (j = i; j < tnum; j++)
		{
		    _cgi_event_queue[j] = _cgi_event_queue[j + 1];
		}
		tnum--;
	    }
	}
	_cgi_num_events = tnum;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: sample_input	 					    */
/*                                                                          */
/*		Reports current value of the specified input device.	    */
/****************************************************************************/

Cerror          sample_input(devclass, devnum, valid, sample)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
Clogical       *valid;		/* device status */
Cinrep         *sample;		/* device value */
{
    int             err;
    err = _cgi_err_check_481(devclass, devnum);
    /* standard input device error checking */
    if (!err)
    {
	_cgi_ioint_handle();
	*valid = L_TRUE;
	switch (devclass)
	{			/* get current measure of device depending on
				 * dev. type */
	case IC_STRING:
	    sample->string = _cgi_keybord[devnum - 1]->initstring;
	    break;
	case IC_CHOICE:
	    sample->choice = _cgi_button[devnum - 1]->curval;
	    break;
	case IC_LOCATOR:
	    sample->xypt->x = _cgi_locator[devnum - 1]->x;
	    sample->xypt->y = _cgi_locator[devnum - 1]->y;
	    break;
	case IC_VALUATOR:
	    sample->val = _cgi_valuatr[devnum - 1]->curval;
	    break;
	case IC_STROKE:
	    sample->points = _cgi_stroker[devnum - 1]->sarray;
	    break;
	}
    }
    else
	*valid = L_FALSE;	/* device SNAFUed */
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: initiate_request 					    */
/*                                                                          */
/*		Sets up a device to process a on-shot request (input).	    */
/****************************************************************************/

Cerror          initiate_request(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    int             err;
    err = _cgi_err_check_481(devclass, devnum);

     /* standard input device error checking */ if (!err)
	err =
	    _cgi_err_is_dev_assoc(devclass, devnum);
    if (!err)
    {
	switch
	    (devclass)
	{
	case IC_STRING:
	    _cgi_keybord[devnum - 1]->subkey.enable = REQUEST_EVENT;
	    break;
	case IC_STROKE:
	    _cgi_stroker[devnum - 1]->substroke.enable = REQUEST_EVENT;
	    /* allow STROKE to pickup events */
	    _cgi_temp_assoc_stroke(devnum);
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->subloc.enable = REQUEST_EVENT;
	    break;
	case IC_PICK:
	    _cgi_pick[devnum - 1]->subpick.enable = REQUEST_EVENT;
	    break;
	case IC_VALUATOR:
	    _cgi_valuatr[devnum - 1]->subval.enable = REQUEST_EVENT;
	    break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->subchoice.enable = REQUEST_EVENT;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: get_last_requested_input 				    */
/*                                                                          */
/*		Reports the value of the request register.		    */
/****************************************************************************/
Cerror          get_last_requested_input(devclass, devnum, valid, sample)
/* WARNING: Revision A of the SunCGI Reference Manual is wrong here. */
Cdevoff         devclass;	/* device type */
Cint            devnum;		/* device number */
Clogical       *valid;		/* device status */
Cinrep         *sample;		/* device value */
{
    int             err;

    /* Return error if device not initialized (err 81) */
    err = _cgi_err_check_481(devclass, devnum);
    if (!err && (valid == (Clogical *) NULL
		 || sample == (Cinrep *) NULL))
	err = EBADDATA;
    if (!err)
    {
	_cgi_ioint_handle();
	*valid = L_TRUE;
	switch (devclass)
	{
	case IC_STRING:
	    if (sample->string == (char *) NULL)
		err = EBADDATA;
	    else
		(void) strcpy(sample->string,
			      _cgi_keybord[devnum - 1]->rinitstring);
	    break;
	case IC_CHOICE:
	    sample->choice = _cgi_button[devnum - 1]->rcurval;
	    break;
	case IC_LOCATOR:
	    sample->xypt->x = _cgi_locator[devnum - 1]->rx;
	    sample->xypt->y = _cgi_locator[devnum - 1]->ry;
	    break;
	case IC_VALUATOR:
	    sample->val = _cgi_valuatr[devnum - 1]->rcurval;
	    break;
	case IC_STROKE:
	    /* WCL 860911 should copy points, not return internal data area */
	    sample->points = _cgi_stroker[devnum - 1]->rsarray;
	    break;
	}
    }
    else
	*valid = L_FALSE;	/* device SNAFUed */
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: enable_events	 					    */
/*                                                                          */
/*		Allows device to put events on the input queue.		    */
/****************************************************************************/

Cerror          enable_events(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */

{
    int             err;
    err = _cgi_err_check_94(devclass, devnum);
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    _cgi_keybord[devnum - 1]->subkey.enable = QUEUE_EVENT;
	    break;
	case IC_STROKE:
	    _cgi_stroker[devnum - 1]->substroke.enable = QUEUE_EVENT;
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->subloc.enable = QUEUE_EVENT;
	    break;
	case IC_PICK:
	    _cgi_pick[devnum - 1]->subpick.enable = QUEUE_EVENT;
	    break;
	case IC_VALUATOR:
	    _cgi_valuatr[devnum - 1]->subval.enable = QUEUE_EVENT;
	    break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->subchoice.enable = QUEUE_EVENT;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: disable_events	 					    */
/*                                                                          */
/*		Prevents device to put events on the input queue.	    */
/****************************************************************************/

Cerror          disable_events(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */

{
    int             err;
    err = _cgi_err_check_481(devclass, devnum);
    /* standard input device error checking */
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    _cgi_keybord[devnum - 1]->subkey.enable = NO_EVENTS;
	    break;
	case IC_STROKE:
	    _cgi_stroker[devnum - 1]->substroke.enable = NO_EVENTS;
	    break;
	case IC_LOCATOR:
	    _cgi_locator[devnum - 1]->subloc.enable = NO_EVENTS;
	    break;
	case IC_PICK:
	    _cgi_pick[devnum - 1]->subpick.enable = NO_EVENTS;
	    break;
	case IC_VALUATOR:
	    _cgi_valuatr[devnum - 1]->subval.enable = NO_EVENTS;
	    break;
	case IC_CHOICE:
	    _cgi_button[devnum - 1]->subchoice.enable = NO_EVENTS;
	    break;
	}
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: await_event	 					    */
/*                                                                          */
/*		Reports next event  and returns  information about the	    */
/*		generating device.					    */
/****************************************************************************/

Cerror          await_event(timeout, valid, devclass, devnum,
                  measure, message_link, replost, time_stamp, qstat, overflow)
Cint            timeout;	/* amount of time to wait for input */
Cawresult      *valid;		/* status */
Cdevoff        *devclass;	/* device type */
Cint           *devnum;		/* device number */
Cinrep         *measure;	/* device value */
Cmesstype      *message_link;	/* type of message */
Cint           *replost;	/* reports lost */
Cint           *time_stamp;	/* time_stamp */
Cqtype         *qstat;		/* queue status */
Ceqflow        *overflow;	/* event queue */
{
    int             err;
    int             i, nm;
    int             cnt;
/*  static  Cinrep imeas;
  static  Ccoorlist pts;
  static  Ccoor list[MAXPTS], pt;
  static char keys[MAXCHAR]; */

    if (valid == (Cawresult *) NULL
	|| devclass == (Cdevoff *) NULL
	|| devnum == (Cint *) NULL
	|| measure == (Cinrep *) NULL
	|| message_link == (Cmesstype *) NULL
	|| replost == (Cint *) NULL
	|| time_stamp == (Cint *) NULL
	|| qstat == (Cqtype *) NULL
	|| overflow == (Ceqflow *) NULL)
    {
	err = EBADDATA;
    }
    else
    {
	*qstat = _cgi_que_status;
	*overflow = _cgi_que_over_status;
	*replost = 0;
	*time_stamp = 0;
	*valid = WRONG_STATE;
	err = _cgi_err_check_4();
    }
    if (!err)
    {
	/* Set an itimer for the user-specified timeout */
	if (timeout >= 0)  {
	    (void) _cgi_set_timeout((unsigned) timeout);
	    *valid = TIMED_OUT;
	}

	do
	{
	    if (! (_cgi_event_occurred || _cgi_alarm_occurred))
                (void) _cgi_timeout(timeout);
	    if (_cgi_event_occurred)
		(void) _cgi_ioint_handle();/* process pending events */
	    *qstat = _cgi_que_status;
	    *overflow = _cgi_que_over_status;
	    if (_cgi_num_events)
	    {
		/* dequeue event at the head of the queue */
		nm = _cgi_event_queue[0].n;
		_cgi_int_rev_dev_conv(nm, devclass, devnum);

		/* copy to a safe place before freeing */
		switch (*devclass)
		{
		case IC_STROKE:
		    if (!measure->points)
			return (_cgi_errhand(EBADDATA));
		    cnt = _cgi_event_queue[0].contents->points->n;
		    if (cnt > measure->points->n)
			cnt = measure->points->n;
		    for (i = 0; i < cnt; i++)
		    {
			measure->points->ptlist[i] =
			    _cgi_event_queue[0].contents->points->ptlist[i];
		    }
		    break;
		case IC_LOCATOR:
		    if (!measure->xypt)
			return (_cgi_errhand(EBADDATA));
		    *measure->xypt = *_cgi_event_queue[0].contents->xypt;
		    break;
		case IC_STRING:
		    if (!measure->string)
			return (_cgi_errhand(EBADDATA));
		    (void) strcpy(measure->string,
				  _cgi_event_queue[0].contents->string);
		    break;
		case IC_CHOICE:
		    measure->choice = _cgi_event_queue[0].contents->choice;
		    break;
		case IC_PICK:
		    if (!measure->pick)
			return (_cgi_errhand(EBADDATA));
		    measure->pick->segid = 
				  _cgi_event_queue[0].contents->pick->segid;
		    measure->pick->pickid = 
				  _cgi_event_queue[0].contents->pick->pickid;
		    break;
		case IC_VALUATOR:
		    measure->val = _cgi_event_queue[0].contents->val;
		    break;
		default:
		    break;
		}
		_cgi_free_queue_element(&_cgi_event_queue[0]);
		*valid = VALID_DATA;	/* set returned args. */
		*message_link = SINGLE_EVENT;
		for (i = 1; i < _cgi_num_events; i++)
		{
		    _cgi_event_queue[i - 1] = _cgi_event_queue[i];
		    if (_cgi_event_queue[i].n == nm)
			*message_link = SIMULTANEOUS_EVENT_FOLLOWS;
		}
		_cgi_num_events--;
		if (!_cgi_num_events)
		    _cgi_que_status = EMPTY;
		if (_cgi_que_status == FULL)
		    _cgi_que_status = ALMOST_FULL;
		break;			/* break out of timeout loop */
	    }
        } while (!_cgi_alarm_occurred);
	*qstat = _cgi_que_status;
	*overflow = _cgi_que_over_status;
	if (_cgi_que_over_status == OFLO)
	    err = 97;
    }
    return (_cgi_errhand(err));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_int_rev_dev_conv	 			            */
/*                                                                          */
/*		 Converts a device name into a device type and number.	    */
/****************************************************************************/
_cgi_int_rev_dev_conv(nm, devclass, devnum)
int             nm;
Cdevoff        *devclass;
Cint           *devnum;
{
    if (nm <= OFF2)
    {
	*devclass = IC_STRING;
	*devnum = nm - OFF1;
    }
    else
    {
	if (nm < OFF3)
	{
	    *devclass = IC_STROKE;
	    *devnum = nm - OFF2;
	}
	else
	{
	    if (nm < OFF4)
	    {
		*devclass = IC_LOCATOR;
		*devnum = nm - OFF3;
	    }
	    else
	    {
		if (nm < OFF5)
		{
		    *devclass = IC_VALUATOR;
		    *devnum = nm - OFF4;
		}
		else if (nm < OFF6)
		{
		    *devclass = IC_CHOICE;
		    *devnum = nm - OFF5;
		}
		else
		{
		    *devclass = IC_PICK;
		    *devnum = nm - OFF6;
		}
	    }
	}
    }
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_is_dev_assoc 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_is_dev_assoc(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    int             err, i, terr;

    err = 85;
    for (i = 1; i < MAXTRIG + 1; i++)
    {
	terr = _cgi_err_is_trig_assoc(devclass, devnum, i);
	if (terr)
	{			/* trigger is associated */
	    err = 0;
	    break;
	}
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_is_trig_assoc					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_is_trig_assoc(devclass, devnum, trigger)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
int             trigger;
{
    int             err, temp, tpt, i;
    err = 0;
    temp = _cgi_int_dev_conv(devclass, devnum);
    tpt = _cgi_trigger[trigger - 1][0];
    if (tpt != 0)
    {
	tpt++;
	for (i = 0; i < tpt; i++)
	    if (_cgi_trigger[trigger - 1][i] == temp)
		err = 83;
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_err_check_94					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_94(devclass, devnum)
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */

{
    int             err = 0;
    err = _cgi_err_check_481(devclass, devnum);
    if (!err)
    {
	switch (devclass)
	{
	case IC_STRING:
	    if (_cgi_keybord[devnum - 1]->subkey.enable != NO_EVENTS)
		err = 94;
	    break;
	case IC_STROKE:
	    if (_cgi_stroker[devnum - 1]->substroke.enable != NO_EVENTS)
		err = 94;
	    break;
	case IC_LOCATOR:
	    if (_cgi_locator[devnum - 1]->subloc.enable != NO_EVENTS)
		err = 94;
	    break;
	case IC_PICK:
	    if (_cgi_pick[devnum - 1]->subpick.enable != NO_EVENTS)
		err = 94;
	    break;
	case IC_VALUATOR:
	    if (_cgi_valuatr[devnum - 1]->subval.enable != NO_EVENTS)
		err = 94;
	    break;
	case IC_CHOICE:
	    if (_cgi_button[devnum - 1]->subchoice.enable != NO_EVENTS)
		err = 94;
	    break;
	}
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_free_queue_element                                    */
/*                                                                          */
/*          frees an event queue element                                    */
/*                                                                          */
/****************************************************************************/
_cgi_free_queue_element(elem)
struct evqueue *elem;		/* event queue element */
{
    Cdevoff         devclass;	/* device type */
    Cint            devnum;	/* device number */
    _cgi_int_rev_dev_conv(elem->n, &devclass, &devnum);

    switch (devclass)
    {
    case IC_STROKE:
	free((char *) elem->contents->points->ptlist);
	free((char *) elem->contents->points);
	break;
    case IC_LOCATOR:
	free((char *) elem->contents->xypt);
	break;
    case IC_STRING:
	free((char *) elem->contents->string);
	break;
    default:
	break;
    }
    free((char *) elem->contents);
}
