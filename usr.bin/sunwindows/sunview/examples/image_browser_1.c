/***************************************************************************/
/*                          image_browser_1.c                              */
/***************************************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/tty.h>
#include <stdio.h>
#include <suntool/icon_load.h>
#include <suntool/seln.h>

Frame frame;
Panel control_panel, display_panel;
Tty   tty;

Panel_item dir_item, fname_item, image_item;

ls_proc(), show_proc(), quit_proc();

char *get_selection();

#define MAX_PATH_LEN 1024
#define MAX_FILENAME_LEN 256 

main(argc, argv)
int argc;
char **argv;
{
    frame = window_create(NULL, FRAME,
		          FRAME_ARGS,  argc, argv, 
		          FRAME_LABEL, "image_browser_1",
		          0);
    init_tty();
    init_control_panel();
    init_display_panel();
    window_fit(frame);
    window_main_loop(frame);
    exit(0);
    /* NOTREACHED */
}

init_tty() 
{
    tty = window_create(frame, TTY, 
			WIN_COLUMNS, 30, 
			WIN_ROWS,    20, 
			0);
}


init_control_panel() 
{
    char *getwd();
    char current_dir[1024];

    control_panel = window_create(frame, PANEL, 0);

    dir_item = panel_create_item(control_panel, PANEL_TEXT,
        PANEL_VALUE_DISPLAY_LENGTH, 13,
        PANEL_LABEL_STRING,         "Dir: ",
	PANEL_VALUE,                getwd(current_dir),
        0);

    fname_item = panel_create_item(control_panel, PANEL_TEXT,
	PANEL_ITEM_X,               ATTR_COL(0),
	PANEL_ITEM_Y,               ATTR_ROW(1),
        PANEL_VALUE_DISPLAY_LENGTH, 13,
        PANEL_LABEL_STRING,         "File:",
        0);

    panel_create_item(control_panel, PANEL_BUTTON,
	PANEL_ITEM_X,       ATTR_COL(0),
	PANEL_ITEM_Y,       ATTR_ROW(2),
        PANEL_LABEL_IMAGE,  panel_button_image(control_panel,"List",0,0),
        PANEL_NOTIFY_PROC,  ls_proc,
        0);

    panel_create_item(control_panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE,  panel_button_image(control_panel,"Show",0,0),
        PANEL_NOTIFY_PROC,  show_proc,
        0);

    panel_create_item(control_panel, PANEL_BUTTON,
        PANEL_LABEL_IMAGE,  panel_button_image(control_panel,"Quit",0,0),
        PANEL_NOTIFY_PROC,  quit_proc,
        0);

    window_fit(control_panel);
}


ls_proc()
{
   static char previous_dir[MAX_PATH_LEN];
   char *current_dir;
   char cmdstring[100];

   current_dir = (char *)panel_get_value(dir_item);

   if (strcmp(current_dir, previous_dir)) {
       chdir(current_dir);
       sprintf(cmdstring, "cd %s\n", current_dir);
       ttysw_input(tty, cmdstring, strlen(cmdstring));
       strcpy(previous_dir, current_dir);
   }

   sprintf(cmdstring, "ls -1 %s\n", panel_get_value(fname_item));
   ttysw_input(tty, cmdstring, strlen(cmdstring));
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
    display_panel = window_create(frame, PANEL, 
			  WIN_BELOW,    control_panel,
			  WIN_RIGHT_OF, tty,
			  0);
    image_item = panel_create_item(display_panel, PANEL_MESSAGE, 0);
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
