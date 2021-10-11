#ifndef lint
static	char	sccsid[] = "@(#)globals.c 1.1 92/07/30";
#endif

/*
 *	Name:		globals.c
 *
 *	Description:	Global variables for the menu system.
 */

#include <signal.h>				/* system software signals */
#include "menu.h"


pointer		_current_fm;			/* ptr to current form/menu */
pointer		_current_obj;			/* pointer to current object */
int		_menu_forward = 0;		/* forward char pressed? */
int		_menu_backward = 0;		/* backward char pressed? */
int		_menu_init_done = 0;		/* was menu init done? */
#ifdef CATCH_SIGNALS
void (*		_menu_signals[NSIG])();		/* pointers to signals */
#endif
