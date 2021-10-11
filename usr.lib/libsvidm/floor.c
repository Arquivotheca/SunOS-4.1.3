#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)floor.c 1.1 92/07/30 SMI"; /* from S5R3 1.8 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	floor(x) returns the largest integer (as a double-precision number)
 *	not greater than x.
 *	ceil(x) returns the smallest integer not less than x.
 */

extern double modf();

double
floor(x) 
double x;
{
	double y; /* can't be in register because of modf() below */

	return (modf(x, &y) < 0 ? y - 1 : y);
}

double
ceil(x)
double x;
{
	double y; /* can't be in register because of modf() below */

	return (modf(x, &y) > 0 ? y + 1 : y);
}
