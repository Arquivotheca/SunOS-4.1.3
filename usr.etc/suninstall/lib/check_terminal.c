#ifndef lint
#ifdef SunB1
#ident			"@(#)check_terminal.c 1.1 92/07/30 SMI; SunOS MLS";
#else
#ident			"@(#)check_terminal.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */


#include <stdio.h>
#include "install.h"


/****************************************************************************
**
** Name:	(int) check_client_terminal()
**
** Return Value : 1 : if the terminal type is in the /etc/termcap file
**		  0 : if the terminal type is not in the /etc/termcap file
**		       or if /etc/tercap is not found
**
**	Description:	Check if the terminal is  in /etc/termcap
**
****************************************************************************
*/
int
check_client_terminal(termtype)
	char	*termtype;
{
	char		buf[BUFSIZ];		/* input buffer */

	switch(tgetent(buf, termtype)) {
	case -1 :
		menu_mesg("/etc/termcap not found");
		return(0);
		break;
	case 0 :
		menu_mesg("terminal type : %s : not in /etc/termcap",
			  termtype);
		return(0);
		break;
	case 1 : /* AOK */
		return(1);
	}

	/* NOTREACHED */
} /* end check_client_terminal() */
