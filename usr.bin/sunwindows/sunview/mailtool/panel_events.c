#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)panel_events.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - generic processing of panel events
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
#include <suntool/alert.h>

#include "glob.h"
#include "tool.h"

/* performance: global cache of getdtablesize() */
extern int dtablesize_cache;
#define GETDTABLESIZE() \
        (dtablesize_cache?dtablesize_cache:(dtablesize_cache=getdtablesize()))

char    *strcpy(), *strcat();

struct Menu_client_data {
	char           *fdir_name;	/* name of dir */
	time_t          fdir_time;	/* time fdir last modified */
	char           *path_string;	/* the concatenation of the items
					 * leading to this menu */
	char           *strtab;	/* string table for pullright menu */
};

static	char	*mt_get_command_string(), *mt_find_command_string();
static int	mt_mail_get_next_line();
static caddr_t	pullright_action_proc();

void
mt_panel_event(item, ie)
	Panel_item      item;
	Event          *ie;
{
	Panel           panel;
	Menu            menu;
	Menu_item       mi;
	char           *command_name;
	struct panel_item_data *p;

	panel = panel_get(item, PANEL_PARENT_PANEL);
	p = (struct panel_item_data *)panel_get(item, PANEL_CLIENT_DATA);
	menu = p->menu;
	command_name = NULL;

	if (event_action(ie) == MS_RIGHT && event_is_down(ie)) {
		mi = (Menu_item)menu_show(menu, panel,
			panel_window_event(panel, ie), 0);
		if (mi != NULL) {
			struct Menu_value *ptr;

			/* mouse has come up at this point */
			command_name = menu_get(mi, MENU_STRING);
			mt_trace(command_name, "");
			ptr = (struct Menu_value *)menu_get(mi, MENU_VALUE);
			event_set_shiftmask(ie, ptr->mask);
			p->menu_item = mi;
			mt_call_cmd_proc(item, ie, ptr->notify_proc);
			/*
			 * If there are more than four items on the menu,
			 * i.e., more operations than can be expressed by
			 * clicking on the button and holding a shift or CTRL
			 * key down, then another notify proc entirely might
			 * be called. For example, the delete menu contains
			 * four different ways of deleting, plus undelete,
			 * which is implemented by an entirely separate
			 * procedure. 
			 */
			mt_log_event(ie);
			return;
		}
	}  
	p->menu_item = NULL;
	mt_default_panel_event(item, ie, (char *) NULL, (char *) NULL, p);
	/*
	 * separate procedure so can be used by mt_file_event and
	 * mt_folder_event 
	 */
}

/*
 * use this procedure when you have an item in hand and want to achieve the
 * effect of having the operation specified by notify_proc and mask to occur.
 * separate procedure so can be used from mt_headersw_event_proc 
 */
void
mt_call_cmd_proc(item, event, notify_proc)
	Panel_item      item;
	Event          *event;
	Void_proc       notify_proc;
{
	Void_proc       old_notify_proc;

	if (notify_proc) {
		old_notify_proc = (void (*)())panel_get(
			item, PANEL_NOTIFY_PROC);
		if (notify_proc != old_notify_proc)
			panel_set(item, PANEL_NOTIFY_PROC, notify_proc, 0);
	}
	panel_begin_preview(item, event); /* panel_accept_preview
					 * assumes that the item
					 * has been inverted */
	panel_accept_preview(item, event);
	if (notify_proc)
		panel_set(item, PANEL_NOTIFY_PROC, old_notify_proc, 0);
}

void
mt_default_panel_event(item, ie, command_name, arg, p)
	Panel_item      item;
	Event          *ie;
	char           *command_name, *arg;
	struct panel_item_data *p;
{
	if (event_is_button(ie)) {
		if (event_is_up(ie) && mt_debugging) {
			if (command_name == NULL)
				command_name = mt_get_command_string(p, ie);
			mt_trace(command_name, arg);
		}
	}
	(void) panel_default_handle_event(item, ie);
	mt_log_event(ie);
}

void
mt_log_event(ie)
	Event          *ie;
{
	if (last_event_time == 0)
		/* tool is in the "about to get your mail" state */
		mt_update_info("Auto-retrieve cancelled.");
	if (event_time(ie).tv_sec != 0)
		last_event_time = event_time(ie).tv_sec;
}

/*
 * Uses the panel item's menu to find the name of the command that
 * corresponds to the state of the SHIFT and CTRL keys in ie 
 */
static char *
mt_get_command_string(p, ie)
	struct panel_item_data *p;
	Event          *ie;

{
	int             mask;
	char           *name;

	mask = 0;
	if (event_shift_is_down(ie))
		mask = (mask | SHIFTMASK);
	if (event_ctrl_is_down(ie))
		mask = (mask | CTRLMASK);
	if (name = mt_find_command_string(p->menu, mask))
		return(name);
	return (""); 
}

/* recursively searches menus looking for an item corresponding to mask */
static char *
mt_find_command_string(menu, mask)
	Menu            menu;
	int             mask;
{
	Menu            m;
	register int    i, n;

	Menu(*genproc) ();
	char           *name;

	if (genproc = (Menu (*)())menu_get(menu, MENU_GEN_PROC)) 
		menu = genproc(menu, MENU_DISPLAY);
	n = (int)menu_get(menu, MENU_NITEMS);
	for (i = 1; i <= n; i++) {
		Menu_item mi;
		struct Menu_value *ptr;

		mi = menu_get(menu, MENU_NTH_ITEM, i);
		if ((m = menu_get(mi, MENU_PULLRIGHT))
			&& (name = mt_find_command_string(m, mask)))
			return(name); 
		ptr = (struct Menu_value *)menu_get(mi, MENU_VALUE);
		if (ptr->mask == mask)
			return (menu_get(mi, MENU_STRING));
	}
	return (NULL);
}

void
mt_trace(command_name, string)
	char           *command_name, *string;
{
	if (mt_debugging) {
		(void) printf("command: %s%s, message #: %d, #fds left: %d\n",
			command_name, (string ? string : ""),
			mt_get_curselmsg(1), nfds_avail()
			);
		fflush(stdout);
	}
}



int
nfds_avail()
{
	struct stat     statb;
	register int    i, n = 0;
	extern int      errno;

	for (i = GETDTABLESIZE() - 1; i >= 0; i--)
		if (fstat(i, &statb) < 0 && errno == EBADF)
			n++;
	return (n);
}

