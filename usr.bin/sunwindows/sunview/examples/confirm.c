/*********************************************************/
#ifndef lint
static char sccsid[] = "@(#)confirm.c 1.1 92/07/30 Copyr 1986 Sun Micro";
#endif
/*********************************************************/

#include <suntool/sunview.h>
#include <suntool/panel.h>

static Frame	init_confirmer();
static int	confirm();
static void	yes_no_ok();


int
confirm_yes(message)
char	*message;
{
    return confirm(message, FALSE);
}
 
int
confirm_ok(message)
char	*message;
{
    return confirm(message, TRUE);
}


static int
confirm(message, ok_only)
char	*message;
int	ok_only;
{
    Frame	confirmer;
    int		answer;

    /* create the confirmer */
    confirmer = init_confirmer(message, ok_only);

    /* make the user answer */
    answer = (int) window_loop(confirmer);

    /* destroy the confirmer */
    window_set(confirmer, FRAME_NO_CONFIRM, TRUE, 0);
    window_destroy(confirmer);

    return answer;
}


static Frame
init_confirmer(message, ok_only)
char	*message;
int	ok_only;
{
    Frame		confirmer;
    Panel		panel;
    Panel_item		message_item;
    int			left, top, width, height;
    Rect		*r;
    struct pixrect	*pr;

    confirmer = window_create(0, FRAME, FRAME_SHOW_LABEL, FALSE, 0);

    panel = window_create(confirmer, PANEL, 0);

    message_item = panel_create_item(panel, PANEL_MESSAGE, 
	PANEL_LABEL_STRING, message,
	0);

    if (ok_only) {
	pr = panel_button_image(panel, "OK", 3, 0);
	width = pr->pr_width;
    } else {
	pr = panel_button_image(panel, "NO", 3, 0);
	width = 2 * pr->pr_width + 10;
    }

    r = (Rect *) panel_get(message_item, PANEL_ITEM_RECT);
 
    /* center the yes/no or ok buttons under the message */
    left = (r->r_width - width) / 2;
    if (left < 0)
	left = 0;
    top = rect_bottom(r) + 5;

    if (ok_only) {
        panel_create_item(panel, PANEL_BUTTON, 
	    PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
	    PANEL_LABEL_IMAGE, pr,
	    PANEL_CLIENT_DATA, 1,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	0);
    } else {
        panel_create_item(panel, PANEL_BUTTON, 
	    PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
	    PANEL_LABEL_IMAGE, pr,
	    PANEL_CLIENT_DATA, 0,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	    0);

	panel_create_item(panel, PANEL_BUTTON, 
	    PANEL_LABEL_IMAGE, panel_button_image(panel, "YES", 3, 0),
	    PANEL_CLIENT_DATA, 1,
	    PANEL_NOTIFY_PROC, yes_no_ok,
	    0);
    }

    window_fit(panel);
    window_fit(confirmer);

    /* center the confirmer frame on the screen */

    r = (Rect *) window_get(confirmer, WIN_SCREEN_RECT);

    width = (int) window_get(confirmer, WIN_WIDTH);
    height = (int) window_get(confirmer, WIN_HEIGHT);

    left = (r->r_width - width) / 2;
    top = (r->r_height - height) / 2;

    if (left < 0)
	left = 0;
    if (top < 0)
	top = 0;

    window_set(confirmer, WIN_X, left, WIN_Y, top, 0);

    return confirmer;
}


/* yes/no/ok notify proc */
static void
yes_no_ok(item, event)
Panel_item	item;
Event		*event;
{
    window_return(panel_get(item, PANEL_CLIENT_DATA));
}
