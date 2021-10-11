/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)erase.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "hp2648.h"

erase()
{
	buffready(8);
	putchar(ESC);
	putchar(GRAPHIC);
	putchar(DISPLAY);
	putchar('a');
	putchar(ESC);
	putchar(GRAPHIC);
	putchar(PLOT);
	putchar(BINARY);
}
