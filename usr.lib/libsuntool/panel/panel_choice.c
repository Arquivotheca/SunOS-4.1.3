#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)panel_choice.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*****************************************************************************/
/*                          panel_choice.c                                   */
/*            Copyright (c) 1985 by Sun Microsystems, Inc.                   */
/*****************************************************************************/

#include <suntool/panel_impl.h>
#include <sundev/kbd.h>
#include "sunwindow/sv_malloc.h"

static short check_off_image[] = {
#include <images/panel_check_off.pr>
};
static mpr_static(panel_check_off, 16, 16, 1, check_off_image);

static short check_on_image[] = {
#include <images/panel_check_on.pr>
};
static mpr_static(panel_check_on, 16, 16, 1, check_on_image);

static short choice_off_image[] = {
#include <images/panel_choice_off.pr>
};
static mpr_static(panel_choice_off, 16, 16, 1, choice_off_image);

static short choice_on_image[] = {
#include <images/panel_choice_on.pr>
};
static mpr_static(panel_choice_on, 16, 16, 1, choice_on_image);

#define NULL_CHOICE	-1
#define	MARK_XOFFSET	3		/* # of pixels to leave after a mark */
#define	CHOICE_X_GAP	10		/* # of x pixels between choices */
#define	CHOICE_Y_GAP	5		/* # of x pixels between choices */


/* assume 8 bits per byte, so byte for nth
 * element is n/8, bit within that byte is
 * defined by the loworder three bits of n.
 */
#define WORD(n)         (n >> 5)        /* word for element n */
#define BIT(n)          (n & 0x1F)      /* bit in word for element n */

/* Create a set with n elements.
 * Clear a set with n elements.
 * Copy n elements from one set to another
 */
#define	CREATE_SET(n)		\
    ((u_int *) LINT_CAST(calloc((u_int) (WORD(n) + 1), sizeof(u_int))))

#define	CLEAR_SET(set, n)	\
    (bzero((char *) (set), (int) (WORD(n) + 1) * sizeof(u_int)))

#define	COPY_SET(from_set, to_set, n)	\
    (bcopy((char *) (from_set), (char *) (to_set), (int) ((WORD(n) + 1) * sizeof(u_int))))

/* Add a choice by or-ing in the correct bit.
 * Remove a choice by and-ing out the correct bit.
 */
#define ADD_CHOICE(set, n)	((set)[WORD(n)] |= (1 << BIT(n)))
#define REMOVE_CHOICE(set, n)	((set)[WORD(n)] &= ~(1 << BIT(n)))

/* See if the nth bit is on */
#define IN(set, n)		(((set)[WORD(n)] >> BIT(n)) & 01)

#define	EACH_CHOICE(set, last_element, n)	\
   for ((n) = 0; (n) <= (last_element); (n)++) \
      if (IN(set, n))


#define choice_dp(ip)		(choice_data *) LINT_CAST((ip)->data)


static	begin_preview(), update_preview(), cancel_preview(), accept_preview(),
	accept_menu(),
	paint(),
	destroy(),
	set_attr(),
	layout(),
	sync_menu();
	
static void     update_item_rect();
static caddr_t	get_attr();
static int	choice_number();
static u_int 	choice_value();

static struct panel_ops ops  =  {
   panel_default_handle_event,		/* handle_event() */
   begin_preview,			/* begin_preview() */
   update_preview,			/* update_preview() */
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
   layout				/* layout() */
};

static struct pr_size image_size();

typedef struct choice_data {		     /* data for a choice item */
   struct {
      unsigned gap_set      	: 1;
      unsigned feedback_set 	: 1;
      unsigned choice_fixed 	: 1;
      unsigned menu_dirty	: 1;
   } status;
   int	 		current;             /* current choice */
   int	 		actual;	     	     /* actual value of current */
   u_int		*value;		     /* bit field of choices */
   int	 	 	last;		     /* last possible choice */
   Panel_setting	 display_level;	     /* NONE, CURRENT, ALL */
   int		 choose_one;	     /* only one choice allowed */
   Panel_setting	 feedback;	     /* MARKED, INVERTED, NONE */
   struct panel_image 	*choices;	     /* each choice */
   Rect 		*choice_rects;	     /* each choice rect */
   Rect 		*mark_rects;	     /* each mark rect */
   int			 choice_gap;	     /* gap between choices */
   int		 	 choices_bold : 1;   /* bold/not choices strings */
   struct pixrect      **mark_on;	     /* selection mark images */
   struct pixrect      **mark_off;	     /* un-selected mark images */
}  choice_data;

/*****************************************************************************/
/* choice item create routine                                                */
/* Appends a new choice item to "panel".  Returns a pointer to the new item. */
/*****************************************************************************/

Panel_item
panel_choice(ip, avlist)
register panel_item_handle	ip;
Attr_avlist			avlist;
{
   register choice_data     *dp;    /* choice data */

   dp = (choice_data *) LINT_CAST(calloc(1, sizeof(choice_data)));
   if (!dp)
      return (NULL);

   ip->flags |= SHOW_MENU_MARK;
   /* default is menu on for choice items */
   if (!show_menu_set(ip->panel))
      ip->flags |= SHOW_MENU;

   ip->ops    = &ops;
   ip->data   = (caddr_t) dp;
   ip->item_type = PANEL_CHOICE_ITEM;

   /* initialize */
   dp->value         = CREATE_SET(1);	/* set with one choice */
   ADD_CHOICE(dp->value, 0);		/* default is choice 0 selected */
   dp->current       = NULL_CHOICE;	/* no current choice */
   dp->actual        = FALSE;		/* current choice is off */
   dp->last	     = 0;		/* last slot is slot zero */
   dp->display_level = PANEL_ALL;	/* default is display all choices */
   dp->choose_one    = TRUE;		/* check only one choice */
   dp->feedback      = PANEL_MARKED;	/* marked feedback */

   /* menu is dirty to start,
    * since it must be created to
    * match the default single choice
    * string.
    */
   dp->status.menu_dirty = TRUE;

   /* initialize choices to a single
    * string choice of "".
    * Note that we do not call layout_choices() here.
    * The default layout is now to not have the label and
    * single mark baselines align.  If we lower the label
    * at this point, and the client has fixed its position,
    * the baseline will not be realigned when choices are added.
    * So we settle for a bug in the initial state: label baseline does not
    * line up with single mark baseline.  This restores release 3.0
    * behavior.
    */
   dp->choices = 
       (panel_image_handle) LINT_CAST(sv_malloc(sizeof(struct panel_image)));
   panel_make_image(ip->panel->font, &dp->choices[0], IM_STRING, "", 
		    FALSE, FALSE);
   dp->choice_rects	= (Rect *) LINT_CAST(sv_malloc(sizeof(Rect)));
   dp->choice_rects[0]  = ip->value_rect;
   /* space between choices */
   dp->choice_gap =
       (ip->layout == PANEL_HORIZONTAL) ? CHOICE_X_GAP : CHOICE_Y_GAP;

   /* set up the default choice marks */
   dp->mark_on = (struct pixrect **) 
       LINT_CAST(malloc(sizeof(struct pixrect *)));
   dp->mark_off = (struct pixrect **) 
       LINT_CAST(sv_malloc(sizeof(struct pixrect *)));
   dp->mark_on[0]    = &panel_choice_on;/* for PANEL_MARKED feedback */
   dp->mark_off[0]   = &panel_choice_off;/* for PANEL_MARKED feedback */
   dp->mark_rects    = (Rect *) LINT_CAST(sv_malloc(sizeof(Rect)));
   dp->mark_rects[0] = dp->choice_rects[0];
   dp->mark_rects[0].r_width = dp->mark_on[0]->pr_width;
   dp->mark_rects[0].r_height = dp->mark_on[0]->pr_height;

   /* update the value and item rect */
   update_item_rect(ip);
 
   if (!set_attr(ip, avlist))
      return NULL;

   /* NOTE: append should be done in central create routine. */
   return (Panel_item) panel_append(ip);

} /* panel_choice */


static int
set_attr(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register Panel_attribute	attr;	/* each attribute */
   register choice_data		*dp = choice_dp(ip);

   int                    choices_type	= -1;   /* IM_STRING or IM_PIXRECT */
   caddr_t               *choices;		/* choices array */
   struct pixfont       **choices_fonts = NULL;	/* choices fonts */
   struct pixrect	**mark_images, **nomark_images;
   short		  mark_images_set	= FALSE;
   short		  nomark_images_set	= FALSE;

   Attr_avlist		 orig_avlist = avlist;	/* original avlist */
   u_int		 value;			/* initial value */
   int			 which_choice, which_arg;
   short		 choices_changed	= FALSE;
   short		 choices_moved		= FALSE;

   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_CHOICE_STRINGS:
	 case PANEL_CHOICE_IMAGES:
	    choices_type = attr == 
	       PANEL_CHOICE_STRINGS ? IM_STRING : IM_PIXRECT;
	    choices = avlist;	/* base of the array */
	    while (*avlist++);	/* skip past the list */
	    break;

	 case PANEL_CHOICE_STRING:
	 case PANEL_CHOICE_IMAGE:
	    which_choice = (int) *avlist++;
	    if (!modify_choice(ip, attr == PANEL_CHOICE_STRING ? 
			  IM_STRING : IM_PIXRECT, which_choice, *avlist++))
	       return 0;
	    choices_changed = TRUE;
	    break;

	 case PANEL_CHOICE_FONTS:
	    choices_fonts = (struct pixfont **) avlist;
	    while (*avlist++);	/* skip past the list */
	    break;

	 case PANEL_CHOICES_BOLD:
	    dp->choices_bold = (*avlist++ != 0);
	    for (which_choice = 0; which_choice <= dp->last; which_choice++)
	       if (is_string(&dp->choices[which_choice]))
		  image_bold(&dp->choices[which_choice]) = dp->choices_bold;
	    choices_changed = TRUE;
	    break;

	 case PANEL_CHOICE_OFFSET:
	    dp->choice_gap = (int) *avlist++;
	    dp->status.gap_set = TRUE;
	    choices_changed = TRUE;
	    break;

	 case PANEL_CHOOSE_ONE:
	    dp->choose_one = (int) *avlist++;
	    CLEAR_SET(dp->value,  dp->last);
	    if (dp->choose_one) {
	       ip->item_type = PANEL_CHOICE_ITEM;
	       ADD_CHOICE(dp->value, 1);
	    } else {
	       ip->item_type = PANEL_TOGGLE_ITEM;
	       /* 
	       Note that this depends on the fact that PANEL_CHOOSE_ONE
	       can only be specified at creat time, as part of the
	       PANEL_TOGGLE macro.  So no choices have been set yet.
	       */
               dp->mark_on[0]    = &panel_check_on;
               dp->mark_off[0]   = &panel_check_off;
               /* force re-layout and resize of rect */
               choices_changed = TRUE;
	    }
	    break;

	 case PANEL_LAYOUT:
	    avlist++;
	    if (!dp->status.gap_set)
	       dp->choice_gap = ip->layout == PANEL_HORIZONTAL ?
				CHOICE_X_GAP : CHOICE_Y_GAP;
	    choices_changed = TRUE;
	    break;

	 case PANEL_FEEDBACK:
	    dp->feedback = (Panel_setting) *avlist++;
	    dp->status.feedback_set = TRUE;
	    choices_changed = TRUE;
	    break;

	 case PANEL_MARK_IMAGES:
	    mark_images = (struct pixrect **) avlist;
	    mark_images_set = TRUE;
	    while (*avlist++);
	    break;

	 case PANEL_NOMARK_IMAGES:
	    nomark_images = (struct pixrect **) avlist;
	    nomark_images_set = TRUE;
	    while (*avlist++);
	    break;

	 case PANEL_MARK_IMAGE:
	 case PANEL_NOMARK_IMAGE:
	    avlist++;
	    if (!*avlist)
	       *avlist = (caddr_t) &panel_empty_pr;
	    avlist++;
	    break;

	 case PANEL_SHOW_MENU:
	    if ((int) *avlist++)
               ip->flags |= SHOW_MENU;
            else
               ip->flags &= ~SHOW_MENU;
	    break;

	 default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   if (set(choices_type)) {
      if (!re_alloc_choices(ip, choices_type, choices))
	 return 0;
      choices_changed = TRUE;
   }

   if (choices_fonts) {
      for (which_choice = which_arg = 0; which_choice <= dp->last; 
	   which_choice++, which_arg += choices_fonts[which_arg + 1] ? 1 : 0)
	 if (is_string(&dp->choices[which_choice]))
	    image_font(&dp->choices[which_choice]) = choices_fonts[which_arg];
      choices_changed = TRUE;
   }

   if (mark_images_set) {
      /* reset to empty pixrect if no list */
      if (!mark_images[0])
	  for (which_choice = 0; which_choice <= dp->last; which_choice++)
	     dp->mark_on[which_choice] = &panel_empty_pr;
      else
	 for (which_choice = which_arg = 0; which_choice <= dp->last; 
	      which_choice++, which_arg += mark_images[which_arg + 1] ? 1 : 0)
	    dp->mark_on[which_choice] = mark_images[which_arg];
      choices_changed = TRUE;
   }

   if (nomark_images_set) {
      /* reset to empty pixrect if no list */
      if (!nomark_images[0])
	  for (which_choice = 0; which_choice <= dp->last; which_choice++)
	     dp->mark_off[which_choice] = &panel_empty_pr;
      else
	 for (which_choice = which_arg = 0; which_choice <= dp->last; 
	      which_choice++, which_arg += nomark_images[which_arg + 1] ? 1 : 0)
	    dp->mark_off[which_choice] = nomark_images[which_arg];
      choices_changed = TRUE;
   }

   /* now set things that depend on the new list 
    * of choices or the attributes that were set above.
    */
   avlist = orig_avlist;
   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_MARK_IMAGE:
	 case PANEL_NOMARK_IMAGE:
	    which_choice = (int) LINT_CAST(*avlist++);
	    if (which_choice < 0 || which_choice > dp->last)
	       return 0;
	    dp->mark_on[which_choice] = (struct pixrect *) LINT_CAST(*avlist++);
	    /* may have to relayout the choices */
	    choices_changed = TRUE;
	    break;

	 case PANEL_CHOICE_FONT:
	    which_choice = (int) *avlist++;
	    if (which_choice < 0 || which_choice > dp->last)
	       return 0;
	    if (is_string(&dp->choices[which_choice]))
	       image_font(&dp->choices[which_choice]) = (Pixfont *) *avlist++;
	    choices_changed = TRUE;
	    break;

	 case PANEL_VALUE:
	    value = (u_int) *avlist++;
	    if (dp->choose_one) {
	       if(value <= dp->last) {
		  CLEAR_SET(dp->value,  dp->last);
		  ADD_CHOICE(dp->value, value);
	       }
	    } else
	       dp->value[0] = value;
	    break;
	 
	 case PANEL_TOGGLE_VALUE:
	    which_choice = (int) *avlist++;
	    if (which_choice < 0 || which_choice > dp->last)
	       return 0;
	    if (*avlist++)
	       ADD_CHOICE(dp->value, which_choice);
	    else
	       REMOVE_CHOICE(dp->value, which_choice);
	    break;

	 case PANEL_DISPLAY_LEVEL:
	    dp->display_level = (Panel_setting) *avlist++;

	    /* ignore PANEL_CURRENT for toggles */
	    if (dp->display_level == PANEL_CURRENT && !dp->choose_one)
	       dp->display_level = PANEL_ALL;

	    /* set the default feedback */
	    if (!dp->status.feedback_set)
	       switch (dp->display_level) {
		  case PANEL_NONE:
		  case PANEL_CURRENT:
		     dp->feedback = PANEL_NONE;
		     break;

		  default:
		     dp->feedback = PANEL_MARKED;
	       }
	    choices_changed = TRUE;
	    break;

	 default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }

   /* layout the choices if they have
    * changed and no choice or mark has a fixed position.
    */
   if (choices_changed) {
      dp->status.menu_dirty = TRUE;
      if (!dp->status.choice_fixed)
         layout_choices(ip);
    }

   /* move any specified choices */
   choices_moved = move_specified(ip, orig_avlist);

   if (choices_changed || choices_moved)
      update_item_rect(ip);

   return 1;

} /* set_attr */


static void
update_item_rect(ip)
panel_item_handle       ip;
{
   update_value_rect(ip);
   ip->rect = panel_enclosing_rect(&ip->label_rect, &ip->value_rect);
}
 
 
/* re_alloc_choices allocates dp->choices from choices.  The
 * old info is reused and then freed.
 */
static int
re_alloc_choices(ip, type, choices)
register panel_item_handle 	ip;		/* the item */
int 				type;		/* IM_STRING or IM_PIXRECT */
caddr_t 			choices[];	/* each choice */
{
   register choice_data *dp = choice_dp(ip);
   panel_image_handle 		 old_choices		= dp->choices;
   int 			 	old_last		= dp->last;
   u_int			*old_value		= dp->value;
   Rect 			*old_choice_rects	= dp->choice_rects;
   Rect 			*old_mark_rects		= dp->mark_rects;
   struct pixrect 		**old_mark_on		= dp->mark_on;
   struct pixrect 		**old_mark_off		= dp->mark_off;
   register int num_choices, i, old_slot;	/* counters */
   struct pr_size		size;


   /* count the # of choices */
   for (num_choices = 0; choices[num_choices]; num_choices++);

   /* allocate the panel_image[] storage for the choices */
   if ((dp->choices = (panel_image_handle)
      LINT_CAST(calloc((u_int) num_choices, sizeof(struct panel_image)))) == NULL)
      return 0;

   /* allocate the enclosing rectangles for each choice */
   if ((dp->choice_rects = (Rect *)
      LINT_CAST(calloc((u_int) num_choices, sizeof(Rect)))) == NULL)
      return 0;

   /* allocate the enclosing rectangles for each mark */
   if ((dp->mark_rects = (Rect *)
      LINT_CAST(calloc((u_int) num_choices, sizeof(Rect)))) == NULL)
      return 0;

   /* allocate the on mark images */
   if ((dp->mark_on = (struct pixrect **)
      LINT_CAST(calloc((u_int) num_choices, sizeof(struct pixrect *)))) == NULL)
      return 0;

   /* allocate the off mark images */
   if ((dp->mark_off = (struct pixrect **)
      LINT_CAST(calloc((u_int) num_choices, sizeof(struct pixrect *)))) == NULL)
      return 0;

   dp->last = num_choices - 1;	/* last slot used in base[] */

   /* allocate the value set */
   if ((dp->value = CREATE_SET(dp->last)) == NULL)
      return 0;

   /* copy the old values */
   COPY_SET(old_value, dp->value, min(dp->last, old_last));

   /* Copy the choices to the allocated storage.
    * Here we reuse the old mark and font info
    * if it was given.
    */
   for (i = 0; i <= dp->last; i++) {
      old_slot = (i <= old_last) ? i : old_last;
      dp->mark_on[i] = old_mark_on[old_slot];
      dp->mark_off[i] = old_mark_off[old_slot];
      dp->mark_rects[i] = old_mark_rects[old_slot];
      dp->choice_rects[i] = old_choice_rects[old_slot];
      size = panel_make_image(is_string(&old_choices[old_slot]) ? 
			image_font(&old_choices[old_slot]) : ip->panel->font,
			&dp->choices[i], type, choices[i], 
			 dp->choices_bold, FALSE);
      dp->choice_rects[i].r_width = size.x;
      dp->choice_rects[i].r_height = size.y;
   }

   /* if there are fewer choices now,
    * and this is not a toggle item,
    * make sure the value is <= the number
    * of the last choice.
    */
   if (dp->choose_one && (dp->last < old_last) && 
       (choice_number(old_value, old_last) > dp->last)) {
          CLEAR_SET(dp->value, dp->last);
          ADD_CHOICE(dp->value, dp->last);
   }
   
   /* now free the old info */
   (void)panel_free_choices(old_choices, 0, old_last);
   free((char *) old_choice_rects);
   free((char *) old_mark_rects);
   free((char *) old_mark_on);
   free((char *) old_mark_off);
   free((char *) old_value);

   return 1;
} /* re_alloc_choices */


/* modify_choice modifies the specified choice string or image.
 * If the specified choice does not already exist, the list of choices
 * is extended by adding empty choice strings.
 */
static int
modify_choice(ip, type, which_choice, choice_info)
register panel_item_handle 	ip;		/* the item */
int 				type;		/* IM_STRING or IM_PIXRECT */
int				which_choice;	/* choice to change */
caddr_t 			choice_info; /* new choice string or pixrect */
{
   register choice_data *dp = choice_dp(ip);
   panel_image_handle 		 old_choices	= dp->choices;
   char				*old_string 	= NULL;
   int 				 old_last	= dp->last;
   u_int			*old_value	= dp->value;
   Rect 			*old_choice_rects = dp->choice_rects;
   Rect 			*old_mark_rects   = dp->mark_rects;
   struct pixrect 		**old_mark_on     = dp->mark_on;
   struct pixrect 		**old_mark_off    = dp->mark_off;
   register int num_choices, i, old_slot;	/* counters */
   struct pr_size		size;


   /* expand the list if not big enough */
   if (which_choice > dp->last) {
      num_choices = which_choice + 1;
      /* allocate the panel_image[] storage for the choices */
      if ((dp->choices = (panel_image_handle)
	 LINT_CAST(calloc((u_int) num_choices, sizeof(struct panel_image)))) == NULL)
	 return 0;

      /* allocate the enclosing rectangles for each choice */
      if ((dp->choice_rects = (Rect *)
	 LINT_CAST(calloc((u_int) num_choices, sizeof(Rect)))) == NULL)
	 return 0;

      /* allocate the enclosing rectangles for each mark */
      if ((dp->mark_rects = (Rect *)
	 LINT_CAST(calloc((u_int) num_choices, sizeof(Rect)))) == NULL)
	 return 0;

      /* allocate the on mark images */
      if ((dp->mark_on = (struct pixrect **)
	 LINT_CAST(calloc((u_int) num_choices, sizeof(struct pixrect *)))) == NULL)
	 return 0;

      /* allocate the off mark images */
      if ((dp->mark_off = (struct pixrect **)
	 LINT_CAST(calloc((u_int) num_choices, sizeof(struct pixrect *)))) == NULL)
	 return 0;

      dp->last = num_choices - 1;	/* last slot used in choices[] */

      /* allocate the value set */
      if ((dp->value = CREATE_SET(dp->last)) == NULL)
         return 0;

      /* copy the old values */
      COPY_SET(old_value, dp->value, min(dp->last, old_last));

      /* Copy the choices to the allocated storage.
       * Here we reuse the old mark and font info
       * if it was given.
       */
      for (i = 0; i <= dp->last; i++) {
	 old_slot = (i <= old_last) ? i : old_last;
	 dp->mark_on[i] = old_mark_on[old_slot];
	 dp->mark_off[i] = old_mark_off[old_slot];
	 dp->mark_rects[i] = old_mark_rects[old_slot];
	 dp->choice_rects[i] = old_choice_rects[old_slot];
	 if (i <= old_last)
	    dp->choices[i] = old_choices[old_slot];
	 else {
	    size = panel_make_image(is_string(&old_choices[old_slot]) ? 
			   image_font(&old_choices[old_slot]) : ip->panel->font,
			   &dp->choices[i], IM_STRING, "", 
			   dp->choices_bold, FALSE);
	    dp->choice_rects[i].r_width = size.x;
	    dp->choice_rects[i].r_height = size.y;
	 }
      }

   }

   if (is_string(&dp->choices[which_choice]))
      old_string = image_string(&dp->choices[which_choice]);

   size = panel_make_image(is_string(&dp->choices[which_choice]) ? 
		            image_font(&dp->choices[which_choice]) : ip->panel->font,
		           &dp->choices[which_choice], type, choice_info,
			   dp->choices_bold, FALSE);
   dp->choice_rects[which_choice].r_width = size.x;
   dp->choice_rects[which_choice].r_height = size.y;

   if (old_string)
      free(old_string);

   if (dp->last != old_last) {
      /* now free the old info */
      /* if new array is smaller, free the unused strings */
      /* in any case, free old_choices */
      (void)panel_free_choices(old_choices, (int) (dp->last + 1), old_last);
      free((char *) old_choice_rects);
      free((char *) old_mark_rects);
      free((char *) old_mark_on);
      free((char *) old_mark_off);
      free((char *) old_value);
   }

   return 1;
} /* modify_choice */


static
layout_choices(ip)
register panel_item_handle 	ip;			/* the item */
/* layout_choices lays out the choices and marks in ip.
*/
{
   register struct choice_data *dp;	/* choice data */
   register int i;			/* counter */
   struct panel_image *image;		/* each choice image */
   struct pr_size size;			/* each choice size */
   Rect *rect;				/* each choice rect */
   Rect *mark_rect;			/* each mark rect */
   int left, top;			/* corner of each choice */
   int above_baseline;			/* amount above baseline */
   register int max_above;		/* max. amount above baseline */

   dp = choice_dp(ip);

   /* initialize above baseline offset */
   max_above = 0;

   /* make sure the label is in the right place.
    * This is a hack to account for the baseline label
    * adjustment done below.
    */
   panel_fix_label_position(ip);

   /* account for label size */
   if (ip->label_rect.r_top == ip->value_rect.r_top) {
      size = image_size(&ip->label, &above_baseline);
      max_above = max(max_above, above_baseline);
   }

   /* initalize the value width & height */
   ip->value_rect.r_width = 0;
   ip->value_rect.r_height = 0;

   left = ip->value_rect.r_left;

   /* layout each choice & mark */
   for (i = 0; i <= dp->last; i++) {
      image = &(dp->choices[i]);
      rect = &(dp->choice_rects[i]);
      mark_rect = &(dp->mark_rects[i]);
      size = image_size(image, &above_baseline);

      /* compute maximum above baseline */
      max_above =  max(max_above, above_baseline);

      /* construct the mark rect */
      rect_construct(mark_rect, left, 0, MARK_XOFFSET + 
		     max(dp->mark_on[i]->pr_width, dp->mark_off[i]->pr_width),
		     max(dp->mark_on[i]->pr_height, dp->mark_off[i]->pr_height)
		    );

      /* adjust for the mark, if any */
      if (dp->feedback == PANEL_MARKED) {
	 /* update maximum above baseline */
	 max_above =  max(max_above, mark_rect->r_height);
      }

      rect_construct(rect, dp->feedback == PANEL_MARKED ?
		     rect_right(mark_rect) + 1 : left,
		     0, size.x, size.y);

      /* compute the position of the next choice */
      if (dp->display_level == PANEL_ALL && ip->layout == PANEL_HORIZONTAL)
	 left = rect_right(rect) + 1 + dp->choice_gap;
   }

   /* compute the top of each mark and choice rect */
   top = ip->value_rect.r_top;

   /* adjust label rect height only if horizontal with value rect */
   if (ip->label_rect.r_top == ip->value_rect.r_top) {
      size = image_size(&ip->label, &above_baseline);
      ip->label_rect.r_top += max_above - above_baseline;
   }

   for (i = 0; i <= dp->last; i++) {
      size = image_size(&dp->choices[i], &above_baseline);
      rect = &dp->choice_rects[i];
      rect->r_top = top + max_above - above_baseline;
      mark_rect = &dp->mark_rects[i];
      mark_rect->r_top = top + max_above - mark_rect->r_height;
      if (dp->display_level == PANEL_ALL && ip->layout == PANEL_VERTICAL)
	 /* move down one row */
	 top = rect_bottom(rect) + 1 + dp->choice_gap;
   }
} /* layout_choices */


/* move_specified moves the specified choices marks in ip.
 * If any choices are moved, TRUE is returned.
 */
static int
move_specified(ip, avlist)
register panel_item_handle	ip;
register Attr_avlist		avlist;
{
   register choice_data	*dp = choice_dp(ip);
   register Panel_attribute	attr;
   register int 	which_choice;		/* index of current choice */
   register int		i;			/* counter */
   int			*xs;			/* choice x coordinates */
   int			*ys;			/* choice y coordinates */
   int			*mark_xs;		/* mark x coordinates */
   int			*mark_ys;		/* mark y coordinates */
   int			moved = FALSE;		/* TRUE if moved */

   while (attr = (Panel_attribute) *avlist++) {
      switch (attr) {
	 case PANEL_CHOICE_X:
	    i = (int)*avlist++;
	    dp->choice_rects[i].r_left = (int)*avlist++;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_X_FIXED;
	    moved = TRUE;
	    break;

	 case PANEL_CHOICE_Y:
	    i = (int)*avlist++;
	    dp->choice_rects[i].r_top = (int)*avlist++;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_Y_FIXED;
	    moved = TRUE;
	    break;

	 case PANEL_MARK_X:
	    i = (int)*avlist++;
	    dp->mark_rects[i].r_left = (int)*avlist++;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_X_FIXED;
	    moved = TRUE;
	    break;

	 case PANEL_MARK_Y:
	    i = (int)*avlist++;
	    dp->mark_rects[i].r_top = (int)*avlist++;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_Y_FIXED;
	    moved = TRUE;
	    break;

	 case PANEL_CHOICE_XS:
	    xs = (int *) avlist;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_X_FIXED;
	    if (xs[0])
	       for (which_choice = i = 0; which_choice <= dp->last; 
		    which_choice++, i += xs[i + 1] ? 1 : 0)
		  dp->choice_rects[which_choice].r_left = xs[i];
	    while (*avlist++);
	    moved = TRUE;
	    break;

	 case PANEL_CHOICE_YS:
	    ys = (int *) avlist;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_Y_FIXED;
	    if (ys[0])
	       for (which_choice = i = 0; which_choice <= dp->last; 
		    which_choice++, i += ys[i + 1] ? 1 : 0)
		  dp->choice_rects[which_choice].r_top = ys[i];
	    while (*avlist++);
	    moved = TRUE;
	    break;

	 case PANEL_MARK_XS:
	    mark_xs = (int *) avlist;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_X_FIXED;
	    if (mark_xs[0])
	       for (which_choice = i = 0; which_choice <= dp->last; 
		    which_choice++, i += mark_xs[i + 1] ? 1 : 0)
		  dp->mark_rects[which_choice].r_left = mark_xs[i];
	    while (*avlist++);
	    moved = TRUE;
	    break;

	 case PANEL_MARK_YS:
	    mark_ys = (int *) avlist;
	    dp->status.choice_fixed = TRUE;
	    ip->flags |= VALUE_Y_FIXED;
	    if (mark_ys[0])
	       for (which_choice = i = 0; which_choice <= dp->last; 
		    which_choice++, i += mark_ys[i + 1] ? 1 : 0)
		  dp->mark_rects[which_choice].r_top = mark_ys[i];
	    while (*avlist++);
	    moved = TRUE;
	    break;

	 default:
	    /* skip past what we don't care about */
	    avlist = attr_skip(attr, avlist);
	    break;
      }
   }
   return moved;
} /* move_specified */


static
update_value_rect(ip)
panel_item_handle	ip;
/* update_value_rect computes the width & height of the value rect
   to enclose all of the choices & marks.
*/
{
   register choice_data	*dp = choice_dp(ip);
   Rect			rect;
   register int		i;

   if (dp->display_level != PANEL_NONE) {
      rect = ip->value_rect;
      rect.r_width = rect.r_height = 0;
      for (i = 0; i <= dp->last; i++)
	 rect = panel_enclosing_rect(&rect, &dp->choice_rects[i]);

      if (dp->feedback == PANEL_MARKED)
	 for (i = 0; i <= dp->last; i++)
	    rect = panel_enclosing_rect(&rect, &dp->mark_rects[i]);
      ip->value_rect = rect;
   }
} /* update_value_rect */


static
pr_ycenter(dest, src)
struct pixrect *dest, *src;
/* pr_ycenter writes src into dest centered in the y direction.
*/
{
   (void)pr_rop(dest, 0, (dest->pr_height - src->pr_height + 1) / 2,
	  src->pr_width, src->pr_height, PIX_SRC, src, 0, 0);
} /* pr_ycenter */


/* ops vector routines */

static
destroy(dp)
register choice_data *dp;
{
   (void)panel_free_choices(dp->choices, 0, (int) dp->last);
   free((char *) dp->choice_rects);
   free((char *) dp->mark_rects);
   free((char *) dp->mark_on);
   free((char *) dp->mark_off);
   free((char *) dp->value);
   free((char *) dp);
} /* destroy */


static
layout(ip, deltas)
panel_item_handle	ip;
Rect			*deltas;
{
   register choice_data *dp = choice_dp(ip);
   register int 	i;

   /* bump each choice & mark rect */
   for (i = 0; i <= dp->last; i++) {
      dp->choice_rects[i].r_left += deltas->r_left;
      dp->choice_rects[i].r_top += deltas->r_top;
      dp->mark_rects[i].r_left += deltas->r_left;
      dp->mark_rects[i].r_top += deltas->r_top;
   }
} /* layout */


static
paint(ip)
register panel_item_handle  ip;
{
   register choice_data *dp = choice_dp(ip);
   register int i;

   /* paint the label */
   (void)panel_paint_image(ip->panel, &ip->label, &ip->label_rect,PIX_COLOR(ip->color_index)); 

   /* paint the choices */
   switch (dp->display_level) {
      case PANEL_NONE:		/* don't draw the choices */
	 break;

      case PANEL_CURRENT:		/* draw the current choice */
	 if (dp->choose_one)
	    paint_choice(ip->panel, dp, choice_number(dp->value, dp->last),ip);
	 else
	    /* just draw the first possible choice */
	    paint_choice(ip->panel, dp, 0,ip);

	 break;

      case PANEL_ALL:		/* draw all the choices */
	 /* draw each choice */
	 for (i = 0; i <= dp->last; i++)
	    paint_choice(ip->panel, dp, i,ip);
	 break;
   }
   /* indicate the current choices */
   EACH_CHOICE(dp->value, dp->last, i)
      update_display(ip, i, TRUE);
} /* paint */


static
begin_preview(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   preview_choice(ip, find_choice(ip, event), event);
}


static
preview_choice(ip, new, event)
panel_item_handle	ip;
int			new;	/* new choice # to preview */
Event 			*event;
{
   register choice_data *dp = choice_dp(ip);
   int			new_is_on;


   /* no change */
   if (new == dp->current)
      return;

   /* if no new choice cancel the current choice & restore the value */
   if (new == NULL_CHOICE) {
      cancel_preview(ip, event);
      return;
   }

   new_is_on = IN(dp->value, new);
   switch (dp->choose_one) {
      case TRUE:
	 /* if no current, un-mark the actual marked choice */
	 if (dp->current == NULL_CHOICE)
	    update_display(ip, choice_number(dp->value, dp->last), FALSE);
	 else
	    update_display(ip, dp->current, FALSE);

	 /* mark the new choice */
	 update_display(ip, new, TRUE);
	 break;

      case FALSE:
	 /* restore the current choice */
	 update_display(ip, dp->current, dp->actual);

	 /* toggle the mark for new */
	 update_display(ip, new, !new_is_on);
	 break;
   }
   dp->current = new;
   dp->actual = new_is_on;
}


static
update_preview(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   begin_preview(ip, event);
}


/* ARGSUSED */
static
cancel_preview(ip, event)
panel_item_handle  	ip;
Event 			*event;
{
   register choice_data *dp = choice_dp(ip);

   /* restore the current choice */
   update_display(ip, dp->current, dp->actual);

   /* restore the value if modified */
   if (dp->choose_one && dp->current != NULL_CHOICE)
      update_display(ip, choice_number(dp->value, dp->last), TRUE);
   dp->current = NULL_CHOICE;
}


static
accept_preview(ip, event)
panel_item_handle  	ip;
Event 			*event;
{
   register choice_data *dp = choice_dp(ip);
   u_int		value;

   /* nothing to accept if no current choice */
   if (dp->current == NULL_CHOICE)
      return;

   /* remove current choice if only one choice allowed
    * modify the value if current is non-null
    */
   if (dp->choose_one) {
      CLEAR_SET(dp->value, dp->last);
      ADD_CHOICE(dp->value, dp->current);
   } else if (!dp->actual)
      ADD_CHOICE(dp->value, dp->current);
   else
      REMOVE_CHOICE(dp->value, dp->current);

   /* notify the client */
   value = choice_value(dp->choose_one, dp->value, dp->last);
   (*ip->notify)(ip, value, event);

   dp->current = NULL_CHOICE;
}


static
accept_menu(ip, event)
panel_item_handle	ip;
Event 			*event;
{
   register choice_data *dp = choice_dp(ip);
   int			new;

   /* Make sure the menu title reflects the label. */
   panel_sync_menu_title(ip);
   
   /* if choices have changed,
    * change the menu choices.
    */
   if (dp->status.menu_dirty && !ip->menu_status.choices_set) {
      panel_copy_menu_choices(ip, dp->choices, dp->last);
      dp->status.menu_dirty = FALSE;
   }
      
   /* cancel any current preview */
   cancel_preview(ip, event);

   /* let the user pick from the menu */
   new = display_menu(ip, event);

   /* preview & accept the new choice */
   preview_choice(ip, new, event);
   accept_preview(ip, event);
}


static int
find_choice(ip, event)
panel_item_handle  ip;
Event *event;
{
   register int		x	= event_x(event);	/* locator x */
   register int		y	= event_y(event);  	/* locator y */
   register choice_data	*dp	= choice_dp(ip);
   int			current_value;		/* current choice value */
   register int		i;			/* counter */

   current_value = choice_number(dp->value, dp->last);
   switch (dp->display_level) {
      case PANEL_NONE:
      case PANEL_CURRENT:
	 if (!rect_includespoint(&ip->rect, x, y))
	    return (NULL_CHOICE);

	 /* don't cycle if a multiple checklist */
	 if (!dp->choose_one)
	    return dp->last == 0 ? 0 : NULL_CHOICE;

	 if (event_shift_is_down(event)) 		/* choice backward */
	    return (current_value == 0 ? dp->last : current_value - 1);
	 else						/* choice forward */
	    return (current_value == dp->last ? 0 : current_value + 1);

      case PANEL_ALL:
	 if(rect_includespoint(&ip->label_rect, x, y) &&
	    (dp->choose_one || (!dp->choose_one && dp->last == 0)))
	    /* advance/retreat */
	    if (event_shift_is_down(event)) 		/* choice backward */
	       return (current_value == 0 ? dp->last : current_value - 1);
	    else					/* choice forward */
	       return (current_value == dp->last ? 0 : current_value + 1);

	 for (i = 0; i <= dp->last; i++)
	    if(rect_includespoint(&dp->choice_rects[i], x, y) ||
	       ((dp->feedback == PANEL_MARKED) &&
	        rect_includespoint(&dp->mark_rects[i], x, y)))
	       return (i);

	 return (NULL_CHOICE);

      default:	/* invalid display level */
	 return (NULL_CHOICE);
   }
} /* find_choice */


static int
display_menu(ip, event)
panel_item_handle	ip;
Event			*event;
/* display_menu modifies the menu of ip to indicate the current
   feedback (dp->value), displays the menu, allows the user to make a
   choice, and un-modifies the menu.
*/
{
   register choice_data *dp = choice_dp(ip);
   struct menuitem 	*mi;		/* selected item */
   struct pixrect 	*pr;		/* checked item */
   register int		i;		/* counter */

   if (show_menu_mark(ip)) {
      /* enhance each selected item */
      EACH_CHOICE(dp->value, dp->last, i) {
	 pr = (struct pixrect *)LINT_CAST(ip->menu->m_items[i].mi_imagedata);

	 /* clear the off mark */
	 (void)pr_rop(pr, 0, 0, ip->menu_mark_width, pr->pr_height,
		PIX_CLR, (struct pixrect *) 0, 0, 0); 
	 /* draw the on mark */
	 pr_ycenter(pr, ip->menu_mark_on);

      }
   }

   /* display the menu */
   mi = panel_menu_display(ip, event);
   /* restore the code in case the user hit MS_LEFT */
   event_id(event) = MS_RIGHT;

   if (show_menu_mark(ip)) {
      /* restore each enhanced item */
      EACH_CHOICE(dp->value, dp->last, i) {
	 pr = (struct pixrect *)LINT_CAST(ip->menu->m_items[i].mi_imagedata);
	 /* clear the on mark */
	 (void)pr_rop(pr, 0, 0, ip->menu_mark_width, pr->pr_height,
		PIX_CLR, (struct pixrect *) 0, 0, 0); 
	 /* draw the off mark */
	 pr_ycenter(pr, ip->menu_mark_off);
      }
   }

   return (mi == NULL ? NULL_CHOICE : (int) mi->mi_data);

}


static
paint_choice(panel, dp, which_choice,ip)
panel_handle     	panel;
register choice_data	*dp;
register int		which_choice;
panel_item_handle	ip;
/* paint_choice paints the choice which_choice.  The off mark is drawn 
   if dp->feedback is PANEL_MARKED.
*/
{
   if (dp->display_level == PANEL_NONE)		/* don't draw the choice */
      return;

   if (dp->feedback == PANEL_MARKED)
      /* draw the off mark */
        (void)panel_paint_pixrect(panel, dp->mark_off[which_choice], 
			 &dp->mark_rects[which_choice],PIX_COLOR(ip->color_index));

      /* draw the choice image */
        (void)panel_paint_image(panel, &dp->choices[which_choice], 
		       &dp->choice_rects[which_choice],PIX_COLOR(ip->color_index));
} /* paint_choice */


static
update_display(ip, which_choice, on)
register panel_item_handle	ip;
register int			which_choice;
int				on;
/* update_display updates the display to suggest or un-suggest which_choice
   depending on the value of on.
*/
{
   register choice_data	*dp = choice_dp(ip);
   register Rect  	*rp;
   register Rect  	*mark_rp;

   if (dp->display_level == PANEL_NONE || which_choice == NULL_CHOICE)
      return;

   if (on) {
      /* turn the choice on */
      rp = &dp->choice_rects[which_choice];
      mark_rp = &dp->mark_rects[which_choice];

      if (dp->display_level == PANEL_CURRENT) {	/* clear the old choice */
	 (void)panel_clear(ip->panel, &ip->value_rect);
	 paint_choice(ip->panel, dp, which_choice,ip);
      }

      switch (dp->feedback) {
	 case PANEL_INVERTED:
	    (void)panel_invert(ip->panel, rp);
	    break;

	    case PANEL_MARKED:
	       /* clear the off mark */
	       (void)panel_clear(ip->panel, mark_rp);
	       /* draw the on mark */
	       (void)panel_paint_pixrect(ip->panel, dp->mark_on[which_choice], mark_rp,
			PIX_COLOR(ip->color_index));
	       break;

	 case PANEL_NONE:
	    break;
      }
   } else  {
      /* turn the choice off */
      rp = &dp->choice_rects[which_choice];
      mark_rp = &dp->mark_rects[which_choice];

      /* un-mark/invert old */
      switch (dp->feedback) {
	 case PANEL_INVERTED:
	    (void)panel_invert(ip->panel, rp);
	    break;

	 case PANEL_MARKED:
	    /* clear the on mark */
	    (void)panel_clear(ip->panel, mark_rp);
	    /* draw the off mark */
	    (void)panel_paint_pixrect(ip->panel, dp->mark_off[which_choice], mark_rp,
		PIX_COLOR(ip->color_index));
	    break;

	 case PANEL_NONE:
	    break;
      }
   }
} /* update_display */


static caddr_t
get_attr(ip, which_attr, arg)
panel_item_handle		ip;
register Panel_attribute	which_attr;
int				arg;
/* get_attr returns the current value of which_attr.
*/
{
   register choice_data *dp = choice_dp(ip);
   register int		arg_lousy = (arg < 0 || arg > dp->last);

   switch (which_attr) {
      case PANEL_VALUE:		/* ordinal value or set of values */
	 return (caddr_t) choice_value(dp->choose_one, dp->value, dp->last);

      case PANEL_TOGGLE_VALUE:	/* on/off value of arg'th choice */
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) IN(dp->value, arg);

      case PANEL_DISPLAY_LEVEL:
	 return (caddr_t) dp->display_level;

      case PANEL_FEEDBACK:
	 return (caddr_t) dp->feedback;

      case PANEL_CHOICE_STRINGS:
      case PANEL_CHOICE_IMAGES:
	 return (caddr_t) NULL;

      case PANEL_CHOOSE_ONE:
	 return (caddr_t) dp->choose_one;

      case PANEL_CHOICE_FONT:
	 if (arg_lousy || !is_string(&dp->choices[arg]))
	    return (caddr_t) 0;
         return (caddr_t) image_font(&dp->choices[arg]);

      case PANEL_CHOICE_FONTS:
	 return (caddr_t) NULL;

      case PANEL_CHOICE_STRING:
	 if (arg_lousy || !is_string(&dp->choices[arg]))
	    return (caddr_t) 0;
         return (caddr_t) image_string(&dp->choices[arg]);
         
      case PANEL_CHOICE_IMAGE:
	 if (arg_lousy || !is_pixrect(&dp->choices[arg]))
	    return (caddr_t) 0;
         return (caddr_t) image_pixrect(&dp->choices[arg]);
         
      case PANEL_CHOICE_X:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->choice_rects[arg].r_left;
	 
      case PANEL_CHOICE_Y:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->choice_rects[arg].r_top;
	 
      case PANEL_MARK_X:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->mark_rects[arg].r_left;
	 
      case PANEL_MARK_Y:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->mark_rects[arg].r_top;

      case PANEL_MARK_IMAGE:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->mark_on[arg];

      case PANEL_NOMARK_IMAGE:
	 if (arg_lousy)
	    return (caddr_t) 0;
	 return (caddr_t) dp->mark_off[arg];

      default:
	 return panel_get_generic(ip, which_attr);
   }
} /* get_attr */


static struct pr_size
image_size(image, above_baseline)
register panel_image_handle 	image;
register int 			*above_baseline;
/* image_size returns the size of image.  The amount of image above 
   the baseline is also returned.
*/
{
   struct pr_size size;		/* full size */
   register char *sp;		/* string version */
   struct pixfont *font;	/* font for string */

   switch (image->im_type) {
      case IM_STRING:
	 sp = image_string(image);
	 font = image_font(image);
	 size = pf_textwidth(strlen(sp), font, sp);
	 if (*sp)
	    *above_baseline = -font->pf_char[*sp].pc_home.y + 1;
	 else
	    *above_baseline = 0;
	 break;

      case IM_PIXRECT:
	 size = image_pixrect(image)->pr_size;
	 *above_baseline = size.y;
	 break;
   }
   return (size);
} /* image_size */


/* Return the index of the first set bit in value_set */
static int
choice_number(value_set, last_element)
register u_int	*value_set;
register int	last_element;
{
   register int	i;

   EACH_CHOICE(value_set, last_element, i)
      return i;
   return 0;
}

static u_int
choice_value(choose_one, value_set, last_element)
int	choose_one;
u_int	*value_set;
int	last_element;
{
   return (choose_one ? (choice_number(value_set, last_element)) : (value_set[0]));
}
