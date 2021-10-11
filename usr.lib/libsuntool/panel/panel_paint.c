#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_paint.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*               Copyright (c) 1986 by Sun Microsystems, Inc.                */

#include <suntool/panel_impl.h>

void
panel_resize(panel)
panel_handle panel;
{
   Rect	r;
   short			caret_was_on = FALSE;

   /* turn the caret off, since it
    * may be clipped at the edge of the panel.
    */
   if (panel->caret_on) {
      caret_was_on = TRUE;
      panel_caret_on(panel, FALSE);
   }

   (void)win_getsize(panel->windowfd, &r);
   panel->rect = r;
   panel_resize_region(panel);
   panel_update_scrollbars(panel, &panel->rect);

   if (caret_was_on)
      panel_caret_on(panel, TRUE);
}

void
panel_display(panel, flag)
register panel_handle panel;
Panel_setting   	flag;
{
   register panel_item_handle   ip;
   short			caret_was_on = FALSE;
   register int			x_offset, y_offset;
   Rect				rect;
   extern   pw_damaged();


   if (panel->caret_on) {
      caret_was_on = TRUE;
      panel_caret_on(panel, FALSE);
   }

   /* clear if needed */
   if (flag == PANEL_CLEAR) {
      (void)pw_writebackground(panel->pixwin, 0,0,
         panel->rect.r_width, panel->rect.r_height, PIX_CLR);

       (void)scrollbar_paint_clear(panel->v_scrollbar);
       (void)scrollbar_paint_clear(panel->h_scrollbar);
   } else {
       (void)scrollbar_paint(panel->v_scrollbar);
       (void)scrollbar_paint(panel->h_scrollbar);
   }

   /* Set view pixwin to be using the same clipping as pixwin */
   if (panel->pixwin->pw_clipops->pwco_getclipping == pw_damaged)
	   (void)pw_damaged(panel->view_pixwin);

   /* lock around the whole view pixwin */
   rect = *panel->view_pixwin->pw_clipdata->pwcd_regionrect;
   rect.r_left = rect.r_top = 0;
   (void)pw_lock(panel->view_pixwin, &rect);

   /* paint each hidden item */
   for (ip = panel->items; ip; ip = ip->next)
      if (hidden(ip))
	 (void)panel_paint_item(ip, PANEL_NO_CLEAR);

   /* paint each non-hidden item */
   x_offset = panel->h_offset - 
	      panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_left;
   y_offset = panel->v_offset - 
	      panel->view_pixwin->pw_clipdata->pwcd_regionrect->r_top;
   for (ip = panel->items; ip; ip = ip->next)
      if (!hidden(ip)) {
	  /* only paint the item if it intersects with the
	   * subwindow pixwin.
	   */
	  rect = ip->rect;
	  rect_passtochild(x_offset, y_offset, &rect);
          if (rl_rectintersects(&rect, &panel->pixwin->pw_clipdata->pwcd_clipping))
	      (void)panel_paint_item(ip, PANEL_NO_CLEAR);
      }

   if (panel->pixwin->pw_clipops->pwco_getclipping == pw_damaged)
	   (void)pw_donedamaged(panel->view_pixwin);
   if (caret_was_on)
      panel_caret_on(panel, TRUE);

   (void)pw_unlock(panel->view_pixwin);
}


/*****************************************************************************/
/* panel_paint()                                                             */
/* calls the painting routine for panels or items, as appropriate.           */
/*****************************************************************************/

panel_paint(client_object, flag)
Panel		client_object;
Panel_setting   flag;
{
   panel_handle    object = PANEL_CAST(client_object);
   short	caret_was_on = FALSE;
   panel_handle	panel;

   if (!object || (flag != PANEL_CLEAR && flag != PANEL_NO_CLEAR))
      return NULL;

   panel = is_panel(object) ? object : ((panel_item_handle)object)->panel;

   if (panel->caret_on) {
      caret_was_on = TRUE;
      panel_caret_on(panel, FALSE);
   }

   if (is_panel(object))
      (*object->ops->paint)(object, flag);
   else
      /* This is a hack to allow pre & post painting actions
       * for all items.
       */
      (void)panel_paint_item((panel_item_handle)object, flag);
      
   if (caret_was_on || ((panel_item_handle) object) == panel->caret)
      panel_caret_on(panel, TRUE);
   return 1;
}


panel_paint_item(ip, flag)
register panel_item_handle	ip;
Panel_setting			flag;
{
   register panel_handle	panel = ip->panel;

   if (flag == PANEL_NONE)
      return;

   /* only clear if specified or hidden */
   if (flag == PANEL_CLEAR || hidden(ip)) {
      /* clear the previous painted item */
      (void)panel_pw_writebackground(panel, 
	  ip->painted_rect.r_left, ip->painted_rect.r_top,
	  ip->painted_rect.r_width, ip->painted_rect.r_height, PIX_CLR);
      /* nothing is painted */
      rect_construct(&ip->painted_rect, 0, 0, 0, 0);
   }

   if (!hidden(ip)) {
      Rect			rect;
      Pixwin			*pixwin = panel->view_pixwin;
      struct pixwin_clipdata	*cd = pixwin->pw_clipdata;

      /* don't lock or paint unless the item is visible */
      rect = ip->rect;
      rect_passtochild(panel->h_offset, panel->v_offset, &rect);
      if (rl_rectintersects(&rect, &cd->pwcd_clipping)) {
         /* Lock, paint & unlock */
         (void)pw_lock(pixwin, &rect);
         (*ip->ops->paint)(ip);
         (void)pw_unlock(pixwin);
         ip->painted_rect = ip->rect;
      }
   }
}
