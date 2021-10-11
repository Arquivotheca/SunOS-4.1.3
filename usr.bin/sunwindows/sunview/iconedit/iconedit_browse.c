#ifndef lint
#ifdef sccs
static char  sccsid[] = "@(#)iconedit_browse.c 1.1 92/07/30";
#endif
#endif

/**************************************************************************/
/*                            iconedit_browse.c                           */
/*             Copyright (c) 1986 by Sun Microsystems Inc.                */
/**************************************************************************/

#include "iconedit.h"
#include <suntool/scrollbar.h>
#include <sys/stat.h>
#include <suntool/expand_name.h>
#include <suntool/icon_load.h>
#include <suntool/fullscreen.h>

static int browser_items_notify_proc();
static int browser_items_event_proc();
static int browser_done_proc();

static Menu image_menu;
static Menu_item image_mi;

static Cursor iconedit_frame_cursor;

static char msgbuf[30];
static char namebuf[100];
static int files_count, images_count;
static struct namelist *name_list;
#define get_name(i) name_list->names[(i)]

#define MAX_ITEMS 200 
static Frame browser_frame;
static Panel browser;
static int browser_created;
static Scrollbar sb;

static char *strsave();

int	iced_browser_filled;

extern Panel_item iced_dir_item, iced_load_item;

char 	*malloc(), *strcpy(), *sprintf();
/**************************************************************************/
/* iced_init_browser                                                           */
/**************************************************************************/
void
iced_init_browser()
{

   browser_frame = window_create(iced_base_frame, FRAME,
	 	WIN_ERROR_MSG, 		"Unable to create browser frame\n",
		FRAME_SHOW_LABEL, 	FALSE,
		FRAME_DONE_PROC,  	browser_done_proc,
		WIN_SHOW,         	FALSE,
		0);
   if (browser_frame == NULL) {
		(void)fprintf(stderr,"Unable to create browser frame\n");
		exit(1);
   }

   iconedit_frame_cursor = (Cursor) window_get(iced_base_frame, WIN_CURSOR, 0);

   sb = scrollbar_create(SCROLL_MARGIN, 4, 0),
   browser = window_create(browser_frame, PANEL, 
	   	WIN_VERTICAL_SCROLLBAR, sb, 
	   	WIN_ROW_HEIGHT,         61,
	 	WIN_COLUMN_WIDTH,       64,
	   	WIN_ROW_GAP,            10,
	   	WIN_COLUMN_GAP,         10,
               	WIN_LEFT_MARGIN,        10,
	   	WIN_RIGHT_MARGIN,       0,
  	   	WIN_TOP_MARGIN,         10,
	   	WIN_BOTTOM_MARGIN,      0,
	   	WIN_ROWS,               4,
	   	WIN_COLUMNS,            4,
                WIN_ERROR_MSG, 		"Unable to create browser panel\n",
	   	PANEL_ITEM_X_GAP,       10,
	   	PANEL_ITEM_Y_GAP,       10,
	   	0);
   if (browser == NULL) { 
                (void)fprintf(stderr,"Unable to create browser panel\n"); 
                exit(1); 
   }
   (void)window_fit(browser_frame);

   image_menu = menu_create(0);
   image_mi   = menu_create_item(MENU_VALUE, 1, 0);
   menu_set(image_menu, MENU_APPEND_ITEM, image_mi, 0);
   
   browser_created = TRUE;
}

/**************************************************************************/
/* iced_browse_proc - browse button's notify proc                              */
/**************************************************************************/

/* ARGSUSED */
void
iced_browse_proc(item)
Panel_item item;
{
   if (!iced_browser_filled) {
      iced_panelmsg("","Matching files...");
      window_set(iced_base_frame, FRAME_LABEL, "Matching files...", 0);
      iced_set_cursors_hourglass();
      if (!match_files()) {
         iced_set_cursors_normal();
         window_set(iced_base_frame, FRAME_LABEL, "iconedit", 0);
         return;
      }
      if (files_count)
	 fill_browser();
      else
	free_namelist(name_list);
      iced_set_cursors_normal();
      window_set(iced_base_frame, FRAME_LABEL, "iconedit", 0);
   }
   display_browsing_msg();
}

static
display_browsing_msg()
{
   if (images_count) {
      if (images_count == 1)
         (void)sprintf(msgbuf, "1 image to browse.");
      else 
         (void)sprintf(msgbuf, "%d images to browse.", images_count);
      iced_panelmsg(msgbuf, "(Select image to load)");
      show_browser_frame();
   } else {
      if (files_count == 1) {
         iced_panelmsg("The 1 file found", "doesn't contain an image.");
      } else {
         (void)sprintf(msgbuf, "%d files found,", files_count);
	 if (!files_count)
            iced_panelmsg("", "No files found.");
	 else {
            (void)sprintf(msgbuf, "%d files found,", files_count);
            iced_panelmsg(msgbuf, "none contained images.");
	 }
      }
   }
}

/**************************************************************************/
/* show_browser_frame - make sure that the whole frame is on the screen   */
/**************************************************************************/

show_browser_frame()
{
   Rect base_r, browser_r;
   int browser_top, browser_left;
   register int x, y;
 
   if (window_get(browser_frame, WIN_SHOW)) return;
 
   base_r    = *(Rect *) (LINT_CAST(window_get(iced_base_frame, WIN_RECT)));
   browser_r = *(Rect *) (LINT_CAST(window_get(browser_frame, WIN_RECT)));

   /* adjust if not zoomed */
   if (browser_r.r_top > 0) {
      browser_top = base_r.r_top + 143;
      for (y=0; browser_top + browser_r.r_height - y > 900; y++);
      (void)window_set(browser_frame, WIN_Y, 143-y, 0);
   }

   /* adjust if not fullscreen */
   if (browser_r.r_left > 0) {
      browser_left= base_r.r_left + 438;
      for (x=0; browser_left + browser_r.r_width - x > 1152; x++);
      (void)window_set(browser_frame, WIN_X, 438-x, 0);
   }
   (void)window_set(browser_frame, WIN_SHOW, TRUE, 0);
}

/**************************************************************************/
/* browser_done_proc                                                      */
/**************************************************************************/

static int
browser_done_proc()
{
   iced_panelmsg("", "");
   (void)window_set(browser_frame, WIN_SHOW, FALSE, 0);
}

/**************************************************************************/
/* browser_items_event_proc                                               */
/**************************************************************************/

static int
browser_items_event_proc(item, event)
Panel_item item;
Event *event;
{
   if (event_id(event) == MS_RIGHT) {
      Event *adjusted_event;
      menu_set(image_mi, MENU_STRING, panel_get(item, PANEL_CLIENT_DATA), 0);
      adjusted_event = panel_window_event(browser, event);
      if (menu_show(image_menu, browser, adjusted_event, 0)) {
	 browser_items_notify_proc(item);
	 return;
      }
   }
   (void)panel_default_handle_event(item, event);
}

/**************************************************************************/
/* browser_items_notify_proc                                              */
/**************************************************************************/

static int
browser_items_notify_proc(item)
Panel_item item;
{
   char *fname;

   fname = panel_get(item, PANEL_CLIENT_DATA);
   (void)panel_set_value(iced_fname_item, fname);
   iced_load_proc(iced_load_item);
   (void)window_set(browser_frame, WIN_SHOW, FALSE, 0);
}

/**************************************************************************/
/* fill_browser                                                           */
/**************************************************************************/

static
fill_browser()
{
   if (files_count == 1) {
      iced_panelmsg("1 file found,", "loading image for browsing...");
      window_set(iced_base_frame, FRAME_LABEL,
	"loading image for browsing...", 0);
   } else {
      (void)sprintf(msgbuf, "%d files found, loading", files_count);
      iced_panelmsg(msgbuf, "images for browsing...");
      window_set(iced_base_frame, FRAME_LABEL,
	"loading images for browsing...", 0);
   }
   do_fill_browser();
   if (images_count > 1) {
      scrollbar_scroll_to(sb, 0L);
      (void)panel_paint(browser, PANEL_CLEAR);
      show_browser_frame();
   } else
      (void)window_set(browser_frame, WIN_SHOW, FALSE, 0);
}

static
do_fill_browser()
{
   Panel_item   old_item;
   register int i;
   Pixrect     *image;
   int          previous_images_count;
   char         error_msg[IL_ERRORMSG_SIZE];

   panel_each_item(browser, old_item)
       (void)pr_destroy((Pixrect *)(LINT_CAST(panel_get(old_item, PANEL_LABEL_IMAGE))));
       (void)panel_free(old_item);
   panel_end_each

   previous_images_count = images_count;
   for (i = images_count = 0; i < files_count; i++) {
      if (image = icon_load_mpr(get_name(i), error_msg)) {
	 (void)panel_create_item(browser, PANEL_BUTTON,
		PANEL_LABEL_IMAGE, image,
		PANEL_CLIENT_DATA, strsave(get_name(i)),
		PANEL_NOTIFY_PROC, browser_items_notify_proc,
		PANEL_EVENT_PROC,  browser_items_event_proc,
		0);
	 images_count++;
      }
   }
   if (images_count <= previous_images_count)
      (void)panel_update_scrolling_size(browser);

   free_namelist(name_list);
   if (images_count)
      iced_browser_filled = TRUE;
}

static
match_files()
{
   char *val;

   val = (char *)panel_get_value(iced_fname_item);
   if (strlen(val))
      (void)strcpy(namebuf, val);
   else {
      namebuf[0] = '*';
      namebuf[1] = '\0';
      (void)panel_set_value(iced_fname_item, "*");
   }
   return do_match_files();
}

static
do_match_files()
{
   if (!iced_change_directory()) {
      iced_panelmsg("Unable to", "change to directory.");
      return (FALSE);
   }
   name_list    = expand_name(namebuf);
   files_count  = name_list->count;
   return (TRUE);
}

/**************************************************************************/
/* iced_complete_filename                                                      */
/**************************************************************************/

iced_complete_filename()
{
   char *val;
   int   len;

   val = (char *)panel_get_value(iced_fname_item);
   len = strlen(val);
   (void)strcpy(namebuf,val);
   namebuf[len++] = '*';
   namebuf[len++] = '\0';
   iced_panelmsg("","Matching files...");

   window_set(iced_base_frame, FRAME_LABEL, "Matching files...", 0);
   iced_set_cursors_hourglass();
   if (do_match_files()) {
      switch (files_count) {
	 case 0:  iced_panelmsg("", "0 files found.");
	          free_namelist(name_list);
		  break;
	 case 1:  (void)panel_set(iced_fname_item, PANEL_VALUE, get_name(0), 0);
		  iced_panelmsg("","File name expanded.");
	          free_namelist(name_list);
		  break;
	 default: fill_browser();
		  (void)panel_set_value(iced_fname_item, namebuf);
		  display_browsing_msg();
		  break;
      }
   }
   window_set(iced_base_frame, FRAME_LABEL, "iconedit", 0);
   iced_set_cursors_normal();
}

/**************************************************************************/
/* set_cursors                                                            */
/**************************************************************************/

iced_set_cursors_hourglass()
{
   (void)window_set(iced_base_frame,  WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(iced_mouse_panel, WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(iced_msg_panel,   WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(iced_canvas,      WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(iced_panel,       WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(iced_proof,       WIN_CURSOR, &iconedit_hourglass_cursor, 0);
   (void)window_set(browser,          WIN_CURSOR, &iconedit_hourglass_cursor, 0);
}

iced_set_cursors_normal()
{
   (void)window_set(iced_base_frame,  WIN_CURSOR, iconedit_frame_cursor, 0);
   (void)window_set(iced_mouse_panel, WIN_CURSOR, &iconedit_main_cursor, 0);
   (void)window_set(iced_msg_panel,   WIN_CURSOR, &iconedit_main_cursor, 0);
   (void)window_set(iced_canvas,      WIN_CURSOR, &iconedit_main_cursor, 0);
   (void)window_set(iced_panel,       WIN_CURSOR, &iconedit_main_cursor, 0);
   (void)window_set(iced_proof,       WIN_CURSOR, &iconedit_main_cursor, 0);
   (void)window_set(browser,          WIN_CURSOR, &iconedit_main_cursor, 0);
}

/**************************************************************************/
/* strsave                                                                */
/**************************************************************************/

static char *
strsave(source)
char *source;
{
   char *dest;

   dest = (char *) malloc((unsigned)(strlen(source) + 1));
   if (!dest)
      return NULL;

   (void)strcpy(dest, source);
   return dest;
}

