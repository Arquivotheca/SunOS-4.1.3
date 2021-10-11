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
static char sccsid[] = "@(#)inprims77.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif

/*
 * Copyright (c) 1984 by Sun Microsystems, Inc.
 */

#include "f77strings.h"

int await_any_button();
int await_any_button_get_locator_2();
int await_any_button_get_valuator();
int await_keyboard();
int await_pick();
int await_stroke_2();
int get_mouse_state();
int initialize_device();
int set_echo();
int set_echo_group();
int set_echo_position();
int set_echo_surface();
int set_keyboard();
int set_pick();
int set_locator_2();
int set_stroke();
int set_valuator();
int terminate_device();

int awaitanybutton_(tim, butnum)
int *tim, *butnum;
	{
	return(await_any_button(*tim, butnum));
	}

int awtbuttongetloc2_(tim, locnum, butnum, x, y)
int *tim, *locnum, *butnum;
float *x, *y;
	{
	return(await_any_button_get_locator_2(*tim, *locnum, butnum, x, y));
	}

int awtbuttongetloc2(tim, locnum, butnum, x, y)
int *tim, *locnum, *butnum;
float *x, *y;
	{
	return(await_any_button_get_locator_2(*tim, *locnum, butnum, x, y));
	}

int awtbuttongetval_(tim, valnum, butnum, val)
int *tim, *valnum, *butnum;
float *val;
	{
	return(await_any_button_get_valuator(*tim, *valnum, butnum, val));
	}

int awaitkeyboard_(tim, keynum, f77string, length, f77strleng)
int *tim, *keynum, *length, f77strleng;
char *f77string;
	{
	char c, *sptr, *f77sptr;
	int f, i, n;

	f = await_keyboard(*tim, *keynum, _core_77string, length);
	f77sptr = f77string;
	sptr = _core_77string;
	while (c = *sptr++)
		*f77sptr++ = c;
	n = f77strleng - (f77sptr - f77string);
	for (i = 0; i < n; i++)
		*f77sptr++ = ' ';
	return(f);
	}

int awaitpick_(tim, picknum, segnam, pickid)
int *tim, *picknum, *segnam, *pickid;
	{
	return(await_pick(*tim, *picknum, segnam, pickid));
	}

int awaitstroke2_(tim, strokenum, arraysize, xarray, yarray, numxy)
int *tim, *strokenum, *arraysize, *numxy;
float *xarray, *yarray;
    {
    return(await_stroke_2(*tim, *strokenum, *arraysize, xarray, yarray, numxy));
    }

int getmousestate_(devclass, devnum, x, y, buttons)
int *devclass, *devnum;
int *buttons;
float *x, *y;
	{
	return(get_mouse_state(*devclass, *devnum, x, y, buttons));
	}

int initializedevice_(devclass, devnum)
int *devclass, *devnum;
	{
	return(initialize_device(*devclass, *devnum));
	}

int initializedevice(devclass, devnum)
int *devclass, *devnum;
	{
	return(initialize_device(*devclass, *devnum));
	}

int setecho_(devclass, devnum, echotype)
int *devclass, *devnum, *echotype;
	{
	return(set_echo(*devclass, *devnum, *echotype));
	}

int setechogroup_(devclass, devnumarray, n, echotype)
int *devclass, *devnumarray, *n, *echotype;
	{
	return(set_echo_group(*devclass, devnumarray, *n, *echotype));
	}

int setechoposition_(devclass, devnum, x, y)
int *devclass, *devnum;
float *x, *y;
	{
	return(set_echo_position(*devclass, *devnum, *x, *y));
	}

#define DEVNAMESIZE 20

struct vwsurf	{
		char screenname[DEVNAMESIZE];
		char windowname[DEVNAMESIZE];
		int windowfd;
		int (*dd)();
		int instance;
		int cmapsize;
		char cmapname[DEVNAMESIZE];
		int flags;
		char **ptr;
		};

int setechosurface_(devclass, devnum, surfname)
int *devclass, *devnum;
struct vwsurf *surfname;
	{
	return(set_echo_surface(*devclass, *devnum, surfname));
	}

int setkeyboard_(keynum, bufsize, istr, pos, f77strleng)
int *keynum, *bufsize, *pos, f77strleng;
char *istr;
	{
	char *sptr;

	f77strleng = f77strleng > MAXLEN ? MAXLEN : f77strleng;
	sptr = _core_77string;
	while (f77strleng--)
		*sptr++ = *istr++;
	*sptr = '\0';
	return(set_keyboard(*keynum, *bufsize, _core_77string, *pos));
	}

int setpick_(picknum,aperture)
int *picknum;
float *aperture;
	{
	return(set_pick(*picknum,*aperture));
	}

int setlocator2_(locnum, x, y)
int *locnum;
float *x, *y;
	{
	return(set_locator_2(*locnum, *x, *y));
	}

int setstroke_(strokenum, bufsize, dist, time)
int *strokenum, *bufsize, *time;
float *dist;
	{
	return(set_stroke(*strokenum, *bufsize, *dist, *time));
	}

int setvaluator_(valnum, init, low, high)
int *valnum;
float *init, *low, *high;
	{
	return(set_valuator(*valnum, *init, *low, *high));
	}

int terminatedevice_(devclass, devnum)
int *devclass, *devnum;
	{
	return(terminate_device(*devclass, *devnum));
	}
