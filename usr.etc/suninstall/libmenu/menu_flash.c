#ifndef lint
static	char	sccsid[] = "@(#)menu_flash.c 1.1 92/07/30";
#endif

/*
 *	Name:		menu_flash.c
 *
 *	Description:	This file contains the routines for flashing messages
 *		on and off the menu message line.
 */

#include <curses.h>
#include <varargs.h>
#include "menu.h"
#include "menu_impl.h"


static	int		too_long;		/* is string too long */
static	int		x_coord, y_coord;	/* saved coordinates */
static	int		flash_flag = 0;		/* 1 to disable flash */

/*
 *	Name:		flash_off()
 *
 *	Description:	disable flashed message from the menu message line.
 *
 *	Call syntax:	flash_off();
 */

void
flash_off()
{
	flash_flag = 1;
}

/*
 *	Name:		flash_on()
 *
 *	Description:	enable flashed message from the menu message line.
 *
 *	Call syntax:	flash_on();
 */

void
flash_on()
{
	flash_flag = 0;
}


/*
 *	Name:		menu_flash_off()
 *
 *      Description:    Remove a flashed message from the menu message line.
 *                      If redisplay is false then it is the users
 *                      responsibility to call redisplay where apropriate.
 *
 *      Call syntax:    menu_flash_off(display);
 *
 *      Parameters:     int             display;
 *
 */

void
menu_flash_off(display)
int	display;
{
	if (flash_flag)
		return;
	if (!_menu_init_done) {
		(void) fprintf(stderr, "done\n");
		return;
	}

	move(LINES - 2, 0);			/* clear the message */
	clrtoeol();

	/*
	 *	Redisplay the form/menu.
	 */
	if (display)
		redisplay(too_long ? MENU_CLEAR : MENU_NO_CLEAR);

	move(y_coord, x_coord);			/* put cursor back */

	refresh();
} /* end menu_flash_off() */




/*
 *	Name:		menu_flash_on()
 *
 *	Description:	Flash a message on the menu message line.
 *
 *	Call syntax:	menu_flash_on(fmt, va_alist);
 *
 *	Parameters:	char *		fmt;
 *			va_dcl
 */

/*VARARGS1*/
void
menu_flash_on(fmt, va_alist)
        char *		fmt;
	va_dcl
{
	va_list		ap;			/* ptr to args */
	

	if (flash_flag)
		return;
	va_start(ap);				/* init varargs stuff */

	if (!_menu_init_done) {
		(void) vfprintf(stderr, fmt, ap);
		(void) fprintf(stderr, " ... ");
		return;
	}

	getyx(stdscr, y_coord, x_coord);	/* save where we were */
#ifdef lint
	y_coord = y_coord;
	x_coord = x_coord;
#endif

	move(LINES - 2, 0);
	clrtoeol();
	standout();
	too_long = menu_print(fmt, ap);
	too_long += menu_print(" ... ", (va_list) NULL);
	standend();

	/*
	 *	Cannot do a redisplay() here or the screen will get messed up.
	 */
	refresh();
} /* end menu_flash_on() */
