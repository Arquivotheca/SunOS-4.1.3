
#ifndef lint
static char sccsid[] = "@(#)file_panel.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/expand_name.h>
#include <suntool/alert.h>

#include <sys/types.h>
#include <dirent.h>
#include <sys/param.h>
#include <fcntl.h>


char file_directory[MAXPATHLEN];
char file_name[MAXPATHLEN]="image";

load_file_notify(), save_file_notify(), directory_notify_proc(),
filename_notify_proc(), ras_verify(), save_postscript();

extern struct pixrect *my_video_pixrect;


static Window
    file_frame,
    file_panel;

static Panel_item
    pathname_item,
    file_item, load_item, save_as;

#define ESC '\033'

Window
create_file_panel(base_frame)
    Window base_frame;
{
    extern char *getwd();
    extern int file_panel_event_proc();

    Panel_item save_item;
    file_frame = window_create(base_frame, FRAME,
			       WIN_EVENT_PROC, file_panel_event_proc,
			       FRAME_SHOW_SHADOW, FALSE, 0);
    file_panel = window_create(file_frame, PANEL,
			       0);
    getwd(file_directory);
    pathname_item = panel_create_item(file_panel, PANEL_TEXT,
		      PANEL_LABEL_STRING, "Directory:",
		      PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		      PANEL_VALUE_DISPLAY_LENGTH, 32,
		      PANEL_NOTIFY_PROC, directory_notify_proc,
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_VALUE, file_directory,
		      0);
    file_item = panel_create_item(file_panel, PANEL_TEXT,
		      PANEL_LABEL_STRING, "File:",
		      PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		      PANEL_VALUE_DISPLAY_LENGTH, 32,
		      PANEL_NOTIFY_PROC, filename_notify_proc,
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_VALUE, file_name,
		      0);
    save_as = panel_create_item(file_panel, PANEL_CHOICE,
    		PANEL_LABEL_STRING, "Save as :",
		PANEL_CHOICE_STRINGS,
			"32 bit Raster File",
    			"8 bit Color Raster File",
    			"8 bit Grayscale Raster File",
     			"1 bit Dithered Raster File",
    			"PostScript File",
    			"Encapsulated PostScript File",
    			0,
    		PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
    		PANEL_VALUE, 0,
    		0);
    register_item(save_as, "Save_as", PANEL_CHOICE);
    put_panel_item_below(file_item, save_as);
    load_item = panel_create_item(file_panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE,
		        panel_button_image(file_panel, "Load", 0, 0),
		      PANEL_NOTIFY_PROC, load_file_notify,
		      0);
    put_panel_item_below(save_as, load_item);
    save_item = panel_create_item(file_panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE,
		        panel_button_image(file_panel, "Save", 0, 0),
		      PANEL_NOTIFY_PROC, save_file_notify,
		      0);
    put_panel_item_to_right(load_item, save_item);
    window_fit(file_panel);
    window_fit(file_frame);
    return(file_frame);
}

load_file_notify(item, event)
    Panel_item item;
    Event *event;
{
    extern Panel_item video_freeze_item;

    char filename[MAXPATHLEN+2], *path, *file;
    path = panel_get_value(pathname_item);
    file = panel_get_value(file_item);
    
    strcpy(filename, path);
    strcat(filename, "/");
    strcat(filename, file);
    
    video_live_notify(video_freeze_item, 0, event);
    panel_set(video_freeze_item, PANEL_VALUE, 0, 0);
    set_busy_cursor(file_panel);
    load_video_image(my_video_pixrect, filename);
    set_regular_cursor(file_panel);
}

save_file_notify(item, event)
    Panel_item item;
    Event *event;
{
    char filename[MAXPATHLEN+2], *path, *file;
    int live, value;

    path = panel_get_value(pathname_item);
    file = panel_get_value(file_item);
    
    strcpy(filename, path);
    strcat(filename, "/");
    strcat(filename, file);
    /* Get live/Frozen status */
    live = (int)panel_get(video_freeze_item, PANEL_VALUE);
    /* Freeze the frame */
    video_live_notify(video_freeze_item, 0, event);
    set_busy_cursor(file_panel);

    value = (int)panel_get(save_as,PANEL_VALUE);
    switch(value) {
    case 0 :	/* 32 bit raster file */
    	save_video_image(my_video_pixrect, filename, value);
    	break;
    case 1 :	/* 8 bit raster file, colour */
    	save_video_image(my_video_pixrect, filename, value);
    	break;
    case 2 :	/* 8 bit raster file, greyscale */
    	save_video_image(my_video_pixrect, filename, value);
    	break;
    case 3 :	/* 1 bit raster file, ordered dither */
    	save_video_image(my_video_pixrect, filename, value);
    	break;
    case 4 :	/* Postscript File */
    	save_postscript(my_video_pixrect, filename, 0);
    	break;
    case 5 :	/* Encapsulated Postscript File */
    	save_postscript(my_video_pixrect, filename, 1);
    	break;
    }

    set_regular_cursor(file_panel);
    /* Set Live back to previous state */
    video_live_notify(video_freeze_item, live, event);
}


directory_notify_proc(item, event)
    Panel_item item;
    Event *event;
{
    extern char *fcomplete();
    extern int dir_verify();

    switch(event_action(event)) {
	case ESC:
	    set_busy_cursor(file_panel);
	    panel_set(item,	    
		      PANEL_VALUE,
		      fcomplete(panel_get(item, PANEL_VALUE), dir_verify),
		      0);
	    set_regular_cursor(file_panel);
	    return(PANEL_NONE);

	default:
	    return(panel_text_notify(item, event));
    }
}

filename_notify_proc(item, event)
    Panel_item item;
    Event *event;
{
    extern char *fcomplete();
    extern int ras_verify();

    char filename[MAXPATHLEN+2], *path, *file;
    path = panel_get_value(pathname_item);
    file = panel_get_value(file_item);
    
    strcpy(filename, path);
    strcat(filename, "/");
    strcat(filename, file);
    switch(event_action(event)) {
	case ESC:
	    set_busy_cursor(file_panel);
	    panel_set(item,	    
		      PANEL_VALUE,
		      fcomplete(filename, ras_verify),
		      0);
	    set_regular_cursor(file_panel);
	    return(PANEL_NONE);

	default:
	    return(panel_text_notify(item, event));
    }
}

char *
fcomplete(name, verifier)
    char *name;
    int (*verifier)();
{
    struct namelist *namelist;
    static char buf[MAXPATHLEN+2];
    int found = -1;
    int i;


    strncpy(buf, name, MAXPATHLEN);
    strcat(buf, "*");
    if (namelist = expand_name(buf)) {
	for (i = 0; i < namelist->count; i++) {
	    if (verifier(namelist->names[i])) {
		if (found != -1) {
		    return(name);
		} else {
		    found = i;
		}
	    }
	}
	if (found != -1) {
	    strncpy(buf, namelist->names[found], 511);
	    free_namelist(namelist);
	    return(buf);
	}
    }
    return(name);
}

/* Verify the the specified path name is a directory */
int
dir_verify(name)
    char *name;
{
    DIR *dirp;
    extern DIR *opendir();

    if ((dirp = opendir(name)) == NULL) {
	return(0);
    } else {
	closedir(dirp);
	return(1);
    }
}

/* Verify the the specified path name is a rasterfile */
ras_verify(name)
    char *name;
{
    int fd;
    int result = 0;
    struct rasterfile rh;
    
    if ((fd = open(name, O_RDONLY, 0)) == -1) {
	return(0); /* No rasterfile here */
    }
    if (read(fd, &rh, sizeof(rh)) ==  sizeof(rh)) {
	if (rh.ras_magic == RAS_MAGIC) {
	    result = 1;
	}
    }
    close(fd);
    return(result);
}


confirm_overwrite()
{
    int result;

    result = alert_prompt(file_panel, (Event *)NULL,
			  ALERT_MESSAGE_STRINGS,
			    "File already exists.",
			    "Confirm Overwrite ?",
			    0,
			  ALERT_BUTTON_YES,
			    "Confirm, overwrite existing file",
			  ALERT_BUTTON_NO,
			    "Cancel",
			  0);
    switch (result) {
	case ALERT_YES:
	    return(1);
	case ALERT_NO:
	default:
	    return(0);
    }
}


file_error(message)
{
    int result;

    result = alert_prompt(file_panel, (Event *)NULL,
			  ALERT_MESSAGE_STRINGS,
			    message,
			    0,
			  ALERT_BUTTON_YES,
			    "Continue",
			  0);
    return;
}

int
file_panel_event_proc(window, event, arg)
    Window window;
    Event *event;
    caddr_t arg;
{
    int id;
    
    window_default_event_proc(window, event, arg);
    switch (id=event_id(event)) {
	case WIN_RESIZE:
	    span_items(load_item, 2);
	    break;

	default:
	    break;
    }
}
