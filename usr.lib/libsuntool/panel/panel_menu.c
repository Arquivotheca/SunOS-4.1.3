#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_menu.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                          panel_menu.c                                     */
/*             Copyright (c) 1985 by Sun Microsystems, Inc.                  */
/*****************************************************************************/

#include <suntool/panel_impl.h>
#include <suntool/walkmenu.h>
#include "sunwindow/sv_malloc.h"

/*****************************************************************************/
/* type indication symbols panel menu titles                                 */
/*****************************************************************************/

static short bs_data[] = {
#include <images/panel_button.pr>
};
mpr_static(panel_button_pr, 16, 16, 1, bs_data);

static short cs_data[] = {
#include <images/panel_choose_one.pr>
};
mpr_static(panel_choose_one_pr, 16, 16, 1, cs_data);

static short ms_data[] = {
#include <images/panel_choose_many.pr>
};
mpr_static(panel_choose_many_pr, 16, 16, 1, ms_data);

static short ss_data[] = {
#include <images/panel_switch.pr>
};
mpr_static(panel_switch_pr, 16, 16, 1, ss_data);

static short ts_data[] = {
#include <images/panel_text.pr>
};
mpr_static(panel_text_pr, 16, 16, 1, ts_data);

static Pixrect *panel_pf_string();

#define	MARK_XOFFSET	3	/* # of pixels to leave after a menu mark */


struct menuitem *
panel_menu_display(ip, event)
register panel_item_handle	ip;
register Event			*event;
{
   Menu			menu;
   struct menuitem	*mi;
   register int		i;

   if (!ip->menu)
      return NULL;

   /* unadjust the event so the menu comes up in the right place */
   (void) panel_window_event((Panel) ip->panel, event);

   /* create and display the walking menu */

   menu = menu_create(0);

   for (i = 0; i < ip->menu->m_itemcount; i++)
       (void)menu_set(menu, 
	   MENU_IMAGE_ITEM, 
	       ip->menu->m_items[i].mi_imagedata, &ip->menu->m_items[i],
	   0);

   mi = (struct menuitem *) LINT_CAST(menu_show_using_fd(menu, ip->panel->windowfd, event));

   menu_destroy(menu);

   return mi;
}


panel_free_menu(ip)
register panel_item_handle	ip;
{
   /* free the menu storage */
   if (ip->menu) {
      if (ip->menu->m_imagedata)
	 (void)prs_destroy((Pixrect *) LINT_CAST(ip->menu->m_imagedata));
      panel_free_menu_items(ip->menu);
      free((char *) ip->menu);
      ip->menu = NULL;
   };

   /* free other menu info */
   if (is_string(&ip->menu_title)) {
      free((char *) image_string(&ip->menu_title));
   }
   
   (void)panel_free_choices(ip->menu_choices, 0, ip->menu_last);
   ip->menu_choices = NULL;

   if (ip->menu_values) {
      free((char *) ip->menu_values);
      ip->menu_values = NULL;
   }
}

static 
panel_free_menu_items(menu)
register struct menu *menu;
{
   register int i;		/* counter */

   if (!menu || !menu->m_items)
      return;

   for (i = 0; i < menu->m_itemcount; i++)
      (void)prs_destroy((Pixrect *) LINT_CAST(menu->m_items[i].mi_imagedata));

   free((char *) menu->m_items);
} /* panel_free_menu_items */


/* Syncronize the menu title
 * with the item's label.
 */
void
panel_sync_menu_title(ip)
register panel_item_handle	ip;
{
   if (ip->menu_status.title_dirty && !ip->menu_status.title_set) {
      panel_copy_menu_title(ip, &ip->label);
      ip->menu_status.title_dirty = FALSE;
   }
}


/* Free the old menu title and copy 
 * new_title.
 */
void
panel_copy_menu_title(ip, new_title)
register panel_item_handle	ip;
register panel_image_handle	new_title;
{
   
   if (is_string(&ip->menu_title))
      free((char *) image_string(&ip->menu_title));

   ip->menu_title = *new_title;
   if (is_string(new_title))
      image_string(&ip->menu_title) = panel_strsave(image_string(new_title));

   (void) re_create_menu_title(ip);
}


/* Free the old menu choices and copy those
 * in choices[0..last].
 */
void
panel_copy_menu_choices(ip, choices, last)
register panel_item_handle	ip;
panel_image_handle	choices;
int			last;
{
   register int			i;
   register panel_image_handle	mp, cp;
   
   (void)panel_free_choices(ip->menu_choices, 0, ip->menu_last);
   
   ip->menu_choices = (panel_image_handle)
      LINT_CAST(sv_calloc((u_int) (last + 1), sizeof(struct panel_image)));

   ip->menu_last = last;
   
   for (i = 0, mp = ip->menu_choices, cp = choices; i <= last;
        i++, mp++, cp++) {
      *mp = *cp;
      if (is_string(cp))
         image_string(mp) = panel_strsave(image_string(cp));
   }
   (void) re_create_menu_items(ip);
}


int
panel_set_menu(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   register Panel_attribute	attr;  	/* each attribute */

   int		  choices_type = -1;    /* IM_STRING or IM_PIXRECT */
   caddr_t	 *choices;		/* choices array */
   Pixfont	**choices_fonts		= NULL;	/* choices fonts */
   short	  choices_changed	= FALSE;

   int		  title_type		= -1;  
   caddr_t	  title_data;
   short	  title_changed		= FALSE;
   Pixfont	 *title_font = is_string(&ip->label) ? 
				   image_font(&ip->label) : ip->panel->font;
   int		  type_pr_set = FALSE;	/* non zero if type_pr is set */

   register int	  which_choice, which_arg;

   switch (ip->item_type) {
     case PANEL_BUTTON_ITEM:
     case PANEL_TEXT_ITEM:
     case PANEL_CHOICE_ITEM:
     case PANEL_TOGGLE_ITEM:
	 break;

      default:
	 return 1;
   }

   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_MENU_TITLE_STRING:
	 case PANEL_MENU_TITLE_IMAGE:
	    title_type = 
	       attr == PANEL_MENU_TITLE_STRING ? IM_STRING : IM_PIXRECT;
	    title_data = *avlist++;
	    ip->menu_status.title_set = TRUE;
	    break;

	    
          case PANEL_MENU_TITLE_FONT:
            title_font = (Pixfont *) LINT_CAST(*avlist++);
	    ip->menu_status.title_font_set = TRUE;
	    if (is_string(&ip->menu_title)) {
	       image_set_font(&ip->menu_title, title_font);
	       title_changed = TRUE;
	    }
	    break;

	 case PANEL_MENU_CHOICE_STRINGS:
	 case PANEL_MENU_CHOICE_IMAGES:
	    choices_type = 
	       attr == PANEL_MENU_CHOICE_STRINGS ? IM_STRING : IM_PIXRECT;
	    choices = avlist;	/* base of the array */
	    ip->menu_status.choices_set = TRUE;
	    while (*avlist++);		/* skip past the list */
	    break;

	 case PANEL_MENU_CHOICE_FONTS:
	    choices_fonts = (Pixfont **)avlist;
	    ip->menu_status.choice_fonts_set = TRUE;
	    while (*avlist++);	/* skip past the list */
	    break;

	 case PANEL_SHOW_MENU_MARK:
	    if((int) *avlist++)
               ip->flags |= SHOW_MENU_MARK;
            else
               ip->flags &= ~SHOW_MENU_MARK;
	    break;

	 case PANEL_MENU_CHOICE_VALUES:
	    if (!*avlist)
	       ip->menu_values = NULL;
	    else {
	       /* free the old value array */
	       if (ip->menu_values)
		  free((char *) ip->menu_values);
	       /* count & allocate the new array */
	       for (which_choice = 0; avlist[which_choice]; which_choice++);
	       ip->menu_values = (caddr_t *) 
		   LINT_CAST(sv_calloc((u_int) which_choice, sizeof(caddr_t)));
	       /* copy the values */
	       for (which_choice = 0; avlist[which_choice]; which_choice++)
		  ip->menu_values[which_choice] = avlist[which_choice];
	    }
	    while (*avlist++);
	    choices_changed = TRUE;
	    break;

	 case PANEL_MENU_MARK_IMAGE:
	    ip->menu_mark_on = 
	       (*avlist ? (Pixrect *) LINT_CAST(*avlist) : &panel_empty_pr);
	    avlist++;
	    ip->menu_mark_width = MARK_XOFFSET +
	       max(ip->menu_mark_on->pr_width, ip->menu_mark_off->pr_width);
	    ip->menu_mark_height = 
	       max(ip->menu_mark_on->pr_height, ip->menu_mark_off->pr_height);
	    choices_changed = TRUE;
	    break;

	 case PANEL_MENU_NOMARK_IMAGE:
	    ip->menu_mark_off = 
	       (*avlist ? (Pixrect *) LINT_CAST(*avlist) : &panel_empty_pr);
	    avlist++;
	    ip->menu_mark_width = MARK_XOFFSET +
	       max(ip->menu_mark_on->pr_width, ip->menu_mark_off->pr_width);
	    ip->menu_mark_height = 
	       max(ip->menu_mark_on->pr_height, ip->menu_mark_off->pr_height);
	    choices_changed = TRUE;
	    break;

	 case PANEL_TYPE_IMAGE:
	    ip->menu_type_pr = 
	       (*avlist ? (Pixrect *) LINT_CAST(*avlist) : &panel_empty_pr);
	    avlist++;
	    type_pr_set = TRUE;
	    title_changed = TRUE;
	    break;

	 default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   /* free old title, allocate new */
   if (set(title_type)) {
      char *old_string = is_string(&ip->menu_title) ? 
			    image_string(&ip->menu_title) : NULL;

      (void) panel_make_image(title_font, &ip->menu_title, title_type, 
			      title_data, TRUE, FALSE);
      title_changed = TRUE;

      /* now free the old string */
      if (old_string)
	 free((char *) old_string);
  }

   if (set(choices_type)) {
      if (!re_alloc_menu_choices(ip, choices_type, choices))
	 return 0;
      choices_changed = TRUE;
   }

   if (choices_fonts) {
      for (which_choice = which_arg = 0; which_choice <= ip->menu_last; 
	   which_choice++, which_arg += choices_fonts[which_arg + 1] ? 1 : 0)
	 if (is_string(&ip->menu_choices[which_choice]))
	    image_font(&ip->menu_choices[which_choice]) 
	       = choices_fonts[which_arg];
      choices_changed = TRUE;
   }

   /* Set the default type pixrect according
    * to the item type.
    */
   if (!type_pr_set && !ip->menu_type_pr) {
      switch (ip->item_type) {
	 case PANEL_BUTTON_ITEM:
	    ip->menu_type_pr = &panel_button_pr;
	    break;

	 case PANEL_TEXT_ITEM:
	    ip->menu_type_pr = &panel_text_pr;
	    break;

	 case PANEL_CHOICE_ITEM:
	    ip->menu_type_pr = 
	       show_menu_mark(ip) ? &panel_choose_one_pr : &panel_button_pr;
	    break;

	 case PANEL_TOGGLE_ITEM:
	    ip->menu_type_pr = 
	       show_menu_mark(ip) ? &panel_choose_many_pr : &panel_button_pr;
	    break;

      }
      /* only update the title if menu is shown */
      if (show_menu(ip))
         title_changed = TRUE;
      else
         ip->menu_status.title_dirty = TRUE;
   }

   /* Re-create the menu title if any part of
    * it has changed.
    */
   if (title_changed || (choices_changed && !ip->menu))
      (void) re_create_menu_title(ip);

   /* Re-create the menu items if the menu choices
    * have changed.
    */
   if (choices_changed)
      (void) re_create_menu_items(ip);

   return 1;

} /* panel_set_menu */


/* re_alloc_menu_choices allocates ip->menu_choices from choices.  The
 * old info is reused and then freed.
 */
static int
re_alloc_menu_choices(ip, type, choices)
register panel_item_handle 	ip;		/* the item */
int 				type;		/* IM_STRING or IM_PIXRECT */
caddr_t 			choices[];	/* each choice */
{
   panel_image_handle 		old_choices = ip->menu_choices;
   int 				old_last    = ip->menu_last;
   register int num_choices, i, old_slot;	/* counters */


   /* count the # of choices */
   for (num_choices = 0; choices[num_choices]; num_choices++);

   /* allocate the panel_image[] storage for the choices */
   if ((ip->menu_choices = (panel_image_handle)
      LINT_CAST(malloc((u_int) (num_choices * sizeof(struct panel_image))))) == NULL)
      return 0;

   ip->menu_last = num_choices - 1; /* last slot used in menu_choices[] */

   /* Copy the choices to the allocated storage.
    * Here we reuse the old font info
    * if it was given.
    */
   for (i = 0; i <= ip->menu_last; i++) {
      if (old_choices) {
	 old_slot = (i <= old_last) ? i : old_last;
	 (void) panel_make_image(is_string(&old_choices[old_slot]) ? 
		    image_font(&old_choices[old_slot]) : ip->panel->font,
		    &ip->menu_choices[i], type, choices[i], 
		    ip->menu_choices_bold, FALSE);
      } else
	  (void) panel_make_image(ip->panel->font, &ip->menu_choices[i], type, 
				  choices[i], ip->menu_choices_bold, FALSE);
   }

   /* now free the old info */
   (void)panel_free_choices(old_choices, 0, old_last);

   return 1;
} /* re_alloc_menu_choices */


static int
re_create_menu_title(ip)
register panel_item_handle	ip;
{

   register struct menu *mp = ip->menu;      /* the menu to be altered */
   Pixrect       	*orig_pr;            
   Pixrect       	*title_pr;           
   int			offset_x	= ip->menu_mark_width + 6;
   int			type_width	= ip->menu_type_pr->pr_width + 6;
   int			type_height	= ip->menu_type_pr->pr_height;
   int                  title_offset;


   /* allocate the menu if none exists */
   if (!mp)
      if ((ip->menu = mp = (struct menu *) LINT_CAST(calloc(1,sizeof(struct menu)))) == NULL) 
	 return 0;

   /* now make the menu title... */
   /* 1) create "orig_pr"  - the original pixrect holding the title info */
   /* 2) create "title_pr" - a bigger pixrect for the type indication & title */
   /* 3) rop type_indication pixrect into title pixrect */
   /* 4) rop original pixrect, centered, into title pixrect */

   if (image_type(&ip->menu_title) == IM_STRING)
      orig_pr = panel_pf_string(0,0,6,
                                image_string(&ip->menu_title),
                                image_font(&ip->menu_title),
                                0
                                );
   else
      orig_pr = image_pixrect(&ip->menu_title);

   ip->menu_max_width -= offset_x;
   ip->menu_max_width  = max(ip->menu_max_width, 6);
   ip->menu_max_width  = max(ip->menu_max_width, orig_pr->pr_width);

   title_pr = mem_create(type_width + ip->menu_max_width,
			 max(orig_pr->pr_height, type_height), 1);

   (void)pr_rop(title_pr, 
	  0, 
	  (title_pr->pr_height - type_height + 1) / 2,
	  type_width, type_height,
	  PIX_SRC, ip->menu_type_pr, 
	  0, 0
	 );

   title_offset  = (ip->menu_max_width - orig_pr->pr_width) / 2;
   title_offset  = max(title_offset, 22);
   (void)pr_rop(title_pr, 
	  title_offset, 
	  title_pr->pr_height - orig_pr->pr_height,
	  orig_pr->pr_width, orig_pr->pr_height,
	  PIX_SRC, orig_pr, 0, 0);

   /* free the old title */
   if (mp->m_imagedata)
      (void)prs_destroy((Pixrect *) LINT_CAST(mp->m_imagedata));

   mp->m_imagetype = MENU_GRAPHIC;
   mp->m_imagedata = (caddr_t) title_pr; 

   /* free the intermediate pixrect if allocated */
   if (image_type(&ip->menu_title) == IM_STRING)
      (void)prs_destroy(orig_pr);

   return 1;
} /* re_create_menu_title */


static int
re_create_menu_items(ip)
register panel_item_handle	ip;
{
   register struct menu *mp = ip->menu;      /* the menu to be altered */
   register int          i, which_value;     /* counter */
   Pixrect       	*orig_pr;            
   Pixrect       	*pr;                 
   int			offset_x = ip->menu_mark_width + 6;
   int			offset_y = ip->menu_mark_height;
   register panel_image_handle   image;

   /* free the existing items */
   panel_free_menu_items(mp);
   
   mp->m_itemcount = ip->menu_last + 1;
   mp->m_next      = (struct menu *) NULL;
   
   /* Nothing to allocate, if no menu choices */

   if (ip->menu_last < 0) {
       mp->m_items = NULL;
       return 1;
   }

   mp->m_items = (struct menuitem *) 
	LINT_CAST(calloc((u_int) (ip->menu_last + 1), sizeof(struct menuitem)));

   if (mp->m_items == NULL)
      return 0;

   /* make each line of the menu... */
   ip->menu_max_width = ((Pixrect *) LINT_CAST(mp->m_imagedata))->pr_width;
   for (i = which_value = 0; i <= ip->menu_last; i++) {
      image = &(ip->menu_choices[i]);
      switch (image_type(image)) {
	 case IM_STRING:
	    /* create a pixrect from the (string, font) pair */
            pr = panel_pf_string(offset_x, 6, offset_y,
                                 image_string(image),
                                 image_font(image),
                                 image_bold(image)
                                 );
	    break;

	 case IM_PIXRECT:
	    /* create a bigger pixrect for the mark, then */
	    /* bottom adjust the original pixrect in the bigger pixrect */
	    orig_pr = image_pixrect(image);
	    pr      = mem_create(orig_pr->pr_width + offset_x,
			         max(orig_pr->pr_height, offset_y), 1);
	    (void)pr_rop(pr, 
		   offset_x, 
		   pr->pr_height - orig_pr->pr_height,
		   orig_pr->pr_width, orig_pr->pr_height,
		   PIX_SRC, orig_pr, 0, 0);
	    break;
      }
      if (pr->pr_width > ip->menu_max_width)
	 ip->menu_max_width = pr->pr_width;

      if (show_menu_mark(ip))
         /* draw the off menu mark, centered in y direction  */
	 (void)pr_rop(pr, 
		0, (pr->pr_height - ip->menu_mark_off->pr_height + 1) / 2,
	        ip->menu_mark_off->pr_width, ip->menu_mark_off->pr_height, 
		PIX_SRC, ip->menu_mark_off, 
		0, 0
	       );

      mp->m_items[i].mi_imagetype = MENU_GRAPHIC;
      mp->m_items[i].mi_imagedata = (caddr_t) pr;

      /* menu returns either value specified or ordinal value of choice */
      if (ip->menu_values) {
	 mp->m_items[i].mi_data = (caddr_t) ip->menu_values[which_value];
	 if (ip->menu_values[which_value + 1])
	    which_value++;
      } else
	 mp->m_items[i].mi_data = (caddr_t) i;
   }

   return 1;
} /* re_create_menu_items */



/*****************************************************************************/
/* panel_pf_string                                                           */
/* panel_pf_string malloc()'s a pixrect just large enough to hold the string */
/* string drawn in the font font.  If font is NULL, the default font is used.*/
/* The string is offset left_offset pixels from the left edge.               */
/* The pixrect is at least min_height pixels high.                           */
/*****************************************************************************/

static Pixrect *
panel_pf_string(left_offset, left_extra, min_height,
                string, font, bold_desired)
int             	left_offset,left_extra;
char           		*string;
register Pixfont 	*font;
int			bold_desired;
{
   struct pr_prpos where;	/* where to write the string */
   struct pr_size size;		/* size of the pixrect */
   register short new_font = FALSE;

   if (!font) {
      new_font = TRUE;
      font = pw_pfsysopen();
   }
      
   size = pf_textwidth(strlen(string), font, string);

   /* allow for 1 pixel slop */
   where.pr = mem_create(size.x + left_offset + left_extra + 1, 
			 max(size.y, min_height) + 1, 
			 1);
   if (!where.pr) {
      if (new_font)
         (void)pw_pfsysclose();
      
      return (NULL);
   }

   where.pos.x = left_offset - font->pf_char[*string].pc_home.x;

   /* center in the y direction */
   where.pos.y = where.pr->pr_height - 
		 ((where.pr->pr_height - size.y) / 2 +
		  (size.y - panel_fonthome(font)));

   (void)pf_text(where, PIX_SRC, font, string);
   if (bold_desired) {
      where.pos.x++;
      (void)pf_text(where, PIX_SRC | PIX_DST, font, string);
   }

   if (new_font)
      (void)pw_pfsysclose();
      
   return (where.pr);
} 
