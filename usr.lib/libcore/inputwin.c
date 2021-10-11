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
static char sccsid[] = "@(#)inputwin.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
 * Device dependent core graphics driver for the mouse and keyboard,
 * uses the window system input routines.
 * June 12, 83 by M. Shantz
 *
 */
#include	"coretypes.h"
#include	"corevars.h"
#include	<sys/types.h>
#include	<sys/file.h>
#include	<sys/time.h>
#include	<stdio.h>
#include	<signal.h>
#include	<pixrect/pixrect.h>
#include	<pixrect/pr_util.h>
#include	<pixrect/memvar.h>

#include	<sunwindow/rect.h>
#include	<sunwindow/rectlist.h>
#include	<sunwindow/pixwin.h>
#include	<sunwindow/win_struct.h>
#include	<sunwindow/win_environ.h>
#include	<sunwindow/win_screen.h>
#include	<sunwindow/win_input.h>
#include	<sunwindow/win_cursor.h>
#include	"corewinvars.h"

static int hit, buttonstate;		/* button down since last polled */
static int mousex, mousey, newxy;
static int ndcmousex, ndcmousey;
static struct inputmask im;
static char kbuf[64];
static int nchar, kbiptr, kboptr;
static struct timeval timeval = { 0, 0};
static struct inputevent event;
static short blnklist[CUR_MAXIMAGEWORDS];
mpr_static(_core_blankcursorpr,0,0,1,blnklist);
static struct cursor blankcursor = {0,0,PIX_SRC,&_core_blankcursorpr};
static short msklist[CUR_MAXIMAGEWORDS];
mpr_static(_core_oldcursorpr,16,16,1,msklist);
static int rwin;

/*--------------------------------------------------------------------*/
_core_winmouse(ddstruct) register ddargtype *ddstruct;
{
    switch(ddstruct->opcode) {
    case INITIAL:
	input_imnull( &im);
	win_setinputcodebit( &im, MS_LEFT);
	win_setinputcodebit( &im, MS_MIDDLE);
	win_setinputcodebit( &im, MS_RIGHT);
	win_setinputcodebit( &im, LOC_MOVE);
	im.im_flags |= IM_ASCII;
	im.im_flags |= IM_NEGEVENT;
	/*
	 * Save old cursor and old input mask
	 */
	_core_oldcursor.cur_shape = &_core_oldcursorpr;
	if (!_core_maskhaschanged)
		win_getinputmask( _core_winfd, &_core_oldim, &rwin);
	if (!_core_curshaschanged)
		win_getcursor( _core_winfd, &_core_oldcursor);
	_core_maskhaschanged = TRUE;
	_core_curshaschanged = TRUE;
	/*
	 * Load blank cursor
	 */
	win_setinputmask( _core_winfd, &im, &im, rwin);
	win_setcursor( _core_winfd, &blankcursor);
	mousex = 0; mousey = 0;			/* loc in window coords */
	buttonstate = hit = 0;
	break;
    case TERMINATE:
	if ( _core_maskhaschanged)
		win_setinputmask( _core_winfd, &_core_oldim, 0, rwin);
	if ( _core_curshaschanged)
		win_setcursor( _core_winfd, &_core_oldcursor);
	break;
    case BUTREAD:
    case XYREAD:
	rdinpevents();
	ddstruct->logical = FALSE;
	if (ddstruct->opcode == BUTREAD) {
		ddstruct->int3 = hit;
		ddstruct->int2 = buttonstate;
		if ( hit & ddstruct->int1) {		/* 4=L,2=M,1=R */
			ddstruct->logical = TRUE;
			hit &= ~ddstruct->int1;
			}
		}
	else {
		if (newxy)
			{
			newxy = 0;
			ndcmousex = (mousex * _core_ndcspace[0]) /
					_core_winwidth;
			ndcmousey = (mousey * -_core_ndcspace[1])/
					_core_winheight + _core_ndcspace[1];
			}
		ddstruct->int1 = ndcmousex;
		ddstruct->int2 = ndcmousey;
		ddstruct->int3 = buttonstate;
		}
	break;
    case XYWRITE:
	ndcmousex = ddstruct->int1;
	ndcmousey = ddstruct->int2;
	mousex = (ndcmousex * _core_winwidth) / _core_ndcspace[0];
	mousey = ((ndcmousey - _core_ndcspace[1]) * _core_winheight) /
			(- _core_ndcspace[1]);
	win_setmouseposition(_core_winfd, mousex, mousey);
	break;
    default: break;
    }
}

/*----------------------------------------------------------------------*/
/* Device dependent core graphics driver for the window keyboard
 * 14-Jun-83 by Mike Shantz.
 */
/*--------------------------------------------------*/
_core_winkey(ddstruct) register ddargtype *ddstruct;
{
    int count;
    char *ch;

    switch(ddstruct->opcode) {
    case INITIAL:
	input_imnull( &im);
	win_setinputcodebit( &im, MS_LEFT);
	win_setinputcodebit( &im, MS_MIDDLE);
	win_setinputcodebit( &im, MS_RIGHT);
	win_setinputcodebit( &im, LOC_MOVE);
	im.im_flags |= IM_ASCII;
	im.im_flags |= IM_NEGEVENT;
	if (!_core_maskhaschanged)
		win_getinputmask( _core_winfd, &_core_oldim, &rwin);
	win_setinputmask( _core_winfd, &im, &im, rwin);
	_core_maskhaschanged = TRUE;
	kbiptr = 0;
	kboptr = 0;
	nchar = 0;
	break;
    case TERMINATE:
	if ( _core_maskhaschanged)
		win_setinputmask( _core_winfd, &_core_oldim, 0, rwin);
	break;
    case KEYREAD:
	ch = ddstruct->ptr1;
	count = 0;
	while ((nchar > 0) && (count < ddstruct->int1))
		{
		count++;
		*ch++ = *(kbuf + kboptr);
		nchar--;
		kboptr = (kboptr + 1) & 63;
		}
	if (count >= ddstruct->int1)
		break;
	rdinpevents();
	while ((nchar > 0) && (count < ddstruct->int1))
		{
		count++;
		*ch++ = *(kbuf + kboptr);
		nchar--;
		kboptr = (kboptr + 1) & 63;
		}
	ddstruct->int1 = count;
	break;
    default:
	    break;
    }
}

static rdinpevents()
	{
	int rfd;

	rfd = 1<<_core_winfd;
	while ( select( 16, &rfd, 0, 0, &timeval) > 0) {
	    input_readevent( _core_winfd, &event);
	    if (event.ie_code == LOC_MOVE) {		/* mouse move event */
		mousex = event.ie_locx;
		mousey = event.ie_locy;
		newxy++;
		}
	    else if (event.ie_code == MS_LEFT){		/* button events */
		if (win_inputposevent(&event)) {
		    hit |= 4;
		    buttonstate |= 4;
		    } else buttonstate &= ~4;
		}
	    else if (event.ie_code == MS_MIDDLE) {
		if (win_inputposevent(&event)) {
		    hit |= 2;
		    buttonstate |= 2;
		    } else buttonstate &= ~2;
		}
	    else if (event.ie_code == MS_RIGHT) {
		if (win_inputposevent(&event)) {
		    hit |= 1;
		    buttonstate |= 1;
		    } else buttonstate &= ~1;
		}
	    else if (event.ie_code <= ASCII_LAST) {
		if (!win_inputnegevent(&event)) {
		    if (nchar < 64)
			{
			*(kbuf + kbiptr) = (char) event.ie_code;
			nchar++;
			kbiptr = (kbiptr + 1) & 63;
			}
		    }
		}
	    }
	}
