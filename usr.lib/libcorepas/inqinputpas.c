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
static char sccsid[] = "@(#)inqinputpas.c 1.1 92/07/30 Copyr 1983 Sun Micro";
#endif

/*
 Copyright (c) 1983 by Sun Microsystems, Inc.
*/

#include "f77strings.h"

int inquire_echo();
int inquire_echo_position();
int inquire_echo_surface();
int inquire_keyboard();
int inquire_locator_2();
int inquire_stroke();
int inquire_valuator();

int inqecho(devclass, devnum, echotype)
int devclass, devnum, *echotype;
	{
	return(inquire_echo(devclass, devnum, echotype));
	}

int inqechoposition(devclass, devnum, x, y)
int devclass, devnum;
double *x, *y;
	{
	float tx,ty;
	int f;
	f=inquire_echo_position(devclass, devnum, &tx, &ty);
	*x=tx;
	*y=ty;
	return(f);
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

int inqechosurface(devclass, devnum, surfname)
int devclass, devnum;
vwsurf *surfname;
	{
	return(inquire_echo_surface(devclass, devnum, *surfname));
	}

int inqkeyboard(keynum, bufsize, istr, length)
int keynum, *bufsize, *length;
char *istr;
	{
	char c, *sptr;
	int f, i;

	f = inquire_keyboard(keynum, bufsize, istr, length);
	istr += *length;
	for (i = (*length+1); i < 257; i++)
		*++istr = ' ';
	return(f);
	}

int inqlocator2(locnum, x, y)
int locnum;
double *x, *y;
	{
	return(inquire_locator_2(locnum, x, y));
	}

int inqstroke(strokenum, bufsize, dist, time)
int strokenum, *bufsize, *time;
double *dist;
	{
	return(inquire_stroke(strokenum, bufsize, dist, time));
	}

int inqvaluator(valnum, init, low, high)
int valnum;
double *init, *low, *high;
	{
	return(inquire_valuator(valnum, init, low, high));
	}
