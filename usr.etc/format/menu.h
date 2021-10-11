
/*      @(#)menu.h 1.1 92/07/30 SMI   */

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * This file contains declarations pertaining to the menus.
 */
/*
 * This structure defines a menu entry.  It consists only of a command
 * name and the function to run the command.  The descriptive text
 * that appears after the command name in the menus is actually part of
 * the command name to the program.  Since abbreviation is allowed for
 * commands, the user never notices the extra characters.
 */
struct menu_item {
	char	*menu_cmd;
	int	(*menu_func)();
};

