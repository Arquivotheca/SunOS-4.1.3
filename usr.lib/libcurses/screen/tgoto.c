/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)tgoto.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.5 */
#endif

/*
 * tgoto: function included only for upward compatibility with old termcap
 * library.  Assumes exactly two parameters in the wrong order.
 */
extern	char	*tparm();

char	*
tgoto(cap, col, row)
char	*cap;
int	col, row;
{
    char	*cp;

    cp = tparm(cap, row, col);
    return (cp);
}
