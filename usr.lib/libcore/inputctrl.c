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
static char sccsid[] = "@(#)inputctrl.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */

#include "coretypes.h"
#include "corevars.h"
#include <strings.h>
#define NULL 0

int _core_mousedd();
int _core_keybrdd();

pickstr *_core_check_pick();
keybstr *_core_check_keyboard();
strokstr *_core_check_stroke();
locatstr *_core_check_locator();
valstr *_core_check_valuator();
butnstr *_core_check_button();

initialize_device(devclass, devnum)
    int devclass, devnum;
{
    static char funcname[] = "initialize_device";
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    ddargtype ddstruct;
    viewsurf *surfp;

    _core_ndcset |= 1;
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, FALSE)) == 0)
		return (1);
	    pickptr->subpick.enable = 1;
	    pickptr->subpick.echo = FALSE;
	    pickptr->subpick.echopos[0] = 0;
	    pickptr->subpick.echopos[1] = 0;
	    pickptr->subpick.echosurfp = NULL;
	    pickptr->subpick.instance = 0;
	    pickptr->subpick.devdrive = _core_mousedd;
	    /* init pick aperture 0 */
	    /* will be set differently when an echo surface is specified */
	    pickptr->aperture = 0;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, FALSE)) == 0)
		return (1);
	    keybptr->subkey.enable = 1;
	    keybptr->subkey.echo = FALSE;
	    keybptr->subkey.echopos[0] = 0;
	    keybptr->subkey.echopos[1] = 0;
	    keybptr->subkey.echosurfp = NULL;
	    keybptr->subkey.instance = 0;
	    keybptr->subkey.devdrive = _core_keybrdd;
	    keybptr->initpos = 7;
	    keybptr->initstring = "enter:";
	    keybptr->bufsize = 80;
	    keybptr->maxbufsz = 80;
	    ddstruct.opcode = INITIAL;
	    (*keybptr->subkey.devdrive) (&ddstruct);
	    if (_core_winsys)
		for (surfp = &_core_surface[0];
		     surfp < &_core_surface[MAXVSURF]; surfp++)
		    if (surfp->vinit) {
			ddstruct.opcode = CLRKMASK;
			ddstruct.int1 = surfp->vsurf.windowfd;
			(*keybptr->subkey.devdrive)
			  (&ddstruct);
		    }
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, FALSE)) == 0)
		return (1);
	    strokptr->substroke.enable = 1;
	    strokptr->substroke.echo = FALSE;
	    strokptr->substroke.echopos[0] = 0;
	    strokptr->substroke.echopos[1] = 0;
	    strokptr->substroke.echosurfp = NULL;
	    strokptr->substroke.instance = 0;
	    strokptr->substroke.devdrive = _core_mousedd;
	    strokptr->bufsize = 80;
	    strokptr->distance = 0.01 * MAX_NDC_COORD;
	    strokptr->time = 10000;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, FALSE)) == 0)
		return (1);
	    locatptr->subloc.enable = 1;
	    locatptr->subloc.echo = FALSE;
	    locatptr->subloc.echopos[0] = 0;
	    locatptr->subloc.echopos[1] = 0;
	    locatptr->subloc.echosurfp = NULL;
	    locatptr->subloc.instance = 0;
	    locatptr->subloc.devdrive = _core_mousedd;
	    locatptr->setpos[0] = 0;
	    locatptr->setpos[1] = 0;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, FALSE)) == 0)
		return (1);
	    valptr->subval.enable = 1;
	    valptr->subval.echo = FALSE;
	    valptr->subval.echopos[0] = 0;
	    valptr->subval.echopos[1] = 0;
	    valptr->subval.echosurfp = NULL;
	    valptr->subval.instance = 0;
	    valptr->subval.devdrive = _core_mousedd;
	    valptr->vlinit = 0.0;
	    valptr->vlmin = 0.0;
	    valptr->vlmax = 1.0;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, FALSE)) == 0)
		return (1);
	    butnptr->subbut.enable = 1;
	    butnptr->subbut.echo = FALSE;
	    butnptr->subbut.echopos[0] = 0;
	    butnptr->subbut.echopos[1] = 0;
	    butnptr->subbut.echosurfp = NULL;
	    butnptr->subbut.instance = 0;
	    butnptr->subbut.devdrive = _core_mousedd;
	    butnptr->prompt = 0;
	    ddstruct.opcode = INITBUT;
	    ddstruct.int1 = 4 >> (devnum - 1);
	    (*butnptr->subbut.devdrive) (&ddstruct);
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    return (0);
}

terminate_device(devclass, devnum)
    int devclass, devnum;
{
    static char funcname[] = "terminate_device";
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    viewsurf *surfp;
    ddargtype ddstruct;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (pickptr->subpick.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    pickptr->subpick.enable = FALSE;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (keybptr->subkey.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    keybptr->subkey.enable = FALSE;
	    ddstruct.opcode = TERMINATE;
	    (*keybptr->subkey.devdrive) (&ddstruct);
	    if (_core_winsys)
		for (surfp = &_core_surface[0];
		     surfp < &_core_surface[MAXVSURF]; surfp++)
		    if (surfp->vinit) {
			ddstruct.opcode = SETKMASK;
			ddstruct.int1 = surfp->vsurf.windowfd;
			(*keybptr->subkey.devdrive)
			  (&ddstruct);
		    }
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (strokptr->substroke.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    strokptr->substroke.enable = FALSE;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (locatptr->subloc.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    locatptr->subloc.enable = FALSE;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (valptr->subval.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    valptr->subval.enable = FALSE;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (butnptr->subbut.echosurfp != NULL)
		(void) set_echo_surface(devclass, devnum, (struct vwsurf *) 0);
	    butnptr->subbut.enable = FALSE;
	    ddstruct.opcode = TERMBUT;
	    ddstruct.int1 = 4 >> (devnum - 1);
	    (*butnptr->subbut.devdrive) (&ddstruct);
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    return (0);
}

/* Set echoing Parameters */
set_echo_group(class, devnum, n, echotype)
    int class, devnum[], n, echotype;
{
    int i;

    for (i = 0; i < n; i++) {
	(void) set_echo(class, devnum[i], echotype);
    }
}

set_echo(devclass, devnum, echotype)
    int devclass, devnum, echotype;
{
    static char funcname[] = "set_echo";
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    ddargtype ddstruct;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 2) {
		_core_errhand(funcname, 106);
		return (1);
	    }
	    if (pickptr->subpick.echosurfp == NULL || pickptr->subpick.echo == echotype) {
		pickptr->subpick.echo = echotype;
		break;
	    }
	    if (pickptr->subpick.echo == FALSE || echotype == FALSE) {
		ddstruct.opcode = echotype ? ICURSTRKON : ICURSTRKOFF;
		ddstruct.instance = pickptr->subpick.instance;
		(*pickptr->subpick.devdrive) (&ddstruct);
	    }
	    pickptr->subpick.echo = echotype;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 1) {
		_core_errhand(funcname, 107);
		return (1);
	    }
	    keybptr->subkey.echo = echotype;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 1) {
		_core_errhand(funcname, 108);
		return (1);
	    }
	    if (strokptr->substroke.echosurfp == NULL ||
		strokptr->substroke.echo == echotype) {
		strokptr->substroke.echo = echotype;
		break;
	    }
	    ddstruct.opcode = echotype ? ICURSTRKON : ICURSTRKOFF;
	    ddstruct.instance = strokptr->substroke.instance;
	    (*strokptr->substroke.devdrive) (&ddstruct);
	    strokptr->substroke.echo = echotype;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 6) {
		_core_errhand(funcname, 109);
		return (1);
	    }
	    if (locatptr->subloc.echosurfp == NULL ||
		locatptr->subloc.echo == echotype) {
		locatptr->subloc.echo = echotype;
		break;
	    }
	    if (locatptr->subloc.echo == 1 || echotype == 1) {
		ddstruct.opcode = (echotype == 1) ? ICURSTRKON : ICURSTRKOFF;
		ddstruct.instance = locatptr->subloc.instance;
		(*locatptr->subloc.devdrive) (&ddstruct);
	    }
	    locatptr->subloc.echo = echotype;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 1) {
		_core_errhand(funcname, 110);
		return (1);
	    }
	    valptr->subval.echo = echotype;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (echotype < 0 || echotype > 1) {
		_core_errhand(funcname, 111);
		return (1);
	    }
	    butnptr->subbut.echo = echotype;
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    return (0);
}

set_echo_position(devclass, devnum, x, y)
    int devclass, devnum;
    float x, y;
{
    static char funcname[] = "set_echo_position";
    int ix, iy;
    float f;
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;

    if (x < 0.0 || x > _core_ndc.width || y < 0.0 || y > _core_ndc.height) {
	_core_errhand(funcname, 112);
	return (1);
    }
    f = x;
    f *= MAX_NDC_COORD;
    ix = _core_roundf(&f);
    f = y;
    f *= MAX_NDC_COORD;
    iy = _core_roundf(&f);
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    pickptr->subpick.echopos[0] = ix;
	    pickptr->subpick.echopos[1] = iy;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    keybptr->subkey.echopos[0] = ix;
	    keybptr->subkey.echopos[1] = iy;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    strokptr->substroke.echopos[0] = ix;
	    strokptr->substroke.echopos[1] = iy;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    locatptr->subloc.echopos[0] = ix;
	    locatptr->subloc.echopos[1] = iy;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    valptr->subval.echopos[0] = ix;
	    valptr->subval.echopos[1] = iy;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    butnptr->subbut.echopos[0] = ix;
	    butnptr->subbut.echopos[1] = iy;
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    return (0);
}

set_echo_surface(devclass, devnum, surfname)
    int devclass, devnum;
    struct vwsurf *surfname;
{
    viewsurf *surfp, *_core_VSnsrch();
    static char funcname[] = "set_echo_surface";
    pickstr *pickptr;
    locatstr *locatptr;
    keybstr *keybptr;
    strokstr *strokptr;
    valstr *valptr;
    butnstr *butnptr;
    int savecho;

    if (surfname == NULL)
	surfp = NULL;
    else {
	if (!surfname->dd) {
	    _core_errhand(funcname, 83);
	    return (1);
	}
	if (!(surfp = _core_VSnsrch(surfname))) {
	    _core_errhand(funcname, 5);
	    return (1);
	}
    }
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (pickptr->aperture == 0) { /* non initialized aperture */
		if (surfp->windptr) {
		    /* Initial aperture has 2 pixels on each side */
		    pickptr->aperture = (4 * surfp->windptr->ndcxmax) /
					surfp->windptr->winwidth;
		}
	    }
		
	    if ((savecho = pickptr->subpick.echo) != FALSE)
		(void) set_echo(devclass, devnum, FALSE);
	    if (pickptr->subpick.echosurfp != NULL)
		term_mouse(&pickptr->subpick, TRUE);
	    if ((pickptr->subpick.echosurfp = surfp) != NULL)
		init_mouse(&pickptr->subpick, TRUE);
	    if (savecho != FALSE)
		(void) set_echo(devclass, devnum, savecho);
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    keybptr->subkey.echosurfp = surfp;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    if ((savecho = strokptr->substroke.echo) != FALSE)
		(void) set_echo(devclass, devnum, FALSE);
	    if (strokptr->substroke.echosurfp != NULL)
		term_mouse(&strokptr->substroke, TRUE);
	    if ((strokptr->substroke.echosurfp = surfp) != NULL)
		init_mouse(&strokptr->substroke, TRUE);
	    if (savecho != FALSE)
		(void) set_echo(devclass, devnum, savecho);
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if ((savecho = locatptr->subloc.echo) != FALSE)
		(void) set_echo(devclass, devnum, FALSE);
	    if (locatptr->subloc.echosurfp != NULL)
		term_mouse(&locatptr->subloc, TRUE);
	    if ((locatptr->subloc.echosurfp = surfp) != NULL)
		init_mouse(&locatptr->subloc, TRUE);
	    if (savecho != FALSE)
		(void) set_echo(devclass, devnum, savecho);
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (valptr->subval.echosurfp != NULL)
		term_mouse(&valptr->subval, TRUE);
	    if ((valptr->subval.echosurfp = surfp) != NULL)
		init_mouse(&valptr->subval, TRUE);
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    if (butnptr->subbut.echosurfp != NULL)
		term_mouse(&butnptr->subbut, FALSE);
	    if ((butnptr->subbut.echosurfp = surfp) != NULL)
		init_mouse(&butnptr->subbut, FALSE);
	    break;
	default:
	    _core_errhand(funcname, 105);
	    return (1);
	    break;
    }
    return (0);
}

set_pick(picknum, aperture)
    int picknum;
    float aperture;
{
    static char funcname[] = "set_pick";
    pickstr *pickptr;

    if ((pickptr = _core_check_pick(funcname, picknum, TRUE)) == 0)
	return (1);
    if (aperture <= 0) {
	_core_errhand(funcname, 22);
	return (1);
    }
    pickptr->aperture = (int) (aperture * (float) MAX_NDC_COORD);
    return (0);
}

set_locator_2(locnum, x, y)
    int locnum;
    float x, y;
{
    ddargtype ddstruct;
    float f;
    static char funcname[] = "set_locator_2";
    locatstr *locatptr;

    if ((locatptr = _core_check_locator(funcname, locnum, TRUE)) == 0)
	return (1);
    if (x < 0.0 || x > _core_ndc.width || y < 0.0 || y > _core_ndc.height) {
	_core_errhand(funcname, 112);
	return (1);
    }
    f = x;
    f *= MAX_NDC_COORD;
    ddstruct.int1 = _core_roundf(&f);
    f = y;
    f *= MAX_NDC_COORD;
    ddstruct.int2 = _core_roundf(&f);
    locatptr->setpos[0] = ddstruct.int1;
    locatptr->setpos[1] = ddstruct.int2;
    ddstruct.opcode = XYWRITE;
    ddstruct.instance = locatptr->subloc.instance;
    (*locatptr->subloc.devdrive) (&ddstruct);
    return (0);
}

set_valuator(valnum, init, low, high)
    int valnum;
    float init, low, high;
{
    static char funcname[] = "set_valuator";
    valstr *valptr;

    if ((valptr = _core_check_valuator(funcname, valnum, TRUE)) == 0)
	return (1);
    valptr->vlinit = init;
    valptr->vlmin = low;
    valptr->vlmax = high;
    return (0);
}

set_stroke(strokenum, bufsize, dist, time)
    int strokenum, bufsize, time;
    float dist;
{
    static char funcname[] = "set_stroke";
    strokstr *strokptr;

    if ((strokptr = _core_check_stroke(funcname, strokenum, TRUE)) == 0)
	return (1);
    strokptr->bufsize = bufsize;
    strokptr->distance = dist * MAX_NDC_COORD;
    return (0);
}

static char initstr[81 * KEYBORDS];

set_keyboard(keynum, bufsize, istr, pos)
    int keynum, bufsize, pos;
    char *istr;
{
    int l;
    static char funcname[] = "set_keyboard";
    keybstr *keybptr;
    int offset;

    if ((keybptr = _core_check_keyboard(funcname, keynum, TRUE)) == 0)
	return (1);
    if (bufsize > 0 && bufsize <= keybptr->maxbufsz) {
	offset = (keynum - 1) * 81;
	keybptr->bufsize = bufsize;
	l = strlen(istr);
	if (l > 80)
	    l = 80;
	(void) strncpy(&initstr[offset], istr, l);
	initstr[offset + l] = '\0';
	keybptr->initstring = &initstr[offset];
	keybptr->initpos = pos;
    } else {
	_core_errhand(funcname, 76);
	return (1);
    }
    return (0);
}

static 
init_mouse(devptr, useloc)
    device *devptr;
    int useloc;
{
    ddargtype ddstruct;

    ddstruct.opcode = INITIAL;
    ddstruct.ptr1 = (char *) devptr;
    ddstruct.logical = useloc;
    (*devptr->devdrive) (&ddstruct);
}

static 
term_mouse(devptr, useloc)
    device *devptr;
    int useloc;
{
    ddargtype ddstruct;

    ddstruct.opcode = TERMINATE;
    ddstruct.instance = devptr->instance;
    ddstruct.logical = useloc;
    (*devptr->devdrive) (&ddstruct);
}

pickstr *
_core_check_pick(funcname, picknum, wantinit)
    char *funcname;
    int picknum, wantinit;
{
    pickstr *pickptr;

    if (--picknum < 0 || picknum >= PICKS) {
	_core_errhand(funcname, 69);
	return (0);
    }
    pickptr = &_core_pick[picknum];
    if (wantinit && !(pickptr->subpick.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (pickptr->subpick.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (pickptr);
}

keybstr *
_core_check_keyboard(funcname, keynum, wantinit)
    char *funcname;
    int keynum, wantinit;
{
    keybstr *keybptr;

    if (--keynum < 0 || keynum >= KEYBORDS) {
	_core_errhand(funcname, 75);
	return (0);
    }
    keybptr = &_core_keybord[keynum];
    if (wantinit && !(keybptr->subkey.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (keybptr->subkey.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (keybptr);
}

strokstr *
_core_check_stroke(funcname, strokenum, wantinit)
    char *funcname;
    int strokenum, wantinit;
{
    strokstr *strokptr;

    if (--strokenum < 0 || strokenum >= STROKES) {
	_core_errhand(funcname, 102);
	return (0);
    }
    strokptr = &_core_stroker[strokenum];
    if (wantinit && !(strokptr->substroke.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (strokptr->substroke.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (strokptr);
}

locatstr *
_core_check_locator(funcname, locnum, wantinit)
    char *funcname;
    int locnum, wantinit;
{
    locatstr *locatptr;

    if (--locnum < 0 || locnum >= LOCATORS) {
	_core_errhand(funcname, 70);
	return (0);
    }
    locatptr = &_core_locator[locnum];
    if (wantinit && !(locatptr->subloc.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (locatptr->subloc.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (locatptr);
}

valstr *
_core_check_valuator(funcname, valnum, wantinit)
    char *funcname;
    int valnum, wantinit;
{
    valstr *valptr;

    if (--valnum < 0 || valnum >= VALUATRS) {
	_core_errhand(funcname, 72);
	return (0);
    }
    valptr = &_core_valuatr[valnum];
    if (wantinit && !(valptr->subval.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (valptr->subval.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (valptr);
}

butnstr *
_core_check_button(funcname, butnum, wantinit)
    char *funcname;
    int butnum, wantinit;
{
    butnstr *butnptr;

    if (--butnum < 0 || butnum >= BUTTONS) {
	_core_errhand(funcname, 77);
	return (0);
    }
    butnptr = &_core_button[butnum];
    if (wantinit && !(butnptr->subbut.enable & 1)) {
	_core_errhand(funcname, 104);
	return (0);
    } else if (!wantinit && (butnptr->subbut.enable & 1)) {
	_core_errhand(funcname, 103);
	return (0);
    }
    return (butnptr);
}
