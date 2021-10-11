#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)generate_menus.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - building menus for the cmdpanel
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

static Menu     mt_file_menu;	/* menu of most recent file names used.
				 * Behind File: panel item */

/* menu generate procs */

Menu
mt_cancel_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	Panel_item	item;
	static Menu cancel_menu, clear_menu;
	struct reply_panel_data *ptr;

			/*
			 * cancel gen proc actually won't work for other
			 * values of operation, because the m that is passed
			 * in is either clear_menu or cancel_menu, and these
			 * are generic and don't have a way of getting to the
			 * panel that they came from via MENU_CLIENT_DATA 
			 */
	item = (Panel_item) menu_get(m, MENU_CLIENT_DATA);
	ptr = (struct reply_panel_data *)panel_get(
		panel_get(item, PANEL_PARENT_PANEL),
		PANEL_CLIENT_DATA);
	if (ptr->behavior == mt_Stay_Up) {
		if (clear_menu == NULL) {		
			clear_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(clear_menu, 
				mt_3x_compatibility ? "clear" : "Clear",
				mt_cancel_proc, 0,
				mt_3x_compatibility
					? "clear, no confirm  [Ctrl]"
					: "Clear, No Confirm  [Ctrl]",
				mt_cancel_proc, CTRLMASK,
				0);
		}
		m = clear_menu;
	} else {
		if (cancel_menu == NULL) {
			cancel_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(cancel_menu, 
				mt_3x_compatibility ? "cancel" : "Cancel",
				mt_cancel_proc, 0,
				mt_3x_compatibility
					? "cancel, no confirm [Ctrl]"
					: "Cancel, No Confirm [Ctrl]",
				mt_cancel_proc, CTRLMASK,
				0);
		}
		m = cancel_menu;
	}
	if (operation == MENU_DISPLAY) 
		(void) menu_set(m, MENU_CLIENT_DATA, item, 0);
		/*
		 * for later calls on menu gen proc, m will be the clear or
		 * cancel menu, which does not have an item in its client
		 * data field. OK to set client data field of clear or cancel
		 * menu to item, even though these are global, because only
		 * one cancel menu can be up at a time. 
		 */
	return (m);
}

/* ARGSUSED */
Menu
mt_compose_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu	compose_cc_menu, compose_menu;

	if (mt_value("askcc")) {
		if (compose_cc_menu == NULL) {
			compose_cc_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(compose_cc_menu, 
				mt_3x_compatibility
					? "compose with Cc:"
					: "Compose with Cc:",
				mt_comp_proc, 0,
				mt_3x_compatibility
					? "compose                 [Shift]"
					: "Compose                          [Shift]",
				mt_comp_proc, SHIFTMASK,
				mt_3x_compatibility
					? "compose with Cc:  [Ctrl]"
					: "Compose with Cc:, Include  [Ctrl]",
				mt_comp_proc, CTRLMASK,
				mt_3x_compatibility
					? "forward           [Ctrl][Shift]"
					: "Compose, Include           [Ctrl][Shift]",
				mt_comp_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (compose_cc_menu);
	} else {
		if (compose_menu == NULL) {
			compose_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(compose_menu, 
				mt_3x_compatibility
					? "compose"
					: "Compose",
				mt_comp_proc, 0,
				mt_3x_compatibility
					? "compose with Cc:        [Shift]"
					: "Compose with Cc:                 [Shift]",
				mt_comp_proc, SHIFTMASK,
				mt_3x_compatibility
					? "forward           [Ctrl]"
					: "Compose, Include           [Ctrl]",
				mt_comp_proc, CTRLMASK,
				mt_3x_compatibility
					? "forward with Cc:  [Ctrl][Shift]"
					: "Compose with Cc:, Include  [Ctrl][Shift]",
				mt_comp_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (compose_menu);
	}
}

/* ARGSUSED */
Menu
mt_del_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu delp_menu, delnop_menu;

	if (mt_value("autoprint")) {
		if (delp_menu == NULL) {
			delp_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(delp_menu, 
				mt_3x_compatibility 
					? "delete, show next"
					: "Delete, Show Next",
				mt_del_proc, 0,
				mt_3x_compatibility
					? "delete, show prev        [Shift]"
					: "Delete, Show Previous         [Shift]",
				mt_del_proc, SHIFTMASK,
				mt_3x_compatibility
					? "delete             [Ctrl]"
					: "Delete                  [Ctrl]",
				mt_del_proc, CTRLMASK,
				mt_3x_compatibility
					? "delete, goto prev  [Ctrl][Shift]"
					: "Delete, Go to Previous  [Ctrl][Shift]",
				mt_del_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (delp_menu);
	} else {
		if (delnop_menu == NULL) {		
			delnop_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(delnop_menu, 
				mt_3x_compatibility
					? "delete"
					: "Delete",
				mt_del_proc, 0,
				mt_3x_compatibility
					? "delete, goto prev        [Shift]"
					: "Delete, Go to Previous      [Shift]",
				mt_del_proc, SHIFTMASK,
				mt_3x_compatibility
					? "delete, show next  [Ctrl]"
					: "Delete, Show Next     [Ctrl]",
				mt_del_proc, CTRLMASK,
				mt_3x_compatibility
					? "delete, show prev  [Ctrl][Shift]"
					: "Delete, Show Previous [Ctrl][Shift]",
				mt_del_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (delnop_menu);
	}
}

/* used Menu on frame menu over icon */
Menu
mt_folder_menu_gen_proc(mi, operation)
	Menu_item       mi;
	Menu_generate   operation;
{
	mt_folder_menu = mt_get_folder_menu(mt_folder_menu, "", "");
	return (mt_folder_menu);
}

/* ARGSUSED */
Menu
mt_new_mail_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu system_mailbox_menu, in_folder_menu;

	if (mt_system_mail_box) {
		if (system_mailbox_menu == NULL) {
			system_mailbox_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(system_mailbox_menu, 
				"Retrieve New Mail without Committing Changes",
				mt_new_mail_proc, 0,
				"Commit Changes and Retrieve New Mail",
				mt_commit_proc, 0,
				0);
		}
		return (system_mailbox_menu);
	} else {
		if (in_folder_menu == NULL) {		
			in_folder_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(in_folder_menu, 
				"Commit Changes and Retrieve New Mail",
				mt_new_mail_proc, 0,
				"Commit Changes",
				mt_commit_proc, 0,
				0);
		}
		return (in_folder_menu);
	}
}

/* ARGSUSED */
Menu
mt_panel_style_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu	panel_style_menu;
	int	i, current;
	
	if (panel_style_menu == NULL) {
		panel_style_menu = mt_create_menu_for_button(NULL);
		mt_add_menu_items(panel_style_menu, 
			"old",
			mt_old_style_panel_proc, 0,
			"new",
			mt_new_style_panel_proc, 0,
			"3D images",
			 mt_3Dimages_style_proc, 0,
			0);

		}
	switch (mt_panel_style) {
	case mt_Old:
		current = 1;
		break;
	case mt_New:
		current = 2;
		break;
	case mt_3DImages:
		current = 3;
		break;
	}
	for (i = 1; i < 3; i++)
		(void) menu_set(menu_get(panel_style_menu, MENU_NTH_ITEM, i),
			MENU_INACTIVE, (i == current), 0);
	return (panel_style_menu);
}


/* ARGSUSED */
Menu
mt_reply_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu	replyall_menu, reply_menu;

	if (mt_value("replyall")) {
		if (replyall_menu == NULL) {
			replyall_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(replyall_menu, 
				mt_3x_compatibility
					? "Reply (all)"
					: "Reply (all)",
				mt_reply_proc, 0,
				mt_3x_compatibility
					? "reply                       [Shift]"
					: "Reply                       [Shift]",
				mt_reply_proc, SHIFTMASK,
				mt_3x_compatibility
					? "Reply (all), include  [Ctrl]"
					: "Reply (all), Include  [Ctrl]",
				mt_reply_proc, CTRLMASK,
				mt_3x_compatibility
					? "reply, include        [Ctrl][Shift]"
					: "Reply, Include        [Ctrl][Shift]",
				mt_reply_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (replyall_menu);
	} else {
		if (reply_menu == NULL) {
			reply_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(reply_menu, 
				mt_3x_compatibility
					? "reply"
					: "Reply",
				mt_reply_proc, 0,
				mt_3x_compatibility
					? "Reply (all)                  [Shift]"
					: "Reply (all)                  [Shift]",
				mt_reply_proc, SHIFTMASK,
				mt_3x_compatibility
					? "reply, include         [Ctrl]"
					: "Reply, Include         [Ctrl]",
				mt_reply_proc, CTRLMASK,
				mt_3x_compatibility
					? "Reply (all), include   [Ctrl][Shift]"
					: "Reply (all), Include   [Ctrl][Shift]",
				mt_reply_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (reply_menu);
	}
}

/* ARGSUSED */
Menu
mt_save_gen_proc(m, operation)
	Menu	m;
	Menu_generate	operation;
{
	static Menu savep_menu, savenop_menu;

	if (mt_value("autoprint")) {
		if (savep_menu == NULL) {
			savep_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(savep_menu, 
				mt_3x_compatibility
					? "save, show next"
					: "Save, Show Next",
				mt_save_proc, 0,
				mt_3x_compatibility
					? "save, show prev        [Shift]"
					: "Save, Show Previous         [Shift]",
				mt_save_proc, SHIFTMASK,
				mt_3x_compatibility
					? "save             [Ctrl]"
					: "Save                  [Ctrl]",
				mt_save_proc, CTRLMASK,
				mt_3x_compatibility
					? "save, goto prev  [Ctrl][Shift]"
					: "Save, Go to Previous  [Ctrl][Shift]",
				mt_save_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (savep_menu);
	} else {
		if (savenop_menu == NULL) {
			savenop_menu = mt_create_menu_for_button(NULL);
			mt_add_menu_items(savenop_menu, 
				mt_3x_compatibility
					? "save"
					: "Save",
				mt_save_proc, 0,
				mt_3x_compatibility
					? "save, goto prev        [Shift]"
					: "Save, Go to Previous       [Shift]",
				mt_save_proc, SHIFTMASK,
				mt_3x_compatibility
					? "save, show next  [Ctrl]"
					: "Save, Show Next      [Ctrl]",
				mt_save_proc, CTRLMASK,
				mt_3x_compatibility
					? "save, show prev  [Ctrl][Shift]"
					: "Save, Show Previous  [Ctrl][Shift]",
				mt_save_proc, SHIFTMASK|CTRLMASK,
				0);
		}
		return (savenop_menu);
	}
}

/*
 * Initialize the file menu with the names in the
 * "filemenu" variable.
 */
void
mt_init_filemenu()
{
	register char *p, *s;

	if (mt_file_menu != NULL)
		return;
	if ((p = mt_value("filemenu")) == NULL)
		return;
	for (;;) {
		while (*p && isspace(*p))
			p++;
		if (*p == '\0')
			break;
		s = p;
		while (*p && !isspace(*p))
			p++;
		if (*p)
			*p++ = '\0';
		mt_save_filename(s);
	}
}

