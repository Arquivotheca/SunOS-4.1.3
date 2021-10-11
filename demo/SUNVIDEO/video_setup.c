
#ifndef lint
static char sccsid[] = "@(#)video_setup.c 1.1 92/07/30 Copyr 1988 Sun Micro";
#endif

#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <video.h>
#include <sun/tvio.h>
#include <sys/param.h>

static Panel_item video_input_item, video_sync_item, video_gen_lock_item,
	          comp_out_item, filename_item, de_interlace_item,
		  epsf_format_item; 

extern Window video_window;

Window
create_video_config_panel(base_frame)
    Window base_frame;
{
    extern video_format_notify(), video_sync_notify(), 
	    video_comp_out_notify(),  video_chroma_sep_notify(),
	    video_gen_lock_notify(), video_chroma_demod_notify(),
	    de_interlace_notify(), epsf_format_notify(),
	    video_save_config_notify(), video_load_config_notify();

    Window setup_frame, setup_panel;
    Panel_item tmp, load_item;

    setup_frame = window_create(base_frame, FRAME,
			        FRAME_SHOW_SHADOW, FALSE,
				0);
    setup_panel = window_create(setup_frame, PANEL,
				PANEL_ITEM_X_GAP, 20,
				0);
    video_input_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "Input Format:",
		      PANEL_CHOICE_STRINGS,
		          "NTSC",
			  "YC",
			  "YUV",
			  "RGB",
			  0,
		      PANEL_NOTIFY_PROC, video_format_notify,
		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      0);
    register_item(video_input_item, "Input_format",PANEL_CHOICE);
    video_sync_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "Sync Select:",
		      PANEL_CHOICE_STRINGS,
		          "Input(NTSC)",
			  "NTSC", 
			  "YC",
			  "YUV",
			  "RGB",
			  0,
		      PANEL_NOTIFY_PROC, video_sync_notify,
		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      PANEL_ITEM_X, item_right_of_x(video_input_item),
		      PANEL_ITEM_Y, item_right_of_y(video_input_item),
		      0);
    register_item(video_sync_item, "Sync_select", PANEL_CHOICE);

    comp_out_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "Component out:",
		      PANEL_CHOICE_STRINGS,
			  "YUV",
			  "RGB",
			  0,
		      PANEL_NOTIFY_PROC, video_comp_out_notify,
		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      PANEL_ITEM_X, item_below_x(video_input_item),
		      PANEL_ITEM_Y, item_below_y(video_input_item),
		      0);
    register_item(comp_out_item, "Component_out",PANEL_CHOICE);


    tmp = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "Chroma Demodulation:",
		      PANEL_CHOICE_STRINGS,
			"Auto",
			"On",
			"Off",
			0,
		      PANEL_VALUE, 0,
		      PANEL_NOTIFY_PROC, video_chroma_demod_notify,
		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      PANEL_ITEM_X, item_right_of_x(comp_out_item),
		      PANEL_ITEM_Y, item_right_of_y(comp_out_item),
		      0);
    register_item(tmp, "Chroma_demodulation", PANEL_CHOICE);

    video_gen_lock_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "Sync lock:",
		      PANEL_CHOICE_STRINGS,
		          "Internal",
			  "Gen Lock",
			  0,
		      PANEL_NOTIFY_PROC, video_gen_lock_notify,
		      PANEL_DISPLAY_LEVEL, PANEL_CURRENT,
		      PANEL_ITEM_X, item_below_x(comp_out_item),
		      PANEL_ITEM_Y, item_below_y(comp_out_item),
		      0);
    register_item(video_gen_lock_item, "Sync_lock", PANEL_CHOICE);

    de_interlace_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "De-interlacing:",
		      PANEL_CHOICE_STRINGS,
		          "Fast",
			  "Slow",
			  0,
		       PANEL_NOTIFY_PROC, de_interlace_notify,
		       PANEL_ITEM_X, item_right_of_x(video_gen_lock_item),
		       PANEL_ITEM_Y, item_right_of_y(video_gen_lock_item),
		       0);
    register_item(de_interlace_item, "De_interlace", PANEL_CHOICE);

    epsf_format_item = panel_create_item(setup_panel, PANEL_CHOICE,
		      PANEL_LABEL_STRING, "EPSF Standard:",
		      PANEL_CHOICE_STRINGS,
		          "New",
			  "Old",
			  0,
		       PANEL_NOTIFY_PROC, epsf_format_notify,
		       PANEL_ITEM_X, item_below_x(video_gen_lock_item),
		       PANEL_ITEM_Y, item_below_y(video_gen_lock_item),
		       0);
    register_item(epsf_format_item, "Epsf_format", PANEL_CHOICE);

    load_item = panel_create_item(setup_panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE,
			panel_button_image(setup_panel, "Load Configuration", 5, 0),
                      PANEL_NOTIFY_PROC, video_load_config_notify,
		      PANEL_ITEM_X, item_below_x(epsf_format_item),
		      PANEL_ITEM_Y, item_below_y(epsf_format_item),
		      0);
    panel_create_item(setup_panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE,
			panel_button_image(setup_panel, "Save Configuration", 5, 0),
                      PANEL_NOTIFY_PROC, video_save_config_notify,
		      PANEL_ITEM_X, item_right_of_x(load_item),
		      PANEL_ITEM_Y, item_right_of_y(load_item),
		      0);
    filename_item = panel_create_item(setup_panel, PANEL_TEXT,
		      PANEL_LABEL_STRING, "Configuration file:",
		      PANEL_VALUE_STORED_LENGTH, MAXPATHLEN,
		      PANEL_VALUE_DISPLAY_LENGTH, 16,
		      PANEL_VALUE, ".video_config",
		      PANEL_ITEM_X, item_below_x(load_item),
		      PANEL_ITEM_Y, item_below_y(load_item),
		      0);
    window_fit(setup_panel);
    window_fit(setup_frame);
    return(setup_frame);

}


static int sync_select, input_select;

video_format_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    char *s;

    window_set(video_window, VIDEO_INPUT, value, 0);
    if ((int)panel_get(video_sync_item, PANEL_VALUE) == 0) {
    	window_set(video_window, VIDEO_SYNC, value, 0);
    }
    switch (value) {
	case 0: s = "Input(NTSC)";break;
	case 1: s = "Input(YC)  ";break;
	case 2: s = "Input(YUV) ";break;
	case 3: s = "Input(RGB) ";break;
    }
    panel_set(video_sync_item, PANEL_CHOICE_STRING, 0, s, 0);
}


video_sync_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    if (value == 0) {
	/* same as input */
	window_set(video_window, VIDEO_SYNC, 
		   panel_get(video_input_item, PANEL_VALUE), 0);
    } else {
	window_set(video_window, VIDEO_SYNC, (value - 1), 0);
    }

}

video_comp_out_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    switch (value) {
	case 0: value = TVIO_YUV; break;
	case 1:	value = TVIO_RGB; break;
    }
    window_set(video_window, VIDEO_COMP_OUT, value, 0);
}

video_chroma_sep_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    window_set(video_window, VIDEO_CHROMA_SEPARATION, value, 0);
}

video_gen_lock_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    window_set(video_window, VIDEO_GENLOCK, value, 0);
}

video_chroma_demod_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    window_set(video_window, VIDEO_DEMOD, value, 0);
}

video_save_config_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    write_items_to_file(panel_get_value(filename_item));

}

video_load_config_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    initialize_items_from_file(panel_get_value(filename_item));
}

de_interlace_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    extern int interlace_method;
    interlace_method = value;
}

epsf_format_notify(item, value, event)
    Panel_item      item;
    int             value;
    Event          *event;
{
    extern int epsf_format;
    epsf_format = value;
}
