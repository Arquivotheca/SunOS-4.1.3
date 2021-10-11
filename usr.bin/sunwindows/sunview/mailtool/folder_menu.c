#ifndef lint
#ifdef sccs
static char sccsid[] = "@(#)folder_menu.c %I 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - building folder menu and processing its events
 */

#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sundev/kbd.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/walkmenu.h>
#include <suntool/scrollbar.h>

#include "glob.h"
#include "tool.h"

char    *strcpy();

Menu	mt_folder_menu;		/* menu of folders in user's folder directory */

struct Menu_client_data {
	char           *fdir_name;	/* name of dir */
	time_t          fdir_time;	/* time fdir last modified */
	char           *path_string;	/* the concatenation of the items
					 * leading to this menu */
	char           *strtab;	/* string table for pullright menu */
};

static	Menu 	mt_folder_menu_build();
static void	mt_destroy_folder_menu();
static	char	*mt_get_fdir_name();
static caddr_t  pullright_action_proc(), pull_right_action_proc0();

/* initialize folder menu. Called when tool is initializing. */
mt_init_folder_menu()
{
	mt_folder_menu = mt_get_folder_menu(mt_folder_menu, "", "");
}

/*
 * Handle input events when over the "folder" button.
 * Menu request gives menu of folder names.
 */
void
mt_folder_event(item, ie)
	Panel_item      item;
	Event          *ie;
{
/* not used - hala  Menu_item       mi;*/

	if (event_action(ie) == MS_RIGHT && event_is_down(ie)) {
		mt_folder_menu = mt_get_folder_menu(mt_folder_menu, "", "");
		(void) menu_show(mt_folder_menu, mt_cmdpanel,
			panel_window_event(mt_cmdpanel, ie), 0);
	} else 
		mt_default_panel_event(item, ie, "change folders",
			(char *)panel_get_value(mt_file_item), panel_get(item, 
			PANEL_CLIENT_DATA));
		/* will call mt_folder_proc */
}

/*
 * Returns a folder menu, building it if necessary
 */
Menu
mt_get_folder_menu(m, path_string, item_string)
	Menu            m;
	char           *path_string, *item_string;
{
	struct Menu_client_data *menu_data;
	struct stat     statb;

	if (m)	{
		menu_data  = (struct Menu_client_data *)LINT_CAST(
			menu_get(m, MENU_CLIENT_DATA));
		if (stat(menu_data->fdir_name, &statb) >= 0 &&
				statb.st_mtime > menu_data->fdir_time) 
			menu_data->fdir_time = statb.st_mtime;
							/* directory has been
							 * touched more recently
							 * than when menu was
							 * made. Remake menu */
		else
			return (m);
		mt_destroy_folder_menu(m);	/* drop through and create */
	} else {				/* no menu. create */
		char	*s;
		int	len;

		menu_data = (struct Menu_client_data *)LINT_CAST(
			malloc(sizeof(struct Menu_client_data)));
		menu_data->strtab = (char *)LINT_CAST(
			malloc(15*MAXFOLDERS));
		s = (char *)LINT_CAST(
			malloc(strlen(path_string)+strlen(item_string)+1));
		(void)strcpy(s, path_string);
		(void)strcat(s, item_string);
		len = strlen(s);
		if (s[len - 1] == '@') /* symbolic link */
			s[len - 1] = '\0';
		menu_data->path_string = s;
		s = mt_get_fdir_name(s);	/* new storage */
		menu_data->fdir_name = s;
		(void)stat(s, &statb);
		menu_data->fdir_time = statb.st_mtime;
	}
	return (mt_folder_menu_build(menu_data));
}

static void
mt_destroy_folder_menu(m)
	Menu            m;
{
	/*
	 * Menus are only destroyed when they are about to be rebuilt. The
	 * storage inside of MENU_CLIENT_DATA, e.g. strtab, fdir_name, etc.
	 * is not freed because it will be reused in mt_build_folder_menu 
	 */
	register int    i;
	Menu            pull_right;

	for (i = (int)menu_get(m, MENU_NITEMS); i > 0; i--) {
		pull_right = (Menu)LINT_CAST(menu_get(menu_get(m, MENU_NTH_ITEM, i),
				MENU_CLIENT_DATA));
		if (pull_right)
			mt_destroy_folder_menu(pull_right);
	}
	menu_destroy(m);
}

static Menu
pullright_gen_proc(mi, operation)
	Menu_item       mi;
	Menu_generate   operation;
{
	Menu            pull_right, parent;
	struct Menu_client_data *menu_data;

	parent = (Menu) LINT_CAST(menu_get(mi, MENU_PARENT));
	pull_right = (Menu) LINT_CAST(menu_get(mi, MENU_CLIENT_DATA));
	switch (operation) {
	case MENU_DISPLAY:
		menu_data = (struct Menu_client_data *)menu_get(
			parent, MENU_CLIENT_DATA);
		/* menu_data for parent menu */
		pull_right = mt_get_folder_menu(
				       pull_right,
				       menu_data->path_string,
				       menu_get(mi, MENU_STRING)
			);	/* mt_get_folder_menu will build menu first
				 * time, and then return it subsequent times
				 * unless directory has been more recently
				 * modified */
		(void)menu_set(mi, MENU_CLIENT_DATA, pull_right, 0);
	case MENU_DISPLAY_DONE:
	case MENU_NOTIFY:
	case MENU_NOTIFY_DONE:
		break;
	}
	return (pull_right);
}

/* ARGSUSED */
static caddr_t
pullright_action_proc0(m, mi)
	Menu            m;
	Menu_item       mi;
{
	return (mi); 
}

/* ARGSUSED */
static caddr_t
pullright_action_proc(m, mi)
	Menu            m;
	Menu_item       mi;
{

	char	s[128];
	struct Menu_client_data	*menu_data;
	int		len;

	*s = '\0';
	menu_data = (struct Menu_client_data *)menu_get(
		m, MENU_CLIENT_DATA);
	(void)strcpy(s, menu_data->path_string);
	(void)strcat(s, menu_get(mi, MENU_STRING));
	/*
	 * value is result of concatting the string(s) from
	 * menus to the left, stored in path_string field of
	 * MENU_CLIENT_DATA, with that of this item's string. 
	 */
	len = strlen(s);
	if (s[len - 1] == '@') /* symbolic link */
		s[len - 1] = '\0';
	(void) panel_set_value(mt_file_item, s);
	mt_trace("folder name = ", s);
	if (mt_idle) {
		/*
		 * selection in folder pullright came from frame menu open
		 * and switch to specified folder 
		 */
		mt_load_from_folder = s;
		window_set(mt_frame, FRAME_CLOSED, FALSE, 0);
	}
}

static Menu
mt_folder_menu_build(menu_data)
	struct Menu_client_data *menu_data;
{
	register int    i;
	int             ac, vertical_line = FALSE;
	char          **av;
	char          **mt_get_folder_list();
	Menu            m;
	Menu_item	mi;

 	mt_waitcursor(); 
 	if (*menu_data->path_string == '\0')
		/*
		 * if path_string != "", this is a pullright, which means a
		 * menu is already up, so can't change namestripe because we
		 * are in fullscreen access
		 */
 		mt_set_namestripe_temp("building folder menu...");
	av = mt_get_folder_list(menu_data->path_string, menu_data->fdir_name,
		menu_data->strtab, &ac);
	/*
	 * Select aspect ratio for menu.
	 * This is essentially a table lookup to
	 * avoid taking a sqrt.
	 */
	if  (ac == 0) {
		av[0] = "Empty Directory!!";
		ac = 1;
	}

	if (ac <= 3)
		i = 1;
	else if (ac <= 12)
		i = 2;
	else if (ac <= 27)
		i = 3;
	else if (ac <= 48)
		i = 4;
	else if (ac <= 75)
		i = 5;
	else if (ac <= 108)
		i = 6;
	else if (ac <= 147)
		i = 7;
	else
		i = 8;
	av[-1] = (char *)MENU_STRINGS;
	av[ac] = av[ac + 1] = 0;/* first NULL to terminate list of
				 * MENUSTRINGS, second NULL to terminate
				 * ATTR_LIST attribute list in call to
				 * menu_create below */

	m = menu_create(ATTR_LIST, &av[-1],
				  MENU_LEFT_MARGIN, 6,
				  MENU_NCOLS, i,
				  MENU_CLIENT_DATA, menu_data,
				  0);
	if (!mt_3x_compatibility) {
		for (i = 0; i < ac; i++) {
			char	*s;
			int	len;

			s = av[i];
			mi = menu_get(m, MENU_NTH_ITEM, i+1);
			len = strlen(s);
			if (s[len - 1] == '/' || 
				(s[len - 1] == '@' && s[len - 2] == '/')) {
				menu_set(mi,
					MENU_GEN_PULLRIGHT, pullright_gen_proc,
					/*
					 * MENU_ACTION_PROC,
					 * pullright_action_proc0, 
					 */
					0);
				vertical_line = TRUE;
			}
			menu_set(mi,
				MENU_ACTION_PROC, pullright_action_proc,
				0);
		}
		if (vertical_line || *(menu_data->path_string) != '\0')
			/*
			 * use vertical line if there are any pullrights, or
			 * if this is itself a descendant of a pullright 
			 */
			menu_set(m,
				MENU_LINE_AFTER_ITEM, MENU_VERTICAL_LINE,
				0);
	}

	mt_restorecursor();
 	if (*menu_data->path_string == '\0')
		mt_restore_namestripe();
	return (m);
}

/*
 * generates full path name for folder (sub)directory. returns new storage.
 * path_string is the result of concatenating the items in the pullright
 * folder menu. It is always relative to the folders directory, if any. 
 */
static char *
mt_get_fdir_name(path_string)
	char           *path_string;
{
	char            line[256];

	mt_full_path_name(mt_value("folder"), line);
	if (*line == '\0')
		return ("");
	else if (line[strlen(line) - 1] != '/')
			(void) strcat(line, "/");
	if (*path_string == '+')
		path_string++;
	(void) strcat(line, path_string);
	return (mt_savestr(line));
}

