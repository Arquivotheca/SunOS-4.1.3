#ifndef lint
static char	sccsid[] = "@(#)input2.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
associate
set_default_trigger_associations
dissociate
_cgi_err_check_86
*/

#include "cgipriv.h"
View_surface   *_cgi_vws;	/* current view surface */
View_surface   *_cgi_view_surfaces[MAXVWS];	/* view surface information */
int             _cgi_vwsurf_count;	/* number of view surfaces */
int             _cgi_trigger[MAXTRIG][MAXASSOC + 1];	/* association list */

/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  associate			 	  		    */
/*                                                                          */
/*		Links a trigger to a specific device			    */
/****************************************************************************/

Cerror          associate(trigger, devclass, devnum)
Cint            trigger;	/* trigger number */
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    register View_surface *vwsP;
    int             i, terr, tpt;
    struct inputmask imask;
    int             designee;

    terr = _cgi_err_check_481(devclass, devnum);
    if (!terr)
	terr = _cgi_err_check_86(trigger);
    if (!terr)
	terr = _cgi_err_is_trig_assoc(devclass, devnum, trigger);
    if (!terr)
    {
	for (i = 0; i < _cgi_vwsurf_count; i++)
	{
	    vwsP = _cgi_view_surfaces[i];
	    win_getinputmask(vwsP->sunview.windowfd, &imask, &designee);
	    switch (devclass)
	    {
	    case IC_STRING:
		switch (trigger)
		{
		case (1):
		    imask.im_flags |= IM_ASCII;
/* 				    win_setinputcodebit (&imask, IM_ASCII); */
		    break;
		default:
		    terr = 84;
		    break;
		}

		break;
	    case IC_STROKE:
	    case IC_PICK:
		switch (trigger)
		{
		case (2):
		    win_setinputcodebit(&imask, MS_LEFT);
		    break;
		case (3):
		    win_setinputcodebit(&imask, MS_MIDDLE);
		    break;
		case (4):
		    win_setinputcodebit(&imask, MS_RIGHT);
		    break;
		default:
		    terr = 84;
		    break;
		}
		break;
	    case IC_LOCATOR:
	    case IC_VALUATOR:
	    case IC_CHOICE:
		switch (trigger)
		{
		case (2):
		    win_setinputcodebit(&imask, MS_LEFT);
		    break;
		case (3):
		    win_setinputcodebit(&imask, MS_MIDDLE);
		    break;
		case (4):
		    win_setinputcodebit(&imask, MS_RIGHT);
		    break;
		case (5):
		    win_setinputcodebit(&imask, LOC_MOVE);
		    break;
		case (6):
		    win_setinputcodebit(&imask, LOC_STILL);
		    break;
		default:
		    terr = 84;
		    break;
		}
		break;
	    }
	    if (!terr)
	    {
		win_setinputmask(vwsP->sunview.windowfd, &imask,
				 (struct inputmask *) 0, designee);
	    }
	}
	if (!terr)
	{
	    tpt = ++_cgi_trigger[trigger - 1][0];
	    _cgi_trigger[trigger - 1][tpt] =
		_cgi_int_dev_conv(devclass, devnum);
	}
    }
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: set_default_trigger_associations			    */
/*                                                                          */
/*		Links a trigger to a specific device			    */
/****************************************************************************/

Cerror          set_default_trigger_associations(devclass, devnum)
Cdevoff         devclass;	/* device type */
Cint            devnum;		/* device number */
{
    int             err;
    switch (devclass)
    {
    case IC_STRING:
	err = associate(1, devclass, devnum);
	break;
    case IC_STROKE:
	err = associate(4, devclass, devnum);
	break;
    case IC_LOCATOR:
	err = associate(5, devclass, devnum);
	break;
    case IC_VALUATOR:
	err = associate(3, devclass, devnum);
	break;
    case IC_CHOICE:
	err = associate(2, devclass, devnum);
	break;
    }
    return (_cgi_errhand(err));
}


/****************************************************************************/
/*                                                                          */
/*     FUNCTION:  dissociate			 	  		    */
/*                                                                          */
/*		Removes a trigger from a specific device		    */
/****************************************************************************/

Cerror          dissociate(trigger, devclass, devnum)
Cint            trigger;	/* trigger number */
Cdevoff         devclass;
Cint            devnum;		/* device type, device number */
{
    register View_surface *vwsP;
    int             terr, i, tpt, tp2;
    struct inputmask imask;
    int             designee;

    terr = _cgi_err_check_481(devclass, devnum);
    if (!terr)
	terr = _cgi_err_check_86(trigger);
    if (!terr)
    {
	terr = _cgi_err_is_trig_assoc(devclass, devnum, trigger);
	if (terr)
	    terr = NO_ERROR;	/* no error because assoc. exists */
	else
	    terr = 85;
    }
    if (!terr)
    {
	tpt = _cgi_trigger[trigger - 1][0];
	i = 1;
	tp2 = _cgi_int_dev_conv(devclass, devnum);
	while (_cgi_trigger[trigger - 1][i] != tp2 && i < tpt)
	    i++;
	_cgi_trigger[trigger - 1][i] = 0;
	while (i < tpt)
	    _cgi_trigger[trigger - 1][i] = _cgi_trigger[trigger - 1][i + 1];
	_cgi_trigger[trigger - 1][0] = tpt - 1;
	(void) selective_flush_of_event_queue(devclass, devnum);
	/* dequeue events */
	if (tpt <= 1)
	{
	    for (i = 0; i < _cgi_vwsurf_count; i++)
	    {
		vwsP = _cgi_view_surfaces[i];
		win_getinputmask(vwsP->sunview.windowfd, &imask, &designee);
		switch (trigger)
		{
		case (1):
		    imask.im_flags |= ~IM_ASCII;
/* 				    win_unsetinputcodebit (&imask, IM_ASCII); */
		    break;
		case (2):
		    win_unsetinputcodebit(&imask, MS_LEFT);
		    break;
		case (3):
		    win_unsetinputcodebit(&imask, MS_MIDDLE);
		    break;
		case (4):
		    win_unsetinputcodebit(&imask, MS_RIGHT);
		    break;
		case (5):
		    win_unsetinputcodebit(&imask, LOC_MOVE);
		    break;
		case (6):
		    win_unsetinputcodebit(&imask, LOC_STILL);
		    break;
		}
		win_setinputmask(vwsP->sunview.windowfd, &imask,
				 (struct inputmask *) 0, designee);
	    }
	}
    }
    return (_cgi_errhand(terr));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_check_86 						    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int             _cgi_err_check_86(trigger)
int             trigger;
{
    int             err = NO_ERROR;

    if (trigger < 1 || trigger > MAXTRIG)
	err = EINTRNEX;
    return (err);
}
