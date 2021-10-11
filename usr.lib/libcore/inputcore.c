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
static char sccsid[] = "@(#)inputcore.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 * Copyright (c) 1983 by Sun Microsystems, Inc.
 */
/*
 * Device dependent core graphics driver for the mouse and keyboard
 * 22-Oct-82 by Mike Shantz.
 *
 */
#include	"coretypes.h"
#include	"corevars.h"
#include	<stdio.h>
#include	<sgtty.h>
#include 	<sys/types.h>

static int fd;
static int hit, last;
static int mousex, mousey, dx, dy;
static int on=1;

/*--------------------------------------------------------------------*/
_core_mousedd(ddstruct) register ddargtype *ddstruct;
{
    unsigned char but;
    char xy[2];
    int n, accel;

    switch(ddstruct->opcode) {
    case INITIAL:
	fd = open("/dev/mouse", 0);
	ioctl( fd, FIONBIO, &on);
	mousex = mousey = dx = dy = 0;
	hit = last = 0;			/* no buttons have been hit */
	break;
    case TERMINATE:
	close( fd);
	break;
    case BUTREAD:
    case XYREAD:
	do {				/* read all mouse input */
	    if (read( fd, &but, 1) < 1) break;
	    if ((but & 0xf8) == 0x80){
		but = (~but & 7);	/* change so one bit means down */
		hit |= but & ~last;	/* update buttons having been hit */
		last = but;
		if (read( fd, xy, 2) < 2) break;
		dx += xy[0];
		dy += xy[1];
		}
	    }
	while (TRUE);
	ddstruct->logical = FALSE;
	if (ddstruct->opcode == BUTREAD) {	/* return button info */
	    ddstruct->int3 = hit;
	    if (hit & ddstruct->int1) {		/* polled button has been hit */
		ddstruct->logical = TRUE;	/* tell caller */
		hit &= ~( ddstruct->int1);	/* turn hit off */
		}
	    ddstruct->int2 = but;
	    }
    	else {
	    accel =  ((abs(dx) + abs(dy)) < 4) ? 5 : 6;
	    mousex += dx << accel;
	    mousey += dy << accel;		/* mouse origin at lower left */
	    dx = dy = 0;
	    if (mousex < 0) mousex = 0;		/* clamp to NDC */
	    else if (mousex > _core_ndcspace[0]) mousex = _core_ndcspace[0];
	    if (mousey < 0) mousey = 0;
	    else if (mousey > _core_ndcspace[1]) mousey = _core_ndcspace[1];
	    ddstruct->int1 = mousex;
	    ddstruct->int2 = mousey;
	    ddstruct->int3 = but;
	    }
	break;
    case XYWRITE:
	mousex = ddstruct->int1;
	mousey = ddstruct->int2;
	break;
    default:
	    break;
    }
}

/*----------------------------------------------------------------------*/
/* Device dependent core graphics driver for the keyboard
22-Oct-82 by Mike Shantz.
*/

static int fildes;
static short ttyflags;
static struct sgttyb ttypar, *argp;
int ioctl();

/*--------------------------------------------------*/
_core_keybrdd(ddstruct) register ddargtype *ddstruct;
{
    int i, count,i3;

    switch(ddstruct->opcode) {
    case INITIAL:
	argp = &ttypar;			/* set tty for polling */
	fildes = fileno( stdin);
	gtty( fildes, argp);
	ttyflags = argp->sg_flags;
	argp->sg_flags |= CBREAK;
	argp->sg_flags &= ~ECHO;
	stty( fildes, argp);
	break;
    case TERMINATE:
	argp->sg_flags = ttyflags;	/* restore tty to original setting */
	stty( fildes, argp);
	break;
    case KEYREAD:
	ioctl( fildes, FIONREAD, &count);
	count = (count <= ddstruct->int1) ? count : ddstruct->int1;
	ddstruct->int1 = read( fildes, ddstruct->ptr1, count);
	break;
    default:
	    break;
    }
}
