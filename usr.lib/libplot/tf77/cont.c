/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)cont.c 1.1 92/07/30 SMI"; /* from UCB 5.1 6/7/85 */
#endif not lint

cont_(x,y)
int *x, *y;
{
	cont(*x, *y);
}
