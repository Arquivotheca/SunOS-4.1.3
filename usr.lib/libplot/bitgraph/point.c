/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)point.c 1.1 92/07/30 SMI"; /* from UCB 5.2 4/30/85 */
#endif not lint


point(xi, yi)
int xi, yi;
{
	move(xi, yi);
	label(".");
}
