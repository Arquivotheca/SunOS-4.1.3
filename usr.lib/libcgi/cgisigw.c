#ifndef lint
static char	sccsid[] = "@(#)cgisigw.c 1.1 92/07/30 Copyr 1985-9 Sun Micro";
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
 * CGI Window Change & Child signal functions 
 */

/*
_cgi_winsig
_cgi_winsigresp 
_cgi_new_window_init
_cgi_sigchild
*/

#include "cgipriv.h"
#include <signal.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/resource.h>

/* view surface information */
View_surface   *_cgi_view_surfaces[MAXVWS];
int             _cgi_vwsurf_count;	/* number of view surfaces ever opened */
int             _cgi_retained;	/* User asked us to make a retained pixrect */
int             _cgi_criticalcnt = 0;	/* critical region protection */
int		_cgi_sigwinched = 0;	/* sigwinch was delayed */

Clogical _cgi_get_r_screen();

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_dodeferred					    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
_cgi_dodeferred()
{
    _cgi_criticalcnt++;
    while (_cgi_sigwinched)
    {
	_cgi_sigwinched = 0;
	_cgi_winsigresp();
    }
    _cgi_criticalcnt--;
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_winsig 						    */
/*                                                                          */
/*                                                                          */
/****************************************************************************/
Notify_value _cgi_winsig()
{
    /*
     * This isn't safe, but changing CGI to use NOTIFY_SYNC requires that CGI's
     * clients know about, and use, the notifier.  CGI could perhaps call
     * notify_dispatch before returning from each CGI function, but this would
     * be expensive. 
     */
    if (_cgi_criticalcnt != 0)
	_cgi_sigwinched = 1;
    else
	_cgi_winsigresp();
    return (NOTIFY_DONE);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_winsigresp 					    */
/*                                                                          */
/*   The only routine that responds to CGI window change signals.           */
/****************************************************************************/
_cgi_winsigresp()
{
    register View_surface *vwsP;
    int             oldsigmask;
    int             i;
    Clogical        do_repair;
    Rect            new_rect, new_r_screen;

    oldsigmask = sigblock((1 << (SIGWINCH - 1)) | (1 << (SIGCHLD - 1)));
    for (i = 0; i < _cgi_vwsurf_count; i++)
    {
	if ((vwsP = _cgi_view_surfaces[i]) == (View_surface *) NULL)
	    continue;
	if (vwsP->sunview.orig_pw != (Pixwin *) NULL)
	{			/* SIGWINCH caused by change in window
				 * overlapping or size */
	    win_getsize(vwsP->sunview.windowfd, &new_rect);
	    do_repair = _cgi_get_r_screen(vwsP, &new_r_screen);
	    if (do_repair)
		pw_damaged(vwsP->sunview.orig_pw);	/* Get rectlist of damage */
	    if ((vwsP->real_screen.r_left == new_r_screen.r_left)
		&& (vwsP->real_screen.r_top == new_r_screen.r_top)
		&& (vwsP->real_screen.r_width == new_r_screen.r_width)
		&& (vwsP->real_screen.r_height == new_r_screen.r_height)
		)
	    {			/* Window was simply exposed -- it is still the
				 * same size */
		if (vwsP->sunview.orig_pw->pw_clipdata->pwcd_state != PWCD_NULL)
		{		/* There is visible damage to this window */
		    if (vwsP->sunview.orig_pw->pw_prretained)
		    {		/* Repaint the window from the retained pixwin */
			if (do_repair)
			    pw_repairretained(vwsP->sunview.orig_pw);
		    }
		    else
		    {		/* not retained: user's sigwinch routine may
				 * redraw */
		    }		/* If user sigwinch routine doesn't redraw, no
				 * one does */
		    if (do_repair)
			pw_donedamaged(vwsP->sunview.orig_pw);
		    if (vwsP->sunview.gp_att)
		    {
			(void) _cgi_gp1_pw_updclplst(vwsP->sunview.orig_pw,
						     vwsP->sunview.gp_att);
		    }
		}		/* There is visible damage to this window */
		else
		{		/* This sigwinch has nothing to do with this
				 * window */
		    if (do_repair)
			pw_donedamaged(vwsP->sunview.orig_pw);
		    continue;	/* ... with other view surfaces in for loop */
		}
	    }			/* Window was simply exposed -- it is still the
				 * same size */
	    else
	    {			/* Window size changed--cut out a new piece to
				 * use */
		if (do_repair)
		    pw_donedamaged(vwsP->sunview.orig_pw);
		/**
		 ** pwcd_state doesn't matter if this window's size changed.
		 ** Overview:
		 **	0. retrieve the real screen size.
		 **	1. set viewport size.
		 **	2. set the translation from VDC to vport coords.
		 **	3. rebuild clipping region from VDC coords & new xform. 
		 **/
		vwsP->real_screen = new_r_screen;
		vwsP->vport = vwsP->real_screen;
		(void) _cgi_windowset(vwsP);	/* also resets clipping */
		if (vwsP->sunview.gp_att)
		    _cgi_update_gp1_xform(vwsP);

		/*
		 * Note that a retained pixwin does not get reallocated when
		 * the window changes size.  It might not be a good idea to do
		 * so, because malloc is a "critical region": it could be
		 * disasterous if the user is executing in malloc when the
		 * signal arrives, invokes this routine, and we malloc before
		 * the other malloc finishes. 
		 */
	    }			/* Window size changed */
	    /* This Window affected: either a size change or it was exposed */
	    if (vwsP->sunview.sig_function)
	    {
		(void) sigsetmask(oldsigmask);
		/* wds 850703 added "name" argument to user's sig_function */
		(*vwsP->sunview.sig_function) (i);
		oldsigmask = sigblock((1 << (SIGWINCH - 1)) |
				      (1 << (SIGCHLD - 1)));
	    }
	}			/* SIGWINCH caused by change in window
				 * overlapping or size */
    }				/* for (i = 0; i < _cgi_vwsurf_count; i++) */
    (void) sigsetmask(oldsigmask);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_new_window_init					    */
/*                                                                          */
/*		Open a pixwin, get its size, set up basic surface attribs   */
/****************************************************************************/
Cerror _cgi_new_window_init(vws)
register View_surface *vws;
{
    Cerror          err = NO_ERROR;

    if (vws->sunview.orig_pw != (Pixwin *) NULL)
	err = ENOWSTYP;
    else
    {
	if (!vws->sunview.windowfd)
	    vws->sunview.windowfd = win_getnewwindow();	/* Ever needed? */
	vws->sunview.pw = vws->sunview.orig_pw = pw_open(vws->sunview.windowfd);
	if (vws->sunview.orig_pw == (Pixwin *) NULL)
	    err = ENOWSTYP;
	else
	{
	    win_getsize(vws->sunview.windowfd, &vws->sunview.lock_rect);
	    (void) _cgi_get_r_screen(vws, &vws->real_screen);
	    vws->vport = vws->real_screen;
	    if (_cgi_retained)
	    {
		vws->sunview.orig_pw->pw_prretained =
		    mem_create(vws->real_screen.r_width,
			       vws->real_screen.r_height,
			       vws->sunview.orig_pw->pw_pixrect->pr_depth);
	    }
	}
    }
    return (err);
}

/****************************************************************************/
/*                                                                          */
/*     FUNCTION: _cgi_sigchild	 					    */
/*                                                                          */
/*  Invoked when...							    */
/****************************************************************************/
/* ARGSUSED WCL: me, rusage not used */
Notify_value _cgi_sigchild(me, pid, status, rusage)
int            *me, pid;
union wait     *status;
struct rusage  *rusage;
{
    register View_surface *vwsP;
    register int    i;


    if (WIFEXITED(*status))	/* Make sure it's dead */
    {
	for (i = 0; i < _cgi_vwsurf_count; i++)
	{
	    vwsP = _cgi_view_surfaces[i];
	    if (vwsP && (vwsP->sunview.tool_pid == pid))
	    {
		vwsP->sunview.tool_pid = 0;
		(void) close_vws(i);
		break;
	    }
	}
	return (NOTIFY_DONE);
    }
    return (NOTIFY_IGNORED);
}
