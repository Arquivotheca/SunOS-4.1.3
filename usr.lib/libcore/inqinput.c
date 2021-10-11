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
static char sccsid[] = "@(#)inqinput.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif

/*
 * Copyright (c) 1986 by Sun Microsystems, Inc.
 */
#include "coretypes.h"
#include "corevars.h"
extern struct vwsurf _core_nullvs;

pickstr *_core_check_pick();
keybstr *_core_check_keyboard();
strokstr *_core_check_stroke();
locatstr *_core_check_locator();
valstr *_core_check_valuator();
butnstr *_core_check_button();

/*
 * Inquire echoing parameters.
 */
inquire_echo(devclass, devnum, echotype)
    int devclass, devnum, *echotype;
{
    static char funcname[] = "inquire_echo";
    pickstr *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = pickptr->subpick.echo;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = keybptr->subkey.echo;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = butnptr->subbut.echo;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = locatptr->subloc.echo;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = valptr->subval.echo;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    *echotype = strokptr->substroke.echo;
	    break;
	default:
	    break;
    }

    return (0);
}

/*
 * Inquire echo position.
 */
inquire_echo_position(devclass, devnum, x, y)
    int devclass, devnum;
    float *x, *y;
{
    static char funcname[] = "inquire_echo_position";
    pickstr *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = pickptr->subpick.echopos[0];
	    *y = pickptr->subpick.echopos[1];
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = keybptr->subkey.echopos[0];
	    *y = keybptr->subkey.echopos[1];
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = butnptr->subbut.echopos[0];
	    *y = butnptr->subbut.echopos[1];
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = locatptr->subloc.echopos[0];
	    *y = locatptr->subloc.echopos[1];
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = valptr->subval.echopos[0];
	    *y = valptr->subval.echopos[1];
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    *x = strokptr->substroke.echopos[0];
	    *y = strokptr->substroke.echopos[1];
	    break;
	default:
	    break;
    }
    *x /= (float) MAX_NDC_COORD;/* users NDC is 0..1.0 */
    *y /= (float) MAX_NDC_COORD;
    return (0);
}

/*
 * Inquire echo surface.
 */
inquire_echo_surface(devclass, devnum, surfname)
    int devclass, devnum;
    struct vwsurf *surfname;
{
    viewsurf *surfp;
    static char funcname[] = "inquire_echo_surface";
    pickstr *pickptr;
    keybstr *keybptr;
    strokstr *strokptr;
    locatstr *locatptr;
    valstr *valptr;
    butnstr *butnptr;

    surfp = (viewsurf *) 0;
    switch (devclass) {
	case PICK:
	    if ((pickptr = _core_check_pick(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = pickptr->subpick.echosurfp;
	    break;
	case KEYBOARD:
	    if ((keybptr = _core_check_keyboard(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = keybptr->subkey.echosurfp;
	    break;
	case BUTTON:
	    if ((butnptr = _core_check_button(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = butnptr->subbut.echosurfp;
	    break;
	case LOCATOR:
	    if ((locatptr = _core_check_locator(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = locatptr->subloc.echosurfp;
	    break;
	case VALUATOR:
	    if ((valptr = _core_check_valuator(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = valptr->subval.echosurfp;
	    break;
	case STROKE:
	    if ((strokptr = _core_check_stroke(funcname, devnum, TRUE)) == 0)
		return (1);
	    surfp = strokptr->substroke.echosurfp;
	    break;
	default:
	    break;
    }
    if (surfp)
	*surfname = surfp->vsurf;
    else
	*surfname = _core_nullvs;
    return (0);
}

/*
 * Inquire 2D locator.
 */
inquire_locator_2(locnum, x, y)
    int locnum;
    float *x, *y;
{
    static char funcname[] = "inquire_locator_2";
    locatstr *locatptr;

    if ((locatptr = _core_check_locator(funcname, locnum, TRUE)) == 0)
	return (1);
    *x = (float) locatptr->setpos[0] / (float) MAX_NDC_COORD;
    *y = (float) locatptr->setpos[1] / (float) MAX_NDC_COORD;
    return (0);
}

/*
 * Inquire valuator.
 */
inquire_valuator(valnum, init, low, high)
    int valnum;
    float *init, *low, *high;
{
    static char funcname[] = "inquire_valuator";
    valstr *valptr;

    if ((valptr = _core_check_valuator(funcname, valnum, TRUE)) == 0)
	return (1);
    *init = valptr->vlinit;
    *low = valptr->vlmin;
    *high = valptr->vlmax;
    return (0);
}

/*
 * Inquire stroke.
 */
inquire_stroke(strokenum, bufsize, dist, time)
    int strokenum, *bufsize, *time;
    float *dist;
{
    static char funcname[] = "inquire_stroke";
    strokstr *strokptr;

    if ((strokptr = _core_check_stroke(funcname, strokenum, TRUE)) == 0)
	return (1);
    *bufsize = strokptr->bufsize;
    *dist = (float) strokptr->distance / (float) MAX_NDC_COORD;
    return (0);
}

/*
 * Inquire keyboard.
 */
inquire_keyboard(keynum, bufsize, istr, pos)
    int keynum, *bufsize, *pos;
    char *istr;
{
    static char funcname[] = "inquire_keyboard";
    keybstr *keybptr;
    char *sptr;

    if ((keybptr = _core_check_keyboard(funcname, keynum, TRUE)) == 0)
	return (1);
    *bufsize = keybptr->bufsize;
    sptr = keybptr->initstring;
    while (*istr++ = *sptr++);
    *pos = keybptr->initpos;
    return (0);
}
