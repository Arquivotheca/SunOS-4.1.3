/*	@(#)addnewtest.c 1.1 92/07/30 SMI	*/
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <stdio.h>

toggle_proc(), dotoggle_proc();

Frame frame;
Panel panel;

main(argc, argv)
int argc;
char **argv;
{
    frame = window_create(NULL, FRAME,
		 FRAME_LABEL,             "create a panelitem",
		 FRAME_ARGS,              argc, argv, 
		 WIN_ERROR_MSG,           "Can't create window",
		 0);
    panel = window_create(frame, PANEL, 0);
    panel_create_item(panel, PANEL_MESSAGE, 
	PANEL_LABEL_STRING, "Hit button to do toggle.",
        0);
    panel_create_item(panel, PANEL_BUTTON, 
        PANEL_LABEL_IMAGE, panel_button_image(panel, "Toggle", 0, 0), 
        PANEL_NOTIFY_PROC,  dotoggle_proc,
        0);
    window_main_loop(frame);
    exit(0);
    /* NOTREACHED */
}

dotoggle_proc()
{
    panel_create_item(panel, PANEL_TOGGLE, 
        PANEL_LABEL_STRING, "Format Options",
	PANEL_CHOICE_STRINGS, "Long", "Reverse", "Show all files", 0,
	PANEL_TOGGLE_VALUE,  1, TRUE,
        PANEL_NOTIFY_PROC,  toggle_proc,
        0);
	
}

toggle_proc(item, value, event)
	Panel_item	item;
	unsigned int	value;
	Event		*event;
{
	printf("Toggle value is %d\n", value);
}
