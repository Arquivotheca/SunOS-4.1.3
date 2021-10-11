#if !defined(lint) && defined(SCCSIDS)
static	char sccsid[] = "@(#)matherr.c 1.1 92/07/30 SMI"; /* from S5R3 1.4 */
#endif

/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*LINTLIBRARY*/

#include <math.h>

/*ARGSUSED*/
int
matherr(x)
struct exception *x;
{
	return (0);
}
