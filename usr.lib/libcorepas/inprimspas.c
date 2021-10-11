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
static char sccsid[] = "@(#)inprimspas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "pasarray.h"
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

int awaitanybutton(tim, butnum)
int tim, *butnum;
	{
	return(await_any_button(tim, butnum));
	}

int awtbuttongetloc2(tim, locnum, butnum, x, y)
int tim, locnum, *butnum;
double *x, *y;
	{
	float tx,ty;
	int f;
	f=await_any_button_get_locator_2(tim, locnum, butnum, &tx, &ty);
	*x=tx;
	*y=ty;
	return(f);
	}

int awtbuttongetval(tim, valnum, butnum, val)
int tim, valnum, *butnum;
double *val;
	{
	float tval;
	int f;
	f=await_any_button_get_valuator(tim, valnum, butnum, &tval);
	*val=tval;
	return(f);
	}

int awaitkeyboard(tim, keynum, f77string, length)
int tim, keynum, length;
char *f77string;
	{
	char c, *sptr;
	int f, i;
	
	f = await_keyboard(tim, keynum, f77string, length);
	f77string += length;
	for (i = (length+1); i < 257; i++)
		*++f77string = ' ';
	return(f);
	}

int awaitpick(tim, picknum, segnam, pickid)
int tim, picknum, *segnam, *pickid;
	{
	return(await_pick(tim, picknum, segnam, pickid));
	}

int awaitstroke2(tim, strokenum, arraysize, xarray, yarray, numxy)
int tim, strokenum, arraysize, numxy;
double xarray[], yarray[];
	{
		int i;
		for (i = 0; i < numxy; ++i) {
			xcoort[i] = xarray[i];
			ycoort[i] = yarray[i];
		}
    return(await_stroke_2(tim, strokenum, arraysize, xcoort, ycoort, numxy));
    }

int getmousestate(devclass, devnum, x, y, buttons)
int devclass, devnum;
int *buttons;
double *x, *y;
	{
	float tx,ty;
	int f;
	f=get_mouse_state(devclass, devnum, &tx, &ty, buttons);
	*x=tx;
	*y=ty;
	return(f);
	}

int initializedevice(devclass, devnum)
int devclass, devnum;
	{
	return(initialize_device(devclass, devnum));
	}

int setecho(devclass, devnum, echotype)
int devclass, devnum, echotype;
	{
	return(set_echo(devclass, devnum, echotype));
	}

int setechogroup(devclass, devnumarray, n, echotype)
int devclass, devnumarray[], n, echotype;
	{
	return(set_echo_group(devclass, devnumarray, n, echotype));
	}

int setechoposition(devclass, devnum, x, y)
int devclass, devnum;
double x, y;
	{
	return(set_echo_position(devclass, devnum, x, y));
	}


#define DEVNAMESIZE 20

typedef struct  {
    		char screenname[DEVNAMESIZE];
    		char windowname[DEVNAMESIZE];
		int fd;
		int (*dd)();
		int instance;
		int cmapsize;
    		char cmapname[DEVNAMESIZE];
		int flags;
		char *ptr;
		} vwsurf;

int setechosurface(devclass, devnum, surfname)
int devclass, devnum;
vwsurf *surfname;
	{
	return(set_echo_surface(devclass, devnum, surfname));
	}

int setkeyboard(keynum, bufsize, istr, pos)
int keynum, bufsize, pos;
char *istr;
	{
	char *sptr;
	char pasarg[257];
	int i,strlen;

	strlen = 256;
	sptr = istr+256;
	while ((*--sptr) == ' ') {strlen--;};
	strncpy (pasarg,istr,strlen);
	pasarg[strlen] = '\0';
	i = set_keyboard(keynum, bufsize, pasarg, pos);
	return(i);
	}

int setpick(picknum, aperture)
int picknum;
double aperture;
	{
	return(set_pick(picknum, aperture));
	}

int setlocator2(locnum, x, y)
int locnum;
double x, y;
	{
	return(set_locator_2(locnum, x, y));
	}

int setstroke(strokenum, bufsize, dist, time)
int strokenum, bufsize, time;
double dist;
	{
	return(set_stroke(strokenum, bufsize, dist, time));
	}

int setvaluator(valnum, init, low, high)
int valnum;
double init, low, high;
	{
	return(set_valuator(valnum, init, low, high));
	}

int terminatedevice(devclass, devnum)
int devclass, devnum;
	{
	return(terminate_device(devclass, devnum));
	}
