/*	@(#)panel_impl.h 1.1 92/07/30 SMI	*/

/*************************************************************************/
/*                            panel_impl.h                               */
/*            Copyright (c) 1985 by Sun Microsystems, Inc.               */
/*************************************************************************/

#ifndef panel_impl_DEFINED
#define panel_impl_DEFINED

#ifdef menu_SKIP
#undef menu_SKIP
#endif menu_SKIP

#ifdef lint

#ifdef menu_DEFINED
#define menu_SKIP
#else
#define menu_DEFINED
#endif menu_DEFINED

#endif lint

#ifndef menu_SKIP

#include <stdio.h>
#include <sunwindow/window_hs.h>
#include <suntool/tool_struct.h>
#include <suntool/panel.h>
#include <suntool/scrollbar.h>
#include <suntool/selection_svc.h>
#include <suntool/help.h>

/*************************************************************************/
/*                    per item status flags                              */
/*************************************************************************/


#define IS_PANEL		0x00000001  /* object is a panel */
#define IS_ITEM			0x00000002  /* object is an item */
#define HIDDEN			0x00000004  /* item is not currently displayed */
#define ITEM_X_FIXED		0x00000008  /* item's x coord fixed by user*/
#define ITEM_Y_FIXED		0x00000010  /* item's y coord fixed by user*/
#define LABEL_X_FIXED		0x00000020  /* label x coord fixed by user */
#define LABEL_Y_FIXED		0x00000040  /* label y coord fixed by user */
#define VALUE_X_FIXED		0x00000080  /* value x coord fixed by user */
#define VALUE_Y_FIXED		0x00000100  /* value y coord fixed by user */
#define WANTS_KEY		0x00000400  /* item wants keystroke events */
#define LABEL_BOLD     	 	0x00000800  /* for panel-wide setting */
#define VALUE_BOLD      	0x00001000  /* for panel-wide setting */
#define ADJUSTABLE		0x00002000  /* temporarily out of service */
#define SHOW_MENU_MARK		0x00004000  /* show the menu mark in the menu */
#define ITEM_NOT_SCROLLABLE     0x00008000  /* item will not scroll */
#define SHOW_MENU		0x00010000  /* show the menu */
#define LABEL_SHADED		0x00020000  /* shade the label */
#define OPS_SET			0x00040000  /* ops vector has been altered */

#define hidden(ip)		((ip)->flags & HIDDEN)
#define adjustable(ip)		((ip)->flags & ADJUSTABLE)
#define item_fixed(ip)		((ip)->flags & (ITEM_X_FIXED | ITEM_Y_FIXED))
#define label_fixed(ip)		((ip)->flags & (LABEL_X_FIXED|LABEL_Y_FIXED))
#define value_fixed(ip)		((ip)->flags & (VALUE_X_FIXED|VALUE_Y_FIXED))
#define item_not_scrollable(ip)	((ip)->flags & ITEM_NOT_SCROLLABLE)
#define wants_key(object)	((object)->flags & WANTS_KEY)
#define label_bold_flag(object)	((object)->flags & LABEL_BOLD)
#define label_shaded_flag(object)	((object)->flags & LABEL_SHADED)
#define value_bold_flag(object)	((object)->flags & VALUE_BOLD)
#define is_panel(object)	((object)->flags & IS_PANEL)
#define is_item(object)		((object)->flags & IS_ITEM)
#define show_menu_mark(object)	((object)->flags & SHOW_MENU_MARK)
#define show_menu(object)	((object)->flags & SHOW_MENU)
#define ops_set(object)		((object)->flags & OPS_SET)

/*************************************************************************/
/*			per panel status flags  		         */
/*************************************************************************/

#define BLINKING		0x00000001
#define TIMING			0x00000002
#define SCROLL_V_NORMALIZE      0x00000004
#define SCROLL_H_NORMALIZE      0x00000008
#define SHOW_MENU_SET	        0x00000010
#define USING_NOTIFIER	        0x00000020
#define USING_WRAPPER		0x00000040
#define ADJUST_IS_PENDING_DELETE 0x00000200

#define blinking(panel)			((panel)->status & BLINKING)
#define timing(panel)			((panel)->status & TIMING)
#define v_normalization_desired(panel) 	((panel)->status & SCROLL_V_NORMALIZE)
#define h_normalization_desired(panel) 	((panel)->status & SCROLL_H_NORMALIZE)
#define show_menu_set(panel)		((panel)->status & SHOW_MENU_SET)
#define using_notifier(panel)		((panel)->status & USING_NOTIFIER)
#define using_wrapper(panel)		((panel)->status & USING_WRAPPER)
#define adjust_is_pending_delete(panel) ((panel)->status & ADJUST_IS_PENDING_DELETE)

/*************************************************************************/
/*			macros for scrolling      		         */
/*************************************************************************/

#define view_width(panel)	((panel)->rect.r_width - (panel)->v_bar_width)
#define view_height(panel)	((panel)->rect.r_height - (panel)->h_bar_width)

/*************************************************************************/
/* miscellaneous constants                                               */
/*************************************************************************/

#define	BIG			0x7FFF
#define	KEY_NEXT		KEY_BOTTOMRIGHT
#define	ITEM_X_GAP		10	/* # of x pixels between items */
#define	ITEM_Y_GAP		5	/* # of y pixels between items rows */
#define LABEL_X_GAP   		8	/* used in panel_attr.c */
#define LABEL_Y_GAP 		4	/* used in panel_attr.c */
/*
 * Definitions to support the default user interface.
 */
#define TOP_KEY			ACTION_FRONT
#define BOTTOM_KEY		ACTION_BACK
#define PUT_KEY			ACTION_COPY
#define OPEN_KEY		ACTION_OPEN
#define CLOSE_KEY		ACTION_CLOSE
#define GET_KEY			ACTION_PASTE
#define FIND_KEY		ACTION_FIND_FORWARD
#define DELETE_KEY		ACTION_CUT
#define AGAIN_KEY		ACTION_AGAIN
#define PROPS_KEY		ACTION_PROPS
#define HELP_KEY		ACTION_HELP

/*************************************************************************/
/* structures                                                            */
/*************************************************************************/

typedef struct panel		*panel_handle;
typedef	struct panel_item	*panel_item_handle;
typedef struct panel_image	*panel_image_handle;

/*********************** panel menu ***************************************/

struct	menu {
	int	m_imagetype;		/* interpretation of m_imagedata */
#define	MENU_IMAGESTRING	0x00	/* imagedata is char * */
#define MENU_GRAPHIC            0x01    /* imagedata is pixrect * */
	caddr_t	m_imagedata;		/* pointer to display data for header */
	int	m_itemcount;		/* number of menuitems in m_items */
	struct	menuitem *m_items;	/* array of menuitems */
	struct	menu *m_next;		/* link to another menu */
	caddr_t	m_data;			/* menu private data (initialize to 0)*/
};

struct	menuitem {
	int	mi_imagetype;		/* interpretation of mi_imagedata */
	caddr_t	mi_imagedata;		/* pointer to display data for item */
	caddr_t	mi_data;		/* item specific data */
};

/*********************** panel_image **************************************/

#define	IM_PIXRECT	2	/* pixrect */
#define	IM_STRING	3	/* string */

struct panel_image {
   u_int	im_type;
   u_int	shaded	: 1;	/* true to shade the image */
   union {
     struct {			
	 char           *text;
	 Pixfont	*font;
	 short 		 bold;	/* TRUE if text should be bold */
     } t;        			/*  IM_STRING arm	*/
     Pixrect		*pixrect;	/*  IM_PIXRECT arm  */
   } im_value;
};

#define image_type(image)  	((image)->im_type)
#define image_shaded(image)    	((image)->shaded)
#define is_string(image)	(image_type(image) == IM_STRING)
#define is_pixrect(image)	(image_type(image) == IM_PIXRECT)
#define image_string(image)  	((image)->im_value.t.text)
#define image_font(image)    	((image)->im_value.t.font)
#define image_bold(image)    	((image)->im_value.t.bold)
#define image_pixrect(image) 	((image)->im_value.pixrect)

#define image_set_type(image, type)	  (image_type(image)    = type)
#define image_set_string(image, string)	  (image_string(image)	= (string))
#define image_set_pixrect(image, pixrect) (image_pixrect(image) = (pixrect))

#define image_set_font(image, font)	(image_font(image)	= (font))
#define image_set_bold(image, bold)  	(image_bold(image)	= (bold) != 0)
#define image_set_shaded(image, shaded)	(image_shaded(image)	= (shaded) != 0)


/******************* selection info ****************************************/

struct panel_selection {
   Seln_rank		rank;	/* SELN_{PRIMARY, SECONDARY, CARET} */
   u_int		is_null;/* TRUE if the selection has no chars */
   panel_item_handle	ip;	/* item with the selection */
};

typedef struct panel_selection	*panel_selection_handle;

#define	panel_seln(panel, rank)	(&((panel)->selections[(u_int)(rank)]))


/****************** ops vector for panels & items *****************************/

/* Note that the null_ops static struct
 * in panel_public.c must be updated when
 * this structure is changed.
 */
struct panel_ops {
   int	   (*handle_event)();
   int	   (*begin_preview)();
   int	   (*update_preview)();
   int	   (*cancel_preview)();
   int	   (*accept_preview)();
   int	   (*accept_menu)();
   int	   (*accept_key)();
   int	   (*paint)();
   int	   (*destroy)();
   caddr_t (*get_attr)();
   int	   (*set_attr)();
   caddr_t (*remove)();
   caddr_t (*restore)();
   int	   (*layout)();
};

typedef struct panel_ops	*panel_ops_handle;

/***************************** panel **************************************/
/* Note that the first two fields of the
 * panel struct must match those of the panel_item
 * struct, since these are used interchangably in 
 * some cases.
 */
struct panel	{
        panel_ops_handle	ops;		/* panel specific operations */
   	u_int			flags;		/* default item flags */
	struct {
	   u_int left_down	: 1;		/* status of left button */
	   u_int middle_down	: 1;		/* status of middle button */
	   u_int right_down	: 1;		/* status of right button */
	} button_status;
	Panel_setting		mouse_state;	/* state of mouse buttons */
	int			windowfd;
	Pixwin		       *pixwin;
	Pixwin		       *view_pixwin;
	Rect			rect;
	Pixfont		       *font;
	char			edit_bk_char;
	char			edit_bk_word;
	char			edit_bk_line;
        Panel_setting		layout;		/* HORIZONTAL, VERTICAL */
	u_int			status;		/* current event state */
	int			item_x;
	int			item_y;
	int			item_x_offset;
	int			item_y_offset;
	int			max_item_y;	/* lowest item on row */
	panel_item_handle	items;
	panel_item_handle	current;
	panel_item_handle	caret;
	int			caret_on;
        int			(*timer_proc)();
        int			(*event_proc)();
	struct timeval		timer;		/* timer for the sw */
	struct timeval		timer_full;	/* initial secs & usecs */
	caddr_t			client_data;	/* for client use */
	caddr_t			help_data;	/* for help use */
        int			SliderActive;	/* TRUE if slider is previewing */
        Panel_setting		repaint;	/* default repaint behavior  */
	Toolsw		       *toolsw;		/* for setting width, height */
        Scrollbar		v_scrollbar;	/* vertical scrollbar        */
	int			v_bar_width;	/* width of vertical sb      */
        int			v_offset;	/* top of viewing window     */
        int			v_end;		/* bottom populated pixel    */
        int			v_margin;	/* gap at top for scrolling  */
        Scrollbar		h_scrollbar;	/* horizontal scrollbar      */
	int			h_bar_width;	/* width of horizontal sb    */
        int			h_offset;	/* left of viewing window    */
        int			h_end;		/* rightmost populated pixel */
        int			h_margin;	/* gap at left for scrolling */
	Tool			*tool;		/* for CLOSE, TOP */
	caddr_t			seln_client;	/* selection svc client id */
	struct panel_selection	selections[(int) SELN_UNSPECIFIED];
	char			*shelf;		/* contents of shelf seln */
	struct pixrect		*caret_pr;	/* current caret pixrect */
};

#define has_scrollbar(panel) (((panel)->v_scrollbar) || ((panel)->h_scrollbar))
#define has_background_proc(panel) \
	((panel)->ops && (panel)->ops->handle_event && \
	(panel)->ops->handle_event != panel_nullproc)

/*************************** panel item ***********************************/

/* types of items */
typedef enum { 
   PANEL_MESSAGE_ITEM,
   PANEL_BUTTON_ITEM, 
   PANEL_CHOICE_ITEM, 
   PANEL_TOGGLE_ITEM, 
   PANEL_TEXT_ITEM,
   PANEL_SLIDER_ITEM,
   PANEL_LINE_ITEM
} PANEL_ITEM_TYPE;
    
/* Note that the first two fields of the
 * panel_item struct must match those of the panel
 * struct, since these are used interchangably in 
 * some cases.
 */
struct panel_item {
   panel_ops_handle	ops;		/* item type specific operations */
   u_int		flags;		/* boolean attributes */
   PANEL_ITEM_TYPE	item_type;	/* type of this item */
   panel_handle		panel;		/* panel subwindow for the item */
   caddr_t		data;		/* item type specific data */
   Rect			painted_rect;	/* painted area in the pw */
   Rect			rect;		/* enclosing item rect */
   struct panel_image	label;		/* the label */
   Rect			label_rect;	/* enclosing label rect */
   Rect			value_rect;	/* enclosing label rect */
   Panel_setting	layout;	        /* HORIZONTAL, VERTICAL */
   int			(*notify)();	/* notify proc */
   struct menu	 	*menu;		/* menu of choices */
   struct panel_image 	 menu_title;	/* menu title string/image */
   struct panel_image 	*menu_choices;	/* each menu choice */
   short		 menu_last;	/* last slot in menu_choices[] */
   short		 menu_choices_bold;	/* bold/regular choices */
   caddr_t		*menu_values;	/* value for each menu item */
   Pixrect 		*menu_type_pr;	/* title type indication */
   Pixrect 		*menu_mark_on;	/* selected menu mark image */
   Pixrect 		*menu_mark_off;		/* un-sel. menu mark image*/
   short 		 menu_mark_width;	/* width of menu mark */
   short		 menu_mark_height;	/* height of menu mark */
   short		 menu_max_width;	/* width of the whole menu */
   struct {				/* status of menu attributes */ 
      u_int title_set		: 1;
      u_int title_font_set	: 1;
      u_int choices_set		: 1;
      u_int choice_fonts_set	: 1;
      u_int title_dirty		: 1;
   } menu_status;
   Panel_setting      repaint;		/* item's repaint behavior */
   panel_item_handle  next; 		/* next item */
   caddr_t	      client_data;	/* for client use */
   caddr_t	      help_data;	/* for help use */
   int		      color_index;	/* for color panel items */

};

/***********************************************************************/
/* external declarations of private functions                          */
/***********************************************************************/

extern 	panel_handle	 	panel_create_panel_struct();
extern 	int	 		panel_set_panel();
extern 	caddr_t	 		panel_get_panel();
extern 	panel_item_handle 	panel_successor();
extern 	struct pr_size 		panel_make_image();
extern  int			panel_set_generic();
extern  caddr_t			panel_get_generic();
extern 	Rect			panel_enclosing_rect();
extern 	panel_item_handle 	panel_append();
extern  int		 	panel_col_to_x();
extern  int		 	panel_row_to_y();
extern  char                   *panel_strsave();
extern int 			panel_nullproc();
extern int 			panel_fonthome();
extern struct menuitem 	       *panel_menu_display();
extern void			panel_sync_menu_title();
extern void			panel_copy_menu_title();
extern void			panel_copy_menu_choices();
extern				panel_find_default_xy();

/* pixwin writing routines */
extern int			panel_pw_write();
extern int			panel_pw_writebackground();
extern int			panel_pw_replrop();
extern int			panel_pw_vector();
extern int			panel_pw_text();
extern int			panel_pw_lock();
extern int			panel_draw_box();

/* paint routines */
extern void			panel_display();
extern int                      panel_paint_item();
extern void			panel_resize();

extern Notify_value		panel_use_event();
extern void			panel_caret_on();
extern void			panel_caret_invert();

/* selection service routines */
extern void			panel_seln_init();
extern void			panel_seln_destroy();
extern void			panel_seln_inform();
extern Seln_holder		panel_seln_inquire();
extern void			panel_seln_acquire();
extern void			panel_seln_cancel();
extern void			panel_seln_dehilite();
extern void			panel_seln_hilite();

/* scrollbar routines */
extern void	                panel_scroll();
extern void			panel_setup_scrollbar();
extern void			panel_update_extent();
extern void			panel_resize_region();
extern void			panel_update_scrollbars();

extern Notify_value		panel_notify_event();
extern Notify_value		panel_destroy();

/* for backwards compatability */
extern Toolsw			*panel_create_sw();

/* for lint */
extern char			*calloc();


/***********************************************************************/
/* external declarations of private data                               */
/***********************************************************************/

extern	Pixrect			panel_empty_pr;
extern	Pixrect			panel_caret_pr;
extern	Pixrect			panel_ghost_caret_pr;

/*************************************************************************/
/* miscellaneous private macros                                          */
/*************************************************************************/

#define	set(value)	((value) != -1) 

#ifndef LINT_CAST
#ifdef lint
#define LINT_CAST(arg)  (arg ? 0 : 0)
#else
#define LINT_CAST(arg)  arg
#endif
#endif LINT_CAST

#define	PANEL_CAST(client_panel)	\
    ((panel_handle) LINT_CAST(client_panel))

#define	PANEL_ITEM_CAST(client_item)	\
    ((panel_item_handle) LINT_CAST(client_item))


#endif menu_SKIP
 
#ifdef menu_SKIP
#undef menu_SKIP
#endif menu_SKIP
/*************************************************************************/

#endif !panel_impl_DEFINED
