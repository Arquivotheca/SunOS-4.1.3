#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_button.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/************************************************************************/
/*                        panel_button.c                                */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.               */
/************************************************************************/

#include <suntool/panel_impl.h>
#include <sunwindow/sun.h>

static short gray25[4]  =  {			/*  25 % gray pattern	*/
	0x8888, 0x2222, 0x4444, 0x1111
};
static mpr_static(panel_gray25_pr, 16, 4, 1, gray25);

static	begin_preview(), cancel_preview(), accept_preview(),
	accept_menu(),
	paint(),
	destroy(),
	set_attr();
static caddr_t get_attr();


static struct panel_ops ops  =  {
   panel_default_handle_event,		/* handle_event() */
   begin_preview,			/* begin_preview() */
   panel_nullproc,			/* update_preview() */
   cancel_preview,			/* cancel_preview() */
   accept_preview,			/* accept_preview() */
   accept_menu,				/* accept_menu() */
   panel_nullproc,			/* accept_key() */
   paint,				/* paint() */
   destroy,				/* destroy() */
   get_attr,				/* get_attr() */
   set_attr,				/* set_attr() */
   (caddr_t (*)()) panel_nullproc,	/* remove() */
   (caddr_t (*)()) panel_nullproc,	/* restore() */
   panel_nullproc			/* layout() */
};

typedef struct button_data {	
   char dummy;		/* no data yet */
}  button_data;

/************************************************************************/
/* button item creation routine                                         */
/************************************************************************/

/* ARGSUSED */
Panel_item
panel_button(ip, avlist)
register panel_item_handle 	ip;
Attr_avlist			avlist;
{
   button_data    *dp;

   if (!(dp = (button_data *) LINT_CAST(calloc(1, sizeof (button_data)))))
      return(NULL);

   ip->ops            = &ops;
   ip->data           = (caddr_t) dp;
   ip->item_type      = PANEL_BUTTON_ITEM;

   return (Panel_item) panel_append(ip);
} 

/************************************************************************/
/* ops vector routines                                                  */
/************************************************************************/

static int
set_attr(ip, avlist)
panel_item_handle 	ip;
Attr_avlist 		avlist;
{
   return panel_set_generic(ip, avlist);
}


static caddr_t
get_attr(ip, which_attr)
panel_item_handle	ip;
Panel_attribute		which_attr;
{
   return panel_get_generic(ip, which_attr);
}


static
destroy(dp)
button_data *dp;
{
   free((char *) dp);
}


static
paint(ip)
panel_item_handle	ip;
{
   (void)panel_paint_image(ip->panel, &ip->label, &ip->label_rect,PIX_COLOR(ip->color_index));
}


/*ARGSUSED*/
static
begin_preview(ip, event)
panel_item_handle  	ip;
Event 			*event;
{
   (void)panel_invert(ip->panel, &ip->rect);
}

/*ARGSUSED*/
static
cancel_preview(ip, event)
panel_item_handle  	ip;
Event 			*event;
{
   (void)panel_invert(ip->panel, &ip->rect);
}


static
accept_preview(ip, event)
panel_item_handle  	ip;
Event		 	*event;
{
   register Rect	 *r = &(ip->rect);

   /* undo the inversion */
   (void)panel_invert(ip->panel, r);

   /* mask in the 'working' feedback */
   (void)panel_pw_replrop(ip->panel, 
                    r->r_left, r->r_top, r->r_width, r->r_height,
	            PIX_SRC | PIX_DST, 
		    (Pixrect *)(LINT_CAST(&panel_gray25_pr)), 0, 0);
			    
    /*  
     * release event lock before calling notify proc
     * to allow other window processes to receive input
     * events while notify proc is executing.
     */
     (void) win_release_event_lock(ip->panel->windowfd);

   /* notify the client */
   (*ip->notify)(ip, event);

   (void) panel_paint((Panel_item) ip, PANEL_CLEAR);
}


static int
accept_menu(ip, event)
panel_item_handle  	ip;
Event 			*event;
{
   /* Make sure the menu title reflects the label. */
   panel_sync_menu_title(ip);
   
  if (panel_menu_display(ip, event)) {
      begin_preview(ip, event);
      accept_preview(ip, event);
   } 
}
