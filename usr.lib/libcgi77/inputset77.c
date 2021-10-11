#ifndef lint
static char	sccsid[] = "@(#)inputset77.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
flush_event_queue
set_prompt_state
set_acknowledgement_state
signal_instructions
sample input
initiate_request
request_input
await_event
*/

#include "cgidefs.h"
#include "cf77.h"
/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfflusheventqu  				    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int     cfflusheventqu_ () {
    return (flush_event_queue ());
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsflusheventqu                                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfsflusheventqu_(devclass, devnum)
int *devclass; /* device type */
int *devnum; /* device number */
{
    return (selective_flush_of_event_queue((Cdevoff)*devclass, (Cint)*devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfsampinp                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfsampinp_ (devclass, devnum, valid, 
x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int *devclass;
int *devnum;			/* device type, device number */
int * valid;		/* device status */
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
    int		err;
    Cinrep	value;		/* device value */
    Ccoor	pt;

    value.xypt = &pt;
    if ((err = sample_input ((Cdevoff) *devclass, (Cint) *devnum,
		(Clogical*)valid, &value)) != NO_ERROR)
	return(err);

    RETURN_INREP(*devclass, x, y, xlist, ylist, n, val, choice,
		string, f77strlen, segid, pickid, &value);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfinitreq                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int  cfinitreq_ (devclass, devnum)
int  *devclass;
int      *devnum;			/* device type, device number */
{
    return (initiate_request ((Cdevoff) *devclass,  (Cint) *devnum));
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfgetlastreqinp                                            */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
int   cfgetlastreqinp_ ( devclass,  devnum, valid, 
x,y,xlist,ylist,n,val,choice,string,segid,pickid,f77strlen)
int  *devclass;
int      *devnum;		/* device type, device number */
int *valid;		/* device status */
int *x,*y;
int *n;
int *xlist;
int *ylist;
float *val;
int *choice;
char *string;
int *segid;
int *pickid;
int f77strlen;
{
    int		err;
    Cinrep	value;		/* device value */
    Ccoor	pt;

    value.xypt = &pt;
    if ((err = get_last_requested_input ((Cdevoff) *devclass, (Cint) *devnum,
		(Clogical*)valid, &value)) != NO_ERROR)
	return(err);

    RETURN_INREP(*devclass, x, y, xlist, ylist, n, val, choice,
		string, f77strlen, segid, pickid, &value);
    return(err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: cfawaitev                                                  */
/*                                                                          */
/*                                                                          */
/****************************************************************************/

int cfawaitev_(timeout,valid,devclass,devnum,
x,y,xlist,ylist,n,val,choice,string,segid,pickid,
message_link, replost, time_stamp, qstat, overflow, f77strlen)

int *timeout; /* amount of time to wait for input */
int *valid; /* status */
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
int *message_link; /* type of message */
int *replost;  /* reports lost */ 
int *time_stamp; /* time_stamp */
int *qstat; /* queue status */
int *overflow; /* event queue */
int f77strlen;
{
    int		err;
    Cinrep	value;		/* device value */
    Ccoor	pt;

    value.xypt = &pt;
    if ((err = await_event (*timeout,(Cawresult*)valid,(Cdevoff*)devclass,
		devnum, &value, (Cmesstype*)message_link,replost,time_stamp,
		(Cqtype*)qstat, (Ceqflow*)overflow)) != NO_ERROR)
	return(err);

    return( RETURN_INREP(*devclass, x, y, xlist, ylist, n, val, choice,
			string, f77strlen, segid, pickid, &value) );
}
