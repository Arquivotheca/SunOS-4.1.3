/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cont.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "dumb.h"

cont(x, y)
	int x,y;
{
	int x1, y1;
	x1 = x;
	y1 = y;
	scale(x1, y1);
	dda_line('*', currentx, currenty, x, y);
}
