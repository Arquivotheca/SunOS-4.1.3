#ifndef lint
static	char sccsid[] = "@(#)sony_itoa.c 1.1 92/07/30 SMI";
#endif not lint

/*
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 */


#include <stdio.h>
#include <string.h>
#include <sys/types.h>

/*
 * The laser disk uses character strings, but it is nicer
 * to specify frame chapter numbers in ints. sony_itoa
 * converts int's into unsigned char arrays for the laser
 * disk player
 */
u_char *
sony_itoa(number)
int number;
{
	u_char *string;

	if((string = (u_char *)malloc(10)) == NULL) {
		perror("sony_itoa: could not allocate space\n");
		exit(1);
	}
	sprintf(string, "%d", number);
	strncat(string, "\0", 1);
	return(string);
}
