#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_message.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/**************************************************************************/
/*                        panel_message.c                                 */
/*             Copyright (c) 1985 by Sun Microsystems, Inc.               */
/**************************************************************************/

#include <suntool/panel_impl.h>

static	accept(),
	paint(),
	set_attr();
static	caddr_t get_attr();

static struct panel_ops ops  =  {
   panel_default_handle_event,		/* handle_event() */
   panel_nullproc,			/* begin_preview() */
   panel_nullproc,			/* update_preview() */
   panel_nullproc,			/* cancel_preview() */
   accept,				/* accept_preview() */
   panel_nullproc,			/* accept_menu() */
   panel_nullproc,			/* accept_key() */
   paint,				/* paint() */
   panel_nullproc,			/* destroy() */
   get_attr,				/* get_attr() */
   set_attr,				/* set_attr() */
   (caddr_t (*)()) panel_nullproc,	/* remove() */
   (caddr_t (*)()) panel_nullproc,	/* restore() */
   panel_nullproc			/* layout() */
};


typedef struct message_data {
   char	dummy;
}  message_data;


/**************************************************************************/
/* message item creation routine                                          */
/**************************************************************************/

/* ARGSUSED */
Panel_item
panel_message(ip, avlist)
panel_item_handle	ip;
Attr_avlist		avlist;
{
   ip->ops            = &ops;
   ip->item_type      = PANEL_MESSAGE_ITEM;

   return (Panel_item) panel_append(ip);
}

/**************************************************************************/
/* ops vector routines                                                    */
/**************************************************************************/

static int
set_attr(ip, avlist)
panel_item_handle 	ip;
Attr_avlist		avlist;
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
paint(ip)
panel_item_handle ip; 
{
   (void)panel_paint_image(ip->panel, &ip->label, &ip->label_rect,PIX_COLOR(ip->color_index));
}

static
accept(ip, event)
panel_item_handle 	ip;
Event 			*event;
{
   (*ip->notify)(ip, event);
}
