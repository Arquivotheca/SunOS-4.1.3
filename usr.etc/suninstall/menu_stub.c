#ifndef lint
#ifdef SunB1
static	char		mls_sccsid[] = "@(#)menu_stub.c 1.1 92/07/30 SMI; SunOS MLS";
#else
static	char		sccsid[] = "@(#)menu_stub.c 1.1 92/07/30 SMI";
#endif /* SunB1 */
#endif lint

/*
 *	Copyright (c) 1989 Sun Microsystems, Inc.
 */

/*
 *	Name:		menu_stub.c
 *
 *	Description:	These routines are defined here to keep from loading
 *		the menu, curses and termlib libraries.
 */

#include <stdio.h>
#include <varargs.h>
#include "install.h"




/*
 *	Name:		menu_abort()
 */
void
menu_abort(exit_code)
	int		exit_code;
{
	exit(exit_code);
} /* end menu_abort() */




/*
 *	Name:		menu_ack()
 */
void
menu_ack()
{
	char		ch;			/* scratch char */


	(void) printf("Press <return> to continue ");
	(void) fflush(stdout);

	do
		ch = getchar();
	while (ch != '\n' && ch != '\r');
} /* end menu_ack() */




/*
 *	Name:		menu_flash_off()
 */
void
menu_flash_off(display)
{
	int		i;

#ifdef lint
	display = display;
#endif lint

	(void) printf("done\r");
	(void) fflush(stdout);
	for (i = 0; i < 79; i++)
		(void) putchar(' ');
	(void) putchar('\r');
	(void) fflush(stdout);
} /* end menu_flash_off() */




/*
 *	Name:		menu_flash_on()
 */
/*VARARGS1*/
void
menu_flash_on(fmt, va_alist)
	char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */


	va_start(ap);				/* init varargs stuff */

	(void) vfprintf(stdout, fmt, ap);
	(void) fflush(stdout);
} /* end menu_flash_on() */




/*
 *	Name:		menu_log()
 */
/*VARARGS1*/
void
menu_log(fmt, va_alist)
	char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */


	va_start(ap);				/* init varargs stuff */

	_log(fmt, ap);
	log("\n");
} /* end menu_log() */




/*
 *	Name:		menu_mesg()
 */
/*VARARGS1*/
void
menu_mesg(fmt, va_alist)
	char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */


	va_start(ap);				/* init varargs stuff */

	(void) vfprintf(stderr, fmt, ap);
	(void) fprintf(stderr, "\n");
} /* end menu_mesg() */




/*
 *	Name:		init_menu()
 *
 *	Description: 	This serves only a a place holder to resolve
 *			external declarations for non-menuing functions.
 */
void
init_menu()
{
	/*
	 *	place holder
	 */
}

/*
 *	Name:		end_menu()
 *
 *	Description: 	This serves only a a place holder to resolve
 *			external declarations for non-menuing functions.
 */
void
end_menu()
{
	/*
	 *	place holder
	 */
}


