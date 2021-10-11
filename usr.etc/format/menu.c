
#ifndef lint
static	char sccsid[] = "@(#)menu.c 1.1 92/07/30";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains routines relating to running the menus.
 */
#include "global.h"
#include "menu.h"
#include "misc.h"

/*
 * This routine takes a menu struct and concatenates the
 * command names into an array of strings describing the menu.
 * All menus have a 'quit' command at the bottom to exit the menu.
 */
char **
create_menu_list(menu)
	struct	menu_item *menu;
{
	register struct menu_item *mptr;
	register char **cpptr, *list;
	int	i;

	/*
	 * Count the number of commands in the menu and allocate space
	 * for the array of pointers.
	 */
	for (mptr = menu; mptr->menu_cmd != NULL; mptr++)
		;
	i = (mptr - menu + 2) * sizeof (char *);
	list = zalloc(i);
	cpptr = (char **)list;
	/*
	 * Fill in the array with the names of the menu commands.
	 */
	for (mptr = menu; mptr->menu_cmd != NULL; mptr++)
		*cpptr++ = mptr->menu_cmd;
	/*
	 * Add the 'quit' command to the end.
	 */
	*cpptr = "quit";
	return ((char **)list);
}

/*
 * This routine takes a menu list created by the above routine and
 * prints it nicely on the screen.
 */
display_menu_list(list)
	char	**list;
{
	register char **str;

	for (str = list; *str != NULL; str++)
		printf("        %s\n", *str);
}

/*
 * This routine 'runs' a menu.  It repeatedly requests a command and
 * executes the command chosen.  It exits when the 'quit' command is
 * executed.
 */
run_menu(menu, title, prompt, display_flag)
	struct	menu_item *menu;
	char	*title;
	char	*prompt;
	int	display_flag;
{
	char	**list;
	int	i;
	struct	env env;

#ifdef	lint
	display_flag = display_flag;
#endif	lint

	/*
	 * Create the menu list and display it.
	 */
	list = create_menu_list(menu);
	printf("\n\n%s MENU:\n", title);
	display_menu_list(list);
	/*
	 * Save the environment so a ctrl-C out of a command lands here.
	 */
	saveenv(env);
	for (;;) {
		/*
		 * Ask the user which command they want to run.
		 */
		i = input(FIO_MSTR, prompt, '>', (char *)list,
		    (int *)NULL, CMD_INPUT);
		/*
		 * If they choose 'quit', the party's over.
		 */
		if (menu[i].menu_cmd == NULL)
			break;
		/*
		 * Mark the saved environment active so the user can now
		 * do a ctrl-C to get out of the command.
		 */
		useenv();
		/*
		 * Run the command.  If it returns an error and we are
		 * running out of a command file, the party's really over.
		 */
		if ((*menu[i].menu_func)() && option_f)
			fullabort();
		/*
		 * Mark the saved environment inactive so ctrl-C doesn't
		 * work at the menu itself.
		 */
		unuseenv();
		if (cur_menu != last_menu) {
			last_menu = cur_menu;
			printf("\n\n%s MENU:\n", title);
			display_menu_list(list);
		}
	}
	/*
	 * Clean up the environment stack and throw away the menu list.
	 */
	clearenv();
	destroy_data((char *)list);
}
