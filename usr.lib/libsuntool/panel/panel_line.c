#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_line.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/**************************************************************************/
/*                            panel_line.c                                */
/*              Copyright (c) 1985 by Sun Microsystems, Inc.              */
/**************************************************************************/

#include <suntool/panel_impl.h>

static	paint(),
	destroy(),
	set_attr();

static caddr_t get_attr();

static struct panel_ops ops  =  {
   panel_nullproc,			/* handle_event() */
   panel_nullproc,			/* begin_preview() */
   panel_nullproc,			/* update_preview() */
   panel_nullproc,			/* cancel_preview() */
   panel_nullproc,			/* accept_preview() */
   panel_nullproc,			/* accept_menu() */
   panel_nullproc,			/* accept_key() */
   paint,				/* paint() */
   destroy,				/* destroy() */
   get_attr,				/* get_attr() */
   set_attr,				/* set_attr() */
   (caddr_t (*)()) panel_nullproc,	/* remove() */
   (caddr_t (*)()) panel_nullproc,	/* restore() */
   panel_nullproc			/* layout() */
};

typedef struct line_data {	/* data for a line */
	int	length;
}  line_data;


Panel_item
panel_line(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   register line_data *dp;

   dp = (line_data *) LINT_CAST(calloc(1, sizeof(line_data)));
   if (!dp)
      return (NULL);

   ip->ops       = &ops;
   ip->data      = (caddr_t) dp;
   ip->item_type = PANEL_LINE_ITEM;

   dp->length	 = 0;

   if (!set_attr(ip, avlist))
      return NULL;

   return (Panel_item) panel_append(ip);
} /* panel_line */


static int
set_attr(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register line_data		*dp = (line_data *) LINT_CAST(ip->data);
   register Panel_attribute 	attr;
   register Rect		*vr = &ip->value_rect;

   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_VALUE_DISPLAY_LENGTH:
	    dp->length = (int) *avlist++;
	    if (ip->layout == PANEL_HORIZONTAL) {
		vr->r_width	= dp->length;
		vr->r_height	= 1;
	    }
	    else {
		vr->r_width	= 1;
		vr->r_height	= dp->length;
	    }
   	    ip->rect = panel_enclosing_rect(&ip->label_rect, vr);
	    break;

	 default:
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }
   return 1;
} /* set_attr */


static caddr_t
get_attr(ip, which_attr)
panel_item_handle		ip;
register Panel_attribute	which_attr;
/* get_attr returns the current value of which_attr.
*/
{
   register line_data *dp = (line_data *) LINT_CAST(ip->data);

   switch (which_attr) {
      case PANEL_VALUE_DISPLAY_LENGTH:
	 return (caddr_t) dp->length;

      default:
	 return panel_get_generic(ip, which_attr);
   }
} /* get_attr */


static
destroy(dp)
line_data *dp;
{
   free((char *) dp);
}


static
paint(ip)
register panel_item_handle  ip;
{
   line_data		*dp = (line_data *) LINT_CAST(ip->data);
   register Rect	*vr = &ip->value_rect;
   panel_handle		panel = ip->panel;
   register int		x1, y1;

   switch (ip->layout) {
      case PANEL_HORIZONTAL:
         x1 = (dp->length == -1) ? 
	      rect_right(&panel->rect) : vr->r_left + dp->length;
	 y1 = vr->r_top;
	 break;

      case PANEL_VERTICAL:
         y1 = (dp->length == -1) ? 
	      rect_bottom(&panel->rect) : vr->r_top + dp->length;
	 x1 = vr->r_left;
	 break;

      default:
	 x1 = y1 = 0;
	 break;
   }
   (void)panel_paint_image(panel, &ip->label, &ip->label_rect,PIX_COLOR(ip->color_index));
   (void)panel_pw_vector(panel, vr->r_left, vr->r_top, x1, y1, PIX_SRC|PIX_COLOR(ip->color_index), 1);
}
