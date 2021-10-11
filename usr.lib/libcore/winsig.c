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
static char sccsid[] = "@(#)winsig.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <signal.h>
#include <sys/file.h>
#include <sunwindow/window_hs.h>

/*----------------------------------------------------------------*/
Notify_value
_core_winsig()
{
	_core_updatewin |= 1;
	if (!_core_critflag)
		_core_winsigresp();
	return (NOTIFY_DONE);
}

/*----------------------------------------------------------------*/
_core_winsigresp()
	{
	register int vsindex;
	register viewsurf *surfp;
	register struct pixwin *pw;
	register segstruc *segptr;
	ddargtype ddstruct;
	struct rect rect;
	int oldsigmask;
	int samesize, somewinchgsize;
	int savupdatewin;

	oldsigmask = sigblock((1 << (SIGWINCH - 1)) | (1 << (SIGCHLD - 1)) |
	    (1 << (SIGXCPU - 1)));
	savupdatewin = _core_updatewin;
	_core_updatewin = FALSE;
	somewinchgsize = FALSE;
	if (savupdatewin & 2)
		sigchildresp();
	if (!(savupdatewin & 1))
		{
		sigsetmask(oldsigmask);
		return;
		}
	for (surfp = &_core_surface[0];surfp < &_core_surface[MAXVSURF];surfp++)
		{
		if (!surfp->vinit)
			continue;
		win_getsize(surfp->windptr->winfd, (Rect *)&rect);
		pw = surfp->windptr->pixwin;
		pw_damaged(pw);
		samesize = (rect.r_width == surfp->windptr->winwidth &&
		    rect.r_height == surfp->windptr->winheight);

		if (rl_empty(&pw->pw_clipdata->pwcd_clipping) && samesize) {

			/* do we HAVE to repaint ? */
			if (surfp->hphardwr) {
				ddstruct.opcode = INQFORCEDREPAINT;
				ddstruct.instance = surfp->vsurf.instance;
				(*surfp->vsurf.dd)(&ddstruct);
				if (!ddstruct.int1) {
					pw_donedamaged(pw);
					continue;
				}
			} else {
				pw_donedamaged(pw);
				continue;
			}
		}

		pw_donedamaged(pw);
		_core_winupdate(surfp->windptr);
		if (surfp->hphardwr)		/* compute GP viewport scale */
			{
			ddstruct.opcode = WINUPDATE;
			ddstruct.instance = surfp->vsurf.instance;
			(*surfp->vsurf.dd)(&ddstruct);
			}
		if (!surfp->windptr->rawdev && 
		    !rl_empty(&pw->pw_clipdata->pwcd_clipping))
			{
			surfp->nwframnd |= 2;
			ddstruct.opcode = CLEAR;
			ddstruct.instance = surfp->vsurf.instance;
			(*surfp->vsurf.dd)(&ddstruct);
			}
		if (!samesize)
			{
			ddstruct.opcode = FLUSHINPUT;
			(viewsurf *) ddstruct.ptr1 = surfp;
			_core_mousedd(&ddstruct);
			somewinchgsize = TRUE;
			}
		}
					/* Do this hack with bit 1 of nwframnd
					   for now.  Should just set nwframnd
					   and call repaint, but the logic in
					   repaint is convoluted and perhaps
					   faulty.  Also that might produce
					   problems when batching updates.
					   Someday batching updates should
					   be fixed.
					*/
	for (segptr = &_core_segment[0]; segptr < &_core_segment[SEGNUM] &&
	    segptr->type != EMPTY; segptr++)
		{
		if (segptr->type < RETAIN)
			continue;
		for (vsindex = 0; vsindex < segptr->vsurfnum; vsindex++)
			{
			surfp = segptr->vsurfptr[vsindex];
			if (surfp->nwframnd & 2)
			    {
		    	    if (surfp->hphardwr)
				{
				ddstruct.opcode = SEGDRAW;
				ddstruct.ptr1 = (char*)segptr;
	 			ddstruct.instance = surfp->vsurf.instance;
				ddstruct.int1 = FALSE;
				ddstruct.int2 = vsindex;
				(*surfp->vsurf.dd)( &ddstruct);
		                }
			    else
				_core_segdraw(segptr,vsindex,FALSE);
			    }
			}
		}
	for (surfp = &_core_surface[0];surfp < &_core_surface[MAXVSURF];surfp++)
		surfp->nwframnd &= ~2;
	if (somewinchgsize)
		{
		if (_core_fastflag)
			_core_set_fastwidth();
		else
			_core_lwflag = TRUE;
		}
	sigsetmask(oldsigmask);
	}

/*----------------------------------------------------------------*/
_core_winupdate(windptr)
register struct windowstruct *windptr;
{
	float ndcratio, winratio;

	win_getsize(windptr->winfd, (Rect *)&windptr->rect);
	windptr->winwidth = windptr->rect.r_width;
	windptr->winheight = windptr->rect.r_height;

	winratio = (float)windptr->winwidth / (float)windptr->winheight;
	ndcratio = (float)windptr->ndcxmax / (float)windptr->ndcymax;

	if ( winratio < ndcratio) {	/* excess height in window */
		windptr->xscale = ((float)windptr->winwidth /
			(float)(windptr->ndcxmax+1)) * (float)32768.;
		windptr->yscale = windptr->xscale;   /* same aspect*/
		windptr->yoff = (windptr->winheight - 1 +
			(windptr->ndcymax*windptr->yscale >> 15)) / 2;
		windptr->xoff = 0;
		}
	else {				/* excess width in window */
		windptr->xscale = ((float)windptr->winheight /
			(float)(windptr->ndcymax+1)) * (float)32768.;
		windptr->yscale = windptr->xscale;   /* same aspect*/
		windptr->xoff = (windptr->winwidth - 1 -
			(windptr->ndcxmax * windptr->xscale >> 15)) / 2;
		windptr->yoff = windptr->winheight -1;
		}
	windptr->xpixmax = (windptr->ndcxmax * windptr->xscale >> 15) +
				windptr->xoff;
	windptr->ypixmax = windptr->yoff - 
			(windptr->ndcymax * windptr->yscale >> 15);
	windptr->xpixdelta = windptr->xpixmax - windptr->xoff;
	windptr->ypixdelta = windptr->ypixmax - windptr->yoff;
	}
/*----------------------------------------------------------------*/
Notify_value
_core_cleanup(me, signal, when)
	int *me;
	int signal;
	Notify_signal_mode when;
{
	terminate_core();

	/* 
	 * Don't return with notifier return code ... just exit.
	 * If users wants to do something special they can interpose
	 * their own SIGINT catcher
	 */
	exit(1);
}
/*-----------------------------------------------------------------------*/
#include <sys/wait.h>
#include <sys/resource.h>

static viewsurf *termlist[MAXVSURF];
static termcount = 0;

Notify_value
_core_sigchild()
	{
	int pid;
	viewsurf *surfp;
	ddargtype ddstruct;
	union wait status;

	while ((pid = wait3(&status, WNOHANG, (struct rusage *)0)) > 0)
		{
		ddstruct.int1 = pid;
		ddstruct.opcode = KILLPID;
		ddstruct.logical = FALSE;
		for (surfp = &_core_surface[0];
		    surfp < &_core_surface[MAXVSURF]; surfp++)
			if (surfp->vinit)
				{
				ddstruct.instance = surfp->vsurf.instance;
				(*surfp->vsurf.dd)(&ddstruct);
				if (ddstruct.logical)
					{
					termlist[termcount++] = surfp;
					break;
					}
				}
		}
	if (_core_critflag)
		_core_updatewin |= 2;
	else {
		sigchildresp();
	}

	return (NOTIFY_DONE);
}

static sigchildresp()
	{
	viewsurf *surfp;

	while (termcount)
		{
		surfp = termlist[--termcount];
		terminate_view_surface(&surfp->vsurf);
		}
	}
/*-----------------------------------------------------------------------*/
