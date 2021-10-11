#ifndef lint
static	char	sccsid[] = "@(#)menu_log.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_log.c
 *
 *	Description:	Routines for managing the menu log file.
 */

#include <sys/param.h>
#include <curses.h>
#include <varargs.h>
#include "menu.h"




/*
 *	Local variables:
 */
static	char		LOGFILE[MAXPATHLEN];	/* path to the log file */




/*
 *	External references:
 */
extern	char *		strcpy();




/*
 *	Name:		menu_log()
 *
 *	Description:	Add a message to the menu log, and print the message
 *		on the menu via menu_mesg().
 *
 *	Call syntax:	menu_log(fmt, va_alist);
 *
 *	Parameters:	char *		fmt;
 *			va_dcl
 */

/*VARARGS1*/
void
menu_log(fmt, va_alist)
        char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */
	FILE *		fp;			/* ptr to LOGFILE */


	va_start(ap);				/* init varargs stuff */

	if (LOGFILE[0]) {
		fp = fopen(LOGFILE, "a");
		if (fp == NULL)
			menu_mesg("%s: cannot open file for append.", LOGFILE);
		else {
			(void) vfprintf(fp, fmt, ap);
			(void) fputc('\n', fp);
			(void) fclose(fp);
		}
	}

	_menu_mesg(fmt, ap);
} /* end menu_log() */




/*
 *	Name:		set_menu_log()
 *
 *	Description:	Set the name of the menu log file.
 */

void
set_menu_log(name)
	char *		name;
{
	(void) strcpy(LOGFILE, name);
} /* end set_menu_log() */
