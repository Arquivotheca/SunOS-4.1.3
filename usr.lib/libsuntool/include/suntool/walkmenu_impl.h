/*	@(#)walkmenu_impl.h 1.1 92/07/30 SMI	*/

/***********************************************************************/
/*	                      walkmenu_impl.h			       */
/*          Copyright (c) 1985 by Sun Microsystems, Inc.               */
/***********************************************************************/

#ifndef walkmenu_impl_DEFINED
#define walkmenu_impl_DEFINED

#include <suntool/help.h>
#include <suntool/walkmenu.h>
#include "image_impl.h"

#define TRUE	1
#define FALSE	0
#define NULL	0

#define Public
#define Pkg_private
#define Pkg_extern extern
#define Private static

#define imax(a, b) ((int)(b) > (int)(a) ? (int)(b) : (int)(a))
#define range(v, min, max) ((v) >= (min) && (v) <= (max))

#define menu_attr_next(attr) (Menu_attribute *)attr_next((caddr_t *)attr)
#define MENU_FILLER 5

/***********************************************************************/
/*	        	Structures 				       */
/***********************************************************************/

/* 
 * Fields: default_position, selected_position and image are not validated
 * and must be checked before each usage.
 */

/***********			Menu			     **********/

struct menu {
    struct menu_ops_vector *ops;
    int			 default_position;	  /* Default item */
    int			 selected_position;	  /* Last selected item */
    /* Values are: default, selected, nth item				 */
    Menu_attribute	 initial_selection;  /* Initial presentation */
    Menu_attribute	 default_selection;  /* Default presentation */
    int			 height;	  /* in pixels, zero implies best fit */
    int			 width;		  /* in pixels */
    struct pixrect	*shadow_pr;	  /* Draw shadow with this pattern */
    int			 (*feedback_proc)(); /* NYI */
    caddr_t		 (*notify_proc)();
    struct menu		*(*gen_proc)();   /* Dynamically generate menu */
    /* Handler for item select,  calls menu item proc */
    struct image	 default_image;	  /* Font, max size, etc. */
    struct menu		*stack_menu;	  /* NYI - Stacking menu (sibling) */
    int			 pullright_delta; /* Distance(pixels) to bring up menu*/
    caddr_t		 client_data;	  /* Client''s use */
    caddr_t		 help_data;	  /* Help use */

/* Flags */
    unsigned		 stay_up;	      /* Double click */
    unsigned		 display_one_level:1; /* Limit initial selection */
    unsigned		 stand_off:1;	      /* Don''t selected item */
    unsigned		 valid_result:1;      /* True if m->value is valid */
    unsigned		 jump_after_selection:1; /* If true then jump */
    unsigned		 jump_after_no_selection:1; /* If true then jump */
    unsigned		 column_major:1;      /* Layout items in col major */
    unsigned		 free_everything:1;   /* Not used */
    unsigned		 free_menu:1;         /* Not used */
    unsigned		 free_shadow_pr:1;    /* Not used */
    unsigned		 free_client_data:1;  /* Not used */
    unsigned		 h_line:1;	      /* Draw horizontal line after item */
    unsigned		 v_line:1;	      /* Draw vertical line after item */

/* Auxiliary fields */
    int			 nitems;	  /* Number of items */
    int			 max_nitems;	  /* Size of item list */
    int			 ncols;
    int			 nrows;
    struct menu_item	**item_list;
    struct menu_info	*dynamic_info;	  /* Holds dynamic info for show() */
    struct menu_item	*parent;	  /* Last pullright item */



};


/***********			Menu item			**********/

struct menu_item {
    struct menu_ops_vector *ops;
    struct image	*image;
    struct menu_item	*(*gen_proc)();	    /* Called before displaying item */
    struct menu		*(*gen_pullright)();/* Called before displaying menu */
    caddr_t		 (*notify_proc)();  /* Called only when selected */
    caddr_t		 value;		    /* union: value, menu_ptr */
    caddr_t		 client_data;
    caddr_t		 help_data;
    struct menu		*parent;	    /* Current enclosing menu */

/* Flags  */
    unsigned		 inactive:1;
    unsigned		 no_feedback:1;
    unsigned		 pullright:1;
    unsigned		 selected:1;
    unsigned		 free_everything:1;   /* Not used */
    unsigned		 free_item:1;
    unsigned		 free_value:1;        /* Not used */
    unsigned		 free_client_data:1;  /* Not used */
    unsigned		 h_line:1;	      /* Draw horizontal line after item */
    unsigned		 v_line:1;	      /* Draw vertical line after item */

};


/***********			Menu ops			**********/

struct menu_ops_vector {
    caddr_t (*menu_get_op)();
    void (*menu_set_op)();
    void (*menu_destroy_op)();
};


/***********			Menu info			**********/

struct menu_info {
    struct inputevent	*first_iep;
    struct inputevent	*last_iep;
    int			 depth;
    caddr_t		 (*notify_proc)();
    caddr_t		 help;
};

#endif ~walkmenu_impl_DEFINED
