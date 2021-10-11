#ifndef lint
#ifdef sccs
static  char sccsid[] = "@(#)create_panels.c 1.1 92/07/30 Copyr 1987 Sun Micro";
#endif
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

/*
 * Mailtool - creation of control panels
 */

#define NULLSTRING (char *) 0
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <ctype.h>
#include <sunwindow/window_hs.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <varargs.h>

#include <suntool/window.h>
#include <suntool/frame.h>
#include <suntool/panel.h>
#include <suntool/text.h>
#include <suntool/scrollbar.h>
#include <suntool/selection.h>
#include <suntool/selection_svc.h>
#include <suntool/selection_attributes.h>
#include <suntool/walkmenu.h>
#include <suntool/icon.h>

#include "glob.h"
#include "tool.h"

int             mt_cmdpanel_fd;	/* the command panel subwindow fd. Used for
				 * releasing the event lock for some
				 * operations that are time consuming */

Panel_item	mt_show_item, mt_del_item, mt_print_item, mt_state_item;
Panel_item	mt_compose_item,/* mt_others_item, mt_others_label_item,*/ mt_file_item;
Panel_item	mt_folder_item, mt_info_item, mt_dir_item;
Panel_item	mt_deliver_item, mt_cancel_item;
Panel_item	mt_pre_item;

Frame		mt_cd_frame;

static Panel_item	panel_create_old_button(), panel_create_new_button();
static Panel_item	panel_create_image(), panel_create_3Dimage();
static Panel_item	panel_create_3Dbutton(), mt_panel_create_item();

static void	mt_layout_cmdpanel(), mt_create_replysw(), mt_layout_reply_panel(), mt_create_3D_cd_popup(), mt_create_2D_cd_popup();
static void	mt_layout_panel(), mt_cd_done_proc();
static Notify_value	mt_cmdpanel_event_proc(), mt_reply_panel_event_proc(), mt_replysw_event_proc();

#define	TOOL_COLS	80
#define BUTTON_SPACING	5
Pixfont	*mt_3Dfont;
Pixrect *outline_image(), *outline_string();
Pixrect *mt_newmail_pr, *mt_nomail_pr, *mt_trash_pr, *mt_trashfull_pr;

#ifdef USE_IMAGES

/*
 * Images for buttons
 */

static short mt_show1_data[ ] = {
#include <images/show.image>
};
mpr_static(mt_show1_pr, 32, 33, 1, mt_show1_data);

static short mt_next1_data[ ] = {
#include <images/next.image>
};
mpr_static(mt_next1_pr, 16, 23, 1, mt_next1_data);

static short mt_trash1_data[ ] = {
#include <images/trash.image>
};
mpr_static(mt_trash1_pr, 32, 30, 1, mt_trash1_data);

static short mt_trashfull1_data[ ] = {
#include <images/trashfull.image>
};
mpr_static(mt_trashfull1_pr, 32, 30, 1, mt_trashfull1_data);

static short mt_reply1_data[ ] = {
#include <images/reply.image>
};
mpr_static(mt_reply1_pr, 64, 31, 1, mt_reply1_data);

static short mt_compose1_data[ ] = {
#include <images/compose.image>
};
mpr_static(mt_compose1_pr, 32, 31, 1, mt_compose1_data);

static short mt_folder1_data[ ] = {
#include <images/folder.image>
};
mpr_static(mt_folder1_pr, 32, 28, 1, mt_folder1_data);

static short mt_save1_data[ ] = {
#include <images/save.image>
};
mpr_static(mt_save1_pr, 32, 31, 1, mt_save1_data);

static short mt_printer1_data[ ] = {
#include <images/printer.image>
};
mpr_static(mt_printer1_pr, 48, 27, 1, mt_printer1_data);

static short mt_newmail1_data[ ] = {
#include <images/newmail.image>
};
mpr_static(mt_newmail1_pr, 32, 31, 1, mt_newmail1_data);

static short mt_nomail1_data[ ] = {
#include <images/nomail.image>
};
mpr_static(mt_nomail1_pr, 32, 31, 1, mt_nomail1_data);


/* images with no outlines for use with 3d buttons */

static short    mt_show2_data[] = {
#include <images/show.3Dimage>
};
mpr_static(mt_show2_pr, 64, 52, 1, mt_show2_data);

static short    mt_load2_data[] = {
#include <images/load.3Dimage>
};
mpr_static(mt_load2_pr, 64, 52, 1, mt_load2_data);

static short    mt_store2_data[] = {
#include <images/store.3Dimage>
};
mpr_static(mt_store2_pr, 64, 52, 1, mt_store2_data);

static short    mt_print2_data[] = {
#include <images/print.3Dimage>
};
mpr_static(mt_print2_pr, 64, 52, 1, mt_print2_data);

static short    mt_new_mail2_data[] = {
#include <images/new_mail_open.3Dimage>
};
mpr_static(mt_new_mail2_pr, 64, 52, 1, mt_new_mail2_data);

static short    mt_no_mail2_data[] = {
#include <images/no_mail.3Dimage>
};
mpr_static(mt_no_mail2_pr, 64, 52, 1, mt_no_mail2_data);

static short    mt_compose2_data[] = {
#include <images/compose.3Dimage>
};
mpr_static(mt_compose2_pr, 64, 52, 1, mt_compose2_data);

static short    mt_reply2_data[] = {
#include <images/reply.3Dimage>
};
mpr_static(mt_reply2_pr, 64, 52, 1, mt_reply2_data);

static short    mt_delete2_data[] = {
#include <images/delete.3Dimage>
};
mpr_static(mt_delete2_pr, 64, 52, 1, mt_delete2_data);

static short    mt_deletefull2_data[] = {
#include <images/delete_full.3Dimage>
};
mpr_static(mt_deletefull2_pr, 64, 52, 1, mt_deletefull2_data);

static short    mt_prev2_data[] = {
#include <images/prev.3Dimage>
};
mpr_static(mt_prev2_pr, 28, 20, 1, mt_prev2_data);

static short    mt_next2_data[] = {
#include <images/next.3Dimage>
};
mpr_static(mt_next2_pr, 28, 20, 1, mt_next2_data);

static short    mt_done2_data[] = {
#include <images/done.3Dimage>
};
mpr_static(mt_done2_pr, 64, 20, 1, mt_done2_data);

static short    mt_misc2_data[] = {
#include <images/misc.3Dimage>
};
mpr_static(mt_misc2_pr, 64, 20, 1, mt_misc2_data);

#endif

static short mt_menu1_data[ ] = {
#include <images/menu.image>
};
mpr_static(mt_menu1_pr, 16, 16, 1, mt_menu1_data);
/*
 * above is outside of ifdef because it is used in the old style panel to
 * provide a method fo getting back to the new style (or three d) panel
 */

/*
 * create control panel using graphic images for buttons, simplified layout.
 * Panel is 3 rows high, not scrollable 
 */
void
mt_create_3Dimages_panel()
{
#ifdef USE_IMAGES
	Panel_item      item;
	Menu            menu, pull_right;
	struct panel_item_data *p;

	mt_3Dfont = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.r.13");
	/* NEED TO CHECK FOR FONT BEING THERE */
	mt_cmdpanel = window_create(mt_frame, PANEL,
		WIN_ERROR_MSG, "Unable to create panel\n",
		WIN_SHOW, FALSE,
		0);
	if (mt_cmdpanel == NULL) {
		if (mt_debugging)
		    (void)fprintf(stderr,"Unable to create 3D-image panel\n");
                mt_warn(mt_frame, "Unable to create 3D-image panel", 0);
		exit(1);
	}	
	mt_add_window((Window)mt_cmdpanel, mt_Panel);
	mt_cmdpanel_fd = (int)window_get(mt_cmdpanel, WIN_FD);

	(void) panel_create_3Dimage(mt_cmdpanel,
		&mt_prev2_pr, 5, 5, 
		mt_prev_proc,
		"Previous", 
		NULLSTRING, NULLSTRING, NULLSTRING);

	(void) panel_create_3Dimage(mt_cmdpanel,
		&mt_next2_pr, 5, 37, 
		mt_next_proc,
		"Next", 
		NULLSTRING, NULLSTRING, NULLSTRING);

	mt_show_item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_show2_pr, (1*77)-31, 5, 
		mt_show_proc,
		"Show", 
		"Show Full Header  [Shift]", NULLSTRING, NULLSTRING);

	mt_trash_pr = outline_image(&mt_delete2_pr);
	mt_trashfull_pr = outline_image(&mt_deletefull2_pr);
	mt_del_item = panel_create_3Dimage(mt_cmdpanel, 
		((mt_delp == NULL) ? &mt_delete2_pr : &mt_delete2_pr),
		(2*77)-31, 5,
		mt_del_proc, 
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	menu = mt_create_menu_for_button(mt_del_item);	/* the menu behind
							 * mt_del_item
							 */
	pull_right = mt_create_menu_for_button((Panel_item) 0);
		/* the pullright menu */
	(void) menu_set(pull_right, MENU_GEN_PROC, mt_del_gen_proc, 0); 
	/* delete menu depends on setting of "autoprint" */
	(void) menu_set(menu, MENU_PULLRIGHT_ITEM, "Delete", pull_right, 0); 
	mt_add_menu_item(menu, "Undelete", mt_undel_proc, 0);
	(void) menu_set(menu_get(menu, MENU_NTH_ITEM, 2),
		MENU_INACTIVE, (mt_delp == NULL),
		0);

	item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_store2_pr, (3*77)-31, 5, mt_save_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	pull_right = mt_create_menu_for_button((Panel_item) 0);
		/* the pullright menu */
	(void) menu_set(pull_right, MENU_GEN_PROC, mt_save_gen_proc, 0); 
	/* save menu depends on setting of "autoprint" */
	menu = mt_create_menu_for_button(item);	/* the menu behind
						 * the save button
						 */
	(void) menu_set(menu, MENU_PULLRIGHT_ITEM, "Save", pull_right, 0); 
	mt_add_menu_item(menu, "Copy", mt_copy_proc, 0);

	mt_folder_item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_load2_pr, (4*77)-31, 5, mt_folder_proc, 
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_set(mt_folder_item, PANEL_EVENT_PROC, mt_folder_event, 0);
	/*
	 * mt_folder_event computes a menu consisting of the folders 
	 */

	mt_compose_item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_compose2_pr, (5*77)-31, 5, mt_comp_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);

	/*
	 * compose menu depends on setting of "askcc"  
	 */
	(void) menu_set(mt_create_menu_for_button(mt_compose_item),
		MENU_GEN_PROC, mt_compose_gen_proc, 0);

	item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_reply2_pr, (6*77)-31, 5, mt_reply_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * reply menu is depends on on setting of "replyall"  
	 */
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_reply_gen_proc, 0);

	mt_print_item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_print2_pr, (7*77)-31, 5,
		mt_print_proc, "Print", NULLSTRING, NULLSTRING, NULLSTRING);

	mt_newmail_pr = outline_image(&mt_new_mail2_pr);
	mt_nomail_pr = outline_image(&mt_no_mail2_pr);
	mt_state_item = panel_create_3Dimage(mt_cmdpanel, 
			&mt_new_mail2_pr, (8*77)-31, 5, mt_new_mail_proc,
	   		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);

	/*
	 * menu depends on whether in system mail box or not  
	 */
	(void) menu_set(mt_create_menu_for_button(mt_state_item), MENU_GEN_PROC,
		mt_new_mail_gen_proc, 0);

	mt_file_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
		PANEL_LABEL_STRING, "File:",
		PANEL_LABEL_X, 5,
		PANEL_LABEL_Y, 71,
		PANEL_VALUE_STORED_LENGTH, 128,
		PANEL_VALUE_DISPLAY_LENGTH, 45, /* XXX SHOULD FIGURE OUT HOW TO COMPUTE THIS NUMBER SO WHEN PANEL IS STRETCHED, IT GET's READJUSTED */
		PANEL_EVENT_PROC, mt_file_event,
		PANEL_BOXED, TRUE,
		0);

	item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_misc2_pr, (7*77)-31, 70, mt_cd_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	menu = mt_create_menu_for_button(item);
	mt_add_menu_items(menu,
		"Change Directory", mt_cd_proc, 0,
		"Source .mailrc", mt_mailrc_proc, 0,
		"Preserve", mt_preserve_proc, 0,
		0);
	if (mt_41_features)
		(void) menu_set(menu,
			MENU_GEN_PULLRIGHT_ITEM, "Panel Style",
			mt_panel_style_gen_proc,
			0); 

	item = panel_create_3Dimage(mt_cmdpanel, 
		&mt_done2_pr, (8*77)-31, 70, mt_done_proc,
		"Commit Changes and Close", NULLSTRING, NULLSTRING, NULLSTRING);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
			item, PANEL_CLIENT_DATA));
	mt_add_menu_items(p->menu,
		"Commit Changes and Quit", mt_quit_proc, 0,
		"Quit Without Committing Changes", mt_abort_proc, 0,
		0);

	p = (struct panel_item_data *)calloc(1, 
		sizeof(struct panel_item_data));
	window_fit_height(mt_cmdpanel);
	(void) window_set(mt_cmdpanel, WIN_SHOW, TRUE, 0);
#endif
}

void
mt_create_cd_popup()
{
	if (nfds_avail() < 4) {
		mt_warn(mt_frame, "Not enough fds left to create popup!", 0);
		return;
	} else if (mt_panel_style == mt_3DImages)
		mt_create_3D_cd_popup();
	else
		mt_create_2D_cd_popup();
	panel_set_value(mt_dir_item, mt_wdir);
	(void) window_set(mt_cd_frame, 
		WIN_SHOW, TRUE, 
		0);
}

static void
mt_create_3D_cd_popup()
{
#ifdef USE_IMAGES
	Panel		mt_cd_panel;
	int		x;

	mt_cd_frame = window_create(mt_frame, FRAME, 
		WIN_ERROR_MSG, "Unable to create 3D-cd frame\n",	
		0);
        if (mt_cd_frame == NULL) {
		if (mt_debugging)
                	(void)fprintf(stderr,"Unable to create 3D-cd frame\n",
                mt_warn(mt_frame, "Unable to create 3D-cd frame", 0);
                exit(1);
        }
	mt_cd_panel = window_create(mt_cd_frame, PANEL,
		WIN_ERROR_MSG, "Unable to create 3D-cd panel\n",
		WIN_FONT, mt_3Dfont,
		0);
        if (mt_cd_panel == NULL) {
		if (mt_debugging)
                	(void)fprintf(stderr,"Unable to create 3D-cd panel\n");
                mt_warn(mt_frame, "Unable to create 3D-cd panel", 0);
                exit(1);
        }
	(void) panel_create_3Dbutton(mt_cd_panel, 
		"Change Directory", 0, 0, mt_cd_proc,
		"Change Directory", NULLSTRING, NULLSTRING, NULLSTRING);

	(void) panel_create_3Dbutton(mt_cd_panel, 
		"Dismiss", 0, 53, mt_cd_done_proc,
		"Dismiss This Panel", NULLSTRING, NULLSTRING, NULLSTRING);

	(void) window_set(mt_cd_frame, FRAME_DONE_PROC, mt_cd_done_proc, 0);

	mt_dir_item = panel_create_item(mt_cd_panel, PANEL_TEXT,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1) + 20,
		PANEL_VALUE_DISPLAY_LENGTH, 50,
		PANEL_LABEL_STRING,         "Directory:",
		PANEL_BOXED,                TRUE,
		0);

	window_fit(mt_cd_panel);
	window_fit(mt_cd_frame);
	x = ((int)window_get(mt_frame, WIN_WIDTH) -
		(int)window_get(mt_cd_frame, WIN_WIDTH)) / 2;
	(void) window_set(mt_cd_frame, 
		WIN_X, x,
		WIN_Y, 320,
		0);
#endif
}

static void
mt_create_2D_cd_popup()
{
	Panel		mt_cd_panel;
	int		x;

	mt_cd_frame = window_create(mt_frame, FRAME, 
		WIN_ERROR_MSG, "Unable to create 2D-cd frame\n",
		0);
        if (mt_cd_frame == NULL) {
		if (mt_debugging)
                	(void)fprintf(stderr,"Unable to create 2D-cd frame\n");
                mt_warn(mt_frame, "Unable to create 2D-cd frame", 0);
                exit(1);
        }
	mt_cd_panel = window_create(mt_cd_frame, PANEL,
		WIN_ERROR_MSG, "Unable to create 2D-cd panel\n",
		WIN_FONT, mt_font,
		0);
        if (mt_cd_panel == NULL) {
		if (mt_debugging)
                	(void)fprintf(stderr,"Unable to create 2D-cd panel\n");
                mt_warn(mt_frame, "Unable to create 2D-cd panel", 0);
                exit(1);
        }
	(void) panel_create_new_button(mt_cd_panel, 
		"Change Directory", 0, 0, mt_cd_proc,
		"Change Directory", NULLSTRING, NULLSTRING, NULLSTRING);

	(void) panel_create_new_button(mt_cd_panel, 
		"Dismiss", 0, 53, mt_cd_done_proc,
		"Dismiss This Panel", NULLSTRING, NULLSTRING, NULLSTRING);

	(void) window_set(mt_cd_frame, FRAME_DONE_PROC, mt_cd_done_proc, 0);

	mt_dir_item = panel_create_item(mt_cd_panel, PANEL_TEXT,
		PANEL_ITEM_X, ATTR_COL(0),
		PANEL_ITEM_Y, ATTR_ROW(1) + 10,
		PANEL_VALUE_DISPLAY_LENGTH, 50,
		PANEL_LABEL_STRING,         "Directory:",
		PANEL_BOXED,                TRUE,
		0);

	window_fit(mt_cd_panel);
	window_fit(mt_cd_frame);
	x = ((int)window_get(mt_frame, WIN_WIDTH) -
		(int)window_get(mt_cd_frame, WIN_WIDTH)) / 2;
	(void) window_set(mt_cd_frame, 
		WIN_X, x,
		WIN_BELOW, mt_cmdpanel,
		0);
}

/*
 * dismiss popup panel. Called from done button on panel and its frame menu.
 */
/* ARGSUSED */
static void
mt_cd_done_proc(item, ie)
	Panel_item      item;
	Event          *ie;
{
	mt_destroy_cd_popup();
	/* window_set(mt_misc_frame, WIN_SHOW, FALSE, 0); */
}

void
mt_destroy_cd_popup()
{
	mt_dir_item = NULL;
	(void) window_set(mt_cd_frame, FRAME_NO_CONFIRM, TRUE, 0);
	(void) window_destroy(mt_cd_frame);
	mt_cd_frame = NULL;
}

/*
 * create control panel using labelled buttons, simplified layout.
 * Panel is 3 rows high, not scrollable 
 */
void
mt_create_new_style_panel()
{
	Panel_item      item;
	Menu            menu, pull_right;
	struct panel_item_data *p;
	int		button_width, last_button_col, i;
	char		*s;

	button_width = 9;
	last_button_col = 71;
	if ((s = mt_value("buttonwidth")) && (i = atoi(s)) > 0)
		button_width = i;

	mt_cmdpanel = window_create(mt_frame, PANEL,
		WIN_ROW_GAP, 7, 
		WIN_ERROR_MSG, "Unable to create new-style panel\n",
		WIN_SHOW, FALSE,/* set to true after call to
				 * mt_layout_cmdpanel or otherwise in case
				 * where changing style of panels, i.e.,
				 * window is already visible, would see those
				 * items that are reset by mt_layout would
				 * get painted */
		0);
	if (mt_cmdpanel == NULL) { 
		if (mt_debugging)
       		    (void)fprintf(stderr,"Unable to create new-style panel\n");
                mt_warn(mt_frame, "Unable to create new-style panel", 0);
        	exit(1); 
        }
	mt_add_window((Window)mt_cmdpanel, mt_Panel);
	mt_cmdpanel_fd = (int)window_get(mt_cmdpanel, WIN_FD);
	mt_show_item = panel_create_new_button(mt_cmdpanel, "Show", 0, 0,
		mt_show_proc,
		"Show", 
		"Show Full Header  [Shift]", NULLSTRING, NULLSTRING);

	(void) panel_create_new_button(mt_cmdpanel, "Next",
		0, (1 * button_width), mt_next_proc,
		"Next", 
		"Previous  [Shift]", NULLSTRING, NULLSTRING);

	mt_del_item = panel_create_new_button(mt_cmdpanel, "Delete",
		0, (2 * button_width), 
		mt_del_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	pull_right = mt_create_menu_for_button((Panel_item) 0);
		/* the pullright menu */
	(void) menu_set(pull_right, MENU_GEN_PROC, mt_del_gen_proc, 0); 
	/* delete menu depends on setting of "autoprint" */
	menu = mt_create_menu_for_button(mt_del_item);	/* the menu behind
							 * mt_del_item
							 */
	(void) menu_set(menu, MENU_PULLRIGHT_ITEM, "Delete", pull_right, 0); 
	mt_add_menu_item(menu, "Undelete", mt_undel_proc, 0);
	(void) menu_set(menu_get(menu, MENU_NTH_ITEM, 2), MENU_INACTIVE, 
		(mt_delp == NULL), 0);

	item = panel_create_new_button(mt_cmdpanel, "Save",
		1, 0, mt_save_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	pull_right = mt_create_menu_for_button((Panel_item) 0);
		/* the pullright menu */
	(void) menu_set(pull_right, MENU_GEN_PROC, mt_save_gen_proc, 0); 
	/* save menu depends on setting of "autoprint" */
	menu = mt_create_menu_for_button(item);	/* the menu behind
						 * the save button
						 */

	(void) menu_set(menu, MENU_PULLRIGHT_ITEM, "Save", pull_right, 0); 
	mt_add_menu_item(menu, "Copy", mt_copy_proc, 0);

	mt_folder_item = panel_create_new_button(mt_cmdpanel, "Folder",
		1, (1 * button_width),
		mt_folder_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_set(mt_folder_item, PANEL_EVENT_PROC, mt_folder_event, 0);
	/*
	 * mt_folder_event computes a menu consisting of the folders 
	 */

	item = panel_create_new_button(mt_cmdpanel, "Reply",
		0, (3 * button_width), 
		mt_reply_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * reply menu is depends on on setting of "replyall"  
	 */
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_reply_gen_proc, 0);

	mt_compose_item = panel_create_new_button(mt_cmdpanel, "Compose",
		0, (4 * button_width),
		mt_comp_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);

	/*
	 * compose menu is depends on on setting of "askcc"  
	 */
	(void) menu_set(mt_create_menu_for_button(mt_compose_item),
		MENU_GEN_PROC, mt_compose_gen_proc,
		0);

	mt_print_item = panel_create_new_button(mt_cmdpanel, "Print",
		0, (last_button_col - (1 * button_width)), 
		mt_print_proc, "Print", NULLSTRING, NULLSTRING, NULLSTRING);

	mt_state_item = panel_create_new_button(mt_cmdpanel, "New Mail", 
			0, last_button_col, mt_new_mail_proc,
	   		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * menu depends on whether in system mail box or not  
	 */
	(void) menu_set(mt_create_menu_for_button(mt_state_item), MENU_GEN_PROC,
		mt_new_mail_gen_proc, 0);

	item = panel_create_old_button(mt_cmdpanel, "Done", 1, last_button_col,
		mt_done_proc, "Commit Changes and Close",
		NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * difference between old and new is padding of  7 vs 8. Since this
	 * is under the new mail button, and that is wider because it
	 * consists of 8 characters, looks better if done button is same width. 
	 */
	p = (struct panel_item_data *)LINT_CAST(panel_get(
			item, PANEL_CLIENT_DATA));
	mt_add_menu_items(p->menu,
		"Commit Changes and Quit", mt_quit_proc, 0,
		"Quit Without Committing Changes", mt_abort_proc, 0,
		0);

	item = panel_create_new_button(mt_cmdpanel, 
		"Misc", 1, (last_button_col - (1 * button_width)), mt_cd_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	menu = mt_create_menu_for_button(item);
	mt_add_menu_items(menu,
		"Change Directory", mt_cd_proc, 0,
		"Source .mailrc", mt_mailrc_proc, 0,
		"Preserve", mt_preserve_proc, 0,
		0);
	if (mt_41_features)
		(void) menu_set(menu,
			MENU_GEN_PULLRIGHT_ITEM, "Panel Style",
			mt_panel_style_gen_proc,
			0); 

	p = (struct panel_item_data *)calloc(1, 
		sizeof(struct panel_item_data));
	p->width = (last_button_col - (1 * button_width))
		- (2 * button_width) - 8;
	/*
	 * position of misc, minus 18, position of "File:"
	 * minus 6, number of characters in "File: " + some padding 
	 */
	mt_file_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
		PANEL_LABEL_STRING, "File:",
		PANEL_LABEL_X, ATTR_COL(2 * button_width) + 5,
		PANEL_LABEL_Y, ATTR_ROW(1) + 2 + BUTTON_SPACING,
		PANEL_VALUE_DISPLAY_LENGTH, p->width,
		PANEL_CLIENT_DATA, p,
		PANEL_EVENT_PROC, mt_file_event,
		0);
	if (mt_41_features)
		(void) panel_set(mt_file_item, PANEL_BOXED, TRUE, 0);

	(void) window_set(mt_cmdpanel, WIN_FIT_HEIGHT, BUTTON_SPACING, 0);
	mt_layout_cmdpanel();	/* readjust positions of right justified
				 * items in case width of tool is not the
				 * default */
	(void) window_set(mt_cmdpanel,
		WIN_SHOW, TRUE,
		0);
	notify_interpose_event_func(mt_cmdpanel,
		mt_cmdpanel_event_proc, NOTIFY_SAFE);
	/* arrange to relay out cmd panel when resized */
}

/*
 * create cmd panel.  7 rows of buttons, 4 visible, no graphics, i.e.,
 * identical to that in 3.2 release 
 */
void
mt_create_old_style_panel()
{
	Panel_item      item;
	Menu            menu;
	struct panel_item_data *p;

	mt_cmdpanel = window_create(mt_frame, PANEL,
		WIN_ROW_GAP, 7,
		WIN_HEIGHT, ATTR_LINES(mt_cmdlines) + 4 + BUTTON_SPACING, 
		WIN_VERTICAL_SCROLLBAR, scrollbar_create(0),
		WIN_ERROR_MSG, "Unable to create old-style panel\n",
		0);
        if (mt_cmdpanel == NULL) {  
		if (mt_debugging)
                    (void)fprintf(stderr,"Unable to create old-style panel\n"); 
                mt_warn(mt_frame, "Unable to create old-style panel", 0);
                exit(1);  
        }
	mt_add_window((Window)mt_cmdpanel, mt_Panel);
	mt_cmdpanel_fd = window_fd(mt_cmdpanel);

	/* first line */
	mt_show_item = panel_create_old_button(mt_cmdpanel, "show", 0, 0,
		mt_show_proc,
		"show", 
		"show all headers  [Shift]", NULLSTRING, NULLSTRING);
	(void) panel_create_old_button(mt_cmdpanel, "next", 0, 10, mt_next_proc,
		"next", 
		"prev  [Shift]", NULLSTRING, NULLSTRING);
	item = panel_create_old_button(mt_cmdpanel, "delete", 0, 20,
		mt_del_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/* delete menu depends on setting of "autoprint" */
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_del_gen_proc, 0);

	(void) panel_create_old_button(mt_cmdpanel, "undelete", 0, 30,
		mt_undel_proc, "undelete", NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_create_old_button(mt_cmdpanel, "print", 0, 40,
		mt_print_proc, "print", NULLSTRING, NULLSTRING, NULLSTRING);
	 mt_state_item = panel_create_old_button(mt_cmdpanel, "new mail", 0, 50,
		mt_new_mail_proc,
		(mt_3x_compatibility ? "new mail" : "incorporate new mail"),
		NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_create_old_button(mt_cmdpanel, "done", 0, 60,
		mt_done_proc, "done", NULLSTRING, NULLSTRING, NULLSTRING);


	/* second line */
	item = panel_create_old_button(mt_cmdpanel, "reply", 1, 0,
		mt_reply_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * reply menu is depends on on setting of "replyall"  
	 */
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_reply_gen_proc, 0);
	mt_compose_item = panel_create_old_button(mt_cmdpanel, "compose",
		1, 10, mt_comp_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	/*
	 * compose menu is depends on on setting of "replyall"  
	 */
	(void) menu_set(mt_create_menu_for_button(mt_compose_item), 
		MENU_GEN_PROC, mt_compose_gen_proc, 0);
	mt_deliver_item = panel_create_old_button(mt_cmdpanel, "deliver", 1, 30,
		mt_deliver_proc, "deliver", NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_set(mt_deliver_item, PANEL_SHOW_ITEM, FALSE, 0);
	mt_cancel_item = panel_create_old_button(mt_cmdpanel, "cancel", 1, 40,
		mt_cancel_proc,
		"cancel", "", "cancel, no confirm  [Ctrl]", NULLSTRING);
	(void) panel_set(mt_cancel_item, PANEL_SHOW_ITEM, FALSE, 0);
	(void) panel_create_old_button(mt_cmdpanel, "commit", 1, 60,
		mt_commit_proc, "commit", NULLSTRING, NULLSTRING, NULLSTRING);

	/* third line */
	item = panel_create_old_button(mt_cmdpanel, "save", 2, 0, mt_save_proc,
		NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_save_gen_proc, 0);
	/* save menu depends on setting of "autoprint" */
	(void) panel_create_old_button(mt_cmdpanel, "copy", 2, 10, mt_copy_proc,
		"copy", NULLSTRING, NULLSTRING, NULLSTRING);
	mt_file_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
		PANEL_LABEL_STRING, "File: ",
		PANEL_LABEL_Y, ATTR_ROW(2) + 7,
		PANEL_LABEL_X, ATTR_COL(20) + 5,
		PANEL_VALUE_STORED_LENGTH, 1024,
		PANEL_VALUE_DISPLAY_LENGTH, 50,
		PANEL_EVENT_PROC, mt_file_event,
		0);

	/* fourth line */
	mt_folder_item = panel_create_old_button(mt_cmdpanel, "folder", 3, 0,
		mt_folder_proc, NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_set(mt_folder_item, PANEL_EVENT_PROC, mt_folder_event, 0);

	p = (struct panel_item_data *)calloc(1, 
		sizeof(struct panel_item_data));
	mt_info_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
		PANEL_LABEL_STRING, "",
		PANEL_LABEL_Y, ATTR_ROW(3) + 7,
		PANEL_LABEL_X, ATTR_COL(11) + 5,
		PANEL_VALUE_DISPLAY_LENGTH, 68,
		PANEL_VALUE_STORED_LENGTH, 128,
		PANEL_CLIENT_DATA, p,
		PANEL_EVENT_PROC, mt_panel_event,
		PANEL_READ_ONLY, TRUE,
		0);
	/*
	 * this really should be a MESSAGE item but no way to specify 
	 * display length for MESSAGE items 
	 */

	/* fifth line */
	mt_pre_item = panel_create_old_button(mt_cmdpanel, "preserve", 4, 0,
		mt_preserve_proc, "preserve",
		NULLSTRING, NULLSTRING, NULLSTRING);
	(void) panel_create_old_button(mt_cmdpanel, "cd", 4, 10, mt_cd_proc,
		"change directory", NULLSTRING, NULLSTRING, NULLSTRING);
	mt_dir_item = panel_create_item(mt_cmdpanel, PANEL_TEXT,
		PANEL_LABEL_STRING, "Directory: ",
		PANEL_LABEL_Y, ATTR_ROW(4) + 7,
		PANEL_LABEL_X, ATTR_COL(20) + 5,
		PANEL_VALUE, mt_wdir,
		PANEL_VALUE_STORED_LENGTH, 1024,
		PANEL_VALUE_DISPLAY_LENGTH, 50,
		0);

	/* sixth line */
	(void) panel_create_old_button(mt_cmdpanel, ".mailrc", 5, 0,
		mt_mailrc_proc, "source .mailrc",
		NULLSTRING, NULLSTRING, NULLSTRING);

	if (mt_41_features) {
		item = panel_create_image(mt_cmdpanel, 
			&mt_menu1_pr, 
			ATTR_ROW(5) + 5, 11, 
			mt_nop_proc,
			NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
		menu = mt_create_menu_for_button(item);
		(void) menu_set(menu,
			MENU_GEN_PROC, mt_panel_style_gen_proc,
			0); 
		item = panel_create_item(mt_cmdpanel, PANEL_MESSAGE,
			PANEL_LABEL_STRING, "Panel Style",
			PANEL_LABEL_X, ATTR_COL(14), 
			PANEL_LABEL_Y, ATTR_ROW(5) + 6,
			PANEL_EVENT_PROC, mt_nop_proc,
			0);
	}
	(void) panel_create_old_button(mt_cmdpanel, "quit", 5, 50, mt_quit_proc,
		"quit", "", 
		"quit, no confirm  [Ctrl]", NULLSTRING);
	(void) panel_create_old_button(mt_cmdpanel, "abort", 5, 60,
		mt_abort_proc,
		"abort", "", 
		"abort, no confirm  [Ctrl]", NULLSTRING);
}

struct reply_panel_data *
mt_create_reply_panel(frame)
	Frame	frame;
{
	Panel           		panel;
	Panel_item      		item;
	struct reply_panel_data 	*ptr;
	struct panel_item_data		*p;

	if((ptr = (struct reply_panel_data *)LINT_CAST(calloc(1,
		sizeof(struct reply_panel_data)))) == NULL)
	  {
	    mt_warn(frame, "malloc failed in mt_create_reply_panel,", 0);
	    return(NULL);
	  }
	ptr->behavior = mt_Disappear; 
	ptr->inuse = FALSE; 
	ptr->normalized = TRUE;
	ptr->frame = frame;

	if (mt_3x_compatibility) {
		mt_create_replysw(ptr);
		return(ptr);
	}

	panel = window_create(frame, PANEL,
		WIN_ERROR_MSG, "Unable to create reply panel\n",
		WIN_SHOW, FALSE,
		WIN_FONT, ((mt_panel_style == mt_3DImages) 
			? mt_3Dfont : mt_font),
		0);
	if (panel == NULL) {
		if (mt_debugging)
                        (void)fprintf(stderr,"Unable to create reply panel\n");
		mt_warn(frame, "Unable to create reply panel,", 0);
		free(ptr);
		ptr = NULL;  /* make sure ptr does not point to anything */
		return (ptr);
	}
	(void) panel_set(panel, PANEL_CLIENT_DATA, ptr, 0);
	ptr->reply_panel = panel;

	if (mt_panel_style == mt_3DImages) {
		(void) panel_create_3Dbutton(panel, "Include",
			0, 0, mt_include_proc,
			    "Include, Bracketed",
			    "",
			    "Include, Indented                [Ctrl]", 
			NULL);
		ptr->deliver_item = panel_create_3Dbutton(panel,
			"Deliver", 0, 11, mt_deliver_proc,
			"Deliver, Take Down Window",
			"",
			"Deliver, Leave Window Intact [Ctrl]",
			 NULLSTRING);

		(void) panel_create_3Dbutton(panel,
			"Re-address", 0, 32, mt_readdress_proc,
			"Insert Address Template", 
			NULLSTRING, NULLSTRING, NULLSTRING);
		/*
		 * re-address button also on reply panel so that user can
		 * close mailtool but still compose messages in a popup. Also
		 * allows user to use deliver, leave intact option, and then
		 * wrap To:, Subject: header around the message 
		 */

		item = panel_create_3Dbutton(panel,
			"Cancel", 0, 22, mt_cancel_proc,
			NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	} else {
		(void) panel_create_new_button(panel, "Include", 0, 0,
			mt_include_proc,
			"Include, Bracketed",
			"",
			"Include, Indented                [Ctrl]", 
			NULLSTRING);
		ptr->deliver_item = panel_create_new_button(panel,
			"Deliver", 0, 9, mt_deliver_proc,
			"Deliver, Take Down Window",
			"",
			"Deliver, Leave Window Intact [Ctrl]",
			 NULLSTRING);

		(void) panel_create_new_button(panel,
			"Re-address", 0, 28, mt_readdress_proc,
			"Insert Address Template", 
			NULLSTRING, NULLSTRING, NULLSTRING);
		item = panel_create_new_button(panel,
			"Cancel", 0, 18, mt_cancel_proc,
			NULLSTRING, NULLSTRING, NULLSTRING, NULLSTRING);
	}
	(void) menu_set(mt_create_menu_for_button(item), MENU_GEN_PROC,
		mt_cancel_gen_proc, 0);
	ptr->cancel_item = item;/* so can change string to "clear"  when
				 * replysw is made permanent */

	p = (struct panel_item_data *)calloc(1, 
		sizeof(struct panel_item_data));
	p->column = 69;
	item = panel_create_item(panel, PANEL_CYCLE,
		PANEL_CHOICE_STRINGS, "Disappear", "Stay Up",  0,
		PANEL_LABEL_X, ATTR_COL(p->column) + 5,
		PANEL_LABEL_Y, -4,
		PANEL_NOTIFY_PROC, mt_replysw_proc,
		PANEL_EVENT_PROC, mt_panel_event,
		PANEL_CLIENT_DATA, p,
		0); 
	/*
	 * reason for having this go through mt_panel_event is to get
	 * consistent treatement of tracing  
	 */
	if (frame != mt_frame)
		(void) panel_set(item, PANEL_CHOICE_STRINGS,
			"Disappear", "Stay Up", "Close",  0,
			0);
	ptr->cycle_item = item;

	window_fit_height(panel);
	mt_layout_reply_panel(panel);
	/*
	 * arrange to re-layout panel when window is resized. Put this
	 * procedure on the panel, rather than the frame, so that it isn't
	 * called if the window is simply stretched vertically. 
	 */

	mt_create_replysw(ptr);
	if (ptr->replysw == NULL) 
	{  /*
	    * the calling funct mt_create_new_replysw will remove
	    * the panel with a window_done call.
	    */
	   free(ptr);	/* Reply sw not created - mssg already given */
	   ptr = NULL;
	   return(ptr);
	}
	notify_interpose_event_func(panel,
		mt_reply_panel_event_proc, NOTIFY_SAFE);
	mt_add_window((Window)panel, mt_Panel);
	return (ptr);
}

static void
mt_create_replysw(ptr)
	struct reply_panel_data *ptr;
{
	char           *replysw_file;

	ptr->replysw = (Textsw)window_create(ptr->frame,
		TEXT,
		WIN_ERROR_MSG, "Unable to create reply subwindow\n",
		WIN_SHOW, FALSE,
		TEXTSW_MEMORY_MAXIMUM, mt_memory_maximum,
		TEXTSW_CONFIRM_OVERWRITE, FALSE,
		TEXTSW_STORE_SELF_IS_SAVE, TRUE,
		WIN_CLIENT_DATA, ptr,
		0);
	if (ptr->replysw == NULL) {
		if (mt_debugging)
                    (void)fprintf(stderr,"Unable to create reply subwindow\n");
		mt_warn(mt_frame, "Unable to create reply subwindow,", 0);
		return; 
	}
	replysw_file = (char *)LINT_CAST(malloc(32));
	(void) strcpy(replysw_file, "/tmp/MTrXXXXXX");
	(void) mktemp(replysw_file); 
	ptr->replysw_file = replysw_file;

        mt_add_window((Window)ptr->replysw, mt_Text);
	if (!mt_retained)
		notify_interpose_event_func(ptr->replysw,
			mt_replysw_event_proc, NOTIFY_SAFE);
}

static Notify_value
mt_replysw_event_proc(client, event, arg, when)
	Notify_client   client;
	Event          *event;
	Notify_arg      arg;
	Notify_event_type when;
{
	struct reply_panel_data *ptr;
	Notify_value	val;

	ptr = (struct reply_panel_data *)LINT_CAST(window_get(
		client, WIN_CLIENT_DATA));
	val = notify_next_event_func(client, event, arg, when);
	if (!ptr->normalized && event_id(event) == WIN_REPAINT) {
/*	  Textsw_index	point; */ /* not used - hala */

		textsw_insert_makes_visible(client);
		ptr->normalized = TRUE;
	}
	return (val);
}

/*
 * toggle permanent/temporary.
 */
void
mt_replypanel_init(reply_panel)
	Panel           reply_panel;
{
	struct reply_panel_data *ptr;
	struct panel_item_data *p;
	char		*s1, *s2;

	if (reply_panel == NULL)
		return;
	ptr = (struct reply_panel_data *)LINT_CAST(panel_get(
		reply_panel, PANEL_CLIENT_DATA));
	/* update deliver menu */
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		ptr->deliver_item, PANEL_CLIENT_DATA));
	switch (ptr->behavior) {
	case mt_Disappear:
		s1 = "Deliver, Take Down Window";
		s2 = "Cancel";
		break;
	case mt_Close:
		s1 = "Deliver, Close Window";
		s2 = "Cancel";
		break;
	case mt_Stay_Up:
		s1 = "Deliver, Clear Window";
		s2 = "Clear";
		break;
	}
	(void) menu_set(menu_get(p->menu, MENU_NTH_ITEM, 1),
		MENU_STRING, s1,
		0);

	/* update cancel button image. menu is handled via a genproc */
	(void) panel_set(ptr->cancel_item, PANEL_LABEL_IMAGE,
		(mt_panel_style == mt_3DImages)
			? outline_string(s2)
			: panel_button_image(reply_panel, s2, 8, (Pixfont *) 0),
		0);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		ptr->cycle_item, PANEL_CLIENT_DATA));
}

/*
 * Create a button in a panel, and a menu behind it.
 * Strings m1 - m4 specify the menu items
 * that correspond to the normal, shifted,
 * ctrled, and ctrl-shifted versions of the
 * button.
 */
/* ARGSUSED */
static Panel_item
panel_create_old_button(panel, label, row, col, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	char           *label;
	int             row, col;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;
	struct panel_item_data *p;

	item =	mt_panel_create_item(
		panel, panel_button_image(panel, label, 8, (Pixfont *) 0),
		ATTR_ROW(row)  + BUTTON_SPACING,
		ATTR_COL(col) + 5,
		notify_proc, m1, m2, m3, m4);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		item, PANEL_CLIENT_DATA));
	p->column = col;
	return (item);
}

static Panel_item
panel_create_new_button(panel, label, row, col, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	char           *label;
	int             row, col;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;
	struct panel_item_data *p;

	item =	mt_panel_create_item(
		panel, panel_button_image(panel, label, 7, (Pixfont *) 0),
		ATTR_ROW(row) + BUTTON_SPACING,
		ATTR_COL(col) + 5 ,
		notify_proc, m1, m2, m3, m4);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		item, PANEL_CLIENT_DATA));
	p->column = col;
	return (item);
}

static Panel_item
panel_create_3Dbutton(panel, label, row, col, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	char           *label;
	int             row, col;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;
	struct panel_item_data *p;

	item =	mt_panel_create_item(
		panel, outline_string(label),
		ATTR_ROW(row),
		ATTR_COL(col),
		notify_proc, m1, m2, m3, m4);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		item, PANEL_CLIENT_DATA));
	p->column = col;
	return (item);
}

static Panel_item
panel_create_image(panel, image, y, col, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	Pixrect        *image;
	int             y, col;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;
	struct panel_item_data *p;

	item =	mt_panel_create_item(
		panel, image,
		y,
		ATTR_COL(col),
		notify_proc, m1, m2, m3, m4);
	p = (struct panel_item_data *)LINT_CAST(panel_get(
		item, PANEL_CLIENT_DATA));
	p->column = col;
	return (item);
}

static Panel_item
panel_create_3Dimage(panel, image, x, y, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	Pixrect        *image;
	int             x, y;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;

	item =	mt_panel_create_item(
		panel, outline_image(image),
		y,
		x,
		notify_proc, m1, m2, m3, m4);
	return (item);
}

/*
 * Create a button in  panel, and a menu behind it.
 * Strings m1 - m4 specify the menu items
 * that correspond to the normal, shifted,
 * ctrled, and ctrl-shifted versions of the
 * button.
 */
/* ARGSUSED */
static Panel_item
mt_panel_create_item(panel, image, y, x, notify_proc, m1, m2, m3, m4)
	Panel           panel;
	Pixrect        *image;
	int             x, y;
	Void_proc       notify_proc;
	char           *m1, *m2, *m3, *m4;
{
	Panel_item      item;
	Menu            menu;
	register char **ip;
	register int    i;
	static int      menuval[4] = {0, SHIFTMASK,
				      CTRLMASK, SHIFTMASK | CTRLMASK};
	struct panel_item_data *p;

	p = (struct panel_item_data *)LINT_CAST(calloc(1,
		sizeof(struct panel_item_data)));
	item = panel_create_item(panel, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, image,
		PANEL_LABEL_X, x,
		PANEL_LABEL_Y, y,
		PANEL_NOTIFY_PROC, notify_proc,
		PANEL_CLIENT_DATA, p,
		PANEL_EVENT_PROC, mt_panel_event,
		0);
	if (m1 != NULL) {
		menu = mt_create_menu_for_button(item);
		for (ip = &m1, i = 0; i < 4 && *ip != NULL; ip++, i++) {
			if (**ip == '\0')	/* empty strings are skipped */
				continue;
			(void) mt_add_menu_item(
				menu, *ip, notify_proc, menuval[i]);
		}
	}
	return (item);
}

Menu
mt_create_menu_for_button(item)
	Panel_item	item;
{
	Menu            menu;

	menu = menu_create(MENU_CLIENT_DATA, item,
	    MENU_NOTIFY_PROC, menu_return_item, 
	    MENU_LEFT_MARGIN, 10, 
	    0);
	if (item) {
		struct panel_item_data	*p;
		p = (struct panel_item_data *)LINT_CAST(panel_get(
			item, PANEL_CLIENT_DATA));
		p->menu = menu;
	}
	return (menu);
}

void
mt_add_menu_item(menu, string, notify_proc, mask)
	Menu            menu;
	char           *string;
	Void_proc       notify_proc;
	int             mask;
{
	struct Menu_value *ptr;

	ptr = (struct Menu_value *)LINT_CAST(malloc(sizeof(struct Menu_value)));
	ptr->notify_proc = notify_proc;
	ptr->mask = mask;
	(void) menu_set(menu, MENU_STRING_ITEM, string, ptr, 0);
	/* ptr will be the value returned when this item is selected */
}

/* VARARGS1 */
void
mt_add_menu_items(menu, va_alist)
	Menu            menu;
	va_dcl
{
	va_list         args;
	char           *s;
	Void_proc       notify_proc;
	int		mask;

	va_start(args);
    	while ((s = va_arg(args, char *)) != (char *) 0) {
 		notify_proc = va_arg(args, Void_proc);
		mask = va_arg(args, int);
		mt_add_menu_item(menu, s, notify_proc, mask);
	}
	va_end(args);

}

static Notify_value 
mt_cmdpanel_event_proc(client, event, arg, when)
	Notify_client	client;
	Event	*event;
	Notify_arg	arg;
	Notify_event_type	when;

{
	if (event_action(event) == WIN_RESIZE) {
		if (client == mt_cmdpanel)  {
			mt_layout_cmdpanel();
			(void) panel_paint(mt_cmdpanel, PANEL_CLEAR); 
		}
	}
	return (notify_next_event_func(client, event, arg, when));
}

static Notify_value 
mt_reply_panel_event_proc(client, event, arg, when)
	Notify_client   client;
	Event          *event;
	Notify_arg      arg;
	Notify_event_type when;

{

	if (event_action(event) == WIN_RESIZE) {
		Frame	frame;
		struct reply_panel_data *ptr;

		frame = window_get(client, WIN_OWNER);
		if (frame == mt_frame) 
			ptr = (struct reply_panel_data *)LINT_CAST(panel_get(
				mt_cmdpanel, PANEL_CLIENT_DATA));
		else
			ptr = (struct reply_panel_data *)LINT_CAST(window_get(
				frame, WIN_CLIENT_DATA));
		mt_layout_reply_panel(ptr->reply_panel);
	}
	return (notify_next_event_func(client, event, arg, when));
}

static void
mt_layout_cmdpanel()
{
	struct reply_panel_data *ptr;

	ptr = (struct reply_panel_data *)LINT_CAST(panel_get(
			mt_cmdpanel, PANEL_CLIENT_DATA));
	mt_layout_panel(mt_cmdpanel,
		(mt_panel_style == mt_New) ? 5 : 0);
	if (ptr && (window_get(ptr->reply_panel, WIN_OWNER) == mt_frame)) 
		/* there is also a replysw in this frame */
		mt_layout_reply_panel(ptr->reply_panel);

}

static void
mt_layout_reply_panel(panel)
	Panel           panel;
{
	mt_layout_panel(panel, 5);
}

static void
mt_layout_panel(panel, offset)
	Panel           panel;
	int             offset;
{
	Panel_item      item;
	int             width, delta, delta_columns;
	struct panel_item_data *p;

	width = (int)window_get(window_get(panel, WIN_OWNER), WIN_WIDTH);
	if (width == 0)
		/* window not installed yet */
		return;
	delta = width - padding - (TOOL_COLS * charwidth);
	/*
	 * original layout based on a window of width: padding +
	 * ATTR_COL(TOOL_COLS) 
	 */
	delta_columns = delta / charwidth;

	panel_each_item(panel, item) 
		p = (struct panel_item_data *)LINT_CAST(panel_get(
			item, PANEL_CLIENT_DATA));
		if (p->column > 50) 
			(void) panel_set(item, PANEL_LABEL_X,
				ATTR_COL(p->column + delta_columns) + offset,
				0);
		if (p->width > 0)
			(void) panel_set(item, PANEL_VALUE_DISPLAY_LENGTH,
				p->width + delta_columns, 0);
	panel_end_each
}


Pixrect *
outline_image(old)
register Pixrect *old;
{
#ifdef USE_IMAGES
   Pixrect *new;
   int x = old->pr_size.x + 7;
   int y = old->pr_size.y + 7;
   int w = x - 1;
   int h = y - 1;

   new = mem_create(x, y, 1);

   pr_rop(new, 2, 2, w, h, PIX_SRC, old, 0, 0);

   pr_vector(new, 3, 0, w-3, 0, PIX_SRC, 1); /* top */
   pr_vector(new, 0, 3, 3, 0, PIX_SRC, 1); /* nw corner */
   pr_vector(new, w, 3, w-3, 0, PIX_SRC, 1); /* ne corner */

   pr_vector(new, 6, h, w-3, h, PIX_SRC, 1); /* bottom */
   pr_vector(new, 0, h-6, 6, h, PIX_SRC, 1); /* sw corner */
   pr_vector(new, w-3, h, w, h-3, PIX_SRC, 1); /* se corner */


   pr_vector(new, 0, 3, 0, h-6, PIX_SRC, 1); /* left */
   pr_vector(new, w, 3, w, h-3, PIX_SRC, 1); /* right */

   pr_vector(new, w-3, 3, w-3, h-6, PIX_SRC, 1); /* inside right */
   pr_vector(new, 3, h-3, w-6, h-3, PIX_SRC, 1); /* inside bottom */
   pr_vector(new, w-3, 3, w-6, 0, PIX_SRC, 1); /* inside ne corner */
   pr_vector(new, w-6, h-3, w-3, h-6, PIX_SRC, 1); /* inside se corner */

   /* fill in bottom shadow */
   for (x=6; x<w-3; x+=2)
      pr_put(new, x, h-2, 1);
   for (x=7; x<w-2; x+=2)
      pr_put(new, x, h-1, 1);
   pr_put(new, w-5, h-3, 1);
   pr_put(new, w-4, h-3, 1);
   pr_put(new, w-3, h-2, 1);
   pr_put(new, w-3, h-4, 1);

   /* fill in right shadow */
   for (y=3; y<h-2; y+=2)
      pr_put(new, w-2, y, 1);
   for (y=4; y<h-2; y+=2)
      pr_put(new, w-1, y, 1);
   pr_put(new, w-3, 1, 1);
   pr_put(new, w-2, 2, 1);

/* ifdef notdef
   * horizontal lines at bottom *
   pr_vector(new, 3, h-3, w, h-3, PIX_SRC, 1);
   pr_vector(new, 4, h-2, w-1, h-2, PIX_SRC, 1);
   pr_vector(new, 5, h-1, w-1, h-1, PIX_SRC, 1);

   * vertical lines at bottom *
   pr_vector(new, w-3, 0, w-3, h-4, PIX_SRC, 1);
   pr_vector(new, w-2, 1, w-2, h-4, PIX_SRC, 1);
   pr_vector(new, w-1, 2, w-1, h-4, PIX_SRC, 1);
endif
*/

   return(new);
#endif
}

Pixrect *
outline_string(s)
	char           *s;
{
#ifdef USE_IMAGES
	struct pr_prpos where;
	struct pr_size  size;
	int             x, y, w, h;

	size = pf_textwidth(strlen(s), mt_3Dfont, s);
	x = size.x + 24;
	y = size.y + 18;
	w = x - 1;
	h = y - 1;

	where.pr = mem_create(x, y, 1);

	where.pos.x = (w - size.x) / 2;
	where.pos.y = fonthome(mt_3Dfont) + 7;

	pf_text(where, PIX_SRC, mt_3Dfont, s);

	pr_vector(where.pr, 3, 0, w - 3, 0, PIX_SRC, 1);	/* top */
	pr_vector(where.pr, 0, 3, 3, 0, PIX_SRC, 1);	/* nw corner */
	pr_vector(where.pr, w, 3, w - 3, 0, PIX_SRC, 1);	/* ne corner */

	pr_vector(where.pr, 6, h, w - 3, h, PIX_SRC, 1);	/* bottom */
	pr_vector(where.pr, 0, h - 6, 6, h, PIX_SRC, 1);	/* sw corner */
	pr_vector(where.pr, w - 3, h, w, h - 3, PIX_SRC, 1);	/* se corner */

	pr_vector(where.pr, 0, 3, 0, h - 6, PIX_SRC, 1);	/* left */
	pr_vector(where.pr, w, 3, w, h - 3, PIX_SRC, 1);	/* right */

	pr_vector(where.pr, w - 3, 3, w - 3, h - 6, PIX_SRC, 1);
		/* inside right */
	pr_vector(where.pr, 3, h - 3, w - 6, h - 3, PIX_SRC, 1);
		/* inside bottom */
	pr_vector(where.pr, w - 3, 3, w - 6, 0, PIX_SRC, 1);
		/* inside ne corner */
	pr_vector(where.pr, w - 6, h - 3, w - 3, h - 6, PIX_SRC, 1);
		/* inside se corner */

	/* fill in bottom shadow */
	for (x = 6; x < w - 3; x += 2)
		pr_put(where.pr, x, h - 2, 1);
	for (x = 7; x < w - 2; x += 2)
		pr_put(where.pr, x, h - 1, 1);
	pr_put(where.pr, w - 5, h - 3, 1);
	pr_put(where.pr, w - 4, h - 3, 1);
	pr_put(where.pr, w - 3, h - 2, 1);
	pr_put(where.pr, w - 3, h - 4, 1);

	/* fill in right shadow */
	for (y = 3; y < h - 2; y += 2)
		pr_put(where.pr, w - 2, y, 1);
	for (y = 4; y < h - 2; y += 2)
		pr_put(where.pr, w - 1, y, 1);
	pr_put(where.pr, w - 3, 1, 1);
	pr_put(where.pr, w - 2, 2, 1);

	return (where.pr);
#endif
}

#ifdef USE_IMAGES
static
fonthome(font)
Pixfont *font;
{
   return -(font->pf_char['n'].pc_home.y);
}
#endif
