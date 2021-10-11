/***************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)image_browser_2.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/***************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <stdio.h>
#include <suntool/icon_load.h>
#include <suntool/seln.h>
#include <suntool/expand_name.h>
#include <suntool/scrollbar.h>

static char namebuf[100];
static int file_count, image_count;
static struct namelist *name_list;
#define get_name(i) name_list->names[(i)]

Frame frame;
Panel control_panel, display_panel;
Tty   tty;

Panel_item dir_item, fname_item, image_item;

show_proc(), browse_proc(), quit_proc();

Pixrect *get_image();

char *get_selection();

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256 

main(argc, argv)
int argc;
char **argv;
{
    frame = window_create(NULL, FRAME,
		          FRAME_ARGS,  argc, argv, 
		          FRAME_LABEL, "image_browser_2",
		          0);
    init_control_panel();
    init_display_panel();
    window_set(control_panel, 
	       WIN_WIDTH, window_get(display_panel, WIN_WIDTH, 0),
	       0);
    window_fit(frame);
    window_main_loop(frame);
    exit(0);
	/* NOTREACHED */
}

init_control_panel() 
{
    char current_dir[MAX_PATH_LEN];

    control_panel = window_create(frame, PANEL, 0);

    dir_item = panel_create_item(control_panel, PANEL_TEXT,
        PANEL_LABEL_X,              ATTR_COL(0),
        PANEL_LABEL_Y,              ATTR_ROW(0),
        PANEL_VALUE_DISPLAY_LENGTH, 23,
        PANEL_VALUE,                getwd(current_dir),
        PANEL_LABEL_STRING,         "Dir: ",
        0);

    (void) panel_create_item(control_panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(control_panel,"Browse",0,0),
        PANEL_NOTIFY_PROC, browse_proc,
        0);

    fname_item = panel_create_item(control_panel, PANEL_TEXT,
        PANEL_LABEL_X,              ATTR_COL(0),
        PANEL_LABEL_Y,              ATTR_ROW(1),
        PANEL_VALUE_DISPLAY_LENGTH, 23,
        PANEL_LABEL_STRING,         "File:",
        0);

    (void) panel_create_item(control_panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE, panel_button_image(control_panel,"Quit",6,0),
        PANEL_NOTIFY_PROC, quit_proc,
        0);

    window_fit_height(control_panel);

    window_set(control_panel, PANEL_CARET_ITEM, fname_item, 0);
}

browse_proc()
{
   Panel_item   old_item;
   register int i;
   int          len;
   Pixrect     *image;
   int          previous_image_count;
   register int row, col;

   set_directory();
   match_files();

   panel_each_item(display_panel, old_item)
       pr_destroy ((Pixrect *)panel_get(old_item, PANEL_LABEL_IMAGE));
       panel_free(old_item);
   panel_end_each

   previous_image_count = image_count;
   for (row = 0, image_count = 0; image_count < file_count; row++) {
       for (col = 0; col < 4 && image_count < file_count; col++) {
         if (image = get_image(image_count)) {
	    panel_create_item(display_panel, PANEL_MESSAGE, 
			      PANEL_ITEM_Y,      ATTR_ROW(row),
			      PANEL_ITEM_X,      ATTR_COL(col),
			      PANEL_LABEL_IMAGE, image, 0);
	    image_count++;
	  }
          else if (file_count == 1) break;
       }
       if (image_count == 0 && file_count == 1) break;
   }

   if (image_count <= previous_image_count)
      panel_update_scrolling_size(display_panel);
 
   panel_paint(display_panel, PANEL_CLEAR);

   free_namelist(name_list);
}

set_directory()
{
   static char previous_dir[MAX_PATH_LEN];
   char *current_dir;

   current_dir = (char *)panel_get_value(dir_item);

   if (strcmp(current_dir, previous_dir)) {
       chdir(current_dir);
       strcpy(previous_dir, current_dir);
   }
}

Pixrect *
get_image(i)
int i;
{
   char error_msg[IL_ERRORMSG_SIZE];
   return (icon_load_mpr(get_name(i), error_msg));
}

match_files()
{
   char *val;

   val = (char *)panel_get_value(fname_item);
   strcpy(namebuf, val);
   name_list  = expand_name(namebuf);
   file_count = name_list->count;
}

quit_proc()
{
   window_destroy(frame);
}

show_proc()
{
    char *filename;

    if (!strlen(filename = get_selection()))
        return;

    load_image(filename);
}

load_image(filename)
char *filename;
{
    Pixrect *image;
    char error_msg[IL_ERRORMSG_SIZE];

    if (image = icon_load_mpr(filename, error_msg)) {
	panel_set(image_item, 
		  PANEL_ITEM_X,      ATTR_COL(5),
		  PANEL_ITEM_Y,      ATTR_ROW(4), 
		  PANEL_LABEL_IMAGE, image, 
		  0);
    }
}

init_display_panel() 
{
  int width;
  Scrollbar sb = scrollbar_create(SCROLL_MARGIN,10,0);
  width = (int)scrollbar_get(sb, SCROLL_THICKNESS, 0);
  display_panel = window_create(frame, PANEL, 
		  WIN_BELOW,              control_panel,
		  WIN_X,                  0,
		  WIN_VERTICAL_SCROLLBAR, sb,
		  WIN_ROW_HEIGHT,         64,
		  WIN_COLUMN_WIDTH,       64,
		  WIN_ROW_GAP,            10,
		  WIN_COLUMN_GAP,         10,
		  WIN_LEFT_MARGIN,        width + 10,
		  WIN_TOP_MARGIN,         10,
		  WIN_ROWS,               4,
		  WIN_COLUMNS,            4,
		  0);
   window_set(display_panel, WIN_LEFT_MARGIN, 10, 0);
}

char *
get_selection()
{
    static char   filename[MAX_FILENAME_LEN];
    Seln_holder   holder;
    Seln_request  *buffer;

    holder = seln_inquire(SELN_PRIMARY);
    buffer = seln_ask(&holder, SELN_REQ_CONTENTS_ASCII, 0, 0);
    strncpy(filename, buffer->data + sizeof(Seln_attribute), MAX_FILENAME_LEN);
    return (filename);
}
