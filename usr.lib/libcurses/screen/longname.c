/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)longname.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4.1.4 */
#endif

/* This routine returns the long name of the terminal. */

char *
longname()
{
    extern	char	ttytype[], *strrchr();
    register	char	*cp = strrchr(ttytype, '|');

    if (cp)
	return (++cp);
    else
	return (ttytype);
}
