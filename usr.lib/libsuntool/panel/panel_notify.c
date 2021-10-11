#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_notify.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                           panel_notify.c                                  */
/*               Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*****************************************************************************/

#include <suntool/panel_impl.h>
#include <suntool/window.h>

static	Notify_value	panel_itimer_expired();
static	void		panel_itimer_set();

/*****************************************************************************/
/* panel_destroy                                                             */
/* frees panel's storage, unregisters panel with notifier, and tells         */
/* notifier to destroy any scrollbars.                                       */
/*****************************************************************************/

Notify_value
panel_destroy(panel, status)
panel_handle	panel;
Destroy_status	status;
{
   Scrollbar sb;

   if (status == DESTROY_CHECKING)
      return NOTIFY_IGNORED;

   panel_itimer_set(panel, NOTIFY_NO_ITIMER.it_value);
   if (status == DESTROY_CLEANUP) {
      if (sb = window_get((Window)(LINT_CAST(panel)), WIN_VERTICAL_SCROLLBAR))
	 (void)notify_post_destroy(sb, status, NOTIFY_IMMEDIATE);
      if (sb = window_get((Window)(LINT_CAST(panel)), WIN_HORIZONTAL_SCROLLBAR))
	 (void)notify_post_destroy(sb, status, NOTIFY_IMMEDIATE);
   }
   (void)win_unregister((char *)(LINT_CAST(panel)));
   (void)panel_free((Panel) panel);
   return NOTIFY_DONE;
}

/*****************************************************************************/
/* panel_notify_event                                                        */
/*****************************************************************************/

/* ARGSUSED */
Notify_value
panel_notify_event(panel, event, arg, type)
register panel_handle	panel;
register Event		*event;
Notify_arg		arg;
Notify_event_type 	type;
{
   switch (event_action(event)) {

      case WIN_REPAINT:
	 panel_display(panel, PANEL_CLEAR);
	 /* show the initial caret */
	 panel_caret_on(panel, TRUE);
	 return NOTIFY_DONE;

      case WIN_RESIZE:
	 panel_resize(panel);
	 return NOTIFY_DONE;

      case KBD_USE:
         (void)tool_kbd_use(panel->tool, (caddr_t)(LINT_CAST(panel)));
	 if (timing(panel))
	    panel_itimer_set(panel, panel->timer_full);
	 panel_caret_on(panel, FALSE);
	 panel->caret_pr = &panel_caret_pr;
	 panel_caret_on(panel, TRUE);
	 break;

      case KBD_DONE:
         (void)tool_kbd_done(panel->tool, (caddr_t)(LINT_CAST(panel)));
         panel_caret_on(panel, FALSE);
	 panel_itimer_set(panel, NOTIFY_NO_ITIMER.it_value);
	 panel->caret_pr = &panel_ghost_caret_pr;
         panel_caret_on(panel, TRUE);
	 break;

      default:
	 break;
   }
   return panel_use_event(panel, event, (Scrollbar)arg);
}

/*****************************************************************************/
/* timer functions                                                           */
/*****************************************************************************/

/* ARGSUSED */
static Notify_value
panel_itimer_expired(panel, which)
register panel_handle	panel;
int			which;
{
   if (blinking(panel))
      panel_caret_invert(panel);

   /* notify the client about the timer */
   if (panel->timer_proc)
      (*panel->timer_proc)(panel);

   panel_itimer_set(panel, panel->timer_full);
}

static void
panel_itimer_set(panel, value)
panel_handle	panel;
struct timeval	value;
{
   struct itimerval	itimer;

   itimer = NOTIFY_NO_ITIMER;
   itimer.it_value = value;
   (void)notify_set_itimer_func((Notify_client)(LINT_CAST(panel)), 
   	panel_itimer_expired, ITIMER_REAL, &itimer, (struct itimerval *)0);
}
