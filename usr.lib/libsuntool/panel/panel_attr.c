#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_attr.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                          panel_attr.c                                     */
/*             Copyright (c) 1985 by Sun Microsystems, Inc.                  */
/*****************************************************************************/

#include <suntool/panel_impl.h>
#include <suntool/window.h>
#include "sunwindow/sv_malloc.h"

void	window_rc_units_to_pixels();
/*****************************************************************************/
/* panel_set_generic                                                         */
/*****************************************************************************/

panel_set_generic(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register Panel_attribute	attr; 
   register panel_handle	panel		   = ip->panel;
   register panel_image_handle	label		   = &ip->label;
   int			 	item_x, item_y;
   int			 	label_x, label_y;
   int			 	value_x, value_y;
   short		 	item_x_changed	   = FALSE;
   short		 	item_y_changed     = FALSE;
   short		 	label_size_changed = FALSE;
   short		 	label_x_changed    = FALSE;
   short		 	label_y_changed    = FALSE;
   short		 	value_x_changed    = FALSE;
   short		 	value_y_changed    = FALSE;
   short		 	layout_changed     = FALSE;
   short		 	label_bold_changed = FALSE;
   short		 	label_font_changed = FALSE;
   int			 	label_type         = -1;  /* no new label yet */
   struct pixfont	       *label_font         = panel->font;
   int			 	label_bold	   = label_bold_flag(panel);
   int			 	label_shaded	   = image_shaded(label);
   struct pr_size   	 	label_size;
   caddr_t		 	label_data;
   Rect		 	 	deltas;
   short		 	potential_new_rect = FALSE;

   if (using_wrapper(panel))
      window_rc_units_to_pixels((Window)(LINT_CAST(panel)), 
      	(Window_attribute *)(LINT_CAST(avlist)));
   else
      attr_replace_cu(avlist, panel->font, PANEL_ITEM_X_START, 
		      panel->item_y_offset, panel->item_y_offset);

   /* use the current font & boldness */
   if (is_string(label)) {
      label_font = image_font(label);
      label_bold = image_bold(label);
   }

   while(attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_SHOW_ITEM:
	    if ((int) *avlist++) {
	       if (hidden(ip)) {
		  ip->flags &= ~HIDDEN;
		  (*ip->ops->restore)(ip);
	       }
	    }
	    else if (!hidden(ip)) {
	       ip->flags |=HIDDEN;
	       (*ip->ops->remove)(ip);
	    }
	    break;

	 case PANEL_SHOW_MENU:
	    if ((int) *avlist++)
               ip->flags |= SHOW_MENU;
            else
               ip->flags &= ~SHOW_MENU;
	    break;

	 case PANEL_ITEM_X :
	    item_x = (int) *avlist++;
	    ip->flags |= ITEM_X_FIXED;
	    item_x_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_ITEM_Y :
	    item_y = (int) *avlist++;
	    ip->flags |= ITEM_Y_FIXED;
	    item_y_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_X:
	    label_x = (int) *avlist++;
	    ip->flags |= LABEL_X_FIXED;
	    label_x_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_Y: 
	    label_y = (int) *avlist++;
	    ip->flags |= LABEL_Y_FIXED;
	    label_y_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_VALUE_X:
	    value_x = (int) *avlist++;
	    ip->flags |= VALUE_X_FIXED;
	    value_x_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_VALUE_Y: 
	    value_y = (int) *avlist++;
	    ip->flags |= VALUE_Y_FIXED;
	    value_y_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_IMAGE:
	    label_type = IM_PIXRECT;
	    label_data = *avlist++;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_STRING:
	    label_type = IM_STRING;
	    label_data = *avlist++;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_FONT :
	    label_font = (struct pixfont *) LINT_CAST(*avlist++);
	    label_font_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_BOLD:
	    label_bold = (int) *avlist++;
	    label_bold_changed = TRUE;
	    potential_new_rect = TRUE;
	    break;

	 case PANEL_LABEL_SHADED:
	    label_shaded = (int) *avlist++;
	    image_set_shaded(label, label_shaded);
	    break;

	 case PANEL_LAYOUT:
	    switch ((Panel_setting) *avlist) {
	       case PANEL_HORIZONTAL:
	       case PANEL_VERTICAL:
		  ip->layout = (Panel_setting) *avlist++;
		  layout_changed = TRUE;
	          potential_new_rect = TRUE;
		  break;

	       default:
		  /* invalid layout */
		  avlist++;
		  break;
	    }
	    break;

	 case PANEL_NOTIFY_PROC:
            ip->notify = (int (*)()) LINT_CAST(*avlist++);
	    if (!ip->notify)
	       ip->notify = panel_nullproc;
	    break;

	 case PANEL_ADJUSTABLE:
	    if ((int) *avlist++)
	       ip->flags |= ADJUSTABLE;
	    else
	       ip->flags &= ~ADJUSTABLE;
	    break;

	 case PANEL_PAINT:
	    ip->repaint = (Panel_setting) *avlist++;
	    break;

	 case PANEL_ACCEPT_KEYSTROKE: 
	    if (*avlist++)
	       ip->flags |= WANTS_KEY;
	    else
	       ip->flags &= ~WANTS_KEY;
	    break;

	 case PANEL_CLIENT_DATA:
	    ip->client_data = *avlist++;
	    break;

	 case HELP_DATA:
	    ip->help_data = *avlist++;
	    break;

	case PANEL_ITEM_COLOR:
	    ip->color_index = (int) *avlist++;
	    break;

	 default:
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   /* first handle any item position
    * changes.
    */
   if (item_x_changed || item_y_changed) {
      rect_construct(&deltas, 0, 0, 0, 0);
      /* compute item offset */
      if (item_x_changed)
	 deltas.r_left = item_x - ip->rect.r_left;
      if (item_y_changed)
	 deltas.r_top = item_y - ip->rect.r_top;
      if (deltas.r_left || deltas.r_top) 
	 /* ITEM_X or ITEM_Y has changed, so re-layout item in
	  * order to cause the entire item to change position.
	  */ 
	 (void)panel_layout(ip, &deltas);
   }

   /* Now handle label size
    * changes .
    */

   /* update label font & boldness if needed.
    * Note that this may be set again below if
    * a new label string or image has been given.
    */
   if (is_string(label) && label_bold_changed) {
      if (image_bold(label) && !label_bold) {
	  image_set_bold(label, label_bold);
	  ip->label_rect.r_width  -= 1;
	  label_size_changed = TRUE;
      } else if (!image_bold(label) && label_bold) {
	  image_set_bold(label, label_bold);
	  ip->label_rect.r_width  += 1;
	  label_size_changed = TRUE;
      }
   }
   if (is_string(label) && label_font_changed) {
      image_set_font(label, label_font);
      label_size = pf_textwidth(strlen(image_string(label)), image_font(label),
				image_string(label));
      if (image_bold(label))
	 label_size.x++;
      ip->label_rect.r_width  = label_size.x;
      ip->label_rect.r_height = label_size.y;
      label_size_changed = TRUE;
   }

   /* free old label, allocate new */
   if (set(label_type)) {
      char *old_string = is_string(label) ? image_string(label) : NULL;

      label_size = panel_make_image(label_font, label, label_type, 
				    label_data, label_bold, label_shaded);
      ip->label_rect.r_width  = label_size.x;
      ip->label_rect.r_height = label_size.y;
      label_size_changed = TRUE;

      /* now free the old string */
      if (old_string)
	 free(old_string);
   }

   /* if the label has changed,
    * menu title may be dirty.
    */
   if (label_size_changed)
      ip->menu_status.title_dirty = TRUE;
      
   /* use default positions for label or
    * value if not specified.
    */
   if (layout_changed || label_size_changed || label_x_changed || 
       label_y_changed) {
      /* layout, label position or size has changed, so re-compute
       * default value position.
       */ 
      if (label_x_changed)
	 ip->label_rect.r_left = label_x;
      if (label_y_changed)
	 ip->label_rect.r_top = label_y;

      /* now move the value if it's not fixed */
      fix_positions(ip);
   }
    
   if (value_x_changed || value_y_changed) {
      /* value position has changed, so re-compute
       * default label position.
       */ 
      rect_construct(&deltas, 0, 0, 0, 0);
      if (value_x_changed) {
	 deltas.r_left = value_x - ip->value_rect.r_left;
	 ip->value_rect.r_left = value_x;
      }
      if (value_y_changed) {
	 deltas.r_top = value_y - ip->value_rect.r_top;
	 ip->value_rect.r_top = value_y;
      }
      if (deltas.r_left || deltas.r_top) {
	 /* VALUE_X or VALUE_Y has changed, so tell item to shift 
	  * all its components (choices, marks etc.).
	  */ 
	 (*ip->ops->layout)(ip, &deltas);

	 /* now move the label if it's not fixed */
	 fix_positions(ip);
      }
   }

   /* make sure the item rect encloses the label and value */
   ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);

   /* for scrolling computations, note new extent of panel, tell scrollbars */
   if (potential_new_rect)
      panel_update_extent(panel, ip->rect);

   return 1;
}


/*****************************************************************************/
/* fix_positions - of label and value rects                                  */
/*****************************************************************************/

static
fix_positions(ip)
register panel_item_handle	ip;
{
   if (!value_fixed(ip)) {
      struct rect	deltas;

      /* compute the value position as after the label */
      /* remember the old position */
      rect_construct(&deltas, 0, 0, 0, 0);
      deltas.r_left = ip->value_rect.r_left;
      deltas.r_top = ip->value_rect.r_top;
      switch (ip->layout) {
	 case PANEL_HORIZONTAL:
	    /* after => to right of */
	    ip->value_rect.r_left = 
	       rect_right(&ip->label_rect) + 1 + 
		  (ip->label_rect.r_width ? LABEL_X_GAP : 0); 
	    ip->value_rect.r_top = ip->label_rect.r_top;
	    break;

	 case PANEL_VERTICAL:
	    /* after => below */
	    ip->value_rect.r_left = ip->label_rect.r_left;
	    ip->value_rect.r_top = 
	       rect_bottom(&ip->label_rect) + 1 +
		  (ip->label_rect.r_height ? LABEL_Y_GAP : 0); 
	    break;
      }
      /* delta is new postion minus old position */
      deltas.r_left = ip->value_rect.r_left - deltas.r_left;
      deltas.r_top = ip->value_rect.r_top - deltas.r_top;
      if (deltas.r_left || deltas.r_top)
	 /* VALUE_X or VALUE_Y has changed, so tell item to shift 
	  * all its components (choices, marks etc.).
	  */ 
	 (*ip->ops->layout)(ip, &deltas);
      return;
   }

   panel_fix_label_position(ip);
}


panel_fix_label_position(ip)
register panel_item_handle	ip;
{
   if (label_fixed(ip))
      return;

   /* compute the label position as before the value. */
   switch (ip->layout) {
      case PANEL_HORIZONTAL:
	 /* before => to left of */
	 ip->label_rect.r_left = ip->value_rect.r_left;
	 if (ip->label_rect.r_width > 0)
	     ip->label_rect.r_left -= ip->label_rect.r_width + LABEL_X_GAP;
	 ip->label_rect.r_top = ip->value_rect.r_top;
	 break;
 
      case PANEL_VERTICAL:
	 /* before => above */
	 ip->label_rect.r_left = ip->value_rect.r_left;
	 ip->label_rect.r_top = ip->value_rect.r_top;
	 if (ip->label_rect.r_height > 0)
	     ip->label_rect.r_top -= ip->label_rect.r_height + LABEL_Y_GAP;
	 break;
   }
}


/*****************************************************************************/
/* panel_get_generic                                                         */
/* returns the value of the generic attribute specified by which_attr.       */
/* NULL is returned if this is not a generic attribute.                      */
/*****************************************************************************/

caddr_t
panel_get_generic(ip, which_attr)
register panel_item_handle 	ip;
register Panel_attribute	which_attr;
{
   switch (which_attr) {
      case PANEL_VALUE_X:
	 return (caddr_t) ip->value_rect.r_left;

      case PANEL_VALUE_Y:
	 return (caddr_t) ip->value_rect.r_top;

      case PANEL_LABEL_STRING:
	 if (is_string(&ip->label))
	    return (caddr_t) image_string(&ip->label);
	 else
	    return (caddr_t) NULL;

      case PANEL_LABEL_FONT:
	 if (is_string(&ip->label))
	    return (caddr_t) image_font(&ip->label);
	 else
	    return (caddr_t) NULL;

      case PANEL_LABEL_BOLD:
	 if (is_string(&ip->label))
	    return (caddr_t) image_bold(&ip->label);
	 else
	    return (caddr_t) NULL;

      case PANEL_LABEL_SHADED:
	    return (caddr_t) image_shaded(&ip->label);

      case PANEL_LABEL_IMAGE:
	 if (is_pixrect(&ip->label))
	    return (caddr_t) image_pixrect(&ip->label);
	 else
	    return (caddr_t) NULL;

      case PANEL_LABEL_X:
	 return (caddr_t) ip->label_rect.r_left;

      case PANEL_LABEL_Y:
	 return (caddr_t) ip->label_rect.r_top;

      case PANEL_ITEM_X:
	 return (caddr_t) ip->rect.r_left;

      case PANEL_ITEM_Y:
	 return (caddr_t) ip->rect.r_top;

      case PANEL_ITEM_RECT:
	 return (caddr_t) &ip->rect;

      case PANEL_ADJUSTABLE:
	 return (caddr_t) adjustable(ip);

      case PANEL_SHOW_ITEM:
	 return (caddr_t) !hidden(ip);

      case PANEL_NOTIFY_PROC:
	 return (caddr_t) LINT_CAST(ip->notify);

      case PANEL_EVENT_PROC:
	 return (caddr_t) LINT_CAST(ip->ops->handle_event);

      case PANEL_NEXT_ITEM:
	 return (caddr_t) ip->next;

      case PANEL_PIXWIN:
	 return (caddr_t) ip->panel->pixwin;

      case PANEL_MOUSE_STATE:
	 return (caddr_t) ip->panel->mouse_state;

      case PANEL_LAYOUT:
	 return (caddr_t) ip->layout;

      case PANEL_MENU_TITLE_STRING:
	 return is_string(&ip->menu_title) ?
		(caddr_t) image_string(&ip->menu_title) : NULL;

      case PANEL_MENU_TITLE_IMAGE:
	 return is_pixrect(&ip->menu_title) ?
		(caddr_t) image_pixrect(&ip->menu_title) : NULL;

       case PANEL_MENU_TITLE_FONT:
	 return is_string(&ip->menu_title) ?
		(caddr_t) image_font(&ip->menu_title) : NULL;

      case PANEL_MENU_CHOICE_STRINGS:
      case PANEL_MENU_CHOICE_IMAGES:
      case PANEL_MENU_CHOICE_FONTS:
      case PANEL_MENU_CHOICE_VALUES:
	 return NULL;

      case PANEL_SHOW_MENU:
	 return (caddr_t) show_menu(ip);

      case PANEL_SHOW_MENU_MARK:
	 return (caddr_t) show_menu_mark(ip);

      case PANEL_MENU_MARK_IMAGE:
	 return (caddr_t) ip->menu_mark_on;

      case PANEL_MENU_NOMARK_IMAGE:
	 return (caddr_t) ip->menu_mark_off;

      case PANEL_TYPE_IMAGE:
	 return (caddr_t) ip->menu_type_pr;

      case PANEL_ACCEPT_KEYSTROKE: 
         return (caddr_t) wants_key(ip);

      case PANEL_CLIENT_DATA:
	 return ip->client_data;

      case HELP_DATA:
	 return ip->help_data;

      case PANEL_ITEM_COLOR:
	 return (caddr_t) ip->color_index;


      case PANEL_PARENT_PANEL:
         return (caddr_t) ip->panel;

      default :
	 return (caddr_t) NULL;
   }
} /* panel_get_generic */


panel_set_ops(ip, avlist)
register panel_item_handle 	ip;
register Attr_avlist		avlist;
{
   register Panel_attribute	attr;
   register panel_ops_handle	new_ops;

   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_EVENT_PROC:
	    if (!ops_set(ip)) {
	       new_ops = (panel_ops_handle) 
		   LINT_CAST(sv_malloc(sizeof(struct panel_ops)));
	       *new_ops = *ip->ops;
	       ip->ops = new_ops;
	       ip->flags |= OPS_SET;
	    }
            ip->ops->handle_event = (int (*)()) LINT_CAST(*avlist++);
	    if (!ip->ops->handle_event)
	       ip->ops->handle_event = panel_nullproc;
	    break;

	 default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }
}
