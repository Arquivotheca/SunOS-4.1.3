#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)fabs.c 1.1 92/07/30 SMI"; /* from S5R3 1.6 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/
/*
 *	fabs returns the absolute value of its double-precision argument.
 */

double
fabs(x)
register double x;
{
	return (x < 0 ? -x : x);
}
