#ifndef lint
static  char sccsid[] = "@(#)fv.main.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

/*	Copyright (c) 1987, 1988, Sun Microsystems, Inc.  All Rights Reserved.
	Sun considers its source code as an unpublished, proprietary
	trade secret, and it is available only under strict license
	provisions.  This copyright notice is placed here only to protect
	Sun in the event the source is deemed a published work.  Dissassembly,
	decompilation, or other means of reducing the object code to human
	readable form is prohibited by the license agreement under which
	this code is provided to the user or company in possession of this
	copy.

	RESTRICTED RIGHTS LEGEND: Use, duplication, or disclosure by the 
	Government is subject to restrictions as set forth in subparagraph 
	(c)(1)(ii) of the Rights in Technical Data and Computer Software 
	clause at DFARS 52.227-7013 and in similar clauses in the FAR and 
	NASA FAR Supplement. */

#include <stdio.h>
#include <fcntl.h>

#ifdef SV1
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <suntool/scrollbar.h>
#else
#include <view2/view2.h>
#include <view2/panel.h>
#include <view2/canvas.h>
#include <view2/scrollbar.h>
#include <view2/icon.h>
#include <view2/win_screen.h>
#endif

#include "fv.port.h"
#include "fv.h"
#include "fv.extern.h"

static short I_image[] = {
#include "fv.icon"
};

#ifdef SV1
DEFINE_ICON_FROM_IMAGE(fv_icon, I_image);
#else
mpr_static_static(Icon_pr, ICON_DEFAULT_WIDTH, ICON_DEFAULT_HEIGHT, 1, I_image);
#endif

#define LEFT_MARGIN		5
#define RIGHT_MARGIN		5
#define SUBWINDOW_SPACING	5

main(argc, argv)
	int argc;
	char **argv;
{
	Menu fmenu;			/* Frame menu */
	Menu_item mi;			/* Menu item */
	char *s_p;
	int len;
	extern int fv_preference();
	Notify_value frame_event();
	Notify_value fv_catch_signal();
	static Notify_value quit();
	void fv_create_folder();
	void fv_init_comm();
	int fv_preference();
#ifdef PROTO
	extern char sundesk_path[];

	if ((s_p = getenv("SUNDESK")) && *s_p)
		(void)strcpy(sundesk_path, s_p);
	else
		(void)strcpy(sundesk_path, "/usr/sundesk");
#endif

#ifndef SV1
	vu_init(VU_INIT_ARGC_PTR_ARGV, &argc, argv, 0);
#endif

	/* Store value of $HOME; we use it often enough! */

	if ((s_p = getenv("HOME")) && (len = strlen(s_p)))
	{
		Fv_home = malloc((unsigned)len+1);
		(void)strcpy(Fv_home, s_p);
	}
	read_rc_file();
	fv_startseln();

	Fv_frame = window_create(0,
#ifdef SV1
		FRAME,
		FRAME_ARGS, argc, argv,
		FRAME_ICON, &fv_icon,
# ifdef PROTO
		FRAME_CLASS, FRAME_CLASS_BASE,
		FRAME_PROPS_PROC, fv_preference,
# endif
#else
		FRAME_BASE,
		FRAME_ICON, vu_create(VU_NULL, ICON, ICON_IMAGE, &Icon_pr, 0),
		FRAME_SHOW_FOOTER, TRUE,
		FRAME_PROPERTIES_PROC, fv_preference,
		FRAME_PROPERTIES_ACTIVE, TRUE,
		WIN_DYNAMIC_COLOR, TRUE,
#endif
		FRAME_NO_CONFIRM, TRUE,
		WIN_COLUMNS, FV_WIDTH_COLS,
		WIN_CONSUME_PICK_EVENTS,
			KEY_LEFT(K_PROPS), KEY_LEFT(K_UNDO), KEY_LEFT(K_PUT),
			KEY_LEFT(K_GET), KEY_LEFT(K_FIND), KEY_LEFT(K_DELETE), 0,
		0);


#ifdef SV1
#ifndef PROTO
	/* Activate props on frame menu */

	fmenu = (Menu)window_get(Fv_frame, WIN_MENU);
	mi = menu_get(fmenu, MENU_NTH_ITEM, 6);
	menu_set(mi, MENU_INACTIVE, FALSE, 0);
	menu_set(mi, MENU_NOTIFY_PROC, fv_preference, 0);
#endif
#endif
	/* All the initializations... */

	Fv_panel = window_create(Fv_frame, PANEL, 
#ifdef PROTO
		PANEL_IS_CONTROL_AREA, TRUE,
#endif
		0);
	fv_create_panel(Fv_panel);
	fv_create_tree_canvas();
	fv_create_folder();

	get_screen_dimensions();
	fv_init_font();
	fv_file_scroll_height();
	fv_init_tree();
	fv_read_bind();
	fv_initcursor();
	fv_init_comm();

	notify_interpose_event_func(Fv_frame, frame_event, NOTIFY_SAFE);
	notify_interpose_destroy_func(Fv_frame, quit);

	notify_set_signal_func(Fv_frame, fv_catch_signal, SIGUSR1, NOTIFY_SYNC);
	notify_set_signal_func(Fv_frame, fv_catch_signal, SIGUSR2, NOTIFY_SYNC);

	fv_auto_update(Fv_update_interval);
	Fv_automount = fv_init_automount();

	Fv_running = TRUE;
	fv_display_folder(FV_BUILD_FOLDER);
	fv_namestripe();
	window_main_loop(Fv_frame);
	Fv_running = FALSE;
	fv_stopseln();
	(void)fv_commit_delete(TRUE);
	write_rc_file();
	exit(0);
}


static Notify_value
quit(frame, status)
	Frame frame;
	Destroy_status status;
{
	if (status == DESTROY_CHECKING)
	{
		fv_quit_frames();
		(void)unlink("/tmp/.fv_dev");
	}

	return(notify_next_destroy_func(frame, status));
}


static Notify_value
frame_event(frame, evnt, arg, type)	/* Catch resize and key events */
	Frame frame;
	Event *evnt;
	Notify_arg arg;
	Notify_event_type type;
{
	Notify_value value;

	if (event_id(evnt) == WIN_RESIZE)
	{
		/* Reconfigure the subwindows after the window system does
		 * the default action.
		 */
		value = notify_next_event_func(frame, evnt, arg, type);
		fv_resize(FALSE);
		return(value);
	}

	fv_check_keys(evnt);

	return(notify_next_event_func(frame, evnt, arg, type));
}


fv_resize(switch_view)			/* Resize subwindows */
	BOOLEAN switch_view;		/* Change Path or Tree View? */
{
	Rect *r;
	int h, height, vheight, width, panel_height, ypos;
	int sheight;			/* Name stripe height */
	int subwindow_spacing;

#ifdef SV1
	Fv_margin = (int)window_get(Fv_frame, WIN_LEFT_MARGIN);
#else
	Fv_margin = 7;		/*KLUDGE*/
#endif
#ifdef PROTO
	/* Proto returns incorrect spacing, frame header height */

	subwindow_spacing = Fv_margin;
	sheight = PROTO_TOP_MARGIN;
#else
	subwindow_spacing = SUBWINDOW_SPACING;
	sheight = (int)window_get(Fv_frame, WIN_TOP_MARGIN);
#endif


	r = (Rect *)window_get(Fv_frame, WIN_RECT);

	height = r->r_height;
	width = r->r_width - Fv_margin*2;
#ifndef SV1
	width += Fv_margin;	/* KLUDGE */
#endif
	Fv_winwidth = width;

	/* Control panel */

#ifdef PROTO
	panel_height = 30;		/* Panels are too tall */
#else
	panel_height = (int)window_get(Fv_panel, WIN_HEIGHT);
#endif
	window_set(Fv_panel,
		WIN_X, 0,
		WIN_Y, 0,
		WIN_HEIGHT, panel_height,
		WIN_WIDTH, width,
		0);
	ypos = panel_height+subwindow_spacing;

	/* Tree canvas: calculate variable height */

	vheight = height - sheight - panel_height - (subwindow_spacing*2)
#ifdef PROTO
			- sheight+6;	/* Shorter footer needed */
#else
			- (int)window_get(Fv_frame, WIN_BOTTOM_MARGIN);
#endif

	if (Fv_treeview)
	{
		/* Tree canvas gets existing percentage of variable 
		 * height.  If we've switched from view to tree,
		 * however, it gets 50%.
		 */

		if (switch_view)
			h = vheight/2;
		else
			h = (float)vheight*
			      ((float)Fv_tree.r.r_height
			       /(float)(Fv_winheight-subwindow_spacing-ypos));
	}
	else
		h = TREE_GLYPH_HEIGHT + MARGIN*2; /* Folder icons are smaller */
#ifndef SV1
	h += MARGIN;
	if (!Fv_treeview && Fv_tree.vsbar)
		h += 16, width += 16;
#endif

	Fv_winheight = ypos+vheight+subwindow_spacing;

	vheight -= (h+subwindow_spacing);
	window_set(Fv_tree.canvas,
		WIN_X, 0,
		WIN_Y, ypos,
		WIN_WIDTH, width,
		WIN_HEIGHT, h,
		0);
#ifndef SV1
	if (!Fv_treeview && Fv_tree.vsbar)
		width -= 16, h -= 16;
#endif

	/* Calculate & adjust widths */

	Fv_tree.r.r_width = (int)window_get(Fv_tree.canvas, WIN_WIDTH);
	Fv_tree.r.r_height = (int)window_get(Fv_tree.canvas, WIN_HEIGHT);

	ypos += h+subwindow_spacing;

	/* Folder canvas gets remainder of variable height */

	window_set(Fv_foldr.canvas, 
		WIN_X, 0,
		WIN_Y, ypos,
		WIN_WIDTH, width,
		WIN_HEIGHT, vheight,
		0);
	Fv_foldr.height = vheight;
	ypos += vheight+subwindow_spacing;
}


fv_check_keys(evnt)		/* PROPS, UNDO, PUT, GET, FIND, & DELETE keys */
	Event *evnt;
{
	void fv_copy_to_shelf();

	switch (event_id(evnt))
	{
	case KEY_LEFT(K_PROPS):		/* Properties of tool/object */

		if (event_is_up(evnt))
			if (fv_getselectedfile() != EMPTY || Fv_tselected)
				fv_info_object();
			else
				fv_preference();
		break;

	case KEY_LEFT(K_UNDO):		/* Undo last command */
		if (event_is_up(evnt))
			fv_undelete();
		break;

	case KEY_LEFT(K_PUT):		/* Clipboard cut: put on shelf*/
		if (event_is_up(evnt))
			fv_copy_to_shelf();
		break;

	case KEY_LEFT(K_GET):		/* Clipboard paste: get off shelf */
		if (event_is_up(evnt))
			fv_paste();
		break;

	case KEY_LEFT(K_FIND):		/* Find object */

		if (event_is_up(evnt))
			fv_find_object();
		break;

	case KEY_LEFT(K_DELETE):	/* Delete object */

		if (event_is_up(evnt))
			fv_delete_object();
		break;
	}
}


static
get_screen_dimensions()
{
#ifdef SV1
	int fd;			/* Framebuffer fd */
	struct screen s;	/* Framebuffer structure */
	char *s_p;		/* Frame environment var */

	if ((s_p = getenv("WINDOW_PARENT")) && *s_p)
		if ((fd = open(s_p, O_RDONLY)) != -1)
		{
			win_screenget(fd, &s);
			Fv_screenwidth = s.scr_rect.r_width;
			Fv_screenheight = s.scr_rect.r_height;
			(void)close(fd);
			return;
		}
#endif
	Fv_screenwidth = 1169;
	Fv_screenheight = 979;
}


static
write_rc_file()			/* Write permanent defaults file */
{
	FILE *fp;
	char buf[255];
	BYTE unused = 'x';	/* Old option */

	(void)sprintf(buf, "%s/.fileviewrc", Fv_home);
	if ((fp = fopen(buf, "w")) == NULL)
	{
		if (Fv_home[1])
			fv_putmsg(TRUE, "Can't open %s for writing", (int)buf, 0);
		return;
	}
	if (fprintf(fp, "%hd %c %hd %c %hd %hd %d %c\n%s\n%s\n",
		Fv_editor, unused, Fv_wantdu, Fv_confirm_delete,
		Fv_update_interval, Fv_sortby, Fv_style, Fv_see_dot,
		Fv_print_script, Fv_filter) == EOF)
	{
		fv_putmsg(TRUE, Fv_message[MECREAT], (int)buf, 0);
		(void)unlink(buf);
	}
	(void)fclose(fp);
}


static
read_rc_file()			/* Read permanent defaults file */
{
	FILE *fp;
	char buf[255];
	BYTE unused;

	(void)sprintf(buf, "%s/.fileviewrc", Fv_home);
	if ((fp = fopen(buf, "r")) == NULL)
	{
		set_defaults();
		return;
	}

	if (fscanf(fp, "%hd %c %hd %c %hd %hd %d %c\n",
		&Fv_editor, &unused, &Fv_wantdu, &Fv_confirm_delete,
		&Fv_update_interval, &Fv_sortby, &Fv_style, &Fv_see_dot) != 8 ||
	    fgets(Fv_print_script, 80, fp) == NULL ||
	    fgets(Fv_filter, 80, fp) == NULL)
	{
		fv_putmsg(TRUE, "Illegal/obsolete syntax in %s", (int)buf, 0);
		set_defaults();
	}
	else
	{
		/* Added later; don't test for ommission */
		fgets(Fv_other_editor, 80, fp);

		clean_up(Fv_print_script);
		clean_up(Fv_filter);
		clean_up(Fv_other_editor);
	}
		
	(void)fclose(fp);
}


static
clean_up(b_p)
	char *b_p;
{
	char *n_p;
	if (b_p[0] == ' ')
		b_p[0] = NULL;
	else
	{	/* Get rid of newline */
		if (n_p = strchr(b_p, '\n'))
			*n_p = NULL;
	}
}


static
set_defaults()
{
	/* Nothing or obsolete; set defaults... */

	Fv_editor = FV_TEXTEDIT;
	Fv_wantdu = FALSE;
	Fv_confirm_delete = TRUE;
	Fv_update_interval = 10;
	Fv_sortby = FV_SORTALPHA;
	Fv_style = FV_DICON;
	Fv_see_dot = FALSE;
	(void)strcpy(Fv_print_script, "lpr %s 2>/tmp/.err");
	Fv_filter[0] = 0;
	Fv_other_editor[0] = 0;
}
