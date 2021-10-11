/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)label.c 1.1 92/07/30 SMI"; /* from UCB 5.1 5/7/85 */
#endif not lint

#include "dumb.h"

label(string)
	char *string;
{
	while(*string != '\0' && currentx < COLS){
		screenmat[currentx][currenty] = *string++;
		currentx++;
	}
	if(currentx == COLS)
		currentx = currentx-1;

}
