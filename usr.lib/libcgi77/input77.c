#ifndef lint
static char	sccsid[] = "@(#)input77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
enable_events
disable_events
associate
dissociate
track_on
track_off
echo_on
echo_update
echo_off
request_input
*/

#include "cgidefs.h"
#include "cf77.h"
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfinitlid			   	    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/


int cfinitlid_ (devclass, devnum, x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int *devclass;
int *devnum;			/* device type, device number */
int *x,*y;
int *xlist;
int *ylist;
int *n;
float *val;
int *choice;
char *string;
int *segid;
int *pickid;
int f77strlen;
{
    Cinrep    ival;		/* initial value of device measure */
    Cpick     ipick;
    Ccoor     pt;
    Ccoorlist	coorlist;
    Cchar   tst[MAXCHAR+1];		/* text */
    Cerror    err;

    if ((err = ASSIGN_INREP(&ival, *x, *y, xlist, ylist, *n, *val, *choice,
		string, f77strlen, *segid, *pickid,
		&pt, &coorlist, tst, sizeof(tst), &ipick)) == NO_ERROR)
    {
	err = initialize_lid ((Cdevoff) *devclass, (Cint) *devnum, &ival);
    }
    FREE_INREP(&ival);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfrelidev					    */
/*                                                                          */
/*                                                                          */
/*              is reinitialized.                                           */
/****************************************************************************/

int     cfrelidev_ (devclass, devnum)
int *devclass;
int *devnum;			/* device type, device number */
{
    return (release_input_device ((Cdevoff) *devclass, (Cint) *devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfenevents 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfenevents_	(devclass, devnum)
int *devclass;
int     *devnum;			/* device type, device number */

{
    return (enable_events ((Cdevoff) *devclass, (Cint) *devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfdaevents 					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfdaevents_ (devclass, devnum)
int  *devclass;
int      *devnum;			/* device type, device number */

{
    return (disable_events ((Cdevoff) *devclass,  (Cint) *devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftrackon                                                  */
/*                                                                          */
/*                                                                          */
/*              the screen for an entire class of devices.                  */
/****************************************************************************/

int   cftrackon_ (devclass,devnum,echotype,exlow,eylow,exup,eyup,
x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int   *devclass;
int   *devnum;
int   *echotype;		/* type of echo */
int  *exlow;
int *eylow;
int  *exup;
int *eyup;
int *x,*y;
int *xlist;
int *ylist;
int *n;
float *val;
int *choice;
char *string;
int *segid;
int *pickid;
int f77strlen;
{
    Ccoorpair echoregion;	/* window where echo is enabled */
    Ccoorlist coorlist;
    Cinrep   value;		/* device value */
    Ccoor     pt;
    Ccoor bot,top;
    Cchar   tst[MAXCHAR+1];		/* text */
    Cpick	ipick;
    Cerror err;
 
    ASSIGN_COOR(&bot, *exlow, *eylow);
    ASSIGN_COOR(&top, *exup, *eyup);
    echoregion.upper = &top;
    echoregion.lower = &bot;
    if ((err = ASSIGN_INREP(&value, *x, *y, xlist, ylist, *n, *val,
		*choice, string, f77strlen, *segid, *pickid,
		&pt, &coorlist, tst, sizeof(tst), &ipick)) == NO_ERROR)
    {
	err = track_on ((Cdevoff)  *devclass, (Cint) *devnum, (Cint) *echotype,
		&echoregion, &value);
    }
    FREE_INREP(&value);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cftrackoff	 				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cftrackoff_ (devclass, devnum, echo, action)
int *devclass;
int     *devnum;			/* device type, device number */
int     *echo;			/* echo number */
int	*action;
{
    return (track_off ((Cdevoff) *devclass, (Cint) *devnum,
		(Cint) *echo, (Cfreeze) *action));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsinitval	 			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfsinitval_(devclass,devnum,x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int *devclass; /* device type */
int *devnum; /* device number */
int *x,*y;
int *xlist;
int *ylist;
int *n;
float *val;
int *choice;
char *string;
int *segid;
int *pickid;
int f77strlen;
{
Cinrep   value;		/* device value */
    Ccoor     pt;
    Ccoorlist	coorlist;
    Cchar   tst[MAXCHAR+1];		/* text */
    Cpick	ipick;
    Cerror err;

    if ((err = ASSIGN_INREP(&value, *x, *y, xlist, ylist, *n, *val,
		*choice, string, f77strlen, *segid, *pickid,
		&pt, &coorlist, tst, sizeof(tst), &ipick)) == NO_ERROR)
    {
	err = set_initial_value((Cdevoff) *devclass, (Cint) *devnum, &value);
    }
    FREE_INREP(&value);
    return(err);
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsvalrange	 			    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfsvalrange_	(devnum,mn,mx)
int *devnum; /* device number */
float *mn,*mx; /* limits of valuator */
{
    return (set_valuator_range((Cint) *devnum, (Cfloat) *mn, (Cfloat) *mx));
}
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfreqinp                                                   */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfreqinp_ (devclass, devnum, timeout, valid, trigger,
x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int *devclass;
int     *devnum;		/* device type, device number */
int     *timeout;		/* amount of time to wait for input */
int * valid;		/* device status */
int    *trigger;		/* trigger number */
int *x,*y;
int *xlist;
int *ylist;
int *n;
float *val;
int *choice;
char *string;
int *segid;
int *pickid;
int f77strlen;
{
    int  err;
    Cinrep   value;		/* device value */
    Ccoor    pt;
    Ccoorlist pcoorlist;
    Cchar    strng[MAXCHAR];
    Cpick    pck;

    switch (*devclass)
    {
	case IC_LOCATOR:
	    value.xypt = &pt;
	    break;
	case IC_STROKE:
	    if ((err = _cgi_f77_alloc_coorlist (&pcoorlist, xlist, ylist, *n)
		    != NO_ERROR))
	    return(err);
	    value.points = &pcoorlist;
	    break;
	case IC_VALUATOR:
	case IC_CHOICE:
	    break;
	case IC_STRING:
	    value.string = strng;
	    break;
	case IC_PICK:
	    value.pick = &pck;
	    break;
    }
    if ((err = request_input ((Cdevoff) *devclass, *devnum,
		*timeout, (Cawresult*)valid, &value, trigger)) != NO_ERROR)
	return(err);

    return( RETURN_INREP(*devclass, x, y, xlist, ylist, n, val, choice,
		string, f77strlen, segid, pickid, &value) );
}
