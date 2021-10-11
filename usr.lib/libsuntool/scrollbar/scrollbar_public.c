#ifndef lint
#ifdef sccs
static	char sccsid[] = "@(#)scrollbar_public.c 1.1 92/07/30 Copyr 1984 Sun Micro";
#endif
#endif

/*************************************************************************/
/*                         scrollbar_public.c                            */
/*           Copyright (c) 1985 by Sun Microsystems, Inc.                */
/*************************************************************************/

#include <sys/types.h>
#include <sys/ioccom.h>
#include <varargs.h>
#include <sunwindow/defaults.h>
#include <suntool/scrollbar_impl.h> 
#include <pixrect/pixrect_hs.h>
#include <sun/fbio.h>

static int               defaults_thickness, defaults_button_length;
static Scrollbar_setting defaults_v_gravity, 
                         defaults_h_gravity;

static scrollbar_set_internal();
static scrollbar_free();
static scrollbar_fix_region();

scrollbar_handle scrollbar_active_sb; /* currently active scrollbar */
scrollbar_handle scrollbar_head_sb;   /* head of linked list of sb's for pixwin */

/**************************************************************************/
/* cursors                                                                */
/* There are 3 cursors for each of the 2 scrollbar orientations.          */
/**************************************************************************/

static short v_west_active_cursor_image[16] = {
	0x0800,0x0C00,0x0E00,0x0F00,0xFF80,0xFFC0,0xFFE0,0xFFF0,
	0xFFE0,0xFFC0,0xFF80,0x0F00,0x0E00,0x0C00,0x0800,0x0000
};
mpr_static(scrollbar_v_west_active_mpr, 16, 16, 1, v_west_active_cursor_image);
struct cursor scrollbar_v_west_active_cursor = 
   {5,7,PIX_SRC|PIX_DST,&scrollbar_v_west_active_mpr};

static short v_east_active_cursor_image[16] = {
 	0x0100,0x0300,0x0700,0x0F00,0x1FF8,0x3FF8,0x7FF8,0xFFF8,
	0x7FF8,0x3FF8,0x1FF8,0x0F00,0x0700,0x0300,0x0100,0x0000

};
mpr_static(scrollbar_v_east_active_mpr, 16, 16, 1, v_east_active_cursor_image);
struct cursor scrollbar_v_east_active_cursor = 
   {5,7,PIX_SRC|PIX_DST,&scrollbar_v_east_active_mpr};

static short h_north_active_cursor_image[16] = {
	0x07F0,0x07F0,0x07F0,0x07F0,0x07F0,0x7FFF,0x3FFE,0x1FFC,
	0x0FF8,0x07F0,0x03E0,0x01C0,0x0080,0x0000,0x0000,0x0000
};
mpr_static(scrollbar_h_north_active_mpr,16,16,1,h_north_active_cursor_image);
struct cursor scrollbar_h_north_active_cursor = 
   {7,5,PIX_SRC|PIX_DST,&scrollbar_h_north_active_mpr};

static short h_south_active_cursor_image[16] = {
	0x0080,0x01C0,0x03E0,0x07F0,0x0FF8,0x1FFC,0x3FFE,0x7FFF,
	0x07F0,0x07F0,0x07F0,0x07F0,0x07F0,0x0000,0x0000,0x0000
};
mpr_static(scrollbar_h_south_active_mpr,16,16,1, h_south_active_cursor_image);
struct cursor scrollbar_h_south_active_cursor = 
   {7,5,PIX_SRC|PIX_DST,&scrollbar_h_south_active_mpr};

static short down_cursor_image[16] = {
	0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0,
	0xFFFF, 0x7FFE, 0x3FFC, 0x1FF8, 0x0FF0, 0x07E0, 0x03C0, 0x0180
};
mpr_static(scrollbar_scrolldown_mpr, 16, 16, 1, down_cursor_image);
struct cursor scrollbar_down_cursor = 
   {8, 15, PIX_SRC|PIX_DST, &scrollbar_scrolldown_mpr};

static short up_cursor_image[16] = {
	0x0180, 0x03C0, 0x07E0, 0x0FF0, 0x1FF8, 0x3FFC, 0x7FFE, 0xFFFF,
	0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0, 0x07E0
};
mpr_static(scrollbar_scrollup_mpr, 16, 16, 1, up_cursor_image);
struct cursor scrollbar_up_cursor = 
   {8, 0, PIX_SRC|PIX_DST, &scrollbar_scrollup_mpr};

static short thumbv_cursor_image[16] = {
	0x8000, 0xC000, 0xE000, 0xF000, 0xF800, 0xFC00, 0xFE00, 0xFF00,
	0xFF00, 0xFE00, 0xFC00, 0xF800, 0xF000, 0xE000, 0xC000, 0x8000
};
mpr_static(scrollbar_thumbv_mpr, 16, 16, 1, thumbv_cursor_image);
struct cursor scrollbar_thumbv_cursor = 
   {3, 8, PIX_SRC|PIX_DST, &scrollbar_thumbv_mpr};

static short left_cursor_image[16] = {
	0x0100, 0x0300, 0x0700, 0x0F00, 0x1F00, 0x3FFF, 0x7FFF, 0xFFFF,
	0xFFFF, 0x7FFF, 0x3FFF, 0x1F00, 0x0F00, 0x0700, 0x0300, 0x0100
};
mpr_static(scrollbar_scrollleft_mpr, 16, 16, 1, left_cursor_image);
struct cursor scrollbar_left_cursor = 
   {15, 8, PIX_SRC|PIX_DST, &scrollbar_scrollleft_mpr};

static short right_cursor_image[16] = {
	0x0080, 0x00C0, 0x00E0, 0x00F0, 0x00F8, 0xFFFC, 0xFFFE, 0xFFFF,
	0xFFFF, 0xFFFE, 0xFFFC, 0x00F8, 0x00F0, 0x00E0, 0x00C0, 0x0080
};
mpr_static(scrollbar_scrollright_mpr, 16, 16, 1, right_cursor_image);
struct cursor scrollbar_right_cursor = 
   {0, 8, PIX_SRC|PIX_DST, &scrollbar_scrollright_mpr};

static short thumbh_cursor_image[16] = {
	0x7FFF,0x3FFE,0x1FFC,0x0FF8,0x07F0,0x03E0,0x01C0,0x0080,
	0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000,0x0000
};
mpr_static(scrollbar_thumbh_mpr, 16, 16, 1, thumbh_cursor_image);
struct cursor scrollbar_thumbh_cursor = 
   {8, 0, PIX_SRC|PIX_DST, &scrollbar_thumbh_mpr};

scrollbar_nullproc() { }

/**************************************************************************/
/* scrollbar_build                                                        */
/**************************************************************************/

Scrollbar
scrollbar_create_internal(avlist)
caddr_t *avlist;
{
   extern char               *calloc();
   register scrollbar_handle  sb;

   if (sb = (scrollbar_handle)(LINT_CAST(
   	sv_calloc(1, sizeof(struct scrollbar))))) {
      (void)scrollbar_init(sb);
      (void)scrollbar_set_internal(sb, avlist);
   }
   return (Scrollbar) sb;
}

Scrollbar
scrollbar_build(argv)
caddr_t argv;
{
   return scrollbar_create_internal(&argv);
}

/*VARARGS*/
Scrollbar
scrollbar_create(va_alist)
va_dcl
{
   va_list valist;
   caddr_t avlist[ATTR_STANDARD_SIZE];

   va_start(valist);
   (void)attr_make(avlist,ATTR_STANDARD_SIZE,valist);
   va_end(valist);
   return scrollbar_create_internal(avlist);
}

/* [use this when we clean up: note *argv vs argv usage]
Scrollbar
scrollbar_create(argv)
caddr_t argv;
{
   extern char               *calloc();
   register scrollbar_handle  sb;

   if (sb = (scrollbar_handle)sv_calloc(1, sizeof(struct scrollbar))) {
      (void)scrollbar_init(sb);
      (void)scrollbar_set_internal(sb, &argv);
   }
   return (Scrollbar) sb;
}
*/
/**************************************************************************/
/* scrollbar_init                                                         */
/**************************************************************************/

scrollbar_init(sb)
scrollbar_handle sb;
{
   static Defaults_pairs display_level[] = {
	"Always",	(int)SCROLL_ALWAYS,
	"Active",	(int)SCROLL_ACTIVE,
	"Never",	(int)SCROLL_NEVER,
	NULL,		(int)SCROLL_ALWAYS,
   };
   static Defaults_pairs vertical_placement[] = {
	"East",		(int)SCROLL_MAX,
	"West",		(int)SCROLL_MIN,
	NULL,		(int)SCROLL_MIN
   };
   static Defaults_pairs horizontal_placement[] = {
	"South",	(int)SCROLL_MAX,
	"North",	(int)SCROLL_MIN,
	NULL,		(int)SCROLL_MIN
   };
   static Defaults_pairs bubble_color[] = {
	"Black",	FALSE,
	"Grey",		TRUE,
	NULL,		TRUE
   };
   static Defaults_pairs bar_color[] = {
	"White",	FALSE,
	"Grey",		TRUE,
	NULL,		TRUE
   };
   defaults_v_gravity = (Scrollbar_setting)defaults_lookup(
	defaults_get_string("/Scrollbar/Vertical_bar_placement", 
		(char *)NULL, (int *)NULL),
	vertical_placement);
   defaults_h_gravity = (Scrollbar_setting)defaults_lookup(
	defaults_get_string("/Scrollbar/Horizontal_bar_placement", 
		(char *)NULL, (int *)NULL),
	horizontal_placement);
   defaults_thickness = defaults_get_integer_check("/Scrollbar/Thickness",
			   SCROLL_DEFAULT_WIDTH, 0, 200, (int *)NULL);
   defaults_button_length = 
      defaults_get_integer_check("/Scrollbar/Page_button_length", 
                           SCROLLBAR_DEFAULT_PAGE_BUTTON_LENGTH, 0, 200, 
			   (int *)NULL);

   sb->gravity                = SCROLL_MIN;
   sb->gravity_not_yet_set    = TRUE;
   sb->pixwin                 = (struct pixwin *) 0;
   sb->modify                 = scrollbar_nullproc;
   sb->view_start             = 0;
   sb->view_length            = 0;
   sb->object_length          = SCROLLBAR_INVALID_LENGTH;
   sb->undo_mark              = 0;
   sb->horizontal	      = FALSE;
   sb->rect.r_width	      = defaults_thickness;
   sb->normalize              = TRUE;
   sb->use_grid               = FALSE;
   sb->line_height	      = 0;
   sb->normalize_margin       = SCROLL_DEFAULT_NORMALIZE_MARGIN;
   sb->gap		      = SCROLLBAR_INVALID_LENGTH;
   sb->button_length	      = defaults_button_length;
   sb->saved_button_length    = defaults_button_length;
   sb->forward_cursor         = &scrollbar_up_cursor;
   sb->backward_cursor        = &scrollbar_down_cursor;
   sb->absolute_cursor        = &scrollbar_thumbv_cursor;
   sb->active_cursor          = &scrollbar_v_west_active_cursor;
   sb->advanced_mode          = FALSE;
   sb->bar_painted            = FALSE;
   sb->bubble_painted         = FALSE;
   sb->buttons_painted        = FALSE;
   sb->border_painted         = FALSE;
   sb->direction_not_yet_set  = TRUE;
   sb->bubble_modified        = TRUE;
   sb->attrs_modified         = TRUE;
   sb->next                   = scrollbar_head_sb; 
   sb->paint_buttons_proc     = scrollbar_paint_buttons;
   scrollbar_head_sb          = sb;

   sb->bar_display_level = (Scrollbar_setting)defaults_lookup(
	defaults_get_string("/Scrollbar/Bar_display_level", 
		(char *)NULL, (int *)NULL),
	display_level);
   sb->bubble_display_level = (Scrollbar_setting)defaults_lookup(
	defaults_get_string("/Scrollbar/Bubble_display_level", 
		(char *)NULL, (int *)NULL),
	display_level);

   sb->border = (unsigned)defaults_get_boolean("/Scrollbar/Border", 
   			(Bool)(LINT_CAST(TRUE)), (int *)NULL);
   sb->buttons= (unsigned)defaults_get_boolean("/Scrollbar/Page_buttons",
   			(Bool)(LINT_CAST(TRUE)),(int *)NULL);

   sb->bubble_grey  = defaults_lookup(
	defaults_get_string ("/Scrollbar/Bubble_color", 
		(char *)NULL, (int *)NULL),
	bubble_color);
   sb->bar_grey  = defaults_lookup(
	defaults_get_string ("/Scrollbar/Bar_color", 
		(char *)NULL, (int *)NULL),
	bar_color);
   sb->bubble_margin      = 
      defaults_get_integer_check("/Scrollbar/Bubble_margin", 
         SCROLL_DEFAULT_BUBBLE_MARGIN, 0, (int)defaults_thickness/2, 
	 (int *)NULL);
   sb->end_point_area = defaults_get_integer("/Scrollbar/End_point_area", 
         SCROLLBAR_DEFAULT_END_POINT_AREA, (int *)NULL);
   sb->delay = defaults_get_integer ("/Scrollbar/Repeat_time",
         SCROLLBAR_DEFAULT_REPEAT_TIME, (int *)NULL);
   sb->help_data	      = "sunview:scrollbar";

   /* initialize timer struct */
   /* timer = NOTIFY_NO_ITIMER; */
   scrollbar_timer.it_value.tv_sec     = 0;
   scrollbar_timer.it_value.tv_usec    = 0;
   scrollbar_timer.it_interval.tv_sec  = 0;
   scrollbar_timer.it_interval.tv_usec = 0;
}

/**************************************************************************/
/* scrollbar_set                                                          */
/**************************************************************************/

/*VARARGS1*/
int
scrollbar_set(sb, va_alist)
scrollbar_handle  sb;
va_dcl
{
   va_list valist;
   caddr_t avlist[ATTR_STANDARD_SIZE];

   va_start(valist);
   (void) attr_make(avlist,ATTR_STANDARD_SIZE,valist);
   va_end(valist);
   (void)scrollbar_set_internal(sb, avlist);
}

static
scrollbar_set_internal(sb, avlist)
scrollbar_handle  sb;
char **avlist;
{


   if (!sb) return 0;
   set_scrollbar_attr(sb, avlist);
   if (sb->attrs_modified)
      sb->modify(sb);
   /* Note: can't clear the attrs_modified flag here
    * because it is looked at elsewhere also.
    */
   return 1;
}

static
set_scrollbar_attr(sb, argv)
scrollbar_handle  sb;
caddr_t          *argv;
{
   register Scrollbar_attribute attr;
   register Rect              *r;
   Rect                        rect;
   int			       client_set          = FALSE;
   int                         thickness_set       = FALSE;
   int                         rect_set            = FALSE;
   int			       direction_set       = FALSE;
   int			       forward_cursor_set  = FALSE;
   int			       backward_cursor_set = FALSE;
   int			       absolute_cursor_set = FALSE;
   int			       active_cursor_set   = FALSE;
   Scrollbar_setting	       defaults_gravity; 
   int                         thickness;
   Rect			       old_bubble, new_bubble;
   int                         sb_major_axis;
   int                         sb_is_tiny;

   (void)scrollbar_compute_bubble_rect(sb, &old_bubble);

   while(attr = (Scrollbar_attribute) *argv++) {
      switch ((Scrollbar_attribute)attr) {

	 case SCROLL_NOTIFY_CLIENT:
	    sb->notify_client = *argv++;
	    client_set = TRUE;
	    break;

	 case SCROLL_OBJECT:
	    sb->object = *argv++;
	    break;

	 case SCROLL_PIXWIN:
	    sb->pixwin = (struct pixwin *) (LINT_CAST(*argv++));
	    sb->attrs_modified = TRUE;
            break;

	 case SCROLL_MODIFY_PROC:
	    sb->modify = (int (*)()) (LINT_CAST(*argv++));
	    if (!sb->modify)
	       sb->modify = scrollbar_nullproc;
	    break;

	 case SCROLL_RECT:
	    r = (struct rect *) (LINT_CAST(*argv++));
	    sb->rect.r_top    = r->r_top;
	    sb->rect.r_left   = r->r_left;
	    sb->rect.r_height = r->r_height;
	    sb->rect.r_width  = r->r_width;
	    sb->rect_fixed    = TRUE;
	    sb->attrs_modified = TRUE;
	    rect_set           = TRUE;
	    break;

	 case SCROLL_TOP:
	    sb->rect.r_top     = (int) *argv++;
	    sb->rect_fixed     = TRUE;
	    sb->attrs_modified = TRUE;
	    rect_set           = TRUE;
	    break;

	 case SCROLL_LEFT:
	    sb->rect.r_left    = (int) *argv++;
	    sb->rect_fixed     = TRUE;
	    sb->attrs_modified = TRUE;
	    rect_set           = TRUE;
	    break;

	 case SCROLL_WIDTH:
	    sb->rect.r_width   = (int) *argv++;
	    sb->rect_fixed     = TRUE;
	    sb->attrs_modified = TRUE;
	    rect_set           = TRUE;
	    break;

	 case SCROLL_HEIGHT:
	    sb->rect.r_height  = (int) *argv++;
	    sb->rect_fixed     = TRUE;
	    sb->attrs_modified = TRUE;
	    rect_set           = TRUE;
	    break;

	 case SCROLL_THICKNESS:
	    thickness          = (int) *argv++;
	    sb->rect_fixed     = TRUE;
	    thickness_set      = TRUE;
	    rect_set           = TRUE;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_PLACEMENT:
	    sb->gravity        = (Scrollbar_setting) *argv++;
	    sb->gravity_not_yet_set = FALSE;
	    sb->attrs_modified = TRUE;
	    break;

         case SCROLL_BUBBLE_MARGIN:
	    sb->bubble_margin  = (unsigned) *argv++;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_BUBBLE_COLOR:
	    if ((Scrollbar_setting) *argv++ == SCROLL_GREY)
	       sb->bubble_grey = TRUE;
	    else
	       sb->bubble_grey = FALSE;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_BAR_COLOR:
	    if ((Scrollbar_setting) *argv++ == SCROLL_GREY)
	       sb->bar_grey = TRUE;
	    else
	       sb->bar_grey = FALSE;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_BORDER:
	    sb->border = (int) *argv++;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_BAR_DISPLAY_LEVEL:
	    sb->bar_display_level = (Scrollbar_setting) *argv++;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_BUBBLE_DISPLAY_LEVEL:
	    sb->bubble_display_level = (Scrollbar_setting) *argv++;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_DIRECTION:
	    if ((Scrollbar_setting) *argv++ == SCROLL_HORIZONTAL) {
	       sb->horizontal    = TRUE;
	       sb->active_cursor = sb->gravity == SCROLL_NORTH ?
	                           &scrollbar_h_north_active_cursor : 
				   &scrollbar_h_south_active_cursor ;
	    } else {
	       sb->horizontal = FALSE;
	       sb->active_cursor = sb->gravity == SCROLL_WEST ?
	                           &scrollbar_v_west_active_cursor : 
				   &scrollbar_v_east_active_cursor ;
	    }
	    direction_set = TRUE;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_FORWARD_CURSOR:
	    sb->forward_cursor = (cursor_handle) (LINT_CAST(*argv++));
	    forward_cursor_set = TRUE;
	    break;

	 case SCROLL_BACKWARD_CURSOR:
	    sb->backward_cursor = (cursor_handle) (LINT_CAST(*argv++));
	    backward_cursor_set = TRUE;
	    break;

	 case SCROLL_ABSOLUTE_CURSOR:
	    sb->absolute_cursor = (cursor_handle) (LINT_CAST(*argv++));
	    absolute_cursor_set = TRUE;
	    break;

	 case SCROLL_ACTIVE_CURSOR:
	    sb->active_cursor = (cursor_handle) (LINT_CAST(*argv++));
	    active_cursor_set = TRUE;
	    break;

         case SCROLL_NORMALIZE:
            if (*argv++)
	       sb->normalize = TRUE;
            else
	       sb->normalize = FALSE;
	    sb->attrs_modified = TRUE;
            break;

         case SCROLL_MARGIN:
            sb->normalize_margin = (int) *argv++;
	    sb->attrs_modified = TRUE;
            break;

         case SCROLL_TO_GRID:
            if (*argv++)
	       sb->use_grid = TRUE;
            else
	       sb->use_grid = FALSE;
	    sb->attrs_modified = TRUE;
            break;

         case SCROLL_GAP:
            sb->gap = (int) *argv++;
	    sb->attrs_modified = TRUE;
            break;

	 case SCROLL_PAGE_BUTTONS:
            if (*argv++)
	       sb->buttons = TRUE;
            else
	       sb->buttons = FALSE;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_PAGE_BUTTON_LENGTH:
	    sb->button_length = (unsigned) *argv++;
	    sb->saved_button_length = sb->button_length;
	    sb->attrs_modified = TRUE;
	    break;

	 case SCROLL_LINE_HEIGHT:
	    sb->line_height = (unsigned) *argv++;
	    break;

	 case SCROLL_MARK:
	    sb->undo_mark = (long unsigned) *argv++;
	    break;

	 case SCROLL_ADVANCED_MODE:
            if (*argv++)
	       sb->advanced_mode = TRUE;
            else
	       sb->advanced_mode = FALSE;
	    break;

	 case SCROLL_VIEW_START:
	    if ((sb->view_start = (unsigned) *argv++) < 0)
	       sb->view_start = 0;
	    break;

	 case SCROLL_VIEW_LENGTH:
	    sb->view_length = (unsigned) *argv++;
	    break;

	 case SCROLL_OBJECT_LENGTH:
	    if ((sb->object_length = (unsigned) *argv++) <= 0)
	       sb->object_length = SCROLLBAR_INVALID_LENGTH;
	    break;

	 case SCROLL_REPEAT_TIME:
	    sb->delay = (long unsigned) *argv++;
	    break;

	 case SCROLL_PAINT_BUTTONS_PROC:
	    sb->paint_buttons_proc = (int (*)()) (LINT_CAST(*argv++));
	    if (!sb->paint_buttons_proc)
	       sb->paint_buttons_proc = scrollbar_paint_buttons;
	    break;

         case SCROLL_REQUEST_OFFSET:
	    sb->request_offset = (long unsigned) *argv++;
	    break;

         case SCROLL_REQUEST_MOTION:
	    sb->request_motion = (Scroll_motion) *argv++;
	    break;

         case SCROLL_END_POINT_AREA:
	    sb->end_point_area = (long unsigned) *argv++;
	    break;

	 case HELP_DATA:
	    sb->help_data = *argv++;
	    break;

	 default:
	    argv = attr_skip(attr, argv);
      }
   }   

   /* set thickness to the appropriate dimension */
   if (thickness_set) {
      if (!sb->horizontal)
	 sb->rect.r_width = thickness;
      else
	 sb->rect.r_height = thickness;
   }

   /* if the notify client has been set, and is not null, get the  */
   /* windowfd for the client and compute new scrollbar rect.      */
   if (client_set && sb->notify_client) {
      sb->client_windowfd = win_get_fd(sb->notify_client);
      if (sb->rect_fixed) {
      /*
       *  If user only set one of the dimension, then default the other.
       */
         Rect   temp_sb_rect;
         if (sb->horizontal) {
            if (sb->rect.r_width == 0) {
               (void)win_getsize(sb->client_windowfd, &temp_sb_rect);
               sb->rect.r_width = temp_sb_rect.r_width;
               }
	 } else {
	    if (sb->rect.r_height == 0) {
	       (void)win_getsize(sb->client_windowfd, &temp_sb_rect);
	       sb->rect.r_height = temp_sb_rect.r_height;
               }
	 }
      } else {
         (void)win_getsize(sb->client_windowfd, &sb->rect);
	 if (sb->horizontal)
	    sb->rect.r_height = defaults_thickness;
	 else
	    sb->rect.r_width  = defaults_thickness;

      }
   }

   /* gravity adjustments... */
   /* if direction is set for first time, use user's preferred gravity */
   if (sb->direction_not_yet_set && direction_set && sb->gravity_not_yet_set) {
      defaults_gravity           = sb->horizontal ? 
				  defaults_h_gravity : defaults_v_gravity;
      sb->gravity               = defaults_gravity;
      sb->gravity_not_yet_set   = FALSE;
      sb->direction_not_yet_set = FALSE;
   }
   if ((sb->pixwin || client_set) && sb->gravity_not_yet_set == FALSE) {
      (void)win_getsize(sb->client_windowfd,&rect);
      if (sb->gravity == SCROLL_MAX) {
	 if (sb->horizontal) {
	    sb->rect.r_top = rect.r_top + rect.r_height - sb->rect.r_height;
	    if (!active_cursor_set)
	       sb->active_cursor = &scrollbar_h_south_active_cursor ;
	 } else {
	    sb->rect.r_left = rect.r_left + rect.r_width - sb->rect.r_width;
	    if (!active_cursor_set)
	       sb->active_cursor = &scrollbar_v_east_active_cursor ;
	 }
      } else {
	 if (sb->horizontal) {
	    sb->rect.r_top = rect.r_top;
	    if (!active_cursor_set)
	       sb->active_cursor = &scrollbar_h_north_active_cursor ;
	 } else {
	    sb->rect.r_left = rect.r_left;
	    if (!active_cursor_set)
	       sb->active_cursor = &scrollbar_v_west_active_cursor ;
	 }
      }
   }

   /* fix the region to reflect the current client and rect. */
   if (client_set || rect_set)
      scrollbar_fix_region(sb);

   /* find out if bubble modified */
   (void)scrollbar_compute_bubble_rect(sb, &new_bubble);
   if (!rect_equal(&new_bubble, &old_bubble)) 
      sb->bubble_modified = TRUE;

   /* page button stuff... */
   /* if bar is too short to show both buttons, default to no buttons. */
   /* if user requests buttons and bar is too short for both, make button */
   /* cover the whole bar by default. */
   sb_major_axis = sb->horizontal ? sb->rect.r_width : sb->rect.r_height;
   sb_is_tiny = sb_major_axis <= (2 * sb->saved_button_length);
   if (!sb->buttons)
      sb->button_length = 0;
   else
      sb->button_length = sb_is_tiny ? sb_major_axis : sb->saved_button_length;
   sb->one_button_case = (sb->buttons && sb_is_tiny);

   /* set the cursors if direction set */
   if (direction_set) {
      if (!forward_cursor_set)
	 sb->forward_cursor = (!sb->horizontal) ? 
			       &scrollbar_up_cursor : 
			       &scrollbar_left_cursor;
      if (!backward_cursor_set)
	 sb->backward_cursor = (!sb->horizontal) ? 
			       &scrollbar_down_cursor : 
			       &scrollbar_right_cursor;
      if (!absolute_cursor_set)
	 sb->absolute_cursor = (!sb->horizontal) ? 
			       &scrollbar_thumbv_cursor : 
			       &scrollbar_thumbh_cursor;
   }
}

/**************************************************************************/
/* scrollbar_fix_region                                                   */
/* If a region exists, close it, and unregister the scrollbar as a notify */
/* client.  Then, if the scrollbar has a client, open a region on the     */
/* client's pixwin reflecting the scrollbar's current rect.               */
/**************************************************************************/
#ifdef notdef
static
scrollbar_fix_region(sb) 
scrollbar_handle sb;
{
   struct pixwin *client_pw;

   /* if a region has been opened, close it and unregister sb. */
   if (sb->pixwin) {
      (void)pw_close(sb->pixwin);
      sb->pixwin = 0;
      (void)win_unregister(sb);  
   }

   /* if there is a client, open a region reflecting the current rect */
   if (sb->notify_client) {
      /*####ACG: pw_open => pw_open_sb */
      if (!(client_pw  = (struct pixwin *)pw_open_sb(sb->client_windowfd)))
          return;
      sb->pixwin = pw_region(client_pw, sb->rect.r_left, sb->rect.r_top,
			     sb->rect.r_width, sb->rect.r_height);
      (void)win_register(sb, sb->pixwin, scrollbar_event, scrollbar_destroy, 0);  
   }
}
#endif

static
scrollbar_fix_region(sb)
    scrollbar_handle sb;
{
    struct pixwin *client_pw;

    if (sb->notify_client) {
	if (sb->pixwin) {
	    (void)pw_set_region_rect(sb->pixwin, &sb->rect, TRUE);
	} else {
	    /*####ACG: pw_open => pw_open_sb */
            if (!(client_pw = (struct pixwin *)pw_open_sb(sb->client_windowfd)))
	        return;
	    sb->pixwin = pw_region(client_pw,
				   sb->rect.r_left,
				   sb->rect.r_top,
 				   sb->rect.r_width,
				   sb->rect.r_height);
	    (void)pw_close(client_pw);

	    if (pr_get_plane_group(sb->pixwin->pw_pixrect) == PIXPG_24BIT_COLOR)
		pw_use_color24(sb->pixwin);

	    (void)win_register((Notify_client)(LINT_CAST(sb)), sb->pixwin,
		         scrollbar_event, (Notify_value (*) ())(LINT_CAST(
			 scrollbar_destroy)), 0);
	}
    } else {
	if (sb->pixwin) {
	    (void)pw_close(sb->pixwin);
	    sb->pixwin = 0;
	    (void)win_unregister((Notify_client)(LINT_CAST(sb)));
	}
    }
}
/**************************************************************************/
/* scrollbar_get                                                          */
/**************************************************************************/

caddr_t
scrollbar_get(sb_client, attr)
Scrollbar sb_client;
Scrollbar_attribute attr;
{
   scrollbar_handle sb;

   sb = (scrollbar_handle)(LINT_CAST(sb_client));
   if (!sb)
      return (caddr_t) 0;

/* Return default values via special generic sb for boundary layout. */
   if (sb == (scrollbar_handle) SCROLLBAR) {
      switch (attr) {
         case SCROLL_THICKNESS:
         case SCROLL_WIDTH:
	    return (caddr_t)
	       defaults_get_integer_check("/Scrollbar/Thickness",
			   SCROLL_DEFAULT_WIDTH, 0, 200, (int *)NULL); 
         default:
	    return (caddr_t) 0;
      }
   }

   switch (attr) {

      case SCROLL_NOTIFY_CLIENT:
	 return (caddr_t) sb->notify_client; 

      case SCROLL_OBJECT:
	 return (caddr_t) sb->object; 

      case SCROLL_PIXWIN:
	 return (caddr_t) sb->pixwin; 

      case SCROLL_PLACEMENT:
	 return (caddr_t) sb->gravity;

      case SCROLL_RECT:
	 return (caddr_t) &sb->rect; 

      case SCROLL_TOP:
	 return (caddr_t) sb->rect.r_top; 

      case SCROLL_LEFT:
	 return (caddr_t) sb->rect.r_left; 

      case SCROLL_WIDTH:
	 return (caddr_t) sb->rect.r_width; 

      case SCROLL_HEIGHT:
	 return (caddr_t) sb->rect.r_height; 

      case SCROLL_THICKNESS:
	 if (!sb->horizontal)
	    return (caddr_t) sb->rect.r_width; 
         else
	    return (caddr_t) sb->rect.r_height; 

      case SCROLL_BUBBLE_MARGIN:
	 return (caddr_t) sb->bubble_margin; 

      case SCROLL_BUBBLE_COLOR:
	 if (sb->bubble_grey)
	    return (caddr_t) SCROLL_GREY;
         else
	    return (caddr_t) SCROLL_BLACK;

      case SCROLL_BAR_COLOR:
	 if (sb->bar_grey)
	    return (caddr_t) SCROLL_GREY;
         else
	    return (caddr_t) SCROLL_WHITE;

      case SCROLL_BORDER:
	 return (caddr_t) sb->border;

      case SCROLL_PAGE_BUTTONS:
	 return (caddr_t) sb->buttons; 

      case SCROLL_PAGE_BUTTON_LENGTH:
	 return (caddr_t) sb->button_length; 

      case SCROLL_BAR_DISPLAY_LEVEL:
	 return (caddr_t) sb->bar_display_level;

      case SCROLL_BUBBLE_DISPLAY_LEVEL:
	 return (caddr_t) sb->bar_display_level;

      case SCROLL_DIRECTION:
	 if (!sb->horizontal)
	    return (caddr_t) SCROLL_VERTICAL; 
         else
	    return (caddr_t) SCROLL_HORIZONTAL; 

      case SCROLL_FORWARD_CURSOR:
	 return (caddr_t) sb->forward_cursor;

      case SCROLL_BACKWARD_CURSOR:
	 return (caddr_t) sb->backward_cursor;

      case SCROLL_ABSOLUTE_CURSOR:
	 return (caddr_t) sb->absolute_cursor;

      case SCROLL_ACTIVE_CURSOR:
	 return (caddr_t) sb->active_cursor;

      case SCROLL_NORMALIZE:
	 if (sb->normalize)
	    return (caddr_t) TRUE; 
         else
	    return (caddr_t) FALSE; 

      case SCROLL_MARGIN:
	 return (caddr_t) sb->normalize_margin;

      case SCROLL_LINE_HEIGHT:
	 return (caddr_t) sb->line_height;

      case SCROLL_TO_GRID:
	 if (sb->use_grid)
	    return (caddr_t) TRUE; 
         else
	    return (caddr_t) FALSE; 

      case SCROLL_GAP:
	 if (sb->gap==SCROLLBAR_INVALID_LENGTH)
	    return (caddr_t) sb->normalize_margin;
	 else
	    return (caddr_t) sb->gap;

     case SCROLL_MARK:
	 return (caddr_t) sb->undo_mark;

      case SCROLL_ADVANCED_MODE:
	 if (sb->advanced_mode)
	    return (caddr_t) TRUE; 
         else
	    return (caddr_t) FALSE; 
	 break;

      case SCROLL_LAST_VIEW_START:
	 return (caddr_t) sb->old_view_start;

      case SCROLL_VIEW_START:
	 return (caddr_t) sb->view_start;

      case SCROLL_VIEW_LENGTH:
	 return (caddr_t) sb->view_length;

      case SCROLL_OBJECT_LENGTH:
	 return (caddr_t) sb->object_length;

      case SCROLL_REPEAT_TIME:
	 return (caddr_t) sb->delay;

      case SCROLL_PAINT_BUTTONS_PROC:
	 return (caddr_t) (LINT_CAST(sb->paint_buttons_proc));

      case SCROLL_MODIFY_PROC:
	 return (caddr_t) (LINT_CAST(sb->modify));

      case SCROLL_REQUEST_OFFSET:
	 return (caddr_t) sb->request_offset;

      case SCROLL_REQUEST_MOTION:
	 return (caddr_t) sb->request_motion;

      case SCROLL_END_POINT_AREA:
	 return (caddr_t) sb->end_point_area;

      case HELP_DATA:
	 return (caddr_t) sb->help_data; 

      default:
	 return (caddr_t) 0;
   }
   /*NOTREACHED*/
}

/**************************************************************************/
/* scrollbar_destroy*                                                     */
/**************************************************************************/

int
scrollbar_destroy(sb_client)
Scrollbar sb_client;
{
      scrollbar_handle sb;

      sb = (scrollbar_handle) (LINT_CAST(sb_client));
      scrollbar_free(sb);
      (void)win_unregister((Notify_client)(LINT_CAST(sb)));
      return 0;
}

/**************************************************************************/
/* scrollbar_free                                                         */
/* remove sb from the list of scrollbars for the pixwin, then free it.    */
/**************************************************************************/

static
scrollbar_free(sb)
scrollbar_handle sb;
{
   register scrollbar_handle	prev;

   if (scrollbar_active_sb == sb) 
      scrollbar_active_sb = NULL;

   if (sb == scrollbar_head_sb)
      scrollbar_head_sb = sb->next;
   else
      for (prev = scrollbar_head_sb; prev; prev = prev->next) {
	  if (sb == prev->next) {
	      prev->next = sb->next;
	      break;
	  }
      }
   free((char *)(LINT_CAST(sb)));
}


void
scrollbar_scroll_to(sb_client, new_view_start)
register Scrollbar	sb_client;
long			new_view_start;
{
    scrollbar_handle sb;

    sb = (scrollbar_handle) (LINT_CAST(sb_client));
    sb->old_view_start = sb->view_start;
    sb->view_start = new_view_start;
    if (sb->view_start != sb->old_view_start)
      sb->bubble_modified = TRUE;

    (void)win_post_id_and_arg(sb->notify_client, SCROLL_REQUEST, NOTIFY_SAFE, 
	(char *)sb, NOTIFY_COPY_NULL, NOTIFY_RELEASE_NULL);
}
