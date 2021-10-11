/*****************************************************************************/
#ifndef lint
static char sccsid[] = "@(#)tty_io.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*****************************************************************************/

#include <stdio.h>
#include <suntool/sunview.h>
#include <suntool/tty.h>
#include <suntool/panel.h>

#define TEXT_ITEM_MAX_LENGTH 25

Tty     	tty;
Panel_item      text_item;
char            tmp_buf[80];

static void     input_text();
static void     output_text();
static void     output_time();

main(argc, argv)
    int             argc;
    char          **argv;
{
    Frame           frame;
    Panel           panel;

    frame = window_create(NULL, FRAME,
			  FRAME_ARGS,		argc, argv,
			  WIN_ERROR_MSG,	"Can't create tool frame",
			  0);
    panel = window_create(frame, PANEL, 0);

    /* set up a simple panel subwindow */
    panel_create_item(panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE, panel_button_image(panel, "Input text", 11, 0),
		      PANEL_NOTIFY_PROC, input_text,
		      0);
    panel_create_item(panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE, panel_button_image(panel, "Output text", 11, 0),
		      PANEL_NOTIFY_PROC, output_text,
		      0);
    text_item = panel_create_item(panel, PANEL_TEXT,
				  PANEL_LABEL_STRING, "Text:",
				  PANEL_VALUE, "Hello hello",
			          PANEL_VALUE_DISPLAY_LENGTH, TEXT_ITEM_MAX_LENGTH,
				  0);
    panel_create_item(panel, PANEL_BUTTON,
		      PANEL_LABEL_IMAGE, panel_button_image(panel, "Show time", 11, 0),
		      PANEL_NOTIFY_PROC, output_time,
		      0);

    window_fit_height(panel);

    /* Assume rest of arguments are for tty subwindow, except FRAME_ARGS leaves the
     * program_name as argv[0], and we don't want to pass this to the tty subwindow.
     */
    argv++;
    tty = window_create(frame, TTY,
			TTY_ARGV,	argv,
			WIN_ROWS,	24,
			WIN_COLUMNS,	80,
			0);

    window_fit(frame);

    ttysw_input(tty, "echo my pseudo-tty is `tty`\n", 28);

    window_main_loop(frame);
    exit(0);
	/* NOTREACHED */
}

static void
input_text(item, event)
    Panel_item      item;
    Event          *event;
{
    strcpy(tmp_buf, (char *) panel_get_value(text_item));
    ttysw_input(tty, tmp_buf, strlen(tmp_buf));
}

static void
output_text(item, event)
    Panel_item      item;
    Event          *event;
{
    strcpy(tmp_buf, (char *) panel_get_value(text_item));
    ttysw_output(tty, tmp_buf, strlen(tmp_buf));
}

static void
output_time(item, event)
    Panel_item      item;
    Event          *event;
{
#include <sys/time.h>
#define ASCTIMELEN	26

    struct timeval  tp;

    /* construct escape sequence to set frame label */
    tmp_buf[0] = '\033';
    tmp_buf[1] = ']';
    tmp_buf[2] = 'l';
    tmp_buf[2 + ASCTIMELEN + 1] = '\033';
    tmp_buf[2 + ASCTIMELEN + 2] = '\\';
    gettimeofday(&tp, NULL);
    strncpy(&tmp_buf[3], ctime(&tp.tv_sec), ASCTIMELEN);
    ttysw_output(tty, tmp_buf, ASCTIMELEN + 5);
}
