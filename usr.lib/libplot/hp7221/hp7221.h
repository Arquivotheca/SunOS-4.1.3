/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */
/*	@(#)hp7221.h 1.1 92/07/30 SMI; from UCB 5.1 5/7/85	*/

/*
 * Displays plot files on an HP7221 plotter.
 * Cloned from bgplot.c and gigiplot.c by Jim Kleckner
 * Thu Jun 30 13:35:04 PDT 1983
 *  Requires a handshaking program such as hp7221cat to get
 *  the plotter open and ready.
 */

#include <stdio.h>
#include <math.h>

#define ESC	033
#define PI	3.141592659
#define ENDOFSTRING 03

/* The graphics address range is 0..XMAX, 0..YMAX. */
#define XMAX	1800
#define YMAX	1800

#define scaleX(xi)	((int) ((xi - lowx)*scale +0.5))
#define scaleY(yi)	((int) ((yi - lowy)*scale +0.5))

extern int currentx;
extern int currenty;
extern double lowx;
extern double lowy;
extern double scale;
