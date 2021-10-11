/*	@(#)simple_panel.c 1.1 92/07/30 SMI	*/
#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/panel.h>
#include <suntool/icon.h>

static void quit_proc();
Frame frame;
Panel panel;
Pixfont *bold;
Icon icon;

static short    icon_image[] = {
#include <images/hello_world.icon>
};
mpr_static(hello_world, 64, 64, 1, icon_image);

main(argc, argv)
int argc; char **argv;
{
    bold = pf_open("/usr/lib/fonts/fixedwidthfonts/screen.b.12");
    if (bold == NULL) exit(1);

    icon = icon_create(ICON_IMAGE, &hello_world, 0);
    frame = window_create(NULL, FRAME,
                 FRAME_LABEL,             "hello_world_panel",
                 FRAME_ICON,              icon,
                 FRAME_ARGS,              argc, argv, 
                 WIN_ERROR_MSG,           "Can't create window.",
                 0);

    panel = window_create(frame, PANEL, WIN_FONT, bold, 0);

    panel_create_item(panel, PANEL_MESSAGE, 
        PANEL_LABEL_STRING, "Push button to quit.", 0);
    panel_create_item(panel, PANEL_BUTTON, 
        PANEL_LABEL_IMAGE,  panel_button_image(panel, "Good-bye", 0, 0), 
        PANEL_NOTIFY_PROC,  quit_proc, 
        0);

    window_fit(panel);
    window_fit(frame);
    window_main_loop(frame);
    exit(0);
    /* NOTREACHED */
}

static void
quit_proc()
{
   window_set(frame, FRAME_NO_CONFIRM, TRUE, 0);
   window_destroy(frame);
}
