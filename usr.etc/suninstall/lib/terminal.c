#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)terminal.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)terminal.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint
/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		terminal.c
 *
 *	Description:	This file contains the routined sys-config needed to 
 *		get the terminal type if it is not known.
 */

#include <stdio.h>
#include <string.h>
#include <curses.h>
#include "host_impl.h"

static	void		print_list();
static	void		print_other();

extern	char		*getenv();


/*
 *	Name:		terminal()
 *
 *	Description:	Get the terminal type if it is not known.
 */

void
terminal(termtype)
	char *		termtype;
{
	char		buf[BUFSIZ];		/* input buffer */
	int		done = 0;		/* got terminal yet? */
	char		*cp;

	if ((cp = getenv("TERM")) != CP_NULL) {
		(void) strcpy(termtype, cp);
		set_menu_term(termtype);
		return;
	}

	if (strlen(termtype)) {
		set_menu_term(termtype);
		return;
	}

	do {
		(void) fflush(stdout); 
		print_list();
		get_stdin(buf);
		switch (buf[0]) {
		case '1':
			(void) strcpy(termtype,"sun");
			done = 1;
			break;

		case '2':
			(void) strcpy(termtype,"925");
			done = 1;
			break;

		case '3':
			(void) strcpy(termtype,"wyse-50");
			done = 1;
			break;

		case '4':
			done = 0;
			do {
				print_other();
				get_stdin(buf);
				(void) strcpy(termtype, buf);
				switch (tgetent(buf, termtype)) {
				case -1:
					log("%s: cannot open /etc/termcap.\n",
					    progname);
					exit(1);

				case 0:
					(void) fprintf(stderr,
				 	   "'%s': not found in /etc/termcap\n",
						       termtype);
					break;
				case 1:
					done = 1;
					break;
				}
			} while (!done);
			break;
		}
	} while (!done);

	set_menu_term(termtype);
} /* end get_terminal() */




static	char *		list_mesg[] = {
	"\n\n",
	" SysConfA0			SYSTEM CONFIGURATION\n",
	"				   TERMINAL TYPE\n",
	"________________________________________________________________________________\n",
	"Select your terminal type:\n\n\n",
	"        1) Sun Workstation\n",
	"        2) Televideo 925\n",
	"        3) Wyse Model 50\n",
	"        4) Others\n",
	"\n",
	"Selection => ",
	CP_NULL
};


/*
 *	Name:		print_list()
 *
 *	Description:	Print the list of supported terminal types.
 */

static	void
print_list()
{
	int		i;			/* index variable */


	for (i = 0; list_mesg[i]; i++)
		(void) printf("%s", list_mesg[i]);

	(void) fflush(stdout); 
} /* end print_list() */




static	char *		other_mesg[] = {
	"\n",
	"Enter the terminal type ( the type must be in /etc/termcap ):\n",
	">> ",

	CP_NULL
};


/*
 *	Name:		print_other()
 *
 *	Description:	Print a usage message if the terminal type
 *		selected is not in /etc/termcap.
 */

static	void
print_other()
{
	int		i;			/* index variable */


	for (i = 0; other_mesg[i]; i++)
		(void) printf("%s", other_mesg[i]);

	(void) fflush(stdout);
} /* end print_list() */
