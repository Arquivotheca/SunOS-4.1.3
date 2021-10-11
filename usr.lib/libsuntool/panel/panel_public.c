#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_public.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                           panel_public.c                                  */
/*               Copyright (c) 1986 by Sun Microsystems, Inc.                */
/*****************************************************************************/

#include <varargs.h>
#include <suntool/panel_impl.h>
#include <suntool/window.h>
#include "sunwindow/sv_malloc.h"

static int     shrink_to_fit();
static void    set_size();


#define MAX_NEGATIVE_SHRINK 2000
#define SHRINK_MARGIN       4 


/*****************************************************************************/
/* panel_get() and panel_set()                                               */
/* they call the routines for either panels or items, as appropriate.        */
/*****************************************************************************/

/* VARARGS2 */
Panel_attribute_value
panel_get(client_object, attr, arg1)
Panel		client_object;
Panel_attribute attr;
caddr_t 	arg1;
{
   panel_handle    object = PANEL_CAST(client_object);
   if (!object)
      return (caddr_t) NULL;

   return (*object->ops->get_attr)(object, attr, arg1);
}


/* VARARGS1 */
panel_set(client_object, va_alist)
Panel	client_object;
va_dcl
{
   panel_handle    object = PANEL_CAST(client_object);
   caddr_t	avlist[ATTR_STANDARD_SIZE];
   va_list	valist;

   if (!object)
      return NULL;

   va_start(valist);
   (void) attr_make(avlist, ATTR_STANDARD_SIZE, valist);
   va_end(valist);

   if (is_item(object))
      /* this is a hack to capture pre & post item setting
       * actions for all the items.
       */
      set_item((panel_item_handle) object, avlist);
   else
      (*object->ops->set_attr)(object, avlist);
   return 1;
}

/*****************************************************************************/
/* routine to set item attributes                                            */
/*****************************************************************************/

static
set_item(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   short		caret_was_on	= FALSE;
   Panel_setting	saved_repaint	= ip->repaint;

   if (ip->panel->caret_on) {
      caret_was_on = TRUE;
      panel_caret_on(ip->panel, FALSE);
   }

   /* set any generic attributes */
   (void) panel_set_generic(ip, avlist);

   /* set item specific attributes */
   (*ip->ops->set_attr)(ip, avlist);

   /* set the menu attributes */
   (void) panel_set_menu(ip, avlist);

   /* set the ops vector attributes */
   (void)panel_set_ops(ip, avlist);

   (void)panel_paint_item(ip, ip->repaint);
   
   /* restore the item's original repaint
    * behavior.  This allows clients to
    * specify a 'one time' repaint policy
    * for this call to panel_set() only.
    */
   ip->repaint = saved_repaint;

   /* if current item is caret item, turn the caret on regardless */
   if (caret_was_on || ip == ip->panel->caret)
      panel_caret_on(ip->panel, TRUE);
}

/*****************************************************************************/
/* routines to get and set attributes which apply to panel as a whole        */
/*****************************************************************************/

panel_set_panel(panel, avlist)
register panel_handle	panel;
register Attr_avlist	avlist;
{
   register Panel_attribute attr;
   int                      width, height;
   Scrollbar                new_v_scrollbar      = 0;
   Scrollbar                new_h_scrollbar      = 0;
   int                      have_new_v_scrollbar = FALSE;
   int                      have_new_h_scrollbar = FALSE;
   panel_ops_handle	    new_ops;

   while(attr = (Panel_attribute) *avlist++) {
      switch (attr) {

	 case PANEL_WIDTH:
	    width = (int) *avlist++;
	    if (width >= PANEL_FIT_ITEMS - MAX_NEGATIVE_SHRINK)
	       width = shrink_to_fit(panel, PANEL_WIDTH, 
				     width - PANEL_FIT_ITEMS + SHRINK_MARGIN);
	    set_size(panel, PANEL_WIDTH, width);
	    break;

	 case PANEL_HEIGHT:
	    height = (int) *avlist++;
	    if (height >= PANEL_FIT_ITEMS - MAX_NEGATIVE_SHRINK)
	       height = shrink_to_fit(panel, PANEL_HEIGHT, 
			              height - PANEL_FIT_ITEMS + SHRINK_MARGIN);
	    set_size(panel, PANEL_HEIGHT, height);
	    break;

	 case PANEL_CLIENT_DATA:
	    panel->client_data = *avlist++;
	    break;

	 case HELP_DATA:
	    panel->help_data = *avlist++;
	    break;

	 case WIN_FONT:
	 case PANEL_FONT:
	    panel->font = (Pixfont *) LINT_CAST(*avlist++);
	    break;

	 case PANEL_ADJUSTABLE:
	    if ((int) *avlist++)
               panel->flags |= ADJUSTABLE;
	    else
	       panel->flags &= ~ADJUSTABLE;
	    break;

	 case PANEL_BLINK_CARET:
	    if ((int) *avlist++)
               panel->status |= BLINKING;
	    else
	       panel->status &= ~BLINKING;
	    panel_caret_on(panel,TRUE);
	    break;

	 case PANEL_CARET_ITEM:
	    panel_caret_on(panel,FALSE);
	    panel->caret = (panel_item_handle) LINT_CAST(*avlist++);
	    panel_caret_on(panel,TRUE);
	    break;

	 case PANEL_TIMER_PROC:
	    panel->timer_proc = (int (*)()) LINT_CAST(*avlist++);
	    break;

	 case PANEL_EVENT_PROC:
            panel->event_proc = (int (*)()) LINT_CAST(*avlist++);
	    if (!panel->event_proc)
	       panel->event_proc = panel_nullproc;
	    break;

	 case PANEL_BACKGROUND_PROC:
	    if (!ops_set(panel)) {
	       new_ops = (panel_ops_handle) 
		   LINT_CAST(sv_malloc(sizeof(struct panel_ops)));
	       *new_ops = *panel->ops;
	       panel->ops = new_ops;
	       panel->flags |= OPS_SET;
	    }
            panel->ops->handle_event = (int (*)()) LINT_CAST(*avlist++);
	    if (!panel->ops->handle_event)
	       panel->ops->handle_event = panel_nullproc;
	    break;

	 case PANEL_TIMER_SECS:
	    panel->timer_full.tv_sec = (u_int) *avlist++;
	    break;

	 case PANEL_TIMER_USECS:
	    panel->timer_full.tv_usec = (u_int) *avlist++;
	    break;

	 case PANEL_ITEM_X:
	    panel->item_x = (int) *avlist++;
	    break;

	 case PANEL_ITEM_Y:
	    panel->item_y = (int) *avlist++;
	    break;

	 case PANEL_ITEM_X_GAP:
	    panel->item_x_offset = (int) *avlist++;
	    break;

	 case PANEL_ITEM_Y_GAP:
	    panel->item_y_offset = (int) *avlist++;
	    break;

	 case PANEL_SHOW_MENU:
	    if (*avlist++)
	       panel->flags |= SHOW_MENU;
	    else
	       panel->flags &= ~SHOW_MENU;
	       panel->status |= SHOW_MENU_SET;
	    break;

	 case PANEL_LABEL_BOLD:
	    if (*avlist++)
	       panel->flags |= LABEL_BOLD;
	    else
	       panel->flags &= ~LABEL_BOLD;
	    break;

	 case PANEL_LABEL_SHADED:
	    if (*avlist++)
	       panel->flags |= LABEL_SHADED;
	    else
	       panel->flags &= ~LABEL_SHADED;
	    break;

	 case PANEL_LAYOUT:
	    switch ((Panel_setting) *avlist) {
	       case PANEL_HORIZONTAL:
	       case PANEL_VERTICAL:
		  panel->layout = (Panel_setting) *avlist++;
		  break;
	       default:    /* invalid layout */
		  avlist++;
		  break;
	    }
	    break;

	 case PANEL_PAINT:
	    panel->repaint = (Panel_setting) *avlist++;
	    break;

         case WIN_VERTICAL_SCROLLBAR:
         case PANEL_VERTICAL_SCROLLBAR:
            new_v_scrollbar      = (Scrollbar) *avlist++;
            have_new_v_scrollbar = TRUE;
            break;
 
         case WIN_HORIZONTAL_SCROLLBAR:
         case PANEL_HORIZONTAL_SCROLLBAR:
            new_h_scrollbar      = (Scrollbar) *avlist++;
            have_new_h_scrollbar = TRUE;
            break;
 
	 case PANEL_ACCEPT_KEYSTROKE: 
	    if (*avlist++)
	       panel->flags |= WANTS_KEY;
	    else
	       panel->flags &= ~WANTS_KEY;
	    break;

	 default: 
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   /* open the default font if user hasn't supplied one */
   if (!panel->font)
      panel->font = pw_pfsysopen();

   /* using timer if blinking caret or notifying timer proc */
   if (((panel->status & BLINKING) && (panel->caret)) 
         || (panel->timer_proc))
      panel->status |= TIMING;

   /* set up any scrollbars */
   panel_setup_scrollbar(panel, SCROLL_VERTICAL, 
		   have_new_v_scrollbar, new_v_scrollbar);
   panel_setup_scrollbar(panel, SCROLL_HORIZONTAL,
		   have_new_h_scrollbar, new_h_scrollbar);

}

/*****************************************************************************/
/* shrink_to_fit                                                             */
/*****************************************************************************/

static int
shrink_to_fit(panel, orientation, excess)
register panel_handle    panel;
register Panel_attribute orientation;
register int             excess;
{
   register panel_item_handle ip;
   register int               low_point   = 0;
   register int               right_point = 0;
   int                        new_size;

   if (orientation == PANEL_HEIGHT) {
      for(ip = panel->items; ip; ip = ip->next)
	 low_point = max(low_point,ip->rect.r_top + ip->rect.r_height);
      new_size = low_point + excess + panel->h_bar_width;
   } else { 
      for(ip = panel->items; ip; ip = ip->next)
	 right_point = max(right_point,ip->rect.r_left + ip->rect.r_width);
      new_size = right_point + excess + panel->v_bar_width;
   }
   return new_size;
}

/*****************************************************************************/
/* set_size                                                                  */
/* sets the rect in 3 places: panel, kernel's window tree, and toolsw.       */
/*****************************************************************************/

static void
set_size(panel, orientation, new_size)
register panel_handle    panel;
register Panel_attribute orientation;
int      new_size;
{
   Rect	rect;
   
   /* ignore extend-to-edge and invalid sizes */
   if (new_size < 0)
       return;

   /* get the current top, left */
   (void)win_getrect(panel->windowfd, &rect);
   
   if (orientation == PANEL_HEIGHT) {
      panel->rect.r_height = new_size;
      rect.r_height = new_size;
      if (panel->toolsw)
         panel->toolsw->ts_height = new_size;
   } else {
      panel->rect.r_width = new_size;
      rect.r_width = new_size;
      if (panel->toolsw)
	 panel->toolsw->ts_width = new_size;
   }
   (void)win_setrect(panel->windowfd, &rect);
}

caddr_t
panel_get_panel(panel, attr)
register panel_handle 		panel; 
register Panel_attribute	attr;
{
   switch (attr) {
      case PANEL_PIXWIN:
	 return (caddr_t) panel->pixwin;
      case PANEL_PAINT_PIXWIN:
	 return (caddr_t)panel->view_pixwin;

      case PANEL_MOUSE_STATE:
	 return (caddr_t) panel->mouse_state;

      case PANEL_CLIENT_DATA:
	 return panel->client_data;

      case HELP_DATA:
	 return panel->help_data;

      case WIN_FONT:
      case PANEL_FONT:
	 return (caddr_t) panel->font;

      case PANEL_ADJUSTABLE:
	 return (caddr_t) adjustable(panel);

      case PANEL_LABEL_BOLD:
	 return (caddr_t) label_bold_flag(panel);

      case PANEL_LABEL_SHADED:
	 return (caddr_t) label_shaded_flag(panel);

      case PANEL_BLINK_CARET:
	 return (caddr_t) blinking(panel);

      case PANEL_CARET_ITEM:
	 return (caddr_t) panel->caret;

      case PANEL_FIRST_ITEM:
	 return (caddr_t) panel->items;
	 break;

      case PANEL_TIMER_PROC:
	 return (caddr_t) LINT_CAST(panel->timer_proc);

      case PANEL_EVENT_PROC:
	 return (caddr_t) LINT_CAST(panel->event_proc);

      case PANEL_BACKGROUND_PROC:
	 return (caddr_t) LINT_CAST(panel->ops->handle_event);

      case PANEL_TIMER_SECS:
	 return (caddr_t) panel->timer_full.tv_sec;

      case PANEL_TIMER_USECS:
	 return (caddr_t) panel->timer_full.tv_usec;

      case PANEL_ITEM_X:
	 return (caddr_t) panel->item_x;

      case PANEL_ITEM_Y:
	 return (caddr_t) panel->item_y;

      case PANEL_ITEM_X_GAP:
	 return (caddr_t) panel->item_x_offset;

      case PANEL_ITEM_Y_GAP:
	 return (caddr_t) panel->item_y_offset;

      case PANEL_LAYOUT:
	 return (caddr_t) panel->layout;

      case WIN_VERTICAL_SCROLLBAR:
      case PANEL_VERTICAL_SCROLLBAR:
	 return (caddr_t) panel->v_scrollbar;

      case WIN_HORIZONTAL_SCROLLBAR:
      case PANEL_HORIZONTAL_SCROLLBAR:
	 return (caddr_t) panel->h_scrollbar;

      case PANEL_ACCEPT_KEYSTROKE: 
         return (caddr_t) wants_key(panel);

      case PANEL_WIDTH:
	 return (caddr_t) panel->rect.r_width;

      case PANEL_HEIGHT:
	 return (caddr_t) panel->rect.r_height;

      case WIN_FIT_WIDTH:
	 return (caddr_t) shrink_to_fit(panel, PANEL_WIDTH, SHRINK_MARGIN);

      case WIN_FIT_HEIGHT:
	 return (caddr_t) shrink_to_fit(panel, PANEL_HEIGHT, SHRINK_MARGIN);

      default:
	 return (caddr_t) FALSE;

   }
   return (caddr_t) FALSE;
}


/*****************************************************************************/
/* panel_make_list                                                           */
/*****************************************************************************/

/* VARARGS */
Attr_avlist
panel_make_list(va_alist)
va_dcl
{
   Attr_avlist	avlist;
   va_list	valist;

   va_start(valist);
   avlist = attr_make((Attr_avlist)0, 0, valist);
   va_end(valist);
   return avlist;
}

/* utilities for event location translation */


/* translate a panel-space event to a
 * window-space event.
 */
Event *
panel_window_event(client_panel, event)
Panel		client_panel;
register Event	*event;
{
    register panel_handle	panel =PANEL_CAST(client_panel);

    event_set_x(event, event_x(event) - panel->h_offset +
		panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_left); 
    event_set_y(event, event_y(event) - panel->v_offset +
		panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_top);
    return event;
}


/* translate a window-space event to a
 * panel-space event.
 */
Event *
panel_event(client_panel, event)
Panel		client_panel;
register Event	*event;
{
    register panel_handle	panel = PANEL_CAST(client_panel);

    event_set_x(event, event_x(event) + panel->h_offset -
		panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_left); 
    event_set_y(event, event_y(event) + panel->v_offset -
		panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_top);
    return event;
}
