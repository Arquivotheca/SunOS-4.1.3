
#ifndef lint
static char sccsid[] = "@(#)vidcal_panel.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>

#include <sun/tvio.h>

static Window frame, panel;
Notify_value slider_fixer();
static int win_fd;

static union tv1_nvram tv1_cal;
extern int tv1_fd;

Window
create_video_cal_panel(frame)
    Window frame;
{
    extern vid_x_notify(), vid_y_notify(),
           luma_gain_notify(), chroma_gain_notify(),
           red_gain_notify(), red_black_level_notify(),
	   green_gain_notify(), green_black_level_notify(),
	   blue_gain_notify(), blue_black_level_notify(),
	   store_cal_notify();

    panel = window_create(frame, PANEL,
				WIN_CONSUME_PICK_EVENTS,
				   LOC_WINENTER,LOC_WINEXIT,
                                   LOC_RGNENTER,LOC_RGNEXIT,
				   0,
				PANEL_EVENT_PROC, slider_fixer,
				PANEL_BACKGROUND_PROC, slider_fixer,
				0);
    win_fd = win_get_fd(panel);
    get_video_cal(&tv1_cal);
    tv1_cal.field.magic = 0xA9;

    panel_create_item(panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE, 
		      panel_button_image(panel, "Store", 5, 0),
		      PANEL_NOTIFY_PROC, store_cal_notify,
		      0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.red_black,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Red Black Level  ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, red_black_level_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.green_black,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Green Black Level",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, green_black_level_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.blue_black,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Blue Black Level ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, blue_black_level_notify, 0);
    

    
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.red_gain,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Red Gain         ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, red_gain_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.green_gain,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Green Gain       ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, green_gain_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.blue_gain,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Blue Gain        ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, blue_gain_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.luma_gain,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Luma Gain        ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, luma_gain_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.chroma_gain,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Chroma Gain      ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, chroma_gain_notify, 0);


    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 511,
		      PANEL_VALUE, tv1_cal.field.offset.x,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "X Offset         ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, vid_x_notify, 0);
    panel_create_item(panel, PANEL_SLIDER,
		      PANEL_MIN_VALUE, 0,
		      PANEL_MAX_VALUE, 255,
		      PANEL_VALUE, tv1_cal.field.offset.y,
		      PANEL_SLIDER_WIDTH, 512,
		      PANEL_LABEL_STRING, "Y Offset         ",
		      PANEL_NOTIFY_LEVEL, PANEL_ALL,
		      PANEL_NOTIFY_PROC, vid_y_notify, 0);
    window_fit(panel);
    return(frame);
}

vid_x_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.offset.x = value;
    set_video_cal(&tv1_cal);
}

vid_y_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.offset.y = value;
    set_video_cal(&tv1_cal);
}

luma_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.luma_gain = value;
    set_video_cal(&tv1_cal);
}

chroma_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.chroma_gain = value;
    set_video_cal(&tv1_cal);
}

red_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.red_gain = value;
    set_video_cal(&tv1_cal);
}

red_black_level_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.red_black = value;
    set_video_cal(&tv1_cal);
}

green_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.green_gain = value;
    set_video_cal(&tv1_cal);

}

green_black_level_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.green_black = value;
    set_video_cal(&tv1_cal);
}

blue_gain_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.blue_gain = value;
    set_video_cal(&tv1_cal);
}

blue_black_level_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    fix_slider(item, event, win_fd);
    tv1_cal.field.blue_black = value;
    set_video_cal(&tv1_cal);
}

store_cal_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    store_video_cal(&tv1_cal);
}

get_video_cal(cal)
    union tv1_nvram *cal;
{
    if (ioctl(tv1_fd, TVIOGVIDEOCAL, cal) == -1) {
	perror("TVIOGVIDEOCAL failed");
    }
}

set_video_cal(cal)
    union tv1_nvram *cal;
{
    if (ioctl(tv1_fd, TVIOSVIDEOCAL, cal) == -1) {
	perror("TVIOSVIDEOCAL failed");
    }
}

store_video_cal(cal)
    union tv1_nvram *cal;
{
    if (ioctl(tv1_fd, TVIONVWRITE, cal) == -1) {
	perror("TVIONVWRITE failed");
    }
}

