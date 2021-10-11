
#ifndef lint
static char sccsid[] = "@(#)main.c 1.1 92/07/30 Copyr 1989 Sun Micro";
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <pixrect/pixrect.h>
#include <pixrect/memvar.h>

#include <sunwindow/notify.h>

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/canvas.h>
#include <video.h>

static short video_icon_image[256] =
{
#include "video_icon.h"
};
mpr_static(icon_pixrect, 64, 64, 1, video_icon_image);

Window video_frame,
       video_window,
       file_panel,
       video_adjust_panel,
       video_panel,
       video_config_panel,
       video_blanket,
       create_file_panel(),
       create_video_adjust_panel(),
       create_video_panel(),
       create_video_config_panel();

struct pixrect *my_video_pixrect;

Menu video_menu;

Icon video_icon;

extern Notify_value video_poll();

static char *default_list[]= {
    "Luma_gain=0",
    "Chroma_gain=0",
    "RGB_gain=0",
    "RGB_black_level=0",
    "Zoom=1/1",
    "Video_live=Live",
    "Output_select=On screen",
    "Save_as=32 bit Raster File",
    "Input_format=NTSC",
    "Sync_select=Input(NTSC)",
    "Component_out=RGB",
    "Chroma_demodulation=On",
    "Sync_lock=Internal",
    "De_interlace=Fast",
    "Epsf_format=New",
    (char *) 0
};

int y_offset;

main(argc, argv)
    int argc;
    char **argv;
{
    struct itimerval	poll_timer;
    create_windows(&argc, argv);
    my_video_pixrect = (struct pixrect *)window_get(video_window,
						     VIDEO_PIXRECT);
    y_offset = (int)window_get(video_window, VIDEO_Y_OFFSET);
    window_set(video_window, VIDEO_LIVE, 0, 0);
    make_cbars(my_video_pixrect); /* put colorbars in as default pattern */
    initialize_items_from_list(default_list);
    initialize_items_from_file(".video_config");
    poll_timer.it_interval.tv_usec = 250000;
    poll_timer.it_interval.tv_sec = 0;
    poll_timer.it_value.tv_usec = 250000;
    poll_timer.it_value.tv_sec = 0;
    notify_set_itimer_func((Notify_client)main, video_poll, ITIMER_REAL,
			    &poll_timer, 0);
    window_main_loop(video_frame);
}

create_windows(argcp, argv)
    int *argcp;
    char **argv;
{
    extern video_event_proc(),
	   video_frame_event_proc(),
	   file_panel_notify(),
	   video_panel_notify(),
	   video_adjust_panel_notify(),
	   video_config_notify(),
	   blanket_repaint(),
	   quit_notify();


    video_icon = icon_create(ICON_IMAGE, &icon_pixrect, 0);
    video_frame = window_create(NULL, FRAME,
                          FRAME_LABEL, "Video",
			  FRAME_ICON, video_icon,
			  WIN_EVENT_PROC, video_frame_event_proc,
			  FRAME_ARGC_PTR_ARGV, argcp, argv,
                          0);
    video_menu = menu_create(
			    MENU_ITEM,
			     MENU_STRING, "Video Control",
			     MENU_NOTIFY_PROC, video_panel_notify,
			     0,
			    MENU_ITEM,
			     MENU_STRING, "Image Load and Save",
			     MENU_NOTIFY_PROC, file_panel_notify,
			     0,
			    MENU_ITEM,
			     MENU_STRING, "Video Adjust",
			     MENU_NOTIFY_PROC, video_adjust_panel_notify,
			     0,
			    MENU_ITEM,
			     MENU_STRING, "Video Configuration",
			     MENU_NOTIFY_PROC, video_config_notify,
			     0,
			    MENU_ITEM,
			    MENU_STRING, "Quit",
			    MENU_NOTIFY_PROC, quit_notify,
			     0,
			    0);
    if(!video_frame) {
	fprintf(stderr, "Must be in windows to run\n");
	exit(1);
    }
    video_window = window_create(video_frame, VIDEO,
				 WIN_EVENT_PROC, video_event_proc,
				 0);
    if(!video_window) {
	fprintf(stderr, "Device does not support video windows\n");
	exit(1);
    }
   /* The blanket canvas is used when video is output off the screen */
    /* so that cursor and the display remains intact. */
    video_blanket = window_create(video_frame, CANVAS,
				 CANVAS_RETAINED, FALSE,
				 CANVAS_AUTO_EXPAND, TRUE,
				 CANVAS_AUTO_SHRINK, TRUE,
				 WIN_SHOW, FALSE,
				 WIN_EVENT_PROC, video_event_proc,
				 CANVAS_REPAINT_PROC, blanket_repaint,
				 0);
    window_fit(video_window);
    window_fit_width(video_frame);
    
    file_panel = create_file_panel(video_frame);
    video_adjust_panel = create_video_adjust_panel(video_frame);
    video_panel = create_video_panel(video_frame);
    video_config_panel = create_video_config_panel(video_frame);
    window_fit(video_window);
    window_fit(video_frame);    
}


video_frame_event_proc(window, event, arg)
    Window window;
    Event *event;
    caddr_t arg;
{
    int id;

    switch (id=event_id(event)) {
	case WIN_RESIZE:
	    window_default_event_proc(window, event, arg);
	    /* If we need to do anything else, do it here */
	    break;

	default:
	    window_default_event_proc(window, event, arg);
    }
}

video_event_proc(window, event, arg)
    Window window;
    Event *event;
    caddr_t arg;
{
    int id;

    switch (id=event_id(event)) {

	case MS_RIGHT:
	    /* Instead of passinig our window, pass the frame's */
	    /* so the menu might show */
	    menu_show(video_menu, video_frame, event, 0);
	    break;

	default:
	    window_default_event_proc(window, event, arg);
    }
}


file_panel_notify()
{
    /* Enable display of file panel */
    window_set(file_panel, WIN_SHOW, 1, 0);
}

video_panel_notify()
{
    /* Enable display of video control panel */
    window_set(video_panel, WIN_SHOW, 1, 0);
}

video_adjust_panel_notify()
{
    /* Enable display of video control panel */
    window_set(video_adjust_panel, WIN_SHOW, 1, 0);
}

video_config_notify()
{
    /* Enable display of video config panel */
    window_set(video_config_panel, WIN_SHOW, 1, 0);
}

quit_notify()
{
    exit(0);
}

blanket_repaint(canvas, pw, area)
    Canvas canvas;
    Pixwin *pw;
    Rectlist *area;
{
    static char message[]="Video being output off screen";

    Pixfont *font;
    int w, h;
    struct pr_subregion subregion;

    font = (Pixfont *)window_get(canvas, WIN_FONT);
    w = (int) window_get(canvas, WIN_WIDTH);
    h = (int) window_get(canvas, WIN_HEIGHT);
    pf_textbound(&subregion, strlen(message), font, message);
    pw_text(pw, (w - subregion.size.x)/2 - subregion.pos.x,
                (h -  subregion.size.y)/2 - subregion.pos.y,
		PIX_SRC, font, message);
}
