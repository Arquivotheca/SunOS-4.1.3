/*	Copyright (c) 1984 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef lint
static	char sccsid[] = "@(#)delkeymap.c 1.1 92/07/30 SMI"; /* from S5R3.1 1.4 */
#endif

#include	"curses_inc.h"

/*
 *	Delete a key table
 */

void
delkeymap(terminal)
TERMINAL	*terminal;
{
    register	_KEY_MAP	**kpp, *kp;
    int				numkeys = terminal->_ksz;

    /* free key slots */
    for (kpp = terminal->_keys; numkeys-- > 0; kpp++)
    {
	kp = *kpp;
	if (kp->_sends == ((char *) (kp + sizeof(_KEY_MAP))))
	    free(kp);
    }

    if (terminal->_keys != NULL)
    {
	free(terminal->_keys);
	if (terminal->internal_keys != NULL)
	    free(terminal->internal_keys);
    }
    _blast_keys(terminal);
}
