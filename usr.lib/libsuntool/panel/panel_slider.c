#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_slider.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/**************************************************************************/
/*                            panel_slider.c                              */
/*              Copyright (c) 1985 by Sun Microsystems, Inc.              */
/**************************************************************************/

 /* TO DO:
 *	change cursor while moving slider
 */

#include <suntool/panel_impl.h>
#include <sunwindow/sun.h>

extern char	*sprintf();

#define	slider_dp(ip)		(slider_data *) LINT_CAST((ip)->data)

static	begin_preview(), update_preview(), cancel_preview(), accept_preview(),
	paint(),
	destroy(),
	set_attr(),
	layout();
static caddr_t get_attr();

static int	round();

static struct panel_ops ops  =  {
   panel_default_handle_event,		/* handle_event() */
   begin_preview,			/* begin_preview() */
   update_preview,			/* update_preview() */
   cancel_preview,			/* cancel_preview() */
   accept_preview,			/* accept_preview() */
   panel_nullproc,			/* accept_menu() */
   panel_nullproc,			/* accept_key() */
   paint,				/* paint() */
   destroy,				/* destroy() */
   get_attr,				/* get_attr() */
   set_attr,				/* set_attr() */
   (caddr_t (*)()) panel_nullproc,	/* remove() */
   (caddr_t (*)()) panel_nullproc,	/* restore() */
   layout				/* layout() */
};

#define	BAR_WIDTH	5
#define	LABEL_XOFFSET	5	/* offset to value rect after label */


static	short gray_data[16] = {
   0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555,
   0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555, 0xAAAA, 0x5555
};
static mpr_static(panel_gray_mpr, 16, 16, 1, gray_data);


typedef struct slider_data {	/*  data for a slider */
	int		 actual;
	int		 apparent;
	u_int		 flags;
	int		 width;
	int		 min_value;
	int		 max_value;
	int		 valuesize;
	Pixfont		*font;
	Rect	 	 sliderrect;	/* rect containing slider */
	Rect	 	 valuerect;	/* rect containing value of slider */
	Rect	 	 leftrect;	/* rect containing info to the left */
	Rect	  	 rightrect;	/* rect containing info to the right */
	int		 print_value;	/* value from PANEL_VALUE */
	int		 use_print_value : 1;
	int		 restore_print_value : 1;
}  slider_data;


/* flags */
#define	OS_SHOWRANGE	1	/* show high and low scale numbers */
#define	OS_SHOWVALUE	2	/* show current value */
#define	OS_CONTINUOUS	4	/* notify on any change */


Panel_item
panel_slider(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   register slider_data *dp;

   dp = (slider_data *) LINT_CAST(calloc(1, sizeof(slider_data)));
   if (!dp)
      return (NULL);

   ip->ops       = &ops;
   ip->data      = (caddr_t) dp;
   ip->item_type = PANEL_SLIDER_ITEM;

   dp->actual    = 0;
   dp->apparent  = 0;
   dp->flags     = OS_SHOWRANGE | OS_SHOWVALUE;
   dp->width     = 100;
   dp->min_value = 0;
   dp->max_value = 100;
   dp->valuesize = 3;			/* three characters wide */
   dp->font      = ip->panel->font;

   /* value info rect starts at value_rect position */
   dp->valuerect = ip->value_rect;

   /* update the various rects */
   update_rects(ip);

   if (!set_attr(ip, avlist))
      return NULL;

   return (Panel_item) panel_append(ip);
} /* panel_slider */


static int
set_attr(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register slider_data		*dp = slider_dp(ip);
   register Panel_attribute 	attr;
   int				value;
   short			value_set = FALSE;
   int				min_value = dp->min_value;
   int				max_value = dp->max_value;
   int				width = dp->width;
   short			range_set = FALSE;
   short			size_changed = FALSE;


   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_VALUE_FONT:
	    dp->font = (Pixfont *) LINT_CAST(*avlist++);
	    size_changed = TRUE;
	    break;

	 case PANEL_NOTIFY_LEVEL:
	    if ((Panel_setting) *avlist++ == PANEL_ALL)
	       dp->flags |= OS_CONTINUOUS;
	    else
	       dp->flags &= ~OS_CONTINUOUS;
	    break;

	 case PANEL_VALUE:
	    value = (int) *avlist++;
	    value_set = TRUE;
	    break;
	 
	 case PANEL_MIN_VALUE:
	    min_value = (int) *avlist++;
	    range_set = TRUE;
	    break;

	 case PANEL_MAX_VALUE:
	    max_value = (int) *avlist++;
	    range_set = TRUE;
	    break;

	 case PANEL_SLIDER_WIDTH:
	    width = (int) *avlist++;
	    range_set = TRUE;
	    break;

	 case PANEL_SHOW_VALUE:
	    if (*avlist++)
	       dp->flags |= OS_SHOWVALUE;
	    else
	       dp->flags &= ~OS_SHOWVALUE;
	    size_changed = TRUE;
	    break;

	 case PANEL_SHOW_RANGE:
	    if ((int) *avlist++)
	       dp->flags |= OS_SHOWRANGE;
	    else
	       dp->flags &= ~OS_SHOWRANGE;
	    size_changed = TRUE;
	    break;

	 default:
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   if (range_set) {
      char	buf[16];	/* string version of max. value */
	char	buf2[16];

      /* get the current value */
      if (!value_set) {
	 value = itoe(dp, dp->actual);
	 value_set = TRUE;
      }

      dp->min_value = min_value;
      /* don't let the max value be <= the min value */
      dp->max_value = 
	 (max_value <= dp->min_value) ? dp->min_value + 1 : max_value;
      (void) sprintf(buf, "%d", dp->max_value);
	(void) sprintf(buf2, "%d", dp->min_value);
      dp->valuesize = (strlen(buf) > strlen(buf2))? strlen(buf): strlen(buf2);
      dp->width = width;
      size_changed = TRUE;
      /* print_value is no longer valid */
      dp->use_print_value = FALSE;
   }

   /* set apparent & actual value */
   if (value_set) {
      if (value < dp->min_value) 
	 value = dp->min_value;
      else if (value > dp->max_value)
	 value = dp->max_value;
      dp->apparent = dp->actual = etoi(dp, value);
      dp->print_value = value;
      dp->use_print_value = TRUE;
   }

   if (size_changed)
      update_rects(ip);

   return 1;
} /* set_attr */


static caddr_t
get_attr(ip, which_attr)
panel_item_handle		ip;
register Panel_attribute	which_attr;
/* get_attr returns the current value of which_attr.
*/
{
   register slider_data *dp = slider_dp(ip);

   switch (which_attr) {
      case PANEL_VALUE_FONT:
	 return (caddr_t) dp->font;

      case PANEL_NOTIFY_LEVEL:
	 return (caddr_t) (dp->flags & OS_CONTINUOUS ? PANEL_ALL : PANEL_DONE);

      case PANEL_VALUE:
	 return (caddr_t) itoe(dp, dp->actual);

      case PANEL_MIN_VALUE:
	 return (caddr_t) dp->min_value;

      case PANEL_MAX_VALUE:
	 return (caddr_t) dp->max_value;

      case PANEL_SLIDER_WIDTH:
	 return (caddr_t) dp->width;

      case PANEL_SHOW_VALUE:
	 return (caddr_t) (dp->flags & OS_SHOWVALUE);

      case PANEL_SHOW_RANGE:
	 return (caddr_t) (dp->flags & OS_SHOWRANGE);

      default:
	 return panel_get_generic(ip, which_attr);
   }
} /* get_attr */


static
update_rects(ip)
register panel_item_handle ip;
{
   register slider_data *dp = slider_dp(ip);
   int width;				/* various widths */

   dp->valuerect.r_height = dp->font->pf_defaultsize.y;

   /* set the value rect width */
   if (dp->flags & OS_SHOWVALUE)	/* "[] " */
      dp->valuerect.r_width = dp->font->pf_defaultsize.x * (dp->valuesize + 3);
   else
      dp->valuerect.r_width = 0;

   /* create the left range rect */
   if (dp->flags & OS_SHOWRANGE)
      width = dp->font->pf_defaultsize.x * (dp->valuesize + 1); /* " " */
   else
      width = 0;
   rect_construct(&dp->leftrect, rect_right(&dp->valuerect) + 1, 
		  dp->valuerect.r_top, width, dp->valuerect.r_height);

   /* account for box lines on left and right and extra slop for bar */
   width = dp->width + 2 + BAR_WIDTH/2 + BAR_WIDTH/2;
   rect_construct(&dp->sliderrect, rect_right(&dp->leftrect) + 1, 
		  dp->valuerect.r_top, width, dp->valuerect.r_height);

   /* create the right range rect */
   rect_construct(&dp->rightrect, rect_right(&dp->sliderrect) + 1, 
                  dp->valuerect.r_top, dp->leftrect.r_width, 
		  dp->valuerect.r_top);

   /* set the item value rect to enclose all the little rects */
   ip->value_rect.r_width = 
      rect_right(&dp->rightrect) + 1 - dp->valuerect.r_left;
   ip->value_rect.r_height = dp->valuerect.r_height;

   /* update the item's enclosing rect */
   ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);
} /* update_rects */


static
destroy(dp)
slider_data *dp;
{
   free((char *) dp);
}


static
layout(ip, deltas)
panel_item_handle	 ip;
Rect			*deltas;
{
   slider_data	*dp = slider_dp(ip);

   dp->valuerect.r_left += deltas->r_left;
   dp->valuerect.r_top += deltas->r_top;
   dp->sliderrect.r_left += deltas->r_left;
   dp->sliderrect.r_top += deltas->r_top;
   dp->leftrect.r_left += deltas->r_left;
   dp->leftrect.r_top += deltas->r_top;
   dp->rightrect.r_left += deltas->r_left;
   dp->rightrect.r_top += deltas->r_top;
}


static
paint(ip)
panel_item_handle  ip;
{
   Rect	*r;
   slider_data	*dp = slider_dp(ip);
   char		 buf[10];

   r = &(ip->label_rect);

   /* paint the label */
   (void)panel_paint_image(ip->panel, &ip->label, r,PIX_COLOR(ip->color_index));

   /* paint the range */
   if (dp->flags & OS_SHOWRANGE) {
      (void) sprintf(buf, "%*d ", dp->valuesize, dp->min_value);
      r = &dp->leftrect;
        (void)panel_pw_text(ip->panel, r->r_left, r->r_top + panel_fonthome(dp->font),
	            PIX_SRC|PIX_COLOR(ip->color_index), dp->font, buf);
      (void) sprintf(buf, " %d", dp->max_value);
      r = &dp->rightrect;
        (void)panel_pw_text(ip->panel, r->r_left, r->r_top + panel_fonthome(dp->font),
	            PIX_SRC|PIX_COLOR(ip->color_index), dp->font, buf);
   }
   /* paint the slider */
   paintslider(ip, 1);
}


static
paintslider(ip, refresh)
panel_item_handle	ip;
int		 	refresh;
{
   slider_data	*dp = slider_dp(ip);
   int		value;
   Rect		*r;
   Rect	 	rr;
   char		buf[20];

   value = dp->apparent - BAR_WIDTH/2;
   if (value < 0)
      value = 0;
   else if (value > dp->width + 1 - BAR_WIDTH/2)
      value = dp->width + 1 - BAR_WIDTH/2;
   r = &dp->sliderrect;
   (void)panel_pw_lock(ip->panel, r);
   if (refresh)
      (void)panel_draw_box(ip->panel, PIX_SET, r, 1, 1);
   rr = *r;  r = &rr;
   rect_marginadjust(&rr, -1);
   (void)panel_pw_replrop(ip->panel, r->r_left, r->r_top, value, r->r_height,
		    PIX_SRC|PIX_COLOR(ip->color_index),
                      (Pixrect *)(LINT_CAST(&panel_gray_mpr)), 0, 0);
   (void)panel_pw_writebackground(ip->panel, r->r_left + value + BAR_WIDTH, r->r_top,
	                    r->r_width-value-BAR_WIDTH, r->r_height, PIX_CLR);
   (void)panel_pw_writebackground(ip->panel, r->r_left + value, r->r_top,
	                    BAR_WIDTH, r->r_height, PIX_SET);
   if (dp->flags & OS_SHOWVALUE) {
      (void) sprintf(buf, "[%d]           ", itoe(dp, dp->apparent));
      buf[dp->valuesize + 3] = '\0';
      r = &dp->valuerect;
        (void)panel_pw_text(ip->panel, r->r_left, r->r_top + panel_fonthome(dp->font),
	            PIX_SRC|PIX_COLOR(ip->color_index), dp->font, buf);
   }
   (void)pw_unlock(ip->panel->view_pixwin);
}


static
begin_preview(ip, event)
panel_item_handle	ip;
Event			*event;
{
   slider_data	*dp = slider_dp(ip);

   /* save status of print value */
   dp->restore_print_value = dp->use_print_value;

   /* update the preview */
   (ip->panel)->SliderActive = TRUE;
   update_preview(ip, event);
}

static
update_preview(ip, event)
panel_item_handle	ip;
Event			*event;
{
   slider_data	*dp = slider_dp(ip);
   register int	 new;
   Rect	 	 r;

   r = dp->sliderrect;
   rect_marginadjust(&r, -1);
   if (!(ip->panel)->SliderActive){
      if (!rect_includespoint(&r, event_x(event), event_y(event)))
         return;
   }

   new = event_x(event) - r.r_left;
   if (new == dp->apparent)
      return;		/* state and display both correct */

   dp->apparent = new;
   dp->use_print_value = FALSE;
   paintslider(ip, 0);
   /* ONLY NOTIFY IF EXTERNAL VALUE HAS CHANGED? */
   if (dp->flags & OS_CONTINUOUS)
      (*ip->notify)(ip, itoe(dp, dp->apparent), event);
}


static
cancel_preview(ip, event)
panel_item_handle	ip;
Event			*event;
{
   slider_data *dp = slider_dp(ip);

   (ip->panel)->SliderActive = FALSE;
   if (dp->apparent != dp->actual) {
      dp->apparent = dp->actual;
      dp->use_print_value = dp->restore_print_value;
      if (dp->flags & OS_CONTINUOUS)
	 (*ip->notify)(ip, itoe(dp, dp->actual), event);
      paintslider(ip, 0);
   }
}


static
accept_preview(ip, event)
panel_item_handle	ip;
Event			*event;
{
   slider_data *dp = slider_dp(ip);

   (ip->panel)->SliderActive = FALSE;
   if (dp->apparent != dp->actual)  {
      dp->actual = dp->apparent;
      /* print_value is no longer valid */
      dp->use_print_value = FALSE;
      (*ip->notify)(ip, itoe(dp, dp->actual), event);
   }
}


/*
 * Convert external value to internal value.
 */
static int
etoi(dp, value)
slider_data	*dp;
int		 value;
{
   if (value <= dp->min_value)
      return (BAR_WIDTH/2);

   if (value >= dp->max_value)
      return (BAR_WIDTH/2  + dp->width);
      
   return (BAR_WIDTH/2 + 
	   round(((value - dp->min_value) * dp->width),
	   (dp->max_value - dp->min_value + 1)));
}

/*
 * Convert internal value to external value.
 */
static int
itoe(dp, value)
slider_data	*dp;
int		 value;
{
   /* use the print value if valid */
   if (dp->use_print_value)
       return dp->print_value;

   value -= BAR_WIDTH/2;
   if (value <= 0)
      return (dp->min_value);

   if (value >= dp->width - 1)
      return (dp->max_value);
      
   return (dp->min_value + ((value * (dp->max_value - dp->min_value + 1)) / 
	   dp->width));
}

static int
round(x, y)
register int x, y;
{
    register int	z, rem;
    register short 	is_neg = FALSE;

    if (x < 0) {
       x = -x;
       if (y < 0)
	  y = -y;
       else
          is_neg = TRUE;
    } else if (y < 0) {
       y = -y;
       is_neg = TRUE;
    }

    z = x / y;
    rem = x % y;
    /* round up if needed */
    if (2 * rem >= y)
       if (is_neg)
	  z--;
       else
	  z++;

    return (is_neg ? -z : z);
}
