#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_create.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*               Copyright (c) 1986 by Sun Microsystems, Inc.                */

#include <sunwindow/sun.h>
#include <varargs.h>
#include <suntool/panel_impl.h>
#include <suntool/window.h>
#include <sunwindow/win_keymap.h>
#include "sunwindow/sv_malloc.h"
#define		FOREGROUND	-1

Bool	defaults_get_boolean();

static short menu_on_image[] = {
#include <images/panel_menu_on.pr>
};
static mpr_static(panel_menu_on, 16, 16, 1, menu_on_image);

static short caret_image[7]={
	0x1000, 0x3800, 0x3800, 0x7C00, 0x7C00, 0xFE00, 0xFE00
	};
static mpr_static(panel_caret_pr, 7, 7, 1, caret_image);

static short ghost_caret[7]={
	0x1000, 0x2800, 0x5400, 0xAA00, 0x5400, 0x2800, 0x1000
};
mpr_static(panel_ghost_caret_pr, 7, 7, 1, ghost_caret);



/* default timer value */
static struct timeval PANEL_TIMER = { 0, 500000 };

/* Note that this must be modified when the 
 * ops structure is modified. 
 */
static struct panel_ops null_ops = {
   panel_nullproc,     			/* handle_event() */
   panel_nullproc,     			/* begin_preview() */
   panel_nullproc,     			/* update_preview() */
   panel_nullproc,     			/* cancel_preview() */
   panel_nullproc,     			/* accept_preview() */
   panel_nullproc,     			/* accept_menu() */
   panel_nullproc,     			/* accept_key() */
   panel_nullproc,     			/* paint() */
   panel_nullproc,     			/* destroy() */
   (caddr_t (*)()) panel_nullproc, 	/* get_attr() */
   panel_nullproc,     			/* set_attr() */
   (caddr_t (*)()) panel_nullproc, 	/* remove() */
   (caddr_t (*)()) panel_nullproc, 	/* restore() */
   panel_nullproc     			/* layout() */
};

static         free_panel(), shrink_to_fit();

/* Note that this must be modified when the ops structure is modified. */
static struct panel_ops default_panel_ops = {
   panel_default_handle_event,		/* handle_event() */
   panel_nullproc,     			/* begin_preview() */
   panel_nullproc,     			/* update_preview() */
   panel_nullproc,     			/* cancel_preview() */
   panel_nullproc,     			/* accept_preview() */
   panel_nullproc,     			/* accept_menu() */
   panel_nullproc,     			/* accept_key() */
   (int (*)()) panel_display,     	/* paint() */
   free_panel,     			/* destroy() */
   panel_get_panel,			/* get_attr() */
   panel_set_panel,     		/* set_attr() */
   (caddr_t (*)()) panel_nullproc, 	/* remove() */
   (caddr_t (*)()) panel_nullproc, 	/* restore() */
   panel_nullproc     			/* layout() */
};

/*****************************************************************************/
/* panel_create_item                                                         */
/*****************************************************************************/

/* VARARGS2 */
Panel_item
panel_create_item(client_panel, create_proc, va_alist)
Panel		client_panel;
Panel_item	(*create_proc)();
va_dcl
{
   register panel_handle	panel = PANEL_CAST(client_panel);
   caddr_t 			avlist[ATTR_STANDARD_SIZE];
   register panel_item_handle	ip;
   va_list			valist;

   va_start(valist);
   (void)attr_make(avlist, ATTR_STANDARD_SIZE,  valist);
   va_end(valist);

   if (!(ip = (panel_item_handle)LINT_CAST(calloc(1, sizeof(struct panel_item)))))
      return(NULL);
 
   ip->panel                = panel;
   ip->next                 = NULL;
   ip->flags                = IS_ITEM | panel->flags; /* get panel-wide attrs */
   ip->flags               &= ~IS_PANEL;	/* this is not a panel */
   ip->flags               &= ~OPS_SET;		/* ops vector is not altered */
   ip->layout               = panel->layout;
   ip->notify               = panel_nullproc;
   ip->ops                  = &null_ops;
   ip->repaint              = panel->repaint;    /* default painting behavior */
   ip->color_index	    = FOREGROUND;	 /* default color is foreground color */

   image_set_type(&ip->label, IM_STRING);
   image_set_string(&ip->label, panel_strsave(""));
   image_set_font(&ip->label, ip->panel->font);
   image_set_bold(&ip->label, label_bold_flag(ip));
   image_set_shaded(&ip->label, label_shaded_flag(ip));

   /* nothing is painted yet */
   rect_construct(&ip->painted_rect, 0, 0, 0, 0)

   /* start the item, label & value rects at the default position */
   rect_construct(&ip->rect, ip->panel->item_x, ip->panel->item_y, 0, 0);
   ip->label_rect = ip->rect;
   ip->value_rect = ip->rect;

   /* set up the menu */
   ip->menu	         = NULL;            /* no menu */
   ip->menu_title        = ip->label;
   image_set_string(&ip->menu_title, panel_strsave(""));
   ip->menu_choices      = NULL;	    /* no menu choices */
   ip->menu_last         = -1;	            /* no last slot */
   ip->menu_choices_bold = FALSE;	    /* regular choice strings */
   ip->menu_values       = NULL;	    /* no values */
   ip->menu_type_pr      = NULL;	    /* no type indication yet */
   ip->menu_mark_on      = &panel_menu_on;  /* for PANEL_MARKED menu feedback */
   ip->menu_mark_off     = &panel_empty_pr; /* for PANEL_MARKED menu feedback */
   ip->menu_mark_width = 
      max(ip->menu_mark_on->pr_width, ip->menu_mark_off->pr_width);
   ip->menu_mark_height = 
      max(ip->menu_mark_on->pr_height, ip->menu_mark_off->pr_height);
    ip->menu_max_width = 0;		    /* menu is zero wide */

   /* set the generic attributes */
   (void) panel_set_generic(ip, avlist);

   /* create the item */
   (void) (*create_proc)(ip, avlist);

   if (panel->event_proc) {
      panel_ops_handle	new_ops;

      if (!ops_set(ip)) {
	   new_ops = (panel_ops_handle) 
	       LINT_CAST(sv_malloc(sizeof(struct panel_ops)));
	   *new_ops = *ip->ops;
	   ip->ops = new_ops;
	   ip->flags |= OPS_SET;
	}
	ip->ops->handle_event = panel->event_proc;
   }

   /* set the menu attributes */
   (void) panel_set_menu(ip, avlist);

   /* set the ops vector attributes */
   (void)panel_set_ops(ip, avlist);

   return (Panel_item) ip;
}

/* ARGSUSED */
panel_handle
panel_create_panel_struct(win, windowfd)
caddr_t		win;
int 		windowfd;
{
   register panel_handle		panel;

   if (!(panel = (panel_handle) LINT_CAST(calloc(1,sizeof(struct panel)))))
      return NULL;

   panel->windowfd         = windowfd;
   panel->flags            = IS_PANEL;
   panel->ops	   	   = &default_panel_ops;
   panel->mouse_state	   = PANEL_NONE_DOWN;
   panel->layout           = PANEL_HORIZONTAL;
   panel->timer_proc       = NULL;
   panel->event_proc       = NULL;
   panel->status           = 0;	
   /* Note that the defaults database is also referenced in panel_text.c. */
   if (defaults_get_boolean("/Text/Blink_caret", (Bool)(LINT_CAST(1)), 
   		(int *)NULL)) {
      panel->status        |= BLINKING;	
   }
/* bug 1028688: set default for Adjust_is-pending_delete to FALSE */
   if (defaults_get_boolean("/Text/Adjust_is_pending_delete",
      (Bool)(LINT_CAST(FALSE)), (int *) NULL))
      panel->status           |= ADJUST_IS_PENDING_DELETE;
   panel->caret_on         = FALSE;
   panel->caret_pr	   = &panel_ghost_caret_pr;
   panel->timer_full       = PANEL_TIMER;
   panel->item_x_offset    = ITEM_X_GAP;
   panel->item_y_offset    = ITEM_Y_GAP;
   panel->item_x           = PANEL_ITEM_X_START;
   panel->item_y           = PANEL_ITEM_Y_START;
   panel->repaint          = PANEL_CLEAR;
   panel->help_data	   = "sunview:panel";
   panel->v_margin         = 4;
   panel->h_margin         = 4;

   /* these are zero from the calloc()
   panel->max_item_y       = 0;
   panel->v_scrollbar      = 0;
   panel->h_scrollbar      = 0;
   panel->v_bar_width      = 0;
   panel->h_bar_width      = 0;
   panel->v_offset         = 0;
   panel->h_offset         = 0;
   panel->v_end            = 0;
   panel->h_end            = 0;
   */

   (void)win_getsize(windowfd, &panel->rect);

   return panel;
}



panel_set_inputmask(panel, windowfd)
   panel_handle panel;
   int windowfd;
{
   struct inputmask mask;

   /* Initialize and set kbd input mask */
   (void)input_imnull(&mask);
   mask.im_flags |= IM_EUC | IM_NEGEVENT | IM_META | IM_NEGMETA | IM_ISO;
   
   win_keymap_set_smask_class(windowfd, KEYMAP_EDIT_KEYS);
   /* Note: KEY_NEXT not apparently used in panel package */
   win_keymap_set_smask(windowfd, KEY_NEXT);
   win_keymap_set_smask(windowfd, PUT_KEY);
   win_keymap_set_smask(windowfd, GET_KEY);
   win_keymap_set_smask(windowfd, FIND_KEY);
   win_keymap_set_smask(windowfd, DELETE_KEY);
   win_keymap_set_imask_from_std_bind(&mask, KEY_NEXT);
   win_keymap_set_imask_from_std_bind(&mask, PUT_KEY);
   win_keymap_set_imask_from_std_bind(&mask, GET_KEY);
   win_keymap_set_imask_from_std_bind(&mask, FIND_KEY);
   win_keymap_set_imask_from_std_bind(&mask, DELETE_KEY);
   /* Note: AGAIN_KEY not apparently use in panel package */
   win_keymap_set_smask(windowfd, AGAIN_KEY);
   win_keymap_set_imask_from_std_bind(&mask, AGAIN_KEY);
   win_setinputcodebit(&mask, KBD_USE);
   win_setinputcodebit(&mask, KBD_DONE);
   win_setinputcodebit(&mask, KEY_RIGHT(8));
   win_setinputcodebit(&mask, KEY_RIGHT(10));
   win_setinputcodebit(&mask, KEY_RIGHT(12));
   win_setinputcodebit(&mask, KEY_RIGHT(14));
   (void)win_set_kbd_mask(windowfd, &mask);

   /* Initialize and set kbd input mask */
   (void)input_imnull(&mask);
   mask.im_flags |= IM_NEGEVENT;
   win_setinputcodebit(&mask, MS_LEFT);
   win_setinputcodebit(&mask, MS_MIDDLE);
   win_setinputcodebit(&mask, MS_RIGHT);
   win_setinputcodebit(&mask, LOC_DRAG);
   win_keymap_set_smask(windowfd, TOP_KEY);
   win_keymap_set_smask(windowfd, BOTTOM_KEY);
   win_keymap_set_smask(windowfd, OPEN_KEY);
   win_keymap_set_smask(windowfd, CLOSE_KEY);
   win_keymap_set_smask(windowfd, PROPS_KEY);
   win_keymap_set_smask(windowfd, HELP_KEY);
   win_keymap_set_imask_from_std_bind(&mask, TOP_KEY);
   win_keymap_set_imask_from_std_bind(&mask, BOTTOM_KEY);
   win_keymap_set_imask_from_std_bind(&mask, OPEN_KEY);
   win_keymap_set_imask_from_std_bind(&mask, CLOSE_KEY);
   win_keymap_set_imask_from_std_bind(&mask, PROPS_KEY);
   win_keymap_set_imask_from_std_bind(&mask, HELP_KEY);
   win_setinputcodebit(&mask, LOC_WINENTER);
   win_setinputcodebit(&mask, LOC_WINEXIT);
   win_setinputcodebit(&mask, KBD_REQUEST);
   if (has_scrollbar(panel) || has_background_proc(panel))
     win_setinputcodebit(&mask, LOC_MOVE);
   (void)win_set_pick_mask(windowfd, &mask);
}


/*****************************************************************************/
/* panel_destroy_item()                                                      */
/* added in SunView 1.5.                                                     */
/*****************************************************************************/

panel_destroy_item(client_item)
Panel_item	client_item;
{
   panel_item_handle item = PANEL_ITEM_CAST(client_item);

   if (is_item(item))
      free_item(item);
}

/*****************************************************************************/
/* panel_free()                                                              */
/* calls either free_panel() or free_item(), as appropriate.                 */
/* Documented through SunView 1.0.  Superceded in SunView 1.5 by             */
/* panel_destroy_item().                                                     */
/*****************************************************************************/

panel_free(client_object)
Panel	client_object;
{
   panel_handle object = PANEL_CAST(client_object);

   if (is_panel(object))
      (*object->ops->destroy)(object);
   else
      /* this is a hack to allow pre and post free actions for
       * all the items.
       */
      free_item((panel_item_handle)object);
}

static
free_panel(panel)
register panel_handle panel;
{
   register panel_item_handle ip, oldip;

   /* free item storage */
   ip = panel->items;
   while (ip) {
      free_item_storage(ip);
      oldip = ip;
      ip    = ip->next;
      free((char *) oldip);
   }

   /* free any scrollbars */
   /*
   if (panel->v_scrollbar)
      scrollbar_free(panel->v_scrollbar);
   if (panel->h_scrollbar)
      scrollbar_free(panel->h_scrollbar);
   */

   /* clean up with selection service */
   panel_seln_destroy(panel);
   
   /* free the ops vector storage */
   if (ops_set(panel))
      free((char *) panel->ops);

   if (panel->view_pixwin)
      (void)pw_close(panel->view_pixwin);
      
   free((char *) panel);
   (void)pw_pfsysclose();
}


static
free_item(ip)
register panel_item_handle ip;
{
   register panel_handle	panel = ip->panel;
   register panel_item_handle	prev_ip, cur_ip;

   /* freed item is no longer current */
   if (panel->current == ip)
      panel->current = (panel_item_handle) NULL;

   /* remove from any private list */
   (*ip->ops->remove)(ip);

   /* free item storage */
   free_item_storage(ip);

   /* find the previous item */
   for (prev_ip = (panel_item_handle) NULL, cur_ip = panel->items;
	cur_ip != ip; prev_ip = cur_ip, cur_ip = cur_ip->next);

   /* unlink ip */
   if (prev_ip)
      prev_ip->next = ip->next;
   else
      panel->items = ip->next;

   free((char *) ip);

   /* update the default position of the next created item */
   (void)panel_find_default_xy(panel);
}

static
free_item_storage(ip)
register panel_item_handle ip;
{
   /* destroy private data */
   (*ip->ops->destroy)(ip->data);

   /* free the label storage */
   if (is_string(&ip->label))
      free((char *) image_string(&ip->label));

   /* free the menu storage */
   (void)panel_free_menu(ip);

   /* free the ops vector storage */
   if (ops_set(ip))
      free((char *) ip->ops);

}
