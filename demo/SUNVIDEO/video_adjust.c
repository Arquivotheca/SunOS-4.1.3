
#ifndef lint
static char sccsid[] = "@(#)video_adjust.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif
#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <video.h>

static Window ioctl_frame, ioctl_panel;
extern Notify_value slider_fixer();
static int win_fd;

extern Window video_window;


Window
create_video_adjust_panel(base_frame)
    Window base_frame;
{
    extern luma_gain_notify(), chroma_gain_notify(),
           rgb_gain_notify(), rgb_black_level_notify();
    Panel_item tmp;

	
    ioctl_frame = window_create(base_frame, FRAME,
			       FRAME_SHOW_SHADOW, FALSE, 0);

    ioctl_panel = window_create(ioctl_frame, PANEL,
				WIN_CONSUME_PICK_EVENTS,
				   LOC_WINENTER,LOC_WINEXIT,
                                   LOC_RGNENTER,LOC_RGNEXIT,
				   0,
				PANEL_EVENT_PROC, slider_fixer,
				PANEL_BACKGROUND_PROC, slider_fixer,
				0);
    win_fd = win_get_fd(ioctl_panel);
    tmp = panel_create_item(ioctl_panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, -128,
		      PANEL_MAX_VALUE, 128,
		      PANEL_VALUE, 0,
		      PANEL_SLIDER_WIDTH, 256,
		      PANEL_LABEL_STRING, "Luma Gain        ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, luma_gain_notify, 0);
    register_item(tmp, "Luma_gain", PANEL_SLIDER);
    tmp = panel_create_item(ioctl_panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, -128,
		      PANEL_MAX_VALUE, 128,
		      PANEL_VALUE, 0,
		      PANEL_SLIDER_WIDTH, 256,
		      PANEL_LABEL_STRING, "Chroma Gain      ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, chroma_gain_notify, 0);
    register_item(tmp, "Chroma_gain", PANEL_SLIDER);
    tmp = panel_create_item(ioctl_panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, -128,
		      PANEL_MAX_VALUE, 128,
		      PANEL_VALUE, 0,
		      PANEL_SLIDER_WIDTH, 256,
		      PANEL_LABEL_STRING, "RGB Gain         ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, rgb_gain_notify, 0);
    register_item(tmp, "RGB_gain", PANEL_SLIDER);
    tmp = panel_create_item(ioctl_panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, -128,
		      PANEL_MAX_VALUE, 128,
		      PANEL_VALUE, 0,
		      PANEL_SLIDER_WIDTH, 256,
		      PANEL_LABEL_STRING, "RGB Black Level  ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, rgb_black_level_notify, 0);
    register_item(tmp, "RGB_black_level", PANEL_SLIDER);
    window_fit(ioctl_panel);
    window_fit(ioctl_frame);
    return(ioctl_frame);
}

luma_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    window_set(video_window, VIDEO_LUMA_GAIN, value, 0);
}

chroma_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    window_set(video_window, VIDEO_CHROMA_GAIN, value, 0);
}

rgb_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    window_set(video_window,
	       VIDEO_R_GAIN, value,
	       VIDEO_G_GAIN, value,
	       VIDEO_B_GAIN, value,
	       0);

}

rgb_black_level_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    window_set(video_window,
	       VIDEO_R_LEVEL, value,
	       VIDEO_G_LEVEL, value,
	       VIDEO_B_LEVEL, value,
	       0);
}
