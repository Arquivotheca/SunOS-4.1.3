#ifndef lint
static	char sccsid[] = "@(#)cf.c 1.1 92/07/30 Copyright Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */
#include "sundiag.h"
#include <sys/time.h>
#include <sunwindow/pixwin.h>
#include <suntool/alert.h>

static Frame		msg_popup=NULL;

/*  confirmer package routines(forward references) */
extern	int      confirm();

static Frame    init_confirmer();
static void     yes_no_ok();

/******************************************************************************
 * Confirmer with "no" and "yes" buttons.				      *
 ******************************************************************************/
int	confirm_yes(message)
char    *message;
{
        return confirm(message, FALSE);
}
 
/******************************************************************************
 * Bring up the confirmer, and wait for user's response.		      *
 * Return: TRUE if ALERT_BUTTON_YES; FALSE otherwise.			      *
 ******************************************************************************/
int	confirm(message, ok_only)
char    *message;
int     ok_only;
{
  int	result;

	if (tty_mode)		/* running in TTY mode */
	{
	  tty_message(message);
	  return(TRUE);
	}

	if (ok_only)	/* just acknownage it */
	  result = alert_prompt(sundiag_frame, (Event *)NULL,
		ALERT_MESSAGE_STRINGS, message, 0,
		ALERT_BUTTON_YES,	"OK",
		0);
	else
	  result = alert_prompt(sundiag_frame, (Event *)NULL,
		ALERT_MESSAGE_STRINGS, message, 0,
		ALERT_BUTTON_YES,	"Yes",
		ALERT_BUTTON_NO,	"No",
		0);

	if (result == ALERT_YES)
	  return(TRUE);
	else
	  return(FALSE);
}

/******************************************************************************
 * "Done" button notify procedure for sundiag's message popup.		      *
 ******************************************************************************/
static	int	*done_flag;
static	void	done_proc()
{
        (void)window_set(msg_popup, FRAME_NO_CONFIRM, TRUE, 0);
        (void)window_destroy(msg_popup);
	*done_flag = TRUE;
	msg_popup = NULL;
}

/******************************************************************************
 * Brings up the popup with passed message in it(non-blocking mode).	      *
 * Input: msg, message to be displayed.					      *
 *	  waiting, pointer to a integer which is used as a flag to indicate   *
 *		   whether user has responded or not(== TRUE, if yes. And,    *
 *		   will be set by done_proc()).				      *
 * Output: TRUE, if successful; FALSE, if failed.			      *
 ******************************************************************************/
int	popup_info(msg, waiting)
char	*msg;
int	*waiting;
{
        Panel           panel;
        Panel_item      message_item;
        int             left, top, width, height;
        Rect            *r;
        struct pixrect  *pr;

	if (msg_popup != NULL)
	  return(FALSE);  /* return error(FALSE) if popup is already used */

        msg_popup = window_create(sundiag_frame, FRAME,
			FRAME_DONE_PROC, done_proc,
			0);
        panel = window_create(msg_popup, PANEL, 0);
        message_item = panel_create_item(panel, PANEL_MESSAGE,
                PANEL_LABEL_STRING, msg, 0);

        pr = panel_button_image(panel, "Done", 4, (Pixfont *)NULL);
        width = pr->pr_width;

        /* center the "Done" buttons under the message */
        r = (Rect *) panel_get(message_item, PANEL_ITEM_RECT);
        left = (r->r_width - width) / 2;
        if (left < 0)
          left = 0;
        top = rect_bottom(r) + 5;

        (void)panel_create_item(panel, PANEL_BUTTON,
                        PANEL_ITEM_X, left, PANEL_ITEM_Y, top,
                        PANEL_LABEL_IMAGE, pr,
                        PANEL_NOTIFY_PROC, done_proc,
        0);

        window_fit(panel);
        window_fit(msg_popup);

        /* center the confirmer frame on the screen */
        r = (Rect *) window_get(msg_popup, WIN_SCREEN_RECT);
        width = (int) window_get(msg_popup, WIN_WIDTH);
        height = (int) window_get(msg_popup, WIN_HEIGHT);
        left = (r->r_width - width) / 2;
        top = (r->r_height - height) / 2;
        if (left < 0)
                left = 0;
        if (top < 0)
                top = 0;

	done_flag = waiting;
        (void)window_set(msg_popup, WIN_X, left, WIN_Y, top, 0);
	(void)window_set(msg_popup, WIN_SHOW, TRUE, 0);	/* non-blocking popup */

	return(TRUE);
}
