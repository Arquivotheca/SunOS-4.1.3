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
static char sccsid[] = "@(#)inputdds.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */
/*
 * Device dependent core graphics driver for the mouse and keyboard
 * 22-Oct-82 by Mike Shantz.
 *
 */
#include "coretypes.h"
#include <stdio.h>
#include <sgtty.h>
#include <sunwindow/window_hs.h>
#define NULL 0

static struct mouse_input_data
	{
	int windowfd;
	int refcount;
	int loccount;
	int curstrkcnt;
	int hit, buttonstate;
	short mousex, mousey, newxy;
	int ndcmousex, ndcmousey;
	viewsurf *surfp;
	struct inputmask saved_mask;
	int saved_designee;
	struct mouse_input_data *next;
	} win_input_data[MAXVSURF];

static struct mouse_input_data *idlst = 0;

/*----------------------------------------------------------------------*/
/* Device dependent core graphics driver for the keyboard
22-Oct-82 by Mike Shantz.
*/

static int kbfd = -1;
static short ttyflags;
static struct sgttyb ttypar;
char backspace_character;

/*--------------------------------------------------*/
_core_keybrdd(ddstruct) register ddargtype *ddstruct;
{
    int count;

    switch(ddstruct->opcode) {
    case INITIAL:
	if (kbfd >= 0)
		break;
	kbfd = fileno( stdin);
	(void)gtty( kbfd, &ttypar);
	ttyflags = ttypar.sg_flags;
	ttypar.sg_flags |= CBREAK;
	ttypar.sg_flags &= ~ECHO;
	(void)stty( kbfd, &ttypar);
	backspace_character = ttypar.sg_erase;
	break;
    case TERMINATE:
	if (kbfd < 0)
		break;
	ttypar.sg_flags = ttyflags;	/* restore tty to original setting */
	(void)stty( kbfd, &ttypar);
	kbfd = -1;
	break;
    case KEYREAD:
	(void)ioctl( kbfd, FIONREAD, &count);
	count = (count <= ddstruct->int1) ? count : ddstruct->int1;
	ddstruct->int1 = read( kbfd, ddstruct->ptr1, count);
	break;
    case SETKMASK:
    case CLRKMASK:
	{
	struct inputmask im;
	struct mouse_input_data *idptr;
	int dsgn;

	(void)win_getinputmask(ddstruct->int1, &im, &dsgn);
	if (ddstruct->opcode == SETKMASK)
		im.im_flags |= IM_ASCII;
	else
		im.im_flags &= ~IM_ASCII;
	(void)win_setinputmask(ddstruct->int1, &im, (struct inputmask *)0, dsgn);
	idptr = idlst;
	while (idptr)
		if (idptr->windowfd == ddstruct->int1)
			{
			if (ddstruct->opcode == SETKMASK)
				idptr->saved_mask.im_flags |= IM_ASCII;
			else
				idptr->saved_mask.im_flags &= ~IM_ASCII;
			break;
			}
	}
	break;
    default:
	    break;
    }
}

/*
 * Device dependent core graphics driver for the mouse and keyboard,
 * uses the window system input routines.
 * June 12, 83 by M. Shantz
 *
 */

static struct timeval timeval = { 0, 0};
static struct inputevent event;
static short left_but_init=FALSE,middle_but_init=FALSE,right_but_init=FALSE;

/*--------------------------------------------------------------------*/
_core_mousedd(ddstruct) register ddargtype *ddstruct;
{
register struct mouse_input_data *idptr;
register struct windowstruct *windptr;

    idptr = (struct mouse_input_data *) ddstruct->instance;
    switch(ddstruct->opcode) {
    case INITIAL:
	{
	viewsurf *surfp;
	struct mouse_input_data **idptrptr;
	struct inputmask im;
	int dsgn;

	idptr = idlst;
	surfp = ((device *)ddstruct->ptr1)->echosurfp;
	while (idptr)
		{
		if (idptr->windowfd == surfp->vsurf.windowfd)
			break;
		idptr = idptr->next;
		}
	if (idptr)
		{
		idptr->refcount++;
		if (ddstruct->logical && (idptr->loccount++ == 0))
				/* want locator input as well as buttons */
			{
			(void)win_getinputmask(idptr->windowfd, &im, &dsgn);
			win_setinputcodebit( &im, LOC_MOVE);
			(void)win_setinputmask(idptr->windowfd, &im, (struct inputmask *)0, dsgn);
			}
		((device *)ddstruct->ptr1)->instance = (int) idptr;
		break;
		}
	for (idptr = &win_input_data[0]; idptr->refcount > 0; idptr++)
		;
	idptrptr = &idlst;
	while (*idptrptr != NULL)
		idptrptr = &(*idptrptr)->next;
	*idptrptr = idptr;
	idptr->next = NULL;
	idptr->windowfd = surfp->vsurf.windowfd;
	idptr->refcount = 1;
	idptr->loccount = 0;
	idptr->curstrkcnt = 0;
	idptr->surfp = surfp;
	(void)win_getinputmask(idptr->windowfd, &idptr->saved_mask,
	    &idptr->saved_designee);
	im = idptr->saved_mask;
	win_setinputcodebit( &im, MS_LEFT);
	win_setinputcodebit( &im, MS_MIDDLE);
	win_setinputcodebit( &im, MS_RIGHT);
	if (ddstruct->logical)
		{
		idptr->loccount = 1;
		win_setinputcodebit(&im, LOC_MOVE);
		}
	im.im_flags |= IM_NEGEVENT | IM_INTRANSIT;
	(void)win_setinputmask(idptr->windowfd, &im, (struct inputmask *)0, idptr->saved_designee);
	rdinpevents(idptr);	     /* flush previous events in this window */
	idptr->buttonstate = idptr->hit = 0;
	idptr->mousex = surfp->windptr->xoff;	     /* loc in window coords */
	idptr->mousey = surfp->windptr->yoff;
	idptr->ndcmousex = 0; idptr->ndcmousey = 0;  /* loc in ndc coords */
	idptr->newxy = 0;
	((device *)ddstruct->ptr1)->instance = (int) idptr;
	}
	break;
    case TERMINATE:
	{
	struct mouse_input_data **idptrptr;
	struct inputmask im;
	int dsgn;

	if (--idptr->refcount > 0)
		{
		if (ddstruct->logical && (--idptr->loccount == 0))
			{
			(void)win_getinputmask(idptr->windowfd, &im, &dsgn);
			win_unsetinputcodebit( &im, LOC_MOVE);
			(void)win_setinputmask(idptr->windowfd, &im, (struct inputmask *)0, dsgn);
			}
		break;
		}
	rdinpevents(idptr);	     /* flush remaining events in this window */
	idptrptr = &idlst;
	while (*idptrptr != idptr)
		idptrptr = &(*idptrptr)->next;
	*idptrptr = idptr->next;
	(void)win_setinputmask(idptr->windowfd, &idptr->saved_mask, (struct inputmask *)0,
	    idptr->saved_designee);
	}
	break;
    case BUTREAD:
    case XYREAD:
	rdinpevents(idptr);
	ddstruct->logical = FALSE;
	if (ddstruct->opcode == BUTREAD) {
		ddstruct->int3 = idptr->hit;
		ddstruct->int2 = idptr->buttonstate;
		if ( idptr->hit & ddstruct->int1) {	/* 4=L,2=M,1=R */
			ddstruct->logical = TRUE;
			idptr->hit &= ~ddstruct->int1;
			}
		}
	else {
		if (idptr->newxy)
			{
			idptr->newxy = 0;
			windptr = idptr->surfp->windptr;
			if ((idptr->mousex -= windptr->xoff) >= 0 &&
			    idptr->mousex <= windptr->xpixdelta &&
			    (idptr->mousey -= windptr->yoff) <= 0 &&
			    idptr->mousey >= windptr->ypixdelta)
				{
				idptr->ndcmousex =
				    (idptr->mousex * windptr->ndcxmax) /
				    windptr->xpixdelta;
				idptr->ndcmousey =
				    (idptr->mousey * windptr->ndcymax) /
				    windptr->ypixdelta;
			}
			}
		ddstruct->int1 = idptr->ndcmousex;
		ddstruct->int2 = idptr->ndcmousey;
		ddstruct->int3 = idptr->buttonstate;
		}
	break;
    case XYWRITE:
	windptr = idptr->surfp->windptr;
	idptr->ndcmousex = ddstruct->int1;
	idptr->ndcmousey = ddstruct->int2;
	idptr->mousex =
	    (idptr->ndcmousex * windptr->winwidth) / windptr->ndcxmax;
	idptr->mousey =
	    ((idptr->ndcmousey - windptr->ndcymax) * windptr->winheight) /
			(- windptr->ndcymax);
	win_setmouseposition(windptr->winfd, idptr->mousex, idptr->mousey);
	break;
    case INITBUT:
    case TERMBUT:
	{int i;

	if (ddstruct->opcode == INITBUT)	/* Since blanket windows must */
		i = TRUE;			/* always accept buttondowns  */
	else					/* at least for release 1.1,  */
		i = FALSE;			/* do button enabling this way*/
	if (ddstruct->int1 == 4)		/* rather than by input mask  */
		left_but_init = i;
	else if (ddstruct->int1 == 2)
		middle_but_init = i;
	else if (ddstruct->int1 == 1)
		right_but_init = i;
	}
	break;
    case ICURSTRKON:
	if (idptr->curstrkcnt++ == 0)
		turn_on_cursor(idptr);
	break;
    case ICURSTRKOFF:
	if (--idptr->curstrkcnt == 0)
		turn_off_cursor(idptr);
	break;
    case FLUSHINPUT:
	idptr = idlst;
	while (idptr)
		{
		if (idptr->surfp == (viewsurf *)ddstruct->ptr1)
			{
			rdinpevents(idptr);
			idptr->buttonstate = idptr->hit = 0;
			idptr->mousex = idptr->surfp->windptr->xoff;
			idptr->mousey = idptr->surfp->windptr->yoff;
			idptr->ndcmousex = 0; idptr->ndcmousey = 0;
			idptr->newxy = 0;
			}
		}
	break;
    default: break;
    }
}

static rdinpevents(idptr)
register struct mouse_input_data *idptr;
	{

#ifndef PRE4.0	    /* as of 4.0, the interface to select() changed */
	int rfd;
	fd_set	readfds;

	rfd = idptr->windowfd; 
	FD_ZERO(&readfds);
	FD_SET(rfd, &readfds);
		while(select(getdtablesize(), &readfds,
			(fd_set *)0,(fd_set *)0,&timeval) > 0) { 
#else
	int rfd, readfds;

	rfd = idptr->windowfd;
	readfds = 1 << rfd;
	while ( select( 32, &readfds, (int *)0, (int *)0, &timeval) > 0) {
#endif
	    (void)input_readevent( rfd, &event);
	    /* all events have location information: */
	    /* get location even if event is not a move event */
	    /* the window system "collapses" events so this is */
	    /* necessary: especially for the first event when the */
	    /* mosue has not been moved */
	    idptr->mousex = event.ie_locx;
	    idptr->mousey = event.ie_locy;
	    idptr->newxy++;
	    if (event.ie_code == MS_LEFT && left_but_init){
							/* button events */
		if (win_inputposevent(&event)) {
		    idptr->hit |= 4;
		    idptr->buttonstate |= 4;
		    } else idptr->buttonstate &= ~4;
		}
	    else if (event.ie_code == MS_MIDDLE && middle_but_init) {
		if (win_inputposevent(&event)) {
		    idptr->hit |= 2;
		    idptr->buttonstate |= 2;
		    } else idptr->buttonstate &= ~2;
		}
	    else if (event.ie_code == MS_RIGHT && right_but_init) {
		if (win_inputposevent(&event)) {
		    idptr->hit |= 1;
		    idptr->buttonstate |= 1;
		    } else idptr->buttonstate &= ~1;
		}
	    }
	}

mpr_static(blankc_mpr, 0, 0, 0, 0);
static struct cursor blank_cursor = {0, 0, PIX_SRC^PIX_DST, &blankc_mpr};

static short dingbat[] = { 0x0020, 0x0070, 0x009c, 0x0106,
			   0x0083, 0x01c9, 0x03e2, 0x07fe,
			   0x0ff0, 0x1ff0, 0x3ff0, 0x3fe0,
			   0x77e0, 0x6140, 0xe000, 0xc000};
mpr_static(dingbat_mpr, 16, 16, 1, dingbat);
static struct cursor dingbat_cursor = {0, 15, PIX_SRC^PIX_DST, &dingbat_mpr};

static turn_on_cursor(idptr)
register struct mouse_input_data *idptr;
	{
	ddargtype ddstruct;

	ddstruct.opcode = CURSTRKON;
	ddstruct.instance = idptr->surfp->vsurf.instance;
	(*idptr->surfp->vsurf.dd)(&ddstruct);
	(void)win_setcursor(idptr->windowfd, &dingbat_cursor);
	}

static turn_off_cursor(idptr)
register struct mouse_input_data *idptr;
	{
	ddargtype ddstruct;

	(void)win_setcursor(idptr->windowfd, &blank_cursor);
	ddstruct.opcode = CURSTRKOFF;
	ddstruct.instance = idptr->surfp->vsurf.instance;
	(*idptr->surfp->vsurf.dd)(&ddstruct);
	}
